/**********************************************************************************************************************/
/**
 **
 ** @file       dpi_wrapper.c
 ** @brief      Wrapper to implement DPI functionalities.
 **
 ** IPOQUE GMBH RESERVES ALL RIGHTS IN THE PROGRAM AS DELIVERED.
 ** THE PROGRAM OR ANY PORTION THEREOF MAY NOT BE USED OR REPRODUCED IN ANY FORM WHATSOEVER WITHOUT THE WRITTEN
 ** CONSENT OF IPOQUE GMBH EXCEPT AS PROVIDED BY LICENSE.
 ** THIS FILE IS PART OF R&S PACE 2 AND IS PROVIDED BY IPOQUE GMBH "AS IS" AND ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL IPOQUE GMBH BE LIABLE FOR ANY DIRECT,
 ** INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
 **
 ** COPYRIGHT (C) 2014 - 2022 BY IPOQUE GMBH
 **/
/**********************************************************************************************************************/
#include "dpi_wrapper.h"
#include <pace2.h>
#include <pthread.h>
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
atomic_uint_fast64_t flow_mtus_exported = 0;
atomic_uint_fast64_t fdep_templates_exported = 0;
#if IPFIX_USE_SOCKET > 0
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static struct fdep_data {
    int sockfd;
    struct sockaddr_in sock_addr;
    int threadid;
    char * ip_dst;
    uint16_t port; //max 65535
} __global_data[DPI_MAX_INSTANCE_COUNT];
#endif

/* Config options via reference. Value can be changed at runtime without restart / plugin reload */
static plugin_flow_data_exporter_runtime_config_t runtime_config_options = {
    .active_timeout_delta_time = IPFIX_ACTIVE_TIMEOUT_DELTA,                 // active flow report in sec. 0 only reports final flow 
    .active_timeout_delta_packets = IPFIX_ACTIVE_DELTA_PKTS,
    .template_refresh_interval = IPFIX_TEMPLATE_INTERVAL_IN_SEC,
    .observation_domain_id = IPFIX_OBSERVATION_DOMAIN_ID,
    .application_id_mapping_refresh_interval = IPFIX_APPLICATION_MAPPING_INTERVAL,
    .mss_timeout = IPFIX_MSS_TIMEOUT,                             // 500ms MSS/PDU timeout flush
};
#endif // PLUGIN_ID

/* Definition of flow-user-data struct for CPI_ENABLED (1)*/
#if DPI_ENABLE_FLOW_TRACKING == 1
typedef struct {
	PACE2_classification_result_event result;
    u64 flow_pkts;
    u32 teid;
    u8 initial_packet_direction;
    char * imsi;
} flow_user_data_t;
#endif
#define INVALID_FLOW_DIR (-1)

struct dpi_object_stats {
    u64 protocol_counter[PACE2_PROTOCOL_COUNT + DPI_DU_NEW_DETECTION_RESERVE];
    u64 protocol_attribute_counter[PACE2_PROTOCOL_ATTRIBUTES_COUNT + DPI_DU_NEW_DETECTION_RESERVE];
    u64 application_counter[PACE2_APPLICATIONS_COUNT + DPI_DU_NEW_DETECTION_RESERVE];
    u64 application_attribute_counter[PACE2_APPLICATION_ATTRIBUTES_COUNT + DPI_DU_NEW_DETECTION_RESERVE];
#if DPI_ENABLE_METADATA_EXTRACTION > 0
    u64 metadata_counter[PACE2_NUMBER_OF_EVENTS];
#endif
    u64 next_packet_id;
    u64 flows_expired_cnt;
    u64 s1_error;
    u64 s2_error;
    u64 s3_error;
    u64 s5_error;
    u64 license_exceeded_cnt;
    u64 gtpu_pkts;
    u64 pkts_offloaded;
    u64 dns_flow_fp;

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
    u64 casc_flows_matched_per_id[CASC_MAX_USER_ENTRIES];
#endif
#if PERFMON > 0
    perf_t perf;
#endif
};

struct dpi_instance_opaque {
    pthread_mutex_t cfg_mutex;
    /* PACE 2 module pointer */
    PACE2_module * pace2[DPI_MAX_INSTANCE_COUNT];
#if CPI_ENABLED >= 1
    struct pace2_pht * flow_pht[DPI_MAX_INSTANCE_COUNT];
    struct pace2_pht * subscr_pht[DPI_MAX_INSTANCE_COUNT];
    u64 flow_entry_size;   /* size of flow pht entry incl. flow_user_data_t */ 
    u64 subscr_entry_size; /* size of subscriber pht entry *excl.* subscr_usr_t_size */
    u64 subscr_usr_t_size; /* size of additional subscr_usr_t */
    /* scratchpad for flows not requiring PHT (special protocols) */
    u8 * flow_data[DPI_MAX_INSTANCE_COUNT];
#endif
    /* PACE 2 configuration structure */
    struct PACE2_global_config config;
    dpi_optargs_t dpi_opts;
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
    PACE2_plugin_configuration plugin_config;
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
    PACE2_plugin_configuration plugin_config;
#endif
    struct dpi_object_stats dpi_object_stats[DPI_MAX_INSTANCE_COUNT];
    u8 dpi_worker_count;
};

static void * dlopen_wrapper(char const * filename, int flag, void * user_ptr)
{
    UNUSED(user_ptr);
    void * handle = dlopen(filename, flag);

    if (!handle) {
        fprintf(stderr, "dlopen_wrapper error: %s\n", dlerror());
    }

    return handle;
} /* dlopen_wrapper */

static void * dlsym_wrapper(void * handle, const char * symbol, void * user_ptr)
{
    UNUSED(user_ptr);

    return dlsym(handle, symbol);
} /* dlsym_wrapper */

static int dlclose_wrapper(void * handle, void * user_ptr)
{
    UNUSED(user_ptr);

    return dlclose(handle);
} /* dlclose_wrapper */

#if CPI_ENABLED >= 1
/**
 * @brief Configures external hash table for tracking of flows
 *
 * @param[in] pace2           PACE2 instance
 * @param[in] tid             Thread id to allocate hash table NUMA aware
 * @param[out] flow_pht       Double pointer to pht struct
 * @param[out] new_entry_size Pointer to u64 entry_size
 * @return int:               Negative value in case of errors. 0 for success. 1 for flow_scratchpad
 */
static int flow_hash_table_config(PACE2_module ** const pace2, unsigned int const tid, struct pace2_pht ** flow_pht, u64 * new_entry_size, dpi_optargs_t * opts)
{
    u32 const flow_memory_size = pace2_get_flow_memory_size(*pace2, tid);

#if DPI_ENABLE_FLOW_TRACKING == 0
    /* EXPERT ONLY: Specific sflow usecase. Skip flow_table pht config and init.*/
    *new_entry_size = flow_memory_size;
    return 1;
#else
    struct PACE2_pht_config pht_conf;
    if (0 == flow_memory_size || new_entry_size == NULL) {
        fprintf(stderr, "Error creating flow pht table\n");
        return -1;
    }

    /* Initialize config structure */
    pace2_pht_init_default_config(&pht_conf);

    /* Set the size of the element key. For flows the 5 tuple
       (src ip [16], dst ip [16], src port [2], dst port [2], l4 protocol [1]) is used.
       This makes a total of 37 bytes for the key, however they key size must be
       a multiple of 4 which is why 40 is used here. The unused bytes will be zeroed out. */
    pht_conf.unique_key_size = 40;

    /* The size of memory required for every element */
    pht_conf.user_buffer_size = flow_memory_size;

    pht_conf.memory_size =
        pace2_pht_calculate_hash_size(opts->flow_table.max_entries, pht_conf.unique_key_size, pht_conf.user_buffer_size);

    /* timeout in dpi.h */
    pht_conf.timeout = opts->flow_table.timeout_in_s * PHT_SECONDS;

    /* Set memory allocation/deallocation functions for the hash table */
    pht_conf.ipq_malloc = util_malloc;
    pht_conf.ipq_free = util_free;

    /* Initialize the hash table. Ignore tid here because we have an array of pht at the dpi struct */
    *flow_pht = pace2_pht_create(&pht_conf, 0);

    if (*flow_pht == NULL) {
        fprintf(stderr, "Initialization of flow hash table failed\n");
        return -2;
    } else{
        printf("flow_hash_table [address:%p] allocated %llu Bytes for %u entries \n", *flow_pht, pht_conf.memory_size, opts->flow_table.max_entries);
    }

    /* Get the size of the user buffer */
    *new_entry_size = pace2_pht_get_user_buffer_size(*flow_pht);
    return 0;
#endif
} /* flow_hash_table_config */


/**
 * @brief Configures external hash table for tracking of subscribers/ IP endpoints.
 *
 * @param[in] pace2             PACE2 instance
 * @param[in] tid               Thread id to allocate hash table NUMA aware
 * @param[out] subscr_pht       Double pointer to pht struct
 * @param[out] new_entry_size   Pointer to u64 entry_size
 * @return int:                 Negative value in case of errors. 0 for success.
 */
static int subscriber_hash_table_config(PACE2_module ** const pace2, unsigned int const tid, struct pace2_pht ** subscr_pht, u64 * new_entry_size, dpi_optargs_t * opts)
{
    struct PACE2_pht_config pht_conf;
    u32 subscr_entry_size = pace2_get_subscriber_memory_size(*pace2, tid);
    if (0 == subscr_entry_size || new_entry_size == NULL) {
        fprintf(stderr, "Error creating subscriber pht table\n");
        return -1;
    }
    /* Initialize config structure */
    pace2_pht_init_default_config(&pht_conf);

    /* Set the size of the element key. For subscribers this is a single ip address.
       For IPv6 16 bytes are needed per address. The remaining bytes will be zeroed for IPv4. */
    pht_conf.unique_key_size = 16;

    /* The size of memory required for every element */
    pht_conf.user_buffer_size = subscr_entry_size;

    /* Use xMB memory for the hash table */
    pht_conf.memory_size = pace2_pht_calculate_hash_size(opts->subscriber_table.max_entries,
                                                         pht_conf.unique_key_size,
                                                         pht_conf.user_buffer_size);

    /* timeout in dpi.h */
    pht_conf.timeout = opts->subscriber_table.timeout_in_s * PHT_SECONDS;

    /* Set memory allocation/deallocation functions for the hash table */
    pht_conf.ipq_malloc = util_malloc;
    pht_conf.ipq_free = util_free;

    /* Initialize the hash table. Ignore tid here because we have an array of pht at the dpi struct */
    *subscr_pht = pace2_pht_create(&pht_conf, 0);

    if (*subscr_pht == NULL) {
        fprintf(stderr, "Initialization of subscriber hash table failed\n");
        return -2;
    } else{
        printf("subscriber_hash_table allocated %llu Bytes\t for %u entries\n", pht_conf.memory_size, opts->subscriber_table.max_entries);
    }

    /* Return the size of the user buffer excl. user_data */
    *new_entry_size = pace2_get_subscriber_memory_size(*pace2, tid);

    return 0;
} /* subscriber_hash_table_config */

