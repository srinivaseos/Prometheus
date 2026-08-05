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
#include <ctype.h>
#include <sys/utsname.h>
#include <math.h>

#include "app_stack.h"


void init_rand()
{
	srand(time(NULL));
}

uint32_t get_rand_number( uint32_t min, uint32_t max)
{
	return (rand() % max) + min;
}


void app_start_time( app_exec_time_t * o_exec_time)
{
	gettimeofday( &o_exec_time->before, NULL);
}


void app_end_time( app_exec_time_t * o_exec_time)
{
	gettimeofday( &o_exec_time->after, NULL);
	
	if ( o_exec_time->before.tv_usec > o_exec_time->after.tv_usec)
	{
		o_exec_time->after.tv_usec += 1000000;
		o_exec_time->after.tv_sec--;
	}
	
	o_exec_time->lapsed.tv_usec = o_exec_time->after.tv_usec - o_exec_time->before.tv_usec;
	o_exec_time->lapsed.tv_sec  = o_exec_time->after.tv_sec  - o_exec_time->before.tv_sec;	
}


void app_end_full_time( app_exec_time_t * o_exec_time, long * days, long * hours, long * minutes, long * seconds) 
{
	long total_seconds = o_exec_time->lapsed.tv_sec;
	//long total_microseconds = o_exec_time->lapsed.tv_usec;

	// Convert total seconds to hours, minutes, seconds
	*days 		= total_seconds / 86400;  // 86400 seconds in a day
	*hours 		= (total_seconds % 86400) / 3600;
    *minutes 	= (total_seconds % 3600) / 60;
    *seconds 	= total_seconds % 60;

    //printf("Time difference: %ld hours, %ld minutes, and %ld seconds\n", hours, minutes, seconds);
}


typedef struct app_counter
{
	char Name[100];
	uint64_t Count;
	uint64_t LastCount;
	pthread_mutex_t CLock;
	uint64_t RespCount;
	uint64_t RespLastCount;
	pthread_mutex_t RespCLock;
	uint64_t ErrRespCount;
	uint64_t ErrRespLastCount;
	pthread_mutex_t ErrRespCLock;
} app_counter_t;


app_counter_t * app_counter_create( char * name)
{
	app_counter_t * counter = (app_counter_t*)malloc(sizeof(app_counter_t));
	memset( counter, 0, sizeof(app_counter_t));
	strcpy( counter->Name, name);
	
	pthread_mutex_init( &counter->CLock, NULL);
	pthread_mutex_init( &counter->RespCLock, NULL);	
	pthread_mutex_init( &counter->ErrRespCLock, NULL);	
	return counter;
}

void app_counter_increment( app_counter_t * counter)
{
	pthread_mutex_lock( &counter->CLock);
	counter->Count++;
	
	if( counter->Count >= (UINT64_MAX-1))
	{
		counter->Count = 0;
		counter->LastCount = 0;
	}
	
	pthread_mutex_unlock( &counter->CLock);	
}

void app_counter_increment_r( app_counter_t * counter)
{
	pthread_mutex_lock( &counter->RespCLock);
	counter->RespCount++;
	
	if( counter->RespCount >= (UINT64_MAX-1))
	{
		counter->RespCount = 0;
		counter->RespLastCount = 0;
	}
	
	pthread_mutex_unlock( &counter->RespCLock);	
}

void app_counter_increment_e( app_counter_t * counter)
{
	pthread_mutex_lock( &counter->ErrRespCLock);
	counter->ErrRespCount++;
	
	if( counter->ErrRespCount >= (UINT64_MAX-1))
	{
		counter->ErrRespCount = 0;
		counter->ErrRespLastCount = 0;
	}
	
	pthread_mutex_unlock( &counter->ErrRespCLock);	
}


void app_counter_plog_1( app_counter_t * counter, app_logger_t * logger)
{
	uint64_t currentCount = counter->Count - counter->LastCount;
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "%s     %-8lu   %-8lu", counter->Name, currentCount, counter->Count);
	counter->LastCount = counter->Count;
}

void app_counter_plog_2( app_counter_t * counter, app_logger_t * logger)
{
	uint64_t currentCount = counter->Count - counter->LastCount;
	uint64_t currentRCount = counter->RespCount - counter->RespLastCount;
	uint64_t currentECount = counter->ErrRespCount - counter->ErrRespLastCount;
	
	if( counter->ErrRespLastCount == 0)
	{
		app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "%-30s     %-8lu   %-8lu      %-8lu   %-8lu", 
			counter->Name, currentCount, counter->Count, currentRCount, counter->RespCount);
	}
	else
	{
		app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "%-30s     %-8lu   %-8lu      %-8lu   %-8lu      %-8lu   %-8lu", 
			counter->Name, currentCount, counter->Count, currentRCount, counter->RespCount, currentECount, counter->ErrRespCount);		
	}
	
	counter->LastCount = counter->Count;
	counter->RespLastCount = counter->RespCount;
	counter->ErrRespLastCount = counter->ErrRespCount;
}

void app_printf_buf( char * key, uint8_t * u8, int len)
{
	if( key)
	{
		printf("%s = ", key);
	}
	
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf("%02X ", u8[i] & 0xFF);
	}
	printf("\n");
}



void app_printf_char( char * key, uint8_t * u8, int len)
{
	if( key)
	{
		printf("%s = ", key);
	}
	
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf("%c ", u8[i] & 0xFF);
	}
	printf("\n");
}


void app_printf_char2( char * key, uint8_t * u8, int len)
{
	if( key)
	{
		printf("%s=", key);
	}
	
	int i = 0;
	for( i = 0; i < len; i++)
	{
		printf("%c", u8[i] & 0xFF);
	}
	printf("\n");
}


void * app_bcd_to_buffer(const char *in, int in_len, void *out, int *out_len)
{
    int i = 0;
    uint8_t *out_p = out;
    //int in_len = strlen(in);

    for (i = 0; i < in_len; i++) {
        if (i & 0x01)
            out_p[i>>1] = out_p[i>>1] | (((in[i] - 0x30) << 4) & 0xF0);
        else
            out_p[i>>1] = (in[i] - 0x30) & 0x0F;
    }

    *out_len = (in_len + 1) / 2;
    if (in_len & 0x01) {
        out_p[(*out_len)-1] |= 0xF0;
    }

    return out;
}

void * app_bcd_to_buffer_reverse_order(const char *in, void *out, int *out_len)
{
    int i = 0;
    uint8_t *out_p = out;
    int in_len = strlen(in);

    for (i = 0; i < in_len; i++) {
        if (i & 0x01)
            out_p[i>>1] = out_p[i>>1] | ((in[i] - 0x30) & 0x0F);
        else
            out_p[i>>1] = ((in[i] - 0x30) << 4) & 0xF0;
    }

    *out_len = (in_len + 1) / 2;
    if (in_len & 0x01) {
        out_p[(*out_len)-1] |= 0xF0;
    }

    return out;
}

void * app_buffer_to_bcd(uint8_t *in, int in_len, void *out)
{
    int i = 0;
    uint8_t *out_p = out;

    for (i = 0; i < in_len-1; i++) {
        out_p[i*2] = 0x30 + (in[i] & 0x0F);
        out_p[i*2+1] = 0x30 + ((in[i] & 0xF0) >> 4);
    }

    if ((in[i] & 0xF0) == 0xF0) {
        out_p[i*2] = 0x30 + (in[i] & 0x0F);
        out_p[i*2+1] = 0;
    } else {
        out_p[i*2] = 0x30 + (in[i] & 0x0F);
        out_p[i*2+1] = 0x30 + ((in[i] & 0xF0) >> 4);
        out_p[i*2+2] = 0;
    }

    return out;
}


void app_buffer_to_hex(char *in, int in_len, char * out, int out_len)
{
    int i = 0, j = 0, k = 0, hex;
    uint8_t *out_p = out;

    while( i < in_len && j < out_len)
	{
		sprintf( &out[j], "%02X", in[i] & 0xFF);
		
        i++;
		j += 2;
    }
}

void * app_ascii_to_hex(char *in, int in_len, void *out, int out_len)
{
    int i = 0, j = 0, k = 0, hex;
    uint8_t *out_p = out;

    while(i < in_len && j < out_len) {
        if (!isspace(in[i])) {
            hex = isdigit(in[i]) ? in[i] - '0' : 
                islower(in[i]) ? in[i] - 'a' + 10 : in[i] - 'A' + 10;
            if ((k & 0x1) == 0) {
                out_p[j] = (hex << 4);
            } else {
                out_p[j] |= hex;
                j++;
            }
            k++;
        }
        i++;
    }

    return out;
}

void * app_hex_to_ascii(void *in, int in_len, void *out, int out_len)
{
    char *p;
    int i = 0, l, off = 0;

    p = out;
    p[0] = 0;

    l = (in_len - off) > out_len ? out_len : in_len - off;
    for (i = 0; i < l; i++) {
        p += sprintf(p, "%02x", ((char*)in)[off+i] & 0xff);
    }

    return out;
}


app_logger_t * logger = NULL;
void app_stack__set_logger( app_logger_t * loggerInstance)
{
	logger = loggerInstance;
}

int32_t lTimeVal = 19800;

void app_set_localtime( int lt)
{
	lTimeVal = lt;
}

void app_makeTimeStamp( char *datestring)
{
	// time_t rawtime;
	// struct tm * timeinfo;
	// struct timeval tval;
	// struct timezone tzone;
	// time( &rawtime);
	// timeinfo = localtime( &rawtime);
	// gettimeofday( &tval, &tzone);
	// sprintf( datestring, "%02d-%02d-%d:%02d:%02d:%02d:%06lu", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tval.tv_usec);
	// datestring[27] = 0;

	struct timeval tval;
	struct timezone tzone;	
	gettimeofday( &tval, &tzone);
	
	if( lTimeVal > 0) {
		tval.tv_sec += lTimeVal;
	}
	
	struct tm * timeinfo = gmtime( &tval.tv_sec);
	sprintf( datestring, "%02d-%02d-%d:%02d:%02d:%02d:%06lu", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tval.tv_usec);
	datestring[27] = 0; 	
}

