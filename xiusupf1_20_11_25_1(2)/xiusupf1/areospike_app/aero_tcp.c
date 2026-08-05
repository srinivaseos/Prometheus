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

#include "app_aero.h"
#include "aero_message.h"
#include "aero_tcp.h"
#include "app_stack.h"


app_ep_stack__tcp_stack_t * app_tcp_stack = NULL;


app_ep_stack__tcp_buffer_t * app_ep_stack__allocate_tcp_buffer( app_ep_stack__tcp_server_t * tcp_server)
{
	app_ep_stack__tcp_buffer_t * buffer = NULL;
	
	pthread_mutex_lock( &tcp_server->bLock);

	buffer = tcp_server->bHead;
	
	if( buffer)
	{
		tcp_server->bHead = buffer->Next;
		buffer->Next = NULL;
		buffer->Length = 0;
		tcp_server->bAvailable--;
	}
	
	pthread_mutex_unlock( &tcp_server->bLock);
	return buffer;
}


void app_ep_stack__release_tcp_buffer2( app_ep_stack__tcp_buffer_t * tcp_buffer);

void app_ep_stack__release_tcp_buffer( app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	//printf("tcp_buffer=%p, tcp_buffer->client=%p tcp_server=%p\n", tcp_buffer, tcp_buffer->client, tcp_buffer->client->tcp_server);
	
	if(!tcp_buffer->client->tcp_server)
	{
		app_ep_stack__release_tcp_buffer2( tcp_buffer);
		return;
	}
	
	pthread_mutex_lock( &tcp_buffer->client->tcp_server->bLock);
	
	if(!tcp_buffer->client->tcp_server->bHead) 
	{
		tcp_buffer->client->tcp_server->bHead = tcp_buffer->client->tcp_server->bCurrent = tcp_buffer;
	}
	else
	{
		tcp_buffer->client->tcp_server->bCurrent->Next = tcp_buffer;
		tcp_buffer->client->tcp_server->bCurrent = tcp_buffer;
	}
	
	tcp_buffer->client->tcp_server->bAvailable--;
	
	pthread_mutex_unlock( &tcp_buffer->client->tcp_server->bLock);
}


app_ep_stack__tcp_buffer_t * app_ep_stack__allocate_tcp_buffer2( app_ep_stack__tcp_client_t * tcp_client)
{
	app_ep_stack__tcp_buffer_t * buffer = NULL;
	
	pthread_mutex_lock( &tcp_client->bLock);

	buffer = tcp_client->bHead;
	
	if( buffer)
	{
		tcp_client->bHead = buffer->Next;
		buffer->Next = NULL;
		buffer->Length = 0;
		buffer->client = tcp_client;
		tcp_client->bAvailable--;
	}
	
	pthread_mutex_unlock( &tcp_client->bLock);
	return buffer;
}


void app_ep_stack__release_tcp_buffer2( app_ep_stack__tcp_buffer_t * tcp_buffer)
{
	pthread_mutex_lock( &tcp_buffer->client->bLock);
	
	if(!tcp_buffer->client->bHead) 
	{
		tcp_buffer->client->bHead = tcp_buffer->client->bCurrent = tcp_buffer;
	}
	else
	{
		tcp_buffer->client->bCurrent->Next = tcp_buffer;
		tcp_buffer->client->bCurrent = tcp_buffer;
	}
	
	tcp_buffer->client->bAvailable--;
	
	pthread_mutex_unlock( &tcp_buffer->client->bLock);
}

void app_ep_stack__tcp_close_client2( app_ep_stack__tcp_client_t * client)
{
	if( client->isActive == 1)
	{
		client->isActive = 0;
		close( client->fd);
		shutdown( client->fd, 2);
		
		app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Server: Connection closed with fd=%d  %s|%s|%d", 
					client->fd, __FILE__, __FUNCTION__, __LINE__);
		
		client->fd = 0;
	}
}

