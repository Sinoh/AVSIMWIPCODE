/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef AIRSIMCONNECT_HELPER_H
#define AIRSIMCONNECT_HELPER_H

#include "ns3/AirSimConnect.h"
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

namespace ns3 {


#define PORT 4001
#define BACKLOG 10
#define MAXBUF 1400
#define DEBUG_FLAG 0

#define TIME_IS_NULL 1
#define TIME_IS_NOT_NULL 2

#define CHATHEDAERSIZE 3
#define MAXNAMELEN 100
#define MAXPACKETLEN 200
#define POLL_SET_SIZE 10
#define POLL_WAIT_FOREVER -1
#define PAYLOADSIZE 1024
#define PORTNUMBER 4001
#define MAXCARS 2
#define COUNT_LIMIT 10
struct Car {
    int socketNumber;
    int carNumber;
    char payload[PAYLOADSIZE];
    struct Car *nextCar;
};

struct CarBuffer {
    int flag;
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
void processSockets(int mainServerSocket, struct CarBuffer *carBuffer);
void handlePacket(struct CarBuffer *carBuffer, int socketNumber, char *payload);

struct Car *initCar(struct CarBuffer *carBuffer, int socketNumber, int carNumber, char *payload);
struct CarBuffer *initCarBuffer();
void updateCar(struct Car *carList, int socketNumber, char *payload);
int getCarPayload(struct Car *carList, int carNumber, char *payload);
struct Car *removeCar(int SocketNumber, struct Car *car, struct CarBuffer *carBuffer);
struct Car *findCar(struct CarBuffer *carBuffer, int socketNumber);
struct Car *addCar(struct Car *carList, struct Car * car);

void printCars(struct Car *car);
void printCarList(struct CarBuffer *list);

void *initServer(void *input);

}

#endif /* AIRSIMCONNECT_HELPER_H */


// #ifdef __cplusplus 
// extern "C" {
// #endif

// #include "networks.h"


// #ifdef __cplusplus 
// }
// #endif

