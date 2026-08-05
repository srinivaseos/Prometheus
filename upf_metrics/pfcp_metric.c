#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <jansson.h>

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include <prom_alloc.h>
#include "prom_collector.h"
#include "prom_metric.h"
#include "pfcp_metric.h"
//#include "metrics_test.h"
//#define PORT 9527
//#define IP "192.168.144.27"



typedef struct app_promo
{
    char pfcp_IPv4[20];
	int pfcp_Port;
    int promotheus_Enabled;
}app_promo_t;

app_promo_t *promo_config=NULL;

typedef struct metrics
{
    prom_counter_t *metrics_est_request;
    prom_counter_t *metrics_est_responce;
    prom_counter_t *metrics_mod_request;
    prom_counter_t *metrics_mod_responce;
    prom_counter_t *metrics_report_request;
    prom_counter_t *metrics_report_responce;
    prom_counter_t *metrics_delete_request;
    prom_counter_t *metrics_delete_responce;
	
	prom_counter_t *c_pkt;
    prom_counter_t *total__availabe_pkts;

    prom_counter_t *metrics_gtp_recvied;
    prom_counter_t *metrics_gtp_allowed;
    prom_counter_t *metrics_gtp_dropped;
    prom_counter_t *metrics_gi_recvied;
    prom_counter_t *metrics_gi_allowed;
    prom_counter_t *metrics_gi_dropped;

    prom_gauge_t  *prom_gauge_test;

}metrics_t;

metrics_t *manager_metrics =NULL;

prom_collector_registry_t *PROM_ACTIVE_REGISTRY;

void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry) {
  if (!active_registry) {
    PROM_ACTIVE_REGISTRY = PROM_COLLECTOR_REGISTRY_DEFAULT;
  } else {
    PROM_ACTIVE_REGISTRY = active_registry;
  }
}


void metrics_init(metrics_t *manager_metrics)
{
    
    
    // Initialize the Prometheus library
    prom_collector_registry_default_init();
    promhttp_set_active_collector_registry(NULL);
    printf("metrics_init metrics_handler2\n");
    // Create a counter metric
    manager_metrics->metrics_est_request =prom_collector_registry_must_register_metric(prom_counter_new("PFCP_EST_requests_total", "Total number of PFCP_EST requests",0,NULL));
    manager_metrics->metrics_est_responce =  prom_collector_registry_must_register_metric(prom_counter_new("PFCP_EST_responses_total", "Total number of PFCP_EST responses", 0,NULL));
    
    manager_metrics->metrics_mod_request =prom_collector_registry_must_register_metric(prom_counter_new("PFCP_MOD_requests_total", "Total number of PFCP_MOD requests",0,NULL));
    manager_metrics->metrics_mod_responce =  prom_collector_registry_must_register_metric(prom_counter_new("PFCP_MOD_responses_total", "Total number of PFCP_MOD responses", 0,NULL));
   
    manager_metrics->metrics_delete_request =prom_collector_registry_must_register_metric(prom_counter_new("PFCP_DELETE_requests_total", "Total number of PFCP_DELETE requests",0,NULL));
    manager_metrics->metrics_delete_responce =  prom_collector_registry_must_register_metric(prom_counter_new("PFCP_DELETE_responses_total", "Total number of PFCP_DELETE responses", 0,NULL));

    manager_metrics->metrics_report_request =prom_collector_registry_must_register_metric(prom_counter_new("PFCP_REPORT_requests_total", "Total number of PFCP_REPORT requests",0,NULL));
    manager_metrics->metrics_report_responce =  prom_collector_registry_must_register_metric(prom_counter_new("PFCP_REPORT_responses_total", "Total number of PFCP_REPORT responses", 0,NULL));
	
	 manager_metrics->c_pkt =prom_collector_registry_must_register_metric(prom_counter_new("PFCP_c_pkt_total", "Total number of PFCP_c_pkt_requests",0,NULL));
    manager_metrics->total__availabe_pkts =  prom_collector_registry_must_register_metric(prom_counter_new("PFCP_total__availabe_pkts", "Total number of PFCP_total__availabe_pkts", 0,NULL));
    
    //manager_metrics->metrics_est_request =  prom_collector_registry_must_register_metric(prom_counter_new("http_requests_total", "Total number of HTTP requests", 3, (const char*[]){"url","method","success"}));
   // manager_metrics->metrics_est_responce =  prom_collector_registry_must_register_metric(prom_counter_new("http_responses_total", "Total number of HTTP responses", 3, (const char*[]){"url","method","success"}));

   manager_metrics->metrics_gtp_recvied =prom_collector_registry_must_register_metric(prom_counter_new("GTP_PACKETS_RECVIED_total", "Total number of GTP_RECVIED requests",0,NULL));
   manager_metrics->metrics_gtp_allowed =prom_collector_registry_must_register_metric(prom_counter_new("GTP_PACKETS_ALLOWED_total", "Total number of GTP_ALLOWED requests",0,NULL));
   manager_metrics->metrics_gtp_dropped =prom_collector_registry_must_register_metric(prom_counter_new("GTP_PACKETS_DROPPED_total", "Total number of GTP_DROPPED requests",0,NULL));
   manager_metrics->metrics_gi_recvied =prom_collector_registry_must_register_metric(prom_counter_new("GI_PACKETS_RECVIED_total", "Total number of GI_RECVIED requests",0,NULL));
   manager_metrics->metrics_gi_allowed =prom_collector_registry_must_register_metric(prom_counter_new("GI_PACKETS_ALLOWED_total", "Total number of GI_ALLOWED requests",0,NULL));
   manager_metrics->metrics_gi_dropped =prom_collector_registry_must_register_metric(prom_counter_new("GI_PACKETS_DROPPED_total", "Total number of GI_DROPPED requests",0,NULL));
    const char *labels[] = {"example_label"};
   manager_metrics->prom_gauge_test=prom_collector_registry_must_register_metric(prom_gauge_new("foo_gauge","gauage per second total request",0,NULL));
    // Cleanup before exiting
    //prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);
}

