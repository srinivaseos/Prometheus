#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <prom.h>
#include <promhttp.h> 
#include <microhttpd.h>
#include<prom_alloc.h>

int metrics_test(char *config);
//int successfull_task_request( const char *url, const char *method);
int successfull_task_req(const char *method,int vaue,int msg_type);  
int deletionfull_task_req(  const char *method,int msg_type);


#define PFCP_EST_REQUEST       1
#define PFCP_MOD_REQUEST    	2
#define PFCP_REPORT_REQUEST	3
#define PFCP_DELETE_REQUEST    4 

#define PFCP_EST_RESPONCE       5
#define PFCP_MOD_RESPONCE    	 6
#define PFCP_REPORT_RESPONCE	 7
#define PFCP_DELETE_RESPONCE    8 