#endif // CPI

dpi_return_t dpi_enable_discrete_thread(dpi_instance * const dpi_instance_in, unsigned int const tid)
{
    pthread_mutex_lock(&dpi_instance_in->cfg_mutex);
    if (tid < (u8)DPI_MAX_INSTANCE_COUNT) {
        pace2_init_thread(dpi_instance_in->pace2[tid], 0);

/* create hash tables for flows and subscribers NUMA aware based on discrete_thread_init */
#if CPI_ENABLED >= 1
        if (flow_hash_table_config(&dpi_instance_in->pace2[tid],
                                   0,
                                   &dpi_instance_in->flow_pht[tid],
                                   &dpi_instance_in->flow_entry_size,
                                   &dpi_instance_in->dpi_opts) < 0) {
            printf("[ERROR] pace2_init flow PHT. Try reducing dpi_object->dpi_opts.flow_table.max_entries\n");
            pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
            return DPI_RETURN_FAILURE;
        }
        if (dpi_instance_in->dpi_opts.flow_table.skip_dns_tracking == 1){
            /* sflow usecase: reserve scratchpad flow area in lieu of flow_pht or to skip flow-pht for dns */
            dpi_instance_in->flow_data[tid] = calloc(dpi_instance_in->flow_entry_size, sizeof(uint8_t));
            if (dpi_instance_in->flow_data == NULL) {
                pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
                return DPI_RETURN_FAILURE;
        }
        }

        if (dpi_instance_in->dpi_opts.subscriber_table.enabled){
            if (0 != subscriber_hash_table_config(&dpi_instance_in->pace2[tid],
                                                0,
                                                &dpi_instance_in->subscr_pht[tid],
                                                &dpi_instance_in->subscr_entry_size,
                                                &dpi_instance_in->dpi_opts)
                                                ) {
                printf("[ERROR] pace2_init subscriber PHT. Try reducing DPI_MAX_CONCURRENT_SUBSCRIBERS\n");
                pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
                return DPI_RETURN_FAILURE;
            }
        }
        printf("Flow tracking mode:\t\t[%s]\n",
               DPI_ENABLE_FLOW_TRACKING > 0 ? "FLOW_PHT" : "disabled, Mock via SCRATCHPAD");
        printf("Subscriber tracking mode:\t[%s]\n", dpi_instance_in->dpi_opts.subscriber_table.enabled > 0 ? "ext. SUBSCR_PHT" : "disabled");
        printf("pace2_init flow/subscr on thread:%u %llu/%llu: \n",
               tid,
               dpi_instance_in->flow_entry_size,
               dpi_instance_in->subscr_entry_size);
#endif //CPI_ENABLED >= 1
        dpi_instance_in->dpi_worker_count++;
        pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
        return DPI_RETURN_SUCCESS;
    } else {
        pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
        return DPI_RETURN_FAILURE;
    }
    pthread_mutex_unlock(&dpi_instance_in->cfg_mutex);
}

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
static bool on_init(int thread_id, unsigned char * tx_buffer, size_t length, void * usr_data)
{
    UNUSED(tx_buffer);
    UNUSED(length);
    struct fdep_data * fd = (struct fdep_data*)usr_data;
    if (fd != NULL){
        thread_id = fd->threadid;
    }
    printf("[FLOW PLUGIN FDEP init called on threadid %i\n", thread_id);
#if IPFIX_USE_SOCKET > 0
    if ((__global_data[thread_id].sockfd = socket(PF_INET, SOCK_DGRAM, 0)) <= 0) {
        printf("[ERROR PLUGIN FDEP threadid %i-> export to port %u]m -> socket() returned < 0 \n", thread_id, IPFIX_DST_PORT+thread_id);
        return false;
    }

    __global_data[thread_id].sock_addr.sin_family = AF_INET;
    if (__global_data[thread_id].port > 0){
        __global_data[thread_id].sock_addr.sin_port = htons(__global_data[thread_id].port + thread_id);
    }else{
        __global_data[thread_id].sock_addr.sin_port = htons(IPFIX_DST_PORT + thread_id);
    }
    
    if (__global_data[thread_id].ip_dst != NULL){
         __global_data[thread_id].sock_addr.sin_addr.s_addr = inet_addr(__global_data[thread_id].ip_dst);
    }else{
        __global_data[thread_id].sock_addr.sin_addr.s_addr = inet_addr(IPFIX_COLLECTOR_ADDRESS);
    }
    printf("[FLOW PLUGIN FDEP init parsed IP %u port %u\n",  __global_data[thread_id].sock_addr.sin_addr.s_addr ,  ntohs(__global_data[thread_id].sock_addr.sin_port));
    memset(__global_data[thread_id].sock_addr.sin_zero, '\0', sizeof(__global_data[thread_id].sock_addr.sin_zero));

    sendto(__global_data[thread_id].sockfd,
           tx_buffer,
           length,
           0,
           (struct sockaddr *)&__global_data[thread_id].sock_addr,
           sizeof(struct sockaddr_in));
#endif
    return true;
}

static bool on_close(int thread_id, unsigned char * tx_buffer, size_t length, void * usr_data)
{
    UNUSED(tx_buffer);
    UNUSED(length);
    struct fdep_data * fd = (struct fdep_data*)usr_data;
    if (fd != NULL){
        thread_id = fd->threadid;
    }
#if IPFIX_USE_SOCKET > 0
    sendto(__global_data[thread_id].sockfd,
           tx_buffer,
           length,
           0,
           (struct sockaddr *)&__global_data[thread_id].sock_addr,
           sizeof(struct sockaddr_in));
    close(__global_data[thread_id].sockfd);
#endif
    return true;
}

static bool on_template_update(int thread_id, unsigned char * tx_buffer, size_t length, void * usr_data)
{
    UNUSED(tx_buffer);
    UNUSED(length);
    ++fdep_templates_exported;
    struct fdep_data * fd = (struct fdep_data*)usr_data;
    if (fd != NULL){
        thread_id = fd->threadid;
    }
#if IPFIX_USE_SOCKET > 0
    sendto(__global_data[thread_id].sockfd,
           tx_buffer,
           length,
           0,
           (struct sockaddr *)&__global_data[thread_id].sock_addr,
           sizeof(struct sockaddr_in));
#endif
    return true;
}

/*
Depending on production code, consider to outsource into another processing
thread via pthread, lcore, dpdk etc. Instead of stalling a worker with socket
handling, consider using dpdk lockfree ring lib. See references for
multi-consumer / multi-producer mode:
https://doc.dpdk.org/guides/prog_guide/ring_lib.html#references
https://github.com/DPDK/dpdk/blob/main/app/test/test_ring_stress_impl.h

For the sake of performance consider handling UDP via dpdk-based sockets,
e.g. https://github.com/leoll2/UDPDK
*/
static bool on_flow_dropped(int thread_id, unsigned char * tx_buffer, size_t length, void * usr_data)
{
    UNUSED(tx_buffer);
    UNUSED(length);
    ++flow_mtus_exported;
    // add ringbuf and/or sendto() handling here. Currently only account exported flows
    struct fdep_data * fd = (struct fdep_data*)usr_data;
    if (fd != NULL){
        thread_id = fd->threadid;
    }
#if IPFIX_USE_SOCKET > 0
    sendto(__global_data[thread_id].sockfd,
           tx_buffer,
           length,
           0,
           (struct sockaddr *)&__global_data[thread_id].sock_addr,
           sizeof(struct sockaddr_in));
#endif
    return true;
}

static dpi_return_t init_plugin_config_flow_exporter(PACE2_plugin_configuration * const pace2_plugin_config,
                                                     dpi_optargs_t * opts,
                                                     char const * const plugin_file,
                                                     char const * const config_file,
                                                     int threadid)
{
    UNUSED(config_file);
    memset(pace2_plugin_config, 0, sizeof(PACE2_plugin_configuration));
    // pace2_plugin_config->logging_callback = logging_callback;
    /* Use custom wrapper functions for alloc, free and realloc.
     * Disable by commenting out or assigning NULL. */
    pace2_plugin_config->memory_malloc = util_malloc;
    pace2_plugin_config->memory_realloc = util_realloc;
    pace2_plugin_config->memory_free = util_free;

    /* Set PLUG-IN *.so file */
    pace2_plugin_config->plugin_file = plugin_file;

    static struct plugin_flow_data_exporter_config flow_data_exporter_config = {0};
    pace2_plugin_config->plugin_config_type = PACE2_PLUGIN_CONFIG_TYPE_STRUCT;
    pace2_plugin_config->plugin_config = &flow_data_exporter_config;
    pace2_plugin_config->plugin_config_len = sizeof(struct plugin_flow_data_exporter_config);

    /**< Called on initialization with new template data. */
    flow_data_exporter_config.fn_init = on_init;
    /**< Called for each flow_dropped_event with flow data records. Set timeout via FLOW_TIMEOUT */
    flow_data_exporter_config.fn_export = on_flow_dropped;
    /**< Called on PACE2 graceful shutdown / cleanup. */
    flow_data_exporter_config.fn_close = on_close;

    /* Change config below to configure max.segment size (mss) */
    flow_data_exporter_config.mss = opts->fdep_plugin.mss_size;
    /* or disable_multi_flow_export -> ignores mss setting */
    flow_data_exporter_config.disable_multi_flow_export = opts->fdep_plugin.disable_name_resolution_flows;
    flow_data_exporter_config.fn_template_update = on_template_update;
    flow_data_exporter_config.enable_all_ipfix_ies = 1;
    flow_data_exporter_config.runtime_options = &runtime_config_options;
    flow_data_exporter_config.disable_name_resolution_flows = 0; //allow dns
    flow_data_exporter_config.usr_data = &__global_data[threadid];

    runtime_config_options.active_timeout_delta_time = opts->fdep_plugin.active_timeout_delta_time;
    runtime_config_options.active_timeout_delta_packets = opts->fdep_plugin.active_timeout_delta_packets;
    runtime_config_options.template_refresh_interval = opts->fdep_plugin.template_refresh_interval;
    runtime_config_options.observation_domain_id = opts->fdep_plugin.observation_domain_id;
    runtime_config_options.application_id_mapping_refresh_interval = opts->fdep_plugin.application_id_mapping_refresh_interval;
    runtime_config_options.mss_timeout = opts->fdep_plugin.mss_timeout;
    
    return DPI_RETURN_SUCCESS;
}
#endif // PLUGIN_ID IPFIX

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
static void panic(const char * msg)
{
    fprintf(stderr, "%s", msg);
    exit(1);
} /* panic */

