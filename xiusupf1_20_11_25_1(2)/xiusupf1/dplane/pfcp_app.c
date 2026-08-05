#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>

#include "app_stack.h"
#include "jansson.h"
#include "pfcp_stack.h"
#include "app_endpoint.h"
#include "pfcp_types.h"
#include "pfcp_parser.h"
#include "app_command.h"

int grant_quota = 1 * (1024 * 1024);

app_ep_udp_client_t * udp_client = NULL;
app_ep_udp_client_t * cli_client = NULL;
app_data_region_t * pfcp_mregion = NULL;

typedef struct pfcp_app__session
{
	uint32_t ReqNo;
	uint32_t ResNo;
	
	uint64_t cp_f_seid;
	uint64_t up_f_seid;
	

	
} pfcp_app__session_t;
 
typedef struct pfcp_app__stack
{
	// created sessions
	pfcp_app__session_t * cshead;
	pfcp_app__session_t * cscurrent;
	pthread_mutex_t cslock;
	
	// running sessions
	pfcp_app__session_t * rnhead;
	pfcp_app__session_t * rncurrent;
	pthread_mutex_t rnlock;
	
	uint32_t seqno;
	pthread_mutex_t seqlock;
	
	app_rbtree_t * pfcp_sessions_tree;
	app_data_region_t * pfcp_sess_region;
	app_iQueue * queue;
} pfcp_app__stack_t;

pfcp_app__stack_t * capp_stack = NULL;




int  dpdk_session__ipv4_count()
{
	return 0;
}

int  dpdk_session__ipv4_add(  uint32_t ipv4, void *  data, int deleteIfExists)
{
	return 0;
}

void dpe_pkt__send_gtp_end_marker_ipv4( uint32_t ranip, uint32_t ran_teid) 
{
}


uint16_t dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes)
{
	return 0;
}


uint16_t dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes)
{
	return 0;
}

void dpdk_qos__find_user_pipe_and_q( uint64_t seid, uint16_t * pipe, uint16_t * nongbr_queue, uint16_t * gbr_queue)
{
}

uint8_t * dpe_find_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol)
{
	return NULL;	
}

uint8_t * dpe_find_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol)
{
	return NULL;
}

int dpe_add_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol, void * ip4flow)
{
	return 0;
}

int dpe_add_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol, void * ip4flow)
{
	return 0;
}

int dpe_del_ipv4flow( uint32_t ueip, uint32_t dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol)
{
	return 0;	
}

int dpe_del_ipv6flow( uint8_t * ueip, uint8_t * dstip, uint16_t ueport, uint16_t dstport, uint8_t protocol)
{
	return 0;	
}


void pfcp_stack___init_header( pfcp_message_t * pfcp_msg, uint8_t msgType, uint8_t * buffer, uint8_t hasSEID, uint64_t SEID);

void pfcp_app__recv_cli_msg( uint8_t * data, int index)
{
	app_ep_udp_message_t * msg = (app_ep_udp_message_t *)data;
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	pfcp_stack__print_buffer_wn( "cli-answer", buffer, len);
	
	app_ep__free_udp_message( msg);	
}






uint32_t pfcp_app__getseqno()
{
	pthread_mutex_lock( &capp_stack->seqlock);
	uint32_t seqno = capp_stack->seqno;
	capp_stack->seqno++;
	pthread_mutex_unlock( &capp_stack->seqlock);
	return seqno;
}

void pfcp_app__compose_and_send__heartbeat_response();

void pfcp_app___handle_heartbeat_request__buffer( app_ep_udp_message_t * msg, int tIndex)
{
	pfcp_message_t pfcp_msg;
	int sts = __up_pfcp_decode__heartbeat_request( &pfcp_msg, msg);

	if( sts == 0)
	{
		pfcp_app__compose_and_send__heartbeat_response();
	}

	__up_pfcp_free__heartbeat_request( &pfcp_msg);
	app_ep__free_udp_message( msg);
}
 
void pfcp_app__compose_and_send__session_modification_request( pfcp_app__session_t * session, uint32_t urr_id);
 
void pfcp_app__compose_and_send__session_modification_request_q( uint8_t * obj, int index)
{
	pfcp_app__compose_and_send__session_modification_request( (pfcp_app__session_t*)obj, 1);
}

void pfcp_app__compose_and_send__session_delete_request( pfcp_app__session_t * session);
 
void pfcp_app__compose_and_send__session_delete_request_q( uint8_t * obj, int index)
{
	pfcp_app__compose_and_send__session_delete_request( (pfcp_app__session_t*)obj);
}

long ser_sent = 0;
long ser_received = 0;
pthread_mutex_t ser_lock;
long smr_sent = 0;
long smr_received = 0;
pthread_mutex_t smr_lock;
long sdr_sent = 0;
long sdr_received = 0;
pthread_mutex_t sdr_lock;
long sur_sent = 0;
long sur_received = 0;
pthread_mutex_t sur_lock;
long pkt_sent = 0;
long pkt_sent_last = 0;

int send_smrequest_for_seres = 0;
int send_sdrequest_for_smres = 0;
char * ip = "192.168.149.50";			//"127.0.0.1";
int port = 8805;