void app_ep_stack__tcp_close_client( app_ep_stack__tcp_client_t * client)
{
	if( client->isActive == 1)
	{
		client->isActive = 0;
		close( client->fd);
		shutdown( client->fd, 2);
		
		app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Server: Connection closed with fd=%d  %s|%s|%d", 
					client->fd, __FILE__, __FUNCTION__, __LINE__);
		
		client->fd = 0;
	}
}

int app_ep_stack__tcp_read( app_ep_stack__tcp_client_t * client, app_ep_stack__tcp_buffer_t * tcp_buffer, int ReadLength)
{
	//ECONNREFUSED, 
	//printf( "fd=%d ReadLength=%d %s|%d\n", client->fd, ReadLength,  __FILE__, __LINE__);
	
	int iReceivedSize = recv( client->fd, &tcp_buffer->buffer[tcp_buffer->Length], ReadLength, 0 );
	int iReceivedChunkSize = 0;
	int ierrno = errno;

	//printf( "ReadLength=%d iReceivedSize=%d errno=%d  %s|%d\n", ReadLength, iReceivedSize, errno, __FILE__, __LINE__);
	
	if( iReceivedSize > 0)
	{
		tcp_buffer->Length += iReceivedSize;
	}
	
	//if( iReceivedSize <= 0 || iReceivedSize == 0 && errno > 0) //ECONNREFUSED
	if( iReceivedSize <= 0 && errno > 0) //ECONNREFUSED
	{
		app_ep_stack__tcp_close_client( client);
		return -1;
	}
	
	if( iReceivedSize == 0)
	{
		return 0;
	}
	
	while( iReceivedSize < ReadLength)
	{
		iReceivedChunkSize = recv( client->fd, &tcp_buffer->buffer[tcp_buffer->Length], ReadLength - iReceivedSize, 0);

		if( iReceivedSize <= 0 && errno > 0)
		{
			app_ep_stack__tcp_close_client( client);
			return -1;
		}

		if( iReceivedChunkSize == 0)
		{
			return 0;
		}
	
		iReceivedSize += iReceivedChunkSize;
		tcp_buffer->Length += iReceivedChunkSize;
	}
	
	return tcp_buffer->Length;
}




int app_ep_stack__send_message( app_ep_stack__tcp_client_t * client, uint8_t * buffer, int len)
{
	if( client->fd > 0)
	{
		int iSentBytes = send( client->fd, buffer, len, 0);
		
		if( iSentBytes < 0 && errno == EPIPE)
		{
			app_ep_stack__tcp_close_client( client);
		} 
		
		return iSentBytes;
	}
	return -1;
}


