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

#define ENABLE_DEBUG    (0)
#include "debug.h"

kernel_pid_t pid;
static char stack[THREAD_STACKSIZE_MAIN];

static void *thread_handler(void *arg)
{
    (void) arg;

    while (1)
    {
        xtimer_sleep(10);
        gpio_set(GPIO_PIN(0,7));
        LED0_ON;
        xtimer_usleep(500);
        gpio_clear(GPIO_PIN(0,7));
        LED0_OFF;
    }
    

    return NULL;
}


int main(void)
{
    (void) puts("Welcome to RIOT!");

    pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                        0, thread_handler, NULL, "thread");

    /* initialize output pins */
    gpio_init(GPIO_PIN(0,7), GPIO_OUT);
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}