void pfcp_app__recv_msg( uint8_t * data, int index)
{
	app_ep_udp_message_t * msg = (app_ep_udp_message_t *)data;
	uint16_t len = app_ep__get_len(msg);
	uint8_t * buffer = app_ep__get_buffer(msg);
	
	
	
	if( len >= 8)
	{
		uint8_t msgType = buffer[1];
		
		//printf("received pfcp message with len=%u msgType=%u  %s|%d\n", len, msgType, __FILE__, __LINE__);
		
		switch( msgType)
		{
			case PFCP_HEARTBEAT_REQUEST:
				pfcp_app___handle_heartbeat_request__buffer( msg, index);
				return;
				break;
			case PFCP_HEARTBEAT_RESPONSE:
				//pfcp_stack___handle_heartbeat_response__buffer( msg, tIndex);
				break;
			case PFCP_PFCP_PFD_MANAGEMENT_REQUEST:
				//pfcp_stack___handle_pfd_management_request__buffer( msg, tIndex);
				break;
			case PFCP_PFCP_PFD_MANAGEMENT_RESPONSE:
				//pfcp_stack___handle_pfd_management_response__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_SETUP_REQUEST:
				//pfcp_stack___handle_association_setup_request__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_SETUP_RESPONSE:
				//pfcp_stack___handle_association_setup_response__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_UPDATE_REQUEST:
				//pfcp_stack___handle_association_update_request__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_UPDATE_RESPONSE:
				//pfcp_stack___handle_association_update_response__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_RELEASE_REQUEST:
				//pfcp_stack___handle_association_release_request__buffer( msg, tIndex);
				break;
			case PFCP_ASSOCIATION_RELEASE_RESPONSE:
				//pfcp_stack___handle_association_release_response__buffer( msg, tIndex);
				break;
			case PFCP_VERSION_NOT_SUPPORTED_RESPONSE:
				//pfcp_stack___handle_version_not_supported_response__buffer( msg, tIndex);
				break;
			case PFCP_NODE_REPORT_REQUEST:
				//pfcp_stack___handle_node_report_request__buffer( msg, tIndex);
				break;
			case PFCP_NODE_REPORT_RESPONSE:
				//pfcp_stack___handle_node_report_response__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_SET_DELETION_REQUEST:
				//pfcp_stack___handle_set_deletion_request__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_SET_DELETION_RESPONSE:
				//pfcp_stack___handle_set_deletion_response__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_ESTABLISHMENT_REQUEST:
				//pfcp_stack___handle_session_establishment_request__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_ESTABLISHMENT_RESPONSE:
				//pfcp_stack___handle_session_establishment_response__buffer( msg, tIndex);
				{
					pthread_mutex_lock( &ser_lock);
					ser_received++;
					pthread_mutex_unlock( &ser_lock);
					
					uint64_t seid = pfcp_stack___get_seid( buffer);
					pfcp_app__session_t * session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
					
					//printf( "se-resp seid=%lu session=%p\n", seid, session);
					
					{
						//uint64_t seid = pfcp_stack___get_seid( buffer);
						
						
						if( session) 
						{
							pfcp_message_t pfcp_request;
							int sts = __up_pfcp_decode__message( buffer[1], &pfcp_request, msg);
							
							pfcp_session_establishment_response_t * response = &pfcp_request.u.session_establishment_response;

							if( response->up_f_seid.presence == 1)
							{
								session->up_f_seid =  app_ep__get_u64( &response->up_f_seid.data[1]);
								//printf( "up_f_seid=%lu seid=%lu\n", session->up_f_seid, seid);
								
								if( send_smrequest_for_seres == 1)
								{
									app_queue__enquee( capp_stack->queue, (uint8_t *)session, pfcp_app__compose_and_send__session_modification_request_q);
								}
							}
							__up_pfcp_free__message( &pfcp_request);
						}
					}
				}
				break;
			case PFCP_SESSION_MODIFICATION_REQUEST:
				//pfcp_stack___handle_session_modification_request__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_MODIFICATION_RESPONSE:
				//pfcp_stack___handle_session_modification_response__buffer( msg, tIndex);
				{
					pthread_mutex_lock( &smr_lock);
					smr_received++;
					pthread_mutex_unlock( &smr_lock);					
					
					{
						uint64_t seid = pfcp_stack___get_seid( buffer);
						pfcp_app__session_t * session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
						
						if( session && (send_sdrequest_for_smres == 1)) 
						{
							app_queue__enquee( capp_stack->queue, (uint8_t *)session, pfcp_app__compose_and_send__session_delete_request_q);
						}
					}
				}
				break;
			case PFCP_SESSION_DELETION_REQUEST:
				//pfcp_stack___handle_session_deletion_request__buffer( msg, tIndex);
				break;
			case PFCP_SESSION_DELETION_RESPONSE:
				//pfcp_stack___handle_session_deletion_response__buffer( msg, tIndex);
				{
					pthread_mutex_lock( &sdr_lock);
					sdr_received++;
					pthread_mutex_unlock( &sdr_lock);
					
					uint64_t seid = pfcp_stack___get_seid( buffer);
					pfcp_app__session_t * session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
					
					if( session) 
					{
						app_rbnode__free_idp( capp_stack->pfcp_sessions_tree, session->cp_f_seid);
						app_region__free( (uint8_t*)session);
					}
				}
				break;
			case PFCP_SESSION_REPORT_REQUEST:
				//pfcp_stack___handle_session_report_request__buffer( msg, tIndex);
				{
					pthread_mutex_lock( &sur_lock);
					sur_received++;
					pthread_mutex_unlock( &sur_lock);
					
					uint64_t seid = pfcp_stack___get_seid( buffer);
					pfcp_app__session_t * session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
					
					if( session) 
					{
						pfcp_message_t pfcp_response;
						pfcp_stack___init_header( &pfcp_response, PFCP_SESSION_REPORT_RESPONSE, buffer, 1, session->up_f_seid);
						
						__up_pfcp_set__u8( &pfcp_response.u.session_report_response.cause, 1);
						
						app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
						__up_pfcp_encode__message( &pfcp_response, bObj);
						app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
						app_ep__free_udp_message( bObj);
						__up_pfcp_free__message( &pfcp_response);
						
						
						pfcp_message_t pfcp_request;
						int sts = __up_pfcp_decode__message( buffer[1], &pfcp_request, msg);
						
						pfcp_session_report_request_t * request = &pfcp_request.u.session_report_request;
							
						uint32_t urr_id = 1;
						
						if( request->usage_report.presence == 1)
						{
							if( request->usage_report.urr_id.presence == 1)
							{
								urr_id = request->usage_report.urr_id.u32;
							}
						}
						
						__up_pfcp_free__message( &pfcp_request);

						pthread_mutex_lock( &sur_lock);
						sur_sent++;
						pthread_mutex_unlock( &sur_lock);
	
						pfcp_app__compose_and_send__session_modification_request( session, urr_id);
					}					
				}
				break;
			case PFCP_SESSION_REPORT_RESPONSE:
				//pfcp_stack___handle_session_report_response__buffer( msg, tIndex);
				break;
			default:
				
				break;
		}
	}
	app_ep__free_udp_message( msg);
}