void * app_ep_stack__tcp_client( void * args)
{
	app_ep_stack__tcp_client_t * client = (app_ep_stack__tcp_client_t *)args;
	int iReceivedSize = 0;
	int iReceivedChunkSize = 0;
	fd_set rfds;
	struct timeval tv;
	int selectFlag = 0;
	int breakOuter = 0;
	
	app_ep_stack__tcp_buffer_t * tcp_buffer = NULL;
	
	
	while(1)
	{
		//printf("waiting %p\n", client);
		sem_wait( &client->semp);
		//printf("resumed %p\n", client);

		app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Client: Starting Client Read Theaad with fd=%d  %s|%s|%d", 
					client->fd, __FILE__, __FUNCTION__, __LINE__);		
		
		while(client->isActive)
		{
			breakOuter = 0;
			
			FD_ZERO( &rfds);
			FD_SET( client->fd, &rfds);
			memset( (char *)&tv, 0, sizeof(tv));

			tv.tv_usec = 3;
			tv.tv_sec = 3;

			selectFlag = select( client->fd + 1, &rfds, NULL, NULL, &tv);

			
			//printf("selectFlag=%d %s|%d\n", selectFlag, __FILE__, __LINE__);
			
			
			if(selectFlag < 0 )
			{
				if(client->isActive == 0)
				{
					break;
				}
				continue;
			}

			if(selectFlag == 0)
			{
				if(client->isActive == 0)
				{
					break;
				}
			}			
		
			if(selectFlag > 0)
			{
				if(FD_ISSET( client->fd, &rfds))
				{
					//printf("fd=%d ReadType=%d errno=%d   %s|%d\n", client->fd, client->tcp_server->ReadType, errno, __FILE__, __LINE__);
					
					// if( errno == ECONNREFUSED)
					// {
						// app_ep_stack__tcp_close_client( client);
						// continue;
					// }
					
					
					
					
					tcp_buffer = app_ep_stack__allocate_tcp_buffer( client->tcp_server);
					
					if(!tcp_buffer)
					{
						sleep(1);
						tcp_buffer = app_ep_stack__allocate_tcp_buffer( client->tcp_server);
						
						if(!tcp_buffer)
						{
							//printf( "tcp buffer allocation failed  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
							exit(0);
						}
					}
					tcp_buffer->client = client;

					
					//printf("tcp_buffer=%p ReadType=%d   %s|%d\n", tcp_buffer, client->tcp_server->ReadType, __FILE__, __LINE__);
					
					
					if( client->tcp_server->ReadType == APP_EP_STACK__TCP_READ_TYPE__FIXED_LENGTH)
					{
						iReceivedSize = recv( client->fd, tcp_buffer->buffer, client->tcp_server->ReadLength, 0 );
						
						if( iReceivedSize < 0)
						{
							app_ep_stack__release_tcp_buffer( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							continue;
						}
						
						while( iReceivedSize < client->tcp_server->ReadLength)
						{
							iReceivedChunkSize = recv( client->fd, &tcp_buffer->buffer[iReceivedSize], client->tcp_server->ReadLength - iReceivedSize, 0);
							
							if( iReceivedChunkSize < 0)
							{
								app_ep_stack__release_tcp_buffer( tcp_buffer);
								app_ep_stack__tcp_close_client( client);
								breakOuter = 1;
								break;
							}							
							
							iReceivedSize += iReceivedChunkSize;
						}
						
						if( breakOuter == 1 )
						{
							app_ep_stack__release_tcp_buffer( tcp_buffer);
							continue;
						}
						
						if( tcp_buffer->Length == 0)
						{
							app_ep_stack__release_tcp_buffer( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							continue;
						}

						if( client->isActive == 0)
						{
							continue;
						}
						
						tcp_buffer->Length = iReceivedSize;
					}
					else if( client->tcp_server->ReadType == APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK)
					{
						//printf("breakOuter=%d  %s|%d\n", breakOuter, __FILE__, __LINE__);
						
						breakOuter = client->tcp_server->tcp_read_cb( client, tcp_buffer);
						
						// TODO:
						// int bL = tcp_buffer->Length;
						//printf("breakOuter=%d Length=%d  %s|%d\n", breakOuter, tcp_buffer->Length, __FILE__, __LINE__);
						
						if( breakOuter <= 0 && tcp_buffer->Length < 12)
						{
							app_ep_stack__release_tcp_buffer( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							//printf( "isActive=%d\n", client->isActive);
							continue;
						}
						
						// if( breakOuter <= 0)
						// {
							// app_ep_stack__release_tcp_buffer( tcp_buffer);
							// continue;
						// }
						
						// if( tcp_buffer->Length < 12)
						// {
							// app_ep_stack__release_tcp_buffer( tcp_buffer);
							// app_ep_stack__tcp_close_client( client);
							// continue;
						// }
						
					}
					
					
					//printf("isActive=%d  %s|%d\n", client->isActive, __FILE__, __LINE__);
					
					if( client->isActive == 0)
					{
						continue;
					}
					
					tcp_buffer->client = client;
					
					
					
					
					if( tcp_buffer->Length >= 12)
					{
						app_queue__enquee( app_tcp_stack->q, (uint8_t *)tcp_buffer, NULL);
					}
					// else
					// {
						// //printf("enquee condition failed=%d %s|%d\n", tcp_buffer->Length, __FILE__, __LINE__);
					// }
				}
			}
		
		}
	}
	
	return NULL;
}


app_ep_stack__tcp_client_t * app_ep_stack__find_or_create_client( app_ep_stack__tcp_server_t * tcp_server)
{
	pthread_mutex_lock( &tcp_server->cLock);
	
	app_ep_stack__tcp_client_t * client = tcp_server->cHead;
	
	while(client)
	{
		if( client->isActive == 0)
		{
			pthread_mutex_unlock( &tcp_server->cLock);
			return client;
		}
		
		client = client->Next;
	}
	
	client = (app_ep_stack__tcp_client_t*)malloc(sizeof(app_ep_stack__tcp_client_t));
	memset( client, 0, sizeof(app_ep_stack__tcp_client_t));
	
	client->isActive 			= 0;
	client->fd 					= 0;
	client->ReceivedMessages 	= 0;
	client->SentMessages 		= 0;
	sem_init( &client->semp, 0, 0);
	
	
	if(!tcp_server->cHead)
	{
		tcp_server->cHead = tcp_server->cCurrent = client;
	}		
	else
	{
		tcp_server->cCurrent->Next = client;
		tcp_server->cCurrent = client;
	}
	
	tcp_server->cCount++;
	
	int iRet;
	pthread_t s_pthread_id;

	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);	
	
	
	iRet = pthread_create( &s_pthread_id, &attr, app_ep_stack__tcp_client, (void *)client);

	pthread_mutex_unlock( &tcp_server->cLock);
	return client;
}


void app_ep_stack__tcp_stack_set_logger( app_logger_t * appLogger)
{
	app_tcp_stack->appLogger = appLogger;
}


void * app_ep_stack__server( void * args)
{
	app_ep_stack__tcp_server_t * tcp_server = (app_ep_stack__tcp_server_t*)args;

	
	int serverSocket, newSocket, clilen;
	struct sockaddr_in serverAddr, cli_addr;
	
	serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	if(serverSocket == -1)
	{
		printf("Server Socket Creation Failed (Host Process)  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
		exit(1);
	}

	int yes = 1;

	if( setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		printf("setsockopt SO_REUSEADDR failed\n");
		exit(1);
	}

	memset( &(serverAddr.sin_zero), '\0', 8);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons( tcp_server->Port);
	serverAddr.sin_addr.s_addr = inet_addr( tcp_server->IP);
	
	if( bind( serverSocket, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) != 0)
	{
		printf("Server Bind Failed\n");
		exit(1);
	}

	//printf("started tcp server\n");


	app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Server Started at IP=%s Port=%d  %s|%s|%d", 
		tcp_server->IP, tcp_server->Port, __FILE__, __FUNCTION__, __LINE__);

	
	
	if( listen( serverSocket, 5) == 0)
	{
		app_ep_stack__tcp_client_t * client = NULL;
		
		while(1)
		{
			clilen = sizeof(cli_addr);
			newSocket = accept( serverSocket, (struct sockaddr *) &cli_addr, &clilen);
			
			if( newSocket != -1)
			{
				client = app_ep_stack__find_or_create_client( tcp_server);
				client->fd = newSocket;
				client->isActive = 1;
				client->tcp_server = tcp_server;
				
				app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Server: Accepted New SocketFD=%d client=%p  %s|%s|%d", 
					newSocket, client, __FILE__, __FUNCTION__, __LINE__);
				
				sem_post( &client->semp);
			}
		}
	}
	
	return NULL;
}


