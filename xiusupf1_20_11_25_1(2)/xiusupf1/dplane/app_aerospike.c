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

#include <aerospike/aerospike.h>
#include <aerospike/aerospike_index.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/aerospike_udf.h>
#include <aerospike/as_bin.h>
#include <aerospike/as_bytes.h>
#include <aerospike/as_dir.h>
#include <aerospike/as_error.h>
#include <aerospike/as_config.h>
#include <aerospike/as_key.h>
#include <aerospike/as_operations.h>
#include <aerospike/as_password.h>
#include <aerospike/as_record.h>
#include <aerospike/as_record_iterator.h>
#include <aerospike/as_sleep.h>
#include <aerospike/as_status.h>
#include <aerospike/as_string.h>
#include <aerospike/as_val.h>
#include <aerospike/aerospike_scan.h>

#include "app_aerospike.h"
#include "app_stack.h"
#include "pfcp_stack.h"

#define AS__MAX_HOST_SIZE 			1024
#define AS__MAX_KEY_STR_SIZE 		1024

#pragma pack(2)
typedef struct as_urr
{
	uint16_t count;
	uint16_t urrid[10];
} as_urr_t;


#pragma pack(2)
typedef struct as_pdr
{
	uint16_t pdrid;
} as_pdr_t;


#pragma pack(2)
typedef struct as_far
{
	uint16_t farid;
} as_far_t;


#pragma pack(2)
typedef struct as_qer
{
	uint16_t qerid;
} as_qer_t;



typedef struct app_aerospike
{
	int ipv;
	char ip[50];
	int port;
	char ns[50];
	char set[50];
	aerospike as;
	int as_connected;
	
	app_logger_t * logger;
} app_aerospike_t;

app_aerospike_t * __aerospike = NULL;

aerospike * app__as_connection() 
{
	return &__aerospike->as;
}

int app_aero__is_connected()
{
	if( __aerospike->as_connected == 1) {
		return aerospike_cluster_is_connected( &__aerospike->as);
	} else {
		return 0;
	}		
}

void app__as__connect();

void app__as__init( int ipv, char * ip, int port, char * ns, char * set, app_logger_t * logger)
{
	if(!__aerospike)
	{
		__aerospike = (app_aerospike_t *)malloc(sizeof(app_aerospike_t));
		memset( __aerospike, 0, sizeof(app_aerospike_t));
		
		__aerospike->logger = logger;
		__aerospike->port = port;
		strcpy( __aerospike->ip, ip);
		strcpy( __aerospike->ns, ns);
		strcpy( __aerospike->set, set);
		
		app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "connecting to aerospike with   ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
			__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
		
		app__as__connect();
	}
}

static void app__as__dump_bin( const as_bin * p_bin)
{
	if (! p_bin) 
	{
		return;
	}

	char * val_as_str = as_val_tostring(as_bin_get_value(p_bin));
	free(val_as_str);
}


void app__as__dump_record( const as_record * p_rec)
{
	if (! p_rec) 
	{
		return;
	}

	if (p_rec->key.valuep) 
	{
		char * key_val_as_str = as_val_tostring( p_rec->key.valuep);
		free(key_val_as_str);
	}

	// uint16_t num_bins = as_record_numbins(p_rec);

	// LOG("  generation %u, ttl %u, %u bin%s", p_rec->gen, p_rec->ttl, num_bins,
			// num_bins == 0 ? "s" : (num_bins == 1 ? ":" : "s:"));

	as_record_iterator it;
	as_record_iterator_init(&it, p_rec);

	while (as_record_iterator_has_next(&it)) 
	{
		app__as__dump_bin(as_record_iterator_next(&it));
	}

	as_record_iterator_destroy(&it);
}


uint16_t dpdk_qos__find_uplink_traffic_class( uint64_t user_speed_bytes);
uint16_t dpdk_qos__find_downlink_traffic_class( uint64_t user_speed_bytes);
void pfcp_stack___initalize_session( pfcp_session_t * pfcpSession);

