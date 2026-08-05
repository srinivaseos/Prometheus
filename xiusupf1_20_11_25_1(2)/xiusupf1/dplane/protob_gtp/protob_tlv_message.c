#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "protob_tlv.h"
#include "protob_tlv_message.h"
#include "protob_gtp.h"


protob_tlv_desc_t protob_tlv_desc_more1 = {  PROTOB_TLV_MORE, "More", 0, 1, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more2 = {  PROTOB_TLV_MORE, "More", 0, 2, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more3 = {  PROTOB_TLV_MORE, "More", 0, 3, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more4 = {  PROTOB_TLV_MORE, "More", 0, 4, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more5 = {  PROTOB_TLV_MORE, "More", 0, 5, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more6 = {  PROTOB_TLV_MORE, "More", 0, 6, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more7 = {  PROTOB_TLV_MORE, "More", 0, 7, 0, 0, { NULL } };
protob_tlv_desc_t protob_tlv_desc_more8 = {  PROTOB_TLV_MORE, "More", 0, 8, 0, 0, { NULL } };
	
	
static protob_tlv_t * tlv_add_leaf( protob_tlv_t * parent_tlv, protob_tlv_t * tlv, protob_tlv_desc_t * desc, void * msg)
{
    switch (desc->ctype) 
	{
		case PROTOB_TLV_UINT8:
		case PROTOB_TLV_INT8:
		{
			protob_tlv_uint8_t *v = (protob_tlv_uint8_t *)msg;

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, 1, desc->instance, &v->u8);
			else
				tlv = protob_tlv_add( tlv, desc->type, 1, desc->instance, &v->u8);

			break;
		}
		case PROTOB_TLV_UINT16:
		{
			protob_tlv_uint16_t *v = (protob_tlv_uint16_t *)msg;

			v->u16 = htobe16(v->u16);

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, 2, desc->instance, &v->u16);
			else
				tlv = protob_tlv_add( tlv, desc->type, 2, desc->instance, &v->u16);

			break;
		}
		case PROTOB_TLV_UINT24:
		case PROTOB_TLV_INT24:
		{
			protob_tlv_uint24_t *v = (protob_tlv_uint24_t *)msg;

			v->u24 = v->u24 << 8;
			v->u24 = htobe32(v->u24);

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, 3, desc->instance, &v->u24);
			else
				tlv = protob_tlv_add( tlv, desc->type, 3, desc->instance, &v->u24);

			break;
		}
		case PROTOB_TLV_UINT32:
		case PROTOB_TLV_INT32:
		{
			protob_tlv_uint32_t *v = (protob_tlv_uint32_t *)msg;

			v->u32 = htobe32(v->u32);

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, 4, desc->instance, &v->u32);
			else
				tlv = protob_tlv_add( tlv, desc->type, 4, desc->instance, &v->u32);
			
			break;
		}
		case PROTOB_TLV_FIXED_STR:
		{
			protob_tlv_octet_t *v = (protob_tlv_octet_t *)msg;

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, desc->length, desc->instance, v->data);
			else
				tlv = protob_tlv_add( tlv, desc->type, desc->length, desc->instance, v->data);
			break;
		}
		case PROTOB_TLV_VAR_STR:
		{
			protob_tlv_octet_t *v = ( protob_tlv_octet_t *)msg;

			if (v->len == 0) 
			{
				//?
			}

			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, v->len, desc->instance, v->data);
			else
				tlv = protob_tlv_add( tlv, desc->type, v->len, desc->instance, v->data);
			break;
		}
		case PROTOB_TLV_NULL:
		{
			if (parent_tlv)
				tlv = protob_tlv_embed( parent_tlv, desc->type, 0, desc->instance, NULL);
			else
				tlv = protob_tlv_add( tlv,  desc->type, 0, desc->instance, NULL);
			break;
		}
		default:
			break;
    }

    return tlv;
}


