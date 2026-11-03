/*
 * Copyright (c) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>
#include "packets.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <stdlib.h>
#include <netdb.h>
#include "byte-order.h"
#include "csum.h"
#include "crc32c.h"
#include "flow.h"
#include "openvswitch/hmap.h"
#include "openvswitch/dynamic-string.h"
#include "ovs-thread.h"
#include "odp-util.h"
#include "dp-packet.h"
#include "dp-packet-gso.h"
#include "unaligned.h"
#include "openvswitch/vlog.h"


VLOG_DEFINE_THIS_MODULE(packet);@@PACKETS_1@@static uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

const struct in6_addr in6addr_exact = IN6ADDR_EXACT_INIT;
const struct in6_addr in6addr_all_hosts = IN6ADDR_ALL_HOSTS_INIT;
const struct in6_addr in6addr_all_routers = IN6ADDR_ALL_ROUTERS_INIT;
const struct in6_addr in6addr_v4mapped_any = IN6ADDR_V4MAPPED_ANY_INIT;

struct in6_addr
flow_tnl_dst(const struct flow_tnl *tnl)
{
    return tnl->ip_dst ? in6_addr_mapped_ipv4(tnl->ip_dst) : tnl->ipv6_dst;
}

struct in6_addr
flow_tnl_src(const struct flow_tnl *tnl)
{
    return tnl->ip_src ? in6_addr_mapped_ipv4(tnl->ip_src) : tnl->ipv6_src;
}

/* Returns true if 's' consists entirely of hex digits, false otherwise. */
static bool
is_all_hex(const char *s)
{
    return s[strspn(s, "0123456789abcdefABCDEF")] == '\0';
}

/* Parses 's' as a 16-digit hexadecimal number representing a datapath ID.  On
 * success stores the dpid into '*dpidp' and returns true, on failure stores 0
 * into '*dpidp' and returns false.
 *
 * Rejects an all-zeros dpid as invalid. */
bool
dpid_from_string(const char *s, uint64_t *dpidp)
{
    size_t len = strlen(s);
    *dpidp = ((len == 16 && is_all_hex(s))
              || (len <= 18 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')
                  && is_all_hex(s + 2))
              ? strtoull(s, NULL, 16)
              : 0);
    return *dpidp != 0;
}

uint64_t
eth_addr_to_uint64(const struct eth_addr ea)
{
    return (((uint64_t) ntohs(ea.be16[0]) << 32)
            | ((uint64_t) ntohs(ea.be16[1]) << 16)
            | ntohs(ea.be16[2]));
}

void
eth_addr_from_uint64(uint64_t x, struct eth_addr *ea)
{
    ea->be16[0] = htons(x >> 32);
    ea->be16[1] = htons((x & 0xFFFF0000) >> 16);
    ea->be16[2] = htons(x & 0xFFFF);
}

void
eth_addr_mark_random(struct eth_addr *ea)
{
    ea->ea[0] &= ~1;                /* Unicast. */
    ea->ea[0] |= 2;                 /* Private. */
}

/* Returns true if 'ea' is a reserved address, that a bridge must never
 * forward, false otherwise.
 *
 * If you change this function's behavior, please update corresponding
 * documentation in vswitch.xml at the same time. */
bool
eth_addr_is_reserved(const struct eth_addr ea)
{
    struct eth_addr_node {
        struct hmap_node hmap_node;
        const uint64_t ea64;
    };

    static struct eth_addr_node nodes[] = {
        /* STP, IEEE pause frames, and other reserved protocols. */
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000000ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000001ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000002ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000003ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000004ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000005ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000006ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000007ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000008ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c2000009ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000aULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000bULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000cULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000dULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000eULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x0180c200000fULL },

        /* Extreme protocols. */
        { HMAP_NODE_NULL_INITIALIZER, 0x00e02b000000ULL }, /* EDP. */
        { HMAP_NODE_NULL_INITIALIZER, 0x00e02b000004ULL }, /* EAPS. */
        { HMAP_NODE_NULL_INITIALIZER, 0x00e02b000006ULL }, /* EAPS. */

        /* Cisco protocols. */
        { HMAP_NODE_NULL_INITIALIZER, 0x01000c000000ULL }, /* ISL. */
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccccULL }, /* PAgP, UDLD, CDP,
                                                            * DTP, VTP. */
        { HMAP_NODE_NULL_INITIALIZER, 0x01000ccccccdULL }, /* PVST+. */
        { HMAP_NODE_NULL_INITIALIZER, 0x01000ccdcdcdULL }, /* STP Uplink Fast,
                                                            * FlexLink. */

        /* Cisco CFM. */
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc0ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc1ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc2ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc3ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc4ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc5ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc6ULL },
        { HMAP_NODE_NULL_INITIALIZER, 0x01000cccccc7ULL },
    };

    static struct ovsthread_once once = OVSTHREAD_ONCE_INITIALIZER;
    struct eth_addr_node *node;
    static struct hmap addrs;
    uint64_t ea64;

    if (ovsthread_once_start(&once)) {
        hmap_init(&addrs);
        for (node = nodes; node < &nodes[ARRAY_SIZE(nodes)]; node++) {
            hmap_insert(&addrs, &node->hmap_node, hash_uint64(node->ea64));
        }
        ovsthread_once_done(&once);
    }

    ea64 = eth_addr_to_uint64(ea);
    HMAP_FOR_EACH_IN_BUCKET (node, hmap_node, hash_uint64(ea64), &addrs) {
        if (node->ea64 == ea64) {
            return true;
        }
    }
    return false;
}

/* Attempts to parse 's' as an Ethernet address.  If successful, stores the
 * address in 'ea' and returns true, otherwise zeros 'ea' and returns
 * false.  This function checks trailing characters. */
bool
eth_addr_from_string(const char *s, struct eth_addr *ea)
{
    int n = 0;
    if (ovs_scan(s, ETH_ADDR_SCAN_FMT"%n", ETH_ADDR_SCAN_ARGS(*ea), &n)
        && !s[n]) {
        return true;
    } else {
        *ea = eth_addr_zero;
        return false;
    }
}

/* Fills 'b' with a Reverse ARP packet with Ethernet source address 'eth_src'.
 * This function is used by Open vSwitch to compose packets in cases where
 * context is important but content doesn't (or shouldn't) matter.
 *
 * The returned packet has enough headroom to insert an 802.1Q VLAN header if
 * desired. */
void
compose_rarp(struct dp_packet *b, const struct eth_addr eth_src)
{
    struct eth_header *eth;
    struct arp_eth_header *arp;

    dp_packet_clear(b);
    dp_packet_prealloc_tailroom(b, 2 + ETH_HEADER_LEN + VLAN_HEADER_LEN
                             + ARP_ETH_HEADER_LEN);
    dp_packet_reserve(b, 2 + VLAN_HEADER_LEN);
    eth = dp_packet_put_uninit(b, sizeof *eth);
    eth->eth_dst = eth_addr_broadcast;
    eth->eth_src = eth_src;
    eth->eth_type = htons(ETH_TYPE_RARP);

    arp = dp_packet_put_uninit(b, sizeof *arp);
    arp->ar_hrd = htons(ARP_HRD_ETHERNET);
    arp->ar_pro = htons(ARP_PRO_IP);
    arp->ar_hln = sizeof arp->ar_sha;
    arp->ar_pln = sizeof arp->ar_spa;
    arp->ar_op = htons(ARP_OP_RARP);
    arp->ar_sha = eth_src;
    put_16aligned_be32(&arp->ar_spa, htonl(0));
    arp->ar_tha = eth_src;
    put_16aligned_be32(&arp->ar_tpa, htonl(0));

    dp_packet_set_l3(b, arp);
    b->packet_type = htonl(PT_ETH);
}

/* Insert VLAN header according to given TCI. Packet passed must be Ethernet
 * packet.  Ignores the CFI bit of 'tci' using 0 instead.
 *
 * Also adjusts the layer offsets accordingly. */
void
eth_push_vlan(struct dp_packet *packet, ovs_be16 tpid, ovs_be16 tci)
{
    struct vlan_eth_header *veh;

    /* Insert new 802.1Q header. */
    veh = dp_packet_resize_l2(packet, VLAN_HEADER_LEN);
    memmove(veh, (char *)veh + VLAN_HEADER_LEN, 2 * ETH_ADDR_LEN);
    veh->veth_type = tpid;
    veh->veth_tci = tci & htons(~VLAN_CFI);
}

/* Removes outermost VLAN header (if any is present) from 'packet'.
 *
 * 'packet->l2_5' should initially point to 'packet''s outer-most VLAN header
 * or may be NULL if there are no VLAN headers. */
void
eth_pop_vlan(struct dp_packet *packet)
{
    struct vlan_eth_header *veh = dp_packet_eth(packet);

    if (veh && dp_packet_size(packet) >= sizeof *veh
        && eth_type_vlan(veh->veth_type)) {

        memmove((char *)veh + VLAN_HEADER_LEN, veh, 2 * ETH_ADDR_LEN);
        dp_packet_resize_l2(packet, -VLAN_HEADER_LEN);
    }
}

/* Push Ethernet header onto 'packet' assuming it is layer 3 */
void
push_eth(struct dp_packet *packet, const struct eth_addr *dst,
         const struct eth_addr *src)
{
    struct eth_header *eh;

    ovs_assert(!dp_packet_is_eth(packet));
    eh = dp_packet_resize_l2(packet, ETH_HEADER_LEN);
    eh->eth_dst = *dst;
    eh->eth_src = *src;
    eh->eth_type = pt_ns_type_be(packet->packet_type);
    packet->packet_type = htonl(PT_ETH);
}

/* Removes Ethernet header, including VLAN header, from 'packet'.
 *
 * Previous to calling this function, 'ofpbuf_l3(packet)' must not be NULL */
void
pop_eth(struct dp_packet *packet)
{
    char *l2_5 = dp_packet_l2_5(packet);
    char *l3 = dp_packet_l3(packet);
    ovs_be16 ethertype;
    int increment;

    ovs_assert(dp_packet_is_eth(packet));
    ovs_assert(l3 != NULL);

    if (l2_5) {
        increment = packet->l2_5_ofs;
        ethertype = *(ALIGNED_CAST(ovs_be16 *, (l2_5 - 2)));
    } else {
        increment = packet->l3_ofs;
        ethertype = *(ALIGNED_CAST(ovs_be16 *, (l3 - 2)));
    }

    dp_packet_resize_l2(packet, -increment);
    packet->packet_type = PACKET_TYPE_BE(OFPHTN_ETHERTYPE, ntohs(ethertype));
}

/* Set ethertype of the packet. */
static void
set_ethertype(struct dp_packet *packet, ovs_be16 eth_type)
{
    struct eth_header *eh = dp_packet_eth(packet);

    if (!eh) {
        return;
    }

    if (eth_type_vlan(eh->eth_type)) {
        ovs_be16 *p;
        char *l2_5 = dp_packet_l2_5(packet);

        p = ALIGNED_CAST(ovs_be16 *,
                         (l2_5 ? l2_5 : (char *)dp_packet_l3(packet)) - 2);
        *p = eth_type;
    } else {
        eh->eth_type = eth_type;
    }
}

static bool is_mpls(struct dp_packet *packet)
{
    return packet->l2_5_ofs != UINT16_MAX;
}

/* Set time to live (TTL) of an MPLS label stack entry (LSE). */
void
set_mpls_lse_ttl(ovs_be32 *lse, uint8_t ttl)
{
    *lse &= ~htonl(MPLS_TTL_MASK);
    *lse |= htonl((ttl << MPLS_TTL_SHIFT) & MPLS_TTL_MASK);
}

/* Set traffic class (TC) of an MPLS label stack entry (LSE). */
void
set_mpls_lse_tc(ovs_be32 *lse, uint8_t tc)
{
    *lse &= ~htonl(MPLS_TC_MASK);
    *lse |= htonl((tc << MPLS_TC_SHIFT) & MPLS_TC_MASK);
}

/* Set label of an MPLS label stack entry (LSE). */
void
set_mpls_lse_label(ovs_be32 *lse, ovs_be32 label)
{
    *lse &= ~htonl(MPLS_LABEL_MASK);
    *lse |= htonl((ntohl(label) << MPLS_LABEL_SHIFT) & MPLS_LABEL_MASK);
}

/* Set bottom of stack (BoS) bit of an MPLS label stack entry (LSE). */
void
set_mpls_lse_bos(ovs_be32 *lse, uint8_t bos)
{
    *lse &= ~htonl(MPLS_BOS_MASK);
    *lse |= htonl((bos << MPLS_BOS_SHIFT) & MPLS_BOS_MASK);
}

/* Compose an MPLS label stack entry (LSE) from its components:
 * label, traffic class (TC), time to live (TTL) and
 * bottom of stack (BoS) bit. */
