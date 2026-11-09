#ifndef ARP_SET_H
#define ARP_SET_H 1

#include <stdbool.h>

/* 获取当前优化输出开关状态。 */
bool get_optimized_output_flag(void);

/* 设置优化输出开关状态。 */
void set_optimized_output_flag(bool enable);

/* 初始化 optimized_output unixctl 命令。 */
void bridge_optimized_output_init(void);

#endif /* ARP_SET_H */
