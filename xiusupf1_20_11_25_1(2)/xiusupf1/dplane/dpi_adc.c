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
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>


#include "app.h"
#include "app_stack.h"
#include "jansson.h"

#define MAX_ADC_RULES					300

typedef struct adcitem
{
	int AppId;
	int RGId;
} adcitem_t;

typedef struct dpiadc
{
	adcitem_t rules[MAX_ADC_RULES];
	int count;
	char fileName[100];
	int DefaultRGId;
} dpiadc_t;


static dpiadc_t adc_rules;

int app_config__get_int( json_t * json_config, char * key, int defval);


int dpiadc__find_rgid( int appid)
{
	int it = 0;

	for( it = 0; it < adc_rules.count; it++)
	{
		if( adc_rules.rules[it].AppId == appid)
		{
			return adc_rules.rules[it].RGId;
		}
	}
	
	return adc_rules.DefaultRGId;
}

adcitem_t * dpiadc__find_or_add( int appid)
{
	int it = 0;
	
	for( it = 0; it < adc_rules.count; it++)
	{
		if( adc_rules.rules[it].AppId == appid)
		{
			return &adc_rules.rules[it];
		}
	}
	
	if( adc_rules.count < MAX_ADC_RULES)
	{
		adc_rules.rules[adc_rules.count].AppId = appid;
		adc_rules.count++;
		
		return &adc_rules.rules[adc_rules.count - 1];
	}
	
	return NULL;
}




void * dpiadc__run_dpiadc_reader( void * args)
{
	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	
	json_t * json_config = NULL;
	json_t * json_item = NULL;
	int xp = 0, x = 0, i = 0, ic = 0, appid = 0;
	adcitem_t * adcItem = NULL;
	
	while(1)
	{
		json_config = NULL;
		json_config = json_load_file( adc_rules.fileName, 0, &error);

		if(!json_config)
		{
			printf("DPI-ADC:    error in json %s file at line=%d column=%d position=%d\n", 
				adc_rules.fileName, error.line, error.column, error.position);
		}	
		else
		{
			if( json_is_array( json_config))
			{
				ic = json_array_size( json_config);
				
				
				
				for( i = 0; i < ic; i++)
				{
					json_item = json_array_get( json_config, i);
					
					if( json_item)
					{
						appid 		= app_config__get_int( json_item, "APPId", 0);
						
						if( appid > 0)
						{
							adcItem	= dpiadc__find_or_add( appid);
						
							if( adcItem)
							{
								adcItem->RGId = app_config__get_int( json_item, "RGId", 0);
								//printf( "AdcItem=%p  AppId=%d  RGId=%d\n", adcItem, adcItem->AppId, adcItem->RGId);
							}
						}
					}
				}
			}
			
			json_decref( json_config);
		}


		xp = 15;
		
		for( x = 0; x < ( xp * 60); x++)
		{
			usleep( 999999);
		}
	}
}



void dpiadc__init( char * fileName, int defaultRGId)
{
	// printf( " ------------***************  adc-file-name=%s defaultRGId=%d   %s|%d \n", 
		// fileName, defaultRGId, __FILE__, __LINE__);
	
	memset( &adc_rules, 0, sizeof(dpiadc_t));

	adc_rules.count = 0;
	adc_rules.DefaultRGId = defaultRGId;

	if( strlen( fileName) > 0)
	{
		memcpy( adc_rules.fileName, fileName, strlen( fileName));

		pthread_t s_pthread_id;
		pthread_attr_t attr;
		pthread_attr_init( &attr);
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
		pthread_create( &s_pthread_id, &attr, dpiadc__run_dpiadc_reader, (void *)NULL);
	}
}