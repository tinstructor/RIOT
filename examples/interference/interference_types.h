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

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief   Interference application message types
 */
typedef enum {
    IF_MSG_TX,
    IF_MSG_PHY_CFG_SUB_GHZ,
    IF_MSG_PHY_CFG_2_4_GHZ,
} if_msg_t;

/**
 * @brief   Interference application transmission type
 */
typedef struct {
    kernel_pid_t iface; /**< PID of interface to transmit on */
    char *dest;         /**< HW addr of destination */
    char *payload;      /**< Bytes to send */
} if_tx_t;

#ifdef MODULE_AT86RF215
typedef enum {
    PHY_CFG_SUB_GHZ_FSK_1,
    PHY_CFG_SUB_GHZ_FSK_2,
    PHY_CFG_SUB_GHZ_OFDM_1,
    PHY_CFG_SUB_GHZ_OFDM_2,
    PHY_CFG_SUB_GHZ_OFDM_3,
    PHY_CFG_SUB_GHZ_OFDM_4,
    NUM_PHY_CFG_SUB_GHZ = 6
} phy_cfg_idx_sub_ghz_t;

#if (GNRC_NETIF_NUMOF >= 2)
typedef enum {
    PHY_CFG_2_4_GHZ_FSK_1,
    PHY_CFG_2_4_GHZ_FSK_2,
    PHY_CFG_2_4_GHZ_OFDM_1,
    PHY_CFG_2_4_GHZ_OFDM_2,
    PHY_CFG_2_4_GHZ_OFDM_3,
    PHY_CFG_2_4_GHZ_OFDM_4,
    NUM_PHY_CFG_2_4_GHZ = 6
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

/**
 * @brief   Interference application PHY configuration type
 */
typedef struct {
    kernel_pid_t iface; /**< PID of the interface to configure */
    uint8_t ieee802154_phy;
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
