#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>


#ifndef S_PFCP_PARSER_DEF
#define S_PFCP_PARSER_DEF

#include "app_endpoint.h"
#include "app_stack.h"

void __up_pfcp__set_data_region( app_data_region_t * pfcp_mregion);

uint8_t __up_pfcp__getElementType( uint16_t ieType);

int __up_pfcp_set__u8( pfcp_tlv_uint8_t * obj, uint8_t val);
int __up_pfcp_set__u16( pfcp_tlv_uint16_t * obj, uint16_t val);
int __up_pfcp_set__u32( pfcp_tlv_uint32_t * obj, uint32_t val);
//int __up_pfcp_set__octet( app_data_region_t * pfcp_mregion, pfcp_tlv_octet_t * obj, void * data, uint32_t len);
int __up_pfcp_set__octet( app_data_region_t * pfcp_mregion, pfcp_tlv_octet_t * obj, char * data, uint32_t len);
void __up_pfcp_set__header( pfcp_message_t * msg, uint8_t msgtype, uint8_t has_seid, uint64_t seid, uint32_t seq);
uint32_t __up_pfcp_decode__header_get_sequence( uint8_t * buffer);

int __up_pfcp__print_element_dict();

int __up_pfcp_free__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj);
int __up_pfcp_free__pdi(pfcp_tlv_pdi_t * mObj);
int __up_pfcp_free__create_pdr(pfcp_tlv_create_pdr_t * mObj);
int __up_pfcp_free__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj);
int __up_pfcp_free__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj);
int __up_pfcp_free__create_far(pfcp_tlv_create_far_t * mObj);
int __up_pfcp_free__update_forwarding_parameters(pfcp_tlv_update_forwarding_parameters_t * mObj);
int __up_pfcp_free__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj);
int __up_pfcp_free__update_far(pfcp_tlv_update_far_t * mObj);
int __up_pfcp_free__pfd_context(pfcp_tlv_pfd_context_t * mObj);
int __up_pfcp_free__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj);
int __up_pfcp_free__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj);
int __up_pfcp_free__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj);
int __up_pfcp_free__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj);
int __up_pfcp_free__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj);
int __up_pfcp_free__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj);
int __up_pfcp_free__create_urr(pfcp_tlv_create_urr_t * mObj);
int __up_pfcp_free__create_qer(pfcp_tlv_create_qer_t * mObj);
int __up_pfcp_free__created_pdr(pfcp_tlv_created_pdr_t * mObj);
int __up_pfcp_free__update_pdr(pfcp_tlv_update_pdr_t * mObj);
int __up_pfcp_free__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj);
int __up_pfcp_free__update_urr(pfcp_tlv_update_urr_t * mObj);
int __up_pfcp_free__update_qer(pfcp_tlv_update_qer_t * mObj);
int __up_pfcp_free__remove_pdr(pfcp_tlv_remove_pdr_t * mObj);
int __up_pfcp_free__remove_far(pfcp_tlv_remove_far_t * mObj);
int __up_pfcp_free__remove_urr(pfcp_tlv_remove_urr_t * mObj);
int __up_pfcp_free__remove_qer(pfcp_tlv_remove_qer_t * mObj);
int __up_pfcp_free__load_control_information(pfcp_tlv_load_control_information_t * mObj);
int __up_pfcp_free__overload_control_information(pfcp_tlv_overload_control_information_t * mObj);
int __up_pfcp_free__application_detection_information(pfcp_tlv_application_detection_information_t * mObj);
int __up_pfcp_free__query_urr(pfcp_tlv_query_urr_t * mObj);
int __up_pfcp_free__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj);
int __up_pfcp_free__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj);
int __up_pfcp_free__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj);
int __up_pfcp_free__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj);
int __up_pfcp_free__create_bar(pfcp_tlv_create_bar_t * mObj);
int __up_pfcp_free__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj);
int __up_pfcp_free__remove_bar(pfcp_tlv_remove_bar_t * mObj);
int __up_pfcp_free__error_indication_report(pfcp_tlv_error_indication_report_t * mObj);
int __up_pfcp_free__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj);
int __up_pfcp_free__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj);
int __up_pfcp_free__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj);
int __up_pfcp_free__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj);
int __up_pfcp_free__create_mar(pfcp_tlv_create_mar_t * mObj);
int __up_pfcp_free__remove_mar(pfcp_tlv_remove_mar_t * mObj);
int __up_pfcp_free__update_mar(pfcp_tlv_update_mar_t * mObj);