void app_makeTimeStamp2( char *datestring)
{
	// time_t rawtime;
	// struct tm * timeinfo;
	// struct timeval tval;
	// struct timezone tzone;
	// time( &rawtime);
	// timeinfo = localtime( &rawtime);
	// gettimeofday( &tval, &tzone);
	// sprintf( datestring, "%02d-%02d-%d:%02d:%02d:%02d", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	// datestring[27] = 0;  

	struct timeval tval;
	struct timezone tzone;	
	gettimeofday( &tval, &tzone);	
	
	if( lTimeVal > 0) {
		tval.tv_sec += lTimeVal;
	}
	
	struct tm * timeinfo = gmtime( &tval.tv_sec);	
	sprintf( datestring, "%02d-%02d-%d:%02d:%02d:%02d", timeinfo->tm_mday, (timeinfo->tm_mon) + 1, (timeinfo->tm_year) + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	datestring[27] = 0; 	
}

void app_makeFileName(char *mFileNameBuffer, char * mPath, char * mFilePrefix)
{
	// time_t the_time;
	// struct tm *tm_ptr;
	// time( &the_time);
	// tm_ptr = localtime( &the_time);
	
	struct timeval tval;
	struct timezone tzone;	
	gettimeofday( &tval, &tzone);	
	
	if( lTimeVal > 0) {
		tval.tv_sec += lTimeVal;
	}
	
	struct tm * tm_ptr = gmtime( &tval.tv_sec);
	
	sprintf( mFileNameBuffer, "%s/%s_%02d_%02d_%02d_%02d_%02d_%02d.log", mPath, mFilePrefix, tm_ptr->tm_mday, (tm_ptr->tm_mon + 1), (tm_ptr->tm_year + 1900), tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
}


char * app_is_ubuntu()
{
	struct utsname name;
	uname( &name);
	
	return strstr( name.version, "Ubuntu");
}


int app_parse_output( char * cmd, char * buf) 
{
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) 
	{
        printf("Error opening pipe!\n");
        return -1;
    }

	int i = 0;
	int j = 0;
	char line[100];
	memset( line, 0, sizeof(line));
    
	while (fgets( line, 512, fp) != NULL)
	{
		i = strlen(line);
		
		if( i >= 1)
		{
			line[i-1] = ' ';
			memcpy( &buf[j], line, i);
			j += i;
		}
		
		memset( line, 0, sizeof(line));
	}
	

    if (pclose(fp)) 
	{
        printf("Command not found or exited with error status\n");
        return -1;
    }

    return 0;
}


int app_validate_osv( char * name, char * version)
{
	char output[512];
	memset( output, 0, sizeof(output));
	
	app_parse_output( "lsb_release -a", output);
	
	char * os_name 		= strstr( output, name);
	char * os_version 	= strstr( output, version);
	
	if(!os_name)
	{
		printf("OS Info:%s\n", output);
		printf("-1: Compiled only for %s %s\n", name, version);
		exit(0);
		return -1;
	}

	if(!os_version)
	{
		printf("OS Info:%s\n", output);
		printf("-2: Compiled only for %s %s\n", name, version);
		exit(0);
		return -2;
	}
	
	return 1;
}



typedef struct app_rbnode
{
	//struct app_rbnode * Left, * Right, * Parent;
	struct app_rbnode * Next;
	// struct app_rbnode * Left;
	// struct app_rbnode * Right;
	struct app_rbnode * NodeHead;
	struct app_rbnode * NodeCurrent;
	
	uint8_t pi;
	uint8_t * Parent;
	uint64_t id;
	uint32_t GenId;
	//uint32_t isFree;
} app_rbnode_t;


typedef struct app_rbtree
{
	app_rbnode_t * root;
	app_rbnode_t * current;
	//app_rbnode_t * last_node;
	app_rbnode_t ** app_rbnode_array;
	uint64_t total;
	uint32_t available;
	int beginNumber;
	pthread_mutex_t lock;
	uint64_t MaxId;
	app_data_region_t * dregion;
	app_data_pool_t * dpool;
} app_rbtree_t;

app_rbtree_t * app_rbnode__create_rbtree( int Count, int keysize)
{
	app_rbtree_t * tree = (app_rbtree_t *)malloc( sizeof(app_rbtree_t));
	memset( tree, 0, sizeof(app_rbtree_t));
	pthread_mutex_init( &tree->lock, NULL);
	tree->dregion 	= app_region__create();
	tree->dpool 	= app_region__add_pool( tree->dregion, "rbnode", sizeof(app_rbnode_t), Count * keysize);
	
	return tree;
}


app_rbnode_t * app_rbnode__findOrAddPathItem( app_rbtree_t * tree, app_rbnode_t * node, uint8_t pi, uint8_t createMode)
{
	app_rbnode_t * nodeItem = node->NodeHead;
	
	while(nodeItem)
	{
		if( nodeItem->pi == pi)
		{
			return nodeItem;
		}
		nodeItem = nodeItem->Next;
	}
	
	
	
	if( createMode == 1) {
		pthread_mutex_lock( &tree->lock);
	}
	
	app_rbnode_t * new_node = (app_rbnode_t*)app_region__allocate_fd( tree->dpool);
	memset( new_node, 0, sizeof(app_rbnode_t));
	new_node->pi = pi;
	
	if(!node->NodeHead)
	{
		node->NodeHead = node->NodeCurrent = new_node;
	}
	else
	{
		node->NodeCurrent->Next = new_node;
		node->NodeCurrent = new_node;
	}
	
	if( createMode == 1) {
		pthread_mutex_unlock( &tree->lock);
	}
	
	return new_node;
}

int app_rbnode__add_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen, uint8_t * data)
{
	if(!tree->root) 
	{
		tree->root = (app_rbnode_t*)app_region__allocate_fd( tree->dpool);
		memset( tree->root, 0, sizeof(app_rbnode_t));
		tree->root->id = 0;
	}
	
	app_rbnode_t * node = tree->root;
	
	int i = 0;
	for( i = 0; i < keylen; i++)
	{
		if( node)
		{
			node = app_rbnode__findOrAddPathItem( tree, node, key[i], 1);
		}
		else
		{
			return -1;
		}
	}
	
	if(node)
	{
		node->Parent = data;
		//printf( "data set at node=%p i=%d\n", data, i);		
	}
	return 1;
}


uint8_t * app_rbnode__rfind_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen)
{
	app_rbnode_t * node = tree->root;
	
	int i = 0;
	for( i = keylen; i > 0; i--)
	{
		if( node)
		{
			node = app_rbnode__findOrAddPathItem( tree, node, key[i-1], 0);
		}
		else
		{
			return NULL;
		}
	}
	
	if(node)
	{
		return node->Parent;
	}
	
	return NULL;	
}


uint8_t * app_rbnode__find_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen)
{
	app_rbnode_t * node = tree->root;
	
	int i = 0;
	for( i = 0; i < keylen; i++)
	{
		if( node)
		{
			node = app_rbnode__findOrAddPathItem( tree, node, key[i], 0);
		}
		else
		{
			return NULL;
		}
	}
	
	if(node)
	{
		return node->Parent;
	}
	
	return NULL;
}


void app_rbnode__setNull_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen)
{
	app_rbnode_t * node = tree->root;
	
	int i = 0;
	for( i = 0; i < keylen; i++)
	{
		if( node)
		{
			node = app_rbnode__findOrAddPathItem( tree, node, key[i], 0);
		}
		else
		{
			return;
		}
	}
	
	if(node)
	{
		node->Parent = NULL;
	}
	
	return;
}


size_t app_rbnode__getsizeof_app_rbtree()
{
	return sizeof(app_rbtree_t);
}