static char * read_config_file(char const * const config_file, u32 * length)
{
    FILE * infile;
    char * buffer;
    long numbytes;

    if ((infile = fopen(config_file, "r")) == NULL) {
        fprintf(stderr, "Could not open configuration file: %s\n", config_file);
        return NULL;
    }

    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    fseek(infile, 0L, SEEK_SET);

    if ((buffer = (char *)util_malloc(numbytes, sizeof(char), NULL, 0)) == NULL) {
        fclose(infile);
        return NULL;
    }

    long result = fread(buffer, sizeof(char), numbytes, infile);
    if (result != numbytes) {
        panic("Could not read configuration file.");
    }

    fclose(infile);
    *length = (u32)numbytes;
    return buffer;
}
static dpi_return_t init_plugin_config_casc(PACE2_plugin_configuration * const pace2_plugin_config,
                                            char const * const plugin_file,
                                            char const * const config_file)
{
    memset(pace2_plugin_config, 0, sizeof(PACE2_plugin_configuration));
    // pace2_plugin_config->logging_callback = logging_callback;
    /* Use custom wrapper functions for alloc, free and realloc.
     * Disable by commenting out or assigning NULL. */
    pace2_plugin_config->memory_malloc = util_malloc;
    pace2_plugin_config->memory_realloc = util_realloc;
    pace2_plugin_config->memory_free = util_free;

    /* Set PLUG-IN *.so file */
    pace2_plugin_config->plugin_file = plugin_file;
    pace2_plugin_config->plugin_config_type = PACE2_PLUGIN_CONFIG_TYPE_TEXT;
    pace2_plugin_config->plugin_config = read_config_file(config_file, &pace2_plugin_config->plugin_config_len);

    return DPI_RETURN_SUCCESS;
}
#endif // PLUGIN_CASC

dpi_return_t dpi_init_instance(dpi_instance ** const dpi_instance_in, u8 const dpi_worker_count, dpi_optargs_t * opts)
{
    if (dpi_worker_count >= DPI_MAX_INSTANCE_COUNT) {
        return DPI_RETURN_FAILURE;
    }

    PACE2_return_state ret;

    dpi_instance * dpi_object = (dpi_instance *)calloc(1, sizeof(struct dpi_instance_opaque));
    if (dpi_object == NULL) {
        return DPI_RETURN_FAILURE;
    }
    if (opts == NULL){
        return DPI_RETURN_INPUT_MISSING;
    }else{
        memcpy(&dpi_object->dpi_opts, opts, sizeof(dpi_optargs_t));
    }

    struct PACE2_global_config * const config = &dpi_object->config;
    PACE2_module * pace2 = NULL;

    for (u8 tid = 0; tid < dpi_worker_count; tid++) {
        pace2_init_default_config(config);
        config->general.io_timestamp_resolution = dpi_object->dpi_opts.dpi.io_resolution;
        
        /* Set necessary memory wrapper functions */
        config->general.pace2_alloc = util_malloc;
        config->general.pace2_free = util_free;
        config->general.pace2_realloc = util_realloc;

        config->dynamic_upgrade.pace2_dlopen = dlopen_wrapper;
        config->dynamic_upgrade.pace2_dlsym = dlsym_wrapper;
        config->dynamic_upgrade.pace2_dlclose = dlclose_wrapper;

        config->dynamic_upgrade.enabled = IPQ_TRUE;
        if (config->dynamic_upgrade.enabled == IPQ_TRUE) {
            config->dynamic_upgrade.reserve_bytes = DPI_DYNAMIC_UPGRADE_BUFFER_BYTES;
            if (dpi_object->dpi_opts.subscriber_table.enabled) {
                config->dynamic_upgrade.release_memory_age =
                    dpi_object->dpi_opts.flow_table.timeout_in_s > dpi_object->dpi_opts.subscriber_table.timeout_in_s ? dpi_object->dpi_opts.flow_table.timeout_in_s : dpi_object->dpi_opts.subscriber_table.timeout_in_s;
            } else {
                config->dynamic_upgrade.release_memory_age = dpi_object->dpi_opts.flow_table.timeout_in_s;
            }
            config->dynamic_upgrade.release_memory_age *= config->general.io_timestamp_resolution;
        }

        config->general.max_number_of_events.element_number = 10000;

        /* Set callback for additional debug informations */
        // config->general.process_information = pace2_default_process_information_callback_impl;

        config->general.number_of_threads = 1; // force multi-instancing, meaning each PACE2 module is separate

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
        config->general.discrete_thread_init = IPQ_FALSE;
        // fixes a bug that the plugin callback usr-ptr is not valid. NUMA-awareness is still given due to PHT init
#else
        config->general.discrete_thread_init = IPQ_TRUE;
#endif

#if CPI_ENABLED == 0
        /* Set external tracking for flows and subscribers */
        config->tracking.flow.generic.type = INTERNAL;
        config->tracking.flow.generic.max_size.mem_basis = ELEMENT_NUMBER;
        
        config->tracking.flow.generic.max_size.element_number = dpi_object->dpi_opts.flow_table.max_entries;
        config->tracking.flow.generic.timeout = dpi_object->dpi_opts.flow_table.timeout_in_s * config->general.io_timestamp_resolution;

        config->tracking.subscriber.generic.type = INTERNAL;
        config->tracking.subscriber.generic.max_size.mem_basis = ELEMENT_NUMBER;
        config->tracking.subscriber.generic.max_size.element_number = dpi_object->dpi_opts.subscriber_table.max_entries;
        config->tracking.subscriber.generic.timeout = dpi_object->dpi_opts.subscriber_table.timeout_in_s * config->general.io_timestamp_resolution;
        /* The following features improves detection rate */

        /* Stage 1: enable IP defragmentation */
        config->s1_preparing.defrag.enabled = 1;
        config->s1_preparing.max_framing_depth = 10;
        config->s1_preparing.max_decaps_level = 10;

        /* Stage 2: enable PARO and set necessary values */
        config->s2_reordering.enabled = 1;
        config->s2_reordering.packet_buffer_size = 32 * 1024 * 1024;
        config->s2_reordering.packet_timeout = 10 * config->general.io_timestamp_resolution;

        /* Stage 3: enable specific classification components */
        config->s3_classification.csi_enabled = 0;
        // config.s3_classification.asym_detection_enabled = 1
        config->s3_classification.sit.enabled = 1;
        config->s3_classification.sit.key_reduce_factor = 1;
        config->s3_classification.sit.memory = 16 * 1024 * 1024;
#elif CPI_ENABLED >= 1
        config->s1_preparing.defrag.enabled = 0;
        config->s1_preparing.max_framing_depth = 10;
        config->s2_reordering.enabled = 0;
        config->tracking.flow.generic.type = EXTERNAL;
        config->tracking.subscriber.generic.type = EXTERNAL;
        config->s3_classification.unclassified_after_packet_count = dpi_object->dpi_opts.dpi.unclassified_after_packet_count;
        // enable ASYM option in case DPI just receive25s request-sided traffic ( Client to S)
        config->s3_classification.asym_detection_enabled = 0;
        config->s3_classification.fpc_ip.enabled = dpi_object->dpi_opts.dpi.fpc_enabled;
        config->s3_classification.fpc_ip.mode = PACE2_FPC_IP_AND_PATTERN_MATCH;
        /* improve Stage3 classification performance with classification_mode adaption:
         */
        config->s3_classification.classification_mode = PACE2_ON_CLASSIFICATION_CHANGE_MODE;
        if (config->s3_classification.classification_mode != PACE2_DEFAULT_EVERY_PACKET_MODE) {
            config->tracking.flow.generic.user_data_size = sizeof(flow_user_data_t);
        }


#endif // CPI_ENABLED >= 1

        if (0 == dpi_object->dpi_opts.subscriber_table.enabled){
            config->tracking.subscriber.specific.side = NONE;
            config->tracking.subscriber.generic.timeout = 0;
        }

        if (dpi_object->dpi_opts.dpi.deca_max_entries > 0){
            config->s3_classification.deca.enabled = IPQ_TRUE;
            config->s3_classification.deca.memory.mem_basis = ELEMENT_NUMBER;
            /* DECA maximal supported elements 2^32 -1 */
            config->s3_classification.deca.memory.element_number = dpi_object->dpi_opts.dpi.deca_max_entries;
            config->s3_classification.deca_mode = DECA_AND_PATTERN_MATCH;
        }

        /* Event policy, first step: disable all events */
        pace2_epol_set_config_policy(config, IPQ_FALSE);
        // Just Enable necessary Events in event-policy
        pace2_epol_set_config_policy_of_event(config, PACE2_LICENSE_EXCEEDED_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_CLASSIFICATION_RESULT, IPQ_TRUE);
        // enable basic metadata events
#if DPI_ENABLE_METADATA_EXTRACTION > 0
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_HTTP_REQUEST_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_HTTP_RESPONSE_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_QUIC_CLIENT_HELLO_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_SSL_CLIENT_HELLO_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_SSL_SERVER_HELLO_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_OS_EVENT, IPQ_TRUE);
        config->s3_classification.os_enabled = IPQ_TRUE;
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_SIP_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_DNS_EVENT, IPQ_TRUE);
        pace2_epol_set_config_policy_of_event(config, PACE2_BASIC_HTTP2_EVENT, IPQ_TRUE);
#endif
        // best performance setup below. Enable protocols which are mostly seens in todays traffic + match with DECA
        // New approach. Disable the CPU-intensive protocols which are very unlikely to be seen.
        config->s3_classification.enabled_classifications.protocols.protocols.gadugadu = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.ares = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.bit = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.imo = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.tango = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.imesh = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.skype = IPQ_FALSE; // only old
                                                                                                 // proprietary,
                                                                                                 // nowadays on SSL/TLS
        config->s3_classification.enabled_classifications.protocols.protocols.ultrasurf = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.viber = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.pubg_mobile = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.edk = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.cassandra_query_language = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.uusee = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.winny = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.gnu = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.winmx = IPQ_FALSE;
        config->s3_classification.enabled_classifications.protocols.protocols.freenet = IPQ_FALSE;

        /*plugin configs below [optional]*/
