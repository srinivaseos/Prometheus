#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>

#include "prom_metric.h"
#include "prom_collector.h"
#include "prom_collector_registry.h"
#define PORT 9093
#define IP "10.0.2.15"

typedef struct metrics
{
    prom_counter_t *request_serverd;
    prom_counter_t *responce_serverd;

}metrics_t;

metrics_t *metrics_manager=NULL;

void init_metrics(metrics_t *metrics_manager)
{
   /// Check for null pointer before using
    if (!metrics_manager) {
        fprintf(stderr, "metrics_manager is NULL\n");
        return;
    }

    prom_collector_registry_default_init();
    //promhttp_set_active_collector_registry(NULL);

    // Create and check the registry
    prom_collector_registry_t *registry = prom_collector_registry_new("default_registry");
    if (!registry) {
        fprintf(stderr, "Failed to create Prometheus registry\n");
        return;
    }

    // Create a new metric
    metrics_manager->request_serverd = prom_counter_new("UPF_SESSION_EST_REQ", "SESSION_ESTABLISHMENT_REQUEST", 0, NULL);
    if (!metrics_manager->request_serverd) {
        fprintf(stderr, "Failed to create counter metric\n");
        return;
    }

    // Register the metric with the registry
    if (!prom_collector_registry_must_register_metric(metrics_manager->request_serverd))!=0 ){   
        fprintf(stderr, "Failed to register metric\n");
        exit(1);
    }
}

//typedef int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen);
int accept_policy_callback(void *cls, const struct sockaddr *addr, socklen_t addrlen)
//int (*MHD_AcceptPolicyCallback)(void *cls, const struct sockaddr *addr, socklen_t addrlen)
{
    printf("function1");
	//uint32_t *ipv4=inet_addr("192.168.144.27");
	if(addr->sa_family == AF_INET)
    {
        struct sockaddr_in *client_addr = (struct sockaddr_in *)addr;
	
	// Use inet_pton for better practice
        if (inet_pton(AF_INET, IP, &client_addr->sin_addr) <= 0) {
            printf("Invalid IP format\n");
            return MHD_NO;
        }
        //client_addr->sin_addr.s_addr=inet_addr("192.168.144.27");
        //client_addr->sin_port = htons(9093);

        char ipv4[32];
        memset(ipv4,0,sizeof(ipv4));

        if(inet_ntop(AF_INET,&client_addr->sin_addr,ipv4,sizeof(ipv4)) == NULL)
        {
            printf("incoorect ip=%s\n",ipv4);
            return MHD_NO;
        }
	printf("Client IP: %s\n", ipv4);
        if(strcmp(ipv4,IP) <0)
        {
            printf("string comapre failed=%s",ipv4);
        }
        return MHD_YES; 
    }
    return MHD_NO;  
}

int promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
    const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) 
{
	
        printf("Request for URL: %s\n", url);
    
        // Respond with a simple HTTP status (you can modify this to return actual metrics)
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, Prometheus!";
        struct MHD_Response *response_obj;
        response_obj = MHD_create_response_from_buffer(strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response_obj);
        MHD_destroy_response(response_obj);
        return ret;;
}

//gcc metrics_link.c -o metrics_link -lmicrohttpd -lpromhttp

int main()
{
    printf("in main function\n");

    //metrics_manager = (metrics_t*)malloc(sizeof(metrics_t));

    metrics_manager = (metrics_t*)malloc(sizeof(metrics_t));
    if (!metrics_manager) {
    perror("Failed to allocate memory for metrics_manager");
    return 1;  // Exit if allocation fails
}

    init_metrics(metrics_manager);
    //struct  MHD_daemon  * daemon;
    struct MHD_Daemon *daemon;


    //struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
   // void *apc_cls)
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,PORT,accept_policy_callback,NULL,promhttp_handler,NULL,MHD_OPTION_END);
	printf("outside main function\n"); 

    if (daemon == NULL) {
        fprintf(stderr, "Failed to start the HTTP server\n");
        return 1;
    }

    printf("Server running on port %d...\n", PORT);
	// Wait for incoming requests (this could be a blocking operation)
    getchar(); // Wait for a key press to exit (just for testing purposes)
    MHD_stop_daemon(daemon);
    return 0;
}


    
   
   

