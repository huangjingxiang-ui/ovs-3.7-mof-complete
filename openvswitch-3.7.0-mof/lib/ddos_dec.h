#ifndef DDOS_DETECT_H
#define DDOS_DETECT_H

#include <errno.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include "openvswitch/thread.h"
#include "openvswitch/hmap.h"
#include "packets.h"
#include "dp-packet.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /* return action for a packet */
    typedef enum
    {
        DDOS_ACTION_FORWARD = 0,
        DDOS_ACTION_DROP = 1
    } ddos_action_t;

    /* configuration for detector */
    struct ddos_conf
    {
        /* Bloom filter: bit array size (bits) and number of hash functions */
        size_t bloom_bits;
        int bloom_k;

        /* Hash table: number of buckets, each bucket has 3 cells (fixed) */
        size_t hash_buckets;

        /* Count-min sketch: width (w) and depth (d) */
        size_t cms_width;
        size_t cms_depth;

        /* Thresholds */
        uint32_t cms_promotion_threshold; /* if CMS estimate > this, promote to hash table */
        uint32_t hash_drop_threshold;     /* if hash-table count >= this, drop packets */

        /* Optional: seed for hash */
        uint64_t seed;
    };

    /* opaque detector handle */
    typedef struct ddos_detector ddos_detector_t;

    static struct ovs_mutex ddos_detector_mutex = OVS_MUTEX_INITIALIZER;
    extern ddos_detector_t *det;
    /* Create & destroy */
    ddos_detector_t *ddos_create(const struct ddos_conf *conf);
    void ddos_destroy(ddos_detector_t *d);

    /*
     * Process a packet identified by source IPv4 (src_ip). Return action:
     *  - DDOS_ACTION_FORWARD: let packet forward
     *  - DDOS_ACTION_DROP: drop packet (mitigation)
     *
     * Note: This function also updates internal data structures (bloom, hash, cms)
     * according to the algorithm described.
     *
     * Caller must convert IP to host-order uint32_t before calling.
     */
    ddos_action_t ddos_process_packet(const struct dp_packet *packet);
    void ddos_init();
    int ddos_detect_init(void);
    /* Helpers to reset / inspect (for tests) */
    void ddos_reset(ddos_detector_t *d);
    void ddos_timer_run(void);

#ifdef __cplusplus
}
#endif

#endif /* DDOS_DETECT_H */