//int successfull_task_request( const char *url, const char *method)
void successful_task_request(  const char *method,int msgtype,double count)
{
    if(manager_metrics)
    {
        //printf("data=%d",data);
        switch(msgtype)
        {
            case 1:
                    //printf("entered successful_task_request\n");
                    //prom_counter_inc(manager_metrics->metrics_est_request,0);
                    prom_counter_add(manager_metrics->metrics_est_request,count,0);
                    break;
            case 2: prom_counter_add(manager_metrics->metrics_mod_request,count,0);
                    //prom_counter_inc(manager_metrics->metrics_mod_request,0);
                    break; 
            case 3:prom_counter_add(manager_metrics->metrics_delete_request,count,0);
                    //prom_counter_inc(manager_metrics->metrics_delete_request,0);
                    break;   
            case 4:prom_counter_add(manager_metrics->metrics_report_request,count,0);
                    //prom_counter_inc(manager_metrics->metrics_report_request,0);
                    break;   
			case 5:	//printf("5==double=%f:count=%f\n",(double)count,count); 
					prom_counter_add(manager_metrics->c_pkt,count,0);
                    //prom_counter_inc(manager_metrics->metrics_report_request,0);
                    break;   
			case 6: //printf("6==double=%f:count=%f\n",(double)count,count); 
					prom_counter_add(manager_metrics->total__availabe_pkts,count,0);
                    //prom_counter_inc(manager_metrics->metrics_report_request,0);
                    break;   
            default:printf("invalid insert=%s %d\n",__FILE__,__LINE__);
                    break;     
        }
        //prom_counter_inc(manager_metrics->metrics_est_request,(const char*[]){"/metrics",method,"success"});
        //printf("successful_task_completed\n");
    }
}
void successful_task_responce(  const char *method,int msgtype,double count)
{
    if(manager_metrics)
    {
        switch(msgtype)
        {
            case 1: 
                    prom_counter_add(manager_metrics->metrics_est_responce,count,0);
                    //prom_counter_inc(manager_metrics->metrics_est_responce,0);
                    break;
            case 2:prom_counter_add(manager_metrics->metrics_mod_responce,count,0);
                    //prom_counter_inc(manager_metrics->metrics_mod_responce,0);
                    break;  
            case 3:prom_counter_add(manager_metrics->metrics_delete_responce,count,0);
                    //prom_counter_inc(manager_metrics->metrics_delete_responce,0);
                    break;
            case 4:prom_counter_add(manager_metrics->metrics_report_responce,count,0);
                    //prom_counter_inc(manager_metrics->metrics_report_responce,0);
                    break;     
            default:printf("invalid insert=%s %d\n",__FILE__,__LINE__);
                    break;          
        }
        //prom_counter_inc(manager_metrics->metrics_est_request,(const char*[]){"/metrics",method,"success"});
        //printf("successful_task_completed\n");
    }
}

