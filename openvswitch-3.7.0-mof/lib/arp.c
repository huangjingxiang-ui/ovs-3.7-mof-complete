#include <config.h>
#include "arp.h"

#include <limits.h>

#include "openvswitch/poll-loop.h"
#include "openvswitch/vlog.h"
#include "timeval.h"

VLOG_DEFINE_THIS_MODULE(arp);

/* ARP 定时模块：周期触发一次 ARP 处理。 */
struct arp_module {
    long long int last_run;      /* 上次运行时间 */
    long long int next_run;      /* 下次运行时间 */
    long long int interval;      /* 运行间隔(毫秒) */
    bool enabled;                /* 是否启用 */
};

static struct arp_module arp_module;

/* 初始化 ARP 模块，默认 60 秒触发一次。 */
void
arp_init(void)
{
    arp_module.interval = 60000; /* 60 秒 */
    arp_module.enabled = true;
    arp_module.last_run = time_msec();
    arp_module.next_run = arp_module.last_run + arp_module.interval;
}

/* 到达执行时间则返回 true，并重算下次执行时间。 */
bool
arp_run(void)
{
    long long int now = time_msec();

    if (arp_module.enabled && now >= arp_module.next_run) {
        arp_module.last_run = now;
        arp_module.next_run = now + arp_module.interval;
        return true;
    }
    return false;
}

/* 等待直到下次执行时间。 */
long long int
arp_wait(void)
{
    long long int wake_time;

    if (!arp_module.enabled) {
        wake_time = LLONG_MAX;
    } else {
        wake_time = arp_module.next_run;
    }

    poll_timer_wait_until(wake_time);
    return wake_time;
}

/* 获取下次唤醒时间。 */
long long int
arp_wake_time(void)
{
    if (!arp_module.enabled) {
        return LLONG_MAX;
    }
    return arp_module.next_run;
}

/* 设置运行间隔。 */
void
arp_set_interval(long long int interval_ms)
{
    arp_module.interval = interval_ms;
    arp_module.next_run = time_msec() + arp_module.interval;
}

/* 启用/禁用 ARP 模块。 */
void
arp_set_enabled(bool enabled)
{
    arp_module.enabled = enabled;
}
