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
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "app.h"
#include "app_stack.h"
#include "jansson.h"
#include "pfcp_stack.h"
#include "app_endpoint.h"
#include "pfcp_stack.h"
#include "app_command.h"
#include "app_aerospike.h"
#include "ls.h"
#include "eng.h"

// #include <prom.h>
// #include <promhttp.h> 
// #include <microhttpd.h>
// #include <prom_alloc.h>
// #include "prom_collector.h"
// #include "prom_metric.h"
 #include "metrics_link.h"




#pragma pack(4)
typedef struct dp_tdd_quota
{
	struct dp_tdd_quota * Next;
	
	uint32_t Id;
	uint32_t Bytes;
	
} dp_tdd_quota_t;


typedef struct dp_application
{
	char systemName[25];
	int InstanceId;
	int DisableDPDK;
	
	char pfcp_IPv4[20];
	char pfcp_IPv6[50];
	int pfcp_IPv4_isset;
	int pfcp_IPv6_isset;
	struct in_addr pfcp_inaddr;
	struct in6_addr pfcp_in6addr;
	int pfcp_Port;
	int pfcp_NodeId;
	int pfcp_MaxSessions;
	int pfcp_SEIDStartNumber;
	int pfcp_UDPBufferSize;
	int pfcp_UDPRawPoolSize;
	int EnableUEIPv6;
	int DisablePROMETHEUS;
	app_ep_udp_server_t * udp_ipv4server;
	app_ep_udp_server_t * udp_ipv6server;
	
	app_rbtree_t * pfcp_sessions_tree;
	app_data_region_t * pfcp_session_region;
	app_data_pool_t * pfcp_session_pool;
	app_data_pool_t * pfcp_session_ip_pool;


	app_data_region_t 	* ippool_region;
	app_data_pool_t 	* ippool_ipv4;
	app_data_pool_t 	* ippool_ipv6;
	app_data_pool_t 	* ippool_port;
	
	
	int adminstrative_Port;
	
	int perfLogInterval;
	int perfLogBufferLogCounter;
	app_logger_t * appLogger;
	app_logger_t * deviceLogger;
	app_logger_t * l7Logger;
	app_logger_t * pfcpLogger;
	app_logger_t * packetLogger;
	app_logger_t * perfLogger;
	
	app_data_region_t * app_mregion;
	
	//ls_uint_table_t * teid_table;
	
	int aerospike_Enabled;
	int aerospike_Port;
	char aerospike_ip[30];
	char aerospike_user[30];
	char aerospike_pwd[30];
	char aerospike_set[40];
	char aerospike_namespace[40];
	
	int prometheus_Enabled;
	int prometheus_Port;
	char prometheus_ip[30];
	
	uint32_t upf_ipv4;
	uint8_t upf_ipv6[16];
	
	int initDPI;
	int DPIFlowCount;

	
	uint32_t TDD_Enabled;
	dp_tdd_quota_t TDD_Quota[6];

	struct 
	{
		int Enable;
		int Create;
		int Count;
		uint32_t SeidBegin;
		uint32_t IPv4Begin;
		uint32_t TeidBegin;
		uint8_t  IPv6Begin[16];
		uint32_t NoOfSessionsPerIP;
		uint32_t RGCount;
		uint32_t IPFlowCount;
	} TDD_Sessions;


	int IP4FlowPoolCount;
	int IP6FlowPoolCount;
	int PortPoolCount;

	int DefaultRGId;
	char adcFile[100];
	char engFile[249];
	
} dp_application_t;

dp_application_t * dp_instance = NULL;


dp_application_t * __dp_application_getInstance()
{
	return dp_instance;
}

typedef uint64_t (*fp_upses__get_seid)( void * sPtr);
typedef uint64_t (*fp_upses__get_quota)( void * sPtr, uint16_t rgid);
typedef uint64_t (*fp_upses__record_usage)( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink);
typedef uint8_t * (*fp_upses__get_dpi_session)( uint64_t seid);
typedef uint8_t * (*fp_upses__get_pfcp_session)( uint64_t seid);
typedef int (*fp_upses__get_ohi)( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no);
typedef void (*fp_logger)(int logtype, int logcat, int logLevel, char * mLogMessage);

void dpdk_session__set_fp_seid( fp_upses__get_seid pfp_get_seid);
void dpdk_session__set_fp_quota( fp_upses__get_quota pfp_get_quota);
void dpdk_session__set_fp_usage( fp_upses__record_usage pfp_record_usage);
void dpdk_session__set_fp_dpi_session( fp_upses__get_dpi_session pfp_get_dpi_session);
void dpdk_session__set_fp_pfcp_session( fp_upses__get_pfcp_session pfp_get_pfcp_session);
void dpdk_session__set_fp_ohi( fp_upses__get_ohi pfp_get_ohi);
void dpdk__set_logger_m( fp_logger flog, fp_logger plog);
void dpdk__init(int argc, char **argv);
void dpdk__initexit();


void app_packetlog( int logtype, int logcat, int logLevel, char * mLogMessage)
{
	//printf( "1 -> %s\n", mLogMessage);
	app_logger__log( dp_instance->packetLogger, NULL, logLevel, mLogMessage);
}


void app_perflog( int logtype, int logcat, int logLevel, char * mLogMessage)
{
	app_logger__log( dp_instance->perfLogger, NULL, logLevel, mLogMessage);
}


uint8_t * 	pfcp_session__get_dpi_session( uint64_t seid);
uint8_t * 	pfcp_session__get_pfcp_session( uint64_t seid);
uint64_t 	pfcp_session__get_seid( void * sPtr);
uint64_t 	pfcp_session__get_quota( void * sPtr, uint16_t rgid);
uint64_t 	pfcp_session__record_usage( void * sPtr, uint16_t rgid, uint64_t uplink, uint64_t downlink);
int 		pfcp_session__session_ohi( void * sPtr, uint32_t * teid, uint8_t * ipv, uint32_t * ranip, uint32_t * upfip, uint8_t ** ranip6, uint8_t ** upfip6, char ** ranmac, char ** upfmac, int * gtpHasSQN, uint16_t * gtp_seq_no);




int app_config__get_int( json_t * json_config, char * key, int defval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}


char * app_config__get_str( json_t * json_config, char * key)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		//int kv = json_string_length( jObj);
		return (char *)json_string_value( jObj);
	}		
	return NULL;
}




void app_config__set_str( json_t * json_config, char * key, char * vItem, int maxlen)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		int kv = json_string_length( jObj);
		const char * v = json_string_value( jObj);
		if( kv < maxlen)
		{
			memcpy( vItem, v, kv);
		}
		else
		{
			memcpy( vItem, v, maxlen);
		}
	}
}