int RecordCount=0;
void app__as_restore_record( const as_record * p_rec)
{
	RecordCount++;
	if (!p_rec) 
	{
		return;
	}
	
	pfcp_session_t * session = pfcp_stack___allocate_session();
	
	if(!session)
		return;
	
	pfcp_stack___initalize_session( session);

	
	session->cp_f_seid 				= as_record_get_int64( 		p_rec, 	"cpseid", 	0);
	session->up_f_seid 				= as_record_get_int64( 		p_rec, 	"upseid", 	0);
	session->nodeIPv4 				= as_integer_get(as_record_get_integer( 	p_rec, 	"nodeip4"));
	session->pdnType 				= as_integer_get(as_record_get_integer( 	p_rec, 	"pdntype"));
	session->ue_ipv4 				= as_integer_get(as_record_get_integer( 	p_rec, 	"ueipv4"));
	session->upf_ip 				= as_integer_get(as_record_get_integer( 	p_rec, 	"upf_ip"));
	session->upf_teid 				= as_integer_get(as_record_get_integer( 	p_rec, 	"upf_teid"));
	session->ran_ipv4 				= as_integer_get(as_record_get_integer( 	p_rec, 	"ran_ip"));
	session->ran_teid 				= as_integer_get(as_record_get_integer( 	p_rec, 	"ran_teid"));
	session->qer_0__gate_status		= as_integer_get(as_record_get_integer( 	p_rec, 	"qer0gate_status"));
	session->qer_0__ul_mbr			= as_record_get_int64( 	p_rec, 	"qer0ul_mbr", 0);
	session->qer_0__dl_mbr			= as_record_get_int64( 	p_rec, 	"qer0dlmbr", 0);
	session->qer_0__qfi				= as_integer_get(as_record_get_integer( 	p_rec, 	"qer0qfi"));
	session->modified_time 			= time( NULL);
	session->idle_session_hb_count	= 0;
	session->released 				= 0;


	if( session->ue_ipv4 > 0)
	{
		session->ue_ipv4_isset = 1;
	}

	as_bytes * as_byt 				= as_record_get_bytes( p_rec, "ueipv6");
	
	if( as_byt) 
	{
		if( as_byt->size == 16)
		{
			as_bytes_copy( as_byt, 0, session->ue_ipv6, as_byt->size);
			session->ue_ipv6_isset = 1;
		}
	}
	
	as_byt 				= as_record_get_bytes( p_rec, "urrids");


	if( as_byt) 
	{
		if( as_byt->size == sizeof(as_urr_t))
		{
			as_urr_t urr;
			as_bytes_copy( as_byt, 0, (uint8_t*)&urr, as_byt->size);
			
			int i = 0;
			pfcp_urr_t * sess_urr = NULL;
			
			for( i = 0; i < urr.count; i++)
			{
				sess_urr = (pfcp_urr_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_urr_t) + 1);
				
				if(sess_urr)
				{
					memset( sess_urr, 0, sizeof(sizeof(pfcp_urr_t) + 1));
					sess_urr->urr_id = urr.urrid[i];
					sess_urr->granted_volume.total = (512 * 1000); //assigning default quota, to not to send peek load on chf
					sess_urr->isallowed = 1;
					sess_urr->quota_granted_times = 1;
					sess_urr->quota_status = 0;
					
					sess_urr->session = session;
					
					if(!session->urr.head)
					{
						session->urr.head = session->urr.current = sess_urr;
					}
					else
					{
						session->urr.current->Next = sess_urr;
						session->urr.current = sess_urr;
					}
					session->urr.count++;
				}
			}
		}
	}
	
	pfcp_pdr_t * sess_pdr = (pfcp_pdr_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_pdr_t) + 1);
	
	if( sess_pdr)
	{
		memset( sess_pdr, 0, sizeof(pfcp_pdr_t) + 1);
		
		sess_pdr->session 				= session;
		sess_pdr->pdr_rule_id 			= 1;
		sess_pdr->source_interface		= 1; // core
		
		if( session->ue_ipv4_isset == 1)
		{
			sess_pdr->ipv4 = session->ue_ipv4;
			sess_pdr->ipv4_isset = 1;
		}
		
		if( session->ue_ipv6_isset == 1)
		{
			memcpy( sess_pdr->ipv6, session->ue_ipv6, 16);
			sess_pdr->ipv6_isset = 1;
		}
		
		sess_pdr->far_id = 1;
		sess_pdr->qer_id = 1;
	
		if(!session->pdr.head)
		{
			session->pdr.head = session->pdr.current = sess_pdr;
		}
		else
		{
			session->pdr.current->Next = sess_pdr;
			session->pdr.current = sess_pdr;
		}
		session->pdr.count++;
	}

	
	sess_pdr = (pfcp_pdr_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_pdr_t) + 1);
	
	if( sess_pdr)
	{
		memset( sess_pdr, 0, sizeof(pfcp_pdr_t) + 1);
		
		sess_pdr->session 				= session;
		sess_pdr->pdr_rule_id 			= 2;
		sess_pdr->source_interface		= 0; // access
		
		sess_pdr->ipv4					= session->upf_ip; 		// upf ip
		sess_pdr->teid					= session->upf_teid; 	// upf teid

		sess_pdr->outer_header_removal 			= 6;
		sess_pdr->outer_header_removal_isset 	= 1;

		sess_pdr->far_id = 2;
		sess_pdr->qer_id = 1;

		if(!session->pdr.head)
		{
			session->pdr.head = session->pdr.current = sess_pdr;
		}
		else
		{
			session->pdr.current->Next = sess_pdr;
			session->pdr.current = sess_pdr;
		}
		session->pdr.count++;
	}
	
	
	pfcp_far_t * sess_far = (pfcp_far_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_far_t) + 1);
	
	if( sess_far)
	{
		memset( sess_far, 0, sizeof(sizeof(pfcp_far_t) + 1));
		sess_far->far_id = 1;
		
		sess_far->apply_action = 2;
		sess_far->apply_action_isset = 1;		

		sess_far->destination_interface = 0;
		sess_far->destination_interface_isset = 1;
		
		
		if(!session->far.head)
		{
			session->far.head = session->far.current = sess_far;
		}
		else
		{
			session->far.current->Next = sess_far;
			session->far.current = sess_far;
		}
		session->far.count++;
		sess_far->session = session;
	}


	sess_far = (pfcp_far_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_far_t) + 1);
	
	if( sess_far)
	{
		memset( sess_far, 0, sizeof(sizeof(pfcp_far_t) + 1));
		sess_far->far_id = 2;
		
		sess_far->apply_action = 2;
		sess_far->apply_action_isset = 1;		

		sess_far->destination_interface = 1;
		sess_far->destination_interface_isset = 1;
		
		
		sess_far->outer_header_creation = 0x0100;
		sess_far->teid = session->ran_teid;
		sess_far->ipv4 = session->ran_ipv4;
		sess_far->ipv4_isset = 1;
		sess_far->outer_header_creation_isset = 1;
		
		if(!session->far.head)
		{
			session->far.head = session->far.current = sess_far;
		}
		else
		{
			session->far.current->Next = sess_far;
			session->far.current = sess_far;
		}
		session->far.count++;
		sess_far->session = session;
	}


	pfcp_qer_t * sess_qer = (pfcp_qer_t *) app_region__allocate_fr( pfcp_stack__get_pfcpmregion(), sizeof(pfcp_qer_t) + 1);

	if( sess_qer)
	{
		memset( sess_qer, 0, sizeof(sizeof(pfcp_qer_t) + 1));
		
		sess_qer->qer_id 		= 1;
		
		sess_qer->ul_mbr 		= session->qer_0__ul_mbr;
		sess_qer->dl_mbr 		= session->qer_0__dl_mbr;
		sess_qer->qfi 			= session->qer_0__qfi;
		sess_qer->gate_status	= session->qer_0__gate_status;
		sess_qer->mbr_isset 	= 1;
		
		sess_qer->ul_tc 		= dpdk_qos__find_uplink_traffic_class( sess_qer->ul_mbr);
		sess_qer->dl_tc 		= dpdk_qos__find_downlink_traffic_class( sess_qer->dl_mbr);
					
		
		if(!session->qer.head)
		{
			session->qer.head = session->qer.current = sess_qer;
		}
		else
		{
			session->qer.current->Next = sess_qer;
			session->qer.current = sess_qer;
		}
		session->qer.count++;
		sess_qer->session = session;
	}
	
	
	/*
	app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike: restoring  cp_f_seid=%lu  up_f_seid=%lu  nodeIPv4=%u  pdnType=%u  ue_ipv4=%u  upf_ip=%u  upf_teid=%u  ran_ipv4=%u ran_teid=%u  gs=%u  ul_mbr=%lu  dl_mbr=%lu  qfi=%u   %s|%s|%d", 
			session->cp_f_seid, session->up_f_seid, session->nodeIPv4,
			session->pdnType, session->ue_ipv4, session->upf_ip,
			session->upf_teid, session->ran_ipv4, session->ran_teid,
			session->qer_0__gate_status, session->qer_0__ul_mbr, session->qer_0__dl_mbr, session->qer_0__qfi,
		__FILE__, __FUNCTION__, __LINE__);	
	*/
	
	session->restored_from_as = 1;
	pfcp_stack___set_pfcp_session_idp( session);
}


