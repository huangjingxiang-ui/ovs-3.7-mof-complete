#include <config.h>
#include "nexthop.h"

#include <string.h>

#include "openvswitch/dynamic-string.h"
#include "openvswitch/thread.h"
#include "openvswitch/vlog.h"
#include "hash.h"
#include "socket-util.h"
#include "timeval.h"
#include "unixctl.h"
#include "util.h"

VLOG_DEFINE_THIS_MODULE(nexthop);

static struct nexthop_table nexthop_table;
static struct ovs_mutex nexthop_table_mutex = OVS_MUTEX_INITIALIZER;

static inline uint32_t
nexthop_hash(const char *port_name)
{
    return hash_string(port_name, 0) % NEXTHOP_HASH_SIZE;
}

static int
route_lookup_any(const char *host_name, struct in6_addr *address)
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

/* 插入/更新下一跳表项。 */
void
route_insert(struct nexthop_entry *nexthop_entry)
{
    ovs_mutex_lock(&nexthop_table_mutex);

    uint32_t idx = nexthop_hash(nexthop_entry->port_name);
    struct nexthop_entry *e = nexthop_table.buckets[idx];

    while (e) {
        if (strcmp(e->port_name, nexthop_entry->port_name) == 0) {
            e->port_ip = nexthop_entry->port_ip;
            e->nexthop_ip = nexthop_entry->nexthop_ip;
            e->timestamp = time_now();
            ovs_mutex_unlock(&nexthop_table_mutex);
            return;
        }
        e = e->next;
    }

    e = xzalloc(sizeof *e);
    e->port_ip = nexthop_entry->port_ip;
    e->nexthop_ip = nexthop_entry->nexthop_ip;
    ovs_strlcpy(e->port_name, nexthop_entry->port_name, sizeof e->port_name);
    e->timestamp = time_now();

    e->next = nexthop_table.buckets[idx];
    nexthop_table.buckets[idx] = e;

    ovs_mutex_unlock(&nexthop_table_mutex);
}

struct nexthop_entry *
nexthop_lookup(const char *port_name)
{
    uint32_t idx = nexthop_hash(port_name);
    struct nexthop_entry *e = nexthop_table.buckets[idx];

    while (e) {
        if (strcmp(e->port_name, port_name) == 0) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

/* route/add 命令：添加或更新下一跳表项。 */
static void
bridge_nexthop_add(struct unixctl_conn *conn, int argc OVS_UNUSED,
                   const char *argv[], void *aux OVS_UNUSED)
{
    if (argc != 3 && argc != 4) {
        unixctl_command_reply(conn,
            "Usage: route/add [PORT_NAME] PORT_IP NEXT_HOP_IP");
        return;
    }

    const char *port_name = (argc == 4) ? argv[1] : "N/A";
    const char *port_ip_str = (argc == 4) ? argv[2] : argv[1];
    const char *nexthop_ip_str = (argc == 4) ? argv[3] : argv[2];

    struct in6_addr port_ip, nexthop_ip;
    if (route_lookup_any(port_ip_str, &port_ip) != 0
        || route_lookup_any(nexthop_ip_str, &nexthop_ip) != 0) {
        unixctl_command_reply(conn, "Invalid IP format.");
        return;
    }

    ovs_mutex_lock(&nexthop_table_mutex);

    struct nexthop_entry *e = nexthop_lookup(port_name);
    if (!e) {
        unixctl_command_reply(conn, "no port");
    } else {
        e->port_ip = port_ip;
        e->nexthop_ip = nexthop_ip;
        e->timestamp = time_now();
        unixctl_command_reply(conn, "OK");
    }

    ovs_mutex_unlock(&nexthop_table_mutex);
}

/* route/show 命令：打印整张下一跳表。 */
static void
bridge_nexthop_show(struct unixctl_conn *conn, int argc OVS_UNUSED,
                    const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    struct ds reply = DS_EMPTY_INITIALIZER;

    ds_put_format(&reply, "%-15s %-15s %-17s %-12s\n",
                  "Port IP", "Next Hop", "Port Name", "Age (s)");
    ds_put_format(&reply,
                  "--------------- --------------- ----------------- "
                  "--------------------\n");

    ovs_mutex_lock(&nexthop_table_mutex);

    long long now = time_now();
    for (int i = 0; i < NEXTHOP_HASH_SIZE; i++) {
        for (struct nexthop_entry *e = nexthop_table.buckets[i]; e;
             e = e->next) {
            int start_len, need_ws;

            start_len = reply.length;
            if (!ipv6_is_zero(&e->port_ip)) {
                ipv6_format_mapped(&e->port_ip, &reply);
            } else {
                ds_put_cstr(&reply, "0.0.0.0");
            }
            need_ws = 16 - (reply.length - start_len);
            ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

            start_len = reply.length;
            if (!ipv6_is_zero(&e->nexthop_ip)) {
                ipv6_format_mapped(&e->nexthop_ip, &reply);
            } else {
                ds_put_cstr(&reply, "0.0.0.0");
            }
            need_ws = 16 - (reply.length - start_len);
            ds_put_char_multiple(&reply, ' ', need_ws > 0 ? need_ws : 1);

            ds_put_format(&reply, "%-15s %-12lld\n",
                          e->port_name[0] ? e->port_name : "N/A",
                          (now - e->timestamp));
        }
    }

    ovs_mutex_unlock(&nexthop_table_mutex);
    unixctl_command_reply(conn, ds_cstr(&reply));
    ds_destroy(&reply);
}

void
bridge_nexthop_init(void)
{
    unixctl_command_register("route/add", "[PORT_NAME] PORT_IP NEXT_HOP_IP",
                             2, 3, bridge_nexthop_add, NULL);
    unixctl_command_register("route/show", "", 0, 0, bridge_nexthop_show,
                             NULL);
}
