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
 * @brief       Configuration of the MR-FSK PHY on the AT86RF215 chip
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include "at86rf215.h"
#include "at86rf215_internal.h"

#include "debug.h"

/* IEEE Std 802.15.4g™-2012 Amendment 3
 * Table 68d—Total number of channels and first channel center frequencies for SUN PHYs */
#define FSK_CHANNEL_SPACING            (200U)     /* kHz */
#define FSK_CENTER_FREQUENCY_SUBGHZ    (863125U)  /* Hz  */
#define FSK_CENTER_FREQUENCY_24GHZ     (2400200U - CCF0_24G_OFFSET) /* Hz  */

/* table 6-51: Symbol Rate Settings */
static uint8_t _TXDFE_SR(at86rf215_t *dev, uint8_t srate)
{
    const uint8_t version = at86rf215_reg_read(dev, RG_RF_VN);

    switch (srate) {
    case FSK_SRATE_50K:
        return version == 1 ? RF_SR_400K : RF_SR_500K;
    case FSK_SRATE_100K:
        return version == 1 ? RF_SR_800K : RF_SR_1000K;
    case FSK_SRATE_150K:
    case FSK_SRATE_200K:
        return RF_SR_2000K;
    case FSK_SRATE_300K:
    case FSK_SRATE_400K:
        return RF_SR_4000K;
    }

    return 0;
}

/* table 6-51: Symbol Rate Settings */
static uint8_t _RXDFE_SR(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_SR_400K;
    case FSK_SRATE_100K:
        return RF_SR_800K;
    case FSK_SRATE_150K:
    case FSK_SRATE_200K:
        return RF_SR_1000K;
    case FSK_SRATE_300K:
    case FSK_SRATE_400K:
        return RF_SR_2000K;
    }

    return 0;
}

/* table 6-53 Recommended Configuration of the Transmitter Frontend */
static uint8_t _TXCUTC_PARAMP(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_PARAMP32U;
    case FSK_SRATE_100K:
    case FSK_SRATE_150K:
    case FSK_SRATE_200K:
        return RF_PARAMP16U;
    case FSK_SRATE_300K:
    case FSK_SRATE_400K:
        return RF_PARAMP8U;
    }

    return 0;
}

/* Table 6-53. Recommended Configuration of the Transmitter Frontend (modulation index 0.5) */
static uint8_t _TXCUTC_LPFCUT_half(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_FLC80KHZ;
    case FSK_SRATE_100K:
        return RF_FLC100KHZ;
    case FSK_SRATE_150K:
        return RF_FLC160KHZ;
    case FSK_SRATE_200K:
        return RF_FLC200KHZ;
    case FSK_SRATE_300K:
        return RF_FLC315KHZ;
    case FSK_SRATE_400K:
        return RF_FLC400KHZ;
    }
    return 0;
}

/* Table 6-54. Recommended Configuration of the Transmitter Frontend (modulation index 1) */
static uint8_t _TXCUTC_LPFCUT_full(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_FLC80KHZ;
    case FSK_SRATE_100K:
        return RF_FLC160KHZ;
    case FSK_SRATE_150K:
        return RF_FLC250KHZ;
    case FSK_SRATE_200K:
        return RF_FLC315KHZ;
    case FSK_SRATE_300K:
        return RF_FLC500KHZ;
    case FSK_SRATE_400K:
        return RF_FLC625KHZ;
    }
    return 0;
}

static inline uint8_t _TXCUTC_LPFCUT(uint8_t srate, bool half)
{
    return half ? _TXCUTC_LPFCUT_half(srate) : _TXCUTC_LPFCUT_full(srate);
}

/* Table 6-60. Recommended Configuration values of the sub-1GHz Receiver Frontend (modulation index 1/2) */
static uint8_t _RXBWC_BW_subGHz_half(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_BW160KHZ_IF250KHZ;
    case FSK_SRATE_100K:
        return RF_BW200KHZ_IF250KHZ;
    case FSK_SRATE_150K:
    case FSK_SRATE_200K:
        return RF_BW320KHZ_IF500KHZ;
    case FSK_SRATE_300K:
        return (RF_BW500KHZ_IF500KHZ | (1 << RXBWC_IFS_SHIFT));
    case FSK_SRATE_400K:
        return RF_BW630KHZ_IF1000KHZ;
    }

    return 0;
}