void pfcp_app__compose_and_send__heartbeat_request()
{
	pfcp_message_t pfcp_heartbeat_request;
	__up_pfcp_set__header( &pfcp_heartbeat_request, PFCP_HEARTBEAT_REQUEST, 0, 0, 1);
	
	__up_pfcp_set__u32( &pfcp_heartbeat_request.u.heartbeat_response.recovery_time_stamp, 5);
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__heartbeat_request( &pfcp_heartbeat_request, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
}

void pfcp_app__compose_and_send__heartbeat_response()
{
	pfcp_message_t pfcp_heartbeat_response;
	__up_pfcp_set__header( &pfcp_heartbeat_response, PFCP_HEARTBEAT_RESPONSE, 0, 0, 1);
	
	__up_pfcp_set__u32( &pfcp_heartbeat_response.u.heartbeat_response.recovery_time_stamp, 5);
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__heartbeat_response( &pfcp_heartbeat_response, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
}

void pfcp_app__compose_and_send__pfd_management_request()
{
	pfcp_message_t pfd_management_request;
	__up_pfcp_set__header( &pfd_management_request, PFCP_PFCP_PFD_MANAGEMENT_REQUEST, 0, 0, 1);

	pfcp_pfd_management_request_t * req = &pfd_management_request.u.pfd_management_request;

	req->application_id_s_pfds.presence = 1;
	__up_pfcp_set__octet( pfcp_mregion, &req->application_id_s_pfds.application_id, "application_id", 14);
	
	// req->application_id_s_pfds.pfd_context.presence = 1;
	// __up_pfcp_set__octet( pfcp_mregion, &req->application_id_s_pfds.pfd_context.pfd_contents, "pfd_context", 11);	
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__pfd_management_request( &pfd_management_request, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
}

void pfcp_app__compose_and_send__association_setup_request()
{
	pfcp_message_t pfd_management_request;
	__up_pfcp_set__header( &pfd_management_request, PFCP_ASSOCIATION_SETUP_REQUEST, 0, 0, 1);


	pfcp_association_setup_request_t * req = &pfd_management_request.u.association_setup_request;

	__up_pfcp_set__octet( pfcp_mregion, &req->node_id, "\x00\x7F\x00\x00\x01", 5);
	__up_pfcp_set__u32( &req->recovery_time_stamp, 25);
	__up_pfcp_set__u8( &req->cp_function_features, 0);

	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__association_setup_request( &pfd_management_request, bObj);
	int sz = app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	//printf( "sent association_setup_request=%d  %s|%s|%d\n", sz, __FILE__, __FUNCTION__, __LINE__);
	app_ep__free_udp_message( bObj);
	__up_pfcp_free__message( &pfd_management_request);
}


void pfcp_app__compose_and_send__association_setup_response()
{
	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_ASSOCIATION_SETUP_RESPONSE, 0, 0, 1);

	
	pfcp_association_setup_response_t * req = &msg.u.association_setup_response;

	__up_pfcp_set__octet( pfcp_mregion, &req->node_id, "\x00\x7F\x00\x00\x01", 5);
	__up_pfcp_set__u8( &req->cause, 1);
	__up_pfcp_set__u32( &req->recovery_time_stamp, 25);
	__up_pfcp_set__octet( pfcp_mregion, &req->up_function_features, "\x00\x01", 2);


	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__association_setup_response( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
}

uint32_t uipadd = 170721282;

void pfcp_app__compose_and_send__session_establishment_request()
{
	pfcp_app__session_t * session = (pfcp_app__session_t *)app_region__allocate_fr( capp_stack->pfcp_sess_region, sizeof(pfcp_app__session_t));
	
	if(!session)
	{
		printf("allocation of pfcp session failed\n");
		return;
	}
	
	session->cp_f_seid = app_rbnode__get_next_idp( capp_stack->pfcp_sessions_tree, (uint8_t *)session);
	//printf( "ser cp_f_seid=%lu\n", session->cp_f_seid);
	
	pfcp_message_t msg;
	
	uint32_t seqno = pfcp_app__getseqno();
	__up_pfcp_set__header( &msg, PFCP_SESSION_ESTABLISHMENT_REQUEST, 1, 0, seqno);
	
	pfcp_session_establishment_request_t * req = &msg.u.session_establishment_request;
	
	__up_pfcp_set__octet( pfcp_mregion, &req->node_id, "\x00\x7F\x00\x00\x01", 5);
	
	char fseid[20];
	memset( fseid, 0, sizeof(fseid));
	fseid[0] |= 1 << 1;
	app_ep__encode__u64toc( &fseid[1], session->cp_f_seid);
	memcpy( &fseid[9], "\x7F\x00\x00\x01",4);
	__up_pfcp_set__octet( pfcp_mregion, &req->cp_f_seid, fseid, 13);
	
	//pdr 0
	req->create_pdr[0].presence = 1;
	__up_pfcp_set__u16( &req->create_pdr[0].pdr_id, 1);
	__up_pfcp_set__u32( &req->create_pdr[0].precedence, 4294967295);
	
	req->create_pdr[0].pdi.presence = 1;
	__up_pfcp_set__u8( &req->create_pdr[0].pdi.source_interface, 1);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_pdr[0].pdi.network_instance, "\x08\x69\x6e\x74\x65\x72\x6e\x65\x74", 9);

	char ueip[30];
	memset( ueip, 0, sizeof(ueip));
	ueip[0] |= 1 << 0;
	ueip[0] |= 1 << 1;
	ueip[0] |= 1 << 2;
	//memcpy( &ueip[1], "\x0A\x2D\x00\x02", 4);
	uint32_t lueip = htonl(uipadd);
	memcpy( &ueip[1], &lueip, 4);
	uipadd++;
	memcpy( &ueip[5], "\xca\xfe\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02", 16);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_pdr[0].pdi.ue_ip_address, ueip, 21);

	__up_pfcp_set__u32( &req->create_pdr[0].far_id, 1);
	__up_pfcp_set__u32( &req->create_pdr[0].qer_id, 1);


	// //pdr 1
	req->create_pdr[1].presence = 1;
	__up_pfcp_set__u16( &req->create_pdr[1].pdr_id, 2);
	__up_pfcp_set__u32( &req->create_pdr[1].precedence, 4294967295);
	
	req->create_pdr[1].pdi.presence = 1;
	__up_pfcp_set__u8( &req->create_pdr[1].pdi.source_interface, 0);
	
	char fteid[30];
	memset( fteid, 0, sizeof(fteid));
	fteid[0] |= 1 << 0;
	fteid[0] |= 1 << 1;
	memcpy( &fteid[1], "\x00\x00\x02\x01", 4);
	memcpy( &fteid[5], "\x7F\x00\x00\x07", 4);
	memcpy( &fteid[9], "\xca\xfe\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x03\x00\x01\x02", 16);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_pdr[1].pdi.local_f_teid, fteid, 25);	
	__up_pfcp_set__octet( pfcp_mregion, &req->create_pdr[1].pdi.network_instance, "\x08\x69\x6e\x74\x65\x72\x6e\x65\x74", 9);	
	__up_pfcp_set__u8( &req->create_pdr[1].pdi.qfi, 1);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_pdr[1].outer_header_removal, "\x01", 1);

	__up_pfcp_set__u32( &req->create_pdr[1].far_id, 2);
	__up_pfcp_set__u32( &req->create_pdr[1].qer_id, 1);
	
	
	//far 1
	req->create_far[0].presence = 1;
	
	// far id
	__up_pfcp_set__u32( &req->create_far[0].far_id, 1);
	
	//apply action
	__up_pfcp_set__u8( &req->create_far[0].apply_action, 2);
	
	// forwarding_parameters
	req->create_far[0].forwarding_parameters.presence = 1;
	__up_pfcp_set__u8( &req->create_far[0].forwarding_parameters.destination_interface, 0);
	

	//far 2
	req->create_far[1].presence = 1;
	
	// far id
	__up_pfcp_set__u32( &req->create_far[1].far_id, 2);
	
	//apply action
	__up_pfcp_set__u8( &req->create_far[1].apply_action, 2);
	
	// forwarding_parameters
	req->create_far[1].forwarding_parameters.presence = 1;
	__up_pfcp_set__u8( &req->create_far[1].forwarding_parameters.destination_interface, 1);
	
	
	// qer
	req->create_qer[0].presence = 1;
	
	// qer id
	//__up_pfcp_set__u32( &req->create_qer[0].qer_id, 2147483649);
	__up_pfcp_set__u32( &req->create_qer[0].qer_id, 1);
	
	// gate status
	__up_pfcp_set__u8( &req->create_qer[0].gate_status, 0);	
	
	//mbr
	__up_pfcp_set__octet( pfcp_mregion, &req->create_qer[0].maximum_bitrate, "\x00\x00\x10\x00\x00\x00\x00\x10\x00\x00", 10);
	
	// qfi
	__up_pfcp_set__u8( &req->create_qer[0].qos_flow_identifier, 1);	
	
	
	// urr
	req->create_urr[0].presence = 1;
	__up_pfcp_set__u32( &req->create_urr[0].urr_id, 1);
	__up_pfcp_set__u8( &req->create_urr[0].measurement_method, 2);
	__up_pfcp_set__u8( &req->create_urr[0].reporting_triggers, 3);
	
	char volume_quota[26];
	memset( volume_quota, 0, sizeof(volume_quota));
	//volume_quota[0] = 7;
	volume_quota[0] = 1;
	app_ep__encode__u64toc( &volume_quota[ 1], grant_quota);
	// app_ep__encode__u64toc( &volume_quota[ 9], 200);
	// app_ep__encode__u64toc( &volume_quota[17], 300);
	//__up_pfcp_set__octet( pfcp_mregion, &req->create_urr[0].volume_quota, volume_quota, 25);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_urr[0].volume_quota, volume_quota, 9);

	char volume_threshold[26];
	memset( volume_threshold, 0, sizeof(volume_threshold));
	volume_threshold[0] = 7;
	app_ep__encode__u64toc( &volume_threshold[ 1], 400);
	app_ep__encode__u64toc( &volume_threshold[ 9], 175);
	app_ep__encode__u64toc( &volume_threshold[17], 225);
	__up_pfcp_set__octet( pfcp_mregion, &req->create_urr[0].volume_threshold, volume_threshold, 25);

	__up_pfcp_set__octet( pfcp_mregion, &req->create_urr[0].time_threshold, "\x00\x00\x01\x2C", 4);

	
	
	// //pdn type
	__up_pfcp_set__u8( &req->pdn_type, 1);



	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_establishment_request( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
	__up_pfcp_free__message( &msg);
	
	pthread_mutex_lock( &ser_lock);
	ser_sent++;
	pthread_mutex_unlock( &ser_lock);
}


