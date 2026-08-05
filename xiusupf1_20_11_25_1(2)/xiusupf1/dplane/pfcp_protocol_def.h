#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>


#ifndef S_PFCP_PROTOCOL_DEF
#define S_PFCP_PROTOCOL_DEF


#define PFCP__E_TYPE_UNKNOWN 													0
#define PFCP__E_TYPE_U8 														1
#define PFCP__E_TYPE_U16 														2
#define PFCP__E_TYPE_U24 														3
#define PFCP__E_TYPE_U32 														4
#define PFCP__E_TYPE_S8 														5
#define PFCP__E_TYPE_S16 														6
#define PFCP__E_TYPE_S24 														7
#define PFCP__E_TYPE_S32 														8
#define PFCP__E_TYPE_OCT 														9
#define PFCP__E_TYPE_GRP 														10


#define PFCP_MESSAGE_RESERVED                                                (0)
// PFCP_NODE_RELATED_MESSAGES
#define PFCP_HEARTBEAT_REQUEST                                               (1)
#define PFCP_HEARTBEAT_RESPONSE                                              (2)
#define PFCP_PFCP_PFD_MANAGEMENT_REQUEST                                     (3)
#define PFCP_PFCP_PFD_MANAGEMENT_RESPONSE                                    (4)
#define PFCP_ASSOCIATION_SETUP_REQUEST                                       (5)
#define PFCP_ASSOCIATION_SETUP_RESPONSE                                      (6)
#define PFCP_ASSOCIATION_UPDATE_REQUEST                                      (7)
#define PFCP_ASSOCIATION_UPDATE_RESPONSE                                     (8)
#define PFCP_ASSOCIATION_RELEASE_REQUEST                                     (9)
#define PFCP_ASSOCIATION_RELEASE_RESPONSE                                    (10)
#define PFCP_VERSION_NOT_SUPPORTED_RESPONSE                                  (11)
#define PFCP_NODE_REPORT_REQUEST                                             (12)
#define PFCP_NODE_REPORT_RESPONSE                                            (13)
#define PFCP_SESSION_SET_DELETION_REQUEST                                    (14)
#define PFCP_SESSION_SET_DELETION_RESPONSE                                   (15)
//PFCP_SESSION_RELATED_MESSAGES
#define PFCP_SESSION_ESTABLISHMENT_REQUEST                                   (50)
#define PFCP_SESSION_ESTABLISHMENT_RESPONSE                                  (51)
#define PFCP_SESSION_MODIFICATION_REQUEST                                    (52)
#define PFCP_SESSION_MODIFICATION_RESPONSE                                   (53)
#define PFCP_SESSION_DELETION_REQUEST                                        (54)
#define PFCP_SESSION_DELETION_RESPONSE                                       (55)
#define PFCP_SESSION_REPORT_REQUEST                                          (56)
#define PFCP_SESSION_REPORT_RESPONSE                                         (57)

