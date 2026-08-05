#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef S_PFCP_TYPES
#define S_PFCP_TYPES

#include "pfcp_protocol_def.h"

#pragma pack(1)
typedef struct pfcp_flags
{
	uint8_t SEID: 1;
	uint8_t MessagePriority: 1;
	uint8_t FollowOn: 1;
	uint8_t Spare: 2;
	uint8_t Version: 3;
} PFCPFlags;
	
#pragma pack(4)	
typedef struct pfcp_header_s 
{
    struct
	{
		uint8_t SEID: 1;
		uint8_t MessagePriority: 1;
		uint8_t FollowOn: 1;
		uint8_t Spare: 2;
		uint8_t Version: 3;
	} Flags;

	uint8_t MessageType;
	uint64_t SEID;
	uint32_t SequenceNumber;
	uint8_t Spare;
} pfcp_header_t;


typedef uint16_t pfcp_pdr_id_t;
typedef uint32_t pfcp_far_id_t;
typedef uint32_t pfcp_urr_id_t;
typedef uint32_t pfcp_qer_id_t;
typedef uint8_t  pfcp_bar_id_t;


typedef uint32_t pfcp_precedence_t;


typedef uint8_t  pfcp_interface_t;



typedef uint64_t pfcp_tlv_presence_t;

typedef struct pfcp_tlv_uint8_s {
    pfcp_tlv_presence_t presence;
    uint8_t u8;
} pfcp_tlv_uint8_t;

typedef struct pfcp_tlv_uint16_s {
    pfcp_tlv_presence_t presence;
    uint16_t u16;
} pfcp_tlv_uint16_t;


typedef struct pfcp_tlv_uint24_s {
    pfcp_tlv_presence_t presence;
    uint32_t u24;
} pfcp_tlv_uint24_t;


typedef struct pfcp_tlv_uint32_s {
    pfcp_tlv_presence_t presence;
    uint32_t u32;
} pfcp_tlv_uint32_t;


typedef struct pfcp_tlv_int8_s {
    pfcp_tlv_presence_t presence;
    int8_t i8;
} pfcp_tlv_int8_t;


typedef struct pfcp_tlv_int16_s {
    pfcp_tlv_presence_t presence;
    int16_t i16;
} pfcp_tlv_int16_t;


typedef struct pfcp_tlv_int24_s {
    pfcp_tlv_presence_t presence;
    int32_t i24;
} pfcp_tlv_int24_t;


typedef struct pfcp_tlv_int32_s {
    pfcp_tlv_presence_t presence;
    int32_t i32;
} pfcp_tlv_int32_t;

typedef struct pfcp_tlv_octet_s {
    pfcp_tlv_presence_t presence;
    uint8_t * data;
    uint32_t len;
} pfcp_tlv_octet_t;

typedef struct pfcp_tlv_null_s {
    pfcp_tlv_presence_t presence;
} pfcp_tlv_null_t;


