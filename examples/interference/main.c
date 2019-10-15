/*
 * Copyright (C) 2019 Robbe Elsas <robbe.elsas@ugent.be>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Application used for interference testing
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "thread.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "interference_types.h"
#include "interference_constants.h"

#include "periph/gpio.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

kernel_pid_t pid;
static char stack[THREAD_STACKSIZE_MAIN];
static gnrc_netif_t *netif = NULL;

#ifdef MODULE_AT86RF215
static if_tx_t tx_sub_ghz = TX_40B_SUB_GHZ;
static phy_cfg_idx_sub_ghz_t current_phy_cfg_idx_sub_ghz;
#if (GNRC_NETIF_NUMOF >= 2)
static if_tx_t tx_2_4_ghz = TX_40B_2_4_GHZ;
static phy_cfg_idx_2_4_ghz_t current_phy_cfg_idx_2_4_ghz;
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

static inline bool _is_iface(kernel_pid_t iface)
{
    return (gnrc_netif_get_by_pid(iface) != NULL);
}

#ifdef MODULE_AT86RF215
static uint8_t set_ofdm_opt(uint8_t iface, uint8_t opt) 
{
    if (gnrc_netapi_set(iface, NETOPT_OFDM_OPTION, 0, &opt, sizeof(opt)) < 0) {
        DEBUG("Unable to set OFDM option to %d\n", opt);
        return 1;
    }

    return 0;
}

static uint8_t set_ofdm_mcs(uint8_t iface, uint8_t mcs) 
{
    if (gnrc_netapi_set(iface, NETOPT_OFDM_MCS, 0, &mcs, sizeof(mcs)) < 0) {
        DEBUG("Unable to set OFDM scheme to %d\n", mcs);
        return 1;
    }

    return 0;
}

static uint8_t set_phy(if_phy_cfg_t *phy_cfg)
{
    int res = 0;
    uint8_t current_opt = 0;
    uint8_t next_opt = 0;

    if (!_is_iface(phy_cfg->iface)) {
        DEBUG("error: invalid interface given\n");
        return 1;
    }

    res = gnrc_netapi_set(phy_cfg->iface, NETOPT_IEEE802154_PHY, 0,
        &phy_cfg->ieee802154_phy, sizeof(phy_cfg->ieee802154_phy));
    if (res < 0) {
        DEBUG("Unable to set IEEE802154 PHY to %d\n", phy_cfg->ieee802154_phy);
        return 1;
    }

    switch (phy_cfg->ieee802154_phy) {
        case IEEE802154_PHY_FSK:
            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_SRATE, 0, 
                &phy_cfg->fsk_cfg.srate, sizeof(phy_cfg->fsk_cfg.srate));
            if (res < 0) {
                DEBUG("Unable to set FSK srate to %d\n", phy_cfg->fsk_cfg.srate);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_CHANNEL_SPACING, 0, 
                &phy_cfg->fsk_cfg.cspace, sizeof(phy_cfg->fsk_cfg.cspace));
            if (res < 0) {
                DEBUG("Unable to set FSK cspace to %d\n", phy_cfg->fsk_cfg.cspace);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_MODULATION_INDEX, 0, 
                &phy_cfg->fsk_cfg.mod_idx, sizeof(phy_cfg->fsk_cfg.mod_idx));
            if (res < 0) {
                DEBUG("Unable to set FSK modulation index to %d\n", phy_cfg->fsk_cfg.mod_idx);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_MODULATION_ORDER, 0, 
                &phy_cfg->fsk_cfg.mod_ord, sizeof(phy_cfg->fsk_cfg.mod_ord));
            if (res < 0) {
                DEBUG("Unable to set FSK modulation order to %d\n", phy_cfg->fsk_cfg.mod_ord);
                return 1;
            }
            break;
        
        case IEEE802154_PHY_OFDM:
            //TODO: more robust way of preventing impossible combos
            res = gnrc_netapi_get(phy_cfg->iface, NETOPT_OFDM_OPTION, 0, &current_opt, sizeof(current_opt));
            if (res < 0) {
                DEBUG("Unable to retrieve currently configured OFDM option\n");
                return 1;
            }

            next_opt = phy_cfg->ofdm_cfg.option;
            if (next_opt > current_opt) {
                set_ofdm_mcs(phy_cfg->iface, phy_cfg->ofdm_cfg.mcs);
                set_ofdm_opt(phy_cfg->iface, next_opt);
            } else {
                set_ofdm_opt(phy_cfg->iface, next_opt);
                set_ofdm_mcs(phy_cfg->iface, phy_cfg->ofdm_cfg.mcs);
            }
            break;

        default:
            DEBUG("PHY not supported\n");
            return 1;
    }

    return 0;
}

static uint8_t set_phy_sub_ghz(phy_cfg_idx_sub_ghz_t phy_cfg_idx)
{
    if (phy_cfg_idx >= NUM_PHY_CFG_SUB_GHZ) {
        DEBUG("Illegal PHY config index\n");
        return 1;
    }

    current_phy_cfg_idx_sub_ghz = phy_cfg_idx;
    if_phy_cfg_t new_phy_cfg = phy_cfg_sub_ghz[phy_cfg_idx];

    return set_phy(&new_phy_cfg);
}

static uint8_t init_phy_sub_ghz(void) 
{
    return set_phy_sub_ghz(PHY_CFG_SUB_GHZ_FSK_1);
}

static uint8_t next_phy_sub_ghz(void) 
{
    return set_phy_sub_ghz((current_phy_cfg_idx_sub_ghz + 1) % NUM_PHY_CFG_SUB_GHZ);
}

#if (GNRC_NETIF_NUMOF >= 2)
static uint8_t set_phy_2_4_ghz(phy_cfg_idx_2_4_ghz_t phy_cfg_idx)
{
    if (phy_cfg_idx >= NUM_PHY_CFG_2_4_GHZ) {
        DEBUG("Illegal PHY config index\n");
        return 1;
    }

    current_phy_cfg_idx_2_4_ghz = phy_cfg_idx;
    if_phy_cfg_t new_phy_cfg = phy_cfg_2_4_ghz[phy_cfg_idx];

    return set_phy(&new_phy_cfg);
}

static uint8_t init_phy_2_4_ghz(void) 
{
    return set_phy_2_4_ghz(PHY_CFG_2_4_GHZ_FSK_1);
}

static uint8_t next_phy_2_4_ghz(void) 
{
    return set_phy_2_4_ghz((current_phy_cfg_idx_2_4_ghz + 1) % NUM_PHY_CFG_2_4_GHZ);
}
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

static uint8_t send(kernel_pid_t iface, const char *dest, const char *payload) 
{
    size_t addr_len;
    uint8_t addr[GNRC_NETIF_L2ADDR_MAXLEN];
    gnrc_pktsnip_t *pkt, *hdr;
    gnrc_netif_hdr_t *nethdr;
    uint8_t flags = 0x00;

    if (!_is_iface(iface)) {
        DEBUG("error: invalid interface given\n");
        return 1;
    }

    /* parse address */
    addr_len = gnrc_netif_addr_from_str(dest, addr);

    if (addr_len == 0) {
        DEBUG("error: invalid address given");
        return 1;
    }

    /* put packet together */
    pkt = gnrc_pktbuf_add(NULL, payload, strlen(payload), GNRC_NETTYPE_UNDEF);
    if (pkt == NULL) {
        DEBUG("error: packet buffer full");
        return 1;
    }
    hdr = gnrc_netif_hdr_build(NULL, 0, addr, addr_len);
    if (hdr == NULL) {
        DEBUG("error: packet buffer full");
        gnrc_pktbuf_release(pkt);
        return 1;
    }
    LL_PREPEND(pkt, hdr);
    nethdr = (gnrc_netif_hdr_t *)hdr->data;
    nethdr->flags = flags;
    /* and send it */
    if (gnrc_netapi_send(iface, pkt) < 1) {
        DEBUG("error: unable to send");
        gnrc_pktbuf_release(pkt);
        return 1;
    }

    return 0;
}

