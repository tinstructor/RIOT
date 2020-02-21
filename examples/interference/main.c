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
#include "mutex.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "interference_types.h"
#include "interference_constants.h"

#include "periph/gpio.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

kernel_pid_t pid;
static char stack[THREAD_STACKSIZE_MAIN];
static gnrc_netif_t *netif = NULL;

#ifdef MODULE_AT86RF215
static if_tx_t tx_sub_ghz = {.iface = IFACE_SUB_GHZ, .dest = TRX_DEST_ADDR, .payload = TX_120B};
static phy_cfg_idx_sub_ghz_t current_phy_cfg_idx_sub_ghz;
#if (GNRC_NETIF_NUMOF >= 2)
static if_tx_t tx_2_4_ghz = {.iface = IFACE_2_4_GHZ, .dest = TRX_DEST_ADDR, .payload = TX_120B};
static phy_cfg_idx_2_4_ghz_t current_phy_cfg_idx_2_4_ghz;
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

static inline bool _is_iface(kernel_pid_t iface)
{
    return (gnrc_netif_get_by_pid(iface) != NULL);
}

#ifdef MODULE_AT86RF215
static uint8_t set_ofdm_opt(uint8_t iface, uint8_t opt) 
{
    if (gnrc_netapi_set(iface, NETOPT_OFDM_OPTION, 0, &opt, sizeof(opt)) < 0) {
        DEBUG("Unable to set OFDM option to %d\n", opt);
        return 1;
    }

    return 0;
}

static uint8_t set_ofdm_mcs(uint8_t iface, uint8_t mcs) 
{
    if (gnrc_netapi_set(iface, NETOPT_OFDM_MCS, 0, &mcs, sizeof(mcs)) < 0) {
        DEBUG("Unable to set OFDM scheme to %d\n", mcs);
        return 1;
    }

    return 0;
}

static uint8_t set_phy(if_phy_cfg_t *phy_cfg)
{
    int res = 0;
    uint8_t current_opt = 0;
    uint8_t next_opt = 0;
    uint16_t default_channel = IF_DEFAULT_CHANNEL;

    if (!_is_iface(phy_cfg->iface)) {
        DEBUG("error: invalid interface given\n");
        return 1;
    }

    res = gnrc_netapi_set(phy_cfg->iface, NETOPT_TX_POWER, 0, 
        &phy_cfg->pac_txpwr, sizeof(phy_cfg->pac_txpwr));
    if (res < 0) {
        DEBUG("Unable to set PAC.TXPWR to %d\n", phy_cfg->pac_txpwr);
        return 1;
    }

    res = gnrc_netapi_set(phy_cfg->iface, NETOPT_IEEE802154_PHY, 0,
        &phy_cfg->ieee802154_phy, sizeof(phy_cfg->ieee802154_phy));
    if (res < 0) {
        DEBUG("Unable to set IEEE802154 PHY to %s\n", phy_cfg->phy_descriptor);
        return 1;
    }

    switch (phy_cfg->ieee802154_phy) {
        case IEEE802154_PHY_FSK:
            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_SRATE, 0, 
                &phy_cfg->fsk_cfg.srate, sizeof(phy_cfg->fsk_cfg.srate));
            if (res < 0) {
                DEBUG("Unable to set FSK srate to %d\n", phy_cfg->fsk_cfg.srate);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_CHANNEL_SPACING, 0, 
                &phy_cfg->fsk_cfg.cspace, sizeof(phy_cfg->fsk_cfg.cspace));
            if (res < 0) {
                DEBUG("Unable to set FSK cspace to %d\n", phy_cfg->fsk_cfg.cspace);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_MODULATION_INDEX, 0, 
                &phy_cfg->fsk_cfg.mod_idx, sizeof(phy_cfg->fsk_cfg.mod_idx));
            if (res < 0) {
                DEBUG("Unable to set FSK modulation index to %d\n", phy_cfg->fsk_cfg.mod_idx);
                return 1;
            }

            res = gnrc_netapi_set(phy_cfg->iface, NETOPT_FSK_MODULATION_ORDER, 0, 
                &phy_cfg->fsk_cfg.mod_ord, sizeof(phy_cfg->fsk_cfg.mod_ord));
            if (res < 0) {
                DEBUG("Unable to set FSK modulation order to %d\n", phy_cfg->fsk_cfg.mod_ord);
                return 1;
            }
            break;
        
        case IEEE802154_PHY_OFDM:
            //TODO: more robust way of preventing impossible combos
            res = gnrc_netapi_get(phy_cfg->iface, NETOPT_OFDM_OPTION, 0, &current_opt, sizeof(current_opt));
            if (res < 0) {
                DEBUG("Unable to retrieve currently configured OFDM option\n");
                return 1;
            }

            next_opt = phy_cfg->ofdm_cfg.option;
            if (next_opt > current_opt) {
                set_ofdm_mcs(phy_cfg->iface, phy_cfg->ofdm_cfg.mcs);
                set_ofdm_opt(phy_cfg->iface, next_opt);
            } else {
                set_ofdm_opt(phy_cfg->iface, next_opt);
                set_ofdm_mcs(phy_cfg->iface, phy_cfg->ofdm_cfg.mcs);
            }
            break;

        default:
            DEBUG("PHY \"%s\" not supported\n", phy_cfg->phy_descriptor);
            return 1;
    }

    DEBUG("PHY reconfigured to %s\n", phy_cfg->phy_descriptor);

    // TODO use a res and debug structure here
    gnrc_netapi_set(phy_cfg->iface, NETOPT_CHANNEL, 0, &default_channel, sizeof(default_channel));

    return 0;
}

