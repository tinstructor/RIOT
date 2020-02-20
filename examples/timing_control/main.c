/*
 * Copyright (C) 2020 Robbe Elsas <robbe.elsas@ugent.be>
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
#include <stdlib.h>

#include "thread.h"
#include "mutex.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "periph/pm.h"
#include "timing_control_constants.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

kernel_pid_t pid_tc;
kernel_pid_t pid_btn;

static char stack_tc[THREAD_STACKSIZE_MAIN];
static char stack_btn[THREAD_STACKSIZE_MAIN];

xtimer_ticks32_t last_wup_tc;
xtimer_ticks32_t last_wup_st;
xtimer_ticks32_t last_wup_si;
xtimer_t phy_cfg_timer;

static tc_start_flag_t start_flag = {.can_start = false, .has_started = false};
static tc_pin_cfg_t if_tx_pin_cfg = {.first_pin = TX_TX_PIN, .second_pin = IF_TX_PIN};
static tc_offset_t if_tx_offset = {.offset = IF_TX_OFFSET_US, .compensation = COMPENSATION_US};
static tc_phy_t if_trx_phy = {.phy = SUN_FSK_OM1};
static tc_phy_t if_if_phy = {.phy = SUN_FSK_OM1};
static tc_numtx_t if_numtx = {.numtx = NUM_OF_TX};
static tc_numphy_t if_if_numphy = {.numphy = NUM_OF_PHY_IF};
static tc_numphy_t if_trx_numphy = {.numphy = NUM_OF_PHY_TRX};

static bool set_can_start(void)
{
    bool succes = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started && !start_flag.can_start) {
        start_flag.can_start = true;
        succes = true;
    }
    mutex_unlock(&start_flag.lock);

    return succes;
}

static bool get_can_start(void)
{
    bool can_start = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started) {
        can_start = start_flag.can_start;
    }
    mutex_unlock(&start_flag.lock);

    return can_start;
}

static bool set_has_started(void)
{
    bool succes = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started && start_flag.can_start) {
        start_flag.has_started = true;
        start_flag.can_start = false;
        succes = true;
    }
    mutex_unlock(&start_flag.lock);

    return succes;
}

static bool get_has_started(void)
{
    bool has_started = false;

    mutex_lock(&start_flag.lock);
    has_started = start_flag.has_started;
    mutex_unlock(&start_flag.lock);

    return has_started;
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

    set_can_start();

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

    // FIXME yielding a thread only causes threads of the same or higher priority to execute.
    // Hence, this thread will be stuck in this loop forever (unless the user button is pressed)
    // because the main thread has lower priority and issuing the "start" command has no effect.
    while (!get_can_start()) { thread_yield(); };
    set_has_started();
    xtimer_sleep(2);

    mutex_lock(&if_trx_phy.lock);
    mutex_lock(&if_if_phy.lock);
    mutex_lock(&if_numtx.lock);
    mutex_lock(&if_if_numphy.lock);
    mutex_lock(&if_trx_numphy.lock);
    msg = msg_phy_cfg;
    last_wup_tc = xtimer_now();
    xtimer_set_msg(&phy_cfg_timer, if_numtx.numtx * TX_WUP_INTERVAL, &msg, thread_getpid());
    while (experiments < if_if_numphy.numphy) {
        mutex_lock(&if_tx_offset.lock);
        mutex_lock(&if_tx_pin_cfg.lock);
        uint32_t compensated_offset = labs(if_tx_offset.offset + if_tx_offset.compensation);
        xtimer_periodic_wakeup(&last_wup_tc, TX_WUP_INTERVAL - compensated_offset - PULSE_DURATION_US);
        gpio_set(if_tx_pin_cfg.first_pin);
        xtimer_periodic_wakeup(&last_wup_tc, compensated_offset);
        gpio_set(if_tx_pin_cfg.second_pin);
        xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US - compensated_offset);
        gpio_clear(if_tx_pin_cfg.first_pin);
        xtimer_periodic_wakeup(&last_wup_tc, compensated_offset);
        gpio_clear(if_tx_pin_cfg.second_pin);
        mutex_unlock(&if_tx_pin_cfg.lock);
        mutex_unlock(&if_tx_offset.lock);
        if (msg_try_receive(&msg) == 1) {
            xtimer_periodic_wakeup(&last_wup_tc, WAITING_PERIOD_US);
            gpio_set(TX_PHY_CFG_PIN);
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(TX_PHY_CFG_PIN);
            gpio_set(RX_PHY_CFG_PIN);
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(RX_PHY_CFG_PIN);

            phy_reconfigs++;

            if (phy_reconfigs >= if_trx_numphy.numphy) {
                gpio_set(IF_PHY_CFG_PIN);
                xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
                gpio_clear(IF_PHY_CFG_PIN);

                experiments++;
                phy_reconfigs = 0;
            }

            xtimer_periodic_wakeup(&last_wup_tc, WAITING_PERIOD_US);
            xtimer_set_msg(&phy_cfg_timer, if_numtx.numtx * TX_WUP_INTERVAL, &msg, thread_getpid());
        }
    }
    mutex_unlock(&if_trx_numphy.lock);
    mutex_unlock(&if_if_numphy.lock);
    mutex_unlock(&if_numtx.lock);
    mutex_unlock(&if_if_phy.lock);
    mutex_unlock(&if_trx_phy.lock);

    return NULL;
}

static int start_handler(int argc, char **argv)
{
    if (argc != 1) {
        printf("usage: %s\n", argv[0]);
        return 1;
    }

    set_can_start();

    return 0;
}

static int offset_handler(int argc, char **argv)
{
    if (argc != 2 || atol(argv[1]) == 0) {
        printf("usage: %s <useconds (non-zero)>\n", argv[0]);
        return 1;
    }

    if (!get_has_started()) {
        int64_t raw_offset = atol(argv[1]);
        mutex_lock(&if_tx_offset.lock);
        if_tx_offset.offset = raw_offset;
        mutex_lock(&if_tx_pin_cfg.lock);
        if_tx_pin_cfg.first_pin = (raw_offset + if_tx_offset.compensation < 0 ? IF_TX_PIN : TX_TX_PIN);
        if_tx_pin_cfg.second_pin = (raw_offset + if_tx_offset.compensation < 0 ? TX_TX_PIN : IF_TX_PIN);
        mutex_unlock(&if_tx_pin_cfg.lock);
        mutex_unlock(&if_tx_offset.lock);
    }
    else {
        DEBUG("experiment has already started: can't set offset\n");
    }

    return 0;
}

static int comp_handler(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: %s <useconds>\n", argv[0]);
        return 1;
    }

    if (!get_has_started()) {
        int32_t compensation = atoi(argv[1]);
        mutex_lock(&if_tx_offset.lock);
        if_tx_offset.compensation = compensation;
        mutex_lock(&if_tx_pin_cfg.lock);
        if_tx_pin_cfg.first_pin = (if_tx_offset.offset + compensation < 0 ? IF_TX_PIN : TX_TX_PIN);
        if_tx_pin_cfg.second_pin = (if_tx_offset.offset + compensation < 0 ? TX_TX_PIN : IF_TX_PIN);
        mutex_unlock(&if_tx_pin_cfg.lock);
        mutex_unlock(&if_tx_offset.lock);
    }
    else {
        DEBUG("experiment has already started: can't set offset\n");
    }

    return 0;
}

static int numtx_handler(int argc, char **argv)
{
    if (argc != 2 || atoi(argv[1]) <= 0) {
        printf("usage: %s <#messages>\n", argv[0]);
        return 1;
    }

    if (!get_has_started()) {
        mutex_lock(&if_numtx.lock);
        if_numtx.numtx = atoi(argv[1]);
        mutex_unlock(&if_numtx.lock);
    }
    else {
        DEBUG("experiment has already started: can't set numtx\n");
    }

    return 0;
}

static int numphy_handler(int argc, char **argv)
{
    if (argc != 2 || atoi(argv[1]) <= 0) {
        printf("usage: %s <# of %s phy configs>\n",argv[0],(!strcmp("ifnumphy",argv[0]) ? "IF" : "TRX"));
        return 1;
    }

    if (!get_has_started()) {
        uint8_t numphy = atoi(argv[1]);

        if (!strcmp("ifnumphy",argv[0])) {
            // NOTE enters this block when strings are equal
            if (numphy <= NUM_PHY_CFG) {
                mutex_lock(&if_if_numphy.lock);
                if_if_numphy.numphy = numphy;
                mutex_unlock(&if_if_numphy.lock);
            }
            else {
                DEBUG("IF phy config index %d out of range\n",numphy);
            }
        }
        else {
            // NOTE enters this block when strings are not equal
            if (numphy <= NUM_PHY_CFG) {
                mutex_lock(&if_trx_numphy.lock);
                if_trx_numphy.numphy = numphy;
                mutex_unlock(&if_trx_numphy.lock);
            }
            else {
                DEBUG("TRX phy index %d out of range\n",numphy);
            }
        }
    }
    else {
        DEBUG("experiment has already started: can't set # of %s phy configs\n", !strcmp("ifphy",argv[0]) ? "IF" : "TRX");
    }

    return 0;
}

static int phy_handler(int argc, char **argv)
{
    if (argc != 2 || atoi(argv[1]) <= 0) {
        printf("usage: %s <index>\n", argv[0]);
        return 1;
    }

    if (!get_has_started()) {
        uint8_t index = atoi(argv[1]);

        if (!strcmp("ifphy",argv[0])) {
            // NOTE enters this block when strings are equal
            if (index <= NUM_PHY_CFG) {
                mutex_lock(&if_if_phy.lock);
                uint8_t pulses = (NUM_PHY_CFG - if_if_phy.phy + index) % NUM_PHY_CFG;
                if_if_phy.phy = index;
                last_wup_si = xtimer_now();
                for (size_t i = 0; i < pulses; i++) {
                    gpio_set(IF_PHY_CFG_PIN);
                    xtimer_periodic_wakeup(&last_wup_si, PULSE_DURATION_US);
                    gpio_clear(IF_PHY_CFG_PIN);
                    xtimer_periodic_wakeup(&last_wup_si, PHY_CFG_INTERVAL);
                }
                mutex_unlock(&if_if_phy.lock);
            }
            else {
                DEBUG("IF phy config index %d out of range\n",index);
            }
        }
        else {
            // NOTE enters this block when strings are not equal
            if (index <= NUM_PHY_CFG) {
                mutex_lock(&if_trx_phy.lock);
                uint8_t pulses = (NUM_PHY_CFG - if_trx_phy.phy + index) % NUM_PHY_CFG;
                if_trx_phy.phy = index;
                last_wup_st = xtimer_now();
                for (size_t i = 0; i < pulses; i++) {
                    gpio_set(TX_PHY_CFG_PIN);
                    xtimer_periodic_wakeup(&last_wup_st, PULSE_DURATION_US);
                    gpio_clear(TX_PHY_CFG_PIN);
                    gpio_set(RX_PHY_CFG_PIN);
                    xtimer_periodic_wakeup(&last_wup_st, PULSE_DURATION_US);
                    gpio_clear(RX_PHY_CFG_PIN);
                    xtimer_periodic_wakeup(&last_wup_st, PHY_CFG_INTERVAL);
                }
                mutex_unlock(&if_trx_phy.lock);
            }
            else {
                DEBUG("TRX phy index %d out of range\n",index);
            }
        }
    }
    else {
        DEBUG("experiment has already started: can't set %s phy config\n", !strcmp("ifphy",argv[0]) ? "IF" : "TRX");
    }

    return 0;
}

static const shell_command_t shell_commands[] = {
    {"start", "starts the interference experiment", start_handler},
    {"offset", "sets an IF/TX offset (in microseconds)", offset_handler},
    {"comp", "sets an IF/TX offset compensation (in microseconds)", comp_handler},
    {"numtx", "sets the number of messages transmitted for each IF/TX PHY combination", numtx_handler},
    {"ifnumphy", "sets the number of IF PHY reconfigurations", numphy_handler},
    {"trxnumphy", "sets the number of TRX PHY reconfigurations", numphy_handler},
    {"ifphy", "set the IF phy configuration", phy_handler},
    {"trxphy", "set the TRX phy configuration", phy_handler},
    {NULL, NULL, NULL}
};

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
    
    // pid_btn = thread_create(stack_btn, sizeof(stack_btn), 
    //           THREAD_PRIORITY_MAIN - 1, 0, thread_btn_handler, 
    //           NULL, "thread btn");
    pid_btn = thread_create(stack_btn, sizeof(stack_btn), 
              THREAD_PRIORITY_MAIN, 0, thread_btn_handler, 
              NULL, "thread btn");

    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_RISING, gpio_cb, NULL);

    // FIXME thread priority is higher than main thread and therefore higher than shell priority.
    // This causes the tc thread to hang when checking the status of the can_start flag because
    // yielding the thread only allows threads of the same or higher priority to execute instead.
    // Previously this worked because the btn & ua threads had the same priority as the tc thread
    // (1 priority above main). Threads of the same priority execute in chronological order and
    // although both these threads started before the tc thread, they blocked immediately, awaiting 
    // a msg originating from an isr. Hence, the tc thread was reached, constantly yealding to the
    // other 2 threads of the same priority if they were awaiting execution because they had received
    // a message from an isr.
    // pid_tc = thread_create(stack_tc, sizeof(stack_tc), 
    //          THREAD_PRIORITY_MAIN - 1, 0, thread_tc_handler, 
    //          NULL, "thread tc");
    pid_tc = thread_create(stack_tc, sizeof(stack_tc), 
             THREAD_PRIORITY_MAIN, 0, thread_tc_handler, 
             NULL, "thread tc");
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}