#ifndef __SWITCH_H__
#define __SWITCH_H__



void handlePacket(char* buffer, struct LinkedList *list, int clientsocket);
void handleRequests(char* buffer, struct LinkedList *list, int clientSocket);
void addNewClient(int mainServerSocket);
void removeClient(int clientSocket);
void recvFromClient(int clientSocket, struct LinkedList *list);
void processSockets(int mainServerSocket, struct LinkedList *list);
void *initialize(void *input);
struct LinkedList *startSwitch();

struct LinkedList *initLinkedList();
void addNode(struct LinkedList *list, int carNumber, int socketNumber);
int getListLength(struct Node *node);
void removeNode(struct LinkedList *list, int socketNumber);
struct Node *removeNodeHelper(struct Node *node, int socketNumber);
struct Node *findCar(struct LinkedList *list, int carNumber);
void printLinkedList(struct LinkedList *list);
void printNode(struct Node *node);
void printSocketNumber(struct Node* node);
void handleRequests(char* buffer, struct LinkedList *list, int clientSocket);


#endif