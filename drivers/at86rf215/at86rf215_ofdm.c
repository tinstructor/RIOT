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
 * @brief       Configuration of the MR-OFDM PHY on the AT86RF215 chip
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include "at86rf215.h"
#include "at86rf215_internal.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define KBIT 1000

/* IEEE Std 802.15.4g™-2012 Amendment 3
 * Table 68d—Total number of channels and first channel center frequencies for SUN PHYs */
static uint32_t _channel_spacing_kHz(uint8_t option)
{
    switch (option) {
    case 1: return 1200;
    case 2: return  800;
    case 3: return  400;
    case 4: return  200;
    }

    return 0;
}

/* IEEE Std 802.15.4g™-2012 Amendment 3
 * Table 68d—Total number of channels and first channel center frequencies for SUN PHYs */
static uint32_t _channel_center_freq_kHz_868MHz(uint8_t option)
{
    switch (option) {
    case 1: return 863625;
    case 2: return 863425;
    case 3: return 863225;
    case 4: return 863125;
    }

    return 0;
}

/* IEEE Std 802.15.4g™-2012 Amendment 3
 * Table 68d—Total number of channels and first channel center frequencies for SUN PHYs */
static uint32_t _channel_center_freq_kHz_2400MHz(uint8_t option)
{
    return 2400000 + _channel_spacing_kHz(option) - CCF0_24G_OFFSET;
}

/* IEEE Std 802.15.4g™-2012 Amendment 3
 * Table 68d—Total number of channels and first channel center frequencies for SUN PHYs */
static uint16_t _get_max_chan(at86rf215_t *dev, uint8_t option)
{
    if (is_subGHz(dev)) {
        switch (option) {
        case 1: return  5;
        case 2: return  8;
        case 3: return 17;
        case 4: return 34;
        }
    } else {
        switch (option) {
        case 1: return 64;
        case 2: return 97;
        case 3: return 207;
        case 4: return 416;
        }
    }

    return 0;
}

/* Table 6-90. Recommended Transmitter Frontend Configuration */
static uint32_t _TXCUTC_LPFCUT(uint8_t option)
{
    switch (option) {
    case 1: return 10 << TXCUTC_LPFCUT_SHIFT;
    case 2: return  8 << TXCUTC_LPFCUT_SHIFT;
    case 3: return  5 << TXCUTC_LPFCUT_SHIFT;
    case 4: return  3 << TXCUTC_LPFCUT_SHIFT;
    }

    return 0;
}

/* Table 6-90. Recommended Transmitter Frontend Configuration */
static uint32_t _TXDFE_SR(uint8_t option)
{
    switch (option) {
    case 1:
    case 2: return 3 << TXDFE_SR_SHIFT;
    case 3:
    case 4: return 6 << TXDFE_SR_SHIFT;
    }

    return 0;
}

/* Table 6-90. Recommended Transmitter Frontend Configuration */
static uint32_t _TXDFE_RCUT(uint8_t option)
{
    switch (option) {
    case 1: return 3 << TXDFE_RCUT_SHIFT;
    case 2:
    case 3: return 3 << TXDFE_RCUT_SHIFT;
    case 4: return 2 << TXDFE_RCUT_SHIFT;
    }

    return 0;
}

/* Table 6-93. Recommended PHY Receiver and Digital Frontend Configuration */
static uint32_t _RXDFE_RCUT(uint8_t option, bool superGHz)
{
    switch (option) {
    case 1: return 4 << RXDFE_RCUT_SHIFT;
    case 2: return 2 << RXDFE_RCUT_SHIFT;
    case 3: return (2 + superGHz) << RXDFE_RCUT_SHIFT;
    case 4: return 1 << RXDFE_RCUT_SHIFT;
    }

    return 0;
}

/* Table 6-93. Recommended PHY Receiver and Digital Frontend Configuration */
static uint32_t _RXBWC_BW(uint8_t option, bool superGHz)
{
    switch (option) {
    case 1: return (9 + superGHz) << RXBWC_BW_SHIFT;
    case 2: return 7 << RXBWC_BW_SHIFT;
    case 3: return (4 + superGHz) << RXBWC_BW_SHIFT;
    case 4: return (2 + superGHz) << RXBWC_BW_SHIFT;
    }

    return 0;
}