typedef pfcp_tlv_uint8_t 	pfcp_tlv_cause_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_source_interface_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_f_teid_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_network_instance_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_sdf_filter_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_application_id_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_gate_status_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_mbr_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_gbr_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_qer_correlation_id_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_precedence_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_transport_level_marking_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_volume_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_time_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_monitoring_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_volume_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_time_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_inactivity_detection_time_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_reporting_triggers_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_redirect_information_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_report_type_t;
typedef pfcp_tlv_uint16_t 	pfcp_tlv_offending_ie_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_forwarding_policy_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_destination_interface_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_up_function_features_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_apply_action_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_downlink_data_service_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_downlink_data_notification_delay_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_dl_buffering_duration_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_dl_buffering_suggested_packet_count_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_pfcpsmreq_flags_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_pfcpsrrsp_flags_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_sequence_number_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_metric_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_timer_t;
typedef pfcp_tlv_uint16_t 	pfcp_tlv_pdr_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_f_seid_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_node_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_pfd_contents_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_measurement_method_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_usage_report_trigger_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_measurement_period_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_fq_csid_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_volume_measurement_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_duration_measurement_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_time_of_first_packet_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_time_of_last_packet_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_quota_holding_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_dropped_dl_traffic_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_volume_quota_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_time_quota_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_start_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_end_time_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_urr_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_linked_urr_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_outer_header_creation_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_bar_id_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_cp_function_features_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_usage_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_application_instance_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_flow_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ue_ip_address_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_packet_rate_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_outer_header_removal_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_recovery_time_stamp_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_dl_flow_level_marking_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_header_enrichment_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_measurement_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_node_report_type_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_remote_gtp_u_peer_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ur_seqn_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_activate_predefined_rules_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_deactivate_predefined_rules_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_far_id_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_qer_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_oci_flags_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_pfcp_association_release_request_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_graceful_release_period_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_pdn_type_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_failed_rule_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_time_quota_mechanism_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_user_plane_ip_resource_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_user_plane_inactivity_timer_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_aggregated_urrs_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_multiplier_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_aggregated_urr_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_volume_quota_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_time_quota_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_rqi_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_qfi_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_query_urr_reference_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_additional_usage_reports_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_update_traffic_endpoint_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_traffic_endpoint_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_mac_address_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_c_tag_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_s_tag_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ethertype_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_proxying_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ethernet_filter_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ethernet_filter_properties_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_suggested_buffering_packets_count_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_user_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ethernet_pdu_session_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_mac_addresses_detected_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_mac_addresses_removed_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ethernet_inactivity_timer_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_additional_monitoring_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_event_quota_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_event_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_event_quota_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_subsequent_event_threshold_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_trace_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_framed_route_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_framed_routing_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_framed_ipv6_route_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_event_time_stamp_t;
typedef pfcp_tlv_uint32_t 	pfcp_tlv_averaging_window_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_paging_policy_indicator_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_apn_dnn_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv__interface_type_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_pfcpsrreq_flags_t;
typedef pfcp_tlv_uint8_t 	pfcp_tlv_pfcpaureq_flags_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_activation_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_deactivation_time_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_mar_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_steering_functionality_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_steering_mode_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_weight_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_priority_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_ue_ip_address_pool_identity_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_alternative_smf_ip_address_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_packet_replication_and_detection_carry_on_information_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_smf_set_id_t;
typedef pfcp_tlv_octet_t 	pfcp_tlv_quota_validity_time_t;



/* Structure for Group Infomration Element */
#pragma pack(4)
typedef struct pfcp_tlv_ethernet_packet_filter_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_ethernet_filter_id_t ethernet_filter_id;
    pfcp_tlv_ethernet_filter_properties_t ethernet_filter_properties;
    pfcp_tlv_mac_address_t mac_address;
    pfcp_tlv_ethertype_t ethertype;
    pfcp_tlv_c_tag_t c_tag;
    pfcp_tlv_s_tag_t s_tag;
    pfcp_tlv_sdf_filter_t sdf_filter[8];
} pfcp_tlv_ethernet_packet_filter_t;

#pragma pack(4)
typedef struct pfcp_tlv_pdi_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_source_interface_t source_interface;
    pfcp_tlv_f_teid_t local_f_teid;
    pfcp_tlv_network_instance_t network_instance;
    pfcp_tlv_ue_ip_address_t ue_ip_address;
    pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;
    pfcp_tlv_sdf_filter_t sdf_filter[8];
    pfcp_tlv_application_id_t application_id;
    pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;
    pfcp_tlv_ethernet_packet_filter_t ethernet_packet_filter;
    pfcp_tlv_qfi_t qfi;
    pfcp_tlv_framed_route_t framed_route;
    pfcp_tlv_framed_routing_t framed_routing;
    pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;
    pfcp_tlv__interface_type_t source_interface_type;
} pfcp_tlv_pdi_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_pdr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pdr_id_t pdr_id;
    pfcp_tlv_precedence_t precedence;
    pfcp_tlv_pdi_t pdi;
    pfcp_tlv_outer_header_removal_t outer_header_removal;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_qer_id_t qer_id;
    pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;
    pfcp_tlv_activation_time_t activation_time;
    pfcp_tlv_deactivation_time_t deactivation_time;
    pfcp_tlv_mar_id_t mar_id;
    pfcp_tlv_packet_replication_and_detection_carry_on_information_t packet_replication_and_detection_carry_on_information;
} pfcp_tlv_create_pdr_t;

#pragma pack(4)
typedef struct pfcp_tlv_forwarding_parameters_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_destination_interface_t destination_interface;
    pfcp_tlv_network_instance_t network_instance;
    pfcp_tlv_redirect_information_t redirect_information;
    pfcp_tlv_outer_header_creation_t outer_header_creation;
    pfcp_tlv_transport_level_marking_t transport_level_marking;
    pfcp_tlv_forwarding_policy_t forwarding_policy;
    pfcp_tlv_header_enrichment_t header_enrichment;
    pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;
    pfcp_tlv_proxying_t proxying;
    pfcp_tlv__interface_type_t destination_interface_type;
} pfcp_tlv_forwarding_parameters_t;

