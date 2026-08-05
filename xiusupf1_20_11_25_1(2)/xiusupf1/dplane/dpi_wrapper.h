/**********************************************************************************************************************/
/**
 **
 ** @file       dpi_wrapper.h
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

#ifndef DPI_WRAPPER_H
#define DPI_WRAPPER_H

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

/* Includes *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <linux/udp.h>
#include <time.h>
#include <stdbool.h>


/* debug macros*/
#ifdef DEBUG_ENABLED
    #define DEBUG_PRINTF(fmt, ...) do { \
        fprintf(stderr, fmt, __VA_ARGS__); \
    }while(0)
#else
    #define DEBUG_PRINTF(fmt, ...) do {} while(0)
#endif

/* DPI return states */
typedef enum{
    DPI_RETURN_SUCCESS = 0,               /*!< Processing was successful. */
    DPI_RETURN_PARAM_MISSING,             /*!< Function parameter is missing. */
    DPI_RETURN_INPUT_MISSING,             /*!< Input data is missing. */
    DPI_RETURN_REQUIRE_SUBSCRIBER,        /*!< Subscriber data is required for further processing. */
    DPI_RETURN_INVALID_PACKET,            /*!< Given packet is invalid. */
    DPI_RETURN_OUT_OF_MEMORY,             /*!< Memory allocation failed. */
    DPI_RETURN_FAILURE,                   /*!< Processing failed. */
    DPI_RETURN_NOT_AVAILABLE,             /*!< A required feature is not available. */
    DPI_RETURN_MIDSTREAM_PACKET_SKIPPED   /*!< A packet of a midstream flow was detected */
}dpi_return_t;

/* Public Prototypes ************************************************************/
typedef struct dpi_instance_opaque dpi_instance;

#include <dlfcn.h>
#include <errno.h>

#define UNUSED (void)

/* IDs are fixed. Defined by R&S ipoque for each plugin */
#define PLUGIN_CASC  11137 // 0x2B81
#define PLUGIN_IPFIX 11138 // 0x2B82

/* PLUGIN_ID is read from Makefile -> sourced enviroment */
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
#include <plugin_flow_data_exporter.h>
    #if PLUGIN_ID != PACE2_PLUGIN_ID_FLOW_DATA_EXPORTER
        #error "invalid plugin ID. Check include headers of plugin tarball"
    #endif
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_CASC
#include <plugin_custom_service_classifier.h>
    #if PLUGIN_ID != PACE2_PLUGIN_ID_CSC
        #error "invalid plugin ID. Check include headers of plugin tarball"
    #endif
#define CASC_MAX_USER_ENTRIES (4096)
#endif

/* DECA maximal supported elements per thread. Max value: 2^32 -1. Set to 0 to disable DECA. */
#define DPI_DECA_MAX_ENTRIES (0)
#define DPI_UNCLASSIFIED_AFTER_PACKET_COUNT (25)
#define PERFMON (0)
#if PERFMON > 0

#ifdef __x86_64__

#if __GNUC__ > 4 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 8)
#define USE_BUILT_IN
#elif defined(__clang__)
#if __has_builtin(__builtin_ia32_rdtscp) && __has_builtin(__builtin_ia32_lfence) && __has_builtin(__builtin_ia32_mfence)
#define USE_BUILT_IN
#endif  // __has_builtin
#endif

static inline uint64_t meter(void)
{
#ifdef USE_BUILT_IN
    __builtin_ia32_mfence(); // ensure previous stores are done, see https://www.felixcloutier.com/x86/rdtscp
    uint32_t aux;
    uint64_t ts = __builtin_ia32_rdtscp(&aux); // rdtscp waits for previous instructions and loads, unlike rdtsc
    __builtin_ia32_lfence(); // ensure counter is read before other instructions
    return ts;
#else
    uint64_t __rax;
    asm volatile ("mfence\n\t"
                  "rdtscp\n\t"
                  "lfence\n\t"
                  "shl $0x20,%%rdx\n\t"
                  "or %%rdx,%%rax": "=a" (__rax) : : "%edx", "%ecx");
    return __rax;
#endif //   USE_BUILT_IN
}

