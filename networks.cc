

#include "networks.h"

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
	uint8_t * ipAddress = NULL;
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
	uint16_t bufferLength = 0;

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
// Helper Functions (Structure) to hold clients (Completed)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct LinkedList *initLinkedList(){
	struct LinkedList *list = (struct LinkedList*)malloc(sizeof(struct LinkedList));
	list->listSize = 0;
	list->root = NULL;
	return list;
}

void addNode(struct LinkedList *list, int carNumber, int socketNumber){
	struct Node *newNode = (struct Node*)sCalloc(1, sizeof(struct Node));
	newNode->socketNumber = socketNumber;
	newNode->carNumber = carNumber;
	newNode->carSpeed = 0;
	newNode->carPosX = 0;
	newNode->carPosY = 0;
	newNode->carPosZ = 0;
	newNode->next = list->root;
	list->root = newNode;
	list->listSize++;
}

int getListLength(struct Node *node){
	if (node != NULL){
		return 1 + getListLength(node->next);
	}
	return 0;
}

void removeNode(struct LinkedList *list, int socketNumber){
	if (list->root != NULL){
		list->root = removeNodeHelper(list->root, socketNumber);
	}
	list->listSize = getListLength(list->root);
}

struct Node *removeNodeHelper(struct Node *node, int socketNumber){
	if (node->socketNumber == socketNumber){
		struct Node *tempNode = node->next;
		free(node);
		return tempNode;
	}else{
		if (node->next == NULL){
			return node;
		}else{
			node->next = removeNodeHelper(node->next, socketNumber);
			return node;
		}
	}
}

struct Node *findCar(struct LinkedList *list, int carNumber){
	struct Node *tempNode = list->root;
	while(tempNode != NULL){
		if (carNumber == tempNode->carNumber){
			return tempNode;
		}
		tempNode = tempNode->next;
	}
	return 0;
}

void printLinkedList(struct LinkedList *list){
	// For debugging purposes
	if (list->root != NULL){
		printNode(list->root);
		printSocketNumber(list->root);
	}
}

void printNode(struct Node *node){
	// Helper function for printLinkedList
	printf("%d -> ", node->carNumber);
	if (node->next == NULL){
		printf("NULL\n");
	}else{
		printNode(node->next);
	}
}

void printSocketNumber(struct Node* node){
	// Helper function for printLinkedList
	printf("%d -> ", node->socketNumber);
	if (node->next == NULL){
		printf("NULL\n");
	}else{
		printSocketNumber(node->next);
	}
}

void printPosX(struct Node* node){
	// Helper function for printLinkedList
	printf("%d -> ", node->carPosX);
	if (node->next == NULL){
		printf("NULL\n");
	}else{
		printPosX(node->next);
	}
}


