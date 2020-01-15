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
 * @brief       Interference application specific definitions of timing control types
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#ifndef TIMING_CONTROL_TYPES_H
#define TIMING_CONTROL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "kernel_types.h"
#include "periph/gpio.h"
#include "mutex.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
    TC_MSG_PHY_CFG,
    TC_MSG_START,
} tc_msg_t;

typedef struct {
    bool can_start;
    bool has_started;
    mutex_t lock;
} tc_start_flag_t;

typedef struct {
    gpio_t first_pin;
    gpio_t second_pin;
    mutex_t lock;
} tc_pin_cfg_t;

typedef struct {
    uint32_t offset;
    mutex_t lock;
} tc_offset_t;

typedef struct {
    uint32_t phy;
    mutex_t lock;
} tc_phy_t;

typedef struct {
    uint32_t numtx;
    mutex_t lock;
} tc_numtx_t;

typedef enum {
    SUN_FSK_OM1,
    SUN_FSK_OM2,
    SUN_OFDM_O4_MCS2,
    SUN_OFDM_O4_MCS3,
    SUN_OFDM_O3_MCS1,
    SUN_OFDM_O3_MCS2,
    NUM_PHY_CFG
} phy_cfg_t;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* TIMING_CONTROL_TYPES_H */
/** @} */