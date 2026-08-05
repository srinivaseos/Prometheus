#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>


#define PROTOB_GTPV1_U_UDP_PORT            2152
#define PROTOB_GTPV2_C_UDP_PORT            2123

#define PROTOB_ED4(x1, x2, x3, x4) x4 x3 x2 x1
//#define PROTOB_ED4(x1, x2, x3, x4) x1 x2 x3 x4
//#define ED4(x1, x2, x3, x4) x4 x3 x2 x1

#define PROTOB_GTPV1U_HEADER_LEN   8
#define PROTOB_GTPV2C_HEADER_LEN   12
#define PROTOB_GTP_TEID_LEN        4
typedef struct protob_gtp_header_s {
    union {
        struct {
#define PROTOB_GTP_VERSION_0 0
#define PROTOB_GTP_VERSION_1 1
        PROTOB_ED4(uint8_t version:3;,
            uint8_t piggybacked:1;,
            uint8_t teid_presence:1;,
            uint8_t spare1:3;)
        };
/* GTU-U flags */
#define PROTOB_GTPU_FLAGS_V                        0x20
#define PROTOB_GTPU_FLAGS_PT                       0x10
#define PROTOB_GTPU_FLAGS_E                        0x04
#define PROTOB_GTPU_FLAGS_S                        0x02
#define PROTOB_GTPU_FLAGS_PN                       0x01
        uint8_t flags;
	};
/* GTP-U message type, defined in 3GPP TS 29.281 Release 11 */
#define PROTOB_GTPU_MSGTYPE_ECHO_REQ               1
#define PROTOB_GTPU_MSGTYPE_ECHO_RSP               2
#define PROTOB_GTPU_MSGTYPE_ERR_IND                26
#define PROTOB_GTPU_MSGTYPE_SUPP_EXTHDR_NOTI       31
#define PROTOB_GTPU_MSGTYPE_END_MARKER             254
#define PROTOB_GTPU_MSGTYPE_GPDU                   255
    uint8_t type;
    uint16_t length;
    union {
        struct {
            uint32_t teid;
            /* sqn : 31bit ~ 8bit, spare : 7bit ~ 0bit */
#define PROTOB_GTP_XID_TO_SQN(__xid) htobe32(((__xid) << 8))
#define PROTOB_GTP_SQN_TO_XID(__sqn) (be32toh(__sqn) >> 8)
            uint32_t sqn;
        };
        /* sqn : 31bit ~ 8bit, spare : 7bit ~ 0bit */
        uint32_t sqn_only;
    };
} __attribute__ ((packed)) protob_gtp_header_t;	


/* GTPv2-C message type */
#define PROTOB_GTP_ECHO_REQUEST_TYPE 1
#define PROTOB_GTP_ECHO_RESPONSE_TYPE 2
#define PROTOB_GTP_VERSION_NOT_SUPPORTED_INDICATION_TYPE 3
#define PROTOB_GTP_CREATE_SESSION_REQUEST_TYPE 32
#define PROTOB_GTP_CREATE_SESSION_RESPONSE_TYPE 33
#define PROTOB_GTP_MODIFY_BEARER_REQUEST_TYPE 34
#define PROTOB_GTP_MODIFY_BEARER_RESPONSE_TYPE 35
#define PROTOB_GTP_DELETE_SESSION_REQUEST_TYPE 36
#define PROTOB_GTP_DELETE_SESSION_RESPONSE_TYPE 37
#define PROTOB_GTP_CHANGE_NOTIFICATION_REQUEST_TYPE 38
#define PROTOB_GTP_CHANGE_NOTIFICATION_RESPONSE_TYPE 39
#define PROTOB_GTP_REMOTE_UE_REPORT_NOTIFICATION_TYPE 40
#define PROTOB_GTP_REMOTE_UE_REPORT_ACKNOWLEDGE_TYPE 41
#define PROTOB_GTP_MODIFY_BEARER_COMMAND_TYPE 64
#define PROTOB_GTP_MODIFY_BEARER_FAILURE_INDICATION_TYPE 65
#define PROTOB_GTP_DELETE_BEARER_COMMAND_TYPE 66
#define PROTOB_GTP_DELETE_BEARER_FAILURE_INDICATION_TYPE 67
#define PROTOB_GTP_BEARER_RESOURCE_COMMAND_TYPE 68
#define PROTOB_GTP_BEARER_RESOURCE_FAILURE_INDICATION_TYPE 69
#define PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION_TYPE 70
#define PROTOB_GTP_TRACE_SESSION_ACTIVATION_TYPE 71
#define PROTOB_GTP_TRACE_SESSION_DEACTIVATION_TYPE 72
#define PROTOB_GTP_STOP_PAGING_INDICATION_TYPE 73
#define PROTOB_GTP_CREATE_BEARER_REQUEST_TYPE 95
#define PROTOB_GTP_CREATE_BEARER_RESPONSE_TYPE 96
#define PROTOB_GTP_UPDATE_BEARER_REQUEST_TYPE 97
#define PROTOB_GTP_UPDATE_BEARER_RESPONSE_TYPE 98
#define PROTOB_GTP_DELETE_BEARER_REQUEST_TYPE 99
#define PROTOB_GTP_DELETE_BEARER_RESPONSE_TYPE 100
#define PROTOB_GTP_DELETE_PDN_CONNECTION_SET_REQUEST_TYPE 101
#define PROTOB_GTP_DELETE_PDN_CONNECTION_SET_RESPONSE_TYPE 102
#define PROTOB_GTP_PGW_DOWNLINK_TRIGGERING_NOTIFICATION_TYPE 103
#define PROTOB_GTP_PGW_DOWNLINK_TRIGGERING_ACKNOWLEDGE_TYPE 104
#define PROTOB_GTP_CREATE_FORWARDING_TUNNEL_REQUEST_TYPE 160
#define PROTOB_GTP_CREATE_FORWARDING_TUNNEL_RESPONSE_TYPE 161
#define PROTOB_GTP_SUSPEND_NOTIFICATION_TYPE 162
#define PROTOB_GTP_SUSPEND_ACKNOWLEDGE_TYPE 163
#define PROTOB_GTP_RESUME_NOTIFICATION_TYPE 164
#define PROTOB_GTP_RESUME_ACKNOWLEDGE_TYPE 165
#define PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE 166
#define PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE 167
#define PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE 168
#define PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE 169
#define PROTOB_GTP_RELEASE_ACCESS_BEARERS_REQUEST_TYPE 170
#define PROTOB_GTP_RELEASE_ACCESS_BEARERS_RESPONSE_TYPE 171
#define PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_TYPE 176
#define PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE_TYPE 177
#define PROTOB_GTP_PGW_RESTART_NOTIFICATION_TYPE 179
#define PROTOB_GTP_PGW_RESTART_NOTIFICATION_ACKNOWLEDGE_TYPE 180
#define PROTOB_GTP_UPDATE_PDN_CONNECTION_SET_REQUEST_TYPE 200
#define PROTOB_GTP_UPDATE_PDN_CONNECTION_SET_RESPONSE_TYPE 201
#define PROTOB_GTP_MODIFY_ACCESS_BEARERS_REQUEST_TYPE 211
#define PROTOB_GTP_MODIFY_ACCESS_BEARERS_RESPONSE_TYPE 212