void pfcp_app__compose_and_send__session_establishment_response()
{
	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_SESSION_ESTABLISHMENT_RESPONSE, 1, 1, 1);

	pfcp_session_establishment_response_t * imsg = &msg.u.session_establishment_response;
	
	__up_pfcp_set__octet( pfcp_mregion, &imsg->node_id, "\x00\x7F\x00\x00\x01", 5);
	__up_pfcp_set__u8( &imsg->cause, 1);

	char fseid[20];
	memset( fseid, 0, sizeof(fseid));
	fseid[0] |= 1 << 1;
	app_ep__encode__u64toc( &fseid[1], 1);
	memcpy( &fseid[9], "\x7F\x00\x00\x07",4);
	__up_pfcp_set__octet( pfcp_mregion, &imsg->up_f_seid, fseid, 13);
	
	imsg->created_pdr[0].presence = 1;
	__up_pfcp_set__u16( &imsg->created_pdr[0].pdr_id, 1);
	
	imsg->created_pdr[1].presence = 1;
	__up_pfcp_set__u16( &imsg->created_pdr[1].pdr_id, 2);
	


	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_establishment_response( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);	
}


void pfcp_app__compose_and_send__session_modification_request( pfcp_app__session_t * session, uint32_t urr_id)
{
	pfcp_message_t msg;
	uint32_t seqno = pfcp_app__getseqno();
	
	if( session) {
		__up_pfcp_set__header( &msg, PFCP_SESSION_MODIFICATION_REQUEST, 1, session->up_f_seid, seqno);
	} else {
		__up_pfcp_set__header( &msg, PFCP_SESSION_MODIFICATION_REQUEST, 1, 47, seqno);
	}
	
	pfcp_session_modification_request_t * imsg = &msg.u.session_modification_request;
	
	imsg->update_far[0].presence = 1;
	__up_pfcp_set__u32( &imsg->update_far[0].far_id, 2);

	imsg->update_far[0].update_forwarding_parameters.presence = 1;
	//__up_pfcp_set__octet( pfcp_mregion, &imsg->update_far[0].update_forwarding_parameters.outer_header_creation, "\x01\x00\x00\x00\x00\x01\x7F\x00\x00\x01", 10);
	__up_pfcp_set__octet( pfcp_mregion, &imsg->update_far[0].update_forwarding_parameters.outer_header_creation, "\x02\x00\x00\x00\x00\x01\x7F\x00\x00\x01\x01\x00\x00\x00\x00\x01\x7F\x00\x00\x01\x00\x01", 22);


	//urr
	imsg->update_urr[0].presence = 1;
	__up_pfcp_set__u32( &imsg->update_urr[0].urr_id, urr_id);
	__up_pfcp_set__u8( &imsg->update_urr[0].measurement_method, 2);
	__up_pfcp_set__u8( &imsg->update_urr[0].reporting_triggers, 3);
	
	char volume_quota[26];
	memset( volume_quota, 0, sizeof(volume_quota));
	//volume_quota[0] = 7;
	volume_quota[0] = 1;
	app_ep__encode__u64toc( &volume_quota[ 1], grant_quota);
	//app_ep__encode__u64toc( &volume_quota[ 9], 300);
	//app_ep__encode__u64toc( &volume_quota[17], 500);
	//__up_pfcp_set__octet( pfcp_mregion, &imsg->update_urr[0].volume_quota, volume_quota, 25);
	__up_pfcp_set__octet( pfcp_mregion, &imsg->update_urr[0].volume_quota, volume_quota, 9);

	char volume_threshold[26];
	memset( volume_threshold, 0, sizeof(volume_threshold));
	volume_threshold[0] = 7;
	app_ep__encode__u64toc( &volume_threshold[ 1], 600);
	app_ep__encode__u64toc( &volume_threshold[ 9], 275);
	app_ep__encode__u64toc( &volume_threshold[17], 325);
	__up_pfcp_set__octet( pfcp_mregion, &imsg->update_urr[0].volume_threshold, volume_threshold, 25);

	__up_pfcp_set__octet( pfcp_mregion, &imsg->update_urr[0].time_threshold, "\x00\x00\x01\x2C", 4);	

	
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_modification_request( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);	
	__up_pfcp_free__message( &msg);
	
	
	pthread_mutex_lock( &smr_lock);
	smr_sent++;
	pthread_mutex_unlock( &smr_lock);	
}

