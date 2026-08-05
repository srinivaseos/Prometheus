#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
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
#include <ctype.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <numa.h>
#include <mntent.h>

#include "jansson.h"


typedef struct esetup
{
	char DriverName[100];
	uint32_t HugePagesGB;
	uint32_t NumaNodes;
	
	struct 
	{
		char Eth[32];
		char Bus[32];
	} Eths[12];
	int EthsCount;
	
} esetup_t;

esetup_t * __esetup = NULL;

int dpdk_config__get_int( json_t * json_config, char * key, int defval, int minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		int v = json_integer_value( jObj);
		if( v < minval)
			return minval;
		else
			return v;
	}
	return defval;
}

int dpdk_config__get_int2( json_t * json_config, char * key, int defval, int minval)
{
	json_t * jObj = json_object_get( json_config, key);
	if( jObj) {
		int v = json_integer_value( jObj);
		return v;
	}
	return defval;
}


void app_config__set_str( json_t * json_config, char * key, char * vItem, int maxlen)
{
	if(!key)
	{
		int kv = json_string_length( json_config);
		const char * v = json_string_value( json_config);
		if( kv < maxlen)
		{
			memcpy( vItem, v, kv);
		}
		else
		{
			memcpy( vItem, v, maxlen);
		}
		return;
	}
	
	json_t * jObj = json_object_get( json_config, key);
	if( jObj)
	{
		int kv = json_string_length( jObj);
		const char * v = json_string_value( jObj);
		if( kv < maxlen)
		{
			memcpy( vItem, v, kv);
		}
		else
		{
			memcpy( vItem, v, maxlen);
		}
	}
}


typedef struct __dict_item
{
	struct __dict_item * Next;
	
	char key[128];
	char value[256];
} __dict_item_t;


typedef struct __dict
{
	__dict_item_t items[25];
	int count;
} __dict_t;


char * ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}


char * rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}


char * trim(char *s)
{
    return rtrim(ltrim(s)); 
}



int __dpdk_env__read_cmd_output( char * output, char * command)
{
	FILE *fp;
	char line[256];
	memset( line, 0, sizeof(line));
	
	/* Open the command for reading. */
	fp = popen( command, "r");
	if (fp == NULL) 
	{
		printf("Failed to run command\n" );
		return 0;
	}
	
	int pos = 0;
	int slen = 0;
	
	/* Read the output a line at a time - output it. */
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		//printf("--%s--%ld\n", line, strlen(line));
		
		slen = strlen(line);
		memcpy( &output[pos], line, slen);
		pos += slen;

		// memcpy( &output[pos], "\n", 1);
		// pos++;
	}
	

	/* close */
	pclose(fp);	

	return 1;
}



int __dpdk_env__get_driver_info( __dict_t * dict, char * busid)
{
	char cmd[128];
	memset( cmd, 0, sizeof(cmd));
	
	if( memcmp( busid, "0000.", 5) == 0) {
		sprintf( cmd, "lspci -vmmks %s", &busid[5]);
	} else {
		sprintf( cmd, "lspci -vmmks %s", busid);
	}

	
	char output[1024];
	memset( output, 0, sizeof(output));
	__dpdk_env__read_cmd_output( output, cmd);
	
	
	//__dict_t dict;
	dict->count = 0;
	
	//printf( "-%s-\n", output);
	int i = 0;
	int begin_pos = 0;
	int end_pos = 0;
	char temp[256];
	
	char * save_ptr = NULL;
	char * key_ptr = NULL;
	
	while(i < 1024)
	{
		if( output[i] == '\n')
		{
			end_pos = i;
			
			if( end_pos-begin_pos > 0)
			{
				key_ptr = NULL;
				save_ptr = NULL;
				
				memset( temp, 0, sizeof(temp));
				memcpy( temp, &output[begin_pos], end_pos-begin_pos);
				//printf( "-%s-\n", temp);
				
				key_ptr = strtok_r( temp, ":", &save_ptr);
				
				if( key_ptr) 
				{
					//printf( "key=%s\nval=%s\n", key_ptr, save_ptr);
				
					if( strlen( key_ptr) > 0) 
					{
						memset( &dict->items[dict->count], 0, sizeof(__dict_item_t));
						memcpy( dict->items[dict->count].key, key_ptr, strlen(key_ptr));
						
						if( save_ptr) {
							if( strlen(save_ptr) > 0) {
								memcpy( dict->items[dict->count].value, save_ptr, strlen(save_ptr));
							}
						}
						dict->count++;
					}
				}
			
			}
			
			begin_pos 	= (i + 1);
		}
		i++;
	}

	// i = 0;
	// while( i < dict.count)
	// {
		// printf( "%d %s: -%s-\n", i, trim(dict.items[i].key), trim(dict.items[i].value));
		// i++;
	// }
	
	return 1;
}



int __dpdk_env__is_dpdk_driver_findkey( __dict_t * dict, char * findkey)
{
	int i = 0;
	char * key = NULL;
	
	while( i < dict->count)
	{
		key = trim( dict->items[i].key);
		
		if( strlen( key) == strlen( findkey) && strlen( key) > 0)
		{
			if( memcmp( findkey, key, strlen( key)) == 0)
			{
				return i;
			}
		}	
		i++;
	}
	
	return -1;
}


int __dpdk_env__is_dpdk_driver_attached( char * driverName, char * busid)
{
	__dict_t dict;
	__dpdk_env__get_driver_info( &dict, busid);

	int index = __dpdk_env__is_dpdk_driver_findkey( &dict, "Driver");

	if( index >= 0)
	{
		printf("UPF-EAL:  bus=%s current driver=%s  |%d\n", busid,  trim(dict.items[index].value), __LINE__);
		
		if( memcmp( driverName, trim(dict.items[index].value), 8) == 0)
		{
			return 1;
		}
	}
	else
	{
		printf("UPF-EAL:  unable to find driver info for bus=%s |%d\n", busid, __LINE__);
		exit(0);
	}

	return 0;
}




