/* Copyright (c) 2013, 2014, 2015, 2017 Nicira, Inc.
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
 * limitations under the License. */

#include <config.h>
#include <errno.h>

#include "byte-order.h"
#include "connectivity.h"
#include "csum.h"
#include "dpif.h"
#include "openvswitch/dynamic-string.h"
#include "fat-rwlock.h"
#include "hash.h"
#include "openvswitch/hmap.h"
#include "netdev.h"
#include "odp-util.h"
#include "openvswitch/ofpbuf.h"
#include "packets.h"
#include "route-table.h"
#include "seq.h"
#include "smap.h"
#include "socket-util.h"
#include "tnl-ports.h"
#include "tunnel.h"
#include "openvswitch/vlog.h"
#include "unaligned.h"
#include "unixctl.h"
#include "ofproto-dpif.h"
#include "netdev-vport.h"

VLOG_DEFINE_THIS_MODULE(tunnel);

////////////////////////////////////////////////
// 引用下方
// 添加数据结构
/* MGRE IP table for storing all MGRE tunnel IPs (IPv4 and IPv6) */
static struct hmap mgre_ip_table = HMAP_INITIALIZER(&mgre_ip_table);
static struct ovs_mutex mgre_ip_mutex = OVS_MUTEX_INITIALIZER;

struct mgre_ip_entry {
    struct hmap_node node;
    struct in6_addr ip_addr; /* IPv6 address or IPv4-mapped IPv6 address */
    bool is_ipv6;            /* True for native IPv6, false for IPv4 */
};

/* Subnet MGRE Neighbor Table for mapping destination IPs/subnets to remote tunnel IPs 
 * Optimized with prefix length buckets for faster lookup
 * Supports both IPv4 (0-32) and IPv6 (0-128) prefixes */

/* Hash tables organized by prefix length (0-128) for O(prefix_count) lookup */
#define MAX_PREFIX_LEN_IPV4 33
#define MAX_PREFIX_LEN_IPV6 129
#define MAX_PREFIX_LEN MAX_PREFIX_LEN_IPV6
static struct hmap subnet_mgre_neighbor_tables[MAX_PREFIX_LEN];
static struct ovs_mutex subnet_mgre_neighbor_mutex = OVS_MUTEX_INITIALIZER;
static bool subnet_mgre_tables_initialized = false;

struct subnet_mgre_neighbor_entry {
    struct hmap_node node;
    struct in6_addr dest_ip;     /* Destination network address (IPv6 or IPv4-mapped) */
    struct in6_addr dest_mask;   /* Subnet mask (IPv6 or IPv4-mapped) */
    uint8_t prefix_len;          /* Prefix length (0-32 for IPv4, 0-128 for IPv6) */
    struct in6_addr remote_ip;   /* Remote tunnel IP (IPv6 or IPv4-mapped) */
    bool dest_is_ipv6;           /* True if dest_ip is native IPv6, false if IPv4 */
    bool remote_is_ipv6;         /* True if remote_ip is native IPv6, false if IPv4 */
};

/* Initialize subnet MGRE neighbor tables */
static void
subnet_mgre_neighbor_tables_init(void)
{
    if (!subnet_mgre_tables_initialized) {
        for (int i = 0; i < MAX_PREFIX_LEN; i++) {
            hmap_init(&subnet_mgre_neighbor_tables[i]);
        }
        subnet_mgre_tables_initialized = true;
    }
}

/* Convert IPv4 mask to prefix length */
static uint8_t
mask_to_prefix_len_ipv4(uint32_t mask)
{
    uint8_t len = 0;
    while (mask) {
        len++;
        mask <<= 1;
    }
    return len;
}

/* Convert IPv6 mask to prefix length */
static uint8_t
mask_to_prefix_len_ipv6(const struct in6_addr *mask)
{
    uint8_t len = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t byte = mask->s6_addr[i];
        if (byte == 0xff) {
            len += 8;
        } else {
            while (byte & 0x80) {
                len++;
                byte <<= 1;
            }
            break;
        }
    }
    return len;
}

/* Create IPv6 mask from prefix length */
static void
prefix_len_to_mask_ipv6(uint8_t prefix_len, struct in6_addr *mask)
{
    memset(mask, 0, sizeof *mask);
    int bytes = prefix_len / 8;
    int bits = prefix_len % 8;
    
    for (int i = 0; i < bytes && i < 16; i++) {
        mask->s6_addr[i] = 0xff;
    }
    if (bytes < 16 && bits > 0) {
        mask->s6_addr[bytes] = 0xff << (8 - bits);
    }
}

/* Apply IPv6 mask to address */
static void
apply_ipv6_mask(const struct in6_addr *addr, const struct in6_addr *mask,
                struct in6_addr *result)
{
    for (int i = 0; i < 16; i++) {
        result->s6_addr[i] = addr->s6_addr[i] & mask->s6_addr[i];
    }
}

/* Add an IP address (IPv4 or IPv6) to MGRE IP table */
static void
mgre_ip_add(const struct in6_addr *ip_addr, bool is_ipv6)
{
    struct mgre_ip_entry *entry;
    
    ovs_mutex_lock(&mgre_ip_mutex);
    
    /* Check if IP already exists */
    uint32_t hash = hash_bytes(ip_addr, sizeof *ip_addr, is_ipv6 ? 1 : 0);
    HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &mgre_ip_table) {
        if (ipv6_addr_equals(&entry->ip_addr, ip_addr)) {
            ovs_mutex_unlock(&mgre_ip_mutex);
            return; /* Already exists */
        }
    }
    
    /* Add new entry */
    entry = xmalloc(sizeof *entry);
    entry->ip_addr = *ip_addr;
    entry->is_ipv6 = is_ipv6;
    hmap_insert(&mgre_ip_table, &entry->node, hash);
    
    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop(is_ipv6 ? AF_INET6 : AF_INET, 
              is_ipv6 ? (void*)ip_addr : (void*)&ip_addr->s6_addr[12],
              addr_str, sizeof addr_str);
    VLOG_INFO("Added MGRE IP: %s (%s)", addr_str, is_ipv6 ? "IPv6" : "IPv4");
    ovs_mutex_unlock(&mgre_ip_mutex);
}

/* Check if an IP address (IPv4 or IPv6) is in MGRE IP table */
bool
mgre_ip_exists(const struct in6_addr *ip_addr, bool is_ipv6)
{
    struct mgre_ip_entry *entry;
    bool found = false;
    
    ovs_mutex_lock(&mgre_ip_mutex);
    uint32_t hash = hash_bytes(ip_addr, sizeof *ip_addr, is_ipv6 ? 1 : 0);
    HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &mgre_ip_table) {
        if (ipv6_addr_equals(&entry->ip_addr, ip_addr)) {
            found = true;
            break;
        }
    }
    ovs_mutex_unlock(&mgre_ip_mutex);
    
    return found;
}

/* Remove an IP address (IPv4 or IPv6) from MGRE IP table */
static void
mgre_ip_remove(const struct in6_addr *ip_addr, bool is_ipv6)
{
    struct mgre_ip_entry *entry;
    
    ovs_mutex_lock(&mgre_ip_mutex);
    uint32_t hash = hash_bytes(ip_addr, sizeof *ip_addr, is_ipv6 ? 1 : 0);
    HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &mgre_ip_table) {
        if (ipv6_addr_equals(&entry->ip_addr, ip_addr)) {
            hmap_remove(&mgre_ip_table, &entry->node);
            char addr_str[INET6_ADDRSTRLEN];
            inet_ntop(is_ipv6 ? AF_INET6 : AF_INET,
                      is_ipv6 ? (void*)ip_addr : (void*)&ip_addr->s6_addr[12],
                      addr_str, sizeof addr_str);
            VLOG_INFO("Removed MGRE IP: %s (%s)", addr_str, is_ipv6 ? "IPv6" : "IPv4");
            free(entry);
            break;
        }
    }
    ovs_mutex_unlock(&mgre_ip_mutex);
}

/* List all IPs in MGRE IP table */
static void
mgre_ip_list_all(struct ds *ds)
{
    struct mgre_ip_entry *entry;
    bool first = true;
    
    ovs_mutex_lock(&mgre_ip_mutex);
    if (hmap_is_empty(&mgre_ip_table)) {
        ds_put_cstr(ds, "No MGRE IPs configured");
    } else {
        ds_put_cstr(ds, "MGRE IPs:\n");
        HMAP_FOR_EACH(entry, node, &mgre_ip_table) {
            char addr_str[INET6_ADDRSTRLEN];
            if (entry->is_ipv6) {
                inet_ntop(AF_INET6, &entry->ip_addr, addr_str, sizeof addr_str);
                ds_put_format(ds, "  %s (IPv6)\n", addr_str);
            } else {
                ovs_be32 ipv4 = in6_addr_get_mapped_ipv4(&entry->ip_addr);
                inet_ntop(AF_INET, &ipv4, addr_str, sizeof addr_str);
                ds_put_format(ds, "  %s (IPv4)\n", addr_str);
            }
        }
    }
    ovs_mutex_unlock(&mgre_ip_mutex);
}

/* Clear all IPs from MGRE IP table */
static void
mgre_ip_clear_all(void)
{
    struct mgre_ip_entry *entry, *next;
    int count = 0;
    
    ovs_mutex_lock(&mgre_ip_mutex);
    HMAP_FOR_EACH_SAFE(entry, next, node, &mgre_ip_table) {
        hmap_remove(&mgre_ip_table, &entry->node);
        free(entry);
        count++;
    }
    ovs_mutex_unlock(&mgre_ip_mutex);
    
    VLOG_INFO("Cleared %d MGRE IP(s) from table", count);
}

/* Add a subnet/host route to the MGRE neighbor table (supports IPv4 + IPv6) */
static void
subnet_mgre_neighbor_add(const struct in6_addr *dest_ip, 
                          const struct in6_addr *dest_mask,
                          const struct in6_addr *remote_ip,
                          uint8_t prefix_len, 
                          bool dest_is_ipv6,
                          bool remote_is_ipv6)
{
    struct subnet_mgre_neighbor_entry *entry;
    struct in6_addr dest_net;
    apply_ipv6_mask(dest_ip, dest_mask, &dest_net);
    uint32_t hash = hash_bytes(&dest_net, sizeof dest_net, prefix_len);
    
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    subnet_mgre_neighbor_tables_init();
    
    /* Check if entry already exists in the prefix-specific table */
    HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &subnet_mgre_neighbor_tables[prefix_len]) {
        if (ipv6_addr_equals(&entry->dest_ip, &dest_net)) {
            /* Update existing entry */
            entry->remote_ip = *remote_ip;
            entry->remote_is_ipv6 = remote_is_ipv6;
            
            char dest_str[INET6_ADDRSTRLEN], remote_str[INET6_ADDRSTRLEN];
            inet_ntop(dest_is_ipv6 ? AF_INET6 : AF_INET,
                      dest_is_ipv6 ? (void*)&dest_net : (void*)&dest_net.s6_addr[12],
                      dest_str, sizeof dest_str);
            inet_ntop(remote_is_ipv6 ? AF_INET6 : AF_INET,
                      remote_is_ipv6 ? (void*)remote_ip : (void*)&remote_ip->s6_addr[12],
                      remote_str, sizeof remote_str);
            VLOG_INFO("Updated subnet MGRE neighbor: %s/%u -> %s", 
                      dest_str, prefix_len, remote_str);
            ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
            return;
        }
    }
    
    /* Add new entry to prefix-specific table */
    entry = xmalloc(sizeof *entry);
    entry->dest_ip = dest_net;
    entry->dest_mask = *dest_mask;
    entry->prefix_len = prefix_len;
    entry->remote_ip = *remote_ip;
    entry->dest_is_ipv6 = dest_is_ipv6;
    entry->remote_is_ipv6 = remote_is_ipv6;
    hmap_insert(&subnet_mgre_neighbor_tables[prefix_len], &entry->node, hash);
    
    char dest_str[INET6_ADDRSTRLEN], remote_str[INET6_ADDRSTRLEN];
    inet_ntop(dest_is_ipv6 ? AF_INET6 : AF_INET,
              dest_is_ipv6 ? (void*)&dest_net : (void*)&dest_net.s6_addr[12],
              dest_str, sizeof dest_str);
    inet_ntop(remote_is_ipv6 ? AF_INET6 : AF_INET,
              remote_is_ipv6 ? (void*)remote_ip : (void*)&remote_ip->s6_addr[12],
              remote_str, sizeof remote_str);
    VLOG_INFO("Added subnet MGRE neighbor: %s/%u -> %s", 
              dest_str, prefix_len, remote_str);
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
}

