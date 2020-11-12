/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef AIRSIMCONNECT_HELPER_H
#define AIRSIMCONNECT_HELPER_H

#include "ns3/AirSimConnect.h"
#include "Packet-helper.h"
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

#define PAYLOADSIZE 1024

struct Car {
    int socketNumber;
    int carNumber;
    char payload[PAYLOADSIZE];
    struct Car *nextCar;
    carData *data;
    // TO DO: Refactor code to include the new packet struct

    double getSpeed();

};

struct CarBuffer {
    int flag;
    int bufferSize;
    struct Car *buffer;
};

struct Car *initCar(struct CarBuffer *carBuffer, int socketNumber, int carNumber, char *payload);
struct CarBuffer *initCarBuffer();
void updateCar(struct Car *carList, int socketNumber, char *payload);
int getCarPayload(struct Car *carList, int carNumber, char *payload);
struct Car *removeCar(int SocketNumber, struct Car *car, struct CarBuffer *carBuffer);
struct Car *findCar(struct CarBuffer *carBuffer, int socketNumber);
struct Car *addCar(struct Car *carList, struct Car * car);
void printCars(struct Car *car);
void printCarList(struct CarBuffer *list);

}

#endif /* AIRSIMCONNECT_HELPER_H */


// #ifdef __cplusplus 
// extern "C" {
// #endif

// #include "networks.h"


// #ifdef __cplusplus 
// }
// #endif

