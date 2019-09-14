/*
 * Copyright (C) 2019 Benjamin Valentin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_telnetd telnet remote shell
 * @ingroup     net_telnetd
 * @brief       Provides a remote shell using the telnet protocol
 * @{
 *
 * @file
 * @brief       Telnet implementation
 *
 * @author      Benjamin Valentin <benpicco@googlemail.com>
 */

#include <isrpipe.h>

void telnetd_init(isrpipe_t *isrpipe);
int telnetd_start(int port);
int telnetd_write(const void* buffer, size_t len);