/* Lookup remote IP for a given destination IP in subnet MGRE neighbor table (supports IPv4 + IPv6)
 * Returns true if found, with remote_ip filled; false otherwise
 * Time complexity: O(prefix_count) - typically O(1) to O(10) instead of O(n)
 * Supports both IPv4 (0-32) and IPv6 (0-128) prefixes */
static bool
subnet_mgre_neighbor_lookup(const struct in6_addr *dest_ip, bool is_ipv6,
                             struct in6_addr *remote_ip, bool *remote_is_ipv6)
{
    struct subnet_mgre_neighbor_entry *entry;
    
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    
    if (!subnet_mgre_tables_initialized) {
        ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
        return false;
    }
    
    /* Debug: log the query */
    char query_str[INET6_ADDRSTRLEN];
    inet_ntop(is_ipv6 ? AF_INET6 : AF_INET,
              is_ipv6 ? (void*)dest_ip : (void*)&dest_ip->s6_addr[12],
              query_str, sizeof query_str);
    VLOG_INFO("Neighbor lookup query: %s (is_ipv6=%d)", query_str, is_ipv6);
    
    /* Search from longest prefix to shortest
     * For IPv4: /120 to /96 (IPv4-mapped IPv6), For IPv6: /128 to /0
     * Return immediately when first match is found (longest prefix match) */
    int max_prefix = is_ipv6 ? 128 : 128;  /* IPv4-mapped uses /96-/128 */
    int min_prefix = is_ipv6 ? 0 : 96;     /* IPv4-mapped starts at /96 */
    for (int prefix_len = max_prefix; prefix_len >= min_prefix; prefix_len--) {
        if (hmap_is_empty(&subnet_mgre_neighbor_tables[prefix_len])) {
            continue;  /* Skip empty prefix tables */
        }
        
        /* Calculate the network address for this prefix length */
        struct in6_addr mask, network;
        prefix_len_to_mask_ipv6(prefix_len, &mask);
        apply_ipv6_mask(dest_ip, &mask, &network);
        uint32_t hash = hash_bytes(&network, sizeof network, prefix_len);
        
        /* Debug: log the search */
        char network_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &network, network_str, sizeof network_str);
        VLOG_DBG("Searching prefix_len=%d, network=%s, table_size=%zu", 
                 prefix_len, network_str, hmap_count(&subnet_mgre_neighbor_tables[prefix_len]));
        
        /* Look for exact match in this prefix table */
        HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &subnet_mgre_neighbor_tables[prefix_len]) {
            char entry_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &entry->dest_ip, entry_str, sizeof entry_str);
            VLOG_DBG("Comparing with entry: %s/%u (dest_is_ipv6=%d)", 
                     entry_str, entry->prefix_len, entry->dest_is_ipv6);
            
            if (ipv6_addr_equals(&entry->dest_ip, &network)) {
                /* Found longest prefix match */
                *remote_ip = entry->remote_ip;
                *remote_is_ipv6 = entry->remote_is_ipv6;
                
                char dest_str[INET6_ADDRSTRLEN], remote_str[INET6_ADDRSTRLEN];
                inet_ntop(entry->dest_is_ipv6 ? AF_INET6 : AF_INET,
                          entry->dest_is_ipv6 ? (void*)dest_ip : (void*)&dest_ip->s6_addr[12],
                          dest_str, sizeof dest_str);
                inet_ntop(entry->remote_is_ipv6 ? AF_INET6 : AF_INET,
                          entry->remote_is_ipv6 ? (void*)remote_ip : (void*)&remote_ip->s6_addr[12],
                          remote_str, sizeof remote_str);
                VLOG_INFO("Subnet MGRE neighbor lookup MATCH: %s -> %s (matched prefix /%u)",
                         dest_str, remote_str, prefix_len);
                
                ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
                return true;
            }
        }
    }
    
    /* No match found */
    VLOG_INFO("Neighbor lookup FAILED for %s (is_ipv6=%d)", query_str, is_ipv6);
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
    return false;
}

/* Remove a subnet/host route from the MGRE neighbor table (supports IPv4 + IPv6) */
static bool
subnet_mgre_neighbor_remove(const struct in6_addr *dest_ip,
                             const struct in6_addr *dest_mask,
                             uint8_t prefix_len, bool dest_is_ipv6)
{
    struct subnet_mgre_neighbor_entry *entry;
    struct in6_addr dest_net;
    apply_ipv6_mask(dest_ip, dest_mask, &dest_net);
    uint32_t hash = hash_bytes(&dest_net, sizeof dest_net, prefix_len);
    bool found = false;
    
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    
    if (!subnet_mgre_tables_initialized) {
        ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
        return found;
    }
    
    /* Exact match: look up in the specific prefix_len table */
    HMAP_FOR_EACH_WITH_HASH(entry, node, hash, &subnet_mgre_neighbor_tables[prefix_len]) {
        if (ipv6_addr_equals(&entry->dest_ip, &dest_net)) {
            hmap_remove(&subnet_mgre_neighbor_tables[prefix_len], &entry->node);
            char dest_str[INET6_ADDRSTRLEN];
            inet_ntop(entry->dest_is_ipv6 ? AF_INET6 : AF_INET,
                      entry->dest_is_ipv6 ? (void*)&dest_net : (void*)&dest_net.s6_addr[12],
                      dest_str, sizeof dest_str);
            VLOG_INFO("Removed subnet MGRE neighbor: %s/%u", dest_str, prefix_len);
            free(entry);
            found = true;
            break;
        }
    }
    
    if (!found) {
        char dest_str[INET6_ADDRSTRLEN];
        inet_ntop(dest_is_ipv6 ? AF_INET6 : AF_INET,
                  dest_is_ipv6 ? (void*)&dest_net : (void*)&dest_net.s6_addr[12],
                  dest_str, sizeof dest_str);
        VLOG_WARN("Failed to remove subnet MGRE neighbor: %s/%u not found", dest_str, prefix_len);
    }
    
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
    return found;
}

/* List all entries in subnet MGRE neighbor table (supports IPv4 and IPv6) */
static void
subnet_mgre_neighbor_list_all(struct ds *ds)
{
    struct subnet_mgre_neighbor_entry *entry;
    int total_count = 0;
    
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    
    if (!subnet_mgre_tables_initialized) {
        ds_put_cstr(ds, "No subnet MGRE neighbors configured");
        ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
        return;
    }
    
    /* Count total entries */
    for (int i = 0; i < MAX_PREFIX_LEN; i++) {
        total_count += hmap_count(&subnet_mgre_neighbor_tables[i]);
    }
    
    if (total_count == 0) {
        ds_put_cstr(ds, "No subnet MGRE neighbors configured");
    } else {
        ds_put_format(ds, "Subnet MGRE Neighbor Table (%d entries):\n", total_count);
        ds_put_cstr(ds, "Destination Network          -> Remote IP\n");
        ds_put_cstr(ds, "------------------------------------------------\n");
        
        /* List from longest to shortest prefix for better readability */
        /* Check both IPv4 (0-32) and IPv6 (33-128) ranges */
        for (int prefix_len = 128; prefix_len >= 0; prefix_len--) {
            if (hmap_is_empty(&subnet_mgre_neighbor_tables[prefix_len])) {
                continue;
            }
            
            HMAP_FOR_EACH(entry, node, &subnet_mgre_neighbor_tables[prefix_len]) {
                char dest_str[INET6_ADDRSTRLEN], remote_str[INET6_ADDRSTRLEN];
                int display_prefix_len;
                
                /* Format destination address */
                if (entry->dest_is_ipv6) {
                    inet_ntop(AF_INET6, &entry->dest_ip, dest_str, sizeof dest_str);
                    display_prefix_len = prefix_len;
                } else {
                    ovs_be32 dest_ipv4 = in6_addr_get_mapped_ipv4(&entry->dest_ip);
                    inet_ntop(AF_INET, &dest_ipv4, dest_str, sizeof dest_str);
                    /* For IPv4, display actual IPv4 prefix length (subtract 96) */
                    display_prefix_len = prefix_len - 96;
                }
                
                /* Format remote address */
                if (entry->remote_is_ipv6) {
                    inet_ntop(AF_INET6, &entry->remote_ip, remote_str, sizeof remote_str);
                } else {
                    ovs_be32 remote_ipv4 = in6_addr_get_mapped_ipv4(&entry->remote_ip);
                    inet_ntop(AF_INET, &remote_ipv4, remote_str, sizeof remote_str);
                }
                
                ds_put_format(ds, "%-28s/%u -> %s\n", dest_str, display_prefix_len, remote_str);
            }
        }
    }
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
}

/* Clear all entries from subnet MGRE neighbor table (optimized) */
static void
subnet_mgre_neighbor_clear_all(void)
{
    struct subnet_mgre_neighbor_entry *entry, *next;
    int count = 0;
    
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    
    if (!subnet_mgre_tables_initialized) {
        ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
        VLOG_INFO("Cleared 0 subnet MGRE neighbor(s) from table");
        return;
    }
    
    /* Clear all prefix tables */
    for (int i = 0; i < MAX_PREFIX_LEN; i++) {
        HMAP_FOR_EACH_SAFE(entry, next, node, &subnet_mgre_neighbor_tables[i]) {
            hmap_remove(&subnet_mgre_neighbor_tables[i], &entry->node);
            free(entry);
            count++;
        }
    }
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
    
    VLOG_INFO("Cleared %d subnet MGRE neighbor(s) from table", count);
}
////////////////////////////////////////////////

struct tnl_match {
    ovs_be64 in_key;
    struct in6_addr ipv6_src;
    struct in6_addr ipv6_dst;
    odp_port_t odp_port;
    bool in_key_flow;
    bool ip_src_flow;
    bool ip_dst_flow;
    enum netdev_pt_mode pt_mode;
};