#define PROTOB_GTP_IMSI_TYPE 1
#define PROTOB_GTP_CAUSE_TYPE 2
#define PROTOB_GTP_RECOVERY_TYPE 3
#define PROTOB_GTP_STN_SR_TYPE 51
#define PROTOB_GTP_APN_TYPE 71
#define PROTOB_GTP_AMBR_TYPE 72
#define PROTOB_GTP_EBI_TYPE 73
#define PROTOB_GTP_IP_ADDRESS_TYPE 74
#define PROTOB_GTP_MEI_TYPE 75
#define PROTOB_GTP_MSISDN_TYPE 76
#define PROTOB_GTP_INDICATION_TYPE 77
#define PROTOB_GTP_PCO_TYPE 78
#define PROTOB_GTP_PAA_TYPE 79
#define PROTOB_GTP_BEARER_QOS_TYPE 80
#define PROTOB_GTP_FLOW_QOS_TYPE 81
#define PROTOB_GTP_RAT_TYPE_TYPE 82
#define PROTOB_GTP_SERVING_NETWORK_TYPE 83
#define PROTOB_GTP_BEARER_TFT_TYPE 84
#define PROTOB_GTP_TAD_TYPE 85
#define PROTOB_GTP_ULI_TYPE 86
#define PROTOB_GTP_F_TEID_TYPE 87
#define PROTOB_GTP_TMSI_TYPE 88
#define PROTOB_GTP_GLOBAL_CN_ID_TYPE 89
#define PROTOB_GTP_S103PDF_TYPE 90
#define PROTOB_GTP_S1UDF_TYPE 91
#define PROTOB_GTP_DELAY_VALUE_TYPE 92
#define PROTOB_GTP_BEARER_CONTEXT_TYPE 93
#define PROTOB_GTP_CHARGING_ID_TYPE 94
#define PROTOB_GTP_CHARGING_CHARACTERISTICS_TYPE 95
#define PROTOB_GTP_TRACE_INFORMATION_TYPE 96
#define PROTOB_GTP_BEARER_FLAGS_TYPE 97
#define PROTOB_GTP_PDN_TYPE_TYPE 99
#define PROTOB_GTP_PTI_TYPE 100
#define PROTOB_GTP_MM_CONTEXT_TYPE 107
#define PROTOB_GTP_PDN_CONNECTION_TYPE 109
#define PROTOB_GTP_PDU_NUMBERS_TYPE 110
#define PROTOB_GTP_P_TMSI_TYPE 111
#define PROTOB_GTP_P_TMSI_SIGNATURE_TYPE 112
#define PROTOB_GTP_HOP_COUNTER_TYPE 113
#define PROTOB_GTP_UE_TIME_ZONE_TYPE 114
#define PROTOB_GTP_TRACE_REFERENCE_TYPE 115
#define PROTOB_GTP_COMPLETE_REQUEST_MESSAGE_TYPE 116
#define PROTOB_GTP_GUTI_TYPE 117
#define PROTOB_GTP_F_CONTAINER_TYPE 118
#define PROTOB_GTP_F_CAUSE_TYPE 119
#define PROTOB_GTP_PLMN_ID_TYPE 120
#define PROTOB_GTP_TARGET_IDENTIFICATION_TYPE 121
#define PROTOB_GTP_PACKET_FLOW_ID_TYPE 123
#define PROTOB_GTP_RAB_CONTEXT_TYPE 124
#define PROTOB_GTP_SOURCE_RNC_PDCP_CONTEXT_INFO_TYPE 125
#define PROTOB_GTP_PORT_NUMBER_TYPE 126
#define PROTOB_GTP_APN_RESTRICTION_TYPE 127
#define PROTOB_GTP_SELECTION_MODE_TYPE 128
#define PROTOB_GTP_SOURCE_IDENTIFICATION_TYPE 129
#define PROTOB_GTP_CHANGE_REPORTING_ACTION_TYPE 131
#define PROTOB_GTP_FQ_CSID_TYPE 132
#define PROTOB_GTP_CHANNEL_NEEDED_TYPE 133
#define PROTOB_GTP_EMLPP_PRIORITY_TYPE 134
#define PROTOB_GTP_NODE_TYPE_TYPE 135
#define PROTOB_GTP_FQDN_TYPE 136
#define PROTOB_GTP_TI_TYPE 137
#define PROTOB_GTP_MBMS_SESSION_DURATION_TYPE 138
#define PROTOB_GTP_MBMS_SERVICE_AREA_TYPE 139
#define PROTOB_GTP_MBMS_SESSION_IDENTIFIER_TYPE 140
#define PROTOB_GTP_MBMS_FLOW_IDENTIFIER_TYPE 141
#define PROTOB_GTP_MBMS_IP_MULTICAST_DISTRIBUTION_TYPE 142
#define PROTOB_GTP_MBMS_DISTRIBUTION_ACKNOWLEDGE_TYPE 143
#define PROTOB_GTP_RFSP_INDEX_TYPE 144
#define PROTOB_GTP_UCI_TYPE 145
#define PROTOB_GTP_CSG_INFORMATION_REPORTING_ACTION_TYPE 146
#define PROTOB_GTP_CSG_ID_TYPE 147
#define PROTOB_GTP_CMI_TYPE 148
#define PROTOB_GTP_SERVICE_INDICATOR_TYPE 149
#define PROTOB_GTP_DETACH_TYPE_TYPE 150
#define PROTOB_GTP_LDN_TYPE 151
#define PROTOB_GTP_NODE_FEATURES_TYPE 152
#define PROTOB_GTP_MBMS_TIME_TO_DATA_TRANSFER_TYPE 153
#define PROTOB_GTP_THROTTLING_TYPE 154
#define PROTOB_GTP_ARP_TYPE 155
#define PROTOB_GTP_EPC_TIMER_TYPE 156
#define PROTOB_GTP_SIGNALLING_PRIORITY_INDICATION_TYPE 157
#define PROTOB_GTP_TMGI_TYPE 158
#define PROTOB_GTP_ADDITIONAL_MM_CONTEXT_FOR_SRVCC_TYPE 159
#define PROTOB_GTP_ADDITIONAL_FLAGS_FOR_SRVCC_TYPE 160
#define PROTOB_GTP_MDT_CONFIGURATION_TYPE 162
#define PROTOB_GTP_APCO_TYPE 163
#define PROTOB_GTP_ABSOLUTE_TIME_OF_MBMS_DATA_TRANSFER_TYPE 164
#define PROTOB_GTP_ENB_INFORMATION_REPORTING_TYPE 165
#define PROTOB_GTP_IP4CP_TYPE 166
#define PROTOB_GTP_CHANGE_TO_REPORT_FLAGS_TYPE 167
#define PROTOB_GTP_ACTION_INDICATION_TYPE 168
#define PROTOB_GTP_TWAN_IDENTIFIER_TYPE 169
#define PROTOB_GTP_ULI_TIMESTAMP_TYPE 170
#define PROTOB_GTP_MBMS_FLAGS_TYPE 171
#define PROTOB_GTP_RAN_NAS_CAUSE_TYPE 172
#define PROTOB_GTP_CN_OPERATOR_SELECTION_ENTITY_TYPE 173
#define PROTOB_GTP_TWMI_TYPE 174
#define PROTOB_GTP_NODE_NUMBER_TYPE 175
#define PROTOB_GTP_NODE_IDENTIFIER_TYPE 176
#define PROTOB_GTP_PRESENCE_REPORTING_AREA_ACTION_TYPE 177
#define PROTOB_GTP_PRESENCE_REPORTING_AREA_INFORMATION_TYPE 178
#define PROTOB_GTP_TWAN_IDENTIFIER_TIMESTAMP_TYPE 179
#define PROTOB_GTP_OVERLOAD_CONTROL_INFORMATION_TYPE 180
#define PROTOB_GTP_LOAD_CONTROL_INFORMATION_TYPE 181
#define PROTOB_GTP_METRIC_TYPE 182
#define PROTOB_GTP_SEQUENCE_NUMBER_TYPE 183
#define PROTOB_GTP_APN_AND_RELATIVE_CAPACITY_TYPE 184
#define PROTOB_GTP_WLAN_OFFLOADABILITY_INDICATION_TYPE 185
#define PROTOB_GTP_PAGING_AND_SERVICE_INFORMATION_TYPE 186
#define PROTOB_GTP_INTEGER_NUMBER_TYPE 187
#define PROTOB_GTP_MILLISECOND_TIME_STAMP_TYPE 188
#define PROTOB_GTP_MONITORING_EVENT_INFORMATION_TYPE 189
#define PROTOB_GTP_ECGI_LIST_TYPE 190
#define PROTOB_GTP_REMOTE_UE_CONTEXT_TYPE 191
#define PROTOB_GTP_REMOTE_USER_ID_TYPE 192
#define PROTOB_GTP_REMOTE_UE_IP_INFORMATION_TYPE 193
#define PROTOB_GTP_CIOT_OPTIMIZATIONS_SUPPORT_INDICATION_TYPE 194
#define PROTOB_GTP_SCEF_PDN_CONNECTION_TYPE 195
#define PROTOB_GTP_HEADER_COMPRESSION_CONFIGURATION_TYPE 196
#define PROTOB_GTP_EPCO_TYPE 197
#define PROTOB_GTP_SERVING_PLMN_RATE_CONTROL_TYPE 198
#define PROTOB_GTP_COUNTER_TYPE 199
#define PROTOB_GTP_MAPPED_UE_USAGE_TYPE_TYPE 200
#define PROTOB_GTP_SECONDARY_RAT_USAGE_DATA_REPORT_TYPE 201
#define PROTOB_GTP_UP_FUNCTION_SELECTION_INDICATION_FLAGS_TYPE 202
#define PROTOB_GTP_MAXIMUM_PACKET_LOSS_RATE_TYPE 203
#define PROTOB_GTP_APN_RATE_CONTROL_STATUS_TYPE 204
#define PROTOB_GTP_EXTENDED_TRACE_INFORMATION_TYPE 205
#define PROTOB_GTP_MONITORING_EVENT_EXTENSION_INFORMATION_TYPE 206
#define PROTOB_GTP_ADDITIONAL_RRM_POLICY_INDEX_TYPE 207
#define PROTOB_GTP_V2X_CONTEXT_TYPE 208
#define PROTOB_GTP_PC5_QOS_PARAMETERS_TYPE 209
#define PROTOB_GTP_SERVICES_AUTHORIZED_TYPE 210
#define PROTOB_GTP_BIT_RATE_TYPE 211
#define PROTOB_GTP_PC5_QOS_FLOW_TYPE 212

