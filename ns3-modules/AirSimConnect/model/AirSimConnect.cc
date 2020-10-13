/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "AirSimConnect.h"

using namespace std;
namespace ns3 {

bool initialize()
{
    int i;
    struct CarBuffer *buffer = startSwitch();
    char payload[PAYLOADSIZE];
    while(1){
        sleep(1);
        printCarList(buffer);
        for (i = 0; i < buffer->bufferSize; i++) {
            if (buffer->buffer != NULL) {
                memset(payload, 0, PAYLOADSIZE);
                getCarPayload(buffer->buffer, i, payload);
                printf("Car %i payload: %s\n", i, payload);
            }
        }
    }
    return 0;
}


}


