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

#include "ls.h"

ls_ins_t * __ls_ins = NULL;

typedef struct ls_test
{
	uint32_t ip_1;
	uint32_t ip_2;
} ls_test_t;

void ls__releaseitem( ls_uint_entry_t * fitem)
{
	if( fitem)
	{
		pthread_mutex_lock( &__ls_ins->PoolLock);
		
		if(!__ls_ins->PoolHead)
		{
			__ls_ins->PoolHead = __ls_ins->PoolCurr = fitem;
		}
		else
		{
			__ls_ins->PoolCurr->Next = fitem;
			__ls_ins->PoolCurr = fitem;
		}
		
		pthread_mutex_unlock( &__ls_ins->PoolLock);
	}
}
		
ls_uint_entry_t * ls__allocitem()
{
	pthread_mutex_lock( &__ls_ins->PoolLock);
	ls_uint_entry_t * item = __ls_ins->PoolHead;
	
	if( item)
	{
		__ls_ins->PoolHead = item->Next;
		item->Next = NULL;
	}
	
	pthread_mutex_unlock( &__ls_ins->PoolLock);
	
	return item;
}

void ls__init( int poolsize)
{
	if(!__ls_ins)
	{
		__ls_ins = (ls_ins_t*)malloc(sizeof(ls_ins_t));
		memset( __ls_ins, 0, sizeof(ls_ins_t));


		pthread_mutex_init( &__ls_ins->tLock, NULL);
		__ls_ins->tHead = NULL;
		__ls_ins->tCurrent = NULL;
		
		pthread_mutex_init( &__ls_ins->PoolLock, NULL);
		__ls_ins->PoolHead = NULL;
		__ls_ins->PoolCurr = NULL;
		
		uint32_t bcap			= poolsize;
		uint8_t * bPtr2 		= (uint8_t *)malloc( sizeof(ls_uint_entry_t) * bcap);
		memset( bPtr2, 		0, (sizeof(ls_uint_entry_t) * bcap));
		
		ls_uint_entry_t * item = NULL;

		int i = 0;
		for( i = 0; i < bcap; i++)
		{
			item 				= (ls_uint_entry_t *)bPtr2;
			
			item->GHead			= NULL;
			item->GCurr			= NULL;
			item->GNext			= NULL;
			item->Next			= NULL;

			if(!__ls_ins->PoolHead)
			{
				__ls_ins->PoolHead = __ls_ins->PoolCurr = item;
			}
			else
			{
				__ls_ins->PoolCurr->Next = item;
				__ls_ins->PoolCurr = item;
			}
			
			bPtr2 += sizeof(ls_uint_entry_t);
		}
		
	}
}


ls_ins_t * ls__init2( int poolsize)
{
	if(1)
	{
		ls_ins_t * __ls_ins2 = (ls_ins_t*)malloc(sizeof(ls_ins_t));
		
		if(!__ls_ins2)
			return NULL;
		
		
		memset( __ls_ins2, 0, sizeof(ls_ins_t));


		pthread_mutex_init( &__ls_ins2->tLock, NULL);
		__ls_ins2->tHead = NULL;
		__ls_ins2->tCurrent = NULL;
		
		pthread_mutex_init( &__ls_ins2->PoolLock, NULL);
		__ls_ins2->PoolHead = NULL;
		__ls_ins2->PoolCurr = NULL;
		
		uint32_t bcap			= poolsize;
		uint8_t * bPtr2 		= (uint8_t *)malloc( sizeof(ls_uint_entry_t) * bcap);
		
		if(!bPtr2)
			return NULL;
		
		memset( bPtr2, 		0, (sizeof(ls_uint_entry_t) * bcap));
		
		ls_uint_entry_t * item = NULL;

		int i = 0;
		for( i = 0; i < bcap; i++)
		{
			item 				= (ls_uint_entry_t *)bPtr2;
			
			item->GHead			= NULL;
			item->GCurr			= NULL;
			item->GNext			= NULL;
			item->Next			= NULL;

			if(!__ls_ins2->PoolHead)
			{
				__ls_ins2->PoolHead = __ls_ins2->PoolCurr = item;
			}
			else
			{
				__ls_ins2->PoolCurr->Next = item;
				__ls_ins2->PoolCurr = item;
			}
			
			bPtr2 += sizeof(ls_uint_entry_t);
		}
		
		return __ls_ins2;
	}
}




