/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef FAKE_CONNECTION_H
#define FAKE_CONNECTION_H

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

#define CHATHEDAERSIZE 3
#define POLL_SET_SIZE 10
#define POLL_WAIT_FOREVER -1
#define PAYLOADSIZE 254
#define PORTNUMBER 4001
#define COUNT_LIMIT 60

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
void removeClient(int clientSocket);
void recvFromClient(int clientSocket);
void processSockets(int mainServerSocket);

#endif 
