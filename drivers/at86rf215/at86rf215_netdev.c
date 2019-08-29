/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_at86rf215
 * @{
 *
 * @file
 * @brief       Netdev adaption for the AT86RF215 driver
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <strings.h>

#include "iolist.h"

#include "net/eui64.h"
#include "net/ieee802154.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/netif/internal.h"

#include "at86rf215.h"
#include "at86rf215_netdev.h"
#include "at86rf215_internal.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

const netdev_driver_t at86rf215_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};

/* executed in the GPIO ISR context */
static void _irq_handler(void *arg)
{
    netdev_t *netdev = (netdev_t *) arg;

    netdev->event_callback(netdev, NETDEV_EVENT_ISR);
}

static int _init(netdev_t *netdev)
{
    at86rf215_t *dev = (at86rf215_t *)netdev;

    /* don't call HW init for both radios */
    if (is_subGHz(dev) || dev->sibling == NULL) {
        /* initialize GPIOs */
        spi_init_cs(dev->params.spi, dev->params.cs_pin);
        gpio_init(dev->params.reset_pin, GPIO_OUT);
        gpio_set(dev->params.reset_pin);
        gpio_init_int(dev->params.int_pin, GPIO_IN, GPIO_RISING, _irq_handler, dev);
    }

    /* test if the SPI is set up correctly and the device is responding */
    uint8_t tries = 5;
    while (tries--) {
        uint8_t pn = at86rf215_reg_read(dev, RG_RF_PN);
        if ((pn == AT86RF215_PN) || (pn == AT86RF215M_PN)) {
            break;
        }

        xtimer_usleep(1000);
    }

    if (tries == 0) {
        DEBUG("[at86rf215] error: unable to read correct part number\n");
        return -1;
    }

    /* reset device to default values and put it into RX state */
    at86rf215_reset(dev);
    dev->state = AT86RF215_STATE_IDLE;

    return 0;
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    at86rf215_t *dev = (at86rf215_t *)netdev;
    size_t len = 0;

    at86rf215_tx_prepare(dev);

    /* load packet data into FIFO */
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {

        /* current packet data + FCS too long */
        if ((len + iol->iol_len + IEEE802154_FCS_LEN) > AT86RF215_MAX_PKT_LENGTH) {
            DEBUG("[at86rf215] error: packet too large (%u byte) to be send\n",
                  (unsigned)len + IEEE802154_FCS_LEN);
            return -EOVERFLOW;
        }

        if (iol->iol_len) {
            len = at86rf215_tx_load(dev, iol->iol_base, iol->iol_len, len);
        }
    }

    /* send data out directly if pre-loading id disabled */
    if (!(dev->flags & AT86RF215_OPT_PRELOADING)) {
        at86rf215_tx_exec(dev);
    }

    /* return the number of bytes that were actually loaded into the frame
     * buffer/send out */
    return (int)len;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    at86rf215_t *dev = (at86rf215_t *)netdev;
    int16_t pkt_len;

    /* get the size of the received packet */
    at86rf215_reg_read_bytes(dev, dev->BBC->RG_RXFLL, &pkt_len, sizeof(pkt_len));

    /* substract length of FCS field */
    pkt_len = (pkt_len & 0x7ff) - IEEE802154_FCS_LEN;

    /* just return length when buf == NULL */
    if (buf == NULL) {
        goto out;
    }

    /* not enough space in buf */
    if (pkt_len > (int) len) {
        pkt_len = -ENOBUFS;
        goto out;
    }

    /* copy payload */
    at86rf215_reg_read_bytes(dev, dev->BBC->RG_FBRXS, buf, pkt_len);

    if (info != NULL) {
        netdev_ieee802154_rx_info_t *radio_info = info;
        radio_info->rssi = (int8_t) at86rf215_reg_read(dev, dev->RF->RG_EDV);
    }

out:
    at86rf215_rf_cmd(dev, CMD_RF_RX);

    dev->state = AT86RF215_STATE_IDLE;

    return pkt_len;
}