ls_uint_table_t * ls__create_u128_table( uint32_t capacity, uint32_t keysize, int addToroot)
{
	ls_uint_table_t * table = (ls_uint_table_t *)malloc(sizeof(ls_uint_table_t));
	memset( table, 0, sizeof(ls_uint_table_t));
	
	table->keysize 		= keysize;
	//table->key32		= 0;
 	//table->key128		= 0;	
	pthread_mutex_init( &table->lock, NULL);
	table->capacity		= capacity;
	table->count 		= 0;

	table->entries		= (ls_uint_entry_t **) malloc( sizeof(ls_uint_entry_t*) * capacity);
	memset( table->entries, 0, ( sizeof(ls_uint_entry_t*) * capacity));
	
	int i = 0;
	uint8_t * bPtr 		= (uint8_t *)malloc( sizeof(ls_uint_entry_t) * capacity);
	memset( bPtr, 		0, (sizeof(ls_uint_entry_t) * capacity));

	ls_uint_entry_t * item = NULL;
	
	for( i = 0; i < capacity; i++)
	{
		item 				= (ls_uint_entry_t *)bPtr;
		
		item->GHead			= NULL;
		item->GCurr			= NULL;
		item->GNext			= NULL;
		item->Next			= NULL;
		item->key32			= 0;
		item->key641		= 0;
		item->key642		= 0;
		item->dataPtr		= NULL;

		pthread_mutex_init( &item->Glock, NULL);
		
		table->entries[i] 	= item;
		bPtr 				+= sizeof(ls_uint_entry_t);
	}
	
	if( addToroot == 1)
	{
		if(!__ls_ins->tHead)
		{
			__ls_ins->tHead = __ls_ins->tCurrent = table;
		}
		else
		{
			__ls_ins->tCurrent->Next = table;
			__ls_ins->tCurrent = table;
		}
	}

	return table;	
}




ls_uint_table_t * ls__create_uint_table( uint32_t capacity, uint32_t keysize, int addToroot)
{
	ls_uint_table_t * table = (ls_uint_table_t *)malloc(sizeof(ls_uint_table_t));
	memset( table, 0, sizeof(ls_uint_table_t));
	
	table->keysize 		= keysize;
	//table->key32		= 0;
 	//table->key128		= 0;	
	pthread_mutex_init( &table->lock, NULL);
	table->capacity		= capacity;
	table->count 		= 0;

	table->entries		= (ls_uint_entry_t **) malloc( sizeof(ls_uint_entry_t*) * capacity);
	memset( table->entries, 0, ( sizeof(ls_uint_entry_t*) * capacity));
	
	int i = 0;
	uint8_t * bPtr 		= (uint8_t *)malloc( sizeof(ls_uint_entry_t) * capacity);
	memset( bPtr, 		0, (sizeof(ls_uint_entry_t) * capacity));

	ls_uint_entry_t * item = NULL;
	
	for( i = 0; i < capacity; i++)
	{
		item 				= (ls_uint_entry_t *)bPtr;
		
		item->GHead			= NULL;
		item->GCurr			= NULL;
		item->GNext			= NULL;
		item->Next			= NULL;
		item->key32			= 0;
		item->key641		= 0;
		item->key642		= 0;
		item->dataPtr		= NULL;
		
		pthread_mutex_init( &item->Glock, NULL);
		
		table->entries[i] 	= item;
		bPtr 				+= sizeof(ls_uint_entry_t);
	}
	
	if( addToroot == 1)
	{
		if(!__ls_ins->tHead)
		{
			__ls_ins->tHead = __ls_ins->tCurrent = table;
		}
		else
		{
			__ls_ins->tCurrent->Next = table;
			__ls_ins->tCurrent = table;
		}
	}

	return table;
}

// ls_uint_table_t * ls__allocate_u32k_table( uint32_t key)
// {
	// ls_uint_table_t * table = __ls_ins->tHead;
	
	// while(table)
	// {
		// if( table->key32 == 0)
		// {
			// table->key32 = key;
			// return table;
		// }
		
		// table = table->Next;
	// }
	
	// return NULL;
// }

// ls_uint_table_t * ls__find_u32k_table( uint32_t key)
// {
	// ls_uint_table_t * table = __ls_ins->tHead;
	
	// while(table)
	// {
		// if( table->key32 == key)
		// {
			// return table;
		// }
		
		// table = table->Next;
	// }
	
	// return NULL;	
// }