void app_config__parse_system_info( json_t * json_config)
{
	json_t * systemObj = json_object_get( json_config, "System");
	
	if( systemObj)
	{
		app_config__set_str( systemObj, "Name", dp_instance->systemName, 25);
		dp_instance->InstanceId = app_config__get_int( systemObj, "InstanceId", 0);
		int timerCount = app_config__get_int( systemObj, "TimersCount", 5000);
		app_timer__init( timerCount);
		
		dp_instance->perfLogInterval 			= app_config__get_int( systemObj, "PerfLogInterval", 5);
		dp_instance->perfLogBufferLogCounter	= app_config__get_int( systemObj, "PerfLogBufferLogCounter", 2);
		dp_instance->DisableDPDK				= app_config__get_int( systemObj, "DisableDPDK", 0);
		
		if( dp_instance->perfLogInterval <= 0) {
			dp_instance->perfLogInterval = 1;
		}

		if( dp_instance->perfLogInterval > 5) {
			dp_instance->perfLogInterval = 5;
		}

		if( dp_instance->perfLogBufferLogCounter <= 1) {
			dp_instance->perfLogBufferLogCounter = 2;
		}
		
		if( dp_instance->perfLogBufferLogCounter > 10) {
			dp_instance->perfLogBufferLogCounter = 10;
		}		
		
		dp_instance->upf_ipv4 = app_config__get_int( systemObj, "UPF_IPv4", 0);
		app_config__set_str( systemObj, "UPF_IPv6", (char *)dp_instance->upf_ipv6, 16);


		dp_instance->DefaultRGId = app_config__get_int( systemObj, "DefaultRGId", 1);
		app_config__set_str( systemObj, "DPIAdcFile", (char *)dp_instance->adcFile, 100);
		app_config__set_str( systemObj, "DPDKFile", (char *)dp_instance->engFile, 100);
		
		
		//printf( "adc-file=%s\n", dp_instance->adcFile);
		
		
		dp_instance->app_mregion = app_region__create();
		
		json_t * buffers = json_object_get( systemObj, "Buffers");
		
		if( buffers)
		{
			if( json_is_array( buffers))
			{
				json_t * bobj = NULL;
				int bCount = json_array_size( buffers);
				int i = 0;
				
				char * key = NULL;
				int Count = 0;
				int Size = 0;
				
				for( i = 0; i < bCount; i++)
				{
					bobj = json_array_get( buffers, i);
					key = app_config__get_str( bobj, "Name");
					Count = app_config__get_int( bobj, "Count", 0);
					Size = app_config__get_int( bobj, "Size", 0);
					
					//printf("Key=%s Count=%d\n", key, Count);
					
					if( Size > 0 && Count > 0)
					{
						app_region__add_pool( dp_instance->app_mregion, key, Size, Count);
					}
				}
				
				// printf("app_mregion=%p\n", dp_instance->app_mregion);
				// uint8_t * dPtr = app_region__allocate_fr( dp_instance->app_mregion, 75);
				// app_region__info( dPtr);
			}
		}
		
		printf( "System-Name: %s, InstanceId: %d, TimerCount: %d  \n", dp_instance->systemName, dp_instance->InstanceId, timerCount);
		
		char upf_sIPv4[20];
		char upf_dIPv6[50];		
		memset( upf_sIPv4, 0, sizeof(upf_sIPv4));
		memset( upf_dIPv6, 0, sizeof(upf_dIPv6));

		app_config__set_str( systemObj, "UPF_IPv4", upf_sIPv4, 20);
		app_config__set_str( systemObj, "UPF_IPv6", upf_dIPv6, 50);
		
		 
		int rval = 0;
		
		if( strlen(upf_sIPv4) > 0)
		{
			if ( (rval = inet_pton( AF_INET, upf_sIPv4, &dp_instance->upf_ipv4)) <= 0) 
			{
				printf("Invalid UPF IPv4 Address: %s\n", upf_sIPv4);
				exit(EXIT_FAILURE);
			}
		}
		
		if( strlen(upf_dIPv6) > 0)
		{
			if ( (rval = inet_pton( AF_INET6, upf_dIPv6, &dp_instance->pfcp_IPv6)) == 0) 
			{
				printf("Invalid UPF IPv6 Address: %s\n", upf_dIPv6);
				exit(EXIT_FAILURE);
			}
		}
		
		dp_instance->upf_ipv4 		= htonl(dp_instance->upf_ipv4);
		
		dp_instance->initDPI 		= app_config__get_int( systemObj, "InitDPI", 0);
		dp_instance->DPIFlowCount 	= app_config__get_int( systemObj, "DPIFlowCount", 0);
		//app_config__set_str( systemObj, "DPIAdcFile", dp_instance->DPIADCFile, 249);
	
		dp_instance->IP4FlowPoolCount 		= app_config__get_int( systemObj, "IP4FlowPoolCount", 1);
		dp_instance->IP6FlowPoolCount 		= app_config__get_int( systemObj, "IP6FlowPoolCount", 1);
		dp_instance->PortPoolCount 			= app_config__get_int( systemObj, "PortPoolCount", 1);
	}
}

