#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#ifndef PROTOB_TLV_MESSAGE_H
#define PROTOB_TLV_MESSAGE_H

#include "protob_tlv.h"


#define PROTOB_TLV_MAX_HEADROOM 				16
#define PROTOB_TLV_VARIABLE_LEN 				0
#define PROTOB_TLV_MAX_MORE 					8
#define PROTOB_TLV_1_OR_MORE(__v) __v[PROTOB_TLV_MAX_MORE]

#define PROTOB_TLV_MAX_CHILD_DESC 128

typedef enum 
{
    PROTOB_TLV_UINT8,
    PROTOB_TLV_UINT16,
    PROTOB_TLV_UINT24,
    PROTOB_TLV_UINT32,
    PROTOB_TLV_INT8,
    PROTOB_TLV_INT16,
    PROTOB_TLV_INT24,
    PROTOB_TLV_INT32,
    PROTOB_TLV_FIXED_STR,
    PROTOB_TLV_VAR_STR,
    PROTOB_TLV_NULL,
    PROTOB_TLV_MORE,
    PROTOB_TLV_COMPOUND,
    PROTOB_TLV_MESSAGE,
} protob_tlv_type_e;

#pragma pack(4)
typedef struct protob_tlv_desc_s {
    protob_tlv_type_e ctype;
    const char *name;
    uint16_t type;
    uint16_t length;
    uint8_t  instance;
    uint16_t vsize;
    void *child_descs[PROTOB_TLV_MAX_CHILD_DESC];
} protob_tlv_desc_t;

extern protob_tlv_desc_t protob_tlv_desc_more1;
extern protob_tlv_desc_t protob_tlv_desc_more2;
extern protob_tlv_desc_t protob_tlv_desc_more3;
extern protob_tlv_desc_t protob_tlv_desc_more4;
extern protob_tlv_desc_t protob_tlv_desc_more5;
extern protob_tlv_desc_t protob_tlv_desc_more6;
extern protob_tlv_desc_t protob_tlv_desc_more7;
extern protob_tlv_desc_t protob_tlv_desc_more8;

#pragma pack(4)
typedef uint64_t protob_tlv_presence_t;

/* 8-bit Unsigned integer */
#pragma pack(4)
typedef struct protob_tlv_uint8_s {
    protob_tlv_presence_t presence;
    uint8_t u8;
} protob_tlv_uint8_t;

/* 16-bit Unsigned integer */
#pragma pack(4)
typedef struct protob_tlv_uint16_s {
    protob_tlv_presence_t presence;
    uint16_t u16;
} protob_tlv_uint16_t;

/* 24-bit Unsigned integer */
#pragma pack(4)
typedef struct protob_tlv_uint24_s {
    protob_tlv_presence_t presence;
    uint32_t u24; /* Only 3 bytes valid */
} protob_tlv_uint24_t;

/* 32-bit Unsigned integer */
#pragma pack(4)
typedef struct protob_tlv_uint32_s {
    protob_tlv_presence_t presence;
    uint32_t u32;
} protob_tlv_uint32_t;

/* 8-bit Signed integer */
#pragma pack(4)
typedef struct protob_tlv_int8_s {
    protob_tlv_presence_t presence;
    int8_t i8;
} protob_tlv_int8_t;

/* 16-bit Signed integer */
#pragma pack(4)
typedef struct protob_tlv_int16_s {
    protob_tlv_presence_t presence;
    int16_t i16;
} protob_tlv_int16_t;

/* 24-bit Signed integer */
#pragma pack(4)
typedef struct tlv_int24_s {
    protob_tlv_presence_t presence;
    int32_t i24; /* Only 3 bytes valid */
} tlv_int24_t;

/* 32-bit Signed integer */
#pragma pack(4)
typedef struct protob_tlv_int32_s {
    protob_tlv_presence_t presence;
    int32_t i32;
} protob_tlv_int32_t;

/* Octets */
#define protob_TLV_CLEAR_DATA(__dATA) \
    do { \
        if ((__dATA)->data) { \
            protob_free((__dATA)->data); \
            (__dATA)->data = NULL; \
            (__dATA)->len = 0; \
            (__dATA)->presence = 0; \
        } \
    } while(0)
#define protob_TLV_STORE_DATA(__dST, __sRC) \
    do { \
        protob_TLV_CLEAR_DATA(__dST); \
        (__dST)->presence = (__sRC)->presence; \
        (__dST)->len = (__sRC)->len; \
        (__dST)->data = protob_calloc((__dST)->len, sizeof(uint8_t)); \
        memcpy((__dST)->data, (__sRC)->data, (__dST)->len); \
    } while(0)

#pragma pack(4)
typedef struct protob_tlv_octet_s {
    protob_tlv_presence_t presence;
    void *data;
    uint32_t len;
} protob_tlv_octet_t;

/* No value */
#pragma pack(4)
typedef struct protob_tlv_null_s {
    protob_tlv_presence_t presence;
} protob_tlv_null_t;


protob_pkbuf_t * protob_tlv_build_msg( protob_tlv_desc_t * desc, void * msg, int mode);
int protob_tlv_parse_msg( void * msg, protob_tlv_desc_t *desc, protob_pkbuf_t * pkbuf, int mode);
		
		

#endif


