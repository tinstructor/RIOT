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
 * @brief       Interference application specific definitions of constants
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#ifndef INTERFERENCE_CONSTANTS_H
#define INTERFERENCE_CONSTANTS_H

#include "interference_types.h"
#include "net/gnrc.h"

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef MODULE_AT86RF215
#define IFACE_SUB_GHZ           4

static const if_phy_cfg_t phy_cfg_sub_ghz[NUM_PHY_CFG_SUB_GHZ] = {
    {IFACE_SUB_GHZ, IEEE802154_PHY_FSK, .fsk_cfg = {50, 200, 64, 2}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_FSK, .fsk_cfg = {100, 400, 64, 2}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {4, 2}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {4, 3}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {3, 1}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {3, 2}}
};

static const if_tx_t TX_40B_SUB_GHZ = {IFACE_SUB_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                 "01234"};
static const if_tx_t TX_80B_SUB_GHZ = {IFACE_SUB_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                 "01234567890123456789"
                                                                                 "01234567890123456789"
                                                                                 "01234"};
static const if_tx_t TX_120B_SUB_GHZ = {IFACE_SUB_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234"};  

#if (GNRC_NETIF_NUMOF >= 2)
#define IFACE_2_4_GHZ           5

static const if_phy_cfg_t phy_cfg_2_4_ghz[NUM_PHY_CFG_2_4_GHZ] = {
    {IFACE_2_4_GHZ, IEEE802154_PHY_FSK, .fsk_cfg = {50, 200, 64, 2}},
    {IFACE_2_4_GHZ, IEEE802154_PHY_FSK, .fsk_cfg = {100, 400, 32, 2}},
    {IFACE_2_4_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {4, 2}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {4, 3}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {3, 1}},
    {IFACE_SUB_GHZ, IEEE802154_PHY_OFDM, .ofdm_cfg = {3, 2}}
};

static const if_tx_t TX_40B_2_4_GHZ = {IFACE_2_4_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                 "01234"};
static const if_tx_t TX_80B_2_4_GHZ = {IFACE_2_4_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                 "01234567890123456789"
                                                                                 "01234567890123456789"
                                                                                 "01234"}; 
static const if_tx_t TX_120B_2_4_GHZ = {IFACE_2_4_GHZ, "22:68:31:23:9D:F1:96:37", "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234567890123456789"
                                                                                  "01234"};  
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* INTERFERENCE_CONSTANTS_H */
/** @} */