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

#include "app_stack.h"
#include "app_jet_message.h"

#ifndef S_APP_AERO
#define S_APP_AERO



#pragma pack(4)
typedef struct app_aerospike
{
	char IP[20];
	int Port;
	
	char AerospikeIP[5][20];
	int AerospikeIPCount;
	int AerospikePort;
	char AerospikeNamespace[100];
	
	aerospike * as;
	int as_connected;

	app_logger_t * appLogger;

} app_aerospike_t;


app_aerospike_t * __appAero;

typedef struct app_ep_stack__tcp_client app_ep_stack__tcp_client_t;




aerospike * app_aero__as_connection();
void app_aero__reset();


#endif