#define PFCP_IE_RESERVED_0                                                             (0)
#define PFCP_IE_CREATE_PDR                                                             (1)
#define PFCP_IE_PDI                                                                    (2)
#define PFCP_IE_CREATE_FAR                                                             (3)
#define PFCP_IE_FORWARDING_PARAMETERS                                                  (4)
#define PFCP_IE_DUPLICATING_PARAMETERS                                                 (5)
#define PFCP_IE_CREATE_URR                                                             (6)
#define PFCP_IE_CREATE_QER                                                             (7)
#define PFCP_IE_CREATED_PDR                                                            (8)
#define PFCP_IE_UPDATE_PDR                                                             (9)
#define PFCP_IE_UPDATE_FAR                                                             (10)
#define PFCP_IE_UPDATE_FORWARDING_PARAMETERS                                           (11)
#define PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_REPORT_RESPONSE                         (12)
#define PFCP_IE_UPDATE_URR                                                             (13)
#define PFCP_IE_UPDATE_QER                                                             (14)
#define PFCP_IE_REMOVE_PDR                                                             (15)
#define PFCP_IE_REMOVE_FAR                                                             (16)
#define PFCP_IE_REMOVE_URR                                                             (17)
#define PFCP_IE_REMOVE_QER                                                             (18)
#define PFCP_IE_CAUSE                                                                  (19)
#define PFCP_IE_SOURCE_INTERFACE                                                       (20)
#define PFCP_IE_F_TEID                                                                 (21)
#define PFCP_IE_NETWORK_INSTANCE                                                       (22)
#define PFCP_IE_SDF_FILTER                                                             (23)
#define PFCP_IE_APPLICATION_ID                                                         (24)
#define PFCP_IE_GATE_STATUS                                                            (25)
#define PFCP_IE_MBR                                                                    (26)
#define PFCP_IE_GBR                                                                    (27)
#define PFCP_IE_QER_CORRELATION_ID                                                     (28)
#define PFCP_IE_PRECEDENCE                                                             (29)
#define PFCP_IE_TRANSPORT_LEVEL_MARKING                                                (30)
#define PFCP_IE_VOLUME_THRESHOLD                                                       (31)
#define PFCP_IE_TIME_THRESHOLD                                                         (32)
#define PFCP_IE_MONITORING_TIME                                                        (33)
#define PFCP_IE_SUBSEQUENT_VOLUME_THRESHOLD                                            (34)
#define PFCP_IE_SUBSEQUENT_TIME_THRESHOLD                                              (35)
#define PFCP_IE_INACTIVITY_DETECTION_TIME                                              (36)
#define PFCP_IE_REPORTING_TRIGGERS                                                     (37)
#define PFCP_IE_REDIRECT_INFORMATION                                                   (38)
#define PFCP_IE_REPORT_TYPE                                                            (39)
#define PFCP_IE_OFFENDING_IE                                                           (40)
#define PFCP_IE_FORWARDING_POLICY                                                      (41)
#define PFCP_IE_DESTINATION_INTERFACE                                                  (42)
#define PFCP_IE_UP_FUNCTION_FEATURES                                                   (43)
#define PFCP_IE_APPLY_ACTION                                                           (44)
#define PFCP_IE_DOWNLINK_DATA_SERVICE_INFORMATION                                      (45)
#define PFCP_IE_DOWNLINK_DATA_NOTIFICATION_DELAY                                       (46)
#define PFCP_IE_DL_BUFFERING_DURATION                                                  (47)
#define PFCP_IE_DL_BUFFERING_SUGGESTED_PACKET_COUNT                                    (48)
#define PFCP_IE_PFCPSMREQ_FLAGS                                                        (49)
#define PFCP_IE_PFCPSRRSP_FLAGS                                                        (50)
#define PFCP_IE_LOAD_CONTROL_INFORMATION                                               (51)
#define PFCP_IE_SEQUENCE_NUMBER                                                        (52)
#define PFCP_IE_METRIC                                                                 (53)
#define PFCP_IE_OVERLOAD_CONTROL_INFORMATION                                           (54)
#define PFCP_IE_TIMER                                                                  (55)
#define PFCP_IE_PACKET_DETECTION_RULE_ID                                               (56)		// PDR ID
#define PFCP_IE_F_SEID                                                                 (57)
#define PFCP_IE_APPLICATION_IDS_PFDS                                                   (58)
#define PFCP_IE_PFD_CONTEXT                                                            (59)
#define PFCP_IE_NODE_ID                                                                (60)
#define PFCP_IE_PFD_CONTENTS                                                           (61)
#define PFCP_IE_MEASUREMENT_METHOD                                                     (62)
#define PFCP_IE_USAGE_REPORT_TRIGGER                                                   (63)
#define PFCP_IE_MEASUREMENT_PERIOD                                                     (64)
#define PFCP_IE_FQ_CSID                                                                (65)
#define PFCP_IE_VOLUME_MEASUREMENT                                                     (66)
#define PFCP_IE_DURATION_MEASUREMENT                                                   (67)
#define PFCP_IE_APPLICATION_DETECTION_INFORMATION                                      (68)
#define PFCP_IE_TIME_OF_FIRST_PACKET                                                   (69)
#define PFCP_IE_TIME_OF_LAST_PACKET                                                    (70)
#define PFCP_IE_QUOTA_HOLDING_TIME                                                     (71)
#define PFCP_IE_DROPPED_DL_TRAFFIC_THRESHOLD                                           (72)
#define PFCP_IE_VOLUME_QUOTA                                                           (73)
#define PFCP_IE_TIME_QUOTA                                                             (74)
#define PFCP_IE_START_TIME                                                             (75)
#define PFCP_IE_END_TIME                                                               (76)
#define PFCP_IE_QUERY_URR                                                              (77)
#define PFCP_IE_USAGE_REPORT_WITHIN_SESSION_MODIFICATION_RESPONSE                      (78)
#define PFCP_IE_USAGE_REPORT_WITHIN_SESSION_DELETION_RESPONSE                          (79)
#define PFCP_IE_USAGE_REPORT_WITHIN_SESSION_REPORT_REQUEST                             (80)
#define PFCP_IE_URR_ID                                                                 (81)
#define PFCP_IE_LINKED_URR_ID                                                          (82)
#define PFCP_IE_DOWNLINK_DATA_REPORT                                                   (83)
#define PFCP_IE_OUTER_HEADER_CREATION                                                  (84)
#define PFCP_IE_CREATE_BAR                                                             (85)
#define PFCP_IE_UPDATE_BAR_WITHIN_PFCP_SESSION_MODIFICATION_REQUEST                    (86)
#define PFCP_IE_REMOVE_BAR                                                             (87)
#define PFCP_IE_BAR_ID                                                                 (88)
#define PFCP_IE_CP_FUNCTION_FEATURES                                                   (89)
#define PFCP_IE_USAGE_INFORMATION                                                      (90)
#define PFCP_IE_APPLICATION_INSTANCE_ID                                                (91)
#define PFCP_IE_FLOW_INFORMATION                                                       (92)
#define PFCP_IE_UE_IP_ADDRESS                                                          (93)
#define PFCP_IE_PACKET_RATE                                                            (94)
#define PFCP_IE_OUTER_HEADER_REMOVAL                                                   (95)
#define PFCP_IE_RECOVERY_TIME_STAMP                                                    (96)
#define PFCP_IE_DL_FLOW_LEVEL_MARKING                                                  (97)
#define PFCP_IE_HEADER_ENRICHMENT                                                      (98)
#define PFCP_IE_ERROR_INDICATION_REPORT                                                (99)
#define PFCP_IE_MEASUREMENT_INFORMATION                                                (100)
#define PFCP_IE_NODE_REPORT_TYPE                                                       (101)
#define PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT                                         (102)
#define PFCP_IE_REMOTE_GTP_U_PEER                                                      (103)
#define PFCP_IE_UR_SEQN                                                                (104)
#define PFCP_IE_UPDATE_DUPLICATING_PARAMETERS                                          (105)
#define PFCP_IE_ACTIVATE_PREDEFINED_RULES                                              (106)
#define PFCP_IE_DEACTIVATE_PREDEFINED_RULES                                            (107)
#define PFCP_IE_FAR_ID                                                                 (108)
#define PFCP_IE_QER_ID                                                                 (109)
#define PFCP_IE_OCI_FLAGS                                                              (110)
#define PFCP_IE_PFCP_ASSOCIATION_RELEASE_REQUEST                                       (111)
#define PFCP_IE_GRACEFUL_RELEASE_PERIOD                                                (112)
#define PFCP_IE_PDN_TYPE                                                               (113)
#define PFCP_IE_FAILED_RULE_ID                                                         (114)
#define PFCP_IE_TIME_QUOTA_MECHANISM                                                   (115)
#define PFCP_IE_USER_PLANE_IP_RESOURCE_INFORMATION                                     (116)		//Reserved
#define PFCP_IE_USER_PLANE_INACTIVITY_TIMER                                            (117)
#define PFCP_IE_AGGREGATED_URRS                                                        (118)
#define PFCP_IE_MULTIPLIER                                                             (119)
#define PFCP_IE_AGGREGATED_URR_ID                                                      (120)
#define PFCP_IE_SUBSEQUENT_VOLUME_QUOTA                                                (121)
#define PFCP_IE_SUBSEQUENT_TIME_QUOTA                                                  (122)
#define PFCP_IE_RQI                                                                    (123)
#define PFCP_IE_QFI                                                                    (124)
#define PFCP_IE_QUERY_URR_REFERENCE                                                    (125)
#define PFCP_IE_ADDITIONAL_USAGE_REPORTS_INFORMATION                                   (126)
#define PFCP_IE_CREATE_TRAFFIC_ENDPOINT                                                (127)
#define PFCP_IE_CREATED_TRAFFIC_ENDPOINT                                               (128)
#define PFCP_IE_UPDATE_TRAFFIC_ENDPOINT                                                (129)
#define PFCP_IE_REMOVE_TRAFFIC_ENDPOINT                                                (130)
#define PFCP_IE_TRAFFIC_ENDPOINT_ID                                                    (131)
#define PFCP_IE_ETHERNET_PACKET_FILTER                                                 (132)
#define PFCP_IE_MAC_ADDRESS                                                            (133)
#define PFCP_IE_C_TAG                                                                  (134)
#define PFCP_IE_S_TAG                                                                  (135)
#define PFCP_IE_ETHERTYPE                                                              (136)
#define PFCP_IE_PROXYING                                                               (137)
#define PFCP_IE_ETHERNET_FILTER_ID                                                     (138)
#define PFCP_IE_ETHERNET_FILTER_PROPERTIES                                             (139)
#define PFCP_IE_SUGGESTED_BUFFERING_PACKETS_COUNT                                      (140)
#define PFCP_IE_USER_ID                                                                (141)
#define PFCP_IE_ETHERNET_PDU_SESSION_INFORMATION                                       (142)
#define PFCP_IE_ETHERNET_TRAFFIC_INFORMATION                                           (143)
#define PFCP_IE_MAC_ADDRESSES_DETECTED                                                 (144)
#define PFCP_IE_MAC_ADDRESSES_REMOVED                                                  (145)
#define PFCP_IE_ETHERNET_INACTIVITY_TIMER                                              (146)
#define PFCP_IE_ADDITIONAL_MONITORING_TIME                                             (147)
#define PFCP_IE_EVENT_QUOTA                                                      		(148)		//Event Quota
#define PFCP_IE_EVENT_THRESHOLD                                                        (149)		//Event Threshold 
#define PFCP_IE_SUBS_EVENT_QUOTA                                                       (150)		//Subsequent Event Quota
#define PFCP_IE_SUBS_EVENT_THRESHOLD                                                   (151)		//Subsequent Event Threshold
#define PFCP_IE_TRACE_INFORMATION                                                      (152)
#define PFCP_IE_FRAMED_ROUTE                                                           (153)
#define PFCP_IE_FRAMED_ROUTING                                                         (154)
#define PFCP_IE_FRAMED_IPV6_ROUTE                                                      (155)
#define PFCP_IE_EVENT_TIME_STAMP_TYPE 													156
#define PFCP_IE_AVERAGING_WINDOW_TYPE 													157
#define PFCP_IE_PAGING_POLICY_INDICATOR_TYPE 											158
#define PFCP_IE_APN_DNN_TYPE 															159
#define PFCP_IE_INTERFACE_TYPE_TYPE 													160
#define PFCP_IE_PFCPSRREQ_FLAGS_TYPE 													161
#define PFCP_IE_PFCPAUREQ_FLAGS_TYPE 													162
#define PFCP_IE_ACTIVATION_TIME_TYPE 													163
#define PFCP_IE_DEACTIVATION_TIME_TYPE 													164
#define PFCP_IE_CREATE_MAR_TYPE 														165
#define PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE 							166		//3GPP Access Forwarding Action Information
#define PFCP_IE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE 							167		//Non-3GPP Access Forwarding Action Information
#define PFCP_IE_REMOVE_MAR_TYPE 														168
#define PFCP_IE_UPDATE_MAR_TYPE 														169
#define PFCP_IE_MAR_ID_TYPE 															170
#define PFCP_IE_STEERING_FUNCTIONALITY_TYPE 											171
#define PFCP_IE_STEERING_MODE_TYPE 														172
#define PFCP_IE_WEIGHT_TYPE 															173
#define PFCP_IE_PRIORITY_TYPE 															174
#define PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_1_TYPE 						175		//Update 3GPP Access Forwarding Action Information
#define PFCP_IE_UPDATE_ACCESS_FORWARDING_ACTION_INFORMATION_2_TYPE 						176		//Update Non 3GPP Access Forwarding Action Information
#define PFCP_IE_UE_IP_ADDRESS_POOL_IDENTITY_TYPE 										177
#define PFCP_IE_ALTERNATIVE_SMF_IP_ADDRESS_TYPE 										178
#define PFCP_IE_PACKET_REPLICATION_AND_DETECTION_CARRY_ON_INFORMATION_TYPE 				179
#define PFCP_IE_SMF_SET_ID_TYPE 														180
#define PFCP_IE_QUOTA_VALIDITY_TIME_TYPE 												181