static uint32_t tlv_add_compound( protob_tlv_t **root, protob_tlv_t * parent_tlv, protob_tlv_desc_t * parent_desc, void * msg, int depth)
{
    protob_tlv_presence_t * presence_p;
    protob_tlv_desc_t * desc = NULL, * next_desc = NULL;
    protob_tlv_t *tlv = NULL, *emb_tlv = NULL;
    uint8_t *p = msg;
    uint32_t offset = 0, count = 0;
    int i, j, r;
    char indent[17] = "                "; /* 16 spaces */

    indent[depth*2] = 0;

    *root = NULL;

    for (i = 0, desc = parent_desc->child_descs[i]; desc != NULL; i++, desc = parent_desc->child_descs[i]) 
	{
        next_desc = parent_desc->child_descs[i+1];
        
		if (next_desc != NULL && next_desc->ctype == PROTOB_TLV_MORE) 
		{
            int offset2 = offset;
            for (j = 0; j < next_desc->length; j++) 
			{
                presence_p = (protob_tlv_presence_t *)(p + offset2);

                if (*presence_p == 0)
                    break;

                if (desc->ctype == PROTOB_TLV_COMPOUND) 
				{
                    // ogs_trace("BUILD %sC#%d [%s] T:%d I:%d (vsz=%d) off:%p ", indent, i, desc->name, desc->type, desc->instance, desc->vsize, p + offset2);

                    if (parent_tlv)
                        tlv = protob_tlv_embed( parent_tlv, desc->type, 0, desc->instance, NULL);
                    else
                        tlv = protob_tlv_add( tlv, desc->type, 0, desc->instance, NULL);

                    r = tlv_add_compound(&emb_tlv, tlv, desc, p + offset2 + sizeof(protob_tlv_presence_t), depth + 1);
                    count += 1 + r;
                } 
				else 
				{
                    // ogs_trace("BUILD %sL#%d [%s] T:%d L:%d I:%d (cls:%d vsz:%d) off:%p ", indent, i, desc->name, desc->type, desc->length, desc->instance, desc->ctype, desc->vsize, p + offset2);
                    tlv = tlv_add_leaf( parent_tlv, tlv, desc, p + offset2);
                    count++;
                }

                if (*root == NULL)
                    *root = tlv;

                offset2 += desc->vsize;
            }
            offset += desc->vsize * next_desc->length;
            i++;
        } 
		else 
		{
            presence_p = (protob_tlv_presence_t *)(p + offset);

            if (*presence_p) 
			{
                if (desc->ctype == PROTOB_TLV_COMPOUND) 
				{
                    // ogs_trace("BUILD %sC#%d [%s] T:%d I:%d (vsz=%d) off:%p ", indent, i, desc->name, desc->type, desc->instance, desc->vsize, p + offset);

                    if (parent_tlv)
                        tlv = protob_tlv_embed( parent_tlv, desc->type, 0, desc->instance, NULL);
                    else
                        tlv = protob_tlv_add( tlv, desc->type, 0, desc->instance, NULL);

                    r = tlv_add_compound( &emb_tlv, tlv, desc, p + offset + sizeof(protob_tlv_presence_t), depth + 1);
                    count += 1 + r;
                } 
				else 
				{
                    // ogs_trace("BUILD %sL#%d [%s] T:%d L:%d I:%d  (cls:%d vsz:%d) off:%p ", indent, i, desc->name, desc->type, desc->length, desc->instance, desc->ctype, desc->vsize,  p + offset);
                    tlv = tlv_add_leaf(parent_tlv, tlv, desc, p + offset);
                    count++;
                }

                if (*root == NULL)
                    *root = tlv;
            }
            offset += desc->vsize;
        }
    }

    return count;
}


protob_pkbuf_t * protob_tlv_build_msg( protob_tlv_desc_t * desc, void *msg, int mode)
{
    protob_tlv_t *root = NULL;
    uint32_t r, length, rendlen;
    protob_pkbuf_t * pkbuf = NULL;

    if (desc->child_descs[0]) 
	{
        r = tlv_add_compound(&root, NULL, desc, msg, 0);
        length = protob_tlv_calc_length(root, mode);
    } else {
        length = 0;
    }

    pkbuf = protob_pkbuf_alloc( NULL, PROTOB_TLV_MAX_HEADROOM + length, 1);
    protob_pkbuf_reserve( pkbuf, PROTOB_TLV_MAX_HEADROOM);
    protob_pkbuf_put( pkbuf, length);

    if (desc->child_descs[0]) 
	{
        //printf("-- %p HR=%d LINE=%d\n", pkbuf->data, PROTOB_TLV_MAX_HEADROOM, __LINE__);
		rendlen = protob_tlv_render(root, pkbuf->data, length, mode);
        protob_tlv_free_all( root);
    }

    return pkbuf;
}


