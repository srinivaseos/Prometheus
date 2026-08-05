#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "pfcp_protocol_def.h"
#include "pfcp_types.h"
#include "app_endpoint.h"
#include "pfcp_parser.h"

app_data_region_t * lpfcp_mregion = NULL;
void __up_pfcp__set_data_region( app_data_region_t * pfcp_mregion)
{
	lpfcp_mregion = pfcp_mregion;
}

void __up_pfcp_encode__tlv_oct( uint16_t ieType, pfcp_tlv_octet_t * oct, app_ep_udp_message_t * bObj);

void __up_pfcp_decode__tlv_u8( pfcp_tlv_uint8_t * u8, char * data)
{
	u8->presence = 1;
	u8->u8 = data[0] & 0xFF;
}

void __up_pfcp_decode__tlv_u16( pfcp_tlv_uint16_t * u16, char * data)
{
	u16->presence = 1;
	u16->u16 = app_ep__get_u16( data);	
}

void __up_pfcp_decode__tlv_u32( pfcp_tlv_uint32_t * u32, char * data)
{
	u32->presence = 1;
	u32->u32 = app_ep__get_u32( data);	
}

int __up_pfcp_set__octet( app_data_region_t * pfcp_mregion, pfcp_tlv_octet_t * obj, char * data, uint32_t len);

void __up_pfcp_decode__tlv_oct( pfcp_tlv_octet_t * oct, uint8_t * buffer, uint32_t len)
{
	__up_pfcp_set__octet( lpfcp_mregion, oct, buffer, len);
}


void __up_pfcp_encode__tlv_uint8( uint16_t ieType, pfcp_tlv_uint8_t * t8, app_ep_udp_message_t * bObj)
{
	if( t8->presence == 1)
	{
		app_ep__encode__u16( bObj, ieType);
		app_ep__encode__u16( bObj, 1);
		app_ep__encode__u8( bObj, t8->u8);
	}
}

void __up_pfcp_encode__tlv_uint16( uint16_t ieType, pfcp_tlv_uint16_t * t16, app_ep_udp_message_t * bObj)
{
	if( t16->presence == 1)
	{
		app_ep__encode__u16( bObj, ieType);
		app_ep__encode__u16( bObj, 2);
		app_ep__encode__u16( bObj, t16->u16);
	}
}

void __up_pfcp_encode__tlv_uint32( uint16_t ieType, pfcp_tlv_uint32_t * t32, app_ep_udp_message_t * bObj)
{
	if( t32->presence == 1)
	{
		app_ep__encode__u16( bObj, ieType);
		app_ep__encode__u16( bObj, 4);
		app_ep__encode__u32( bObj, t32->u32);
	}
}


void __up_pfcp_encode__tlv_oct( uint16_t ieType, pfcp_tlv_octet_t * oct, app_ep_udp_message_t * bObj)
{
	if( oct->presence == 1)
	{
		app_ep__encode__u16( bObj, ieType);
		app_ep__encode__u16( bObj, oct->len);
		
		if( oct->len > 0)
		{
			app_ep__encode__str( bObj, oct->data, oct->len);
		}
		
		// if( PFCP_IE_OUTER_HEADER_CREATION == ieType)
		// {
			// //printf("PFCP_IE_OUTER_HEADER_CREATION=%u  len=%u %s|%s|%d\n", ieType, oct->len, __FILE__, __FUNCTION__, __LINE__);;
		// }
	}	
}


void __up_pfcp_set__header( pfcp_message_t * msg, uint8_t msgtype, uint8_t has_seid, uint64_t seid, uint32_t seq)
{
	//printf("sizeof->pfcp_message_t=%ld  %s|%s|%d\n", sizeof(pfcp_message_t), __FILE__, __FUNCTION__, __LINE__);	
	memset( msg, 0, sizeof(pfcp_message_t));
	msg->header.Flags.Version = 1;
	msg->header.Flags.SEID = has_seid;
	msg->header.MessageType = msgtype;
	if( has_seid == 1) {
		msg->header.SEID = seid;
	}
	msg->header.SequenceNumber = seq;
}


int __up_pfcp_set__u8( pfcp_tlv_uint8_t * obj, uint8_t val)
{
	obj->presence = 1;
	obj->u8 = val;
	return 0;
}

int __up_pfcp_set__u16( pfcp_tlv_uint16_t * obj, uint16_t val)
{
	obj->presence = 1;
	obj->u16 = val;
	return 0;
}

int __up_pfcp_set__u32( pfcp_tlv_uint32_t * obj, uint32_t val)
{
	obj->presence = 1;
	obj->u32 = val;	
	return 0;
}



int __up_pfcp_set__octet( app_data_region_t * pfcp_mregion, pfcp_tlv_octet_t * obj, char * data, uint32_t len)
{
	obj->presence = 1;
	obj->data = app_region__allocate_fr( pfcp_mregion, len + 1);
	memcpy( obj->data, data, len);
	obj->len = len;
	return 0;
}


int __up_pfcp_encode__header( pfcp_header_t * header, app_ep_udp_message_t * bObj)
{
	app_ep__encode__u8( bObj, *(uint8_t*)&header->Flags);
	app_ep__encode__u8( bObj, header->MessageType);
	app_ep__encode__u16( bObj, 0);
	
	if( header->Flags.SEID == 1)
	{
		app_ep__encode__u64( bObj, header->SEID);
	}
	
	app_ep__encode__u24( bObj, header->SequenceNumber);		
	app_ep__encode__u8( bObj, 0);
	
	return 0;
}



uint8_t __up_pfcp__getElementType( uint16_t ieType)
{
	switch( ieType)
	{
		case PFCP_IE_CAUSE:
		case PFCP_IE_SOURCE_INTERFACE:
		case PFCP_IE_GATE_STATUS:
		case PFCP_IE_REPORTING_TRIGGERS:
		case PFCP_IE_REPORT_TYPE:
		case PFCP_IE_DESTINATION_INTERFACE:
		case PFCP_IE_APPLY_ACTION:
		case PFCP_IE_PFCPSMREQ_FLAGS:
		case PFCP_IE_PFCPSRRSP_FLAGS:
		case PFCP_IE_MEASUREMENT_METHOD:
		case PFCP_IE_BAR_ID:
		case PFCP_IE_CP_FUNCTION_FEATURES:
		case PFCP_IE_PDN_TYPE:
		case PFCP_IE_RQI:
		case PFCP_IE_QFI:
		case PFCP_IE_PAGING_POLICY_INDICATOR_TYPE:
		case PFCP_IE_PFCPSRREQ_FLAGS_TYPE:
		case PFCP_IE_PFCPAUREQ_FLAGS_TYPE:
			return PFCP__E_TYPE_U8;
		case PFCP_IE_OFFENDING_IE:
		case PFCP_IE_PACKET_DETECTION_RULE_ID:
			return PFCP__E_TYPE_U16;
		case PFCP_IE_QER_CORRELATION_ID:
		case PFCP_IE_PRECEDENCE:
		case PFCP_IE_URR_ID:
		case PFCP_IE_RECOVERY_TIME_STAMP:
		case PFCP_IE_FAR_ID:
		case PFCP_IE_QER_ID:
		case PFCP_IE_AVERAGING_WINDOW_TYPE:
			return PFCP__E_TYPE_U32;
		case PFCP_IE_F_TEID:
		case PFCP_IE_NETWORK_INSTANCE:
		case PFCP_IE_SDF_FILTER:
		case PFCP_IE_APPLICATION_ID:
		case PFCP_IE_MBR:
		case PFCP_IE_GBR:
		case PFCP_IE_TRANSPORT_LEVEL_MARKING:
		case PFCP_IE_VOLUME_THRESHOLD:
		case PFCP_IE_TIME_THRESHOLD:
		case PFCP_IE_MONITORING_TIME:
		case PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD:
		case PFCP_IE_SUBSEQUENT_TIME_THRESHOLD:
		case PFCP_IE_INACTIVITY_DETECTION_TIME:
		case PFCP_IE_REDIRECT_INFORMATION:
		case PFCP_IE_FORWARDING_POLICY:
		case PFCP_IE_UP_FUNCTION_FEATURES:
		case PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION:
		case PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY:
		case PFCP_IE_DL_BUFFERING_DURATION:
		case PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT:
		case PFCP_IE_SEQUENCE_NUMBER:
		case PFCP_IE_METRIC:
		case PFCP_IE_TIMER:
		case PFCP_IE_F_SEID:
		case PFCP_IE_NODE_ID:
		case PFCP_IE_PFD_CONTENTS:
		case PFCP_IE_USAGE_REPORT_TRIGGER:
		case PFCP_IE_MEASUREMENT_PERIOD:
		case PFCP_IE_FQ_CSID:
		case PFCP_IE_VOLUME_MEASUREMENT:
		case PFCP_IE_DURATION_MEASUREMENT:
		case PFCP_IE_TIME_OF_FIRST_PACKET:
		case PFCP_IE_TIME_OF_LAST_PACKET:
		case PFCP_IE_QUOTA_HOLDING_TIME:
		case PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD:
		case PFCP_IE_VOLUME_QUOTA:
		case PFCP_IE_TIME_QUOTA:
		case PFCP_IE_START_TIME:
		case PFCP_IE_END_TIME:
		case PFCP_IE_LINKED_URR_ID:
		case PFCP_IE_OUTER_HEADER_CREATION:
		case PFCP_IE_USAGE_INFORMATION:
		case PFCP_IE_APPLICATION_INSTANCE_ID:
		case PFCP_IE_FLOW_INFORMATION:
		case PFCP_IE_UE_IP_ADDRESS:
		case PFCP_IE_PACKET_RATE:
		case PFCP_IE_OUTER_HEADER_REMOVAL:
		case PFCP_IE_DL_FLOW_LEVEL_MARKING:
		case PFCP_IE_HEADER_ENRICHMENT:
		case PFCP_IE_MEASUREMENT_INFORMATION:
		case PFCP_IE_NODE_REPORT_TYPE:
		case PFCP_IE_REMOTE_GTP_U_PEER:
		case PFCP_IE_UR_SEQN:
		case PFCP_IE_ACTIVATE_PREDEFINED_RULES:
		case PFCP_IE_OCI_FLAGS:
		case PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST:
		case PFCP_IE_GRACEFUL_RELEASE_PERIOD:
		case PFCP_IE_FAILED_RULE_ID:
		case PFCP_IE_TIME_QUOTA_MECHANISM:
		case PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION:
		case PFCP_IE_USER_PLANE_INACTIVITY_TIMER:
		case PFCP_IE_AGGREGATED_URRS:
		case PFCP_IE_MULTIPLIER:
		case PFCP_IE_AGGREGATED_URR_ID:
		case PFCP_IE_SUBSEQUENT_VOLUME_QUOTA:
		case PFCP_IE_SUBSEQUENT_TIME_QUOTA:
		case PFCP_IE_QUERY_URR_REFERENCE:
		case PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION:
		case PFCP_IE_UPDATE_TRAFFIC_ENDPOINT:
		case PFCP_IE_TRAFFIC_ENDPOINT_ID:
		case PFCP_IE_MAC_ADDRESS:
		case PFCP_IE_C_TAG:
		case PFCP_IE_S_TAG:
		case PFCP_IE_ETHERTYPE:
		case PFCP_IE_PROXYING:
		case PFCP_IE_ETHERNET_FILTER_ID:
		case PFCP_IE_ETHERNET_FILTER_PROPERTIES:
		case PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT:
		case PFCP_IE_USER_ID:
		case PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION:
		case PFCP_IE_MAC_ADDRESSES_DETECTED:
		case PFCP_IE_MAC_ADDRESSES_REMOVED:
		case PFCP_IE_ETHERNET_INACTIVITY_TIMER:
		case PFCP_IE_ADDITIONAL_MONITORING_TIME:
		case PFCP_IE_EVENT_QUOTA:							//pfcp_tlv_event_quota_t
		case PFCP_IE_EVENT_THRESHOLD:						//pfcp_tlv_event_threshold_t
		case PFCP_IE_SUBS_EVENT_QUOTA:						//pfcp_tlv_subsequent_event_quota_t
		case PFCP_IE_SUBS_EVENT_THRESHOLD:					//pfcp_tlv_subsequent_event_threshold_t
		case PFCP_IE_TRACE_INFORMATION:
		case PFCP_IE_FRAMED_ROUTE:
		case PFCP_IE_FRAMED_ROUTING:
		case PFCP_IE_FRAMED_IPV6_ROUTE:
		case PFCP_IE_EVENT_TIME_STAMP_TYPE:
		case PFCP_IE_APN_DNN_TYPE:
		case PFCP_IE_INTERFACE_TYPE_TYPE:
		case PFCP_IE_ACTIVATION_TIME_TYPE:
		case PFCP_IE_DEACTIVATION_TIME_TYPE:
		case PFCP_IE_MAR_ID_TYPE:
		case PFCP_IE_STEERING_FUNCTIONALITY_TYPE:
		case PFCP_IE_STEERING_MODE_TYPE:
		case PFCP_IE_WEIGHT_TYPE:
		case PFCP_IE_PRIORITY_TYPE:
		case PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE:
		case PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE:
		case PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE:
		case PFCP_IE_SMF_SET_ID_TYPE:
		case PFCP_IE_QUOTA_VALIDITY_TIME_TYPE:
			return PFCP__E_TYPE_OCT;
		case PFCP_IE_PFD_CONTEXT:
		case PFCP_IE_ETHERNET_PACKET_FILTER:
		case PFCP_IE_PDI:
		case PFCP_IE_FORWARDING_PARAMETERS:
		case PFCP_IE_DUPLICATING_PARAMETERS:
		case PFCP_IE_CREATE_FAR:
		case PFCP_IE_UPDATE_FORWARDING_PARAMETERS:
		case PFCP_IE_UPDATE_DUPLICATING_PARAMETERS:
		case PFCP_IE_UPDATE_FAR:
		case PFCP_IE_APPLICATION_IDS_PFDS:
		case PFCP_IE_ETHERNET_TRAFFIC_INFORMATION:
		case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE:
		case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE:
		case PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE:
		case PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE:
		case PFCP_IE_CREATE_URR:
		case PFCP_IE_CREATE_QER:
		case PFCP_IE_CREATE_PDR:
		case PFCP_IE_UPDATE_PDR:
		case PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE:
		case PFCP_IE_UPDATE_URR:
		case PFCP_IE_UPDATE_QER:
		case PFCP_IE_REMOVE_PDR:
		case PFCP_IE_REMOVE_FAR:
		case PFCP_IE_REMOVE_URR:
		case PFCP_IE_REMOVE_QER:
		case PFCP_IE_LOAD_CONTROL_INFORMATION:
		case PFCP_IE_OVERLOAD_CONTROL_INFORMATION:
		case PFCP_IE_APPLICATION_DETECTION_INFORMATION:
		case PFCP_IE_QUERY_URR:
		case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE:
		case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE:
		case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST:
		case PFCP_IE_DOWNLINK_DATA_REPORT:
		case PFCP_IE_CREATE_BAR:
		case PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST:
		case PFCP_IE_REMOVE_BAR:
		case PFCP_IE_ERROR_INDICATION_REPORT:
		case PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT:
		case PFCP_IE_CREATE_TRAFFIC_ENDPOINT:
		case PFCP_IE_CREATED_TRAFFIC_ENDPOINT:
		case PFCP_IE_REMOVE_TRAFFIC_ENDPOINT:
		case PFCP_IE_CREATE_MAR_TYPE:
		case PFCP_IE_REMOVE_MAR_TYPE:
		case PFCP_IE_UPDATE_MAR_TYPE:
			return PFCP__E_TYPE_GRP;
	}
	
	return PFCP__E_TYPE_UNKNOWN;
}

typedef int (*fp_decoder)(uint8_t*,app_ep_udp_message_t*);
typedef int (*fp_freef)(uint8_t*,app_ep_udp_message_t*);

typedef struct pfcp_element_dict
{
	char * name;
	uint16_t id;
	uint16_t fixedoctets;
	uint16_t type;
	fp_decoder fdc;
	fp_freef ffc;
} pfcp_element_dict_t;

pfcp_element_dict_t pfcp_elements[] = {
	{"Resvered",	PFCP_IE_RESERVED_0, 0, 0, NULL, NULL},
	{"Create PDR",	PFCP_IE_CREATE_PDR, 0, 0, (fp_decoder)__up_pfcp_decode__created_pdr, NULL},
	{"PDI",	PFCP_IE_PDI, 0, 0, (fp_decoder)__up_pfcp_decode__pdi, NULL},
	{"Create FAR",	PFCP_IE_CREATE_FAR, 0, 0, (fp_decoder)__up_pfcp_decode__create_far, NULL},
	{"Forwarding Parameters",	PFCP_IE_FORWARDING_PARAMETERS, 0, 0, (fp_decoder)__up_pfcp_decode__forwarding_parameters, NULL},
	{"Duplicating Parameters",	PFCP_IE_DUPLICATING_PARAMETERS, 0, 0, (fp_decoder)__up_pfcp_decode__duplicating_parameters, NULL},
	{"Create URR",	PFCP_IE_CREATE_URR, 0, 0, (fp_decoder)__up_pfcp_decode__create_urr, NULL},
	{"Create QER",	PFCP_IE_CREATE_QER, 0, 0, (fp_decoder)__up_pfcp_decode__create_qer, NULL},
	{"Created PDR",	PFCP_IE_CREATED_PDR, 0, 0, NULL, NULL},
	{"Update PDR",	PFCP_IE_UPDATE_PDR, 0, 0, (fp_decoder)__up_pfcp_decode__update_pdr, NULL},
	{"Update FAR",	PFCP_IE_UPDATE_FAR, 0, 0, (fp_decoder)__up_pfcp_decode__update_far, NULL},
	{"Update Forwarding Parameters",	PFCP_IE_UPDATE_FORWARDING_PARAMETERS, 0, 0, (fp_decoder)__up_pfcp_decode__update_forwarding_parameters, NULL},
	{"Update BAR (PFCP Session Report Response)",	PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE, 0, 0, (fp_decoder)__up_pfcp_decode__update_bar, NULL},
	{"Update URR",	PFCP_IE_UPDATE_URR, 0, 0, (fp_decoder)__up_pfcp_decode__update_urr, NULL},
	{"Update QER",	PFCP_IE_UPDATE_QER, 0, 0, (fp_decoder)__up_pfcp_decode__update_qer, NULL},
	{"Remove PDR",	PFCP_IE_REMOVE_PDR, 0, 0, (fp_decoder)__up_pfcp_decode__remove_pdr, NULL},
	{"Remove FAR",	PFCP_IE_REMOVE_FAR, 0, 0, (fp_decoder)__up_pfcp_decode__remove_far, NULL},
	{"Remove URR",	PFCP_IE_REMOVE_URR, 0, 0, (fp_decoder)__up_pfcp_decode__remove_urr, NULL},
	{"Remove QER",	PFCP_IE_REMOVE_QER, 0, 0, (fp_decoder)__up_pfcp_decode__remove_qer, NULL},
	{"Cause",	PFCP_IE_CAUSE, 1, 0, NULL, NULL},		//, pfcp_tlv_uint8_t
	{"Source Interface",	PFCP_IE_SOURCE_INTERFACE, 1, 0, NULL, NULL},
	{"F-TEID",	PFCP_IE_F_TEID, 1, 0, NULL, NULL},
	{"Network Instance",	PFCP_IE_NETWORK_INSTANCE, 0, 0, NULL, NULL},
	{"SDF Filter",	PFCP_IE_SDF_FILTER, 2, 0, NULL, NULL},
	{"Application ID",	PFCP_IE_APPLICATION_ID, 0, 0, NULL, NULL},
	{"Gate Status",	PFCP_IE_GATE_STATUS, 1, 0, NULL, NULL},
	{"MBR",	PFCP_IE_MBR, 10, 0, NULL, NULL},
	{"GBR",	PFCP_IE_GBR, 10, 0, NULL, NULL},
	{"QER Correlation ID",	PFCP_IE_QER_CORRELATION_ID, 4, 0, NULL, NULL},
	{"Precedence",	PFCP_IE_PRECEDENCE, 4, 0, NULL, NULL},
	{"Transport Level Marking",	PFCP_IE_TRANSPORT_LEVEL_MARKING, 2, 0, NULL, NULL},
	{"Volume Threshold",	PFCP_IE_VOLUME_THRESHOLD, 1, 0, NULL, NULL},
	{"Time Threshold",	PFCP_IE_TIME_THRESHOLD, 4, 0, NULL, NULL},
	{"Monitoring Time",	PFCP_IE_MONITORING_TIME, 4, 0, NULL, NULL},
	{"Subsequent Volume Threshold",	PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD, 1, 0, NULL, NULL},
	{"Subsequent Time Threshold",	PFCP_IE_SUBSEQUENT_TIME_THRESHOLD, 4, 0, NULL, NULL},
	{"Inactivity Detection Time",	PFCP_IE_INACTIVITY_DETECTION_TIME, 4, 0, NULL, NULL},
	{"Reporting Triggers",	PFCP_IE_REPORTING_TRIGGERS, 2, 0, NULL, NULL},
	{"Redirect Information",	PFCP_IE_REDIRECT_INFORMATION, 3, 0, NULL, NULL},
	{"Report Type",	PFCP_IE_REPORT_TYPE, 1, 0, NULL, NULL},
	{"Offending IE",	PFCP_IE_OFFENDING_IE, 2, 0, NULL, NULL},
	{"Forwarding Policy",	PFCP_IE_FORWARDING_POLICY, 1, 0, NULL, NULL},
	{"Destination Interface",	PFCP_IE_DESTINATION_INTERFACE, 1, 0, NULL, NULL},
	{"UP Function Features",	PFCP_IE_UP_FUNCTION_FEATURES, 1, 0, NULL, NULL},
	{"Apply Action",	PFCP_IE_APPLY_ACTION, 1, 0, NULL, NULL},
	{"Downlink Data Service Information",	PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION, 1, 0, NULL, NULL},
	{"Downlink Data Notification Delay",	PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY, 1, 0, NULL, NULL},
	{"DL Buffering Duration",	PFCP_IE_DL_BUFFERING_DURATION, 1, 0, NULL, NULL},
	{"DL Buffering Suggested Packet Count",	PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT, 0, 0, NULL, NULL},
	{"PFCPSMReq-Flags",	PFCP_IE_PFCPSMREQ_FLAGS, 0, 0, NULL, NULL},
	{"PFCPSRRsp-Flags",	PFCP_IE_PFCPSRRSP_FLAGS, 1, 0, NULL, NULL},
	{"Load Control Information",	PFCP_IE_LOAD_CONTROL_INFORMATION, 0, 0, (fp_decoder)__up_pfcp_decode__load_control_information, NULL},
	{"Sequence Number",	PFCP_IE_SEQUENCE_NUMBER, 4, 0, NULL, NULL},
	{"Metric",	PFCP_IE_METRIC, 1, 0, NULL, NULL},
	{"Overload Control Information",	PFCP_IE_OVERLOAD_CONTROL_INFORMATION, 0, 0, (fp_decoder)__up_pfcp_decode__overload_control_information, NULL},
	{"Timer",	PFCP_IE_TIMER, 1, 0, NULL, NULL},
	{"PDR ID",	PFCP_IE_PACKET_DETECTION_RULE_ID, 2, 0, NULL, NULL},
	{"F-SEID",	PFCP_IE_F_SEID, 9, 0, NULL, NULL},
	{"Application IDs PFDs",	PFCP_IE_APPLICATION_IDS_PFDS, 0, 0, (fp_decoder)__up_pfcp_decode__application_id_s_pfds, NULL},
	{"PFD context",	PFCP_IE_PFD_CONTEXT, 0, 0, (fp_decoder)__up_pfcp_decode__pfd_context, NULL},
	{"Node ID",	PFCP_IE_NODE_ID, 1, 0, NULL, NULL},
	{"PFD contents",	PFCP_IE_PFD_CONTENTS, 2, 0, NULL, NULL},
	{"Measurement Method",	PFCP_IE_MEASUREMENT_METHOD, 1, 0, NULL, NULL},
	{"Usage Report Trigger",	PFCP_IE_USAGE_REPORT_TRIGGER, 2, 0, NULL, NULL},
	{"Measurement Period",	PFCP_IE_MEASUREMENT_PERIOD, 4, 0, NULL, NULL},
	{"FQ-CSID",	PFCP_IE_FQ_CSID, 1, 0, NULL, NULL},
	{"Volume Measurement",	PFCP_IE_VOLUME_MEASUREMENT, 1, 0, NULL, NULL},
	{"Duration Measurement",	PFCP_IE_DURATION_MEASUREMENT, 4, 0, NULL, NULL},
	{"Application Detection Information",	PFCP_IE_APPLICATION_DETECTION_INFORMATION, 0, 0, (fp_decoder)__up_pfcp_decode__application_detection_information, NULL},
	{"Time of First Packet",	PFCP_IE_TIME_OF_FIRST_PACKET, 4, 0, NULL, NULL},
	{"Time of Last Packet",	PFCP_IE_TIME_OF_LAST_PACKET, 4, 0, NULL, NULL},
	{"Quota Holding Time",	PFCP_IE_QUOTA_HOLDING_TIME, 4, 0, NULL, NULL},
	{"Dropped DL Traffic Threshold",	PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD, 1, 0, NULL, NULL},
	{"Volume Quota",	PFCP_IE_VOLUME_QUOTA, 1, 0, NULL, NULL},
	{"Time Quota",	PFCP_IE_TIME_QUOTA, 4, 0, NULL, NULL},
	{"Start Time",	PFCP_IE_START_TIME, 4, 0, NULL, NULL},
	{"End Time",	PFCP_IE_END_TIME, 4, 0, NULL, NULL},
	{"Query URR",	PFCP_IE_QUERY_URR, 0, 0, (fp_decoder)__up_pfcp_decode__query_urr, NULL},
	{"Usage Report (Session Modification Response)",	PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE, 0, 0, (fp_decoder)__up_pfcp_decode__usage_report_session_modification_response, NULL},
	{"Usage Report (Session Deletion Response)",	PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE, 0, 0, (fp_decoder)__up_pfcp_decode__usage_report_session_deletion_response, NULL},
	{"Usage Report (Session Report Request)",	PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST, 0, 0, (fp_decoder)__up_pfcp_decode__usage_report_session_report_request, NULL},
	{"URR ID",	PFCP_IE_URR_ID, 4, 0, NULL, NULL},
	{"Linked URR ID",	PFCP_IE_LINKED_URR_ID, 4, 0, NULL, NULL},
	{"Downlink Data Report",	PFCP_IE_DOWNLINK_DATA_REPORT, 0, 0, (fp_decoder)__up_pfcp_decode__downlink_data_report, NULL},
	{"Outer Header Creation",	PFCP_IE_OUTER_HEADER_CREATION, 2, 0, NULL, NULL},
	{"Create BAR",	PFCP_IE_CREATE_BAR, 0, 0, (fp_decoder)__up_pfcp_decode__create_bar, NULL},
	{"Update BAR (Session Modification Request)",	PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST, 0, 0, (fp_decoder)__up_pfcp_decode__update_bar_session_modification_request, NULL},
	{"Remove BAR",	PFCP_IE_REMOVE_BAR, 0, 0, (fp_decoder)__up_pfcp_decode__remove_bar, NULL},
	{"BAR ID",	PFCP_IE_BAR_ID, 1, 0, NULL, NULL},
	{"CP Function Features",	PFCP_IE_CP_FUNCTION_FEATURES, 1, 0, NULL, NULL},
	{"Usage Information",	PFCP_IE_USAGE_INFORMATION, 1, 0, NULL, NULL},
	{"Application Instance ID",	PFCP_IE_APPLICATION_INSTANCE_ID, 0, 0, NULL, NULL},
	{"Flow Information",	PFCP_IE_FLOW_INFORMATION, 3, 0, NULL, NULL},
	{"UE IP Address",	PFCP_IE_UE_IP_ADDRESS, 1, 0, NULL, NULL},
	{"Packet Rate",	PFCP_IE_PACKET_RATE, 1, 0, NULL, NULL},
	{"Outer Header Removal",	PFCP_IE_OUTER_HEADER_REMOVAL, 1, 0, NULL, NULL},
	{"Recovery Time Stamp",	PFCP_IE_RECOVERY_TIME_STAMP, 4, 0, NULL, NULL},
	{"DL Flow Level Marking",	PFCP_IE_DL_FLOW_LEVEL_MARKING, 1, 0, NULL, NULL},
	{"Header Enrichment",	PFCP_IE_HEADER_ENRICHMENT, 1, 0, NULL, NULL},
	{"Error Indication Report",	PFCP_IE_ERROR_INDICATION_REPORT, 0, 0, (fp_decoder)__up_pfcp_decode__error_indication_report, NULL},
	{"Measurement Information",	PFCP_IE_MEASUREMENT_INFORMATION, 1, 0, NULL, NULL},
	{"Node Report Type",	PFCP_IE_NODE_REPORT_TYPE, 1, 0, NULL, NULL},
	{"User Plane Path Failure Report",	PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT, 0, 0, (fp_decoder)__up_pfcp_decode__user_plane_path_failure_report, NULL},
	{"Remote GTP-U Peer",	PFCP_IE_REMOTE_GTP_U_PEER, 1, 0, NULL, NULL},
	{"UR-SEQN",	PFCP_IE_UR_SEQN, 4, 0, NULL, NULL},
	{"Update Duplicating Parameters",	PFCP_IE_UPDATE_DUPLICATING_PARAMETERS, 0, 0, (fp_decoder)__up_pfcp_decode__update_duplicating_parameters, NULL},
	{"Activate Predefined Rules",	PFCP_IE_ACTIVATE_PREDEFINED_RULES, 0, 0, NULL, NULL},
	{"Deactivate Predefined Rules",	PFCP_IE_DEACTIVATE_PREDEFINED_RULES, 0, 0, NULL, NULL},
	{"FAR ID",	PFCP_IE_FAR_ID, 4, 0, NULL, NULL},
	{"QER ID",	PFCP_IE_QER_ID, 4, 0, NULL, NULL},
	{"OCI Flags",	PFCP_IE_OCI_FLAGS, 1, 0, NULL, NULL},
	{"PFCP Association Release Request",	PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST, 1, 0, NULL, NULL},
	{"Graceful Release Period",	PFCP_IE_GRACEFUL_RELEASE_PERIOD, 1, 0, NULL, NULL},
	{"PDN Type",	PFCP_IE_PDN_TYPE, 1, 0, NULL, NULL},
	{"Failed Rule ID",	PFCP_IE_FAILED_RULE_ID, 1, 0, NULL, NULL},
	{"Time Quota Mechanism",	PFCP_IE_TIME_QUOTA_MECHANISM, 1, 0, NULL, NULL},
	{"Resvered",	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, 0, 0, NULL, NULL},		//Reserved
	{"User Plane Inactivity Timer",	PFCP_IE_USER_PLANE_INACTIVITY_TIMER, 4, 0, NULL, NULL},
	{"Aggregated URRs",	PFCP_IE_AGGREGATED_URRS, 0, 0, NULL, NULL},
	{"Multiplier",	PFCP_IE_MULTIPLIER, 12, 0, NULL, NULL},
	{"Aggregated URR ID",	PFCP_IE_AGGREGATED_URR_ID, 4, 0, NULL, NULL},
	{"Subsequent Volume Quota",	PFCP_IE_SUBSEQUENT_VOLUME_QUOTA, 1, 0, NULL, NULL},
	{"Subsequent Time Quota",	PFCP_IE_SUBSEQUENT_TIME_QUOTA, 4, 0, NULL, NULL},
	{"RQI",	PFCP_IE_RQI, 1, 0, NULL, NULL},
	{"QFI",	PFCP_IE_QFI, 1, 0, NULL, NULL},
	{"Query URR Reference",	PFCP_IE_QUERY_URR_REFERENCE, 4, 0, NULL, NULL},
	{"Additional Usage Reports Information",	PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION, 2, 0, NULL, NULL},
	{"Create Traffic Endpoint",	PFCP_IE_CREATE_TRAFFIC_ENDPOINT, 0, 0, (fp_decoder)__up_pfcp_decode__create_traffic_endpoint, NULL},
	{"Created Traffic Endpoint",	PFCP_IE_CREATED_TRAFFIC_ENDPOINT, 0, 0, (fp_decoder)__up_pfcp_decode__created_traffic_endpoint, NULL},
	{"Update Traffic Endpoint",	PFCP_IE_UPDATE_TRAFFIC_ENDPOINT, 0, 0, NULL, NULL},
	{"Remove Traffic Endpoint",	PFCP_IE_REMOVE_TRAFFIC_ENDPOINT, 0, 0, (fp_decoder)__up_pfcp_decode__remove_traffic_endpoint, NULL},
	{"Traffic Endpoint ID",	PFCP_IE_TRAFFIC_ENDPOINT_ID, 1, 0, NULL, NULL},
	{"Ethernet Packet Filter",	PFCP_IE_ETHERNET_PACKET_FILTER, 0, 0, (fp_decoder)__up_pfcp_decode__ethernet_packet_filter, NULL},
	{"MAC address",	PFCP_IE_MAC_ADDRESS, 1, 0, NULL, NULL},
	{"C-TAG",	PFCP_IE_C_TAG,3, 0, NULL, NULL},
	{"S-TAG ",	PFCP_IE_S_TAG, 3, 0, NULL, NULL},
	{"Ethertype",	PFCP_IE_ETHERTYPE, 2, 0, NULL, NULL},
	{"Proxying",	PFCP_IE_PROXYING, 1, 0, NULL, NULL},
	{"Ethernet Filter ID",	PFCP_IE_ETHERNET_FILTER_ID, 4, 0, NULL, NULL},
	{"Ethernet Filter Properties",	PFCP_IE_ETHERNET_FILTER_PROPERTIES, 1, 0, NULL, NULL},
	{"Suggested Buffering Packets Count",	PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT, 1, 0, NULL, NULL},
	{"User ID",	PFCP_IE_USER_ID, 1, 0, NULL, NULL},
	{"Ethernet PDU Session Information",	PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION, 1, 0, NULL, NULL},
	{"Ethernet Traffic Information",	PFCP_IE_ETHERNET_TRAFFIC_INFORMATION, 0, 0, (fp_decoder)__up_pfcp_decode__ethernet_traffic_information, NULL},
	{"MAC Addresses Detected",	PFCP_IE_MAC_ADDRESSES_DETECTED, 7, 0, NULL, NULL},
	{"MAC Addresses Removed",	PFCP_IE_MAC_ADDRESSES_REMOVED, 7, 0, NULL, NULL},
	{"Ethernet Inactivity Timer",	PFCP_IE_ETHERNET_INACTIVITY_TIMER, 4, 0, NULL, NULL},
	{"Additional Monitoring Time",	PFCP_IE_ADDITIONAL_MONITORING_TIME, 0, 0, NULL, NULL},
	{"Event Quota",	PFCP_IE_EVENT_QUOTA, 4, 0, NULL, NULL},
	{"Event Threshold",	PFCP_IE_EVENT_THRESHOLD, 4, 0, NULL, NULL},
	{"Subsequent Event Quota",	PFCP_IE_SUBS_EVENT_QUOTA, 4, 0, NULL, NULL},
	{"Subsequent Event Threshold",	PFCP_IE_SUBS_EVENT_THRESHOLD, 4, 0, NULL, NULL},
	{"Trace Information",	PFCP_IE_TRACE_INFORMATION, 7, 0, NULL, NULL},
	{"Framed-Route",	PFCP_IE_FRAMED_ROUTE, 0, 0, NULL, NULL},
	{"Framed-Routing",	PFCP_IE_FRAMED_ROUTING, 4, 0, NULL, NULL},
	{"Framed-IPv6-Route",	PFCP_IE_FRAMED_IPV6_ROUTE, 0, 0, NULL, NULL},
	{"Time Stamp",	PFCP_IE_EVENT_TIME_STAMP_TYPE, 4, 0, NULL, NULL},
	{"Averaging Window",	PFCP_IE_AVERAGING_WINDOW_TYPE, 4, 0, NULL, NULL},
	{"Paging Policy Indicator",	PFCP_IE_PAGING_POLICY_INDICATOR_TYPE, 1, 0, NULL, NULL},
	{"APN/DNN",	PFCP_IE_APN_DNN_TYPE, 0, 0, NULL, NULL},
	{"3GPP Interface Type",	PFCP_IE_INTERFACE_TYPE_TYPE, 1, 0, NULL, NULL},
	{"PFCPSRReq-Flags",	PFCP_IE_PFCPSRREQ_FLAGS_TYPE, 1, 0, NULL, NULL},
	{"PFCPAUReq-Flags",	PFCP_IE_PFCPAUREQ_FLAGS_TYPE, 1, 0, NULL, NULL},
	{"Activation Time",	PFCP_IE_ACTIVATION_TIME_TYPE, 4, 0, NULL, NULL},
	{"Deactivation Time",	PFCP_IE_DEACTIVATION_TIME_TYPE, 4, 0, NULL, NULL},
	{"Create MAR",	PFCP_IE_CREATE_MAR_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__create_mar, NULL},
	{"3GPP Access Forwarding Action Information",	PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__access_forwarding_action_information_1, NULL},
	{"Non-3GPP Access Forwarding Action Information",	PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__access_forwarding_action_information_2, NULL},
	{"Remove MAR",	PFCP_IE_REMOVE_MAR_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__remove_mar, NULL},
	{"Update MAR",	PFCP_IE_UPDATE_MAR_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__update_mar, NULL},
	{"MAR ID",		PFCP_IE_MAR_ID_TYPE, 1, 0, NULL, NULL},
	{"Steering Functionality",	PFCP_IE_STEERING_FUNCTIONALITY_TYPE, 1, 0, NULL, NULL},
	{"Steering Mode",	PFCP_IE_STEERING_MODE_TYPE, 1, 0, NULL, NULL},
	{"Weight",		PFCP_IE_WEIGHT_TYPE, 1, 0, NULL, NULL},
	{"Priority",	PFCP_IE_PRIORITY_TYPE, 1, 0, NULL, NULL},
	{"Update 3GPP Access Forwarding Action Information",	PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__update_access_forwarding_action_information_1, NULL},
	{"Update Non 3GPP Access Forwarding Action Information",	PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE, 0, 0, (fp_decoder)__up_pfcp_decode__update_access_forwarding_action_information_2, NULL},
	{"UE IP address Pool Identity",	PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE, 2, 0, NULL, NULL},
	{"Alternative SMF IP Address",	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE, 1, 0, NULL, NULL},
	{"Packet Replication and Detection Carry-On Information",	PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE, 1, 0, NULL, NULL},
	{"SMF Set ID",	PFCP_IE_SMF_SET_ID_TYPE, 0, 0, NULL, NULL},
	{"Quota Validity Time",	PFCP_IE_QUOTA_VALIDITY_TIME_TYPE, 4, 0, NULL, NULL},
	{0}
};



