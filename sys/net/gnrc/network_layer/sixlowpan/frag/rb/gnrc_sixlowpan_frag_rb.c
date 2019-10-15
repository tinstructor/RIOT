/*
 * Copyright (C) 2019 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 */

#include <inttypes.h>
#include <stdbool.h>

#include "net/ieee802154.h"
#include "net/ipv6.h"
#include "net/ipv6/hdr.h"
#include "net/gnrc.h"
#include "net/gnrc/sixlowpan.h"
#ifdef  MODULE_GNRC_SIXLOWPAN_FRAG_VRB
#include "net/gnrc/sixlowpan/frag/vrb.h"
#endif  /* MODULE_GNRC_SIXLOWPAN_FRAG_VRB */
#include "net/sixlowpan.h"
#include "thread.h"
#include "xtimer.h"
#include "utlist.h"

#include "net/gnrc/sixlowpan/frag/rb.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

/* estimated fragment payload size to determinate RBUF_INT_SIZE, default to
 * MAC payload size - fragment header. */
#ifndef GNRC_SIXLOWPAN_FRAG_SIZE
/* assuming 64-bit source/destination address, source PAN ID omitted */
#define GNRC_SIXLOWPAN_FRAG_SIZE (104 - 5)
#endif

#ifndef RBUF_INT_SIZE
/* same as ((int) ceil((double) N / D)) */
#define DIV_CEIL(N, D) (((N) + (D) - 1) / (D))
#define RBUF_INT_SIZE (DIV_CEIL(IPV6_MIN_MTU, GNRC_SIXLOWPAN_FRAG_SIZE) * \
                       GNRC_SIXLOWPAN_FRAG_RBUF_SIZE)
#endif

static gnrc_sixlowpan_frag_rb_int_t rbuf_int[RBUF_INT_SIZE];

static gnrc_sixlowpan_frag_rb_t rbuf[GNRC_SIXLOWPAN_FRAG_RBUF_SIZE];

static char l2addr_str[3 * IEEE802154_LONG_ADDRESS_LEN];

static xtimer_t _gc_timer;
static msg_t _gc_timer_msg = { .type = GNRC_SIXLOWPAN_MSG_FRAG_GC_RBUF };

/* ------------------------------------
 * internal function definitions
 * ------------------------------------*/
/* checks whether start and end overlaps, but not identical to, given interval i */
static inline bool _rbuf_int_overlap_partially(gnrc_sixlowpan_frag_rb_int_t *i,
                                               uint16_t start, uint16_t end);
/* gets a free entry from interval buffer */
static gnrc_sixlowpan_frag_rb_int_t *_rbuf_int_get_free(void);
/* update interval buffer of entry */
static bool _rbuf_update_ints(gnrc_sixlowpan_frag_rb_base_t *entry,
                              uint16_t offset, size_t frag_size);
/* gets an entry identified by its tupel */
static gnrc_sixlowpan_frag_rb_t *_rbuf_get(const void *src, size_t src_len,
                                           const void *dst, size_t dst_len,
                                           size_t size, uint16_t tag,
                                           unsigned page);
/* internal add to repeat add when fragments overlapped */
static int _rbuf_add(gnrc_netif_hdr_t *netif_hdr, gnrc_pktsnip_t *pkt,
                     size_t offset, unsigned page);

/* status codes for _rbuf_add() */
enum {
    RBUF_ADD_SUCCESS,
    RBUF_ADD_ERROR,
    RBUF_ADD_REPEAT,
    RBUF_ADD_DUPLICATE,
};

#ifdef MODULE_GNRC_SIXLOWPAN_FRAG_STATS
static gnrc_sixlowpan_frag_stats_t _stats;

gnrc_sixlowpan_frag_stats_t *gnrc_sixlowpan_frag_stats_get(void)
{
    return &_stats;
}
#endif