ovs_be32
set_mpls_lse_values(uint8_t ttl, uint8_t tc, uint8_t bos, ovs_be32 label)
{
    ovs_be32 lse = htonl(0);
    set_mpls_lse_ttl(&lse, ttl);
    set_mpls_lse_tc(&lse, tc);
    set_mpls_lse_bos(&lse, bos);
    set_mpls_lse_label(&lse, label);
    return lse;
}

/* Set MPLS label stack entry to outermost MPLS header.*/
void
set_mpls_lse(struct dp_packet *packet, ovs_be32 mpls_lse)
{
    /* Packet type should be MPLS to set label stack entry. */
    if (is_mpls(packet)) {
        struct mpls_hdr *mh = dp_packet_l2_5(packet);

        /* Update mpls label stack entry. */
        put_16aligned_be32(&mh->mpls_lse, mpls_lse);
    }
}

/* Push MPLS label stack entry 'lse' onto 'packet' as the outermost MPLS
 * header.  If 'packet' does not already have any MPLS labels, then its
 * Ethertype is changed to 'ethtype' (which must be an MPLS Ethertype). */
void
push_mpls(struct dp_packet *packet, ovs_be16 ethtype, ovs_be32 lse)
{
    char * header;
    size_t len;

    if (!eth_type_mpls(ethtype)) {
        return;
    }

    if (!is_mpls(packet)) {
        /* Set MPLS label stack offset. */
        packet->l2_5_ofs = packet->l3_ofs;
    }

    set_ethertype(packet, ethtype);

    /* Push new MPLS shim header onto packet. */
    len = packet->l2_5_ofs;
    header = dp_packet_resize_l2_5(packet, MPLS_HLEN);
    memmove(header, header + MPLS_HLEN, len);
    memcpy(header + len, &lse, sizeof lse);

    pkt_metadata_init_conn(&packet->md);
}

void
add_mpls(struct dp_packet *packet, ovs_be16 ethtype, ovs_be32 lse,
         bool l3_encap)
{
    if (!eth_type_mpls(ethtype)) {
        return;
    }

    if (!l3_encap) {
        struct mpls_hdr *header = dp_packet_resize_l2(packet, MPLS_HLEN);

        put_16aligned_be32(&header->mpls_lse, lse);
        packet->l2_5_ofs = 0;
        packet->packet_type = PACKET_TYPE_BE(OFPHTN_ETHERTYPE,
                                             ntohs(ethtype));
    } else {
        size_t len;
        char *header;

        if (!is_mpls(packet)) {
            /* Set MPLS label stack offset. */
            packet->l2_5_ofs = packet->l3_ofs;
        }
        set_ethertype(packet, ethtype);

        /* Push new MPLS shim header onto packet. */
        len = packet->l2_5_ofs;
        header = dp_packet_resize_l2_5(packet, MPLS_HLEN);
        memmove(header, header + MPLS_HLEN, len);
        memcpy(header + len, &lse, sizeof lse);
    }
    pkt_metadata_init_conn(&packet->md);
}

/* If 'packet' is an MPLS packet, removes its outermost MPLS label stack entry.
 * If the label that was removed was the only MPLS label, changes 'packet''s
 * Ethertype to 'ethtype' (which ordinarily should not be an MPLS
 * Ethertype). */
void
pop_mpls(struct dp_packet *packet, ovs_be16 ethtype)
{
    if (is_mpls(packet)) {
        struct mpls_hdr *mh = dp_packet_l2_5(packet);
        size_t len = packet->l2_5_ofs;

        set_ethertype(packet, ethtype);
        if (get_16aligned_be32(&mh->mpls_lse) & htonl(MPLS_BOS_MASK)) {
            dp_packet_set_l2_5(packet, NULL);
        }
        /* Shift the l2 header forward. */
        memmove((char*)dp_packet_data(packet) + MPLS_HLEN, dp_packet_data(packet), len);
        dp_packet_resize_l2_5(packet, -MPLS_HLEN);

        /* Invalidate offload flags as they are not valid after
         * decapsulation of MPLS header. */
        dp_packet_reset_offload(packet);

        /* packet_type must be reset for the MPLS packets with no l2 header */
        if (!len) {
            if (ethtype == htons(ETH_TYPE_TEB)) {
                /* The inner packet must be classified as ethernet if the
                 * ethtype is ETH_TYPE_TEB. */
                packet->packet_type = htonl(PT_ETH);
            } else {
                packet->packet_type = PACKET_TYPE_BE(OFPHTN_ETHERTYPE,
                                                     ntohs(ethtype));
            }
        }
    }
}

void
get_offset_index(struct dp_packet *packet, enum pmap flag, size_t *offset, int *index)
{
    size_t ofs = 0;
    int id = 0;

    for (uint64_t i = 1; i < flag; i <<= 1, id++) {
        if (packet->flag & i) {
            ofs += phlens[id];
        }
    }
    if (offset) {
        *offset = ofs;
    }
    if (index) {
        *index = id;
    }
}

void
push_protocol(struct dp_packet *packet, void *data, size_t ofs, size_t len)
{
    char * header;
    header = dp_packet_push_uninit(packet, len);
    memmove(header, header + len, ofs);
    memcpy(header + ofs, data, len);
}

void
add_protocol(struct dp_packet *packet, enum pmap flag, union protocols protocol)
{
    
    size_t offset;
    int index;

    VLOG_INFO("add_protocol: packet->flag: %d", packet->flag);
    VLOG_INFO("add_protocol: flag: %d", flag);
    if (flag & packet->flag) {
        return;
    }
    
    get_offset_index(packet, flag, &offset, &index);

    VLOG_INFO("add_protocol: offset: %d, index: %d", offset, index);
    push_protocol(packet, &protocol, offset, phlens[index]);

    packet->flag += flag;
}

void
pop_protocol(struct dp_packet *packet, size_t ofs, size_t len)
{
    memmove((char*)dp_packet_data(packet) + len, dp_packet_data(packet), ofs);
    dp_packet_pull(packet, len);

    // dp_packet_reset_offload(packet);
}

void 
del_protocol(struct dp_packet *packet, enum pmap flag)
{
    size_t offset;
    int index;

    if (!(packet->flag & flag)) {
        return;
    }

    get_offset_index(packet, flag, &offset, &index);
    pop_protocol(packet, offset, phlens[index]);

    packet->flag -= flag;
}

void 
mod_fields__(struct dp_packet *packet, uint8_t *data, uint8_t *mask,
             size_t offset, size_t len)
{
    uint8_t *header;
    size_t i;

    header = dp_packet_data(packet) + offset;
    for (i = 0; i < len; i++) {
        if (*(mask+i)) {
            *(header+i) = ((*header+i) & ~(*(mask+i))) | (*(data+i) & *(mask+i));
        }
    }
}

void 
mod_fields(struct dp_packet *packet, enum pmap flag, 
            union protocols protocol, union protocols mask)
{
    size_t offset;
    int index;

    if (!(packet->flag & flag)) {
        return;
    }

    get_offset_index(packet, flag, &offset, &index);
    mod_fields__(packet, &protocol, &mask, offset, phlens[index]);
}

void
crc8(uint8_t *data, uint8_t *crc)
{
    uint8_t i = 0x80;

    for (; i != 0; i /= 2) {
        if ((*crc & 0x80) != 0) {
            *crc *= 2;
            *crc ^= 0x07;
        } else {
            *crc *= 2;
        }
        if ((*data & i) != 0) {
            *crc ^= 0x07;
        }
    }
}

void 
crc16(uint8_t *data, uint16_t *crc) 
{
    *crc = crc16_table[((*crc >> 8) ^ (*data)) & 0xff] ^ (*crc << 8);
}   

bool
check_crc__(struct dp_packet *packet, enum pmap flag,
            uint8_t *mask, size_t offset, size_t index)
{
    uint8_t crc_8 = 0;
    uint16_t crc_16 = 0;
    uint32_t crc_32 = 0;
    size_t i;
    bool same;
    char *header;

    header = dp_packet_data(packet) + offset;
    switch (crc_len[index]) {
        case 1 : {
            for (i = 0; i < crc_ofs[index]; i++) {
                if (*(mask+i)) {
                    crc8((header+i), &crc_8);
                }
            }
            same = *(header+crc_ofs[index]) == crc_8;
            *(header+crc_ofs[index]) = crc_8;
            break;
        }
        case 2 : {
            for (i = 0; i < crc_ofs[index]; i++) {
                if (*(mask+i)) {
                    crc16((header+i), &crc_16);
                }
            }
            same = *(uint16_t*)(header+crc_ofs[index]) == htons(crc_16);
            *(uint16_t*)(header+crc_ofs[index]) = htons(crc_16);
            break;
        }
        default: 
            same = false;
    }
    return same;
}

bool 
check_crc(struct dp_packet *packet, enum pmap flag,
          union protocols mask)
{
    size_t offset;
    int index;
    // bool same;

    if (!(packet->flag & flag)) {
        return true;
    }

    get_offset_index(packet, flag, &offset, &index);
    return check_crc__(packet, flag, &mask, offset, index);

}

// void
// calc_crc__(struct dp_packet *packet, enum pmap flag,
//             uint8_t *mask, size_t offset, size_t index)
// {
//     uint8_t crc_8 = 0;
//     uint16_t crc_16 = 0;
//     uint32_t crc_32 = 0;
//     size_t i;
//     char *header;

//     header = dp_packet_data(packet) + offset;
//     switch (crc_len[index]) {
//         case 1 : {
//             for (i = 0; i < crc_ofs[index]; i++) {
//                 if (*(mask+i)) {
//                     crc8((header+i), &crc_8);
//                 }
//             }
//             *(header+crc_ofs[index]) = crc_8;
//             return;
//         }
//         case 2 : {
//             for (i = 0; i < crc_ofs[index]; i++) {
//                 if (*(mask+i)) {
//                     crc16((header+i), &crc_16);
//                 }
//             }
//             *(uint16_t*)(header+crc_ofs[index]) = htons(crc_16);
//             return;
//         }
//         default: 
//             return;
//     }
// }

// void
// calc_crc(struct dp_packet *packet, enum pmap flag,
//          union protocols mask)
// {
//     size_t offset;
//     int index;
    
//     if (!(packet->flag & flag)) {
//         return;
//     }

//     get_offset_index(packet, flag, &offset, &index);
//     check_crc__(packet, flag, &mask, offset, index);
// }

size_t 
get_headers_len(struct dp_packet *packet)
{
    size_t hl = 0;
    uint64_t i = 1;
    int id = 0;

    for ( ; i <= packet->flag; i <<= 1, id++) {
        if (packet->flag & i) {
            hl += phlens[id];
        }
    }
    return hl;
}

void 
payload_crc__(struct dp_packet *packet, size_t offset, size_t crc_len)
{
    uint8_t crc_8 = 0;
    uint16_t crc_16 = 0;
    uint32_t crc_32 = 0;
    size_t i;
    char *payload;

    payload = dp_packet_data(packet) + offset;
    switch (crc_len) {
        case 1 : {
            for (i = 0; i < dp_packet_size(packet)-offset; i++) {
                crc8((payload+i), &crc_8);
            }
            uint8_t *header = dp_packet_add_uninit(packet, 1);
            *header = crc_8;
            break;
        }
        case 2 : {
            for (i = 0; i < dp_packet_size(packet)-offset; i++) {
                crc16((payload+i), &crc_16);
            }
            uint16_t *header = dp_packet_add_uninit(packet, 2);
            *header = htons(crc_16);
            break;
        }
        case 4 : {
            VLOG_INFO("unrealized");
            break;
        }
    }
}

void
payload_crc(struct dp_packet *packet, size_t len)
{
    size_t headers_len = get_headers_len(packet);
    payload_crc__(packet, headers_len, len);
}@@PACKETS_2@@void 
move_protocol(struct dp_packet *packet, enum pmap dst, enum pmap src)
{
    ovs_assert(dst != src);

    if (!((packet->flag & src) && !(packet->flag & dst))) {
        return;
    }

    size_t offset_src, offset_dst;
    int index_src, index_dst;
    union protocols data;

    get_offset_index(packet, src, &offset_src, &index_src);
    get_offset_index(packet, dst, &offset_dst, &index_dst);

    if (phlens[index_src] != phlens[index_dst]) {
        VLOG_INFO("move_protocol: length is not the same, do nothing and return");
        return;
    }

    memcpy(&data, (char*)dp_packet_data(packet) + offset_src, phlens[index_src]);
    pop_protocol(packet, offset_src, phlens[index_src]);
    packet->flag -= src;

    if (dst > src) {
        offset_dst -= phlens[index_src];
    }
    push_protocol(packet, &data, offset_dst, phlens[index_dst]);
    packet->flag += dst;
}