static protob_tlv_desc_t * tlv_find_desc( uint8_t * desc_index, uint32_t * tlv_offset, protob_tlv_desc_t * parent_desc, protob_tlv_t * tlv)
{
    protob_tlv_desc_t *prev_desc = NULL, *desc = NULL;
    int i, offset = 0;

    for (i = 0, desc = parent_desc->child_descs[i]; desc != NULL; i++, desc = parent_desc->child_descs[i]) 
	{
        if (desc->type == tlv->type && desc->instance == tlv->instance) 
		{
            *desc_index = i;
            *tlv_offset = offset;
            break;
        }

        if (desc->ctype == PROTOB_TLV_MORE) 
		{
            offset += prev_desc->vsize * (desc->length - 1);
        } else {
            offset += desc->vsize;
        }

        prev_desc = desc;
    }

    return desc;
}

static int tlv_parse_leaf( void * msg, protob_tlv_desc_t * desc, protob_tlv_t * tlv)
{
    switch (desc->ctype) 
	{
		case PROTOB_TLV_UINT8:
		case PROTOB_TLV_INT8:
		{
			protob_tlv_uint8_t *v = (protob_tlv_uint8_t *)msg;
			v->presence = 0;
			
			if (tlv->length != 1)
			{
				//ogs_error("Invalid TLV length %d. It should be 1", tlv->length);
				return -1;
			}
			
			v->u8 = *(uint8_t*)(tlv->value);
			v->presence = 1;
			
			//printf( "GTPv2 U8  %s=%u\n", desc->name, v->u8);
			break;
		}
		case PROTOB_TLV_UINT16:
		case PROTOB_TLV_INT16:
		{
			protob_tlv_uint16_t *v = (protob_tlv_uint16_t *)msg;
			v->presence = 0;
			
			if (tlv->length != 2)
			{
				//ogs_error("Invalid TLV length %d. It should be 2", tlv->length);
				return -1;
			}
			
			v->u16 = ((((uint8_t*)tlv->value)[0]<< 8)&0xff00) | ((((uint8_t*)tlv->value)[1]    )&0x00ff);
			//printf( "GTPv2 U16  %s=%u\n", desc->name, v->u16);
			v->presence = 1;
			
			break;
		}
		case PROTOB_TLV_UINT24:
		case PROTOB_TLV_INT24:
		{
			protob_tlv_uint24_t *v = (protob_tlv_uint24_t *)msg;
			v->presence = 0;
			
			if (tlv->length != 3)
			{
				//ogs_error("Invalid TLV length %d. It should be 3", tlv->length);
				return -1;
			}
			
			v->presence = 1;
			v->u24 = ((((uint8_t*)tlv->value)[0]<<16)&0x00ff0000) | ((((uint8_t*)tlv->value)[1]<< 8)&0x0000ff00) | ((((uint8_t*)tlv->value)[2]    )&0x000000ff);
			
			//printf( "GTPv2 U24  %s=%u\n", desc->name, v->u24);
			break;
		}
		case PROTOB_TLV_UINT32:
		case PROTOB_TLV_INT32:
		{
			protob_tlv_uint32_t *v = (protob_tlv_uint32_t *)msg;
			v->presence = 0;
			
			if (tlv->length != 4)
			{
				//ogs_error("Invalid TLV length %d. It should be 4", tlv->length);
				return -1;
			}
			
			v->presence = 1;
			v->u32 = ((((uint8_t*)tlv->value)[0]<<24)&0xff000000) | ((((uint8_t*)tlv->value)[1]<<16)&0x00ff0000) |
				   ((((uint8_t*)tlv->value)[2]<< 8)&0x0000ff00) | ((((uint8_t*)tlv->value)[3]    )&0x000000ff);
			
			//printf( "GTPv2 U32  %s=%u\n", desc->name, v->u32);
			break;
		}
		case PROTOB_TLV_FIXED_STR:
		{
			protob_tlv_octet_t *v = (protob_tlv_octet_t *)msg;
			v->presence = 0;
			
			if (tlv->length != desc->length)
			{
				//ogs_error("Invalid TLV length %d. It should be %d", tlv->length, desc->length);
				return -1;
			}

			v->presence = 1;
			v->data = tlv->value;
			v->len = tlv->length;
			
			// printf( "GTPv2 FS  %s ", desc->name);
			
			// int z = 0;
			// for( z = 0; z < tlv->length; z++)
			// {
				// char c = *(char*)&v->data[z];
				// printf( " %02X ", c & 0xFF);
			// }
			// printf("\n");
			
			break;
		}
		case PROTOB_TLV_VAR_STR:
		{
			protob_tlv_octet_t *v = (protob_tlv_octet_t *)msg;

			v->data = tlv->value;
			v->len = tlv->length;
			v->presence = 1;
			
			//printf( "GTPv2 VS  %s \n", desc->name);
			// int z = 0;
			// for( z = 0; z < tlv->length; z++)
			// {
				// char c = *(char*)&v->data[z];
				// printf( " %02X ", c & 0xFF);
			// }
			// printf("\n");
			
			break;
		}
		case PROTOB_TLV_NULL:
		{
			if (tlv->length != 0) {
				//ogs_error("Invalid TLV length %d. It should be 0", tlv->length);
				return -1;
			}
			break;
		}
		default:
			break;
    }

    return 1;
}