void pfcp_app__compose_and_send__session_modification_response()
{
	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_SESSION_MODIFICATION_RESPONSE, 1, 1, 1);

	pfcp_session_modification_response_t * imsg = &msg.u.session_modification_response;
	
	__up_pfcp_set__u8( &imsg->cause, 1);
	
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_modification_response( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);	
}


void pfcp_app__compose_and_send__session_delete_request( pfcp_app__session_t * session)
{
	pfcp_message_t msg;
	uint32_t seqno = pfcp_app__getseqno();
	
	if( session) {
		__up_pfcp_set__header( &msg, PFCP_SESSION_DELETION_REQUEST, 1, session->up_f_seid, seqno);
	} else {
		__up_pfcp_set__header( &msg, PFCP_SESSION_DELETION_REQUEST, 1, 47, seqno);
	}

	pfcp_session_deletion_request_t * imsg = &msg.u.session_deletion_request;
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_deletion_request( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
	__up_pfcp_free__message( &msg);	
	
	pthread_mutex_lock( &sdr_lock);
	sdr_sent++;
	pthread_mutex_unlock( &sdr_lock);	
}

void pfcp_app__compose_and_send__session_delete_response()
{
	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_SESSION_DELETION_RESPONSE, 1, 1, 1);
	pfcp_session_deletion_response_t * imsg = &msg.u.session_deletion_response;
	
	__up_pfcp_set__u8( &imsg->cause, 1);
	
	
	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_deletion_response( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);	
}

