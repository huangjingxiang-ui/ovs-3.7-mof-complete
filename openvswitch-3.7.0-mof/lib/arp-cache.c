#include <config.h>
#include "arp-cache.h"

#include <inttypes.h>
#include <string.h>

#include "openvswitch/dynamic-string.h"
#include "openvswitch/thread.h"
#include "openvswitch/vlog.h"
#include "flow.h"
#include "hash.h"
#include "socket-util.h"
#include "timeval.h"
#include "unixctl.h"
#include "util.h"

VLOG_DEFINE_THIS_MODULE(arp_cache);

static struct arp_table arp_table;
static struct ovs_mutex arp_table_mutex = OVS_MUTEX_INITIALIZER;

static inline uint32_t
arp_hash(const char *port_name)
{
    return hash_string(port_name, 0) % ARP_HASH_SIZE;
}

/* 将字符串形式的 IPv4/IPv6 地址解析为 in6_addr（IPv4 映射为 IPv6）。 */
static int
lookup_any(const char *host_name, struct in6_addr *address)
{
    if (addr_is_ipv6(host_name)) {
        return lookup_ipv6(host_name, address);
    }

    struct in_addr ip;
    int r = lookup_ip(host_name, &ip);
    if (r == 0) {
        in6_addr_set_mapped_ipv4(address, ip.s_addr);
    }
    return r;
}

/* 在某个端口条目的目的链表中查找指定目的 IP。 */
struct arp_dst_entry *
arp_dst_entry_lookup(struct arp_entry *entry, const struct in6_addr *dst_ip)
{
    struct arp_dst_entry *dst_entry = entry->dst_list;

    while (dst_entry) {
        if (ipv6_addr_equals(&dst_entry->dst_ip, dst_ip)) {
            return dst_entry;
        }
        dst_entry = dst_entry->next;
    }
    return NULL;
}

/* 插入/更新 ARP 表项：按 port_name 定位端口条目，合并目的链表。 */
void
arp_cache_insert(const struct arp_entry *arp_entry)
{
    if (!arp_entry) {
        return;
    }

    ovs_mutex_lock(&arp_table_mutex);

    uint32_t idx = arp_hash(arp_entry->port_name);
    struct arp_entry *e = arp_table.buckets[idx];

    while (e) {
        if (strcmp(e->port_name, arp_entry->port_name) == 0) {
            /* 更新源信息。 */
            e->src_ip = arp_entry->src_ip;
            e->src_mac = arp_entry->src_mac;
            e->odp_port = arp_entry->odp_port;
            e->timestamp = time_now();

            /* 合并目的条目。 */
            for (const struct arp_dst_entry *d = arp_entry->dst_list; d;
                 d = d->next) {
                struct arp_dst_entry *dst_entry =
                    arp_dst_entry_lookup(e, &d->dst_ip);
                if (dst_entry) {
                    dst_entry->dst_mac = d->dst_mac;
                    dst_entry->timestamp = time_now();
                } else {
                    struct arp_dst_entry *new_dst = xzalloc(sizeof *new_dst);
                    new_dst->dst_ip = d->dst_ip;
                    new_dst->dst_mac = d->dst_mac;
                    new_dst->timestamp = time_now();
                    new_dst->next = e->dst_list;
                    e->dst_list = new_dst;
                }
            }

            ovs_mutex_unlock(&arp_table_mutex);
            return;
        }
        e = e->next;
    }

    /* 不存在，创建新的端口条目。 */
    e = xzalloc(sizeof *e);
    e->src_ip = arp_entry->src_ip;
    e->src_mac = arp_entry->src_mac;
    e->odp_port = arp_entry->odp_port;
    e->timestamp = time_now();
    ovs_strlcpy(e->port_name, arp_entry->port_name, sizeof e->port_name);

    struct arp_dst_entry **tail = &e->dst_list;
    for (const struct arp_dst_entry *d = arp_entry->dst_list; d; d = d->next) {
        struct arp_dst_entry *new_dst = xzalloc(sizeof *new_dst);
        new_dst->dst_ip = d->dst_ip;
        new_dst->dst_mac = d->dst_mac;
        new_dst->timestamp = time_now();
        *tail = new_dst;
        tail = &new_dst->next;
    }

    e->next = arp_table.buckets[idx];
    arp_table.buckets[idx] = e;

    ovs_mutex_unlock(&arp_table_mutex);
}