/* Infomration Element TLV Descriptor */
extern protob_tlv_desc_t protob_gtp_tlv_desc_imsi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_cause_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_recovery_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_stn_sr_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_apn_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ambr_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ebi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ebi_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_3;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mei_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_msisdn_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_indication_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pco_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_paa_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_qos_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_flow_qos_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_rat_type_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_serving_network_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_tft_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_tad_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_uli_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_uli_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_3;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_4;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_5;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_6;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_7;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_8;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_9;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_10;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_11;
extern protob_tlv_desc_t protob_gtp_tlv_desc_tmsi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_global_cn_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_s103pdf_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_s1udf_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delay_value_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_charging_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_charging_characteristics_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_trace_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_flags_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pdn_type_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pti_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mm_context_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pdu_numbers_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_p_tmsi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_p_tmsi_signature_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_hop_counter_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ue_time_zone_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_trace_reference_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_complete_request_message_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_guti_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_container_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_f_cause_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_plmn_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_target_identification_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_packet_flow_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_rab_context_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_source_rnc_pdcp_context_info_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_port_number_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_port_number_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_port_number_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_apn_restriction_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_selection_mode_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_source_identification_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_change_reporting_action_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_3;
extern protob_tlv_desc_t protob_gtp_tlv_desc_channel_needed_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_emlpp_priority_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_node_type_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fqdn_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_fqdn_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ti_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_session_duration_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_service_area_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_session_identifier_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_flow_identifier_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_ip_multicast_distribution_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_distribution_acknowledge_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_rfsp_index_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_uci_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_csg_information_reporting_action_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_csg_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_cmi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_service_indicator_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_detach_type_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ldn_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ldn_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ldn_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ldn_3;
extern protob_tlv_desc_t protob_gtp_tlv_desc_node_features_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_time_to_data_transfer_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_throttling_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_arp_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_epc_timer_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_signalling_priority_indication_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_tmgi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_additional_mm_context_for_srvcc_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_additional_flags_for_srvcc_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mdt_configuration_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_apco_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_absolute_time_of_mbms_data_transfer_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_enb_information_reporting_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ip4cp_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_change_to_report_flags_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_action_indication_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_uli_timestamp_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mbms_flags_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ran_nas_cause_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_cn_operator_selection_entity_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_twmi_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_node_number_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_node_identifier_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_presence_reporting_area_action_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_presence_reporting_area_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_timestamp_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_timestamp_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_metric_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_sequence_number_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_apn_and_relative_capacity_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_wlan_offloadability_indication_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_paging_and_service_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_integer_number_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_millisecond_time_stamp_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_monitoring_event_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ecgi_list_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_remote_user_id_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_ip_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_ciot_optimizations_support_indication_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_header_compression_configuration_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_epco_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_serving_plmn_rate_control_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_counter_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_mapped_ue_usage_type_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_secondary_rat_usage_data_report_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_up_function_selection_indication_flags_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_maximum_packet_loss_rate_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_apn_rate_control_status_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_extended_trace_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_monitoring_event_extension_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_additional_rrm_policy_index_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_services_authorized_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_services_authorized_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bit_rate_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bit_rate_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pc5_qos_flow_0;

/* Group Infomration Element TLV Descriptor */
extern protob_tlv_desc_t protob_gtp_tlv_desc_pc5_qos_parameters_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_context_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_v2x_context_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_context_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_context_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pdn_connection_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_0;
extern protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_1;
extern protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_2;
extern protob_tlv_desc_t protob_gtp_tlv_desc_scef_pdn_connection_0;

