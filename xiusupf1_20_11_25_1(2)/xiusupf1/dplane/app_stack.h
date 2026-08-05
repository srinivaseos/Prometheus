#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef S_APP_STACK_DEF
#define S_APP_STACK_DEF

#define APP_LOG__LEVEL_LOWER_LIMIT   			1
#define APP_LOG__LEVEL_CRITICAL      			1
#define APP_LOG__LEVEL_ERROR         			2
#define APP_LOG__LEVEL_WARNING       			3
#define APP_LOG__LEVEL_INFO          			4
#define APP_LOG__LEVEL_NORMAL        			4
#define APP_LOG__LEVEL_DEBUG         			5
#define APP_LOG__LEVEL_UPPER_LIMIT   			5

#define APP_LOG__MESSAGE_MAX_SIZE				1000
#define APP_LOG__BUFFER_SIZE					1100

void init_rand();
uint32_t get_rand_number( uint32_t min, uint32_t max);

#pragma pack(4)
typedef struct app_exec_time
{
	struct timeval before;
	struct timeval after;
	struct timeval lapsed;
} app_exec_time_t; 

void app_start_time( app_exec_time_t * o_exec_time);
void app_end_time( app_exec_time_t * o_exec_time);
void app_end_full_time( app_exec_time_t * o_exec_time, long * days, long * hours, long * minutes, long * seconds);

typedef struct app_counter app_counter_t;
typedef struct app_rbtree app_rbtree_t;
typedef struct app_rbnode app_rbnode_t;
typedef struct app_logger app_logger_t;
typedef struct app_logger_categories app_logger_categories_t;
typedef struct app_data app_data_t;
typedef struct app_data_pool app_data_pool_t;
typedef struct app_data_region app_data_region_t;
typedef struct app_QueueRecord app_iQueueRecord;
typedef struct app_Queue app_iQueue;
typedef struct app_Queue app_fsmQueue;
typedef struct app_fsmQueue app_fsmQueue_t;
typedef struct app_timer app_timer_t;

typedef void ( * fp_queue_call_back)( uint8_t * Data, int tIndex);
typedef void ( * fp_timer_handler)( uint8_t * Data, int tIndex);


char * app_is_ubuntu();
int app_validate_osv( char * name, char * version);

app_counter_t * app_counter_create( char * name);
void app_counter_increment( app_counter_t *);
void app_counter_increment_r( app_counter_t * counter);
void app_counter_increment_e( app_counter_t * counter);
void app_counter_plog_1( app_counter_t *, app_logger_t *);
void app_counter_plog_2( app_counter_t *, app_logger_t *);


void app_timer__init( int capacity);
app_timer_t * app_timer__start( uint8_t * data, fp_timer_handler handler, int seconds);
void app_timer__clear( app_timer_t * timer);

uint8_t * app_queue__dequee( app_iQueue * queue, fp_queue_call_back * cb);
long app_queue__get_queuecount( app_iQueue * queue);
app_iQueue * app_queue__create( char * name, int capacity, int wThreads, fp_queue_call_back cb);
int app_queue__enquee( app_iQueue * queue, uint8_t * data, fp_queue_call_back cb);
int app_queue__is_limit_reached( app_iQueue * queue);
void app_queue__set_limit( app_iQueue * queue, int limit);
void app_queue__dropped( app_iQueue * queue);
void app_queue__logstats( app_logger_t * plogger, app_iQueue * queue);



app_fsmQueue_t * app_fsmqueue__create( char * name, int capacity, int queues, fp_queue_call_back cb);
char * app_fsmqueue__name( app_fsmQueue_t * fsmq);
int app_fsmqueue__thread_count( app_fsmQueue_t * fsmq);
int app_fsmqueue__capacity_per_thread( app_fsmQueue_t * fsmq);
int app_fsmqueue__enquee( app_fsmQueue_t * fq, uint8_t * data, int index, fp_queue_call_back cb);
int app_fsmqueue__idenquee( app_fsmQueue_t * fq, uint8_t * data, long id, fp_queue_call_back cb);
void app_fsmqueue__logstats( app_logger_t * plogger, app_fsmQueue_t * fq);
long app_fsmqueue__get_queuecount( app_fsmQueue_t * fq);



void app_stack__set_logger( app_logger_t * loggerInstance);

size_t app_rbnode__getsizeof_app_rbtree();
app_rbtree_t * app_rbnode__create_idp( int capacity, int beginNumber);
app_rbtree_t * app_rbnode__create_rbtree( int Count, int keysize);
int app_rbnode__add_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen, uint8_t * data);
uint8_t * app_rbnode__find_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen);
uint8_t * app_rbnode__rfind_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen);
void app_rbnode__setNull_rbitem( app_rbtree_t * tree, uint8_t * key, int keylen);
void app_rbnode__perflog( char * name, app_rbtree_t * tree, app_logger_t * logger);
uint64_t app_rbnode__get_next_idp( app_rbtree_t * tree, uint8_t * obj);
void app_rbnode__set_idp_data( app_rbtree_t * tree, uint64_t id, uint8_t * data);
void app_rbnode__set_id_and_gen( app_rbtree_t * tree, uint64_t index, uint64_t fc, uint32_t GenId);
void app_rbnode__mark_as_allocate( app_rbtree_t * tree, uint64_t index, uint64_t fc, uint32_t GenId, uint8_t * obj);
void app_rbnode__get_set_object_and_gen_idp( app_rbtree_t * tree, uint64_t id, uint8_t * obj);
uint64_t app_rbnode__get_total( app_rbtree_t * tree);
uint64_t app_rbnode__get_available( app_rbtree_t * tree);
uint8_t * app_rbnode__get_obj_at_index( app_rbtree_t * tree, uint32_t index);
uint8_t * app_rbnode__find_idp( app_rbtree_t * tree, uint64_t id);
void app_rbnode__get( app_rbnode_t * node, uint64_t * id, uint32_t * GenId);
app_rbnode_t * app_rbnode__find( app_rbtree_t * tree, uint64_t id);
void app_rbnode__free_idp( app_rbtree_t * tree, uint64_t id);
void app_rbnode__set_maxid( app_rbtree_t * tree, uint64_t maxid);
//uint64_t app_rbnode__get_index( app_rbtree_t * tree, uint64_t base_id);
uint64_t app_rbnode__get_beginNumber( app_rbtree_t * tree);
uint64_t app_rbnode__get_baseid_idp( app_rbtree_t * tree, uint64_t id);
int app_rbnode__calc_gen_id( app_rbtree_t * tree, uint64_t id);