struct tnl_port {
    struct hmap_node ofport_node;
    struct hmap_node match_node;

    const struct ofport_dpif *ofport;
    uint64_t change_seq;
    struct netdev *netdev;

    struct tnl_match match;
};

static struct fat_rwlock rwlock;

/* Tunnel matches.
 *
 * This module maps packets received over tunnel protocols to vports.  The
 * tunnel protocol and, for some protocols, tunnel-specific information (e.g.,
 * for VXLAN, the UDP destination port number) are always use as part of the
 * mapping.  Which other fields are used for the mapping depends on the vports
 * themselves (the parenthesized notations refer to "struct tnl_match" fields):
 *
 *     - in_key: A vport may match a specific tunnel ID (in_key_flow == false)
 *       or arrange for the tunnel ID to be matched as tunnel.tun_id in the
 *       OpenFlow flow (in_key_flow == true).
 *
 *     - ip_dst: A vport may match a specific destination IP address
 *       (ip_dst_flow == false) or arrange for the destination IP to be matched
 *       as tunnel.ip_dst in the OpenFlow flow (ip_dst_flow == true).
 *
 *     - ip_src: A vport may match a specific IP source address (ip_src_flow ==
 *       false, ip_src != 0), wildcard all source addresses (ip_src_flow ==
 *       false, ip_src == 0), or arrange for the IP source address to be
 *       handled in the OpenFlow flow table (ip_src_flow == true).
 *
 * Thus, there are 2 * 2 * 3 == 12 possible ways a vport can match against a
 * tunnel packet.  We number the possibilities for each field in increasing
 * order as listed in each bullet above.  We order the 12 overall combinations
 * in lexicographic order considering in_key first, then ip_dst, then
 * ip_src. */
#define N_MATCH_TYPES (2 * 2 * 3)

/* The three possibilities (see above) for vport ip_src matches. */
enum ip_src_type {
    IP_SRC_CFG,             /* ip_src must equal configured address. */
    IP_SRC_ANY,             /* Any ip_src is acceptable. */
    IP_SRC_FLOW             /* ip_src is handled in flow table. */
};

/* Each hmap contains "struct tnl_port"s.
 * The index is a combination of how each of the fields listed under "Tunnel
 * matches" above matches, see the final paragraph for ordering. */
static struct hmap *tnl_match_maps[N_MATCH_TYPES] OVS_GUARDED_BY(rwlock);
static struct hmap **tnl_match_map(const struct tnl_match *);

static struct hmap ofport_map__ = HMAP_INITIALIZER(&ofport_map__);
static struct hmap *ofport_map OVS_GUARDED_BY(rwlock) = &ofport_map__;

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(1, 5);
static struct vlog_rate_limit dbg_rl = VLOG_RATE_LIMIT_INIT(60, 60);

static struct tnl_port *tnl_find(const struct flow *) OVS_REQ_RDLOCK(rwlock);
static struct tnl_port *tnl_find_exact(struct tnl_match *, struct hmap *)
    OVS_REQ_RDLOCK(rwlock);
static struct tnl_port *tnl_find_ofport(const struct ofport_dpif *)
    OVS_REQ_RDLOCK(rwlock);

static uint32_t tnl_hash(struct tnl_match *);
static void tnl_match_fmt(const struct tnl_match *, struct ds *);
static char *tnl_port_to_string(const struct tnl_port *)
    OVS_REQ_RDLOCK(rwlock);
static void tnl_port_format(const struct tnl_port *, struct ds *)
    OVS_REQ_RDLOCK(rwlock);
static void tnl_port_mod_log(const struct tnl_port *, const char *action)
    OVS_REQ_RDLOCK(rwlock);
static const char *tnl_port_get_name(const struct tnl_port *)
    OVS_REQ_RDLOCK(rwlock);
static void tnl_port_del__(const struct ofport_dpif *, odp_port_t)
    OVS_REQ_WRLOCK(rwlock);

static unixctl_cb_func tnl_unixctl_list;

//////////////////////////////////////////
static unixctl_cb_func mgre_ip_add_cmd;
static unixctl_cb_func mgre_ip_del_cmd;
static unixctl_cb_func mgre_ip_list_cmd;
static unixctl_cb_func mgre_ip_clear_cmd;
static unixctl_cb_func subnet_mgre_neighbor_add_cmd;
static unixctl_cb_func subnet_mgre_neighbor_del_cmd;
static unixctl_cb_func subnet_mgre_neighbor_list_cmd;
static unixctl_cb_func subnet_mgre_neighbor_clear_cmd;
/* Helper function forward declaration */
static bool parse_ipv6_subnet(const char *str, struct in6_addr *ip, 
                              struct in6_addr *mask, uint8_t *prefix_len);
//////////////////////////////////////////

void
ofproto_tunnel_init(void)
{

    //////////////////////////////////////////////

    // static struct ovsthread_once once = OVSTHREAD_ONCE_INITIALIZER;

    // if (ovsthread_once_start(&once)) {
    //     fat_rwlock_init(&rwlock);
    //     unixctl_command_register("ofproto/list-tunnels", "", 0, 0,
    //                              tnl_unixctl_list, NULL);
    //     ovsthread_once_done(&once);
    // }
    
    static struct ovsthread_once once = OVSTHREAD_ONCE_INITIALIZER;

    if (ovsthread_once_start(&once)) {
        fat_rwlock_init(&rwlock);
        unixctl_command_register("ofproto/list-tunnels", "", 0, 0,
                                 tnl_unixctl_list, NULL);
        /* MGRE IP management commands */
        unixctl_command_register("mgre/ip-add", "IP [IP ...]", 1, INT_MAX,
                                 mgre_ip_add_cmd, NULL);
        unixctl_command_register("mgre/ip-del", "IP [IP ...]", 1, INT_MAX,
                                 mgre_ip_del_cmd, NULL);
        unixctl_command_register("mgre/ip-clear", "", 0, 0,
                                 mgre_ip_clear_cmd, NULL);
        unixctl_command_register("mgre/ip-list", "", 0, 0,
                                 mgre_ip_list_cmd, NULL);
        /* Subnet MGRE neighbor table management commands */
        unixctl_command_register("mgre/neighbor-add", "<dest_subnet> <remote_ip>", 2, 2,
                                 subnet_mgre_neighbor_add_cmd, NULL);
        unixctl_command_register("mgre/neighbor-del", "<dest_subnet>", 1, 1,
                                 subnet_mgre_neighbor_del_cmd, NULL);
        unixctl_command_register("mgre/neighbor-list", "", 0, 0,
                                 subnet_mgre_neighbor_list_cmd, NULL);
        unixctl_command_register("mgre/neighbor-clear", "", 0, 0,
                                 subnet_mgre_neighbor_clear_cmd, NULL);
        ovsthread_once_done(&once);
    //////////////////////////////////////////////
    }
}

static bool
tnl_port_add__(const struct ofport_dpif *ofport, const struct netdev *netdev,
               odp_port_t odp_port, bool warn, bool native_tnl, const char name[])
    OVS_REQ_WRLOCK(rwlock)
{
    const struct netdev_tunnel_config *cfg;
    struct tnl_port *existing_port;
    struct tnl_port *tnl_port;
    struct hmap **map;

    cfg = netdev_get_tunnel_config(netdev);
    ovs_assert(cfg);

    tnl_port = xzalloc(sizeof *tnl_port);
    tnl_port->ofport = ofport;
    tnl_port->netdev = netdev_ref(netdev);
    tnl_port->change_seq = netdev_get_change_seq(tnl_port->netdev);

    tnl_port->match.in_key = cfg->in_key;
    tnl_port->match.ipv6_src = cfg->ipv6_src;
    tnl_port->match.ipv6_dst = cfg->ipv6_dst;
    tnl_port->match.ip_src_flow = cfg->ip_src_flow;
    tnl_port->match.ip_dst_flow = cfg->ip_dst_flow;
    tnl_port->match.in_key_flow = cfg->in_key_flow;
    tnl_port->match.odp_port = odp_port;
    tnl_port->match.pt_mode = netdev_get_pt_mode(netdev);

    map = tnl_match_map(&tnl_port->match);
    existing_port = tnl_find_exact(&tnl_port->match, *map);
    if (existing_port) {
        if (warn) {
            struct ds ds = DS_EMPTY_INITIALIZER;
            tnl_match_fmt(&tnl_port->match, &ds);
            VLOG_WARN("%s: attempting to add tunnel port with same config as "
                      "port '%s' (%s)", tnl_port_get_name(tnl_port),
                      tnl_port_get_name(existing_port), ds_cstr(&ds));
            ds_destroy(&ds);
        }
        netdev_close(tnl_port->netdev);
        free(tnl_port);
        return false;
    }

    hmap_insert(ofport_map, &tnl_port->ofport_node, hash_pointer(ofport, 0));

    if (!*map) {
        *map = xmalloc(sizeof **map);
        hmap_init(*map);
    }
    hmap_insert(*map, &tnl_port->match_node, tnl_hash(&tnl_port->match));
    tnl_port_mod_log(tnl_port, "adding");

    if (native_tnl) {
        const char *type;

        type = netdev_get_type(netdev);
        tnl_port_map_insert(odp_port, cfg->dst_port, name, type);

    }
    return true;
}

/* Adds 'ofport' to the module with datapath port number 'odp_port'. 'ofport's
 * must be added before they can be used by the module. 'ofport' must be a
 * tunnel.
 *
 * Returns 0 if successful, otherwise a positive errno value. */
int
tnl_port_add(const struct ofport_dpif *ofport, const struct netdev *netdev,
             odp_port_t odp_port, bool native_tnl, const char name[])
    OVS_EXCLUDED(rwlock)
{
    bool ok;

    fat_rwlock_wrlock(&rwlock);
    ok = tnl_port_add__(ofport, netdev, odp_port, true, native_tnl, name);
    fat_rwlock_unlock(&rwlock);

    return ok ? 0 : EEXIST;
}

/* Checks if the tunnel represented by 'ofport' reconfiguration due to changes
 * in its netdev_tunnel_config. If it does, returns true. Otherwise, returns
 * false. 'new_odp_port' should be the port number coming from 'ofport' that
 * is passed to tnl_port_add__(). 'old_odp_port' should be the port number
 * that is passed to tnl_port_del__(). */
bool
tnl_port_reconfigure(const struct ofport_dpif *ofport,
                     const struct netdev *netdev, odp_port_t new_odp_port,
                     odp_port_t old_odp_port, bool native_tnl,
                     const char name[])
    OVS_EXCLUDED(rwlock)
{
    struct tnl_port *tnl_port;
    bool changed = false;

    fat_rwlock_wrlock(&rwlock);
    tnl_port = tnl_find_ofport(ofport);
    if (!tnl_port) {
        changed = tnl_port_add__(ofport, netdev, new_odp_port, false,
                                 native_tnl, name);
    } else if (tnl_port->netdev != netdev
               || tnl_port->match.odp_port != new_odp_port
               || tnl_port->change_seq != netdev_get_change_seq(tnl_port->netdev)) {
        VLOG_DBG("reconfiguring %s", tnl_port_get_name(tnl_port));
        tnl_port_del__(ofport, old_odp_port);
        tnl_port_add__(ofport, netdev, new_odp_port, true, native_tnl, name);
        changed = true;
    }
    fat_rwlock_unlock(&rwlock);
    return changed;
}

