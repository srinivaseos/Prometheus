#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#ifndef PROTOB_TLV_H
#define PROTOB_TLV_H

#define PROTOB_TLV_MODE_T1_L1              		1
#define PROTOB_TLV_MODE_T1_L2              		2
#define PROTOB_TLV_MODE_T1_L2_I1           		3
#define PROTOB_TLV_MODE_T2_L2              		4

typedef struct protob_tlv_s
{
    /* for tlv management */
    struct protob_tlv_s *head;
    struct protob_tlv_s *tail;  /* this is used only for head ogs_tlv_t */
    struct protob_tlv_s *next;

    struct protob_tlv_s *parent;
    struct protob_tlv_s *embedded;

    /* tlv basic element */
    uint32_t type;
    uint32_t length;
    uint8_t instance;
    void *value;

    /* can be needed in encoding ogs_tlv_t*/
    uint8_t buff_allocated;
    uint32_t buff_len;
    unsigned char *buff_ptr;
    unsigned char *buff;
} protob_tlv_t;

typedef struct protob_pkbuf_s
{
   // ogs_cluster_t *cluster;

    // unsigned int len;

    // unsigned char *head;
    // unsigned char *tail;
    // unsigned char *data;
    // unsigned char *end;

    // const char *file_line;
    
    // ogs_pkbuf_pool_t *pool;
	
	
	uint8_t * odata;
	uint8_t * data;
	uint16_t len;
	uint8_t * edata;
	uint16_t elen;
	uint16_t allowcate; 	// =1
} protob_pkbuf_t;

protob_pkbuf_t * protob_pkbuf_alloc( void * m,uint16_t len, int);
void protob_pkbuf_reserve( protob_pkbuf_t * m,uint16_t len);
void protob_pkbuf_put( protob_pkbuf_t * m,uint16_t len);

#define protob_tlv_type(pTlv) pTlv->type
#define protob_tlv_length(pTlv) pTlv->length
#define protob_tlv_instance(pTlv) pTlv->instance
#define protob_tlv_value(pTlv) pTlv->value

/* ogs_tlv_t pool related functions */
protob_tlv_t * protob_tlv_get( void);
void protob_tlv_free( protob_tlv_t *tlv);
void protob_tlv_free_all( protob_tlv_t *root);
void protob_tlv_init( void);
void protob_tlv_final( void);

uint32_t protob_tlv_pool_avail( void);

/* ogs_tlv_t encoding functions */
protob_tlv_t * protob_tlv_add( protob_tlv_t *head, uint32_t type, uint32_t length, uint8_t instance, void *value);
protob_tlv_t * protob_tlv_copy(void *buff, uint32_t buff_len, uint32_t type, uint32_t length, uint8_t instance, void *value);
protob_tlv_t * protob_tlv_embed(protob_tlv_t *parent, uint32_t type, uint32_t length, uint8_t instance, void *value);

uint32_t protob_tlv_render( protob_tlv_t *root, void *data, uint32_t length, uint8_t mode);

/* protob_tlv_t parsing functions */
protob_tlv_t * protob_tlv_parse_block( uint32_t length, void *data, uint8_t mode);
protob_tlv_t * protob_tlv_parse_embedded_block( protob_tlv_t *tlv, uint8_t mode);

/* tlv operation-related function */
protob_tlv_t * protob_tlv_find(protob_tlv_t *root, uint32_t type);
protob_tlv_t * protob_tlv_find_root(protob_tlv_t *tlv);
uint32_t protob_tlv_calc_length(protob_tlv_t *tlv, uint8_t mode);
uint32_t protob_tlv_calc_count(protob_tlv_t *tlv);
uint8_t protob_tlv_value_8(protob_tlv_t *tlv);
uint16_t protob_tlv_value_16(protob_tlv_t *tlv);
uint32_t protob_tlv_value_32(protob_tlv_t *tlv);
void protob_pkbuf_free( protob_pkbuf_t * pkbuf);




#endif

