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

#include "kernel_types.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
    TC_MSG_PHY_CFG,
} tc_msg_t;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* TIMING_CONTROL_TYPES_H */
/** @} */