void 
add_bytes(struct dp_packet *packet, size_t len, uint8_t data[]) 
{
    if (len < 14) {
        VLOG_WARN("add_bytes: data length is less than 14");
    }
    char *header;
    header = dp_packet_push_uninit(packet, len-14);
    memcpy(header, data, len);
}

void 
ipv42ipv6(struct dp_packet *packet, size_t len, uint8_t data[])
{
    if (!packet->flag & IPV4_E) {
        VLOG_WARN("ipv42ipv6: packet does NOT have IPV4_E  (ipv42ipv6)");
    }
    size_t offset;
    int index;
    get_offset_index(packet, IPV4_E, &offset, &index);
    uint16_t *length = (uint16_t*)((char*)dp_packet_data(packet)+offset+2);
    uint8_t *ttl = (uint8_t*)((char*)dp_packet_data(packet)+offset+8);
    uint8_t *next_header = (uint8_t)((char*)dp_packet_data(packet)+offset+9);
    *length = htons(ntohs(*length)-20);
    memcpy(&data[4], length, 2);
    memcpy(&data[7], ttl, 1);
    memcpy(&data[6], next_header, 1);
    char *header;
    header = dp_packet_push_uninit(packet, len-20);
    memmove(header, header + len - 20, offset);
    memcpy(header + offset, data, len);
}

void 
ipv62ipv4(struct dp_packet *packet, size_t len, uint8_t data[])
{
    if (!packet->flag & IPV6_E) {
        VLOG_WARN("ipv62ipv4: packet does NOT have IPV6_E");
    }
    size_t offset;
    int index;
    get_offset_index(packet, IPV6_E, &offset, &index);
    // VLOG_INFO("ipv62ipv4: offset: %d", offset);
    uint16_t *length = (uint16_t*)((char*)dp_packet_data(packet)+offset+4);
    uint8_t *ttl = (uint8_t*)((char*)dp_packet_data(packet)+offset+7);
    uint8_t *next_header = (uint8_t*)((char*)dp_packet_data(packet)+offset+6);
    *length = htons(ntohs(*length)+20);
    memcpy(&data[2], length, 2);
    memcpy(&data[8], ttl, 1);
    memcpy(&data[9], next_header, 1);
    char *header;
    memmove((char *)dp_packet_data(packet) + 20, dp_packet_data(packet), offset);
    dp_packet_pull(packet, 20);
    header = (char *)dp_packet_data(packet)+offset;
    memcpy(header, data, len);

	if (!packet->flag & IPV4_E) {
        VLOG_WARN("ipv42ipv6: packet does NOT have IPV4_E (ipv4_checksum)");
    }
    size_t _offset;
    int _index;
    get_offset_index(packet, IPV4_E, &_offset, &_index);
    struct ipv4_e *ipv4 = (struct ipv4_e *)((char*)dp_packet_data(packet)+_offset);
    uint16_t *checksum = (uint16_t *)&ipv4->ip_check_e[0];
    *checksum = 0;
    uint8_t *_data = (uint8_t*)ipv4;
    uint32_t tmp = 0;
    for (size_t i = 0; i < sizeof(*ipv4); i += 2) {
        tmp += ((_data[i] << 8) + _data[i+1]);
    }
    while (tmp > UINT16_MAX) {
        uint16_t high = tmp >> 16;
        uint16_t low = tmp & UINT16_MAX;
        tmp = high + low;
    }
    *checksum = htons((~tmp)&0xffff);
}

#define CRC16_POLY 0x1021
#define CRC16_INIT 0xFFFF

const uint16_t crc16_tab[256] = {
    0x0000,
    0x1021,
    0x2042,
    0x3063,
    0x4084,
    0x50a5,
    0x60c6,
    0x70e7,
    0x8108,
    0x9129,
    0xa14a,
    0xb16b,
    0xc18c,
    0xd1ad,
    0xe1ce,
    0xf1ef,
    0x1231,
    0x0210,
    0x3273,
    0x2252,
    0x52b5,
    0x4294,
    0x72f7,
    0x62d6,
    0x9339,
    0x8318,
    0xb37b,
    0xa35a,
    0xd3bd,
    0xc39c,
    0xf3ff,
    0xe3de,
    0x2462,
    0x3443,
    0x0420,
    0x1401,
    0x64e6,
    0x74c7,
    0x44a4,
    0x5485,
    0xa56a,
    0xb54b,
    0x8528,
    0x9509,
    0xe5ee,
    0xf5cf,
    0xc5ac,
    0xd58d,
    0x3653,
    0x2672,
    0x1611,
    0x0630,
    0x76d7,
    0x66f6,
    0x5695,
    0x46b4,
    0xb75b,
    0xa77a,
    0x9719,
    0x8738,
    0xf7df,
    0xe7fe,
    0xd79d,
    0xc7bc,
    0x48c4,
    0x58e5,
    0x6886,
    0x78a7,
    0x0840,
    0x1861,
    0x2802,
    0x3823,
    0xc9cc,
    0xd9ed,
    0xe98e,
    0xf9af,
    0x8948,
    0x9969,
    0xa90a,
    0xb92b,
    0x5af5,
    0x4ad4,
    0x7ab7,
    0x6a96,
    0x1a71,
    0x0a50,
    0x3a33,
    0x2a12,
    0xdbfd,
    0xcbdc,
    0xfbbf,
    0xeb9e,
    0x9b79,
    0x8b58,
    0xbb3b,
    0xab1a,
    0x6ca6,
    0x7c87,
    0x4ce4,
    0x5cc5,
    0x2c22,
    0x3c03,
    0x0c60,
    0x1c41,
    0xedae,
    0xfd8f,
    0xcdec,
    0xddcd,
    0xad2a,
    0xbd0b,
    0x8d68,
    0x9d49,
    0x7e97,
    0x6eb6,
    0x5ed5,
    0x4ef4,
    0x3e13,
    0x2e32,
    0x1e51,
    0x0e70,
    0xff9f,
    0xefbe,
    0xdfdd,
    0xcffc,
    0xbf1b,
    0xaf3a,
    0x9f59,
    0x8f78,
    0x9188,
    0x81a9,
    0xb1ca,
    0xa1eb,
    0xd10c,
    0xc12d,
    0xf14e,
    0xe16f,
    0x1080,
    0x00a1,
    0x30c2,
    0x20e3,
    0x5004,
    0x4025,
    0x7046,
    0x6067,
    0x83b9,
    0x9398,
    0xa3fb,
    0xb3da,
    0xc33d,
    0xd31c,
    0xe37f,
    0xf35e,
    0x02b1,
    0x1290,
    0x22f3,
    0x32d2,
    0x4235,
    0x5214,
    0x6277,
    0x7256,
    0xb5ea,
    0xa5cb,
    0x95a8,
    0x8589,
    0xf56e,
    0xe54f,
    0xd52c,
    0xc50d,
    0x34e2,
    0x24c3,
    0x14a0,
    0x0481,
    0x7466,
    0x6447,
    0x5424,
    0x4405,
    0xa7db,
    0xb7fa,
    0x8799,
    0x97b8,
    0xe75f,
    0xf77e,
    0xc71d,
    0xd73c,
    0x26d3,
    0x36f2,
    0x0691,
    0x16b0,
    0x6657,
    0x7676,
    0x4615,
    0x5634,
    0xd94c,
    0xc96d,
    0xf90e,
    0xe92f,
    0x99c8,
    0x89e9,
    0xb98a,
    0xa9ab,
    0x5844,
    0x4865,
    0x7806,
    0x6827,
    0x18c0,
    0x08e1,
    0x3882,
    0x28a3,
    0xcb7d,
    0xdb5c,
    0xeb3f,
    0xfb1e,
    0x8bf9,
    0x9bd8,
    0xabbb,
    0xbb9a,
    0x4a75,
    0x5a54,
    0x6a37,
    0x7a16,
    0x0af1,
    0x1ad0,
    0x2ab3,
    0x3a92,
    0xfd2e,
    0xed0f,
    0xdd6c,
    0xcd4d,
    0xbd00,
    0xad8b,
    0x9de8,
    0x8dc9,
    0x7c26,
    0x6c07,
    0x5c64,
    0x4c45,
    0x3ca2,
    0x2c83,
    0x1ce0,
    0x0cc1,
    0xef1f,
    0xff3e,
    0xcf5d,
    0xdf7c,
    0xaf9b,
    0xbfba,
    0x8fd9,
    0x9ff8,
    0x6e17,
    0x7e36,
    0x4e55,
    0x5e74,
    0x2e93,
    0x3eb2,
    0x0ed1,
    0x1ef0
};

uint16_t fast_crc16(const uint16_t initial_value, const uint8_t *data, const uint32_t data_length)
{
    uint16_t crc = initial_value;
    uint32_t idx = 0;

    for (idx = 0; idx < data_length; idx++)
    {
        crc = crc16_tab[((crc >> 8) ^ data[idx]) & 0xFF] ^ (crc << 8);
    }
    return crc;
}

void
ipv4_checksum(struct dp_packet *packet)
{
    // if (!packet->flag & IPV4_E) {
    //     VLOG_WARN("ipv42ipv6: packet does NOT have IPV4_E (ipv4_checksum)");
    // }
    // size_t offset;
    // int index;
    // get_offset_index(packet, IPV4_E, &offset, &index);
    // struct ipv4_e *ipv4 = (struct ipv4_e *)((char*)dp_packet_data(packet)+offset);
    // uint16_t *checksum = (uint16_t *)&ipv4->ip_check_e[0];
    // *checksum = 0;
    // uint8_t *data = (uint8_t*)ipv4;
    // uint32_t tmp = 0;
    // for (size_t i = 0; i < sizeof(*ipv4); i += 2) {
    //     tmp += ((data[i] << 8) + data[i+1]);
    // }
    // while (tmp > UINT16_MAX) {
    //     uint16_t high = tmp >> 16;
    //     uint16_t low = tmp & UINT16_MAX;
    //     tmp = high + low;
    // }
    // *checksum = htons((~tmp)&0xffff);
    uint8_t *crc = dp_packet_put_uninit(packet, 2);
    *(uint16_t *)crc = ntohs(fast_crc16(CRC16_INIT, dp_packet_data(packet), dp_packet_size(packet)));
}

void
push_nsh(struct dp_packet *packet, const struct nsh_hdr *nsh_hdr_src)
{
    struct nsh_hdr *nsh;
    size_t length = nsh_hdr_len(nsh_hdr_src);
    uint8_t next_proto;

    switch (ntohl(packet->packet_type)) {
        case PT_ETH:
            next_proto = NSH_P_ETHERNET;
            break;
        case PT_IPV4:
            next_proto = NSH_P_IPV4;
            break;
        case PT_IPV6:
            next_proto = NSH_P_IPV6;
            break;
        case PT_NSH:
            next_proto = NSH_P_NSH;
            break;
        default:
            OVS_NOT_REACHED();
    }

    nsh = (struct nsh_hdr *) dp_packet_resize_l2(packet, length);
    memcpy(nsh, nsh_hdr_src, length);
    nsh->next_proto = next_proto;
    packet->packet_type = htonl(PT_NSH);
    dp_packet_reset_offsets(packet);
    packet->l3_ofs = 0;
}

bool
pop_nsh(struct dp_packet *packet)
{
    struct nsh_hdr *nsh = (struct nsh_hdr *) dp_packet_l3(packet);
    size_t length;
    uint32_t next_pt;

    if (packet->packet_type == htonl(PT_NSH) && nsh) {
        switch (nsh->next_proto) {
            case NSH_P_ETHERNET:
                next_pt = PT_ETH;
                break;
            case NSH_P_IPV4:
                next_pt = PT_IPV4;
                break;
            case NSH_P_IPV6:
                next_pt = PT_IPV6;
                break;
            case NSH_P_NSH:
                next_pt = PT_NSH;
                break;
            default:
                /* Unknown inner packet type. Drop packet. */
                return false;
        }

        length = nsh_hdr_len(nsh);
        dp_packet_reset_packet(packet, length);
        packet->packet_type = htonl(next_pt);
        /* Packet must be recirculated for further processing. */
    }
    return true;
}

/* Converts hex digits in 'hex' to an Ethernet packet in '*packetp'.  The
 * caller must free '*packetp'.  On success, returns NULL.  On failure, returns
 * an error message and stores NULL in '*packetp'.
 *
 * Aligns the L3 header of '*packetp' on a 32-bit boundary. */
const char *
eth_from_hex(const char *hex, struct dp_packet **packetp)
{
    struct dp_packet *packet;

    /* Use 2 bytes of headroom to 32-bit align the L3 header. */
    packet = *packetp = dp_packet_new_with_headroom(strlen(hex) / 2, 2);

    if (dp_packet_put_hex(packet, hex, NULL)[0] != '\0') {
        dp_packet_delete(packet);
        *packetp = NULL;
        return "Trailing garbage in packet data";
    }

    if (dp_packet_size(packet) < ETH_HEADER_LEN) {
        dp_packet_delete(packet);
        *packetp = NULL;
        return "Packet data too short for Ethernet";
    }

    return NULL;
}

