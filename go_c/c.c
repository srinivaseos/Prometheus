#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#pragma pack(1)
typedef struct nprofile
{
   char name[20];
   uint32_t ipv4;
   int id;
   int port;
}nprofile_t;

nprofile_t * nrf =NULL;


nprofile_t* create_profile() {
    nprofile_t *nrf = (nprofile_t*)malloc(sizeof(nprofile_t));
    if (nrf == NULL) {
        return NULL;
    }

    strcpy(nrf->name, "srinivas");

    const char *ip_str = "192.6.0.1";  // String representation of the IP address
    // Convert the string IP address to network byte order (binary form)
    if (inet_pton(AF_INET, ip_str, &(nrf->ipv4)) != 1) {
        printf("Invalid IP address format\n");
        free(nrf);
        return NULL;
    }

    nrf->id = 5;
    nrf->port = 9099;

    return nrf;
}

void free_profile(nprofile_t *nrf) {
    free(nrf);
}



/*int main()
{
	int sts =create_profile();
	printf("sts=%d\n",sts);

	free_profile(nrf);
}*/










/*void Madd(uint8_t * h)
{
}

int main()
{
	nrf = (nprofile_t*)malloc(sizeof(nprofile_t));
	strcpy(nrf->name,"srinivas");
	//strcpy(nrf->ipv4,"192.6.0.1");
	const char *ip_str = "192.6.0.1";  // String representation of the IP address

    	// Convert the string IP address to network byte order (binary form)
    	if (inet_pton(AF_INET, ip_str,&(nrf->ipv4)) == 1) {
        printf("IPv4 Address as uint32_t: %u\n", nrf->ipv4);
    	} else {
        printf("Invalid IP address format\n");
    	}		

	nrf->id=5;
	nrf->port=9099;
	uint8_t *byt = (uint8_t*)nrf;
	Madd(byt);	
	printf("name=%s ipv4=%u id=%d port=%d\n",nrf->name,nrf->ipv4,nrf->id,nrf->port);
	printf("\n");

	free(nrf);
	return 0;
}*/