void packets_proccesed_promo(  const char *method,int data,double count)
{
    if(manager_metrics)
    {
        //printf("data=%d",data);
        switch(data)
        {
            case 1: prom_counter_add(manager_metrics->metrics_gtp_recvied,count,0);
                    break;
            case 2:prom_counter_add(manager_metrics->metrics_gtp_allowed,count,0);
                    break;  
            case 3:prom_counter_add( manager_metrics->metrics_gtp_dropped,count,0);
                    break;   
            case 4:prom_counter_add( manager_metrics->metrics_gi_recvied,count,0);
                    break;  
            case 5:prom_counter_add( manager_metrics->metrics_gi_allowed,count,0);
                    break; 
            case 6:prom_counter_add( manager_metrics->metrics_gi_dropped,count,0); 
                    break;   
            default:printf("invalid insert=%s %d\n",__FILE__,__LINE__);
                    break;     
        }
        //prom_counter_inc(manager_metrics->metrics_est_request,(const char*[]){"/metrics",method,"success"});
        //printf("successful_task_completed\n");
    }
}


int gauge_test(int count)
{
    // const char *labels[] = {"example_labe"};
     if(manager_metrics)
    { 
        //int ret= prom_gauge_add(manager_metrics->prom_gauge_test, (double)count,labels);
        prom_gauge_set(manager_metrics->prom_gauge_test, (double)count,NULL);
    }
    return 0;
}

//typedef int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen);
int accept_policy_callback(void *cls, const struct sockaddr *addr, socklen_t addrlen)
//int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen)
{
    //printf("function1\n");
	//uint32_t *ipv4=inet_addr("192.168.144.27");
	if(addr->sa_family == AF_INET)
    {
        struct sockaddr_in *client_addr = (struct sockaddr_in *)addr;
        // Use inet_pton for better practice
        if (inet_pton(AF_INET,promo_config->pfcp_IPv4, &client_addr->sin_addr) <= 0) {
            printf("Invalid IP format\n");
            return MHD_NO;
        }


        char ipv4[32];
        memset(ipv4,0,sizeof(ipv4));

        if(inet_ntop(AF_INET,&client_addr->sin_addr,ipv4,sizeof(ipv4))<0)
        {
            printf("incoorect ip=%s\n",ipv4);
            return MHD_NO;
        }
        //printf("inetpton=%s:%s:0x%08X\n",ipv4,promo_config->pfcp_IPv4,ntohl(client_addr->sin_addr.s_addr));
        if(strcmp(ipv4,promo_config->pfcp_IPv4) !=0)
        {
            printf("string comapre failed=%s:%s\n",ipv4,promo_config->pfcp_IPv4);
            return MHD_NO;  
        }
        return MHD_YES; 
    }
    //printf("function1\n");
    return MHD_NO;  
}
int metrics_handler(void *cls, struct MHD_Connection *connection, const char *url,
    const char *method, const char *version, const char *upload_data,
    size_t *upload_data_size, void **ptr) 

{
    //printf("metrics_handelr1\n");
    struct MHD_Response *response;
    int ret;
    prom_collector_registry_t *registry ;
    // Create a new collector registry
    const char *registry_name = "my_registry";
    registry = prom_collector_registry_new(registry_name);
    if (registry == NULL) 
    {
        perror("Failed to create collector registry");
        return EXIT_FAILURE;
    }
    //printf("metrics_handelr2\n");
    //char *metrics = promhttp_metric_handler(NULL); 
    //char *metrics =  prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
     char *metrics =(char*) prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
    if (!metrics) 
    {
        fprintf(stderr, "Error: Failed to collect Prometheus metrics\n");
        return MHD_NO;
    } 
    response = MHD_create_response_from_buffer(strlen(metrics), (void *)metrics, MHD_RESPMEM_MUST_COPY);
    free(metrics);
    
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    //printf("metrics_handler ret=%d\n",ret);
    return ret;
}