void
eth_format_masked(const struct eth_addr eth,
                  const struct eth_addr *mask, struct ds *s)
{
    ds_put_format(s, ETH_ADDR_FMT, ETH_ADDR_ARGS(eth));
    if (mask && !eth_mask_is_exact(*mask)) {
        ds_put_format(s, "/"ETH_ADDR_FMT, ETH_ADDR_ARGS(*mask));
    }
}

void
in6_addr_solicited_node(struct in6_addr *addr, const struct in6_addr *ip6)
{
    union ovs_16aligned_in6_addr *taddr =
        (union ovs_16aligned_in6_addr *) addr;
    memset(taddr->be16, 0, sizeof(taddr->be16));
    taddr->be16[0] = htons(0xff02);
    taddr->be16[5] = htons(0x1);
    taddr->be16[6] = htons(0xff00);
    memcpy(&addr->s6_addr[13], &ip6->s6_addr[13], 3);
}

/*
 * Generates ipv6 EUI64 address from the given eth addr
 * and prefix and stores it in 'lla'
 */
void
in6_generate_eui64(struct eth_addr ea, const struct in6_addr *prefix,
                   struct in6_addr *lla)
{
    union ovs_16aligned_in6_addr *taddr =
        (union ovs_16aligned_in6_addr *) lla;
    union ovs_16aligned_in6_addr *prefix_taddr =
        (union ovs_16aligned_in6_addr *) prefix;
    taddr->be16[0] = prefix_taddr->be16[0];
    taddr->be16[1] = prefix_taddr->be16[1];
    taddr->be16[2] = prefix_taddr->be16[2];
    taddr->be16[3] = prefix_taddr->be16[3];
    taddr->be16[4] = htons(((ea.ea[0] ^ 0x02) << 8) | ea.ea[1]);
    taddr->be16[5] = htons(ea.ea[2] << 8 | 0x00ff);
    taddr->be16[6] = htons(0xfe << 8 | ea.ea[3]);
    taddr->be16[7] = ea.be16[2];
}

/* Generates ipv6 link local address from the given eth addr
 * with prefix 'fe80::/64' and stores it in 'lla'. */
void
in6_generate_lla(struct eth_addr ea, struct in6_addr *lla)
{
    union ovs_16aligned_in6_addr *taddr =
        (union ovs_16aligned_in6_addr *) lla;
    memset(taddr->be16, 0, sizeof(taddr->be16));
    taddr->be16[0] = htons(0xfe80);
    taddr->be16[4] = htons(((ea.ea[0] ^ 0x02) << 8) | ea.ea[1]);
    taddr->be16[5] = htons(ea.ea[2] << 8 | 0x00ff);
    taddr->be16[6] = htons(0xfe << 8 | ea.ea[3]);
    taddr->be16[7] = ea.be16[2];
}

/* Returns true if 'addr' is a link local address.  Otherwise, false. */
bool
in6_is_lla(struct in6_addr *addr)
{
#ifdef s6_addr32
    return addr->s6_addr32[0] == htonl(0xfe800000) && !(addr->s6_addr32[1]);
#else
    return addr->s6_addr[0] == 0xfe && addr->s6_addr[1] == 0x80 &&
         !(addr->s6_addr[2] | addr->s6_addr[3] | addr->s6_addr[4] |
           addr->s6_addr[5] | addr->s6_addr[6] | addr->s6_addr[7]);
#endif
}

void
ipv6_multicast_to_ethernet(struct eth_addr *eth, const struct in6_addr *ip6)
{
    eth->ea[0] = 0x33;
    eth->ea[1] = 0x33;
    eth->ea[2] = ip6->s6_addr[12];
    eth->ea[3] = ip6->s6_addr[13];
    eth->ea[4] = ip6->s6_addr[14];
    eth->ea[5] = ip6->s6_addr[15];
}

/* Given the IP netmask 'netmask', returns the number of bits of the IP address
 * that it specifies, that is, the number of 1-bits in 'netmask'.
 *
 * If 'netmask' is not a CIDR netmask (see ip_is_cidr()), the return value will
 * still be in the valid range but isn't otherwise meaningful. */
int
ip_count_cidr_bits(ovs_be32 netmask)
{
    return 32 - ctz32(ntohl(netmask));
}

void
ip_format_masked(ovs_be32 ip, ovs_be32 mask, struct ds *s)
{
    ds_put_format(s, IP_FMT, IP_ARGS(ip));
    if (mask != OVS_BE32_MAX) {
        if (ip_is_cidr(mask)) {
            ds_put_format(s, "/%d", ip_count_cidr_bits(mask));
        } else {
            ds_put_format(s, "/"IP_FMT, IP_ARGS(mask));
        }
    }
}

/* Parses string 's', which must be an IP address.  Stores the IP address into
 * '*ip'.  Returns true if successful, otherwise false. */
bool
ip_parse(const char *s, ovs_be32 *ip)
{
    return inet_pton(AF_INET, s, ip) == 1;
}

/* Parses string 's', which must be an IP address with a port number
 * with ":" as a separator (e.g.: 192.168.1.2:80).
 * Stores the IP address into '*ip' and port number to '*port'.
 *
 * Returns NULL if successful, otherwise an error message that the caller must
 * free(). */
char * OVS_WARN_UNUSED_RESULT
ip_parse_port(const char *s, ovs_be32 *ip, ovs_be16 *port)
{
    int n = 0;
    if (ovs_scan(s, IP_PORT_SCAN_FMT"%n", IP_PORT_SCAN_ARGS(ip, port), &n)
        && !s[n]) {
        return NULL;
    }

    return xasprintf("%s: invalid IP address or port number", s);
}

/* Parses string 's', which must be an IP address with an optional netmask or
 * CIDR prefix length.  Stores the IP address into '*ip', netmask into '*mask',
 * (255.255.255.255, if 's' lacks a netmask), and number of scanned characters
 * into '*n'.
 *
 * Returns NULL if successful, otherwise an error message that the caller must
 * free(). */
char * OVS_WARN_UNUSED_RESULT
ip_parse_masked_len(const char *s, int *n, ovs_be32 *ip,
                    ovs_be32 *mask)
{
    int prefix;

    if (ovs_scan_len(s, n, IP_SCAN_FMT"/"IP_SCAN_FMT,
                 IP_SCAN_ARGS(ip), IP_SCAN_ARGS(mask))) {
        /* OK. */
    } else if (ovs_scan_len(s, n, IP_SCAN_FMT"/%d",
                            IP_SCAN_ARGS(ip), &prefix)) {
        if (prefix < 0 || prefix > 32) {
            return xasprintf("%s: IPv4 network prefix bits not between 0 and "
                              "32, inclusive", s);
        }
        *mask = be32_prefix_mask(prefix);
    } else if (ovs_scan_len(s, n, IP_SCAN_FMT, IP_SCAN_ARGS(ip))) {
        *mask = OVS_BE32_MAX;
    } else {
        return xasprintf("%s: invalid IP address", s);
    }
    return NULL;
}

/* This function is similar to ip_parse_masked_len(), but doesn't return the
 * number of scanned characters and expects 's' to end after the ip/(optional)
 * mask.
 *
 * Returns NULL if successful, otherwise an error message that the caller must
 * free(). */
char * OVS_WARN_UNUSED_RESULT
ip_parse_masked(const char *s, ovs_be32 *ip, ovs_be32 *mask)
{
    int n = 0;

    char *error = ip_parse_masked_len(s, &n, ip, mask);
    if (!error && s[n]) {
        return xasprintf("%s: invalid IP address", s);
    }
    return error;
}

/* Similar to ip_parse_masked_len(), but the mask, if present, must be a CIDR
 * mask and is returned as a prefix len in '*plen'. */
char * OVS_WARN_UNUSED_RESULT
ip_parse_cidr_len(const char *s, int *n, ovs_be32 *ip, unsigned int *plen)
{
    ovs_be32 mask;
    char *error;

    error = ip_parse_masked_len(s, n, ip, &mask);
    if (error) {
        return error;
    }

    if (!ip_is_cidr(mask)) {
        return xasprintf("%s: CIDR network required", s);
    }
    *plen = ip_count_cidr_bits(mask);
    return NULL;
}

/* Similar to ip_parse_cidr_len(), but doesn't return the number of scanned
 * characters and expects 's' to be NULL terminated at the end of the
 * ip/(optional) cidr. */
char * OVS_WARN_UNUSED_RESULT
ip_parse_cidr(const char *s, ovs_be32 *ip, unsigned int *plen)
{
    int n = 0;

    char *error = ip_parse_cidr_len(s, &n, ip, plen);
    if (!error && s[n]) {
        return xasprintf("%s: invalid IP address", s);
    }
    return error;
}

/* Parses string 's', which must be an IPv6 address.  Stores the IPv6 address
 * into '*ip'.  Returns true if successful, otherwise false. */
bool
ipv6_parse(const char *s, struct in6_addr *ip)
{
    return inet_pton(AF_INET6, s, ip) == 1;
}

/* Parses string 's', which must be an IPv6 address with an optional netmask or
 * CIDR prefix length.  Stores the IPv6 address into '*ip' and the netmask into
 * '*mask' (if 's' does not contain a netmask, all-one-bits is assumed), and
 * number of scanned characters into '*n'.
 *
 * Returns NULL if successful, otherwise an error message that the caller must
 * free(). */
char * OVS_WARN_UNUSED_RESULT
ipv6_parse_masked_len(const char *s, int *n, struct in6_addr *ip,
                      struct in6_addr *mask)
{
    char ipv6_s[IPV6_SCAN_LEN + 1];
    int prefix;

    if (ovs_scan_len(s, n, " "IPV6_SCAN_FMT, ipv6_s)
        && ipv6_parse(ipv6_s, ip)) {
        if (ovs_scan_len(s, n, "/%d", &prefix)) {
            if (prefix < 0 || prefix > 128) {
                return xasprintf("%s: IPv6 network prefix bits not between 0 "
                                 "and 128, inclusive", s);
            }
            *mask = ipv6_create_mask(prefix);
        } else if (ovs_scan_len(s, n, "/"IPV6_SCAN_FMT, ipv6_s)) {
             if (!ipv6_parse(ipv6_s, mask)) {
                 return xasprintf("%s: Invalid IPv6 mask", s);
             }
            /* OK. */
        } else {
            /* OK. No mask. */
            *mask = in6addr_exact;
        }
        return NULL;
    }
    return xasprintf("%s: invalid IPv6 address", s);
}

/* This function is similar to ipv6_parse_masked_len(), but doesn't return the
 * number of scanned characters and expects 's' to end following the
 * ipv6/(optional) mask. */
char * OVS_WARN_UNUSED_RESULT
ipv6_parse_masked(const char *s, struct in6_addr *ip, struct in6_addr *mask)
{
    int n = 0;

    char *error = ipv6_parse_masked_len(s, &n, ip, mask);
    if (!error && s[n]) {
        return xasprintf("%s: invalid IPv6 address", s);
    }
    return error;
}

/* Similar to ipv6_parse_masked_len(), but the mask, if present, must be a CIDR
 * mask and is returned as a prefix length in '*plen'. */
char * OVS_WARN_UNUSED_RESULT
ipv6_parse_cidr_len(const char *s, int *n, struct in6_addr *ip,
                    unsigned int *plen)
{
    struct in6_addr mask;
    char *error;

    error = ipv6_parse_masked_len(s, n, ip, &mask);
    if (error) {
        return error;
    }

    if (!ipv6_is_cidr(&mask)) {
        return xasprintf("%s: IPv6 CIDR network required", s);
    }
    *plen = ipv6_count_cidr_bits(&mask);
    return NULL;
}

/* Similar to ipv6_parse_cidr_len(), but doesn't return the number of scanned
 * characters and expects 's' to end after the ipv6/(optional) cidr. */
char * OVS_WARN_UNUSED_RESULT
ipv6_parse_cidr(const char *s, struct in6_addr *ip, unsigned int *plen)
{
    int n = 0;

    char *error = ipv6_parse_cidr_len(s, &n, ip, plen);
    if (!error && s[n]) {
        return xasprintf("%s: invalid IPv6 address", s);
    }
    return error;
}

/* Stores the string representation of the IPv6 address 'addr' into the
 * character array 'addr_str', which must be at least INET6_ADDRSTRLEN
 * bytes long. */
void
ipv6_format_addr(const struct in6_addr *addr, struct ds *s)
{
    char *dst;

    ds_reserve(s, s->length + INET6_ADDRSTRLEN);

    dst = s->string + s->length;
    inet_ntop(AF_INET6, addr, dst, INET6_ADDRSTRLEN);
    s->length += strlen(dst);
}

