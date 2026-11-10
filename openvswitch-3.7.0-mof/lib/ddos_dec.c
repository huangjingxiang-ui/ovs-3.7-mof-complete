#define _GNU_SOURCE
#include "ddos_dec.h"
#include "packets.h"
#include "dp-packet.h"
#include "openvswitch/vlog.h"
#include <stdio.h>

VLOG_DEFINE_THIS_MODULE(ddos_dec);
/* Implementation notes:
 * - Using simple fast 64-bit hash (splitmix64) seeded with seed + salt for multiple hashes.
 * - Bloom is a bitset (m bits) with k hash functions.
 * - Hash table: each bucket has 3 cells: cell.ip, cell.count, cell.used.
 * - Count-min sketch: uint32_t counts[w * d], hash per row with different salt.
 *
 * Thread-safety: brief mutex protecting updates (simple coarse-grained lock).
 * In OVS integration, you may prefer per-CPU structures or lock-free / rte-style.
 */

/* ---------- small hash family (splitmix64) ---------- */
static inline uint64_t splitmix64_next(uint64_t *state)
{
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
struct ddos_conf conf = {
    .bloom_bits = 512 * 8, /* 8k bits */
    .bloom_k = 3,
    .hash_buckets = 32, /* small table for demo */
    .cms_width = 2048,
    .cms_depth = 3,
    .cms_promotion_threshold = 20, /* after ~20 hits in cms -> promote */
    .hash_drop_threshold = 30,     /* drop once precise count >= 30 */
    .seed = 0xdeadbeefcafef00dULL};

/* Timer related variables */
static long long int ddos_next_cleanup_ms;  /* Next cleanup time in milliseconds */
static long long int ddos_last_hash_change; /* Last time hash table was modified */
static int ddos_enabled_prev;               /* Previous state of ddos detector */

ddos_detector_t *det;
/* combine ip (32-bit) with salt -> 64-bit hash */
static uint64_t dd_hash64(uint64_t seed, uint64_t salt, uint32_t ip)
{
    uint64_t st = seed ^ salt;
    /* start with ip as part of state */
    uint64_t s = st + ((uint64_t)ip << 1);
    return splitmix64_next(&s);
}
// 修改hash函数以支持IP对
static uint64_t dd_hash64_pair(uint64_t seed, uint64_t salt, uint32_t src_ip, uint32_t dst_ip)
{
    uint64_t st = seed ^ salt;
    // 将源IP和目的IP组合成64位值
    uint64_t combined = ((uint64_t)src_ip << 32) | dst_ip;
    uint64_t s = st + (combined << 1);
    return splitmix64_next(&s);
}

/* ---------- bloom filter ---------- */
typedef struct
{
    uint8_t *bits;
    size_t bits_len_bytes;
    size_t m_bits;
    int k;
    uint64_t seed;
} bloom_t;

static bloom_t *bloom_create(size_t m_bits, int k, uint64_t seed)
{
    bloom_t *b = calloc(1, sizeof(*b));
    if (!b)
        return NULL;
    b->m_bits = m_bits;
    b->k = k;
    b->seed = seed;
    b->bits_len_bytes = (m_bits + 7) / 8;
    b->bits = calloc(1, b->bits_len_bytes);
    if (!b->bits)
    {
        free(b);
        return NULL;
    }
    return b;
}

static void bloom_destroy(bloom_t *b)
{
    if (!b)
        return;
    free(b->bits);
    free(b);
}

static inline void bloom_setbit(bloom_t *b, size_t pos)
{
    b->bits[pos >> 3] |= (1u << (pos & 7));
}

static inline int bloom_testbit(const bloom_t *b, size_t pos)
{
    return (b->bits[pos >> 3] >> (pos & 7)) & 1;
}

static void bloom_add(bloom_t *b, uint32_t src_ip, uint32_t dst_ip)
{
    for (int i = 0; i < b->k; ++i)
    {
        uint64_t h = dd_hash64_pair(b->seed, (uint64_t)i * 0x9e3779b97f4a7c1ULL, src_ip, dst_ip);
        size_t pos = (size_t)(h % b->m_bits);
        bloom_setbit(b, pos);
    }
}

static int bloom_contains(const bloom_t *b, uint32_t src_ip, uint32_t dst_ip)
{
    for (int i = 0; i < b->k; ++i)
    {
        uint64_t h = dd_hash64_pair(b->seed, (uint64_t)i * 0x9e3779b97f4a7c1ULL, src_ip, dst_ip);
        size_t pos = (size_t)(h % b->m_bits);
        if (!bloom_testbit(b, pos))
            return 0;
    }
    return 1;
}

/* ---------- hash table (buckets x 3 cells) ---------- */
#define CELLS_PER_BUCKET 3
typedef struct
{
    uint32_t ip;
    uint32_t cnt;
    uint8_t used;
} cell_t;

typedef struct
{
    cell_t *cells; /* array: buckets * CELLS_PER_BUCKET */
    size_t buckets;
    uint64_t seed;
} hash_table_t;

static hash_table_t *hash_table_create(size_t buckets, uint64_t seed)
{
    hash_table_t *h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;
    h->buckets = buckets;
    h->seed = seed;
    h->cells = calloc(buckets * CELLS_PER_BUCKET, sizeof(cell_t));
    if (!h->cells)
    {
        free(h);
        return NULL;
    }
    return h;
}

static void hash_table_destroy(hash_table_t *h)
{
    if (!h)
        return;
    free(h->cells);
    free(h);
}

/* returns pointer to cell if found, NULL if not found */
static cell_t *hash_table_find_cell(hash_table_t *h, uint32_t ip)
{
    uint64_t hv = dd_hash64(h->seed, 0xdeadbeefULL, ip);
    size_t idx = (size_t)(hv % h->buckets);
    cell_t *bstart = &h->cells[idx * CELLS_PER_BUCKET];
    for (int i = 0; i < CELLS_PER_BUCKET; ++i)
    {
        if (bstart[i].used && bstart[i].ip == ip)
            return &bstart[i];
    }
    return NULL;
}

/* find empty cell or return NULL if full */
static cell_t *hash_table_find_empty(hash_table_t *h, uint32_t ip)
{
    uint64_t hv = dd_hash64(h->seed, 0xdeadbeefULL, ip);
    size_t idx = (size_t)(hv % h->buckets);
    cell_t *bstart = &h->cells[idx * CELLS_PER_BUCKET];
    for (int i = 0; i < CELLS_PER_BUCKET; ++i)
    {
        if (!bstart[i].used)
            return &bstart[i];
    }
    return NULL;
}

/* insert IP into table (if empty cell available). Returns pointer to inserted/found cell, else NULL */
static cell_t *hash_table_insert(hash_table_t *h, uint32_t ip, uint32_t init_cnt)
{
    /* check existing first */
    cell_t *c = hash_table_find_cell(h, ip);
    if (c)
    {
        return c;
    }
    /* find empty */
    cell_t *e = hash_table_find_empty(h, ip);
    if (e)
    {
        e->used = 1;
        e->ip = ip;
        e->cnt = init_cnt;
        return e;
    }
    return NULL;
}

/* ---------- Count-min sketch ---------- */
typedef struct
{
    uint32_t *counts; /* length = depth * width */
    size_t width;
    size_t depth;
    uint64_t seed;
} cms_t;

static cms_t *cms_create(size_t width, size_t depth, uint64_t seed)
{
    cms_t *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->width = width;
    c->depth = depth;
    c->seed = seed;
    c->counts = calloc(width * depth, sizeof(uint32_t));
    if (!c->counts)
    {
        free(c);
        return NULL;
    }
    return c;
}

static void cms_destroy(cms_t *c)
{
    if (!c)
        return;
    free(c->counts);
    free(c);
}

static void cms_add(cms_t *c, uint32_t ip, uint32_t delta)
{
    for (size_t r = 0; r < c->depth; ++r)
    {
        uint64_t h = dd_hash64(c->seed, (uint64_t)r * 0x123456789abcdefULL, ip);
        size_t idx = (size_t)(h % c->width);
        uint32_t *slot = &c->counts[r * c->width + idx];
        /* saturating add to avoid overflow */
        if (UINT32_MAX - *slot < delta)
            *slot = UINT32_MAX;
        else
            *slot += delta;
    }
}

static uint32_t cms_query(const cms_t *c, uint32_t ip)
{
    uint32_t est = UINT32_MAX;
    for (size_t r = 0; r < c->depth; ++r)
    {
        uint64_t h = dd_hash64(c->seed, (uint64_t)r * 0x123456789abcdefULL, ip);
        size_t idx = (size_t)(h % c->width);
        uint32_t val = c->counts[r * c->width + idx];
        if (val < est)
            est = val;
    }
    return est == UINT32_MAX ? 0 : est;
}

/* ---------- detector struct ---------- */
struct ddos_detector
{
    struct ddos_conf conf;
    bloom_t *bloom;
    hash_table_t *htable;
    cms_t *cms;
    pthread_mutex_t lock;
    int enabled;
};

ddos_detector_t *ddos_create(const struct ddos_conf *conf)
{
    if (!conf)
        return NULL;
    ddos_detector_t *d = calloc(1, sizeof(*d));
    if (!d)
        return NULL;
    d->conf = *conf;
    d->bloom = bloom_create(conf->bloom_bits, conf->bloom_k, conf->seed ? conf->seed : 0xabcdef1234567890ULL);
    d->htable = hash_table_create(conf->hash_buckets, conf->seed ? conf->seed : 0xfeedfacecafebabeULL);
    d->cms = cms_create(conf->cms_width, conf->cms_depth, conf->seed ? conf->seed : 0x13579bdf2468ace0ULL);
    d->enabled = 1;
    pthread_mutex_init(&d->lock, NULL);
    return d;
}

void ddos_destroy(ddos_detector_t *d)
{
    if (!d)
        return;
    bloom_destroy(d->bloom);
    hash_table_destroy(d->htable);
    cms_destroy(d->cms);
    pthread_mutex_destroy(&d->lock);
    free(d);
}

/* Reset all counters and structures */
void ddos_reset(ddos_detector_t *d)
{
    if (!d)
        return;
    VLOG_INFO("ddos_reset");
    ovs_mutex_lock(&ddos_detector_mutex);
    memset(d->bloom->bits, 0, d->bloom->bits_len_bytes);
    memset(d->htable->cells, 0, d->htable->buckets * CELLS_PER_BUCKET * sizeof(cell_t));
    memset(d->cms->counts, 0, d->cms->width * d->cms->depth * sizeof(uint32_t));
    ovs_mutex_unlock(&ddos_detector_mutex);
}

size_t ddos_get_hash_buckets(const ddos_detector_t *d)
{
    return d ? d->htable->buckets : 0;
}

/*
 * Core processing function:
 * 1) Bloom quick check: if present -> FORWARD
 * 2) Hash table: if found -> increment count, if >= hash_drop_threshold then DROP else FORWARD
 *    if not found:
 *      * if empty cell in bucket -> insert with cnt=1 -> FORWARD
 *      * else bucket full -> cms_add + cms_query
 *         - if cms >= cms_promotion_threshold: promote to hash table (if space)
 *         - FORWARD (we don't immediately drop here; drops controlled by hash threshold)
 */
ddos_action_t ddos_process_packet(const struct dp_packet *packet)
{
    if (!det || !packet)
        return DDOS_ACTION_FORWARD;

    if (!det->enabled) // 添加enabled检查
        return DDOS_ACTION_FORWARD;

    ddos_timer_run();

    VLOG_INFO("ddos_process_packet");

    struct ip_header *ip;
    ip = (void *)((char *)dp_packet_data(packet) + sizeof(struct eth_header));
    uint32_t src_ip = ntohl(get_16aligned_be32(&ip->ip_src));
    uint32_t dst_ip = ntohl(get_16aligned_be32(&ip->ip_dst));
    VLOG_INFO("src IP:" IP_FMT " src IP:" IP_FMT "", IP_ARGS(src_ip), IP_ARGS(dst_ip));

    /* Run timer logic on each packet */
    ovs_mutex_lock(&ddos_detector_mutex);
    /* 1) Bloom quick check */
    if (bloom_contains(det->bloom, src_ip, dst_ip))
    {
        VLOG_INFO("bloom_contains");
        ovs_mutex_unlock(&ddos_detector_mutex);
        return DDOS_ACTION_FORWARD;
    }
    else
        bloom_add(det->bloom, src_ip, dst_ip);

    /* 2) Hash table */
    cell_t *c = hash_table_find_cell(det->htable, dst_ip);
    if (c)
    {
        ddos_last_hash_change = time_msec();
        /* found exact cell: increment and test drop threshold */
        if (UINT32_MAX - c->cnt > 0)
            c->cnt++;
        else
            c->cnt = UINT32_MAX;
        /* optionally update bloom: mark known (here we don't necessarily add to bloom) */
        if (c->cnt >= det->conf.hash_drop_threshold)
        {

            VLOG_WARN("Destination IP:" IP_FMT " may be ddos detected,attackers=%d", IP_ARGS(htonl(dst_ip)), c->cnt);
            ovs_mutex_unlock(&ddos_detector_mutex);
            return DDOS_ACTION_DROP;
        }
        else
        {
            ovs_mutex_unlock(&ddos_detector_mutex);
            return DDOS_ACTION_FORWARD;
        }
    }

    /* not found in hash table-> try to find in Count-min sketch*/
    cms_add(det->cms, dst_ip, 1);
    uint32_t est = cms_query(det->cms, dst_ip);

    if (est >= det->conf.cms_promotion_threshold)
    {
        /* try to promote to hash table (may fail if bucket full) */
        cell_t *ins = hash_table_insert(det->htable, dst_ip, est);
        (void)ins; /* if ins==NULL, bucket full aborts promotion */
        /* When promoted, we also set bloom bit to speed up future checks (optional) */
        bloom_add(det->bloom, src_ip, dst_ip);

        /* Update hash change time */
        ddos_last_hash_change = time_msec();
    }

    /* we forward this packet now (real mitigation via hash_drop_threshold) */
    ovs_mutex_unlock(&ddos_detector_mutex);
    return DDOS_ACTION_FORWARD;
}

/* Enable DDoS detection */
static void
ddos_on(struct unixctl_conn *conn, int argc OVS_UNUSED,
        const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    det->enabled = 1;
    /* Reset all structures when enabling */
    ddos_reset(det);
    ddos_timer_init();
    unixctl_command_reply(conn, "DDos already on");
}

/* Disable DDoS detection */
static void
ddos_off(struct unixctl_conn *conn, int argc OVS_UNUSED,
         const char *argv[] OVS_UNUSED, void *aux OVS_UNUSED)
{
    det->enabled = 0;
    unixctl_command_reply(conn, "DDos already off");
}

/* Initialize DDoS detection timers */
void ddos_timer_init(void)
{
    /* Initialize timer */
    ddos_next_cleanup_ms = time_msec() + 30000; // 30 seconds from now
    ddos_last_hash_change = 0;
}

/* Run DDoS detection timer logic */
void ddos_timer_run(void)
{
    if (!det->enabled)
    {
        return;
    }

    long long int now = time_msec();

    /* Check if hash table was modified recently */
    if (ddos_last_hash_change > 0 && (now - ddos_last_hash_change) < 5000)
    {
        /* Extend cleanup time by 5 seconds if hash table changed in the last 5 seconds */
        if (ddos_next_cleanup_ms < (now + 5000))
        {
            VLOG_INFO("Extending cleanup time by 5 seconds");
            ddos_next_cleanup_ms = now + 5000;
            ddos_last_hash_change = 0; // Reset to avoid repeatedly extending
        }
    }

    /* Check if it's time to clean up */
    if (now >= ddos_next_cleanup_ms)
    {
        ddos_reset(det);
        ddos_next_cleanup_ms = now + 30000; // Schedule next cleanup in 30 seconds
    }
}

/* Initialize DDoS detection */
int ddos_detect_init(void)
{
    det = ddos_create(&conf);
    if (!det)
    {
        VLOG_INFO("create ddos conf failed");
        return 1;
    }

    /* simulate: many packets from same IP */
    return 0;
}

/* Initialize DDoS module */
void ddos_init()
{
    unixctl_command_register("ddos/on", "", 0, 0, ddos_on, NULL);
    unixctl_command_register("ddos/off", "", 0, 0, ddos_off, NULL);
    ddos_timer_init();
}