static int _set_state(at86rf215_t *dev, netopt_state_t state)
{
    switch (state) {
        case NETOPT_STATE_STANDBY:
            at86rf215_set_state(dev, CMD_RF_TRXOFF);
            break;
        case NETOPT_STATE_SLEEP:
            at86rf215_set_state(dev, CMD_RF_SLEEP);
            break;
        case NETOPT_STATE_IDLE:
            break;
        case NETOPT_STATE_TX:
            if (dev->flags & AT86RF215_OPT_PRELOADING) {
                at86rf215_tx_exec(dev);
            }
            break;
        case NETOPT_STATE_RESET:
            at86rf215_reset(dev);
            break;
        default:
            return -ENOTSUP;
    }
    return sizeof(netopt_state_t);
}

netopt_state_t _get_state(at86rf215_t *dev)
{
    switch (dev->state) {
        case AT86RF215_STATE_SLEEP:
            return NETOPT_STATE_SLEEP;
        case AT86RF215_STATE_RX:
            return NETOPT_STATE_RX;
        case AT86RF215_STATE_TX:
        case AT86RF215_STATE_TX_PREP:
            return NETOPT_STATE_TX;
        case AT86RF215_STATE_OFF:
            return NETOPT_STATE_OFF;
        case AT86RF215_STATE_IDLE:
        default:
            return NETOPT_STATE_IDLE;
    }
}

static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{
    at86rf215_t *dev = (at86rf215_t *) netdev;

    if (netdev == NULL) {
        return -ENODEV;
    }

    /* getting these options doesn't require the transceiver to be responsive */
    switch (opt) {
        case NETOPT_STATE:
            assert(max_len >= sizeof(netopt_state_t));
            *((netopt_state_t *)val) = _get_state(dev);
            return sizeof(netopt_state_t);

        case NETOPT_PRELOADING:
            if (dev->flags & AT86RF215_OPT_PRELOADING) {
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            return sizeof(netopt_enable_t);

        case NETOPT_PROMISCUOUSMODE:
            if (dev->flags & AT86RF215_OPT_PROMISCUOUS) {
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            return sizeof(netopt_enable_t);

        case NETOPT_RX_START_IRQ:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_TELL_RX_START);
            return sizeof(netopt_enable_t);

        case NETOPT_RX_END_IRQ:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_TELL_RX_END);
            return sizeof(netopt_enable_t);

        case NETOPT_TX_START_IRQ:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_TELL_TX_START);
            return sizeof(netopt_enable_t);

        case NETOPT_TX_END_IRQ:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_TELL_TX_END);
            return sizeof(netopt_enable_t);

        case NETOPT_CSMA:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_CSMA);
            return sizeof(netopt_enable_t);

        case NETOPT_TX_RETRIES_NEEDED:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t *)val) = dev->retries_max - dev->retries;
            return sizeof(uint8_t);

        default:
            /* Can still be handled in second switch */
            break;
    }

    int res;

    if (((res = netdev_ieee802154_get((netdev_ieee802154_t *)netdev, opt, val,
                                      max_len)) >= 0) || (res != -ENOTSUP)) {
        return res;
    }

//    uint8_t old_state = at86rf215_get_rf_state(dev);

    /* temporarily wake up if sleeping */
//    if (old_state == AT86RF215_STATE_SLEEP) {
//        at86rf215_assert_awake(dev);
//    }

    /* these options require the transceiver to be not sleeping*/
    switch (opt) {
        case NETOPT_TX_POWER:
            assert(max_len >= sizeof(int16_t));
            *((uint16_t *)val) = at86rf215_get_txpower(dev);
            res = sizeof(uint16_t);
            break;

        case NETOPT_RETRANS:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t *)val) = dev->retries_max;
            res = sizeof(uint8_t);
            break;

        case NETOPT_CSMA_RETRIES:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t *)val) = dev->csma_retries_max;
            res = sizeof(uint8_t);
            break;

        case NETOPT_CCA_THRESHOLD:
            assert(max_len >= sizeof(int8_t));
            *((int8_t *)val) = at86rf215_get_cca_threshold(dev);
            res = sizeof(int8_t);
            break;

        case NETOPT_IS_CHANNEL_CLR:
            assert(max_len >= sizeof(netopt_enable_t));
            *((netopt_enable_t *)val) = at86rf215_cca(dev);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_LAST_ED_LEVEL:
            assert(max_len >= sizeof(int8_t));
            *((int8_t *)val) = at86rf215_get_ed_level(dev);
            res = sizeof(int8_t);
            break;

        case NETOPT_AUTOACK:
            *((netopt_enable_t *)val) =
                !!(dev->flags & AT86RF215_OPT_AUTOACK);
            res = sizeof(netopt_enable_t);
            break;

        default:
            res = -ENOTSUP;
            break;
    }

    /* go back to sleep if were sleeping */
