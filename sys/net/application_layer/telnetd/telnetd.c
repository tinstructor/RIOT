/*
 * Copyright (C) 2019 Benjamin Valentin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "net/af.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/tcp.h"
#include "isrpipe.h"

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

#define EOT 0x4

static isrpipe_t *_isrpipe;
static gnrc_tcp_tcb_t *_tcb;
static char telnetd_stack[3072];

static void *telnetd(void *arg)
{
    uint32_t port = *(uint32_t*) arg;

    puts("telnetd started");

    while (1) {
        uint8_t buffer[64];

        gnrc_tcp_tcb_t tcb;
        gnrc_tcp_tcb_init(&tcb);

        puts("starting server");

        int ret = gnrc_tcp_open_passive(&tcb, AF_INET6, NULL, port);
        if (ret) {
            printf("gnrc_tcp_open_passive: %d\n", ret);
            return NULL;
        }

        puts("connected.");
        _tcb = &tcb;

        do {
            ret = gnrc_tcp_recv(&tcb, buffer, sizeof(buffer),
                                GNRC_TCP_CONNECTION_TIMEOUT_DURATION);

            printf("got %d bytes [%x]\n", ret, buffer[0]);

            for (int i = 0; i < ret; ++i) {
                isrpipe_write_one(_isrpipe, buffer[i]);
            }

            if (buffer[0] == EOT) {
                break;
            }
        } while (ret > 0);

        _tcb = NULL;

        gnrc_tcp_abort(&tcb);
        gnrc_tcp_close(&tcb);
    }

    return NULL;
}

int telnetd_write(const void* buffer, size_t len)
{
    int res;

    if (_tcb == NULL) {
        return 0;
    }

    res = gnrc_tcp_send(_tcb, buffer, len,
        GNRC_TCP_CONNECTION_TIMEOUT_DURATION);

    if (res < 0) {
        _tcb = NULL;
        printf("res: %d\n", res);
        return 0;
    }

    return len;
}

void telnetd_init(isrpipe_t *isrpipe)
{
    _isrpipe = isrpipe;
}

int telnetd_start(int port)
{
    thread_create(telnetd_stack, sizeof(telnetd_stack),
                  THREAD_PRIORITY_MAIN - 1, 0, telnetd,
                  &port, "telnet");

    return 0;
}