#elif defined(NON_X86)

#define meter() meter_ns()

#else
#warning The performance test will not work correctly on 32bit x86-architectures.
static inline uint64_t meter(void)
{
    return 0;
}
#endif // __x86_64__

typedef struct {
    uint64_t cycles_flow_lookup;
    uint64_t cycles_dpi;
    uint64_t cycles_total;
} perf_t;
#endif
/* PACE2 settings */
/* cpi is the high-performance pipeline disabling Stage1,Stage2 features. See manual for further reference. */
#define CPI_ENABLED (1)
#define DPI_USE_HUGEPAGE (0) // Use rte_hugepages for DPI allocators. See @ref util_malloc, util_realloc, util_free
/* set the processing resolution: 1000 equals ms, 1e6 equals us  */
#define DPI_CLOCK_TICKS_PER_SEC (1000*1000)
/* Maximal number of PACE2 instances, u8 value. The actual value depends on the EAL CLI argument as per maximal port count
   http://doc.dpdk.org/guides/linux_gsg/quick_start.html */
#define DPI_MAX_INSTANCE_COUNT (32)

// **EXPERIMENTAL`` disables flow_tracking. Ignores other options like CONCURRENT_FLOWS or TIMEOUTs. Only valid for single sflow samples + DNS-cacher
#define DPI_ENABLE_FLOW_TRACKING (1)
// disable endpoint_tracking. May result in lower detection accuracy for flow-correlation such as control+data flow
#define DPI_ENABLE_SUBSCRIBER_TRACKING (1)

/* defines the maximal supported nr_of_elements in the hash tables. For large soak please adapt to larger values
   current requirements as of PACE2 v22.02.03 are:
        554 bytes per flow
        753 bytes per subscriber entry
   For further optimizations adapt the PACE2 config @ref dpi_init_instance()
*/
#define DPI_MAX_CONCURRENT_FLOWS        (500*1000)
#define DPI_MAX_CONCURRENT_SUBSCRIBERS  (100*1000)

/* Timestamp resolution of PHT. Do not change */
#define PHT_SECONDS (1000) //(PACE2_PHT_RESOLUTION_MILLI)

/* timeout in seconds */
#define DPI_FLOW_TIMEOUT (1 * 30)

/* timeout in seconds */
#define DPI_SUBSCRIBER_TIMEOUT (2 * 60)

/*Dynamic upgrade reserve bytes per flow/subscriber for future updates */
#define DPI_DYNAMIC_UPGRADE_BUFFER_BYTES (100)
/* added reserve members for new applications, protocols, etc */
#define DPI_DU_NEW_DETECTION_RESERVE (10000)

/*  Flow offloading threshold in max. packets per flow.
    Valid range `5..20` packets. This feature allows for a
    significant performance boost. Set to `(0)` to disable.
    Lower numbers increase performance. Higher numbers improve accuracy.
    See README section: `Compile-time configuration` for details */
#define DPI_FLOW_OFFLOAD (0)

/* Enable additional gtp-u decapsulation. If disabled `dpi_process_l2_frame` still supports VLAN/MPLS/MAC/ieee8021ah */
#define DPI_GTPU_ENABLED (1)

/* adds redzone byte fields to verify correct flow user data offsets. Disable for production */
#define DPI_DEBUG_REDZONE (0)

/* Excludes DNS traffic from flow-pht tracking. Improves performance */
#define DPI_SKIP_DNS_FLOW_TRACKING (1)

/* Enable additional metadata extraction. See @ref util_metadata_translate_enum() for supported fields */
#define DPI_ENABLE_METADATA_EXTRACTION (1)