static void
tnl_port_del__(const struct ofport_dpif *ofport, odp_port_t odp_port)
    OVS_REQ_WRLOCK(rwlock)
{
    struct tnl_port *tnl_port;

    if (!ofport) {
        return;
    }

    tnl_port = tnl_find_ofport(ofport);
    if (tnl_port) {
        struct hmap **map;

        tnl_port_map_delete(odp_port, netdev_get_type(tnl_port->netdev));
        tnl_port_mod_log(tnl_port, "removing");
        map = tnl_match_map(&tnl_port->match);
        hmap_remove(*map, &tnl_port->match_node);
        if (hmap_is_empty(*map)) {
            hmap_destroy(*map);
            free(*map);
            *map = NULL;
        }
        hmap_remove(ofport_map, &tnl_port->ofport_node);
        netdev_close(tnl_port->netdev);
        free(tnl_port);
    }
}

/* Removes 'ofport' from the module. */
void
tnl_port_del(const struct ofport_dpif *ofport, odp_port_t odp_port)
    OVS_EXCLUDED(rwlock)
{
    fat_rwlock_wrlock(&rwlock);
    tnl_port_del__(ofport, odp_port);
    fat_rwlock_unlock(&rwlock);
}

/* Looks in the table of tunnels for a tunnel matching the metadata in 'flow'.
 * Returns the 'ofport' corresponding to the new in_port, or a null pointer if
 * none is found.
 *
 * Callers should verify that 'flow' needs to be received by calling
 * tnl_port_should_receive() before this function. */
const struct ofport_dpif *
tnl_port_receive(const struct flow *flow) OVS_EXCLUDED(rwlock)
{
    ////////////////////////////////////////
//     const struct ofport_dpif *ofport;
//     struct tnl_port *tnl_port;

//     fat_rwlock_rdlock(&rwlock);
//     tnl_port = tnl_find(flow);
//     ofport = tnl_port ? tnl_port->ofport : NULL;
//     if (!tnl_port) {
//         if (!VLOG_DROP_WARN(&rl)) {
//             char *flow_str = flow_to_string(flow, NULL);
//             VLOG_WARN("receive tunnel port not found (%s)", flow_str);
//             free(flow_str);
//         }
//         goto out;
//     }

//     if (!VLOG_DROP_DBG(&dbg_rl)) {
//         char *flow_str = flow_to_string(flow, NULL);
//         char *tnl_str = tnl_port_to_string(tnl_port);
//         VLOG_DBG("tunnel port %s receive from flow %s", tnl_str, flow_str);
//         free(tnl_str);
//         free(flow_str);
//     }

// out:
//     fat_rwlock_unlock(&rwlock);
//     return ofport;


    const struct ofport_dpif *ofport;
    struct tnl_port *tnl_port;

    /* Create a copy of flow for MGRE processing */
    struct flow flow_for_mgre = *flow;

    /* Check if tunnel source is in MGRE IP table and modify accordingly */
    /* For GRE tunnels (check tp_dst == 0 to identify GRE, as GRE doesn't use UDP/TCP ports) */
    if (flow_for_mgre.tunnel.tp_dst == 0) {
        /* Check for IPv4 tunnel */
        if (flow_for_mgre.tunnel.ip_src != 0) {
            struct in6_addr src_ip6;
            in6_addr_set_mapped_ipv4(&src_ip6, flow_for_mgre.tunnel.ip_src);
            
            if (mgre_ip_exists(&src_ip6, false)) {
                VLOG_INFO("MGRE: Modifying tunnel source (IPv4) "IP_FMT" -> 255.255.255.255 for MGRE tunnel",
                          IP_ARGS(flow_for_mgre.tunnel.ip_src));
                flow_for_mgre.tunnel.ip_src = htonl(0xFFFFFFFF); /* 255.255.255.255 */
            }
        }
        /* Check for IPv6 tunnel */
        else if (ipv6_addr_is_set(&flow_for_mgre.tunnel.ipv6_src)) {
            if (mgre_ip_exists(&flow_for_mgre.tunnel.ipv6_src, true)) {
                char addr_str[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &flow_for_mgre.tunnel.ipv6_src, addr_str, sizeof addr_str);
                VLOG_INFO("MGRE: Modifying tunnel source (IPv6) %s -> ::1 for MGRE tunnel",
                          addr_str);
                /* Set to ::1 (loopback) for MGRE */
                memset(&flow_for_mgre.tunnel.ipv6_src, 0, sizeof flow_for_mgre.tunnel.ipv6_src);
                flow_for_mgre.tunnel.ipv6_src.s6_addr[15] = 1;
            }
        }
    }

    /* Display original flow information */
    if (ipv6_addr_is_set(&flow->tunnel.ipv6_src) || ipv6_addr_is_set(&flow->tunnel.ipv6_dst)) {
        char src_str[INET6_ADDRSTRLEN], dst_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &flow->tunnel.ipv6_src, src_str, sizeof src_str);
        inet_ntop(AF_INET6, &flow->tunnel.ipv6_dst, dst_str, sizeof dst_str);
        VLOG_INFO("tnl_port_receive original flow info (IPv6): "
                  "tunnel.ipv6_src=%s, tunnel.ipv6_dst=%s, tunnel.tun_id=0x%"PRIx64", "
                  "tunnel.tp_src=%u, tunnel.tp_dst=%u, tunnel.flags=0x%04x, "
                  "dl_type=0x%04x, in_port.odp_port=%u",
                  src_str, dst_str, ntohll(flow->tunnel.tun_id),
                  flow->tunnel.tp_src, flow->tunnel.tp_dst, flow->tunnel.flags,
                  ntohs(flow->dl_type), flow->in_port.odp_port);
    } else {
        VLOG_INFO("tnl_port_receive original flow info (IPv4): "
                  "tunnel.ip_src="IP_FMT", tunnel.ip_dst="IP_FMT", tunnel.tun_id=0x%"PRIx64", "
                  "tunnel.tp_src=%u, tunnel.tp_dst=%u, tunnel.flags=0x%04x, "
                  "nw_src="IP_FMT", nw_dst="IP_FMT", nw_proto=%u, "
                  "dl_type=0x%04x, in_port.odp_port=%u, packet_type=0x%08x",
                  IP_ARGS(flow->tunnel.ip_src), IP_ARGS(flow->tunnel.ip_dst), 
                  ntohll(flow->tunnel.tun_id),
                  flow->tunnel.tp_src, flow->tunnel.tp_dst, flow->tunnel.flags,
                  IP_ARGS(flow->nw_src), IP_ARGS(flow->nw_dst), 
                  flow->nw_proto, ntohs(flow->dl_type),
                  flow->in_port.odp_port, flow->packet_type);
    }
              
    /* Display modified flow information for comparison */
    if (flow_for_mgre.tunnel.ip_src != flow->tunnel.ip_src) {
        VLOG_INFO("tnl_port_receive MGRE modified flow (IPv4): "
                  "tunnel.ip_src="IP_FMT"→"IP_FMT" (modified for matching)",
                  IP_ARGS(flow->tunnel.ip_src), IP_ARGS(flow_for_mgre.tunnel.ip_src));
    } else if (!IN6_ARE_ADDR_EQUAL(&flow_for_mgre.tunnel.ipv6_src, &flow->tunnel.ipv6_src)) {
        char orig_str[INET6_ADDRSTRLEN], mod_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &flow->tunnel.ipv6_src, orig_str, sizeof orig_str);
        inet_ntop(AF_INET6, &flow_for_mgre.tunnel.ipv6_src, mod_str, sizeof mod_str);
        VLOG_INFO("tnl_port_receive MGRE modified flow (IPv6): "
                  "tunnel.ipv6_src=%s→%s (modified for matching)",
                  orig_str, mod_str);
    }

    /* Log flow_tnl_src/dst values before tnl_find */
    struct in6_addr tnl_src = flow_tnl_src(&flow_for_mgre.tunnel);
    struct in6_addr tnl_dst = flow_tnl_dst(&flow_for_mgre.tunnel);
    char tnl_src_str[INET6_ADDRSTRLEN], tnl_dst_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &tnl_src, tnl_src_str, sizeof tnl_src_str);
    inet_ntop(AF_INET6, &tnl_dst, tnl_dst_str, sizeof tnl_dst_str);
    VLOG_INFO("tnl_find matching criteria:");
    VLOG_INFO("  flow_tnl_src=%s (will match tunnel's remote_ip via ipv6_dst)", tnl_src_str);
    VLOG_INFO("  flow_tnl_dst=%s (will match tunnel's local_ip via ipv6_src)", tnl_dst_str);
    VLOG_INFO("  tun_id=0x%"PRIx64", in_port.odp_port=%u", 
              ntohll(flow_for_mgre.tunnel.tun_id), flow_for_mgre.in_port.odp_port);
    VLOG_INFO("  packet_type=0x%08x (namespace=%u)", 
              flow_for_mgre.packet_type, pt_ns(flow_for_mgre.packet_type));

    fat_rwlock_rdlock(&rwlock);
    tnl_port = tnl_find(&flow_for_mgre);

    ofport = tnl_port ? tnl_port->ofport : NULL;
    
    /* Log when tunnel port is found at line 324 */
    if (tnl_port) {
        VLOG_INFO("tnl_port_receive: Found tunnel port! device=%s, odp_port=%u, "
                  "in_key=0x%"PRIx64", pt_mode=%d, ofport=%p",
                  netdev_get_name(tnl_port->netdev), tnl_port->match.odp_port,
                  ntohll(tnl_port->match.in_key), tnl_port->match.pt_mode, ofport);
    }
    
    if (!tnl_port) {
        if (!VLOG_DROP_WARN(&rl)) {
            char *flow_str = flow_to_string(flow, NULL);
            VLOG_WARN("receive tunnel port not found (%s)", flow_str);
            free(flow_str);
        }
        goto out;
    }

    if (!VLOG_DROP_DBG(&dbg_rl)) {
        char *flow_str = flow_to_string(flow, NULL);
        char *tnl_str = tnl_port_to_string(tnl_port);
        VLOG_DBG("tunnel port %s receive from flow %s", tnl_str, flow_str);
        free(tnl_str);
        free(flow_str);
    }

out:
    fat_rwlock_unlock(&rwlock);
    return ofport;

    ////////////////////////////////////////
}

/* Should be called at the beginning of action translation to initialize
 * wildcards and perform any actions based on receiving on tunnel port.
 *
 * Returns false if the packet must be dropped. */
bool
tnl_process_ecn(struct flow *flow)
{
    if (!tnl_port_should_receive(flow)) {
        return true;
    }

    if (is_ip_any(flow) && IP_ECN_is_ce(flow->tunnel.ip_tos)) {
        if ((flow->nw_tos & IP_ECN_MASK) == IP_ECN_NOT_ECT) {
            VLOG_WARN_RL(&rl, "dropping tunnel packet marked ECN CE"
                         " but is not ECN capable");
            return false;
        }

        /* Set the ECN CE value in the tunneled packet. */
        flow->nw_tos |= IP_ECN_CE;
    }

    return true;
}