void app__as_cleanup()
{
	as_error err;
	aerospike_close( &__aerospike->as, &err);
	aerospike_destroy( &__aerospike->as);
}

void app__as_reset()
{
	as_error err;
	aerospike_close( &__aerospike->as, &err);
	aerospike_destroy( &__aerospike->as);

	app__as__connect();
}


void app__as__connect()
{
	as_config config;
	as_config_init( &config);	
	
	if (! as_config_add_hosts( &config, __aerospike->ip, __aerospike->port)) 
	{
		app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "as_config_add_hosts  -- invalid host   ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
			__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);		
		
		as_event_close_loops();
		exit(-1);
	}



	//as_config_set_user( &config, g_user, g_password);
	aerospike_init( &__aerospike->as, &config);
	
	as_error err;

	if (aerospike_connect( &__aerospike->as, &err) != AEROSPIKE_OK) 
	{
		printf("aerospike_connect() returned %d - %s\n", err.code, err.message);

		app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "connecting to aerospike failed   aerospike_connect() returned %d - %s     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
			err.code, err.message, __aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);		
		
		as_event_close_loops();
		aerospike_destroy( &__aerospike->as);
		exit(-1);
	}
	else
	{
		app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "connected to aerospike sucess   aerospike_connect() returned %d - %s     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
			err.code, err.message, __aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
			
		printf("aerospike_connect() connected succesfully to aerosipke %d - %s\n", err.code, err.message);
		__aerospike->as_connected = 1;
	}
}


