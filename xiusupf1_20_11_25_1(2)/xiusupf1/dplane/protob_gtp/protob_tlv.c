#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "protob_tlv.h"

void * (* malloc_func)(size_t size);
void (* free_func)( uint8_t * b);
void protob_malloc_func( void * (*mf)(size_t size))
{
	malloc_func = mf;
}

void protob_free_func( void (* ff)( uint8_t * b))
{
	free_func = ff;
}


protob_tlv_t * protob_tlv_get( void)
{
    protob_tlv_t * tlv = NULL;
	
	if( malloc_func > 0) {
		tlv = (protob_tlv_t*)malloc_func( sizeof(protob_tlv_t));
	} else {
		tlv = (protob_tlv_t*)malloc( sizeof(protob_tlv_t));
	}
	
    //ogs_pool_alloc(&pool, &tlv);
	//protob_tlv_t obj_protob_tlv_t;
	//tlv = &obj_protob_tlv_t;

    /* intialize tlv node */
    memset( tlv, 0, sizeof( protob_tlv_t));
    return tlv;
}

void protob_tlv_free(protob_tlv_t *tlv)
{
    /* free tlv node to the node pool */
    //protob_pool_free(&pool, tlv);
	
	if( free_func > 0) {
		free_func( (uint8_t *)tlv);
	} else {
		free( (uint8_t *)tlv);
	}
}

void protob_tlv_init(void)
{
    //protob_pool_init(&pool, protob_core()->tlv.pool);
}

void protob_tlv_final(void)
{
    //protob_pool_final(&pool);
}

uint32_t protob_tlv_pool_avail(void)
{
    //return protob_pool_avail(&pool);
	return 0;
}

void protob_tlv_free_all(protob_tlv_t *root)
{
    /* free all tlv node to the node pool */
    protob_tlv_t *iter = root;
    protob_tlv_t *next = NULL;
    while (iter) {
        if(iter->embedded != NULL) {
            protob_tlv_free_all(iter->embedded);
        }
        next = iter->next;
        protob_tlv_free(iter);
        iter = next;
    }
}

uint8_t protob_tlv_value_8( protob_tlv_t * tlv)
{
    return (*((uint8_t*)(tlv->value)));
}

uint16_t protob_tlv_value_16( protob_tlv_t *tlv)
{
    uint16_t u_16;
    uint8_t *v = tlv->value;

    u_16 = ((v[0] <<  8) & 0xff00) |
           ((v[1]      ) & 0x00ff);

    return u_16;
}

uint32_t protob_tlv_value_32( protob_tlv_t * tlv)
{
    uint32_t u_32;
    uint8_t *v = tlv->value;

    u_32 = ((v[0] << 24) & 0xff000000) |
           ((v[1] << 16) & 0x00ff0000) |
           ((v[2] <<  8) & 0x0000ff00) |
           ((v[3]      ) & 0x000000ff);

    return u_32;
}


uint32_t protob_tlv_calc_length( protob_tlv_t * tlv, uint8_t mode)
{
    protob_tlv_t * iter = tlv;
    uint32_t length = 0;

    while( iter) 
	{
        /* this is length for type field */
        switch( mode) 
		{
			case PROTOB_TLV_MODE_T1_L1:
				length += 2;
				break;
			case PROTOB_TLV_MODE_T1_L2:
				length += 3;
				break;
			case PROTOB_TLV_MODE_T1_L2_I1:
			case PROTOB_TLV_MODE_T2_L2:
				length += 4;
				break;
			default:
				break;
        }

        /* this is length for type field */
        if(iter->embedded != NULL) 
		{
            iter->length = protob_tlv_calc_length( iter->embedded, mode);
        }

        /* this is length for value field */
        length += iter->length;
        iter = iter->next;
    }
	
    return length;
}

uint32_t protob_tlv_calc_count( protob_tlv_t *tlv)
{
    protob_tlv_t * iter = tlv;
    uint32_t count = 0;

    while(iter) {
        if(iter->embedded != NULL) {
            count += protob_tlv_calc_count(iter->embedded);
        } else {
            count++;
        }
        iter = iter->next;
    }
    return count;
}

static uint8_t * protob_put_type( uint32_t type, uint8_t * pos, uint8_t mode)
{    
    switch(mode) 
	{
		case PROTOB_TLV_MODE_T1_L1:
		case PROTOB_TLV_MODE_T1_L2:
		case PROTOB_TLV_MODE_T1_L2_I1:
			*(pos++) = type & 0xFF;
			break;
		case PROTOB_TLV_MODE_T2_L2:
			*(pos++) = (type >> 8) & 0xFF;
			*(pos++) = type & 0xFF;
			break;
		default:
			break;
    }
    return pos;
}

static uint8_t * protob_put_length( uint32_t length, uint8_t * pos, uint8_t mode)
{
    switch(mode) 
	{
		case PROTOB_TLV_MODE_T1_L1:
			*(pos++) = length & 0xFF;
			break;
		case PROTOB_TLV_MODE_T1_L2:
		case PROTOB_TLV_MODE_T1_L2_I1:
		case PROTOB_TLV_MODE_T2_L2:
			*(pos++) = (length >> 8) & 0xFF;
			*(pos++) = length & 0xFF;
			break;
		default:
			break;
    }

    return pos;
}