#ifdef PLUGIN_ID
        if (getenv("PACE2_PLUGIN_SO") == NULL) {
            printf("[ERROR] PLUGIN support compiled, but not source. Set environment variables \n");
            free(dpi_object);
            return DPI_RETURN_FAILURE;
        }
        pace2_epol_set_config_policy_of_event(config, PACE2_PLUGIN_EVENT, IPQ_TRUE);
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
        /* macro check is needed since header file will not be present otherwise*/
        if (dpi_object->dpi_opts.fdep_plugin.enabled){
            config->s3_classification.pa_tags.enabled = 1;
            /* Set PACE2 PLUG-IN-specific configuration */
            config->plugin.configurations = &dpi_object->plugin_config;
            config->plugin.configurations_len = 1;

            __global_data[tid].threadid = tid; //  increment per lcoreid
            // parse ip port 
            if (opts != NULL){
                
                __global_data[tid].ip_dst = dpi_object->dpi_opts.fdep_plugin.ip_dst != NULL ? strdup(dpi_object->dpi_opts.fdep_plugin.ip_dst) : IPFIX_COLLECTOR_ADDRESS;
                
                __global_data[tid].port = dpi_object->dpi_opts.fdep_plugin.port > 0 ? dpi_object->dpi_opts.fdep_plugin.port : IPFIX_DST_PORT;
                printf("[INFO] FDEP custom collector ip %s port %u\n", __global_data[tid].ip_dst, __global_data[tid].port);
            }else{
                //default collector
                __global_data[tid].ip_dst = IPFIX_COLLECTOR_ADDRESS;
                __global_data[tid].port = IPFIX_DST_PORT;
            }

            init_plugin_config_flow_exporter(&dpi_object->plugin_config, &dpi_object->dpi_opts, getenv("PACE2_PLUGIN_SO"), NULL, tid);
        }

#endif // PLUGIN_ID
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
        /* Set PACE2 PLUG-IN-specific configuration */
        config->plugin.configurations = &dpi_object->plugin_config;
        config->plugin.configurations_len = 1;

        init_plugin_config_casc(&dpi_object->plugin_config, getenv("PACE2_PLUGIN_SO"), getenv("CUSTOM_SIGNATURES_FILE"));
#endif
        /* Initialize PACE 2 detection module */
        ret = pace2_init(config, &pace2);

        if (ret != PACE2_SUCCESS) {
            printf("pace2_init returned error code %u: '%s'\n", ret, pace2_return_state_str(ret));
            free(dpi_object);
            return DPI_RETURN_FAILURE;
        }

        if (pace2 == NULL) {
            printf("pace2 module is null \n");
            free(dpi_object);
            return DPI_RETURN_FAILURE;
        }

        dpi_object->pace2[tid] = pace2;
    }

    dpi_object->dpi_worker_count = 0; // will be incremented for each port to observe via dpi_enable_discrete_thread()
                                      // shall equal later on the requested value of dpi_worker_count
    *dpi_instance_in = dpi_object;

    return DPI_RETURN_SUCCESS;
}

#if DPI_ENABLE_METADATA_EXTRACTION > 0
static char const * util_metadata_translate_enum(PACE2_event_type m_id)
{
    switch (m_id){
        case PACE2_BASIC_HTTP_REQUEST_EVENT: return "METADATA_HTTP_REQUEST";      /**< Metadata for an HTTP request, */
        case PACE2_BASIC_HTTP_RESPONSE_EVENT: return "METADATA_HTTP_RESPONSE";     /**< Metadata for an HTTP response, */
        case PACE2_BASIC_QUIC_CLIENT_HELLO_EVENT: return "METADATA_QUIC_CLIENT_HLO"; /**< Metadata for a QUIC client hello, */
        case PACE2_BASIC_SSL_CLIENT_HELLO_EVENT: return "METADATA_TLS_CLIENT_HLO";  /**< Metadata for a TLS client hello response*/
        case PACE2_BASIC_SSL_SERVER_HELLO_EVENT: return "METADATA_TLS_SERVER_HLO";  /**< Metadata for a TLS server hello, */
        case PACE2_OS_EVENT: return "METADATA_OS_DETECTION";      /**< Metadata for OS detection, */
        case PACE2_BASIC_SIP_EVENT: return "METADATA_SIP";               /**< Metadata for SIP, */
        case PACE2_BASIC_DNS_EVENT: return "METADATA_DNS";               /**< Metadata for DNS, */
        case PACE2_BASIC_HTTP2_EVENT: return "METADATA_HTTP2";             /**< Metadata for an HTTP2 */
        case PACE2_NAT_EVENT: return "METADATA_BTD";               /**< Metadata for BTD */
        case PACE2_NUMBER_OF_EVENTS: return "METADATA_TYPES_COUNT";        /**< Number of available metadata types */
        default: break;
    }
    return NULL;
}
#endif

void dpi_print_stats(FILE * fp, dpi_instance * const dpi_instance_in)
{
    if (fp == NULL || dpi_instance_in == NULL) {
        return;
    }
    fprintf(fp, "PACE2 statistics ===========================================\n");

    for (u8 tid = 0; tid < dpi_instance_in->dpi_worker_count; tid++) {
        fprintf(fp,
                "Total Packets @dpi_thread %02u %14llu\n",
                tid,
                dpi_instance_in->dpi_object_stats[tid].next_packet_id);
        if (dpi_instance_in->dpi_opts.dpi.flow_offload > 0){
            fprintf(fp,
                    "\t\t->fp_offload %14llu\t%.0f%%\n",
                    dpi_instance_in->dpi_object_stats[tid].pkts_offloaded,
                    dpi_instance_in->dpi_object_stats[tid].pkts_offloaded > 0
                        ? dpi_instance_in->dpi_object_stats[tid].pkts_offloaded * 100 /
                            dpi_instance_in->dpi_object_stats[tid].next_packet_id
                        : 0.0);
        }

        if (dpi_instance_in->dpi_object_stats[tid].gtpu_pkts > 0){
        fprintf(fp,
                "GTP-U Packets @dpi_thread %02u %14llu\n",
                tid,
                dpi_instance_in->dpi_object_stats[tid].gtpu_pkts); 
        }
        fprintf(fp, "Protocol\t-------------------------------\n");
        for (u64 i = 0; i < pace2_get_number_of_protocols(dpi_instance_in->pace2[tid], 0) &&
                        i < PACE2_PROTOCOL_COUNT + DPI_DU_NEW_DETECTION_RESERVE;
             i++) {
            if (dpi_instance_in->dpi_object_stats[tid].protocol_counter[i] > 0) {
                char const * const protocol_str = pace2_get_protocol_short_string(dpi_instance_in->pace2[tid], 0, i);
                fprintf(fp,
                        "Prtcl_L7:%-22s|#%19llu\n",
                        protocol_str,
                        dpi_instance_in->dpi_object_stats[tid].protocol_counter[i]);
            }
        }
        for (u64 i = 0; i < pace2_get_number_of_protocol_attributes(dpi_instance_in->pace2[tid], 0) &&
                        i < PACE2_PROTOCOL_ATTRIBUTES_COUNT + DPI_DU_NEW_DETECTION_RESERVE;
             i++) {
            if (dpi_instance_in->dpi_object_stats[tid].protocol_attribute_counter[i] > 0) {
                char const * const attr_str = pace2_get_protocol_attribute_string(dpi_instance_in->pace2[tid], 0, i);
                fprintf(fp,
                        "Attrib:%-24s|#%19llu\n",
                        attr_str,
                        dpi_instance_in->dpi_object_stats[tid].protocol_attribute_counter[i]);
            }
        }
        fprintf(fp, "Applications\t-------------------------------\n");
        for (u64 i = 0; i < pace2_get_number_of_applications(dpi_instance_in->pace2[tid], 0) &&
                        i < PACE2_APPLICATIONS_COUNT + DPI_DU_NEW_DETECTION_RESERVE;
             i++) {
            if (dpi_instance_in->dpi_object_stats[tid].application_counter[i] > 0) {
                char const * const app_str = pace2_get_application_short_string(dpi_instance_in->pace2[tid], 0, i);
                fprintf(fp,
                        "App_L8:%-24s|#%19llu\n",
                        app_str,
                        dpi_instance_in->dpi_object_stats[tid].application_counter[i]);
            }
        }
        for (u64 i = 0; i < pace2_get_number_of_application_attributes(dpi_instance_in->pace2[tid], 0) &&
                        i < PACE2_APPLICATION_ATTRIBUTES_COUNT + DPI_DU_NEW_DETECTION_RESERVE;
             i++) {
            if (dpi_instance_in->dpi_object_stats[tid].application_attribute_counter[i] > 0) {
                char const * const attr_str = pace2_get_application_attribute_string(dpi_instance_in->pace2[tid], 0, i);
                fprintf(fp,
                        "Attrib:%-24s|#%19llu\n",
                        attr_str,
                        dpi_instance_in->dpi_object_stats[tid].application_attribute_counter[i]);
            }
        }
#if CPI_ENABLED >= 1 && DPI_ENABLE_FLOW_TRACKING > 0
        fprintf(fp, "\nFlow PHT summary [Size: %llu MiB]\t ------------\n", 
                       dpi_instance_in->dpi_opts.flow_table.max_entries*dpi_instance_in->flow_entry_size/1024/1024);
        fprintf(fp, "Flow table current active flows: %u\n", pace2_pht_used_elements(dpi_instance_in->flow_pht[tid]));
        fprintf(fp,
                "Flow table max active flows: %u\n",
                pace2_pht_maximum_number_of_used_elements(dpi_instance_in->flow_pht[tid]));
        fprintf(fp,
                "Flow table max configured elements: %u\n",
                pace2_pht_number_of_elements(dpi_instance_in->flow_pht[tid]));
        fprintf(fp, "Flow entries expired elements: %llu\n", dpi_instance_in->dpi_object_stats[tid].flows_expired_cnt);
        fprintf(fp, "DNS excluded flow cnt: %llu\n", dpi_instance_in->dpi_object_stats[tid].dns_flow_fp);
#endif
        if (dpi_instance_in->dpi_opts.subscriber_table.enabled){
            fprintf(fp, "Subscriber PHT summary [Size:%llu MiB]\t --------------\n",
                       dpi_instance_in->dpi_opts.subscriber_table.max_entries*dpi_instance_in->subscr_entry_size/1024/1024);
            fprintf(fp,
                    "Subscriber table current active IPs: %u\n",
                    pace2_pht_used_elements(dpi_instance_in->subscr_pht[tid]));
            fprintf(fp,
                    "Subscriber table max active IPs: %u\n",
                    pace2_pht_maximum_number_of_used_elements(dpi_instance_in->subscr_pht[tid]));
            fprintf(fp,
                    "Subscriber table max configured IPs: %u\n",
                    pace2_pht_number_of_elements(dpi_instance_in->subscr_pht[tid]));
        }
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
        fprintf(fp, "\n[FDEP] plugin summary\t-------------------------------\n");
        fprintf(fp,
                "MTUs exported:\t%lu\t=> %llu flows\n",
                flow_mtus_exported,
                dpi_instance_in->dpi_object_stats[tid].flows_expired_cnt);
        fprintf(fp, "templates exported:\t%lu\n", fdep_templates_exported);
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
        fprintf(fp, "\n[CASC] plugin summary\t-------------------------------\n");
        for (u32 i = 0; i < CASC_MAX_USER_ENTRIES; i++) {
            if (dpi_instance_in->dpi_object_stats[tid].casc_flows_matched_per_id[i] > 0) {
                fprintf(fp,
                        "[CASC]->id[%03u] flows:\t%llu\n",
                        i,
                        dpi_instance_in->dpi_object_stats[tid].casc_flows_matched_per_id[i]);
            }
        }
#endif
#if DPI_ENABLE_METADATA_EXTRACTION > 0
        fprintf(fp, "Metadata\t-------------------------------\n");
        for (u32 i = 0; i < PACE2_NUMBER_OF_EVENTS; i++){
            u64 const metadata_cnt = dpi_instance_in->dpi_object_stats[tid].metadata_counter[i];
            if (metadata_cnt > 0){
                fprintf(fp,
                        "%s:%-24s|#%19llu\n",
                        "      ",
                        util_metadata_translate_enum((PACE2_event_type)i),
                        metadata_cnt);
            }
        }
#endif
        fprintf(fp, "\nStages return codes: \t-------------------------------\n");
        fprintf(fp,
                "error_cnt: S1:%llu,s2:%llu,s3:%llu,s5:%llu,demo_limit:%llu\n",
                dpi_instance_in->dpi_object_stats[tid].s1_error,
                dpi_instance_in->dpi_object_stats[tid].s2_error,
                dpi_instance_in->dpi_object_stats[tid].s3_error,
                dpi_instance_in->dpi_object_stats[tid].s5_error,
                dpi_instance_in->dpi_object_stats[tid].license_exceeded_cnt);
        fprintf(fp, "============================================================\n");
    }
}