static uint8_t set_phy_sub_ghz(phy_cfg_idx_sub_ghz_t phy_cfg_idx)
{
    if (phy_cfg_idx >= NUM_PHY_CFG_SUB_GHZ) {
        DEBUG("Illegal PHY config index\n");
        return 1;
    }

    current_phy_cfg_idx_sub_ghz = phy_cfg_idx;
    if_phy_cfg_t new_phy_cfg = phy_cfg_sub_ghz[phy_cfg_idx];

    return set_phy(&new_phy_cfg);
}

static uint8_t init_phy_sub_ghz(void) 
{
    // initialize the first available PHY from interference_types.h
    return set_phy_sub_ghz(0);
}

static uint8_t next_phy_sub_ghz(void) 
{
    return set_phy_sub_ghz((current_phy_cfg_idx_sub_ghz + 1) % NUM_PHY_CFG_SUB_GHZ);
}

#if (GNRC_NETIF_NUMOF >= 2)
static uint8_t set_phy_2_4_ghz(phy_cfg_idx_2_4_ghz_t phy_cfg_idx)
{
    if (phy_cfg_idx >= NUM_PHY_CFG_2_4_GHZ) {
        DEBUG("Illegal PHY config index\n");
        return 1;
    }

    current_phy_cfg_idx_2_4_ghz = phy_cfg_idx;
    if_phy_cfg_t new_phy_cfg = phy_cfg_2_4_ghz[phy_cfg_idx];

    return set_phy(&new_phy_cfg);
}

static uint8_t init_phy_2_4_ghz(void) 
{
    // initialize the first available PHY from interference_types.h
    return set_phy_2_4_ghz(0);
}

static uint8_t next_phy_2_4_ghz(void) 
{
    return set_phy_2_4_ghz((current_phy_cfg_idx_2_4_ghz + 1) % NUM_PHY_CFG_2_4_GHZ);
}
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

static uint8_t send(kernel_pid_t iface, const char *dest, const char *payload) 
{
    size_t addr_len;
    uint8_t addr[GNRC_NETIF_L2ADDR_MAXLEN];
    gnrc_pktsnip_t *pkt, *hdr;
    gnrc_netif_hdr_t *nethdr;
    uint8_t flags = 0x00;

    if (!_is_iface(iface)) {
        DEBUG("error: invalid interface given\n");
        return 1;
    }

    /* parse address */
    addr_len = gnrc_netif_addr_from_str(dest, addr);

    if (addr_len == 0) {
        DEBUG("error: invalid address given");
        return 1;
    }

    /* put packet together */
    pkt = gnrc_pktbuf_add(NULL, payload, strlen(payload), GNRC_NETTYPE_UNDEF);
    if (pkt == NULL) {
        DEBUG("error: packet buffer full");
        return 1;
    }
    hdr = gnrc_netif_hdr_build(NULL, 0, addr, addr_len);
    if (hdr == NULL) {
        DEBUG("error: packet buffer full");
        gnrc_pktbuf_release(pkt);
        return 1;
    }
    LL_PREPEND(pkt, hdr);
    nethdr = (gnrc_netif_hdr_t *)hdr->data;
    nethdr->flags = flags;
    /* and send it */
    if (gnrc_netapi_send(iface, pkt) < 1) {
        DEBUG("error: unable to send");
        gnrc_pktbuf_release(pkt);
        return 1;
    }

    return 0;
}