void *start_metrics_server(void *port) {
    struct MHD_Daemon *daemon1;
    //printf("start_metrics_server port=%d\n",*(int*)port);
    //int port_number =*(int*)port; //atoi((const char *)port)
    daemon1 = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,*(int*)port, accept_policy_callback, NULL, &metrics_handler, NULL, MHD_OPTION_END);

    if (NULL == daemon1)
    {
        fprintf(stderr, "Error:start_metrics_server  failed to start HTTP server\n");
        return NULL;
    }

    //getchar(); // Wait for user input to stop the server
    //MHD_stop_daemon(daemon1);
    //return 1;
}

//int promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
    //const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) 
//{
	
       // printf("Request for URL: %s\n", url);

       /* if (strcmp(url, "/metrics") == 0) {
           // char *metrics_buf = NULL;
            //size_t metrics_size = 0;
            printf("urlmetrics\n");
            // Collect Prometheus metrics
            const char *metrics_buf=prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
            size_t metrics_size = strlen(metrics_buf);
            struct MHD_Response *response_obj;
            response_obj = MHD_create_response_from_buffer(metrics_size, (void*)metrics_buf, MHD_RESPMEM_MUST_COPY);
    
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response_obj);
            MHD_destroy_response(response_obj);
            return ret;
        }

        // Respond with a simple HTTP status (you can modify this to return actual metrics)
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, Prometheus!";
        struct MHD_Response *response_obj;
        response_obj = MHD_create_response_from_buffer(strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response_obj);
       // MHD_destroy_response(response_obj);
        return ret;*/
//}
/*void *start_server(void *port) {
    struct MHD_Daemon *daemon;
    printf("start_metrics_server port=%d\n",atoi((const char *)port));
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,atoi((const char *)port),NULL,NULL,promhttp_handler,NULL,MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Error: start_server failed to start HTTP server\n");
        return NULL;
    }

    getchar(); // Wait for user input to stop the server
    //MHD_stop_daemon(daemon);
    //return NULL;
}*/
//gcc metrics_link.c -o metrics_link -lmicrohttpd -lpromhttp
//gcc -fPIC -shared metrics_link.c -o libmetrics_link.so
/*
int app_config__get_int( json_t * json_config, char * key, int defval)
{
    //printf("defval=%d key=%s\n",defval,key);
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		return json_integer_value( jObj);
	}
	return defval;
}

const char * app_config__get_str( json_t * json_config, char * key)
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
    //printf("defval=%s key=%s\n",vItem,key);
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
*/
//gcc aero_test.c metrics_link.c -o metrics_link -lmicrohttpd -lpromhttp -lprom -lpthread -ljansson -DENABLE_PROMETHEUS
//gcc -shared -fPIC metrics_link.c -o libmetricslink.so -lmicrohttpd -lpromhttp -lprom -lpthread -ljansson
//int main(int argc,char *argv[])
//int aero_metrics()
// void metrics_test(char *config)
// {
//     //printf("in main function\n");
//     json_error_t error;
// 	memset( &error, 0, sizeof( json_error_t));
// 	//json_t * json_config = json_load_file( argv[1], 0, &error);
// 	json_t * json_config = json_load_file( config, 0, &error); //fro object .so file
// 	if(!json_config)
// 	{
// 		printf("error in json %s file at line=%d column=%d position=%d\n", config, error.line, error.column, error.position);
// 		exit(0);
// 	}
//     else
//     {
//         //printf("json_config=%s\n",(char*)json_config);
//     }
//     promo_config=(app_promo_t*)malloc(sizeof(app_promo_t));
//     json_t * pfcpObj = json_object_get( json_config, "Promo");
	
// 	if( pfcpObj)
// 	{
//         //printf("finded pfcpObj\n");
//         promo_config->promotheus_Enabled = app_config__get_int( pfcpObj, "Enable", 0);
//         printf("promo_config->promotheus_Enabled=%d\n",promo_config->promotheus_Enabled);
//         if(promo_config->promotheus_Enabled ==1)
//         {
       