/* Message Descriptor */
extern protob_tlv_desc_t protob_gtp_tlv_desc_echo_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_echo_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_version_not_supported_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_session_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_session_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_session_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_session_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_change_notification_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_change_notification_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_report_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_report_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_command;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_failure_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_command;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_failure_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_resource_command;
extern protob_tlv_desc_t protob_gtp_tlv_desc_bearer_resource_failure_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification_failure_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_trace_session_activation;
extern protob_tlv_desc_t protob_gtp_tlv_desc_trace_session_deactivation;
extern protob_tlv_desc_t protob_gtp_tlv_desc_stop_paging_indication;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_bearer_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_bearer_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_update_bearer_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_update_bearer_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_pdn_connection_set_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_pdn_connection_set_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pgw_downlink_triggering_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pgw_downlink_triggering_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_forwarding_tunnel_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_forwarding_tunnel_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_suspend_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_suspend_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_resume_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_resume_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_release_access_bearers_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_release_access_bearers_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pgw_restart_notification;
extern protob_tlv_desc_t protob_gtp_tlv_desc_pgw_restart_notification_acknowledge;
extern protob_tlv_desc_t protob_gtp_tlv_desc_update_pdn_connection_set_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_update_pdn_connection_set_response;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_access_bearers_request;
extern protob_tlv_desc_t protob_gtp_tlv_desc_modify_access_bearers_response;

/* Structure for Infomration Element */
typedef protob_tlv_octet_t protob_gtp_tlv_imsi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_cause_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_recovery_t;
typedef protob_tlv_octet_t protob_gtp_tlv_stn_sr_t;
typedef protob_tlv_octet_t protob_gtp_tlv_apn_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ambr_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_ebi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ip_address_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mei_t;
typedef protob_tlv_octet_t protob_gtp_tlv_msisdn_t;
typedef protob_tlv_octet_t protob_gtp_tlv_indication_t;
typedef protob_tlv_octet_t protob_gtp_tlv_pco_t;
typedef protob_tlv_octet_t protob_gtp_tlv_paa_t;
typedef protob_tlv_octet_t protob_gtp_tlv_bearer_qos_t;
typedef protob_tlv_octet_t protob_gtp_tlv_flow_qos_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_rat_type_t;
typedef protob_tlv_octet_t protob_gtp_tlv_serving_network_t;
typedef protob_tlv_octet_t protob_gtp_tlv_bearer_tft_t;
typedef protob_tlv_octet_t protob_gtp_tlv_tad_t;
typedef protob_tlv_octet_t protob_gtp_tlv_uli_t;
typedef protob_tlv_octet_t protob_gtp_tlv_f_teid_t;
typedef protob_tlv_octet_t protob_gtp_tlv_tmsi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_global_cn_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_s103pdf_t;
typedef protob_tlv_octet_t protob_gtp_tlv_s1udf_t;
typedef protob_tlv_octet_t protob_gtp_tlv_delay_value_t;
typedef protob_tlv_octet_t protob_gtp_tlv_charging_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_charging_characteristics_t;
typedef protob_tlv_octet_t protob_gtp_tlv_trace_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_bearer_flags_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_pdn_type_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_pti_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mm_context_t;
typedef protob_tlv_octet_t protob_gtp_tlv_pdu_numbers_t;
typedef protob_tlv_octet_t protob_gtp_tlv_p_tmsi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_p_tmsi_signature_t;
typedef protob_tlv_octet_t protob_gtp_tlv_hop_counter_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ue_time_zone_t;
typedef protob_tlv_octet_t protob_gtp_tlv_trace_reference_t;
typedef protob_tlv_octet_t protob_gtp_tlv_complete_request_message_t;
typedef protob_tlv_octet_t protob_gtp_tlv_guti_t;
typedef protob_tlv_octet_t protob_gtp_tlv_f_container_t;
typedef protob_tlv_octet_t protob_gtp_tlv_f_cause_t;
typedef protob_tlv_octet_t protob_gtp_tlv_plmn_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_target_identification_t;
typedef protob_tlv_octet_t protob_gtp_tlv_packet_flow_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_rab_context_t;
typedef protob_tlv_octet_t protob_gtp_tlv_source_rnc_pdcp_context_info_t;
typedef protob_tlv_uint16_t protob_gtp_tlv_port_number_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_apn_restriction_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_selection_mode_t;
typedef protob_tlv_octet_t protob_gtp_tlv_source_identification_t;
typedef protob_tlv_octet_t protob_gtp_tlv_change_reporting_action_t;
typedef protob_tlv_octet_t protob_gtp_tlv_fq_csid_t;
typedef protob_tlv_octet_t protob_gtp_tlv_channel_needed_t;
typedef protob_tlv_octet_t protob_gtp_tlv_emlpp_priority_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_node_type_t;
typedef protob_tlv_octet_t protob_gtp_tlv_fqdn_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ti_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_session_duration_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_service_area_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_session_identifier_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_flow_identifier_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_ip_multicast_distribution_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_distribution_acknowledge_t;
typedef protob_tlv_octet_t protob_gtp_tlv_rfsp_index_t;
typedef protob_tlv_octet_t protob_gtp_tlv_uci_t;
typedef protob_tlv_octet_t protob_gtp_tlv_csg_information_reporting_action_t;
typedef protob_tlv_octet_t protob_gtp_tlv_csg_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_cmi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_service_indicator_t;
typedef protob_tlv_octet_t protob_gtp_tlv_detach_type_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ldn_t;
typedef protob_tlv_uint8_t protob_gtp_tlv_node_features_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_time_to_data_transfer_t;
typedef protob_tlv_octet_t protob_gtp_tlv_throttling_t;
typedef protob_tlv_octet_t protob_gtp_tlv_arp_t;
typedef protob_tlv_octet_t protob_gtp_tlv_epc_timer_t;
typedef protob_tlv_octet_t protob_gtp_tlv_signalling_priority_indication_t;
typedef protob_tlv_octet_t protob_gtp_tlv_tmgi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_additional_mm_context_for_srvcc_t;
typedef protob_tlv_octet_t protob_gtp_tlv_additional_flags_for_srvcc_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mdt_configuration_t;
typedef protob_tlv_octet_t protob_gtp_tlv_apco_t;
typedef protob_tlv_octet_t protob_gtp_tlv_absolute_time_of_mbms_data_transfer_t;
typedef protob_tlv_octet_t protob_gtp_tlv_enb_information_reporting_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ip4cp_t;
typedef protob_tlv_octet_t protob_gtp_tlv_change_to_report_flags_t;
typedef protob_tlv_octet_t protob_gtp_tlv_action_indication_t;
typedef protob_tlv_octet_t protob_gtp_tlv_twan_identifier_t;
typedef protob_tlv_octet_t protob_gtp_tlv_uli_timestamp_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mbms_flags_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ran_nas_cause_t;
typedef protob_tlv_octet_t protob_gtp_tlv_cn_operator_selection_entity_t;
typedef protob_tlv_octet_t protob_gtp_tlv_twmi_t;
typedef protob_tlv_octet_t protob_gtp_tlv_node_number_t;
typedef protob_tlv_octet_t protob_gtp_tlv_node_identifier_t;
typedef protob_tlv_octet_t protob_gtp_tlv_presence_reporting_area_action_t;
typedef protob_tlv_octet_t protob_gtp_tlv_presence_reporting_area_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_twan_identifier_timestamp_t;
typedef protob_tlv_octet_t protob_gtp_tlv_metric_t;
typedef protob_tlv_octet_t protob_gtp_tlv_sequence_number_t;
typedef protob_tlv_octet_t protob_gtp_tlv_apn_and_relative_capacity_t;
typedef protob_tlv_octet_t protob_gtp_tlv_wlan_offloadability_indication_t;
typedef protob_tlv_octet_t protob_gtp_tlv_paging_and_service_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_integer_number_t;
typedef protob_tlv_octet_t protob_gtp_tlv_millisecond_time_stamp_t;
typedef protob_tlv_octet_t protob_gtp_tlv_monitoring_event_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ecgi_list_t;
typedef protob_tlv_octet_t protob_gtp_tlv_remote_user_id_t;
typedef protob_tlv_octet_t protob_gtp_tlv_remote_ue_ip_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_ciot_optimizations_support_indication_t;
typedef protob_tlv_octet_t protob_gtp_tlv_header_compression_configuration_t;
typedef protob_tlv_octet_t protob_gtp_tlv_epco_t;
typedef protob_tlv_octet_t protob_gtp_tlv_serving_plmn_rate_control_t;
typedef protob_tlv_octet_t protob_gtp_tlv_counter_t;
typedef protob_tlv_octet_t protob_gtp_tlv_mapped_ue_usage_type_t;
typedef protob_tlv_octet_t protob_gtp_tlv_secondary_rat_usage_data_report_t;
typedef protob_tlv_octet_t protob_gtp_tlv_up_function_selection_indication_flags_t;
typedef protob_tlv_octet_t protob_gtp_tlv_maximum_packet_loss_rate_t;
typedef protob_tlv_octet_t protob_gtp_tlv_apn_rate_control_status_t;
typedef protob_tlv_octet_t protob_gtp_tlv_extended_trace_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_monitoring_event_extension_information_t;
typedef protob_tlv_octet_t protob_gtp_tlv_additional_rrm_policy_index_t;
typedef protob_tlv_octet_t protob_gtp_tlv_services_authorized_t;
typedef protob_tlv_octet_t protob_gtp_tlv_bit_rate_t;
typedef protob_tlv_octet_t protob_gtp_tlv_pc5_qos_flow_t;