static uint8_t * protob_put_instance( uint8_t instance, uint8_t * pos, uint8_t mode)
{
    switch(mode) 
	{
        case PROTOB_TLV_MODE_T1_L2_I1:
            *(pos++) = instance & 0xFF;
            break;
        default:
            break;
    }

    return pos;
}

static uint8_t * protob_tlv_get_element( protob_tlv_t * tlv, uint8_t * blk, uint8_t mode)
{
    uint8_t *pos = blk;

    switch(mode) 
	{
		case PROTOB_TLV_MODE_T1_L1:
			tlv->type = *(pos++);
			tlv->length = *(pos++);
			break;
		case PROTOB_TLV_MODE_T1_L2:
			tlv->type = *(pos++);
			tlv->length = *(pos++) << 8;
			tlv->length += *(pos++);
			break;
		case PROTOB_TLV_MODE_T1_L2_I1:
			tlv->type = *(pos++);
			tlv->length = *(pos++) << 8;
			tlv->length += *(pos++);
			tlv->instance = *(pos++);
			break;
		case PROTOB_TLV_MODE_T2_L2:
			tlv->type = *(pos++) << 8;
			tlv->type += *(pos++);
			tlv->length = *(pos++) << 8;
			tlv->length += *(pos++);
			break;
		default:
			break;
    }

    tlv->value = pos;
    return (pos + protob_tlv_length(tlv));
}

static void protob_tlv_alloc_buff_to_tlv( protob_tlv_t * head, uint8_t * buff, uint32_t buff_len)
{
    head->buff_allocated = 1;
    head->buff_len = buff_len;
    head->buff_ptr = buff;
    head->buff = buff;
}

protob_tlv_t * protob_tlv_find_root( protob_tlv_t * tlv)
{
    protob_tlv_t * head = tlv->head;
    protob_tlv_t * parent;

    parent = head->parent;
    while(parent) 
	{
        head = parent->head;
        parent = head->parent;
    }

    return head;
}

protob_tlv_t * protob_tlv_add( protob_tlv_t * head, uint32_t type, uint32_t length, uint8_t instance, void * value)
{
    protob_tlv_t * curr = head;
    protob_tlv_t * new = NULL;

    new = protob_tlv_get();

    new->type = type;
    new->length = length;
    new->instance = instance;
    new->value = value;

    if (head != NULL && head->buff_allocated == 1) 
	{
        //protob_assert((head->buff_ptr - head->buff + length) < head->buff_len);

        memcpy( head->buff_ptr, value, length);
        new->value = head->buff_ptr;
        head->buff_ptr += length;
    }

    if(curr == NULL) 
	{
        new->head = new;
        new->tail = new;
    } 
	else 
	{
        head = head->head; /* in case head is not head */
        new->head = head;
        head->tail->next = new;
        head->tail = new;
    }
    
	return new;
}


protob_tlv_t *protob_tlv_copy( void * buff, uint32_t buff_len, uint32_t type, uint32_t length, uint8_t instance, void *value)
{
    protob_tlv_t *new = NULL;

    new = protob_tlv_get();

    new->type = type;
    new->length = length;
    new->instance = instance;
    new->value = value;
    new->head = new->tail = new;

    protob_tlv_alloc_buff_to_tlv( new, buff, buff_len);

    memcpy( new->buff_ptr, value, length);
    new->value = new->buff_ptr;
    new->buff_ptr += length;

    return new;
}


protob_tlv_t * protob_tlv_embed( protob_tlv_t * parent, uint32_t type, uint32_t length, uint8_t instance, void *value)
{
    protob_tlv_t * new = NULL, * root = NULL;

    new = protob_tlv_get();

    new->type = type;
    new->length = length;
    new->instance = instance;
    new->value = value;

    root = protob_tlv_find_root(parent);

    if(root->buff_allocated == 1) 
	{
        memcpy(root->buff_ptr, value, length);
        new->value = root->buff_ptr;
        root->buff_ptr += length;
    }

    if(parent->embedded == NULL) 
	{
        parent->embedded = new->head = new->tail = new;
        new->parent = parent;
    } 
	else 
	{
        new->head = parent->embedded;
        parent->embedded->tail->next = new;
        parent->embedded->tail = new;
    }

    return new;
}



