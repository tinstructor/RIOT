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
 * @brief       Low-Level functions for the AT86RF215 driver
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include "periph/spi.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "at86rf215_internal.h"

#include <string.h>

#define SPIDEV          (dev->params.spi)
#define CSPIN           (dev->params.cs_pin)

static inline void getbus(const at86rf215_t *dev)
{
    spi_acquire(SPIDEV, CSPIN, SPI_MODE_0, dev->params.spi_clk);
}

void at86rf215_assert_awake(const at86rf215_t *dev)
{
    if (at86rf215_get_rf_state(dev) != RF_STATE_RESET) {
        return;
    }

    at86rf215_rf_cmd(dev, CMD_RF_TRXOFF);
    xtimer_usleep(AT86RF215_WAKEUP_DELAY);

    while (at86rf215_get_rf_state(dev) != RF_STATE_TRXOFF) {}
}

void at86rf215_hardware_reset(at86rf215_t *dev)
{
    /* trigger hardware reset */
    gpio_clear(dev->params.reset_pin);
    xtimer_usleep(AT86RF215_RESET_PULSE_WIDTH);
    gpio_set(dev->params.reset_pin);
    xtimer_usleep(AT86RF215_RESET_DELAY);

    while (at86rf215_get_rf_state(dev) != RF_STATE_TRXOFF) {}
}

void at86rf215_reg_write(const at86rf215_t *dev, uint16_t reg, uint8_t value)
{
    reg = htons(reg | FLAG_WRITE);

    getbus(dev);
    spi_transfer_bytes(SPIDEV, CSPIN, true, &reg, NULL, sizeof(reg));
    spi_transfer_byte(SPIDEV, CSPIN, false, value);
    spi_release(SPIDEV);
}

void at86rf215_reg_write_bytes(const at86rf215_t *dev, uint16_t reg, const void *data, size_t len)
{
    reg = htons(reg | FLAG_WRITE);

    getbus(dev);
    spi_transfer_bytes(SPIDEV, CSPIN, true, &reg, NULL, sizeof(reg));
    spi_transfer_bytes(SPIDEV, CSPIN, false, data, NULL, len);
    spi_release(SPIDEV);
}

uint8_t at86rf215_reg_read(const at86rf215_t *dev, uint16_t reg)
{
    uint8_t val;

    reg = htons(reg | FLAG_READ);

    getbus(dev);
    spi_transfer_bytes(SPIDEV, CSPIN, true, &reg, NULL, sizeof(reg));
    val = spi_transfer_byte(SPIDEV, CSPIN, false, 0);
    spi_release(SPIDEV);

    return val;
}

void at86rf215_reg_read_bytes(const at86rf215_t *dev, uint16_t reg, void *data, size_t len)
{
    reg = htons(reg | FLAG_READ);

    getbus(dev);
    spi_transfer_bytes(SPIDEV, CSPIN, true, &reg, NULL, sizeof(reg));
    spi_transfer_bytes(SPIDEV, CSPIN, false, NULL, data, len);
    spi_release(SPIDEV);
}

void at86rf215_filter_ack(at86rf215_t *dev, bool on)
{
    /* only listen for ACK frames */
    uint8_t val = on ? (1 << IEEE802154_FCF_TYPE_ACK)
                     : (1 << IEEE802154_FCF_TYPE_BEACON)
                     | (1 << IEEE802154_FCF_TYPE_DATA)
                     | (1 << IEEE802154_FCF_TYPE_MACCMD);

    at86rf215_reg_write(dev, dev->BBC->RG_AFFTM, val);
}

void at86rf215_get_random(at86rf215_t *dev, uint8_t *data, size_t len)
{
    /* make sure the radio is idle */
    _wait_for_idle(dev);

    at86rf215_disable_baseband(dev);

    while (len--) {
        *data++ = at86rf215_reg_read(dev, dev->RF->RG_RNDV);
    }

    at86rf215_enable_baseband(dev);
    mutex_unlock(&dev->cond_lock);
}

const char* at86rf215_state2a(uint8_t state)
{
    switch (state) {
        case RF_STATE_TRXOFF: return "TRXOFF";
        case RF_STATE_TXPREP: return "TXPREP";
        case RF_STATE_TX: return "TX";
        case RF_STATE_RX: return "RX";
        case RF_STATE_TRANSITION: return "TRANSITION";
        case RF_STATE_RESET: return "RESET";
        default: return "invalid";
    }
}