/* Table 6-61. Recommended Configuration values of the 2.4GHz Receiver Frontend (modulation index 1/2) */
static uint8_t _RXBWC_BW_2dot4GHz_half(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_BW160KHZ_IF250KHZ;
    case FSK_SRATE_100K:
        return RF_BW200KHZ_IF250KHZ;
    case FSK_SRATE_150K:
        return RF_BW320KHZ_IF500KHZ;
    case FSK_SRATE_200K:
        return RF_BW400KHZ_IF500KHZ;
    case FSK_SRATE_300K:
        return RF_BW630KHZ_IF1000KHZ;
    case FSK_SRATE_400K:
        return RF_BW800KHZ_IF1000KHZ;
    }

    return 0;
}

/* Table 6-62. Recommended Configuration values of the sub-1GHz Receiver Frontend (modulation index 1) */
static uint8_t _RXBWC_BW_subGHz_full(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_BW160KHZ_IF250KHZ;
    case FSK_SRATE_100K:
        return RF_BW320KHZ_IF500KHZ;
    case FSK_SRATE_150K:
        return RF_BW400KHZ_IF500KHZ;
    case FSK_SRATE_200K:
        return (RF_BW500KHZ_IF500KHZ | (1 << RXBWC_IFS_SHIFT));
    case FSK_SRATE_300K:
        return RF_BW630KHZ_IF1000KHZ;
    case FSK_SRATE_400K:
        return (RF_BW1000KHZ_IF1000KHZ | (1 << RXBWC_IFS_SHIFT));
    }

    return 0;
}

/* Table 6-63. Recommended Configuration values of the 2.4 GHz Receiver Frontend (modulation index 1) */
static uint8_t _RXBWC_BW_2dot4GHz_full(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return RF_BW200KHZ_IF250KHZ;
    case FSK_SRATE_100K:
        return RF_BW400KHZ_IF500KHZ;
    case FSK_SRATE_150K:
    case FSK_SRATE_200K:
        return RF_BW630KHZ_IF1000KHZ;
    case FSK_SRATE_300K:
        return RF_BW800KHZ_IF1000KHZ;
    case FSK_SRATE_400K:
        return (RF_BW1000KHZ_IF1000KHZ | (1 << RXBWC_IFS_SHIFT));
    }

    return 0;
}

static inline uint8_t _RXBWC_BW(uint8_t srate, bool subGHz, bool half)
{
    if (subGHz) {
        return half ? _RXBWC_BW_subGHz_half(srate) : _RXBWC_BW_subGHz_full(srate);
    } else {
        return half ? _RXBWC_BW_2dot4GHz_half(srate) : _RXBWC_BW_2dot4GHz_full(srate);
    }
}

static uint8_t _RXDFE_RCUT_half(uint8_t srate, bool subGHz)
{
    if (srate == FSK_SRATE_200K) {
        return RF_RCUT_FS_BY_5P3;
    }

    if (srate == FSK_SRATE_400K && !subGHz) {
        return RF_RCUT_FS_BY_5P3;
    }

    return RF_RCUT_FS_BY_8;
}

static uint8_t _RXDFE_RCUT_full(uint8_t srate, bool subGHz)
{
    switch (srate) {
    case FSK_SRATE_50K:
    case FSK_SRATE_100K:
    case FSK_SRATE_300K:
        return RF_RCUT_FS_BY_5P3;
    case FSK_SRATE_150K:
    case FSK_SRATE_400K:
        return subGHz ? RF_RCUT_FS_BY_5P3 : RF_RCUT_FS_BY_4;
    case FSK_SRATE_200K:
        return subGHz ? RF_RCUT_FS_BY_4 : RF_RCUT_FS_BY_2P6;
    }

    return 0;
}

static uint32_t _srate(uint8_t srate)
{
    switch (srate) {
    case FSK_SRATE_50K:
        return  50 * 1000;
    case FSK_SRATE_100K:
        return 100 * 1000;
    case FSK_SRATE_150K:
        return 150 * 1000;
    case FSK_SRATE_200K:
        return 200 * 1000;
    case FSK_SRATE_300K:
        return 300 * 1000;
    case FSK_SRATE_400K:
        return 400 * 1000;
    }

    return 0;
}