void
tnl_wc_init(struct flow *flow, struct flow_wildcards *wc)
{
    if (tnl_port_should_receive(flow)) {
        wc->masks.tunnel.tun_id = OVS_BE64_MAX;
        if (flow->tunnel.ip_dst) {
            wc->masks.tunnel.ip_src = OVS_BE32_MAX;
            wc->masks.tunnel.ip_dst = OVS_BE32_MAX;
        } else {
            wc->masks.tunnel.ipv6_src = in6addr_exact;
            wc->masks.tunnel.ipv6_dst = in6addr_exact;
        }
        /* FLOW_TNL_F_KEY is necessary to tell if the tun_id is present.  Other
         * flags are either controlled by OpenFlow or not used for matching. */
        wc->masks.tunnel.flags = FLOW_TNL_F_KEY;
        wc->masks.tunnel.ip_tos = UINT8_MAX;
        wc->masks.tunnel.ip_ttl = 0;
        /* The tp_src and tp_dst members in flow_tnl are set to be always
         * wildcarded, not to unwildcard them here. */
        wc->masks.tunnel.tp_src = 0;
        wc->masks.tunnel.tp_dst = 0;

        if (is_ip_any(flow)
            && IP_ECN_is_ce(flow->tunnel.ip_tos)) {
            wc->masks.nw_tos |= IP_ECN_MASK;
        }
    }
}

//////////////////////////////////////////////////////
/* Given that 'flow' should be output to the ofport corresponding to
 * 'tnl_port', updates 'flow''s tunnel headers and returns the actual datapath
 * port that the output should happen on.  May return ODPP_NONE if the output
 * shouldn't occur. */
// odp_port_t
// tnl_port_send(const struct ofport_dpif *ofport, struct flow *flow,
//               struct flow_wildcards *wc) OVS_EXCLUDED(rwlock)
// {
//     const struct netdev_tunnel_config *cfg;
//     struct tnl_port *tnl_port;
//     char *pre_flow_str = NULL;
//     odp_port_t out_port;

//     fat_rwlock_rdlock(&rwlock);
//     tnl_port = tnl_find_ofport(ofport);
//     out_port = tnl_port ? tnl_port->match.odp_port : ODPP_NONE;
//     if (!tnl_port) {
//         goto out;
//     }

//     cfg = netdev_get_tunnel_config(tnl_port->netdev);
//     ovs_assert(cfg);

//     if (!VLOG_DROP_DBG(&dbg_rl)) {
//         pre_flow_str = flow_to_string(flow, NULL);
//     }

//     if (!cfg->ip_src_flow) {
//         flow->tunnel.ip_src = in6_addr_get_mapped_ipv4(&tnl_port->match.ipv6_src);
//         if (!flow->tunnel.ip_src) {
//             flow->tunnel.ipv6_src = tnl_port->match.ipv6_src;
//         } else {
//             flow->tunnel.ipv6_src = in6addr_any;
//         }
//     }
//     if (!cfg->ip_dst_flow) {
//         flow->tunnel.ip_dst = in6_addr_get_mapped_ipv4(&tnl_port->match.ipv6_dst);
//         if (!flow->tunnel.ip_dst) {
//             flow->tunnel.ipv6_dst = tnl_port->match.ipv6_dst;
//         } else {
//             flow->tunnel.ipv6_dst = in6addr_any;
//         }
//     }
//     flow->tunnel.tp_src = 0; /* Do not carry from a previous tunnel. */
//     flow->tunnel.tp_dst = cfg->dst_port;
//     if (!cfg->out_key_flow) {
//         flow->tunnel.tun_id = cfg->out_key;
//     }

//     if (cfg->ttl_inherit && is_ip_any(flow)) {
//         wc->masks.nw_ttl = 0xff;
//         flow->tunnel.ip_ttl = flow->nw_ttl;
//     } else {
//         flow->tunnel.ip_ttl = cfg->ttl;
//     }

//     if (cfg->tos_inherit && is_ip_any(flow)) {
//         wc->masks.nw_tos |= IP_DSCP_MASK;
//         flow->tunnel.ip_tos = flow->nw_tos & IP_DSCP_MASK;
//     } else {
//         flow->tunnel.ip_tos = cfg->tos;
//     }

//     /* ECN fields are always inherited. */
//     if (is_ip_any(flow)) {
//         wc->masks.nw_tos |= IP_ECN_MASK;

//         if (IP_ECN_is_ce(flow->nw_tos)) {
//             flow->tunnel.ip_tos |= IP_ECN_ECT_0;
//         } else {
//             flow->tunnel.ip_tos |= flow->nw_tos & IP_ECN_MASK;
//         }
//     }

//     flow->tunnel.flags &= ~(FLOW_TNL_F_MASK & ~FLOW_TNL_PUB_F_MASK);
//     flow->tunnel.flags |= (cfg->dont_fragment ? FLOW_TNL_F_DONT_FRAGMENT : 0)
//         | (cfg->out_key_present ? FLOW_TNL_F_KEY : 0);

//     if (cfg->csum == NETDEV_TNL_CSUM_ENABLED ||
//         (cfg->csum == NETDEV_TNL_CSUM_DEFAULT && !flow->tunnel.ip_dst)) {
//         flow->tunnel.flags |= FLOW_TNL_F_CSUM;
//     }

//     if (cfg->set_egress_pkt_mark) {
//         flow->pkt_mark = cfg->egress_pkt_mark;
//         wc->masks.pkt_mark = UINT32_MAX;
//     }

//     if (!cfg->erspan_ver_flow) {
//         flow->tunnel.erspan_ver = cfg->erspan_ver;
//     }

//     if (!cfg->erspan_idx_flow) {
//         flow->tunnel.erspan_idx = cfg->erspan_idx;
//     }

//     if (!cfg->erspan_dir_flow) {
//         flow->tunnel.erspan_dir = cfg->erspan_dir;
//     }

//     if (!cfg->erspan_hwid_flow) {
//         flow->tunnel.erspan_hwid = cfg->erspan_hwid;
//     }

//     if (pre_flow_str) {
//         char *post_flow_str = flow_to_string(flow, NULL);
//         char *tnl_str = tnl_port_to_string(tnl_port);
//         VLOG_DBG("flow sent\n"
//                  "%s"
//                  " pre: %s\n"
//                  "post: %s",
//                  tnl_str, pre_flow_str, post_flow_str);
//         free(tnl_str);
//         free(pre_flow_str);
//         free(post_flow_str);
//     }

// out:
//     fat_rwlock_unlock(&rwlock);
//     return out_port;
// }

odp_port_t
tnl_port_send(const struct ofport_dpif *ofport, struct flow *flow,
              struct flow_wildcards *wc) OVS_EXCLUDED(rwlock)
{
    const struct netdev_tunnel_config *cfg;
    struct tnl_port *tnl_port;
    char *pre_flow_str = NULL;
    odp_port_t out_port;

    fat_rwlock_rdlock(&rwlock);
    tnl_port = tnl_find_ofport(ofport);
    out_port = tnl_port ? tnl_port->match.odp_port : ODPP_NONE;
    if (!tnl_port) {
        goto out;
    }

    cfg = netdev_get_tunnel_config(tnl_port->netdev);
    ovs_assert(cfg);

    if (!VLOG_DROP_DBG(&dbg_rl)) {
        pre_flow_str = flow_to_string(flow, NULL);
    }

    if (!cfg->ip_src_flow) {
        flow->tunnel.ip_src = in6_addr_get_mapped_ipv4(&tnl_port->match.ipv6_src);
        if (!flow->tunnel.ip_src) {
            flow->tunnel.ipv6_src = tnl_port->match.ipv6_src;
        } else {
            flow->tunnel.ipv6_src = in6addr_any;
        }
    }
            
    /* Log flow field information - showing native values */
    VLOG_INFO("Flow fields info (native): tunnel.ip_src=0x%08x, tunnel.ip_dst=0x%08x"
                ", tunnel.tun_id=0x%016"PRIx64", tunnel.tp_src=%u"
                ", tunnel.tp_dst=%u, tunnel.ip_tos=%u"
                ", tunnel.ip_ttl=%u, in_port.odp_port=%u",
                flow->tunnel.ip_src, flow->tunnel.ip_dst,
                flow->tunnel.tun_id, flow->tunnel.tp_src,
                flow->tunnel.tp_dst, flow->tunnel.ip_tos,
                flow->tunnel.ip_ttl, flow->in_port.odp_port);
    
    /* Cast to flow_mof for accessing MOF format fields */
    struct flow_mof *flow_mof = (struct flow_mof *)flow;
    uint16_t mof_dl_type_val = (flow_mof->dl_type[0] << 8) | flow_mof->dl_type[1];
    
    /* Log Layer 2/3 protocol information using MOF format */
    uint32_t mof_nw_src = (flow_mof->ip_saddr_e[0] << 24) | (flow_mof->ip_saddr_e[1] << 16) |
                         (flow_mof->ip_saddr_e[2] << 8) | flow_mof->ip_saddr_e[3];
    uint32_t mof_nw_dst = (flow_mof->ip_daddr_e[0] << 24) | (flow_mof->ip_daddr_e[1] << 16) |
                         (flow_mof->ip_daddr_e[2] << 8) | flow_mof->ip_daddr_e[3];
    uint16_t mof_vlan1_tci = (flow_mof->vlan1_tpid[0] << 8) | flow_mof->vlan1_tpid[1];
    
    VLOG_INFO("L2/L3 protocol info (MOF format): "
              "mac_src=%02x:%02x:%02x:%02x:%02x:%02x, mac_dst=%02x:%02x:%02x:%02x:%02x:%02x, "
              "dl_type=0x%04x, vlan1_tpid=0x%04x, "
              "ip_saddr="IP_FMT", ip_daddr="IP_FMT", "
              "ipv4_e_type=%u, tos_e=0x%02x, ttl_e=%u",
              flow_mof->mac_src[0], flow_mof->mac_src[1], flow_mof->mac_src[2], 
              flow_mof->mac_src[3], flow_mof->mac_src[4], flow_mof->mac_src[5],
              flow_mof->mac_dst[0], flow_mof->mac_dst[1], flow_mof->mac_dst[2],
              flow_mof->mac_dst[3], flow_mof->mac_dst[4], flow_mof->mac_dst[5],
              mof_dl_type_val, mof_vlan1_tci,
              IP_ARGS(htonl(mof_nw_src)), IP_ARGS(htonl(mof_nw_dst)),
              flow_mof->ipv4_e_type[0], flow_mof->tos_e[0], flow_mof->ttl_e[0]);
    