#pragma pack(4)
typedef struct pfcp_tlv_duplicating_parameters_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_destination_interface_t destination_interface;
    pfcp_tlv_outer_header_creation_t outer_header_creation;
    pfcp_tlv_transport_level_marking_t transport_level_marking;
    pfcp_tlv_forwarding_policy_t forwarding_policy;
} pfcp_tlv_duplicating_parameters_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_far_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_apply_action_t apply_action;
    pfcp_tlv_forwarding_parameters_t forwarding_parameters;
    pfcp_tlv_duplicating_parameters_t duplicating_parameters;
    pfcp_tlv_bar_id_t bar_id;
} pfcp_tlv_create_far_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_forwarding_parameters_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_destination_interface_t destination_interface;
    pfcp_tlv_network_instance_t network_instance;
    pfcp_tlv_redirect_information_t redirect_information;
    pfcp_tlv_outer_header_creation_t outer_header_creation;
    pfcp_tlv_transport_level_marking_t transport_level_marking;
    pfcp_tlv_forwarding_policy_t forwarding_policy;
    pfcp_tlv_header_enrichment_t header_enrichment;
    pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;
    pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;
    pfcp_tlv__interface_type_t destination_interface_type;
} pfcp_tlv_update_forwarding_parameters_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_duplicating_parameters_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_destination_interface_t destination_interface;
    pfcp_tlv_outer_header_creation_t outer_header_creation;
    pfcp_tlv_transport_level_marking_t transport_level_marking;
    pfcp_tlv_forwarding_policy_t forwarding_policy;
} pfcp_tlv_update_duplicating_parameters_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_far_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_apply_action_t apply_action;
    pfcp_tlv_update_forwarding_parameters_t update_forwarding_parameters;
    pfcp_tlv_update_duplicating_parameters_t update_duplicating_parameters;
    pfcp_tlv_bar_id_t bar_id;
} pfcp_tlv_update_far_t;

#pragma pack(4)
typedef struct pfcp_tlv_pfd_context_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pfd_contents_t pfd_contents;
} pfcp_tlv_pfd_context_t;

#pragma pack(4)
typedef struct pfcp_tlv_application_id_s_pfds_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_application_id_t application_id;
    pfcp_tlv_pfd_context_t pfd_context;
} pfcp_tlv_application_id_s_pfds_t;

#pragma pack(4)
typedef struct pfcp_tlv_ethernet_traffic_information_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_mac_addresses_detected_t mac_addresses_detected;
    pfcp_tlv_mac_addresses_removed_t mac_addresses_removed;
} pfcp_tlv_ethernet_traffic_information_t;

#pragma pack(4)
typedef struct pfcp_tlv_access_forwarding_action_information_1_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_weight_t weight;
    pfcp_tlv_priority_t priority;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_access_forwarding_action_information_1_t;

#pragma pack(4)
typedef struct pfcp_tlv_access_forwarding_action_information_2_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_weight_t weight;
    pfcp_tlv_priority_t priority;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_access_forwarding_action_information_2_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_access_forwarding_action_information_1_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_weight_t weight;
    pfcp_tlv_priority_t priority;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_update_access_forwarding_action_information_1_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_access_forwarding_action_information_2_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_weight_t weight;
    pfcp_tlv_priority_t priority;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_update_access_forwarding_action_information_2_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_urr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_measurement_method_t measurement_method;
    pfcp_tlv_reporting_triggers_t reporting_triggers;
    pfcp_tlv_measurement_period_t measurement_period;
    pfcp_tlv_volume_threshold_t volume_threshold;
    pfcp_tlv_volume_quota_t volume_quota;
    pfcp_tlv_event_threshold_t event_threshold;
    pfcp_tlv_event_quota_t event_quota;
    pfcp_tlv_time_threshold_t time_threshold;
    pfcp_tlv_time_quota_t time_quota;
    pfcp_tlv_quota_holding_time_t quota_holding_time;
    pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;
    pfcp_tlv_quota_validity_time_t quota_validity_time;
    pfcp_tlv_monitoring_time_t monitoring_time;
    pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;
    pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;
    pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;
    pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;
    pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;
    pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;
    pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;
    pfcp_tlv_linked_urr_id_t linked_urr_id;
    pfcp_tlv_measurement_information_t measurement_information;
    pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;
    pfcp_tlv_aggregated_urrs_t aggregated_urrs;
    pfcp_tlv_far_id_t far_id_for_quota_action;
    pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;
    pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;
} pfcp_tlv_create_urr_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_qer_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_qer_id_t qer_id;
    pfcp_tlv_qer_correlation_id_t qer_correlation_id;
    pfcp_tlv_gate_status_t gate_status;
    pfcp_tlv_mbr_t maximum_bitrate;
    pfcp_tlv_gbr_t guaranteed_bitrate;
    pfcp_tlv_packet_rate_t packet_rate;
    pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;
    pfcp_tlv_qfi_t qos_flow_identifier;
    pfcp_tlv_rqi_t reflective_qos;
    pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;
    pfcp_tlv_averaging_window_t averaging_window;
} pfcp_tlv_create_qer_t;