/* Same as print_ipv6_addr, but optionally encloses the address in square
 * brackets. */
void
ipv6_format_addr_bracket(const struct in6_addr *addr, struct ds *s,
                         bool bracket)
{
    if (bracket) {
        ds_put_char(s, '[');
    }
    ipv6_format_addr(addr, s);
    if (bracket) {
        ds_put_char(s, ']');
    }
}

void
ipv6_format_mapped(const struct in6_addr *addr, struct ds *s)
{
    if (IN6_IS_ADDR_V4MAPPED(addr)) {
        ds_put_format(s, IP_FMT, addr->s6_addr[12], addr->s6_addr[13],
                                 addr->s6_addr[14], addr->s6_addr[15]);
    } else {
        ipv6_format_addr(addr, s);
    }
}

void
ipv6_format_masked(const struct in6_addr *addr, const struct in6_addr *mask,
                   struct ds *s)
{
    ipv6_format_addr(addr, s);
    if (mask && !ipv6_mask_is_exact(mask)) {
        if (ipv6_is_cidr(mask)) {
            int cidr_bits = ipv6_count_cidr_bits(mask);
            ds_put_format(s, "/%d", cidr_bits);
        } else {
            ds_put_char(s, '/');
            ipv6_format_addr(mask, s);
        }
    }
}

/* Stores the string representation of the IPv6 address 'addr' into the
 * character array 'addr_str', which must be at least INET6_ADDRSTRLEN
 * bytes long. If addr is IPv4-mapped, store an IPv4 dotted-decimal string. */
const char *
ipv6_string_mapped(char *addr_str, const struct in6_addr *addr)
{
    ovs_be32 ip;
    ip = in6_addr_get_mapped_ipv4(addr);
    if (ip) {
        return inet_ntop(AF_INET, &ip, addr_str, INET6_ADDRSTRLEN);
    } else {
        return inet_ntop(AF_INET6, addr, addr_str, INET6_ADDRSTRLEN);
    }
}

#ifdef s6_addr32
#define s6_addrX s6_addr32
#define IPV6_FOR_EACH(VAR) for (int VAR = 0; VAR < 4; VAR++)
#else
#define s6_addrX s6_addr
#define IPV6_FOR_EACH(VAR) for (int VAR = 0; VAR < 16; VAR++)
#endif

struct in6_addr
ipv6_addr_bitand(const struct in6_addr *a, const struct in6_addr *b)
{
   struct in6_addr dst;
   IPV6_FOR_EACH (i) {
       dst.s6_addrX[i] = a->s6_addrX[i] & b->s6_addrX[i];
   }
   return dst;
}

struct in6_addr
ipv6_addr_bitxor(const struct in6_addr *a, const struct in6_addr *b)
{
   struct in6_addr dst;
   IPV6_FOR_EACH (i) {
       dst.s6_addrX[i] = a->s6_addrX[i] ^ b->s6_addrX[i];
   }
   return dst;
}

bool
ipv6_is_zero(const struct in6_addr *a)
{
   IPV6_FOR_EACH (i) {
       if (a->s6_addrX[i]) {
           return false;
       }
   }
   return true;
}

/* Returns an in6_addr consisting of 'mask' high-order 1-bits and 128-N
 * low-order 0-bits. */
struct in6_addr
ipv6_create_mask(int mask)
{
    struct in6_addr netmask;
    uint8_t *netmaskp = &netmask.s6_addr[0];

    memset(&netmask, 0, sizeof netmask);
    while (mask > 8) {
        *netmaskp = 0xff;
        netmaskp++;
        mask -= 8;
    }

    if (mask) {
        *netmaskp = 0xff << (8 - mask);
    }

    return netmask;
}

/* Given the IPv6 netmask 'netmask', returns the number of bits of the IPv6
 * address that it specifies, that is, the number of 1-bits in 'netmask'.
 * 'netmask' must be a CIDR netmask (see ipv6_is_cidr()).
 *
 * If 'netmask' is not a CIDR netmask (see ipv6_is_cidr()), the return value
 * will still be in the valid range but isn't otherwise meaningful. */
int
ipv6_count_cidr_bits(const struct in6_addr *netmask)
{
    int i;
    int count = 0;
    const uint8_t *netmaskp = &netmask->s6_addr[0];

    for (i=0; i<16; i++) {
        if (netmaskp[i] == 0xff) {
            count += 8;
        } else {
            uint8_t nm;

            for(nm = netmaskp[i]; nm; nm <<= 1) {
                count++;
            }
            break;
        }

    }

    return count;
}

/* Returns true if 'netmask' is a CIDR netmask, that is, if it consists of N
 * high-order 1-bits and 128-N low-order 0-bits. */