// macaddr = json_string_value( jMAC);
// macaddrlen = json_string_length( jMAC);
												
// export LD_LIBRARY_PATH=.
// gcc -g3 env_setup.c -o env_setup -I../jansson-2.13/src/ -lpthread -ljansson
int main( int argc, char* argv[])
{
	if( argc < 2)
	{
		printf("config file not provided\n syntax: %s ./env_setup.json\n", argv[0]);
		exit(0);
	}
	
	uid_t euid = geteuid();
	
	if( euid != 0)
	{
		printf("The program is NOT running with sudo or root privileges.\n");
		exit(0);
	}
	
	__esetup = (esetup_t*)malloc(sizeof(esetup_t));
	memset( __esetup, 0, sizeof( esetup_t));
	
	
	json_t * j_str = NULL;
	int j_str_len = 0;
	
	json_error_t error;
	memset( &error, 0, sizeof( json_error_t));
	json_t * json_config = json_load_file( argv[1], 0, &error);	

	if(!json_config)
	{
		printf("error in json %s file at line=%d column=%d position=%d\n", argv[1], error.line, error.column, error.position);
		exit(0);
	}
	
	__esetup->HugePagesGB 	= dpdk_config__get_int( json_config, "HugePagesGB", 12, 12);
	__esetup->NumaNodes		= dpdk_config__get_int( json_config, "NumaNodes", 	1 , 1);
	app_config__set_str( json_config, "DriverName", __esetup->DriverName, 100);
	
	json_t * eth_ary = json_object_get( json_config, "Eths");
	
	if( eth_ary)
	{
		if( json_is_array( eth_ary))
		{
			int i 	= 0;
			int sz 	= json_array_size( eth_ary);
		
			for( i = 0; i < sz; i++)
			{
				json_t * eth_item = json_array_get( eth_ary, i);
				
				if( eth_item)
				{
					app_config__set_str( eth_item, "Name", 	__esetup->Eths[__esetup->EthsCount].Eth, 32);
					app_config__set_str( eth_item, "Bus", 	__esetup->Eths[__esetup->EthsCount].Bus, 32);
					__esetup->EthsCount++;
				}
			}
		}
	}
	
	
	if( strcmp( __esetup->DriverName, "vfio-pci") == 0)
	{
		printf( "vfio-pci driver\n");

		char folder[1024];
		int sts = 0 ;

		memset( folder, 0, sizeof(folder));
		strcpy( folder, "sudo modprobe vfio-pci");
		sts = system( folder);
		printf( "[%s] - command status = %d\n", folder, sts);

		memset( folder, 0, sizeof(folder));
		strcpy( folder, "modprobe vfio enable_unsafe_noiommu_mode=1");
		sts = system( folder);
		printf( "[%s] - command status = %d\n", folder, sts);

		memset( folder, 0, sizeof(folder));
		strcpy( folder, "echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode");
		sts = system( folder);
		printf( "[%s] - command status = %d\n", folder, sts);
	}
	
	
	{
		struct stat sb;

		if (!(stat( "/dev/hugepages/", &sb) == 0 && S_ISDIR(sb.st_mode))) 
		{
			if( mkdir( "/dev/hugepages/", S_IRWXU | S_IRWXG | S_IRWXO) == -1)
			{
				printf("hugepages: creating hugepages folder :- \"%s\" failed., run with sudo  \n", "/dev/hugepages/");
				exit(0);
			}
			else
			{
				printf("hugepages: creating hugepages folder \"%s\" success \n", "/dev/hugepages/");
			}		
		}
	}
	
	
	{
		char * command = "mount -t hugetlbfs nodev /dev/hugepages/";
		printf("mount: %s\n", command);
		
		int sts = system( command);
		printf( "[%s] - command status=%d\n", command, sts);
	}
	
	
	{
		int i = 0;
		for( i = 0; i < __esetup->NumaNodes; i++)
		{
			char folder[1024];
			memset( folder, 0, sizeof(folder));
			
			sprintf( folder, "echo %d > /sys/devices/system/node/node%d/hugepages/hugepages-1048576kB/nr_hugepages", __esetup->HugePagesGB, i);
		
			int sts = system( folder);
			printf( "[%s] - command status = %d\n", folder, sts);
		}
	}
	
	
	{
		int i = 0;
		for( i = 0; i < __esetup->EthsCount; i++)
		{
			if( __dpdk_env__is_dpdk_driver_attached( __esetup->DriverName, __esetup->Eths[i].Bus) == 0)
			{
				int sts = 0 ;
				char folder[1024];

				memset( folder, 0, sizeof(folder));
				sprintf( folder, "sudo ifconfig %s down", __esetup->Eths[i].Eth);
				sts = system( folder);
				printf( "[%s] - command status = %d |%d\n", folder, sts, __LINE__);
				printf("UPF-EAL:  turned eth=%s down\n", __esetup->Eths[i].Eth);

				memset( folder, 0, sizeof(folder));
				sprintf( folder, "./dpdk-devbind.py --bind=%s %s", __esetup->DriverName, __esetup->Eths[i].Bus);
				sts = system( folder);
				printf( "[%s] - command status = %d  |%d\n", folder, sts, __LINE__);
				//  errno=%d|%s  , errno, strerror(errno)

				printf("UPF-EAL:  dpdk-bind command executed for eth=%s  bus_info=%s  |%d\n", __esetup->Eths[i].Eth, __esetup->Eths[i].Bus, __LINE__);
			}
		}
	}
	
	return 0;
}