void app_config__parse_pfcp_info( json_t * json_config)
{
	json_t * pfcpObj = json_object_get( json_config, "PFCP");
	
	if( pfcpObj)
	{
		app_config__set_str( pfcpObj, "IPv4", dp_instance->pfcp_IPv4, 20);
		app_config__set_str( pfcpObj, "IPv6", dp_instance->pfcp_IPv6, 50);
		dp_instance->pfcp_Port 				= app_config__get_int( pfcpObj, "Port", 8805);	
		dp_instance->pfcp_NodeId 			= app_config__get_int( pfcpObj, "NodeId", 0);
		dp_instance->pfcp_MaxSessions 		= app_config__get_int( pfcpObj, "MaxSessions", 0);
		dp_instance->pfcp_SEIDStartNumber 	= app_config__get_int( pfcpObj, "SEIDStartNumber", 0);
		dp_instance->pfcp_UDPBufferSize 	= app_config__get_int( pfcpObj, "UDPBufferSize", 0);
		dp_instance->pfcp_UDPRawPoolSize 	= app_config__get_int( pfcpObj, "UDPRawPoolSize", 0);
		dp_instance->EnableUEIPv6 			= app_config__get_int( pfcpObj, "EnableUEIPv6", 0);
		dp_instance->DisablePROMETHEUS		= app_config__get_int( pfcpObj, "DisablePROMETHEUS", 0);
		
		int iUDPWThreadCount				= app_config__get_int( pfcpObj, "UDPWThreadCount", 10);
		int iFSMThreads						= app_config__get_int( pfcpObj, "FSMThreads", 10);
		int iHeartbeatSeconds				= app_config__get_int( pfcpObj, "HeartbeatSeconds", 10);
		int iMaxTPS							= app_config__get_int( pfcpObj, "MaxTPS", 2000);
		int iPACE2_Enabled					= app_config__get_int( pfcpObj, "PACE2_Enabled", 0);
		int iEnableUPIR						= app_config__get_int( pfcpObj, "EnableUPIR", 0);

		
		if( dp_instance->pfcp_MaxSessions > 500000)
		{
			printf("MaxSessions supported 500000, configured as %d\n", dp_instance->pfcp_MaxSessions);
			exit(0);
		}
		
		if( iUDPWThreadCount < 5) 
		{
			iUDPWThreadCount = 5;
		}
		
		if( iFSMThreads < 5) 
		{
			iFSMThreads = 5;
		}
		
		if( dp_instance->pfcp_MaxSessions > 100000 && iFSMThreads < 20)
		{
			iFSMThreads = 10;
		}
		
		if( dp_instance->pfcp_MaxSessions > 200000 && iFSMThreads < 50)
		{
			iFSMThreads = 50;
		}

		
		if( iHeartbeatSeconds < 10) 
		{
			iHeartbeatSeconds = 10;
		}
		
		// struct in_addr pfcp_inaddr;
		// struct in6_addr pfcp_in6addr;
		int rval = 0;
		
		
		
		if( strlen(dp_instance->pfcp_IPv4) > 0)
		{
			if ( (rval = inet_pton( AF_INET, dp_instance->pfcp_IPv4, &dp_instance->pfcp_inaddr)) <= 0) 
			{
				printf("Invalid PFCP IPv4 Address: %s\n", dp_instance->pfcp_IPv4);
				exit(EXIT_FAILURE);
			}

			dp_instance->pfcp_IPv4_isset = 1;
			pfcp_stack___init( 4, dp_instance->pfcp_IPv4, "", (int)dp_instance->pfcp_inaddr.s_addr, NULL, dp_instance->app_mregion, iFSMThreads, iHeartbeatSeconds, dp_instance->pfcp_MaxSessions, dp_instance->EnableUEIPv6, iPACE2_Enabled);
		}
		else if( strlen(dp_instance->pfcp_IPv6) > 0)
		{
			if ( (rval = inet_pton( AF_INET6, dp_instance->pfcp_IPv6, &dp_instance->pfcp_in6addr)) == 0) 
			{
				printf("Invalid PFCP IPv6 Address: %s\n", dp_instance->pfcp_IPv6);
				exit(EXIT_FAILURE);
			}
	
			dp_instance->pfcp_IPv6_isset = 1;
			pfcp_stack___init( 6, "", dp_instance->pfcp_IPv6, 0, dp_instance->pfcp_in6addr.s6_addr, dp_instance->app_mregion, iFSMThreads, iHeartbeatSeconds, dp_instance->pfcp_MaxSessions, dp_instance->EnableUEIPv6, iPACE2_Enabled);
		}
		
		
		if( iMaxTPS > 5000)
		{
			iMaxTPS = 5000;
		}
		
		
		pfcp_stack___set_pfcp_maxtps( iMaxTPS);

		if( iEnableUPIR == 1)
		{
			pfcp_stack___set_upir( 1);
		}
		
		
		//int HeartbeatSeconds = app_config__get_int( pfcpObj, "HeartbeatSeconds", 10);	
		 //pfcp_stack__set_upf_ip( uint32_t ipv4, uint8_t * ipv6);
		
		printf("PFCP: IPv4:%s  IPv6:%s  NodeId:%d  MaxSessions:%d  SEIDStartNumber:%d\n", 
			dp_instance->pfcp_IPv4, dp_instance->pfcp_IPv6, dp_instance->pfcp_NodeId, 
			dp_instance->pfcp_MaxSessions, dp_instance->pfcp_SEIDStartNumber);

		if( strlen(dp_instance->upf_ipv6) > 0)
		{
			uint8_t upfip[16];

			if ( (rval = inet_pton( AF_INET6, dp_instance->pfcp_IPv6, (char *)&upfip)) == 0) 
			{
				printf("Invalid UPF IPv6 Address: %s\n", dp_instance->pfcp_IPv6);
				exit(EXIT_FAILURE);
			}
			
			pfcp_stack__set_upf_ip( dp_instance->upf_ipv4, upfip);
		}
		else
		{
			pfcp_stack__set_upf_ip( dp_instance->upf_ipv4, NULL);
		}
		
		
		app_ep__init_udp( dp_instance->pfcp_UDPBufferSize, dp_instance->pfcp_UDPRawPoolSize);
		
		if( dp_instance->pfcp_IPv4_isset > 0) {
			dp_instance->udp_ipv4server = app_ep__create_udpv4_server( dp_instance->pfcp_IPv4, dp_instance->pfcp_Port, dp_instance->pfcp_UDPBufferSize, iUDPWThreadCount, pfcp_stack__buffer_handler);
			pfcp_stack__set_pfcp_ipv4server( dp_instance->udp_ipv4server);
			printf("Started UPF-PFCP: IPv4 Address: %s:%d\n", dp_instance->pfcp_IPv4, dp_instance->pfcp_Port);
		} else if( dp_instance->pfcp_IPv6_isset > 0) {
			dp_instance->udp_ipv6server = app_ep__create_udpv6_server( dp_instance->pfcp_IPv6, dp_instance->pfcp_Port, dp_instance->pfcp_UDPBufferSize, iUDPWThreadCount, pfcp_stack__buffer_handler);
			pfcp_stack__set_pfcp_ipv6server( dp_instance->udp_ipv6server);
			printf("Started UPF-PFCP: IPv6 Address: %s:%d\n", dp_instance->pfcp_IPv6, dp_instance->pfcp_Port);
		}
		
		
		
		
		json_t * cli = json_object_get( json_config, "Administration");
		
		if( cli)
		{
			char IP[50];
			memset( IP, 0, sizeof(IP));
			app_config__set_str( cli, "IP", IP, 50);
			int cliPort = app_config__get_int( cli, "Port", 9025);
			int wThreads = app_config__get_int( cli, "WThreads", 3);
			
			app_cmd__init( 4, IP, cliPort, wThreads);
		}
	}
}	

void app_config__parse_adminstrative_info( json_t * json_config)
{
	json_t * adminstrativeObj = json_object_get( json_config, "Administration");
	
	if( adminstrativeObj)
	{
		dp_instance->adminstrative_Port = app_config__get_int( adminstrativeObj, "Port", 0);
		printf( "Administration-Port:%d \n", dp_instance->adminstrative_Port);
	}		
}

void app_config__parse_aerospike( json_t * json_config)
{
	json_t * aerospikeObj = json_object_get( json_config, "Aerospike");

	if( aerospikeObj)
	{
		dp_instance->aerospike_Enabled = app_config__get_int( aerospikeObj, "Enabled", 0);
		
		if( dp_instance->aerospike_Enabled == 1)
		{
			dp_instance->aerospike_Port = app_config__get_int( aerospikeObj, "Port", 0);
			
			memset( dp_instance->aerospike_ip, 0, sizeof(dp_instance->aerospike_ip));
			app_config__set_str( aerospikeObj, "ServerIP", dp_instance->aerospike_ip, 30);

			memset( dp_instance->aerospike_set, 0, sizeof(dp_instance->aerospike_set));
			app_config__set_str( aerospikeObj, "Set", dp_instance->aerospike_set, 30);

			memset( dp_instance->aerospike_namespace, 0, sizeof(dp_instance->aerospike_namespace));
			app_config__set_str( aerospikeObj, "Namespace", dp_instance->aerospike_namespace, 30);
			
			printf("aerospike port=%d ip=%s set=%s ns=%s\n",
				dp_instance->aerospike_Port, dp_instance->aerospike_ip, dp_instance->aerospike_set, dp_instance->aerospike_namespace);
		}
	}
}

