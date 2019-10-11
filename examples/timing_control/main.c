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
#include <stdbool.h>

#include "thread.h"
#include "mutex.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "timing_control_constants.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

kernel_pid_t pid_tc;
kernel_pid_t pid_btn;

static char stack_tc[THREAD_STACKSIZE_MAIN];
static char stack_btn[THREAD_STACKSIZE_MAIN];

xtimer_ticks32_t last_wup_tc;
xtimer_t phy_cfg_timer;

static tc_flag_t start_flag = {.flag = false};

static bool allowed_to_start(void)
{
    bool allowed = false;

    mutex_lock(&start_flag.lock);
    allowed = start_flag.flag;
    start_flag.flag = false;
    mutex_unlock(&start_flag.lock);

    return allowed;
}

static void gpio_cb(void *arg)
{
    (void) arg;

    msg_t msg;
    msg = msg_start;
    msg_send(&msg, pid_btn);
}

static void *thread_btn_handler(void *arg)
{
    (void) arg;
    msg_t msg;
    msg_receive(&msg);

    mutex_lock(&start_flag.lock);
    start_flag.flag = true;
    mutex_unlock(&start_flag.lock);

    return NULL;
}

static void *thread_tc_handler(void *arg)
{
    (void) arg;

    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);

    uint8_t phy_reconfigs = 0;
    uint8_t experiments = 0;

    while (!allowed_to_start()) { thread_yield(); };

    msg = msg_phy_cfg;
    last_wup_tc = xtimer_now();
    xtimer_set_msg(&phy_cfg_timer, PHY_CFG_INTERVAL, &msg, thread_getpid());

    while (experiments <= NUM_OF_EXP) {
        xtimer_periodic_wakeup(&last_wup_tc, TX_WUP_INTERVAL - IF_TX_OFFSET_US - PULSE_DURATION_US);
        gpio_set(TX_TX_PIN);
        DEBUG("tx tx on: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
        xtimer_periodic_wakeup(&last_wup_tc, IF_TX_OFFSET_US);
        gpio_set(IF_TX_PIN);
        DEBUG("if tx on: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
        xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US - IF_TX_OFFSET_US);
        gpio_clear(TX_TX_PIN);
        DEBUG("tx tx off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
        xtimer_periodic_wakeup(&last_wup_tc, IF_TX_OFFSET_US);
        gpio_clear(IF_TX_PIN);
        DEBUG("if tx off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
        if (msg_try_receive(&msg) == 1) {
            xtimer_periodic_wakeup(&last_wup_tc, WAITING_PERIOD_US);
            gpio_set(TX_PHY_CFG_PIN);
            DEBUG("tx phy cfg on: %"PRIu32"\n",xtimer_usec_from_ticks(last_wup_tc));
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(TX_PHY_CFG_PIN);
            DEBUG("tx phy cfg off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
            gpio_set(RX_PHY_CFG_PIN);
            DEBUG("rx phy cfg on: %"PRIu32"\n",xtimer_usec_from_ticks(last_wup_tc));
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(RX_PHY_CFG_PIN);
            DEBUG("rx phy cfg off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));

            phy_reconfigs++;

            if (phy_reconfigs >= NUM_OF_PHY) {
                gpio_set(IF_PHY_CFG_PIN);
                DEBUG("if phy cfg on: %"PRIu32"\n",xtimer_usec_from_ticks(last_wup_tc));
                xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
                gpio_clear(IF_PHY_CFG_PIN);
                DEBUG("if phy cfg off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));

                experiments++;
                phy_reconfigs = 0;
            }

            xtimer_set_msg(&phy_cfg_timer, PHY_CFG_INTERVAL, &msg, thread_getpid());
        }
    }
    
    return NULL;
}

int main(void)
{
    puts("Welcome to RIOT!");

    /* initialize output pins */
    gpio_init(TX_TX_PIN, GPIO_OUT);
    gpio_init(IF_TX_PIN, GPIO_OUT);
    gpio_init(TX_PHY_CFG_PIN, GPIO_OUT);
    gpio_init(RX_PHY_CFG_PIN, GPIO_OUT);
    gpio_init(IF_PHY_CFG_PIN, GPIO_OUT);

    /* Clear output pins */
    gpio_clear(TX_TX_PIN);
    gpio_clear(IF_TX_PIN);
    gpio_clear(TX_PHY_CFG_PIN);
    gpio_clear(RX_PHY_CFG_PIN);
    gpio_clear(IF_PHY_CFG_PIN);
    
    pid_btn = thread_create(stack_btn, sizeof(stack_btn), 
              THREAD_PRIORITY_MAIN - 1, 0, thread_btn_handler, 
              NULL, "thread btn");

    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_RISING, gpio_cb, NULL);

    pid_tc = thread_create(stack_tc, sizeof(stack_tc), 
             THREAD_PRIORITY_MAIN - 1, 0, thread_tc_handler, 
             NULL, "thread tc");
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}