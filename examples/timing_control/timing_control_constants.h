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
 * @brief       Timing control application specific definitions of constants
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#ifndef TIMING_CONTROL_CONSTANTS_H
#define TIMING_CONTROL_CONSTANTS_H

#include "timing_control_types.h"
#include "board.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "msg.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define TX_TX_PIN           GPIO_PIN(PORT_A, 7)
#define IF_TX_PIN           GPIO_PIN(PORT_A, 5)
#define TX_PHY_CFG_PIN      GPIO_PIN(PORT_A, 4)
#define RX_PHY_CFG_PIN      GPIO_PIN(PORT_C, 1)
#define IF_PHY_CFG_PIN      GPIO_PIN(PORT_C, 0)

#define NUM_OF_TX           (100UL)
#define NUM_OF_PHY          (6UL)

#define TX_WUP_INTERVAL     (500UL * US_PER_MS)
#define PHY_CFG_INTERVAL    (TX_WUP_INTERVAL * NUM_OF_TX)

#define WAITING_PERIOD_US   (100UL * US_PER_MS)
#define IF_TX_OFFSET_US     (1UL * US_PER_MS)
#define PULSE_DURATION_US   (150UL * US_PER_MS)

static const msg_t msg_phy_cfg = {.type = TC_MSG_PHY_CFG};
static const msg_t msg_start = {.type = TC_MSG_START};

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* TIMING_CONTROL_CONSTANTS_H */
/** @} */