void app_ep_stack__start_server( char * ip, int port, int iReadType, int ReadLength, app_ep_stack__tcp_read_cb tcp_read_cb, int poolSize, int bufferSize)
{
	int iRet;
	pthread_t s_pthread_id;

	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);

	app_ep_stack__tcp_server_t * tcp_server = (app_ep_stack__tcp_server_t*)malloc(sizeof(app_ep_stack__tcp_server_t));
	memset( tcp_server, 0, sizeof(app_ep_stack__tcp_server_t));
	tcp_server->Next = NULL;
	
	tcp_server->Port = port;
	memcpy( tcp_server->IP, ip, strlen(ip));
	pthread_mutex_init( &tcp_server->cLock, NULL);
	
	tcp_server->cHead 			= NULL;
	tcp_server->cCurrent 		= NULL;
	tcp_server->ReadType 		= iReadType;
	tcp_server->ReadLength 		= ReadLength;
	tcp_server->tcp_read_cb 	= tcp_read_cb;
	
	uint8_t * buffer = (uint8_t *)malloc( (sizeof(app_ep_stack__tcp_buffer_t) * poolSize) + (poolSize * bufferSize));

	if( buffer)
	{
		int jx = 0;
		app_ep_stack__tcp_buffer_t * bitem = NULL;
		
		for( jx= 0; jx < poolSize; jx++)
		{
			
			bitem 	= (app_ep_stack__tcp_buffer_t*)buffer;
			buffer += sizeof(app_ep_stack__tcp_buffer_t);
			
			bitem->buffer 	= buffer;
			buffer += bufferSize;


			bitem->Next 	= NULL;
			bitem->Length 	= 0;
			
			if(!tcp_server->bHead)
			{
				tcp_server->bHead = tcp_server->bCurrent = bitem;
			}
			else
			{
				tcp_server->bCurrent->Next = bitem;
				tcp_server->bCurrent = bitem;
			}
			
			tcp_server->bAvailable++;
			tcp_server->bTotal++;
		}
	}
	
	
	
	if(!app_tcp_stack->tcp_server_head)
	{
		app_tcp_stack->tcp_server_head = app_tcp_stack->tcp_server_current = tcp_server;
	}
	else
	{
		app_tcp_stack->tcp_server_current->Next = tcp_server;
		app_tcp_stack->tcp_server_current = tcp_server;
	}
	app_tcp_stack->tcp_server_count++;

	iRet = pthread_create( &s_pthread_id, &attr, app_ep_stack__server, (void *)tcp_server);
	//printf("launched tcp server thread\n");
}


