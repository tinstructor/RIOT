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

#define ENABLE_DEBUG    (1)
#include "debug.h"

kernel_pid_t pid_tc;
static char stack_tc[THREAD_STACKSIZE_MAIN];
xtimer_ticks32_t last_wup_tc;
xtimer_t trx_phy_cfg_timer;
// xtimer_t if_phy_cfg_timer;

static void *thread_tc_handler(void *arg)
{
    (void) arg;

    msg_t msg;
    msg_t msg_trx = {.type = TC_MSG_TRX_PHY_CFG};
    // msg_t msg_if = {.type = TC_MSG_IF_PHY_CFG};
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);

    xtimer_set_msg(&trx_phy_cfg_timer, PHY_CFG_INTERVAL, &msg_trx, thread_getpid());
    // xtimer_set_msg(&if_phy_cfg_timer, EXP_INTERVAL, &msg_if, thread_getpid());

    while (1) {
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
            switch (msg.type) {
                case TC_MSG_TRX_PHY_CFG:
                    last_wup_tc = xtimer_now();
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
                    xtimer_set_msg(&trx_phy_cfg_timer, PHY_CFG_INTERVAL, &msg_trx, thread_getpid());
                    break;
                
                // case TC_MSG_IF_PHY_CFG:
                //     last_wup_tc = xtimer_now();
                //     gpio_set(IF_PHY_CFG_PIN);
                //     DEBUG("if phy cfg on: %"PRIu32"\n",xtimer_usec_from_ticks(last_wup_tc));
                //     xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
                //     gpio_clear(IF_PHY_CFG_PIN);
                //     DEBUG("if phy cfg off: %"PRIu32"\n", xtimer_usec_from_ticks(last_wup_tc));
                //     xtimer_set_msg(&if_phy_cfg_timer, EXP_INTERVAL, &msg_if, thread_getpid());
                //     break;

                default:
                    break;
            }
        }
    }
    
    return NULL;
}

int main(void)
{
    puts("Welcome to RIOT!");

    last_wup_tc = xtimer_now();

    pid_tc = thread_create(stack_tc, sizeof(stack_tc), 
             THREAD_PRIORITY_MAIN - 1, 0, thread_tc_handler, 
             NULL, "thread tc");

    /* initialize output pins */
    gpio_init(TX_TX_PIN, GPIO_OUT);
    gpio_init(IF_TX_PIN, GPIO_OUT);
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}