/* compile time config checks below: */
#if DPI_FLOW_OFFLOAD > 0 && (CPI_ENABLED == 0 || DPI_ENABLE_FLOW_TRACKING == 0)
    #error "DPI_FLOW_OFFLOAD enabled without FLOW_TRACKING or CPI_ENABLED. Change to CPI_ENABLED (1)\n"
#endif
#if DPI_FLOW_OFFLOAD > 0 && DPI_FLOW_OFFLOAD < 5
    #error "DPI_FLOW_OFFLOAD enabled: Configured less than 5 packets. Recommendation is 5 or higher for better accuracy\n"
#endif
#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX && DPI_FLOW_OFFLOAD > 0
    #warning "DPI_FLOW_OFFLOAD enabled with usage of flow exporter plugin. IE1 and IE2 (octet and packet) accounting will be invalid\n"
#endif

#if DPI_ENABLE_FLOW_TRACKING == 0
    #warning "Disabled flow-tracking is EXPERIMENTAL and only allowed for sflow sample usecases with DECA\n"
#if DPI_DECA_MAX_ENTRIES == 0
    #error "Disabled flow-tracking is EXPERIMENTAL and requires DECA enabled with corresponding DNS traffic\n"
#endif
#endif

#if DPI_ENABLE_SUBSCRIBER_TRACKING == 0 && DPI_ENABLE_SUBSCRIBER_CACHING == 1
    #warning "DPI_ENABLE_SUBSCRIBER_CACHING ignored due to disabled DPI_ENABLE_SUBSCRIBER_TRACKING\n"
#endif
#if CPI_ENABLED == 0 && DPI_ENABLE_SUBSCRIBER_CACHING == 1
    #warning "DPI_ENABLE_SUBSCRIBER_CACHING ignored due to disabled CPI_ENABLED (not supported for internal tracking)\n"
#endif
#if DPI_ENABLE_SUBSCRIBER_CACHING == 1 && DPI_ENABLE_FLOW_TRACKING == 0
    #error "invalid setting DPI_ENABLE_SUBSCRIBER_CACHING == 1 && DPI_ENABLE_FLOW_TRACKING == 0\n"
#endif


#if DPI_ENABLE_SUBSCRIBER_CACHING == 1
/** Subscriber user data structure. Carries data that is required for the subscriber caching. */
typedef struct subscriber_user_data {
    /** The ID that will be compared to the ID cached on the flow. The result of the comparison will determine whether
     * the cached subscriber data pointer can be used. Otherwise the usual PHT lookup will take place. */
    u64 subscriber_id;
} subscriber_user_data_t;
#endif

#if DPI_GTPU_ENABLED > 0
/* GTPU definition below: */
/* Change GTP_U default port */
#define GTP_U_UDP_PORT (2152)
#define GTP_MESSAGE_TYPE_TPDU (0xff)
#define GTP_MAX_SUPPORTED_EXTENSION_HEADERS (10)

// default values for fdep plugin. Will be ignored if plugin unavailable
#define IPFIX_USE_SOCKET (1)
#define IPFIX_DST_PORT (4739)
#define IPFIX_COLLECTOR_ADDRESS "127.0.0.1"             //"COLLECTOR_IP"
#define IPFIX_TEMPLATE_INTERVAL_IN_SEC (30)
#define IPFIX_ACTIVE_TIMEOUT_DELTA (20)                 // active flow report in sec. 0 only reports final flow 
#define IPFIX_ACTIVE_DELTA_PKTS (0)
#define IPFIX_OBSERVATION_DOMAIN_ID (0)
#define IPFIX_APPLICATION_MAPPING_INTERVAL (3600)
#define IPFIX_MSS_SIZE (1400)
#define IPFIX_MSS_TIMEOUT (500)                         // 500ms MSS/PDU timeout flush
#define IPFIX_DISABLE_NAME_RESOLUTION_FLOWS (1)         // default 1, disable DNS/MDNS reporting for ipfix

struct gtp_next_extension_header {
    /* content length in 4-octet units (must be a multiple of 4) */
    u16 len;
    const u8 * content;
    const u8 * next_extension_header;
};

