/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "AirSimConnect.h"

using namespace std;
namespace ns3 {

bool initialize()
{
    int i;
    struct CarBuffer *buffer = initCarBuffer();
    startSwitch(buffer);
    char payload[PAYLOADSIZE];
    while(buffer->flag){
        sleep(1);
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