/* 按端口名查找（不加锁，调用方需自行保证并发安全）。 */
struct arp_entry *
arp_cache_lookup(const char *port_name)
{
    uint32_t idx = arp_hash(port_name);
    struct arp_entry *e = arp_table.buckets[idx];

    while (e) {
        if (strcmp(e->port_name, port_name) == 0) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

/* 带锁的深拷贝查找。返回 true 时 out->dst_list 为新分配内存，调用方需释放。 */
bool
arp_cache_lookup_copy(const char *port_name, struct arp_entry *out)
{
    if (!port_name || !out) {
        return false;
    }

    ovs_mutex_lock(&arp_table_mutex);
    uint32_t idx = arp_hash(port_name);
    struct arp_entry *e = arp_table.buckets[idx];

    while (e) {
        if (strcmp(e->port_name, port_name) == 0) {
            out->src_ip = e->src_ip;
            out->src_mac = e->src_mac;
            memcpy(out->port_name, e->port_name, IFNAMSIZ);
            out->odp_port = e->odp_port;
            out->timestamp = e->timestamp;
            out->next = NULL;

            out->dst_list = NULL;
            struct arp_dst_entry **tail = &out->dst_list;
            for (struct arp_dst_entry *d = e->dst_list; d; d = d->next) {
                struct arp_dst_entry *copy = xzalloc(sizeof *copy);
                *copy = *d;
                copy->next = NULL;
                *tail = copy;
                tail = &copy->next;
            }

            ovs_mutex_unlock(&arp_table_mutex);
            return true;
        }
        e = e->next;
    }

    ovs_mutex_unlock(&arp_table_mutex);
    return false;
}

void
arp_cache_remove(const char *port_name)
{
    ovs_mutex_lock(&arp_table_mutex);

    uint32_t idx = arp_hash(port_name);
    struct arp_entry *e = arp_table.buckets[idx];
    struct arp_entry *prev = NULL;

    while (e) {
        if (strcmp(e->port_name, port_name) == 0) {
            if (prev) {
                prev->next = e->next;
            } else {
                arp_table.buckets[idx] = e->next;
            }

            struct arp_dst_entry *dst_entry = e->dst_list;
            while (dst_entry) {
                struct arp_dst_entry *next = dst_entry->next;
                free(dst_entry);
                dst_entry = next;
            }

            free(e);
            ovs_mutex_unlock(&arp_table_mutex);
            return;
        }
        prev = e;
        e = e->next;
    }

    ovs_mutex_unlock(&arp_table_mutex);
}

/* arp/show 命令：打印整张 ARP 表。 */
static void
bridge_arp_cache_show(struct unixctl_conn *conn, int argc OVS_UNUSED,
                      const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;

    ds_put_format(&reply,
                  "%-15s %-17s %-15s %-17s %-12s %-8s\n",
                  "Local IP", "Local MAC", "Dest IP", "Dest MAC",
                  "Port Name", "ODP Port");
    ds_put_format(&reply,
                  "--------------- ----------------- --------------- "
                  "----------------- ------------ --------\n");

    ovs_mutex_lock(&arp_table_mutex);

    for (int i = 0; i < ARP_HASH_SIZE; i++) {
        for (struct arp_entry *entry = arp_table.buckets[i]; entry;
             entry = entry->next) {
            int start_len, need_ws;

            start_len = reply.length;
            if (!ipv6_is_zero(&entry->src_ip)) {
                ipv6_format_mapped(&entry->src_ip, &reply);
            } else {
                ds_put_cstr(&reply, "0.0.0.0");
            }
            need_ws = 16 - (reply.length - start_len);
            ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

            start_len = reply.length;
            if (!eth_addr_is_zero(entry->src_mac)) {
                ds_put_format(&reply, ETH_ADDR_FMT,
                              ETH_ADDR_ARGS(entry->src_mac));
            } else {
                ds_put_cstr(&reply, "00:00:00:00:00:00");
            }
            need_ws = 18 - (reply.length - start_len);
            ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

            if (entry->dst_list) {
                struct arp_dst_entry *dst_entry = entry->dst_list;

                start_len = reply.length;
                if (!ipv6_is_zero(&dst_entry->dst_ip)) {
                    ipv6_format_mapped(&dst_entry->dst_ip, &reply);
                } else {
                    ds_put_cstr(&reply, "0.0.0.0");
                }
                need_ws = 16 - (reply.length - start_len);
                ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

                start_len = reply.length;
                if (!eth_addr_is_zero(dst_entry->dst_mac)) {
                    ds_put_format(&reply, ETH_ADDR_FMT,
                                  ETH_ADDR_ARGS(dst_entry->dst_mac));
                } else {
                    ds_put_cstr(&reply, "00:00:00:00:00:00");
                }
                need_ws = 18 - (reply.length - start_len);
                ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

                ds_put_format(&reply, "%-13s%"PRIu32"\n",
                              entry->port_name[0] ? entry->port_name : "N/A",
                              odp_to_u32(entry->odp_port));

                for (dst_entry = dst_entry->next; dst_entry;
                     dst_entry = dst_entry->next) {
                    ds_put_format(&reply, "%-15s %-17s ", "", "");

                    start_len = reply.length;
                    if (!ipv6_is_zero(&dst_entry->dst_ip)) {
                        ipv6_format_mapped(&dst_entry->dst_ip, &reply);
                    } else {
                        ds_put_cstr(&reply, "0.0.0.0");
                    }
                    need_ws = 16 - (reply.length - start_len);
                    ds_put_char_multiple(&reply, ' ',
                                         need_ws > 0 ? need_ws : 1);

                    start_len = reply.length;
                    if (!eth_addr_is_zero(dst_entry->dst_mac)) {
                        ds_put_format(&reply, ETH_ADDR_FMT,
                                      ETH_ADDR_ARGS(dst_entry->dst_mac));
                    } else {
                        ds_put_cstr(&reply, "00:00:00:00:00:00");
                    }
                    need_ws = 18 - (reply.length - start_len);
                    ds_put_char_multiple(&reply, ' ',
                                         need_ws > 0 ? need_ws : 1);

                    ds_put_format(&reply, "%-13s%"PRIu32"\n", "",
                                  odp_to_u32(entry->odp_port));
                }
            } else {
                ds_put_format(&reply, "%-15s %-17s %-13s%"PRIu32"\n",
                              "", "",
                              entry->port_name[0] ? entry->port_name : "N/A",
                              odp_to_u32(entry->odp_port));
            }
        }
    }

    ovs_mutex_unlock(&arp_table_mutex);
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

/* arp/set 命令：设置端口本地 IP，可选追加目的 IP。 */
static void
bridge_arp_cache_add(struct unixctl_conn *conn, int argc OVS_UNUSED,
                     const char *argv[], void *aux OVS_UNUSED)
{
    if (argc < 3 || argc > 4) {
        unixctl_command_reply(conn, "Usage: arp/set PORT_NAME SRC_IP DST_IP");
        return;
    }

    const char *port_name = argv[1];
    const char *src_ip_str = argv[2];
    const char *dst_ip_str = (argc == 4) ? argv[3] : NULL;
    struct in6_addr src_ip, dst_ip;

    if (lookup_any(src_ip_str, &src_ip) != 0) {
        unixctl_command_reply(conn, "bad source IP address");
        return;
    }

    if (dst_ip_str && lookup_any(dst_ip_str, &dst_ip) != 0) {
        unixctl_command_reply(conn, "bad destination IP address");
        return;
    }

    ovs_mutex_lock(&arp_table_mutex);

    struct arp_entry *e = arp_cache_lookup(port_name);
    if (!e) {
        unixctl_command_reply(conn, "port not found");
        ovs_mutex_unlock(&arp_table_mutex);
        return;
    }

    e->src_ip = src_ip;
    e->timestamp = time_now();

    if (dst_ip_str) {
        struct arp_dst_entry *dst_entry = arp_dst_entry_lookup(e, &dst_ip);
        if (dst_entry) {
            dst_entry->timestamp = time_now();
        } else {
            struct arp_dst_entry *new_dst = xzalloc(sizeof *new_dst);
            new_dst->dst_ip = dst_ip;
            new_dst->dst_mac = eth_addr_zero;
            new_dst->timestamp = time_now();
            new_dst->next = e->dst_list;
            e->dst_list = new_dst;
        }
    }

    unixctl_command_reply(conn, "OK");
    ovs_mutex_unlock(&arp_table_mutex);
}

void
bridge_arp_cache_init(void)
{
    unixctl_command_register("arp/show", "", 0, 0, bridge_arp_cache_show,
                             NULL);
    unixctl_command_register("arp/set", "[PORT_NAME] SRC_IP DST_IP", 2, 3,
                             bridge_arp_cache_add, NULL);
}