struct gtp_v1_header {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    u8 n_pdu_present : 1;
    u8 seq_num_present : 1;
    u8 next_extension_header_present : 1;
    u8 reserved : 1;
    u8 protocol_type : 1;
    u8 version : 3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    u8 version : 3;
    u8 protocol_type : 1;
    u8 reserved : 1;
    u8 next_extension_header_present : 1;
    u8 seq_num_present : 1;
    u8 n_pdu_present : 1;
#endif /* __BYTE_ORDER__ */

    u8 message_type;
    u16 length;
    u32 tunnel_endpoint_id;
};
#endif

/*
DPI config struct. Can be set via dpi_read_configfile() if DPDK dependency <rte_cfgfile.h> is available.
*/
typedef struct{
    struct{
        uint32_t io_resolution;
        uint32_t deca_max_entries;
        uint32_t unclassified_after_packet_count;
        uint32_t flow_offload;
        bool fpc_enabled;
    }dpi;
    struct{
        uint32_t max_entries;
        uint32_t timeout_in_s;
        bool skip_dns_tracking;
    }flow_table;
    struct{
        uint32_t max_entries;
        uint32_t timeout_in_s;
        bool enabled;
    }subscriber_table;
    struct{
        char * ip_dst;
        uint16_t port;
        bool enabled;
        uint32_t active_timeout_delta_time ;       
        uint32_t active_timeout_delta_packets ;
        uint32_t template_refresh_interval ;
        uint32_t observation_domain_id ;
        uint32_t application_id_mapping_refresh_interval;
        uint32_t mss_size ;
        uint32_t mss_timeout;
        bool disable_name_resolution_flows ;
    }fdep_plugin; // do not hide via macro, to avoid config parser issues
} dpi_optargs_t;

#define DPI_MAX_ATTRIBUTE_ENTRIES (6)
/**
 * @brief Definition of a generic DPI classification result structure. Use the corresponding PACE2 or vPACE APIs to retrieve string/enum mapping.
 * 
 * E.g. for PACE2:
 * map application id with typededf enum PACE2_applications in pace2_applications_enum.h. Retrieve strings and details with pace2_get_application_short_string().
 * 
 * E.g. for vPACE:
 * map application id with enum vpace_applications in applications.h. Retrieve strings and details with vpace_get_application_info_by_application_id()
 * as shown in dpi_wrapper_vpace.c
 * 
 */
typedef struct {
    /** Application classification result.  */
    uint16_t application;
    /** Protocol classification result for layer 7. */
    uint16_t protocol_l7;

    /** Protocol classification result for layer 4. */
    uint16_t protocol_l4;

    /** Protocol classification result for layer 3. */
    uint16_t protocol_l3;

    /** Attributes of classified protocol. */
    uint8_t protocol_attributes[DPI_MAX_ATTRIBUTE_ENTRIES];

    /** Attributes of classified application. */
    uint8_t application_attributes[DPI_MAX_ATTRIBUTE_ENTRIES];

    /** Represents the current state of the application classification. If not set, no application has been identified
     *  so far. */
    uint8_t classification_finished_or_offloaded : 1;

    /** 1 if flow is a control-flow. These flows must not be offloaded to NICs since crucial for detection */
    uint8_t is_control_flow : 1;
} dpi_result_t;

// external

extern void * util_malloc(size_t const size, void * const user_ptr);

extern void * util_realloc(void * const ptr, size_t const size, void * const user_ptr);

extern void util_free(void * const ptr, void * const user_ptr);

/* Public function declarations: */
/**
 * @brief Retrieves the version string of the DPI module
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] tid               Thread/worker id.
 * @param[out] version          Output char* to store DPI version.
 * @param[in] version_len       Max length of version string.
 * 
 * @return  dpi_return_t
 */
dpi_return_t dpi_get_dpi_version(dpi_instance * const dpi_instance_in , u16 const tid, char * const version, u16 const version_len);

