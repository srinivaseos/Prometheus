#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include<prom_alloc.h>

//int aero();
int metrics_test(char *config);
int successfull_task_request( const char *url, const char *method);
int successfull_task_req(const char *method);
int deletionfull_task_req(  const char *method);