#if CPI_ENABLED >= 1

static void * get_subscriber(PACE2_packet_descriptor * const pd,
                                   u8 const use_dst,
                                   u64 const entry_size,
                                   struct pace2_pht * subscr_pht)
{
    PACE2_subscriber_key key;
    void * subscr;
    u8 new_subscr = 0;
    pace2_build_subscriber_key(pd, &key, pd->framing->inner_ip_index, use_dst);

    subscr = pace2_pht_insert(subscr_pht, (u8 *)&key.buffer[0], &new_subscr);

    if (subscr != NULL && new_subscr != 0) {
        memset(subscr, 0, entry_size);
    }

    return subscr;
} /* get_subscriber */


#if DPI_ENABLE_FLOW_TRACKING > 0
static void * get_flow(PACE2_packet_descriptor * const pd, u64 const entry_size, struct pace2_pht * flow_pht)
{
    ipoque_unique_flow_ipv4_and_6_struct_t key;

    if (pace2_build_flow_key(pd, &key, NULL, pd->framing->inner_ip_index) == PACE2_SUCCESS) {
        u8 new_flow;
        void * const flow = pace2_pht_insert(flow_pht, (u8 *)&key, &new_flow);
        
        if (flow != NULL && new_flow != 0) {
            /* Initialize flow memory */
            memset(flow, 0, entry_size);
        }

        return flow;
    }

    return NULL;
} /* get_flow */
#endif
#endif

/* Process all PACE 2 events currently in the event queue */
static void process_events(PACE2_module * const pace2, u8 tid)
{
    PACE2_event * event;

    while ((event = pace2_get_next_event(pace2, tid))) {
        /* some additional processing is necessary */
    }
} /* process_events */

static inline u8 int_pd_is_dns(PACE2_packet_descriptor * pd)
{
    u16 const dns_port = htons(53);
    if (pd != NULL && (int)pd->framing->stack_size > (pd->framing->inner_ip_index + 1) &&
        pd->framing->stack[pd->framing->inner_ip_index + 1].type == UDP) {
        const struct udphdr * const udp_p = pd->framing->stack[pd->framing->inner_ip_index + 1].frame_data.udp;
        if (udp_p != NULL && (udp_p->dest == dns_port || udp_p->source == dns_port)) {
            // match
            return 1;
        }
    }
    return 0;
}

static inline u8 int_is_control_flow(dpi_instance * const dpi_instance_in,
                                     u8 const tid,
                                     PACE2_classification_result_event const * const classification)
{
    if (dpi_instance_in == NULL || classification == NULL) {
        return 0;
    }
    UNUSED(tid);
    for (u8 i = 0; i < classification->application.attributes.length; i++) {
        if (classification->application.attributes.list[i] == PACE2_APPLICATION_ATTRIBUTE_CONTROL_FLOW){
            return 1;
        }
    }
    return 0;
}

/* generic accounting of classification results. If dpi_result_t given, translates PACE2 result into a generic DPI-result.
*/
static inline int int_handle_classification_result(dpi_instance * const dpi_instance_in,
                                                   u8 const tid,
                                                   PACE2_classification_result_event const * const classification,
                                                   dpi_result_t * dpi_result)
{
    if (dpi_instance_in == NULL || classification == NULL) {
        return -1;
    }
    /*update L7 protocol, OTT application and attribute counters*/
    if (classification->protocol.stack.length > PACE2_RESULT_PROTOCOL_LAYER_7) {
        u64 const protocol_id = classification->protocol.stack.entry[PACE2_RESULT_PROTOCOL_LAYER_7];
        dpi_instance_in->dpi_object_stats[tid].protocol_counter[protocol_id]++;
        if (dpi_result){
            dpi_result->protocol_l7 = protocol_id;
            dpi_result->protocol_l4 = classification->protocol.stack.entry[PACE2_RESULT_PROTOCOL_LAYER_4];
            dpi_result->protocol_l3 = classification->protocol.stack.entry[PACE2_RESULT_PROTOCOL_LAYER_3];
        }
    }
    for (u8 i = 0; i < classification->application.attributes.length; i++) {
        dpi_instance_in->dpi_object_stats[tid]
            .application_attribute_counter[classification->application.attributes.list[i]]++;
        if (dpi_result){
            dpi_result->application_attributes[i] = classification->application.attributes.list[i];
        }
    }
    for (u8 i = 0; i < classification->protocol.attributes.length; i++) {
        dpi_instance_in->dpi_object_stats[tid].protocol_attribute_counter[classification->protocol.attributes.list[i]]++;
        if (dpi_result){
            dpi_result->protocol_attributes[i] = classification->protocol.attributes.list[i];
        }
    }
    dpi_instance_in->dpi_object_stats[tid].application_counter[classification->application.type]++;
    if (dpi_result){
        dpi_result->application = classification->application.type;
        // set control flow
        dpi_result->is_control_flow = int_is_control_flow(dpi_instance_in, tid, classification);
    }

    return 0;
}

static void handle_additional_pace2_events(dpi_instance * const dpi_instance_in,
                                           u8 const tid,
                                           PACE2_event const * const event)
{
    if (event == NULL || dpi_instance_in == NULL) {
        return;
    }

#if DPI_ENABLE_METADATA_EXTRACTION > 0
    // account events
    dpi_instance_in->dpi_object_stats[tid].metadata_counter[event->header.type]++;
#endif

    switch (event->header.type) {
        case PACE2_PLUGIN_EVENT:
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
        {
            if (event->plugin_event.plugin_id.full_id != PLUGIN_CASC) {
                break;
            }
            /* consume custom classifier results */
            struct plugin_csc_event const * const csc_result =
                (struct plugin_csc_event const *)event->plugin_event.plugin_data;
            if (csc_result != NULL) {
                /* account matched flows per id */
                /* check plugin integration example for more metadata fields, e.g. descriptions etc. */
                dpi_instance_in->dpi_object_stats[tid]
                    .casc_flows_matched_per_id[csc_result->event_data.classification_event.id]++;
            }
        }
#endif
        break;
        // ... add your code here: implement more event consumer depending on metadata required
        case PACE2_BASIC_SSL_CLIENT_HELLO_EVENT:
            break;
        case PACE2_BASIC_DISSECTOR_METADATA_EVENT:
            break;
        case PACE2_BASIC_HTTP_PARSED_REQUEST_EVENT:
            break;
        case PACE2_LICENSE_EXCEEDED_EVENT:
            // demo or license limit reached
            dpi_instance_in->dpi_object_stats[tid].license_exceeded_cnt++;
            break;

        default:
            break;
    }
}