static void gpio_cb(void *arg)
{
    msg_t msg = *(msg_t *)arg;
    msg_send(&msg, pid);
}

static void *thread_handler(void *arg)
{
    (void) arg;

    while (1) {
        msg_t msg;
        msg_receive(&msg);
        switch (msg.type) {
            case IF_MSG_TX:
                if (msg.content.ptr != NULL) {
                    if_tx_t *tx = (if_tx_t *)msg.content.ptr;
                    send(tx->iface, tx->dest, tx->payload);
                } else {
                    DEBUG("TX pointer is NULL\n");
                }
                break;
    
            case IF_MSG_PHY_CFG_SUB_GHZ:
#ifdef MODULE_AT86RF215
                next_phy_sub_ghz();
#else
                DEBUG("PHY is not configurable\n");
#endif /* MODULE_AT86RF215 */      
                break;

            case IF_MSG_PHY_CFG_2_4_GHZ:
#ifdef MODULE_AT86RF215
#if (GNRC_NETIF_NUMOF >= 2)
                next_phy_2_4_ghz();
#else
                DEBUG("2.4GHz interface not available\n");
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#else
                DEBUG("PHY is not configurable\n");
#endif /* MODULE_AT86RF215 */             
                break;

            default:
                break;
        }
    }

    return NULL;
}