int __up_pfcp_free__heartbeat_request( pfcp_message_t * mObj);
int __up_pfcp_free__heartbeat_response( pfcp_message_t * mObj);
int __up_pfcp_free__pfd_management_request( pfcp_message_t * mObj);
int __up_pfcp_free__pfd_management_response( pfcp_message_t * mObj);
int __up_pfcp_free__association_setup_request( pfcp_message_t * mObj);
int __up_pfcp_free__association_setup_response( pfcp_message_t * mObj);
int __up_pfcp_free__association_update_request( pfcp_message_t * mObj);
int __up_pfcp_free__association_update_response( pfcp_message_t * mObj);
int __up_pfcp_free__association_release_request( pfcp_message_t * mObj);
int __up_pfcp_free__association_release_response( pfcp_message_t * mObj);
int __up_pfcp_free__version_not_supported_response( pfcp_message_t * mObj);
int __up_pfcp_free__node_report_request( pfcp_message_t * mObj);
int __up_pfcp_free__node_report_response( pfcp_message_t * mObj);
int __up_pfcp_free__session_set_deletion_request( pfcp_message_t * mObj);
int __up_pfcp_free__session_set_deletion_response( pfcp_message_t * mObj);
int __up_pfcp_free__session_establishment_request( pfcp_message_t * mObj);
int __up_pfcp_free__session_establishment_response( pfcp_message_t * mObj);
int __up_pfcp_free__session_modification_request( pfcp_message_t * mObj);
int __up_pfcp_free__session_modification_response( pfcp_message_t * mObj);
int __up_pfcp_free__session_deletion_request( pfcp_message_t * mObj);
int __up_pfcp_free__session_deletion_response( pfcp_message_t * mObj);
int __up_pfcp_free__session_report_request( pfcp_message_t * mObj);
int __up_pfcp_free__session_report_response( pfcp_message_t * mObj);

int __up_pfcp_free__message( pfcp_message_t * mObj);


int __up_pfcp_decode__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__pdi(pfcp_tlv_pdi_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_pdr(pfcp_tlv_create_pdr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_far(pfcp_tlv_create_far_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_forwarding_parameters(pfcp_tlv_update_forwarding_parameters_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_far(pfcp_tlv_update_far_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__pfd_context(pfcp_tlv_pfd_context_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_urr(pfcp_tlv_create_urr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_qer(pfcp_tlv_create_qer_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__created_pdr(pfcp_tlv_created_pdr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_pdr(pfcp_tlv_update_pdr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_urr(pfcp_tlv_update_urr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_qer(pfcp_tlv_update_qer_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_pdr(pfcp_tlv_remove_pdr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_far(pfcp_tlv_remove_far_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_urr(pfcp_tlv_remove_urr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_qer(pfcp_tlv_remove_qer_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__load_control_information(pfcp_tlv_load_control_information_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__overload_control_information(pfcp_tlv_overload_control_information_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__application_detection_information(pfcp_tlv_application_detection_information_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__query_urr(pfcp_tlv_query_urr_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_bar(pfcp_tlv_create_bar_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_bar(pfcp_tlv_remove_bar_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__error_indication_report(pfcp_tlv_error_indication_report_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__create_mar(pfcp_tlv_create_mar_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__remove_mar(pfcp_tlv_remove_mar_t * mObj, uint8_t*, uint16_t);
int __up_pfcp_decode__update_mar(pfcp_tlv_update_mar_t * mObj, uint8_t*, uint16_t);

int __up_pfcp_encode__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__pdi(pfcp_tlv_pdi_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_pdr(pfcp_tlv_create_pdr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_far(pfcp_tlv_create_far_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_forwarding_parameters(pfcp_tlv_update_forwarding_parameters_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_far(pfcp_tlv_update_far_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__pfd_context(pfcp_tlv_pfd_context_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_urr(pfcp_tlv_create_urr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_qer(pfcp_tlv_create_qer_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__created_pdr(pfcp_tlv_created_pdr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_pdr(pfcp_tlv_update_pdr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_urr(pfcp_tlv_update_urr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_qer(pfcp_tlv_update_qer_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_pdr(pfcp_tlv_remove_pdr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_far(pfcp_tlv_remove_far_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_urr(pfcp_tlv_remove_urr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_qer(pfcp_tlv_remove_qer_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__load_control_information(pfcp_tlv_load_control_information_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__overload_control_information(pfcp_tlv_overload_control_information_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__application_detection_information(pfcp_tlv_application_detection_information_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__query_urr(pfcp_tlv_query_urr_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_bar(pfcp_tlv_create_bar_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_bar(pfcp_tlv_remove_bar_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__error_indication_report(pfcp_tlv_error_indication_report_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__create_mar(pfcp_tlv_create_mar_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__remove_mar(pfcp_tlv_remove_mar_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__update_mar(pfcp_tlv_update_mar_t * mObj, app_ep_udp_message_t * bObj);

int __up_pfcp_decode__heartbeat_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__heartbeat_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__pfd_management_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__pfd_management_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_setup_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_setup_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_update_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_update_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_release_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__association_release_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__version_not_supported_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__node_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__node_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_set_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_set_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_establishment_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_establishment_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_modification_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_modification_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__session_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_decode__message( uint8_t MessageType, pfcp_message_t * mObj, app_ep_udp_message_t * bObj);

int __up_pfcp_encode__heartbeat_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__heartbeat_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__pfd_management_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__pfd_management_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_setup_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_setup_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_update_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_update_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_release_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__association_release_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__version_not_supported_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__node_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__node_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_set_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_set_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_establishment_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_establishment_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_modification_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_modification_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);
int __up_pfcp_encode__session_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);

int __up_pfcp_encode__message( pfcp_message_t * mObj, app_ep_udp_message_t * bObj);


#endif