/* Table 6-93. Recommended PHY Receiver and Digital Frontend Configuration */
static uint32_t _RXBWC_IFS(uint8_t option, bool superGHz)
{
    switch (option) {
    case 1:
    case 2: return 1;
    case 3: return superGHz;
    case 4: return !superGHz;
    }

    return 0;
}

/* Table 6-88. MR-OFDM PHY Options and Modulation Schemes */
static uint32_t _get_bitrate(uint8_t option, uint8_t scheme)
{
    switch (scheme) {
    case BB_MCS_BPSK_REP4:  return  100 * KBIT / option;
    case BB_MCS_BPSK_REP2:  return  200 * KBIT / option;
    case BB_MCS_QPSK_REP2:  return  400 * KBIT / option;
    case BB_MCS_QPSK_1BY2:  return  800 * KBIT / option;
    case BB_MCS_QPSK_3BY4:  return 1200 * KBIT / option;
    case BB_MCS_16QAM_1BY2: return 1600 * KBIT / option;
    case BB_MCS_16QAM_3BY4: return 2400 * KBIT / option;
    }

    return 0;
}

void at86rf215_configure_OFDM(at86rf215_t *dev, uint8_t option, uint8_t scheme)
{
    if (option < 1 || option > 4) {
        DEBUG("[%s] invalid option: %d\n", __func__, option);
        return;
    }

    if (scheme > BB_MCS_16QAM_3BY4) {
        DEBUG("[%s] invalid scheme: %d\n", __func__, scheme);
        return;
    }

    const bool superGHz = !is_subGHz(dev);

    /* disable radio */
    at86rf215_reg_write(dev, dev->BBC->RG_PC, 0);

    /* Set Receiver Bandwidth */
    at86rf215_reg_write(dev, dev->RF->RG_RXBWC, _RXBWC_BW(option, superGHz)
                                              | _RXBWC_IFS(option, superGHz));
    /* Set fS (same as TX); fCUT for RX */
    at86rf215_reg_write(dev, dev->RF->RG_RXDFE, _TXDFE_SR(option)
                                              | _RXDFE_RCUT(option, superGHz));
    /* Set Power Amplifier Ramp Time; fLPCUT */
    at86rf215_reg_write(dev, dev->RF->RG_TXCUTC, RF_PARAMP8U
                                               | _TXCUTC_LPFCUT(option));
    /* Set fS; fCUT for TX */
    at86rf215_reg_write(dev, dev->RF->RG_TXDFE, _TXDFE_SR(option)
                                              | _TXDFE_RCUT(option));

    /* set receiver gain target according to data sheet */
    at86rf215_reg_write(dev, dev->RF->RG_AGCS, 3 << AGCS_TGT_SHIFT);
    /* enable automatic receiver gain */
    at86rf215_reg_write(dev, dev->RF->RG_AGCC, AGCC_EN_MASK);

    /* set channel spacing */
    at86rf215_reg_write(dev, dev->RF->RG_CS, _channel_spacing_kHz(option) / 25);

    /* set channel center frequency */
    if (superGHz) {
        at86rf215_reg_write16(dev, dev->RF->RG_CCF0L, _channel_center_freq_kHz_2400MHz(option) / 25);
    } else {
        at86rf215_reg_write16(dev, dev->RF->RG_CCF0L, _channel_center_freq_kHz_868MHz(option) / 25);
    }

    at86rf215_reg_write(dev, dev->BBC->RG_OFDMC, option - 1);
    at86rf215_reg_write(dev, dev->BBC->RG_OFDMPHRTX, scheme);

    at86rf215_enable_radio(dev, BB_MROFDM);

    dev->num_chans = _get_max_chan(dev, option);
    dev->ack_timeout_usec = AT86RF215_ACK_PERIOD_IN_BITS * 1000000 / _get_bitrate(option, scheme);
    DEBUG("[%s] ACK timeout: %d µs\n", __func__, dev->ack_timeout_usec);
}