int ls__setobject_u128_lm( ls_uint_table_t * table, uint8_t * key, uint8_t * dptr)
{
	uint64_t k1 = *(uint64_t*)&key[0];
	uint64_t k2 = *(uint64_t*)&key[8];
	
	pthread_mutex_lock( &table->lock);
	
	uint32_t pos = k2 % table->capacity;

	if( table->entries[pos]->key641 == 0 && table->entries[pos]->key642 == 0 || table->entries[pos]->key641 == k1 && table->entries[pos]->key642 == k2)
	{
		table->entries[pos]->key641 = k1;
		table->entries[pos]->key642 = k2;
		table->entries[pos]->dataPtr = dptr;
		
		pthread_mutex_unlock( &table->lock);
		return 1;
	}
	else
	{
		ls_uint_entry_t * item = ls__allocitem();
		
		if( item)
		{
			pthread_mutex_unlock( &table->lock);
		
			pthread_mutex_lock( &table->entries[pos]->Glock);
			
			if(!table->entries[pos]->GHead)
			{
				table->entries[pos]->GHead = table->entries[pos]->GCurr = item;
			}
			else
			{
				table->entries[pos]->GCurr->GNext = item;
				table->entries[pos]->GCurr = item;
			}
			
			// printf("adding to LL @ pos=%u key=%u\n", pos, key);
			item->key641 = k1;
			item->key642 = k2;
			item->dataPtr = dptr;
		
			pthread_mutex_unlock( &table->entries[pos]->Glock);
			return 1;
		} 
		else 
		{
			pthread_mutex_unlock( &table->lock);
			return 0;
		}
	}
}

int ls__setobject_u32k_lm( ls_uint_table_t * table, uint32_t key, uint8_t * dptr)
{
	pthread_mutex_lock( &table->lock);
	
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key32 == 0 || table->entries[pos]->key32 == key)
	{
		// printf("root is null, adding to root pos=%u key=%u\n", pos, key);

		table->entries[pos]->key32 = key;
		table->entries[pos]->dataPtr = dptr;
		
		pthread_mutex_unlock( &table->lock);
		return 1;
	}
	else
	{
		ls_uint_entry_t * item = ls__allocitem();
		
		if( item)
		{
			pthread_mutex_unlock( &table->lock);
		
			pthread_mutex_lock( &table->entries[pos]->Glock);
			
			if(!table->entries[pos]->GHead)
			{
				table->entries[pos]->GHead = table->entries[pos]->GCurr = item;
			}
			else
			{
				table->entries[pos]->GCurr->GNext = item;
				table->entries[pos]->GCurr = item;
			}
			
			// printf("adding to LL @ pos=%u key=%u\n", pos, key);
			item->key32 = key;
			item->dataPtr = dptr;
		
			pthread_mutex_unlock( &table->entries[pos]->Glock);
			return 1;
		} 
		else 
		{
			pthread_mutex_unlock( &table->lock);
			return 0;
		}
	}
}



int ls__setobject_u64k_lm( ls_uint_table_t * table, uint64_t key, uint8_t * dptr)
{
	pthread_mutex_lock( &table->lock);
	
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key641 == 0 || table->entries[pos]->key641 == key)
	{
		// printf("root is null, adding to root pos=%u key=%u\n", pos, key);

		table->entries[pos]->key641 = key;
		table->entries[pos]->dataPtr = dptr;
		
		pthread_mutex_unlock( &table->lock);
		return 1;
	}
	else
	{
		ls_uint_entry_t * item = ls__allocitem();
		
		if( item)
		{
			pthread_mutex_unlock( &table->lock);
		
			pthread_mutex_lock( &table->entries[pos]->Glock);
			
			if(!table->entries[pos]->GHead)
			{
				table->entries[pos]->GHead = table->entries[pos]->GCurr = item;
			}
			else
			{
				table->entries[pos]->GCurr->GNext = item;
				table->entries[pos]->GCurr = item;
			}
			
			// printf("adding to LL @ pos=%u key=%u\n", pos, key);
			item->key32 = key;
			item->dataPtr = dptr;
		
			pthread_mutex_unlock( &table->entries[pos]->Glock);
			return 1;
		} 
		else 
		{
			pthread_mutex_unlock( &table->lock);
			return 0;
		}
	}
}




uint8_t * ls__getobject_u32k( ls_uint_table_t * table, uint32_t key)
{
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key32 == key)
	{
		return table->entries[pos]->dataPtr;
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		while( item)
		{
			if( item->key32 == key)
			{
				pthread_mutex_unlock( &table->entries[pos]->Glock);
				return item->dataPtr;
			}
			item = item->GNext;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
	}
	return NULL;
}



uint8_t * ls__getobject_u64k( ls_uint_table_t * table, uint64_t key)
{
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key641 == key)
	{
		return table->entries[pos]->dataPtr;
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		while( item)
		{
			if( item->key641 == key)
			{
				pthread_mutex_unlock( &table->entries[pos]->Glock);
				return item->dataPtr;
			}
			item = item->GNext;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
	}
	return NULL;
}