//    if (old_state == AT86RF215_STATE_SLEEP) {
//        at86rf215_set_state(dev, AT86RF215_STATE_SLEEP);
//    }

    return res;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len)
{
    at86rf215_t *dev = (at86rf215_t *) netdev;
    int res = -ENOTSUP;

    if (dev == NULL) {
        return -ENODEV;
    }

    /* temporarily wake up if sleeping and opt != NETOPT_STATE.
     * opt != NETOPT_STATE check prevents redundant wake-up.
     * when opt == NETOPT_STATE, at86rf215_set_state() will wake up the
     * radio if needed. */
//    if ((old_state == AT86RF215_STATE_SLEEP) && (opt != NETOPT_STATE)) {
//        at86rf215_assert_awake(dev);
//    }

    switch (opt) {
        case NETOPT_ADDRESS:
            assert(len <= sizeof(uint16_t));
            at86rf215_set_addr_short(dev, *((const uint16_t *)val));
            /* don't set res to set netdev_ieee802154_t::short_addr */
            break;
        case NETOPT_ADDRESS_LONG:
            assert(len <= sizeof(uint64_t));
            at86rf215_set_addr_long(dev, *((const uint64_t *)val));
            /* don't set res to set netdev_ieee802154_t::long_addr */
            break;
        case NETOPT_NID:
            assert(len <= sizeof(uint16_t));
            at86rf215_set_pan(dev, *((const uint16_t *)val));
            /* don't set res to set netdev_ieee802154_t::pan */
            break;
        case NETOPT_CHANNEL:
            assert(len == sizeof(uint16_t));
            uint16_t chan = *((const uint16_t *)val);

            if (is_subGHz(dev)) {
                if (chan > dev->num_chans) {
                    res = -EINVAL;
                    break;
                }
            } else {
                if (chan < IEEE802154_CHANNEL_MIN ||
                    chan > IEEE802154_CHANNEL_MAX + dev->num_chans) {
                    res = -EINVAL;
                    break;
                }
            }

            at86rf215_set_chan(dev, chan);
            /* don't set res to set netdev_ieee802154_t::chan */
            break;

        case NETOPT_CHANNEL_PAGE:
            assert(len == sizeof(uint16_t));
            uint8_t page = (((const uint16_t *)val)[0]) & UINT8_MAX;

            at86rf215_set_page(dev, page);
            res = sizeof(uint16_t);
            break;

        case NETOPT_TX_POWER:
            assert(len <= sizeof(int16_t));
            at86rf215_set_txpower(dev, *((const int16_t *)val));
            res = sizeof(uint16_t);
            break;

        case NETOPT_STATE:
            assert(len <= sizeof(netopt_state_t));
            res = _set_state(dev, *((const netopt_state_t *)val));
            break;

        case NETOPT_AUTOACK:
            at86rf215_set_option(dev, AT86RF215_OPT_AUTOACK,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_RETRANS:
            assert(len <= sizeof(uint8_t));
            dev->retries_max =  *((const uint8_t *)val);
            res = sizeof(uint8_t);
            break;

        case NETOPT_PRELOADING:
            at86rf215_set_option(dev, AT86RF215_OPT_PRELOADING,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_PROMISCUOUSMODE:
            at86rf215_set_option(dev, AT86RF215_OPT_PROMISCUOUS,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_RX_START_IRQ:
            at86rf215_set_option(dev, AT86RF215_OPT_TELL_RX_START,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_RX_END_IRQ:
            at86rf215_set_option(dev, AT86RF215_OPT_TELL_RX_END,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_START_IRQ:
            at86rf215_set_option(dev, AT86RF215_OPT_TELL_TX_START,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_END_IRQ:
            at86rf215_set_option(dev, AT86RF215_OPT_TELL_TX_END,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_CSMA:
            at86rf215_set_option(dev, AT86RF215_OPT_CSMA,
                                 ((const bool *)val)[0]);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_CSMA_RETRIES:
            assert(len <= sizeof(uint8_t));
            dev->csma_retries_max = *((const uint8_t *)val);
            res = sizeof(uint8_t);
            break;

        case NETOPT_CCA_THRESHOLD:
            assert(len <= sizeof(int8_t));
            at86rf215_set_cca_threshold(dev, *((const int8_t *)val));
            res = sizeof(int8_t);
            break;

        default:
            break;
    }

    /* go back to sleep if were sleeping and state hasn't been changed */
//    if ((old_state == AT86RF215_STATE_SLEEP)
//        && (opt != NETOPT_STATE)) {
//        at86rf215_set_state(dev, AT86RF215_STATE_SLEEP);
//    }

    if (res == -ENOTSUP) {
        res = netdev_ieee802154_set((netdev_ieee802154_t *)netdev, opt, val, len);
    }

    return res;
}

static void _tx_end(at86rf215_t *dev)
{
    netdev_t *netdev = (netdev_t *)dev;

    /* listen to non-ACK packets again */
    if (dev->flags & AT86RF215_OPT_ACK_REQUESTED) {
        dev->flags &= ~AT86RF215_OPT_ACK_REQUESTED;
        at86rf215_filter_ack(dev, false);
    }

    at86rf215_tx_done(dev);

    if (dev->flags & AT86RF215_OPT_TELL_TX_END) {
        netdev->event_callback(netdev, NETDEV_EVENT_TX_COMPLETE);
    }

    dev->state = AT86RF215_STATE_IDLE;
}

static void _ack_timeout_cb(void* arg) {
    at86rf215_t *dev = arg;
    dev->ack_timeout = true;
    msg_send_int(&dev->ack_msg, dev->ack_msg.sender_pid);
}

/* wake up the radio thread when on ACK timeout */
static void _start_ack_timer(at86rf215_t *dev)
{
    dev->ack_msg.type = NETDEV_MSG_TYPE_EVENT;
    dev->ack_msg.sender_pid = thread_getpid();

    dev->ack_timer.arg = dev;
    dev->ack_timer.callback = _ack_timeout_cb;

    xtimer_set(&dev->ack_timer, dev->ack_timeout_usec);
}

static inline bool _ack_frame_received(at86rf215_t *dev)
{
    /* check if the sequence number matches */
    return at86rf215_reg_read(dev, dev->BBC->RG_FBRXS + 2)
        == at86rf215_reg_read(dev, dev->BBC->RG_FBTXS + 2);
}

static void _handle_ack_timeout(at86rf215_t *dev)
{
    if (dev->retries) {
        dev->csma_retries = dev->csma_retries_max;
        /* start energy measurement - will trigger TX again */
        at86rf215_reg_write(dev, dev->RF->RG_EDC, 1);
    } else {
        /* no retransmissions left */
        _tx_end(dev);
    }
}

/* clear the other IRQ if the sibling is not ready yet */
static inline void _clear_sibling_irq(at86rf215_t *dev) {
    if (is_subGHz(dev)) {
        at86rf215_reg_read(dev, RG_RF09_IRQS);
        at86rf215_reg_read(dev, RG_BBC1_IRQS);
    } else {
        at86rf215_reg_read(dev, RG_RF24_IRQS);
        at86rf215_reg_read(dev, RG_BBC0_IRQS);
    }
}

/* executed in the radio thread */
static void _isr(netdev_t *netdev)
{
    at86rf215_t *dev = (at86rf215_t *) netdev;
    uint8_t bb_irq_mask, rf_irq_mask, amcs;

    rf_irq_mask = at86rf215_reg_read(dev, dev->RF->RG_IRQS);
    bb_irq_mask = at86rf215_reg_read(dev, dev->BBC->RG_IRQS);

    bool ack_timeout = dev->ack_timeout;
    if (ack_timeout) {
        dev->ack_timeout = false;
    }

    /* If the interrupt pin is still high, there was an IRQ on the other radio */
    if (gpio_read(dev->params.int_pin)) {
        if (dev->sibling && dev->sibling->state != AT86RF215_STATE_OFF) {
            netdev->event_callback((netdev_t *) dev->sibling, NETDEV_EVENT_ISR);
        } else {
            _clear_sibling_irq(dev);
        }
    }

    /* exit early if the interrupt was not for this interface */
    if (!((bb_irq_mask & (BB_IRQ_RXFE | BB_IRQ_TXFE | BB_IRQ_RXAM)) |
          (rf_irq_mask & RF_IRQ_EDC))) {

        /* check if we did get here because of ACK timeout */
        if (ack_timeout) {
            _handle_ack_timeout(dev);
        }

        return;
    }

    amcs = at86rf215_reg_read(dev, dev->BBC->RG_AMCS);

    /* Energy Detection Complete */
    if ((rf_irq_mask & RF_IRQ_EDC) &&
         dev->state == AT86RF215_STATE_TX) {

        /* if the channel is clear, do TX */
        if (!(amcs & AMCS_CCAED_MASK)) {
            at86rf215_enable_baseband(dev);
            at86rf215_set_state(dev, CMD_RF_TX);
        } else if (dev->csma_retries) {
            --dev->csma_retries;
            /* re-start energy detection */
            at86rf215_reg_write(dev, dev->RF->RG_EDC, 1);
        } else {
            /* channel busy and no retries left */
            at86rf215_enable_baseband(dev);
            at86rf215_tx_done(dev);

            dev->state = AT86RF215_STATE_IDLE;
            /* radio is still in RX mode */
        }
    }

    /* Address match */
    if (bb_irq_mask & BB_IRQ_RXAM) {
        if (dev->flags & AT86RF215_OPT_TELL_RX_START) {
            netdev->event_callback(netdev, NETDEV_EVENT_RX_STARTED);
        }
    }

    /* End of Receive */
    if (bb_irq_mask & BB_IRQ_RXFE) {

        switch (dev->state) {
        /* check if we received a pending ACK */
        case AT86RF215_STATE_TX:
            if ((dev->flags & AT86RF215_OPT_ACK_REQUESTED) &&
                _ack_frame_received(dev)) {

                ack_timeout = false;
                xtimer_remove(&dev->ack_timer);

                _tx_end(dev);

                at86rf215_rf_cmd(dev, CMD_RF_RX);

            } else {
                /* we got an ACK with the wrong sequence number */
                at86rf215_rf_cmd(dev, CMD_RF_RX);
            }

            break;
        default:
            printf("RX while  %s!\n", rf215_state(dev->state));
            /* fall-through */
        case AT86RF215_STATE_IDLE:
            dev->state = AT86RF215_STATE_RX;
            /* RX done, not sending ACK */
            if (!(amcs & AMCS_AACKFT_MASK) ||
                !(dev->flags & AT86RF215_OPT_AUTOACK)) {

                if (dev->flags & AT86RF215_OPT_TELL_RX_END) {
                    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
                }

                at86rf215_rf_cmd(dev, CMD_RF_RX);

                dev->state = AT86RF215_STATE_IDLE;
            }
        }
    }

    /* End of Transmit */
    if (bb_irq_mask & BB_IRQ_TXFE) {

        switch (dev->state) {
        case AT86RF215_STATE_TX:

            /* only consider TX done when ACK has been received */
            if (dev->flags & AT86RF215_OPT_ACK_REQUESTED) {
                if (dev->retries) {
                    --dev->retries;
                    _start_ack_timer(dev);
                }
            } else {
                _tx_end(dev);
            }

            break;
        case AT86RF215_STATE_RX:
            /* only consider RX done when ACK has been sent */
            if (dev->flags & AT86RF215_OPT_TELL_RX_END) {
                netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
            }
            break;
        }
    }

    if (ack_timeout) {
        _handle_ack_timeout(dev);
    }
}