#pragma pack(4)
typedef struct pfcp_tlv_created_pdr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pdr_id_t pdr_id;
    pfcp_tlv_f_teid_t local_f_teid;
    pfcp_tlv_ue_ip_address_t ue_ip_address;
} pfcp_tlv_created_pdr_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_pdr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pdr_id_t pdr_id;
    pfcp_tlv_outer_header_removal_t outer_header_removal;
    pfcp_tlv_precedence_t precedence;
    pfcp_tlv_pdi_t pdi;
    pfcp_tlv_far_id_t far_id;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_qer_id_t qer_id;
    pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;
    pfcp_tlv_deactivate_predefined_rules_t deactivate_predefined_rules;
    pfcp_tlv_activation_time_t activation_time;
    pfcp_tlv_deactivation_time_t deactivation_time;
} pfcp_tlv_update_pdr_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_bar_pfcp_session_report_response_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_bar_id_t bar_id;
    pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;
    pfcp_tlv_dl_buffering_duration_t dl_buffering_duration;
    pfcp_tlv_dl_buffering_suggested_packet_count_t dl_buffering_suggested_packet_count;
    pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;
} pfcp_tlv_update_bar_pfcp_session_report_response_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_urr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_measurement_method_t measurement_method;
    pfcp_tlv_reporting_triggers_t reporting_triggers;
    pfcp_tlv_measurement_period_t measurement_period;
    pfcp_tlv_volume_threshold_t volume_threshold;
    pfcp_tlv_volume_quota_t volume_quota;
    pfcp_tlv_time_threshold_t time_threshold;
    pfcp_tlv_time_quota_t time_quota;
    pfcp_tlv_event_threshold_t event_threshold;
    pfcp_tlv_event_quota_t event_quota;
    pfcp_tlv_quota_holding_time_t quota_holding_time;
    pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;
    pfcp_tlv_quota_validity_time_t quota_validity_time;
    pfcp_tlv_monitoring_time_t monitoring_time;
    pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;
    pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;
    pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;
    pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;
    pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;
    pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;
    pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;
    pfcp_tlv_linked_urr_id_t linked_urr_id;
    pfcp_tlv_measurement_information_t measurement_information;
    pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;
    pfcp_tlv_aggregated_urrs_t aggregated_urrs;
    pfcp_tlv_far_id_t far_id_for_quota_action;
    pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;
    pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;
} pfcp_tlv_update_urr_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_qer_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_qer_id_t qer_id;
    pfcp_tlv_qer_correlation_id_t qer_correlation_id;
    pfcp_tlv_gate_status_t gate_status;
    pfcp_tlv_mbr_t maximum_bitrate;
    pfcp_tlv_gbr_t guaranteed_bitrate;
    pfcp_tlv_packet_rate_t packet_rate;
    pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;
    pfcp_tlv_qfi_t qos_flow_identifier;
    pfcp_tlv_rqi_t reflective_qos;
    pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;
    pfcp_tlv_averaging_window_t averaging_window;
} pfcp_tlv_update_qer_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_pdr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pdr_id_t pdr_id;
} pfcp_tlv_remove_pdr_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_far_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_far_id_t far_id;
} pfcp_tlv_remove_far_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_urr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_remove_urr_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_qer_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_qer_id_t qer_id;
} pfcp_tlv_remove_qer_t;