static int _check_fragments(gnrc_sixlowpan_frag_rb_base_t *entry,
                            size_t frag_size, size_t offset)
{
    gnrc_sixlowpan_frag_rb_int_t *ptr = entry->ints;

    /* If the fragment overlaps another fragment and differs in either the size
     * or the offset of the overlapped fragment, discards the datagram
     * https://tools.ietf.org/html/rfc4944#section-5.3 */
    while (ptr != NULL) {
        if (_rbuf_int_overlap_partially(ptr, offset, offset + frag_size - 1)) {

            /* "A fresh reassembly may be commenced with the most recently
             * received link fragment"
             * https://tools.ietf.org/html/rfc4944#section-5.3 */
            return RBUF_ADD_REPEAT;
        }
        /* End was already checked in overlap check */
        if (ptr->start == offset) {
            DEBUG("6lo rbuf: fragment already in reassembly buffer");
            return RBUF_ADD_DUPLICATE;
        }
        ptr = ptr->next;
    }
    return RBUF_ADD_SUCCESS;
}

void gnrc_sixlowpan_frag_rb_add(gnrc_netif_hdr_t *netif_hdr,
                                gnrc_pktsnip_t *pkt, size_t offset,
                                unsigned page)
{
    if (_rbuf_add(netif_hdr, pkt, offset, page) == RBUF_ADD_REPEAT) {
        _rbuf_add(netif_hdr, pkt, offset, page);
    }
}

static int _rbuf_add(gnrc_netif_hdr_t *netif_hdr, gnrc_pktsnip_t *pkt,
                     size_t offset, unsigned page)
{
    gnrc_sixlowpan_frag_rb_t *entry;
    sixlowpan_frag_n_t *frag = pkt->data;
    uint8_t *data = ((uint8_t *)pkt->data) + sizeof(sixlowpan_frag_t);
    size_t frag_size;

    /* check if provided offset is the same as in fragment */
    assert(((((frag->disp_size.u8[0] & SIXLOWPAN_FRAG_DISP_MASK) ==
                SIXLOWPAN_FRAG_1_DISP)) && (offset == 0)) ||
           ((((frag->disp_size.u8[0] & SIXLOWPAN_FRAG_DISP_MASK) ==
                SIXLOWPAN_FRAG_N_DISP)) && (offset == (frag->offset * 8U))));
    gnrc_sixlowpan_frag_rb_gc();
    entry = _rbuf_get(gnrc_netif_hdr_get_src_addr(netif_hdr), netif_hdr->src_l2addr_len,
                      gnrc_netif_hdr_get_dst_addr(netif_hdr), netif_hdr->dst_l2addr_len,
                      byteorder_ntohs(frag->disp_size) & SIXLOWPAN_FRAG_SIZE_MASK,
                      byteorder_ntohs(frag->tag), page);

    if (entry == NULL) {
        DEBUG("6lo rbuf: reassembly buffer full.\n");
        gnrc_pktbuf_release(pkt);
        return RBUF_ADD_ERROR;
    }

    /* dispatches in the first fragment are ignored */
    if (offset == 0) {
        frag_size = pkt->size - sizeof(sixlowpan_frag_t);
        if (data[0] == SIXLOWPAN_UNCOMP) {
            frag_size--;
        }
    }
    else {
        frag_size = pkt->size - sizeof(sixlowpan_frag_n_t);
        data++; /* FRAGN header is one byte longer (offset) */
    }

    if ((offset + frag_size) > entry->super.datagram_size) {
        DEBUG("6lo rfrag: fragment too big for resulting datagram, discarding datagram\n");
        gnrc_pktbuf_release(entry->pkt);
        gnrc_pktbuf_release(pkt);
        gnrc_sixlowpan_frag_rb_remove(entry);
        return RBUF_ADD_ERROR;
    }

    switch (_check_fragments(&entry->super, frag_size, offset)) {
        case RBUF_ADD_REPEAT:
            DEBUG("6lo rfrag: overlapping intervals, discarding datagram\n");
            gnrc_pktbuf_release(entry->pkt);
            gnrc_sixlowpan_frag_rb_remove(entry);
            return RBUF_ADD_REPEAT;
        case RBUF_ADD_DUPLICATE:
            gnrc_pktbuf_release(pkt);
            return RBUF_ADD_SUCCESS;
        default:
            break;
    }

    if (_rbuf_update_ints(&entry->super, offset, frag_size)) {
        DEBUG("6lo rbuf: add fragment data\n");
        entry->super.current_size += (uint16_t)frag_size;
        if (offset == 0) {
#ifdef MODULE_GNRC_SIXLOWPAN_IPHC
            if (sixlowpan_iphc_is(data)) {
                gnrc_pktsnip_t *frag_hdr = gnrc_pktbuf_mark(pkt,
                        sizeof(sixlowpan_frag_t), GNRC_NETTYPE_SIXLOWPAN);
                if (frag_hdr == NULL) {
                    gnrc_pktbuf_release(entry->pkt);
                    gnrc_pktbuf_release(pkt);
                    gnrc_sixlowpan_frag_rb_remove(entry);
                    return RBUF_ADD_ERROR;
                }
                gnrc_sixlowpan_iphc_recv(pkt, entry, 0);
                return RBUF_ADD_SUCCESS;
            }
            else
#endif
            if (data[0] == SIXLOWPAN_UNCOMP) {
                data++;
            }
        }
        memcpy(((uint8_t *)entry->pkt->data) + offset, data,
               frag_size);
    }
    gnrc_sixlowpan_frag_rb_dispatch_when_complete(entry, netif_hdr);
    gnrc_pktbuf_release(pkt);
    return RBUF_ADD_SUCCESS;
}

