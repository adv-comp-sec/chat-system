/*
	Date:
	Project:
	By:
	Description:
*/

#define _REENTRANT

#include "../../common/inc/common.h"
#include "../inc/chat-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>	// for socket programming
#include <netdb.h>		// for gethostbyname
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>		// to close socket
#include <pthread.h>
#include <time.h>		// to calculate current time

static int numClients = 0;	// to keep track of total clients connected
static int keepRunning = 1;

ConnectedClients connected_client[MAX_CLIENTS];
Threads client_thread[MAX_CLIENTS];
Sockets sockets;

// function prototypes
void *client_handler(void* client_socket);
void broadcast_message(int sender, char* messageToSend);
int removeClientFromArray(int sender);
void formatMessage(char* message, int whichClient, int isSender);
void getCurrentTime(char* whatTime);
void shutdown_signal(int client_socket);

int main()
{	
	
	//int server_socket;
	struct sockaddr_in server, client;
	
	//int clientSockets[MAX_CLIENTS];	// data structure holds all sockets
	
	
	// create socket
	sockets.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(sockets.server_socket == -1)
	{
		printf("ERROR: %s\n", hstrerror(errno));
		return 0;
	}
	
	// initialize socket
	memset (&server, 0, sizeof (server));
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = htonl (INADDR_ANY);
  	server.sin_port = htons (PORT);
  	
  	// bind socket
  	if(bind(sockets.server_socket,(struct sockaddr *)&server, sizeof(server)) < 0)
  	{
  		printf("ERROR: %s\n", hstrerror(errno));
		return 0;
  	}
  	
  	// listen for connections
  	if (listen (sockets.server_socket, MAX_CLIENTS) < 0) 
  	{
		printf("ERROR: %s\n", hstrerror(errno));
		return ERROR;
  	}
    
  	while(keepRunning)
  	{	
  		printf("keep running: %d\n", keepRunning);
  		if (numClients == 9)
  		{
  			break;
  		}
  		
	  	int client_len = sizeof(struct sockaddr_in);
	  	int new_connection = accept(sockets.server_socket, (struct sockaddr *)&client, (socklen_t*)&client_len);	// accept new client
	  	if(new_connection < 0)
	  	{
	  		printf("ERROR: %s\n", hstrerror(errno));
	  		close(sockets.server_socket);
	  		keepRunning = 0; // break loop when accept fails
	  	}
	  	else
	  	{
	  		if (keepRunning == 0)
	  		{
	  			("exiting server...\n");
	  			break;
	  		}
	  		
	  		sockets.clientSockets[numClients] = new_connection;	// add connected client to list
	  		printf("socket # %d:\n", sockets.clientSockets[numClients]);

			numClients++;		// increment the number of clients connected	
			
			if (keepRunning != 0)
			{
				if (pthread_create(&client_thread[numClients].client_threads, NULL, client_handler, (void *)&new_connection))
				{
					printf("ERROR host ID: %s\n", strerror(errno));
					return 0;	// change this to constant
				}	
			}
			
	  	}
  	}

	/*printf("Stop listening");
  	int numOfThreads = sizeof(client_thread[numClients].client_threads) / sizeof(client_thread[0].client_threads);
	
	printf("number of threads: %d\n", numOfThreads);
	
	// wait for all clients to finish
	for(int i = 0; i < numOfThreads; i++)
	{
		pthread_join(client_thread[numClients].client_threads, NULL);
	}

	printf("Closing the server");
	
	// close socket
	//close(server_socket);*/
	
	return 0;
}

void shutdown_signal(int client_socket)
{
	printf("in watchdog timer\n");
	
	printf("closing socket\n");
	close(client_socket);	// close client connection
	printf("closed 1\n");
	//fflush(stdout);
	//pthread_exit(NULL);
	//printf("closed2\n");
				
	// keepRunning is being set to 0 in removeClientFromArray function
	if((keepRunning == 0) && (numClients == 0))
	{
		// join all threads
		int numOfThreads = sizeof(client_thread[numClients].client_threads) / sizeof(client_thread[0].client_threads);
	
		printf("number of threads: %d\n", numOfThreads);
		
		// wait for all clients to finish
		for(int i = 0; i < numOfThreads; i++)
		{
			pthread_join(client_thread[numClients].client_threads, NULL);
		}
		
		printf("closing server\n");
		// close server socket
		close(sockets.server_socket);
		printf("closed\n");
		
		exit(-1);
	}
}