bool
ipv6_is_cidr(const struct in6_addr *netmask)
{
    const uint8_t *netmaskp = &netmask->s6_addr[0];
    int i;

    for (i=0; i<16; i++) {
        if (netmaskp[i] != 0xff) {
            uint8_t x = ~netmaskp[i];
            if (x & (x + 1)) {
                return false;
            }
            while (++i < 16) {
                if (netmaskp[i]) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
ipv6_addr_equals_masked(const struct in6_addr *a, const struct in6_addr *b,
                        int plen)
{
    struct in6_addr mask;
    struct in6_addr ma;
    struct in6_addr mb;

    if (plen == 128) {
        return ipv6_addr_equals(a, b);
    }

    mask = ipv6_create_mask(plen);
    ma = ipv6_addr_bitand(a, &mask);
    mb = ipv6_addr_bitand(b, &mask);

    return ipv6_addr_equals(&ma, &mb);
}

/* Populates 'b' with an Ethernet II packet headed with the given 'eth_dst',
 * 'eth_src' and 'eth_type' parameters.  A payload of 'size' bytes is allocated
 * in 'b' and returned.  This payload may be populated with appropriate
 * information by the caller.  Sets 'b''s 'frame' pointer and 'l3' offset to
 * the Ethernet header and payload respectively.  Aligns b->l3 on a 32-bit
 * boundary.
 *
 * The returned packet has enough headroom to insert an 802.1Q VLAN header if
 * desired. */
void *
eth_compose(struct dp_packet *b, const struct eth_addr eth_dst,
            const struct eth_addr eth_src, uint16_t eth_type,
            size_t size)
{
    void *data;
    struct eth_header *eth;


    dp_packet_clear(b);

    /* The magic 2 here ensures that the L3 header (when it is added later)
     * will be 32-bit aligned. */
    dp_packet_prealloc_tailroom(b, 2 + ETH_HEADER_LEN + VLAN_HEADER_LEN + size);
    dp_packet_reserve(b, 2 + VLAN_HEADER_LEN);
    eth = dp_packet_put_uninit(b, ETH_HEADER_LEN);
    data = dp_packet_put_zeros(b, size);

    eth->eth_dst = eth_dst;
    eth->eth_src = eth_src;
    eth->eth_type = htons(eth_type);

    b->packet_type = htonl(PT_ETH);
    dp_packet_set_l3(b, data);

    return data;
}

void
packet_set_ipv4_addr(struct dp_packet *packet,
                     ovs_16aligned_be32 *addr, ovs_be32 new_addr)
{
    struct ip_header *nh = dp_packet_l3(packet);
    ovs_be32 old_addr = get_16aligned_be32(addr);
    size_t l4_size = dp_packet_l4_size(packet);

    pkt_metadata_init_conn(&packet->md);

    if (nh->ip_proto == IPPROTO_TCP && l4_size >= TCP_HEADER_LEN) {
        if (dp_packet_l4_checksum_valid(packet)) {
            dp_packet_l4_checksum_set_partial(packet);
        } else {
            struct tcp_header *th = dp_packet_l4(packet);
            th->tcp_csum = recalc_csum32(th->tcp_csum, old_addr, new_addr);
        }
    } else if (nh->ip_proto == IPPROTO_UDP && l4_size >= UDP_HEADER_LEN ) {
        if (dp_packet_l4_checksum_valid(packet)) {
            dp_packet_l4_checksum_set_partial(packet);
        } else {
            struct udp_header *uh = dp_packet_l4(packet);
            if (uh->udp_csum) {
                uh->udp_csum = recalc_csum32(uh->udp_csum, old_addr, new_addr);
                if (!uh->udp_csum) {
                    uh->udp_csum = htons(0xffff);
                }
            }
        }
    }

    if (dp_packet_ip_checksum_valid(packet)) {
        dp_packet_ip_checksum_set_partial(packet);
    } else {
        nh->ip_csum = recalc_csum32(nh->ip_csum, old_addr, new_addr);
    }
    put_16aligned_be32(addr, new_addr);
}

/* Returns true, if packet contains at least one routing header where
 * segements_left > 0.
 *
 * This function assumes that L3 and L4 offsets are set in the packet. */
bool
packet_rh_present(struct dp_packet *packet, uint8_t *nexthdr, bool *first_frag)
{
    const struct ovs_16aligned_ip6_hdr *nh;
    size_t len;
    size_t remaining;
    uint8_t *data = dp_packet_l3(packet);

    remaining = packet->l4_ofs - packet->l3_ofs;
    if (remaining < sizeof *nh) {
        return false;
    }
    nh = ALIGNED_CAST(struct ovs_16aligned_ip6_hdr *, data);
    data += sizeof *nh;
    remaining -= sizeof *nh;
    *nexthdr = nh->ip6_nxt;

    while (1) {
        if ((*nexthdr != IPPROTO_HOPOPTS)
                && (*nexthdr != IPPROTO_ROUTING)
                && (*nexthdr != IPPROTO_DSTOPTS)
                && (*nexthdr != IPPROTO_AH)
                && (*nexthdr != IPPROTO_FRAGMENT)) {
            /* It's either a terminal header (e.g., TCP, UDP) or one we
             * don't understand.  In either case, we're done with the
             * packet, so use it to fill in 'nw_proto'. */
            break;
        }

        /* We only verify that at least 8 bytes of the next header are
         * available, but many of these headers are longer.  Ensure that
         * accesses within the extension header are within those first 8
         * bytes. All extension headers are required to be at least 8
         * bytes. */
        if (remaining < 8) {
            return false;
        }

        if (*nexthdr == IPPROTO_AH) {
            /* A standard AH definition isn't available, but the fields
             * we care about are in the same location as the generic
             * option header--only the header length is calculated
             * differently. */
            const struct ip6_ext *ext_hdr = (struct ip6_ext *)data;

            *nexthdr = ext_hdr->ip6e_nxt;
            len = (ext_hdr->ip6e_len + 2) * 4;
        } else if (*nexthdr == IPPROTO_FRAGMENT) {
            const struct ovs_16aligned_ip6_frag *frag_hdr
                = ALIGNED_CAST(struct ovs_16aligned_ip6_frag *, data);

            *first_frag = !(frag_hdr->ip6f_offlg & IP6F_OFF_MASK) &&
                           (frag_hdr->ip6f_offlg & IP6F_MORE_FRAG);
            *nexthdr = frag_hdr->ip6f_nxt;
            len = sizeof *frag_hdr;
        } else if (*nexthdr == IPPROTO_ROUTING) {
            const struct ip6_rthdr *rh = (struct ip6_rthdr *)data;

            if (rh->ip6r_segleft > 0) {
                return true;
            }

            *nexthdr = rh->ip6r_nxt;
            len = (rh->ip6r_len + 1) * 8;
        } else {
            const struct ip6_ext *ext_hdr = (struct ip6_ext *)data;

            *nexthdr = ext_hdr->ip6e_nxt;
            len = (ext_hdr->ip6e_len + 1) * 8;
        }

        if (remaining < len) {
            return false;
        }
        remaining -= len;
        data += len;
    }

    return false;
}

static void
packet_update_csum128(struct dp_packet *packet, uint8_t proto,
                      ovs_16aligned_be32 addr[4],
                      const struct in6_addr *new_addr)
{
    size_t l4_size = dp_packet_l4_size(packet);

    if (proto == IPPROTO_TCP && l4_size >= TCP_HEADER_LEN) {
        if (dp_packet_l4_checksum_valid(packet)) {
            dp_packet_l4_checksum_set_partial(packet);
        } else {
            struct tcp_header *th = dp_packet_l4(packet);

            th->tcp_csum = recalc_csum128(th->tcp_csum, addr, new_addr);
        }
    } else if (proto == IPPROTO_UDP && l4_size >= UDP_HEADER_LEN) {
        if (dp_packet_l4_checksum_valid(packet)) {
            dp_packet_l4_checksum_set_partial(packet);
        } else {
            struct udp_header *uh = dp_packet_l4(packet);

            if (uh->udp_csum) {
                uh->udp_csum = recalc_csum128(uh->udp_csum, addr, new_addr);
                if (!uh->udp_csum) {
                    uh->udp_csum = htons(0xffff);
                }
            }
        }
    } else if (proto == IPPROTO_ICMPV6 &&
               l4_size >= sizeof(struct icmp6_header)) {
        struct icmp6_header *icmp = dp_packet_l4(packet);

        icmp->icmp6_cksum = recalc_csum128(icmp->icmp6_cksum, addr, new_addr);
    }
}

void
packet_set_ipv6_addr(struct dp_packet *packet, uint8_t proto,
                     ovs_16aligned_be32 addr[4],
                     const struct in6_addr *new_addr,
                     bool recalculate_csum)
{
    if (recalculate_csum) {
        packet_update_csum128(packet, proto, addr, new_addr);
    }
    memcpy(addr, new_addr, sizeof(ovs_be32[4]));
    pkt_metadata_init_conn(&packet->md);
}

void
packet_set_ipv6_flow_label(ovs_16aligned_be32 *flow_label, ovs_be32 flow_key)
{
    ovs_be32 old_label = get_16aligned_be32(flow_label);
    ovs_be32 new_label = (old_label & htonl(~IPV6_LABEL_MASK)) | flow_key;
    put_16aligned_be32(flow_label, new_label);
}

void
packet_set_ipv6_tc(ovs_16aligned_be32 *flow_label, uint8_t tc)
{
    ovs_be32 old_label = get_16aligned_be32(flow_label);
    ovs_be32 new_label = (old_label & htonl(0xF00FFFFF)) | htonl(tc << 20);
    put_16aligned_be32(flow_label, new_label);
}

/* Modifies the IPv4 header fields of 'packet' to be consistent with 'src',
 * 'dst', 'tos', and 'ttl'.  Updates 'packet''s L4 checksums as appropriate.
 * 'packet' must contain a valid IPv4 packet with correctly populated l[347]
 * markers. */
void
packet_set_ipv4(struct dp_packet *packet, ovs_be32 src, ovs_be32 dst,
                uint8_t tos, uint8_t ttl)
{
    struct ip_header *nh = dp_packet_l3(packet);

    if (get_16aligned_be32(&nh->ip_src) != src) {
        packet_set_ipv4_addr(packet, &nh->ip_src, src);
    }

    if (get_16aligned_be32(&nh->ip_dst) != dst) {
        packet_set_ipv4_addr(packet, &nh->ip_dst, dst);
    }

    if (nh->ip_tos != tos) {
        uint8_t *field = &nh->ip_tos;

        if (dp_packet_ip_checksum_valid(packet)) {
            dp_packet_ip_checksum_set_partial(packet);
        } else {
            nh->ip_csum = recalc_csum16(nh->ip_csum, htons((uint16_t) *field),
                                        htons((uint16_t) tos));
        }

        *field = tos;
    }

    if (nh->ip_ttl != ttl) {
        uint8_t *field = &nh->ip_ttl;

        if (dp_packet_ip_checksum_valid(packet)) {
            dp_packet_ip_checksum_set_partial(packet);
        } else {
            nh->ip_csum = recalc_csum16(nh->ip_csum, htons(*field << 8),
                                        htons(ttl << 8));
        }

        *field = ttl;
    }
}

/* Modifies the IPv6 header fields of 'packet' to be consistent with 'src',
 * 'dst', 'traffic class', and 'next hop'.  Updates 'packet''s L4 checksums as
 * appropriate. 'packet' must contain a valid IPv6 packet with correctly
 * populated l[34] offsets. */
void
packet_set_ipv6(struct dp_packet *packet, const struct in6_addr *src,
                const struct in6_addr *dst, uint8_t key_tc, ovs_be32 key_fl,
                uint8_t key_hl)
{
    struct ovs_16aligned_ip6_hdr *nh = dp_packet_l3(packet);
    bool recalc_csum = true;
    uint8_t proto = 0;
    bool rh_present;

    rh_present = packet_rh_present(packet, &proto, &recalc_csum);

    if (memcmp(&nh->ip6_src, src, sizeof(ovs_be32[4]))) {
        packet_set_ipv6_addr(packet, proto, nh->ip6_src.be32,
                             src, recalc_csum);
    }

    if (memcmp(&nh->ip6_dst, dst, sizeof(ovs_be32[4]))) {
        packet_set_ipv6_addr(packet, proto, nh->ip6_dst.be32, dst,
                             !rh_present && recalc_csum);
    }

    packet_set_ipv6_tc(&nh->ip6_flow, key_tc);
    packet_set_ipv6_flow_label(&nh->ip6_flow, key_fl);
    nh->ip6_hlim = key_hl;
}

static void
packet_set_port(ovs_be16 *port, ovs_be16 new_port, ovs_be16 *csum)
{
    if (*port != new_port) {
        if (csum) {
            *csum = recalc_csum16(*csum, *port, new_port);
        }
        *port = new_port;
    }
}

/* Sets the TCP source and destination port ('src' and 'dst' respectively) of
 * the TCP header contained in 'packet'.  'packet' must be a valid TCP packet
 * with its l4 offset properly populated. */
void
packet_set_tcp_port(struct dp_packet *packet, ovs_be16 src, ovs_be16 dst)
{
    struct tcp_header *th = dp_packet_l4(packet);
    ovs_be16 *csum = NULL;

    if (dp_packet_l4_checksum_valid(packet)) {
        dp_packet_l4_checksum_set_partial(packet);
    } else {
        csum = &th->tcp_csum;
    }

    packet_set_port(&th->tcp_src, src, csum);
    packet_set_port(&th->tcp_dst, dst, csum);
    pkt_metadata_init_conn(&packet->md);
}

/* Sets the UDP source and destination port ('src' and 'dst' respectively) of
 * the UDP header contained in 'packet'.  'packet' must be a valid UDP packet
 * with its l4 offset properly populated. */
void
packet_set_udp_port(struct dp_packet *packet, ovs_be16 src, ovs_be16 dst)
{
    struct udp_header *uh = dp_packet_l4(packet);

    if (dp_packet_l4_checksum_valid(packet)) {
        dp_packet_l4_checksum_set_partial(packet);
        packet_set_port(&uh->udp_src, src, NULL);
        packet_set_port(&uh->udp_dst, dst, NULL);
    } else {
        ovs_be16 *csum = uh->udp_csum ? &uh->udp_csum : NULL;

        packet_set_port(&uh->udp_src, src, csum);
        packet_set_port(&uh->udp_dst, dst, csum);

        if (csum && !uh->udp_csum) {
            uh->udp_csum = htons(0xffff);
        }
    }

    pkt_metadata_init_conn(&packet->md);
}

/* Sets the SCTP source and destination port ('src' and 'dst' respectively) of
 * the SCTP header contained in 'packet'.  'packet' must be a valid SCTP packet
 * with its l4 offset properly populated. */
void
packet_set_sctp_port(struct dp_packet *packet, ovs_be16 src, ovs_be16 dst)
{
    struct sctp_header *sh = dp_packet_l4(packet);

    if (dp_packet_l4_checksum_valid(packet)) {
        dp_packet_l4_checksum_set_partial(packet);
        sh->sctp_src = src;
        sh->sctp_dst = dst;
    } else {
        ovs_be32 old_csum, old_correct_csum, new_csum;
        uint16_t tp_len = dp_packet_l4_size(packet);

        old_csum = get_16aligned_be32(&sh->sctp_csum);
        put_16aligned_be32(&sh->sctp_csum, 0);
        old_correct_csum = crc32c((void *) sh, tp_len);

        sh->sctp_src = src;
        sh->sctp_dst = dst;

        new_csum = crc32c((void *) sh, tp_len);
        put_16aligned_be32(&sh->sctp_csum, old_csum ^ old_correct_csum
                           ^ new_csum);
    }

    pkt_metadata_init_conn(&packet->md);
}

/* Sets the ICMP type and code of the ICMP header contained in 'packet'.
 * 'packet' must be a valid ICMP packet with its l4 offset properly
 * populated. */
void
packet_set_icmp(struct dp_packet *packet, uint8_t type, uint8_t code)
{
    struct icmp_header *ih = dp_packet_l4(packet);
    ovs_be16 orig_tc = htons(ih->icmp_type << 8 | ih->icmp_code);
    ovs_be16 new_tc = htons(type << 8 | code);

    if (orig_tc != new_tc) {
        ih->icmp_type = type;
        ih->icmp_code = code;

        ih->icmp_csum = recalc_csum16(ih->icmp_csum, orig_tc, new_tc);
    }
    pkt_metadata_init_conn(&packet->md);
}

/* Sets the IGMP type to IGMP_HOST_MEMBERSHIP_QUERY and populates the
 * v3 query header fields in 'packet'. 'packet' must be a valid IGMPv3
 * query packet with its l4 offset properly populated.
 */
void
packet_set_igmp3_query(struct dp_packet *packet, uint8_t max_resp,
                       ovs_be32 group, bool srs, uint8_t qrv, uint8_t qqic)
{
    struct igmpv3_query_header *igh = dp_packet_l4(packet);
    ovs_be16 orig_type_max_resp =
        htons(igh->type << 8 | igh->max_resp);
    ovs_be16 new_type_max_resp =
        htons(IGMP_HOST_MEMBERSHIP_QUERY << 8 | max_resp);

    if (orig_type_max_resp != new_type_max_resp) {
        igh->type = IGMP_HOST_MEMBERSHIP_QUERY;
        igh->max_resp = max_resp;
        igh->csum = recalc_csum16(igh->csum, orig_type_max_resp,
                                  new_type_max_resp);
    }

    ovs_be32 old_group = get_16aligned_be32(&igh->group);

    if (old_group != group) {
        put_16aligned_be32(&igh->group, group);
        igh->csum = recalc_csum32(igh->csum, old_group, group);
    }

    /* See RFC 3376 4.1.6. */
    if (qrv > 7) {
        qrv = 0;
    }

    ovs_be16 orig_srs_qrv_qqic = htons(igh->srs_qrv << 8 | igh->qqic);
    ovs_be16 new_srs_qrv_qqic = htons(srs << 11 | qrv << 8 | qqic);

    if (orig_srs_qrv_qqic != new_srs_qrv_qqic) {
        igh->srs_qrv = (srs << 3 | qrv);
        igh->qqic = qqic;
        igh->csum = recalc_csum16(igh->csum, orig_srs_qrv_qqic,
                                  new_srs_qrv_qqic);
    }
}

void
packet_set_nd_ext(struct dp_packet *packet, const ovs_16aligned_be32 rso_flags,
                  const uint8_t opt_type)
{
    struct ovs_nd_msg *ns;
    struct ovs_nd_lla_opt *opt;
    int bytes_remain = dp_packet_l4_size(packet);
    struct ovs_16aligned_ip6_hdr * nh = dp_packet_l3(packet);
    uint32_t pseudo_hdr_csum = 0;

    if (OVS_UNLIKELY(bytes_remain < sizeof(*ns))) {
        return;
    }

    if (nh) {
        pseudo_hdr_csum = packet_csum_pseudoheader6(nh);
    }

    ns = dp_packet_l4(packet);
    opt = &ns->options[0];

    /* set RSO flags and option type */
    ns->rso_flags = rso_flags;
    opt->type = opt_type;

    /* recalculate checksum */
    ovs_be16 *csum_value = &(ns->icmph.icmp6_cksum);
    *csum_value = 0;
    *csum_value = csum_finish(csum_continue(pseudo_hdr_csum,
                              &(ns->icmph), bytes_remain));

}

void
packet_set_nd(struct dp_packet *packet, const struct in6_addr *target,
              const struct eth_addr sll, const struct eth_addr tll)
{
    struct ovs_nd_msg *ns;
    struct ovs_nd_lla_opt *opt;
    int bytes_remain = dp_packet_l4_size(packet);

    if (OVS_UNLIKELY(bytes_remain < sizeof(*ns))) {
        return;
    }

    ns = dp_packet_l4(packet);
    opt = &ns->options[0];
    bytes_remain -= sizeof(*ns);

    if (memcmp(&ns->target, target, sizeof(ovs_be32[4]))) {
        packet_set_ipv6_addr(packet, IPPROTO_ICMPV6, ns->target.be32, target,
                             true);
    }

    while (bytes_remain >= ND_LLA_OPT_LEN && opt->len != 0) {
        if (opt->type == ND_OPT_SOURCE_LINKADDR && opt->len == 1) {
            if (!eth_addr_equals(opt->mac, sll)) {
                ovs_be16 *csum = &(ns->icmph.icmp6_cksum);

                *csum = recalc_csum48(*csum, opt->mac, sll);
                opt->mac = sll;
            }

            /* A packet can only contain one SLL or TLL option */
            break;
        } else if (opt->type == ND_OPT_TARGET_LINKADDR && opt->len == 1) {
            if (!eth_addr_equals(opt->mac, tll)) {
                ovs_be16 *csum = &(ns->icmph.icmp6_cksum);

                *csum = recalc_csum48(*csum, opt->mac, tll);
                opt->mac = tll;
            }

            /* A packet can only contain one SLL or TLL option */
            break;
        }

        opt += opt->len;
        bytes_remain -= opt->len * ND_LLA_OPT_LEN;
    }
}

const char *
packet_tcp_flag_to_string(uint32_t flag)
{
    switch (flag) {
    case TCP_FIN:
        return "fin";
    case TCP_SYN:
        return "syn";
    case TCP_RST:
        return "rst";
    case TCP_PSH:
        return "psh";
    case TCP_ACK:
        return "ack";
    case TCP_URG:
        return "urg";
    case TCP_ECE:
        return "ece";
    case TCP_CWR:
        return "cwr";
    case TCP_NS:
        return "ns";
    case 0x200:
        return "[200]";
    case 0x400:
        return "[400]";
    case 0x800:
        return "[800]";
    default:
        return NULL;
    }
}

/* Appends a string representation of the TCP flags value 'tcp_flags'
 * (e.g. from struct flow.tcp_flags or obtained via TCP_FLAGS) to 's', in the
 * format used by tcpdump. */
void
packet_format_tcp_flags(struct ds *s, uint16_t tcp_flags)
{
    if (!tcp_flags) {
        ds_put_cstr(s, "none");
        return;
    }

    if (tcp_flags & TCP_SYN) {
        ds_put_char(s, 'S');
    }
    if (tcp_flags & TCP_FIN) {
        ds_put_char(s, 'F');
    }
    if (tcp_flags & TCP_PSH) {
        ds_put_char(s, 'P');
    }
    if (tcp_flags & TCP_RST) {
        ds_put_char(s, 'R');
    }
    if (tcp_flags & TCP_URG) {
        ds_put_char(s, 'U');
    }
    if (tcp_flags & TCP_ACK) {
        ds_put_char(s, '.');
    }
    if (tcp_flags & TCP_ECE) {
        ds_put_cstr(s, "E");
    }
    if (tcp_flags & TCP_CWR) {
        ds_put_cstr(s, "C");
    }
    if (tcp_flags & TCP_NS) {
        ds_put_cstr(s, "N");
    }
    if (tcp_flags & 0x200) {
        ds_put_cstr(s, "[200]");
    }
    if (tcp_flags & 0x400) {
        ds_put_cstr(s, "[400]");
    }
    if (tcp_flags & 0x800) {
        ds_put_cstr(s, "[800]");
    }
}

#define ARP_PACKET_SIZE  (2 + ETH_HEADER_LEN + VLAN_HEADER_LEN + \
                          ARP_ETH_HEADER_LEN)

/* Clears 'b' and replaces its contents by an ARP frame with the specified
 * 'arp_op', 'arp_sha', 'arp_tha', 'arp_spa', and 'arp_tpa'.  The outer
 * Ethernet frame is initialized with Ethernet source 'arp_sha' and destination
 * 'arp_tha', except that destination ff:ff:ff:ff:ff:ff is used instead if
 * 'broadcast' is true.  Points the L3 header to the ARP header. */
void
compose_arp(struct dp_packet *b, uint16_t arp_op,
            const struct eth_addr arp_sha, const struct eth_addr arp_tha,
            bool broadcast, ovs_be32 arp_spa, ovs_be32 arp_tpa)
{
    compose_arp__(b);

    struct eth_header *eth = dp_packet_eth(b);
    eth->eth_dst = broadcast ? eth_addr_broadcast : arp_tha;
    eth->eth_src = arp_sha;

    struct arp_eth_header *arp = dp_packet_l3(b);
    arp->ar_op = htons(arp_op);
    arp->ar_sha = arp_sha;
    arp->ar_tha = arp_tha;
    put_16aligned_be32(&arp->ar_spa, arp_spa);
    put_16aligned_be32(&arp->ar_tpa, arp_tpa);
}

/* Clears 'b' and replaces its contents by an ARP frame.  Sets the fields in
 * the Ethernet and ARP headers that are fixed for ARP frames to those fixed
 * values, and zeroes the other fields.  Points the L3 header to the ARP
 * header. */
void
compose_arp__(struct dp_packet *b)
{
    dp_packet_clear(b);
    dp_packet_prealloc_tailroom(b, ARP_PACKET_SIZE);
    dp_packet_reserve(b, 2 + VLAN_HEADER_LEN);

    struct eth_header *eth = dp_packet_put_zeros(b, sizeof *eth);
    eth->eth_type = htons(ETH_TYPE_ARP);

    struct arp_eth_header *arp = dp_packet_put_zeros(b, sizeof *arp);
    arp->ar_hrd = htons(ARP_HRD_ETHERNET);
    arp->ar_pro = htons(ARP_PRO_IP);
    arp->ar_hln = sizeof arp->ar_sha;
    arp->ar_pln = sizeof arp->ar_spa;

    dp_packet_set_l3(b, arp);

    b->packet_type = htonl(PT_ETH);
}

/* This function expects packet with ethernet header with correct
 * l3 pointer set. */
void *
compose_ipv6(struct dp_packet *packet, uint8_t proto,
             const struct in6_addr *src, const struct in6_addr *dst,
             uint8_t key_tc, ovs_be32 key_fl, uint8_t key_hl, int size)
{
    struct ovs_16aligned_ip6_hdr *nh;
    void *data;

    nh = dp_packet_l3(packet);
    nh->ip6_vfc = 0x60;
    nh->ip6_nxt = proto;
    nh->ip6_plen = htons(size);
    data = dp_packet_put_zeros(packet, size);
    dp_packet_set_l4(packet, data);
    packet_set_ipv6(packet, src, dst, key_tc, key_fl, key_hl);
    return data;
}

/* Compose an IPv6 Neighbor Discovery Neighbor Solicitation message. */
void
compose_nd_ns(struct dp_packet *b, const struct eth_addr eth_src,
              const struct in6_addr *ipv6_src, const struct in6_addr *ipv6_dst)
{
    struct in6_addr sn_addr;
    struct eth_addr eth_dst;
    struct ovs_nd_msg *ns;
    struct ovs_nd_lla_opt *lla_opt;
    uint32_t icmp_csum;

    in6_addr_solicited_node(&sn_addr, ipv6_dst);
    ipv6_multicast_to_ethernet(&eth_dst, &sn_addr);

    eth_compose(b, eth_dst, eth_src, ETH_TYPE_IPV6, IPV6_HEADER_LEN);
    ns = compose_ipv6(b, IPPROTO_ICMPV6, ipv6_src, &sn_addr,
                      0, 0, 255, ND_MSG_LEN + ND_LLA_OPT_LEN);

    ns->icmph.icmp6_type = ND_NEIGHBOR_SOLICIT;
    ns->icmph.icmp6_code = 0;
    put_16aligned_be32(&ns->rso_flags, htonl(0));

    lla_opt = &ns->options[0];
    lla_opt->type = ND_OPT_SOURCE_LINKADDR;
    lla_opt->len = 1;

    packet_set_nd(b, ipv6_dst, eth_src, eth_addr_zero);

    ns->icmph.icmp6_cksum = 0;
    icmp_csum = packet_csum_pseudoheader6(dp_packet_l3(b));
    ns->icmph.icmp6_cksum = csum_finish(
        csum_continue(icmp_csum, ns, ND_MSG_LEN + ND_LLA_OPT_LEN));
}

/* Compose an IPv6 Neighbor Discovery Neighbor Advertisement message. */
void
compose_nd_na(struct dp_packet *b,
              const struct eth_addr eth_src, const struct eth_addr eth_dst,
              const struct in6_addr *ipv6_src, const struct in6_addr *ipv6_dst,
              ovs_be32 rso_flags)
{
    struct ovs_nd_msg *na;
    struct ovs_nd_lla_opt *lla_opt;
    uint32_t icmp_csum;

    eth_compose(b, eth_dst, eth_src, ETH_TYPE_IPV6, IPV6_HEADER_LEN);
    na = compose_ipv6(b, IPPROTO_ICMPV6, ipv6_src, ipv6_dst,
                      0, 0, 255, ND_MSG_LEN + ND_LLA_OPT_LEN);

    na->icmph.icmp6_type = ND_NEIGHBOR_ADVERT;
    na->icmph.icmp6_code = 0;
    put_16aligned_be32(&na->rso_flags, rso_flags);

    lla_opt = &na->options[0];
    lla_opt->type = ND_OPT_TARGET_LINKADDR;
    lla_opt->len = 1;

    packet_set_nd(b, ipv6_src, eth_addr_zero, eth_src);

    na->icmph.icmp6_cksum = 0;
    icmp_csum = packet_csum_pseudoheader6(dp_packet_l3(b));
    na->icmph.icmp6_cksum = csum_finish(csum_continue(
        icmp_csum, na, ND_MSG_LEN + ND_LLA_OPT_LEN));
}

/* Compose an IPv6 Neighbor Discovery Router Advertisement message with
 * Source Link-layer Address Option and MTU Option.
 * Caller can call packet_put_ra_prefix_opt to append Prefix Information
 * Options to composed messags in 'b'. */
void
compose_nd_ra(struct dp_packet *b,
              const struct eth_addr eth_src, const struct eth_addr eth_dst,
              const struct in6_addr *ipv6_src, const struct in6_addr *ipv6_dst,
              uint8_t cur_hop_limit, uint8_t mo_flags,
              ovs_be16 router_lt, ovs_be32 reachable_time,
              ovs_be32 retrans_timer, uint32_t mtu)
{
    /* Don't compose Router Advertisement packet with MTU Option if mtu
     * value is 0. */
    bool with_mtu = mtu != 0;
    size_t mtu_opt_len = with_mtu ? ND_MTU_OPT_LEN : 0;

    eth_compose(b, eth_dst, eth_src, ETH_TYPE_IPV6, IPV6_HEADER_LEN);

    struct ovs_ra_msg *ra = compose_ipv6(
        b, IPPROTO_ICMPV6, ipv6_src, ipv6_dst, 0, 0, 255,
        RA_MSG_LEN + ND_LLA_OPT_LEN + mtu_opt_len);
    ra->icmph.icmp6_type = ND_ROUTER_ADVERT;
    ra->icmph.icmp6_code = 0;
    ra->cur_hop_limit = cur_hop_limit;
    ra->mo_flags = mo_flags;
    ra->router_lifetime = router_lt;
    ra->reachable_time = reachable_time;
    ra->retrans_timer = retrans_timer;

    struct ovs_nd_lla_opt *lla_opt = ra->options;
    lla_opt->type = ND_OPT_SOURCE_LINKADDR;
    lla_opt->len = 1;
    lla_opt->mac = eth_src;

    if (with_mtu) {
        /* ovs_nd_mtu_opt has the same size with ovs_nd_lla_opt. */
        struct ovs_nd_mtu_opt *mtu_opt
            = (struct ovs_nd_mtu_opt *)(lla_opt + 1);
        mtu_opt->type = ND_OPT_MTU;
        mtu_opt->len = 1;
        mtu_opt->reserved = 0;
        put_16aligned_be32(&mtu_opt->mtu, htonl(mtu));
    }

    ra->icmph.icmp6_cksum = 0;
    uint32_t icmp_csum = packet_csum_pseudoheader6(dp_packet_l3(b));
    ra->icmph.icmp6_cksum = csum_finish(csum_continue(
        icmp_csum, ra, RA_MSG_LEN + ND_LLA_OPT_LEN + mtu_opt_len));
}

/* Append an IPv6 Neighbor Discovery Prefix Information option to a
 * Router Advertisement message. */
void
packet_put_ra_prefix_opt(struct dp_packet *b,
                         uint8_t plen, uint8_t la_flags,
                         ovs_be32 valid_lifetime, ovs_be32 preferred_lifetime,
                         const ovs_be128 prefix)
{
    size_t prev_l4_size = dp_packet_l4_size(b);
    struct ovs_16aligned_ip6_hdr *nh = dp_packet_l3(b);
    nh->ip6_plen = htons(prev_l4_size + ND_PREFIX_OPT_LEN);

    struct ovs_nd_prefix_opt *prefix_opt =
        dp_packet_put_uninit(b, sizeof *prefix_opt);
    prefix_opt->type = ND_OPT_PREFIX_INFORMATION;
    prefix_opt->len = 4;
    prefix_opt->prefix_len = plen;
    prefix_opt->la_flags = la_flags;
    put_16aligned_be32(&prefix_opt->valid_lifetime, valid_lifetime);
    put_16aligned_be32(&prefix_opt->preferred_lifetime, preferred_lifetime);
    put_16aligned_be32(&prefix_opt->reserved, 0);
    memcpy(prefix_opt->prefix.be32, prefix.be32, sizeof(ovs_be32[4]));

    struct ovs_ra_msg *ra = dp_packet_l4(b);
    ra->icmph.icmp6_cksum = 0;
    uint32_t icmp_csum = packet_csum_pseudoheader6(dp_packet_l3(b));
    ra->icmph.icmp6_cksum = csum_finish(csum_continue(
        icmp_csum, ra, prev_l4_size + ND_PREFIX_OPT_LEN));
}

uint32_t
packet_csum_pseudoheader(const struct ip_header *ip)
{
    uint32_t partial = 0;

    partial = csum_add32(partial, get_16aligned_be32(&ip->ip_src));
    partial = csum_add32(partial, get_16aligned_be32(&ip->ip_dst));
    partial = csum_add16(partial, htons(ip->ip_proto));
    partial = csum_add16(partial, htons(ntohs(ip->ip_tot_len) -
                                        IP_IHL(ip->ip_ihl_ver) * 4));

    return partial;
}

#ifndef __CHECKER__
uint32_t
packet_csum_pseudoheader6(const struct ovs_16aligned_ip6_hdr *ip6)
{
    uint32_t partial = 0;

    partial = csum_continue(partial, &ip6->ip6_src, sizeof ip6->ip6_src);
    partial = csum_continue(partial, &ip6->ip6_dst, sizeof ip6->ip6_dst);
    partial = csum_add16(partial, htons(ip6->ip6_nxt));
    partial = csum_add16(partial, ip6->ip6_plen);

    return partial;
}

/* Calculate the IPv6 upper layer checksum according to RFC2460. We pass the
   ip6_nxt and ip6_plen values, so it will also work if extension headers
   are present. */
ovs_be16
packet_csum_upperlayer6(const struct ovs_16aligned_ip6_hdr *ip6,
                        const void *data, uint8_t l4_protocol,
                        uint16_t l4_size)
{
    uint32_t partial = 0;

    partial = csum_continue(partial, &ip6->ip6_src, sizeof ip6->ip6_src);
    partial = csum_continue(partial, &ip6->ip6_dst, sizeof ip6->ip6_dst);
    partial = csum_add16(partial, htons(l4_protocol));
    partial = csum_add16(partial, htons(l4_size));

    partial = csum_continue(partial, data, l4_size);

    return csum_finish(partial);
}
#endif

void
IP_ECN_set_ce(struct dp_packet *pkt, bool is_ipv6)
{
    if (is_ipv6) {
        ovs_16aligned_be32 *ip6 = dp_packet_l3(pkt);

        put_16aligned_be32(ip6, get_16aligned_be32(ip6) |
                                htonl(IP_ECN_CE << 20));
    } else {
        struct ip_header *nh = dp_packet_l3(pkt);
        uint8_t tos = nh->ip_tos;

        tos |= IP_ECN_CE;
        if (nh->ip_tos != tos) {
            if (dp_packet_ip_checksum_valid(pkt)) {
                dp_packet_ip_checksum_set_partial(pkt);
            } else {
                nh->ip_csum = recalc_csum16(nh->ip_csum, htons(nh->ip_tos),
                                            htons((uint16_t) tos));
            }

            nh->ip_tos = tos;
        }
    }
}

/* Set TCP checksum field in packet 'p' with complete checksum.
 * The packet must have the L3 and L4 offsets. */
void
packet_tcp_complete_csum(struct dp_packet *p, bool inner)
{
    struct tcp_header *tcp;
    size_t tcp_sz;
    void *ip_hdr;

    if (inner) {
        tcp = dp_packet_inner_l4(p);
        ip_hdr = dp_packet_inner_l3(p);
        tcp_sz = dp_packet_inner_l4_size(p);
    } else {
        tcp = dp_packet_l4(p);
        ip_hdr = dp_packet_l3(p);
        tcp_sz = dp_packet_l4_size(p);
    }

    ovs_assert(tcp);
    ovs_assert(ip_hdr);

    tcp->tcp_csum = 0;
    if (IP_VER(((const struct ip_header *) ip_hdr)->ip_ihl_ver) == 4) {
        struct ip_header *ip = ip_hdr;

        tcp->tcp_csum = csum_finish(csum_continue(packet_csum_pseudoheader(ip),
                                                  tcp, tcp_sz));
    } else {
        struct ovs_16aligned_ip6_hdr *ip6 = ip_hdr;

        tcp->tcp_csum = packet_csum_upperlayer6(ip6, tcp, ip6->ip6_nxt,
                                                tcp_sz);
    }

    if (inner) {
        dp_packet_inner_l4_checksum_set_good(p);
    } else {
        dp_packet_l4_checksum_set_good(p);
    }
}

/* Set UDP checksum field in packet 'p' with complete checksum.
 * The packet must have the L3 and L4 offsets. */
void
packet_udp_complete_csum(struct dp_packet *p, bool inner)
{
    struct udp_header *udp;
    size_t udp_sz;
    void *ip_hdr;

    if (inner) {
        udp = dp_packet_inner_l4(p);
        ip_hdr = dp_packet_inner_l3(p);
        udp_sz = dp_packet_inner_l4_size(p);
    } else {
        udp = dp_packet_l4(p);
        ip_hdr = dp_packet_l3(p);
        udp_sz = dp_packet_l4_size(p);
    }

    ovs_assert(udp);
    ovs_assert(ip_hdr);

    /* Skip csum calculation if the udp_csum is zero. */
    if (!udp->udp_csum) {
        goto out;
    }

    udp->udp_csum = 0;
    if (IP_VER(((const struct ip_header *) ip_hdr)->ip_ihl_ver) == 4) {
        struct ip_header *ip = ip_hdr;

        udp->udp_csum = csum_finish(csum_continue(packet_csum_pseudoheader(ip),
                                                  udp, udp_sz));
    } else {
        struct ovs_16aligned_ip6_hdr *ip6 = ip_hdr;

        udp->udp_csum = packet_csum_upperlayer6(ip6, udp, ip6->ip6_nxt,
                                                udp_sz);
    }

    if (!udp->udp_csum) {
        udp->udp_csum = htons(0xffff);
    }

out:
    if (inner) {
        dp_packet_inner_l4_checksum_set_good(p);
    } else {
        dp_packet_l4_checksum_set_good(p);
    }
}

/* This helper computes a "constant" UDP checksum without looking at the
 * L4 payload.
 *
 * This is possible when L4 is either TCP or UDP: the L4 payload checksum
 * is either computed in SW or in HW later, but its contribution to the
 * outer checksum is cancelled by the L4 payload being part of the global
 * packet sum. */
bool
packet_udp_tunnel_csum(struct dp_packet *p)
{
    struct ip_header *inner_ip;
    const void *inner_l4_data;
    char *after_inner_l4_csum;
    size_t inner_l4_csum_off;
    struct udp_header *udp;
    ovs_be16 inner_l4_csum;
    uint32_t partial_csum;
    struct ip_header *ip;
    uint32_t inner_csum;
    uint16_t tso_segsz;
    bool inner_ipv4;
    void *inner_l4;

    inner_ip = dp_packet_inner_l3(p);
    inner_l4 = dp_packet_inner_l4(p);
    ip = dp_packet_l3(p);
    udp = dp_packet_l4(p);

    if (dp_packet_inner_l4_proto_tcp(p)) {
        inner_l4_csum_off = offsetof(struct tcp_header, tcp_csum);
        inner_l4_data = dp_packet_get_inner_tcp_payload(p);
        if (!inner_l4_data) {
            /* Malformed packet. */
            return false;
        }
    } else if (dp_packet_inner_l4_proto_udp(p)) {
        inner_l4_csum_off = offsetof(struct udp_header, udp_csum);
        inner_l4_data = (char *) inner_l4 + sizeof (struct udp_header);
        if (((struct udp_header *) inner_l4)->udp_csum == 0) {
            /* There is no nested checksum.
             * No choice but compute a full checksum. */
            return false;
        }
    } else {
        /* This optimisation applies only to inner TCP/UDP. */
        return false;
    }

    if (!dp_packet_inner_l4_checksum_valid(p)) {
        /* We have no idea about the contribution of the payload data
         * and what the L4 checksum put in the packet data looks like.
         * Simpler is to let a full checksum happen. */
        return false;
    }

    inner_ipv4 = IP_VER(inner_ip->ip_ihl_ver) == 4;
    if (inner_ipv4) {
        inner_csum = packet_csum_pseudoheader(inner_ip);
    } else {
        struct ovs_16aligned_ip6_hdr *inner_ip6 = dp_packet_inner_l3(p);

        inner_csum = packet_csum_pseudoheader6(inner_ip6);
    }

    inner_csum = csum_continue(inner_csum, inner_l4, inner_l4_csum_off);
    after_inner_l4_csum = (char *) inner_l4 + inner_l4_csum_off + 2;
    inner_l4_csum = csum_finish(csum_continue(inner_csum, after_inner_l4_csum,
        (char *) inner_l4_data - after_inner_l4_csum));
    /* Important: for inner UDP, a null inner_l4_csum here should in theory be
     * replaced with 0xffff.  However, since the only use of inner_l4_csum is
     * for the final outer checksum with a csum_add16() below, we can skip this
     * entirely because adding 0xffff will have the same effect as adding 0x0
     * after reducing in csum_finish. */

    udp->udp_csum = 0;
    if (IP_VER(ip->ip_ihl_ver) == 4) {
        partial_csum = packet_csum_pseudoheader(ip);
    } else {
        struct ovs_16aligned_ip6_hdr *ip6 = dp_packet_l3(p);

        partial_csum = packet_csum_pseudoheader6(ip6);
    }

    partial_csum = csum_continue(partial_csum, udp,
        (char *) inner_ip - (char *) udp);
    if (!inner_ipv4 || !dp_packet_inner_ip_checksum_valid(p)) {
        /* IPv6 has no checksum, so for inner IPv6, we need to sum the header.
         *
         * In IPv4 case, if inner checksum is already good or HW offload
         * has been requested, the (final) sum of the IPv4 header will be 0.
         * Otherwise, we need to sum the header like for IPv6. */
        partial_csum = csum_continue(partial_csum, inner_ip,
            (char *) inner_l4 - (char *) inner_ip);
    }
    partial_csum = csum_continue(partial_csum, inner_l4, inner_l4_csum_off);
    partial_csum = csum_add16(partial_csum, inner_l4_csum);
    partial_csum = csum_continue(partial_csum, after_inner_l4_csum,
        (char *) inner_l4_data - after_inner_l4_csum);
    udp->udp_csum = csum_finish(partial_csum);
    tso_segsz = dp_packet_get_tso_segsz(p);
    if (tso_segsz) {
        uint16_t payload_len = dp_packet_get_inner_tcp_payload_length(p);

        ovs_assert(payload_len == tso_segsz * dp_packet_gso_nr_segs(p));

        /* The pseudo header used in the outer UDP checksum is dependent on
         * the ip_tot_len / ip6_plen which was a reflection of the TSO frame
         * size. The segmented packet will be shorter. */
        udp->udp_csum = recalc_csum16(udp->udp_csum, htons(payload_len),
                                      htons(tso_segsz));

        /* When segmenting the packet, various headers get updated:
         * - inner L3
         *   - for IPv4, ip_tot_len is updated, BUT it is not affecting the
         *     outer UDP checksum because the IPv4 header itself contains
         *     a checksum that compensates for this update,
         *   - for IPv6, ip6_plen is updated, and this must be considered,
         * - inner L4
         *   - inner pseudo header used in the TCP checksum is dependent on
         *     the inner ip_tot_len / ip6_plen,
         *   - TCP seq number is updated,
         *   - the HW may change some TCP flags (think PSH/FIN),
         *   BUT the TCP checksum will compensate for those updates,
         *
         * Summary: we only care about the inner IPv6 header update.
         */
        if (IP_VER(inner_ip->ip_ihl_ver) != 4) {
            udp->udp_csum = recalc_csum16(udp->udp_csum, htons(payload_len),
                                          htons(tso_segsz));
        }
    }
    if (!udp->udp_csum) {
        udp->udp_csum = htons(0xffff);
    }
    dp_packet_l4_checksum_set_good(p);

    return true;
}

/* Set SCTP checksum field in packet 'p' with complete checksum.
 * The packet must have the L3 and L4 offsets. */
void
packet_sctp_complete_csum(struct dp_packet *p, bool inner)
{
    struct sctp_header *sh;
    uint16_t tp_len;
    ovs_be32 csum;

    if (inner) {
        sh = dp_packet_inner_l4(p);
        tp_len = dp_packet_inner_l4_size(p);
    } else {
        sh = dp_packet_l4(p);
        tp_len = dp_packet_l4_size(p);
    }

    ovs_assert(sh);

    put_16aligned_be32(&sh->sctp_csum, 0);
    csum = crc32c((void *) sh, tp_len);
    put_16aligned_be32(&sh->sctp_csum, csum);

    if (inner) {
        dp_packet_inner_l4_checksum_set_good(p);
    } else {
        dp_packet_l4_checksum_set_good(p);
    }
}
