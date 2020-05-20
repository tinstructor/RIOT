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
 * @brief       Interference application specific definitions of interference types
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#ifndef INTERFERENCE_TYPES_H
#define INTERFERENCE_TYPES_H

#include "kernel_types.h"
#include "mutex.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
    IF_MSG_TX_SUB_GHZ,
    IF_MSG_TX_2_4_GHZ,
    IF_MSG_PHY_CFG_SUB_GHZ,
    IF_MSG_PHY_CFG_2_4_GHZ,
    IF_MSG_EXP_SIG,
} if_msg_t;

typedef struct {
    kernel_pid_t iface;
    char dest[24];
    char payload[493];
    mutex_t lock;
} if_tx_t;

#ifdef MODULE_AT86RF215
typedef enum {
    PHY_CFG_SUB_GHZ_FSK_1,
    PHY_CFG_SUB_GHZ_FSK_2,
    PHY_CFG_SUB_GHZ_OFDM_1,
    PHY_CFG_SUB_GHZ_OFDM_2,
    PHY_CFG_SUB_GHZ_OFDM_3,
    PHY_CFG_SUB_GHZ_OFDM_4,
    PHY_CFG_SUB_GHZ_OFDM_5,
    PHY_CFG_SUB_GHZ_OFDM_6,
    PHY_CFG_SUB_GHZ_OFDM_7,
    NUM_PHY_CFG_SUB_GHZ
} phy_cfg_idx_sub_ghz_t;

#if (GNRC_NETIF_NUMOF >= 2)
typedef enum {
    PHY_CFG_2_4_GHZ_FSK_1,
    PHY_CFG_2_4_GHZ_FSK_2,
    PHY_CFG_2_4_GHZ_OFDM_1,
    PHY_CFG_2_4_GHZ_OFDM_2,
    PHY_CFG_2_4_GHZ_OFDM_3,
    PHY_CFG_2_4_GHZ_OFDM_4,
    NUM_PHY_CFG_2_4_GHZ
} phy_cfg_idx_2_4_ghz_t;
#endif /* (GNRC_NETIF_NUMOF >= 2) */

typedef struct {
    uint8_t srate;
    uint16_t cspace;
    uint8_t mod_idx;
    uint8_t mod_ord;
} if_fsk_cfg_t;

typedef struct {
    uint8_t option;
    uint8_t mcs;
} if_ofdm_cfg_t;

typedef struct {
    kernel_pid_t iface;
    uint8_t pac_txpwr;
    uint8_t ieee802154_phy;
    char *phy_descriptor;
    uint16_t channel;
    union {
        if_fsk_cfg_t fsk_cfg;
        if_ofdm_cfg_t ofdm_cfg;
    };
} if_phy_cfg_t;
#endif /* MODULE_AT86RF215 */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* INTERFERENCE_TYPES_H */
/** @} */