int main(void)
{
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);
    
    netopt_enable_t disable = NETOPT_DISABLE;
    netif = gnrc_netif_iter(netif);
    while (netif)
    {
        gnrc_netapi_set(netif->pid, NETOPT_AUTOACK, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_ACK_REQ, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_CSMA, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_AUTOCCA, 0, &disable, sizeof(disable));
        netif = gnrc_netif_iter(netif);
    }

#ifdef MODULE_AT86RF215
    init_phy_sub_ghz();
#if (GNRC_NETIF_NUMOF >= 2)
    init_phy_2_4_ghz();
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

    (void) puts("Welcome to RIOT!");

    pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                        0, thread_handler, NULL, "thread");

    msg_t tx_message_sub_ghz = {.type = IF_MSG_TX};
#ifdef MODULE_AT86RF215
    tx_message_sub_ghz.content.ptr = &tx_sub_ghz;
#endif /* MODULE_AT86RF215 */
    msg_t tx_message_2_4_ghz = {.type = IF_MSG_TX};
#if defined(MODULE_AT86RF215) && (GNRC_NETIF_NUMOF >= 2)
    tx_message_2_4_ghz.content.ptr = &tx_2_4_ghz;
#endif /* MODULE_AT86RF215 && (GNRC_NETIF_NUMOF >= 2) */
    msg_t phy_cfg_sub_ghz_message;
    phy_cfg_sub_ghz_message.type = IF_MSG_PHY_CFG_SUB_GHZ;
    msg_t phy_cfg_2_4_ghz_message;
    phy_cfg_2_4_ghz_message.type = IF_MSG_PHY_CFG_2_4_GHZ;
    /* initialize input pins */
    gpio_init(TX_SUB_GHZ_PIN, GPIO_IN);
    gpio_init(TX_2_4_GHZ_PIN, GPIO_IN);
    gpio_init(PHY_CFG_SUB_GHZ_PIN, GPIO_IN);
    gpio_init(PHY_CFG_2_4_GHZ_PIN, GPIO_IN);
    /* initialize input pins to be triggered */
    gpio_init_int(TX_SUB_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &tx_message_sub_ghz);
    gpio_init_int(TX_2_4_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &tx_message_2_4_ghz);
    gpio_init_int(PHY_CFG_SUB_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &phy_cfg_sub_ghz_message);
    gpio_init_int(PHY_CFG_2_4_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &phy_cfg_2_4_ghz_message);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