    if (!cfg->ip_dst_flow) {
        /* MGRE dynamic tunnel destination resolution
         * Check if tnl_port is configured with special MGRE addresses:
         * - 255.255.255.255 (0xFFFFFFFF) for IPv4 MGRE
         * - ::1 (in6addr_loopback) for IPv6 MGRE
         * If so, lookup the remote tunnel IP based on inner packet destination */
        bool needs_lookup = false;
        bool is_ipv6_inner = false;
        struct in6_addr inner_dst_ip6;
        
        /* Check if this is IPv4 MGRE (dst = 255.255.255.255) */
        uint32_t tnl_dst_ipv4 = in6_addr_get_mapped_ipv4(&tnl_port->match.ipv6_dst);
        if (tnl_dst_ipv4 == htonl(0xFFFFFFFF)) {
            /* IPv4 MGRE: lookup based on inner packet destination */
            needs_lookup = true;
            /* Get inner packet destination IP (IPv4 or IPv6) using MOF format */
            uint16_t mof_dl_type_val = (flow_mof->dl_type[0] << 8) | flow_mof->dl_type[1];
            if (mof_dl_type_val == ETH_TYPE_IPV6) {
                /* Inner packet is IPv6 */
                is_ipv6_inner = true;
                /* Convert flow_mof IPv6 dst to standard format */
                memcpy(&inner_dst_ip6, flow_mof->ipv6_dst_e, 16);
            } else if (mof_dl_type_val == ETH_TYPE_IP) {
                /* Inner packet is IPv4, convert to IPv4-mapped IPv6 */
                is_ipv6_inner = false;
                uint32_t mof_ip_dst = (flow_mof->ip_daddr_e[0] << 24) | (flow_mof->ip_daddr_e[1] << 16) |
                                     (flow_mof->ip_daddr_e[2] << 8) | flow_mof->ip_daddr_e[3];
                in6_addr_set_mapped_ipv4(&inner_dst_ip6, htonl(mof_ip_dst));
            } else {
                needs_lookup = false; /* Non-IP inner packet */
            }
        } 
        /* Check if this is IPv6 MGRE (dst = ::1) */
        else if (ipv6_addr_equals(&tnl_port->match.ipv6_dst, &in6addr_loopback)) {
            /* IPv6 MGRE: lookup based on inner packet destination using MOF format */
            needs_lookup = true;
            if (mof_dl_type_val == ETH_TYPE_IPV6) {
                is_ipv6_inner = true;
                /* Convert flow_mof IPv6 dst to standard format */
                memcpy(&inner_dst_ip6, flow_mof->ipv6_dst_e, 16);
            } else if (mof_dl_type_val == ETH_TYPE_IP) {
                is_ipv6_inner = false;
                uint32_t mof_ip_dst = (flow_mof->ip_daddr_e[0] << 24) | (flow_mof->ip_daddr_e[1] << 16) |
                                     (flow_mof->ip_daddr_e[2] << 8) | flow_mof->ip_daddr_e[3];
                in6_addr_set_mapped_ipv4(&inner_dst_ip6, htonl(mof_ip_dst));
            } else {
                needs_lookup = false; /* Non-IP inner packet */
            }
        }
        
        if (needs_lookup) {
            /* Lookup in subnet MGRE neighbor table */
            struct in6_addr remote_ip6;
            bool remote_is_ipv6;
            if (subnet_mgre_neighbor_lookup(&inner_dst_ip6, is_ipv6_inner, &remote_ip6, &remote_is_ipv6)) {
                /* Found matching remote IP, set tunnel destination */
                char inner_str[INET6_ADDRSTRLEN], remote_str[INET6_ADDRSTRLEN];
                inet_ntop(is_ipv6_inner ? AF_INET6 : AF_INET,
                          is_ipv6_inner ? (void*)&inner_dst_ip6 : (void*)&inner_dst_ip6.s6_addr[12],
                          inner_str, sizeof inner_str);
                
                if (remote_is_ipv6) {
                    /* Remote tunnel endpoint is IPv6 */
                    flow->tunnel.ipv6_dst = remote_ip6;
                    flow->tunnel.ip_dst = 0;
                    inet_ntop(AF_INET6, &remote_ip6, remote_str, sizeof remote_str);
                } else {
                    /* Remote tunnel endpoint is IPv4 */
                    flow->tunnel.ip_dst = in6_addr_get_mapped_ipv4(&remote_ip6);
                    flow->tunnel.ipv6_dst = in6addr_any;
                    inet_ntop(AF_INET, &flow->tunnel.ip_dst, remote_str, sizeof remote_str);
                }
                
                VLOG_INFO("MGRE tunnel dst resolved: inner_dst=%s -> remote_ip=%s (is_ipv6=%d)",
                          inner_str, remote_str, remote_is_ipv6);
            } else {
                /* No neighbor found, keep MGRE broadcast address to drop/flood packet */
                char inner_str[INET6_ADDRSTRLEN];
                inet_ntop(is_ipv6_inner ? AF_INET6 : AF_INET,
                          is_ipv6_inner ? (void*)&inner_dst_ip6 : (void*)&inner_dst_ip6.s6_addr[12],
                          inner_str, sizeof inner_str);
                VLOG_WARN("No MGRE neighbor found for inner_dst=%s, using broadcast address", inner_str);
                
                /* Keep the original MGRE special address based on tunnel type */
                if (tnl_dst_ipv4 == htonl(0xFFFFFFFF)) {
                    /* IPv4 MGRE tunnel: keep 255.255.255.255 */
                    flow->tunnel.ip_dst = htonl(0xFFFFFFFF);
                    flow->tunnel.ipv6_dst = in6addr_any;
                } else {
                    /* IPv6 MGRE tunnel: keep ::1 */
                    flow->tunnel.ip_dst = 0;
                    flow->tunnel.ipv6_dst = in6addr_loopback;
                }
            }
        } else {
            /* Not MGRE or non-IP inner packet, use configured tunnel destination */
            flow->tunnel.ip_dst = in6_addr_get_mapped_ipv4(&tnl_port->match.ipv6_dst);
            if (!flow->tunnel.ip_dst) {
                flow->tunnel.ipv6_dst = tnl_port->match.ipv6_dst;
            } else {
                flow->tunnel.ipv6_dst = in6addr_any;
            }
        }
    }
    flow->tunnel.tp_dst = cfg->dst_port;
    if (!cfg->out_key_flow) {
        flow->tunnel.tun_id = cfg->out_key;
    }

    if (cfg->ttl_inherit && is_ip_any(flow)) {
        wc->masks.nw_ttl = 0xff;
        flow->tunnel.ip_ttl = flow->nw_ttl;
    } else {
        flow->tunnel.ip_ttl = cfg->ttl;
    }

    if (cfg->tos_inherit && is_ip_any(flow)) {
        wc->masks.nw_tos |= IP_DSCP_MASK;
        flow->tunnel.ip_tos = flow->nw_tos & IP_DSCP_MASK;
    } else {
        flow->tunnel.ip_tos = cfg->tos;
    }

    /* ECN fields are always inherited. */
    if (is_ip_any(flow)) {
        wc->masks.nw_tos |= IP_ECN_MASK;

        if (IP_ECN_is_ce(flow->nw_tos)) {
            flow->tunnel.ip_tos |= IP_ECN_ECT_0;
        } else {
            flow->tunnel.ip_tos |= flow->nw_tos & IP_ECN_MASK;
        }
    }

    flow->tunnel.flags &= ~(FLOW_TNL_F_MASK & ~FLOW_TNL_PUB_F_MASK);
    flow->tunnel.flags |= (cfg->dont_fragment ? FLOW_TNL_F_DONT_FRAGMENT : 0)
        | (cfg->csum ? FLOW_TNL_F_CSUM : 0)
        | (cfg->out_key_present ? FLOW_TNL_F_KEY : 0);

    if (cfg->set_egress_pkt_mark) {
        flow->pkt_mark = cfg->egress_pkt_mark;
        wc->masks.pkt_mark = UINT32_MAX;
    }

    if (!cfg->erspan_ver_flow) {
        flow->tunnel.erspan_ver = cfg->erspan_ver;
    }

    if (!cfg->erspan_idx_flow) {
        flow->tunnel.erspan_idx = cfg->erspan_idx;
    }

    if (!cfg->erspan_dir_flow) {
        flow->tunnel.erspan_dir = cfg->erspan_dir;
    }

    if (!cfg->erspan_hwid_flow) {
        flow->tunnel.erspan_hwid = cfg->erspan_hwid;
    }

    if (pre_flow_str) {
        char *post_flow_str = flow_to_string(flow, NULL);
        char *tnl_str = tnl_port_to_string(tnl_port);
        VLOG_DBG("flow sent\n"
                 "%s"
                 " pre: %s\n"
                 "post: %s",
                 tnl_str, pre_flow_str, post_flow_str);
        free(tnl_str);
        free(pre_flow_str);
        free(post_flow_str);
    }

out:
    fat_rwlock_unlock(&rwlock);
    return out_port;
}

/////////////////////////////////////////////////////

static uint32_t
tnl_hash(struct tnl_match *match)
{
    BUILD_ASSERT_DECL(sizeof *match % sizeof(uint32_t) == 0);
    return hash_words((uint32_t *) match, sizeof *match / sizeof(uint32_t), 0);
}

static struct tnl_port *
tnl_find_ofport(const struct ofport_dpif *ofport) OVS_REQ_RDLOCK(rwlock)
{
    struct tnl_port *tnl_port;

    HMAP_FOR_EACH_IN_BUCKET (tnl_port, ofport_node, hash_pointer(ofport, 0),
                             ofport_map) {
        if (tnl_port->ofport == ofport) {
            return tnl_port;
        }
    }
    return NULL;
}

static struct tnl_port *
tnl_find_exact(struct tnl_match *match, struct hmap *map)
    OVS_REQ_RDLOCK(rwlock)
{
    if (map) {
        struct tnl_port *tnl_port;

        HMAP_FOR_EACH_WITH_HASH (tnl_port, match_node, tnl_hash(match), map) {
            if (!memcmp(match, &tnl_port->match, sizeof *match)) {
                return tnl_port;
            }
        }
    }
    return NULL;
}

/* Returns the tnl_port that is the best match for the tunnel data in 'flow',
 * or NULL if no tnl_port matches 'flow'. */
static struct tnl_port *
tnl_find(const struct flow *flow) OVS_REQ_RDLOCK(rwlock)
{
    enum ip_src_type ip_src;
    int in_key_flow;
    int ip_dst_flow;
    int i;

    i = 0;
    for (in_key_flow = 0; in_key_flow < 2; in_key_flow++) {
        for (ip_dst_flow = 0; ip_dst_flow < 2; ip_dst_flow++) {
            for (ip_src = 0; ip_src < 3; ip_src++) {
                struct hmap *map = tnl_match_maps[i];

                if (map) {
                    struct tnl_port *tnl_port;
                    struct tnl_match match;

                    memset(&match, 0, sizeof match);

                    /* The apparent mix-up of 'ip_dst' and 'ip_src' below is
                     * correct, because "struct tnl_match" is expressed in
                     * terms of packets being sent out, but we are using it
                     * here as a description of how to treat received
                     * packets. */
                    match.in_key = in_key_flow ? 0 : flow->tunnel.tun_id;
                    if (ip_src == IP_SRC_CFG) {
                        match.ipv6_src = flow_tnl_dst(&flow->tunnel);
                    }
                    if (!ip_dst_flow) {
                        match.ipv6_dst = flow_tnl_src(&flow->tunnel);
                    }
                    match.odp_port = flow->in_port.odp_port;
                    match.in_key_flow = in_key_flow;
                    match.ip_dst_flow = ip_dst_flow;
                    match.ip_src_flow = ip_src == IP_SRC_FLOW;

                    /* Look for a legacy L2 or L3 tunnel port first. */
                    if (pt_ns(flow->packet_type) == OFPHTN_ETHERTYPE) {
                        match.pt_mode = NETDEV_PT_LEGACY_L3;
                    } else {
                        match.pt_mode = NETDEV_PT_LEGACY_L2;
                    }
                    tnl_port = tnl_find_exact(&match, map);
                    if (tnl_port) {
                        return tnl_port;
                    }

                    /* Then check for a packet type aware port. */
                    match.pt_mode = NETDEV_PT_AWARE;
                    tnl_port = tnl_find_exact(&match, map);
                    if (tnl_port) {
                        return tnl_port;
                    }
                }

                i++;
            }
        }
    }

    return NULL;
}