#pragma pack(4)
typedef struct pfcp_tlv_load_control_information_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_sequence_number_t load_control_sequence_number;
    pfcp_tlv_metric_t load_metric;
} pfcp_tlv_load_control_information_t;

#pragma pack(4)
typedef struct pfcp_tlv_overload_control_information_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_sequence_number_t overload_control_sequence_number;
    pfcp_tlv_metric_t overload_reduction_metric;
    pfcp_tlv_timer_t period_of_validity;
    pfcp_tlv_oci_flags_t overload_control_information_flags;
} pfcp_tlv_overload_control_information_t;

#pragma pack(4)
typedef struct pfcp_tlv_application_detection_information_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_application_id_t application_id;
    pfcp_tlv_application_instance_id_t application_instance_id;
    pfcp_tlv_flow_information_t flow_information;
} pfcp_tlv_application_detection_information_t;

#pragma pack(4)
typedef struct pfcp_tlv_query_urr_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
} pfcp_tlv_query_urr_t;



#pragma pack(4)
typedef struct pfcp_tlv_usage_report_session_modification_response_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_ur_seqn_t ur_seqn;
    pfcp_tlv_usage_report_trigger_t usage_report_trigger;
    pfcp_tlv_start_time_t start_time;
    pfcp_tlv_end_time_t end_time;
    pfcp_tlv_volume_measurement_t volume_measurement;
    pfcp_tlv_duration_measurement_t duration_measurement;
    pfcp_tlv_time_of_first_packet_t time_of_first_packet;
    pfcp_tlv_time_of_last_packet_t time_of_last_packet;
    pfcp_tlv_usage_information_t usage_information;
    pfcp_tlv_query_urr_reference_t query_urr_reference;
    pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;
} pfcp_tlv_usage_report_session_modification_response_t;

#pragma pack(4)
typedef struct pfcp_tlv_usage_report_session_deletion_response_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_ur_seqn_t ur_seqn;
    pfcp_tlv_usage_report_trigger_t usage_report_trigger;
    pfcp_tlv_start_time_t start_time;
    pfcp_tlv_end_time_t end_time;
    pfcp_tlv_volume_measurement_t volume_measurement;
    pfcp_tlv_duration_measurement_t duration_measurement;
    pfcp_tlv_time_of_first_packet_t time_of_first_packet;
    pfcp_tlv_time_of_last_packet_t time_of_last_packet;
    pfcp_tlv_usage_information_t usage_information;
    pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;
} pfcp_tlv_usage_report_session_deletion_response_t;

#pragma pack(4)
typedef struct pfcp_tlv_usage_report_session_report_request_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_urr_id_t urr_id;
    pfcp_tlv_ur_seqn_t ur_seqn;
    pfcp_tlv_usage_report_trigger_t usage_report_trigger;
    pfcp_tlv_start_time_t start_time;
    pfcp_tlv_end_time_t end_time;
    pfcp_tlv_volume_measurement_t volume_measurement;
    pfcp_tlv_duration_measurement_t duration_measurement;
    pfcp_tlv_application_detection_information_t application_detection_information;
    pfcp_tlv_ue_ip_address_t ue_ip_address;
    pfcp_tlv_network_instance_t network_instance;
    pfcp_tlv_time_of_first_packet_t time_of_first_packet;
    pfcp_tlv_time_of_last_packet_t time_of_last_packet;
    pfcp_tlv_usage_information_t usage_information;
    pfcp_tlv_query_urr_reference_t query_urr_reference;
    pfcp_tlv_event_time_stamp_t event_time_stamp;
    pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;
} pfcp_tlv_usage_report_session_report_request_t;

#pragma pack(4)
typedef struct pfcp_tlv_downlink_data_report_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_pdr_id_t pdr_id;
    pfcp_tlv_downlink_data_service_information_t downlink_data_service_information;
} pfcp_tlv_downlink_data_report_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_bar_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_bar_id_t bar_id;
    pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;
    pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;
} pfcp_tlv_create_bar_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_bar_session_modification_request_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_bar_id_t bar_id;
    pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;
    pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;
} pfcp_tlv_update_bar_session_modification_request_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_bar_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_bar_id_t bar_id;
} pfcp_tlv_remove_bar_t;

#pragma pack(4)
typedef struct pfcp_tlv_error_indication_report_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_f_teid_t remote_f_teid;
} pfcp_tlv_error_indication_report_t;