void pfcp_app__compose_and_send__session_report_request()
{
	pfcp_message_t msg;
	__up_pfcp_set__header( &msg, PFCP_SESSION_REPORT_REQUEST, 1, 1, 1);
	pfcp_session_report_request_t * imsg = &msg.u.session_report_request;	

	__up_pfcp_set__u8( &imsg->report_type, 2);
	
	imsg->usage_report.presence = 1;
	__up_pfcp_set__u32( &imsg->usage_report.urr_id, 1);
	__up_pfcp_set__octet( pfcp_mregion, &imsg->usage_report.ur_seqn, "\x00\x00\x00\x01", 4);
	
	char volume_measurement[26];
	memset( volume_measurement, 0, sizeof(volume_measurement));
	volume_measurement[0] = 7;
	app_ep__encode__u64toc( &volume_measurement[ 1], 600);
	app_ep__encode__u64toc( &volume_measurement[ 9], 275);
	app_ep__encode__u64toc( &volume_measurement[17], 325);	
	__up_pfcp_set__octet( pfcp_mregion, &imsg->usage_report.volume_measurement, volume_measurement, 25);

	app_ep_udp_message_t * bObj = app_ep__allocate_udp_message();
	__up_pfcp_encode__session_report_request( &msg, bObj);
	app_ep__udp_sendto_v4( udp_client->fd, ip, port, app_ep__get_buffer(bObj), app_ep__get_len(bObj));
	app_ep__free_udp_message( bObj);
}

