/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_at25xxx
 * @{
 *
 * @file
 * @brief       Driver for the AT25xxx family of SPI-EEPROMs.
 *              This also includes M95xxx, 25AAxxx, 25LCxxx,
 *              CAT25xxx & BR25Sxxx.
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include <errno.h>
#include <string.h>

#include "at25xxx.h"
#include "at25xxx_params.h"
#include "byteorder.h"

#ifdef USEMODULE_XTIMER
#include "xtimer.h"
#define POLL_DELAY_US   (1000)
#endif

#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

#define PAGE_SIZE   (AT25XXX_PARAM_PAGE_SIZE)
#define ADDR_LEN    (AT25XXX_PARAM_ADDR_LEN)
#define ADDR_MSK    ((1UL << ADDR_LEN) - 1)

#define CMD_WREN    (0x6)   /* Write Enable             */
#define CMD_WRDI    (0x4)   /* Write Disable            */
#define CMD_RDSR    (0x5)   /* Read Status Register     */
#define CMD_WRSR    (0x1)   /* Write Status Register    */
#define CMD_READ    (0x3)   /* Read from Memory Array   */
#define CMD_WRITE   (0x2)   /* Write to Memory Array    */

#define SR_WIP      (0x01)  /* Write In Progress        */
#define SR_WEL      (0x02)  /* Write Enable Latch       */
#define SR_BP0      (0x04)  /* Block Protect 0          */
#define SR_BP1      (0x08)  /* Block Protect 1          */
#define SR_SRWD     (0x80)  /* Status Register Write Disable */

static inline void getbus(const at25xxx_t *dev)
{
    spi_acquire(dev->params.spi, dev->params.cs_pin, SPI_MODE_0, dev->params.spi_clk);
}

static inline uint32_t _pos(uint8_t cmd, uint32_t pos)
{
    /* first byte is CMD, then addr with MSB first */
    pos = htonl((pos & ADDR_MSK) | ((uint32_t)cmd << ADDR_LEN));
    pos >>= 8 * sizeof(pos) - (ADDR_LEN + 8);
    return pos;
}

static inline bool _write_in_progress(const at25xxx_t *dev)
{
    return spi_transfer_reg(dev->params.spi, dev->params.cs_pin, CMD_RDSR, 0) & SR_WIP;
}

static inline bool _write_enabled(const at25xxx_t *dev)
{
    return spi_transfer_reg(dev->params.spi, dev->params.cs_pin, CMD_RDSR, 0) & SR_WEL;
}

static size_t _write_page(const at25xxx_t *dev, uint32_t pos, const void *data, size_t len)
{
    len = min(len, PAGE_SIZE - (pos & (PAGE_SIZE - 1)));
    pos = _pos(CMD_WRITE, pos);

    /* wait for previous write to finish - may take up to 5 ms */
    while (_write_in_progress(dev)) {
#ifdef USEMODULE_XTIMER
        xtimer_usleep(POLL_DELAY_US);
#endif
    }

    /* set write enable and wait for status change */
    spi_transfer_byte(dev->params.spi, dev->params.cs_pin, false, CMD_WREN);
    while (!_write_enabled(dev)) {}

    /* write the data */
    spi_transfer_bytes(dev->params.spi, dev->params.cs_pin, true, &pos, NULL, 1 + ADDR_LEN / 8);
    spi_transfer_bytes(dev->params.spi, dev->params.cs_pin, false, data, NULL, len);

    return len;
}

size_t at25xxx_write(const at25xxx_t *dev, uint32_t pos, const void *data, size_t len)
{
    const uint8_t *d = data;

    if (pos + len > dev->params.size) {
        errno = ERANGE;
        return 0;
    }

    getbus(dev);

    while (len) {
        size_t written = _write_page(dev, pos, d, len);
        len -= written;
        pos += written;
        d   += written;
    }

    spi_release(dev->params.spi);

    return (void*)d - data;
}

void at25xxx_write_byte(const at25xxx_t *dev, uint32_t pos, uint8_t data)
{
    at25xxx_write(dev, pos, &data, sizeof(data));
}

size_t at25xxx_read(const at25xxx_t *dev, uint32_t pos, void *data, size_t len)
{
    if (pos + len > dev->params.size) {
        errno = ERANGE;
        return 0;
    }

    getbus(dev);

    /* wait for previous write to finish - may take up to 5 ms */
    while (_write_in_progress(dev)) {
#ifdef USEMODULE_XTIMER
        xtimer_usleep(POLL_DELAY_US);
#endif
    }

    pos = _pos(CMD_READ, pos);
    spi_transfer_bytes(dev->params.spi, dev->params.cs_pin, true, &pos, NULL, 1 + ADDR_LEN / 8);
    spi_transfer_bytes(dev->params.spi, dev->params.cs_pin, false, NULL, data, len);

    spi_release(dev->params.spi);

    return len;
}

uint8_t at25xxx_read_byte(const at25xxx_t *dev, uint32_t pos)
{
    uint8_t b;
    at25xxx_read(dev, pos, &b, sizeof(b));
    return b;
}

size_t at25xxx_set(const at25xxx_t *dev, uint32_t pos, uint8_t val, size_t len)
{
    uint8_t data[PAGE_SIZE];
    size_t total = 0;

    if (pos + len > dev->params.size) {
        errno = ERANGE;
        return 0;
    }

    memset(data, val, sizeof(data));

    getbus(dev);

    while (len) {
        size_t written = _write_page(dev, pos, data, len);
        len   -= written;
        pos   += written;
        total += written;
    }

    spi_release(dev->params.spi);

    return total;
}

size_t at25xxx_clear(const at25xxx_t *dev, uint32_t pos, size_t len)
{
    return at25xxx_set(dev, pos, 0, len);
}

void at25xxx_init(at25xxx_t *dev, const at25xxx_params_t *params)
{
    dev->params = *params;
    spi_init_cs(dev->params.spi, dev->params.cs_pin);

    if (dev->params.wp_pin != GPIO_UNDEF) {
        gpio_init(dev->params.wp_pin, GPIO_OUT);
        gpio_set(dev->params.wp_pin);
    }

    if (dev->params.hold_pin != GPIO_UNDEF) {
        gpio_init(dev->params.hold_pin, GPIO_OUT);
        gpio_set(dev->params.hold_pin);
    }
}
