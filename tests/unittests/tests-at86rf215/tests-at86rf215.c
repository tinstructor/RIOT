/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @addtogroup  unittests
 * @{
 *
 * @file
 * @brief       Unit tests for the AT86RF215 driver
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include <stdlib.h>
#include "embUnit.h"
#include "embUnit/embUnit.h"

#include "mock_at86rf215.h"

#define ENABLE_DEBUG (1)
#include "../at86rf215_netdev.c"

void at86rf215_mock_init(const at86rf215_t *dev);

static const at86rf215_params_t test_params;
static at86rf215_t test_dev[2];
static void *rx_buffer;

static inline uint8_t _state(netdev_t *dev) {
    return ((at86rf215_t*)dev)->state;
}

static uint8_t _cb_set_state(at86rf215_t *dev, uint16_t reg, uint8_t val, void *ctx)
{
    (void) ctx;
    (void) reg;
    at86rf215_reg_write(dev, dev->RF->RG_STATE, val);
    return val;
}

static uint8_t _cb_clear_on_read(at86rf215_t *dev, uint16_t reg, uint8_t val, void *ctx)
{
    (void) ctx;
    at86rf215_reg_write(dev, reg, 0);
    return val;
}

static void _netdev_event_cb(netdev_t *dev, netdev_event_t event)
{
    int size;

    switch (event) {
    case NETDEV_EVENT_ISR:
        _isr(dev);
        break;
    case NETDEV_EVENT_RX_COMPLETE:
        TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_RX, _state(dev));
        size = _recv(dev, NULL, 0, NULL);
        rx_buffer = realloc(rx_buffer, size);
        TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_RX, _state(dev));
        TEST_ASSERT_EQUAL_INT(size, _recv(dev, rx_buffer, size, NULL));
        break;
    case NETDEV_EVENT_TX_COMPLETE:
        puts("TX done!");
        break;
    default:
        TEST_FAIL("unexpected netdev event");
    }
}

static void _setup_regs(at86rf215_t *dev)
{
    at86rf215_reg_write(dev, RG_RF_PN, AT86RF215M_PN);

    at86rf215_reg_write(dev, dev->RF->RG_STATE, RF_STATE_TRXOFF);
    at86rf215_mock_reg_on_write_cb(dev->RF->RG_CMD, _cb_set_state, NULL);

    /* make sure IRQ regs are cleared when read */
    at86rf215_mock_reg_on_read_cb(dev->RF->RG_IRQS, _cb_clear_on_read, NULL);
    at86rf215_mock_reg_on_read_cb(dev->BBC->RG_IRQS, _cb_clear_on_read, NULL);

    dev->netdev.netdev.event_callback = _netdev_event_cb;
    dev->flags |= AT86RF215_OPT_TELL_RX_END;
}

static void _setup(void) {

    if (rx_buffer) {
        free(rx_buffer);
        rx_buffer = NULL;
    }

    at86rf215_setup(&test_dev[0], &test_dev[1], &test_params);

    /* 2.4 GHz dev has the higher registers */
    at86rf215_mock_init(&test_dev[1]);

    _setup_regs(&test_dev[0]);
    _setup_regs(&test_dev[1]);

    TEST_ASSERT_EQUAL_INT(0, _init(&test_dev[0].netdev.netdev));
}

/*
 * Simulates receiving a message without ACK request set
 */
static void test_at86rf215_rx(void)
{
    at86rf215_t *dev = &test_dev[0];
    const char payload[] = "Hello AT86RF215!";

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);

    /* set RXFE interrupt */
    at86rf215_reg_write(dev, dev->BBC->RG_IRQS, BB_IRQ_RXFE);

    at86rf215_reg_write(dev, dev->BBC->RG_RXFLL, sizeof(payload) + IEEE802154_FCS_LEN);
    at86rf215_reg_write_bytes(dev, dev->BBC->RG_FBRXS, payload, sizeof(payload));

    _isr(&test_dev[0].netdev.netdev);

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);
    TEST_ASSERT_EQUAL_STRING(payload, rx_buffer);
}

/*
 * Simulates receiving a message with ACK request set
 */
static void test_at86rf215_rx_aack(void)
{
    at86rf215_t *dev = &test_dev[0];
    const char payload[] = "Hello AT86RF215, are you there?";

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);

    /* set RX done interrupt */
    at86rf215_reg_write(dev, dev->BBC->RG_IRQS, BB_IRQ_RXFE);

    /* set ACK requested bit */
    at86rf215_reg_write(dev, dev->BBC->RG_AMCS, AMCS_AACKFT_MASK);

    at86rf215_reg_write(dev, dev->BBC->RG_RXFLL, sizeof(payload) + IEEE802154_FCS_LEN);
    at86rf215_reg_write_bytes(dev, dev->BBC->RG_FBRXS, payload, sizeof(payload));

    _isr(&test_dev[0].netdev.netdev);

    /* receive function should not have been called yet */
    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_RX, dev->state);
    TEST_ASSERT_NULL(rx_buffer);

    /* set TX done interrupt - 'ACK has been sent' */
    at86rf215_reg_write(dev, dev->BBC->RG_IRQS, BB_IRQ_TXFE);

    _isr(&test_dev[0].netdev.netdev);

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);
    TEST_ASSERT_EQUAL_STRING(payload, rx_buffer);
}

/*
 * Simulates sending a message (without ACK)
 */
static void test_at86rf215_tx(void)
{
    uint8_t tx_buffer_len;
    char tx_buffer[128];
    at86rf215_t *dev = &test_dev[0];
    const char payload[] = "Hello Test!";

    dev->flags &= ~AT86RF215_OPT_ACK_REQUESTED;

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);

    TEST_ASSERT_EQUAL_INT(sizeof(payload), at86rf215_send(dev, payload, sizeof(payload)));

    /* read TX buffer */
    tx_buffer_len = at86rf215_reg_read(dev, dev->BBC->RG_TXFLL) - IEEE802154_FCS_LEN;
    TEST_ASSERT_EQUAL_INT(sizeof(payload), tx_buffer_len);

    at86rf215_reg_read_bytes(dev, dev->BBC->RG_FBTXS, tx_buffer, tx_buffer_len);
    TEST_ASSERT_EQUAL_STRING(payload, tx_buffer);

    /* set energy detect interrupt */
    at86rf215_reg_write(dev, dev->RF->RG_IRQS, RF_IRQ_EDC);

    /* set TX done interrupt */
    at86rf215_reg_write(dev, dev->BBC->RG_IRQS, BB_IRQ_TXFE);

    /* set channel clear */
    at86rf215_reg_write(dev, dev->BBC->RG_AMCS, 0);

    _isr(&test_dev[0].netdev.netdev);

    TEST_ASSERT_EQUAL_INT(AT86RF215_STATE_IDLE, dev->state);
}


Test *tests_at86rf215_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_at86rf215_rx),
        new_TestFixture(test_at86rf215_rx_aack),
        new_TestFixture(test_at86rf215_tx),
    };

    EMB_UNIT_TESTCALLER(at86rf215_tests, _setup, NULL, fixtures);

    return (Test *)&at86rf215_tests;
}

void tests_at86rf215(void)
{
    TESTS_RUN(tests_at86rf215_tests());
}