app_data_region_t * app_region__create();
app_data_pool_t * 	app_region__add_pool( app_data_region_t * region, unsigned char * Name, int Size, int Count);
uint8_t * 			app_region__allocate_fd( app_data_pool_t * datap);
uint8_t * 			app_region__allocate_fr( app_data_region_t * region, int Size);
app_data_pool_t * 	app_region__find_pool( app_data_region_t * region, int Size);
char * 				app_region__get_name( app_data_pool_t * datap);
void 				app_region__get_counts( app_data_pool_t * datap, uint64_t * TotalCount, uint64_t * Available);
void 				app_region__set_core( app_data_pool_t * datap, uint64_t Available);
void 				app_region__set_core_at_served( app_data_pool_t * datap, uint64_t Served);
void 				app_region__set_served( app_data_pool_t * datap, uint32_t Served);
void 				app_region__print_stats( app_logger_t * plogger, app_data_region_t * region);
uint64_t 			app_region__available_fd( app_data_pool_t * datap);
void 				app_region__free( uint8_t * data);
void 				app_region__info( uint8_t * data);
int 				app_region__datalen( uint8_t * data);
int 				app_region__mark_as_allocate( uint8_t * data);
uint8_t * 			app_region__get_head( app_data_pool_t * dpool);
uint8_t * 			app_region__get_next( app_data_pool_t * dpool, uint8_t * item);
int 				app_region__item_allocated( uint8_t * item);
uint8_t * 			app_region__get_pointer( uint8_t * item);
int 				app_region__create_index( app_data_pool_t * datap);
int 				app_region__is_item_allocated_at_index( app_data_pool_t * datap, int index);
uint8_t * 			app_region__get_pointer_at_index( app_data_pool_t * datap, int index);

void app__wait();

void app_logger__create_directory( char * dirname);
app_logger_t * app_logger__create( unsigned char * path, unsigned char * prefix, unsigned char * ext, uint32_t maxlines, uint32_t logLevel);
app_logger_categories_t * app_logger__set_category( app_logger_t * logger, uint32_t id, uint32_t enabled, unsigned char * tag, int taglen);
void app_logger__set_loglevel( app_logger_t * logger, int loglevel);
void app_logger__log( app_logger_t * logger, app_logger_categories_t * cat, int logLevel, char* mLogMessage, ...);
void app_logger__setplainlog( app_logger_t * logger);

void app_set_localtime( int lt);
void app_makeTimeStamp( char *datestring);
void app_makeTimeStamp2( char *datestring);

uint32_t app_hash_string__crc32b( uint8_t * str);

typedef struct ahc ahc_t;
typedef struct ahr ahr_t;
typedef struct aht aht_t;

aht_t * aht_c( uint8_t T, uint8_t KL, uint32_t cpcty, app_data_region_t * r);
uint8_t aht__ai32( aht_t * t, int32_t k, void * d);
uint8_t aht__au32( aht_t * t, uint32_t k, void * d);
uint8_t aht__ai64( aht_t * t, int64_t k, void * d);
uint8_t aht__au64( aht_t * t, uint64_t k, void * d);
uint8_t aht__as( aht_t * t, char * k, int kl, void * d);
uint8_t aht__di32( aht_t * t, int32_t k);
uint8_t aht__du32( aht_t * t, uint32_t k);
uint8_t aht__di64( aht_t * t, int64_t k);
uint8_t aht__du64( aht_t * t, uint64_t k);
uint8_t aht_fi32( aht_t * t,  int32_t k, void ** d);
uint8_t aht_fu32( aht_t * t, uint32_t k, void ** d);
uint8_t aht_fi64( aht_t * t, int64_t k, void ** d);
uint8_t aht_fu64( aht_t * t, uint64_t k, void ** d);
uint8_t aht_fs( aht_t * t, char * k, int kl, void ** d);
uint8_t aht__ds( aht_t * t, char * k, int kl);

void * app_bcd_to_buffer(const char * in, int in_len, void * out, int * out_len);
void * app_bcd_to_buffer_reverse_order( const char * in, void * out, int * out_len);
void * app_buffer_to_bcd(uint8_t * in, int in_len, void * out);
void   app_buffer_to_hex(char *in, int in_len, char * out, int out_len);
void * app_ascii_to_hex(char * in, int in_len, void * out, int out_len);
void * app_hex_to_ascii(void *in, int in_len, void *out, int out_len);

void app_printf_buf( char * key, uint8_t * u8, int len);
void app_printf_char( char * key, uint8_t * u8, int len);
void app_printf_char2( char * key, uint8_t * u8, int len);


void app_memcpy( char * dst, char * src, int len, int maxlen);
void app_reverse_str( char * dst, char * src, int len);

#endif