/* Returns a pointer to the 'tnl_match_maps' element corresponding to 'm''s
 * matching criteria. */
static struct hmap **
tnl_match_map(const struct tnl_match *m)
{
    enum ip_src_type ip_src;

    ip_src = (m->ip_src_flow ? IP_SRC_FLOW
              : ipv6_addr_is_set(&m->ipv6_src) ? IP_SRC_CFG
              : IP_SRC_ANY);

    return &tnl_match_maps[6 * m->in_key_flow + 3 * m->ip_dst_flow + ip_src];
}

static void
tnl_match_fmt(const struct tnl_match *match, struct ds *ds)
    OVS_REQ_RDLOCK(rwlock)
{
    if (!match->ip_dst_flow) {
        ipv6_format_mapped(&match->ipv6_src, ds);
        ds_put_cstr(ds, "->");
        ipv6_format_mapped(&match->ipv6_dst, ds);
    } else if (!match->ip_src_flow) {
        ipv6_format_mapped(&match->ipv6_src, ds);
        ds_put_cstr(ds, "->flow");
    } else {
        ds_put_cstr(ds, "flow->flow");
    }

    if (match->in_key_flow) {
        ds_put_cstr(ds, ", key=flow");
    } else {
        ds_put_format(ds, ", key=%#"PRIx64, ntohll(match->in_key));
    }

    const char *pt_mode
        = (match->pt_mode == NETDEV_PT_LEGACY_L2 ? "legacy_l2"
           : match->pt_mode == NETDEV_PT_LEGACY_L3 ? "legacy_l3"
           : "ptap");
    ds_put_format(ds, ", %s, dp port=%"PRIu32, pt_mode, match->odp_port);
}

static void
tnl_port_mod_log(const struct tnl_port *tnl_port, const char *action)
    OVS_REQ_RDLOCK(rwlock)
{
    if (VLOG_IS_DBG_ENABLED()) {
        struct ds ds = DS_EMPTY_INITIALIZER;

        tnl_match_fmt(&tnl_port->match, &ds);
        VLOG_INFO("%s tunnel port %s (%s)", action,
                  tnl_port_get_name(tnl_port), ds_cstr(&ds));
        ds_destroy(&ds);
    }
}

static void OVS_REQ_RDLOCK(rwlock)
tnl_port_format(const struct tnl_port *tnl_port, struct ds *ds)
{
    const struct netdev_tunnel_config *cfg =
        netdev_get_tunnel_config(tnl_port->netdev);

    ds_put_format(ds, "port %"PRIu32": %s (%s: ", tnl_port->match.odp_port,
                  tnl_port_get_name(tnl_port),
                  netdev_get_type(tnl_port->netdev));
    tnl_match_fmt(&tnl_port->match, ds);

    if (cfg->out_key != cfg->in_key ||
        cfg->out_key_present != cfg->in_key_present ||
        cfg->out_key_flow != cfg->in_key_flow) {
        ds_put_cstr(ds, ", out_key=");
        if (!cfg->out_key_present) {
            ds_put_cstr(ds, "none");
        } else if (cfg->out_key_flow) {
            ds_put_cstr(ds, "flow");
        } else {
            ds_put_format(ds, "%#"PRIx64, ntohll(cfg->out_key));
        }
    }

    if (cfg->ttl_inherit) {
        ds_put_cstr(ds, ", ttl=inherit");
    } else {
        ds_put_format(ds, ", ttl=%"PRIu8, cfg->ttl);
    }

    if (cfg->tos_inherit) {
        ds_put_cstr(ds, ", tos=inherit");
    } else if (cfg->tos) {
        ds_put_format(ds, ", tos=%#"PRIx8, cfg->tos);
    }

    if (!cfg->dont_fragment) {
        ds_put_cstr(ds, ", df=false");
    }

    if (cfg->csum == NETDEV_TNL_CSUM_ENABLED) {
        ds_put_cstr(ds, ", csum=true");
    } else if (cfg->csum == NETDEV_TNL_CSUM_DISABLED) {
        ds_put_cstr(ds, ", csum=false");
    }

    ds_put_cstr(ds, ")\n");
}

static char * OVS_REQ_RDLOCK(rwlock)
tnl_port_to_string(const struct tnl_port *tnl_port)
{
    struct ds ds = DS_EMPTY_INITIALIZER;
    tnl_port_format(tnl_port, &ds);
    return ds_steal_cstr(&ds);
}

static const char *
tnl_port_get_name(const struct tnl_port *tnl_port) OVS_REQ_RDLOCK(rwlock)
{
    return netdev_get_name(tnl_port->netdev);
}

const char *
tnl_port_get_type(const struct ofport_dpif *ofport) OVS_REQ_RDLOCK(rwlock)
{
    struct tnl_port *tnl_port;

    tnl_port = tnl_find_ofport(ofport);
    return !tnl_port ? NULL :
                       netdev_get_type(tnl_port->netdev);
}

int
tnl_port_build_header(const struct ofport_dpif *ofport,
                      struct ovs_action_push_tnl *data,
                      const struct netdev_tnl_build_header_params *params)
{
    struct tnl_port *tnl_port;
    int res;

    fat_rwlock_rdlock(&rwlock);
    tnl_port = tnl_find_ofport(ofport);
    ovs_assert(tnl_port);
    res = netdev_build_header(tnl_port->netdev, data, params);
    fat_rwlock_unlock(&rwlock);

    return res;
}