static inline bool _rbuf_int_overlap_partially(gnrc_sixlowpan_frag_rb_int_t *i,
                                               uint16_t start, uint16_t end)
{
    /* start and ends are both inclusive, so using <= for both */
    return ((i->start <= end) && (start <= i->end)) && /* overlaps */
        ((start != i->start) || (end != i->end)); /* not identical */
}

static gnrc_sixlowpan_frag_rb_int_t *_rbuf_int_get_free(void)
{
    for (unsigned int i = 0; i < RBUF_INT_SIZE; i++) {
        if (rbuf_int[i].end == 0) { /* start must be smaller than end anyways*/
            return rbuf_int + i;
        }
    }

    return NULL;
}

static bool _rbuf_update_ints(gnrc_sixlowpan_frag_rb_base_t *entry,
                              uint16_t offset, size_t frag_size)
{
    gnrc_sixlowpan_frag_rb_int_t *new;
    uint16_t end = (uint16_t)(offset + frag_size - 1);

    new = _rbuf_int_get_free();

    if (new == NULL) {
        DEBUG("6lo rfrag: no space left in rbuf interval buffer.\n");
        return false;
    }

    new->start = offset;
    new->end = end;

    DEBUG("6lo rfrag: add interval (%" PRIu16 ", %" PRIu16 ") to entry (%s, ",
          new->start, new->end, gnrc_netif_addr_to_str(entry->src,
                                                       entry->src_len,
                                                       l2addr_str));
    DEBUG("%s, %u, %u)\n", gnrc_netif_addr_to_str(entry->dst,
                                                  entry->dst_len,
                                                  l2addr_str),
          entry->datagram_size, entry->tag);

    LL_PREPEND(entry->ints, new);

    return true;
}

void gnrc_sixlowpan_frag_rb_gc(void)
{
    uint32_t now_usec = xtimer_now_usec();
    unsigned int i;

    for (i = 0; i < GNRC_SIXLOWPAN_FRAG_RBUF_SIZE; i++) {
        /* since pkt occupies pktbuf, aggressivly collect garbage */
        if (!gnrc_sixlowpan_frag_rb_entry_empty(&rbuf[i]) &&
              ((now_usec - rbuf[i].super.arrival) >
               GNRC_SIXLOWPAN_FRAG_RBUF_TIMEOUT_US)) {
            DEBUG("6lo rfrag: entry (%s, ",
                  gnrc_netif_addr_to_str(rbuf[i].super.src,
                                         rbuf[i].super.src_len,
                                         l2addr_str));
            DEBUG("%s, %u, %u) timed out\n",
                  gnrc_netif_addr_to_str(rbuf[i].super.dst,
                                         rbuf[i].super.dst_len,
                                         l2addr_str),
                  (unsigned)rbuf[i].super.datagram_size, rbuf[i].super.tag);

            gnrc_pktbuf_release(rbuf[i].pkt);
            gnrc_sixlowpan_frag_rb_remove(&(rbuf[i]));
        }
    }
#ifdef MODULE_GNRC_SIXLOWPAN_FRAG_VRB
    gnrc_sixlowpan_frag_vrb_gc();
#endif
}