#pragma pack(4)
typedef struct pfcp_tlv_user_plane_path_failure_report_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_remote_gtp_u_peer_t remote_gtp_u_peer_;
} pfcp_tlv_user_plane_path_failure_report_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_traffic_endpoint_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;
    pfcp_tlv_f_teid_t local_f_teid;
    pfcp_tlv_network_instance_t network_instance;
    pfcp_tlv_ue_ip_address_t ue_ip_address;
    pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;
    pfcp_tlv_framed_route_t framed_route;
    pfcp_tlv_framed_routing_t framed_routing;
    pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;
    pfcp_tlv_qfi_t qfi;
} pfcp_tlv_create_traffic_endpoint_t;

#pragma pack(4)
typedef struct pfcp_tlv_created_traffic_endpoint_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;
    pfcp_tlv_f_teid_t local_f_teid;
    pfcp_tlv_ue_ip_address_t ue_ip_address;
} pfcp_tlv_created_traffic_endpoint_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_traffic_endpoint_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;
} pfcp_tlv_remove_traffic_endpoint_t;

#pragma pack(4)
typedef struct pfcp_tlv_create_mar_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_mar_id_t mar_id;
    pfcp_tlv_steering_functionality_t steering_functionality;
    pfcp_tlv_steering_mode_t steering_mode;
    pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
    pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
} pfcp_tlv_create_mar_t;

#pragma pack(4)
typedef struct pfcp_tlv_remove_mar_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_mar_id_t mar_id;
} pfcp_tlv_remove_mar_t;

#pragma pack(4)
typedef struct pfcp_tlv_update_mar_s {
    pfcp_tlv_presence_t presence;
    pfcp_tlv_mar_id_t mar_id;
    pfcp_tlv_steering_functionality_t steering_functionality;
    pfcp_tlv_steering_mode_t steering_mode;
    pfcp_tlv_update_access_forwarding_action_information_1_t update_access_forwarding_action_information_1;
    pfcp_tlv_update_access_forwarding_action_information_2_t update_access_forwarding_action_information_2;
    pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
    pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
} pfcp_tlv_update_mar_t;



/* Structure for Message */
#pragma pack(4)
typedef struct pfcp_heartbeat_request_s {
    pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
} pfcp_heartbeat_request_t;

#pragma pack(4)
typedef struct pfcp_heartbeat_response_s {
    pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
} pfcp_heartbeat_response_t;

#pragma pack(4)
typedef struct pfcp_pfd_management_request_s {
    pfcp_tlv_application_id_s_pfds_t application_id_s_pfds;
} pfcp_pfd_management_request_t;

#pragma pack(4)
typedef struct pfcp_pfd_management_response_s {
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
} pfcp_pfd_management_response_t;

#pragma pack(4)
typedef struct pfcp_association_setup_request_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
    pfcp_tlv_up_function_features_t up_function_features;
    pfcp_tlv_cp_function_features_t cp_function_features;
    pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];
    pfcp_tlv_ue_ip_address_t ue_ip_address_pool_identity;
    pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;
    pfcp_tlv_smf_set_id_t smf_set_id;
} pfcp_association_setup_request_t;

#pragma pack(4)
typedef struct pfcp_association_setup_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
    pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
    pfcp_tlv_up_function_features_t up_function_features;
    pfcp_tlv_cp_function_features_t cp_function_features;
    pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];
    pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;
} pfcp_association_setup_response_t;

#pragma pack(4)
typedef struct pfcp_association_update_request_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_up_function_features_t up_function_features;
    pfcp_tlv_cp_function_features_t cp_function_features;
    pfcp_tlv_pfcp_association_release_request_t pfcp_association_release_request;
    pfcp_tlv_graceful_release_period_t graceful_release_period;
    pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];
    pfcp_tlv_pfcpaureq_flags_t pfcpaureq_flags;
    pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;
} pfcp_association_update_request_t;

#pragma pack(4)
typedef struct pfcp_association_update_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
    pfcp_tlv_up_function_features_t up_function_features;
    pfcp_tlv_cp_function_features_t cp_function_features;
} pfcp_association_update_response_t;

#pragma pack(4)
typedef struct pfcp_association_release_request_s {
    pfcp_tlv_node_id_t node_id;
} pfcp_association_release_request_t;

#pragma pack(4)
typedef struct pfcp_association_release_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
} pfcp_association_release_response_t;

#pragma pack(4)
typedef struct pfcp_version_not_supported_response_s {
} pfcp_version_not_supported_response_t;