/**
 * @brief creates an initialized dpi_instance including processing statistics
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] dpi_worker_count  Amount of workers to be initialized 
 * 
 * @return dpi_return_t
 */
dpi_return_t dpi_init_instance(dpi_instance ** const dpi_instance_in, u8 const dpi_worker_count, dpi_optargs_t * opts);

/**
 * @brief required to be invoked for each lcore/processing thread to ensure NUMA-awareness
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] tid               Thread/worker id.
 * 
 * @return dpi_return_t
 */
dpi_return_t dpi_enable_discrete_thread(dpi_instance * const dpi_instance_in, unsigned int const tid);

/**
 * @brief destroys_the dpi_instance and frees underlying allocations
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] dpi_worker_count  Amount of workers to be initialized 
 * 
 * @return dpi_return_t
 */
dpi_return_t dpi_destroy_instance(dpi_instance ** const dpi_instance_in, u8 const dpi_worker_count);

/**
 * @brief prints out the DPI statistics for each active dpi_worker
 * 
 * @param[in,out] fp            File pointer (stderr, stdout, data file etc. )
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 */
void dpi_print_stats(FILE * fp, dpi_instance * const dpi_instance_in);

/**
 * @brief processes an IP-packet on a specified tid. Requires a valid ptr to IPv4 or IPv6 hdr.
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] tid               Thread/worker id.
 * @param[in] time              Timestamp in ticks_per_second scaled to DPI_CLOCK_TICKS_PER_SEC
 * @param[in] iph               void * pointer to the innermost Layer3 IP-header
 * @param[in] ipsize            Expected length from iph onwards in Bytes
 * @param[out] dpi_result       [Optional] Pointer to dpi_result_t struct to store classification results. Can be NULL.
 * 
 * @return dpi_return_t
 */
dpi_return_t dpi_process_packet(dpi_instance * const dpi_instance_in, u8 const tid , u64 const time, void const * const iph, u16  const ipsize, dpi_result_t * dpi_result);

/**
 * @brief dpi_process_packet processes an L2 frame on a specified tid. A mbuf pointer can be used as l2h due to supported L2 decapsulation.
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] tid               Thread/worker id.
 * @param[in] time              Timestamp in ticks_per_second scaled to DPI_CLOCK_TICKS_PER_SEC
 * @param[in] l2h               void * pointer to the l2 header
 * @param[in] l2size            Expected length from l2 (mbuf) onwards in Bytes
 * @param[in] decaps_gtpu       bool flag, if enabled calculate offset for GTP-U traffic incl. GTP-extension headers
 * @param[out] dpi_result       [Optional] Pointer to dpi_result_t struct to store classification results. Can be NULL.
 * 
 * @return dpi_return_t
 */
dpi_return_t dpi_process_l2_frame(dpi_instance * const dpi_instance_in, u8 const tid , u64 const time, void const * const l2h, u16  const l2size, u8 const decaps_gtpu, dpi_result_t * dpi_result);

/**
 * @brief Upgrades dpi worker instances to newer version using dynamic upgrade APIs
 * 
 * @param[in] dpi_instance_in   Pointer to dpi instance.
 * @param[in] path_to_library   Path to DPI library upgrade file ( e.g. "/var/tmp/new_lib/libipoque_pace2.so" )
 * 
 * @return dpi_return_t 
 */
dpi_return_t dpi_upgrade_library(dpi_instance * const dpi_instance_in, char const * const path_to_library);

/**
 * @brief Print the current DPI config
 * 
 * @param[in,out] fp            File pointer (stderr, stdout, data file etc. )
 */
void dpi_print_dpi_config(FILE * fp, dpi_instance * const dpi_instance_in);

