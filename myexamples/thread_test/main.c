/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       IPC pingpong application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "thread.h"
#include "msg.h"
#include "vtimer.h"

#define TID(pid) thread_get(pid)->name, pid
#define TIP TID(thread_getpid())

void *third_thread(void *arg) {
    (void) arg;

    printf("[%s|%i] thread started.\n", TIP);
    msg_t m;

    while (1) {
        msg_receive(&m);
        printf("        [%s|%i] Got msg from [%s|%i]\n", TIP, TID(m.sender_pid));
        printf("        [%s|%i] Sleep %lu usecs...\n", TIP, m.content.value);
        vtimer_usleep(m.content.value);
        printf("        [%s|%i] Wakeup.\n", TIP);
        msg_reply(&m, &m);
    }
    return NULL;
}

void *second_thread(void *arg)
{
    kernel_pid_t pid3 = *((kernel_pid_t*) arg);

    printf("[%s|%i] thread started.\n", TIP);
    msg_t m, m3;

    while (1) {
        msg_receive(&m);
        printf("    [%s|%i] Got msg from [%s|%i]\n", TIP, TID(m.sender_pid));
        m3.content.value = m.content.value;
        msg_send_receive(&m3, &m3, pid3);
        printf("    [%s|%i] Got reply from [%s|%i]\n", TIP, TID(pid3));
        msg_reply(&m, &m);
    }

    return NULL;
}

char second_thread_stack[KERNEL_CONF_STACKSIZE_MAIN];
char third_thread_stack[KERNEL_CONF_STACKSIZE_MAIN];

int main(void)
{
    printf("Starting IPC Ping-pong example...\n");
    printf("[%s|%i] thread started.\n", TIP);

    msg_t m;

    kernel_pid_t pid3 = thread_create(third_thread_stack, sizeof(third_thread_stack),
                            PRIORITY_MAIN - 1, CREATE_STACKTEST,
                            third_thread, NULL, "THREAD#3");

    kernel_pid_t pid2 = thread_create(second_thread_stack, sizeof(second_thread_stack),
                            PRIORITY_MAIN - 1, CREATE_STACKTEST,
                            second_thread, &pid3, "THREAD#2");

    m.content.value = 1000000;

    while (1) {
        printf("[%s|%i] Send to [%s|%i]\n", TIP, TID(pid2));
        msg_send_receive(&m, &m, pid2);
        printf("[%s|%i] Got reply from [%s|%i]\n", TIP, TID(pid2));
    }
}