typedef struct aero_message_buffer aero_message_buffer_t;

aero_message_buffer_t * app_aero_message__decode( app_ep_stack__tcp_buffer_t * tcp_buffer);
void app_aero_message__execute( aero_message_buffer_t * msg_buf);

int app_ep_stack__tcp_start_client_isconnected( app_ep_stack__tcp_client_t * client)
{
	return client->isActive;
}


void * app_ep_stack__tcp_start_client_thread( void * args)
{
	app_ep_stack__tcp_client_t * client = (app_ep_stack__tcp_client_t*) args;
	
	int sockfd;
	struct sockaddr_in their_paddr;
	
	fd_set rfds;
	struct timeval tv;
	int selectFlag = 0;
	int iReceivedSize = 0;
	app_ep_stack__tcp_buffer_t * tcp_buffer = NULL;
	int iReceivedChunkSize = 0;
	int breakLoop = 0;
	
			
	while(1)
	{
		while(1)
		{
			memset( &their_paddr, 0, sizeof(struct sockaddr_in));

			their_paddr.sin_family = PF_INET;
			their_paddr.sin_addr.s_addr = inet_addr( client->IP );
			their_paddr.sin_port = htons( client->Port );

			memset(&(their_paddr.sin_zero), '\0', 8);

			if ((sockfd = socket( PF_INET, SOCK_STREAM, 0)) == -1)
			{
				//CLog( 0, LOG_LEVEL_CRITICAL, __FUNCTION__, __LINE__, "Socket creation failed for Peer[%d]", *iPeerThreadIndex);
				//return -1;
				sleep(1);
				continue;
			}

			if (connect(sockfd, (struct sockaddr *)&their_paddr,sizeof(struct sockaddr)) == -1)
			{
				close( sockfd);
				//CLog(  0, LOG_LEVEL_CRITICAL, __FUNCTION__, __LINE__, "Connect failed for Peer[%d]", *iPeerThreadIndex);
				//return -1;
				sleep(1);
				continue;
			}
	
			client->fd = sockfd;
			client->isActive = 1;
			
			app_logger__log( app_tcp_stack->appLogger, NULL, APP_LOG__LEVEL_CRITICAL, "TCP Server: Connected to Server with fd=%d  %s|%s|%d", 
					client->fd, __FILE__, __FUNCTION__, __LINE__);
		
			break;
		}
		
		while( client->isActive)
		{
			breakLoop = 0;
			FD_ZERO( &rfds);
			FD_SET( client->fd, &rfds);
			memset( (char *)&tv, 0, sizeof(tv));
			
			tv.tv_usec = 0;
			tv.tv_sec = 3;

			selectFlag = select( client->fd + 1, &rfds, NULL, NULL, &tv);
			
			//printf( "selectFlag=%d\n", selectFlag);

			if( selectFlag < 0 )
			{
				if( client->isActive == 0)
				{
					break;
				}
				continue;
			}
			
			if( selectFlag == 0 )
			{
				if( client->isActive == 0)
				{
					break;
				}
			}
			
			if( selectFlag > 0)
			{
				if(FD_ISSET( client->fd, &rfds))
				{
					//printf("fd=%d     %s|%s|%d\n", client->fd, __FILE__, __FUNCTION__, __LINE__);
					
					tcp_buffer = app_ep_stack__allocate_tcp_buffer2( client);
					
					if(!tcp_buffer)
					{
						sleep(1);
						tcp_buffer = app_ep_stack__allocate_tcp_buffer2( client);
						
						if(!tcp_buffer)
						{
							printf( "tcp buffer allocation failed  %s|%s|%d\n", __FILE__, __FUNCTION__, __LINE__);
							exit(0);
						}
					}
					
					//printf("ReadType=%d tcp_buffer=%p    %s|%s|%d\n", client->ReadType, tcp_buffer, __FILE__, __FUNCTION__, __LINE__);
					
					if( client->ReadType == APP_EP_STACK__TCP_READ_TYPE__FIXED_LENGTH)
					{
						iReceivedSize = recv( client->fd, tcp_buffer->buffer, client->ReadLength, 0 );
						
						if( iReceivedSize < 0)
						{
							app_ep_stack__release_tcp_buffer2( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							//printf("client closed %s|%d\n", __FILE__, __LINE__);
							continue;
						}
	
						while( iReceivedSize < client->ReadLength)
						{
							iReceivedChunkSize = recv( client->fd, &tcp_buffer->buffer[iReceivedSize], client->ReadLength - iReceivedSize, 0);
							
							if( iReceivedChunkSize < 0)
							{
								breakLoop = 1;
								app_ep_stack__release_tcp_buffer2( tcp_buffer);
								app_ep_stack__tcp_close_client( client);
								//printf("client closed %s|%d\n", __FILE__, __LINE__);
								break;
							}
							
							iReceivedSize += iReceivedChunkSize;
						}
						
						if( breakLoop == 1)
						{
							app_ep_stack__release_tcp_buffer2( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							//printf("client closed %s|%d\n", __FILE__, __LINE__);
							continue;
						}
						
						if( tcp_buffer->Length == 0)
						{
							app_ep_stack__release_tcp_buffer2( tcp_buffer);
							app_ep_stack__tcp_close_client( client);
							//printf("client closed %s|%d\n", __FILE__, __LINE__);
							continue;
						}
						
						if( client->isActive == 0)
						{
							continue;
						}
						
						tcp_buffer->Length = iReceivedSize;
					}
					else if( client->ReadType == APP_EP_STACK__TCP_READ_TYPE__FUNC_CALLBACK)
					{
						//printf("breakLoop=%d len=%d    %s|%s|%d\n", breakLoop, tcp_buffer->Length, __FILE__, __FUNCTION__, __LINE__);
						
						breakLoop = client->tcp_read_cb( client, tcp_buffer);
						
						//TODO:
						// int bLength = tcp_buffer->Length;

						
						//printf("breakLoop=%d len=%d    %s|%s|%d\n", breakLoop, tcp_buffer->Length, __FILE__, __FUNCTION__, __LINE__);
						
						if( breakLoop <= 0 || tcp_buffer->Length < 12)
						{
							app_ep_stack__release_tcp_buffer2( tcp_buffer);
							app_ep_stack__tcp_close_client2( client);
							
							//printf("client closed %s|%d\n", __FILE__, __LINE__);
							continue;
						}
					}
					
					// if( client->isActive == 0)
					// {
						// continue;
					// }
					
					//printf("isActive=%d client=%p  pushing message %s|%d\n", client->isActive, tcp_buffer->client, __FILE__, __LINE__);
					tcp_buffer->client = client;
					app_queue__enquee( app_tcp_stack->q, (uint8_t *)tcp_buffer, NULL);
					tcp_buffer = NULL;
				}
			}
			
		}
	}
}


app_ep_stack__tcp_client_t * app_ep_stack__tcp_start_client( char * ip, int port, int iReadType, int ReadLength, app_ep_stack__tcp_read_cb tcp_read_cb, int poolSize, int bufferSize)
{
	app_ep_stack__tcp_client_t * client = (app_ep_stack__tcp_client_t*)malloc(sizeof(app_ep_stack__tcp_client_t));
	memset( client, 0, sizeof(app_ep_stack__tcp_client_t));
	

	client->Port = port;
	memcpy( client->IP, ip, strlen(ip));

	
	client->ReadType 		= iReadType;
	client->ReadLength 		= ReadLength;
	client->tcp_read_cb 	= tcp_read_cb;
	
	uint8_t * buffer = (uint8_t *)malloc( (sizeof(app_ep_stack__tcp_buffer_t) * poolSize) + (poolSize * bufferSize));

	if( buffer)
	{
		int jx = 0;
		app_ep_stack__tcp_buffer_t * bitem = NULL;
		
		for( jx= 0; jx < poolSize; jx++)
		{
			
			bitem 	= (app_ep_stack__tcp_buffer_t*)buffer;
			buffer += sizeof(app_ep_stack__tcp_buffer_t);
			
			bitem->buffer 	= buffer;
			buffer += bufferSize;


			bitem->Next 	= NULL;
			bitem->Length 	= 0;
			
			if(!client->bHead)
			{
				client->bHead = client->bCurrent = bitem;
			}
			else
			{
				client->bCurrent->Next = bitem;
				client->bCurrent = bitem;
			}
			
			client->bAvailable++;
			client->bTotal++;
		}
	}
	
	int iRet;
	pthread_t s_pthread_id;

	pthread_attr_t attr;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED);
	
	iRet = pthread_create( &s_pthread_id, &attr, app_ep_stack__tcp_start_client_thread, (void *)client);
	//printf("launched tcp client thread\n");
	
	return client;
}


void app_ep_stack__signal_handler( int signum)
{
	printf("Caught signal %d ...\n", signum);
   //exit(1);
}


void app_ep_stack__tcp_stack_init( fp_queue_call_back cb)
{
	if(!app_tcp_stack)
	{
		signal( SIGPIPE, app_ep_stack__signal_handler);

		app_tcp_stack = (app_ep_stack__tcp_stack_t*)malloc(sizeof(app_ep_stack__tcp_stack_t));
		memset( app_tcp_stack, 0, sizeof(app_ep_stack__tcp_stack_t));
	
		app_tcp_stack->tcp_server_head 		= NULL;
		app_tcp_stack->tcp_server_current 	= NULL;
		app_tcp_stack->tcp_server_count 	= 0;
		
		app_tcp_stack->q = app_queue__create( "tcpq", 4000, 1, cb);
	}
}