void updateNode(LinkedList *list, int carNumber, int posX, int posY, int posZ, int speed){
	struct Node *car;
	if ((car = findCar(list, carNumber)) == 0){
		return;
	}

	struct Node *tempNode = list->root;
	while(tempNode != NULL){
		if (carNumber == tempNode->carNumber){
			tempNode->carPosX = posX;
			tempNode->carPosY = posY;
			tempNode->carPosZ = posZ;
			tempNode->carSpeed = speed;
		}
		tempNode = tempNode->next;
	}

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions (Works for Now) WIP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlePacket(char* buffer, struct LinkedList *list, int clientsocket){

	//printf("BufferLen: %i from socket: %i\n", strlen(buffer), clientsocket);
	//printf("Message Recv: %s\n", buffer);
	// if (strlen(buffer) < 6){
	// 	printf("Handling Request\n");
	// 	handleRequests(buffer, list, clientsocket);
	// }else{
	if (findCar(list, atoi(&buffer[12])) == 0){
		addNode(list, atoi(&buffer[12]), clientsocket);
	}
	updateNode(list, atoi(&buffer[12]), atoi(&buffer[42]), atoi(&buffer[58]), 0, 0);
	//}

}

// Not in use right now
/*
void handleRequests(char* buffer, struct LinkedList *list, int clientSocket){
	struct Node *car;
	char sendBuf[5];
	char data[1];
	int carNumber =  atoi(&buffer[2]);
	int flag = atoi(&buffer[3]);

	if ((car = findCar(list, carNumber)) == 0){
	}

	switch(flag)
	{
		case 0:
			list->simSocket = clientSocket;
		case 1:
			memcpy(data, &car->carSpeed, 1);
		case 2:
			memcpy(data, &car->carPosX, 1);
		case 3:
			memcpy(data, &car->carPosY, 1);
		case 4:
			memcpy(data, &car->carPosZ, 1);
		default:
			memset(data, 0, 1);
			flag = -1;
	}

	createPDU(sendBuf, carNumber, flag, data);
	safeSend(clientSocket, sendBuf, 5);


}
*/


void addNewClient(int mainServerSocket){
	int newClientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	printf("New Client socket: %i\n", newClientSocket);
	addToPollSet(newClientSocket);
}

void removeClient(int clientSocket){
	printf("Client on socket %d terminted\n", clientSocket);
	removeFromPollSet(clientSocket);
	close(clientSocket);
}

void recvFromClient(int clientSocket, struct LinkedList *list){
	char buf[MAXBUF];
	memset(&buf, 0, MAXBUF);
	if (safeRecv(clientSocket, buf, MSG_DONTWAIT) != 0){
		if (strlen(buf) != 0){
			handlePacket(buf, list, clientSocket);
		}else{
			printBytes(buf);
			printf("Removing Client\n");
			removeClient(clientSocket);
		}


	}
}

void processSockets(int mainServerSocket, struct LinkedList *list){

	int socketToProcess = 0;

	addToPollSet(mainServerSocket);
	while (1){
		if ((socketToProcess = pollCall(POLL_WAIT_FOREVER)) != -1){
			if (socketToProcess == mainServerSocket){
				addNewClient(mainServerSocket);
			}else{
				recvFromClient(socketToProcess, list);
			}
		}
	}
}

void *initialize(void *input){
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 4001;

    mainServerSocket = tcpServerSetup(portNumber);

	processSockets(mainServerSocket, (struct LinkedList *)input);
	close(mainServerSocket);

	return NULL;

}

struct LinkedList *startSwitch(){
	struct LinkedList *list = initLinkedList();
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, initialize, list);

	return list;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions (Car) WIP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
void createPDU(char *buffer, int carNumber, int flag, char *data){

	int bufferSize = 4;
	uint16_t PDUSize;
	u_int16_t formatedPDUSize;
	switch(flag){
		case 0:
			PDUSize = bufferSize;
			formatedPDUSize = htons(PDUSize);
		default:
			PDUSize = bufferSize + strlen(data);
			formatedPDUSize = htons(PDUSize);
	}

	memset(buffer, 0, bufferSize);
	memcpy(buffer, &formatedPDUSize, 2);
	memcpy(buffer + 2, &carNumber, 1);
	memcpy(buffer + 3, &flag, 1);
	memcpy(buffer + 4, data, strlen(data));
}

int connectToServer(){
	int socketNumber = -1;
	char buffer[4];
	char *serverName = "0.0.0.0";
	char *portNumber = "4001";


    puts("Attempt to Connect\n");
	socketNumber = tcpClientSetup(serverName, portNumber, 0);
    puts("Connected\n");
	createPDU(buffer, 99, 0, "");
	safeSend(socketNumber, buffer, 4);
    printf("SocketNumber: %i\n", socketNumber);
	return socketNumber;
}

int disconnectFromServer(int socketNumber){
	close(socketNumber);
}

int getCarSpeed(int carNumber, int socketNumber){
	char buffer[4];
	char recvBuffer[5];

	createPDU(buffer, carNumber, 1, "");
	safeSend(socketNumber, buffer, 4);
	sleep(1);
	safeRecv(socketNumber, recvBuffer, MSG_DONTWAIT);
	return atoi(&recvBuffer[4]);
}

int getCarPosX(int carNumber, int socketNumber){
	char buffer[4];
	char recvBuffer[5];

	createPDU(buffer, carNumber, 2, "");
	safeSend(socketNumber, buffer, 4);
    return 0;
}

int getCarPosY(int carNumber, int socketNumber){
	char buffer[4];
	createPDU(buffer, carNumber, 3, "");
	safeSend(socketNumber, buffer, 4);
	sleep(1);
}

int getCarPosZ(int carNumber, int socketNumber){

	char buffer[4];
	createPDU(buffer, carNumber, 4, "");
	safeSend(socketNumber, buffer, 4);
	sleep(1);
}

*/

int getCarSpeed(struct LinkedList *list, int carNumber){
	struct Node* car = findCar(list, carNumber);
	return car->carSpeed;
}

int getCarPosX(struct LinkedList *list, int carNumber){
	struct Node* car = findCar(list, carNumber);
	return car->carPosX;
}

int getCarPosY(struct LinkedList *list, int carNumber){
	struct Node* car = findCar(list, carNumber);
	return car->carPosY;
}

int getCarPosZ(struct LinkedList *list, int carNumber){
	struct Node* car = findCar(list, carNumber);
	return car->carPosZ;
}
