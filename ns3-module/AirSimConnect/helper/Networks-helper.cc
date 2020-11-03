/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "Networks-helper.h"

namespace ns3 {

// For DEBUG purposes only. Prints every byte in a buffer in Hex, Num, and Char
void printBytes(char *PDU){
	int i;
	uint16_t length;
	memcpy(&length, PDU, 2);

	printf("\tHex\t|\tNum\t|\tChar\n");
	for (i = 0; i < ntohs(length); i++){
		printf("%i:\t%x\t|\t%i\t|\t%c\n", i, PDU[i] & 0xffff, PDU[i], PDU[i]);
		if (i > 200){
			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions (Networks) Don't Touch
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function creates the server socket.  The function
// returns the server socket number and prints the port
// number to the screen.

int tcpServerSetup(int portNumber)
{
	int server_socket= 0;
	struct sockaddr_in6 server;      /* socket address for local side  */
	socklen_t len = sizeof(server);  /* length of local address        */

	/* create the tcp socket  */
	server_socket = socket(AF_INET6, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		perror("socket call");
		exit(1);
	}

	// setup the information to name the socket
	server.sin6_family= AF_INET6;
	server.sin6_addr = in6addr_any;   //wild card machine address
	server.sin6_port= htons(portNumber);

	// bind the name to the socket  (name the socket)
	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind call");
		exit(-1);
	}

	//get the port number and print it out
	if (getsockname(server_socket, (struct sockaddr*)&server, &len) < 0)
	{
		perror("getsockname call");
		exit(-1);
	}

	if (listen(server_socket, BACKLOG) < 0)
	{
		perror("listen call");
		exit(-1);
	}

	printf("Server Port Number %d \n", ntohs(server.sin6_port));

	return server_socket;
}

// This function waits for a client to ask for services.  It returns
// the client socket number.

int tcpAccept(int server_socket, int debugFlag)
{
	struct sockaddr_in6 clientInfo;
	int clientInfoSize = sizeof(clientInfo);
	int client_socket= 0;

	if ((client_socket = accept(server_socket, (struct sockaddr*) &clientInfo, (socklen_t *) &clientInfoSize)) < 0)
	{
		perror("accept call error");
		exit(-1);
	}

	return(client_socket);
}

// This is used by the client to connect to a server using TCP

int tcpClientSetup(char * serverName, char * port, int debugFlag)
{
	int socket_num;
	//uint8_t * ipAddress = NULL;
	struct sockaddr_in6 server;

	// create the socket
	if ((socket_num = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("socket call");
		exit(-1);
	}

	// setup the server structure
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(atoi(port));

	if(connect(socket_num, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		perror("connect call");
		exit(-1);
	}

	return socket_num;
}

// Sends message to client and check if sending caused any errors

void safeSend(int socketNumber, char *buffer, int bufferSize)
{
	int sent = 0;
	sent =  send(socketNumber, buffer, bufferSize, MSG_WAITALL);
	if (sent < 0){
		puts("Sending Error");
		perror("send call");
		exit(-1);
	}
}

// Tries to receive from client and check for any recv caused and errors
// Also copies received data into a give buffer

int safeRecv(int socketNumber, char *buffer, int flag){
	//uint16_t bufferLength = 0;

	if (recv(socketNumber, buffer, 1024, flag) < 0){
		perror("recv call");
		return 0;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions (Poll) Don't Touch
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Poll global variables
static struct pollfd * pollFileDescriptors;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;
static void growPollSet(int newSetSize);

void * srealloc(void *ptr, size_t size)
{
	void * returnValue = NULL;

	if ((returnValue = realloc(ptr, size)) == NULL)
	{
		printf("Error on realloc (tried for size: %d\n", (int) size);
		exit(-1);
	}

	return returnValue;
}

void * sCalloc(size_t nmemb, size_t size)
{
	void * returnValue = NULL;
	if ((returnValue = calloc(nmemb, size)) == NULL)
	{
		perror("calloc");
		exit(-1);
	}
	return returnValue;
}

// Poll functions (setup, add, remove, call)
void setupPollSet()
{
	currentPollSetSize = POLL_SET_SIZE;
	pollFileDescriptors = (struct pollfd *) sCalloc(POLL_SET_SIZE, sizeof(struct pollfd));
}

void addToPollSet(int socketNumber)
{

	if (socketNumber >= currentPollSetSize)
	{
		// needs to increase off of the biggest socket number since
		// the file desc. may grow with files open or sockets
		// so socketNumber could be much bigger than currentPollSetSize
		growPollSet(socketNumber + POLL_SET_SIZE);
	}

	if (socketNumber + 1 >= maxFileDescriptor)
	{
		maxFileDescriptor = socketNumber + 1;
	}

	pollFileDescriptors[socketNumber].fd = socketNumber;
	pollFileDescriptors[socketNumber].events = POLLIN;
}

void removeFromPollSet(int socketNumber)
{
	pollFileDescriptors[socketNumber].fd = 0;
	pollFileDescriptors[socketNumber].events = 0;
}

int pollCall(int timeInMilliSeconds)
{
	// returns the socket number if one is ready for read
	// returns -1 if timeout occurred
	// if timeInMilliSeconds == -1 blocks forever (until a socket ready)
	// (this -1 is a feature of poll)

	int i = 0;
	int returnValue = -1;
	int pollValue = 0;

	if ((pollValue = poll(pollFileDescriptors, maxFileDescriptor, timeInMilliSeconds)) < 0)
	{
		perror("pollCall");
		exit(-1);
	}

	// check to see if timeout occurred (poll returned 0)
	if (pollValue > 0)
	{
		// see which socket is ready
		for (i = 0; i < maxFileDescriptor; i++)
		{
			//if(pollFileDescriptors[i].revents & (POLLIN|POLLHUP|POLLNVAL))
			//Could just check for some revents, but want to catch any of them
			//Otherwise, this could mask an error (eat the error condition)
			if(pollFileDescriptors[i].revents > 0)
			{
				//printf("for socket %d poll revents: %d\n", i, pollFileDescriptors[i].revents);
				returnValue = i;
				break;
			}
		}

	}

	// Ready socket # or -1 if timeout/none
	return returnValue;
}

static void growPollSet(int newSetSize)
{
	int i = 0;

	// just check to see if someone screwed up
	if (newSetSize <= currentPollSetSize)
	{
		printf("Error - current poll set size: %d newSetSize is not greater: %d\n",
			currentPollSetSize, newSetSize);
		exit(-1);
	}

	printf("Increasing poll set from: %d to %d\n", currentPollSetSize, newSetSize);
	pollFileDescriptors = (struct pollfd *) srealloc(pollFileDescriptors, newSetSize * sizeof(struct pollfd));

	// zero out the new poll set elements
	for (i = currentPollSetSize; i < newSetSize; i++)
	{
		pollFileDescriptors[i].fd = 0;
		pollFileDescriptors[i].events = 0;
	}

	currentPollSetSize = newSetSize;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS ABOVE THIS LINE SHOULD NOT BE TOUCHED IF YOU DO NOT KNOW WHAT YOU ARE DOING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions (Works for Now) WIP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This is used to update/init cars or send packets
// Not complete yet
void handlePacket(struct CarBuffer *carBuffer, int socketNumber, char *payload) {
	char buf[PAYLOADSIZE];
	struct Car* car;

	memset(buf, 1, 1);
	if (findCar(carBuffer, socketNumber) == 0) {
		car = initCar(carBuffer, socketNumber, atoi(&payload[12]), payload);
		carBuffer->buffer = addCar(carBuffer->buffer, car);
		carBuffer->bufferSize++;

	}else{
		updateCar(carBuffer->buffer, socketNumber, payload);
	}

	//safeSend(socketNumber, buf, 1);
}

// Part of the tcp server functionality. Just a wrapper function
void addNewClient(int mainServerSocket){
	int newClientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	printf("New Client socket: %i\n", newClientSocket);
	addToPollSet(newClientSocket);
}

// Part of the tcp server functionality. Just a wrapper function
void removeClient(int clientSocket){
	printf("Client on socket %d terminted\n", clientSocket);
	removeFromPollSet(clientSocket);
	close(clientSocket);
}

void recvFromClient(int clientSocket, struct CarBuffer *carBuffer) {
	char buf[PAYLOADSIZE];
	memset(&buf, 0, PAYLOADSIZE);

	if (safeRecv(clientSocket, buf, MSG_DONTWAIT) != 0){
		if (strlen(buf) != 0){
			handlePacket(carBuffer, clientSocket, (char *) buf);
		}else {
			removeClient(clientSocket);
			carBuffer->buffer = removeCar(clientSocket, carBuffer->buffer, carBuffer);
		}
	}
}

// Part of the tcp server functionality. Will also remove cars if no data is recv,///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void processSockets(int mainServerSocket, struct CarBuffer *carBuffer){

	int socketToProcess = 0;
	int counter = 0;

	addToPollSet(mainServerSocket);
	while (counter++ < COUNT_LIMIT){
		if ((socketToProcess = pollCall(1000)) != -1){
			if (socketToProcess == mainServerSocket){
				addNewClient(mainServerSocket);
			}else{
				recvFromClient(socketToProcess, carBuffer);
			}
			counter = 0;
		}
	}
	carBuffer->flag = 0;
}


// Used in ns3 to initialize this server
// The input argument needs to be a new CarBuffer Struct
void *initServer(void *input){
	int mainServerSocket = 0;   //socket descriptor for the server socket

	mainServerSocket = tcpServerSetup(PORTNUMBER);

	processSockets(mainServerSocket, (struct CarBuffer *)input);
	close(mainServerSocket);

	return NULL;
}

}
