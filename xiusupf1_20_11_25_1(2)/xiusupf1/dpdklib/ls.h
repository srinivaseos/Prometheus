#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <resolv.h>
#include <ifaddrs.h>

#ifndef LIB_LS_H
#define LIB_LS_H



#pragma pack(4)
typedef struct ls_uint_entry
{
	struct ls_uint_entry 	* Next;
	struct ls_uint_entry 	* GHead;
	struct ls_uint_entry 	* GCurr;
	struct ls_uint_entry 	* GNext;
	pthread_mutex_t 		  Glock;
	uint32_t 				key32;
	//__uint128_t 			key128;	
	uint64_t				key641;
	uint64_t				key642;
	uint8_t 				* dataPtr;
} ls_uint_entry_t;


#pragma pack(4)
typedef struct ls_uint_table 
{
	struct ls_uint_table * Next;

	uint32_t 			key32;
	//__uint128_t 		key128;
	uint64_t			key641;
	uint64_t			key642;
	uint32_t 			keysize;
	ls_uint_entry_t ** 	entries;
	pthread_mutex_t 	lock;
	uint32_t			capacity;
	uint32_t			count;
} ls_uint_table_t;


#pragma pack(4)
typedef struct ls_ins 
{
	ls_uint_table_t * tHead;
	ls_uint_table_t * tCurrent;
	pthread_mutex_t   tLock;
	
	ls_uint_entry_t * 	PoolHead;
	ls_uint_entry_t * 	PoolCurr;
	pthread_mutex_t 	PoolLock;
} ls_ins_t;


#pragma pack(4)
typedef struct ExecutionTime
{
	struct timeval before;
	struct timeval after;
	struct timeval lapsed;
} iExecTime; 



void 				ls__init( int poolsize);

ls_uint_table_t * 	ls__create_uint_table( uint32_t capacity, uint32_t keysize, int addToroot);
ls_uint_table_t * 	ls__create_u128_table( uint32_t capacity, uint32_t keysize, int addToroot);

int 				ls__setobject_u32k_lm( 	ls_uint_table_t * table, uint32_t key, uint8_t * dptr);
uint8_t * 			ls__getobject_u32k( 	ls_uint_table_t * table, uint32_t key);
int 				ls__delobject_u32k( 	ls_uint_table_t * table, uint32_t key);

int 				ls__setobject_u64k_lm( ls_uint_table_t * table, uint64_t key, uint8_t * dptr);
uint8_t * 			ls__getobject_u64k( 	ls_uint_table_t * table, uint64_t key);
int 				ls__delobject_u64k( 	ls_uint_table_t * table, uint64_t key);

int 				ls__setobject_u128_lm( 	ls_uint_table_t * table, uint8_t * key, uint8_t * dptr);
uint8_t * 			ls__getobject_u128k( 	ls_uint_table_t * table, uint8_t * key);
int 				ls__delobject_u128k( 	ls_uint_table_t * table, uint8_t * key);


#endif











