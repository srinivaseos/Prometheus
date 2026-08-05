#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>



#ifndef ENABLE_PROMETHEUS
#define ENABLE_PROMETHEUS

#ifdef ENABLE_PROMETHEUS

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include <prom_alloc.h>

void metrics_prometheus(char * ipv4,int port); 
//int successfull_task_request( const char *url, const char *method);
void successful_task_request(  const char *method,int data,double count);
void successful_task_responce(  const char *method,int data,double count); 
void packets_proccesed_promo(  const char *method,int data,double count);

int gauge_test(int count);

#endif // ENABLE_PROMETHEUS

#endif  // PFCP_METRIC_H