/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2015 Nicira, Inc.
 * Copyright (c) 2019, 2020, 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <config.h>

#include "dpif-netdev-private-dfc.h"

void ipv4_frag_table_init(struct ipv4_frag_table *table)
{
    for (int i = 0; i < FRAG_HASH_SIZE; i++)
    {
        for (int j = 0; j < FRAG_EACH_BUCKET; j++)
        {
            table->buckets[i].entries[j].packet = NULL;
            table->buckets[i].entries[j].ip_id = 0;
            table->buckets[i].entries[j].time_stamp = 0;
        }
    }
}
void ipv6_frag_table_init(struct ipv6_frag_table *table)
{
    for (int i = 0; i < FRAG_HASH_SIZE; i++)
    {
        for (int j = 0; j < FRAG_EACH_BUCKET; j++)
        {
            table->buckets[i].entries[j].packet = NULL;
            table->buckets[i].entries[j].identification = 0;
            table->buckets[i].entries[j].ext_len = -1;
            table->buckets[i].entries[j].time_stamp = 0;
        }
    }
}

void ipv4_frag_table_uninit(struct ipv4_frag_table *table)
{
    int i, j;

    for (i = 0; i < FRAG_HASH_SIZE; i++) {
        for (j = 0; j < FRAG_EACH_BUCKET; j++) {
            struct ipv4_frag_info *entry = &table->buckets[i].entries[j];

            if (entry->packet) {
                dp_packet_delete(entry->packet);
                entry->packet = NULL;
            }
            entry->ip_id = 0;
            entry->time_stamp = 0;
        }
    }
    table->cout = 0;
}
void ipv6_frag_table_uninit(struct ipv6_frag_table *table)
{
    int i, j;

    for (i = 0; i < FRAG_HASH_SIZE; i++) {
        for (j = 0; j < FRAG_EACH_BUCKET; j++) {
            struct ipv6_frag_info *entry = &table->buckets[i].entries[j];

            if (entry->packet) {
                dp_packet_delete(entry->packet);
                entry->packet = NULL;
            }
            entry->identification = 0;
            entry->ext_len = -1;
            entry->time_stamp = 0;
        }
    }
}

static void
emc_clear_entry(struct emc_entry *ce)
{
    if (ce->flow) {
        dp_netdev_flow_unref(ce->flow);
        ce->flow = NULL;
    }
}

static void
smc_clear_entry(struct smc_bucket *b, int idx)
{
    b->flow_idx[idx] = UINT16_MAX;
}

static void
emc_cache_init(struct emc_cache *flow_cache)
{
    int i;

    flow_cache->sweep_idx = 0;
    for (i = 0; i < ARRAY_SIZE(flow_cache->entries); i++) {
        flow_cache->entries[i].flow = NULL;
        flow_cache->entries[i].key.hash = 0;
        flow_cache->entries[i].key.len = sizeof(struct miniflow);
        flowmap_init(&flow_cache->entries[i].key.mf.map);
    }
}

static void
smc_cache_init(struct smc_cache *smc_cache)
{
    int i, j;
    for (i = 0; i < SMC_BUCKET_CNT; i++) {
        for (j = 0; j < SMC_ENTRY_PER_BUCKET; j++) {
            smc_cache->buckets[i].flow_idx[j] = UINT16_MAX;
        }
    }
}

void
dfc_cache_init(struct dfc_cache *flow_cache)
{
    emc_cache_init(&flow_cache->emc_cache);
    smc_cache_init(&flow_cache->smc_cache);
    ipv4_frag_table_init(&flow_cache->ipv4_frag_table);
    ipv6_frag_table_init(&flow_cache->ipv6_frag_table);
}

static void
emc_cache_uninit(struct emc_cache *flow_cache)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(flow_cache->entries); i++) {
        emc_clear_entry(&flow_cache->entries[i]);
    }
}

static void
smc_cache_uninit(struct smc_cache *smc)
{
    int i, j;

    for (i = 0; i < SMC_BUCKET_CNT; i++) {
        for (j = 0; j < SMC_ENTRY_PER_BUCKET; j++) {
            smc_clear_entry(&(smc->buckets[i]), j);
        }
    }
}

void
dfc_cache_uninit(struct dfc_cache *flow_cache)
{
    ipv6_frag_table_uninit(&flow_cache->ipv6_frag_table);
    ipv4_frag_table_uninit(&flow_cache->ipv4_frag_table);
    smc_cache_uninit(&flow_cache->smc_cache);
    emc_cache_uninit(&flow_cache->emc_cache);
}

/* Check and clear dead flow references slowly (one entry at each
 * invocation).  */
void
emc_cache_slow_sweep(struct emc_cache *flow_cache)
{
    struct emc_entry *entry = &flow_cache->entries[flow_cache->sweep_idx];

    if (!emc_entry_alive(entry)) {
        emc_clear_entry(entry);
    }
    flow_cache->sweep_idx = (flow_cache->sweep_idx + 1) & EM_FLOW_HASH_MASK;
}
