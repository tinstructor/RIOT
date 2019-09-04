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

#ifndef AT86RF215_INTERNAL_H
#define AT86RF215_INTERNAL_H

#include <stdint.h>
#include "at86rf215.h"
#include "at86rf215_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AT86RF215_WAKEUP_DELAY          (306U)
/* Minimum reset pulse width (tRST) - 625ns */
#define AT86RF215_RESET_PULSE_WIDTH     (1U)
/* The typical transition time to TRX_OFF after reset pulse is 1 µs (tPOWERON) */
#define AT86RF215_RESET_DELAY           (1U)

/* This is used to calculate the ACK timeout based on the bitrate.
 * AT86RF233 uses an ACK timeout of 54 symbol periods, or 864 µs @ 250 kbit/s
 * -> 864µs * 250kbit/s = 216 bit */
// #define AT86RF215_ACK_PERIOD_IN_BITS    (216U)
#define AT86RF215_ACK_PERIOD_IN_SYMBOLS (54U)
#define AT86RF215_ACK_PERIOD_IN_BITS    (AT86RF215_ACK_PERIOD_IN_SYMBOLS * 4)

/* legacy mode, 250 kbit/s */
#define AT86RF215_OQPSK_MODE_LEGACY           (0x1)
/* legacy mode, high data rate */
#define AT86RF215_OQPSK_MODE_LEGACY_HDR       (0x3)
/* MR-QPSK */
#define AT86RF215_MR_OQPSK_MODE(n)            ((n) << OQPSKPHRTX_MOD_SHIFT)

extern const uint8_t at86rf215_fsk_srate_10kHz[];

void at86rf215_hardware_reset(at86rf215_t *dev);

void at86rf215_reg_write(const at86rf215_t *dev, uint16_t reg, uint8_t val);

void at86rf215_reg_write_bytes(const at86rf215_t *dev, uint16_t reg, const void *data, size_t len);

uint8_t at86rf215_reg_read(const at86rf215_t *dev, uint16_t regAddr16);

void at86rf215_reg_read_bytes(const at86rf215_t *dev, uint16_t reg, void *data, size_t len);

void at86rf215_filter_ack(at86rf215_t *dev, bool on);

void at86rf215_get_random(at86rf215_t *dev, uint8_t *data, size_t len);

/** FSK **/
void at86rf215_configure_FSK(at86rf215_t *dev, uint8_t srate, uint8_t mod_idx, uint8_t mod_order);

int at86rf215_FSK_set_srate(at86rf215_t *dev, uint8_t srate);
uint8_t at86rf215_FSK_get_srate(at86rf215_t *dev);

int at86rf215_FSK_set_mod_idx(at86rf215_t *dev, uint8_t mod_idx);
uint8_t at86rf215_FSK_get_mod_idx(at86rf215_t *dev);

void at86rf215_FSK_set_fec(at86rf215_t *dev, bool enable);
bool at86rf215_FSK_get_fec(at86rf215_t *dev);

int at86rf215_FSK_set_mod_order(at86rf215_t *dev, uint8_t mod_order);
uint8_t at86rf215_FSK_get_mod_order(at86rf215_t *dev);

/** OFDM **/
void at86rf215_configure_OFDM(at86rf215_t *dev, uint8_t option, uint8_t scheme);

int at86rf215_OFDM_set_scheme(at86rf215_t *dev, uint8_t scheme);
uint8_t at86rf215_OFDM_get_scheme(at86rf215_t *dev);

int at86rf215_OFDM_set_option(at86rf215_t *dev, uint8_t option);
uint8_t at86rf215_OFDM_get_option(at86rf215_t *dev);

/** O-QPSK **/
void at86rf215_configure_OQPSK(at86rf215_t *dev, uint8_t chips, uint8_t rate);

uint8_t at86rf215_OQPSK_get_chips(at86rf215_t *dev);
int at86rf215_OQPSK_set_chips(at86rf215_t *dev, uint8_t chips);

uint8_t at86rf215_OQPSK_get_mode(at86rf215_t *dev);
int at86rf215_OQPSK_set_mode(at86rf215_t *dev, uint8_t mode);

uint8_t at86rf215_get_phy_mode(at86rf215_t *dev);

const char* at86rf215_hw_state2a(uint8_t state);
const char* at86rf215_sw_state2a(at86rf215_state_t state);

static inline void at86rf215_reg_and(const at86rf215_t *dev, uint16_t reg, uint8_t val)
{
    val &= at86rf215_reg_read(dev, reg);
    at86rf215_reg_write(dev, reg, val);
}

static inline void at86rf215_reg_or(const at86rf215_t *dev, uint16_t reg, uint8_t val)
{
    val |= at86rf215_reg_read(dev, reg);
    at86rf215_reg_write(dev, reg, val);
}

static inline void at86rf215_reg_write16(const at86rf215_t *dev, uint16_t reg, uint16_t value)
{
    at86rf215_reg_write_bytes(dev, reg, &value, sizeof(value));
}

static inline uint16_t at86rf215_reg_read16(const at86rf215_t *dev, uint16_t reg)
{
    uint16_t value;
    at86rf215_reg_read_bytes(dev, reg, &value, sizeof(value));
    return value;
}

static inline void at86rf215_rf_cmd(const at86rf215_t *dev, uint8_t rf_cmd)
{
    at86rf215_reg_write(dev, dev->RF->RG_CMD, rf_cmd);
}

static inline uint8_t at86rf215_get_rf_state(const at86rf215_t *dev)
{
    return at86rf215_reg_read(dev, dev->RF->RG_STATE) & STATE_STATE_MASK;
}

static inline void at86rf215_enable_baseband(const at86rf215_t *dev)
{
    at86rf215_reg_or(dev, dev->BBC->RG_PC, PC_BBEN_MASK);
}

static inline void at86rf215_disable_baseband(const at86rf215_t *dev) {
    at86rf215_reg_and(dev, dev->BBC->RG_PC, ~PC_BBEN_MASK);
}

static inline void at86rf215_enable_radio(at86rf215_t *dev, uint8_t modulation)
{
    /* 16 bit frame-checksum, baseband enabled, checksum calculated by chip,
       frames with invalid cs are dropped */
    at86rf215_reg_write(dev, dev->BBC->RG_PC, modulation   | PC_BBEN_MASK
                                            | PC_FCST_MASK | PC_TXAFCS_MASK
                                            | PC_FCSFE_MASK);
}

static inline bool is_subGHz(const at86rf215_t *dev)
{
    return dev->flags & AT86RF215_OPT_SUBGHZ;
}

#ifdef __cplusplus
}
#endif

#endif /* AT86RF215_INTERNAL_H */
/** @} */