static inline uint8_t _RXDFE_RCUT(uint8_t srate, bool subGHz, bool half)
{
    return half ? _RXDFE_RCUT_half(srate, subGHz) : _RXDFE_RCUT_full(srate, subGHz);
}

void at86rf215_configure_FSK(at86rf215_t *dev, uint8_t srate, bool mod_idx_half)
{
    if (srate > FSK_SRATE_400K) {
        DEBUG("[%s] invalid symbol rate: %d\n", __func__, srate);
        return;
    }

    /* disable radio */
    at86rf215_reg_write(dev, dev->BBC->RG_PC, 0);

    /* Set Receiver Bandwidth: fBW = 160 kHz, fIF = 250 kHz */
    at86rf215_reg_write(dev, dev->RF->RG_RXBWC, _RXBWC_BW(srate, is_subGHz(dev), mod_idx_half));
    /* fS = 400 kHz; fCUT = fS/5.333 = 75 kHz */
    at86rf215_reg_write(dev, dev->RF->RG_RXDFE, _RXDFE_SR(srate)
                                              | _RXDFE_RCUT(srate, is_subGHz(dev), mod_idx_half));
    /* Power Amplifier Ramp Time = 32 µs; fLPCUT = 80 kHz */
    at86rf215_reg_write(dev, dev->RF->RG_TXCUTC, _TXCUTC_PARAMP(srate)
                                               | _TXCUTC_LPFCUT(srate, mod_idx_half));
    /* fS = 500 kHz; fCUT = fS/2 = 250 kHz */
    at86rf215_reg_write(dev, dev->RF->RG_TXDFE, _TXDFE_SR(dev, srate)
                                              | (mod_idx_half ? RF_RCUT_FS_BY_8 : RF_RCUT_FS_BY_2)
                                              | TXDFE_DM_MASK);

    /* set receiver gain target according to data sheet */
    at86rf215_reg_write(dev, dev->RF->RG_AGCS, 1 << AGCS_TGT_SHIFT);
    /* enable automatic receiver gain */
    at86rf215_reg_write(dev, dev->RF->RG_AGCC, AGCC_EN_MASK);

    /* set channel spacing, same for both sub-GHz & 2.4 GHz */
    at86rf215_reg_write(dev, dev->RF->RG_CS, FSK_CHANNEL_SPACING / 25);

    /* set center frequency */
    if (is_subGHz(dev)) {
        at86rf215_reg_write16(dev, dev->RF->RG_CCF0L, FSK_CENTER_FREQUENCY_SUBGHZ / 25);
    } else {
        at86rf215_reg_write16(dev, dev->RF->RG_CCF0L, FSK_CENTER_FREQUENCY_24GHZ / 25);
    }

    /* set Bandwidth Time Product, Modulation Index & Modulation Order */
    /* effective modulation index = MIDXS * MIDX */
    at86rf215_reg_write(dev, dev->BBC->RG_FSKC0, FSK_BT_20
                                               | FSK_MIDXS_SCALE_8_BY_8
                                               | (mod_idx_half ? FSK_MIDX_4_BY_8 : FSK_MIDX_8_BY_8)
                                               | FSK_MORD_2SFK);

    /* set symbol rate, preamble is less than 256 so set hight bits 0 */
    at86rf215_reg_write(dev, dev->BBC->RG_FSKC1, srate);

    /* enable direct modulation */
    at86rf215_reg_write(dev, dev->BBC->RG_FSKDM, FSKDM_EN_MASK | FSKDM_PE_MASK);

    /* configure pre-emphasis */
    at86rf215_reg_write(dev, dev->BBC->RG_FSKPE0, FSKPE_Val[0][srate]);
    at86rf215_reg_write(dev, dev->BBC->RG_FSKPE1, FSKPE_Val[1][srate]);
    at86rf215_reg_write(dev, dev->BBC->RG_FSKPE2, FSKPE_Val[2][srate]);

    at86rf215_enable_radio(dev, BB_MRFSK);

    /* assume channel spacing for mode #1 */
    dev->num_chans = is_subGHz(dev) ? 34 : 416;
    dev->ack_timeout_usec =  3 * AT86RF215_ACK_PERIOD_IN_SYMBOLS * 1000000 / _srate(srate);
    DEBUG("[%s] ACK timeout: %d µs\n", __func__, dev->ack_timeout_usec);
}