// send and receive messages from clients
void *client_handler(void* client_socket)
{
	int client_sock = *(int *)client_socket;
	char buffer[1024] = {"\0"};
	char* returnVal;
	int readMsg;
	int i = 0;
	
	while (numClients > 0)
	{
		while ((readMsg = read(client_sock, buffer, 1024)) > 0)
		{
			printf("received from client: %s\n", buffer);
			
			if (strcmp(buffer, ">>bye<<") == 0)
		  	{	
		  		// remove client from array
				int client_removed = removeClientFromArray(client_sock);
		  		if (client_removed == ERROR)
				{
					printf("ERROR: Client not found.\n");
				}
				
				shutdown_signal(client_sock);	// close socket, join threads and shutdown server
				/*close(client_sock);	// close client connection
				fflush(stdout);
				pthread_exit(&numClients);*/
				
				break;
			}
			// check if it's the first message sent -- "FIRST|-userID|IPAddress"
			// client will send their IP address and userID as the first message, once the connection is established
			returnVal = strstr(buffer, "FIRST|");
		  	if(returnVal)
		  	{
		  		connected_client[numClients - 1].client_socket = client_sock;	// add client socket to structure
		  		
		  		// first message is sent by client to register it's userID and IP address
		  		returnVal = strstr(buffer, "r");
			  	returnVal++;
			  	i = 0;
			  	while(*returnVal != '|')
			  	{
			  		connected_client[numClients - 1].userID[i] = *returnVal;
			  		returnVal++;
			  		i++;
			  	}
			  	connected_client[numClients - 1].userID[i] = '\0';	// null terminate the string
			  	
			  	returnVal = strrchr(buffer, '|');
			  	returnVal++;
			  	i = 0;
			  	while(*returnVal != '\0')	// while pointer hasn't reached end of string
			  	{
			  		connected_client[numClients - 1].clientIP[i] = *returnVal;
			  		returnVal++;
			  		i++;
			  	}
			  	connected_client[numClients - 1].clientIP[i] = '\0';	// null terminate the string			  	
		  	}
		  	else
		  	{
		  		// send a reponse to all clients excpet the sender
				// format the message first before invoking this function
				printf("broadcasting...\n");
				broadcast_message(client_sock, buffer);
		  	}
			
			memset(buffer, 0, 1024);
		}
	}
	
	//shutdown(client_sock, SHUT_RDWR);
	
}

// send message to all clients
void broadcast_message(int sender, char* messageToSend)
{
	char sendMsg[1024];
	
	int senderIndx;
	
	int msgLength;
	
	int numPacket;
	
	char packet[2][1024];
	char fstPacket[1024];
	char sndPacket[1024];
	
	strcpy(sendMsg, messageToSend);	// keep track of the original message sent by sender
	
	// loop to get sender's IP, userID, message to create the formatted message
	for(int i = 0; i < numClients; i++)
	{
		if(connected_client[i].client_socket == sender)
		{			
			//formatMessage(messageToSend, i, 1);
			senderIndx = i;	// get sender's index
			
			// send a formatted reponse to sender
			//write(connected_client[i].client_socket, messageToSend , strlen(messageToSend));	
		}
	}
	
	// check message size and break in chunks of 40 characters	
	msgLength = strlen(sendMsg);

	// if message is less than / equal to 40 characters
	if (msgLength <= 40)
	{
		numPacket = 1;
		memset(packet[0], 0, 1024);
		memset(packet[1], 0, 1024);

		memcpy(packet[0], sendMsg, 40);

		// send packets to sender
		formatMessage(packet[0], senderIndx, 1);
		write(connected_client[senderIndx].client_socket, packet[0] , strlen(packet[0]));
		printf("sent: %s\n",packet[0]);

		memcpy(packet[0], sendMsg, 40);		// packet[] has been overwritten, reset to the original message
		for (int j = 0; j < numPacket; j++)
		{
			formatMessage(packet[j], senderIndx, 0);

			// send formatted message to each client
			for(int i = 0; i < numClients; i++)
			{
				if(connected_client[i].client_socket != sender)
				{			
					// send a formatted reponse to all clients except the sender
					write(connected_client[i].client_socket, packet[j] , strlen(packet[j]));
				}
			}

			printf("sent: %s\n",packet[j]);
		}
	}
	// else if message is greater than 40 characters
	else
	{
		numPacket = 2;
		
		// clear the buffer
		memset(packet[0], 0, 1024);
		memset(packet[1], 0, 1024);

		if (sendMsg[40] == 32)
		{
			// check if char at index 40 is an empty space
			//break message at index 40
			memcpy(packet[0], sendMsg, 40); 		// first 40 characters
			printf("1st packet: %s\n", packet[0]);
			memcpy(packet[1], sendMsg+41, 40); 		// second part of the message
			printf("2nd packet: %s\n", packet[1]);
		}
		else
		{		
			//break next to the last empty space before 40 characters

			memcpy(packet[0], sendMsg, 40); // first 40 characters
			printf("1st packet: %s\n", packet[0]);				

			int i = 0;
			int index = 0;
			
			// loop the string until the end
			while(packet[0][i] != '\0')
		  	{
		  		if(packet[0][i] == 32)  
				{
		  			index = i;		// get the index of the last empty space in the string
		 		}
		 		i++;
			}
			
			memset(packet[0], 0, 1024);				// clear the buffer
			memcpy(packet[0], sendMsg, index+1); 	// first part of the message
			printf("1st packet: %s\n", packet[0]);	


			memcpy(packet[1], sendMsg+index+1, 60); // second part of the message
			printf("2nd packet: %s\n", packet[1]);
			
		}

		// TODO still need to diplay the formatted for the client who sent the message after breaking it
		for (int j = 0; j < numPacket; j++)
		{
			strcpy(sendMsg, packet[j]);		// to prevent overwriting the broken up message
			// send packets to sender
			formatMessage(sendMsg, senderIndx, 1);
			write(connected_client[senderIndx].client_socket, sendMsg, strlen(sendMsg));

			int len = strlen(sendMsg);
			printf("sent: %s\n", sendMsg);
			printf("length: %d\n", len);
		}
		
		// send packets to all other clients
		for (int j = 0; j < numPacket; j++)
		{
			strcpy(sendMsg, packet[j]);		// to prevent overwriting the broken up message
			formatMessage(sendMsg, senderIndx, 0);

			// send formatted message to each client
			for(int i = 0; i < numClients; i++)
			{					
				if(connected_client[i].client_socket != sender)
				{			
					// send a formatted reponse to all clients except the sender
					write(connected_client[i].client_socket, sendMsg , strlen(sendMsg));
				}
			}

			int len = strlen(sendMsg);
			printf("sent: %s\n", sendMsg);
			printf("length: %d\n", len);
		}
	}
}