void app_config__parse_prometheus( json_t * json_config)
{
	json_t * prometheusObj = json_object_get( json_config, "Prometheus");
	
	if(prometheusObj)
	{
		dp_instance->prometheus_Enabled = app_config__get_int( prometheusObj, "Enabled", 0);
		
		if( dp_instance->prometheus_Enabled == 1)
		{
			dp_instance->prometheus_Port = app_config__get_int( prometheusObj, "Port", 0);
			
			memset( dp_instance->prometheus_ip, 0, sizeof(dp_instance->prometheus_ip));
			app_config__set_str( prometheusObj, "ServerIP", dp_instance->prometheus_ip, 30);
			
			printf("prometheus port=%d ip=%s s\n",dp_instance->prometheus_Port, dp_instance->prometheus_ip);
		}
	}
}



void app_config__parse_loginstance( json_t * logInstance, app_logger_t ** appLogger)
{
	char folder[249];
	memset( folder, 0, sizeof(folder));
	char prefix[10];
	memset( prefix, 0, sizeof(prefix));	
	char ext[10];
	memset( ext, 0, sizeof(ext));	
	
	int enabled = app_config__get_int( logInstance, "Enabled", 0);
	int level   = app_config__get_int( logInstance, "Level", 0);
	int maxSize   = app_config__get_int( logInstance, "MaxSize", 3145728);
	
	app_config__set_str( logInstance, "Folder", folder, 249);
	app_config__set_str( logInstance, "Prefix", prefix, 10);
	app_config__set_str( logInstance, "Ext", ext, 10);
	
	*appLogger = app_logger__create( folder, prefix, ext, maxSize, level);
}



void app_config__parse_log_info( json_t * json_config)
{
	json_t * logObj = json_object_get( json_config, "Log");
	
	if( logObj)
	{
		json_t * logRoot = NULL;
		logRoot = json_object_get( logObj, "LogRootDire");
		
		if( logRoot)
		{
			char * c_root_dir = (char*)json_string_value( logRoot);
			int c_root_dir_len = json_string_length( logRoot);
			
			if( c_root_dir && c_root_dir_len > 0)
			{
				app_logger__create_directory( c_root_dir);
			}
		}
		
		
		json_t * logInstance = NULL;
		
		logInstance = json_object_get( logObj, "App");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->appLogger);
		}

		logInstance = json_object_get( logObj, "Perf");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->perfLogger);
			app_logger__setplainlog( dp_instance->perfLogger);
		}
		
		logInstance = json_object_get( logObj, "Device");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->deviceLogger);
		}

		logInstance = json_object_get( logObj, "L7");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->l7Logger);
		}

		logInstance = json_object_get( logObj, "PFCP");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->pfcpLogger);
		}

		logInstance = json_object_get( logObj, "Packet");
		if( logInstance)
		{
			app_config__parse_loginstance( logInstance, &dp_instance->packetLogger);
		}

	}
}

void pfcp_stack___setinitDPI( int initDPI);

void pfcp_stack___create_session( uint64_t cp_f_seid, uint32_t upf_teid, uint32_t ueIPv4, uint8_t * ueIPv6, uint32_t rgCount, uint32_t ipflowCount);
int cp__teid_allowed_v4( uint32_t ran_ip, uint32_t teid, uint8_t ** session, uint8_t * mac);
int cp__flow_allowed_sess_v4( uint8_t * session, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  int direction, int coreid);
int cp__flow_allowed_ueipv4( uint32_t ueip, uint32_t dstip, uint16_t srcport, uint16_t dstport, uint8_t protocol, char * pload, int ploadlen,  uint8_t ** session, uint32_t * ran_teid, int direction, int coreid, uint8_t * qfi);