static void gpio_cb(void *arg)
{
    msg_t msg = *(msg_t *)arg;
    msg_send(&msg, pid);
}

static void *thread_handler(void *arg)
{
    (void) arg;

    while (1) {
        msg_t msg;
        msg_receive(&msg);
        switch (msg.type) {
            case IF_MSG_TX_SUB_GHZ:
#ifdef MODULE_AT86RF215
                mutex_lock(&tx_sub_ghz.lock);
                send(tx_sub_ghz.iface, tx_sub_ghz.dest, tx_sub_ghz.payload);
                mutex_unlock(&tx_sub_ghz.lock);
#endif /* MODULE_AT86RF215 */  
                break;
            
            case IF_MSG_TX_2_4_GHZ:
#ifdef MODULE_AT86RF215
#if (GNRC_NETIF_NUMOF >= 2)
                mutex_lock(&tx_2_4_ghz.lock);
                send(tx_2_4_ghz.iface, tx_2_4_ghz.dest, tx_2_4_ghz.payload);
                mutex_unlock(&tx_2_4_ghz.lock);
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */  
                break;
    
            case IF_MSG_PHY_CFG_SUB_GHZ:
#ifdef MODULE_AT86RF215
                next_phy_sub_ghz();
#else
                DEBUG("PHY is not configurable\n");
#endif /* MODULE_AT86RF215 */      
                break;

            case IF_MSG_PHY_CFG_2_4_GHZ:
#ifdef MODULE_AT86RF215
#if (GNRC_NETIF_NUMOF >= 2)
                next_phy_2_4_ghz();
#else
                DEBUG("2.4GHz interface not available\n");
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#else
                DEBUG("PHY is not configurable\n");
#endif /* MODULE_AT86RF215 */             
                break;

            default:
                break;
        }
    }

    return NULL;
}

static int numbyte_handler(int argc, char **argv)
{
    if (argc != 2 || atoi(argv[1]) < 1 || atoi(argv[1]) > 128) {
        printf("usage: %s <# of %s L2 payload bytes [1-128]>\n",argv[0],(!strcmp("numbytesub",argv[0]) ? "sub-GHz" : "2.4GHz"));
        return 1;
    }

#ifdef MODULE_AT86RF215
    if (!strcmp("numbytesub",argv[0])) {
        // NOTE enters this block when strings are equal
        uint8_t payload_size = atoi(argv[1]);
        char *payload = malloc(sizeof(char) * payload_size + 1);
        for (uint8_t i = 0; i < payload_size; i++) {
            memset(payload + i, (i % 10) +'0', 1);
        }
        payload[payload_size] = '\0';
        mutex_lock(&tx_sub_ghz.lock);
        strcpy(tx_sub_ghz.payload, payload);
        mutex_unlock(&tx_sub_ghz.lock);
        free(payload);
    }
#if (GNRC_NETIF_NUMOF >= 2)
    else {
        // NOTE enters this block when strings are not equal
        uint8_t payload_size = atoi(argv[1]);
        char *payload = malloc(sizeof(char) * payload_size + 1);
        for (uint8_t i = 0; i < payload_size; i++) {
            memset(payload + i, (i % 10) +'0', 1);
        }
        payload[payload_size] = '\0';
        mutex_lock(&tx_2_4_ghz.lock);
        strcpy(tx_2_4_ghz.payload, payload);
        mutex_unlock(&tx_2_4_ghz.lock);
        free(payload);
    }
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */   

    return 0;
}

static int taddr_handler(int argc, char **argv)
{
    if (argc != 1) {
        printf("usage: %s\n", argv[0]);
        return 1;
    }

#ifdef MODULE_AT86RF215
    if (!strcmp("taddrsub",argv[0])) {
        // NOTE enters this block when strings are equal
        mutex_lock(&tx_sub_ghz.lock);
        strcpy(tx_sub_ghz.dest, !strcmp(TRX_DEST_ADDR, tx_sub_ghz.dest) ? IF_DEST_ADDR : TRX_DEST_ADDR);
        mutex_unlock(&tx_sub_ghz.lock);
    }
#if (GNRC_NETIF_NUMOF >= 2)
    else {
        // NOTE enters this block when strings are not equal
        mutex_lock(&tx_2_4_ghz.lock);
        strcpy(tx_2_4_ghz.dest, !strcmp(TRX_DEST_ADDR, tx_2_4_ghz.dest) ? IF_DEST_ADDR : TRX_DEST_ADDR);
        mutex_unlock(&tx_2_4_ghz.lock);
    }
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */   

    return 0;
}