int totol_records_restored = 0;

bool scan_cb( const as_val * p_val, void * udata)
{
	if (! p_val) 
	{
		return true;
	}

	as_record * p_rec = as_record_fromval( p_val);

	if (! p_rec) 
	{
		return true;
	}

	app__as_restore_record( p_rec);
	totol_records_restored++;
	
	return true;
}




void app__as__remove( pfcp_session_t * session)
{
	if( __aerospike)
	{	
		aerospike * as = app__as_connection();
		as_error err;

		// char skey[50];
		// memset( skey, 0, sizeof(skey));
		// sprintf( skey, "%d-%ld", session->nodeIPv4, session->cp_f_seid);
		
		as_key key;
		//as_key_init_str( &key, __aerospike->ns, __aerospike->set, skey);
		as_key_init_int64( &key, __aerospike->ns, __aerospike->set, session->up_f_seid);

		if( aerospike_key_remove( as, &err, NULL, &key) != AEROSPIKE_OK) 
		{
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "error(%d) %s at [%s:%d]     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				err.code, err.message, err.file, err.line, __aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
		}
	}
}


int restore_completed = 0;

void app__as__restoresession()
{
	if( __aerospike)
	{
		if( app_aero__is_connected())
		{
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "app__as__restoresession()  restoring sessions      ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);

			aerospike * as = app__as_connection();
			as_error err;

			as_scan scan;
			as_scan_init( &scan, __aerospike->ns, __aerospike->set);
			
			totol_records_restored = 0;

			if (aerospike_scan_foreach( as, &err, NULL, &scan, scan_cb, NULL) != AEROSPIKE_OK) 
			{
				//printf("aerospike_scan_foreach() returned %d - %s\n", err.code, err.message);
				
				app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "aerospike_scan_foreach() returned   %d - %s      ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
					err.code, err.message, __aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
				
				as_scan_destroy( &scan);
				return;
			}
			
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "app__as__restoresession()  restored session record count=%d      ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				totol_records_restored, __aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
			
			restore_completed = 1;
		}
		else
		{
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "app__as__restoresession() NOT CONNECTED to aerospike     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
		}
	}
}