int __up_pfcp__print_element_dict()
{
	pfcp_element_dict_t * element = NULL;
	int count = sizeof(pfcp_elements)/sizeof(pfcp_element_dict_t);
	
	printf("pfcp_element_dict count=%d\n", count);
	int i = 0;
	for( i = 0; i < count; i++)
	{
		printf("%03d | Name=%s ID=%d\n", i, pfcp_elements[i].name, pfcp_elements[i].id);
	}
	
	return 0;
}


void __up_pfcp_free__oct( pfcp_tlv_octet_t * oct)
{
	//printf( "presence=%lu oct->data=%p\n", oct->presence, oct->data);
	if( oct->presence == 1 && oct->data)
	{
		app_region__free( oct->data);
		oct->data = NULL;
	}
}


int __up_pfcp_free__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj)
{
    if( mObj->presence == 1)
	{
		// pfcp_tlv_ethernet_filter_id_t ethernet_filter_id;					//PFCP_IE_ETHERNET_FILTER_ID			pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_filter_properties_t ethernet_filter_properties;	//PFCP_IE_ETHERNET_FILTER_PROPERTIES	pfcp_tlv_octet_t
		// pfcp_tlv_mac_address_t mac_address;									//PFCP_IE_MAC_ADDRESS					pfcp_tlv_octet_t
		// pfcp_tlv_ethertype_t ethertype;										//PFCP_IE_ETHERTYPE						pfcp_tlv_octet_t
		// pfcp_tlv_c_tag_t c_tag;												//PFCP_IE_C_TAG							pfcp_tlv_octet_t
		// pfcp_tlv_s_tag_t s_tag;												//PFCP_IE_S_TAG							pfcp_tlv_octet_t
		// pfcp_tlv_sdf_filter_t sdf_filter[8];									//PFCP_IE_SDF_FILTER					pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->ethernet_filter_id);
		__up_pfcp_free__oct( &mObj->ethernet_filter_properties);
		__up_pfcp_free__oct( &mObj->mac_address);
		__up_pfcp_free__oct( &mObj->ethertype);
		__up_pfcp_free__oct( &mObj->c_tag);
		__up_pfcp_free__oct( &mObj->s_tag);
		
		int i = 0;
		for( i = 0; i < 8; i++)
		{
			__up_pfcp_free__oct( &mObj->sdf_filter[i]);
		}	
	}
	
	return 0;
}

int __up_pfcp_free__pdi(pfcp_tlv_pdi_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_source_interface_t source_interface;									PFCP_IE_SOURCE_INTERFACE	pfcp_tlv_uint8_t
		// pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;								PFCP_IE_TRAFFIC_ENDPOINT_ID	pfcp_tlv_octet_t
		// pfcp_tlv_sdf_filter_t sdf_filter[8];												PFCP_IE_SDF_FILTER			pfcp_tlv_octet_t
		// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID		pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;	PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_packet_filter_t ethernet_packet_filter;						PFCP_IE_ETHERNET_PACKET_FILTER
		// pfcp_tlv_qfi_t qfi;																PFCP_IE_QFI					pfcp_tlv_uint8_t
		// pfcp_tlv_framed_route_t framed_route;											PFCP_IE_FRAMED_ROUTE		pfcp_tlv_octet_t
		// pfcp_tlv_framed_routing_t framed_routing;										PFCP_IE_FRAMED_ROUTING		pfcp_tlv_octet_t
		// pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;									PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
		// pfcp_tlv__interface_type_t source_interface_type;								PFCP_IE_INTERFACE_TYPE_TYPE	pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->local_f_teid);
		__up_pfcp_free__oct( &mObj->network_instance);
		__up_pfcp_free__oct( &mObj->ue_ip_address);
		__up_pfcp_free__oct( &mObj->traffic_endpoint_id);
		
		int i = 0;
		for( i = 0; i < 8; i++)
		{
			__up_pfcp_free__oct( &mObj->sdf_filter[i]);
		}
		
		__up_pfcp_free__oct( &mObj->application_id);
		__up_pfcp_free__oct( &mObj->ethernet_pdu_session_information);
		__up_pfcp_free__ethernet_packet_filter( &mObj->ethernet_packet_filter);
		__up_pfcp_free__oct( &mObj->framed_route);
		__up_pfcp_free__oct( &mObj->framed_routing);
		__up_pfcp_free__oct( &mObj->framed_ipv6_route);
		__up_pfcp_free__oct( &mObj->source_interface_type);
	}
	return 0;
}


int __up_pfcp_free__create_pdr(pfcp_tlv_create_pdr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t		
		// pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE					pfcp_tlv_uint32_t		
		// pfcp_tlv_pdi_t pdi;																	
		// pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID						pfcp_tlv_uint32_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID						pfcp_tlv_uint32_t
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID						pfcp_tlv_uint32_t
		// pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		// pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_mar_id_t mar_id;														PFCP_IE_MAR_ID_TYPE					pfcp_tlv_octet_t
		// pfcp_tlv_packet_replication_and_detection_carry_on_information_t packet_replication_and_detection_carry_on_information;	
															//PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE	pfcp_tlv_octet_t
	
		__up_pfcp_free__pdi( &mObj->pdi);
		__up_pfcp_free__oct( &mObj->outer_header_removal);
		__up_pfcp_free__oct( &mObj->activate_predefined_rules);
		__up_pfcp_free__oct( &mObj->activation_time);
		__up_pfcp_free__oct( &mObj->deactivation_time);
		__up_pfcp_free__oct( &mObj->mar_id);
		__up_pfcp_free__oct( &mObj->packet_replication_and_detection_carry_on_information);
		
	}
	return 0;
}

int __up_pfcp_free__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
		// pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		// pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
		// pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		// pfcp_tlv_proxying_t proxying;													PFCP_IE_PROXYING				pfcp_tlv_octet_t
		// pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t	
		
		__up_pfcp_free__oct( &mObj->network_instance);
		__up_pfcp_free__oct( &mObj->redirect_information);
		__up_pfcp_free__oct( &mObj->outer_header_creation);
		__up_pfcp_free__oct( &mObj->transport_level_marking);
		__up_pfcp_free__oct( &mObj->forwarding_policy);
		__up_pfcp_free__oct( &mObj->header_enrichment);
		__up_pfcp_free__oct( &mObj->linked_traffic_endpoint_id);
		__up_pfcp_free__oct( &mObj->proxying);
		__up_pfcp_free__oct( &mObj->destination_interface_type);
	}
	return 0;
}

int __up_pfcp_free__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->outer_header_creation);
		__up_pfcp_free__oct( &mObj->transport_level_marking);
		__up_pfcp_free__oct( &mObj->forwarding_policy);
	}
	return 0;
}

int __up_pfcp_free__create_far(pfcp_tlv_create_far_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
		// pfcp_tlv_forwarding_parameters_t forwarding_parameters;							PFCP_IE_FORWARDING_PARAMETERS
		// pfcp_tlv_duplicating_parameters_t duplicating_parameters;						PFCP_IE_DUPLICATING_PARAMETERS
		// pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t
		
		__up_pfcp_free__forwarding_parameters( &mObj->forwarding_parameters);
		__up_pfcp_free__duplicating_parameters( &mObj->duplicating_parameters);
	}
	
	return 0;
}

int __up_pfcp_free__update_forwarding_parameters(pfcp_tlv_update_forwarding_parameters_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
		// pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		// pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
		// pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;										PFCP_IE_PFCPSMREQ_FLAGS			pfcp_tlv_uint8_t
		// pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		// pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->network_instance);
		__up_pfcp_free__oct( &mObj->redirect_information);
		__up_pfcp_free__oct( &mObj->outer_header_creation);
		__up_pfcp_free__oct( &mObj->transport_level_marking);
		__up_pfcp_free__oct( &mObj->forwarding_policy);
		__up_pfcp_free__oct( &mObj->header_enrichment);
		__up_pfcp_free__oct( &mObj->linked_traffic_endpoint_id);
		__up_pfcp_free__oct( &mObj->destination_interface_type);
	}
	return 0;
}

int __up_pfcp_free__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->outer_header_creation);
		__up_pfcp_free__oct( &mObj->transport_level_marking);
		__up_pfcp_free__oct( &mObj->forwarding_policy);
	}
	return 0;
}

int __up_pfcp_free__update_far(pfcp_tlv_update_far_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
		// pfcp_tlv_update_forwarding_parameters_t update_forwarding_parameters;			PFCP_IE_UPDATE_FORWARDING_PARAMETERS
		// pfcp_tlv_update_duplicating_parameters_t update_duplicating_parameters;			PFCP_IE_UPDATE_DUPLICATING_PARAMETERS
		// pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t
	
		__up_pfcp_free__update_forwarding_parameters( &mObj->update_forwarding_parameters);
		__up_pfcp_free__update_duplicating_parameters( &mObj->update_duplicating_parameters);	
	}
	
	return 0;
}

int __up_pfcp_free__pfd_context(pfcp_tlv_pfd_context_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pfd_contents_t pfd_contents;											PFCP_IE_PFD_CONTENTS		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->pfd_contents);
	}
	return 0;
}

int __up_pfcp_free__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_IDS_PFDS	pfcp_tlv_octet_t
		// pfcp_tlv_pfd_context_t pfd_context;												PFCP_IE_PFD_CONTEXT
	
		__up_pfcp_free__oct( &mObj->application_id);
		__up_pfcp_free__pfd_context( &mObj->pfd_context);
	}
	return 0;
}

int __up_pfcp_free__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mac_addresses_detected_t mac_addresses_detected;						PFCP_IE_MAC_ADDRESSES_DETECTED	pfcp_tlv_octet_t	
		// pfcp_tlv_mac_addresses_removed_t mac_addresses_removed;							PFCP_IE_MAC_ADDRESSES_REMOVED	pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->mac_addresses_detected);
		__up_pfcp_free__oct( &mObj->mac_addresses_removed);
	}
	return 0;
}

int __up_pfcp_free__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		
		__up_pfcp_free__oct( &mObj->weight);
		__up_pfcp_free__oct( &mObj->priority);
	}
	return 0;
}

int __up_pfcp_free__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		
		__up_pfcp_free__oct( &mObj->weight);
		__up_pfcp_free__oct( &mObj->priority);		
	}
	return 0;
}

int __up_pfcp_free__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		
		__up_pfcp_free__oct( &mObj->weight);
		__up_pfcp_free__oct( &mObj->priority);		
	}
	return 0;
}

int __up_pfcp_free__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		
		__up_pfcp_free__oct( &mObj->weight);
		__up_pfcp_free__oct( &mObj->priority);		
	}
	return 0;
}

int __up_pfcp_free__create_urr(pfcp_tlv_create_urr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
		// pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t
		// pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
		// pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
		// pfcp_tlv_event_threshold_t event_threshold;										PFCP_IE_EVENT_THRESHOLD		pfcp_tlv_octet_t
		// pfcp_tlv_event_quota_t event_quota;												PFCP_IE_EVENT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
		// pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
		// pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
		// pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_VOLUME_QUOTA	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_EVENT_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_EVENT_ID	pfcp_tlv_octet_t
		// pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
		// pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
		// pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
		// pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
		// pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->measurement_period);
		__up_pfcp_free__oct( &mObj->volume_threshold);
		__up_pfcp_free__oct( &mObj->volume_quota);
		__up_pfcp_free__oct( &mObj->event_threshold);
		__up_pfcp_free__oct( &mObj->event_quota);
		__up_pfcp_free__oct( &mObj->time_threshold);
		__up_pfcp_free__oct( &mObj->time_quota);
		__up_pfcp_free__oct( &mObj->quota_holding_time);
		__up_pfcp_free__oct( &mObj->dropped_dl_traffic_threshold);
		__up_pfcp_free__oct( &mObj->quota_validity_time);
		__up_pfcp_free__oct( &mObj->monitoring_time);
		__up_pfcp_free__oct( &mObj->subsequent_volume_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_time_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_volume_quota);
		__up_pfcp_free__oct( &mObj->subsequent_time_quota);
		__up_pfcp_free__oct( &mObj->subsequent_event_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_event_quota);
		__up_pfcp_free__oct( &mObj->inactivity_detection_time);
		__up_pfcp_free__oct( &mObj->linked_urr_id);
		__up_pfcp_free__oct( &mObj->measurement_information);
		__up_pfcp_free__oct( &mObj->time_quota_mechanism);
		__up_pfcp_free__oct( &mObj->aggregated_urrs);
		__up_pfcp_free__oct( &mObj->ethernet_inactivity_timer);
		__up_pfcp_free__oct( &mObj->additional_monitoring_time);
	}
	return 0;
}

int __up_pfcp_free__create_qer(pfcp_tlv_create_qer_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
		// pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS	pfcp_tlv_uint8_t
		// pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR	pfcp_tlv_octet_t
		// pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR	pfcp_tlv_octet_t
		// pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
		// pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI	pfcp_tlv_uint8_t
		// pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI	pfcp_tlv_uint8_t
		// pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t	
		// pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE	pfcp_tlv_uint32_t
		
		__up_pfcp_free__oct( &mObj->maximum_bitrate);
		__up_pfcp_free__oct( &mObj->guaranteed_bitrate);
		__up_pfcp_free__oct( &mObj->packet_rate);
		__up_pfcp_free__oct( &mObj->dl_flow_level_marking);
	}
	
	return 0;
}

int __up_pfcp_free__created_pdr(pfcp_tlv_created_pdr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
		// pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->local_f_teid);
		__up_pfcp_free__oct( &mObj->ue_ip_address);
	}
	return 0;
}

int __up_pfcp_free__update_pdr(pfcp_tlv_update_pdr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t
		// pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
		// pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE		pfcp_tlv_uint32_t
		// pfcp_tlv_pdi_t pdi	;															PFCP_IE_PDI
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		// pfcp_tlv_deactivate_predefined_rules_t deactivate_predefined_rules;				PFCP_IE_DEACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		// pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE	pfcp_tlv_octet_t	
		// pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE	pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->outer_header_removal);
		__up_pfcp_free__oct( &mObj->activate_predefined_rules);
		__up_pfcp_free__oct( &mObj->deactivate_predefined_rules);
		__up_pfcp_free__oct( &mObj->activation_time);
		__up_pfcp_free__oct( &mObj->deactivation_time);
	}
	return 0;
}

int __up_pfcp_free__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;																	PFCP_IE_BAR_ID		pfcp_tlv_uint8_t
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;				PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		// pfcp_tlv_dl_buffering_duration_t dl_buffering_duration;										PFCP_IE_DL_BUFFERING_DURATION	pfcp_tlv_octet_t
		// pfcp_tlv_dl_buffering_suggested_packet_count_t dl_buffering_suggested_packet_count;			PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT	pfcp_tlv_octet_t
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;				PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
	
		__up_pfcp_free__oct( &mObj->downlink_data_notification_delay);
		__up_pfcp_free__oct( &mObj->dl_buffering_duration);
		__up_pfcp_free__oct( &mObj->dl_buffering_suggested_packet_count);
		__up_pfcp_free__oct( &mObj->suggested_buffering_packets_count);
	}
	return 0;
}

int __up_pfcp_free__update_urr(pfcp_tlv_update_urr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		// pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
		// pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t	
		// pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
		// pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
		// pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
		// pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
		// pfcp_tlv_event_threshold_t event_threshold;										PFCP_IE_EVENT_THRESHOLD		pfcp_tlv_octet_t
		// pfcp_tlv_event_quota_t event_quota;												PFCP_IE_EVENT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t	
		// pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
		// pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t	
		// pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_EVENT_THRESHOLD	pfcp_tlv_octet_t
		// pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_EVENT_ID	pfcp_tlv_octet_t
		// pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
		// pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
		// pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
		// pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
		// pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->measurement_period);
		__up_pfcp_free__oct( &mObj->volume_threshold);
		__up_pfcp_free__oct( &mObj->volume_quota);
		__up_pfcp_free__oct( &mObj->time_threshold);
		__up_pfcp_free__oct( &mObj->time_quota);
		__up_pfcp_free__oct( &mObj->event_threshold);
		__up_pfcp_free__oct( &mObj->event_quota);
		__up_pfcp_free__oct( &mObj->quota_holding_time);
		__up_pfcp_free__oct( &mObj->dropped_dl_traffic_threshold);
		__up_pfcp_free__oct( &mObj->quota_validity_time);
		__up_pfcp_free__oct( &mObj->monitoring_time);
		__up_pfcp_free__oct( &mObj->subsequent_volume_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_time_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_volume_quota);
		__up_pfcp_free__oct( &mObj->subsequent_time_quota);
		__up_pfcp_free__oct( &mObj->subsequent_event_threshold);
		__up_pfcp_free__oct( &mObj->subsequent_event_quota);
		__up_pfcp_free__oct( &mObj->inactivity_detection_time);
		__up_pfcp_free__oct( &mObj->linked_urr_id);
		__up_pfcp_free__oct( &mObj->measurement_information);
		__up_pfcp_free__oct( &mObj->time_quota_mechanism);
		__up_pfcp_free__oct( &mObj->aggregated_urrs);
		__up_pfcp_free__oct( &mObj->ethernet_inactivity_timer);
		__up_pfcp_free__oct( &mObj->additional_monitoring_time);
	}
	return 0;
}

int __up_pfcp_free__update_qer(pfcp_tlv_update_qer_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
		// pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS		pfcp_tlv_uint8_t
		// pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR		pfcp_tlv_octet_t
		// pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR		pfcp_tlv_octet_t
		// pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
		// pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
		// pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI		pfcp_tlv_uint8_t
		// pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI		pfcp_tlv_uint8_t
		// pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t
		// pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE		pfcp_tlv_uint32_t

		__up_pfcp_free__oct( &mObj->maximum_bitrate);
		__up_pfcp_free__oct( &mObj->guaranteed_bitrate);
		__up_pfcp_free__oct( &mObj->packet_rate);
		__up_pfcp_free__oct( &mObj->dl_flow_level_marking);		
	}
	return 0;
}

int __up_pfcp_free__remove_pdr(pfcp_tlv_remove_pdr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t
	}
	return 0;
}