static int saddr_handler(int argc, char **argv)
{
    if (argc != 2 || strlen(argv[1]) != 23) {
        printf("usage: %s <address string>\n",argv[0]);
        return 1;
    }

    uint8_t foo[GNRC_NETIF_L2ADDR_MAXLEN];
    if (!gnrc_netif_addr_from_str(argv[1], foo)) {
        DEBUG("Incorrectly formatted address string was passed!\n");
        printf("usage: %s <address string>\n",argv[0]);
        return 1;
    }

#ifdef MODULE_AT86RF215
    if (!strcmp("saddrsub",argv[0])) {
        // NOTE enters this block when strings are equal
        mutex_lock(&tx_sub_ghz.lock);
        strcpy(tx_sub_ghz.dest, argv[1]);
        mutex_unlock(&tx_sub_ghz.lock);
    }
#if (GNRC_NETIF_NUMOF >= 2)
    else {
        // NOTE enters this block when strings are not equal
        mutex_lock(&tx_2_4_ghz.lock);
        strcpy(tx_2_4_ghz.dest, argv[1]);
        mutex_unlock(&tx_2_4_ghz.lock);
    }
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */   

    return 0;
}

static const shell_command_t shell_commands[] = {
    {"numbytesub", "set the number of payload bytes in a sub-ghz message", numbyte_handler},
    {"numbytesup", "set the number of payload bytes in a 2.4-ghz message", numbyte_handler},
    {"taddrsub", "toggle between a preset IF and TRX destination address (sub-GHz)", taddr_handler},
    {"taddrsup", "toggle between a preset IF and TRX destination address (2.4 GHz)", taddr_handler},
    {"saddrsub", "set a destination address (sub-GHz)", saddr_handler},
    {"saddrsup", "set a destination address (2.4 GHz)", saddr_handler},
    {NULL, NULL, NULL}
};

int main(void)
{
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);
    
    netopt_enable_t disable = NETOPT_DISABLE;
    uint16_t default_channel = IF_DEFAULT_CHANNEL;
    netif = gnrc_netif_iter(netif);
    while (netif)
    {
        gnrc_netapi_set(netif->pid, NETOPT_AUTOACK, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_ACK_REQ, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_CSMA, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_AUTOCCA, 0, &disable, sizeof(disable));
        gnrc_netapi_set(netif->pid, NETOPT_CHANNEL, 0, &default_channel, sizeof(default_channel));
        netif = gnrc_netif_iter(netif);
    }

#ifdef MODULE_AT86RF215
    init_phy_sub_ghz();
#if (GNRC_NETIF_NUMOF >= 2)
    init_phy_2_4_ghz();
#endif /* (GNRC_NETIF_NUMOF >= 2) */
#endif /* MODULE_AT86RF215 */

    (void) puts("Welcome to RIOT!");

    pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                        0, thread_handler, NULL, "thread");

    msg_t tx_message_sub_ghz = {.type = IF_MSG_TX_SUB_GHZ};
    msg_t tx_message_2_4_ghz = {.type = IF_MSG_TX_2_4_GHZ};
    msg_t phy_cfg_sub_ghz_message = {.type = IF_MSG_PHY_CFG_SUB_GHZ};
    msg_t phy_cfg_2_4_ghz_message = {.type = IF_MSG_PHY_CFG_2_4_GHZ};
    /* initialize input pins */
    // REVIEW this initialization is probably redudant and should be removed if so
    gpio_init(TX_SUB_GHZ_PIN, GPIO_IN);
    gpio_init(TX_2_4_GHZ_PIN, GPIO_IN);
    gpio_init(PHY_CFG_SUB_GHZ_PIN, GPIO_IN);
    gpio_init(PHY_CFG_2_4_GHZ_PIN, GPIO_IN);
    /* initialize input pins to be triggered */
    gpio_init_int(TX_SUB_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &tx_message_sub_ghz);
    gpio_init_int(TX_2_4_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &tx_message_2_4_ghz);
    gpio_init_int(PHY_CFG_SUB_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &phy_cfg_sub_ghz_message);
    gpio_init_int(PHY_CFG_2_4_GHZ_PIN, GPIO_IN_PD, GPIO_RISING, gpio_cb, &phy_cfg_2_4_ghz_message);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
