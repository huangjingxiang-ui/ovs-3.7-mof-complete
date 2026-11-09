#ifndef NEXTHOP_H
#define NEXTHOP_H 1

#include "openvswitch/types.h"
#include "packets.h"

#define NEXTHOP_HASH_SIZE 64

/* 下一跳/路由表项，以 port_name 为 key。 */
struct nexthop_entry {
    struct in6_addr port_ip;      /* 端口 IP */
    struct in6_addr nexthop_ip;   /* 下一跳 IP（全零表示直连/交换机模式） */
    char port_name[32];
    long long timestamp;
    struct nexthop_entry *next;
};

struct nexthop_table {
    struct nexthop_entry *buckets[NEXTHOP_HASH_SIZE];
};

void bridge_nexthop_init(void);
struct nexthop_entry *nexthop_lookup(const char *port_name);
void route_insert(struct nexthop_entry *nexthop_entry);

#endif /* nexthop.h */
