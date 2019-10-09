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
 * @brief       Application used for interference testing
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "thread.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "timing_control_constants.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

kernel_pid_t pid_tx;
// kernel_pid_t pid_phy_cfg;

static char stack_tx[THREAD_STACKSIZE_MAIN];
// static char stack_phy_cfg[THREAD_STACKSIZE_MAIN];

static xtimer_ticks32_t last_wakeup_tx_tx;
static xtimer_ticks32_t last_wakeup_if_tx;
static xtimer_ticks32_t last_wakeup_tx_phy_cfg;
static xtimer_ticks32_t last_wakeup_rx_phy_cfg;
static xtimer_ticks32_t last_wakeup_if_phy_cfg;

static void init_timers(void) 
{
    xtimer_ticks32_t now = xtimer_now();
    xtimer_ticks32_t if_tx_offset_32 = xtimer_ticks_from_usec(IF_TX_OFFSET_US);

    last_wakeup_tx_tx = now;
    last_wakeup_if_tx = now;

    last_wakeup_if_tx.ticks32 += if_tx_offset_32.ticks32;

    last_wakeup_tx_phy_cfg = now;
    last_wakeup_rx_phy_cfg = now;
    last_wakeup_if_phy_cfg = now;
}

static void *thread_tx_handler(void *arg)
{
    (void) arg;

    while (1)
    {
        xtimer_periodic_wakeup(&last_wakeup_tx_tx, TX_WUP_INTERVAL);
        gpio_set(TX_TX_PIN);
        LED0_ON;
        xtimer_usleep(100);
        gpio_clear(GPIO_PIN(PORT_A,7));
        LED0_OFF;
    }
    
    return NULL;
}

int main(void)
{
    init_timers();

    puts("Welcome to RIOT!");

    pid_tx = thread_create(stack_tx, sizeof(stack_tx), 
             THREAD_PRIORITY_MAIN - 1, 0, thread_tx_handler, 
             NULL, "thread tx");

    /* initialize output pins */
    gpio_init(GPIO_PIN(PORT_A,7), GPIO_OUT);
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}