app_rbtree_t * app_rbnode__create_idp( int capacity, int beginNumber)
{
	if( beginNumber <= 0)
	{
		printf("rbtree_idp beginNumber should be greater than 0 %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}
	
	app_rbtree_t * tree = (app_rbtree_t *)malloc( sizeof(app_rbtree_t));
	memset( tree, 0, sizeof(app_rbtree_t));
	pthread_mutex_init( &tree->lock, NULL);
	tree->app_rbnode_array = (app_rbnode_t **)malloc( capacity * sizeof( app_rbnode_t *));
	tree->total = capacity;
	tree->available = 0;
	tree->beginNumber = beginNumber;
	//tree->root = tree->current = tree->last_node = NULL;
	tree->root = tree->current = NULL;
	
	if(!tree->app_rbnode_array)
	{
		printf("tree creation failed for rbtree_idp array %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}
	
	uint8_t * m = malloc( capacity * sizeof( app_rbtree_t));
	if(!m)
	{
		printf("tree creation failed for rbtree_idp rbnodes %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(0);
	}

	int i = 0;
	app_rbnode_t * node = NULL;
	for( i = 0; i < capacity; i++)
	{
		node = (app_rbnode_t *)m;
		//node->Left = node->Right = node->Parent = NULL;
		node->id = (i + tree->beginNumber);
		node->GenId = 1;
		//node->isFree = 1;
		node->Parent = NULL;
		//node->Right = tree->current;
		
		//printf( "i=%d node->id=%ld\n", i , node->id);
		tree->app_rbnode_array[i] = node;
		//tree->current = node;
		
		if(!tree->root) {
			tree->root = tree->current = node;
		} else {
			tree->current->Next = node;
			tree->current = node;
		}
		
		m += sizeof( app_rbtree_t);
	}
	
	//tree->root = tree->app_rbnode_array[0];
	tree->available = capacity;
	tree->MaxId = (UINT64_MAX - ( tree->total * 2 ));
	
	return tree;
}

void app_rbnode__perflog( char * name, app_rbtree_t * tree, app_logger_t * logger)
{
	app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, 
		"%-30s  TotalCount %-7ld  Available %-7ld  Pending %-7ld", name, tree->total, tree->available, tree->total - tree->available);
}

uint64_t app_rbnode__get_beginNumber( app_rbtree_t * tree)
{
	return tree->beginNumber;
}

// uint64_t app_rbnode__get_index( app_rbtree_t * tree, uint64_t base_id)
// {
	// return 0;
// }

uint64_t app_rbnode__get_baseid_idp( app_rbtree_t * tree, uint64_t id)
{
	if( id < tree->beginNumber)
		return UINT64_MAX;
	
	uint64_t baseid = (id - tree->beginNumber);
	
	//printf( "baseid(%lu) >= tree->total(%lu) L-%d\n", baseid, tree->total, __LINE__);

	if( baseid >= tree->total) {
		baseid = baseid % tree->total;
		//printf( "		baseid(%lu) >= tree->total(%lu)  L-%d\n", baseid, tree->total, __LINE__);
	}

	return baseid;
}


int app_rbnode__calc_gen_id( app_rbtree_t * tree, uint64_t id) 
{
	if( id < tree->beginNumber)
		return -1;	
	
	uint64_t baseid = (id - tree->beginNumber);
	
	if( baseid >= tree->total) 
	{
		return baseid / tree->total;
	}
	
	return 0;
}


app_rbnode_t * app_rbnode__get_idp( app_rbtree_t * tree, uint64_t id, int recover)
{
	if( id < tree->beginNumber)
		return NULL;
	
	uint64_t baseid = (id - tree->beginNumber);
	
	//printf( "baseid(%lu) >= tree->total(%lu)  L-%d\n", baseid, tree->total, __LINE__);
	
	if( baseid >= tree->total) {
		baseid = baseid % tree->total;
		//printf( "		baseid(%lu) >= tree->total(%lu)  L-%d\n", baseid, tree->total, __LINE__);
	}
	
	if( baseid >= 0 && baseid < tree->total)
	{	
		app_rbnode_t * node = tree->app_rbnode_array[ baseid ];
		
		//printf( "   -->> node=%p baseid=%lu Parent=%p  L-%d\n", node, baseid, node->Parent, __LINE__);
		
		// app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "GenId=%d baseid=%d beginNumber=%ld gen-id=%ld id=%ld match=%d  %s|%s|%d", 
			// node->GenId, baseid, tree->beginNumber, (((node->GenId-1) * tree->total) + (baseid + tree->beginNumber)), id, ( (((node->GenId-1) * tree->total) + (baseid + tree->beginNumber)) == id)?1:0, __FILE__, __FUNCTION__, __LINE__);
		
		if( recover == 0)
		{
			if( (((node->GenId-1) * tree->total) + (baseid + tree->beginNumber)) == id)	
				return node;
		}
		else
		{
			node->GenId = (id / tree->total) + 1;
			//uint64_t calcid = (((node->GenId-1) * tree->total) + (baseid + tree->beginNumber));
			
			// printf("total=%ld beginNo=%d id=%ld GenId=%d baseid=%ld G=%ld calcid=%ld\n", 
				// tree->total, tree->beginNumber, id, node->GenId, baseid, id / tree->total, calcid);

			return node;
		}
	}

	return NULL;
}


void app_rbnode__set_idp_data( app_rbtree_t * tree, uint64_t id, uint8_t * data)
{
	app_rbnode_t * node = app_rbnode__get_idp( tree, id, 0);

	if( node)
	{
		node->Parent = data;
	}
}


uint8_t * app_rbnode__find_idp( app_rbtree_t * tree, uint64_t id)
{
	app_rbnode_t * node = app_rbnode__get_idp( tree, id, 0);
	//printf("  --> id=%lu, node=%p  %s|%d\n", id, node, __FILE__, __LINE__);
	
	if( node)
	{
		return (uint8_t *)node->Parent;
	}
	return NULL;
}


void app_rbnode__set_maxid( app_rbtree_t * tree, uint64_t maxid)
{
	tree->MaxId = maxid;
}


void app_rbnode__free_idp( app_rbtree_t * tree, uint64_t id)
{
	app_rbnode_t * node = app_rbnode__get_idp( tree, id, 0);
	
	if(node) 
	{
		//node->isFree = 1;
		node->Parent = NULL;
		
		// + tree->beginNumber
		
		
		if( (node->id + tree->total) > tree->MaxId)
		{
			node->id  = app_rbnode__get_baseid_idp( tree, node->id) + tree->beginNumber;
			node->GenId = 1;
			
			// printf( "Next = %lu  MaxId=%lu  - %d\n", node->id, tree->MaxId, __LINE__);
		}
		else
		{
			node->id += tree->total;
			node->GenId++;
			
			// if( (node->id) > tree->MaxId)
			// {
				// node->id  = app_rbnode__get_baseid_idp( tree, node->id) + tree->beginNumber;
				// node->GenId = 1;				
			// }
			
			// printf( "Next = %lu  MaxId=%lu  - %d\n", node->id, tree->MaxId, __LINE__);
		}
		
		

		
		pthread_mutex_lock( &tree->lock);
		
		if(!tree->root)
		{
			tree->root = tree->current = node;
		}
		else
		{
			tree->current->Next = node;
			tree->current = node;
		}
		
		tree->available++;
		pthread_mutex_unlock( &tree->lock);
	}
}


	

void app_rbnode__set_idp( app_rbtree_t * tree, app_rbnode_t * node, uint8_t * obj)
{
	pthread_mutex_lock( &tree->lock);
	node->Parent = obj;
	
	if( tree->root == node)
	{
		tree->root = node->Next;
		node->Next = NULL;
		tree->available--;
	}
	else
	{
		app_rbnode_t * prevnode = NULL;
		app_rbnode_t * iternode = tree->root;
		
		while( iternode)
		{
			if( iternode == node)
			{
				if( prevnode)
				{
					prevnode->Next = node->Next;
					node->Next = NULL;
					tree->available--;
				}
				break;
			}
			
			prevnode = iternode;
			iternode = iternode->Next;
		}
	}
	
	pthread_mutex_unlock( &tree->lock);
}


void app_rbnode__set_id_and_gen( app_rbtree_t * tree, uint64_t index, uint64_t fc, uint32_t GenId)
{
	if( index >= 0 && index < tree->total)
	{
		app_rbnode_t * node = tree->app_rbnode_array[ index ];
		
		if( node) 
		{
			node->id 		= fc;
			node->GenId 	= GenId;
		}
	}
}

void app_rbnode__mark_as_allocate( app_rbtree_t * tree, uint64_t index, uint64_t fc, uint32_t GenId, uint8_t * obj)
{
	if( index >= 0 && index < tree->total)
	{
		app_rbnode_t * node = tree->app_rbnode_array[ index ];
		
		if( node) 
		{
			node->id 		= fc;
			node->GenId 	= GenId;
			app_rbnode__set_idp( tree, node, obj);
		}
	}
}

void app_rbnode__get_set_object_and_gen_idp( app_rbtree_t * tree, uint64_t id, uint8_t * obj)
{
	app_rbnode_t * node = app_rbnode__get_idp( tree, id, 0);
	
	//printf("id=%lu, node=%p, obj=%p Parent=%p  %s|%d\n", id, node, obj, node->Parent, __FILE__, __LINE__);
	if( node) {
		app_rbnode__set_idp( tree, node, obj);
	}
	//printf("id=%lu, node=%p, obj=%p Parent=%p  %s|%d\n", id, node, obj, node->Parent, __FILE__, __LINE__);
}


uint64_t app_rbnode__get_total( app_rbtree_t * tree)
{
	return tree->total;
}


uint64_t app_rbnode__get_available( app_rbtree_t * tree)
{
	return tree->available;
}


uint8_t * app_rbnode__get_obj_at_index( app_rbtree_t * tree, uint32_t index)
{
	if( index < tree->total )
	{
		//pthread_mutex_lock( &tree->lock);
		// uint64_t id = UINT64_MAX;
		
		// && !tree->app_rbnode_array[index]->Next
		if( tree->app_rbnode_array[index]->Parent )
		{
			return tree->app_rbnode_array[index]->Parent;
		}
		
		//pthread_mutex_unlock( &tree->lock);	
	}	
	return NULL;
}



uint64_t app_rbnode__get_next_idp( app_rbtree_t * tree, uint8_t * obj)
{
	pthread_mutex_lock( &tree->lock);
	uint64_t id = UINT64_MAX;
	
	if( tree->available > 0)
	{
		app_rbnode_t * node = tree->root;
		
		if( node)
		{
			node->Parent = obj;
			
			tree->root = node->Next;
			node->Next = NULL;
			tree->available--;
			
			// app_logger__log( logger, NULL, APP_LOG__LEVEL_CRITICAL, "allocated node=%p id=%lu[%lu] for obj=%p %s|%s|%d", 
				// node, node->id, app_rbnode__get_baseid_idp( tree, node->id), obj, __FILE__, __FUNCTION__, __LINE__);
				
			pthread_mutex_unlock( &tree->lock);
			return node->id;
		}
	}
	// else
	// {
		// printf("allocation failed\n");
	// }
	
	pthread_mutex_unlock( &tree->lock);
	return id;
}



void app_rbnode__insert( app_rbtree_t * tree, app_rbnode_t * item)
{
	// pthread_mutex_lock( &tree->lock);
	
	// if(!tree->root)
	// {
		// tree->root = item;
		// item->Parent = tree->root;
		// pthread_mutex_unlock( &tree->lock);
		// return;
	// }
	
	// app_rbnode_t * node = tree->root;
	// app_rbnode_t * last_node = NULL;
	
	// while(node)
	// {
		// last_node = node;

		// if( item->id > node->id)
		// {
			// node = node->Right;
		// }
		// else
		// {
			// node = node->Left;
		// }
	// }
	
	// if( item->id > last_node->id)
	// {
		// last_node->Right = item;
	// }
	// else
	// {
		// last_node->Left = item;
	// }
	// item->Parent = last_node;
	// pthread_mutex_unlock( &tree->lock);
}


void app_rbnode__get( app_rbnode_t * node, uint64_t * id, uint32_t * GenId)
{
	*id = node->id;
	*GenId = node->GenId;
}

app_rbnode_t * app_rbnode__find( app_rbtree_t * tree, uint64_t index)
{
	// app_rbnode_t * node = tree->root;
	// app_rbnode_t * last_node = NULL;
	
	// while(node)
	// {
		// last_node = node;

		// if( node->id == id)
		// {
			// return node;
		// }

		// if( id > node->id)
		// {
			// node = node->Right;
		// }
		// else
		// {
			// node = node->Left;
		// }
	// }

	if( index < tree->total )
	{
		//pthread_mutex_lock( &tree->lock);
		// uint64_t id = UINT64_MAX;
		
		// && !tree->app_rbnode_array[index]->Next
		//if( tree->app_rbnode_array[index]->Parent )
		//{
			return tree->app_rbnode_array[index];
		//}
		
		//pthread_mutex_unlock( &tree->lock);	
	}	
	return NULL;	
}

void app_rbnode__delete( app_rbtree_t * tree, uint64_t id)
{
	return;
}


void app__wait()
{
	while(1)
	{
		usleep( 999999);
	}
}


#define APP_LOG_MAX_CATEGORIES 							30

typedef struct app_logger_categories
{
	uint32_t id;
	uint32_t enabled;
	unsigned char Tag[8];
	uint32_t TagLen;
} app_logger_categories_t;


typedef struct app_logger
{
	int iHandle;
	unsigned char path[249];
	unsigned char prefix[50];
	unsigned char extension[10];
	uint32_t maxsize;
	uint32_t logLevel;
	uint32_t excludeTime;
	uint32_t excludeLogLevel;
	uint32_t excludeThreadId;
	pthread_mutex_t lock;
	app_logger_categories_t categories[APP_LOG_MAX_CATEGORIES];
} app_logger_t;


void app_logger__create_directory( char * dirname)
{
	struct stat sb;

	if (!(stat( dirname, &sb) == 0 && S_ISDIR( sb.st_mode)))
	{
		int iFileCreationMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		int iDirCreationMode = iFileCreationMode | S_IRWXU | S_IRWXG | S_IRWXO;
		int p = mkdir( dirname, iDirCreationMode);

		if(p == -1)
		{
			printf("Error in Creating Directory: %s\n", dirname);
			exit(0);
		}
	}
}

void app_logger__create_folder( app_logger_t * logger)
{
	int retd;
	int iUmask = S_IWGRP | S_IWOTH;
	int iFileCreationMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int iFlags = O_WRONLY | O_CREAT | O_APPEND;
	int iOldUmask = umask(iUmask);

	char path[300];
	memset( path, 0, sizeof(path));
	memcpy( path, logger->path, strlen(logger->path));
	
	retd = open( path, O_RDONLY);
	if (retd == -1)
	{
		int iDirCreationMode = iFileCreationMode | S_IRWXU | S_IRWXG | S_IRWXO;
		int p = mkdir( path, iDirCreationMode);

		if(p == -1)
		{
			printf("Error in Creating Log Directory: %s\n", path);
			exit(0);
		}		
	}
}

void app_logger__create_file( app_logger_t * logger)
{
	char mFileNameBuffer[500];
	memset( &mFileNameBuffer, 0, sizeof(mFileNameBuffer));
	app_makeFileName( mFileNameBuffer, logger->path, logger->prefix);
	logger->iHandle = creat( mFileNameBuffer, O_CREAT | O_RDWR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}

void app_logger__log( app_logger_t * logger, app_logger_categories_t * cat, int logLevel, char* mLogMessage, ...);

app_logger_t * app_logger__create( unsigned char * path, unsigned char * prefix, unsigned char * ext, uint32_t maxsize, uint32_t logLevel)
{
	app_logger_t * logger = (app_logger_t *)malloc(sizeof(app_logger_t));
	memset( logger, 0, sizeof(app_logger_t));
	strcpy( logger->path, path);
	strcpy( logger->prefix, prefix);
	strcpy( logger->extension, ext);
	pthread_mutex_init( &logger->lock, NULL);
	logger->maxsize = maxsize;
	
	app_logger__create_folder( logger);
	app_logger__create_file( logger);
	
	if( strlen(logger->extension) == 4)
	{
		if( memcmp( ".log", logger->extension, 4) == 0)
		{
			app_logger__log( logger, NULL, 1, "LOGGER Started");
		}
	}
	
	return logger;
}

void app_logger__set_loglevel( app_logger_t * logger, int loglevel)
{
	logger->logLevel = loglevel;
}

app_logger_categories_t * app_logger__set_category( app_logger_t * logger, uint32_t id, uint32_t enabled, unsigned char * tag, int taglen)
{
	int i = 0;
	for( i = 0; i < APP_LOG_MAX_CATEGORIES; i++)
	{
		if( logger->categories[i].id == id)
		{
			logger->categories[i].enabled = enabled;
			
			if( taglen > 8) {
				logger->categories[i].TagLen = 8;
				memcpy( logger->categories[i].Tag, tag, 8);
			} else {
				logger->categories[i].TagLen = taglen;
				memcpy( logger->categories[i].Tag, tag, taglen);
			}
			
			return &logger->categories[i];
		}
	}
	
	for( i = 0; i < APP_LOG_MAX_CATEGORIES; i++)
	{
		if( logger->categories[i].id == 0)
		{
			logger->categories[i].id = id;
			logger->categories[i].enabled = enabled;

			if( taglen > 8) {
				logger->categories[i].TagLen = 8;
				memcpy( logger->categories[i].Tag, tag, 8);
			} else {
				logger->categories[i].TagLen = taglen;
				memcpy( logger->categories[i].Tag, tag, taglen);
			}

			return &logger->categories[i];
		}
	}

	return NULL;
}

void app_logger__setplainlog( app_logger_t * logger)
{
	logger->excludeTime = 1;
	logger->excludeLogLevel = 1;
	logger->excludeThreadId = 1;
}

void app_logger__log( app_logger_t * logger, app_logger_categories_t * cat, int logLevel, char* mLogMessage, ...)
{
	if(!logger)
		return;
	
	if( cat) {
		if(cat->enabled == 0) {
			return;
		}
	}
	
	if( ( logLevel < APP_LOG__LEVEL_LOWER_LIMIT) || ( logLevel > APP_LOG__LEVEL_UPPER_LIMIT))
    {
		return;
    }	
	
    if( !mLogMessage || (APP_LOG__MESSAGE_MAX_SIZE < (int)strlen(mLogMessage)) )	
		return;
	
	pthread_mutex_lock( &logger->lock);
 
    char m_buffer[APP_LOG__BUFFER_SIZE];

    va_list args;// = 0;
    va_start( args, mLogMessage);
    vsnprintf( m_buffer, APP_LOG__MESSAGE_MAX_SIZE, mLogMessage, args);
	va_end( args);

	int bRotate = 0;
	struct stat fileStat;
	fstat( logger->iHandle, &fileStat);
	unsigned long m_FileSize = fileStat.st_size;
	
	if(m_FileSize >= logger->maxsize)
	{
		close( logger->iHandle);
		app_logger__create_file( logger);
	}

	if(logger->excludeTime == 1 && logger->excludeLogLevel == 1 && logger->excludeThreadId == 1)
	{
		write( logger->iHandle, m_buffer, strlen( m_buffer));
		write( logger->iHandle, "\n", 1);
	}
	else
	{
		char dateString[27];
		
		memset( &dateString, '\0', sizeof( dateString));
		app_makeTimeStamp( &dateString[0]);
		unsigned long  ulThreadID = pthread_self();
		 
		int debugTagLen;
		char * debugTag;
	
		switch( logLevel)
		{
			case APP_LOG__LEVEL_CRITICAL:
				debugTag = "[C]";
				debugTagLen = strlen("[C]");
			break;
			case APP_LOG__LEVEL_ERROR:
				debugTag = "[E]";
				debugTagLen = strlen("[E]");
			break;
			case APP_LOG__LEVEL_WARNING:
				debugTag = "[W]";
				debugTagLen = strlen("[W]");
			break;
			case APP_LOG__LEVEL_INFO:
				debugTag = "[I]";
				debugTagLen = strlen("[I]");
			break;
			case APP_LOG__LEVEL_DEBUG:
				debugTag = "[D]";
				debugTagLen = strlen("[D]");
			break;
			default:
				debugTag = "[E]";
				debugTagLen = strlen("[E]");
			break;
		}
		write( logger->iHandle, debugTag, debugTagLen);
		//write( logger->iHandle, "|", 1);
		
		write( logger->iHandle, dateString, strlen(dateString));
		write( logger->iHandle, "|", 1);
		
		if( cat) {
			write( logger->iHandle, cat->Tag, cat->TagLen);
			write( logger->iHandle, "|", 1);
		}
		
		write( logger->iHandle, m_buffer, strlen( m_buffer));
		write( logger->iHandle, "\n", 1);
	}
	
	pthread_mutex_unlock( &logger->lock);	
}


#pragma pack(4)
typedef struct app_data_template
{
	app_data_pool_t * parent;
	app_data_t * item;
	uint8_t * data;
	uint32_t is_allocated;
	uint32_t requested_size;
} app_data_template_t;


#pragma pack(4)
typedef struct app_data
{
	uint8_t * data;
	struct app_data * Next;
	struct app_data * Prev;
	int IsReleased;
	int Spare;
} app_data_t;


#pragma pack(4)
typedef struct app_data_pool
{
	struct app_data_pool * Next;

	unsigned char Name[100];
	int Size;
	int InitCount;
	int IncrCount;	
	
	app_data_t * Head;
	app_data_t * Curr;

	uint64_t TotalCount;
	uint64_t Available;
	uint64_t Served;
	pthread_mutex_t Lock;
	uint64_t CoreAtAvailable;
	uint64_t CoreAtServed;
	
	app_data_t ** items;
} app_data_pool_t;


#pragma pack(4)
typedef struct app_data_region
{
	int Count;
	app_data_pool_t * Head;
	app_data_pool_t * Curr;
	
	pthread_mutex_t Lock;
} app_data_region_t;


app_data_region_t * app_region__create()
{
	app_data_region_t * region = (app_data_region_t *)malloc( sizeof(app_data_region_t));
	memset( region, 0, sizeof(app_data_region_t));
	region->Count = 0;
	region->Head = region->Curr = NULL;
	pthread_mutex_init( &region->Lock, NULL);
	return region;
}


int app_region__create_index( app_data_pool_t * datap)
{
	datap->items = (app_data_t**)malloc( sizeof(app_data_t*) * datap->TotalCount);
	
	if( datap->items)
	{
		app_data_t * item = datap->Head;
		int i = 0;
		
		while( item && i < datap->TotalCount)
		{
			datap->items[i] = item;
			i++;
			
			item = item->Next;
		}
		return 1;
	}
	return 0;
}


int app_region__is_item_allocated_at_index( app_data_pool_t * datap, int index)
{
	if( index < datap->TotalCount)
	{
		app_data_t * dataItem 			= (app_data_t *)datap->items[index];
		
		if(dataItem)
		{
			app_data_template_t * template 	= (app_data_template_t *)dataItem->data;
			
			if( template)
			{
				return template->is_allocated;
			}
		}	
	}
	return -1;
}


uint8_t * app_region__get_pointer_at_index( app_data_pool_t * datap, int index)
{
	if( index < datap->TotalCount)
	{
		app_data_t * dataItem 			= (app_data_t *)datap->items[index];
		
		if( dataItem)
		{
			app_data_template_t * template = (app_data_template_t *)dataItem->data;
			
			if( template)
			{
				return template->data;
			}
		}
	}
	return NULL;
}


app_data_pool_t * app_region__add_pool( app_data_region_t * region, unsigned char * Name, int Size, int Count)
{
	//printf("Name=%s Size=%d\n", Name, Size);

	app_data_pool_t * dpool = (app_data_pool_t *)malloc(sizeof(app_data_pool_t));
	memset( dpool, 0, sizeof(app_data_pool_t));
	
	strcpy( dpool->Name, Name);
	dpool->Size = Size;
	dpool->Head = dpool->Curr = NULL;
	dpool->TotalCount = dpool->Available = Count;
	dpool->InitCount = dpool->IncrCount = Count;
	dpool->CoreAtAvailable = 0;
	dpool->CoreAtServed = 0;
	pthread_mutex_init( &dpool->Lock, NULL);



	uint8_t * pObj = NULL;
	uint8_t * mObj = NULL;

	app_data_t * item = NULL;
	app_data_t * itemPrev = NULL;
	int i = 0;
	
	
	int ReqCount = Count;
	int ChunkCount = 100000;
	int iTreations = 1;
	int ii = 1;
	
	
	if( Count > 0)
	{
		//printf(  "1 Name=%s  Count=%d  ReqCount=%d   [%d Of iTreations=%d]  %s|%d\n", Name, Count, ReqCount, ii, iTreations, __FILE__, __LINE__);
		
		if( ReqCount > ChunkCount)
		{
			//printf( "%d %d   %2.f  %2.f \n", Count, ChunkCount, ( (double)Count / (double)ChunkCount), ( (double)Count / (double)ChunkCount));
			
			iTreations = (Count / ChunkCount);
			double temp  = ((double)Count / (double)ChunkCount);
			
			//printf("temp=%2.f\n", temp);
			
			iTreations = (int)temp;
			
			//printf("temp=%2.f  temp=%f  iTreations=%d\n", temp, (temp), iTreations);
			
			if( temp > iTreations)
			{
				iTreations++;
			}
			
			ReqCount = ChunkCount;
		}
		
		//printf(  "2 Name=%s  Count=%d  ReqCount=%d   [%d Of iTreations=%d]  %s|%d\n", Name, Count, ReqCount, ii, iTreations, __FILE__, __LINE__);

		int Consumed = 0;
		for( ii = 0; ii < iTreations; ii++)
		{
			//printf("%s iTreations=%d %d %d\n", Name, ii, ii+1, iTreations);
			if( ii > 0 && (iTreations) == (ii+1))
			{
				//ReqCount = Count % ChunkCount;
				ReqCount = Count - Consumed;
				printf( "ReqCount=%d  Consumed=%d\n", ReqCount, Consumed);
			} 
			//printf(  "3  Name=%s  Count=%d  ReqCount=%d   [%d Of iTreations=%d]  %s|%d\n", Name, Count, ReqCount, ii, iTreations, __FILE__, __LINE__);
			
			
			pObj = (uint8_t *) malloc( sizeof(app_data_t) * ReqCount);
			mObj = (uint8_t *) malloc( (sizeof(app_data_template_t) + Size) * ReqCount);
			
			if(!pObj || !mObj)
			{
				printf("memory allocation failed for - %s with Count=%d\n", Name, ReqCount);
				exit(0);
			}
			
			Consumed += ReqCount;
			i = 0;
			for( i = 0; i < ReqCount; i++)
			{
				item = (app_data_t *)pObj;
				item->data = mObj;
				item->IsReleased = 1;
				item->Next = NULL;
				item->Prev = itemPrev;

				if(!dpool->Head) 
				{
					dpool->Head = dpool->Curr = item;
				} 
				else 
				{
					dpool->Curr->Next = item;
					dpool->Curr = item;
				}
				
				((app_data_template_t*)item->data)->is_allocated = 0;
				
				itemPrev = item;
				pObj += sizeof(app_data_t);
				mObj += ( sizeof(app_data_template_t) + Size);
			}
		}
	}

	
	pthread_mutex_lock( &region->Lock);
	
	if(!region->Head)
	{
		region->Head = region->Curr = dpool;
	} 
	else 
	{
		region->Curr->Next = dpool;
		region->Curr = dpool;
	}
	
	pthread_mutex_unlock( &region->Lock);
	
	//printf("---\n");
	
	return dpool;
}



void app_region__info( uint8_t * data)
{
	if(!data)
		return;
	
	app_data_template_t * template = (app_data_template_t *)(data-(sizeof(app_data_template_t)));
	
	if( template) {
		app_data_pool_t * parent = template->parent;

		if( parent) {
			printf("Name=%s Size=%d TotalCount=%ld Available=%ld \n", parent->Name, parent->Size, parent->TotalCount, parent->Available);
		} else {
			printf("Invalid p-packet=%p\n", data);
		}
	} else {
		printf("Invalid packet=%p\n", data);
	}
}

int app_region__datalen( uint8_t * data)
{
	if(!data)
		return 0;
	
	app_data_template_t * template = (app_data_template_t *)(data-(sizeof(app_data_template_t)));
	
	if( template) {
		// app_data_pool_t * parent = template->parent;
		
		// if( parent) {
			// return parent->Size;
		// }
		
		return template->requested_size;
	}
	
	return 0;
}


uint8_t * app_region__get_head( app_data_pool_t * dpool)
{
	return (uint8_t*)dpool->Head;
}


uint8_t * app_region__get_next( app_data_pool_t * dpool, uint8_t * item)
{
	app_data_t * itemNext = NULL;
	
	pthread_mutex_lock( &dpool->Lock);
	itemNext = ((app_data_t*)item)->Next;
	pthread_mutex_unlock( &dpool->Lock);
	
	return (uint8_t*)itemNext;
}


int app_region__item_allocated( uint8_t * item)
{
	app_data_t * data_item = (app_data_t *)item;
	
	if( data_item)
	{
		app_data_template_t * template = (app_data_template_t *)data_item->data;
		
		if( template)
		{
			return template->is_allocated;
		}
	}
	return 0;
}


uint8_t * app_region__get_pointer( uint8_t * item)
{
	app_data_t * data_item = (app_data_t *)item;
	
	if( data_item)
	{
		app_data_template_t * template = (app_data_template_t *)data_item->data;
		
		if( template)
		{
			return template->data;
		}
	}
	return NULL;	
}


int app_region__mark_as_allocate( uint8_t * data)
{
	//dont use not working, use Mark property
	
	if(!data)
		return -1;
	
	app_data_template_t * template = (app_data_template_t *)(data-(sizeof(app_data_template_t)));

	if( template)
	{
		app_data_pool_t * parent = template->parent;

		
		if( parent)
		{
			pthread_mutex_lock( &parent->Lock);

			if(!template->item->Next && !template->item->Prev) 
			{
				if(parent->Head == template->item)
				{
					template->is_allocated = 1;
					parent->Served++;
					parent->Available--;
					parent->Head = NULL;
					parent->Curr = NULL;
					pthread_mutex_unlock( &parent->Lock);
					//printf("its a head\n");
					return 1;
				}
				return -3;	//already detached
			}

			
			app_data_t * item = template->item;	
			
			app_data_t * prev = item->Prev;
			app_data_t * next = item->Next;
			
			if(parent->Head == item) 
			{
				parent->Head = item->Next;
				
				if( parent->Head) 
				{
					parent->Head->Prev = NULL;
				}
			} 
			else 
			{
				if( prev) 
				{
					prev->Next = item->Next;
				}
				
				if( next) 
				{
					next->Prev = prev;
				}
			}

			template->is_allocated = 1;
			item->Next = NULL;
			item->Prev = NULL;
		
			parent->Served++;
			parent->Available--;
		
			pthread_mutex_unlock( &parent->Lock);
			
			return 1;
		}
		else 
		{
			return -4;
		}
	}		
	else
	{
		return -2;
	}
}

 

void app_region__free( uint8_t * data)
{
	if(!data)
		return;
	
	app_data_template_t * template = (app_data_template_t *)(data-(sizeof(app_data_template_t)));
	//printf( "item=%p\n", template);
	
	if( template)
	{
		//printf( "is_allocated=%u\n", template->is_allocated);

		if( template->is_allocated == 1)
		{
			template->is_allocated = 0;

			app_data_pool_t * parent = template->parent;
			
			//printf( "parent=%p\n", parent);
			
			if( parent)
			{
				pthread_mutex_lock( &parent->Lock);
				
				if(!parent->Head)
				{
					parent->Head = parent->Curr = template->item;
				}
				else
				{
					template->item->Prev = parent->Curr;
					
					parent->Curr->Next = template->item;
					parent->Curr = template->item;
				}
				
				parent->Available++;
				pthread_mutex_unlock( &parent->Lock);
			}
		}
	}
}

uint64_t app_region__available_fd( app_data_pool_t * datap)
{
	return datap->Available;
}

uint8_t * app_region__allocate_fd( app_data_pool_t * datap)
{
	pthread_mutex_lock( &datap->Lock);
	
	app_data_t * item = datap->Head;
	
	if( item)
	{
		if( datap->CoreAtAvailable > 0 && datap->CoreAtAvailable == datap->Available)
		{
			int j = 100 - 100;
			int x = 100/j;
		}
		

		
		
		datap->Head = item->Next;
		
		if( datap->Head)
		{
			datap->Head->Prev = NULL;
		}

		item->Prev = NULL;
		item->Next = NULL;
		
		datap->Served++;
		datap->Available--;

		if( datap->CoreAtServed > 0 && datap->CoreAtServed == datap->Served)
		{
			int j = 100 - 100;
			int x = 100/j;
		}
	}
	
	pthread_mutex_unlock( &datap->Lock);
	//printf( "item=%p\n", item);
	
	
	// if( datap->Size == 25)
	// {
		// int j = 0;
		// int z = 100/j;		
	// }
	
	if( item)
	{
		app_data_template_t * template = (app_data_template_t *) item->data;
		template->parent = datap;
		template->item = item;
		template->is_allocated = 1;
		template->requested_size = datap->Size;
		template->data = (uint8_t *)((item->data) + sizeof(app_data_template_t));
		//printf( "app_data_template_t-sz=%ld item=%ld tempd=%ld\n", sizeof(app_data_template_t), (long)item->data, (long)template->data);
		return (uint8_t *)template->data;
	}
	else
	{
		printf("data allocation failed for Name=%s size=%d  Available=%lu  TotalCount=%lu  Pending=%lu  Served=%ld %s|%d\n", 
			datap->Name, datap->Size, datap->Available, datap->TotalCount, (datap->TotalCount - datap->Available), datap->Served, __FILE__, __LINE__);
		

		// int j = 0;
		// int z = 100/j;
		
		return NULL;

		//exit(0);
	}
}


uint8_t * app_region__allocate_fd2( app_data_pool_t * datap, int ReqestedSize)
{
	pthread_mutex_lock( &datap->Lock);
	
	app_data_t * item = datap->Head;
	
	if( item)
	{
		if( datap->CoreAtAvailable > 0 && datap->CoreAtAvailable == datap->Available)
		{
			int j = 100 - 100;
			int x = 100/j;
		}
		

		
		
		datap->Head = item->Next;
		
		if( datap->Head)
		{
			datap->Head->Prev = NULL;
		}

		item->Prev = NULL;
		item->Next = NULL;
		
		datap->Served++;
		datap->Available--;

		if( datap->CoreAtServed > 0 && datap->CoreAtServed == datap->Served)
		{
			int j = 100 - 100;
			int x = 100/j;
		}
	}
	
	pthread_mutex_unlock( &datap->Lock);
	//printf( "item=%p\n", item);


	// if( datap->Available < 7 && datap->Size == 25)
	// {
		// uint64_t TotalCount = datap->TotalCount;
		// uint64_t Available = datap->Available;
		// int av = (datap->TotalCount - datap->Available); 
		
		// int j = 0;
		// int z = 100/j;		
	// }
	
	if( item)
	{
		app_data_template_t * template = (app_data_template_t *) item->data;
		template->parent = datap;
		template->item = item;
		template->is_allocated = 1;
		template->requested_size = ReqestedSize;
		template->data = (uint8_t *)((item->data) + sizeof(app_data_template_t));
		//printf( "app_data_template_t-sz=%ld item=%ld tempd=%ld\n", sizeof(app_data_template_t), (long)item->data, (long)template->data);
		return (uint8_t *)template->data;
	}
	else
	{
		printf("data allocation failed for size=%d  Available=%lu  TotalCount=%lu  Pending=%lu  Served=%ld %s|%d\n", 
			datap->Size, datap->Available, datap->TotalCount, (datap->TotalCount - datap->Available), datap->Served, __FILE__, __LINE__);
		

		// int j = 0;
		// int z = 100/j;
		
		return NULL;

		//exit(0);
	}
}


app_data_pool_t * app_region__find_pool( app_data_region_t * region, int Size)
{
	app_data_pool_t * datap = region->Head;
	uint8_t * ptr = NULL;
	
	while( datap)
	{
		if( datap->Size == Size)
		{
			return datap;
		}
		datap = datap->Next;
	}
	
	return NULL;
}


uint8_t * app_region__allocate_fr( app_data_region_t * region, int Size)
{
	app_data_pool_t * datap = region->Head;
	uint8_t * ptr = NULL;
	
	while( datap)
	{
		if( datap->Size == Size)
		{
			ptr = app_region__allocate_fd2( datap, Size);
			break;
		}
		datap = datap->Next;
	}
	
	
	if(ptr) {
		return ptr;
	}
	
	datap = region->Head;

	while( datap)
	{
		if( datap->Size >= Size)
		{
			return app_region__allocate_fd2( datap, Size);
		}
		datap = datap->Next;
	}
	return NULL;
}

void app_region__set_core( app_data_pool_t * datap, uint64_t Available)
{
	datap->CoreAtAvailable = Available;
}

void app_region__set_core_at_served( app_data_pool_t * datap, uint64_t Served)
{
	datap->CoreAtServed = Served;
}


char * app_region__get_name( app_data_pool_t * datap)
{
	return datap->Name;
}




void app_region__get_counts( app_data_pool_t * datap, uint64_t * TotalCount, uint64_t * Available)
{
	*TotalCount	= datap->TotalCount;
	*Available	= datap->Available;
}


void app_region__set_served( app_data_pool_t * datap, uint32_t Served)
{
	datap->Served = Served;
}


void app_region__print_stats( app_logger_t * plogger, app_data_region_t * region)
{
	app_data_pool_t * datap = region->Head;
	
	app_logger_t * lgr = (plogger) ? plogger : logger;
	
	while( datap)
	{
		//app_region__allocate_fd( datap);
		app_logger__log( lgr, NULL, APP_LOG__LEVEL_CRITICAL, "%-40s  Size %-4u  TotalCount %-10ld  Available %-10ld  Pending %-10ld  Served %-10ld", 
			datap->Name, datap->Size, datap->TotalCount, datap->Available, datap->TotalCount - datap->Available, datap->Served);
		
		datap = datap->Next;
	}
}



typedef struct app_QueueRecord
{
	void * Data;
	struct app_QueueRecord *Next;
	struct app_QueueRecord *PoolNext;
	int isReleased;
	int PoolIndex;	
	fp_queue_call_back cb;
} app_iQueueRecord;

typedef struct app_Queue
{
	app_iQueueRecord * Front;
	app_iQueueRecord * Rear;	
	app_iQueueRecord * Temp;	
	app_iQueueRecord * Front1;
	long LastDequeued;
	long LastQueued;
	long LastDropped;
	long TotalDequeued;
	long TotalQueued;
	long TotalDropped;
	long QueueCount;
	long QueueLimit;		
	int Type;
	pthread_mutex_t Lock;	
	sem_t sem_lock;
	
	int tCount;
	fp_queue_call_back cb;
	app_data_region_t * dregion;
	app_data_pool_t * dpool;
	app_fsmQueue_t * fsmq;
	int fsmIndex;
	char Name[100];
} app_iQueue;

typedef struct app_QueueH {
	app_iQueue * queue;
	int index;
} app_iQueueH;

long app_queue__get_queuecount( app_iQueue * queue)
{
	return queue->QueueCount;
}

void * app_queue__thread( void * args)
{
	void * Data = NULL;
	app_iQueueH * queueH = (app_iQueueH *) args;
	fp_queue_call_back cb;
	int index = 0;
	
	while(1)
	{
		Data = app_queue__dequee( queueH->queue, &cb);
		index = queueH->index;
		
		if( queueH->queue->fsmq)
		{
			index = queueH->queue->fsmIndex;
		}
		
		if(cb)
		{
			cb( Data, index);
		}
		else if( queueH->queue->cb)
		{
			queueH->queue->cb( Data, index);
		}
		
		Data = NULL;
	}
	
	return NULL;
}

app_iQueue * app_queue__create( char * name, int capacity, int wThreads, fp_queue_call_back cb)
{
	if( wThreads == 0 || capacity == 0)
		return NULL;
	
	app_iQueue * queue = (app_iQueue*)malloc( sizeof(app_iQueue));
	memset( queue, 0, sizeof( app_iQueue));
	
	queue->dregion = app_region__create();
	queue->dpool = app_region__add_pool( queue->dregion, name, sizeof(app_iQueueRecord), capacity);
	
	queue->Front = NULL;
	queue->Rear = NULL;
	queue->Temp = NULL;
	queue->Front1 = NULL;
	queue->TotalDequeued = 0;
	queue->TotalQueued = 0;
	queue->QueueCount = 0;
	queue->QueueLimit = 0;
	queue->Type;
	queue->tCount = wThreads;
	queue->LastDropped = 0;
	queue->TotalDropped = 0;
	strcpy( queue->Name, name);
	
	queue->cb = cb;
	pthread_mutex_init( &queue->Lock, NULL);
	sem_init( &queue->sem_lock, 0, 0);
	
	int i = 0;
	int iRet;
	
	pthread_t s_pthread_id;
	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	//iRet = pthread_create( &s_pthread_id, &attr, app_queue__thread, (void *)qH);
	
	app_iQueueH * qH = NULL;
	
	for( i = 0; i < wThreads; i++)
	{
		qH = (app_iQueueH *)malloc( sizeof(app_iQueueH));
		memset( qH, 0, sizeof(app_iQueueH));
		qH->queue = queue;
		qH->index = i;
		
		iRet = pthread_create( &s_pthread_id, &attr, app_queue__thread, (void *)qH);

		if(iRet)
		{
			perror("Error: ");
			printf("unable to create thread Queue \n");
			exit(-1);
		}		
	}
	
	return queue;
}


uint8_t * app_queue__dequee( app_iQueue * queue, fp_queue_call_back * cb)
{
	void * Data = NULL;
	sem_wait( &queue->sem_lock);
	
	pthread_mutex_lock( &queue->Lock);
	
	
	queue->Front1 = queue->Front;
	
	if(queue->Front1)
	{
		if( queue->Front1->Next)
		{
			queue->Front1 = queue->Front1->Next;
			Data = queue->Front->Data;
			*cb = queue->Front->cb;
			app_region__free( (uint8_t*)queue->Front);
			queue->Front = queue->Front1;
		}
		else
		{
			Data = queue->Front->Data;
			*cb = queue->Front->cb;
			app_region__free( (uint8_t*)queue->Front);
			queue->Front = queue->Rear = NULL;
		}
		
		if( Data)
		{
			queue->TotalDequeued++;
			queue->QueueCount--;
		}
	}

	pthread_mutex_unlock( &queue->Lock);	
	return Data;
}


int app_queue__enquee( app_iQueue * queue, uint8_t * data, fp_queue_call_back cb)
{
	pthread_mutex_lock( &queue->Lock);
	
	if(!queue->Rear)	
	{
		queue->Rear = (app_iQueueRecord*)app_region__allocate_fd( queue->dpool);
		
		if(!queue->Rear)
		{
			pthread_mutex_unlock( &queue->Lock);
			return -10;
		}
		
		queue->Rear->Data = data;
		queue->Rear->cb = cb;
		queue->Rear->Next = NULL;
		queue->Front = queue->Rear;
	}
	else
	{
		queue->Temp = (app_iQueueRecord*)app_region__allocate_fd( queue->dpool);
		
		if(!queue->Temp)
		{
			pthread_mutex_unlock( &queue->Lock);
			return -20;
		}
		
		queue->Temp->Data = data;
		queue->Temp->cb = cb;
		queue->Temp->Next = NULL;
		
		queue->Rear->Next = queue->Temp;
		queue->Rear = queue->Temp;
	}	
	
	queue->TotalQueued++;
	queue->QueueCount++;
	
	pthread_mutex_unlock( &queue->Lock);
	sem_post( &queue->sem_lock);
	return 1;
}


int app_queue__is_limit_reached( app_iQueue * queue)
{
	if( queue->QueueLimit > 0)
	{
		if( queue->QueueLimit > queue->QueueCount)
		{
			//printf( "QL=%ld QC=%ld  %d|%s\n", queue->QueueLimit, queue->QueueCount, __LINE__, __FILE__);
			return 0;
		}
		else
		{
			return 1;
		}
	}
	return 0;
}


void app_queue__set_limit( app_iQueue * queue, int limit)
{
	queue->QueueLimit = limit;
}


void app_queue__dropped( app_iQueue * queue)
{
	//pthread_mutex_lock( &queue->Lock);
	queue->TotalDropped++;
	//pthread_mutex_unlock( &queue->Lock);
}



void app_queue__logstats( app_logger_t * plogger, app_iQueue * queue)
{
	app_logger_t * lgr = (plogger) ? plogger : logger;

	long cQueued 	= queue->TotalQueued - queue->LastQueued;
	long cDequeued 	= queue->TotalDequeued - queue->LastDequeued;
	long cDropped 	= queue->TotalDropped - queue->LastDropped;
	
	app_logger__log( lgr, NULL, APP_LOG__LEVEL_CRITICAL, "Q=%s WT %-3d [Enq %-8ld Tot %-8ld] [Deq %-8ld Tot %-8ld] InQ %-10ld PL[TotCnt %-10ld  Avai %-10ld] Drpd[Curr %-8ld Tot %-8ld]", 
				queue->Name, queue->tCount, cQueued, queue->TotalQueued, cDequeued, 
				queue->TotalDequeued, queue->QueueCount, queue->dpool->TotalCount, queue->dpool->Available, cDropped, queue->TotalDropped);
				
	queue->LastQueued = queue->TotalQueued;
	queue->LastDequeued = queue->TotalDequeued;
}


void app_queue__collectlogstats( app_iQueue * queue, uint32_t * threadCOunt, long * currQueued, long * currDequeued, long * totaQueued, long * totaDequeued, long * queDequeued, long * dpTotal, long * dpAvail)
{
	long cQueued 	= queue->TotalQueued - queue->LastQueued;
	long cDequeued 	= queue->TotalDequeued - queue->LastDequeued;

	*threadCOunt 	= queue->tCount;
	*currQueued		= cQueued;
	*currDequeued	= cDequeued;
	*totaQueued		= queue->TotalQueued;
	*totaDequeued	= queue->TotalDequeued;
	*queDequeued	= queue->QueueCount;
	*dpTotal		= queue->dpool->TotalCount;
	*dpAvail		= queue->dpool->Available;

	queue->LastQueued = queue->TotalQueued;
	queue->LastDequeued = queue->TotalDequeued;	
}

#pragma pack(4)
typedef struct app_fsmQueue
{
	app_iQueue ** queue;
	int count;
	int capacity;
	char name[100];
} app_fsmQueue_t;


char * app_fsmqueue__name( app_fsmQueue_t * fsmq)
{
	return fsmq->name;
}


int app_fsmqueue__thread_count( app_fsmQueue_t * fsmq)
{
	return fsmq->count;
}


int app_fsmqueue__capacity_per_thread( app_fsmQueue_t * fsmq)
{
	return fsmq->capacity;
}


app_fsmQueue_t * app_fsmqueue__create( char * name, int capacity, int queues, fp_queue_call_back cb)
{
	if( queues == 0 || capacity == 0)
		return NULL;
	
	app_fsmQueue_t * fsmqueue = (app_fsmQueue_t*)malloc( sizeof(app_fsmQueue_t));
	memset( fsmqueue, 0, sizeof( app_fsmQueue_t));
	strcpy( fsmqueue->name, name);
	fsmqueue->capacity 	= capacity;
	fsmqueue->count 	= queues;
	fsmqueue->queue 	= (app_iQueue **) malloc( sizeof(app_iQueue*) * queues);
	
	char qname[80];
	
	int i = 0;
	for( i = 0; i < queues; i++)
	{
		memset( qname, 0, sizeof(qname));
		sprintf( qname, "%s-%d", name, i);
		fsmqueue->queue[i] = app_queue__create( qname, capacity, 1, cb);
		fsmqueue->queue[i]->fsmq = fsmqueue;
		fsmqueue->queue[i]->fsmIndex = i;
	}
	
	return fsmqueue;
}

int app_fsmqueue__enquee( app_fsmQueue_t * fq, uint8_t * data, int index, fp_queue_call_back cb)
{
	if( index < fq->count ) {
		return app_queue__enquee( fq->queue[index], data, cb);
	}
	
	printf("invalid index=%d\n", index);
	return -50;
}

int app_fsmqueue__idenquee( app_fsmQueue_t * fq, uint8_t * data, long id, fp_queue_call_back cb)
{
	return app_fsmqueue__enquee( fq, data, id % fq->count, cb);
}


void app_fsmqueue__logstats( app_logger_t * plogger, app_fsmQueue_t * fq)
{
	uint32_t t_threadCOunt = 0;
	long t_currQueued = 0;
	long t_currDequeued = 0;
	long t_totaQueued = 0;
	long t_totaDequeued = 0;
	long t_queDequeued = 0;
	long t_dpTotal = 0;
	long t_dpAvail = 0;

	uint32_t threadCOunt = 0;
	long currQueued = 0;
	long currDequeued = 0;
	long totaQueued = 0;
	long totaDequeued = 0;
	long queDequeued = 0;
	long dpTotal = 0;
	long dpAvail = 0;
	
	int i = 0;
	for( i = 0; i < fq->count; i++)
	{
		threadCOunt = 0;
		currQueued = 0;
		currDequeued = 0;
		totaQueued = 0;
		totaDequeued = 0;
		queDequeued = 0;
		dpTotal = 0;
		dpAvail = 0;
	
		app_queue__collectlogstats( fq->queue[i], &threadCOunt, &currQueued, &currDequeued, &totaQueued, &totaDequeued, &queDequeued, &dpTotal, &dpAvail);

		t_threadCOunt += threadCOunt;
		t_currQueued += currQueued;
		t_currDequeued += currDequeued;
		t_totaQueued += totaQueued;
		t_totaDequeued += totaDequeued;
		t_queDequeued += queDequeued;
		t_dpTotal += dpTotal;
		t_dpAvail += dpAvail;
		
		if( dpAvail == 0)
		{
			app_logger__log( plogger, NULL, APP_LOG__LEVEL_CRITICAL, "%s-Threads Index=%d Available Pool is 0", fq->name, i);
		}
	}

	app_logger__log( plogger, NULL, APP_LOG__LEVEL_CRITICAL, "%s-Threads %-3d [Queued %-8ld Total %-8ld]  [Dequeued %-8ld Total %-8ld] InQ %-10ld Pool[TotalCount %-10ld   Available %-10ld]", 
				fq->name, t_threadCOunt, t_currQueued, t_totaQueued, t_currDequeued, 
				t_totaDequeued, t_queDequeued, t_dpTotal, t_dpAvail);

}


long app_fsmqueue__get_queuecount( app_fsmQueue_t * fq)
{
	long qCount = 0;

	int i = 0;
	for( i = 0; i < fq->count; i++)
	{
		qCount += fq->queue[i]->QueueCount;
	}
	
	return qCount;
}



#define			APP_MAX_TIMER_TICKS			21600			//6 HRS

typedef struct app_timer
{
	struct app_timer * Next;
	uint32_t isReleased;
	uint32_t isCleared;	
	
	uint8_t * data;
	void ( * handler) ( uint8_t * Data, int tIndex);	
} app_timer_t;

typedef struct app_timer_tick
{
	app_timer_t * Head;
	app_timer_t * Current;
	pthread_mutex_t Lock;
	
} app_timer_tick_t;

typedef struct app_timer_stack
{
	app_timer_tick_t Timers[APP_MAX_TIMER_TICKS];
	int TimerTickIndex;
	app_iQueue * queue;
	app_data_region_t * dregion;
	app_data_pool_t * dpool;
} app_timer_stack_t;

app_timer_stack_t * timer_stack = NULL;


void * app_timer__thread( void * args)
{
	while(1)
	{
		timer_stack->TimerTickIndex++;
		
		if( timer_stack->TimerTickIndex >= APP_MAX_TIMER_TICKS )
		{
			timer_stack->TimerTickIndex = 0;
		}
		
		app_timer_tick_t * _ticker = &timer_stack->Timers[ timer_stack->TimerTickIndex];
		
		pthread_mutex_lock( &_ticker->Lock);
		
		app_timer_t * _timer = _ticker->Head;
		app_timer_t * _prev_timer = NULL;
		
		while( _timer)
		{
			if(_timer->isCleared == 0)
			{	
				app_queue__enquee( timer_stack->queue, _timer->data, _timer->handler);
			}
			
			_prev_timer = _timer;
			_timer = _timer->Next;
			
			app_region__free( (uint8_t *)_prev_timer);
		}
		
		pthread_mutex_unlock( &_ticker->Lock);
		
		usleep( 999900);
	}
}


void app_timer__init( int capacity)
{
	if(!timer_stack)
	{
		timer_stack = (app_timer_stack_t *) malloc(sizeof(app_timer_stack_t));
		memset( timer_stack, 0, sizeof(app_timer_stack_t));
		timer_stack->queue = app_queue__create( "time-out", capacity, 1, NULL);
		
		timer_stack->dregion = app_region__create();
		timer_stack->dpool = app_region__add_pool( timer_stack->dregion, "timer-dp", sizeof(app_timer_t), capacity);

		int i;
		for( i = 0; i < APP_MAX_TIMER_TICKS; i++)
		{
			timer_stack->Timers[i].Head = timer_stack->Timers[i].Current = NULL;
			pthread_mutex_init( &timer_stack->Timers[i].Lock, NULL);
		}
		
		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		int iRet = pthread_create( &s_pthread_id, &attr, app_timer__thread, (void *)timer_stack);

		if(iRet)
		{
			perror("Error: ");
			printf("unable to create thread for timer \n");
			exit(-1);
		}	
		
		//__si_create_pthread2( __si_timer_thread, NULL, "si_timer");
		//__si_create_pthread2( __si_app_events_thread, NULL, "si_appevents");		
	}
}

void app_timer__schedule( app_timer_t * _timer, int seconds)
{
	int schAt = timer_stack->TimerTickIndex + seconds;
	
	if( schAt > APP_MAX_TIMER_TICKS)
	{
		schAt = schAt - APP_MAX_TIMER_TICKS;
	}
	
	app_timer_tick_t * _ticker = &timer_stack->Timers[schAt];
	
	pthread_mutex_lock( &_ticker->Lock);
	
	if(!_ticker->Head)
	{
		_ticker->Head = _ticker->Current = _timer;
	}
	else
	{
		_ticker->Current->Next = _timer;
		_ticker->Current = _timer;
	}
	
	pthread_mutex_unlock( &_ticker->Lock);
}

app_timer_t * app_timer__start( uint8_t * data, fp_timer_handler handler, int seconds)
{
	if( !handler || seconds <= 0)	
		return NULL;
	
	app_timer_t * _timer = ( app_timer_t *)app_region__allocate_fd( timer_stack->dpool);
	
	if(!_timer)
		return NULL;
	
	_timer->data = data;
	_timer->handler = handler;
	_timer->isCleared = 0;
	
	app_timer__schedule( _timer, seconds);
	
	return _timer;
}

void app_timer__clear( app_timer_t * timer)
{
	if( timer) 
	{
		timer->isCleared = 1;
		timer = NULL;
	}
}

uint32_t app_hash__crc32b( uint8_t * str)
{
	unsigned int byte, crc, mask;
    int i = 0, j;
    crc = 0xFFFFFFFF;
    while (str[i] != 0)
	{
        byte = str[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}


uint32_t m32( uint8_t * str)
{
	uint32_t h = 3323198485;
	for (; *str; ++str) 
	{
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint64_t m64( uint8_t * key)
{
	uint64_t h = 525201411107845655;
	for (;*key;++key) 
	{
		h ^= *key;
		h *= 0x5bd1e9955bd1e995;
		h ^= h >> 47;
	}
	return h;
}

#pragma pack(4)
typedef struct ahc
{
	struct ahc * P;
	struct ahc * N;
	uint8_t * k;
	uint8_t kl;
	uint8_t * v;
} ahc_t;

#pragma pack(4)
typedef struct ahr
{
	ahc_t * H;
	ahc_t * C;
	uint32_t Cnt;
	pthread_mutex_t Lk;
} ahr_t;

typedef struct aht
{
	uint8_t T;
	uint8_t KL;
	uint8_t HF;
	uint8_t S;
	uint32_t TRC;
	uint32_t URC;
	pthread_mutex_t Lk;
	app_data_region_t * r;
	ahr_t ** t;
} aht_t;



aht_t * aht_c( uint8_t T, uint8_t KL, uint32_t cpcty, app_data_region_t * r)
{
	//printf( "ahc_t=%ld aht_t=%ld\n", sizeof(ahc_t), sizeof(aht_t));


	aht_t * ht = (aht_t *)malloc( sizeof(aht_t));
	memset( ht, 0, sizeof(aht_t));
	
	pthread_mutex_init( &ht->Lk, NULL);
	ht->KL = KL;
	ht->TRC = cpcty;
	ht->URC = 0;
	ht->t = (ahr_t **) malloc( sizeof(ahr_t*) * cpcty);
	memset( ht->t, 0, sizeof(ahr_t*) * cpcty);
	
	ht->r = r;
	
	uint8_t * rows = malloc( sizeof(ahr_t) * cpcty);
	memset( rows, 0, sizeof(ahr_t) * cpcty);
	
	int i = 0;
	for( i = 0; i < cpcty; i++)
	{
		ht->t[i] = (ahr_t*) rows;
		
		ht->t[i]->H 	= 0;
		ht->t[i]->C 	= 0;
		ht->t[i]->Cnt	= 0;
		pthread_mutex_init( &ht->t[i]->Lk, NULL);
		
		rows += sizeof(ahr_t);
	}

	app_region__add_pool( ht->r, "ahc_t1", 	sizeof(ahc_t), 	cpcty);
	app_region__add_pool( ht->r, "ahc_t2", 	KL,   			cpcty);
	
	return ht;
}

ahc_t * ahc_c( aht_t * t, ahr_t * r, void * d)
{
	int reqSize = sizeof(ahc_t);
	ahc_t * c = (ahc_t *)app_region__allocate_fr( t->r, reqSize);
	memset( c, 0, sizeof(ahc_t));
	
	c->N = 0;
	c->P = 0;
	c->k = 0;
	c->v = d;

	if(!r->H) 
	{
		r->H = r->C = c;
	} 
	else 
	{
		ahc_t * h = r->H;
		ahc_t * ln = NULL;
		
		while(h)
		{
			ln = h;
			h = h->N;
		}
		
		c->P = ln;
		ln->N = c;
	}
	
	// if(!r->H) 
	// {
		// r->H = r->C = c;
	// } 
	// else 
	// {
		// // ahc_t * H = r->H;
		// // ahc_t * C = r->C;
		
		// c->P = r->C;
		// r->C->N = c;
		// r->C = c;
	// }
	
	
	r->Cnt++;

	return c;
}

uint8_t aht__ai32( aht_t * t, int32_t k, void * d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = ahc_c( t, r, d);
	c->k = (uint8_t*)app_region__allocate_fr( t->r, 4);
	*((int32_t*)c->k) = k;
	pthread_mutex_unlock( &r->Lk);
	
	return 0;
}

uint8_t aht__au32( aht_t * t, uint32_t k, void * d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = ahc_c( t, r, d);
	c->k = (uint8_t*)app_region__allocate_fr( t->r, 4);
	*((uint32_t*)c->k) = k;
	pthread_mutex_unlock( &r->Lk);
	
	return 0;
}

uint8_t aht__ai64( aht_t * t, int64_t k, void * d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = ahc_c( t, r, d);
	c->k = (uint8_t*)app_region__allocate_fr( t->r, 8);
	*((int64_t*)c->k) = k;
	pthread_mutex_unlock( &r->Lk);
	
	return 0;
}

uint8_t aht__au64( aht_t * t, uint64_t k, void * d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = ahc_c( t, r, d);
	c->k = (uint8_t*)app_region__allocate_fr( t->r, 8);
	*((uint64_t*)c->k) = k;
	pthread_mutex_unlock( &r->Lk);
	
	//printf("index=%d k=%lu Cnt=%d ky=%lu\n", i, k, r->Cnt, *((uint64_t*)c->k));
	
	return 0;
}


uint8_t aht__as( aht_t * t, char * k, int kl, void * d)
{
	if( kl >= t->KL) return -1;
	
	uint32_t hk = m64( k);
	
	uint32_t i = ( hk % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);

	//printf("%p  adding pointer H=%p  C=%p  %d\n", r, r->H, r->C, __LINE__);
	
	ahc_t * c = ahc_c( t, r, d);

	c->k = (uint8_t*)app_region__allocate_fr( t->r, t->KL);
	memset( c->k, 0, t->KL);
	
	if( kl < t->KL) {
		memcpy( c->k, k, kl);
	} else {
		memcpy( c->k, k, t->KL);
	}

	//printf("%p  added pointer=%p  H=%p  C=%p  %d\n", r, c, r->H, r->C, __LINE__);
	
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

void ahc_dc( ahc_t * c, ahr_t * r)
{
	//printf("%p  deleting pointer=%p  H=%p  C=%p  %d\n", r, c, r->H, r->C, __LINE__);

	// ahc_t * it = r->H;
	// ahc_t * pi = NULL;
	
	// if( r->H == c)
	// {
		// c->P = NULL;
		// r->H = c->N;
		// c->N = NULL;
		
		// if( r->C == c)
		// {
			// r->C = r->H;
		// }
	// }
	// else
	// {
		// while( it)
		// {
			// if( it == c)
			// {
				// if( pi)
				// {
					// pi->N = c->N;

				// }
				
				
			// }
			
			// pi = it;
			// it = it->N;
		// }
	// }
	
	ahc_t * pc = NULL;
	ahc_t * nc = NULL;
	if( c->P) pc = c->P;
	if( c->N) nc = c->N;

	if( r->H == c)
		r->H = c->N;

	if( r->C == c)
		r->C = c->N;
		
	// if(!r->C)
		// r->C = c->P;
		
	c->P = NULL, c->N = NULL; 
	if( pc) pc->N = nc;
	if( nc) nc->P = pc;
	r->Cnt--;
	app_region__free( (uint8_t *)c->k);
	app_region__free( (uint8_t *)c);
	c->k = NULL;
	c = NULL;

}


uint8_t aht__di32( aht_t * t, int32_t k)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((int32_t*)c->k) == k)
		{
			ahc_dc( c, r);
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

uint8_t aht__du32( aht_t * t, uint32_t k)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((uint32_t*)c->k) == k)
		{
			ahc_dc( c, r);
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}


uint8_t aht__di64( aht_t * t, int64_t k)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((int64_t*)c->k) == k)
		{
			ahc_dc( c, r);
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

uint8_t aht__du64( aht_t * t, uint64_t k)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		//if(c->k)
		//	printf("del - index=%d k=%lu Cnt=%d ky=%lu c=%p\n", i, k, r->Cnt, *((uint64_t*)c->k), c);
		
		if( *((uint64_t*)c->k) == k)
		{
			ahc_dc( c, r);
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}


uint8_t aht__ds( aht_t * t, char * k, int kl)
{
	uint32_t hk = m64( k);
	
	uint32_t i = ( hk % t->TRC);
	ahr_t * r = t->t[i];
	
	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( memcmp( c->k, k, kl) == 0)
		{
			ahc_dc( c, r);
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

uint8_t aht_fi32( aht_t * t, int32_t k, void ** d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];

	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((int32_t*)c->k) == k)
		{
			*d = c->v;
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}


uint8_t aht_fu32( aht_t * t, uint32_t k, void ** d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];

	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((uint32_t*)c->k) == k)
		{
			*d = c->v;
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}


uint8_t aht_fi64( aht_t * t, int64_t k, void ** d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];

	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( *((int64_t*)c->k) == k)
		{
			*d = c->v;
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}


uint8_t aht_fu64( aht_t * t, uint64_t k, void ** d)
{
	uint32_t i = ( k % t->TRC);
	ahr_t * r = t->t[i];

	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		//printf("H=%p c=%p Cnt=%d\n", r->H, c, r->Cnt);
		
		if( *((uint64_t*)c->k) == k)
		{
			*d = c->v;
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

uint8_t aht_fs( aht_t * t, char * k, int kl, void ** d)
{
	uint32_t hk = m64( k);
	uint32_t i = ( hk % t->TRC);
	ahr_t * r = t->t[i];

	pthread_mutex_lock( &r->Lk);
	ahc_t * c = r->H;
	
	while( c)
	{
		if( memcmp( c->k, k, kl) == 0)
		{
			*d = c->v;
			pthread_mutex_unlock( &r->Lk);
			return 1;
		}
		c = c->N;
	}
	pthread_mutex_unlock( &r->Lk);
	return 0;
}

void app_memcpy( char * dst, char * src, int len, int maxlen)
{
	if( len < maxlen) 
	{
		memcpy( dst, src, len);
		dst[len] = '\0';
	} 
	else 
	{
		memcpy( dst, src, maxlen);
		dst[maxlen] = '\0';
	}
}

void app_reverse_str( char * dst, char * src, int len)
{
	int j = 0;
	for (j = 0; j < len; j++)
	{
		dst[len - (j+1)] = src[j];
		//printf( "j = %d  %d\n", j, len - (j+1));
	}
	dst[j] = '\0';
}





































