#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include<prom_alloc.h>
#include "prom_collector.h"
#include "prom_metric.h"
//#include "metrics_link.h"

#define PORT 9090
#define IP "10.0.2.15"
 
typedef struct metrics
{
    prom_counter_t *metrics_est_request;
    prom_counter_t *metrics_est_responce;
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


int metrics_init()
{
    printf("metrics_handler2\n");
    manager_metrics =(metrics_t*)prom_malloc(sizeof(metrics_t));
    if (!manager_metrics) {
        printf("Memory allocation failed\n");
        free(manager_metrics);

        exit(1);
    }
    // Initialize the Prometheus library
    prom_collector_registry_default_init();
    promhttp_set_active_collector_registry(NULL);
    // Create a counter metric
    //manager_metrics->metrics_est_request = (prom_collector_registry_must_register_metric(prom_counter_new("http_requests_total", "Total number of HTTP requests",0,NULL));
    manager_metrics->metrics_est_request = prom_collector_registry_must_register_metric(prom_counter_new("http_requests_total", "Total number of HTTP requests", 3, (const char*[]){"url","method","success"}));
    manager_metrics->metrics_est_responce = prom_collector_registry_must_register_metric(prom_counter_new("http_responses_total", "Total number of HTTP responses", 3, (const char*[]){"url","method","success"}));

    if (!manager_metrics->metrics_est_request || !manager_metrics->metrics_est_responce) {
    perror("Error creating metric");

    exit(1);
    }
    printf("must register=%d\n",__LINE__);
    // Register the counter with the default registry
    //if(prom_collector_registry_must_register_metric(manager_metrics->metrics_est_request)!=0)
    //{
      //  printf("metrics_est_responce must register=%d\n",__LINE__);
    	//perror("failed to register\n");
    	//exit(1);
    //}
   //if((prom_collector_registry_must_register_metric(manager_metrics->metrics_est_responce))!=0)
	//{
	 // printf("metrics_est_responce must register=%d\n",__LINE__);
	  //exit(1);
	//}
    // Increment the counter
    //prom_counter_inc(manager_metrics->metrics_est_request,0);
    //prom_counter_inc(manager_metrics->metrics_est_request, (const char*[]){"home", "GET", "1"});


    // Cleanup before exiting
    //prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);
}

//int successfull_task_request( const char *url, const char *method)
int successfull_task_req(  const char *method)
{
    prom_counter_inc(manager_metrics->metrics_est_request,(const char*[]){"user","POST","success"});
    printf("successful_task_completed\n");
}

int aero()
{
    const char *method="POST";   
    for(int i=0;i<10;i++)
    {
        successfull_task_req(method);
    }
}
//typedef int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen);
int accept_policy_callback(void *cls, const struct sockaddr *addr, socklen_t addrlen)
//int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen)
{
    printf("function1\n");
	//uint32_t *ipv4=inet_addr("192.168.144.27");
	if(addr->sa_family == AF_INET)
    {
        struct sockaddr_in *client_addr = (struct sockaddr_in *)addr;
        // Use inet_pton for better practice
        if (inet_pton(AF_INET, IP, &client_addr->sin_addr) <= 0) {
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
        //printf("inetpton=%s\n",ipv4);
        if(strcmp(ipv4,IP) !=0)
        {
            printf("string comapre failed=%s:%s\n",ipv4,IP);
        }
        return MHD_YES; 
    }
    return MHD_NO;  
}

int metrics_handler(void *cls, struct MHD_Connection *connection, const char *url,
                    const char *method, const char *version, const char *upload_data,
                    size_t *upload_data_size, void **ptr) {
	printf("usr handel=%s:%s LINE=%d\n",url,method,__LINE__);   
	 struct MHD_Response *response;
    int ret;
	prom_collector_registry_t *registry ;
	// Create a new collector registry
    const char *registry_name = "my_registry";
    registry = prom_collector_registry_new(registry_name);
    if (registry == NULL) {
        perror("Failed to create collector registry");
        return EXIT_FAILURE;
    }
    //char *metrics = promhttp_metric_handler(NULL);
    const char *metrics =  prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);

    response = MHD_create_response_from_buffer(strlen(metrics), (void *)metrics, MHD_RESPMEM_MUST_COPY);
    //free(metrics);

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}
int promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
    const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) 
{
	
        printf("Request for URL: %s\n", url);

        if (strcmp(url, "/metrics") == 0) {
           // char *metrics_buf = NULL;
            //size_t metrics_size = 0;
            printf("urlmetrics\n");
            // Collect Prometheus metrics
            //const char *metrics_buf=prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
            //size_t metrics_size = strlen(metrics_buf);
            struct MHD_Response *response_obj;
            response_obj = MHD_create_response_from_buffer(strlen("user_created"), (void*)"usr_Creared", MHD_RESPMEM_MUST_COPY);
    
            int ret = MHD_queue_response(connection, MHD_HTTP_OK, response_obj);
            MHD_destroy_response(response_obj);
            return ret;
        }

        // Respond with a simple HTTP status (you can modify this to return actual metrics)
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, Prometheus!";
        struct MHD_Response *response_obj;
        response_obj = MHD_create_response_from_buffer(strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response_obj);
       MHD_destroy_response(response_obj);
        return ret;;
}
void *start_server(void *port) {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,PORT,accept_policy_callback,NULL,promhttp_handler,NULL,MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Error: start_server failed to start HTTP server\n");
        return NULL;
    }

    getchar(); // Wait for user input to stop the server
    //MHD_stop_daemon(daemon);
    //return NULL;
}

void *start_metrics_server(void *port) {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, atoi((const char *)port), NULL, NULL, &metrics_handler, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Error:start_metrics_server  failed to start HTTP server\n");
        return NULL;
    }

    getchar(); // Wait for user input to stop the server
    MHD_stop_daemon(daemon);
    return NULL;
}

//gcc metrics_link.c -o metrics_link -lmicrohttpd -lpromhttp
//gcc -fPIC -shared metrics_link.c -o libmetrics_link.so

int main()
//int aero_metrics()
{
    printf("in main function\n");
	
	
   
    struct MHD_Daemon *daemon,*daemon1;


    //struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,void *apc_cls)
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,PORT,accept_policy_callback,NULL,promhttp_handler,NULL,MHD_OPTION_END);
	printf("outside main function\n"); 

    if (daemon == NULL) {
        fprintf(stderr, "Failed to start the HTTP server\n");
        return -1;
    }

    int res =metrics_init();
	    if(res <=0)
	    {
		printf("metrics_init failed\n");
		return -1;
	    }
    start_metrics_server((void*)9090);
    
    aero();
    printf("Server running on port %d...\n", PORT);
   
    // Wait for incoming requests (this could be a blocking operation)
    getchar(); // Wait for a key press to exit (just for testing purposes)
    MHD_stop_daemon(daemon);
    return 0;
}