int  app__as__restore_records()
{
	return totol_records_restored;
}


int app__as__restore_completed()
{
	return restore_completed;
}


void app__as__pfcpsavesession( pfcp_session_t * session)
{
	if( __aerospike)
	{
		aerospike * as = app__as_connection();
		as_error err;

		as_record rec;
		as_record_inita(&rec, 15);

		as_bytes b[2];
		
		// char skey[50];
		// memset( skey, 0, sizeof(skey));
		// sprintf( skey, "%d-%ld", session->nodeIPv4, session->cp_f_seid);
		
		as_key key;
		//as_key_init_str( &key, __aerospike->ns, __aerospike->set, skey);
		as_key_init_int64( &key, __aerospike->ns, __aerospike->set, session->up_f_seid);
		
		// node->IPv4
		as_record_set_int64( 	&rec, 	"cpseid", 			session->cp_f_seid);
		as_record_set_int64( 	&rec, 	"upseid", 			session->up_f_seid);
		as_record_set_integer( 	&rec,  	"nodeip4", 			as_integer_new(session->nodeIPv4));
		as_record_set_integer( 	&rec,  	"pdntype", 			as_integer_new(session->pdnType));
		as_record_set_integer( 	&rec,  	"ueipv4", 			as_integer_new(session->ue_ipv4));
		as_record_set_integer( 	&rec,  	"upf_ip", 			as_integer_new(session->upf_ip));
		as_record_set_integer( 	&rec,  	"upf_teid", 		as_integer_new(session->upf_teid));
		as_record_set_integer( 	&rec,  	"ran_ip", 			as_integer_new(session->ran_ipv4));
		as_record_set_integer( 	&rec,  	"ran_teid", 		as_integer_new(session->ran_teid));	
		as_record_set_integer( 	&rec,  	"qer0gate_status", 	as_integer_new(session->qer_0__gate_status));	
		as_record_set_integer( 	&rec,  	"qer0ul_mbr", 		as_integer_new(session->qer_0__ul_mbr));	
		as_record_set_integer( 	&rec,  	"qer0dlmbr", 		as_integer_new(session->qer_0__dl_mbr));	
		as_record_set_integer( 	&rec,  	"qer0qfi", 			as_integer_new(session->qer_0__qfi));	
		
			
		as_bytes_init( &b[0], 16);
		as_bytes_set( &b[0], 0, session->ue_ipv6, 16);
		as_record_set_bytes( &rec, "ueipv6", &b[0]);

		as_urr_t urr;
		urr.count = 0;
		
		pfcp_urr_t * urritem = session->urr.head;
		
		while( urritem)
		{
			if( urritem->isremoved == 0 && urritem->urr_id > 0)
			{
				urr.urrid[urr.count] = urritem->urr_id;
				urr.count++;
			}
			urritem = urritem->Next;
		}
		
		as_bytes_init( &b[1], sizeof(as_urr_t));
		as_bytes_set( &b[1], 0, (uint8_t*) &urr, sizeof(as_urr_t));
		as_record_set_bytes( &rec, "urrids", &b[1]);
		
		

		
		//printf("ran_ipv4=%u ran_teid=%d\n",session->ran_ipv4,session->ran_teid);

		if ( aerospike_key_put( as, &err, NULL, &key, &rec) != AEROSPIKE_OK) 
		{
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_CRITICAL, "app__as__pfcpsavesession() SAVED TO FAILED aerospike     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);

			return;
		}
		else
		{
			app_logger__log( __aerospike->logger, NULL, APP_LOG__LEVEL_DEBUG, "app__as__pfcpsavesession() SAVED TO SUCCESS aerospike     ip=%s   port=%d   namespace=%s   set=%s   %s|%s|%d", 
				__aerospike->ip, __aerospike->port, __aerospike->ns, __aerospike->set, __FILE__, __FUNCTION__, __LINE__);
		}
		
		
		as_record_destroy( &rec);
	}
}