void app_config__tdd( json_t * config )
{
	json_t * tdd_config = json_object_get( config, "TDD");
	
	if( tdd_config)
	{
		int enabled = app_config__get_int( tdd_config, "Enabled", 0);
		
		printf( "TDD Enabled=%u\n", enabled);
		
		if( enabled == 1)
		{
			dp_instance->TDD_Enabled = 1;

			json_t * tdd_sessions = json_object_get( tdd_config, "Sessions");
			
			if( tdd_sessions)
			{
				dp_instance->TDD_Sessions.Enable			= app_config__get_int( tdd_sessions, "Enabled", 0);
				
				if( dp_instance->TDD_Sessions.Enable == 1)
				{
					dp_instance->TDD_Sessions.Create			= app_config__get_int( tdd_sessions, "Create", 0);
					dp_instance->TDD_Sessions.Count				= app_config__get_int( tdd_sessions, "Count", 0);
					dp_instance->TDD_Sessions.SeidBegin			= app_config__get_int( tdd_sessions, "CP_SEID_BEGIN_NO", 0);
					dp_instance->TDD_Sessions.TeidBegin			= app_config__get_int( tdd_sessions, "CP_TEID_BEGIN_NO", 0);
					dp_instance->TDD_Sessions.NoOfSessionsPerIP	= app_config__get_int( tdd_sessions, "NoOfSessionsPerIP", 1);
					dp_instance->TDD_Sessions.RGCount			= app_config__get_int( tdd_sessions, "RGCount", 1);
					dp_instance->TDD_Sessions.IPFlowCount		= app_config__get_int( tdd_sessions, "IPFlowCount", 0);

					if( dp_instance->TDD_Sessions.Enable == 0 || dp_instance->TDD_Sessions.Count == 0)
						return;
					
					if( dp_instance->TDD_Sessions.Create == 0)
						return;
					

					if( dp_instance->TDD_Sessions.NoOfSessionsPerIP == 0) 
					{
						dp_instance->TDD_Sessions.NoOfSessionsPerIP = 1;
					}

					if( dp_instance->TDD_Sessions.RGCount == 0) 
					{
						dp_instance->TDD_Sessions.RGCount = 1;
					}
					
					if( dp_instance->TDD_Sessions.SeidBegin == 0) 
					{
						dp_instance->TDD_Sessions.SeidBegin = 1;
					}

					if( dp_instance->TDD_Sessions.TeidBegin == 0) 
					{
						dp_instance->TDD_Sessions.TeidBegin = 1;
					}
					
					

					char IPv4[100];
					memset( IPv4, 0, sizeof(IPv4));
					app_config__set_str( tdd_sessions, "IPv4_BEGIN_NO", IPv4, 100);
					
					char IPv6[100];
					memset( IPv6, 0, sizeof(IPv6));
					app_config__set_str( tdd_sessions, "IPv6_BEGIN_NO", IPv6, 100);


					uint32_t u32 = app_ep__get_u32_ipv4( IPv4);
					dp_instance->TDD_Sessions.IPv4Begin			= htonl(u32);
					
					struct in6_addr ip6;
					if (inet_pton( AF_INET6, IPv6, &ip6) <= 0) 
					{
						fprintf(stderr, "ipcalc: bad IPv6 address: %s\n", IPv6);
						//exit(0);
					}
					
					memcpy( dp_instance->TDD_Sessions.IPv6Begin, ip6.s6_addr, 16); 
					
					
					// // printf( "time_t=%lu\n", sizeof(time_t));
					// // exit(0);

					app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
						"TDD Session Count=%lu  SeidBegin=%lu  TeidBegin=%u   NoOfSessionsPerIP=%u  RGCount=%u  IPFlowCount=%u   %s|%s|%d", 
							dp_instance->TDD_Sessions.Count, dp_instance->TDD_Sessions.SeidBegin, 
							dp_instance->TDD_Sessions.TeidBegin, dp_instance->TDD_Sessions.NoOfSessionsPerIP,
							dp_instance->TDD_Sessions.RGCount, dp_instance->TDD_Sessions.IPFlowCount, 
						__FILE__, __FUNCTION__, __LINE__);
					
					
					// // usleep( 999999);
					// // exit(0);
					
					
					int j = 0;
					int z = 0;
					uint32_t z1 = 0;
					uint32_t zuEIP = 0;
					uint32_t uuEIP6 = htonl(*(uint32_t*)&dp_instance->TDD_Sessions.IPv6Begin[12]);
					uint8_t zuEIPv6[16];

					for( j = 0; j < dp_instance->TDD_Sessions.Count; j++)
					{
						if( z == 0)
						{
							zuEIP = dp_instance->TDD_Sessions.IPv4Begin + z1;
							memcpy( zuEIPv6, dp_instance->TDD_Sessions.IPv6Begin, 16);
							*(uint32_t*)&zuEIPv6[12] = htonl(uuEIP6 + z1);
						}

						pfcp_stack___create_session( 
								(dp_instance->TDD_Sessions.SeidBegin + j), 
								(dp_instance->TDD_Sessions.TeidBegin + j), 
								zuEIP, zuEIPv6, dp_instance->TDD_Sessions.RGCount,
								dp_instance->TDD_Sessions.IPFlowCount
							);

						z++;

						if( dp_instance->TDD_Sessions.NoOfSessionsPerIP == z)
						{
							z = 0;
							z1++;
						}
					}
					
					

					app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, 
						"TDD Session Count=%lu   Completed  %s|%s|%d", dp_instance->TDD_Sessions.Count,
							__FILE__, __FUNCTION__, __LINE__);

						
					// int sts = 0;
					// uint8_t * session = NULL;
					
					// app_exec_time_t et;
					// app_start_time( &et);
					// int f = 12345;
					
					// for( j = 0; j < dp_instance->TDD_Sessions.Count; j++)
					// {
						// sts = cp__teid_allowed_v4( 1, (dp_instance->TDD_Sessions.TeidBegin + j), &session, "\x00\x00\x00\x00\x00\x00");
						// //printf( "sts=%d %3d session=%p  %d\n", sts, (dp_instance->TDD_Sessions.TeidBegin + j), session, __LINE__);
						
						
						// if( sts == 1)
						// {
							// for( z = 0; z < 5; z++)
							// {
								// cp__flow_allowed_sess_v4( session, f, 12345, f, 6, "x00\x00\x00\x00\x00\x00", 6,  1, 1);
								// //printf( "1  z=%d\n", z);
								// f++;
							// }
						// }	
					// }

					// app_end_time( &et);
					// printf( "done %d in %lu-%lu\n", dp_instance->TDD_Sessions.Count, et.lapsed.tv_sec, et.lapsed.tv_usec);
					

					// uint8_t qfi 		= 0;
					// uint32_t ran_teid 	= 0;
					// session 			= NULL;
					
					// app_start_time( &et);
					// int t = 0;
					// f = 12345;
					
					// for( j = 0; j < dp_instance->TDD_Sessions.Count; j++)
					// {
						// for( t = 0; t < 2; t++)
						// {
							// for( z = 0; z < 5; z++)
							// {
								// sts = cp__flow_allowed_ueipv4( 
																	// htonl(dp_instance->TDD_Sessions.IPv4Begin + t), 
																	// f, 
																	// 12345, 
																	// f,
																	// 6, 
																	// "\x00\x00\x00\x00\x00\x00", 
																	// 6,  
																	// &session, 
																	// &ran_teid, 
																	// 2, 
																	// 1, 
																	// &qfi
																// );
								
								// // printf( "session=%p  %d\n", session, __LINE__);
								
								// f++;
							// }
						// }
					// }

					// app_end_time( &et);
					// printf( "done %d in %lu-%lu\n", dp_instance->TDD_Sessions.Count, et.lapsed.tv_sec, et.lapsed.tv_usec);

					
					
					// pfcp_stack___setippool( dp_instance->ippool_ipv4, dp_instance->ippool_ipv6, dp_instance->ippool_port);
		 
					
					// uint64_t TotalCount = 0;
					// uint64_t Available 	= 0;
					
					// app_region__get_counts( dp_instance->ippool_port, &TotalCount, &Available);

					// printf( "TotalCount=%lu  Available=%lu  %lu  %s|%d\n", 
						// TotalCount, Available, TotalCount - Available, __FUNCTION__, __LINE__);
					
					// t = 0;
					// int alloc = 0;
					// pfcp_port_flow_t * portFlow = NULL;
					
					// for( t = 0; t < TotalCount; t++)
					// {
						// alloc = app_region__is_item_allocated_at_index( dp_instance->ippool_port, t);
					
						// if( alloc > 0)
						// {
							// //printf( "alloc=%d  %d\n", alloc, __LINE__);
							// portFlow = (pfcp_port_flow_t*)app_region__get_pointer_at_index( dp_instance->ippool_port, t);
						
							
						// }
					// }
						

					// exit(0);

					// memset( IPv4, 0, sizeof(IPv4));
					// app_ep__get_str_ipv4( htonl(htonl(u32) + 1), IPv4);

					//dp_instance->TDD_Sessions.IPv4Begin			= app_config__get_int( tdd_sessions, "Count", 0);
					//uint8_t  IPv6Begin[16];
			
					// printf( "Enable=%d Create=%d Count=%d SeidBegin=%d TeidBegin=%d NoOfSessionsPerIP=%d u32=%u|%s   %s|%d\n", 
						// dp_instance->TDD_Sessions.Enable,
						// dp_instance->TDD_Sessions.Create,
						// dp_instance->TDD_Sessions.Count,
						// dp_instance->TDD_Sessions.SeidBegin,
						// dp_instance->TDD_Sessions.TeidBegin,
						// dp_instance->TDD_Sessions.NoOfSessionsPerIP,
						// u32, IPv4,
						// __FILE__, __LINE__
					// );
					
					// int i = 0;
					// for (i = 0; i < sizeof(struct in6_addr); i++)
						// printf("%02X", ip6.s6_addr[i] & 0xFF);
					
					// printf("\n");
					
					// exit(0);
				}
			}
			

			json_t * tdd_usage = json_object_get( tdd_config, "Usage");
			
			if( tdd_usage)
			{
				int usage_enabled = app_config__get_int( tdd_usage, "Enabled", 0);
				
				//printf( "tdd usage_enabled=%u\n", usage_enabled);
				
				if( usage_enabled == 1)
				{
					json_t * jQuota = json_object_get( tdd_usage, "Quota");
					json_t * jQuotaItem = NULL;
				
					if(jQuota)
					{
						if( json_is_array(jQuota))
						{
							int iQuotaSize = json_array_size( jQuota);
							int iQC = 0;
							uint32_t Id = 0;
							uint32_t Bytes = 0;
							
							
							if( iQuotaSize > 6)
								iQuotaSize = 6;
							
							int tdd_quota_index = 0;
							
							for( iQC = 0; iQC < iQuotaSize; iQC++)
							{
								jQuotaItem = json_array_get( jQuota, iQC);
								
								if( jQuotaItem)
								{
									Id = app_config__get_int( jQuotaItem, "Id", 0);
									Bytes = app_config__get_int( jQuotaItem, "Bytes", 0);
								
									if( Id > 0 && Bytes > 0)
									{
										dp_instance->TDD_Quota[tdd_quota_index].Id = Id;
										dp_instance->TDD_Quota[tdd_quota_index].Bytes = Bytes;
										
										printf( "TDD  %u  Id=%u  Bytes=%u\n", 
											tdd_quota_index, dp_instance->TDD_Quota[tdd_quota_index].Id, dp_instance->TDD_Quota[tdd_quota_index].Bytes);
										
										tdd_quota_index++;
									}
								}
							}
						}
					}
				}
			}
		}
	}	
}

