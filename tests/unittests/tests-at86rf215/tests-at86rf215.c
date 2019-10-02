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

#include "../at86rf215_netdev.c"

void at86rf215_mock_init(const at86rf215_t *dev);

static const at86rf215_params_t test_params;
static at86rf215_t test_dev;

static uint8_t _cb_set_state(at86rf215_t *dev, uint16_t reg, uint8_t val, void *ctx)
{
    (void) dev;
    (void) ctx;
    (void) reg;
    at86rf215_reg_write(&test_dev, test_dev.RF->RG_STATE, val);
    return val;
}

static void _setup(void) {
    at86rf215_setup(&test_dev, NULL, &test_params);
    at86rf215_mock_init(&test_dev);

    at86rf215_reg_write(&test_dev, RG_RF_PN, AT86RF215M_PN);
    at86rf215_reg_write(&test_dev, test_dev.RF->RG_STATE, RF_STATE_TRXOFF);
    at86rf215_mock_reg_on_write_cb(test_dev.RF->RG_CMD, _cb_set_state, NULL);
}

static void test_at86rf215_dummy(void)
{
    TEST_ASSERT_EQUAL_INT(0, _init(&test_dev.netdev.netdev));
    _isr(&test_dev.netdev.netdev);
    TEST_ASSERT(1 == 1);
}

Test *tests_at86rf215_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_at86rf215_dummy),
    };

    EMB_UNIT_TESTCALLER(at86rf215_tests, _setup, NULL, fixtures);

    return (Test *)&at86rf215_tests;
}

void tests_at86rf215(void)
{
    TESTS_RUN(tests_at86rf215_tests());
}