static void stage3_to_5(dpi_instance * const dpi_instance_in, u8 const tid, PACE2_packet_descriptor * cpi_pd, u8 const skip_s3_sp, dpi_result_t * dpi_result)
{
#if CPI_ENABLED == 0
    (void)(cpi_pd);
    const PACE2_event * event;
    PACE2_bitmask pace2_event_mask;
    PACE2_packet_descriptor * out_pd;
    PACE2_module * const pace2 = dpi_instance_in->pace2[tid];

    /* Process stage 3 and 4 as long as packets are available from stage 2 */
    while ((out_pd = pace2_s2_get_next_packet(pace2, 0))) {
        /* Process stage 3: packet classification */
        if (pace2_s3_process_packet(pace2, 0, out_pd, &pace2_event_mask) != PACE2_S3_SUCCESS) {
            dpi_instance_in->dpi_object_stats[tid].s3_error++;
            continue;
        } /* Stage 3 processing */

        /* Get all thrown events of stage 3 */
        while ((event = pace2_get_next_event(pace2, 0))) {
            /* some additional processing is necessary */
            if (event->header.type == PACE2_CLASSIFICATION_RESULT) {
                PACE2_classification_result_event const * const classification = &event->classification_result_data;
                int_handle_classification_result(dpi_instance_in, tid, classification, dpi_result);
            } else {
                handle_additional_pace2_events(dpi_instance_in, tid, event);
            }
        } /* end of Stage 3 event processing */

    } /* Stage 2 packets */

    /* Process stage 5: timeout handling */
    if (pace2_s5_handle_timeout(pace2, 0, &pace2_event_mask) != 0) {
        dpi_instance_in->dpi_object_stats[tid].s5_error++;
        return;
    }

    /* Print out other events generated while cleaning up flows that timed out */
    process_events(pace2, 0);

#elif CPI_ENABLED >= 1
    const PACE2_event * event;
    PACE2_bitmask pace2_event_mask;
    PACE2_module * const pace2 = dpi_instance_in->pace2[tid];

#if DPI_ENABLE_FLOW_TRACKING > 0 
    if (dpi_instance_in->dpi_opts.dpi.flow_offload > 0){
        // flow offloading path. If flows are released via `pace2_release_flow()` cpi_pd == NULL and reach stage5()
        flow_user_data_t * const f_usr_data = (cpi_pd == NULL || skip_s3_sp) ? NULL : (flow_user_data_t *)cpi_pd->flow_user_data;
        if (f_usr_data != NULL) {
            f_usr_data->flow_pkts++;
            // offloading condition: exclude flows exceeding pkt limit or app detected w/o control-flow
            u8 const is_control_flow = int_is_control_flow(dpi_instance_in, tid, &f_usr_data->result);
            if ((f_usr_data->flow_pkts >= dpi_instance_in->dpi_opts.dpi.flow_offload && !is_control_flow) ||
                (f_usr_data->result.application.type != PACE2_APPLICATION_UNKNOWN &&
                f_usr_data->result.protocol.stack.entry[PACE2_RESULT_PROTOCOL_LAYER_7] > PACE2_PROTOCOL_UNKNOWN &&
                !is_control_flow)) {
                // once offloaded user TODO: add your NIC/ ASIC offload APIs here
                // currently we keep accounting the classification_stats derived from flow_user_data and return
                int_handle_classification_result(dpi_instance_in, tid, &f_usr_data->result, dpi_result);
                dpi_instance_in->dpi_object_stats[tid].pkts_offloaded++;
                if (dpi_result){
                    dpi_result->classification_finished_or_offloaded  = 1;
                }
                return;
            }
        }
    }

#endif
    // Stage3->5 full detection path below:
    if (cpi_pd != NULL) {
        u8 continue_processing = 1;
        flow_user_data_t * const this_flow_usr_data = (flow_user_data_t *)cpi_pd->flow_user_data;

        switch (pace2_s3_process_packet(pace2, 0, cpi_pd, &pace2_event_mask)) {
            case PACE2_S3_REQUIRE_SUBSCRIBER:
                /* Do subscriber lookup for slowpath. Skip lookup if skip_s3_sp is set */
                if (!skip_s3_sp && dpi_instance_in->dpi_opts.subscriber_table.enabled && this_flow_usr_data != NULL){

                    /* Track only the client/flow-initiating side for higher performance. See logic post pace2_cpi_flow_update2()*/
                    if (this_flow_usr_data->initial_packet_direction == cpi_pd->direction){
                        cpi_pd->src_stage3 =
                            get_subscriber(cpi_pd, 0, dpi_instance_in->subscr_entry_size, dpi_instance_in->subscr_pht[tid]);
                    } else{
                        cpi_pd->dst_stage3 =
                            get_subscriber(cpi_pd, 1, dpi_instance_in->subscr_entry_size, dpi_instance_in->subscr_pht[tid]);
                    }
                }

                /* Process stage 3: packet classification */
                if (pace2_s3_process_packet(pace2, 0, cpi_pd, &pace2_event_mask) != PACE2_S3_SUCCESS) {
                    // error S3 subscriber lookup
                    dpi_instance_in->dpi_object_stats[tid].s3_error++;
                    break;
                }

                break;
            case PACE2_S3_SUCCESS:
                break;

            default:
                continue_processing = 0;
                break;
        } /* Stage 3 */

        if (continue_processing) {
            /* Get all thrown events of stage 3 */
            while ((event = pace2_get_next_event(pace2, 0))) {
                /* some additional processing is necessary */
                if (event->header.type == PACE2_CLASSIFICATION_RESULT) {
                    PACE2_classification_result_event const * const classification = &event->classification_result_data;
                    if (!skip_s3_sp && DPI_ENABLE_FLOW_TRACKING && 
                        (dpi_instance_in->config.s3_classification.classification_mode !=
                         PACE2_DEFAULT_EVERY_PACKET_MODE || dpi_instance_in->dpi_opts.dpi.flow_offload > 0)) {
                        // cpi uses PACE2_ON_CLASSIFICATION_CHANGE_MODE, hence we need to store the classification
                        // result on the flow
                        memcpy(event->header.flow_user_data, classification, sizeof(PACE2_classification_result_event));
                    } else {
                        /*update L7 protocol and OTT application counters*/
                        int_handle_classification_result(dpi_instance_in, tid, classification, dpi_result);
                    }
                } else {
                    handle_additional_pace2_events(dpi_instance_in, tid, event);
                }
            } /* Stage 3 event processing */
        }

#if DPI_ENABLE_FLOW_TRACKING > 0
        if (!skip_s3_sp && (dpi_instance_in->dpi_opts.dpi.flow_offload > 0 || 
                dpi_instance_in->config.s3_classification.classification_mode != PACE2_DEFAULT_EVERY_PACKET_MODE)){
            
            if (this_flow_usr_data != NULL) {
                int_handle_classification_result(dpi_instance_in, tid, &this_flow_usr_data->result, dpi_result);
            }
        }
#endif

    } /* Stage 2 packet is processed */

    /* Process stage 5: timeout handling */
    if (pace2_s5_handle_timeout(pace2, 0, &pace2_event_mask) != 0) {
        dpi_instance_in->dpi_object_stats[tid].s5_error++;
        return;
    }
    /* Print out other events generated while cleaning up flows that timed out */
    process_events(pace2, 0);
#endif
} /* stage3_to_5 */