void formatMessage(char* message, int whichClient, int isSender)
{
	int length = 0;
	char senderMessage[1024];
	char whatTime[TIME_LEN];
	getCurrentTime(whatTime);
	strcpy(senderMessage, message);	// keep track of the message
	
	// POSITION 1 - 15 				= IP
	// POSITION 16, 24, 27, 28 		= SPACES
	// POSITION 17 					= [
	// POSITION 18 - 22 			= USERID
	// POSITION 23 					= ]
	// POSITION 25, 26 				= >> OR <<
	// POSITION 28 - 68 			= MESSAGE (UP TO 40 chars)
	// POSITION 69 - 78 			= (HH:MM:SS)
	
	memset(message, 0, 1024);

	length = strlen(connected_client[whichClient].clientIP);		// get size of IP
	if(length <= 15)
	{
		while(length < 15)
		{
			strcat(connected_client[whichClient].clientIP, " ");
			length++;
		}
		strcpy(message, connected_client[whichClient].clientIP);	// get sender's IP
		printf("IP: %s\n", message);
	}
	
	strcat(message, " [");	// bracket
	
	length = strlen(connected_client[whichClient].userID);			// get size of the userID
	printf("connected_client[whichClient].userID: %s\n", connected_client[whichClient].userID);
	if(length <= 5)
	{
		while(length < 5)
		{
			strcat(connected_client[whichClient].userID, " ");
			length++;
		}
		strcat(message, connected_client[whichClient].userID);		// get sender's ID
		printf("ID: %s\n", message);
	}
	
	strcat(message, "] ");	// bracket
	
	if(isSender)
	{
		strcat(message, ">> ");
	}
	else
	{
		strcat(message, "<< ");
	}
	
	length = strlen(senderMessage);	// get size of the message
	printf("size: %d\n", length);
	
	// TODO message should be maximum of 40 characters
	if(length <= 40)	// if message length is less than 40
	{
		while(length < 40)
		{
			// append white space to message array
			strcat(senderMessage, " ");
			length++;
		}
		strcat(message, senderMessage);	
		printf("message: %s\n", message);
		printf("message: %s\n", senderMessage);
	}
	else
	{
		// no need to append blank space after the message
		strcat(message, senderMessage);	
		printf("message: %s\n", message);
	}
	
	// get time
	strcat(message, " (");
	strcat(message, whatTime);
	strcat(message, ")");
}

void getCurrentTime(char* whatTime)
{
	time_t currentTime;
	struct tm *timeIs;
	
	time(&currentTime);
	timeIs = localtime(&currentTime);
	
	strftime(whatTime, 9, "%H:%M:%S", timeIs);
}

// remove sender from array and update the number of clients
int removeClientFromArray(int sender)
{
	// flag to determine if the client was found
	int client_found = FALSE; // false	
		
	// get size of array connected_client
	int size = sizeof(connected_client) / sizeof(*connected_client);
	int position;
	int client_removed;
		
	// search sender in the array connected_client
	for (int i = 0; i < size; i++)
	{
		if (connected_client[i].client_socket == sender)
		{
			client_found = TRUE; // true
			position = i;			// get index of element
			break;
		}	
	}

	// if client is in the list, delete it
	// if not, return error
	if (client_found == TRUE)
	{
		for (int i = position; i < size; i++)
		{
			connected_client[position] = connected_client[position + 1];
		}
		
		numClients--;	// update number of clients
		printf("client removed. number of clients: %d\n", numClients);
		
		
		client_removed = TRUE;	
	}
	else
	{
		client_removed = ERROR;	// sender not in the list
	}
	if (numClients == 0)
	{
		printf("keep running is being set to 0\n");
		keepRunning = 0;	// this will prevent while loop in main to accept more connections
	}
	//shutdown_signal();	// check if server needs to shutdown

	return client_removed;
}