uint8_t * ls__getobject_u128k( ls_uint_table_t * table, uint8_t * key)
{
	uint64_t k1 = *(uint64_t*)&key[0];
	uint64_t k2 = *(uint64_t*)&key[8];
	
	uint32_t pos = k2 % table->capacity;

	if(table->entries[pos]->key641 == k1 && table->entries[pos]->key642 == k2)
	{
		return table->entries[pos]->dataPtr;
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		while( item)
		{
			if( item->key641 == k1 && item->key642 == k2)
			{
				pthread_mutex_unlock( &table->entries[pos]->Glock);
				return item->dataPtr;
			}
			item = item->GNext;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
	}
	return NULL;
}



int ls__delobject_u32k( ls_uint_table_t * table, uint32_t key)
{
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key32 == key)
	{
		table->entries[pos]->key32 = 0;
		table->entries[pos]->dataPtr = NULL;
		
		// printf("deleted root  pos=%u key=%u\n", pos, key);
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		ls_uint_entry_t * pfitem = NULL;
		ls_uint_entry_t * fitem = NULL;
		
		while( item)
		{
			// printf("key=%u == %u\n", item->key32, key);

			if( item->key32 == key)
			{
				fitem = item;
				break;
			}

			pfitem = item;
			item = item->GNext;
		}
		
		// printf("fitem=%p pfitem=%p\n", fitem, pfitem);
		
		if( fitem)
		{
			fitem->key32 = 0;
			fitem->dataPtr = NULL;
			
			if( fitem == table->entries[pos]->GHead && !pfitem)
			{
				table->entries[pos]->GHead = NULL;
				table->entries[pos]->GCurr = NULL;
				// printf("deleted GRoot\n");
			}
			else if(pfitem)
			{
				// printf("pfitem=%p %d\n", pfitem, pfitem->key32);
				
				pfitem->GNext = fitem->GNext;
				
				if( fitem == table->entries[pos]->GCurr)
				{
					table->entries[pos]->GCurr = pfitem;
					// printf("deleted Tail\n");
				}
				// else
				// {
					// printf("deleted Middle\n");
				// }
			}
			
			fitem->GNext = NULL;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
		
		ls__releaseitem( fitem);
	}
}


int ls__delobject_u64k( ls_uint_table_t * table, uint64_t key)
{
	uint32_t pos = key % table->capacity;

	if( table->entries[pos]->key641 == key)
	{
		table->entries[pos]->key641 = 0;
		table->entries[pos]->dataPtr = NULL;
		
		// printf("deleted root  pos=%u key=%u\n", pos, key);
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		ls_uint_entry_t * pfitem = NULL;
		ls_uint_entry_t * fitem = NULL;
		
		while( item)
		{
			// printf("key=%u == %u\n", item->key641, key);

			if( item->key641 == key)
			{
				fitem = item;
				break;
			}

			pfitem = item;
			item = item->GNext;
		}
		
		// printf("fitem=%p pfitem=%p\n", fitem, pfitem);
		
		if( fitem)
		{
			fitem->key641 = 0;
			fitem->dataPtr = NULL;
			
			if( fitem == table->entries[pos]->GHead && !pfitem)
			{
				table->entries[pos]->GHead = NULL;
				table->entries[pos]->GCurr = NULL;
				// printf("deleted GRoot\n");
			}
			else if(pfitem)
			{
				// printf("pfitem=%p %ld\n", pfitem, pfitem->key641);
				
				pfitem->GNext = fitem->GNext;
				
				if( fitem == table->entries[pos]->GCurr)
				{
					table->entries[pos]->GCurr = pfitem;
					// printf("deleted Tail\n");
				}
				// else
				// {
					// printf("deleted Middle\n");
				// }
			}
			
			fitem->GNext = NULL;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
		
		ls__releaseitem( fitem);
	}
}




int ls__delobject_u128k( ls_uint_table_t * table, uint8_t * key)
{
	uint64_t k1 = *(uint64_t*)&key[0];
	uint64_t k2 = *(uint64_t*)&key[8];	
	
	uint32_t pos = k2 % table->capacity;

	if( table->entries[pos]->key641 == k1 && table->entries[pos]->key642 == k2)
	{
		table->entries[pos]->key641 	= 0;
		table->entries[pos]->key642 	= 0;
		table->entries[pos]->dataPtr 	= NULL;
		
		// printf("deleted root  pos=%u key=%u\n", pos, key);
	}
	else
	{
		pthread_mutex_lock( &table->entries[pos]->Glock);
		
		ls_uint_entry_t * item = table->entries[pos]->GHead;
		ls_uint_entry_t * pfitem = NULL;
		ls_uint_entry_t * fitem = NULL;
		
		while( item)
		{
			// printf("key=%u == %u\n", item->key32, key);

			if( item->key641 == k1 && item->key642 == k2)
			{
				fitem = item;
				break;
			}

			pfitem = item;
			item = item->GNext;
		}
		
		// printf("fitem=%p pfitem=%p\n", fitem, pfitem);
		
		if( fitem)
		{
			fitem->key641 	= 0;
			fitem->key642 	= 0;
			fitem->dataPtr 	= NULL;
			
			if( fitem == table->entries[pos]->GHead && !pfitem)
			{
				table->entries[pos]->GHead = NULL;
				table->entries[pos]->GCurr = NULL;
				// printf("deleted GRoot\n");
			}
			else if(pfitem)
			{
				// printf("pfitem=%p %d\n", pfitem, pfitem->key32);
				
				pfitem->GNext = fitem->GNext;
				
				if( fitem == table->entries[pos]->GCurr)
				{
					table->entries[pos]->GCurr = pfitem;
					// printf("deleted Tail\n");
				}
				// else
				// {
					// printf("deleted Middle\n");
				// }
			}
			
			fitem->GNext = NULL;
		}
		
		pthread_mutex_unlock( &table->entries[pos]->Glock);
		
		ls__releaseitem( fitem);
	}
}



/*
ls_test_t lst;
	memcpy( &lst, "\xC0\xA8\x95\x34\xC0\xA8\x95\x35", 8);
	
	uint32_t ip_1 = 0xC0A89534;
	uint32_t ip_2 = 0xC0A89535;
	
	uint32_t anipv4_2 = ((lst.ip_1 << 8) & 0xFF00FF00 ) | ((lst.ip_1 >> 8) & 0xFF00FF ); 
	anipv4_2 = (anipv4_2 << 16) | (anipv4_2 >> 16);	
	
	printf("hello ls ip-1=%u ip-2=%u  %u %u\n", 0xC0A89534, 0xC0A89535, anipv4_2 % 500000, ntohl(lst.ip_2) % 500000);

	char str_1[INET_ADDRSTRLEN];
	memset( str_1, 0, sizeof(str_1));
	inet_ntop( AF_INET, &ip_1, str_1, INET_ADDRSTRLEN);
	
	char str_2[INET_ADDRSTRLEN];
	memset( str_2, 0, sizeof(str_2));
	inet_ntop( AF_INET, &ip_2, str_2, INET_ADDRSTRLEN);
	
	printf("ip-1=%s ip-2=%s\n", str_1, str_2);
*/


void __initStartTime( iExecTime *oExecTime)
{
	gettimeofday( &oExecTime->before, NULL);
}


void __initEndTime( iExecTime *oExecTime)
{
	gettimeofday( &oExecTime->after, NULL);
	
	if ( oExecTime->before.tv_usec > oExecTime->after.tv_usec)
	{
		oExecTime->after.tv_usec += 1000000;
		oExecTime->after.tv_sec--;
	}
	
	oExecTime->lapsed.tv_usec = oExecTime->after.tv_usec - oExecTime->before.tv_usec;
	oExecTime->lapsed.tv_sec  = oExecTime->after.tv_sec  - oExecTime->before.tv_sec;	
}


#if IMAIN

// gcc -g3 ls.c -o ls -lpthread -DIMAIN
int main( int argc, char* argv[])
{
	// printf( "%u %u\n", 1 % 500000, 500001 % 500000);

	iExecTime oExecTime;
	
	ls__init( 100000);
	ls_uint_table_t * table = ls__create_uint_table( 500000, 32, 1);

	ls__setobject_u32k_lm( table, 1, NULL);
	ls__setobject_u32k_lm( table, 500001, NULL);
	ls__setobject_u32k_lm( table, 1000001, NULL);
	ls__setobject_u32k_lm( table, 2000001, NULL);

	__initStartTime( &oExecTime);
	ls__setobject_u32k_lm( table, 1500001, NULL);
	__initEndTime( &oExecTime);
	printf( "lapsed time  -> %ld-%ld\n", oExecTime.lapsed.tv_sec, oExecTime.lapsed.tv_usec);


	__initStartTime( &oExecTime);	
	ls__delobject_u32k(    table, 1500001);
	__initEndTime( &oExecTime);
	printf( "lapsed time  -> %ld-%ld\n", oExecTime.lapsed.tv_sec, oExecTime.lapsed.tv_usec);
	
	return 0;
}

#endif