dpi_return_t dpi_destroy_instance(dpi_instance ** const dpi_instance_in, u8 const dpi_worker_count)
{
    if (dpi_instance_in == NULL || (*dpi_instance_in)->pace2 == NULL) {
        return DPI_RETURN_FAILURE;
    }
    PACE2_module * pace2 = NULL;
    // Print stats before exit of PACE2 module
    dpi_print_stats(stderr, *dpi_instance_in);

#if PERFMON > 0
    for (u8 i = 0; i < dpi_worker_count; i++) {
/**
 * @brief per example output before hugepage
 * 
[PERF] cycles_total  9818928185
 cycles/pkt 348.11


        [PERF] flow cycles 4552373646
         cycles/pkt 161.40

        [PERF] dpi vPACE cycles 5266554539
         cycles/pkt 186.72
 */

 /* after hugepage
 [PERF] cycles_total  9551155521
 cycles/pkt 344.82


        [PERF] flow cycles 4337354274
         cycles/pkt 156.59

        [PERF] dpi vPACE cycles 5213801247
         cycles/pkt 188.23
 */
        if ( (*dpi_instance_in)->dpi_object_stats[i].next_packet_id  > 0 ){
            (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_total = (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_flow_lookup +
                (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_dpi ;
            fprintf(stderr, "[PERF] cycles_total  %lu\n cycles/pkt %.2f\n\n\n",
                            (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_total,
                            1.0 *(*dpi_instance_in)->dpi_object_stats[i].perf.cycles_total / (*dpi_instance_in)->dpi_object_stats[i].next_packet_id );
            fprintf(stderr, "\t[PERF] flow cycles %lu\n\t cycles/pkt %.2f\n\n",
                            (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_flow_lookup,
                            1.0 *(*dpi_instance_in)->dpi_object_stats[i].perf.cycles_flow_lookup / (*dpi_instance_in)->dpi_object_stats[i].next_packet_id );
            fprintf(stderr, "\t[PERF] dpi PACE2 cycles %lu\n\t cycles/pkt %.2f\n\n",
                            (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_dpi,
                            1.0 * (*dpi_instance_in)->dpi_object_stats[i].perf.cycles_dpi / (*dpi_instance_in)->dpi_object_stats[i].next_packet_id );
        }
    }
#endif // PERFMON
#if CPI_ENABLED >= 1
    for (u8 i = 0; i < dpi_worker_count; i++) {
        pace2 = (*dpi_instance_in)->pace2[i];
        /* Clear the flow hash table and handle each removed flow. */
#if DPI_ENABLE_FLOW_TRACKING > 0
        pace2_pht_clear((*dpi_instance_in)->flow_pht[i]);
        {
            void * p;

            while ((p = pace2_pht_get_next_element_to_remove((*dpi_instance_in)->flow_pht[i], NULL, NULL))) {
                pace2_release_flow(pace2, 0, p);
                stage3_to_5(*dpi_instance_in, i, NULL, 1, NULL);
                (*dpi_instance_in)->dpi_object_stats[i].flows_expired_cnt++;
            }
        }
        pace2_pht_destroy((*dpi_instance_in)->flow_pht[i]);
#endif
        if ((*dpi_instance_in)->dpi_opts.subscriber_table.enabled){
            /* Clear the subscriber hash table */
            pace2_pht_clear((*dpi_instance_in)->subscr_pht[i]);
            {
                void * p;

                while ((p = pace2_pht_get_next_element_to_remove((*dpi_instance_in)->subscr_pht[i], NULL, NULL))) {
                }
            }
            /* Destroy the hash tables */
            pace2_pht_destroy((*dpi_instance_in)->subscr_pht[i]);
        }

    }

#elif CPI_ENABLED == 0
    for (uint8_t i = 0; i < dpi_worker_count && i < DPI_MAX_INSTANCE_COUNT; i++) {
        /* Flush any remaining packets from the buffers */
        pace2 = (*dpi_instance_in)->pace2[i];
        pace2_flush_engine(pace2, i);

        /* Process packets which are ejected after flushing */
        stage3_to_5(*dpi_instance_in, i, NULL, 0, NULL);

        /* Output detection results */
        // pace_print_results();
            /* Destroy PACE 2 module and free memory */
        pace2_exit_module(pace2);
    }
#endif

    if (DPI_ENABLE_FLOW_TRACKING == 0 || (*dpi_instance_in)->dpi_opts.flow_table.skip_dns_tracking > 0){
        /* free flow scratchpad */
        for (u8 i = 0; i < dpi_worker_count; i++) {
            free((*dpi_instance_in)->flow_data[i]);
        }
    }

    free(*dpi_instance_in);
    return DPI_RETURN_SUCCESS;
}

dpi_return_t dpi_process_packet(
    dpi_instance * const dpi_instance_in, u8 const tid, u64 const time, void const * const iph, u16 const ipsize, dpi_result_t * dpi_result)
{
#if CPI_ENABLED == 0
    PACE2_packet_descriptor pd;
    PACE2_module * const pace2 = dpi_instance_in->pace2[tid];

    /* Stage 1: Prepare packet descriptor and run ip defragmentation */
    PACE2_s1_return_state ret_s1 = pace2_s1_process_packet(pace2, 0, time, iph, ipsize, PACE2_S1_L3, &pd, NULL, 0);
    if (ret_s1 != PACE2_S1_SUCCESS && ret_s1 != PACE2_S1_TRUNCATED_PACKET) {
        dpi_instance_in->dpi_object_stats[tid].s1_error++;
        return DPI_RETURN_FAILURE;
    }

    /* Set unique packet id. The flow_id is set by the internal flow tracking */
    pd.packet_id = ++(dpi_instance_in->dpi_object_stats[tid].next_packet_id);

    /* Stage 2: Packet reordering */
    if (pace2_s2_process_packet(pace2, 0, &pd) != PACE2_S2_SUCCESS) {
        dpi_instance_in->dpi_object_stats[tid].s2_error++;
        return DPI_RETURN_FAILURE;
    }

    stage3_to_5(dpi_instance_in, tid, NULL, 0, dpi_result) ;

    return DPI_RETURN_SUCCESS;

#elif CPI_ENABLED >= 1

    /* Pointer to a packet descriptor structure. The difference with the packet descriptor
     *  used in stage 1 is that here the pointer will be filled internally and points to internal
     *  structure afterwards. However, it takes the reference to an actual packet
     *  descriptor instantiation in the stage 1.*/
#if PERFMON > 0
    u64 start = 0;
    u64 end = 0;
#endif // PERFMON
#if PERFMON > 0
    start = meter();
#endif // PERFMON
    PACE2_packet_descriptor * pd;
    PACE2_module * const pace2 = dpi_instance_in->pace2[tid];
    PACE2_s1_return_state ret_s1 = 0;
    ret_s1 = pace2_cpi_get_packet_descriptor(pace2, 0, time, iph, ipsize, &pd);

    // only allow valid frames incl. IPv4 or IPv6 header or truncated packets.
    // Running Stage3 on PACE2_S1_TRUNCATED_PACKET relies on valid length provided by user.
    if (ret_s1 != PACE2_S1_SUCCESS && ret_s1 != PACE2_S1_TRUNCATED_PACKET) {
        dpi_instance_in->dpi_object_stats[tid].s1_error++;
        return DPI_RETURN_FAILURE;
    }

#if DPI_ENABLE_FLOW_TRACKING > 0
    pace2_pht_set_timestamp(dpi_instance_in->flow_pht[tid], PACE2_PHT_TIME_TO_MILLI(time, DPI_CLOCK_TICKS_PER_SEC));
#endif
    if (dpi_instance_in->dpi_opts.subscriber_table.enabled){
        pace2_pht_set_timestamp(dpi_instance_in->subscr_pht[tid], PACE2_PHT_TIME_TO_MILLI(time, DPI_CLOCK_TICKS_PER_SEC));
    }
   
    /* Set unique packet id. The flow_id is set by pace2_cpi_flow_update2() */
    /* Set unique packet id. The flow_id is set by the internal flow tracking */
    pd->packet_id = ++(dpi_instance_in->dpi_object_stats[tid].next_packet_id);
    u8 is_dns = 0;
#if DPI_ENABLE_FLOW_TRACKING > 0
    if (dpi_instance_in->dpi_opts.flow_table.skip_dns_tracking != 0 && int_pd_is_dns(pd)){
        /* DNS traffic does not require full flow tracking */
        is_dns = 1;
        memset(dpi_instance_in->flow_data[tid], 0, dpi_instance_in->flow_entry_size);
        pd->flow_data = dpi_instance_in->flow_data[tid];
        dpi_instance_in->dpi_object_stats[tid].dns_flow_fp++;
    } else{
        /* regular path for non-DNS traffic */
        pd->flow_data = get_flow(pd, dpi_instance_in->flow_entry_size, dpi_instance_in->flow_pht[tid]);
    }
#else
    /* mock behavior of flow PHT + get_flow via memset()-ing the scratchpad.*/
    memset(dpi_instance_in->flow_data[tid], 0, dpi_instance_in->flow_entry_size);
    pd->flow_data = dpi_instance_in->flow_data[tid];
#endif

    if (pd->flow_data == NULL) {
        dpi_instance_in->dpi_object_stats[tid].s1_error++;
        return DPI_RETURN_FAILURE;
    }

    if (pace2_cpi_flow_update2(pace2, 0, pd) != PACE2_SUCCESS) {
        dpi_instance_in->dpi_object_stats[tid].s1_error++;
        return DPI_RETURN_FAILURE;
    }
    /* update direction to flow_user_data for more efficient subscriber lookup. 
       Flow_user_data is only available after pace2_dpi_flow_update2() */
    if (pd->new_flow == 1){
        flow_user_data_t * const f_usr_data = (flow_user_data_t *)pd->flow_user_data;
        if (f_usr_data != NULL){
            f_usr_data->initial_packet_direction = pd->direction;
        }
    }

#if PERFMON > 0
    end = meter();
    dpi_instance_in->dpi_object_stats[tid].perf.cycles_flow_lookup += end - start;
    start = meter();
#endif // PERFMON
    stage3_to_5(dpi_instance_in, tid, pd, is_dns, dpi_result);
#if PERFMON > 0
    end = meter();
    dpi_instance_in->dpi_object_stats[tid].perf.cycles_dpi += end -start;
#endif // PERFMON
#if DPI_ENABLE_FLOW_TRACKING == 0
    /* instantly release flow scratchpad instead of waiting for timeout queue from PHT */
    pace2_release_flow(pace2, 0, dpi_instance_in->flow_data[tid]);
    // stage3_to_5 call with NULL, to generate flow_dropped_event and FDEP event via Stage5()
    stage3_to_5(dpi_instance_in, tid, NULL, 1, NULL);
    dpi_instance_in->dpi_object_stats[tid].flows_expired_cnt++;
#else
    if (is_dns){
        // DNS uses scratchpad, so no need to reserve_elements in PHT
        return DPI_RETURN_SUCCESS;
    }
    /* Reserve flow elements for the next insert */
    pace2_pht_reserve_elements(dpi_instance_in->flow_pht[tid], 1);
    {
        void * p;

        /* age out stale flows. Use last arg to get removal reason. */
        while ((p = pace2_pht_get_next_element_to_remove(dpi_instance_in->flow_pht[tid], NULL, NULL))) {
            pace2_release_flow(pace2, 0, p);
            stage3_to_5(dpi_instance_in, tid, NULL, 1, NULL);
            dpi_instance_in->dpi_object_stats[tid].flows_expired_cnt++;
        }
    }
#endif // DPI_ENABLE_FLOW_TRACKING

    if (dpi_instance_in->dpi_opts.subscriber_table.enabled){
        /* Reserve subscriber elements for the next insert */
        pace2_pht_reserve_elements(dpi_instance_in->subscr_pht[tid], 2);
        {
            void * p;

            while ((p = pace2_pht_get_next_element_to_remove(dpi_instance_in->subscr_pht[tid], NULL, NULL))) {
            }
        }
    }
    return DPI_RETURN_SUCCESS;
#endif // CPI_ENABLED
}

#if DPI_GTPU_ENABLED > 0
/**
 * @brief Decapsulation of GTP-U traffic. Currently supports incoming GTP-U IPv4 traffic with extension headers
 * 
 * @param[in,out]   hdr               Double pointer to a IPv4 hdr before GTP-U
 * @param[in]       hdr_len           Expected length
 * @param[in]       flags             Optional flag
 * @param[out]      out_tunnel_offset Calculated offset to the innermost IPv4/IPv6 header.
 * @param[out]      out_teid          Optional output of u32 TEID which can be later correlated to flow.
 * @return int      Negative values on error. 0 on success
 */
static int get_gtp_tunnel(u8 const ** hdr, u16 * hdr_len, u32 flags, u16 * out_tunnel_offset, u32 * out_teid)
{
    if (*hdr == NULL || hdr_len == NULL){
        return -1;
    }
    struct iphdr const * ip = (struct iphdr const *)*hdr;

    if (*hdr_len < sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct gtp_v1_header) ||
        *hdr[0] != 0x45 ||
        ip->protocol != IPPROTO_UDP ) {
        return -1;
    }
    const struct udphdr * udp = (struct udphdr const *)((u8 const *)*hdr + sizeof(struct iphdr));
    u16 offset = 0;
    UNUSED(flags);
    /* gtp decapsulation in UDP just GTPv1 handling is required since there is no GTPv2-U */
    if (*hdr_len >= sizeof(struct udphdr) + 8 && (udp->source == htons(GTP_U_UDP_PORT) || udp->dest == htons(GTP_U_UDP_PORT))) {

        struct gtp_v1_header const * gtp = (struct gtp_v1_header const *)(((u8 const *)udp) + 8);

        if (gtp->version == 0x01 && gtp->protocol_type == 1 && gtp->reserved == 0 &&
            gtp->message_type == GTP_MESSAGE_TYPE_TPDU) {

            offset = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct gtp_v1_header);

            if (gtp->seq_num_present == 1 || gtp->next_extension_header_present == 1 || gtp->n_pdu_present == 1) {
                offset += 4;
            }

            if (gtp->next_extension_header_present == 1) {

                u32 gtp_neh_offset = offset + 4;

                if (gtp->seq_num_present != 0 || gtp->next_extension_header_present != 0) {
                    offset = 4;
                }

                if (*hdr_len >= gtp_neh_offset + 1 && sizeof(struct udphdr) >= gtp_neh_offset) {

                    u8 i;
                    for (i = 0; i <= GTP_MAX_SUPPORTED_EXTENSION_HEADERS; i++) {

                        const u16 gtp_neh_len = ((u8 const *)udp)[gtp_neh_offset] * 4;
                        const u8 * gtp_neh_content = ((u8 const *)udp) + gtp_neh_offset + 1;

                        if (gtp_neh_len < 2) {
                            *out_tunnel_offset = 0;
                            // handle invalid extensions as error
                            return -1;
                        }

                        struct gtp_next_extension_header const gtp_next_extension_header = {
                            gtp_neh_len, gtp_neh_content, gtp_neh_content + gtp_neh_len - 2};

                        if (gtp_neh_len != 0 && gtp_next_extension_header.len + gtp_neh_offset < *hdr_len) {

                            offset += gtp_next_extension_header.len;
                            gtp_neh_offset += gtp_next_extension_header.len;
                        } else {
                            *out_tunnel_offset = 0;
                            // handle invalid extensions as error
                            return -1;
                        }

                        if (gtp_next_extension_header.next_extension_header[0] == 0x00) {
                            break;
                        }
                    }
                }
            }

            if (offset < *hdr_len) {

                *hdr = ((u8 const *)udp) + offset;
                *hdr_len = *hdr_len - offset;
                if (out_teid != NULL) {
                    // extract TEID if pointer is provided
                    *out_teid = ntohl(gtp->tunnel_endpoint_id);
                }
                *out_tunnel_offset = offset;
                return 0;
            }
        }
        *out_tunnel_offset = 0;
        return -3;
    }
    *out_tunnel_offset = 0;
    return -2;
}
#endif

dpi_return_t dpi_process_l2_frame(dpi_instance * const dpi_instance_in,
                                  u8 const tid,
                                  u64 const time,
                                  void const * const l2h,
                                  u16 const l2size,
                                  u8 const decaps_gtpu,
                                  dpi_result_t * dpi_result)
{
    if (dpi_instance_in == NULL || l2h == NULL || l2size < (14 + 20)) {
        return DPI_RETURN_INPUT_MISSING;
    }
    struct pace2_osi_layer_2_info_data l2data = {0};
    // use PACE2 provided parsing pace2_detection_get_l2_data() to strip of l2 VLAN/MPLS/MAC/ieee8021ah labels
    int return_code_decaps =
        pace2_detection_get_l2_data((u8 const *)l2h, l2size, l2size, PACE2_NEXT_L2_ETHERNET, &l2data);

    if (return_code_decaps != 0) {
        return DPI_RETURN_INVALID_PACKET;
    }

    if (decaps_gtpu) {
        struct pace2_osi_layer_2_info_data l2data_inner = {0};
#if DPI_GTPU_ENABLED > 0
        const u8 * ip = (const u8 *)l2h + l2data.offset;
        u16 length = l2data.length;
        u16 gtp_tunnel_offset = 0;
        if (0 == get_gtp_tunnel(&ip, &length, 0, &gtp_tunnel_offset, NULL)) {
            l2data_inner.offset += gtp_tunnel_offset;
            l2data_inner.length = l2size - l2data_inner.offset;
            dpi_instance_in->dpi_object_stats[tid].gtpu_pkts++;
        }
#endif
        return dpi_process_packet(dpi_instance_in,
                                  tid,
                                  time,
                                  (void const * const)((u8 const * const)l2h + l2data.offset + l2data_inner.offset),
                                  l2data.length - l2data_inner.offset,
                                  dpi_result);
    } else {
        return dpi_process_packet(
            dpi_instance_in, tid, time, (void const * const)((u8 const * const)l2h + l2data.offset), l2data.length, dpi_result);
    }
}

dpi_return_t dpi_upgrade_library(dpi_instance * const dpi_instance_in, char const * const path_to_library)
{
    if (dpi_instance_in == NULL || path_to_library == NULL || strlen(path_to_library) < strlen("*.so")) {
        return DPI_RETURN_INPUT_MISSING;
    }
    for (u8 tid = 0; tid < dpi_instance_in->dpi_worker_count; tid++){
        char v_old[64] = {0};
        char v_newer[64] = {0};
        u64 const v_old_app_count = pace2_get_number_of_applications(dpi_instance_in->pace2[tid], 0);

        dpi_get_dpi_version(dpi_instance_in, tid, v_old, sizeof(v_old) / sizeof(v_old[0]));

        PACE2_dynamic_upgrade_load_state ret =
            pace2_dynamic_upgrade_load_library(dpi_instance_in->pace2[tid], path_to_library, 1);
        if (ret != PACE2_DYNAMIC_UPGRADE_LOAD_SUCCESSFUL) {
            printf("[DU] ThreadId %u Could not load library. Error code %u\n", tid, ret);
            return DPI_RETURN_NOT_AVAILABLE;
        }
        PACE2_dynamic_upgrade_activate_state ret_enable =
            pace2_dynamic_upgrade_activate_loaded_library(dpi_instance_in->pace2[tid]);
        if (ret_enable != PACE2_DYNAMIC_UPGRADE_ACTIVATE_SUCCESSFUL) {
            printf("[DU] ThreadId %u Could not activate library. Error code %u\n", tid, ret_enable);
            return DPI_RETURN_NOT_AVAILABLE;
        }
        dpi_get_dpi_version(dpi_instance_in, tid, v_newer, sizeof(v_newer) / sizeof(v_newer[0]));
        u64 const v_new_app_count = pace2_get_number_of_applications(dpi_instance_in->pace2[tid], 0);
        printf("[DU] ThreadId %u Updated successfully from %s to %s, App_count %llu -> %llu\n",
            tid,
            v_old,
            v_newer,
            v_old_app_count,
            v_new_app_count);
    }

    return DPI_RETURN_SUCCESS;
}

void dpi_print_dpi_config(FILE * fp, dpi_instance * const dpi_instance_in)
{
    if (fp == NULL || dpi_instance_in == NULL) {
        return;
    }
    fprintf(fp, "DPI CONFIGURATION ==========================================\n");

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
    fprintf(fp, "[FDEP] PLUGIN_FILE %s\n", getenv("PACE2_PLUGIN_SO"));
    fprintf(fp, "[FDEP] IPFIX_USE_SOCKET:%u\n", IPFIX_USE_SOCKET);
    fprintf(fp, "[FDEP] IPFIX_DST_PORT:%u\n", dpi_instance_in->dpi_opts.fdep_plugin.port);
    fprintf(fp, "[FDEP] IPFIX_COLLECTOR_ADDRESS:%s\n", dpi_instance_in->dpi_opts.fdep_plugin.ip_dst);
    fprintf(fp, "[FDEP] IPFIX_TEMPLATE_INTERVAL_IN_SEC:%u\n", dpi_instance_in->dpi_opts.fdep_plugin.template_refresh_interval);
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
    fprintf(fp, "[CASC] PLUGIN_FILE %s\n", getenv("PACE2_PLUGIN_SO"));
    fprintf(fp, "[CASC] max entries:%u\n", CASC_MAX_USER_ENTRIES);
    fprintf(fp, "[CASC] JSON SIGNATURES %s\n", getenv("CUSTOM_SIGNATURES_FILE"));
#endif
    fprintf(fp, "[DPI] CPI_ENABLED:%u\n", CPI_ENABLED);
    fprintf(fp, "[DPI] DPI_DECA_MAX_ENTRIES:%u\n", dpi_instance_in->dpi_opts.dpi.deca_max_entries);
    fprintf(fp, "[DPI] DPI_CLOCK_TICKS_PER_SEC:%u\n", dpi_instance_in->dpi_opts.dpi.io_resolution);
    fprintf(fp, "[DPI] DPI_MAX_INSTANCE_COUNT:%u\n", DPI_MAX_INSTANCE_COUNT);
    fprintf(fp, "[DPI] DPI_ENABLE_FLOW_TRACKING:%u\n", DPI_ENABLE_FLOW_TRACKING);
    fprintf(fp, "[DPI] DPI_ENABLE_SUBSCRIBER_TRACKING:%u\n", dpi_instance_in->dpi_opts.subscriber_table.enabled);
    fprintf(fp, "[DPI] DPI_MAX_CONCURRENT_FLOWS:%u\n", dpi_instance_in->dpi_opts.flow_table.max_entries);
    fprintf(fp, "[DPI] DPI_MAX_CONCURRENT_SUBSCRIBERS:%u\n", dpi_instance_in->dpi_opts.subscriber_table.max_entries);
    fprintf(fp, "[DPI] PHT_SECONDS:%u\n", PHT_SECONDS);
    fprintf(fp, "[DPI] DPI_FLOW_TIMEOUT:%u\n", dpi_instance_in->dpi_opts.flow_table.timeout_in_s);
    fprintf(fp, "[DPI] DPI_FLOW_OFFLOAD:%u\n", dpi_instance_in->dpi_opts.dpi.flow_offload);
    fprintf(fp, "[DPI] DPI_SUBSCRIBER_TIMEOUT:%u\n", dpi_instance_in->dpi_opts.subscriber_table.timeout_in_s);
}

dpi_return_t dpi_get_dpi_version(dpi_instance * const dpi_instance_in,
                                 const u16 tid,
                                 char * const version,
                                 const u16 version_len)
{
    if (dpi_instance_in == NULL) {
        fprintf(stderr, "[ERROR] %s\n", "dpi_instance_in is null");
        return DPI_RETURN_INPUT_MISSING;
    }
    if (dpi_instance_in->pace2[tid] == NULL) {
        fprintf(stderr, "[ERROR] %s\n", "pace2 module is null");
        return DPI_RETURN_NOT_AVAILABLE;
    }
    if (version == NULL || version_len < 8) {
        fprintf(stderr, "[ERROR] %s\n", "version or len is missing");
        return DPI_RETURN_INPUT_MISSING;
    }
    PACE2_classification_status_event tmp_api = {0};
    if (pace2_class_get_version(dpi_instance_in->pace2[tid], 0, &tmp_api) == PACE2_CLASS_SUCCESS) {
        memset(version, 0, version_len);
        strncpy(version, tmp_api.version.version_string, version_len - 1);

        return DPI_RETURN_SUCCESS;
    } else {
        return DPI_RETURN_FAILURE;
    }
}

void dpi_print_result(FILE *fp, dpi_instance * const dpi_instance_in, const u16 tid, dpi_result_t * dpi_result)
{   
    if (fp == NULL || dpi_instance_in == NULL ||dpi_result == NULL) {
        return;
    }
    fprintf(fp, "DPI RESULT ==========================================\n");
    fprintf(fp, "Application: %u -> %s\n", dpi_result->application, pace2_get_application_short_string(dpi_instance_in->pace2[tid], 0, dpi_result->application));
    fprintf(fp, "Protocol L7: %u -> %s\n", dpi_result->protocol_l7, pace2_get_protocol_short_string(dpi_instance_in->pace2[tid], 0, dpi_result->protocol_l7));
    fprintf(fp, "Protocol L4: %u -> %s\n", dpi_result->protocol_l4, pace2_get_protocol_short_string(dpi_instance_in->pace2[tid], 0, dpi_result->protocol_l4));
    fprintf(fp, "Protocol L3: %u -> %s\n", dpi_result->protocol_l3, pace2_get_protocol_short_string(dpi_instance_in->pace2[tid], 0, dpi_result->protocol_l3));

    fprintf(fp, "Protocol Attributes: ");
    for (int i = 0; i < DPI_MAX_ATTRIBUTE_ENTRIES; i++) {
        if (dpi_result->protocol_attributes[i] != 0) {
            fprintf(fp, "%u -> %s |", dpi_result->protocol_attributes[i], pace2_get_protocol_attribute_string(dpi_instance_in->pace2[tid], 0, dpi_result->protocol_attributes[i]));
        }
    }
    fprintf(fp, "\n");

    fprintf(fp, "Application Attributes: ");
    for (int i = 0; i < DPI_MAX_ATTRIBUTE_ENTRIES; i++) {
        if (dpi_result->application_attributes[i] != 0) {
            fprintf(fp, "%u -> %s |", dpi_result->application_attributes[i], pace2_get_application_attribute_string(dpi_instance_in->pace2[tid], 0, dpi_result->application_attributes[i]));
        }
    }
    fprintf(fp, "\n");

    fprintf(fp, "Classification Finished or Offloaded: %u\n", dpi_result->classification_finished_or_offloaded);
    fprintf(fp, "Is Control Flow: %u\n", dpi_result->is_control_flow);

}

uint64_t dpi_get_timestamp(uint64_t const io_resolution)
{
    struct timespec now;
    // COARSE version is faster than `CLOCK_REALTIME` and fine for lower resolutions
    // if ns resolution required, use CLOCK_REALTIME
    if (io_resolution >= 1000000000ULL) {
        clock_gettime(CLOCK_REALTIME, &now);
    } else {
        clock_gettime(CLOCK_REALTIME_COARSE, &now);
    }

    switch (io_resolution) {
        case 1ULL:          // 1sec resolution
            return (uint64_t)now.tv_sec;
        case 1000ULL:       // 1ms resolution
            return (uint64_t)now.tv_sec * 1000ULL + now.tv_nsec / 1000000ULL;
        case 1000000ULL:    // 1us resolution
            return (uint64_t)now.tv_sec * 1000000ULL + now.tv_nsec / 1000ULL;
        case 1000000000ULL: // 1ns resolution
            return (uint64_t)now.tv_sec * 1000000000ULL + now.tv_nsec;
        default:
            break;
    }
    return 0;
}