#pragma pack(4)
typedef struct pfcp_node_report_request_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_node_report_type_t node_report_type;
    pfcp_tlv_user_plane_path_failure_report_t user_plane_path_failure_report;
} pfcp_node_report_request_t;

#pragma pack(4)
typedef struct pfcp_node_report_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
} pfcp_node_report_response_t;

#pragma pack(4)
typedef struct pfcp_session_set_deletion_request_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_fq_csid_t sgw_c_fq_csid;
    pfcp_tlv_fq_csid_t pgw_c_fq_csid;
    pfcp_tlv_fq_csid_t sgw_u_fq_csid;
    pfcp_tlv_fq_csid_t pgw_u_fq_csid;
    pfcp_tlv_fq_csid_t twan_fq_csid;
    pfcp_tlv_fq_csid_t epdg_fq_csid;
    pfcp_tlv_fq_csid_t mme_fq_csid;
} pfcp_session_set_deletion_request_t;

#pragma pack(4)
typedef struct pfcp_session_set_deletion_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
} pfcp_session_set_deletion_response_t;

#pragma pack(4)
typedef struct pfcp_session_establishment_request_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_f_seid_t cp_f_seid;
    pfcp_tlv_create_pdr_t create_pdr[8];
    pfcp_tlv_create_far_t create_far[8];
    pfcp_tlv_create_urr_t create_urr[10];
    pfcp_tlv_create_qer_t create_qer[4];
    pfcp_tlv_create_bar_t create_bar;
    pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;
    pfcp_tlv_pdn_type_t pdn_type;
    pfcp_tlv_fq_csid_t sgw_c_fq_csid;
    pfcp_tlv_fq_csid_t mme_fq_csid;
    pfcp_tlv_fq_csid_t pgw_c_fq_csid;
    pfcp_tlv_fq_csid_t epdg_fq_csid;
    pfcp_tlv_fq_csid_t twan_fq_csid;
    pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;
    pfcp_tlv_user_id_t user_id;
    pfcp_tlv_trace_information_t trace_information;
    pfcp_tlv_apn_dnn_t apn_dnn;
    pfcp_tlv_create_mar_t create_mar;
} pfcp_session_establishment_request_t;

#pragma pack(4)
typedef struct pfcp_session_establishment_response_s {
    pfcp_tlv_node_id_t node_id;
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
    pfcp_tlv_f_seid_t up_f_seid;
    pfcp_tlv_created_pdr_t created_pdr[8];
    pfcp_tlv_load_control_information_t load_control_information;
    pfcp_tlv_overload_control_information_t overload_control_information;
    pfcp_tlv_fq_csid_t sgw_u_fq_csid;
    pfcp_tlv_fq_csid_t pgw_u_fq_csid;
    pfcp_tlv_failed_rule_id_t failed_rule_id;
    pfcp_tlv_created_traffic_endpoint_t created_traffic_endpoint;
} pfcp_session_establishment_response_t;

#pragma pack(4)
typedef struct pfcp_session_modification_request_s {
    pfcp_tlv_f_seid_t cp_f_seid;
    pfcp_tlv_remove_pdr_t remove_pdr[8];
    pfcp_tlv_remove_far_t remove_far[8];
    pfcp_tlv_remove_urr_t remove_urr[10];
    pfcp_tlv_remove_qer_t remove_qer[4];
    pfcp_tlv_remove_bar_t remove_bar;
    pfcp_tlv_remove_traffic_endpoint_t remove_traffic_endpoint;
    pfcp_tlv_create_pdr_t create_pdr[8];
    pfcp_tlv_create_far_t create_far[8];
    pfcp_tlv_create_urr_t create_urr[10];
    pfcp_tlv_create_qer_t create_qer[4];
    pfcp_tlv_create_bar_t create_bar;
    pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;
    pfcp_tlv_update_pdr_t update_pdr[8];
    pfcp_tlv_update_far_t update_far[8];
    pfcp_tlv_update_urr_t update_urr[10];
    pfcp_tlv_update_qer_t update_qer[4];
    pfcp_tlv_update_bar_session_modification_request_t update_bar;
    pfcp_tlv_update_traffic_endpoint_t update_traffic_endpoint;
    pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;
    pfcp_tlv_query_urr_t query_urr;
    pfcp_tlv_fq_csid_t pgw_c_fq_csid;
    pfcp_tlv_fq_csid_t sgw_c_fq_csid;
    pfcp_tlv_fq_csid_t mme_fq_csid;
    pfcp_tlv_fq_csid_t epdg_fq_csid;
    pfcp_tlv_fq_csid_t twan_fq_csid;
    pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;
    pfcp_tlv_query_urr_reference_t query_urr_reference;
    pfcp_tlv_trace_information_t trace_information;
    pfcp_tlv_remove_mar_t remove_mar;
    pfcp_tlv_update_mar_t update_mar;
    pfcp_tlv_create_mar_t create_mar;
    pfcp_tlv_node_id_t node_id;
} pfcp_session_modification_request_t;