static int tlv_parse_compound( void * msg, protob_tlv_desc_t * parent_desc, protob_tlv_t * parent_tlv, int depth, int mode)
{
    int rv;
    protob_tlv_presence_t * presence_p = (protob_tlv_presence_t *)msg;
    protob_tlv_desc_t * desc = NULL, *next_desc = NULL;
    protob_tlv_t *tlv = NULL, *emb_tlv = NULL;
    uint8_t *p = msg;
    uint32_t offset = 0;
    uint8_t index = 0;
    int i = 0, j;
    char indent[17] = "                "; /* 16 spaces */

    indent[depth*2] = 0;

    tlv = parent_tlv;
    while (tlv) 
	{
        desc = tlv_find_desc( &index, &offset, parent_desc, tlv);
        if (desc == NULL) 
		{
            //ogs_warn("Unknown TLV type [%d]", tlv->type);
            tlv = tlv->next;
            continue;
        }

        presence_p = (protob_tlv_presence_t *)(p + offset);

        /* Multiple of the same type TLV may be included */
        next_desc = parent_desc->child_descs[index+1];
		
        if (next_desc != NULL && next_desc->ctype == PROTOB_TLV_MORE) 
		{
            for (j = 0; j < next_desc->length; j++) 
			{
                presence_p = (protob_tlv_presence_t *)(p + offset + desc->vsize * j);
                if (*presence_p == 0) 
				{
                    offset += desc->vsize * j;
                    break;
                }
            }
			
            if (j == next_desc->length) 
			{
                //ogs_fatal("Multiple of the same type TLV need more room");
                //ogs_assert_if_reached();
                tlv = tlv->next;
                continue;
            }
        }

        if (desc->ctype == PROTOB_TLV_COMPOUND) 
		{
            emb_tlv = protob_tlv_parse_embedded_block(tlv, mode);
            if (emb_tlv == NULL) 
			{
                //ogs_error("Error while parse TLV");
                return -1;
            }

            // printf("COMPOUND - PARSE %sC#%d [%s] T:%d I:%d (vsz=%d) off:%p \n",
                    // indent, i++, desc->name, desc->type, desc->instance, 
                    // desc->vsize, p + offset);

            offset += sizeof(protob_tlv_presence_t);

            rv = tlv_parse_compound( p + offset, desc, emb_tlv, depth + 1, mode);
            if (rv != 1) 
			{
                //ogs_error("Can't parse compound TLV");
                return -1;
            }

            *presence_p = 1;
        } 
		else 
		{
            // printf("LEAF - PARSE %sL#%d [%s] T:%d L:%d I:%d "
                    // "(cls:%d vsz:%d) off:%p \n",
                    // indent, i++, desc->name, desc->type, desc->length, 
                    // desc->instance, desc->ctype, desc->vsize, p + offset);

            rv = tlv_parse_leaf(p + offset, desc, tlv);
            if (rv != 1) 
			{
                //ogs_error("Can't parse leaf TLV");
                return -1;
            }

            *presence_p = 1;
        }

        tlv = tlv->next;
    }

    return 1;
}


int protob_tlv_parse_msg( void * msg, protob_tlv_desc_t * desc, protob_pkbuf_t * pkbuf, int mode)
{
    int rv;
    protob_tlv_t *root;

	//printf( "elen=%u  %s|%s|%d\n", pkbuf->elen, __FILE__, __FUNCTION__, __LINE__);

    //root = protob_tlv_parse_block( pkbuf->len, pkbuf->data, mode);
	root = protob_tlv_parse_block( pkbuf->elen, pkbuf->edata, mode);
	
	// printf("exiting %d\n", __LINE__);
	// exit(0);
	
    if (root == NULL) 
	{
        printf("Can't parse TLV message\n");
        return -1;
    }

    rv = tlv_parse_compound( msg, desc, root, 0, mode);
    protob_tlv_free_all(root);

    return rv;
}


