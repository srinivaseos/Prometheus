#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "protob_tlv.h"
#include "protob_tlv_message.h"
#include "protob_gtp.h"

protob_tlv_desc_t protob_gtp_tlv_desc_imsi_0 =
{
    PROTOB_TLV_VAR_STR,
    "IMSI",
    PROTOB_GTP_IMSI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_imsi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_cause_0 =
{
    PROTOB_TLV_VAR_STR,
    "Cause",
    PROTOB_GTP_CAUSE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_cause_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_recovery_0 =
{
    PROTOB_TLV_UINT8,
    "Recovery",
    PROTOB_GTP_RECOVERY_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_recovery_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_stn_sr_0 =
{
    PROTOB_TLV_VAR_STR,
    "STN-SR",
    PROTOB_GTP_STN_SR_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_stn_sr_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_apn_0 =
{
    PROTOB_TLV_VAR_STR,
    "APN",
    PROTOB_GTP_APN_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_apn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ambr_0 =
{
    PROTOB_TLV_VAR_STR,
    "AMBR",
    PROTOB_GTP_AMBR_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ambr_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ebi_0 =
{
    PROTOB_TLV_UINT8,
    "EBI",
    PROTOB_GTP_EBI_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_ebi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ebi_1 =
{
    PROTOB_TLV_UINT8,
    "EBI",
    PROTOB_GTP_EBI_TYPE,
    1,
    1,
    sizeof(protob_gtp_tlv_ebi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_0 =
{
    PROTOB_TLV_VAR_STR,
    "IP Address",
    PROTOB_GTP_IP_ADDRESS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ip_address_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_1 =
{
    PROTOB_TLV_VAR_STR,
    "IP Address",
    PROTOB_GTP_IP_ADDRESS_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_ip_address_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_2 =
{
    PROTOB_TLV_VAR_STR,
    "IP Address",
    PROTOB_GTP_IP_ADDRESS_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_ip_address_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ip_address_3 =
{
    PROTOB_TLV_VAR_STR,
    "IP Address",
    PROTOB_GTP_IP_ADDRESS_TYPE,
    0,
    3,
    sizeof(protob_gtp_tlv_ip_address_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mei_0 =
{
    PROTOB_TLV_VAR_STR,
    "MEI",
    PROTOB_GTP_MEI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mei_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_msisdn_0 =
{
    PROTOB_TLV_VAR_STR,
    "MSISDN",
    PROTOB_GTP_MSISDN_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_msisdn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_indication_0 =
{
    PROTOB_TLV_VAR_STR,
    "Indication",
    PROTOB_GTP_INDICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_indication_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pco_0 =
{
    PROTOB_TLV_VAR_STR,
    "PCO",
    PROTOB_GTP_PCO_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_pco_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_paa_0 =
{
    PROTOB_TLV_VAR_STR,
    "PAA",
    PROTOB_GTP_PAA_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_paa_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_qos_0 =
{
    PROTOB_TLV_VAR_STR,
    "Bearer QoS",
    PROTOB_GTP_BEARER_QOS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_bearer_qos_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_flow_qos_0 =
{
    PROTOB_TLV_VAR_STR,
    "Flow QoS",
    PROTOB_GTP_FLOW_QOS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_flow_qos_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_rat_type_0 =
{
    PROTOB_TLV_UINT8,
    "RAT Type",
    PROTOB_GTP_RAT_TYPE_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_rat_type_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_serving_network_0 =
{
    PROTOB_TLV_VAR_STR,
    "Serving Network",
    PROTOB_GTP_SERVING_NETWORK_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_serving_network_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_tft_0 =
{
    PROTOB_TLV_VAR_STR,
    "Bearer TFT",
    PROTOB_GTP_BEARER_TFT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_bearer_tft_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_tad_0 =
{
    PROTOB_TLV_VAR_STR,
    "TAD",
    PROTOB_GTP_TAD_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_tad_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_uli_0 =
{
    PROTOB_TLV_VAR_STR,
    "ULI",
    PROTOB_GTP_ULI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_uli_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_uli_1 =
{
    PROTOB_TLV_VAR_STR,
    "ULI",
    PROTOB_GTP_ULI_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_uli_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_0 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_1 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_2 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_3 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    3,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_4 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    4,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_5 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    5,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_6 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    6,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_7 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    7,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_8 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    8,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_9 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    9,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_10 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    10,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_teid_11 =
{
    PROTOB_TLV_VAR_STR,
    "F-TEID",
    PROTOB_GTP_F_TEID_TYPE,
    0,
    11,
    sizeof(protob_gtp_tlv_f_teid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_tmsi_0 =
{
    PROTOB_TLV_VAR_STR,
    "TMSI",
    PROTOB_GTP_TMSI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_tmsi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_global_cn_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "Global CN-Id",
    PROTOB_GTP_GLOBAL_CN_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_global_cn_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_s103pdf_0 =
{
    PROTOB_TLV_VAR_STR,
    "S103PDF",
    PROTOB_GTP_S103PDF_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_s103pdf_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_s1udf_0 =
{
    PROTOB_TLV_VAR_STR,
    "S1UDF",
    PROTOB_GTP_S1UDF_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_s1udf_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_delay_value_0 =
{
    PROTOB_TLV_VAR_STR,
    "Delay Value",
    PROTOB_GTP_DELAY_VALUE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_delay_value_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_charging_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "Charging ID",
    PROTOB_GTP_CHARGING_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_charging_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_charging_characteristics_0 =
{
    PROTOB_TLV_VAR_STR,
    "Charging Characteristics",
    PROTOB_GTP_CHARGING_CHARACTERISTICS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_charging_characteristics_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_trace_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Trace Information",
    PROTOB_GTP_TRACE_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_trace_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_flags_0 =
{
    PROTOB_TLV_VAR_STR,
    "Bearer Flags",
    PROTOB_GTP_BEARER_FLAGS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_bearer_flags_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pdn_type_0 =
{
    PROTOB_TLV_UINT8,
    "PDN Type",
    PROTOB_GTP_PDN_TYPE_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_pdn_type_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pti_0 =
{
    PROTOB_TLV_UINT8,
    "PTI",
    PROTOB_GTP_PTI_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_pti_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mm_context_0 =
{
    PROTOB_TLV_VAR_STR,
    "MM Context",
    PROTOB_GTP_MM_CONTEXT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mm_context_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pdu_numbers_0 =
{
    PROTOB_TLV_VAR_STR,
    "PDU Numbers",
    PROTOB_GTP_PDU_NUMBERS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_pdu_numbers_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_p_tmsi_0 =
{
    PROTOB_TLV_VAR_STR,
    "P-TMSI",
    PROTOB_GTP_P_TMSI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_p_tmsi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_p_tmsi_signature_0 =
{
    PROTOB_TLV_VAR_STR,
    "P-TMSI Signature",
    PROTOB_GTP_P_TMSI_SIGNATURE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_p_tmsi_signature_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_hop_counter_0 =
{
    PROTOB_TLV_VAR_STR,
    "Hop Counter",
    PROTOB_GTP_HOP_COUNTER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_hop_counter_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ue_time_zone_0 =
{
    PROTOB_TLV_VAR_STR,
    "UE Time Zone",
    PROTOB_GTP_UE_TIME_ZONE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ue_time_zone_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_trace_reference_0 =
{
    PROTOB_TLV_VAR_STR,
    "Trace Reference",
    PROTOB_GTP_TRACE_REFERENCE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_trace_reference_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_complete_request_message_0 =
{
    PROTOB_TLV_VAR_STR,
    "Complete Request Message",
    PROTOB_GTP_COMPLETE_REQUEST_MESSAGE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_complete_request_message_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_guti_0 =
{
    PROTOB_TLV_VAR_STR,
    "GUTI",
    PROTOB_GTP_GUTI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_guti_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_container_0 =
{
    PROTOB_TLV_VAR_STR,
    "F-Container",
    PROTOB_GTP_F_CONTAINER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_f_container_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_f_cause_0 =
{
    PROTOB_TLV_VAR_STR,
    "F-Cause",
    PROTOB_GTP_F_CAUSE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_f_cause_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_plmn_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "PLMN ID",
    PROTOB_GTP_PLMN_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_plmn_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_target_identification_0 =
{
    PROTOB_TLV_VAR_STR,
    "Target Identification",
    PROTOB_GTP_TARGET_IDENTIFICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_target_identification_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_packet_flow_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "Packet Flow ID",
    PROTOB_GTP_PACKET_FLOW_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_packet_flow_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_rab_context_0 =
{
    PROTOB_TLV_VAR_STR,
    "RAB Context",
    PROTOB_GTP_RAB_CONTEXT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_rab_context_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_source_rnc_pdcp_context_info_0 =
{
    PROTOB_TLV_VAR_STR,
    "Source RNC PDCP Context Info",
    PROTOB_GTP_SOURCE_RNC_PDCP_CONTEXT_INFO_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_source_rnc_pdcp_context_info_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_port_number_0 =
{
    PROTOB_TLV_UINT16,
    "Port Number",
    PROTOB_GTP_PORT_NUMBER_TYPE,
    2,
    0,
    sizeof(protob_gtp_tlv_port_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_port_number_1 =
{
    PROTOB_TLV_UINT16,
    "Port Number",
    PROTOB_GTP_PORT_NUMBER_TYPE,
    2,
    1,
    sizeof(protob_gtp_tlv_port_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_port_number_2 =
{
    PROTOB_TLV_UINT16,
    "Port Number",
    PROTOB_GTP_PORT_NUMBER_TYPE,
    2,
    2,
    sizeof(protob_gtp_tlv_port_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_apn_restriction_0 =
{
    PROTOB_TLV_UINT8,
    "APN Restriction",
    PROTOB_GTP_APN_RESTRICTION_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_apn_restriction_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_selection_mode_0 =
{
    PROTOB_TLV_UINT8,
    "Selection Mode",
    PROTOB_GTP_SELECTION_MODE_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_selection_mode_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_source_identification_0 =
{
    PROTOB_TLV_VAR_STR,
    "Source Identification",
    PROTOB_GTP_SOURCE_IDENTIFICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_source_identification_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_change_reporting_action_0 =
{
    PROTOB_TLV_VAR_STR,
    "Change Reporting Action",
    PROTOB_GTP_CHANGE_REPORTING_ACTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_change_reporting_action_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_0 =
{
    PROTOB_TLV_VAR_STR,
    "FQ-CSID",
    PROTOB_GTP_FQ_CSID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_fq_csid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_1 =
{
    PROTOB_TLV_VAR_STR,
    "FQ-CSID",
    PROTOB_GTP_FQ_CSID_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_fq_csid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_2 =
{
    PROTOB_TLV_VAR_STR,
    "FQ-CSID",
    PROTOB_GTP_FQ_CSID_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_fq_csid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fq_csid_3 =
{
    PROTOB_TLV_VAR_STR,
    "FQ-CSID",
    PROTOB_GTP_FQ_CSID_TYPE,
    0,
    3,
    sizeof(protob_gtp_tlv_fq_csid_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_channel_needed_0 =
{
    PROTOB_TLV_VAR_STR,
    "Channel needed",
    PROTOB_GTP_CHANNEL_NEEDED_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_channel_needed_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_emlpp_priority_0 =
{
    PROTOB_TLV_VAR_STR,
    "eMLPP Priority",
    PROTOB_GTP_EMLPP_PRIORITY_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_emlpp_priority_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_node_type_0 =
{
    PROTOB_TLV_UINT8,
    "Node Type",
    PROTOB_GTP_NODE_TYPE_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_node_type_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fqdn_0 =
{
    PROTOB_TLV_VAR_STR,
    "FQDN",
    PROTOB_GTP_FQDN_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_fqdn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_fqdn_1 =
{
    PROTOB_TLV_VAR_STR,
    "FQDN",
    PROTOB_GTP_FQDN_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_fqdn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ti_0 =
{
    PROTOB_TLV_VAR_STR,
    "TI",
    PROTOB_GTP_TI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ti_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_session_duration_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Session Duration",
    PROTOB_GTP_MBMS_SESSION_DURATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_session_duration_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_service_area_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Service Area",
    PROTOB_GTP_MBMS_SERVICE_AREA_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_service_area_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_session_identifier_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Session Identifier",
    PROTOB_GTP_MBMS_SESSION_IDENTIFIER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_session_identifier_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_flow_identifier_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Flow Identifier",
    PROTOB_GTP_MBMS_FLOW_IDENTIFIER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_flow_identifier_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_ip_multicast_distribution_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS IP Multicast Distribution",
    PROTOB_GTP_MBMS_IP_MULTICAST_DISTRIBUTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_ip_multicast_distribution_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_distribution_acknowledge_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Distribution Acknowledge",
    PROTOB_GTP_MBMS_DISTRIBUTION_ACKNOWLEDGE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_distribution_acknowledge_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_rfsp_index_0 =
{
    PROTOB_TLV_VAR_STR,
    "RFSP Index",
    PROTOB_GTP_RFSP_INDEX_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_rfsp_index_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_uci_0 =
{
    PROTOB_TLV_VAR_STR,
    "UCI",
    PROTOB_GTP_UCI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_uci_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_csg_information_reporting_action_0 =
{
    PROTOB_TLV_VAR_STR,
    "CSG Information Reporting Action",
    PROTOB_GTP_CSG_INFORMATION_REPORTING_ACTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_csg_information_reporting_action_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_csg_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "CSG ID",
    PROTOB_GTP_CSG_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_csg_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_cmi_0 =
{
    PROTOB_TLV_VAR_STR,
    "CMI",
    PROTOB_GTP_CMI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_cmi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_service_indicator_0 =
{
    PROTOB_TLV_VAR_STR,
    "Service indicator",
    PROTOB_GTP_SERVICE_INDICATOR_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_service_indicator_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_detach_type_0 =
{
    PROTOB_TLV_VAR_STR,
    "Detach Type",
    PROTOB_GTP_DETACH_TYPE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_detach_type_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ldn_0 =
{
    PROTOB_TLV_VAR_STR,
    "LDN",
    PROTOB_GTP_LDN_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ldn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ldn_1 =
{
    PROTOB_TLV_VAR_STR,
    "LDN",
    PROTOB_GTP_LDN_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_ldn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ldn_2 =
{
    PROTOB_TLV_VAR_STR,
    "LDN",
    PROTOB_GTP_LDN_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_ldn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ldn_3 =
{
    PROTOB_TLV_VAR_STR,
    "LDN",
    PROTOB_GTP_LDN_TYPE,
    0,
    3,
    sizeof(protob_gtp_tlv_ldn_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_node_features_0 =
{
    PROTOB_TLV_UINT8,
    "Node Features",
    PROTOB_GTP_NODE_FEATURES_TYPE,
    1,
    0,
    sizeof(protob_gtp_tlv_node_features_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_time_to_data_transfer_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Time to Data Transfer",
    PROTOB_GTP_MBMS_TIME_TO_DATA_TRANSFER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_time_to_data_transfer_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_throttling_0 =
{
    PROTOB_TLV_VAR_STR,
    "Throttling",
    PROTOB_GTP_THROTTLING_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_throttling_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_arp_0 =
{
    PROTOB_TLV_VAR_STR,
    "ARP",
    PROTOB_GTP_ARP_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_arp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_epc_timer_0 =
{
    PROTOB_TLV_VAR_STR,
    "EPC Timer",
    PROTOB_GTP_EPC_TIMER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_epc_timer_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_signalling_priority_indication_0 =
{
    PROTOB_TLV_VAR_STR,
    "Signalling Priority Indication",
    PROTOB_GTP_SIGNALLING_PRIORITY_INDICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_signalling_priority_indication_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_tmgi_0 =
{
    PROTOB_TLV_VAR_STR,
    "TMGI",
    PROTOB_GTP_TMGI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_tmgi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_additional_mm_context_for_srvcc_0 =
{
    PROTOB_TLV_VAR_STR,
    "Additional MM context for SRVCC",
    PROTOB_GTP_ADDITIONAL_MM_CONTEXT_FOR_SRVCC_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_additional_mm_context_for_srvcc_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_additional_flags_for_srvcc_0 =
{
    PROTOB_TLV_VAR_STR,
    "Additional flags for SRVCC",
    PROTOB_GTP_ADDITIONAL_FLAGS_FOR_SRVCC_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_additional_flags_for_srvcc_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mdt_configuration_0 =
{
    PROTOB_TLV_VAR_STR,
    "MDT Configuration",
    PROTOB_GTP_MDT_CONFIGURATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mdt_configuration_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_apco_0 =
{
    PROTOB_TLV_VAR_STR,
    "APCO",
    PROTOB_GTP_APCO_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_apco_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_absolute_time_of_mbms_data_transfer_0 =
{
    PROTOB_TLV_VAR_STR,
    "Absolute Time of MBMS Data Transfer",
    PROTOB_GTP_ABSOLUTE_TIME_OF_MBMS_DATA_TRANSFER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_absolute_time_of_mbms_data_transfer_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_enb_information_reporting_0 =
{
    PROTOB_TLV_VAR_STR,
    "eNB Information Reporting",
    PROTOB_GTP_ENB_INFORMATION_REPORTING_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_enb_information_reporting_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ip4cp_0 =
{
    PROTOB_TLV_VAR_STR,
    "IP4CP",
    PROTOB_GTP_IP4CP_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ip4cp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_change_to_report_flags_0 =
{
    PROTOB_TLV_VAR_STR,
    "Change to Report Flags",
    PROTOB_GTP_CHANGE_TO_REPORT_FLAGS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_change_to_report_flags_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_action_indication_0 =
{
    PROTOB_TLV_VAR_STR,
    "Action Indication",
    PROTOB_GTP_ACTION_INDICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_action_indication_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_0 =
{
    PROTOB_TLV_VAR_STR,
    "TWAN Identifier",
    PROTOB_GTP_TWAN_IDENTIFIER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_twan_identifier_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_1 =
{
    PROTOB_TLV_VAR_STR,
    "TWAN Identifier",
    PROTOB_GTP_TWAN_IDENTIFIER_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_twan_identifier_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_uli_timestamp_0 =
{
    PROTOB_TLV_VAR_STR,
    "ULI Timestamp",
    PROTOB_GTP_ULI_TIMESTAMP_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_uli_timestamp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mbms_flags_0 =
{
    PROTOB_TLV_VAR_STR,
    "MBMS Flags",
    PROTOB_GTP_MBMS_FLAGS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mbms_flags_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ran_nas_cause_0 =
{
    PROTOB_TLV_VAR_STR,
    "RAN/NAS Cause",
    PROTOB_GTP_RAN_NAS_CAUSE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ran_nas_cause_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_cn_operator_selection_entity_0 =
{
    PROTOB_TLV_VAR_STR,
    "CN Operator Selection Entity",
    PROTOB_GTP_CN_OPERATOR_SELECTION_ENTITY_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_cn_operator_selection_entity_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_twmi_0 =
{
    PROTOB_TLV_VAR_STR,
    "TWMI",
    PROTOB_GTP_TWMI_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_twmi_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_node_number_0 =
{
    PROTOB_TLV_VAR_STR,
    "Node Number",
    PROTOB_GTP_NODE_NUMBER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_node_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_node_identifier_0 =
{
    PROTOB_TLV_VAR_STR,
    "Node Identifier",
    PROTOB_GTP_NODE_IDENTIFIER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_node_identifier_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_presence_reporting_area_action_0 =
{
    PROTOB_TLV_VAR_STR,
    "Presence Reporting Area Action",
    PROTOB_GTP_PRESENCE_REPORTING_AREA_ACTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_presence_reporting_area_action_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_presence_reporting_area_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Presence Reporting Area Information",
    PROTOB_GTP_PRESENCE_REPORTING_AREA_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_presence_reporting_area_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_timestamp_0 =
{
    PROTOB_TLV_VAR_STR,
    "TWAN Identifier Timestamp",
    PROTOB_GTP_TWAN_IDENTIFIER_TIMESTAMP_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_twan_identifier_timestamp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_twan_identifier_timestamp_1 =
{
    PROTOB_TLV_VAR_STR,
    "TWAN Identifier Timestamp",
    PROTOB_GTP_TWAN_IDENTIFIER_TIMESTAMP_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_twan_identifier_timestamp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_metric_0 =
{
    PROTOB_TLV_VAR_STR,
    "Metric",
    PROTOB_GTP_METRIC_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_metric_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_sequence_number_0 =
{
    PROTOB_TLV_VAR_STR,
    "Sequence Number",
    PROTOB_GTP_SEQUENCE_NUMBER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_sequence_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_apn_and_relative_capacity_0 =
{
    PROTOB_TLV_VAR_STR,
    "APN and Relative Capacity",
    PROTOB_GTP_APN_AND_RELATIVE_CAPACITY_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_apn_and_relative_capacity_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_wlan_offloadability_indication_0 =
{
    PROTOB_TLV_VAR_STR,
    "WLAN Offloadability Indication",
    PROTOB_GTP_WLAN_OFFLOADABILITY_INDICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_wlan_offloadability_indication_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_paging_and_service_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Paging and Service Information",
    PROTOB_GTP_PAGING_AND_SERVICE_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_paging_and_service_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_integer_number_0 =
{
    PROTOB_TLV_VAR_STR,
    "Integer Number",
    PROTOB_GTP_INTEGER_NUMBER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_integer_number_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_millisecond_time_stamp_0 =
{
    PROTOB_TLV_VAR_STR,
    "Millisecond Time Stamp",
    PROTOB_GTP_MILLISECOND_TIME_STAMP_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_millisecond_time_stamp_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_monitoring_event_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Monitoring Event Information",
    PROTOB_GTP_MONITORING_EVENT_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_monitoring_event_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ecgi_list_0 =
{
    PROTOB_TLV_VAR_STR,
    "ECGI List",
    PROTOB_GTP_ECGI_LIST_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ecgi_list_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_remote_user_id_0 =
{
    PROTOB_TLV_VAR_STR,
    "Remote User ID",
    PROTOB_GTP_REMOTE_USER_ID_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_remote_user_id_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_ip_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Remote UE IP Information",
    PROTOB_GTP_REMOTE_UE_IP_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_remote_ue_ip_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_ciot_optimizations_support_indication_0 =
{
    PROTOB_TLV_VAR_STR,
    "CIoT Optimizations Support Indication",
    PROTOB_GTP_CIOT_OPTIMIZATIONS_SUPPORT_INDICATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_ciot_optimizations_support_indication_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_header_compression_configuration_0 =
{
    PROTOB_TLV_VAR_STR,
    "Header Compression Configuration",
    PROTOB_GTP_HEADER_COMPRESSION_CONFIGURATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_header_compression_configuration_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_epco_0 =
{
    PROTOB_TLV_VAR_STR,
    "ePCO",
    PROTOB_GTP_EPCO_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_epco_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_serving_plmn_rate_control_0 =
{
    PROTOB_TLV_VAR_STR,
    "Serving PLMN Rate Control",
    PROTOB_GTP_SERVING_PLMN_RATE_CONTROL_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_serving_plmn_rate_control_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_counter_0 =
{
    PROTOB_TLV_VAR_STR,
    "Counter",
    PROTOB_GTP_COUNTER_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_counter_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_mapped_ue_usage_type_0 =
{
    PROTOB_TLV_VAR_STR,
    "Mapped UE Usage Type",
    PROTOB_GTP_MAPPED_UE_USAGE_TYPE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_mapped_ue_usage_type_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_secondary_rat_usage_data_report_0 =
{
    PROTOB_TLV_VAR_STR,
    "Secondary RAT Usage Data Report",
    PROTOB_GTP_SECONDARY_RAT_USAGE_DATA_REPORT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_secondary_rat_usage_data_report_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_up_function_selection_indication_flags_0 =
{
    PROTOB_TLV_VAR_STR,
    "UP Function Selection Indication Flags",
    PROTOB_GTP_UP_FUNCTION_SELECTION_INDICATION_FLAGS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_up_function_selection_indication_flags_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_maximum_packet_loss_rate_0 =
{
    PROTOB_TLV_VAR_STR,
    "Maximum Packet Loss Rate",
    PROTOB_GTP_MAXIMUM_PACKET_LOSS_RATE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_maximum_packet_loss_rate_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_apn_rate_control_status_0 =
{
    PROTOB_TLV_VAR_STR,
    "APN Rate Control Status",
    PROTOB_GTP_APN_RATE_CONTROL_STATUS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_apn_rate_control_status_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_extended_trace_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Extended Trace Information",
    PROTOB_GTP_EXTENDED_TRACE_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_extended_trace_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_monitoring_event_extension_information_0 =
{
    PROTOB_TLV_VAR_STR,
    "Monitoring Event Extension Information",
    PROTOB_GTP_MONITORING_EVENT_EXTENSION_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_monitoring_event_extension_information_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_additional_rrm_policy_index_0 =
{
    PROTOB_TLV_VAR_STR,
    "Additional RRM Policy Index",
    PROTOB_GTP_ADDITIONAL_RRM_POLICY_INDEX_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_additional_rrm_policy_index_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_services_authorized_0 =
{
    PROTOB_TLV_VAR_STR,
    "Services Authorized",
    PROTOB_GTP_SERVICES_AUTHORIZED_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_services_authorized_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_services_authorized_1 =
{
    PROTOB_TLV_VAR_STR,
    "Services Authorized",
    PROTOB_GTP_SERVICES_AUTHORIZED_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_services_authorized_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bit_rate_0 =
{
    PROTOB_TLV_VAR_STR,
    "Bit Rate",
    PROTOB_GTP_BIT_RATE_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_bit_rate_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bit_rate_1 =
{
    PROTOB_TLV_VAR_STR,
    "Bit Rate",
    PROTOB_GTP_BIT_RATE_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_bit_rate_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pc5_qos_flow_0 =
{
    PROTOB_TLV_VAR_STR,
    "PC5 QoS Flow",
    PROTOB_GTP_PC5_QOS_FLOW_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_pc5_qos_flow_t),
    { NULL }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pc5_qos_parameters_0 =
{
    PROTOB_TLV_COMPOUND,
    "PC5 QoS Parameters",
    PROTOB_GTP_PC5_QOS_PARAMETERS_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_pc5_qos_parameters_t),
    {
        &protob_gtp_tlv_desc_pc5_qos_flow_0,
        &protob_gtp_tlv_desc_bit_rate_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_remote_ue_context_0 =
{
    PROTOB_TLV_COMPOUND,
    "Remote UE Context",
    PROTOB_GTP_REMOTE_UE_CONTEXT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_remote_ue_context_t),
    {
        &protob_gtp_tlv_desc_remote_user_id_0,
        &protob_gtp_tlv_desc_remote_ue_ip_information_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_v2x_context_0 =
{
    PROTOB_TLV_COMPOUND,
    "V2X Context",
    PROTOB_GTP_V2X_CONTEXT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_v2x_context_t),
    {
        &protob_gtp_tlv_desc_services_authorized_0,
        &protob_gtp_tlv_desc_services_authorized_1,
        &protob_gtp_tlv_desc_bit_rate_0,
        &protob_gtp_tlv_desc_bit_rate_1,
        &protob_gtp_tlv_desc_pc5_qos_parameters_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_context_0 =
{
    PROTOB_TLV_COMPOUND,
    "Bearer Context",
    PROTOB_GTP_BEARER_CONTEXT_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_bearer_context_t),
    {
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_bearer_tft_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_f_teid_1,
        &protob_gtp_tlv_desc_f_teid_2,
        &protob_gtp_tlv_desc_f_teid_3,
        &protob_gtp_tlv_desc_f_teid_4,
        &protob_gtp_tlv_desc_f_teid_5,
        &protob_gtp_tlv_desc_f_teid_6,
        &protob_gtp_tlv_desc_bearer_qos_0,
        &protob_gtp_tlv_desc_f_teid_7,
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_charging_id_0,
        &protob_gtp_tlv_desc_bearer_flags_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_maximum_packet_loss_rate_0,
        &protob_gtp_tlv_desc_f_teid_8,
        &protob_gtp_tlv_desc_f_teid_9,
        &protob_gtp_tlv_desc_f_teid_10,
        &protob_gtp_tlv_desc_f_teid_11,
        &protob_gtp_tlv_desc_ran_nas_cause_0,
        &protob_gtp_tlv_desc_apco_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_ti_0,
        &protob_gtp_tlv_desc_packet_flow_id_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_context_1 =
{
    PROTOB_TLV_COMPOUND,
    "Bearer Context",
    PROTOB_GTP_BEARER_CONTEXT_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_bearer_context_t),
    {
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_bearer_tft_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_f_teid_1,
        &protob_gtp_tlv_desc_f_teid_2,
        &protob_gtp_tlv_desc_f_teid_3,
        &protob_gtp_tlv_desc_f_teid_4,
        &protob_gtp_tlv_desc_f_teid_5,
        &protob_gtp_tlv_desc_f_teid_6,
        &protob_gtp_tlv_desc_bearer_qos_0,
        &protob_gtp_tlv_desc_f_teid_7,
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_charging_id_0,
        &protob_gtp_tlv_desc_bearer_flags_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_maximum_packet_loss_rate_0,
        &protob_gtp_tlv_desc_f_teid_8,
        &protob_gtp_tlv_desc_f_teid_9,
        &protob_gtp_tlv_desc_f_teid_10,
        &protob_gtp_tlv_desc_f_teid_11,
        &protob_gtp_tlv_desc_ran_nas_cause_0,
        &protob_gtp_tlv_desc_apco_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_ti_0,
        &protob_gtp_tlv_desc_packet_flow_id_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_pdn_connection_0 =
{
    PROTOB_TLV_COMPOUND,
    "PDN Connection",
    PROTOB_GTP_PDN_CONNECTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_pdn_connection_t),
    {
        &protob_gtp_tlv_desc_apn_0,
        &protob_gtp_tlv_desc_apn_restriction_0,
        &protob_gtp_tlv_desc_selection_mode_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_ip_address_1,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_fqdn_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_charging_characteristics_0,
        &protob_gtp_tlv_desc_change_reporting_action_0,
        &protob_gtp_tlv_desc_csg_information_reporting_action_0,
        &protob_gtp_tlv_desc_enb_information_reporting_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_signalling_priority_indication_0,
        &protob_gtp_tlv_desc_change_to_report_flags_0,
        &protob_gtp_tlv_desc_fqdn_1,
        &protob_gtp_tlv_desc_presence_reporting_area_action_0,
        &protob_gtp_tlv_desc_wlan_offloadability_indication_0,
        &protob_gtp_tlv_desc_remote_ue_context_0,
        &protob_gtp_tlv_desc_pdn_type_0,
        &protob_gtp_tlv_desc_header_compression_configuration_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_0 =
{
    PROTOB_TLV_COMPOUND,
    "Overload Control Information",
    PROTOB_GTP_OVERLOAD_CONTROL_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_overload_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_epc_timer_0,
        &protob_gtp_tlv_desc_apn_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_1 =
{
    PROTOB_TLV_COMPOUND,
    "Overload Control Information",
    PROTOB_GTP_OVERLOAD_CONTROL_INFORMATION_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_overload_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_epc_timer_0,
        &protob_gtp_tlv_desc_apn_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_overload_control_information_2 =
{
    PROTOB_TLV_COMPOUND,
    "Overload Control Information",
    PROTOB_GTP_OVERLOAD_CONTROL_INFORMATION_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_overload_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_epc_timer_0,
        &protob_gtp_tlv_desc_apn_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_0 =
{
    PROTOB_TLV_COMPOUND,
    "Load Control Information",
    PROTOB_GTP_LOAD_CONTROL_INFORMATION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_load_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_apn_and_relative_capacity_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_1 =
{
    PROTOB_TLV_COMPOUND,
    "Load Control Information",
    PROTOB_GTP_LOAD_CONTROL_INFORMATION_TYPE,
    0,
    1,
    sizeof(protob_gtp_tlv_load_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_apn_and_relative_capacity_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_load_control_information_2 =
{
    PROTOB_TLV_COMPOUND,
    "Load Control Information",
    PROTOB_GTP_LOAD_CONTROL_INFORMATION_TYPE,
    0,
    2,
    sizeof(protob_gtp_tlv_load_control_information_t),
    {
        &protob_gtp_tlv_desc_sequence_number_0,
        &protob_gtp_tlv_desc_metric_0,
        &protob_gtp_tlv_desc_apn_and_relative_capacity_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_scef_pdn_connection_0 =
{
    PROTOB_TLV_COMPOUND,
    "SCEF PDN Connection",
    PROTOB_GTP_SCEF_PDN_CONNECTION_TYPE,
    0,
    0,
    sizeof(protob_gtp_tlv_scef_pdn_connection_t),
    {
        &protob_gtp_tlv_desc_apn_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_node_identifier_0,
        NULL,
    }
};

protob_tlv_desc_t protob_gtp_tlv_desc_echo_request =
{
    PROTOB_TLV_MESSAGE,
    "Echo Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_node_features_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_echo_response =
{
    PROTOB_TLV_MESSAGE,
    "Echo Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_node_features_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_session_request =
{
    PROTOB_TLV_MESSAGE,
    "Create Session Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_imsi_0,
        &protob_gtp_tlv_desc_msisdn_0,
        &protob_gtp_tlv_desc_mei_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_serving_network_0,
        &protob_gtp_tlv_desc_rat_type_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_f_teid_1,
        &protob_gtp_tlv_desc_apn_0,
        &protob_gtp_tlv_desc_selection_mode_0,
        &protob_gtp_tlv_desc_pdn_type_0,
        &protob_gtp_tlv_desc_paa_0,
        &protob_gtp_tlv_desc_apn_restriction_0,
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_twmi_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_trace_information_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_fq_csid_2,
        &protob_gtp_tlv_desc_fq_csid_3,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_uci_0,
        &protob_gtp_tlv_desc_charging_characteristics_0,
        &protob_gtp_tlv_desc_ldn_0,
        &protob_gtp_tlv_desc_ldn_1,
        &protob_gtp_tlv_desc_ldn_2,
        &protob_gtp_tlv_desc_ldn_3,
        &protob_gtp_tlv_desc_signalling_priority_indication_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_apco_0,
        &protob_gtp_tlv_desc_ip_address_1,
        &protob_gtp_tlv_desc_port_number_1,
        &protob_gtp_tlv_desc_ip_address_2,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_ip_address_3,
        &protob_gtp_tlv_desc_cn_operator_selection_entity_0,
        &protob_gtp_tlv_desc_presence_reporting_area_information_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_millisecond_time_stamp_0,
        &protob_gtp_tlv_desc_integer_number_0,
        &protob_gtp_tlv_desc_twan_identifier_1,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_remote_ue_context_0,
        &protob_gtp_tlv_desc_node_identifier_0,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_serving_plmn_rate_control_0,
        &protob_gtp_tlv_desc_counter_0,
        &protob_gtp_tlv_desc_port_number_2,
        &protob_gtp_tlv_desc_mapped_ue_usage_type_0,
        &protob_gtp_tlv_desc_uli_1,
        &protob_gtp_tlv_desc_fqdn_0,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
        &protob_gtp_tlv_desc_up_function_selection_indication_flags_0,
        &protob_gtp_tlv_desc_apn_rate_control_status_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_session_response =
{
    PROTOB_TLV_MESSAGE,
    "Create Session Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_change_reporting_action_0,
        &protob_gtp_tlv_desc_csg_information_reporting_action_0,
        &protob_gtp_tlv_desc_enb_information_reporting_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_f_teid_1,
        &protob_gtp_tlv_desc_paa_0,
        &protob_gtp_tlv_desc_apn_restriction_0,
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_fqdn_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_ldn_0,
        &protob_gtp_tlv_desc_ldn_1,
        &protob_gtp_tlv_desc_epc_timer_0,
        &protob_gtp_tlv_desc_apco_0,
        &protob_gtp_tlv_desc_ip4cp_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_presence_reporting_area_action_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_charging_id_0,
        &protob_gtp_tlv_desc_epco_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_request =
{
    PROTOB_TLV_MESSAGE,
    "Modify Bearer Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_mei_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_serving_network_0,
        &protob_gtp_tlv_desc_rat_type_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_delay_value_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_uci_0,
        &protob_gtp_tlv_desc_ip_address_1,
        &protob_gtp_tlv_desc_port_number_1,
        &protob_gtp_tlv_desc_ldn_0,
        &protob_gtp_tlv_desc_ldn_1,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_ip_address_2,
        &protob_gtp_tlv_desc_cn_operator_selection_entity_0,
        &protob_gtp_tlv_desc_presence_reporting_area_information_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_serving_plmn_rate_control_0,
        &protob_gtp_tlv_desc_counter_0,
        &protob_gtp_tlv_desc_imsi_0,
        &protob_gtp_tlv_desc_uli_1,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_0,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_response =
{
    PROTOB_TLV_MESSAGE,
    "Modify Bearer Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_msisdn_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_apn_restriction_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_change_reporting_action_0,
        &protob_gtp_tlv_desc_csg_information_reporting_action_0,
        &protob_gtp_tlv_desc_enb_information_reporting_0,
        &protob_gtp_tlv_desc_fqdn_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_ldn_0,
        &protob_gtp_tlv_desc_ldn_1,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_presence_reporting_area_action_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_charging_id_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_session_request =
{
    PROTOB_TLV_MESSAGE,
    "Delete Session Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_node_type_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_uli_timestamp_0,
        &protob_gtp_tlv_desc_ran_nas_cause_0,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_twan_identifier_1,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_1,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_port_number_1,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_session_response =
{
    PROTOB_TLV_MESSAGE,
    "Delete Session Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_apn_rate_control_status_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_command =
{
    PROTOB_TLV_MESSAGE,
    "Modify Bearer Command",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_f_teid_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_bearer_failure_indication =
{
    PROTOB_TLV_MESSAGE,
    "Modify Bearer Failure Indication",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_command =
{
    PROTOB_TLV_MESSAGE,
    "Delete Bearer Command",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_uli_timestamp_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_failure_indication =
{
    PROTOB_TLV_MESSAGE,
    "Delete Bearer Failure Indication",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_resource_command =
{
    PROTOB_TLV_MESSAGE,
    "Bearer Resource Command",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_pti_0,
        &protob_gtp_tlv_desc_flow_qos_0,
        &protob_gtp_tlv_desc_tad_0,
        &protob_gtp_tlv_desc_rat_type_0,
        &protob_gtp_tlv_desc_serving_network_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_ebi_1,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_f_teid_1,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_signalling_priority_indication_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_epco_0,
        &protob_gtp_tlv_desc_f_teid_2,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_bearer_resource_failure_indication =
{
    PROTOB_TLV_MESSAGE,
    "Bearer Resource Failure Indication",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_pti_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_f_container_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification_failure_indication =
{
    PROTOB_TLV_MESSAGE,
    "Downlink Data Notification Failure Indication",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_node_type_0,
        &protob_gtp_tlv_desc_imsi_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_bearer_request =
{
    PROTOB_TLV_MESSAGE,
    "Create Bearer Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_pti_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_change_reporting_action_0,
        &protob_gtp_tlv_desc_csg_information_reporting_action_0,
        &protob_gtp_tlv_desc_enb_information_reporting_0,
        &protob_gtp_tlv_desc_presence_reporting_area_action_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_container_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_bearer_response =
{
    PROTOB_TLV_MESSAGE,
    "Create Bearer Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_2,
        &protob_gtp_tlv_desc_fq_csid_3,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_presence_reporting_area_information_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_twan_identifier_1,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_1,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_port_number_1,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_update_bearer_request =
{
    PROTOB_TLV_MESSAGE,
    "Update Bearer Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_pti_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_ambr_0,
        &protob_gtp_tlv_desc_change_reporting_action_0,
        &protob_gtp_tlv_desc_csg_information_reporting_action_0,
        &protob_gtp_tlv_desc_enb_information_reporting_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_presence_reporting_area_action_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_container_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_update_bearer_response =
{
    PROTOB_TLV_MESSAGE,
    "Update Bearer Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_fq_csid_2,
        &protob_gtp_tlv_desc_fq_csid_3,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_presence_reporting_area_information_0,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_twan_identifier_1,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_1,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_port_number_1,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_request =
{
    PROTOB_TLV_MESSAGE,
    "Delete Bearer Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_ebi_1,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_pti_0,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_load_control_information_1,
        &protob_gtp_tlv_desc_load_control_information_2,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_apn_rate_control_status_0,
        &protob_gtp_tlv_desc_epco_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_bearer_response =
{
    PROTOB_TLV_MESSAGE,
    "Delete Bearer Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_fq_csid_0,
        &protob_gtp_tlv_desc_fq_csid_1,
        &protob_gtp_tlv_desc_fq_csid_2,
        &protob_gtp_tlv_desc_fq_csid_3,
        &protob_gtp_tlv_desc_pco_0,
        &protob_gtp_tlv_desc_ue_time_zone_0,
        &protob_gtp_tlv_desc_uli_0,
        &protob_gtp_tlv_desc_uli_timestamp_0,
        &protob_gtp_tlv_desc_twan_identifier_0,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_1,
        &protob_gtp_tlv_desc_ip_address_0,
        &protob_gtp_tlv_desc_overload_control_information_2,
        &protob_gtp_tlv_desc_twan_identifier_1,
        &protob_gtp_tlv_desc_twan_identifier_timestamp_1,
        &protob_gtp_tlv_desc_port_number_0,
        &protob_gtp_tlv_desc_f_container_0,
        &protob_gtp_tlv_desc_port_number_1,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_request =
{
    PROTOB_TLV_MESSAGE,
    "Create Indirect Data Forwarding Tunnel Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_imsi_0,
        &protob_gtp_tlv_desc_mei_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_tlv_desc_more8,
        &protob_gtp_tlv_desc_recovery_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_response =
{
    PROTOB_TLV_MESSAGE,
    "Create Indirect Data Forwarding Tunnel Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_tlv_desc_more8,
        &protob_gtp_tlv_desc_recovery_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_request =
{
    PROTOB_TLV_MESSAGE,
    "Delete Indirect Data Forwarding Tunnel Request",
    0, 0, 0, 0, {
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_response =
{
    PROTOB_TLV_MESSAGE,
    "Delete Indirect Data Forwarding Tunnel Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_recovery_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_release_access_bearers_request =
{
    PROTOB_TLV_MESSAGE,
    "Release Access Bearers Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_node_type_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_release_access_bearers_response =
{
    PROTOB_TLV_MESSAGE,
    "Release Access Bearers Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification =
{
    PROTOB_TLV_MESSAGE,
    "Downlink Data Notification",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_ebi_0,
        &protob_gtp_tlv_desc_arp_0,
        &protob_gtp_tlv_desc_imsi_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
        &protob_gtp_tlv_desc_paging_and_service_information_0,
        &protob_gtp_tlv_desc_integer_number_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_downlink_data_notification_acknowledge =
{
    PROTOB_TLV_MESSAGE,
    "Downlink Data Notification Acknowledge",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_delay_value_0,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_throttling_0,
        &protob_gtp_tlv_desc_imsi_0,
        &protob_gtp_tlv_desc_epc_timer_0,
        &protob_gtp_tlv_desc_integer_number_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_access_bearers_request =
{
    PROTOB_TLV_MESSAGE,
    "Modify Access Bearers Request",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_f_teid_0,
        &protob_gtp_tlv_desc_delay_value_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_secondary_rat_usage_data_report_0,
    NULL,
}};

protob_tlv_desc_t protob_gtp_tlv_desc_modify_access_bearers_response =
{
    PROTOB_TLV_MESSAGE,
    "Modify Access Bearers Response",
    0, 0, 0, 0, {
        &protob_gtp_tlv_desc_cause_0,
        &protob_gtp_tlv_desc_bearer_context_0,
        &protob_gtp_tlv_desc_bearer_context_1,
        &protob_gtp_tlv_desc_recovery_0,
        &protob_gtp_tlv_desc_indication_0,
        &protob_gtp_tlv_desc_load_control_information_0,
        &protob_gtp_tlv_desc_overload_control_information_0,
    NULL,
}};



int protob_tlv_parse_create_session_request( protob_gtp_create_session_request_t * create_session_request, protob_tlv_desc_t * desc, protob_pkbuf_t * pkbuf, int mode);

int protob_gtp_parse_msg( protob_gtp_message_t * gtp_message, protob_pkbuf_t * pkbuf)
{
	
	int rv = -1;
    protob_gtp_header_t *h = NULL;
    uint16_t size = 0;

    h = (protob_gtp_header_t *)pkbuf->data;
    
	//memset is happening in switch/case below at request level 
    //memset(gtp_message, 0, sizeof(protob_gtp_message_t));

    if (h->teid_presence)
        size = PROTOB_GTPV2C_HEADER_LEN;
    else
        size = PROTOB_GTPV2C_HEADER_LEN - PROTOB_GTP_TEID_LEN;

    //memcpy(&gtp_message->h, pkbuf->data - size, size);
	memcpy(&gtp_message->h, pkbuf->data, size);

    if (h->teid_presence)
	{
        gtp_message->h.teid = be32toh(gtp_message->h.teid);
		gtp_message->h.sqn = htobe32(gtp_message->h.sqn  << 8);
	}
	else
	{
		//gtp_message->h.sqn = htobe32(gtp_message->h.teid) << 8;
		gtp_message->h.sqn = htobe32(gtp_message->h.teid  << 8);
	}
	


	pkbuf->edata = &pkbuf->data[size];
	pkbuf->elen = pkbuf->len - size;
	
	//printf("edata=%02X size=%d  elen=%d len=%d LINE=%d\n", pkbuf->edata[0] & 0xFF, size, pkbuf->elen, pkbuf->len, __LINE__);
	
	
	
	
	gtp_message->h.length = htons(gtp_message->h.length);

    if (pkbuf->len == 0) 
	{
        return -2;
    }
	
	// printf("Type=%02X %d len=%d, %02X %d, 2=%02X 3=%02X  teid_presence=%d   len=%d  l=%d\n", 
		// gtp_message->h.type, gtp_message->h.type, gtp_message->h.length, 
		// pkbuf->data[1] & 0xFF, pkbuf->data[1] & 0xFF, pkbuf->data[2] & 0xFF, pkbuf->data[3] & 0xFF, 
		// h->teid_presence, htons(63744),
		// __LINE__);

	//exit(0);
			
    switch(gtp_message->h.type) 
	{
		case PROTOB_GTP_ECHO_REQUEST_TYPE:
			memset( &gtp_message->echo_request, 0, sizeof(protob_gtp_echo_request_t));
			rv = protob_tlv_parse_msg( &gtp_message->echo_request, &protob_gtp_tlv_desc_echo_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_ECHO_RESPONSE_TYPE:
			memset( &gtp_message->echo_response, 0, sizeof(protob_gtp_echo_response_t));
			rv = protob_tlv_parse_msg( &gtp_message->echo_response, &protob_gtp_tlv_desc_echo_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_SESSION_REQUEST_TYPE:
			memset( &gtp_message->create_session_request, 0, sizeof(protob_gtp_create_session_request_t));
			//rv = protob_tlv_parse_create_session_request( &gtp_message->create_session_request, &protob_gtp_tlv_desc_create_session_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			rv = protob_tlv_parse_msg( &gtp_message->create_session_request, &protob_gtp_tlv_desc_create_session_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			//printf("rv=%d l=%d\n", rv, __LINE__);
			break;
		case PROTOB_GTP_CREATE_SESSION_RESPONSE_TYPE:
			memset( &gtp_message->create_session_response, 0, sizeof(protob_gtp_create_session_response_t));
			rv = protob_tlv_parse_msg( &gtp_message->create_session_response, &protob_gtp_tlv_desc_create_session_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_REQUEST_TYPE:
			memset( &gtp_message->modify_bearer_request, 0, sizeof(protob_gtp_modify_bearer_request_t));
			rv = protob_tlv_parse_msg( &gtp_message->modify_bearer_request, &protob_gtp_tlv_desc_modify_bearer_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_RESPONSE_TYPE:
			memset( &gtp_message->modify_bearer_response, 0, sizeof(protob_gtp_modify_bearer_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->modify_bearer_response, &protob_gtp_tlv_desc_modify_bearer_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_SESSION_REQUEST_TYPE:
			memset( &gtp_message->delete_session_request, 0, sizeof(protob_gtp_delete_session_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_session_request, &protob_gtp_tlv_desc_delete_session_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_SESSION_RESPONSE_TYPE:
			memset( &gtp_message->delete_session_response, 0, sizeof(protob_gtp_delete_session_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_session_response, &protob_gtp_tlv_desc_delete_session_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_COMMAND_TYPE:
			memset( &gtp_message->modify_bearer_command, 0, sizeof(protob_gtp_modify_bearer_command_t));
			rv = protob_tlv_parse_msg(&gtp_message->modify_bearer_command, &protob_gtp_tlv_desc_modify_bearer_command, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_FAILURE_INDICATION_TYPE:
			memset( &gtp_message->modify_bearer_failure_indication, 0, sizeof(protob_gtp_modify_bearer_failure_indication_t));
			rv = protob_tlv_parse_msg(&gtp_message->modify_bearer_failure_indication, &protob_gtp_tlv_desc_modify_bearer_failure_indication, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_COMMAND_TYPE:
			memset( &gtp_message->delete_bearer_command, 0, sizeof(protob_gtp_delete_bearer_command_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_bearer_command, &protob_gtp_tlv_desc_delete_bearer_command, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_FAILURE_INDICATION_TYPE:
			memset( &gtp_message->delete_bearer_failure_indication, 0, sizeof(protob_gtp_delete_bearer_failure_indication_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_bearer_failure_indication, &protob_gtp_tlv_desc_delete_bearer_failure_indication, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_BEARER_RESOURCE_COMMAND_TYPE:
			memset( &gtp_message->bearer_resource_command, 0, sizeof(protob_gtp_bearer_resource_command_t));
			rv = protob_tlv_parse_msg(&gtp_message->bearer_resource_command, &protob_gtp_tlv_desc_bearer_resource_command, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_BEARER_RESOURCE_FAILURE_INDICATION_TYPE:
			memset( &gtp_message->bearer_resource_failure_indication, 0, sizeof(protob_gtp_bearer_resource_failure_indication_t));
			rv = protob_tlv_parse_msg(&gtp_message->bearer_resource_failure_indication, &protob_gtp_tlv_desc_bearer_resource_failure_indication, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION_TYPE:
			memset( &gtp_message->downlink_data_notification_failure_indication, 0, sizeof(protob_gtp_downlink_data_notification_failure_indication_t));
			rv = protob_tlv_parse_msg(&gtp_message->downlink_data_notification_failure_indication, &protob_gtp_tlv_desc_downlink_data_notification_failure_indication, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_BEARER_REQUEST_TYPE:
			memset( &gtp_message->create_bearer_request, 0, sizeof(protob_gtp_create_bearer_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->create_bearer_request, &protob_gtp_tlv_desc_create_bearer_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_BEARER_RESPONSE_TYPE:
			memset( &gtp_message->create_bearer_response, 0, sizeof(protob_gtp_create_bearer_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->create_bearer_response, &protob_gtp_tlv_desc_create_bearer_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_UPDATE_BEARER_REQUEST_TYPE:
			memset( &gtp_message->update_bearer_request, 0, sizeof(protob_gtp_update_bearer_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->update_bearer_request, &protob_gtp_tlv_desc_update_bearer_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_UPDATE_BEARER_RESPONSE_TYPE:
			memset( &gtp_message->update_bearer_response, 0, sizeof(protob_gtp_update_bearer_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->update_bearer_response, &protob_gtp_tlv_desc_update_bearer_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_REQUEST_TYPE:
			memset( &gtp_message->delete_bearer_request, 0, sizeof(protob_gtp_delete_bearer_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_bearer_request, &protob_gtp_tlv_desc_delete_bearer_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_RESPONSE_TYPE:
			memset( &gtp_message->delete_bearer_response, 0, sizeof(protob_gtp_delete_bearer_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_bearer_response, &protob_gtp_tlv_desc_delete_bearer_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
			memset( &gtp_message->create_indirect_data_forwarding_tunnel_request, 0, sizeof(protob_gtp_create_indirect_data_forwarding_tunnel_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->create_indirect_data_forwarding_tunnel_request, &protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
			memset( &gtp_message->create_indirect_data_forwarding_tunnel_response, 0, sizeof(protob_gtp_create_indirect_data_forwarding_tunnel_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->create_indirect_data_forwarding_tunnel_response, &protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
			memset( &gtp_message->delete_indirect_data_forwarding_tunnel_request, 0, sizeof(protob_gtp_delete_indirect_data_forwarding_tunnel_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_indirect_data_forwarding_tunnel_request, &protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
			memset( &gtp_message->delete_indirect_data_forwarding_tunnel_response, 0, sizeof(protob_gtp_delete_indirect_data_forwarding_tunnel_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->delete_indirect_data_forwarding_tunnel_response, &protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_RELEASE_ACCESS_BEARERS_REQUEST_TYPE:
			memset( &gtp_message->release_access_bearers_request, 0, sizeof(protob_gtp_release_access_bearers_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->release_access_bearers_request, &protob_gtp_tlv_desc_release_access_bearers_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_RELEASE_ACCESS_BEARERS_RESPONSE_TYPE:
			memset( &gtp_message->release_access_bearers_response, 0, sizeof(protob_gtp_release_access_bearers_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->release_access_bearers_response, &protob_gtp_tlv_desc_release_access_bearers_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_TYPE:
			memset( &gtp_message->downlink_data_notification, 0, sizeof(protob_gtp_downlink_data_notification_t));
			rv = protob_tlv_parse_msg(&gtp_message->downlink_data_notification, &protob_gtp_tlv_desc_downlink_data_notification, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE_TYPE:
			memset( &gtp_message->downlink_data_notification_acknowledge, 0, sizeof(protob_gtp_downlink_data_notification_acknowledge_t));
			rv = protob_tlv_parse_msg(&gtp_message->downlink_data_notification_acknowledge, &protob_gtp_tlv_desc_downlink_data_notification_acknowledge, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_ACCESS_BEARERS_REQUEST_TYPE:
			memset( &gtp_message->modify_access_bearers_request, 0, sizeof(protob_gtp_modify_access_bearers_request_t));
			rv = protob_tlv_parse_msg(&gtp_message->modify_access_bearers_request, &protob_gtp_tlv_desc_modify_access_bearers_request, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_ACCESS_BEARERS_RESPONSE_TYPE:
			memset( &gtp_message->modify_access_bearers_response, 0, sizeof(protob_gtp_modify_access_bearers_response_t));
			rv = protob_tlv_parse_msg(&gtp_message->modify_access_bearers_response, &protob_gtp_tlv_desc_modify_access_bearers_response, pkbuf, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		default:
			//protob_warn("Not implmeneted(type:%d)", gtp_message->h.type);
			break;
    }

    //protob_assert(protob_pkbuf_push(pkbuf, size));

    return rv;
}

protob_pkbuf_t * protob_gtp_build_msg( protob_gtp_message_t * gtp_message)
{
    protob_pkbuf_t *pkbuf = NULL;

    switch(gtp_message->h.type) 
	{
		case PROTOB_GTP_ECHO_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_echo_request, &gtp_message->echo_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_ECHO_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_echo_response, &gtp_message->echo_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_SESSION_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_session_request, &gtp_message->create_session_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_SESSION_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_session_response, &gtp_message->create_session_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_bearer_request, &gtp_message->modify_bearer_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_bearer_response, &gtp_message->modify_bearer_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_SESSION_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_session_request, &gtp_message->delete_session_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_SESSION_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_session_response, &gtp_message->delete_session_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_COMMAND_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_bearer_command, &gtp_message->modify_bearer_command, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_BEARER_FAILURE_INDICATION_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_bearer_failure_indication, &gtp_message->modify_bearer_failure_indication, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_COMMAND_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_bearer_command, &gtp_message->delete_bearer_command, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_FAILURE_INDICATION_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_bearer_failure_indication, &gtp_message->delete_bearer_failure_indication, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_BEARER_RESOURCE_COMMAND_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_bearer_resource_command, &gtp_message->bearer_resource_command, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_BEARER_RESOURCE_FAILURE_INDICATION_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_bearer_resource_failure_indication, &gtp_message->bearer_resource_failure_indication, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_FAILURE_INDICATION_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_downlink_data_notification_failure_indication, &gtp_message->downlink_data_notification_failure_indication, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_BEARER_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_bearer_request, &gtp_message->create_bearer_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_BEARER_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_bearer_response, &gtp_message->create_bearer_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_UPDATE_BEARER_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_update_bearer_request, &gtp_message->update_bearer_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_UPDATE_BEARER_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_update_bearer_response, &gtp_message->update_bearer_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_bearer_request, &gtp_message->delete_bearer_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_BEARER_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_bearer_response, &gtp_message->delete_bearer_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_request, &gtp_message->create_indirect_data_forwarding_tunnel_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_create_indirect_data_forwarding_tunnel_response, &gtp_message->create_indirect_data_forwarding_tunnel_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_request, &gtp_message->delete_indirect_data_forwarding_tunnel_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_delete_indirect_data_forwarding_tunnel_response, &gtp_message->delete_indirect_data_forwarding_tunnel_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_RELEASE_ACCESS_BEARERS_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_release_access_bearers_request, &gtp_message->release_access_bearers_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_RELEASE_ACCESS_BEARERS_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_release_access_bearers_response, &gtp_message->release_access_bearers_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_downlink_data_notification, &gtp_message->downlink_data_notification, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_downlink_data_notification_acknowledge, &gtp_message->downlink_data_notification_acknowledge, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_ACCESS_BEARERS_REQUEST_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_access_bearers_request, &gtp_message->modify_access_bearers_request, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		case PROTOB_GTP_MODIFY_ACCESS_BEARERS_RESPONSE_TYPE:
			pkbuf = protob_tlv_build_msg(&protob_gtp_tlv_desc_modify_access_bearers_response, &gtp_message->modify_access_bearers_response, PROTOB_TLV_MODE_T1_L2_I1);
			break;
		default:
			//protob_warn("Not implmeneted(type:%d)", gtp_message->h.type);
			break;
    }

    return pkbuf;
}




//gcc -o protob protob_tlv.c protob_tlv_message.c protob_gtp_specs.c
// int main( int argc, char* argv[])
// {
	// printf("main:c\n");
	// return 0;
// }




