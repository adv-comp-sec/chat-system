/*
	Date:
	Project:
	By:
	Description:
*/

#include "../../common/inc/common.h"
#include "../inc/chat-server.h"
#include <stdio.h>
#include <sys/socket.h>	// for socket programming
#include <netdb.h>		// for gethostbyname
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>		// to close socket
#include <pthread.h>
#include <time.h>		// to calculate current time

static int numClients = 0;	// to keep track of total clients connected
ConnectedClients connected_client[MAX_CLIENTS];

// function prototypes
void *client_handler(void* client_socket);
void broadcast_message(int sender, char* messageToSend);
int removeClientFromArray(int sender);

int main()
{	
	int server_socket;
	struct sockaddr_in server, client;
	
	pthread_t client_threads[MAX_CLIENTS];
	//ClientThreads client_thread;
	
	// create socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket == -1)
	{
		printf("ERROR: %s\n", hstrerror(errno));
	}
	
	// initialize socket
	memset (&server, 0, sizeof (server));
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = htonl (INADDR_ANY);
  	server.sin_port = htons (PORT);
  	
  	// bind socket
  	if(bind(server_socket,(struct sockaddr *)&server, sizeof(server)) < 0)
  	{
  		printf("ERROR: %s\n", hstrerror(errno));
  	}
  	
  	// listen for connections
  	if (listen (server_socket, MAX_CLIENTS) < 0) 
  	{
		printf("ERROR: %s\n", hstrerror(errno));
		return ERROR;
  	}
  	
  	while(numClients < MAX_CLIENTS)
  	{
  		// accept incoming connections - upto 10 clients
  		
	  	int client_len = sizeof(struct sockaddr_in);
	  	int new_connection = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&client_len);	// accept new client
	  	if(new_connection < 0)
	  	{
	  		printf("ERROR: %s\n", hstrerror(errno));
	  		close(server_socket);
	  	}
	  	else
	  	{
	  		connected_client[numClients].client_socket = new_connection;	// add connected client to list
	  		printf("%d:\n", connected_client[numClients].client_socket);

			if (pthread_create(&client_threads[numClients], NULL, client_handler, (void *)&new_connection))
			{
				printf("ERROR host ID: %s\n", strerror(errno));
				return 0;
			}
			
			numClients++;		// increment the number of clients connected
	  	}
  	}
  	
  	int numOfThreads = sizeof(client_threads) / sizeof(client_threads[0]);

	// wait for all clients to finish
	for(int i = 0; i < numOfThreads; i++)
	{
		pthread_join(client_threads[i], NULL);
	}
	
	
	// close socket
	close(server_socket);
	
	return 0;
}

// send and receive messages from clients
void *client_handler(void* client_socket)
{
	int client_sock = *(int *)client_socket;
	char buffer[1024];
	char* returnVal;
	int readMsg;
	int i = 0;
	
	while ((readMsg = read(client_sock, buffer, 1024)) > 0)
	{
		printf("received from client: %s\n", buffer);
		
		// check if it's the first message sent -- "-userID|IPAddress"
		// client will send their IP address and userID as the first message, once the connection is established
		returnVal = strstr(buffer, "FIRST|");
	  	if(returnVal)
	  	{
	  		// first message is sent by client to register it's userID and IP address
	  		returnVal = strstr(buffer, "r");
		  	returnVal++;
		  	i = 0;
		  	while(*returnVal != '|')
		  	{
		  		connected_client[numClients].userID[i] = *returnVal;
		  		returnVal++;
		  		i++;
		  	}
		  	connected_client[numClients].userID[i] = '\0';	// null terminate the string
		  	
		  	printf("userID: %s\n", connected_client[numClients].userID);
		  	
		  	returnVal = strrchr(buffer, '|');
		  	returnVal++;
		  	i = 0;
		  	while(*returnVal != '\0')	// while pointer hasn't reached end of string
		  	{
		  		connected_client[numClients].clientIP[i] = *returnVal;
		  		returnVal++;
		  		i++;
		  	}
		  	connected_client[numClients].clientIP[i] = '\0';	// null terminate the string
		  	printf("IP: %s\n", connected_client[numClients].clientIP);
	  	}
	  	else if (strcmp(buffer, "<<bye>>") == 0){
	  		
	  		// TODO -- remove client from array
	  		removeClientFromArray(client_sock);
			printf("Client disconnected.\n");
		}
	  	else
	  	{
	  		// send a reponse to all clients excpet the sender
			// format the message first before invoking this function
			broadcast_message(client_sock, buffer);
	  	}
		
		memset(buffer, 0, 1024);
	}
	
	pthread_exit(NULL);
}

// send message to all clients except sender
void broadcast_message(int sender, char* messageToSend)
{
	char sendMsg[1024];
	char whatTime[TIME_LEN];
	
	printf("clients: %d\n", numClients);
	
	for(int i = 0; i < numClients; i++)
	{
		if(connected_client[i].client_socket != sender)
		{	
			// send a reponse to all clients excpet the sender
			write(connected_client[i].client_socket, messageToSend , strlen(messageToSend));
		}
	}
}



// TODO
int removeClientFromArray(int sender)
{
	
}