#pragma pack(4)
typedef struct pfcp_session_modification_response_s {
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
    pfcp_tlv_created_pdr_t created_pdr[8];
    pfcp_tlv_load_control_information_t load_control_information;
    pfcp_tlv_overload_control_information_t overload_control_information;
    pfcp_tlv_usage_report_session_modification_response_t usage_report;
    pfcp_tlv_failed_rule_id_t failed_rule_id;
    pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;
    pfcp_tlv_created_traffic_endpoint_t created_updated_traffic_endpoint;
} pfcp_session_modification_response_t;

#pragma pack(4)
typedef struct pfcp_session_deletion_request_s {
} pfcp_session_deletion_request_t;

#pragma pack(4)
typedef struct pfcp_session_deletion_response_s {
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
    pfcp_tlv_load_control_information_t load_control_information;
    pfcp_tlv_overload_control_information_t overload_control_information;
    pfcp_tlv_usage_report_session_deletion_response_t usage_report;
} pfcp_session_deletion_response_t;

#pragma pack(4)
typedef struct pfcp_session_report_request_s {
    pfcp_tlv_report_type_t report_type;
    pfcp_tlv_downlink_data_report_t downlink_data_report;
    pfcp_tlv_usage_report_session_report_request_t usage_report;
    pfcp_tlv_error_indication_report_t error_indication_report;
    pfcp_tlv_load_control_information_t load_control_information;
    pfcp_tlv_overload_control_information_t overload_control_information;
    pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;
    pfcp_tlv_pfcpsrreq_flags_t pfcpsrreq_flags;
    pfcp_tlv_f_seid_t old_cp_f_seid;
} pfcp_session_report_request_t;

#pragma pack(4)
typedef struct pfcp_session_report_response_s {
    pfcp_tlv_cause_t cause;
    pfcp_tlv_offending_ie_t offending_ie;
    pfcp_tlv_update_bar_pfcp_session_report_response_t update_bar;
    pfcp_tlv_pfcpsrrsp_flags_t pfcpsrrsp_flags;
    pfcp_tlv_f_seid_t cp_f_seid;
    pfcp_tlv_f_teid_t n4_u_f_teid;
    pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;
} pfcp_session_report_response_t;


#pragma pack(4)
typedef struct pfcp_message_s {
	pfcp_header_t header;
	uint64_t SEID;
	uint32_t hasSEID;
	uint32_t SequenceNumber;
	uint16_t length;
	uint16_t MsgType;
	union {
		pfcp_heartbeat_request_t heartbeat_request;
		pfcp_heartbeat_response_t heartbeat_response;
		pfcp_pfd_management_request_t pfd_management_request;
		pfcp_pfd_management_response_t pfd_management_response;
		pfcp_association_setup_request_t association_setup_request;
		pfcp_association_setup_response_t association_setup_response;
		pfcp_association_update_request_t association_update_request;
		pfcp_association_update_response_t association_update_response;
		pfcp_association_release_request_t association_release_request;
		pfcp_association_release_response_t association_release_response;
		pfcp_version_not_supported_response_t version_not_supported_response;
		pfcp_node_report_request_t node_report_request;
		pfcp_node_report_response_t node_report_response;
		pfcp_session_set_deletion_request_t session_set_deletion_request;
		pfcp_session_set_deletion_response_t session_set_deletion_response;
		pfcp_session_establishment_request_t session_establishment_request;
		pfcp_session_establishment_response_t session_establishment_response;
		pfcp_session_modification_request_t session_modification_request;
		pfcp_session_modification_response_t session_modification_response;
		pfcp_session_deletion_request_t session_deletion_request;
		pfcp_session_deletion_response_t session_deletion_response;
		pfcp_session_report_request_t session_report_request;
		pfcp_session_report_response_t session_report_response;
	} u;
} pfcp_message_t;






#endif


