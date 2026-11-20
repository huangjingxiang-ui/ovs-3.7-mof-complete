#ifndef ARP_CACHE_H
#define ARP_CACHE_H 1

#include <stdint.h>
#include <net/if.h>

#include "openvswitch/types.h"
#include "packets.h"

#define ARP_HASH_SIZE 256

/* 目的条目：一个端口可对应多个目的 IP/MAC。 */
struct arp_dst_entry {
    struct in6_addr dst_ip;      /* 目的 IP */
    struct eth_addr dst_mac;     /* 目的 MAC */
    uint64_t timestamp;
    struct arp_dst_entry *next;
};

/* 端口条目：以 port_name 为 key。 */
struct arp_entry {
    struct in6_addr src_ip;         /* 本地 IP */
    struct eth_addr src_mac;        /* 本地 MAC */
    char port_name[IFNAMSIZ];       /* 端口名称 */
    odp_port_t odp_port;            /* OVS datapath 端口号 */
    uint64_t timestamp;             /* 更新时间/超时 */
    struct arp_dst_entry *dst_list; /* 指向多个目的条目的链表 */
    struct arp_entry *next;         /* 哈希冲突链 */
};

struct arp_table {
    struct arp_entry *buckets[ARP_HASH_SIZE];
};

void bridge_arp_cache_init(void);
void arp_cache_insert(const struct arp_entry *arp_entry);
struct arp_entry *arp_cache_lookup(const char *port_name);
bool arp_cache_lookup_copy(const char *port_name, struct arp_entry *out);
void arp_cache_remove(const char *port_name);
struct arp_dst_entry *arp_dst_entry_lookup(struct arp_entry *entry,
                                           const struct in6_addr *dst_ip);

#endif /* arp-cache.h */