void app_config__pfcp()
{
	// int DPIFlowCount = 0;
	
	// if( dp_instance->initDPI == 1) {
		// //DPIFlowCount = dp_instance->DPIFlowCount;
		// DPIFlowCount = 1;
	// }
	
	dp_instance->pfcp_sessions_tree 	= app_rbnode__create_idp( dp_instance->pfcp_MaxSessions, dp_instance->pfcp_SEIDStartNumber);
	dp_instance->pfcp_session_region	= app_region__create();
	dp_instance->pfcp_session_pool 		= app_region__add_pool( dp_instance->pfcp_session_region, "PFCP-S", sizeof(pfcp_session_t), dp_instance->pfcp_MaxSessions);
	//dp_instance->pfcp_session_ip_pool 	= app_region__add_pool( dp_instance->pfcp_session_region, "PFCP-I", sizeof(pfcp_ip_flows_t), dp_instance->pfcp_MaxSessions);
	//app_data_pool_t * flowPool 			= app_region__add_pool( dp_instance->pfcp_session_region, "FLOWPL", sizeof(ip4flow_t), DPIFlowCount);
	app_data_pool_t * flowPool			= NULL;

	dp_instance->ippool_region			= app_region__create();
	dp_instance->ippool_ipv4			= app_region__add_pool( dp_instance->ippool_region, "IPv4FPC", sizeof(ip4_flow_t), 	dp_instance->IP4FlowPoolCount);
	dp_instance->ippool_ipv6			= app_region__add_pool( dp_instance->ippool_region, "IPv6FPC", sizeof(ip6_flow_t), 	dp_instance->IP6FlowPoolCount);
	//dp_instance->ippool_port			= app_region__add_pool( dp_instance->ippool_region, "PortFPC", sizeof(pfcp_port_flow_t), 	dp_instance->PortPoolCount);

	
	// int ij = 0;
	// pfcp_ip_flows_t * ipflows 	= NULL;
	// pfcp_ip4_flow_t * ip4flow 	= NULL;
	// pfcp_ip6_flow_t * ip6flow 	= NULL;
	// pfcp_port_flow_t * portFlow = NULL;
	
	// for( ij = 0; ij < dp_instance->pfcp_MaxSessions; ij++)
	// {
		// ipflows = (pfcp_ip_flows_t*)app_region__allocate_fd( dp_instance->pfcp_session_ip_pool);
		// memset( ipflows, 0, sizeof(pfcp_ip_flows_t));
		
		// pthread_mutex_init( &ipflows->f4Lock, NULL);
		// pthread_mutex_init( &ipflows->f6Lock, NULL);
		// pthread_mutex_init( &ipflows->session_count_lock, NULL);
		
		// app_region__free( (uint8_t *) ipflows);
	// }
	
	// for( ij = 0; ij < dp_instance->IP4FlowPoolCount; ij++)
	// {
		// ip4flow = (pfcp_ip4_flow_t*)app_region__allocate_fd( dp_instance->ippool_ipv4);
		// memset( ip4flow, 0, sizeof(pfcp_ip4_flow_t));
		
		// pthread_mutex_init( &ip4flow->portLock, NULL);
		
		// app_region__free( (uint8_t *) ip4flow);
	// }
	
	// for( ij = 0; ij < dp_instance->IP6FlowPoolCount; ij++)
	// {
		// ip6flow = (pfcp_ip6_flow_t*)app_region__allocate_fd( dp_instance->ippool_ipv6);
		// memset( ip6flow, 0, sizeof(pfcp_ip6_flow_t));
		
		// pthread_mutex_init( &ip6flow->portLock, NULL);
		
		// app_region__free( (uint8_t *) ip6flow);
	// }


	// for( ij = 0; ij < dp_instance->PortPoolCount; ij++)
	// {
		// portFlow = app_region__allocate_fd( dp_instance->ippool_port);
		// memset( portFlow, 0, sizeof(pfcp_port_flow_t));
		// app_region__free( (uint8_t *) portFlow);
	// }
	
	
	// printf( "4FPC=%d  6FPC=%d  PoolFPC=%d  %p %p %p %p   %s|%d\n", 
		// dp_instance->IP4FlowPoolCount, dp_instance->IP6FlowPoolCount, dp_instance->PortPoolCount,
		// dp_instance->ippool_region, dp_instance->ippool_ipv4, dp_instance->ippool_ipv6, dp_instance->ippool_port,
	// __FILE__, __LINE__);

	// exit(0);
	
	app_region__create_index( dp_instance->pfcp_session_pool);
	app_region__create_index( dp_instance->ippool_ipv4);
	app_region__create_index( dp_instance->ippool_ipv6);
	//app_region__create_index( dp_instance->ippool_port);
	
	
	pfcp_stack___setinitDPI(  1);
	//pfcp_stack___setinitDPI(  dp_instance->initDPI);
	//pfcp_stack___setinitDPDK( dp_instance->DisableDPDK); 
	pfcp_stack___setpool( dp_instance->pfcp_sessions_tree, dp_instance->pfcp_session_pool, dp_instance->pfcp_session_ip_pool, flowPool);
	//pfcp_stack___setippool( dp_instance->ippool_ipv4, dp_instance->ippool_ipv6, dp_instance->ippool_port);
	pfcp_stack___setippool( dp_instance->ippool_ipv4, dp_instance->ippool_ipv6, NULL);
	
	
	//printf( "pfcp_session_region=%p pfcp_session_pool=%p\n", dp_instance->pfcp_session_region, dp_instance->pfcp_session_pool);
	
	/*
	pfcp_session_t * pfcpSession = NULL;
	pfcp_session_t * pfcpSessionF = NULL;
	
	int lCount = (dp_instance->pfcp_MaxSessions * 1000);
	int i = 0;
	
	for( i = 0; i < lCount; i++)
	{
		//pfcp_session_t * pfcpSession = NULL;
		pfcpSession = (pfcp_session_t *)app_region__allocate_fd( dp_instance->pfcp_session_pool);
		
		if( pfcpSession)
		{
			pfcpSessionF = NULL;
			
			pfcpSession->seid = app_rbnode__get_next_idp( dp_instance->pfcp_sessions_tree, (uint8_t *)pfcpSession);
			
			app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "seid=%ld pfcpSession=%p", pfcpSession->seid, pfcpSession);
			
			if( pfcpSession->seid > 0)
			{
				pfcpSessionF = (pfcp_session_t *)app_rbnode__find_idp( dp_instance->pfcp_sessions_tree, pfcpSession->seid);
			}
			
			if( pfcpSessionF) {
				app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "F seid=%ld pfcpSession=%p", pfcpSessionF->seid, pfcpSessionF);
			}
			
			app_rbnode__free_idp( dp_instance->pfcp_sessions_tree, pfcpSession->seid);
			app_region__free( (uint8_t*)pfcpSession);
		}	
	}
	*/
}