static inline void _set_rbuf_timeout(void)
{
    xtimer_set_msg(&_gc_timer, GNRC_SIXLOWPAN_FRAG_RBUF_TIMEOUT_US,
                   &_gc_timer_msg, sched_active_pid);
}

static gnrc_sixlowpan_frag_rb_t *_rbuf_get(const void *src, size_t src_len,
                                           const void *dst, size_t dst_len,
                                           size_t size, uint16_t tag,
                                           unsigned page)
{
    gnrc_sixlowpan_frag_rb_t *res = NULL, *oldest = NULL;
    uint32_t now_usec = xtimer_now_usec();

    for (unsigned int i = 0; i < GNRC_SIXLOWPAN_FRAG_RBUF_SIZE; i++) {
        /* check first if entry already available */
        if ((rbuf[i].pkt != NULL) && (rbuf[i].super.datagram_size == size) &&
            (rbuf[i].super.tag == tag) && (rbuf[i].super.src_len == src_len) &&
            (rbuf[i].super.dst_len == dst_len) &&
            (memcmp(rbuf[i].super.src, src, src_len) == 0) &&
            (memcmp(rbuf[i].super.dst, dst, dst_len) == 0)) {
            DEBUG("6lo rfrag: entry %p (%s, ", (void *)(&rbuf[i]),
                  gnrc_netif_addr_to_str(rbuf[i].super.src,
                                         rbuf[i].super.src_len,
                                         l2addr_str));
            DEBUG("%s, %u, %u) found\n",
                  gnrc_netif_addr_to_str(rbuf[i].super.dst,
                                         rbuf[i].super.dst_len,
                                         l2addr_str),
                  (unsigned)rbuf[i].super.datagram_size, rbuf[i].super.tag);
            rbuf[i].super.arrival = now_usec;
            _set_rbuf_timeout();
            return &(rbuf[i]);
        }

        /* if there is a free spot: remember it */
        if ((res == NULL) && gnrc_sixlowpan_frag_rb_entry_empty(&rbuf[i])) {
            res = &(rbuf[i]);
        }

        /* remember oldest slot */
        /* note that xtimer_now will overflow in ~1.2 hours */
        if ((oldest == NULL) ||
            (oldest->super.arrival - rbuf[i].super.arrival < UINT32_MAX / 2)) {
            oldest = &(rbuf[i]);
        }
    }

    /* entry not in buffer and no empty spot found */
    if (res == NULL) {
        assert(oldest != NULL);
        /* if oldest is not empty, res must not be NULL (because otherwise
         * oldest could have been picked as res) */
        assert(!gnrc_sixlowpan_frag_rb_entry_empty(oldest));
        if (GNRC_SIXLOWPAN_FRAG_RBUF_AGGRESSIVE_OVERRIDE ||
            ((now_usec - oldest->super.arrival) >
            GNRC_SIXLOWPAN_FRAG_RBUF_TIMEOUT_US)) {
            DEBUG("6lo rfrag: reassembly buffer full, remove oldest entry\n");
            gnrc_pktbuf_release(oldest->pkt);
            gnrc_sixlowpan_frag_rb_remove(oldest);
            res = oldest;
#if GNRC_SIXLOWPAN_FRAG_RBUF_AGGRESSIVE_OVERRIDE && \
    defined(MODULE_GNRC_SIXLOWPAN_FRAG_STATS)
            _stats.rbuf_full++;
#endif
        }
        else {
#ifdef MODULE_GNRC_SIXLOWPAN_FRAG_STATS
            _stats.rbuf_full++;
#endif
            return NULL;
        }
    }

    /* now we have an empty spot */

    gnrc_nettype_t reass_type;
    switch (page) {
        /* use switch(page) to be extendable */
#ifdef MODULE_GNRC_IPV6
        case 0U:
            reass_type = GNRC_NETTYPE_IPV6;
            break;
#endif
        default:
            reass_type = GNRC_NETTYPE_UNDEF;
    }
    res->pkt = gnrc_pktbuf_add(NULL, NULL, size, reass_type);
    if (res->pkt == NULL) {
        DEBUG("6lo rfrag: can not allocate reassembly buffer space.\n");
        return NULL;
    }

    *((uint64_t *)res->pkt->data) = 0;  /* clean first few bytes for later
                                               * look-ups */
    res->super.datagram_size = size;
    res->super.arrival = now_usec;
    memcpy(res->super.src, src, src_len);
    memcpy(res->super.dst, dst, dst_len);
    res->super.src_len = src_len;
    res->super.dst_len = dst_len;
    res->super.tag = tag;
    res->super.current_size = 0;

    DEBUG("6lo rfrag: entry %p (%s, ", (void *)res,
          gnrc_netif_addr_to_str(res->super.src, res->super.src_len,
                                 l2addr_str));
    DEBUG("%s, %u, %u) created\n",
          gnrc_netif_addr_to_str(res->super.dst, res->super.dst_len,
                                 l2addr_str), res->super.datagram_size,
          res->super.tag);

    _set_rbuf_timeout();

    return res;
}