int protob_tlv_parse_create_session_request( protob_gtp_create_session_request_t * create_session_request, protob_tlv_desc_t * desc, protob_pkbuf_t * pkbuf, int mode)
{
	int rv = 1;
    protob_tlv_t * root = protob_tlv_parse_block( pkbuf->elen, pkbuf->edata, mode);

    if (root == NULL) 
	{
        printf("Can't parse TLV message\n");
        return -1;
    }
	
	protob_tlv_t * tlv = root;
	
	// while( tlv)
	// {
		// switch( tlv->type)
		// {
			// case 1:
				// create_session_request->imsi.presence = 1;
				// create_session_request->imsi.data = tlv->value;
				// create_session_request->imsi.len = tlv->length;
				// break;
			// case 75:
				// create_session_request->me_identity.presence = 1;
				// create_session_request->me_identity.data = tlv->value;
				// create_session_request->me_identity.len = tlv->length;
				// break;
			// case 86:
				// create_session_request->user_location_information.presence = 1;
				// create_session_request->user_location_information.data = tlv->value;
				// create_session_request->user_location_information.len = tlv->length;
				// break;
			// case 83:
				// create_session_request->serving_network.presence = 1;
				// create_session_request->serving_network.data = tlv->value;
				// create_session_request->serving_network.len = tlv->length;
				// break;
			// case 82:
				// create_session_request->rat_type.presence = 1;
				// create_session_request->rat_type.data = tlv->value;
				// create_session_request->rat_type.len = tlv->length;
				// break;
			// case 87:
				// create_session_request->sender_f_teid_for_control_plane.presence = 1;
				// create_session_request->pgw_s5_s8_address_for_control_plane_or_pmip.presence = 1;
				// create_session_request->sender_f_teid_for_control_plane.data = tlv->value;
				// create_session_request->sender_f_teid_for_control_plane.len = tlv->length;
				// break;				
			// case 71:
				// create_session_request->access_point_name.presence = 1;
				// create_session_request->access_point_name.data = tlv->value;
				// create_session_request->access_point_name.len = tlv->length;
				// break;
			// case 128:
				// create_session_request->selection_mode.presence = 1;
				// create_session_request->selection_mode.data = tlv->value;
				// create_session_request->selection_mode.len = tlv->length;
				// break;
			// case 99:
				// create_session_request->pdn_type.presence = 1;
				// create_session_request->pdn_type.data = tlv->value;
				// create_session_request->pdn_type.len = tlv->length;
				// break;
			// case 79:
				// create_session_request->pdn_address_allocation.presence = 1;
				// create_session_request->pdn_address_allocation.data = tlv->value;
				// create_session_request->pdn_address_allocation.len = tlv->length;
				// break;
			// case 127:
				// create_session_request->maximum_apn_restriction.presence = 1;
				// create_session_request->maximum_apn_restriction.u8 = *(uint8_t*)&tlv->value;
				// //create_session_request->maximum_apn_restriction.len = tlv->length;
				// break;
			// case 72:
				// create_session_request->aggregate_maximum_bit_rate.presence = 1;
				// create_session_request->aggregate_maximum_bit_rate.data = tlv->value;
				// create_session_request->aggregate_maximum_bit_rate.len = tlv->length;
				// break;
			// case 78:
				// create_session_request->protocol_configuration_options.presence = 1;
				// create_session_request->protocol_configuration_options.data = tlv->value;
				// create_session_request->protocol_configuration_options.len = tlv->length;
				// break;
			// case 93:
				// create_session_request->bearer_contexts_to_be_created.presence = 1;
				// // create_session_request->bearer_contexts_to_be_created.data = tlv->value;
				// // create_session_request->bearer_contexts_to_be_created.len = tlv->length;
				// break;
			// case 114:
				// create_session_request->ue_time_zone.presence = 1;
				// create_session_request->ue_time_zone.data = tlv->value;
				// create_session_request->ue_time_zone.len = tlv->length;
				// break;
			// case 95:
				// create_session_request->charging_characteristics.presence = 1;
				// create_session_request->charging_characteristics.data = tlv->value;
				// create_session_request->charging_characteristics.len = tlv->length;
				// break;
			// default:
				// break;
		// }
		
		// //printf( "type=%u length=%u\n", tlv->type, tlv->length);
		
		// tlv = tlv->next;
	// }
	
	
    protob_tlv_free_all(root);	
	return rv;
}