void test_queue_call_back( void * Data, int tIndex)
{
	app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "Data=%p tIndex=%d %s|%s|%d", Data, tIndex, __FILE__, __FUNCTION__, __LINE__);
}


int  cp__get_on_going_sessions( pfcp_session_t ** tdd_pfcp_sessions, int * tdd_pfcp_sessions_count);
void cp__tdd_set__session_urrs( pfcp_session_t * pfcp_session, uint32_t urrId, uint64_t volume);
void cp__tdd_trigger__session_usage( pfcp_session_t * pfcp_session);


void * app__run_tdd_thread( void * args)
{
	if( dp_instance->TDD_Enabled == 1)
	{
		while(1)
		{
			// int tdd_fetch_counter = 0;
			// pfcp_session_t * tdd_pfcp_sessions[100];
			// int tdd_pfcp_sessions_count = 0;
			// int sts = 0;
		
			// //printf( "tdd_fetch_counter=%d tdd_pfcp_sessions_count=%d\n", tdd_fetch_counter, tdd_pfcp_sessions_count);
			// if( tdd_fetch_counter == 0)
			// {
				// tdd_pfcp_sessions_count = 0;
				
				// sts = cp__get_on_going_sessions( tdd_pfcp_sessions, &tdd_pfcp_sessions_count);
				// //printf( "sts=%d %s|%s|%d\n", sts, __FILE__, __FUNCTION__, __LINE__);
				
				// if( tdd_pfcp_sessions_count > 0) {
					// printf( "tdd - sessions found , total=%u\n", tdd_pfcp_sessions_count);
				// }
			// }

			
			
			// if( tdd_pfcp_sessions_count > 0)
			// {
				// int j = 0;
				// int z = 0;
				
				// for( j = 0; j < tdd_pfcp_sessions_count; j++)
				// {
					// for( z = 0; z < 10; z++)
					// {
						// //printf( "z=%d Id=%u\n", z, dp_instance->TDD_Quota[z].Id);
						
						// if(dp_instance->TDD_Quota[z].Id == 0)
						// {
							// break;
						// }
						// else
						// {
							
							// cp__tdd_set__session_urrs( tdd_pfcp_sessions[j], dp_instance->TDD_Quota[z].Id, dp_instance->TDD_Quota[z].Bytes);
						// }
					// }
					
					// // if( z > 0)
					// // {
						// // cp__tdd_trigger__session_usage( tdd_pfcp_sessions[j]);
					// // }
				// }
			// }

			// tdd_fetch_counter++;

			// if( tdd_fetch_counter == 10 && tdd_pfcp_sessions_count == 0 || tdd_fetch_counter == 100)
			// {
				// tdd_fetch_counter = 0;
			// }


			usleep( 999999);
		}
	}	
}


void * app__run_pref_thread( void * args)
{
	int i = 0;

	char ctimestamp[30];
	int log_buffers = 0;

	int tdd_fetch_counter = 0;
	pfcp_session_t * tdd_pfcp_sessions[100];
	int tdd_pfcp_sessions_count = 0;
	int sts = 0;
	
	char * UPF_VERSION = "XIUS NPG-UPF 1.0.0.0";
	time_t current_time;
    struct tm * time_info;
	long days = 0;
	long hours = 0;
	long minutes = 0;
	long seconds = 0;
	
	
	char time_string[100];
	memset( time_string, 0, sizeof(time_string));
	
	time( &current_time);
	time_info = localtime(&current_time);
	
	strftime( time_string, sizeof(time_string), "%d-%m-%Y %H:%M:%S", time_info);
	
	app_exec_time_t app_time;
	app_start_time( &app_time);
	
	while(1)
	{
		days = 0;
		hours = 0;
		minutes = 0;
		seconds = 0;

		app_end_time( &app_time);
		app_end_full_time( &app_time, &days, &hours, &minutes, &seconds);
		
		//log_buffers = (i == dp_instance->perfLogBufferLogCounter) ? 1 : 0; 
		log_buffers = 1;
		
		memset( ctimestamp, 0, sizeof(ctimestamp));
		app_makeTimeStamp2( ctimestamp);
		app_logger__log( dp_instance->perfLogger, NULL, APP_LOG__LEVEL_CRITICAL, "%s     Started-Time: %s    Current-Time: %s    Running Time: %03ld %02ld:%02ld:%02ld   %ld", 
			UPF_VERSION, time_string, ctimestamp, days, hours, minutes, seconds, app_time.lapsed.tv_sec);
		
		//app_time.lapsed, 
		
		app_logger__log( dp_instance->perfLogger, NULL, APP_LOG__LEVEL_CRITICAL, "");

		pfcp_stack__perflog( dp_instance->perfLogger, log_buffers);
		app_ep__perflog( dp_instance->perfLogger, log_buffers);
		
		if( i == dp_instance->perfLogBufferLogCounter)
		{
			app_logger__log( dp_instance->perfLogger, NULL, APP_LOG__LEVEL_CRITICAL, "---------------------------------------------------------------------------------------------");
			app_region__print_stats( dp_instance->perfLogger, dp_instance->app_mregion);
			app_region__print_stats( dp_instance->perfLogger, dp_instance->pfcp_session_region);
			i = 0;
		}
		else 
		{
			i++;
		}
		
		app_logger__log( dp_instance->perfLogger, NULL, APP_LOG__LEVEL_CRITICAL, "---------------------------------------------------------------------------------------------");
		
		
		sleep( dp_instance->perfLogInterval);
	}	
}

// 
void app__start_pref_thread()
{
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	int iRet = pthread_create( &s_pthread_id, &attr, app__run_pref_thread, (void *)NULL);
}


void app__start_tdd_thread()
{
	if( dp_instance->TDD_Enabled == 1)
	{
		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		int iRet = pthread_create( &s_pthread_id, &attr, app__run_tdd_thread, (void *)NULL);
	}
}


int exiting = 0;

void app__signalhandler( int signum)
{
	if( exiting == 1) return;
	exiting = 1;
	
	switch( signum)
	{	
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
		{
			
		}
		break;
	}
	
	int secs = 2;
	
	printf("\n\nexiting upf...\n\n");
	if(!dp_instance->DisableDPDK) 
	{
		dpdk__initexit();
	}
	pfcp_stack___exit();
	//printf("\n\nEncountered SIGNAL %d, exiting in %d seconds.\n", signum, secs);
	//sleep(secs);
	exit(0);
}

