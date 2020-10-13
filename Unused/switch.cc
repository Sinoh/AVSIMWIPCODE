// Server side C/C++ program to demonstrate Socket programming 


#include "switch.h"
#include "networks.h"
#include <pthread.h>

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
// Helper Functions 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlePacket(char* buffer, struct LinkedList *list, int clientsocket){

	//printf("BufferLen: %i from socket: %i\n", strlen(buffer), clientsocket);
	//printf("Message Recv: %s\n", buffer);
	if (strlen(buffer) < 6){
		printf("Handling Request\n");
		handleRequests(buffer, list, clientsocket);
	}else{
		if (findCar(list, atoi(&buffer[12])) == 0){
			addNode(list, atoi(&buffer[12]), clientsocket);
		}
		updateNode(list, atoi(&buffer[12]), atoi(&buffer[42]), atoi(&buffer[58]), 0, 0);
	}

}

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

int main(int argc, char const *argv[]) 
{   
	struct LinkedList *list = initLinkedList();
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, initialize, list); 
	
	while(1){
		// pthread_join(thread_id, NULL);
		if (list->root != NULL){
			printPosX(list->root);
		}
		
	}
    return 0; 
    
} 

