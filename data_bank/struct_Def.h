#include <sys/socket.h> 

typedef struct  allocate_udp_server
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr; 

}allocate_udp_server_t;


struct bank_details
{
    char branch[20],name[30];
    int ifsc_code,phno,acc_no,balance;

    struct bank_details *next;
}; 