static void
tnl_unixctl_list(struct unixctl_conn *conn,
                 int argc OVS_UNUSED, const char *argv[] OVS_UNUSED,
                 void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;

    fat_rwlock_rdlock(&rwlock);
    for (int i = 0; i < N_MATCH_TYPES; i++) {
        struct hmap *map = tnl_match_maps[i];
        if (map) {
            struct tnl_port *tnl_port;
            HMAP_FOR_EACH (tnl_port, match_node, map) {
                tnl_port_format(tnl_port, &reply);
            }
        }
    }
    fat_rwlock_unlock(&rwlock);

    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

///////////////////////////////////////////////////////
/* Parse IP address string to host byte order */
static bool
parse_ip_address(const char *ip_str, uint32_t *ip)
{
    struct in_addr addr;
    if (inet_aton(ip_str, &addr)) {
        *ip = ntohl(addr.s_addr);
        return true;
    }
    return false;
}

/* Parse subnet string (e.g., "192.168.2.0/24" or "192.168.2.2") to IP and mask */
static bool
parse_subnet(const char *subnet_str, uint32_t *ip, uint32_t *mask)
{
    char *str_copy = xstrdup(subnet_str);
    char *slash = strchr(str_copy, '/');
    bool result = false;
    
    if (slash) {
        /* Has CIDR notation */
        *slash = '\0';
        char *prefix_str = slash + 1;
        int prefix_len;
        
        if (parse_ip_address(str_copy, ip) && 
            str_to_int(prefix_str, 10, &prefix_len) &&
            prefix_len >= 0 && prefix_len <= 32) {
            /* Calculate mask from prefix length */
            if (prefix_len == 0) {
                *mask = 0;
            } else {
                *mask = 0xFFFFFFFF << (32 - prefix_len);
            }
            *ip = *ip & *mask; /* Normalize to network address */
            result = true;
        }
    } else {
        /* No CIDR notation, treat as host address (/32) */
        if (parse_ip_address(str_copy, ip)) {
            *mask = 0xFFFFFFFF;
            result = true;
        }
    }
    
    free(str_copy);
    return result;
}

/* Command handler for adding MGRE IPs (supports both IPv4 and IPv6) */
static void
mgre_ip_add_cmd(struct unixctl_conn *conn, int argc, const char *argv[],
                void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    int added = 0, failed = 0;
    
    for (int i = 1; i < argc; i++) {
        struct in6_addr ip6;
        bool is_ipv6 = false;
        
        /* Try to parse as IPv6 first */
        if (inet_pton(AF_INET6, argv[i], &ip6) == 1) {
            is_ipv6 = true;
        } else {
            /* Try to parse as IPv4 */
            uint32_t ip4;
            if (parse_ip_address(argv[i], &ip4)) {
                /* Convert IPv4 to IPv4-mapped IPv6 address */
                in6_addr_set_mapped_ipv4(&ip6, htonl(ip4));
                is_ipv6 = false;
            } else {
                ds_put_format(&reply, "Invalid IP address: %s\n", argv[i]);
                failed++;
                continue;
            }
        }
        
        if (!mgre_ip_exists(&ip6, is_ipv6)) {
            mgre_ip_add(&ip6, is_ipv6);
            added++;
        } else {
            ds_put_format(&reply, "IP %s already exists\n", argv[i]);
        }
    }
    
    if (added > 0) {
        ds_put_format(&reply, "Successfully added %d MGRE IP(s)\n", added);
    }
    if (failed > 0) {
        ds_put_format(&reply, "Failed to add %d IP(s)\n", failed);
    }
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for deleting MGRE IPs (supports both IPv4 and IPv6) */
static void
mgre_ip_del_cmd(struct unixctl_conn *conn, int argc, const char *argv[],
                void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    int removed = 0, failed = 0;
    
    for (int i = 1; i < argc; i++) {
        struct in6_addr ip6;
        bool is_ipv6 = false;
        
        /* Try to parse as IPv6 first */
        if (inet_pton(AF_INET6, argv[i], &ip6) == 1) {
            is_ipv6 = true;
        } else {
            /* Try to parse as IPv4 */
            uint32_t ip4;
            if (parse_ip_address(argv[i], &ip4)) {
                /* Convert IPv4 to IPv4-mapped IPv6 address */
                in6_addr_set_mapped_ipv4(&ip6, htonl(ip4));
                is_ipv6 = false;
            } else {
                ds_put_format(&reply, "Invalid IP address: %s\n", argv[i]);
                failed++;
                continue;
            }
        }
        
        if (mgre_ip_exists(&ip6, is_ipv6)) {
            mgre_ip_remove(&ip6, is_ipv6);
            removed++;
        } else {
            ds_put_format(&reply, "IP %s not found\n", argv[i]);
        }
    }
    
    if (removed > 0) {
        ds_put_format(&reply, "Successfully removed %d MGRE IP(s)\n", removed);
    }
    if (failed > 0) {
        ds_put_format(&reply, "Failed to remove %d IP(s)\n", failed);
    }
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for listing MGRE IPs */
static void
mgre_ip_list_cmd(struct unixctl_conn *conn, int argc OVS_UNUSED,
                 const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    
    mgre_ip_list_all(&reply);
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for clearing all MGRE IPs */
static void
mgre_ip_clear_cmd(struct unixctl_conn *conn, int argc OVS_UNUSED,
                  const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    struct mgre_ip_entry *entry;
    int count = 0;
    
    /* Count current entries before clearing */
    ovs_mutex_lock(&mgre_ip_mutex);
    HMAP_FOR_EACH(entry, node, &mgre_ip_table) {
        count++;
    }
    ovs_mutex_unlock(&mgre_ip_mutex);
    
    if (count == 0) {
        ds_put_cstr(&reply, "MGRE IP table is already empty");
    } else {
        mgre_ip_clear_all();
        ds_put_format(&reply, "Successfully cleared %d MGRE IP(s)", count);
    }
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for adding subnet MGRE neighbor entries (supports IPv4/IPv6 mixed scenarios) */
static void
subnet_mgre_neighbor_add_cmd(struct unixctl_conn *conn, int argc, 
                              const char *argv[], void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    
    if (argc != 3) {
        ds_put_cstr(&reply, "Usage: mgre/neighbor-add <dest_subnet> <remote_ip>\n");
        ds_put_cstr(&reply, "Supports both IPv4 and IPv6, including mixed scenarios.\n");
        ds_put_cstr(&reply, "Example (IPv4): mgre/neighbor-add 192.168.2.0/24 10.0.0.2\n");
        ds_put_cstr(&reply, "Example (IPv6): mgre/neighbor-add 2001:db8:1::/64 2001:db8::2\n");
        ds_put_cstr(&reply, "Example (Mixed): mgre/neighbor-add 192.168.2.0/24 2001:db8::2\n");
        unixctl_command_reply_error(conn, ds_cstr(&reply));
        ds_destroy(&reply);
        return;
    }
    
    struct in6_addr dest_ip, dest_mask, remote_ip;
    uint8_t prefix_len;
    bool dest_is_ipv6 = false, remote_is_ipv6 = false;
    
    /* Try to parse destination as IPv6 subnet */
    if (parse_ipv6_subnet(argv[1], &dest_ip, &dest_mask, &prefix_len)) {
        dest_is_ipv6 = true;
    } else {
        /* Try to parse as IPv4 subnet */
        uint32_t dest_ip4, dest_mask4;
        if (!parse_subnet(argv[1], &dest_ip4, &dest_mask4)) {
            ds_put_format(&reply, "Invalid destination subnet: %s\n", argv[1]);
            unixctl_command_reply_error(conn, ds_cstr(&reply));
            ds_destroy(&reply);
            return;
        }
        /* Convert IPv4 to IPv4-mapped IPv6 */
        in6_addr_set_mapped_ipv4(&dest_ip, htonl(dest_ip4));
        in6_addr_set_mapped_ipv4(&dest_mask, htonl(dest_mask4));
        /* For IPv4-mapped IPv6: prefix_len = 96 + IPv4_prefix_len */
        prefix_len = 96 + count_1bits(dest_mask4);
        dest_is_ipv6 = false;
    }
    
    /* Try to parse remote IP as IPv6 */
    if (inet_pton(AF_INET6, argv[2], &remote_ip) == 1) {
        remote_is_ipv6 = true;
    } else {
        /* Try to parse as IPv4 */
        uint32_t remote_ip4;
        if (!parse_ip_address(argv[2], &remote_ip4)) {
            ds_put_format(&reply, "Invalid remote IP address: %s\n", argv[2]);
            unixctl_command_reply_error(conn, ds_cstr(&reply));
            ds_destroy(&reply);
            return;
        }
        /* Convert IPv4 to IPv4-mapped IPv6 */
        in6_addr_set_mapped_ipv4(&remote_ip, htonl(remote_ip4));
        remote_is_ipv6 = false;
    }
    
    subnet_mgre_neighbor_add(&dest_ip, &dest_mask, &remote_ip, prefix_len, dest_is_ipv6, remote_is_ipv6);
    ds_put_cstr(&reply, "Successfully added subnet MGRE neighbor entry");
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for deleting subnet MGRE neighbor entries (supports both IPv4 and IPv6) */
static void
subnet_mgre_neighbor_del_cmd(struct unixctl_conn *conn, int argc,
                              const char *argv[], void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    
    if (argc != 2) {
        ds_put_cstr(&reply, "Usage: mgre/neighbor-del <dest_subnet>\n");
        ds_put_cstr(&reply, "Supports both IPv4 and IPv6.\n");
        ds_put_cstr(&reply, "Must specify exact prefix length to match the entry:\n");
        ds_put_cstr(&reply, "  mgre/neighbor-del 192.168.2.0/24   (delete subnet)\n");
        ds_put_cstr(&reply, "  mgre/neighbor-del 192.168.2.5/32   (delete host)\n");
        ds_put_cstr(&reply, "Without prefix length, defaults to /32 (IPv4) or /128 (IPv6):\n");
        ds_put_cstr(&reply, "  mgre/neighbor-del 192.168.2.0      (treated as /32)\n");
        ds_put_cstr(&reply, "Example (IPv6): mgre/neighbor-del 2001:db8:1::/64\n");
        unixctl_command_reply_error(conn, ds_cstr(&reply));
        ds_destroy(&reply);
        return;
    }
    
    struct in6_addr dest_ip, dest_mask;
    uint8_t prefix_len;
    bool dest_is_ipv6 = false;
    
    /* Try to parse destination as IPv6 subnet */
    if (parse_ipv6_subnet(argv[1], &dest_ip, &dest_mask, &prefix_len)) {
        dest_is_ipv6 = true;
    } else {
        /* Try to parse as IPv4 subnet */
        uint32_t dest_ip4, dest_mask4;
        if (!parse_subnet(argv[1], &dest_ip4, &dest_mask4)) {
            ds_put_format(&reply, "Invalid destination subnet: %s\n", argv[1]);
            unixctl_command_reply_error(conn, ds_cstr(&reply));
            ds_destroy(&reply);
            return;
        }
        /* Convert IPv4 to IPv4-mapped IPv6 */
        in6_addr_set_mapped_ipv4(&dest_ip, htonl(dest_ip4));
        in6_addr_set_mapped_ipv4(&dest_mask, htonl(dest_mask4));
        /* For IPv4-mapped IPv6: prefix_len = 96 + IPv4_prefix_len */
        prefix_len = 96 + count_1bits(dest_mask4);
        dest_is_ipv6 = false;
    }
    
    bool removed = subnet_mgre_neighbor_remove(&dest_ip, &dest_mask, prefix_len, dest_is_ipv6);
    
    if (removed) {
        ds_put_cstr(&reply, "Successfully removed subnet MGRE neighbor entry");
        unixctl_command_reply(conn, ds_cstr(&reply));
    } else {
        /* Show helpful message about prefix length */
        char dest_str[INET6_ADDRSTRLEN];
        inet_ntop(dest_is_ipv6 ? AF_INET6 : AF_INET,
                  dest_is_ipv6 ? (void*)&dest_ip : (void*)&dest_ip.s6_addr[12],
                  dest_str, sizeof dest_str);
        
        if (dest_is_ipv6) {
            ds_put_format(&reply, "Entry not found: %s/%u\n", dest_str, prefix_len);
            ds_put_cstr(&reply, "Hint: Use 'mgre/neighbor-list' to see actual prefix lengths\n");
        } else {
            int ipv4_prefix = prefix_len - 96;
            ds_put_format(&reply, "Entry not found: %s/%u\n", dest_str, ipv4_prefix);
            ds_put_cstr(&reply, "Hint: Use 'mgre/neighbor-list' to see actual prefix lengths\n");
            if (ipv4_prefix == 32) {
                ds_put_cstr(&reply, "Note: You specified a host address (/32). If you want to delete a subnet,\n");
                ds_put_cstr(&reply, "      specify the prefix length, e.g., 192.168.2.0/24\n");
            }
        }
        unixctl_command_reply_error(conn, ds_cstr(&reply));
    }
    
    ds_destroy(&reply);
}

/* Command handler for listing subnet MGRE neighbor entries */
static void
subnet_mgre_neighbor_list_cmd(struct unixctl_conn *conn, int argc OVS_UNUSED,
                               const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    
    subnet_mgre_neighbor_list_all(&reply);
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* Command handler for clearing all subnet MGRE neighbor entries */
static void
subnet_mgre_neighbor_clear_cmd(struct unixctl_conn *conn, int argc OVS_UNUSED,
                                const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;
    int count = 0;
    
    /* Count current entries before clearing */
    ovs_mutex_lock(&subnet_mgre_neighbor_mutex);
    
    if (!subnet_mgre_tables_initialized) {
        ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
        ds_put_cstr(&reply, "Subnet MGRE neighbor table is already empty");
        unixctl_command_reply(conn, ds_cstr(&reply));
        ds_destroy(&reply);
        return;
    }
    
    /* Count all entries across all prefix tables */
    for (int i = 0; i < MAX_PREFIX_LEN; i++) {
        count += hmap_count(&subnet_mgre_neighbor_tables[i]);
    }
    ovs_mutex_unlock(&subnet_mgre_neighbor_mutex);
    
    if (count == 0) {
        ds_put_cstr(&reply, "Subnet MGRE neighbor table is already empty");
    } else {
        subnet_mgre_neighbor_clear_all();
        ds_put_format(&reply, "Successfully cleared %d subnet MGRE neighbor(s)", count);
    }
    
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* ============== Helper Functions for Parsing ============== */

/* Parse IPv6 subnet in format "2001:db8::/64" */
static bool
parse_ipv6_subnet(const char *str, struct in6_addr *ip, struct in6_addr *mask, uint8_t *prefix_len)
{
    char *subnet_copy = xstrdup(str);
    char *slash = strchr(subnet_copy, '/');
    
    if (!slash) {
        /* No prefix, assume /128 (single host) */
        if (inet_pton(AF_INET6, subnet_copy, ip) == 1) {
            *prefix_len = 128;
            prefix_len_to_mask_ipv6(128, mask);
            free(subnet_copy);
            return true;
        }
        free(subnet_copy);
        return false;
    }
    
    *slash = '\0';
    const char *prefix_str = slash + 1;
    
    /* Parse IP address */
    if (inet_pton(AF_INET6, subnet_copy, ip) != 1) {
        free(subnet_copy);
        return false;
    }
    
    /* Parse prefix length */
    char *endptr;
    long prefix = strtol(prefix_str, &endptr, 10);
    if (*endptr != '\0' || prefix < 0 || prefix > 128) {
        free(subnet_copy);
        return false;
    }
    
    *prefix_len = (uint8_t)prefix;
    prefix_len_to_mask_ipv6(*prefix_len, mask);
    
    /* Apply mask to get network address */
    apply_ipv6_mask(ip, mask, ip);
    
    free(subnet_copy);
    return true;
}
///////////////////////////////////////////////////////
