#ifndef ARP_H
#define ARP_H 1

#include <stdbool.h>

void arp_init(void);
bool arp_run(void);
long long int arp_wait(void);
long long int arp_wake_time(void);

void arp_set_interval(long long int interval_ms);
void arp_set_enabled(bool enabled);

#endif /* arp.h */