/* Structure for Group Infomration Element */
#pragma pack(4)
typedef struct protob_gtp_tlv_pc5_qos_parameters_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_pc5_qos_flow_t pc5_qos_flows;
    protob_gtp_tlv_bit_rate_t pc5_link_aggregated_bit_rates;
} protob_gtp_tlv_pc5_qos_parameters_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_remote_ue_context_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_remote_user_id_t remote_user_id;
    protob_gtp_tlv_remote_ue_ip_information_t remote_ue_ip_information;
} protob_gtp_tlv_remote_ue_context_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_v2x_context_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_services_authorized_t lte_v2x_services_authorized;
    protob_gtp_tlv_services_authorized_t nr_v2x_services_authorized;
    protob_gtp_tlv_bit_rate_t lte_ue_sidelink_aggregate_maximum_bit_rate;
    protob_gtp_tlv_bit_rate_t nr_ue_sidelink_aggregate_maximum_bit_rate;
    protob_gtp_tlv_pc5_qos_parameters_t pc5_qos_parameters;
} protob_gtp_tlv_v2x_context_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_bearer_context_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_ebi_t eps_bearer_id;
    protob_gtp_tlv_bearer_tft_t tft;
    protob_gtp_tlv_f_teid_t s1_u_enodeb_f_teid; /* Instance : 0 */
    protob_gtp_tlv_f_teid_t s4_u_sgsn_f_teid; /* Instance : 1 */
    protob_gtp_tlv_f_teid_t s5_s8_u_sgw_f_teid; /* Instance : 2 */
    protob_gtp_tlv_f_teid_t s5_s8_u_pgw_f_teid; /* Instance : 3 */
    protob_gtp_tlv_f_teid_t s12_rnc_f_teid; /* Instance : 4 */
    protob_gtp_tlv_f_teid_t s2b_u_epdg_f_teid_5; /* Instance : 5 */
    protob_gtp_tlv_f_teid_t s2a_u_twan_f_teid_6; /* Instance : 6 */
    protob_gtp_tlv_bearer_qos_t bearer_level_qos;
    protob_gtp_tlv_f_teid_t s11_u_mme_f_teid; /* Instance : 7 */
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_charging_id_t charging_id;
    protob_gtp_tlv_bearer_flags_t bearer_flags;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
    protob_gtp_tlv_maximum_packet_loss_rate_t maximum_packet_loss_rate;
    protob_gtp_tlv_f_teid_t s2b_u_epdg_f_teid_8; /* Instance : 8 */
    protob_gtp_tlv_f_teid_t s2b_u_pgw_f_teid; /* Instance : 9 */
    protob_gtp_tlv_f_teid_t s2a_u_twan_f_teid_10; /* Instance : 10 */
    protob_gtp_tlv_f_teid_t s2a_u_pgw_f_teid; /* Instance : 11 */
    protob_gtp_tlv_ran_nas_cause_t ran_nas_cause;
    protob_gtp_tlv_apco_t additional_protocol_configuration_options;
    protob_gtp_tlv_f_container_t bss_container;
    protob_gtp_tlv_ti_t transaction_identifier;
    protob_gtp_tlv_packet_flow_id_t packet_flow_id;
} protob_gtp_tlv_bearer_context_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_pdn_connection_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_apn_t apn;
    protob_gtp_tlv_apn_restriction_t apn_restriction;
    protob_gtp_tlv_selection_mode_t selection_mode;
    protob_gtp_tlv_ip_address_t ipv4_address;
    protob_gtp_tlv_ip_address_t ipv6_address;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_f_teid_t pgw_s5_s8_ip_address_for_control_plane_or_pmip; /* Instance : 0 */
    protob_gtp_tlv_fqdn_t pgw_node_name;
    protob_gtp_tlv_bearer_context_t bearer_contexts_;
    protob_gtp_tlv_ambr_t aggregate_maximum_bit_rate;
    protob_gtp_tlv_charging_characteristics_t charging_characteristics;
    protob_gtp_tlv_change_reporting_action_t change_reporting_action;
    protob_gtp_tlv_csg_information_reporting_action_t csg_information_reporting_action;
    protob_gtp_tlv_enb_information_reporting_t hnb_information_reporting_;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_signalling_priority_indication_t signalling_priority_indication__;
    protob_gtp_tlv_change_to_report_flags_t change_to_report_flags;
    protob_gtp_tlv_fqdn_t local_home_network_id;
    protob_gtp_tlv_presence_reporting_area_action_t presence_reporting_area_action;
    protob_gtp_tlv_wlan_offloadability_indication_t wlan_offloadability_indication;
    protob_gtp_tlv_remote_ue_context_t remote_ue_context_connected;
    protob_gtp_tlv_pdn_type_t pdn_type;
    protob_gtp_tlv_header_compression_configuration_t header_compression_configuration;
} protob_gtp_tlv_pdn_connection_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_overload_control_information_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_sequence_number_t overload_control_sequence_number;
    protob_gtp_tlv_metric_t overload_reduction_metric;
    protob_gtp_tlv_epc_timer_t period_of_validity;
    protob_gtp_tlv_apn_t list_of_access_point_name;
} protob_gtp_tlv_overload_control_information_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_load_control_information_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_sequence_number_t load_control_sequence_number;
    protob_gtp_tlv_metric_t load_metric;
    protob_gtp_tlv_apn_and_relative_capacity_t list_of_apn_and_relative_capacity;
} protob_gtp_tlv_load_control_information_t;

