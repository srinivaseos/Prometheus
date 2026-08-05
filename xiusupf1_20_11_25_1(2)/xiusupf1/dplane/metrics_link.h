#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void metrics_prometheus(char * ipv4,int port);

#ifndef ENABLE_PROMETHEUS
#define ENABLE_PROMETHEUS

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include<prom_alloc.h>

void metrics_prometheus(char * ipv4,int port);
//void metrics_test(char *config);
//int successfull_task_request( const char *url, const char *method);
void successful_task_request(  const char *method,int data);
void successful_task_responce(  const char *method,int data);
void packets_proccesed_promo(  const char *method,int data,int count);

/* 1. establishment 2.modification 3.report 4.delete*/

#endif