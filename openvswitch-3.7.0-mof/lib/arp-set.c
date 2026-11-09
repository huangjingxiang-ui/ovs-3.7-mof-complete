#include <config.h>
#include "arp-set.h"

#include <string.h>

#include "openvswitch/vlog.h"
#include "unixctl.h"

VLOG_DEFINE_THIS_MODULE(arp_set);

/* 优化输出开关。 */
static bool enable_optimized_output = false;

bool
get_optimized_output_flag(void)
{
    return enable_optimized_output;
}

void
set_optimized_output_flag(bool enable)
{
    enable_optimized_output = enable;
    VLOG_INFO("Optimized output = %s", enable ? "true" : "false");
}

/* optimized_output true|false 命令回调。 */
static void
bridge_optimized_output_cmd(struct unixctl_conn *conn, int argc OVS_UNUSED,
                            const char *argv[], void *aux OVS_UNUSED)
{
    if (argc != 2) {
        unixctl_command_reply(conn, "Usage: optimized_output true|false");
        return;
    }

    if (!strcmp(argv[1], "true")) {
        set_optimized_output_flag(true);
    } else if (!strcmp(argv[1], "false")) {
        set_optimized_output_flag(false);
    } else {
        unixctl_command_reply(conn, "Invalid argument. Use true|false.");
        return;
    }

    unixctl_command_reply(conn,
                          enable_optimized_output ? "Enabled" : "Disabled");
}

void
bridge_optimized_output_init(void)
{
    unixctl_command_register("optimized_output", "true|false", 1, 1,
                             bridge_optimized_output_cmd, NULL);
}