#pragma pack(4)
typedef struct protob_gtp_tlv_scef_pdn_connection_s {
    protob_tlv_presence_t presence;
    protob_gtp_tlv_apn_t apn;
    protob_gtp_tlv_ebi_t default_eps_bearer_id;
    protob_gtp_tlv_node_identifier_t scef_id;
} protob_gtp_tlv_scef_pdn_connection_t;

/* Structure for Message */
#pragma pack(4)
typedef struct protob_gtp_echo_request_s {
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_node_features_t sending_node_features;
} protob_gtp_echo_request_t;

#pragma pack(4)
typedef struct protob_gtp_echo_response_s {
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_node_features_t sending_node_features;
} protob_gtp_echo_response_t;

#pragma pack(4)
typedef struct protob_gtp_create_session_request_s {
    protob_gtp_tlv_imsi_t imsi;
    protob_gtp_tlv_msisdn_t msisdn;
    protob_gtp_tlv_mei_t me_identity;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_serving_network_t serving_network;
    protob_gtp_tlv_rat_type_t rat_type;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_f_teid_t pgw_s5_s8_address_for_control_plane_or_pmip;
    protob_gtp_tlv_apn_t access_point_name;
    protob_gtp_tlv_selection_mode_t selection_mode;
    protob_gtp_tlv_pdn_type_t pdn_type;
    protob_gtp_tlv_paa_t pdn_address_allocation;
    protob_gtp_tlv_apn_restriction_t maximum_apn_restriction;
    protob_gtp_tlv_ambr_t aggregate_maximum_bit_rate;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_twmi_t trusted_wlan_mode_indication;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_created;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_removed;
    protob_gtp_tlv_trace_information_t trace_information;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_fq_csid_t mme_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_fq_csid_t epdg_fq_csid;
    protob_gtp_tlv_fq_csid_t twan_fq_csid;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_uci_t user_csg_information;
    protob_gtp_tlv_charging_characteristics_t charging_characteristics;
    protob_gtp_tlv_ldn_t mme_s4_sgsn_ldn;
    protob_gtp_tlv_ldn_t sgw_ldn;
    protob_gtp_tlv_ldn_t epdg_ldn;
    protob_gtp_tlv_ldn_t twan_ldn;
    protob_gtp_tlv_signalling_priority_indication_t signalling_priority_indication;
    protob_gtp_tlv_ip_address_t ue_local_ip_address;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_apco_t additional_protocol_configuration_options;
    protob_gtp_tlv_ip_address_t hnb_local_ip_address;
    protob_gtp_tlv_port_number_t hnb_udp_port;
    protob_gtp_tlv_ip_address_t mme_s4_sgsn_identifier;
    protob_gtp_tlv_twan_identifier_t twan_identifier;
    protob_gtp_tlv_ip_address_t epdg_ip_address;
    protob_gtp_tlv_cn_operator_selection_entity_t cn_operator_selection_entity;
    protob_gtp_tlv_presence_reporting_area_information_t presence_reporting_area_information;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_millisecond_time_stamp_t origination_time_stamp;
    protob_gtp_tlv_integer_number_t maximum_wait_time;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_remote_ue_context_t remote_ue_context_connected;
    protob_gtp_tlv_node_identifier_t _aaa_server_identifier;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
    protob_gtp_tlv_serving_plmn_rate_control_t serving_plmn_rate_control;
    protob_gtp_tlv_counter_t mo_exception_data_counter;
    protob_gtp_tlv_port_number_t ue_tcp_port;
    protob_gtp_tlv_mapped_ue_usage_type_t mapped_ue_usage_type;
    protob_gtp_tlv_uli_t user_location_information_for_sgw_;
    protob_gtp_tlv_fqdn_t sgw_u_node_name;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
    protob_gtp_tlv_up_function_selection_indication_flags_t up_function_selection_indication_flags;
    protob_gtp_tlv_apn_rate_control_status_t apn_rate_control_status;
} protob_gtp_create_session_request_t;

#pragma pack(4)
typedef struct protob_gtp_create_session_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_change_reporting_action_t change_reporting_action_;
    protob_gtp_tlv_csg_information_reporting_action_t csg_information_reporting_action;
    protob_gtp_tlv_enb_information_reporting_t hnb_information_reporting;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_f_teid_t pgw_s5_s8__s2a_s2b_f_teid_for_pmip_based_interface_or_for_gtp_based_control_plane_interface;
    protob_gtp_tlv_paa_t pdn_address_allocation;
    protob_gtp_tlv_apn_restriction_t apn_restriction;
    protob_gtp_tlv_ambr_t aggregate_maximum_bit_rate;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_bearer_context_t bearer_contexts_created;
    protob_gtp_tlv_bearer_context_t bearer_contexts_marked_for_removal;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_fqdn_t charging_gateway_name;
    protob_gtp_tlv_ip_address_t charging_gateway_address;
    protob_gtp_tlv_fq_csid_t pgw_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_ldn_t sgw_ldn;
    protob_gtp_tlv_ldn_t pgw_ldn;
    protob_gtp_tlv_epc_timer_t pgw_back_off_time;
    protob_gtp_tlv_apco_t additional_protocol_configuration_options;
    protob_gtp_tlv_ip4cp_t trusted_wlan_ipv4_parameters_;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_presence_reporting_area_action_t presence_reporting_area_action;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_charging_id_t pdn_connection_charging_id;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
} protob_gtp_create_session_response_t;

#pragma pack(4)
typedef struct protob_gtp_modify_bearer_request_s {
    protob_gtp_tlv_mei_t me_identity;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_serving_network_t serving_network;
    protob_gtp_tlv_rat_type_t rat_type;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_ambr_t aggregate_maximum_bit_rate;
    protob_gtp_tlv_delay_value_t delay_downlink_packet_notification_request;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_modified;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_removed;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_fq_csid_t mme_fq_csid;
    protob_gtp_tlv_uci_t user_csg_information;
    protob_gtp_tlv_ip_address_t ue_local_ip_address;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_ldn_t mme_s4_sgsn_ldn;
    protob_gtp_tlv_ldn_t sgw_ldn;
    protob_gtp_tlv_ip_address_t hnb_local_ip_address;
    protob_gtp_tlv_port_number_t hnb_udp_port;
    protob_gtp_tlv_ip_address_t mme_s4_sgsn_identifier;
    protob_gtp_tlv_cn_operator_selection_entity_t cn_operator_selection_entity;
    protob_gtp_tlv_presence_reporting_area_information_t presence_reporting_area_information;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t epdg_s_overload_control_information;
    protob_gtp_tlv_serving_plmn_rate_control_t serving_plmn_rate_control;
    protob_gtp_tlv_counter_t mo_exception_data_counter;
    protob_gtp_tlv_imsi_t imsi;
    protob_gtp_tlv_uli_t user_location_information_for_sgw_;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_modify_bearer_request_t;

#pragma pack(4)
typedef struct protob_gtp_modify_bearer_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_msisdn_t msisdn;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_apn_restriction_t apn_restriction;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_bearer_context_t bearer_contexts_modified;
    protob_gtp_tlv_bearer_context_t bearer_contexts_marked_for_removal;
    protob_gtp_tlv_change_reporting_action_t change_reporting_action;
    protob_gtp_tlv_csg_information_reporting_action_t csg_information_reporting_action;
    protob_gtp_tlv_enb_information_reporting_t hnb_information_reporting_;
    protob_gtp_tlv_fqdn_t charging_gateway_name;
    protob_gtp_tlv_ip_address_t charging_gateway_address;
    protob_gtp_tlv_fq_csid_t pgw_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_ldn_t sgw_ldn;
    protob_gtp_tlv_ldn_t pgw_ldn;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_presence_reporting_area_action_t presence_reporting_area_action;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_charging_id_t pdn_connection_charging_id;
} protob_gtp_modify_bearer_response_t;