/**
 * @brief Return the time since 1/1/1970 in given io_resolution
 *        derived from https://github.com/DPDK/dpdk/blob/HEAD/app/dumpcap/main.c#L516-L517
 *         io_resoluton needs to match PACE2 config!
 * @example usage: 
 *          #define DPI_CLOCK_TICKS_PER_SEC (1000*1000) //us
 *          dpi_get_timestamp(DPI_CLOCK_TICKS_PER_SEC);
 *         
 * @param io_resolution 1000 for ms, 1000*1000 for us, 1000*1000*1000 for ns
 * @return uint64_t timestamp since epoch. 0 for invalid io_resolution
 */
uint64_t dpi_get_timestamp(uint64_t const io_resolution);

/**
 * @brief Prints generic dpi_result to file pointer (stderr, stdout, file etc)
 * 
 * @param fp  file pointer (stderr, stdout, file etc)
 * @param dpi_instance_in
 * @param tid
 * @param dpi_result 
 */
void dpi_print_result(FILE *fp,  dpi_instance * const dpi_instance_in, const u16 tid, dpi_result_t * dpi_result);

/**
 * @brief Set default values to cfg file.
 * 
 * @param dpi_cfg 
 * @return int 
 */
inline int dpi_init_default_cfg(dpi_optargs_t * dpi_cfg)
{
    if (dpi_cfg == NULL)
    {
        return -1;
    }
    memset(dpi_cfg, 0, sizeof(dpi_optargs_t));
    dpi_cfg->dpi.io_resolution = DPI_CLOCK_TICKS_PER_SEC;
    dpi_cfg->dpi.deca_max_entries = DPI_DECA_MAX_ENTRIES;
    dpi_cfg->dpi.unclassified_after_packet_count = DPI_UNCLASSIFIED_AFTER_PACKET_COUNT;
    dpi_cfg->dpi.flow_offload = DPI_FLOW_OFFLOAD;
    dpi_cfg->dpi.fpc_enabled = 1;

    dpi_cfg->flow_table.max_entries = DPI_MAX_CONCURRENT_FLOWS;
    dpi_cfg->flow_table.timeout_in_s = DPI_FLOW_TIMEOUT;
    dpi_cfg->flow_table.skip_dns_tracking = 1;

    dpi_cfg->subscriber_table.max_entries = DPI_MAX_CONCURRENT_SUBSCRIBERS;
    dpi_cfg->subscriber_table.timeout_in_s = DPI_SUBSCRIBER_TIMEOUT;
    dpi_cfg->subscriber_table.enabled = DPI_ENABLE_SUBSCRIBER_TRACKING;

#if defined(PLUGIN_ID) && PLUGIN_ID == PLUGIN_IPFIX
    dpi_cfg->fdep_plugin.enabled = 1;
    dpi_cfg->fdep_plugin.ip_dst = IPFIX_COLLECTOR_ADDRESS;
    dpi_cfg->fdep_plugin.port = IPFIX_DST_PORT;
    dpi_cfg->fdep_plugin.active_timeout_delta_time = IPFIX_ACTIVE_TIMEOUT_DELTA;
    dpi_cfg->fdep_plugin.active_timeout_delta_packets = IPFIX_ACTIVE_DELTA_PKTS;
    dpi_cfg->fdep_plugin.template_refresh_interval = IPFIX_TEMPLATE_INTERVAL_IN_SEC;
    dpi_cfg->fdep_plugin.observation_domain_id = IPFIX_OBSERVATION_DOMAIN_ID;
    dpi_cfg->fdep_plugin.application_id_mapping_refresh_interval = IPFIX_APPLICATION_MAPPING_INTERVAL;
    dpi_cfg->fdep_plugin.mss_size = IPFIX_MSS_SIZE;
    dpi_cfg->fdep_plugin.mss_timeout = IPFIX_MSS_TIMEOUT;
    dpi_cfg->fdep_plugin.disable_name_resolution_flows = IPFIX_DISABLE_NAME_RESOLUTION_FLOWS;
#endif
    return 0;
}


#endif /* DPI_WRAPPER_H */

/*********************************************************************************
 ** EOF
 ********************************************************************************/
