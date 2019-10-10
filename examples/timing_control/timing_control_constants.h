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

#ifdef __cplusplus
 extern "C" {
#endif

#define TX_TX_PIN           GPIO_PIN(PORT_A, 7)
#define IF_TX_PIN           GPIO_PIN(PORT_C, 2)
#define TX_PHY_CFG_PIN      GPIO_PIN(PORT_A, 6)
#define RX_PHY_CFG_PIN      GPIO_PIN(PORT_D, 0)
#define IF_PHY_CFG_PIN      GPIO_PIN(PORT_C, 3)

#define NUM_OF_TX           (20UL)
#define TX_WUP_INTERVAL     (1UL * US_PER_SEC)
#define IF_TX_OFFSET_US     (100UL)
#define PULSE_DURATION_US   (100UL * US_PER_MS)
#define PHY_CFG_INTERVAL    (TX_WUP_INTERVAL * NUM_OF_TX)   

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* TIMING_CONTROL_CONSTANTS_H */
/** @} */