int __up_pfcp_free__remove_far(pfcp_tlv_remove_far_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID		pfcp_tlv_uint32_t
	}
	return 0;
}

int __up_pfcp_free__remove_urr(pfcp_tlv_remove_urr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
	}
	return 0;
}

int __up_pfcp_free__remove_qer(pfcp_tlv_remove_qer_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t
	}
	return 0;
}

int __up_pfcp_free__load_control_information(pfcp_tlv_load_control_information_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_sequence_number_t load_control_sequence_number;							PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t
		// pfcp_tlv_metric_t load_metric;													PFCP_IE_METRIC				pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->load_control_sequence_number);		
		__up_pfcp_free__oct( &mObj->load_metric);		
	}
	return 0;
}

int __up_pfcp_free__overload_control_information(pfcp_tlv_overload_control_information_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_sequence_number_t overload_control_sequence_number;						PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t	
		// pfcp_tlv_metric_t overload_reduction_metric;										PFCP_IE_METRIC				pfcp_tlv_octet_t
		// pfcp_tlv_timer_t period_of_validity;												PFCP_IE_TIMER				pfcp_tlv_octet_t
		// pfcp_tlv_oci_flags_t overload_control_information_flags;							PFCP_IE_OCI_FLAGS			pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->overload_control_sequence_number);
		__up_pfcp_free__oct( &mObj->overload_reduction_metric);
		__up_pfcp_free__oct( &mObj->period_of_validity);
		__up_pfcp_free__oct( &mObj->overload_control_information_flags);
	}
	return 0;
}

int __up_pfcp_free__application_detection_information(pfcp_tlv_application_detection_information_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID	pfcp_tlv_octet_t
		// pfcp_tlv_application_instance_id_t application_instance_id;						PFCP_IE_APPLICATION_INSTANCE_ID	pfcp_tlv_octet_t
		// pfcp_tlv_flow_information_t flow_information;									PFCP_IE_FLOW_INFORMATION	pfcp_tlv_octet_t	
		
		__up_pfcp_free__oct( &mObj->application_id);
		__up_pfcp_free__oct( &mObj->application_instance_id);
		__up_pfcp_free__oct( &mObj->flow_information);
	}
	return 0;
}

int __up_pfcp_free__query_urr(pfcp_tlv_query_urr_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
	}
	return 0;
}

int __up_pfcp_free__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t	
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
		
		__up_pfcp_free__oct( &mObj->ur_seqn);
		__up_pfcp_free__oct( &mObj->usage_report_trigger);
		__up_pfcp_free__oct( &mObj->start_time);
		__up_pfcp_free__oct( &mObj->end_time);
		__up_pfcp_free__oct( &mObj->volume_measurement);
		__up_pfcp_free__oct( &mObj->duration_measurement);
		__up_pfcp_free__oct( &mObj->time_of_first_packet);
		__up_pfcp_free__oct( &mObj->time_of_last_packet);
		__up_pfcp_free__oct( &mObj->usage_information);
		__up_pfcp_free__oct( &mObj->query_urr_reference);
	}
	return 0;
}

int __up_pfcp_free__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN	pfcp_tlv_octet_t
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT pfcp_tlv_octet_t
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t	
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
		
		__up_pfcp_free__oct( &mObj->ur_seqn);
		__up_pfcp_free__oct( &mObj->usage_report_trigger);
		__up_pfcp_free__oct( &mObj->start_time);
		__up_pfcp_free__oct( &mObj->end_time);
		__up_pfcp_free__oct( &mObj->volume_measurement);
		__up_pfcp_free__oct( &mObj->duration_measurement);
		__up_pfcp_free__oct( &mObj->time_of_first_packet);
		__up_pfcp_free__oct( &mObj->time_of_last_packet);
		__up_pfcp_free__oct( &mObj->usage_information);
		__up_pfcp_free__ethernet_traffic_information( &mObj->ethernet_traffic_information);
	}
	return 0;
}

int __up_pfcp_free__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t	
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME		pfcp_tlv_octet_t
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME		pfcp_tlv_octet_t
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
		// pfcp_tlv_application_detection_information_t application_detection_information;	PFCP_IE_APPLICATION_DETECTION_INFORMATION
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
		// pfcp_tlv_event_time_stamp_t event_time_stamp;									PFCP_IE_EVENT_TIME_STAMP_TYPE	pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
	
		__up_pfcp_free__oct( &mObj->ur_seqn);
		__up_pfcp_free__oct( &mObj->usage_report_trigger);
		__up_pfcp_free__oct( &mObj->start_time);
		__up_pfcp_free__oct( &mObj->end_time);
		__up_pfcp_free__oct( &mObj->volume_measurement);
		__up_pfcp_free__oct( &mObj->duration_measurement);
		__up_pfcp_free__application_detection_information( &mObj->application_detection_information);
		__up_pfcp_free__oct( &mObj->ue_ip_address);
		__up_pfcp_free__oct( &mObj->network_instance);
		__up_pfcp_free__oct( &mObj->time_of_first_packet);
		__up_pfcp_free__oct( &mObj->time_of_last_packet);
		__up_pfcp_free__oct( &mObj->usage_information);
		__up_pfcp_free__oct( &mObj->query_urr_reference);
		__up_pfcp_free__oct( &mObj->event_time_stamp);
		__up_pfcp_free__ethernet_traffic_information( &mObj->ethernet_traffic_information);
	}
	return 0;
}

int __up_pfcp_free__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pdr_id_t pdr_id;															PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
		// pfcp_tlv_downlink_data_service_information_t downlink_data_service_information;		PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_free__oct( &mObj->downlink_data_service_information);
	}
	return 0;
}

int __up_pfcp_free__create_bar(pfcp_tlv_create_bar_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
		__up_pfcp_free__oct( &mObj->downlink_data_notification_delay);
		__up_pfcp_free__oct( &mObj->suggested_buffering_packets_count);
	}
	return 0;
}

int __up_pfcp_free__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
		__up_pfcp_free__oct( &mObj->downlink_data_notification_delay);
		__up_pfcp_free__oct( &mObj->suggested_buffering_packets_count);
	}
	return 0;
}

int __up_pfcp_free__remove_bar(pfcp_tlv_remove_bar_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
	}
	return 0;
}

int __up_pfcp_free__error_indication_report(pfcp_tlv_error_indication_report_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_f_teid_t remote_f_teid;														PFCP_IE_F_TEID	pfcp_tlv_octet_t
		__up_pfcp_free__oct( &mObj->remote_f_teid);
	}
	return 0;
}

int __up_pfcp_free__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_remote_gtp_u_peer_t remote_gtp_u_peer_;										PFCP_IE_REMOTE_GTP_U_PEER	pfcp_tlv_octet_t	
		__up_pfcp_free__oct( &mObj->remote_gtp_u_peer_);
	}
	return 0;
}

int __up_pfcp_free__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		// pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID		pfcp_tlv_octet_t
		// pfcp_tlv_network_instance_t network_instance;										PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		// pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
		// pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;		PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION	pfcp_tlv_octet_t
		// pfcp_tlv_framed_route_t framed_route;												PFCP_IE_FRAMED_ROUTE	pfcp_tlv_octet_t
		// pfcp_tlv_framed_routing_t framed_routing;											PFCP_IE_FRAMED_ROUTING	pfcp_tlv_octet_t
		// pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;										PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
		// pfcp_tlv_qfi_t qfi;																	PFCP_IE_QFI			pfcp_tlv_uint8_t
	
		__up_pfcp_free__oct( &mObj->traffic_endpoint_id);
		__up_pfcp_free__oct( &mObj->local_f_teid);
		__up_pfcp_free__oct( &mObj->network_instance);
		__up_pfcp_free__oct( &mObj->ue_ip_address);
		__up_pfcp_free__oct( &mObj->ethernet_pdu_session_information);
		__up_pfcp_free__oct( &mObj->framed_route);
		__up_pfcp_free__oct( &mObj->framed_routing);
		__up_pfcp_free__oct( &mObj->framed_ipv6_route);
	}
	return 0;
}

int __up_pfcp_free__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		// pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID					pfcp_tlv_octet_t
		// pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS			pfcp_tlv_octet_t
	
		__up_pfcp_free__oct( &mObj->traffic_endpoint_id);
		__up_pfcp_free__oct( &mObj->local_f_teid);
		__up_pfcp_free__oct( &mObj->ue_ip_address);
	}
	return 0;
}

int __up_pfcp_free__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		
		__up_pfcp_free__oct( &mObj->traffic_endpoint_id);
	}
	return 0;
}

int __up_pfcp_free__create_mar(pfcp_tlv_create_mar_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE pfcp_tlv_octet_t
		// pfcp_tlv_steering_functionality_t steering_functionality;												PFCP_IE_STEERING_FUNCTIONALITY_TYPE pfcp_tlv_octet_t
		// pfcp_tlv_steering_mode_t steering_mode;																	PFCP_IE_STEERING_MODE_TYPE 	pfcp_tlv_octet_t
		// pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
		// pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
	
		__up_pfcp_free__oct( &mObj->mar_id);
		__up_pfcp_free__oct( &mObj->steering_functionality);
		__up_pfcp_free__oct( &mObj->steering_mode);
		__up_pfcp_free__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1);
		__up_pfcp_free__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2);
	}
	return 0;
}

int __up_pfcp_free__remove_mar(pfcp_tlv_remove_mar_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE		pfcp_tlv_octet_t
		__up_pfcp_free__oct( &mObj->mar_id);
	}
	return 0;
}

int __up_pfcp_free__update_mar(pfcp_tlv_update_mar_t * mObj)
{
    if( mObj->presence == 1)
	{	
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mar_id_t mar_id;																		PFCP_IE_MAR_ID_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_steering_functionality_t steering_functionality;										PFCP_IE_STEERING_FUNCTIONALITY_TYPE			pfcp_tlv_octet_t	
		// pfcp_tlv_steering_mode_t steering_mode;															PFCP_IE_STEERING_MODE_TYPE			pfcp_tlv_octet_t
		// pfcp_tlv_update_access_forwarding_action_information_1_t update_access_forwarding_action_information_1;
		// pfcp_tlv_update_access_forwarding_action_information_2_t update_access_forwarding_action_information_2;
		// pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
		// pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
		
		__up_pfcp_free__oct( &mObj->mar_id);
		__up_pfcp_free__oct( &mObj->steering_functionality);
		__up_pfcp_free__oct( &mObj->steering_mode);
		__up_pfcp_free__update_access_forwarding_action_information_1( &mObj->update_access_forwarding_action_information_1);
		__up_pfcp_free__update_access_forwarding_action_information_2( &mObj->update_access_forwarding_action_information_2);
		__up_pfcp_free__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1);
		__up_pfcp_free__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2);		
	}
	
	return 0;
}








///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int __up_pfcp_free__heartbeat_request( pfcp_message_t * mObj)
{
	return 0;
}

int __up_pfcp_free__heartbeat_response( pfcp_message_t * mObj)
{
	return 0;
}

int __up_pfcp_free__pfd_management_request( pfcp_message_t * mObj)
{
	__up_pfcp_free__application_id_s_pfds( &mObj->u.pfd_management_request.application_id_s_pfds);
	return 0;
}

int __up_pfcp_free__pfd_management_response( pfcp_message_t * mObj)
{
	return 0;
}

int __up_pfcp_free__association_setup_request( pfcp_message_t * mObj)
{
	//   pfcp_tlv_node_id_t node_id;
	__up_pfcp_free__oct( &mObj->u.association_setup_request.node_id);
	
	//pfcp_tlv_up_function_features_t up_function_features;
	__up_pfcp_free__oct( &mObj->u.association_setup_request.up_function_features);
	
    // pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4]; pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_free__oct( &mObj->u.association_setup_request.user_plane_ip_resource_information[0]);
	__up_pfcp_free__oct( &mObj->u.association_setup_request.user_plane_ip_resource_information[1]);
	__up_pfcp_free__oct( &mObj->u.association_setup_request.user_plane_ip_resource_information[2]);
	__up_pfcp_free__oct( &mObj->u.association_setup_request.user_plane_ip_resource_information[3]);
	
	// pfcp_tlv_ue_ip_address_t ue_ip_address_pool_identity;						pfcp_tlv_octet_t	PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE
    __up_pfcp_free__oct( &mObj->u.association_setup_request.ue_ip_address_pool_identity);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;			pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
    __up_pfcp_free__oct( &mObj->u.association_setup_request.alternative_smf_ip_address);
	
	// pfcp_tlv_smf_set_id_t smf_set_id;											pfcp_tlv_octet_t	PFCP_IE_SMF_SET_ID_TYPE
	__up_pfcp_free__oct( &mObj->u.association_setup_request.smf_set_id);


	return 0;
}

int __up_pfcp_free__association_setup_response( pfcp_message_t * mObj)
{
	//pfcp_tlv_node_id_t node_id;
	__up_pfcp_free__oct( &mObj->u.association_setup_response.node_id);
	
	//pfcp_tlv_up_function_features_t up_function_features;
	__up_pfcp_free__oct(  &mObj->u.association_setup_response.up_function_features);
	
	// pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];		pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_free__oct(  &mObj->u.association_setup_response.user_plane_ip_resource_information[0]);
	__up_pfcp_free__oct(  &mObj->u.association_setup_response.user_plane_ip_resource_information[1]);
	__up_pfcp_free__oct(  &mObj->u.association_setup_response.user_plane_ip_resource_information[2]);
	__up_pfcp_free__oct(  &mObj->u.association_setup_response.user_plane_ip_resource_information[3]);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;				pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
	__up_pfcp_free__oct(  &mObj->u.association_setup_response.alternative_smf_ip_address);
	
	return 0;
}

int __up_pfcp_free__association_update_request( pfcp_message_t * mObj)
{
	pfcp_association_update_request_t  *  rObj = &mObj->u.association_update_request;
	
    // pfcp_tlv_node_id_t node_id;									pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
    // pfcp_tlv_up_function_features_t up_function_features;		pfcp_tlv_octet_t	PFCP_IE_UP_FUNCTION_FEATURES
    __up_pfcp_free__oct( &rObj->up_function_features);
	
	// pfcp_tlv_pfcp_association_release_request_t pfcp_association_release_request;		pfcp_tlv_octet_t	PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST
    __up_pfcp_free__oct( &rObj->pfcp_association_release_request);
	
	// pfcp_tlv_graceful_release_period_t graceful_release_period;			pfcp_tlv_octet_t	PFCP_IE_GRACEFUL_RELEASE_PERIOD
    __up_pfcp_free__oct( &rObj->graceful_release_period);
	
	// pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];		pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_free__oct( &rObj->user_plane_ip_resource_information[0]);
	__up_pfcp_free__oct( &rObj->user_plane_ip_resource_information[1]);
	__up_pfcp_free__oct( &rObj->user_plane_ip_resource_information[2]);
	__up_pfcp_free__oct( &rObj->user_plane_ip_resource_information[3]);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;		pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
	__up_pfcp_free__oct( &rObj->alternative_smf_ip_address);
	
	return 0;
}

int __up_pfcp_free__association_update_response( pfcp_message_t * mObj)
{
	pfcp_association_update_response_t  *  rObj = &mObj->u.association_update_response;
	
    // pfcp_tlv_node_id_t node_id;									pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
    // pfcp_tlv_up_function_features_t up_function_features;		pfcp_tlv_octet_t	PFCP_IE_UP_FUNCTION_FEATURES
    __up_pfcp_free__oct( &rObj->up_function_features);
	
	return 0;
}

int __up_pfcp_free__association_release_request( pfcp_message_t * mObj)
{
	pfcp_association_release_request_t  *  rObj = &mObj->u.association_release_request;
	
	//pfcp_tlv_node_id_t node_id;					pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
	return 0;
}

int __up_pfcp_free__association_release_response( pfcp_message_t * mObj)
{
	pfcp_association_release_response_t  *  rObj = &mObj->u.association_release_response;
	
    // pfcp_tlv_node_id_t node_id;						pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
	return 0;
}

int __up_pfcp_free__version_not_supported_response( pfcp_message_t * mObj)
{
	return 0;
}

int __up_pfcp_free__node_report_request( pfcp_message_t * mObj)
{
	pfcp_node_report_request_t  *  rObj = &mObj->u.node_report_request;
	
    // pfcp_tlv_node_id_t node_id;							pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
    // pfcp_tlv_node_report_type_t node_report_type;		pfcp_tlv_octet_t	PFCP_IE_NODE_REPORT_TYPE	
    __up_pfcp_free__oct( &rObj->node_report_type);
	
	// pfcp_tlv_user_plane_path_failure_report_t user_plane_path_failure_report;		PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT
	__up_pfcp_free__user_plane_path_failure_report( &rObj->user_plane_path_failure_report);

	return 0;
}

int __up_pfcp_free__node_report_response( pfcp_message_t * mObj)
{
	pfcp_node_report_response_t  *  rObj = &mObj->u.node_report_response;
	
    // pfcp_tlv_node_id_t node_id;						pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
	return 0;
}

int __up_pfcp_free__session_set_deletion_request( pfcp_message_t * mObj)
{
	pfcp_session_set_deletion_request_t  *  rObj = &mObj->u.session_set_deletion_request;

    // pfcp_tlv_node_id_t node_id;				pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
	
    // pfcp_tlv_fq_csid_t sgw_c_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->sgw_c_fq_csid);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->pgw_c_fq_csid);
	
	// pfcp_tlv_fq_csid_t sgw_u_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->sgw_u_fq_csid);
	
	// pfcp_tlv_fq_csid_t pgw_u_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->pgw_u_fq_csid);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->twan_fq_csid);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->epdg_fq_csid);
	
	// pfcp_tlv_fq_csid_t mme_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
	__up_pfcp_free__oct( &rObj->mme_fq_csid);
	return 0;
}

int __up_pfcp_free__session_set_deletion_response( pfcp_message_t * mObj)
{
	pfcp_session_set_deletion_response_t  *  rObj = &mObj->u.session_set_deletion_response;

    // pfcp_tlv_node_id_t node_id;					pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);
    
	return 0;
}


int __up_pfcp_free__session_establishment_request( pfcp_message_t * mObj)
{
	pfcp_session_establishment_request_t  *  rObj = &mObj->u.session_establishment_request;
	
	__up_pfcp_free__oct( &rObj->node_id);
	__up_pfcp_free__oct( &rObj->cp_f_seid);
	
	int i = 0;
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_free__create_pdr( &rObj->create_pdr[i]);
	}
	
    // pfcp_tlv_create_far_t create_far[8];
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_free__create_far( &rObj->create_far[i]);
	}
	
    // pfcp_tlv_create_urr_t create_urr[2];
	for( i = 0; i < 10; i++)
	{
		__up_pfcp_free__create_urr( &rObj->create_urr[i]);
	}
    
	// pfcp_tlv_create_qer_t create_qer[4];
	for( i = 0; i < 4; i++)
	{
		__up_pfcp_free__create_qer( &rObj->create_qer[i]);
	}
	
    // pfcp_tlv_create_bar_t create_bar;
	__up_pfcp_free__create_bar( &rObj->create_bar);
	
    // pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;			PFCP_IE_CREATE_TRAFFIC_ENDPOINT
    __up_pfcp_free__create_traffic_endpoint( &rObj->create_traffic_endpoint);
	
    // pfcp_tlv_fq_csid_t sgw_c_fq_csid;			//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
	__up_pfcp_free__oct( &rObj->sgw_c_fq_csid);
	
    // pfcp_tlv_fq_csid_t mme_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->mme_fq_csid);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;			//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->pgw_c_fq_csid);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->epdg_fq_csid);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->twan_fq_csid);
	
	// pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;	pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_INACTIVITY_TIMER
    __up_pfcp_free__oct( &rObj->user_plane_inactivity_timer);
	
	// pfcp_tlv_user_id_t user_id;											pfcp_tlv_octet_t	PFCP_IE_USER_ID
    __up_pfcp_free__oct( &rObj->user_id);
	
	// pfcp_tlv_trace_information_t trace_information;						pfcp_tlv_octet_t	PFCP_IE_TRACE_INFORMATION
    __up_pfcp_free__oct( &rObj->trace_information);
	
	// pfcp_tlv_apn_dnn_t apn_dnn;											pfcp_tlv_octet_t	PFCP_IE_APN_DNN_TYPE
    __up_pfcp_free__oct( &rObj->apn_dnn);
	
	// pfcp_tlv_create_mar_t create_mar;									PFCP_IE_CREATE_MAR_TYPE
	__up_pfcp_free__create_mar( &rObj->create_mar);
	
	return 0;
}


int __up_pfcp_free__session_establishment_response( pfcp_message_t * mObj)
{
	pfcp_session_establishment_response_t  *  rObj = &mObj->u.session_establishment_response;

    // pfcp_tlv_node_id_t node_id;
	__up_pfcp_free__oct( &rObj->node_id);

	// pfcp_tlv_f_seid_t up_f_seid;
	__up_pfcp_free__oct( &rObj->up_f_seid);
	
    // pfcp_tlv_created_pdr_t created_pdr[8];
	int i = 0;
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_free__created_pdr( &rObj->created_pdr[i]);
	}
	
    // pfcp_tlv_load_control_information_t load_control_information;					PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_free__load_control_information( &rObj->load_control_information);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;			PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_free__overload_control_information( &rObj->overload_control_information);
	
	// pfcp_tlv_fq_csid_t sgw_u_fq_csid;												pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
	__up_pfcp_free__oct( &rObj->sgw_u_fq_csid);
	
    // pfcp_tlv_fq_csid_t pgw_u_fq_csid;												pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->pgw_u_fq_csid);
	
	// pfcp_tlv_failed_rule_id_t failed_rule_id;										pfcp_tlv_octet_t	PFCP_IE_FAILED_RULE_ID
    __up_pfcp_free__oct( &rObj->failed_rule_id);
	
	// pfcp_tlv_created_traffic_endpoint_t created_traffic_endpoint;					PFCP_IE_CREATED_TRAFFIC_ENDPOINT
	__up_pfcp_free__created_traffic_endpoint( &rObj->created_traffic_endpoint);

	return 0;
}

int __up_pfcp_free__session_modification_request( pfcp_message_t * mObj)
{
	pfcp_session_modification_request_t  *  rObj = &mObj->u.session_modification_request;
	
	int i = 0;
    // pfcp_tlv_f_seid_t cp_f_seid;								pfcp_tlv_octet_t	PFCP_IE_F_SEID
	__up_pfcp_free__oct( &rObj->cp_f_seid);
	
    // pfcp_tlv_remove_pdr_t remove_pdr[8];						PFCP_IE_REMOVE_PDR
	for( i = 0; i < 8; i++)
	{	
		__up_pfcp_free__remove_pdr( &rObj->remove_pdr[i]);
	}
	
    // pfcp_tlv_remove_far_t remove_far[8];						PFCP_IE_REMOVE_FAR		
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_free__remove_far( &rObj->remove_far[i]);
	}
	
	// pfcp_tlv_remove_urr_t remove_urr[2];						PFCP_IE_REMOVE_URR
    for( i = 0; i < 10; i++)
	{	
		__up_pfcp_free__remove_urr( &rObj->remove_urr[i]);
	}
	
	// pfcp_tlv_remove_qer_t remove_qer[4];						PFCP_IE_REMOVE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_free__remove_qer( &rObj->remove_qer[i]);
	}
	
	// pfcp_tlv_remove_bar_t remove_bar;						PFCP_IE_REMOVE_BAR
    __up_pfcp_free__remove_bar( &rObj->remove_bar);
	
	// pfcp_tlv_remove_traffic_endpoint_t remove_traffic_endpoint;	PFCP_IE_REMOVE_TRAFFIC_ENDPOINT
    __up_pfcp_free__remove_traffic_endpoint( &rObj->remove_traffic_endpoint);
	
	// pfcp_tlv_create_pdr_t create_pdr[8];							PFCP_IE_CREATE_PDR
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_free__create_pdr( &rObj->create_pdr[i]);
	}
	
	// pfcp_tlv_create_far_t create_far[8];							PFCP_IE_CREATE_FAR
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_free__create_far( &rObj->create_far[i]);
	}
	
	// pfcp_tlv_create_urr_t create_urr[2];							PFCP_IE_CREATE_URR
    for( i = 0; i < 10; i++)
	{	
		__up_pfcp_free__create_urr( &rObj->create_urr[i]);
	}
	
	// pfcp_tlv_create_qer_t create_qer[4];							PFCP_IE_CREATE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_free__create_qer( &rObj->create_qer[i]);
	}
	
	// pfcp_tlv_create_bar_t create_bar;							PFCP_IE_CREATE_BAR
    __up_pfcp_free__create_bar( &rObj->create_bar);
	
	// pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;	PFCP_IE_CREATE_TRAFFIC_ENDPOINT
    __up_pfcp_free__create_traffic_endpoint( &rObj->create_traffic_endpoint);
	
	// pfcp_tlv_update_pdr_t update_pdr[8];							PFCP_IE_UPDATE_PDR
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_free__update_pdr( &rObj->update_pdr[i]);
	}
	
    // pfcp_tlv_update_far_t update_far[8];
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_free__update_far( &rObj->update_far[i]);
	}

    // pfcp_tlv_update_urr_t update_urr[2];									PFCP_IE_UPDATE_URR
	for( i = 0; i < 10; i++)
	{		
		__up_pfcp_free__update_urr( &rObj->update_urr[i]);
	}
	
	// pfcp_tlv_update_qer_t update_qer[4];									PFCP_IE_UPDATE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_free__update_qer( &rObj->update_qer[i]);
	}
	
	// pfcp_tlv_update_bar_session_modification_request_t update_bar;		PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST
    __up_pfcp_free__update_bar_session_modification_request( &rObj->update_bar);
	
	// pfcp_tlv_update_traffic_endpoint_t update_traffic_endpoint;			pfcp_tlv_octet_t	PFCP_IE_UPDATE_TRAFFIC_ENDPOINT
    __up_pfcp_free__oct( &rObj->update_traffic_endpoint);
	
	// pfcp_tlv_query_urr_t query_urr	;									PFCP_IE_QUERY_URR	
    __up_pfcp_free__query_urr( &rObj->query_urr);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;									pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
	__up_pfcp_free__oct( &rObj->pgw_c_fq_csid);
	
	// pfcp_tlv_fq_csid_t sgw_c_fq_csid;									pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->sgw_c_fq_csid);
	
	// pfcp_tlv_fq_csid_t mme_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->mme_fq_csid);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->epdg_fq_csid);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_free__oct( &rObj->twan_fq_csid);
	
	// pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;	pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_INACTIVITY_TIMER
    __up_pfcp_free__oct( &rObj->user_plane_inactivity_timer);
	
	// pfcp_tlv_query_urr_reference_t query_urr_reference;					pfcp_tlv_octet_t	PFCP_IE_QUERY_URR_REFERENCE
    __up_pfcp_free__oct( &rObj->query_urr_reference);
	
	// pfcp_tlv_trace_information_t trace_information;						pfcp_tlv_octet_t	PFCP_IE_TRACE_INFORMATION
    __up_pfcp_free__oct( &rObj->trace_information);
	
	// pfcp_tlv_remove_mar_t remove_mar;									PFCP_IE_REMOVE_MAR_TYPE
    __up_pfcp_free__remove_mar( &rObj->remove_mar);
	
	// pfcp_tlv_update_mar_t update_mar;									PFCP_IE_UPDATE_MAR_TYPE
    __up_pfcp_free__update_mar( &rObj->update_mar);
	
	// pfcp_tlv_create_mar_t create_mar;									PFCP_IE_CREATE_MAR_TYPE
    __up_pfcp_free__create_mar( &rObj->create_mar);
	
	// pfcp_tlv_node_id_t node_id;											pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_free__oct( &rObj->node_id);

	
	return 0;
}

int __up_pfcp_free__session_modification_response( pfcp_message_t * mObj)
{
	pfcp_session_modification_response_t  *  rObj = &mObj->u.session_modification_response;
	
    // pfcp_tlv_created_pdr_t created_pdr[8];						PFCP_IE_CREATED_PDR
    int i = 0;
	for( i = 0; i < 8; i++) {
		__up_pfcp_free__created_pdr( &rObj->created_pdr[i]);
	}
	
	// pfcp_tlv_load_control_information_t load_control_information;	PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_free__load_control_information( &rObj->load_control_information);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;		PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_free__overload_control_information( &rObj->overload_control_information);
	
	// pfcp_tlv_usage_report_session_modification_response_t usage_report;		PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE
    __up_pfcp_free__usage_report_session_modification_response( &rObj->usage_report);
	
	// pfcp_tlv_failed_rule_id_t failed_rule_id;								pfcp_tlv_octet_t	PFCP_IE_FAILED_RULE_ID					// pfcp_tlv_octet_t
    __up_pfcp_free__oct( &rObj->failed_rule_id);
	
	// pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;		pfcp_tlv_octet_t PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION // pfcp_tlv_octet_t	
    __up_pfcp_free__oct( &rObj->additional_usage_reports_information);
	
	// pfcp_tlv_created_traffic_endpoint_t created_updated_traffic_endpoint;		PFCP_IE_CREATED_TRAFFIC_ENDPOINT
	__up_pfcp_free__created_traffic_endpoint( &rObj->created_updated_traffic_endpoint);
	
	return 0;
}