// BBF IE extensions
#define PFCP_IE_BBF_OUTER_HEADER_CREATION                                              (32770)
#define PFCP_IE_BBF_OUTER_HEADER_REMOVAL                                               (32771)
#define PFCP_IE_PPPOE_SESSION_ID                                                       (32772)


#define PFCP_CAUSE_VALUE_RESERVED                        								0
#define PFCP_CAUSE_VALUE_REQUEST_ACCEPTED                								1
#define PFCP_CAUSE_VALUE_REQUEST_REJECTED                								64
#define PFCP_CAUSE_VALUE_SESSION_CONTEXT_NOT_FOUND       								65
#define PFCP_CAUSE_VALUE_MANDATORY_IE_MISSING            								66
#define PFCP_CAUSE_VALUE_CONDITIONAL_IE_MISSING          								67
#define PFCP_CAUSE_VALUE_INVALID_LENGTH                  								68
#define PFCP_CAUSE_VALUE_MANDATORY_IE_INCORRECT          								69
#define PFCP_CAUSE_VALUE_INVALID_FORWARDING_POLICY       								70
#define PFCP_CAUSE_VALUE_INVALID_FTEID_ALLOCATION_OPTION 								71
#define PFCP_CAUSE_VALUE_NO_ESTABLISHED_PFCP_ASSOCIATION 								72
#define PFCP_CAUSE_VALUE_RULE_CREATION_MODIFICATION_FAILURE 							73
#define PFCP_CAUSE_VALUE_PFCP_ENTITY_IN_CONGESTION       								74
#define PFCP_CAUSE_VALUE_NO_RESOURCES_AVAILABLE          								75
#define PFCP_CAUSE_VALUE_SERVICE_NOT_SUPPORTED           								76
#define PFCP_CAUSE_VALUE_SYSTEM_FAILURE                  								77
	
	
#define PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IPV4         								0
#define PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IPV6         								1
#define PFCP_OUTER_HEADER_REMOVAL_UDP_IPV4              								2
#define PFCP_OUTER_HEADER_REMOVAL_UDP_IPV6              								3
#define PFCP_OUTER_HEADER_REMOVAL_IPV4                  								4
#define PFCP_OUTER_HEADER_REMOVAL_IPV6                  								5
#define PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IP           								6
#define PFCP_OUTER_HEADER_REMOVAL_VLAN_STAG             								7
#define PFCP_OUTER_HEADER_REMOVAL_SLAN_CTAG             								8


#define PFCP_PDU_SESSION_CONTAINER_TO_BE_DELETED        								1


#define PFCP_INTERFACE_ACCESS                           								0			// RAN
#define PFCP_INTERFACE_CORE                             								1			// ISP
#define PFCP_INTERFACE_SGI_N6_LAN                       								2
#define PFCP_INTERFACE_CP_FUNCTION                      								3
#define PFCP_INTERFACE_LI_FUNCTION                      								4
#define PFCP_INTERFACE_UNKNOWN                          								0xff


#define PFCP_NODE_ID_IPV4   															0
#define PFCP_NODE_ID_IPV6   															1
#define PFCP_NODE_ID_FQDN   															2


#define PFCP_DEFAULT_CHOOSE_ID 															5


#define PFCP_UE_IP_SRC     																0
#define PFCP_UE_IP_DST     																1


#define PFCP_BITRATE_LEN 																10


#define PFCP_GATE_OPEN 																	0
#define PFCP_GATE_CLOSE 																1





#endif











