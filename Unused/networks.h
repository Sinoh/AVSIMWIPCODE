#ifndef __NETWORKS_H__
#define __NETWORKS_H__


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>

#define PORT 4001
#define BACKLOG 10
#define MAXBUF 1400
#define DEBUG_FLAG 0

#define TIME_IS_NULL 1
#define TIME_IS_NOT_NULL 2

#define FLAG1 1
#define FLAG2 2
#define FLAG3 3
#define FLAG4 4
#define FLAG5 5
#define FLAG6 6
#define FLAG7 7
#define FLAG8 8
#define FLAG9 9
#define FLAG10 10
#define FLAG11 11
#define FLAG12 12
#define FLAG13 13

#define CHATHEDAERSIZE 3
#define MAXNAMELEN 100
#define MAXPACKETLEN 200
#define POLL_SET_SIZE 10
#define POLL_WAIT_FOREVER -1
#define PAYLOADSIZE 1024
#define PORTNUMBER 4001
#define MAXCARS 2

struct Node{
    int socketNumber;
    int carNumber;
    int carPosX;
    int carPosY;
    int carPosZ;
    int carSpeed;

    struct Node *next;
};

struct LinkedList{
    int listSize;
    int simSocket;
    struct Node *root;
};

struct Car {
    int socketNumber;
    int carNumber;
    char payload[PAYLOADSIZE];
    struct Car *nextCar;
};

struct CarBuffer {
    int bufferSize;
    struct Car *buffer;
};

void printBytes(char *PDU);
int tcpServerSetup(int portNumber);
int tcpAccept(int server_socket, int debugFlag);
int tcpClientSetup(char * serverName, char * port, int debugFlag);
void safeSend(int socketNumber, char *buffer, int bufferSize);
int safeRecv(int socketNumber, char *buffer, int flag);
void * srealloc(void *ptr, size_t size);
void * sCalloc(size_t nmemb, size_t size);

void setupPollSet();
void addToPollSet(int socketNumber);
void removeFromPollSet(int socketNumber);
int pollCall(int timeInMilliSeconds);

void addNewClient(int mainServerSocket);
void removeClient(int clientSocket, struct CarBuffer *carBuffer);
void recvFromClient(int clientSocket, struct CarBuffer *list);
void processSockets(int mainServerSocket, struct LinkedList *list);
void handlePacket(struct CarBuffer *carBuffer, int socketNumber, char *payload);

void initCar(struct CarBuffer *carBuffer, int socketNumber, int carNumberm, char *payload);
struct CarBuffer *initCarBuffer();
void updateCar(struct Car *carList, int socketNumber, char *payload);
int getCarPayload(struct Car *carList, int carNumber, char *payload);
int removeCar(struct CarBuffer *carBuffer, int carNumber);
struct Car *findCar(struct CarBuffer *carBuffer, int socketNumber);
struct Car *addCar(struct Car *carList, struct Car * car);

void printCars(struct Car *car);
void printCarList(struct CarBuffer *list);

void *initServer(void *input);
struct CarBuffer *startSwitch();
#endif