int 	dpe__get_recv_worker();
void 	upfndpi_Initalize( int iThreadcount, int flowcount);
void 	dpe__start_interfaces();
int app_promethes_perf_stats();
// export LD_LIBRARY_PATH=.
// gcc -g3 app.c app_stack.c app_endpoint.c pfcp_parser.c pfcp_stack.c -o upf -I../jansson-2.13/src/ -lpthread -ljansson
int main( int argc, char* argv[])
{
	// int TCount    =  900001;
	// int ChunkCount = 500000;
	
	// if( TCount > ChunkCount)
	// {
		// printf( "%u  %u\n", TCount / ChunkCount,  TCount % ChunkCount  );
	// }
	
	// exit(0);
	
	
	
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./dpnc.json\n", argv[0]);
		exit(0);
	}
	
	uid_t euid = geteuid();
	
	if( euid != 0)
	{
		printf("The program is NOT running with sudo or root privileges.\n");
		exit(0);
	}
	
	
	signal( SIGINT, app__signalhandler);		// Ctrl + C
	signal( SIGQUIT, app__signalhandler);		// Ctrl + \ 			//
	signal( SIGTERM, app__signalhandler);		// shell command kill generates SIGTERM by default
	
	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( argv[1], 0, &error);
	
	if(!json_config)
	{
		printf("error in json %s file at line=%d column=%d position=%d\n", argv[1], error.line, error.column, error.position);
		exit(0);
	}
	
	dp_instance = (dp_application_t *)malloc( sizeof(dp_application_t));
	memset( dp_instance, 0, sizeof(dp_application_t));
	

	// dpdk_session__set_fp_seid( 	pfcp_session__get_seid);
	// dpdk_session__set_fp_quota( pfcp_session__get_quota);
	// dpdk_session__set_fp_usage( pfcp_session__record_usage);
	// dpdk_session__set_fp_dpi_session( pfcp_session__get_dpi_session);
	// dpdk_session__set_fp_pfcp_session( pfcp_session__get_pfcp_session);
	// dpdk_session__set_fp_ohi( 	pfcp_session__session_ohi);
	// dpdk__set_logger_m( app_packetlog, app_perflog);
	init_rand();
	app_config__parse_system_info( json_config);
	app_config__parse_pfcp_info( json_config);
	app_config__parse_adminstrative_info( json_config);
	app_config__parse_aerospike( json_config);
	app_config__parse_prometheus( json_config);
	app_config__parse_log_info( json_config);
	app_stack__set_logger( dp_instance->appLogger);
	pfcp_stack__set_applogger( dp_instance->appLogger);
	pfcp_stack__set_pfcplogger( dp_instance->pfcpLogger);
	pfcp_stack__set_devicelogger( dp_instance->deviceLogger);
	
	app_config__pfcp();
	
	app__start_pref_thread();
	
	
	app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "%s initalization completed %s|%s|%d", dp_instance->systemName, __FILE__, __FUNCTION__, __LINE__);
	app_region__print_stats( NULL, dp_instance->app_mregion);
	app_region__print_stats( NULL, dp_instance->pfcp_session_region);
	
	// ls__init( 1500000);
	// dp_instance->teid_table = ls__create_uint_table( 500000, 32, 0);

	//dpdk__init( argc, argv);
	
	if( dp_instance->aerospike_Enabled == 1)
	{
		app__as__init( 4, dp_instance->aerospike_ip, dp_instance->aerospike_Port, dp_instance->aerospike_namespace, dp_instance->aerospike_set, dp_instance->appLogger);
		app__as__restoresession();
		
		while( app__as__restore_completed() == 0)
		{
			usleep( 999999);
			app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "restoring sessions from aerospike  count=%s  %s|%s|%d", app__as__restore_records(), __FILE__, __FUNCTION__, __LINE__);
		}
	}
	app_promethes_perf_stats();
	if(dp_instance->prometheus_Enabled == 1) 
	{
		//app_promethes_perf_stats();
		//metrics_prometheus(dp_instance->prometheus_ip,dp_instance->prometheus_Port);
		app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "initalized PROMETHEUS    %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
	}
	
	
	
	//DPDK and DPI both are different
	if( dp_instance->DisableDPDK == 0)
	{
		app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "initalizing dpdk    %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
		
		
		if( strlen( dp_instance->engFile) == 0) {
			dpe__init( "eng.json");
		} else {
			dpe__init( dp_instance->engFile);
		}
		
		dpe__init_engine( argc, argv);	
		pfcp_stack___init_pace2();
		dpe__start_interfaces();

		app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "initalized dpdk    %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);
	}
	
	// if( dp_instance->initDPI == 1 && dp_instance->DisableDPDK == 0)
	// {
		// printf("initDPI=%d  DPIFlowCount=%d  recv_worker_count=%d\n", 
				// dp_instance->initDPI, 
				// dp_instance->DPIFlowCount,
				// dpe__get_recv_worker()
			// );

		// //dp_instance->DPIFlowCount 	= app_config__get_int( systemObj, "DPIFlowCount", 0);
		// //app_config__set_str( systemObj, "DPIAdcFile", dp_instance->adcFile, 249);

		// upfndpi_Initalize( dpe__get_recv_worker(), dp_instance->DPIFlowCount);
	// }
	


	dpiadc__init( dp_instance->adcFile, dp_instance->DefaultRGId);

	app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "starting udp server      %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);


	
	app_queue__set_limit( dp_instance->udp_ipv4server->queue, 4000);
	
	app_ep__udp_server_start( dp_instance->udp_ipv4server);
	app_ep__udp_server_start( dp_instance->udp_ipv6server);

	app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "started udp server      %s|%s|%d", __FILE__, __FUNCTION__, __LINE__);


	
	usleep( 999999);
	
	printf("----------------------------------- upf-v 1.7.14.24  started -----------------------------------\n");
	app_logger__log( dp_instance->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "----------------------------------- upf-v 1.7.14.24  started -----------------------------------  %s|%s|%d", 
		__FILE__, __FUNCTION__, __LINE__);


	usleep( 999999);

	app_config__tdd( json_config);
	app__start_tdd_thread();

	// if( dp_instance->DisableDPDK == 0)
	// {
		// sleep(1);
		// //dpe__test();
	// }

	printf("----------------------------------------------------------------------------------------------\n");

	while(1) {
		sleep(1);
	}
	
	return 0;
}








	//int i = 0;
	/*
	app_iQueue * queue = app_queue__create( "appQ1", 10000, 2, test_queue_call_back);
	
	void * d = NULL;
	for( i = 0; i < 10000; i++)
	{
		d = malloc(1);
		app_queue__enquee( queue, d, NULL);
	}
	*/
	
	/*
	app_fsmQueue_t * fsmq = app_fsmqueue__create( "fsmq", 5000, 5, test_queue_call_back);
	void * d = NULL;
	int x = 0;
	for( i = 0; i < 10000; i++)
	{
		d = malloc(1);
		app_fsmqueue__idenquee( fsmq, d, i, NULL);
		
		if( i > 5000 && x == 0)
		{
			x = 1;
			app_fsmqueue__logstats( fsmq);
		}
	}
	app_fsmqueue__logstats( fsmq);
	*/
	