void pfcp_app__compose_and_send()
{
	// pfcp_app__compose_and_send__heartbeat_request();
	// pfcp_app__compose_and_send__heartbeat_response();
	// pfcp_app__compose_and_send__pfd_management_request();
	// pfcp_app__compose_and_send__association_setup_request();
	// pfcp_app__compose_and_send__association_setup_response();
	// pfcp_app__compose_and_send__session_establishment_request();
	// pfcp_app__compose_and_send__session_establishment_response();
	// pfcp_app__compose_and_send__session_modification_request( NULL, 1);
	// pfcp_app__compose_and_send__session_modification_response();
	// pfcp_app__compose_and_send__session_delete_request();
	// pfcp_app__compose_and_send__session_delete_response();
	
	// pfcp_app__compose_and_send__association_setup_request();
	// sleep(1);
	// pfcp_app__compose_and_send__session_establishment_request();
	// sleep(1);
	// pfcp_app__compose_and_send__session_modification_request();
	// sleep(1);
	// pfcp_app__compose_and_send__session_delete_request();
	// sleep(1);
	// pfcp_app__compose_and_send__session_report_request();
}



void * pfcp_app__perf( void * args)
{
	long i = 0;
	
	while(1)
	{
		long cpkt = pkt_sent - pkt_sent_last;
		
		printf("establishment sent=%-10lu received=%-10lu\n", ser_sent, ser_received);
		printf("modification  sent=%-10lu received=%-10lu\n", smr_sent, smr_received);
		printf("delete        sent=%-10lu received=%-10lu\n", sdr_sent, sdr_received);
		printf("usage         sent=%-10lu received=%-10lu\n", sur_sent, sur_received);
		printf("pkt           sent=%-10lu  current=%-10lu\n", pkt_sent, cpkt);
		printf("------------------------------------------------------------------------------------------------------ %ld\n", i);
		
		pkt_sent_last = pkt_sent;
		
		i++;
		sleep(1);
	}
	return NULL;
}

void pfcp_app__compose_and_send__perodic_load()
{
	pfcp_app__compose_and_send__association_setup_request();
	sleep(2);
	send_smrequest_for_seres = 1;
	send_sdrequest_for_smres = 1;

	int iterations = 1000;
	int ic = 0;
	int sub_iterations = 300;
	int sic = 0;
	
	while( ic < iterations)
	{
		while( sic < sub_iterations)
		{
			pfcp_app__compose_and_send__session_establishment_request();
			sic++;
		}
		
		sic = 0;
		ic++;
		sleep(1);
		
	}
}


uint64_t cp_seid_begin_no = 0;

void pfcp_app__data_usage_test()
{
	pfcp_app__compose_and_send__association_setup_request();
	sleep(2);
	send_smrequest_for_seres = 0;
	send_sdrequest_for_smres = 0;

	int iterations = 1;
	int ic = 0;
	
	
	while( ic < iterations)
	{
		pfcp_app__compose_and_send__session_establishment_request();
		ic++;
	}		
	
	sleep(1);

	ic = 0;
	pfcp_app__session_t * session = NULL;
	
	

	uint64_t seid = 0;
	int ti = 0;
	
	
	
	while( ti < 400)
	{		
		while( ic < iterations)
		{
			seid = (cp_seid_begin_no + ic);
			session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
			
			//printf( "seid=%lu session=%p \n", seid, session);
			
			if( session)
			{
				if( (ti % 2) == 0)
				{
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 1, 51000, 0);
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 2, 0, 52000);
				}
				else
				{
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 1, 0, 52500);
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 2, 53500, 0);					
				}
			}
			
			ic++;
			sleep(1);
		}
		ic = 0;
		ti++;
	}
	
	
	ic = 0;
	while( ic < iterations)
	{
		seid = (cp_seid_begin_no + ic);
		session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
		
		if( session)
		{
			pfcp_app__compose_and_send__session_delete_request( session);
		}
		
		ic++;
		sleep(1);
	}
}