#pragma pack(4)
typedef struct protob_gtp_delete_session_request_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_node_type_t originating_node;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_uli_timestamp_t uli_timestamp;
    protob_gtp_tlv_ran_nas_cause_t ran_nas_release_cause;
    protob_gtp_tlv_twan_identifier_t twan_identifier;
    protob_gtp_tlv_twan_identifier_timestamp_t twan_identifier_timestamp;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_ip_address_t ue_local_ip_address;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
    protob_gtp_tlv_port_number_t ue_tcp_port;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_delete_session_request_t;

#pragma pack(4)
typedef struct protob_gtp_delete_session_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
    protob_gtp_tlv_apn_rate_control_status_t apn_rate_control_status;
} protob_gtp_delete_session_response_t;

#pragma pack(4)
typedef struct protob_gtp_modify_bearer_command_s {
    protob_gtp_tlv_ambr_t apn_aggregate_maximum_bit_rate;
    protob_gtp_tlv_bearer_context_t bearer_context;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
} protob_gtp_modify_bearer_command_t;

#pragma pack(4)
typedef struct protob_gtp_modify_bearer_failure_indication_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
} protob_gtp_modify_bearer_failure_indication_t;

#pragma pack(4)
typedef struct protob_gtp_delete_bearer_command_s {
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_uli_timestamp_t uli_timestamp;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_delete_bearer_command_t;

#pragma pack(4)
typedef struct protob_gtp_delete_bearer_failure_indication_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_bearer_context_t bearer_context;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
} protob_gtp_delete_bearer_failure_indication_t;

#pragma pack(4)
typedef struct protob_gtp_bearer_resource_command_s {
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_pti_t procedure_transaction_id;
    protob_gtp_tlv_flow_qos_t flow_quality_of_service;
    protob_gtp_tlv_tad_t traffic_aggregate_description;
    protob_gtp_tlv_rat_type_t rat_type;
    protob_gtp_tlv_serving_network_t serving_network;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_ebi_t eps_bearer_id;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_f_teid_t s4_u_sgsn_f_teid;
    protob_gtp_tlv_f_teid_t s12_rnc_f_teid;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_signalling_priority_indication_t signalling_priority_indication__;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
} protob_gtp_bearer_resource_command_t;

#pragma pack(4)
typedef struct protob_gtp_bearer_resource_failure_indication_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_pti_t procedure_transaction_id;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_f_container_t nbifom_container;
} protob_gtp_bearer_resource_failure_indication_t;

#pragma pack(4)
typedef struct protob_gtp_downlink_data_notification_failure_indication_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_node_type_t originating_node;
    protob_gtp_tlv_imsi_t imsi;
} protob_gtp_downlink_data_notification_failure_indication_t;

#pragma pack(4)
typedef struct protob_gtp_create_bearer_request_s {
    protob_gtp_tlv_pti_t procedure_transaction_id;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_fq_csid_t pgw_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_change_reporting_action_t change_reporting_action;
    protob_gtp_tlv_csg_information_reporting_action_t csg_information_reporting_action;
    protob_gtp_tlv_enb_information_reporting_t hnb_information_reporting;
    protob_gtp_tlv_presence_reporting_area_action_t presence_reporting_area_action;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_container_t nbifom_container;
} protob_gtp_create_bearer_request_t;

#pragma pack(4)
typedef struct protob_gtp_create_bearer_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_fq_csid_t mme_fq_csid;
    protob_gtp_tlv_fq_csid_t epdg_fq_csid;
    protob_gtp_tlv_fq_csid_t twan_fq_csid;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_twan_identifier_t twan_identifier;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_presence_reporting_area_information_t presence_reporting_area_information;
    protob_gtp_tlv_ip_address_t mme_s4_sgsn_identifier;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_port_number_t ue_tcp_port;
} protob_gtp_create_bearer_response_t;

#pragma pack(4)
typedef struct protob_gtp_update_bearer_request_s {
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_pti_t procedure_transaction_id;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_ambr_t aggregate_maximum_bit_rate;
    protob_gtp_tlv_change_reporting_action_t change_reporting_action;
    protob_gtp_tlv_csg_information_reporting_action_t csg_information_reporting_action;
    protob_gtp_tlv_enb_information_reporting_t hnb_information_reporting_;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_fq_csid_t pgw_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_presence_reporting_area_action_t presence_reporting_area_action;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_container_t nbifom_container;
} protob_gtp_update_bearer_request_t;

#pragma pack(4)
typedef struct protob_gtp_update_bearer_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_fq_csid_t mme_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_fq_csid_t epdg_fq_csid;
    protob_gtp_tlv_fq_csid_t twan_fq_csid;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_twan_identifier_t twan_identifier;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_presence_reporting_area_information_t presence_reporting_area_information;
    protob_gtp_tlv_ip_address_t mme_s4_sgsn_identifier;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_port_number_t ue_tcp_port;
} protob_gtp_update_bearer_response_t;

#pragma pack(4)
typedef struct protob_gtp_delete_bearer_request_s {
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_ebi_t eps_bearer_ids;
    protob_gtp_tlv_bearer_context_t failed_bearer_contexts;
    protob_gtp_tlv_pti_t procedure_transaction_id;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_fq_csid_t pgw_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t pgw_s_node_level_load_control_information;
    protob_gtp_tlv_load_control_information_t pgw_s_apn_level_load_control_information;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t pgw_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_apn_rate_control_status_t apn_rate_control_status;
    protob_gtp_tlv_epco_t extended_protocol_configuration_options;
} protob_gtp_delete_bearer_request_t;