//         app_config__set_str( pfcpObj, "IPv4", promo_config->pfcp_IPv4, 20);
//         promo_config->pfcp_Port 				= app_config__get_int( pfcpObj, "Port", 9521);	
//         }
//     }

//     printf("promo_config->pfcp_IPv4=%s,promo_config->pfcp_Port=%d\n",promo_config->pfcp_IPv4,promo_config->pfcp_Port );

//     manager_metrics =(metrics_t*)malloc(sizeof(metrics_t));
//     if(!manager_metrics)
//     {
//         printf("memory_allocationfailed=%d\n",__LINE__);
//         exit(1);
//     }

//     struct MHD_Daemon *daemon,*daemon1;


//     //struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
//    // void *apc_cls)
//     //daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,PORT,accept_policy_callback,NULL,promhttp_handler,NULL,MHD_OPTION_END);
// 	//printf("outside main function\n"); 

//    // if (daemon == NULL) {
//       //  fprintf(stderr, "Failed to start the HTTP server\n");
//       //  return 1;
//    // }
//     pthread_t server_thread, metrics_server_thread;
//     /*if (pthread_create(&server_thread, NULL, start_server, IP)) {
//         fprintf(stderr, "Error: failed to create server thread\n");
//        exit(1);
//     }
//     else
//     {
//         printf("server_thread successful\n");
//     }*/
//    // char *port = "9527"; // Define an int variable

//     if (pthread_create(&metrics_server_thread, NULL, start_metrics_server, &promo_config->pfcp_Port )) {
//         fprintf(stderr, "Error: failed to create metrics server thread\n");
//        exit(1);
//     }
//     else
//     {
//         printf("start_metrics_server successful\n");
//     }
//    // start_metrics_server();
//     metrics_init(manager_metrics);
   
//     //aero();
//     printf("Server running on port %d...\n", promo_config->pfcp_Port);
//     //promhttp_start_daemon();
//     //pthread_join(server_thread, NULL);
//     pthread_join(metrics_server_thread, NULL);
//     // Wait for incoming requests (this could be a blocking operation)
//     //getchar(); // Wait for a key press to exit (just for testing purposes)
//     //free(manager_metrics);
//     //MHD_stop_daemon(daemon);
//    // return 0;
// }


//below is without individual promotheus config_file
//gcc upf_stats.c pfcp_metric.c -o metrics_link -lmicrohttpd -lpromhttp -lprom -lpthread -ljansson -DENABLE_PROMETHEUS
void metrics_prometheus(char * ipv4,int port) 
{
    if(!manager_metrics)
    {
        promo_config=(app_promo_t*)malloc(sizeof(app_promo_t));
        
        memset(promo_config->pfcp_IPv4, 0, sizeof(promo_config->pfcp_IPv4));
        strcpy(promo_config->pfcp_IPv4,ipv4);
        promo_config->pfcp_Port=port; 
        
        printf("promo_config->pfcp_IPv4=%s,promo_config->pfcp_Port=%d\n",promo_config->pfcp_IPv4,promo_config->pfcp_Port );
        
        
        
        manager_metrics =(metrics_t*)malloc(sizeof(metrics_t));
        memset( manager_metrics, 0, sizeof(metrics_t));
        if(!manager_metrics)
        {
            printf("memory_allocationfailed=%d\n",__LINE__);
            exit(1);
        }

        struct MHD_Daemon *daemon;

        pthread_t  metrics_server_thread;

        if (pthread_create(&metrics_server_thread, NULL, start_metrics_server, &promo_config->pfcp_Port )) {
            fprintf(stderr, "Error: failed to create metrics server thread\n");
        exit(1);
        }
        else
        {
            printf("start_metrics_server successful\n");
        }
    
        metrics_init(manager_metrics);
    
        printf("Server running on port %d...\n", promo_config->pfcp_Port);
    
        pthread_join(metrics_server_thread, NULL);
        
        //free(manager_metrics);
        //MHD_stop_daemon(daemon);
         // return 0;
    }
}