uint32_t protob_tlv_render( protob_tlv_t * root, void *data, uint32_t length, uint8_t mode)
{
    protob_tlv_t *curr = root;
    uint8_t *pos = data;
    uint8_t *blk = data;
    uint32_t embedded_len = 0;

    while(curr) 
	{
		//printf("POS -- %p LINE=%d\n", pos, __LINE__);
        pos = protob_put_type( curr->type, pos, mode);

        if(curr->embedded == NULL) 
		{
            pos = protob_put_length( curr->length, pos, mode);
            pos = protob_put_instance( curr->instance, pos, mode);

            // if ((pos - blk) + protob_tlv_length(curr) > length)
                // protob_assert_if_reached();

            memcpy((char*)pos, (char*)curr->value, curr->length);
            pos += curr->length;
        } 
		else 
		{
            embedded_len = protob_tlv_calc_length( curr->embedded, mode);
            pos = protob_put_length( embedded_len, pos, mode);
            pos = protob_put_instance( curr->instance, pos, mode);
            protob_tlv_render( curr->embedded, pos, length - (uint32_t)(pos-blk), mode);
            pos += embedded_len;
        }
        curr = curr->next;
    }

    return (pos - blk);
}

/* protob_tlv_t parsing functions */
protob_tlv_t *protob_tlv_parse_block( uint32_t length, void *data, uint8_t mode)
{


    uint8_t * pos = data;
    uint8_t * blk = data;

	// int z = 0;
	// for( z = 0; z < length; z++)
	// {
		// printf( " %02X ", pos[z] & 0x0FF);
	// }
	// printf("\n");


    protob_tlv_t * root = NULL;
    protob_tlv_t * prev = NULL;
    protob_tlv_t * curr = NULL;

    root = curr = protob_tlv_get();
	
	
	// creates l-list of tlv, based on Type and Length, present in message


    //protob_assert(curr);
	//printf( "Before Type=%d LEn=%d LINE=%d\n", curr->type, curr->length, __LINE__);
    pos = protob_tlv_get_element( curr, pos, mode);
	//printf( "Type=%d LEn=%d LINE=%d\n", curr->type, curr->length, __LINE__);
	
    //protob_assert(pos);

    while(pos - blk < length) 
	{
        prev = curr;

        curr = protob_tlv_get();
		
		//protob_assert(curr);
        prev->next = curr;

		//printf( "Before Type=%d LEn=%d LINE=%d\n", curr->type, curr->length, __LINE__);
        pos = protob_tlv_get_element( curr, pos, mode);
		//printf( "Type=%d LEn=%d  %ld-%d LINE=%d\n", curr->type, curr->length, pos - blk,  length, __LINE__);
	
        //protob_assert(pos);
    }
	
	//printf("----------------%d\n", __LINE__);

    //protob_assert(length == (pos - blk));

    return root;
}

protob_tlv_t * protob_tlv_parse_embedded_block( protob_tlv_t * tlv, uint8_t mode)
{
    tlv->embedded = protob_tlv_parse_block( tlv->length, tlv->value, mode);
    return tlv->embedded;
}

/* tlv operation-related function */
protob_tlv_t * protob_tlv_find( protob_tlv_t * root, uint32_t type)
{
    protob_tlv_t *iter = root, *embed = NULL;
    while(iter) 
	{
        if(iter->type == type) 
		{
            return iter;
        }

        if(iter->embedded != NULL) 
		{
            embed = protob_tlv_find(iter->embedded, type);
            if(embed != NULL) 
			{
                return embed;
            }
        }
        iter = iter->next;
    }

    /* tlv for the designated type doesn't exist */
    return NULL;
}


void protob_pkbuf_free( protob_pkbuf_t * pkbuf)
{
	if( pkbuf->data)
	{
		free_func( (uint8_t *)pkbuf->odata);
	}
	pkbuf->odata = NULL;
	pkbuf->data = NULL;
	free_func( (uint8_t * )pkbuf);
	pkbuf = NULL;
}


protob_pkbuf_t * protob_pkbuf_alloc( void * m, uint16_t len, int allowcate)
{
	protob_pkbuf_t * pkbuf = NULL;
	
	if( malloc_func > 0) 
	{
		pkbuf = malloc_func( sizeof(protob_pkbuf_t));
		pkbuf->allowcate = allowcate;
		
		if( allowcate == 1)
		{
			pkbuf->data = malloc_func( len + 1);
			
			if( m)
			{
				memcpy( pkbuf->data, m, len);
			}
			pkbuf->odata = pkbuf->data;
		}
	} 
	else 
	{
		pkbuf = malloc( sizeof(protob_pkbuf_t));
		pkbuf->allowcate = allowcate;
		
		if( allowcate == 1)
		{		
			pkbuf->data = malloc( len + 1);
			
			if( m)
			{
				memcpy( pkbuf->data, m, len);
			}
		}
	}

	pkbuf->len = len;

    // pkbuf->data = cluster->buffer;
    // pkbuf->head = cluster->buffer;
    // pkbuf->tail = cluster->buffer;
    // pkbuf->end = cluster->buffer + size;
	
	return pkbuf;
}

void protob_pkbuf_reserve( protob_pkbuf_t * pkbuf, uint16_t len)
{
	//printf("%p LINE=%d\n", pkbuf->data, __LINE__);
	pkbuf->data += len;
	//printf("%p LINE=%d\n", pkbuf->data, __LINE__);
    //pkbuf->tail += len;
}

void protob_pkbuf_put( protob_pkbuf_t * pkbuf, uint16_t len)
{
    //pkbuf->tail += len;
    //pkbuf->len += len;
}