int __up_pfcp_free__session_deletion_request( pfcp_message_t * mObj)
{
	return 0;
}

int __up_pfcp_free__session_deletion_response( pfcp_message_t * mObj)
{
	pfcp_session_deletion_response_t  *  rObj = &mObj->u.session_deletion_response;
	
    // pfcp_tlv_load_control_information_t load_control_information;			PFCP_IE_LOAD_CONTROL_INFORMATION
	__up_pfcp_free__load_control_information( &rObj->load_control_information);
	
    // pfcp_tlv_overload_control_information_t overload_control_information;	PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_free__overload_control_information( &rObj->overload_control_information);
	
	// pfcp_tlv_usage_report_session_deletion_response_t usage_report;			PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE
	__up_pfcp_free__usage_report_session_deletion_response( &rObj->usage_report);

	
	return 0;
}

int __up_pfcp_free__session_report_request( pfcp_message_t * mObj)
{
	pfcp_session_report_request_t *  rObj = &mObj->u.session_report_request;

	// pfcp_tlv_downlink_data_report_t downlink_data_report;				PFCP_IE_DOWNLINK_DATA_REPORT
    __up_pfcp_free__downlink_data_report( &rObj->downlink_data_report);
	
	// pfcp_tlv_usage_report_session_report_request_t usage_report;			PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST
    __up_pfcp_free__usage_report_session_report_request( &rObj->usage_report);
	
	// pfcp_tlv_error_indication_report_t error_indication_report;			PFCP_IE_ERROR_INDICATION_REPORT
    __up_pfcp_free__error_indication_report( &rObj->error_indication_report);
	
	// pfcp_tlv_load_control_information_t load_control_information;		PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_free__load_control_information( &rObj->load_control_information);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;		PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_free__overload_control_information( &rObj->overload_control_information);
	
	// pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;	pfcp_tlv_octet_t PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION
    __up_pfcp_free__oct( &rObj->additional_usage_reports_information);
	
	// pfcp_tlv_f_seid_t old_cp_f_seid;												pfcp_tlv_octet_t	PFCP_IE_F_SEID
	__up_pfcp_free__oct( &rObj->old_cp_f_seid);
	
	
	return 0;
}

//pfcp_session_report_response_t
int __up_pfcp_free__session_report_response( pfcp_message_t * mObj)
{
	pfcp_session_report_response_t * rObj = &mObj->u.session_report_response;
	
	// pfcp_tlv_update_bar_pfcp_session_report_response_t update_bar;		PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE
    __up_pfcp_free__update_bar( &rObj->update_bar);
	
	// pfcp_tlv_f_seid_t cp_f_seid;											PFCP_IE_F_SEID				pfcp_tlv_octet_t
    __up_pfcp_free__oct( &rObj->cp_f_seid);
	
	// pfcp_tlv_f_teid_t n4_u_f_teid;										PFCP_IE_F_TEID				pfcp_tlv_octet_t
    __up_pfcp_free__oct( &rObj->n4_u_f_teid);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE		pfcp_tlv_octet_t
	__up_pfcp_free__oct( &rObj->alternative_smf_ip_address);
	
	
	return 0;
}

