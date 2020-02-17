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
#include "board.h"
#include "periph/gpio.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define TX_SUB_GHZ_PIN           GPIO_PIN(PORT_B, 0)
#define TX_2_4_GHZ_PIN           GPIO_PIN(PORT_B, 1)
#define PHY_CFG_SUB_GHZ_PIN      GPIO_PIN(PORT_B, 2)
#define PHY_CFG_2_4_GHZ_PIN      GPIO_PIN(PORT_B, 3)

#define IF_DEFAULT_CHANNEL       0UL
#define IF_FSK_9_DBM_SUB_GHZ     21
#define IF_OFDM_9_DBM_SUB_GHZ    30
#define IF_FSK_9_DBM_2_4_GHZ     22
#define IF_OFDM_9_DBM_2_4_GHZ    30

#ifdef MODULE_AT86RF215
#define IFACE_SUB_GHZ           4

static const if_phy_cfg_t phy_cfg_sub_ghz[NUM_PHY_CFG_SUB_GHZ] = {
    {IFACE_SUB_GHZ, IF_FSK_9_DBM_SUB_GHZ, IEEE802154_PHY_FSK, "SUN-FSK 863-870MHz OM1", .fsk_cfg = {50, 200, 64, 2}},
    {IFACE_SUB_GHZ, IF_FSK_9_DBM_SUB_GHZ, IEEE802154_PHY_FSK, "SUN-FSK 863-870MHz OM2", .fsk_cfg = {100, 400, 64, 2}},
    {IFACE_SUB_GHZ, IF_OFDM_9_DBM_SUB_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 863-870MHz O4 MCS2", .ofdm_cfg = {4, 2}},
    {IFACE_SUB_GHZ, IF_OFDM_9_DBM_SUB_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 863-870MHz O4 MCS3", .ofdm_cfg = {4, 3}},
    {IFACE_SUB_GHZ, IF_OFDM_9_DBM_SUB_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 863-870MHz O3 MCS1", .ofdm_cfg = {3, 1}},
    {IFACE_SUB_GHZ, IF_OFDM_9_DBM_SUB_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 863-870MHz O3 MCS2", .ofdm_cfg = {3, 2}}
};

#define TX_21B                  "01"
#define TX_120B                 "01234567890123456789"\
                                "01234567890123456789"\
                                "01234567890123456789"\
                                "01234567890123456789"\
                                "01234567890123456789"\
                                "0"

#define TRX_DEST_ADDR           "22:68:31:23:9D:F1:96:37"
#define IF_DEST_ADDR            "22:68:31:23:14:F1:99:37"

#if (GNRC_NETIF_NUMOF >= 2)
#define IFACE_2_4_GHZ           5

static const if_phy_cfg_t phy_cfg_2_4_ghz[NUM_PHY_CFG_2_4_GHZ] = {
    {IFACE_2_4_GHZ, IF_FSK_9_DBM_2_4_GHZ, IEEE802154_PHY_FSK, "SUN-FSK 2.4GHz OM1", .fsk_cfg = {50, 200, 64, 2}},
    {IFACE_2_4_GHZ, IF_FSK_9_DBM_2_4_GHZ, IEEE802154_PHY_FSK, "SUN-FSK 2.4GHz OM2", .fsk_cfg = {100, 400, 32, 2}},
    {IFACE_2_4_GHZ, IF_OFDM_9_DBM_2_4_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 2.4GHz O4 MCS2", .ofdm_cfg = {4, 2}},
    {IFACE_2_4_GHZ, IF_OFDM_9_DBM_2_4_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 2.4GHz O4 MCS3", .ofdm_cfg = {4, 3}},
    {IFACE_2_4_GHZ, IF_OFDM_9_DBM_2_4_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 2.4GHz O3 MCS1", .ofdm_cfg = {3, 1}},
    {IFACE_2_4_GHZ, IF_OFDM_9_DBM_2_4_GHZ, IEEE802154_PHY_OFDM, "SUN-OFDM 2.4GHz O3 MCS2", .ofdm_cfg = {3, 2}}
};

#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* INTERFERENCE_CONSTANTS_H */
/** @} */