#pragma pack(4)
typedef struct protob_gtp_delete_bearer_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_ebi_t linked_eps_bearer_id;
    protob_gtp_tlv_bearer_context_t bearer_contexts;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_fq_csid_t mme_fq_csid;
    protob_gtp_tlv_fq_csid_t sgw_fq_csid;
    protob_gtp_tlv_fq_csid_t epdg_fq_csid;
    protob_gtp_tlv_fq_csid_t twan_fq_csid;
    protob_gtp_tlv_pco_t protocol_configuration_options;
    protob_gtp_tlv_ue_time_zone_t ue_time_zone;
    protob_gtp_tlv_uli_t user_location_information;
    protob_gtp_tlv_uli_timestamp_t uli_timestamp;
    protob_gtp_tlv_twan_identifier_t twan_identifier;
    protob_gtp_tlv_twan_identifier_timestamp_t twan_identifier_timestamp;
    protob_gtp_tlv_overload_control_information_t mme_s4_sgsn_s_overload_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_ip_address_t mme_s4_sgsn_identifier;
    protob_gtp_tlv_overload_control_information_t twan_epdg_s_overload_control_information;
    protob_gtp_tlv_twan_identifier_t wlan_location_information;
    protob_gtp_tlv_twan_identifier_timestamp_t wlan_location_timestamp;
    protob_gtp_tlv_port_number_t ue_udp_port;
    protob_gtp_tlv_f_container_t nbifom_container;
    protob_gtp_tlv_port_number_t ue_tcp_port;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_delete_bearer_response_t;

#pragma pack(4)
typedef struct protob_gtp_create_indirect_data_forwarding_tunnel_request_s {
    protob_gtp_tlv_imsi_t imsi;
    protob_gtp_tlv_mei_t me_identity;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_bearer_context_t bearer_contexts[8];
    protob_gtp_tlv_recovery_t recovery;
} protob_gtp_create_indirect_data_forwarding_tunnel_request_t;

#pragma pack(4)
typedef struct protob_gtp_create_indirect_data_forwarding_tunnel_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_bearer_context_t bearer_contexts[8];
    protob_gtp_tlv_recovery_t recovery;
} protob_gtp_create_indirect_data_forwarding_tunnel_response_t;

#pragma pack(4)
typedef struct protob_gtp_delete_indirect_data_forwarding_tunnel_request_s {
} protob_gtp_delete_indirect_data_forwarding_tunnel_request_t;

#pragma pack(4)
typedef struct protob_gtp_delete_indirect_data_forwarding_tunnel_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_recovery_t recovery;
} protob_gtp_delete_indirect_data_forwarding_tunnel_response_t;

#pragma pack(4)
typedef struct protob_gtp_release_access_bearers_request_s {
    protob_gtp_tlv_ebi_t list_of_rabs;
    protob_gtp_tlv_node_type_t originating_node;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_release_access_bearers_request_t;

#pragma pack(4)
typedef struct protob_gtp_release_access_bearers_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
} protob_gtp_release_access_bearers_response_t;

#pragma pack(4)
typedef struct protob_gtp_downlink_data_notification_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_ebi_t eps_bearer_id;
    protob_gtp_tlv_arp_t allocation_retention_priority;
    protob_gtp_tlv_imsi_t imsi;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
    protob_gtp_tlv_paging_and_service_information_t paging_and_service_information;
    protob_gtp_tlv_integer_number_t dl_data_packets_size;
} protob_gtp_downlink_data_notification_t;

#pragma pack(4)
typedef struct protob_gtp_downlink_data_notification_acknowledge_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_delay_value_t data_notification_delay;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_throttling_t dl_low_priority_traffic_throttling_;
    protob_gtp_tlv_imsi_t imsi;
    protob_gtp_tlv_epc_timer_t dl_buffering_duration;
    protob_gtp_tlv_integer_number_t dl_buffering_suggested_packet_count;
} protob_gtp_downlink_data_notification_acknowledge_t;

#pragma pack(4)
typedef struct protob_gtp_modify_access_bearers_request_s {
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_f_teid_t sender_f_teid_for_control_plane;
    protob_gtp_tlv_delay_value_t delay_downlink_packet_notification_request;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_modified;
    protob_gtp_tlv_bearer_context_t bearer_contexts_to_be_removed;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_secondary_rat_usage_data_report_t secondary_rat_usage_data_report;
} protob_gtp_modify_access_bearers_request_t;

#pragma pack(4)
typedef struct protob_gtp_modify_access_bearers_response_s {
    protob_gtp_tlv_cause_t cause;
    protob_gtp_tlv_bearer_context_t bearer_contexts_modified;
    protob_gtp_tlv_bearer_context_t bearer_contexts_marked_for_removal;
    protob_gtp_tlv_recovery_t recovery;
    protob_gtp_tlv_indication_t indication_flags;
    protob_gtp_tlv_load_control_information_t sgw_s_node_level_load_control_information;
    protob_gtp_tlv_overload_control_information_t sgw_s_overload_control_information;
} protob_gtp_modify_access_bearers_response_t;

#pragma pack(4)
typedef struct protob_gtp_message_s {
   protob_gtp_header_t h;
   union {
        protob_gtp_echo_request_t echo_request;
        protob_gtp_echo_response_t echo_response;
        protob_gtp_create_session_request_t create_session_request;
        protob_gtp_create_session_response_t create_session_response;
        protob_gtp_modify_bearer_request_t modify_bearer_request;
        protob_gtp_modify_bearer_response_t modify_bearer_response;
        protob_gtp_delete_session_request_t delete_session_request;
        protob_gtp_delete_session_response_t delete_session_response;
        protob_gtp_modify_bearer_command_t modify_bearer_command;
        protob_gtp_modify_bearer_failure_indication_t modify_bearer_failure_indication;
        protob_gtp_delete_bearer_command_t delete_bearer_command;
        protob_gtp_delete_bearer_failure_indication_t delete_bearer_failure_indication;
        protob_gtp_bearer_resource_command_t bearer_resource_command;
        protob_gtp_bearer_resource_failure_indication_t bearer_resource_failure_indication;
        protob_gtp_downlink_data_notification_failure_indication_t downlink_data_notification_failure_indication;
        protob_gtp_create_bearer_request_t create_bearer_request;
        protob_gtp_create_bearer_response_t create_bearer_response;
        protob_gtp_update_bearer_request_t update_bearer_request;
        protob_gtp_update_bearer_response_t update_bearer_response;
        protob_gtp_delete_bearer_request_t delete_bearer_request;
        protob_gtp_delete_bearer_response_t delete_bearer_response;
        protob_gtp_create_indirect_data_forwarding_tunnel_request_t create_indirect_data_forwarding_tunnel_request;
        protob_gtp_create_indirect_data_forwarding_tunnel_response_t create_indirect_data_forwarding_tunnel_response;
        protob_gtp_delete_indirect_data_forwarding_tunnel_request_t delete_indirect_data_forwarding_tunnel_request;
        protob_gtp_delete_indirect_data_forwarding_tunnel_response_t delete_indirect_data_forwarding_tunnel_response;
        protob_gtp_release_access_bearers_request_t release_access_bearers_request;
        protob_gtp_release_access_bearers_response_t release_access_bearers_response;
        protob_gtp_downlink_data_notification_t downlink_data_notification;
        protob_gtp_downlink_data_notification_acknowledge_t downlink_data_notification_acknowledge;
        protob_gtp_modify_access_bearers_request_t modify_access_bearers_request;
        protob_gtp_modify_access_bearers_response_t modify_access_bearers_response;
   };
} protob_gtp_message_t;

int protob_gtp_parse_msg( protob_gtp_message_t * gtp_message, protob_pkbuf_t * pkbuf);
protob_pkbuf_t * protob_gtp_build_msg(protob_gtp_message_t *gtp_message);