void pfcp_app__load_test()
{
	pfcp_app__compose_and_send__association_setup_request();
	sleep(2);
	send_smrequest_for_seres = 0;
	send_sdrequest_for_smres = 0;
	pfcp_app__session_t * session = NULL;
	uint64_t seid = 0;

	int send_delete = 0;
	int send_data_load = 0;

	int sessions_count = 300000;
	int data_used_times = 1000;
	int ic = 0;
	int sec_counter = 0;
	int per_second = 500;
	
	//single packet test
	sessions_count = 1;
	//data_used_times = 100;
	data_used_times = 0;
	
	while( ic < sessions_count)
	{
		pfcp_app__compose_and_send__session_establishment_request();
		
		sec_counter++;
		if( sec_counter == per_second) 
		{
			sleep(1);
			sec_counter = 0;
		}
		ic++;
	}		
	
	ic = 0;
	if( send_data_load == 1)
	{
		ic = 0;
		sleep(1);
		sec_counter = 0;
		
		int idu = 0;
		
		
		//----------------------
		per_second = 1000;
		
		for( idu = 0; idu < data_used_times; idu++)
		{
			ic = 0;
			while( ic < sessions_count)
			{
				sec_counter++;
				if( sec_counter == per_second) 
				{
					sec_counter = 0;
					sleep(1);
				}
				
				seid = (cp_seid_begin_no + ic);
				session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
			
				if( session)
				{
					//data usage
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 1, 51000, 0);
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 1, 0, 51500);
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 2, 0, 52000);
					app_cmd__send_usage( cli_client->fd, "127.0.0.1", 9025, session->up_f_seid, 2, 52500, 0);
					pkt_sent += 4;
				}
			
				ic++;
			}
			printf("data loop completed time=%d of %d\n", idu, data_used_times);
			sleep(1);
		}	
	}

	
	if( send_delete == 1)
	{
		ic = 0;
		sleep(20);
		sec_counter = 0;
		
		while( ic < sessions_count)
		{
			seid = (cp_seid_begin_no + ic);
			session = (pfcp_app__session_t *)app_rbnode__find_idp( capp_stack->pfcp_sessions_tree, seid);
			
			if( session)
			{
				pfcp_app__compose_and_send__session_delete_request( session);
			}
			
			sec_counter++;
			if( sec_counter == per_second) 
			{
				sleep(1);
				sec_counter = 0;
			}
			ic++;
		}	
	}
	
	sleep(1);	
}

// export LD_LIBRARY_PATH=.
// gcc -g3 pfcp_app.c app_stack.c app_endpoint.c pfcp_parser.c pfcp_stack.c -o pfcp_app -I../jansson-2.13/src/ -lpthread -ljansson
int main( int argc, char* argv[])
{
	printf("pfcp app\n");
	app_timer__init( 10000);
	app_ep__init_udp( 1600, 2000);
	
	capp_stack = (pfcp_app__stack_t *)malloc(sizeof(pfcp_app__stack_t));
	memset( capp_stack, 0, sizeof(pfcp_app__stack_t));
	
	// created sessions
	capp_stack->cshead = NULL;
	capp_stack->cscurrent = NULL;
	pthread_mutex_init( &capp_stack->cslock, NULL);
	
	// running sessions
	capp_stack->rnhead = NULL;
	capp_stack->rncurrent = NULL;
	pthread_mutex_init( &capp_stack->rnlock, NULL);
	
	capp_stack->seqno = 1;
	pthread_mutex_init( &capp_stack->seqlock, NULL);
	
	pkt_sent = 0;
	pkt_sent_last = 0;

	pthread_mutex_init( &ser_lock, NULL);	
	pthread_mutex_init( &smr_lock, NULL);	
	pthread_mutex_init( &sdr_lock, NULL);	
	pthread_mutex_init( &sur_lock, NULL);	
	
	
	uint32_t totalSessionCount = 200000;
	cp_seid_begin_no = totalSessionCount;

	capp_stack->pfcp_sess_region = app_region__create();
	app_region__add_pool( capp_stack->pfcp_sess_region, "PSESS", sizeof(pfcp_app__session_t), totalSessionCount);
	
	capp_stack->pfcp_sessions_tree = app_rbnode__create_idp( totalSessionCount, totalSessionCount);
	capp_stack->queue = app_queue__create( "recvmq", totalSessionCount, 20, NULL);


	pfcp_mregion = app_region__create();
	app_region__add_pool( pfcp_mregion, "25", 		25, 	500000);
	app_region__add_pool( pfcp_mregion, "50", 		50, 	500000);
	app_region__add_pool( pfcp_mregion, "100", 		100, 	200000);
	app_region__add_pool( pfcp_mregion, "200", 		200, 	200000);
	app_region__add_pool( pfcp_mregion, "500", 		500, 	200000);
	app_region__add_pool( pfcp_mregion, "1024", 	1024, 	200000);
	app_region__add_pool( pfcp_mregion, "2048", 	2048, 	 10000);
	
	pfcp_stack___init( 4, "127.0.0.1", "0", 0x7F000001, NULL, pfcp_mregion, 5, 15, totalSessionCount, 0, 0);
	udp_client = app_ep__create_udpv4_client( "127.0.0.1", 8806, 1600, 10, pfcp_app__recv_msg);
	//udp_client = app_ep__create_udpv4_client( "192.168.149.50", 8806, 1600, 10, pfcp_app__recv_msg);
	cli_client = app_ep__create_udpv4_client( "127.0.0.1", 9026, 1600, 10, pfcp_app__recv_cli_msg);
	




	int iRet;
	
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	iRet = pthread_create( &s_pthread_id, &attr, pfcp_app__perf, NULL);
	
	
	sleep(1);
	//app_ep__udp_sendto_v4( udp_client->fd, "127.0.0.1", 8805, "ANIL", 4);
	//pfcp_app__compose_and_send();
	//pfcp_app__compose_and_send__perodic_load();
	//app_ep__udp_sendto_v4( cli_client->fd, "127.0.0.1", 9025, "CLI-COMMAND", 11);
	//pfcp_app__data_usage_test();
	pfcp_app__load_test();
	
	while(1) {
		sleep(1);
	}
	return 0;
}