int __up_pfcp_free__message( pfcp_message_t * mObj)
{
	//printf( "MessageType=%u  %s|%d\n", mObj->header.MessageType, __FILE__, __LINE__);
	
	switch( mObj->header.MessageType)
	{
		case PFCP_HEARTBEAT_REQUEST:
			return __up_pfcp_free__heartbeat_request( mObj);
		case PFCP_HEARTBEAT_RESPONSE:
			return __up_pfcp_free__heartbeat_response( mObj);
		case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
			return __up_pfcp_free__pfd_management_request( mObj);
		case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
			return __up_pfcp_free__pfd_management_response( mObj);
		case PFCP_ASSOCIATION_SETUP_REQUEST:
			return __up_pfcp_free__association_setup_request( mObj);
		case PFCP_ASSOCIATION_SETUP_RESPONSE:
			return __up_pfcp_free__association_setup_response( mObj);
		case PFCP_ASSOCIATION_UPDATE_REQUEST:
			return __up_pfcp_free__association_update_request( mObj);
		case PFCP_ASSOCIATION_UPDATE_RESPONSE:
			return __up_pfcp_free__association_update_response( mObj);
		case PFCP_ASSOCIATION_RELEASE_REQUEST:
			return __up_pfcp_free__association_release_request( mObj);
		case PFCP_ASSOCIATION_RELEASE_RESPONSE:
			return __up_pfcp_free__association_release_response( mObj);
		case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
			return __up_pfcp_free__version_not_supported_response( mObj);
		case PFCP_NODE_REPORT_REQUEST:
			return __up_pfcp_free__node_report_request( mObj);
		case PFCP_NODE_REPORT_RESPONSE:
			return __up_pfcp_free__node_report_response( mObj);
		case PFCP_SESSION_SET_DELETION_REQUEST:
			return __up_pfcp_free__session_set_deletion_request( mObj);
		case PFCP_SESSION_SET_DELETION_RESPONSE:
			return __up_pfcp_free__session_set_deletion_response( mObj);
		case PFCP_SESSION_ESTABLISHMENT_REQUEST:
			return __up_pfcp_free__session_establishment_request( mObj);
		case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
			return __up_pfcp_free__session_establishment_response( mObj);
		case PFCP_SESSION_MODIFICATION_REQUEST:
			return __up_pfcp_free__session_modification_request( mObj);
		case PFCP_SESSION_MODIFICATION_RESPONSE:
			return __up_pfcp_free__session_modification_response( mObj);
		case PFCP_SESSION_DELETION_REQUEST:
			return __up_pfcp_free__session_deletion_request( mObj);
		case PFCP_SESSION_DELETION_RESPONSE:
			return __up_pfcp_free__session_deletion_response( mObj);
		case PFCP_SESSION_REPORT_REQUEST:
			return __up_pfcp_free__session_report_request( mObj);
		case PFCP_SESSION_REPORT_RESPONSE:
			return __up_pfcp_free__session_report_response( mObj);
		default:
			return -1;
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





int __up_pfcp_decode__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_ethernet_filter_id_t ethernet_filter_id;					//PFCP_IE_ETHERNET_FILTER_ID			pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_filter_properties_t ethernet_filter_properties;	//PFCP_IE_ETHERNET_FILTER_PROPERTIES	pfcp_tlv_octet_t
    // pfcp_tlv_mac_address_t mac_address;									//PFCP_IE_MAC_ADDRESS					pfcp_tlv_octet_t
    // pfcp_tlv_ethertype_t ethertype;										//PFCP_IE_ETHERTYPE						pfcp_tlv_octet_t
    // pfcp_tlv_c_tag_t c_tag;												//PFCP_IE_C_TAG							pfcp_tlv_octet_t
    // pfcp_tlv_s_tag_t s_tag;												//PFCP_IE_S_TAG							pfcp_tlv_octet_t
    // pfcp_tlv_sdf_filter_t sdf_filter[8];									//PFCP_IE_SDF_FILTER					pfcp_tlv_octet_t
	
	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	uint8_t isdf_filter = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_ETHERNET_FILTER_ID:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_filter_id, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERNET_FILTER_PROPERTIES:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_filter_properties, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_MAC_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->mac_address, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERTYPE:
				__up_pfcp_decode__tlv_oct( &mObj->ethertype, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_C_TAG:
				__up_pfcp_decode__tlv_oct( &mObj->c_tag, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_S_TAG:
				__up_pfcp_decode__tlv_oct( &mObj->s_tag, &buffer[ bPos + 4], ELen);
				break;
			case PFCP_IE_SDF_FILTER:
				if( isdf_filter < 8) {
					__up_pfcp_decode__tlv_oct( &mObj->sdf_filter[isdf_filter], &buffer[ bPos + 4], ELen);
					isdf_filter++;
				}
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__pdi(pfcp_tlv_pdi_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_source_interface_t source_interface;									PFCP_IE_SOURCE_INTERFACE	pfcp_tlv_uint8_t
    // pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
    // pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
    // pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t
    // pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;								PFCP_IE_TRAFFIC_ENDPOINT_ID	pfcp_tlv_octet_t
    // pfcp_tlv_sdf_filter_t sdf_filter[8];												PFCP_IE_SDF_FILTER			pfcp_tlv_octet_t
    // pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID		pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;	PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_packet_filter_t ethernet_packet_filter;						PFCP_IE_ETHERNET_PACKET_FILTER
    // pfcp_tlv_qfi_t qfi;																PFCP_IE_QFI					pfcp_tlv_uint8_t
    // pfcp_tlv_framed_route_t framed_route;											PFCP_IE_FRAMED_ROUTE		pfcp_tlv_octet_t
    // pfcp_tlv_framed_routing_t framed_routing;										PFCP_IE_FRAMED_ROUTING		pfcp_tlv_octet_t
    // pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;									PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
    // pfcp_tlv__interface_type_t source_interface_type;								PFCP_IE_INTERFACE_TYPE_TYPE	pfcp_tlv_octet_t


	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	int isdf_filter = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_SOURCE_INTERFACE:
				__up_pfcp_decode__tlv_u8( &mObj->source_interface, &buffer[bPos + 4]);
				break;
			case PFCP_IE_F_TEID:
				__up_pfcp_decode__tlv_oct( &mObj->local_f_teid, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_NETWORK_INSTANCE:
				__up_pfcp_decode__tlv_oct( &mObj->network_instance, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UE_IP_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->ue_ip_address, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SDF_FILTER:
				if( isdf_filter < 8)
				{
					__up_pfcp_decode__tlv_oct( &mObj->sdf_filter[isdf_filter], &buffer[bPos + 4], ELen);
					isdf_filter++;
				}
				break;
			case PFCP_IE_APPLICATION_ID:
				__up_pfcp_decode__tlv_oct( &mObj->application_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_pdu_session_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERNET_PACKET_FILTER:
				__up_pfcp_decode__ethernet_packet_filter( &mObj->ethernet_packet_filter, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QFI:
				__up_pfcp_decode__tlv_u8( &mObj->qfi, &buffer[bPos + 4]);
				break;
			case PFCP_IE_FRAMED_ROUTE:
				__up_pfcp_decode__tlv_oct( &mObj->framed_route, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FRAMED_ROUTING:
				__up_pfcp_decode__tlv_oct( &mObj->framed_routing, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FRAMED_IPV6_ROUTE:
				__up_pfcp_decode__tlv_oct( &mObj->framed_ipv6_route, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_INTERFACE_TYPE_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->source_interface_type, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_pdr(pfcp_tlv_create_pdr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t		
    // pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE					pfcp_tlv_uint32_t		
    // pfcp_tlv_pdi_t pdi;
    // pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID						pfcp_tlv_uint32_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID						pfcp_tlv_uint32_t
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID						pfcp_tlv_uint32_t
    // pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
    // pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_mar_id_t mar_id;														PFCP_IE_MAR_ID_TYPE					pfcp_tlv_octet_t
    // pfcp_tlv_packet_replication_and_detection_carry_on_information_t packet_replication_and_detection_carry_on_information;	
														//PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE	pfcp_tlv_octet_t
	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PACKET_DETECTION_RULE_ID:
				__up_pfcp_decode__tlv_u16( &mObj->pdr_id, &buffer[bPos + 4]);
				break;			
			case PFCP_IE_PRECEDENCE:
				__up_pfcp_decode__tlv_u32( &mObj->precedence, &buffer[bPos + 4]);
				break;
			case PFCP_IE_PDI:
				__up_pfcp_decode__pdi( &mObj->pdi, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_OUTER_HEADER_REMOVAL:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_removal, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;			
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_QER_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_id, &buffer[bPos + 4]);
				break;			
			case PFCP_IE_ACTIVATE_PREDEFINED_RULES:
				__up_pfcp_decode__tlv_oct( &mObj->activate_predefined_rules, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACTIVATION_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->activation_time, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_DEACTIVATION_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->deactivation_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MAR_ID_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->mar_id, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->packet_replication_and_detection_carry_on_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}

	
	return 0;
}

int __up_pfcp_decode__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
    // pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
    // pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
    // pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
    // pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
    // pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
    // pfcp_tlv_proxying_t proxying;													PFCP_IE_PROXYING				pfcp_tlv_octet_t
    // pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t	

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_DESTINATION_INTERFACE:
				__up_pfcp_decode__tlv_u8( &mObj->destination_interface, &buffer[bPos + 4]);
				break;
			case PFCP_IE_NETWORK_INSTANCE:
				__up_pfcp_decode__tlv_oct( &mObj->network_instance, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_REDIRECT_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->redirect_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_OUTER_HEADER_CREATION:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_creation, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRANSPORT_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->transport_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FORWARDING_POLICY:
				__up_pfcp_decode__tlv_oct( &mObj->forwarding_policy, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_HEADER_ENRICHMENT:
				__up_pfcp_decode__tlv_oct( &mObj->header_enrichment, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->linked_traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PROXYING:
				__up_pfcp_decode__tlv_oct( &mObj->proxying, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_INTERFACE_TYPE_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->destination_interface_type, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
    // pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
    // pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_DESTINATION_INTERFACE:
				__up_pfcp_decode__tlv_u8( &mObj->destination_interface, &buffer[bPos + 4]);
				break;
			case PFCP_IE_OUTER_HEADER_CREATION:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_creation, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRANSPORT_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->transport_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FORWARDING_POLICY:
				__up_pfcp_decode__tlv_oct( &mObj->forwarding_policy, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_far(pfcp_tlv_create_far_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
    // pfcp_tlv_forwarding_parameters_t forwarding_parameters;							PFCP_IE_UPDATE_FORWARDING_PARAMETERS
    // pfcp_tlv_duplicating_parameters_t duplicating_parameters;						PFCP_IE_UPDATE_DUPLICATING_PARAMETERS
    // pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_APPLY_ACTION:
				__up_pfcp_decode__tlv_u8( &mObj->apply_action, &buffer[bPos + 4]);
				break;
			case PFCP_IE_FORWARDING_PARAMETERS:
				__up_pfcp_decode__forwarding_parameters( &mObj->forwarding_parameters, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DUPLICATING_PARAMETERS:
				__up_pfcp_decode__duplicating_parameters( &mObj->duplicating_parameters, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_forwarding_parameters(pfcp_tlv_update_forwarding_parameters_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
    // pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
    // pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
    // pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
    // pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
    // pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;										PFCP_IE_PFCPSMREQ_FLAGS			pfcp_tlv_uint8_t
    // pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
    // pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_DESTINATION_INTERFACE:
				__up_pfcp_decode__tlv_u8( &mObj->destination_interface, &buffer[bPos + 4]);
				break;
			case PFCP_IE_NETWORK_INSTANCE:
				__up_pfcp_decode__tlv_oct( &mObj->network_instance, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_REDIRECT_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->redirect_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_OUTER_HEADER_CREATION:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_creation, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRANSPORT_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->transport_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FORWARDING_POLICY:
				__up_pfcp_decode__tlv_oct( &mObj->forwarding_policy, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_HEADER_ENRICHMENT:
				__up_pfcp_decode__tlv_oct( &mObj->header_enrichment, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PFCPSMREQ_FLAGS:
				__up_pfcp_decode__tlv_u8( &mObj->pfcpsmreq_flags, &buffer[bPos + 4]);
				break;
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->linked_traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_INTERFACE_TYPE_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->destination_interface_type, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
    // pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
    // pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_DESTINATION_INTERFACE:
				__up_pfcp_decode__tlv_u8( &mObj->destination_interface, &buffer[bPos + 4]);
				break;
			case PFCP_IE_OUTER_HEADER_CREATION:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_creation, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TRANSPORT_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->transport_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FORWARDING_POLICY:
				__up_pfcp_decode__tlv_oct( &mObj->forwarding_policy, &buffer[bPos + 4], ELen);
				break;			
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_far(pfcp_tlv_update_far_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
    // pfcp_tlv_update_forwarding_parameters_t update_forwarding_parameters;			PFCP_IE_UPDATE_FORWARDING_PARAMETERS
    // pfcp_tlv_update_duplicating_parameters_t update_duplicating_parameters;			PFCP_IE_UPDATE_DUPLICATING_PARAMETERS
    // pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_APPLY_ACTION:
				__up_pfcp_decode__tlv_u8( &mObj->apply_action, &buffer[bPos + 4]);
				break;
			case PFCP_IE_UPDATE_FORWARDING_PARAMETERS:
				__up_pfcp_decode__update_forwarding_parameters( &mObj->update_forwarding_parameters, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UPDATE_DUPLICATING_PARAMETERS:
				__up_pfcp_decode__update_duplicating_parameters( &mObj->update_duplicating_parameters, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__pfd_context(pfcp_tlv_pfd_context_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pfd_contents_t pfd_contents;											PFCP_IE_PFD_CONTENTS		pfcp_tlv_octet_t
	
	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PFD_CONTENTS:
				__up_pfcp_decode__tlv_oct( &mObj->pfd_contents, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_IDS_PFDS	pfcp_tlv_octet_t
    // pfcp_tlv_pfd_context_t pfd_context;												PFCP_IE_PFD_CONTEXT

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_APPLICATION_IDS_PFDS:
				__up_pfcp_decode__tlv_oct( &mObj->application_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PFD_CONTEXT:
				__up_pfcp_decode__pfd_context( &mObj->pfd_context, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_mac_addresses_detected_t mac_addresses_detected;						PFCP_IE_MAC_ADDRESSES_DETECTED	pfcp_tlv_octet_t	
    // pfcp_tlv_mac_addresses_removed_t mac_addresses_removed;							PFCP_IE_MAC_ADDRESSES_REMOVED	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_MAC_ADDRESSES_DETECTED:
				__up_pfcp_decode__tlv_oct( &mObj->mac_addresses_detected, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MAC_ADDRESSES_REMOVED:
				__up_pfcp_decode__tlv_oct( &mObj->mac_addresses_removed, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_WEIGHT_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->weight, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PRIORITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->priority, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_WEIGHT_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->weight, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PRIORITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->priority, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_WEIGHT_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->weight, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PRIORITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->priority, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_WEIGHT_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->weight, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PRIORITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->priority, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_urr(pfcp_tlv_create_urr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
    // pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t
    // pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
    // pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
    // pfcp_tlv_event_threshold_t event_threshold;										PFCP_IE_EVENT_THRESHOLD		pfcp_tlv_octet_t
    // pfcp_tlv_event_quota_t event_quota;												PFCP_IE_EVENT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
    // pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
    // pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
    // pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_VOLUME_QUOTA	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_EVENT_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_EVENT_ID	pfcp_tlv_octet_t
    // pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
    // pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
    // pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
    // pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
    // pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MEASUREMENT_METHOD:
				__up_pfcp_decode__tlv_u8( &mObj->measurement_method, &buffer[bPos + 4]);
				break;
			case PFCP_IE_REPORTING_TRIGGERS:
				__up_pfcp_decode__tlv_u8( &mObj->reporting_triggers, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MEASUREMENT_PERIOD:
				__up_pfcp_decode__tlv_oct( &mObj->measurement_period, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_VOLUME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->volume_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_VOLUME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->volume_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_EVENT_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->event_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_EVENT_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->event_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->time_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->time_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QUOTA_HOLDING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->quota_holding_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->dropped_dl_traffic_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QUOTA_VALIDITY_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->quota_validity_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MONITORING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->monitoring_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_volume_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_TIME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_time_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_VOLUME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_volume_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_TIME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_time_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBS_EVENT_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_event_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBS_EVENT_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_event_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_INACTIVITY_DETECTION_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->inactivity_detection_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_LINKED_URR_ID:
				__up_pfcp_decode__tlv_oct( &mObj->linked_urr_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MEASUREMENT_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->measurement_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_QUOTA_MECHANISM:
				__up_pfcp_decode__tlv_oct( &mObj->time_quota_mechanism, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_AGGREGATED_URRS:
				__up_pfcp_decode__tlv_oct( &mObj->aggregated_urrs, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id_for_quota_action, &buffer[bPos + 4]);
				break;
			case PFCP_IE_ETHERNET_INACTIVITY_TIMER:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_inactivity_timer, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ADDITIONAL_MONITORING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->additional_monitoring_time, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}

	
	return 0;
}

int __up_pfcp_decode__create_qer(pfcp_tlv_create_qer_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
    // pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS	pfcp_tlv_uint8_t
    // pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR	pfcp_tlv_octet_t
    // pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR	pfcp_tlv_octet_t
    // pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
    // pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI	pfcp_tlv_uint8_t
    // pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI	pfcp_tlv_uint8_t
    // pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t	
    // pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE	pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_QER_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_QER_CORRELATION_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_correlation_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_GATE_STATUS:
				__up_pfcp_decode__tlv_u8( &mObj->gate_status, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MBR:
				__up_pfcp_decode__tlv_oct( &mObj->maximum_bitrate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_GBR:
				__up_pfcp_decode__tlv_oct( &mObj->guaranteed_bitrate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PACKET_RATE:
				__up_pfcp_decode__tlv_oct( &mObj->packet_rate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DL_FLOW_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->dl_flow_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QFI:
				__up_pfcp_decode__tlv_u8( &mObj->qos_flow_identifier, &buffer[bPos + 4]);
				break;
			case PFCP_IE_RQI:
				__up_pfcp_decode__tlv_u8( &mObj->reflective_qos, &buffer[bPos + 4]);
				break;
			case PFCP_IE_PAGING_POLICY_INDICATOR_TYPE:
				__up_pfcp_decode__tlv_u8( &mObj->paging_policy_indicator, &buffer[bPos + 4]);
				break;
			case PFCP_IE_AVERAGING_WINDOW_TYPE:
				__up_pfcp_decode__tlv_u32( &mObj->averaging_window, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__created_pdr(pfcp_tlv_created_pdr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
    // pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
    // pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PACKET_DETECTION_RULE_ID:
				__up_pfcp_decode__tlv_u16( &mObj->pdr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_F_TEID:
				__up_pfcp_decode__tlv_oct( &mObj->local_f_teid, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UE_IP_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->ue_ip_address, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_pdr(pfcp_tlv_update_pdr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t
    // pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
    // pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE		pfcp_tlv_uint32_t
    // pfcp_tlv_pdi_t pdi	;															PFCP_IE_PDI
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
    // pfcp_tlv_deactivate_predefined_rules_t deactivate_predefined_rules;				PFCP_IE_DEACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
    // pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE	pfcp_tlv_octet_t	
    // pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PACKET_DETECTION_RULE_ID:
				__up_pfcp_decode__tlv_u16( &mObj->pdr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_OUTER_HEADER_REMOVAL:
				__up_pfcp_decode__tlv_oct( &mObj->outer_header_removal, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PRECEDENCE:
				__up_pfcp_decode__tlv_u32( &mObj->precedence, &buffer[bPos + 4]);
				break;
			case PFCP_IE_PDI:
				__up_pfcp_decode__pdi( &mObj->pdi, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_QER_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_ACTIVATE_PREDEFINED_RULES:
				__up_pfcp_decode__tlv_oct( &mObj->activate_predefined_rules, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DEACTIVATE_PREDEFINED_RULES:
				__up_pfcp_decode__tlv_oct( &mObj->deactivate_predefined_rules, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACTIVATION_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->activation_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DEACTIVATION_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->deactivation_time, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_bar_id_t bar_id;																	PFCP_IE_BAR_ID		pfcp_tlv_uint8_t
    // pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;				PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
    // pfcp_tlv_dl_buffering_duration_t dl_buffering_duration;										PFCP_IE_DL_BUFFERING_DURATION	pfcp_tlv_octet_t
    // pfcp_tlv_dl_buffering_suggested_packet_count_t dl_buffering_suggested_packet_count;			PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT	pfcp_tlv_octet_t
    // pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;				PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
	
	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY:
				__up_pfcp_decode__tlv_oct( &mObj->downlink_data_notification_delay, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DL_BUFFERING_DURATION:
				__up_pfcp_decode__tlv_oct( &mObj->dl_buffering_duration, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT:
				__up_pfcp_decode__tlv_oct( &mObj->dl_buffering_suggested_packet_count, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT:
				__up_pfcp_decode__tlv_oct( &mObj->suggested_buffering_packets_count, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_urr(pfcp_tlv_update_urr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
    // pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t	
    // pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
    // pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
    // pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
    // pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
    // pfcp_tlv_event_threshold_t event_threshold;										pfcp_tlv_event_threshold_t
    // pfcp_tlv_event_quota_t event_quota;												pfcp_tlv_octet_t
    // pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t	
    // pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
    // pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_VOLUME_QUOTA	pfcp_tlv_octet_t	
    // pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_EVENT_THRESHOLD	pfcp_tlv_octet_t
    // pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_EVENT_ID	pfcp_tlv_octet_t
    // pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
    // pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
    // pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
    // pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
    // pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MEASUREMENT_METHOD:
				__up_pfcp_decode__tlv_u8( &mObj->measurement_method, &buffer[bPos + 4]);
				break;
			case PFCP_IE_REPORTING_TRIGGERS:
				__up_pfcp_decode__tlv_u8( &mObj->reporting_triggers, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MEASUREMENT_PERIOD:
				__up_pfcp_decode__tlv_oct( &mObj->measurement_period, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_VOLUME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->volume_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_VOLUME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->volume_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->time_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->time_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_EVENT_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->event_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_EVENT_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->event_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QUOTA_HOLDING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->quota_holding_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->dropped_dl_traffic_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QUOTA_VALIDITY_TIME_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->quota_validity_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MONITORING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->monitoring_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_volume_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_TIME_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_time_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_VOLUME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_volume_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBSEQUENT_TIME_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_time_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBS_EVENT_THRESHOLD:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_event_threshold, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUBS_EVENT_QUOTA:
				__up_pfcp_decode__tlv_oct( &mObj->subsequent_event_quota, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_INACTIVITY_DETECTION_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->inactivity_detection_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_LINKED_URR_ID:
				__up_pfcp_decode__tlv_oct( &mObj->linked_urr_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_MEASUREMENT_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->measurement_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_QUOTA_MECHANISM:
				__up_pfcp_decode__tlv_oct( &mObj->time_quota_mechanism, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_AGGREGATED_URRS:
				__up_pfcp_decode__tlv_oct( &mObj->aggregated_urrs, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id_for_quota_action, &buffer[bPos + 4]);
				break;
			case PFCP_IE_ETHERNET_INACTIVITY_TIMER:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_inactivity_timer, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ADDITIONAL_MONITORING_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->additional_monitoring_time, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_qer(pfcp_tlv_update_qer_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
    // pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS		pfcp_tlv_uint8_t
    // pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR		pfcp_tlv_octet_t
    // pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR		pfcp_tlv_octet_t
    // pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
    // pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI		pfcp_tlv_uint8_t
    // pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI		pfcp_tlv_uint8_t
    // pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t
    // pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE		pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_QER_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_QER_CORRELATION_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_correlation_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_GATE_STATUS:
				__up_pfcp_decode__tlv_u8( &mObj->gate_status, &buffer[bPos + 4]);
				break;
			case PFCP_IE_MBR:
				__up_pfcp_decode__tlv_oct( &mObj->maximum_bitrate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_GBR:
				__up_pfcp_decode__tlv_oct( &mObj->guaranteed_bitrate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_PACKET_RATE:
				__up_pfcp_decode__tlv_oct( &mObj->packet_rate, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DL_FLOW_LEVEL_MARKING:
				__up_pfcp_decode__tlv_oct( &mObj->dl_flow_level_marking, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QFI:
				__up_pfcp_decode__tlv_u8( &mObj->qos_flow_identifier, &buffer[bPos + 4]);
				break;
			case PFCP_IE_RQI:
				__up_pfcp_decode__tlv_u8( &mObj->reflective_qos, &buffer[bPos + 4]);
				break;
			case PFCP_IE_PAGING_POLICY_INDICATOR_TYPE:
				__up_pfcp_decode__tlv_u8( &mObj->paging_policy_indicator, &buffer[bPos + 4]);
				break;
			case PFCP_IE_AVERAGING_WINDOW_TYPE:
				__up_pfcp_decode__tlv_u32( &mObj->averaging_window, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_pdr(pfcp_tlv_remove_pdr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PACKET_DETECTION_RULE_ID:
				__up_pfcp_decode__tlv_u16( &mObj->pdr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_far(pfcp_tlv_remove_far_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID		pfcp_tlv_uint32_t
	
	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_FAR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->far_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_urr(pfcp_tlv_remove_urr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_qer(pfcp_tlv_remove_qer_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->qer_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__load_control_information(pfcp_tlv_load_control_information_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_sequence_number_t load_control_sequence_number;							PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t
    // pfcp_tlv_metric_t load_metric;													PFCP_IE_METRIC				pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_SEQUENCE_NUMBER:
				__up_pfcp_decode__tlv_oct( &mObj->load_control_sequence_number, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_METRIC:
				__up_pfcp_decode__tlv_oct( &mObj->load_metric, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__overload_control_information(pfcp_tlv_overload_control_information_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_sequence_number_t overload_control_sequence_number;						PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t	
    // pfcp_tlv_metric_t overload_reduction_metric;										PFCP_IE_METRIC				pfcp_tlv_octet_t
    // pfcp_tlv_timer_t period_of_validity;												PFCP_IE_TIMER				pfcp_tlv_octet_t
    // pfcp_tlv_oci_flags_t overload_control_information_flags;							PFCP_IE_OCI_FLAGS			pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_SEQUENCE_NUMBER:
				__up_pfcp_decode__tlv_oct( &mObj->overload_control_sequence_number, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_METRIC:
				__up_pfcp_decode__tlv_oct( &mObj->overload_reduction_metric, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIMER:
				__up_pfcp_decode__tlv_oct( &mObj->period_of_validity, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_OCI_FLAGS:
				__up_pfcp_decode__tlv_oct( &mObj->overload_control_information_flags, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__application_detection_information(pfcp_tlv_application_detection_information_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID	pfcp_tlv_octet_t
    // pfcp_tlv_application_instance_id_t application_instance_id;						PFCP_IE_APPLICATION_INSTANCE_ID	pfcp_tlv_octet_t
    // pfcp_tlv_flow_information_t flow_information;									PFCP_IE_FLOW_INFORMATION	pfcp_tlv_octet_t	

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_APPLICATION_ID:
				__up_pfcp_decode__tlv_oct( &mObj->application_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_APPLICATION_INSTANCE_ID:
				__up_pfcp_decode__tlv_oct( &mObj->application_instance_id, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_FLOW_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->flow_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__query_urr(pfcp_tlv_query_urr_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
    // pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t
    // pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
    // pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t	
    // pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
    // pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
    // pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;			
			case PFCP_IE_UR_SEQN:
				__up_pfcp_decode__tlv_oct( &mObj->ur_seqn, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_USAGE_REPORT_TRIGGER:
				__up_pfcp_decode__tlv_oct( &mObj->usage_report_trigger, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_START_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->start_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_END_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->end_time, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_VOLUME_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->volume_measurement, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DURATION_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->duration_measurement, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_TIME_OF_FIRST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_first_packet, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_OF_LAST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_last_packet, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_USAGE_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->usage_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QUERY_URR_REFERENCE:
				__up_pfcp_decode__tlv_oct( &mObj->query_urr_reference, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_ETHERNET_TRAFFIC_INFORMATION:
				__up_pfcp_decode__ethernet_traffic_information( &mObj->ethernet_traffic_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
    // pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN	pfcp_tlv_octet_t
    // pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
    // pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
    // pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT pfcp_tlv_octet_t
    // pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t	
    // pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_UR_SEQN:
				__up_pfcp_decode__tlv_oct( &mObj->ur_seqn, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_USAGE_REPORT_TRIGGER:
				__up_pfcp_decode__tlv_oct( &mObj->usage_report_trigger, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_START_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->start_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_END_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->end_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_VOLUME_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->volume_measurement, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DURATION_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->duration_measurement, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_OF_FIRST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_first_packet, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_OF_LAST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_last_packet, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_USAGE_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->usage_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERNET_TRAFFIC_INFORMATION:
				__up_pfcp_decode__ethernet_traffic_information( &mObj->ethernet_traffic_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
    // pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t	
    // pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
    // pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME		pfcp_tlv_octet_t
    // pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME		pfcp_tlv_octet_t
    // pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
    // pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
    // pfcp_tlv_application_detection_information_t application_detection_information;	PFCP_IE_APPLICATION_DETECTION_INFORMATION
    // pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
    // pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
    // pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
    // pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
    // pfcp_tlv_event_time_stamp_t event_time_stamp;									PFCP_IE_EVENT_TIME_STAMP_TYPE	pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_URR_ID:
				__up_pfcp_decode__tlv_u32( &mObj->urr_id, &buffer[bPos + 4]);
				break;			
			case PFCP_IE_UR_SEQN:
				__up_pfcp_decode__tlv_oct( &mObj->ur_seqn, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_USAGE_REPORT_TRIGGER:
				__up_pfcp_decode__tlv_oct( &mObj->usage_report_trigger, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_START_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->start_time, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_END_TIME:
				__up_pfcp_decode__tlv_oct( &mObj->end_time, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_VOLUME_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->volume_measurement, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_DURATION_MEASUREMENT:
				__up_pfcp_decode__tlv_oct( &mObj->duration_measurement, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_APPLICATION_DETECTION_INFORMATION:
				__up_pfcp_decode__application_detection_information( &mObj->application_detection_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UE_IP_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->ue_ip_address, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_NETWORK_INSTANCE:
				__up_pfcp_decode__tlv_oct( &mObj->network_instance, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_TIME_OF_FIRST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_first_packet, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_TIME_OF_LAST_PACKET:
				__up_pfcp_decode__tlv_oct( &mObj->time_of_last_packet, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_USAGE_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->usage_information, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_QUERY_URR_REFERENCE:
				__up_pfcp_decode__tlv_oct( &mObj->query_urr_reference, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_EVENT_TIME_STAMP_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->event_time_stamp, &buffer[bPos + 4], ELen);
				break;			
			case PFCP_IE_ETHERNET_TRAFFIC_INFORMATION:
				__up_pfcp_decode__ethernet_traffic_information( &mObj->ethernet_traffic_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pdr_id_t pdr_id;															PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
    // pfcp_tlv_downlink_data_service_information_t downlink_data_service_information;		PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_PACKET_DETECTION_RULE_ID:
				__up_pfcp_decode__tlv_u16( &mObj->pdr_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->downlink_data_service_information, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_bar(pfcp_tlv_create_bar_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
    // pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
    // pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY:
				__up_pfcp_decode__tlv_oct( &mObj->downlink_data_notification_delay, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT:	
				__up_pfcp_decode__tlv_oct( &mObj->suggested_buffering_packets_count, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
    // pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
    // pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;
			case PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY:
				__up_pfcp_decode__tlv_oct( &mObj->downlink_data_notification_delay, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT:	
				__up_pfcp_decode__tlv_oct( &mObj->suggested_buffering_packets_count, &buffer[bPos + 4], ELen);
				break;			
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_bar(pfcp_tlv_remove_bar_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_BAR_ID:
				__up_pfcp_decode__tlv_u8( &mObj->bar_id, &buffer[bPos + 4]);
				break;			
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__error_indication_report(pfcp_tlv_error_indication_report_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_f_teid_t remote_f_teid;														PFCP_IE_F_TEID	pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_F_TEID:
				__up_pfcp_decode__tlv_oct( &mObj->remote_f_teid, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_remote_gtp_u_peer_t remote_gtp_u_peer_;										PFCP_IE_REMOTE_GTP_U_PEER	pfcp_tlv_octet_t	

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_REMOTE_GTP_U_PEER:
				__up_pfcp_decode__tlv_oct( &mObj->remote_gtp_u_peer_, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
    // pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID		pfcp_tlv_octet_t
    // pfcp_tlv_network_instance_t network_instance;										PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
    // pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
    // pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;		PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_framed_route_t framed_route;												PFCP_IE_FRAMED_ROUTE	pfcp_tlv_octet_t
    // pfcp_tlv_framed_routing_t framed_routing;											PFCP_IE_FRAMED_ROUTING	pfcp_tlv_octet_t
    // pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;										PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
    // pfcp_tlv_qfi_t qfi;																	PFCP_IE_QFI			pfcp_tlv_uint8_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_F_TEID:
				__up_pfcp_decode__tlv_oct( &mObj->local_f_teid, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_NETWORK_INSTANCE:
				__up_pfcp_decode__tlv_oct( &mObj->network_instance, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UE_IP_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->ue_ip_address, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION:
				__up_pfcp_decode__tlv_oct( &mObj->ethernet_pdu_session_information, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FRAMED_ROUTE:
				__up_pfcp_decode__tlv_oct( &mObj->framed_route, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FRAMED_ROUTING:
				__up_pfcp_decode__tlv_oct( &mObj->framed_routing, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_FRAMED_IPV6_ROUTE:
				__up_pfcp_decode__tlv_oct( &mObj->framed_ipv6_route, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_QFI:
				__up_pfcp_decode__tlv_u8( &mObj->qfi, &buffer[bPos + 4]);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
    // pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID					pfcp_tlv_octet_t
    // pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS			pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_F_TEID:
				__up_pfcp_decode__tlv_oct( &mObj->local_f_teid, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UE_IP_ADDRESS:
				__up_pfcp_decode__tlv_oct( &mObj->ue_ip_address, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_TRAFFIC_ENDPOINT_ID:
				__up_pfcp_decode__tlv_oct( &mObj->traffic_endpoint_id, &buffer[bPos + 4], ELen);
				break;
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__create_mar(pfcp_tlv_create_mar_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE pfcp_tlv_octet_t
    // pfcp_tlv_steering_functionality_t steering_functionality;												PFCP_IE_STEERING_FUNCTIONALITY_TYPE pfcp_tlv_octet_t
    // pfcp_tlv_steering_mode_t steering_mode;																	PFCP_IE_STEERING_MODE_TYPE 	pfcp_tlv_octet_t
    // pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
    // pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_MAR_ID_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->mar_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_STEERING_FUNCTIONALITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->steering_functionality, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_STEERING_MODE_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->steering_mode, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE:
				__up_pfcp_decode__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE:
				__up_pfcp_decode__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2, &buffer[bPos + 4], ELen);
				break;	
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__remove_mar(pfcp_tlv_remove_mar_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE		pfcp_tlv_octet_t

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_MAR_ID_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->mar_id, &buffer[bPos + 4], ELen);
				break;			
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}

int __up_pfcp_decode__update_mar(pfcp_tlv_update_mar_t * mObj, uint8_t * buffer, uint16_t len)
{
    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_mar_id_t mar_id;																		PFCP_IE_MAR_ID_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_steering_functionality_t steering_functionality;										PFCP_IE_STEERING_FUNCTIONALITY_TYPE			pfcp_tlv_octet_t	
    // pfcp_tlv_steering_mode_t steering_mode;															PFCP_IE_STEERING_MODE_TYPE			pfcp_tlv_octet_t
    // pfcp_tlv_update_access_forwarding_action_information_1_t update_access_forwarding_action_information_1;
    // pfcp_tlv_update_access_forwarding_action_information_2_t update_access_forwarding_action_information_2;
    // pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
    // pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;

	mObj->presence = 1;
	
	uint16_t bPos = 0;
	
	uint16_t EType = 0;
	uint16_t ELen = 0;
	
	while( bPos < len && (len-bPos) >= 4)
	{
		EType = app_ep__get_u16( &buffer[ bPos    ]);
		ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
	
		switch( EType)
		{
			case PFCP_IE_MAR_ID_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->mar_id, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_STEERING_FUNCTIONALITY_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->steering_functionality, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_STEERING_MODE_TYPE:
				__up_pfcp_decode__tlv_oct( &mObj->steering_mode, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE:
				__up_pfcp_decode__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE:
				__up_pfcp_decode__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE:
				__up_pfcp_decode__update_access_forwarding_action_information_1( &mObj->update_access_forwarding_action_information_1, &buffer[bPos + 4], ELen);
				break;
			case PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE:
				__up_pfcp_decode__update_access_forwarding_action_information_2( &mObj->update_access_forwarding_action_information_2, &buffer[bPos + 4], ELen);
				break;				
			default:
				break;
		}
	
		bPos += (4 + ELen);
	}
	
	return 0;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////
PFCPFlags * __up_pfcp_decode__header_get_flags( uint8_t * buffer)
{
	return (PFCPFlags *)&buffer[0];
}

uint8_t __up_pfcp_decode__header_get_msgtype( uint8_t * buffer)
{
	return buffer[1] & 0xFF;
}

uint16_t __up_pfcp_decode__header_get_length( uint8_t * buffer)
{
	return app_ep__get_u16( &buffer[2]);
}

uint8_t __up_pfcp_decode__header_has_seid( uint8_t * buffer)
{
	PFCPFlags * flags = (PFCPFlags *)&buffer[0];
	return flags->SEID;
}

uint64_t __up_pfcp_decode__header_get_seid( uint8_t * buffer)
{
	return app_ep__get_u64( &buffer[4]);
}

uint32_t __up_pfcp_decode__header_get_sequence( uint8_t * buffer)
{
	PFCPFlags * flags = (PFCPFlags *)&buffer[0];
	if( flags->SEID == 0) {
		return app_ep__get_u24( &buffer[4]);
	} else {
		return app_ep__get_u24( &buffer[4+8]);
	}
}

int __up_pfcp_decode__header( app_ep_udp_message_t * msg, pfcp_message_t * mObj)
{
	if(!msg)
		return -3;
	
	uint16_t len = app_ep__get_len(msg);
	
	if( len < 8)
		return -1;
	
	uint8_t * buffer = app_ep__get_buffer(msg);

	if(!buffer)
		return -4;

	memset( mObj, 0, sizeof(pfcp_message_t));

	mObj->SEID = 0;
	mObj->hasSEID = __up_pfcp_decode__header_has_seid( buffer);
	
	if( mObj->hasSEID == 1)
	{
		mObj->SEID = __up_pfcp_decode__header_get_seid( buffer);
	}
	
	mObj->SequenceNumber = __up_pfcp_decode__header_get_sequence( buffer);
	mObj->length = __up_pfcp_decode__header_get_length( buffer);
	mObj->MsgType = buffer[1] & 0xFF;
	mObj->header.MessageType = mObj->MsgType;
	
	// printf( "HAS-SEID=%u MsgType=%u length=%u SequenceNumber=%u SEID=%lu  %s|%d\n", 
		// mObj->hasSEID, mObj->MsgType, mObj->length, mObj->SequenceNumber, mObj->SEID, __FILE__, __LINE__);
		
	if( mObj->length != (len-4))
		return -2;
	
	if( mObj->hasSEID == 1)
		return 16;

	return 8;
}


int __up_pfcp_decode__heartbeat_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		//int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_RECOVERY_TIME_STAMP:
					__up_pfcp_decode__tlv_u32( &mObj->u.heartbeat_request.recovery_time_stamp, &buffer[ bPos + 2]);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}

int __up_pfcp_decode__heartbeat_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		//int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_RECOVERY_TIME_STAMP:
					__up_pfcp_decode__tlv_u32( &mObj->u.heartbeat_response.recovery_time_stamp, &buffer[ bPos + 2]);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}

int __up_pfcp_decode__pfd_management_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
	//	int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				//__up_pfcp_encode__application_id_s_pfds( &mObj->u.pfd_management_request.application_id_s_pfds, bObj);
				//pfcp_tlv_application_id_s_pfds_t application_id_s_pfds;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}

int __up_pfcp_decode__pfd_management_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		//int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				//pfcp_tlv_offending_ie_t offending_ie;				PFCP_IE_OFFENDING_IE	pfcp_tlv_uint16_t
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_setup_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_UP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.up_function_features, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION:
					if( user_plane_ip_resource_information_counter < 4)
					{
						__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.user_plane_ip_resource_information[user_plane_ip_resource_information_counter], &buffer[ bPos + 4], ELen);
						user_plane_ip_resource_information_counter++;
					}
					break;
				case PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.ue_ip_address_pool_identity, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.alternative_smf_ip_address, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_SMF_SET_ID_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_request.smf_set_id, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_setup_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_setup_response.cause, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_RECOVERY_TIME_STAMP:
					__up_pfcp_decode__tlv_u32( &mObj->u.association_setup_response.recovery_time_stamp, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_CP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_setup_response.cp_function_features, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_UP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_response.up_function_features, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION:
					if( user_plane_ip_resource_information_counter < 4)
					{
						__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_response.user_plane_ip_resource_information[user_plane_ip_resource_information_counter], &buffer[ bPos + 4], ELen);
						user_plane_ip_resource_information_counter++;
					}
					break;
				case PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_setup_response.alternative_smf_ip_address, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_update_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_UP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.up_function_features, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_update_request.cp_function_features, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.pfcp_association_release_request, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_GRACEFUL_RELEASE_PERIOD:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.graceful_release_period, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION:
					if( user_plane_ip_resource_information_counter < 4)
					{
						__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.user_plane_ip_resource_information[user_plane_ip_resource_information_counter], &buffer[ bPos + 4], ELen);
						user_plane_ip_resource_information_counter++;
					}
					break;
				case PFCP_IE_PFCPAUREQ_FLAGS_TYPE:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_update_request.pfcpaureq_flags, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_request.alternative_smf_ip_address, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_update_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_update_response.cause, &buffer[ bPos + 4]);
					break;	
				case PFCP_IE_UP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_update_response.up_function_features, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CP_FUNCTION_FEATURES:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_update_response.cp_function_features, &buffer[ bPos + 4]);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_release_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_release_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__association_release_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.association_release_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.association_release_response.cause, &buffer[ bPos + 4]);
					break;	
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__version_not_supported_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__node_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.node_report_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_NODE_REPORT_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.node_report_request.node_report_type, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT:
					__up_pfcp_decode__user_plane_path_failure_report( &mObj->u.node_report_request.user_plane_path_failure_report, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__node_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		//int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.node_report_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.node_report_response.cause, &buffer[ bPos + 4]);
					break;	
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.node_report_response.offending_ie, &buffer[ bPos + 4]);
					break;	
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_set_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_FQ_CSID:
					if( mObj->u.session_set_deletion_request.sgw_c_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.sgw_c_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.pgw_c_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.pgw_c_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.sgw_u_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.sgw_u_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.pgw_u_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.pgw_u_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.twan_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.twan_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.epdg_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.epdg_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_set_deletion_request.mme_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_request.mme_fq_csid, &buffer[ bPos + 4], ELen);
					}
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_set_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_set_deletion_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_set_deletion_response.cause, &buffer[ bPos + 4]);
					break;	
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.session_set_deletion_response.offending_ie, &buffer[ bPos + 4]);
					break;	
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_establishment_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		
		int icreate_pdr = 0;
		int icreate_far = 0;
		int icreate_urr = 0;
		int icreate_qer = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_F_SEID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.cp_f_seid, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_PDR:
					if( icreate_pdr < 8)
					{
						__up_pfcp_decode__create_pdr( &mObj->u.session_establishment_request.create_pdr[icreate_pdr], &buffer[ bPos + 4], ELen);
						icreate_pdr++;
					}
					break;
				case PFCP_IE_CREATE_FAR:
					if( icreate_far < 2)
					{
						__up_pfcp_decode__create_far( &mObj->u.session_establishment_request.create_far[icreate_far], &buffer[ bPos + 4], ELen);
						icreate_far++;
					}
					break;
				case PFCP_IE_CREATE_URR:
					if( icreate_urr < 10)
					{
						__up_pfcp_decode__create_urr( &mObj->u.session_establishment_request.create_urr[icreate_urr], &buffer[ bPos + 4], ELen);
						icreate_urr++;
					}
					break;
				case PFCP_IE_CREATE_QER:
					if( icreate_qer < 2)
					{
						__up_pfcp_decode__create_qer( &mObj->u.session_establishment_request.create_qer[icreate_qer], &buffer[ bPos + 4], ELen);
						icreate_qer++;
					}
					break;
				case PFCP_IE_CREATE_BAR:
					__up_pfcp_decode__create_bar( &mObj->u.session_establishment_request.create_bar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__create_traffic_endpoint( &mObj->u.session_establishment_request.create_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_PDN_TYPE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_establishment_request.pdn_type, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_FQ_CSID:
					{
						if( mObj->u.session_establishment_request.sgw_c_fq_csid.presence == 0) {
							__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.sgw_c_fq_csid, &buffer[ bPos + 4], ELen);
						} else if( mObj->u.session_establishment_request.mme_fq_csid.presence == 0) {
							__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.mme_fq_csid, &buffer[ bPos + 4], ELen);
						} else if( mObj->u.session_establishment_request.pgw_c_fq_csid.presence == 0) {
							__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.pgw_c_fq_csid, &buffer[ bPos + 4], ELen);
						} else if( mObj->u.session_establishment_request.epdg_fq_csid.presence == 0) {
							__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.epdg_fq_csid, &buffer[ bPos + 4], ELen);
						} else if( mObj->u.session_establishment_request.twan_fq_csid.presence == 0) {
							__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.twan_fq_csid, &buffer[ bPos + 4], ELen);
						}	
					}
					break;
				case PFCP_IE_USER_PLANE_INACTIVITY_TIMER:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.user_plane_inactivity_timer, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USER_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.user_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_TRACE_INFORMATION:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.trace_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_APN_DNN_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_request.apn_dnn, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_MAR_TYPE:
					__up_pfcp_decode__create_mar( &mObj->u.session_establishment_request.create_mar, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_establishment_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int icreated_pdr = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_response.node_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_establishment_response.cause, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.session_establishment_response.offending_ie, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_F_SEID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_response.up_f_seid, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATED_PDR:
					if( icreated_pdr < 8 ) {
						__up_pfcp_decode__created_pdr( &mObj->u.session_establishment_response.created_pdr[icreated_pdr], &buffer[ bPos + 4], ELen);
						icreated_pdr++;
					}
					break;
				case PFCP_IE_LOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__load_control_information( &mObj->u.session_establishment_response.load_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_OVERLOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__overload_control_information( &mObj->u.session_establishment_response.overload_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_FQ_CSID:
					if( mObj->u.session_establishment_response.sgw_u_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_response.sgw_u_fq_csid, &buffer[ bPos + 4], ELen);
					} else if( mObj->u.session_establishment_response.pgw_u_fq_csid.presence == 0) {
						__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_response.pgw_u_fq_csid, &buffer[ bPos + 4], ELen);
					}
					break;
				case PFCP_IE_FAILED_RULE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_establishment_response.failed_rule_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATED_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__created_traffic_endpoint( &mObj->u.session_establishment_response.created_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;				
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_modification_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int iremove_pdr = 0;
		int iremove_far = 0;
		int iremove_urr = 0;
		int iremove_qer = 0;
		int icreate_pdr = 0;
		int icreate_far = 0;
		int icreate_urr = 0;
		int icreate_qer = 0;
		int iupdate_pdr = 0;
		int iupdate_far = 0;
		int iupdate_urr = 0;
		int iupdate_qer = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_F_SEID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.cp_f_seid, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_REMOVE_PDR:
					if( iremove_pdr < 8)
					{
						__up_pfcp_decode__remove_pdr( &mObj->u.session_modification_request.remove_pdr[iremove_pdr], &buffer[ bPos + 4], ELen);
						iremove_pdr++;
					}
					break;
				case PFCP_IE_REMOVE_FAR:
					if( iremove_far < 8)
					{
						__up_pfcp_decode__remove_far( &mObj->u.session_modification_request.remove_far[iremove_far], &buffer[ bPos + 4], ELen);
						iremove_far++;
					}
					break;
				case PFCP_IE_REMOVE_URR:
					if( iremove_urr < 10)
					{
						__up_pfcp_decode__remove_urr( &mObj->u.session_modification_request.remove_urr[iremove_urr], &buffer[ bPos + 4], ELen);
						iremove_urr++;
					}					
					break;
				case PFCP_IE_REMOVE_QER:
					if( iremove_qer < 2)
					{
						__up_pfcp_decode__remove_qer( &mObj->u.session_modification_request.remove_qer[iremove_qer], &buffer[ bPos + 4], ELen);
						iremove_qer++;
					}
					break;
				case PFCP_IE_REMOVE_BAR:
					__up_pfcp_decode__remove_bar( &mObj->u.session_modification_request.remove_bar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_REMOVE_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__remove_traffic_endpoint( &mObj->u.session_modification_request.remove_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_PDR:
					if( icreate_pdr < 8)
					{
						__up_pfcp_decode__create_pdr( &mObj->u.session_modification_request.create_pdr[icreate_pdr], &buffer[ bPos + 4], ELen);
						icreate_pdr++;
					}
					break;
				case PFCP_IE_CREATE_FAR:
					if( icreate_far < 8)
					{
						__up_pfcp_decode__create_far( &mObj->u.session_modification_request.create_far[icreate_far], &buffer[ bPos + 4], ELen);
						icreate_far++;
					}
					break;
				case PFCP_IE_CREATE_URR:
					if( icreate_urr < 10)
					{
						__up_pfcp_decode__create_urr( &mObj->u.session_modification_request.create_urr[icreate_urr], &buffer[ bPos + 4], ELen);
						icreate_urr++;
					}
					break;
				case PFCP_IE_CREATE_QER:
					if( icreate_qer < 4)
					{
						__up_pfcp_decode__create_qer( &mObj->u.session_modification_request.create_qer[icreate_qer], &buffer[ bPos + 4], ELen);
						icreate_qer++;
					}
					break;
				case PFCP_IE_CREATE_BAR:
					__up_pfcp_decode__create_bar( &mObj->u.session_modification_request.create_bar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__create_traffic_endpoint( &mObj->u.session_modification_request.create_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_UPDATE_PDR:
					if( iupdate_pdr < 8)
					{
						__up_pfcp_decode__update_pdr( &mObj->u.session_modification_request.update_pdr[iupdate_pdr], &buffer[ bPos + 4], ELen);
						iupdate_pdr++;
					}
					break;
				case PFCP_IE_UPDATE_FAR:
					if( iupdate_far < 8)
					{
						__up_pfcp_decode__update_far( &mObj->u.session_modification_request.update_far[iupdate_far], &buffer[ bPos + 4], ELen);
						iupdate_far++;
					}				
					break;
				case PFCP_IE_UPDATE_URR:
					if( iupdate_urr < 10)
					{
						__up_pfcp_decode__update_urr( &mObj->u.session_modification_request.update_urr[iupdate_urr], &buffer[ bPos + 4], ELen);
						iupdate_urr++;
					}					
					break;
				case PFCP_IE_UPDATE_QER:
					if( iupdate_qer < 4)
					{
						__up_pfcp_decode__update_qer( &mObj->u.session_modification_request.update_qer[iupdate_qer], &buffer[ bPos + 4], ELen);
						iupdate_qer++;
					}	
					break;
				case PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST:
					__up_pfcp_decode__update_bar_session_modification_request( &mObj->u.session_modification_request.update_bar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_UPDATE_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.update_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_PFCPSMREQ_FLAGS:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_modification_request.pfcpsmreq_flags, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_QUERY_URR:
					__up_pfcp_decode__query_urr( &mObj->u.session_modification_request.query_urr, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_FQ_CSID:
					{
						if( mObj->u.session_modification_request.pgw_c_fq_csid.presence == 0)
						{
							__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.pgw_c_fq_csid, &buffer[ bPos + 4], ELen);
						} 
						else if( mObj->u.session_modification_request.sgw_c_fq_csid.presence == 0)
						{
							__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.sgw_c_fq_csid, &buffer[ bPos + 4], ELen);
						} 
						else if( mObj->u.session_modification_request.mme_fq_csid.presence == 0)
						{
							__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.mme_fq_csid, &buffer[ bPos + 4], ELen);
						}
						else if( mObj->u.session_modification_request.epdg_fq_csid.presence == 0)
						{
							__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.epdg_fq_csid, &buffer[ bPos + 4], ELen);
						}
						else if( mObj->u.session_modification_request.twan_fq_csid.presence == 0)
						{
							__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.twan_fq_csid, &buffer[ bPos + 4], ELen);
						}
					}
					break;
				case PFCP_IE_USER_PLANE_INACTIVITY_TIMER:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.user_plane_inactivity_timer, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_QUERY_URR_REFERENCE:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.query_urr_reference, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_TRACE_INFORMATION:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.trace_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_REMOVE_MAR_TYPE:
					__up_pfcp_decode__remove_mar( &mObj->u.session_modification_request.remove_mar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_UPDATE_MAR_TYPE:
					__up_pfcp_decode__update_mar( &mObj->u.session_modification_request.update_mar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATE_MAR_TYPE:
					__up_pfcp_decode__create_mar( &mObj->u.session_modification_request.create_mar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_NODE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_request.node_id, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_modification_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int icreated_pdr = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_modification_response.cause, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.session_modification_response.offending_ie, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_CREATED_PDR:
					if( icreated_pdr < 8)
					{
						__up_pfcp_decode__created_pdr( &mObj->u.session_modification_response.created_pdr[icreated_pdr], &buffer[ bPos + 4], ELen);
						icreated_pdr++;
					}
					break;
				case PFCP_IE_LOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__load_control_information( &mObj->u.session_modification_response.load_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_OVERLOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__overload_control_information( &mObj->u.session_modification_response.overload_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE:
					__up_pfcp_decode__usage_report_session_modification_response( &mObj->u.session_modification_response.usage_report, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_FAILED_RULE_ID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_response.failed_rule_id, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_modification_response.additional_usage_reports_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_CREATED_TRAFFIC_ENDPOINT:
					__up_pfcp_decode__created_traffic_endpoint( &mObj->u.session_modification_response.created_updated_traffic_endpoint, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_deletion_response.cause, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.session_deletion_response.offending_ie, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_LOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__load_control_information( &mObj->u.session_deletion_response.load_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_OVERLOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__overload_control_information( &mObj->u.session_deletion_response.overload_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE:
					__up_pfcp_decode__usage_report_session_deletion_response( &mObj->u.session_deletion_response.usage_report, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_REPORT_TYPE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_report_request.report_type, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_DOWNLINK_DATA_REPORT:
					__up_pfcp_decode__downlink_data_report( &mObj->u.session_report_request.downlink_data_report, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST:
					__up_pfcp_decode__usage_report_session_report_request( &mObj->u.session_report_request.usage_report, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_ERROR_INDICATION_REPORT:
					__up_pfcp_decode__error_indication_report( &mObj->u.session_report_request.error_indication_report, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_LOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__load_control_information( &mObj->u.session_report_request.load_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_OVERLOAD_CONTROL_INFORMATION:
					__up_pfcp_decode__overload_control_information( &mObj->u.session_report_request.overload_control_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_report_request.additional_usage_reports_information, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_PFCPSRREQ_FLAGS_TYPE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_report_request.pfcpsrreq_flags, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_F_SEID:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_report_request.old_cp_f_seid, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__session_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	int bPos = 	__up_pfcp_decode__header( bObj, mObj);
	if( bPos > 0)
	{
		uint8_t * buffer = app_ep__get_buffer( bObj);
		uint16_t len = app_ep__get_len( bObj);
		
		uint16_t EType = 0;
		uint16_t ELen = 0;
		int user_plane_ip_resource_information_counter = 0;
		
		while( bPos < len && (len-bPos) >= 4)
		{
			EType = app_ep__get_u16( &buffer[ bPos    ]);
			ELen  = app_ep__get_u16( &buffer[ bPos + 2]);
			
			switch( EType)
			{
				case PFCP_IE_CAUSE:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_report_response.cause, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_OFFENDING_IE:
					__up_pfcp_decode__tlv_u16( &mObj->u.session_report_response.offending_ie, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE:
					__up_pfcp_decode__update_bar( &mObj->u.session_report_response.update_bar, &buffer[ bPos + 4], ELen);
					break;
				case PFCP_IE_PFCPSRRSP_FLAGS:
					__up_pfcp_decode__tlv_u8( &mObj->u.session_report_response.pfcpsrrsp_flags, &buffer[ bPos + 4]);
					break;
				case PFCP_IE_F_SEID:
					if( mObj->u.session_report_response.cp_f_seid.presence == 0)
					{
						__up_pfcp_decode__tlv_oct( &mObj->u.session_report_response.cp_f_seid, &buffer[ bPos + 4], ELen);
					} 
					else if( mObj->u.session_report_response.n4_u_f_teid.presence == 0)
					{	
						__up_pfcp_decode__tlv_oct( &mObj->u.session_report_response.n4_u_f_teid, &buffer[ bPos + 4], ELen);
					}
					break;
				case PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE:
					__up_pfcp_decode__tlv_oct( &mObj->u.session_report_response.alternative_smf_ip_address, &buffer[ bPos + 4], ELen);
					break;
				default:
					break;
			}
			
			bPos += (4 + ELen);
			// printf("EType=%u ELen=%u bPos=%d len=%u left=%d  %s|%d\n", EType, ELen, bPos, mObj->length, (mObj->length-bPos), __FILE__, __LINE__);			
		}
		return 0;
	}
	return bPos;
}


int __up_pfcp_decode__message( uint8_t MessageType, pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	switch( MessageType)
	{
		case PFCP_HEARTBEAT_REQUEST:
			return __up_pfcp_decode__heartbeat_request( mObj, bObj);
		case PFCP_HEARTBEAT_RESPONSE:
			return __up_pfcp_decode__heartbeat_response( mObj, bObj);
		case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
			return __up_pfcp_decode__pfd_management_request( mObj, bObj);
		case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
			return __up_pfcp_decode__pfd_management_response( mObj, bObj);
		case PFCP_ASSOCIATION_SETUP_REQUEST:
			return __up_pfcp_decode__association_setup_request( mObj, bObj);
		case PFCP_ASSOCIATION_SETUP_RESPONSE:
			return __up_pfcp_decode__association_setup_response( mObj, bObj);
		case PFCP_ASSOCIATION_UPDATE_REQUEST:
			return __up_pfcp_decode__association_update_request( mObj, bObj);
		case PFCP_ASSOCIATION_UPDATE_RESPONSE:
			return __up_pfcp_decode__association_update_response( mObj, bObj);
		case PFCP_ASSOCIATION_RELEASE_REQUEST:
			return __up_pfcp_decode__association_release_request( mObj, bObj);
		case PFCP_ASSOCIATION_RELEASE_RESPONSE:
			return __up_pfcp_decode__association_release_response( mObj, bObj);
		case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
			return __up_pfcp_decode__version_not_supported_response( mObj, bObj);
		case PFCP_NODE_REPORT_REQUEST:
			return __up_pfcp_decode__node_report_request( mObj, bObj);
		case PFCP_NODE_REPORT_RESPONSE:
			return __up_pfcp_decode__node_report_response( mObj, bObj);
		case PFCP_SESSION_SET_DELETION_REQUEST:
			return __up_pfcp_decode__session_set_deletion_request( mObj, bObj);
		case PFCP_SESSION_SET_DELETION_RESPONSE:
			return __up_pfcp_decode__session_set_deletion_response( mObj, bObj);
		case PFCP_SESSION_ESTABLISHMENT_REQUEST:
			return __up_pfcp_decode__session_establishment_request( mObj, bObj);
		case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
			return __up_pfcp_decode__session_establishment_response( mObj, bObj);
		case PFCP_SESSION_MODIFICATION_REQUEST:
			return __up_pfcp_decode__session_modification_request( mObj, bObj);
		case PFCP_SESSION_MODIFICATION_RESPONSE:
			return __up_pfcp_decode__session_modification_response( mObj, bObj);
		case PFCP_SESSION_DELETION_REQUEST:
			return __up_pfcp_decode__session_deletion_request( mObj, bObj);
		case PFCP_SESSION_DELETION_RESPONSE:
			return __up_pfcp_decode__session_deletion_response( mObj, bObj);
		case PFCP_SESSION_REPORT_REQUEST:
			return __up_pfcp_decode__session_report_request( mObj, bObj);
		case PFCP_SESSION_REPORT_RESPONSE:
			return __up_pfcp_decode__session_report_response( mObj, bObj);
		default:
			return -1;
	}
	return -1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////











int __up_pfcp_encode__ethernet_packet_filter(pfcp_tlv_ethernet_packet_filter_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_ETHERNET_PACKET_FILTER);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_ethernet_filter_id_t ethernet_filter_id;					//PFCP_IE_ETHERNET_FILTER_ID			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_FILTER_ID, &mObj->ethernet_filter_id, bObj);
		
		// pfcp_tlv_ethernet_filter_properties_t ethernet_filter_properties;	//PFCP_IE_ETHERNET_FILTER_PROPERTIES	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_FILTER_PROPERTIES, &mObj->ethernet_filter_properties, bObj);
		
		// pfcp_tlv_mac_address_t mac_address;									//PFCP_IE_MAC_ADDRESS					pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAC_ADDRESS, &mObj->mac_address, bObj);
		
		// pfcp_tlv_ethertype_t ethertype;										//PFCP_IE_ETHERTYPE						pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERTYPE, &mObj->ethertype, bObj);
		
		// pfcp_tlv_c_tag_t c_tag;												//PFCP_IE_C_TAG							pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_C_TAG, &mObj->c_tag, bObj);
		
		// pfcp_tlv_s_tag_t s_tag;												//PFCP_IE_S_TAG							pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_S_TAG, &mObj->s_tag, bObj);
		
		// pfcp_tlv_sdf_filter_t sdf_filter[8];									//PFCP_IE_SDF_FILTER					pfcp_tlv_octet_t
		
		int i = 0;
		for( i = 0; i < 8; i++)
		{
			__up_pfcp_encode__tlv_oct( PFCP_IE_SDF_FILTER, &mObj->sdf_filter[i], bObj);
		}
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	
	return 0;
}



   
    
    
    
    

	
int __up_pfcp_encode__pdi( pfcp_tlv_pdi_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;

		app_ep__encode__u16( bObj, PFCP_IE_PDI);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_source_interface_t source_interface;									PFCP_IE_SOURCE_INTERFACE	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_SOURCE_INTERFACE, &mObj->source_interface, bObj);
		
		// pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &mObj->local_f_teid, bObj);
		
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_NETWORK_INSTANCE, &mObj->network_instance, bObj);
		
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UE_IP_ADDRESS, &mObj->ue_ip_address, bObj);
		
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;								PFCP_IE_TRAFFIC_ENDPOINT_ID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->traffic_endpoint_id, bObj);
		
		// pfcp_tlv_sdf_filter_t sdf_filter[8];												PFCP_IE_SDF_FILTER			pfcp_tlv_octet_t
		int i = 0;
		for( i = 0; i < 8; i++)
		{
			__up_pfcp_encode__tlv_oct( PFCP_IE_SDF_FILTER, &mObj->sdf_filter[i], bObj);
		}
	
		// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_APPLICATION_ID, &mObj->application_id, bObj);
		
		// pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;	PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION, &mObj->ethernet_pdu_session_information, bObj);
		
		// pfcp_tlv_ethernet_packet_filter_t ethernet_packet_filter;						PFCP_IE_ETHERNET_PACKET_FILTER
		__up_pfcp_encode__ethernet_packet_filter( &mObj->ethernet_packet_filter, bObj);
		
		// pfcp_tlv_qfi_t qfi;																PFCP_IE_QFI					pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_QFI, &mObj->qfi, bObj);
		
		// pfcp_tlv_framed_route_t framed_route;											PFCP_IE_FRAMED_ROUTE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_ROUTE, &mObj->framed_route, bObj);
		
		// pfcp_tlv_framed_routing_t framed_routing;										PFCP_IE_FRAMED_ROUTING		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_ROUTING, &mObj->framed_routing, bObj);
		
		// pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;									PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_IPV6_ROUTE, &mObj->framed_ipv6_route, bObj);
		
		// pfcp_tlv__interface_type_t source_interface_type;								PFCP_IE_INTERFACE_TYPE_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_INTERFACE_TYPE_TYPE, &mObj->source_interface_type, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

    // pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t		
    // pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE					pfcp_tlv_uint32_t		
    // pfcp_tlv_pdi_t pdi;
    // pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID						pfcp_tlv_uint32_t
    // pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID						pfcp_tlv_uint32_t
    // pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID						pfcp_tlv_uint32_t
														
int __up_pfcp_encode__create_pdr(pfcp_tlv_create_pdr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_PDR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		__up_pfcp_encode__tlv_uint16( PFCP_IE_PACKET_DETECTION_RULE_ID, &mObj->pdr_id, bObj);
		__up_pfcp_encode__tlv_uint32( PFCP_IE_PRECEDENCE, &mObj->precedence, bObj);
		__up_pfcp_encode__pdi( &mObj->pdi, bObj);
		
		// pfcp_tlv_outer_header_removal_t outer_header_removal;		//pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_REMOVAL, &mObj->outer_header_removal, bObj);
		
		// pfcp_tlv_far_id_t far_id;									//pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;									//pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_qer_id_t qer_id;									//pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_ID, &mObj->qer_id, bObj);
		
		// pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ACTIVATE_PREDEFINED_RULES, &mObj->activate_predefined_rules, bObj);
		
		// pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ACTIVATION_TIME_TYPE, &mObj->activation_time, bObj);
		
		// pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DEACTIVATION_TIME_TYPE, &mObj->deactivation_time, bObj);
		
		// pfcp_tlv_mar_id_t mar_id;														PFCP_IE_MAR_ID_TYPE					pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAR_ID_TYPE, &mObj->mar_id, bObj);
		
		// pfcp_tlv_packet_replication_and_detection_carry_on_information_t packet_replication_and_detection_carry_on_information;	
															//PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE, &mObj->packet_replication_and_detection_carry_on_information, bObj);
	
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
	


int __up_pfcp_encode__forwarding_parameters(pfcp_tlv_forwarding_parameters_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_FORWARDING_PARAMETERS);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_destination_interface_t destination_interface;			//pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_DESTINATION_INTERFACE, &mObj->destination_interface, bObj);
		
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_NETWORK_INSTANCE, &mObj->network_instance, bObj);
		
		// pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_REDIRECT_INFORMATION, &mObj->redirect_information, bObj);
		
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_CREATION, &mObj->outer_header_creation, bObj);
		
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRANSPORT_LEVEL_MARKING, &mObj->transport_level_marking, bObj);
		
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FORWARDING_POLICY, &mObj->forwarding_policy, bObj);
		
		// pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_HEADER_ENRICHMENT, &mObj->header_enrichment, bObj);
		
		// pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->linked_traffic_endpoint_id, bObj);
		
		// pfcp_tlv_proxying_t proxying;													PFCP_IE_PROXYING				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PROXYING, &mObj->proxying, bObj);
		
		// pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_INTERFACE_TYPE_TYPE, &mObj->destination_interface_type, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}


int __up_pfcp_encode__duplicating_parameters(pfcp_tlv_duplicating_parameters_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_DUPLICATING_PARAMETERS);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);

		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_DESTINATION_INTERFACE, &mObj->destination_interface, bObj);
		
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_CREATION, &mObj->outer_header_creation, bObj);
		
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRANSPORT_LEVEL_MARKING, &mObj->transport_level_marking, bObj);
		
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FORWARDING_POLICY, &mObj->forwarding_policy, bObj);

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
    // pfcp_tlv_forwarding_parameters_t forwarding_parameters;							PFCP_IE_UPDATE_FORWARDING_PARAMETERS
    // pfcp_tlv_duplicating_parameters_t duplicating_parameters;						PFCP_IE_UPDATE_DUPLICATING_PARAMETERS
    // pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t


int __up_pfcp_encode__create_far(pfcp_tlv_create_far_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_FAR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		
		// pfcp_tlv_far_id_t far_id;									//pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_apply_action_t apply_action;						//pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_APPLY_ACTION, &mObj->apply_action, bObj);
		
		// pfcp_tlv_forwarding_parameters_t forwarding_parameters;
		__up_pfcp_encode__forwarding_parameters( &mObj->forwarding_parameters, bObj);
		
		// pfcp_tlv_duplicating_parameters_t duplicating_parameters;
		__up_pfcp_encode__duplicating_parameters( &mObj->duplicating_parameters, bObj);
		
		// pfcp_tlv_bar_id_t bar_id;									//pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	
	return 0;
}

    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
    // pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
    // pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
    // pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
    // pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
    // pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
    // pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
    // pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;										PFCP_IE_PFCPSMREQ_FLAGS			pfcp_tlv_uint8_t
    // pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
    // pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t

int __up_pfcp_encode__update_forwarding_parameters( pfcp_tlv_update_forwarding_parameters_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_FORWARDING_PARAMETERS);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);

		// pfcp_tlv_destination_interface_t destination_interface;					// pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_DESTINATION_INTERFACE, &mObj->destination_interface, bObj);
		
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_NETWORK_INSTANCE, &mObj->network_instance, bObj);
		
		// pfcp_tlv_redirect_information_t redirect_information;							PFCP_IE_REDIRECT_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_REDIRECT_INFORMATION, &mObj->redirect_information, bObj);
		
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_CREATION, &mObj->outer_header_creation, bObj);
		
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRANSPORT_LEVEL_MARKING, &mObj->transport_level_marking, bObj);
		
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FORWARDING_POLICY, &mObj->forwarding_policy, bObj);
		
		// pfcp_tlv_header_enrichment_t header_enrichment;									PFCP_IE_HEADER_ENRICHMENT		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_HEADER_ENRICHMENT, &mObj->header_enrichment, bObj);
		
		// pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;										PFCP_IE_PFCPSMREQ_FLAGS			pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_PFCPSMREQ_FLAGS, &mObj->pfcpsmreq_flags, bObj);
		
		// pfcp_tlv_traffic_endpoint_id_t linked_traffic_endpoint_id;						PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->linked_traffic_endpoint_id, bObj);
		
		// pfcp_tlv__interface_type_t destination_interface_type;							PFCP_IE_INTERFACE_TYPE_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_INTERFACE_TYPE_TYPE, &mObj->destination_interface_type, bObj);



		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}		
	return 0;
}


int __up_pfcp_encode__update_duplicating_parameters(pfcp_tlv_update_duplicating_parameters_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_DUPLICATING_PARAMETERS);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);		
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_destination_interface_t destination_interface;							PFCP_IE_DESTINATION_INTERFACE 	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_DESTINATION_INTERFACE, &mObj->destination_interface, bObj);
		
		// pfcp_tlv_outer_header_creation_t outer_header_creation;							PFCP_IE_OUTER_HEADER_CREATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_CREATION, &mObj->outer_header_creation, bObj);
		
		// pfcp_tlv_transport_level_marking_t transport_level_marking;						PFCP_IE_TRANSPORT_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRANSPORT_LEVEL_MARKING, &mObj->transport_level_marking, bObj);
		
		// pfcp_tlv_forwarding_policy_t forwarding_policy;									PFCP_IE_FORWARDING_POLICY		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FORWARDING_POLICY, &mObj->forwarding_policy, bObj);

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}


    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
    // pfcp_tlv_apply_action_t apply_action;											PFCP_IE_APPLY_ACTION		pfcp_tlv_uint8_t
    // pfcp_tlv_update_forwarding_parameters_t update_forwarding_parameters;			PFCP_IE_UPDATE_FORWARDING_PARAMETERS
    // pfcp_tlv_update_duplicating_parameters_t update_duplicating_parameters;			PFCP_IE_UPDATE_DUPLICATING_PARAMETERS
    // pfcp_tlv_bar_id_t bar_id;														PFCP_IE_BAR_ID				pfcp_tlv_uint8_t
	
int __up_pfcp_encode__update_far(pfcp_tlv_update_far_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_FAR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;													// pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_apply_action_t apply_action;										// pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_APPLY_ACTION, &mObj->apply_action, bObj);
		
		// pfcp_tlv_update_forwarding_parameters_t update_forwarding_parameters;
		__up_pfcp_encode__update_forwarding_parameters( &mObj->update_forwarding_parameters, bObj);
		
		// pfcp_tlv_update_duplicating_parameters_t update_duplicating_parameters;
		__up_pfcp_encode__update_duplicating_parameters( &mObj->update_duplicating_parameters, bObj);
		
		// pfcp_tlv_bar_id_t bar_id;													// pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);		


		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

    // pfcp_tlv_presence_t presence;
    // pfcp_tlv_pfd_contents_t pfd_contents;											PFCP_IE_PFD_CONTENTS		pfcp_tlv_octet_t
	
int __up_pfcp_encode__pfd_context(pfcp_tlv_pfd_context_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		app_ep__encode__u16( bObj, PFCP_IE_PFD_CONTEXT);
		app_ep__encode__u16( bObj, mObj->pfd_contents.len + 4);
		__up_pfcp_encode__tlv_oct( PFCP_IE_PFD_CONTENTS, &mObj->pfd_contents, bObj);
	}
	return 0;
}

// pfcp_tlv_presence_t presence;
// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_IDS_PFDS	pfcp_tlv_octet_t
// pfcp_tlv_pfd_context_t pfd_context;												PFCP_IE_PFD_CONTEXT
int __up_pfcp_encode__application_id_s_pfds(pfcp_tlv_application_id_s_pfds_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_APPLICATION_IDS_PFDS);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		__up_pfcp_encode__tlv_oct( PFCP_IE_APPLICATION_ID, &mObj->application_id, bObj);			//pfcp_tlv_application_id_t
		__up_pfcp_encode__pfd_context( &mObj->pfd_context, bObj);									//pfcp_tlv_application_id_t

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__ethernet_traffic_information(pfcp_tlv_ethernet_traffic_information_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_ETHERNET_TRAFFIC_INFORMATION);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);

		// pfcp_tlv_mac_addresses_detected_t mac_addresses_detected;						PFCP_IE_MAC_ADDRESSES_DETECTED	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAC_ADDRESSES_DETECTED, &mObj->mac_addresses_detected, bObj);
		
		// pfcp_tlv_mac_addresses_removed_t mac_addresses_removed;							PFCP_IE_MAC_ADDRESSES_REMOVED	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAC_ADDRESSES_REMOVED, &mObj->mac_addresses_removed, bObj);

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__access_forwarding_action_information_1(pfcp_tlv_access_forwarding_action_information_1_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_WEIGHT_TYPE, &mObj->weight, bObj);
		
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PRIORITY_TYPE, &mObj->priority, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);

		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__access_forwarding_action_information_2(pfcp_tlv_access_forwarding_action_information_2_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_WEIGHT_TYPE, &mObj->weight, bObj);
		
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PRIORITY_TYPE, &mObj->priority, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_access_forwarding_action_information_1(pfcp_tlv_update_access_forwarding_action_information_1_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_WEIGHT_TYPE, &mObj->weight, bObj);
		
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PRIORITY_TYPE, &mObj->priority, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_access_forwarding_action_information_2(pfcp_tlv_update_access_forwarding_action_information_2_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_weight_t weight;														PFCP_IE_WEIGHT_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_WEIGHT_TYPE, &mObj->weight, bObj);
		
		// pfcp_tlv_priority_t priority;													PFCP_IE_PRIORITY_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PRIORITY_TYPE, &mObj->priority, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__create_urr(pfcp_tlv_create_urr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_URR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_MEASUREMENT_METHOD, &mObj->measurement_method, bObj);
		
		// pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_REPORTING_TRIGGERS, &mObj->reporting_triggers, bObj);
		
		// pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MEASUREMENT_PERIOD, &mObj->measurement_period, bObj);
		
		// pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_THRESHOLD, &mObj->volume_threshold, bObj);
		
		// pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_QUOTA, &mObj->volume_quota, bObj);
		
		// pfcp_tlv_event_threshold_t event_threshold;										PFCP_IE_EVENT_THRESHOLD		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_THRESHOLD, &mObj->event_threshold, bObj);
		
		// pfcp_tlv_event_quota_t event_quota;												PFCP_IE_EVENT_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_QUOTA, &mObj->event_quota, bObj);
		
		// pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_THRESHOLD, &mObj->time_threshold, bObj);
		
		// pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_QUOTA, &mObj->time_quota, bObj);
		
		// pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUOTA_HOLDING_TIME, &mObj->quota_holding_time, bObj);
		
		// pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD, &mObj->dropped_dl_traffic_threshold, bObj);
		
		// pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUOTA_VALIDITY_TIME_TYPE, &mObj->quota_validity_time, bObj);
		
		// pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MONITORING_TIME, &mObj->monitoring_time, bObj);
		
		// pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD, &mObj->subsequent_volume_threshold, bObj);
		
		// pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_TIME_THRESHOLD, &mObj->subsequent_time_threshold, bObj);
		
		// pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_VOLUME_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_VOLUME_QUOTA, &mObj->subsequent_volume_quota, bObj);
		
		// pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_TIME_QUOTA, &mObj->subsequent_time_quota, bObj);
		
		// pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_SUBS_EVENT_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBS_EVENT_THRESHOLD, &mObj->subsequent_event_threshold, bObj);
		
		// pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_SUBS_EVENT_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBS_EVENT_QUOTA, &mObj->subsequent_event_quota, bObj);
		
		// pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_INACTIVITY_DETECTION_TIME, &mObj->inactivity_detection_time, bObj);
		
		// pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_LINKED_URR_ID, &mObj->linked_urr_id, bObj);
		
		// pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MEASUREMENT_INFORMATION, &mObj->measurement_information, bObj);
		
		// pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_QUOTA_MECHANISM, &mObj->time_quota_mechanism, bObj);
		
		// pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_AGGREGATED_URRS, &mObj->aggregated_urrs, bObj);
		
		// pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id_for_quota_action, bObj);
		
		// pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_INACTIVITY_TIMER, &mObj->ethernet_inactivity_timer, bObj);
		
		// pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ADDITIONAL_MONITORING_TIME, &mObj->additional_monitoring_time, bObj);


		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}


	
int __up_pfcp_encode__create_qer(pfcp_tlv_create_qer_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_QER);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_ID, &mObj->qer_id, bObj);
		
		// pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_CORRELATION_ID, &mObj->qer_correlation_id, bObj);
		
		// pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_GATE_STATUS, &mObj->gate_status, bObj);
		
		// pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MBR, &mObj->maximum_bitrate, bObj);
		
		// pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_GBR, &mObj->guaranteed_bitrate, bObj);
		
		// pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PACKET_RATE, &mObj->packet_rate, bObj);
		
		// pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DL_FLOW_LEVEL_MARKING, &mObj->dl_flow_level_marking, bObj);
		
		// pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_QFI, &mObj->qos_flow_identifier, bObj);
		
		// pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_RQI, &mObj->reflective_qos, bObj);
		
		// pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t	
		__up_pfcp_encode__tlv_uint8( PFCP_IE_PAGING_POLICY_INDICATOR_TYPE, &mObj->paging_policy_indicator, bObj);
		
		// pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE	pfcp_tlv_uint32_t		
		__up_pfcp_encode__tlv_uint32( PFCP_IE_AVERAGING_WINDOW_TYPE, &mObj->averaging_window, bObj);

		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}	
	return 0;
}


	
int __up_pfcp_encode__created_pdr(pfcp_tlv_created_pdr_t * mObj, app_ep_udp_message_t * bObj)
{
    if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATED_PDR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
		__up_pfcp_encode__tlv_uint16( PFCP_IE_PACKET_DETECTION_RULE_ID, &mObj->pdr_id, bObj);

		// pfcp_tlv_f_teid_t local_f_teid;													PFCP_IE_F_TEID				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &mObj->local_f_teid, bObj);
		
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UE_IP_ADDRESS, &mObj->ue_ip_address, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

	
int __up_pfcp_encode__update_pdr(pfcp_tlv_update_pdr_t * mObj, app_ep_udp_message_t * bObj)
{
    if( mObj->presence == 1)
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_PDR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t
		__up_pfcp_encode__tlv_uint16( PFCP_IE_PACKET_DETECTION_RULE_ID, &mObj->pdr_id, bObj);
		
		// pfcp_tlv_outer_header_removal_t outer_header_removal;							PFCP_IE_OUTER_HEADER_REMOVAL		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OUTER_HEADER_REMOVAL, &mObj->outer_header_removal, bObj);
		
		// pfcp_tlv_precedence_t precedence;												PFCP_IE_PRECEDENCE		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_PRECEDENCE, &mObj->precedence, bObj);
		
		// pfcp_tlv_pdi_t pdi	;															PFCP_IE_PDI
		__up_pfcp_encode__pdi( &mObj->pdi, bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_ID, &mObj->qer_id, bObj);
		
		// pfcp_tlv_activate_predefined_rules_t activate_predefined_rules;					PFCP_IE_ACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ACTIVATE_PREDEFINED_RULES, &mObj->activate_predefined_rules, bObj);
		
		// pfcp_tlv_deactivate_predefined_rules_t deactivate_predefined_rules;				PFCP_IE_DEACTIVATE_PREDEFINED_RULES	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DEACTIVATE_PREDEFINED_RULES, &mObj->deactivate_predefined_rules, bObj);
		
		// pfcp_tlv_activation_time_t activation_time;										PFCP_IE_ACTIVATION_TIME_TYPE	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_ACTIVATION_TIME_TYPE, &mObj->activation_time, bObj);
		
		// pfcp_tlv_deactivation_time_t deactivation_time;									PFCP_IE_DEACTIVATION_TIME_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DEACTIVATION_TIME_TYPE, &mObj->deactivation_time, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_bar(pfcp_tlv_update_bar_pfcp_session_report_response_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_bar_id_t bar_id;																	PFCP_IE_BAR_ID		pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);
		
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;				PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY, &mObj->downlink_data_notification_delay, bObj);
		
		// pfcp_tlv_dl_buffering_duration_t dl_buffering_duration;										PFCP_IE_DL_BUFFERING_DURATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DL_BUFFERING_DURATION, &mObj->dl_buffering_duration, bObj);
		
		// pfcp_tlv_dl_buffering_suggested_packet_count_t dl_buffering_suggested_packet_count;			PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT, &mObj->dl_buffering_suggested_packet_count, bObj);
		
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;				PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT, &mObj->suggested_buffering_packets_count, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_urr(pfcp_tlv_update_urr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_URR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID				pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_measurement_method_t measurement_method;								PFCP_IE_MEASUREMENT_METHOD	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_MEASUREMENT_METHOD, &mObj->measurement_method, bObj);
		
		// pfcp_tlv_reporting_triggers_t reporting_triggers;								PFCP_IE_REPORTING_TRIGGERS	pfcp_tlv_uint8_t	
		__up_pfcp_encode__tlv_uint8( PFCP_IE_REPORTING_TRIGGERS, &mObj->reporting_triggers, bObj);
		
		// pfcp_tlv_measurement_period_t measurement_period;								PFCP_IE_MEASUREMENT_PERIOD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MEASUREMENT_PERIOD, &mObj->measurement_period, bObj);
		
		// pfcp_tlv_volume_threshold_t volume_threshold;									PFCP_IE_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_THRESHOLD, &mObj->volume_threshold, bObj);
		
		// pfcp_tlv_volume_quota_t volume_quota;											PFCP_IE_VOLUME_QUOTA		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_QUOTA, &mObj->volume_quota, bObj);
		
		// pfcp_tlv_time_threshold_t time_threshold;										PFCP_IE_TIME_THRESHOLD		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_THRESHOLD, &mObj->time_threshold, bObj);
		
		// pfcp_tlv_time_quota_t time_quota;												PFCP_IE_TIME_QUOTA			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_QUOTA, &mObj->time_quota, bObj);
		
		// pfcp_tlv_event_threshold_t event_threshold;										PFCP_IE_EVENT_THRESHOLD		pfcp_tlv_event_threshold_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_THRESHOLD, &mObj->event_threshold, bObj);
		
		// pfcp_tlv_event_quota_t event_quota;												PFCP_IE_EVENT_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_QUOTA, &mObj->event_quota, bObj);
		
		// pfcp_tlv_quota_holding_time_t quota_holding_time;								PFCP_IE_QUOTA_HOLDING_TIME	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUOTA_HOLDING_TIME, &mObj->quota_holding_time, bObj);
		
		// pfcp_tlv_dropped_dl_traffic_threshold_t dropped_dl_traffic_threshold;			PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD, &mObj->dropped_dl_traffic_threshold, bObj);
		
		// pfcp_tlv_quota_validity_time_t quota_validity_time;								PFCP_IE_QUOTA_VALIDITY_TIME_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUOTA_VALIDITY_TIME_TYPE, &mObj->quota_validity_time, bObj);
		
		// pfcp_tlv_monitoring_time_t monitoring_time;										PFCP_IE_MONITORING_TIME			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MONITORING_TIME, &mObj->monitoring_time, bObj);
		
		// pfcp_tlv_subsequent_volume_threshold_t subsequent_volume_threshold;				PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD, &mObj->subsequent_volume_threshold, bObj);
		
		// pfcp_tlv_subsequent_time_threshold_t subsequent_time_threshold;					PFCP_IE_SUBSEQUENT_TIME_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_TIME_THRESHOLD, &mObj->subsequent_time_threshold, bObj);
		
		// pfcp_tlv_subsequent_volume_quota_t subsequent_volume_quota;						PFCP_IE_SUBSEQUENT_VOLUME_QUOTA	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_VOLUME_QUOTA, &mObj->subsequent_volume_quota, bObj);
		
		// pfcp_tlv_subsequent_time_quota_t subsequent_time_quota;							PFCP_IE_SUBSEQUENT_TIME_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBSEQUENT_TIME_QUOTA, &mObj->subsequent_time_quota, bObj);
		
		// pfcp_tlv_subsequent_event_threshold_t subsequent_event_threshold;				PFCP_IE_EVENT_THRESHOLD	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_THRESHOLD, &mObj->subsequent_event_threshold, bObj);
		
		// pfcp_tlv_subsequent_event_quota_t subsequent_event_quota;						PFCP_IE_SUBS_EVENT_QUOTA	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUBS_EVENT_QUOTA, &mObj->subsequent_event_quota, bObj);
		
		// pfcp_tlv_inactivity_detection_time_t inactivity_detection_time;					PFCP_IE_INACTIVITY_DETECTION_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_INACTIVITY_DETECTION_TIME, &mObj->inactivity_detection_time, bObj);
		
		// pfcp_tlv_linked_urr_id_t linked_urr_id;											PFCP_IE_LINKED_URR_ID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_LINKED_URR_ID, &mObj->linked_urr_id, bObj);
		
		// pfcp_tlv_measurement_information_t measurement_information;						PFCP_IE_MEASUREMENT_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MEASUREMENT_INFORMATION, &mObj->measurement_information, bObj);
		
		// pfcp_tlv_time_quota_mechanism_t time_quota_mechanism;							PFCP_IE_TIME_QUOTA_MECHANISM	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_QUOTA_MECHANISM, &mObj->time_quota_mechanism, bObj);
		
		// pfcp_tlv_aggregated_urrs_t aggregated_urrs;										PFCP_IE_AGGREGATED_URRS	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_AGGREGATED_URRS, &mObj->aggregated_urrs, bObj);
		
		// pfcp_tlv_far_id_t far_id_for_quota_action;										PFCP_IE_FAR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id_for_quota_action, bObj);
		
		// pfcp_tlv_ethernet_inactivity_timer_t ethernet_inactivity_timer;					PFCP_IE_ETHERNET_INACTIVITY_TIMER	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_INACTIVITY_TIMER, &mObj->ethernet_inactivity_timer, bObj);
		
		// pfcp_tlv_additional_monitoring_time_t additional_monitoring_time;				PFCP_IE_ADDITIONAL_MONITORING_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ADDITIONAL_MONITORING_TIME, &mObj->additional_monitoring_time, bObj);


		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_qer(pfcp_tlv_update_qer_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_QER);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_ID, &mObj->qer_id, bObj);
		
		// pfcp_tlv_qer_correlation_id_t qer_correlation_id;								PFCP_IE_QER_CORRELATION_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_CORRELATION_ID, &mObj->qer_correlation_id, bObj);
		
		// pfcp_tlv_gate_status_t gate_status;												PFCP_IE_GATE_STATUS		pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_GATE_STATUS, &mObj->gate_status, bObj);
		
		// pfcp_tlv_mbr_t maximum_bitrate;													PFCP_IE_MBR		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MBR, &mObj->maximum_bitrate, bObj);
		
		// pfcp_tlv_gbr_t guaranteed_bitrate;												PFCP_IE_GBR		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_GBR, &mObj->guaranteed_bitrate, bObj);
		
		// pfcp_tlv_packet_rate_t packet_rate;												PFCP_IE_PACKET_RATE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_PACKET_RATE, &mObj->packet_rate, bObj);
		
		// pfcp_tlv_dl_flow_level_marking_t dl_flow_level_marking;							PFCP_IE_DL_FLOW_LEVEL_MARKING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DL_FLOW_LEVEL_MARKING, &mObj->dl_flow_level_marking, bObj);
		
		// pfcp_tlv_qfi_t qos_flow_identifier;												PFCP_IE_QFI		pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_QFI, &mObj->qos_flow_identifier, bObj);
		
		// pfcp_tlv_rqi_t reflective_qos;													PFCP_IE_RQI		pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_RQI, &mObj->reflective_qos, bObj);
		
		// pfcp_tlv_paging_policy_indicator_t paging_policy_indicator;						PFCP_IE_PAGING_POLICY_INDICATOR_TYPE	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_PAGING_POLICY_INDICATOR_TYPE, &mObj->paging_policy_indicator, bObj);
		
		// pfcp_tlv_averaging_window_t averaging_window;									PFCP_IE_AVERAGING_WINDOW_TYPE		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_AVERAGING_WINDOW_TYPE, &mObj->averaging_window, bObj);


		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_pdr(pfcp_tlv_remove_pdr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_PDR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_pdr_id_t pdr_id;														PFCP_IE_PACKET_DETECTION_RULE_ID	pfcp_tlv_uint16_t
		__up_pfcp_encode__tlv_uint16( PFCP_IE_PACKET_DETECTION_RULE_ID, &mObj->pdr_id, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_far(pfcp_tlv_remove_far_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_FAR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_far_id_t far_id;														PFCP_IE_FAR_ID		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_FAR_ID, &mObj->far_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_urr(pfcp_tlv_remove_urr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_URR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_qer(pfcp_tlv_remove_qer_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_QER);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_qer_id_t qer_id;														PFCP_IE_QER_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_QER_ID, &mObj->qer_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__load_control_information(pfcp_tlv_load_control_information_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_LOAD_CONTROL_INFORMATION);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_sequence_number_t load_control_sequence_number;							PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SEQUENCE_NUMBER, &mObj->load_control_sequence_number, bObj);
		
		// pfcp_tlv_metric_t load_metric;													PFCP_IE_METRIC				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_METRIC, &mObj->load_metric, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__overload_control_information(pfcp_tlv_overload_control_information_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_OVERLOAD_CONTROL_INFORMATION);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_sequence_number_t overload_control_sequence_number;						PFCP_IE_SEQUENCE_NUMBER		pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_SEQUENCE_NUMBER, &mObj->overload_control_sequence_number, bObj);
		
		// pfcp_tlv_metric_t overload_reduction_metric;										PFCP_IE_METRIC				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_METRIC, &mObj->overload_reduction_metric, bObj);
		
		// pfcp_tlv_timer_t period_of_validity;												PFCP_IE_TIMER				pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIMER, &mObj->period_of_validity, bObj);
		
		// pfcp_tlv_oci_flags_t overload_control_information_flags;							PFCP_IE_OCI_FLAGS			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_OCI_FLAGS, &mObj->overload_control_information_flags, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__application_detection_information(pfcp_tlv_application_detection_information_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_APPLICATION_DETECTION_INFORMATION);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		
		// pfcp_tlv_application_id_t application_id;										PFCP_IE_APPLICATION_ID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_APPLICATION_ID, &mObj->application_id, bObj);
		
		// pfcp_tlv_application_instance_id_t application_instance_id;						PFCP_IE_APPLICATION_INSTANCE_ID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_APPLICATION_INSTANCE_ID, &mObj->application_instance_id, bObj);
		
		// pfcp_tlv_flow_information_t flow_information;									PFCP_IE_FLOW_INFORMATION	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_FLOW_INFORMATION, &mObj->flow_information, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__query_urr(pfcp_tlv_query_urr_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_QUERY_URR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__usage_report_session_modification_response(pfcp_tlv_usage_report_session_modification_response_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UR_SEQN, &mObj->ur_seqn, bObj);
		
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_REPORT_TRIGGER, &mObj->usage_report_trigger, bObj);
		
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_START_TIME, &mObj->start_time, bObj);
		
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_END_TIME, &mObj->end_time, bObj);
		
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_MEASUREMENT, &mObj->volume_measurement, bObj);
		
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DURATION_MEASUREMENT, &mObj->duration_measurement, bObj);
		
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_FIRST_PACKET, &mObj->time_of_first_packet, bObj);
		
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_LAST_PACKET, &mObj->time_of_last_packet, bObj);
		
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_INFORMATION, &mObj->usage_information, bObj);
		
		// pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUERY_URR_REFERENCE, &mObj->query_urr_reference, bObj);
		
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
		__up_pfcp_encode__ethernet_traffic_information( &mObj->ethernet_traffic_information, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__usage_report_session_deletion_response(pfcp_tlv_usage_report_session_deletion_response_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID	pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UR_SEQN, &mObj->ur_seqn, bObj);
		
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_REPORT_TRIGGER, &mObj->usage_report_trigger, bObj);
		
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_START_TIME, &mObj->start_time, bObj);
		
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_END_TIME, &mObj->end_time, bObj);
		
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_MEASUREMENT, &mObj->volume_measurement, bObj);
		
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_DURATION_MEASUREMENT, &mObj->duration_measurement, bObj);
		
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_FIRST_PACKET, &mObj->time_of_first_packet, bObj);
		
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_LAST_PACKET, &mObj->time_of_last_packet, bObj);
		
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_INFORMATION, &mObj->usage_information, bObj);
		
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
		__up_pfcp_encode__ethernet_traffic_information( &mObj->ethernet_traffic_information, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__usage_report_session_report_request(pfcp_tlv_usage_report_session_report_request_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		
		// pfcp_tlv_urr_id_t urr_id;														PFCP_IE_URR_ID		pfcp_tlv_uint32_t
		__up_pfcp_encode__tlv_uint32( PFCP_IE_URR_ID, &mObj->urr_id, bObj);
		
		// pfcp_tlv_ur_seqn_t ur_seqn;														PFCP_IE_UR_SEQN		pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_UR_SEQN, &mObj->ur_seqn, bObj);
		
		// pfcp_tlv_usage_report_trigger_t usage_report_trigger;							PFCP_IE_USAGE_REPORT_TRIGGER	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_REPORT_TRIGGER, &mObj->usage_report_trigger, bObj);
		
		// pfcp_tlv_start_time_t start_time;												PFCP_IE_START_TIME		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_START_TIME, &mObj->start_time, bObj);
		
		// pfcp_tlv_end_time_t end_time;													PFCP_IE_END_TIME		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_END_TIME, &mObj->end_time, bObj);
		
		// pfcp_tlv_volume_measurement_t volume_measurement;								PFCP_IE_VOLUME_MEASUREMENT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_VOLUME_MEASUREMENT, &mObj->volume_measurement, bObj);
		
		// pfcp_tlv_duration_measurement_t duration_measurement;							PFCP_IE_DURATION_MEASUREMENT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DURATION_MEASUREMENT, &mObj->duration_measurement, bObj);
		
		// pfcp_tlv_application_detection_information_t application_detection_information;	PFCP_IE_APPLICATION_DETECTION_INFORMATION
		__up_pfcp_encode__application_detection_information( &mObj->application_detection_information, bObj);
		
		// pfcp_tlv_ue_ip_address_t ue_ip_address;											PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UE_IP_ADDRESS, &mObj->ue_ip_address, bObj);
		
		// pfcp_tlv_network_instance_t network_instance;									PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_NETWORK_INSTANCE, &mObj->network_instance, bObj);
		
		// pfcp_tlv_time_of_first_packet_t time_of_first_packet;							PFCP_IE_TIME_OF_FIRST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_FIRST_PACKET, &mObj->time_of_first_packet, bObj);
		
		// pfcp_tlv_time_of_last_packet_t time_of_last_packet;								PFCP_IE_TIME_OF_LAST_PACKET	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TIME_OF_LAST_PACKET, &mObj->time_of_last_packet, bObj);
		
		// pfcp_tlv_usage_information_t usage_information;									PFCP_IE_USAGE_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_USAGE_INFORMATION, &mObj->usage_information, bObj);
		
		// pfcp_tlv_query_urr_reference_t query_urr_reference;								PFCP_IE_QUERY_URR_REFERENCE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_QUERY_URR_REFERENCE, &mObj->query_urr_reference, bObj);
		
		// pfcp_tlv_event_time_stamp_t event_time_stamp;									PFCP_IE_EVENT_TIME_STAMP_TYPE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_EVENT_TIME_STAMP_TYPE, &mObj->event_time_stamp, bObj);
		
		// pfcp_tlv_ethernet_traffic_information_t ethernet_traffic_information;			PFCP_IE_ETHERNET_TRAFFIC_INFORMATION
		__up_pfcp_encode__ethernet_traffic_information( &mObj->ethernet_traffic_information, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__downlink_data_report(pfcp_tlv_downlink_data_report_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_DOWNLINK_DATA_REPORT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_pdr_id_t pdr_id;															PFCP_IE_PACKET_DETECTION_RULE_ID pfcp_tlv_uint16_t
		__up_pfcp_encode__tlv_uint16( PFCP_IE_PACKET_DETECTION_RULE_ID, &mObj->pdr_id, bObj);
		
		// pfcp_tlv_downlink_data_service_information_t downlink_data_service_information;		PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION, &mObj->downlink_data_service_information, bObj);
		

		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__create_bar(pfcp_tlv_create_bar_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_BAR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);
		
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY, &mObj->downlink_data_notification_delay, bObj);
		
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT, &mObj->suggested_buffering_packets_count, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_bar_session_modification_request(pfcp_tlv_update_bar_session_modification_request_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);
		
		// pfcp_tlv_downlink_data_notification_delay_t downlink_data_notification_delay;		PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY, &mObj->downlink_data_notification_delay, bObj);
		
		// pfcp_tlv_suggested_buffering_packets_count_t suggested_buffering_packets_count;		PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT, &mObj->suggested_buffering_packets_count, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_bar(pfcp_tlv_remove_bar_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_BAR);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_bar_id_t bar_id;															PFCP_IE_BAR_ID	pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_BAR_ID, &mObj->bar_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__error_indication_report(pfcp_tlv_error_indication_report_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_ERROR_INDICATION_REPORT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_f_teid_t remote_f_teid;														PFCP_IE_F_TEID	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &mObj->remote_f_teid, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__user_plane_path_failure_report(pfcp_tlv_user_plane_path_failure_report_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_remote_gtp_u_peer_t remote_gtp_u_peer_;										PFCP_IE_REMOTE_GTP_U_PEER	pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_REMOTE_GTP_U_PEER, &mObj->remote_gtp_u_peer_, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__create_traffic_endpoint(pfcp_tlv_create_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_TRAFFIC_ENDPOINT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->traffic_endpoint_id, bObj);
		
		// pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &mObj->local_f_teid, bObj);
		
		// pfcp_tlv_network_instance_t network_instance;										PFCP_IE_NETWORK_INSTANCE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_NETWORK_INSTANCE, &mObj->network_instance, bObj);
		
		// pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UE_IP_ADDRESS, &mObj->ue_ip_address, bObj);
		
		// pfcp_tlv_ethernet_pdu_session_information_t ethernet_pdu_session_information;		PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION, &mObj->ethernet_pdu_session_information, bObj);
		
		// pfcp_tlv_framed_route_t framed_route;												PFCP_IE_FRAMED_ROUTE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_ROUTE, &mObj->framed_route, bObj);
		
		// pfcp_tlv_framed_routing_t framed_routing;											PFCP_IE_FRAMED_ROUTING	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_ROUTING, &mObj->framed_routing, bObj);
		
		// pfcp_tlv_framed_ipv6_route_t framed_ipv6_route;										PFCP_IE_FRAMED_IPV6_ROUTE	pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_FRAMED_IPV6_ROUTE, &mObj->framed_ipv6_route, bObj);
		
		// pfcp_tlv_qfi_t qfi;																	PFCP_IE_QFI			pfcp_tlv_uint8_t
		__up_pfcp_encode__tlv_uint8( PFCP_IE_QFI, &mObj->qfi, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__created_traffic_endpoint(pfcp_tlv_created_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATED_TRAFFIC_ENDPOINT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->traffic_endpoint_id, bObj);
		
		// pfcp_tlv_f_teid_t local_f_teid;														PFCP_IE_F_TEID					pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &mObj->local_f_teid, bObj);
		
		// pfcp_tlv_ue_ip_address_t ue_ip_address;												PFCP_IE_UE_IP_ADDRESS			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_UE_IP_ADDRESS, &mObj->ue_ip_address, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_traffic_endpoint(pfcp_tlv_remove_traffic_endpoint_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_TRAFFIC_ENDPOINT);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_traffic_endpoint_id_t traffic_endpoint_id;									PFCP_IE_TRAFFIC_ENDPOINT_ID		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_TRAFFIC_ENDPOINT_ID, &mObj->traffic_endpoint_id, bObj);
		
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__create_mar(pfcp_tlv_create_mar_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_CREATE_MAR_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAR_ID_TYPE, &mObj->mar_id, bObj);

		// pfcp_tlv_steering_functionality_t steering_functionality;										PFCP_IE_STEERING_FUNCTIONALITY_TYPE			pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_STEERING_FUNCTIONALITY_TYPE, &mObj->steering_functionality, bObj);
		
		// pfcp_tlv_steering_mode_t steering_mode;															PFCP_IE_STEERING_MODE_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_STEERING_MODE_TYPE, &mObj->steering_mode, bObj);
		
		// pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
		__up_pfcp_encode__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1, bObj);
			
		// pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
		__up_pfcp_encode__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2, bObj);
		


		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__remove_mar(pfcp_tlv_remove_mar_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{	
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_REMOVE_MAR_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mar_id_t mar_id;																				PFCP_IE_MAR_ID_TYPE		pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAR_ID_TYPE, &mObj->mar_id, bObj);
		
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	return 0;
}

int __up_pfcp_encode__update_mar(pfcp_tlv_update_mar_t * mObj, app_ep_udp_message_t * bObj)
{
	if( mObj->presence == 1) 
	{
		uint16_t glen = 0;
		
		app_ep__encode__u16( bObj, PFCP_IE_UPDATE_MAR_TYPE);
		app_ep__encode__u16( bObj, 0);
		
		uint16_t len = app_ep__get_len( bObj);
		
		// pfcp_tlv_presence_t presence;
		// pfcp_tlv_mar_id_t mar_id;																		PFCP_IE_MAR_ID_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_MAR_ID_TYPE, &mObj->mar_id, bObj);
		
		// pfcp_tlv_steering_functionality_t steering_functionality;										PFCP_IE_STEERING_FUNCTIONALITY_TYPE			pfcp_tlv_octet_t	
		__up_pfcp_encode__tlv_oct( PFCP_IE_STEERING_FUNCTIONALITY_TYPE, &mObj->steering_functionality, bObj);
		
		// pfcp_tlv_steering_mode_t steering_mode;															PFCP_IE_STEERING_MODE_TYPE			pfcp_tlv_octet_t
		__up_pfcp_encode__tlv_oct( PFCP_IE_STEERING_MODE_TYPE, &mObj->steering_mode, bObj);
		
		// pfcp_tlv_update_access_forwarding_action_information_1_t update_access_forwarding_action_information_1;
		__up_pfcp_encode__update_access_forwarding_action_information_1( &mObj->update_access_forwarding_action_information_1, bObj);
		
		// pfcp_tlv_update_access_forwarding_action_information_2_t update_access_forwarding_action_information_2;
		__up_pfcp_encode__update_access_forwarding_action_information_2( &mObj->update_access_forwarding_action_information_2, bObj);
			
		// pfcp_tlv_access_forwarding_action_information_1_t access_forwarding_action_information_1;
		__up_pfcp_encode__access_forwarding_action_information_1( &mObj->access_forwarding_action_information_1, bObj);
			
		// pfcp_tlv_access_forwarding_action_information_2_t access_forwarding_action_information_2;
		__up_pfcp_encode__access_forwarding_action_information_2( &mObj->access_forwarding_action_information_2, bObj);
			
		glen = app_ep__get_len( bObj) - len;
		app_ep__encode__setAt( bObj, len-2, glen);
	}
	
	return 0;
}








int __up_pfcp_encode__heartbeat_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	__up_pfcp_encode__tlv_uint32( PFCP_IE_RECOVERY_TIME_STAMP, &mObj->u.heartbeat_request.recovery_time_stamp, bObj);
	app_ep__encode__msglen( bObj);
	return 0;
}


int __up_pfcp_encode__heartbeat_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	__up_pfcp_encode__tlv_uint32( PFCP_IE_RECOVERY_TIME_STAMP, &mObj->u.heartbeat_response.recovery_time_stamp, bObj);
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__pfd_management_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	__up_pfcp_encode__application_id_s_pfds( &mObj->u.pfd_management_request.application_id_s_pfds, bObj);
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__pfd_management_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	//pfcp_tlv_offending_ie_t offending_ie;				PFCP_IE_OFFENDING_IE	pfcp_tlv_uint16_t
	__up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &mObj->u.pfd_management_response.offending_ie, bObj);
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__association_setup_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	//   pfcp_tlv_node_id_t node_id;
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &mObj->u.association_setup_request.node_id, bObj);
	
	//pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
	__up_pfcp_encode__tlv_uint32( PFCP_IE_RECOVERY_TIME_STAMP, &mObj->u.association_setup_request.recovery_time_stamp, bObj);

	//pfcp_tlv_up_function_features_t cp_function_features;
	__up_pfcp_encode__tlv_uint8(  PFCP_IE_CP_FUNCTION_FEATURES, &mObj->u.association_setup_request.cp_function_features, bObj);
	
	//pfcp_tlv_up_function_features_t up_function_features;
	__up_pfcp_encode__tlv_oct(  PFCP_IE_UP_FUNCTION_FEATURES, &mObj->u.association_setup_request.up_function_features, bObj);
	
    // pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4]; pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_request.user_plane_ip_resource_information[0], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_request.user_plane_ip_resource_information[1], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_request.user_plane_ip_resource_information[2], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_request.user_plane_ip_resource_information[3], bObj);
	
	// pfcp_tlv_ue_ip_address_t ue_ip_address_pool_identity;						pfcp_tlv_octet_t	PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE
    __up_pfcp_encode__tlv_oct(  PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE, &mObj->u.association_setup_request.ue_ip_address_pool_identity, bObj);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;			pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
    __up_pfcp_encode__tlv_oct(  PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE, &mObj->u.association_setup_request.alternative_smf_ip_address, bObj);
	
	// pfcp_tlv_smf_set_id_t smf_set_id;											pfcp_tlv_octet_t	PFCP_IE_SMF_SET_ID_TYPE
	__up_pfcp_encode__tlv_oct(  PFCP_IE_SMF_SET_ID_TYPE, &mObj->u.association_setup_request.smf_set_id, bObj);
	
	
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__association_setup_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	//pfcp_tlv_node_id_t node_id;
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &mObj->u.association_setup_response.node_id, bObj);
	
	//pfcp_tlv_cause_t cause;			pfcp_tlv_uint8_t	PFCP_IE_CAUSE
	__up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &mObj->u.association_setup_response.cause, bObj);
	
	//pfcp_tlv_recovery_time_stamp_t recovery_time_stamp;
	__up_pfcp_encode__tlv_uint32( PFCP_IE_RECOVERY_TIME_STAMP, &mObj->u.association_setup_response.recovery_time_stamp, bObj);
	
	//pfcp_tlv_up_function_features_t up_function_features;
	__up_pfcp_encode__tlv_oct(  PFCP_IE_UP_FUNCTION_FEATURES, &mObj->u.association_setup_response.up_function_features, bObj);
	
    // pfcp_tlv_cp_function_features_t cp_function_features;			pfcp_tlv_uint8_t	PFCP_IE_CP_FUNCTION_FEATURES
    __up_pfcp_encode__tlv_uint8( PFCP_IE_CP_FUNCTION_FEATURES, &mObj->u.association_setup_response.cp_function_features, bObj);
	
	// pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];		pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_response.user_plane_ip_resource_information[0], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_response.user_plane_ip_resource_information[1], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_response.user_plane_ip_resource_information[2], bObj);
	__up_pfcp_encode__tlv_oct(  PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &mObj->u.association_setup_response.user_plane_ip_resource_information[3], bObj);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;				pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
	__up_pfcp_encode__tlv_oct(  PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE, &mObj->u.association_setup_response.alternative_smf_ip_address, bObj);
	
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__association_update_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_association_update_request_t  *  rObj = &mObj->u.association_update_request;
	
    // pfcp_tlv_node_id_t node_id;									pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_up_function_features_t up_function_features;		pfcp_tlv_octet_t	PFCP_IE_UP_FUNCTION_FEATURES
    __up_pfcp_encode__tlv_oct( PFCP_IE_UP_FUNCTION_FEATURES, &rObj->up_function_features, bObj);
	
	// pfcp_tlv_cp_function_features_t cp_function_features;			pfcp_tlv_uint8_t	PFCP_IE_CP_FUNCTION_FEATURES
    __up_pfcp_encode__tlv_uint8( PFCP_IE_CP_FUNCTION_FEATURES, &rObj->cp_function_features, bObj);
	
	// pfcp_tlv_pfcp_association_release_request_t pfcp_association_release_request;		pfcp_tlv_octet_t	PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST
    __up_pfcp_encode__tlv_oct( PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST, &rObj->pfcp_association_release_request, bObj);
	
	// pfcp_tlv_graceful_release_period_t graceful_release_period;			pfcp_tlv_octet_t	PFCP_IE_GRACEFUL_RELEASE_PERIOD
    __up_pfcp_encode__tlv_oct( PFCP_IE_GRACEFUL_RELEASE_PERIOD, &rObj->graceful_release_period, bObj);
	
	// pfcp_tlv_user_plane_ip_resource_information_t user_plane_ip_resource_information[4];		pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION
    __up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &rObj->user_plane_ip_resource_information[0], bObj);
	__up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &rObj->user_plane_ip_resource_information[1], bObj);
	__up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &rObj->user_plane_ip_resource_information[2], bObj);
	__up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION, &rObj->user_plane_ip_resource_information[3], bObj);
	
	// pfcp_tlv_pfcpaureq_flags_t pfcpaureq_flags;								pfcp_tlv_uint8_t	PFCP_IE_PFCPAUREQ_FLAGS_TYPE
    __up_pfcp_encode__tlv_uint8( PFCP_IE_PFCPAUREQ_FLAGS_TYPE, &rObj->pfcpaureq_flags, bObj);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;		pfcp_tlv_octet_t	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE
	__up_pfcp_encode__tlv_oct( PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE, &rObj->alternative_smf_ip_address, bObj);
	
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__association_update_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_association_update_response_t  *  rObj = &mObj->u.association_update_response;
	
    // pfcp_tlv_node_id_t node_id;									pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_cause_t cause;										pfcp_tlv_uint8_t	PFCP_IE_CAUSE
	__up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &rObj->cause, bObj);
	
    // pfcp_tlv_up_function_features_t up_function_features;		pfcp_tlv_octet_t	PFCP_IE_UP_FUNCTION_FEATURES
    __up_pfcp_encode__tlv_oct( PFCP_IE_UP_FUNCTION_FEATURES, &rObj->up_function_features, bObj);
	
	// pfcp_tlv_cp_function_features_t cp_function_features;		pfcp_tlv_uint8_t	PFCP_IE_CP_FUNCTION_FEATURES
	__up_pfcp_encode__tlv_uint8( PFCP_IE_CP_FUNCTION_FEATURES, &rObj->cp_function_features, bObj);

	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__association_release_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_association_release_request_t  *  rObj = &mObj->u.association_release_request;
	
	//pfcp_tlv_node_id_t node_id;					pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__association_release_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_association_release_response_t  *  rObj = &mObj->u.association_release_response;
	
    // pfcp_tlv_node_id_t node_id;						pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_cause_t cause;							pfcp_tlv_uint8_t		PFCP_IE_CAUSE
	__up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &rObj->cause, bObj);
	
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__version_not_supported_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_version_not_supported_response_t  *  rObj = &mObj->u.version_not_supported_response;
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__node_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_node_report_request_t  *  rObj = &mObj->u.node_report_request;
	
    // pfcp_tlv_node_id_t node_id;							pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_node_report_type_t node_report_type;		pfcp_tlv_octet_t	PFCP_IE_NODE_REPORT_TYPE	
    __up_pfcp_encode__tlv_oct( PFCP_IE_NODE_REPORT_TYPE, &rObj->node_report_type, bObj);
	
	// pfcp_tlv_user_plane_path_failure_report_t user_plane_path_failure_report;		PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT
	__up_pfcp_encode__user_plane_path_failure_report( &rObj->user_plane_path_failure_report, bObj);

	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__node_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_node_report_response_t  *  rObj = &mObj->u.node_report_response;
	
    // pfcp_tlv_node_id_t node_id;						pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_cause_t cause;							pfcp_tlv_uint8_t		PFCP_IE_CAUSE
	__up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &rObj->cause, bObj);
	
    // pfcp_tlv_offending_ie_t offending_ie;			pfcp_tlv_uint16_t		PFCP_IE_OFFENDING_IE	
	__up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);

	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_set_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_session_set_deletion_request_t  *  rObj = &mObj->u.session_set_deletion_request;

    // pfcp_tlv_node_id_t node_id;				pfcp_tlv_octet_t		PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	
    // pfcp_tlv_fq_csid_t sgw_c_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->sgw_c_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->pgw_c_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t sgw_u_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->sgw_u_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t pgw_u_fq_csid;		pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->pgw_u_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->twan_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->epdg_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t mme_fq_csid;			pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
	__up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->mme_fq_csid, bObj);
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_set_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	pfcp_session_set_deletion_response_t  *  rObj = &mObj->u.session_set_deletion_response;

    // pfcp_tlv_node_id_t node_id;					pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
    
	// pfcp_tlv_cause_t cause;						pfcp_tlv_uint8_t	PFCP_IE_CAUSE
    __up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &rObj->cause, bObj);
	
	// pfcp_tlv_offending_ie_t offending_ie;		pfcp_tlv_uint16_t	PFCP_IE_OFFENDING_IE
	__up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);

	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_establishment_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_establishment_request_t  *  rObj = &mObj->u.session_establishment_request;
	
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);
	__up_pfcp_encode__tlv_oct( PFCP_IE_F_SEID, &rObj->cp_f_seid, bObj);
	
	int i = 0;
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_encode__create_pdr( &rObj->create_pdr[i], bObj);
	}
	
    // pfcp_tlv_create_far_t create_far[8];
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_encode__create_far( &rObj->create_far[i], bObj);
	}
	
    // pfcp_tlv_create_urr_t create_urr[2];
	for( i = 0; i < 10; i++)
	{
		__up_pfcp_encode__create_urr( &rObj->create_urr[i], bObj);
	}
    
	// pfcp_tlv_create_qer_t create_qer[4];
	for( i = 0; i < 4; i++)
	{
		__up_pfcp_encode__create_qer( &rObj->create_qer[i], bObj);
	}
	
    // pfcp_tlv_create_bar_t create_bar;
	__up_pfcp_encode__create_bar( &rObj->create_bar, bObj);
	
    // pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;			PFCP_IE_CREATE_TRAFFIC_ENDPOINT
    __up_pfcp_encode__create_traffic_endpoint( &rObj->create_traffic_endpoint, bObj);
	
	// pfcp_tlv_pdn_type_t pdn_type;				//pfcp_tlv_uint8_t
	__up_pfcp_encode__tlv_uint8(  PFCP_IE_PDN_TYPE, &rObj->pdn_type, bObj);
	
    // pfcp_tlv_fq_csid_t sgw_c_fq_csid;			//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
	__up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->sgw_c_fq_csid, bObj);
	
    // pfcp_tlv_fq_csid_t mme_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->mme_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;			//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->pgw_c_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->epdg_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;				//pfcp_tlv_octet_t		PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->twan_fq_csid, bObj);
	
	// pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;	pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_INACTIVITY_TIMER
    __up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_INACTIVITY_TIMER, &rObj->user_plane_inactivity_timer, bObj);
	
	// pfcp_tlv_user_id_t user_id;											pfcp_tlv_octet_t	PFCP_IE_USER_ID
    __up_pfcp_encode__tlv_oct( PFCP_IE_USER_ID, &rObj->user_id, bObj);
	
	// pfcp_tlv_trace_information_t trace_information;						pfcp_tlv_octet_t	PFCP_IE_TRACE_INFORMATION
    __up_pfcp_encode__tlv_oct( PFCP_IE_TRACE_INFORMATION, &rObj->trace_information, bObj);
	
	// pfcp_tlv_apn_dnn_t apn_dnn;											pfcp_tlv_octet_t	PFCP_IE_APN_DNN_TYPE
    __up_pfcp_encode__tlv_oct( PFCP_IE_APN_DNN_TYPE, &rObj->apn_dnn, bObj);
	
	// pfcp_tlv_create_mar_t create_mar;									PFCP_IE_CREATE_MAR_TYPE
	__up_pfcp_encode__create_mar( &rObj->create_mar, bObj);
	
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_establishment_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_establishment_response_t  *  rObj = &mObj->u.session_establishment_response;

    // pfcp_tlv_node_id_t node_id;
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);

    // pfcp_tlv_cause_t cause;
	__up_pfcp_encode__tlv_uint8(  PFCP_IE_CAUSE, &rObj->cause, bObj);

    // pfcp_tlv_offending_ie_t offending_ie;
    __up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);
	
	// pfcp_tlv_f_seid_t up_f_seid;
	__up_pfcp_encode__tlv_oct( PFCP_IE_F_SEID, &rObj->up_f_seid, bObj);
	
	
    // pfcp_tlv_created_pdr_t created_pdr[8];
	int i = 0;
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_encode__created_pdr( &rObj->created_pdr[i], bObj);
	}
	
    // pfcp_tlv_load_control_information_t load_control_information;					PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_encode__load_control_information( &rObj->load_control_information, bObj);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;			PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_encode__overload_control_information( &rObj->overload_control_information, bObj);
	
	// pfcp_tlv_fq_csid_t sgw_u_fq_csid;												pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
	__up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->sgw_u_fq_csid, bObj);
	
    // pfcp_tlv_fq_csid_t pgw_u_fq_csid;												pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->pgw_u_fq_csid, bObj);
	
	// pfcp_tlv_failed_rule_id_t failed_rule_id;										pfcp_tlv_octet_t	PFCP_IE_FAILED_RULE_ID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FAILED_RULE_ID, &rObj->failed_rule_id, bObj);
	
	// pfcp_tlv_created_traffic_endpoint_t created_traffic_endpoint;					PFCP_IE_CREATED_TRAFFIC_ENDPOINT
	__up_pfcp_encode__created_traffic_endpoint( &rObj->created_traffic_endpoint, bObj);

	
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__session_modification_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_modification_request_t  *  rObj = &mObj->u.session_modification_request;
	
	int i = 0;
    // pfcp_tlv_f_seid_t cp_f_seid;								pfcp_tlv_octet_t	PFCP_IE_F_SEID
	__up_pfcp_encode__tlv_oct( PFCP_IE_F_SEID, &rObj->cp_f_seid, bObj);
	
    // pfcp_tlv_remove_pdr_t remove_pdr[8];						PFCP_IE_REMOVE_PDR
	for( i = 0; i < 8; i++)
	{	
		__up_pfcp_encode__remove_pdr( &rObj->remove_pdr[i], bObj);
	}
	
    // pfcp_tlv_remove_far_t remove_far[8];						PFCP_IE_REMOVE_FAR		
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_encode__remove_far( &rObj->remove_far[i], bObj);
	}
	
	// pfcp_tlv_remove_urr_t remove_urr[2];						PFCP_IE_REMOVE_URR
    for( i = 0; i < 10; i++)
	{	
		__up_pfcp_encode__remove_urr( &rObj->remove_urr[i], bObj);
	}
	
	// pfcp_tlv_remove_qer_t remove_qer[4];						PFCP_IE_REMOVE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_encode__remove_qer( &rObj->remove_qer[i], bObj);
	}
	
	// pfcp_tlv_remove_bar_t remove_bar;						PFCP_IE_REMOVE_BAR
    __up_pfcp_encode__remove_bar( &rObj->remove_bar, bObj);
	
	// pfcp_tlv_remove_traffic_endpoint_t remove_traffic_endpoint;	PFCP_IE_REMOVE_TRAFFIC_ENDPOINT
    __up_pfcp_encode__remove_traffic_endpoint( &rObj->remove_traffic_endpoint, bObj);
	
	// pfcp_tlv_create_pdr_t create_pdr[8];							PFCP_IE_CREATE_PDR
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_encode__create_pdr( &rObj->create_pdr[i], bObj);
	}
	
	// pfcp_tlv_create_far_t create_far[8];							PFCP_IE_CREATE_FAR
    for( i = 0; i < 8; i++)
	{	
		__up_pfcp_encode__create_far( &rObj->create_far[i], bObj);
	}
	
	// pfcp_tlv_create_urr_t create_urr[2];							PFCP_IE_CREATE_URR
    for( i = 0; i < 10; i++)
	{	
		__up_pfcp_encode__create_urr( &rObj->create_urr[i], bObj);
	}
	
	// pfcp_tlv_create_qer_t create_qer[4];							PFCP_IE_CREATE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_encode__create_qer( &rObj->create_qer[i], bObj);
	}
	
	// pfcp_tlv_create_bar_t create_bar;							PFCP_IE_CREATE_BAR
    __up_pfcp_encode__create_bar( &rObj->create_bar, bObj);
	
	// pfcp_tlv_create_traffic_endpoint_t create_traffic_endpoint;	PFCP_IE_CREATE_TRAFFIC_ENDPOINT
    __up_pfcp_encode__create_traffic_endpoint( &rObj->create_traffic_endpoint, bObj);
	
	// pfcp_tlv_update_pdr_t update_pdr[8];							PFCP_IE_UPDATE_PDR
	for( i = 0; i < 8; i++)
	{
		__up_pfcp_encode__update_pdr( &rObj->update_pdr[i], bObj);
	}
	
    // pfcp_tlv_update_far_t update_far[8];
	for( i = 0; i < 8; i++)
	{
		//printf("OuterHeaderCreation_len=%u  %s|%s|%d\n", rObj->update_far[i].update_forwarding_parameters, __FILE__, __FUNCTION__, __LINE__);
		__up_pfcp_encode__update_far( &rObj->update_far[i], bObj);
	}

    // pfcp_tlv_update_urr_t update_urr[2];									PFCP_IE_UPDATE_URR
	for( i = 0; i < 10; i++)
	{		
		__up_pfcp_encode__update_urr( &rObj->update_urr[i], bObj);
	}
	
	// pfcp_tlv_update_qer_t update_qer[4];									PFCP_IE_UPDATE_QER
    for( i = 0; i < 4; i++)
	{	
		__up_pfcp_encode__update_qer( &rObj->update_qer[i], bObj);
	}
	
	// pfcp_tlv_update_bar_session_modification_request_t update_bar;		PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST
    __up_pfcp_encode__update_bar_session_modification_request( &rObj->update_bar, bObj);
	
	// pfcp_tlv_update_traffic_endpoint_t update_traffic_endpoint;			pfcp_tlv_octet_t	PFCP_IE_UPDATE_TRAFFIC_ENDPOINT
    __up_pfcp_encode__tlv_oct( PFCP_IE_UPDATE_TRAFFIC_ENDPOINT, &rObj->update_traffic_endpoint, bObj);
	
	// pfcp_tlv_pfcpsmreq_flags_t pfcpsmreq_flags;							pfcp_tlv_uint8_t	PFCP_IE_PFCPSMREQ_FLAGS
    __up_pfcp_encode__tlv_uint8( PFCP_IE_PFCPSMREQ_FLAGS, &rObj->pfcpsmreq_flags, bObj);
	
	// pfcp_tlv_query_urr_t query_urr	;									PFCP_IE_QUERY_URR	
    __up_pfcp_encode__query_urr( &rObj->query_urr, bObj);
	
	// pfcp_tlv_fq_csid_t pgw_c_fq_csid;									pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
	__up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->pgw_c_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t sgw_c_fq_csid;									pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->sgw_c_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t mme_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->mme_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t epdg_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->epdg_fq_csid, bObj);
	
	// pfcp_tlv_fq_csid_t twan_fq_csid;										pfcp_tlv_octet_t	PFCP_IE_FQ_CSID
    __up_pfcp_encode__tlv_oct( PFCP_IE_FQ_CSID, &rObj->twan_fq_csid, bObj);
	
	// pfcp_tlv_user_plane_inactivity_timer_t user_plane_inactivity_timer;	pfcp_tlv_octet_t	PFCP_IE_USER_PLANE_INACTIVITY_TIMER
    __up_pfcp_encode__tlv_oct( PFCP_IE_USER_PLANE_INACTIVITY_TIMER, &rObj->user_plane_inactivity_timer, bObj);
	
	// pfcp_tlv_query_urr_reference_t query_urr_reference;					pfcp_tlv_octet_t	PFCP_IE_QUERY_URR_REFERENCE
    __up_pfcp_encode__tlv_oct( PFCP_IE_QUERY_URR_REFERENCE, &rObj->query_urr_reference, bObj);
	
	// pfcp_tlv_trace_information_t trace_information;						pfcp_tlv_octet_t	PFCP_IE_TRACE_INFORMATION
    __up_pfcp_encode__tlv_oct( PFCP_IE_TRACE_INFORMATION, &rObj->trace_information, bObj);
	
	// pfcp_tlv_remove_mar_t remove_mar;									PFCP_IE_REMOVE_MAR_TYPE
    __up_pfcp_encode__remove_mar( &rObj->remove_mar, bObj);
	
	// pfcp_tlv_update_mar_t update_mar;									PFCP_IE_UPDATE_MAR_TYPE
    __up_pfcp_encode__update_mar( &rObj->update_mar, bObj);
	
	// pfcp_tlv_create_mar_t create_mar;									PFCP_IE_CREATE_MAR_TYPE
    __up_pfcp_encode__create_mar( &rObj->create_mar, bObj);
	
	// pfcp_tlv_node_id_t node_id;											pfcp_tlv_octet_t	PFCP_IE_NODE_ID
	__up_pfcp_encode__tlv_oct( PFCP_IE_NODE_ID, &rObj->node_id, bObj);

	
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__session_modification_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_modification_response_t  *  rObj = &mObj->u.session_modification_response;
	
    // pfcp_tlv_cause_t cause;										PFCP_IE_CAUSE pfcp_tlv_uint8_t
    __up_pfcp_encode__tlv_uint8(  PFCP_IE_CAUSE, &rObj->cause, bObj);
	
	// pfcp_tlv_offending_ie_t offending_ie;						PFCP_IE_OFFENDING_IE 	pfcp_tlv_uint16_t
	__up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);
	
    // pfcp_tlv_created_pdr_t created_pdr[8];						PFCP_IE_CREATED_PDR
    int i = 0;
	for( i = 0; i < 8; i++) {
		__up_pfcp_encode__created_pdr( &rObj->created_pdr[i], bObj);
	}
	
	// pfcp_tlv_load_control_information_t load_control_information;	PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_encode__load_control_information( &rObj->load_control_information, bObj);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;		PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_encode__overload_control_information( &rObj->overload_control_information, bObj);
	
	// pfcp_tlv_usage_report_session_modification_response_t usage_report;		PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE
    __up_pfcp_encode__usage_report_session_modification_response( &rObj->usage_report, bObj);
	
	// pfcp_tlv_failed_rule_id_t failed_rule_id;								pfcp_tlv_octet_t	PFCP_IE_FAILED_RULE_ID					// pfcp_tlv_octet_t
    __up_pfcp_encode__tlv_oct( PFCP_IE_FAILED_RULE_ID, &rObj->failed_rule_id, bObj);
	
	// pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;		pfcp_tlv_octet_t PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION // pfcp_tlv_octet_t	
    __up_pfcp_encode__tlv_oct( PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION, &rObj->additional_usage_reports_information, bObj);
	
	// pfcp_tlv_created_traffic_endpoint_t created_updated_traffic_endpoint;		PFCP_IE_CREATED_TRAFFIC_ENDPOINT
	__up_pfcp_encode__created_traffic_endpoint( &rObj->created_updated_traffic_endpoint, bObj);
	
	
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__session_deletion_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	//pfcp_session_deletion_request_t  *  rObj = &mObj->u.session_deletion_request;
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_deletion_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_deletion_response_t  *  rObj = &mObj->u.session_deletion_response;
	
    // pfcp_tlv_cause_t cause;													PFCP_IE_CAUSE
	__up_pfcp_encode__tlv_uint8(  PFCP_IE_CAUSE, &rObj->cause, bObj);

    // pfcp_tlv_offending_ie_t offending_ie;									pfcp_tlv_uint16_t	PFCP_IE_OFFENDING_IE
	__up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);
	
    // pfcp_tlv_load_control_information_t load_control_information;			PFCP_IE_LOAD_CONTROL_INFORMATION
	__up_pfcp_encode__load_control_information( &rObj->load_control_information, bObj);
	
    // pfcp_tlv_overload_control_information_t overload_control_information;	PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_encode__overload_control_information( &rObj->overload_control_information, bObj);
	
	// pfcp_tlv_usage_report_session_deletion_response_t usage_report;			PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE
	__up_pfcp_encode__usage_report_session_deletion_response( &rObj->usage_report, bObj);
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

int __up_pfcp_encode__session_report_request( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
	
	pfcp_session_report_request_t *  rObj = &mObj->u.session_report_request;

    // pfcp_tlv_report_type_t report_type;									pfcp_tlv_uint8_t	PFCP_IE_REPORT_TYPE
    __up_pfcp_encode__tlv_uint8( PFCP_IE_REPORT_TYPE, &rObj->report_type, bObj);
	
	// pfcp_tlv_downlink_data_report_t downlink_data_report;				PFCP_IE_DOWNLINK_DATA_REPORT
    __up_pfcp_encode__downlink_data_report( &rObj->downlink_data_report, bObj);
	
	// pfcp_tlv_usage_report_session_report_request_t usage_report;			PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST
    __up_pfcp_encode__usage_report_session_report_request( &rObj->usage_report, bObj);
	
	// pfcp_tlv_error_indication_report_t error_indication_report;			PFCP_IE_ERROR_INDICATION_REPORT
    __up_pfcp_encode__error_indication_report( &rObj->error_indication_report, bObj);
	
	// pfcp_tlv_load_control_information_t load_control_information;		PFCP_IE_LOAD_CONTROL_INFORMATION
    __up_pfcp_encode__load_control_information( &rObj->load_control_information, bObj);
	
	// pfcp_tlv_overload_control_information_t overload_control_information;		PFCP_IE_OVERLOAD_CONTROL_INFORMATION
    __up_pfcp_encode__overload_control_information( &rObj->overload_control_information, bObj);
	
	// pfcp_tlv_additional_usage_reports_information_t additional_usage_reports_information;	pfcp_tlv_octet_t PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION
    __up_pfcp_encode__tlv_oct( PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION, &rObj->additional_usage_reports_information, bObj);
	
	// pfcp_tlv_pfcpsrreq_flags_t pfcpsrreq_flags;									pfcp_tlv_uint8_t		PFCP_IE_PFCPSRREQ_FLAGS_TYPE
    __up_pfcp_encode__tlv_uint8( PFCP_IE_PFCPSRREQ_FLAGS_TYPE, &rObj->pfcpsrreq_flags, bObj);
	
	// pfcp_tlv_f_seid_t old_cp_f_seid;												pfcp_tlv_octet_t	PFCP_IE_F_SEID
	__up_pfcp_encode__tlv_oct( PFCP_IE_F_SEID, &rObj->old_cp_f_seid, bObj);
	
	
	app_ep__encode__msglen( bObj);	
	return 0;
}

//pfcp_session_report_response_t
int __up_pfcp_encode__session_report_response( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	__up_pfcp_encode__header( &mObj->header, bObj);
    
	pfcp_session_report_response_t * rObj = &mObj->u.session_report_response;
	
    // pfcp_tlv_cause_t cause;												PFCP_IE_CAUSE				pfcp_tlv_uint8_t
    __up_pfcp_encode__tlv_uint8( PFCP_IE_CAUSE, &rObj->cause, bObj);
	
	// pfcp_tlv_offending_ie_t offending_ie;								PFCP_IE_OFFENDING_IE		pfcp_tlv_uint16_t
    __up_pfcp_encode__tlv_uint16( PFCP_IE_OFFENDING_IE, &rObj->offending_ie, bObj);
	
	// pfcp_tlv_update_bar_pfcp_session_report_response_t update_bar;		PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE
    __up_pfcp_encode__update_bar( &rObj->update_bar, bObj);
	
	// pfcp_tlv_pfcpsrrsp_flags_t pfcpsrrsp_flags;							PFCP_IE_PFCPSRRSP_FLAGS		pfcp_tlv_uint8_t
    __up_pfcp_encode__tlv_uint8( PFCP_IE_PFCPSRRSP_FLAGS, &rObj->pfcpsrrsp_flags, bObj);
	
	// pfcp_tlv_f_seid_t cp_f_seid;											PFCP_IE_F_SEID				pfcp_tlv_octet_t
    __up_pfcp_encode__tlv_oct( PFCP_IE_F_SEID, &rObj->cp_f_seid, bObj);
	
	// pfcp_tlv_f_teid_t n4_u_f_teid;										PFCP_IE_F_TEID				pfcp_tlv_octet_t
    __up_pfcp_encode__tlv_oct( PFCP_IE_F_TEID, &rObj->n4_u_f_teid, bObj);
	
	// pfcp_tlv_alternative_smf_ip_address_t alternative_smf_ip_address;	PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE		pfcp_tlv_octet_t
	__up_pfcp_encode__tlv_oct( PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE, &rObj->alternative_smf_ip_address, bObj);
	
	
	app_ep__encode__msglen( bObj);
	return 0;
}

int __up_pfcp_encode__message( pfcp_message_t * mObj, app_ep_udp_message_t * bObj)
{
	switch( mObj->header.MessageType)
	{
		case PFCP_HEARTBEAT_REQUEST:
			return __up_pfcp_encode__heartbeat_request( mObj, bObj);
		case PFCP_HEARTBEAT_RESPONSE:
			return __up_pfcp_encode__heartbeat_response( mObj, bObj);
		case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
			return __up_pfcp_encode__pfd_management_request( mObj, bObj);
		case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
			return __up_pfcp_encode__pfd_management_response( mObj, bObj);
		case PFCP_ASSOCIATION_SETUP_REQUEST:
			return __up_pfcp_encode__association_setup_request( mObj, bObj);
		case PFCP_ASSOCIATION_SETUP_RESPONSE:
			return __up_pfcp_encode__association_setup_response( mObj, bObj);
		case PFCP_ASSOCIATION_UPDATE_REQUEST:
			return __up_pfcp_encode__association_update_request( mObj, bObj);
		case PFCP_ASSOCIATION_UPDATE_RESPONSE:
			return __up_pfcp_encode__association_update_response( mObj, bObj);
		case PFCP_ASSOCIATION_RELEASE_REQUEST:
			return __up_pfcp_encode__association_release_request( mObj, bObj);
		case PFCP_ASSOCIATION_RELEASE_RESPONSE:
			return __up_pfcp_encode__association_release_response( mObj, bObj);
		case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
			return __up_pfcp_encode__version_not_supported_response( mObj, bObj);
		case PFCP_NODE_REPORT_REQUEST:
			return __up_pfcp_encode__node_report_request( mObj, bObj);
		case PFCP_NODE_REPORT_RESPONSE:
			return __up_pfcp_encode__node_report_response( mObj, bObj);
		case PFCP_SESSION_SET_DELETION_REQUEST:
			return __up_pfcp_encode__session_set_deletion_request( mObj, bObj);
		case PFCP_SESSION_SET_DELETION_RESPONSE:
			return __up_pfcp_encode__session_set_deletion_response( mObj, bObj);
		case PFCP_SESSION_ESTABLISHMENT_REQUEST:
			return __up_pfcp_encode__session_establishment_request( mObj, bObj);
		case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
			return __up_pfcp_encode__session_establishment_response( mObj, bObj);
		case PFCP_SESSION_MODIFICATION_REQUEST:
			return __up_pfcp_encode__session_modification_request( mObj, bObj);
		case PFCP_SESSION_MODIFICATION_RESPONSE:
			return __up_pfcp_encode__session_modification_response( mObj, bObj);
		case PFCP_SESSION_DELETION_REQUEST:
			return __up_pfcp_encode__session_deletion_request( mObj, bObj);
		case PFCP_SESSION_DELETION_RESPONSE:
			return __up_pfcp_encode__session_deletion_response( mObj, bObj);
		case PFCP_SESSION_REPORT_REQUEST:
			return __up_pfcp_encode__session_report_request( mObj, bObj);
		case PFCP_SESSION_REPORT_RESPONSE:
			return __up_pfcp_encode__session_report_response( mObj, bObj);
		default:
			return -1;
	}
	return -1;
}