#ifdef TEST_SUITES
void gnrc_sixlowpan_frag_rb_reset(void)
{
    xtimer_remove(&_gc_timer);
    memset(rbuf_int, 0, sizeof(rbuf_int));
    for (unsigned int i = 0; i < GNRC_SIXLOWPAN_FRAG_RBUF_SIZE; i++) {
        if ((rbuf[i].pkt != NULL) &&
            (rbuf[i].pkt->users > 0)) {
            gnrc_pktbuf_release(rbuf[i].pkt);
        }
    }
    memset(rbuf, 0, sizeof(rbuf));
}

const gnrc_sixlowpan_frag_rb_t *gnrc_sixlowpan_frag_rb_array(void)
{
    return &rbuf[0];
}
#endif

void gnrc_sixlowpan_frag_rb_base_rm(gnrc_sixlowpan_frag_rb_base_t *entry)
{
    while (entry->ints != NULL) {
        gnrc_sixlowpan_frag_rb_int_t *next = entry->ints->next;

        entry->ints->start = 0;
        entry->ints->end = 0;
        entry->ints->next = NULL;
        entry->ints = next;
    }
    entry->datagram_size = 0;
}

void gnrc_sixlowpan_frag_rb_dispatch_when_complete(gnrc_sixlowpan_frag_rb_t *rbuf,
                                                   gnrc_netif_hdr_t *netif_hdr)
{
    assert(rbuf);
    assert(netif_hdr);
    if (rbuf->super.current_size == rbuf->super.datagram_size) {
        gnrc_pktsnip_t *netif = gnrc_netif_hdr_build(rbuf->super.src,
                                                     rbuf->super.src_len,
                                                     rbuf->super.dst,
                                                     rbuf->super.dst_len);

        if (netif == NULL) {
            DEBUG("6lo rbuf: error allocating netif header\n");
            gnrc_pktbuf_release(rbuf->pkt);
            gnrc_sixlowpan_frag_rb_remove(rbuf);
            return;
        }

        /* copy the transmit information of the latest fragment into the newly
         * created header to have some link_layer information. The link_layer
         * info of the previous fragments is discarded.
         */
        gnrc_netif_hdr_t *new_netif_hdr = netif->data;
        new_netif_hdr->if_pid = netif_hdr->if_pid;
        new_netif_hdr->flags = netif_hdr->flags;
        new_netif_hdr->lqi = netif_hdr->lqi;
        new_netif_hdr->rssi = netif_hdr->rssi;
        LL_APPEND(rbuf->pkt, netif);
        gnrc_sixlowpan_dispatch_recv(rbuf->pkt, NULL, 0);
        gnrc_sixlowpan_frag_rb_remove(rbuf);
    }
}


/** @} */
