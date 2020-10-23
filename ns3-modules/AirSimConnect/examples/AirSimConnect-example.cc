/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/AirSimConnect-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

int 
main (int argc, char *argv[])
{
  int i;
  NS_LOG_UNCOND ("Scratch Simulator");

  // Initialize a buffer to store packets
  char payload[PAYLOADSIZE];

  // Start the server and return a buffer of info
  // For now you need to also start fake_clients manually
	struct CarBuffer *buffer = initialize();
  
  // Easiest way to use this is to have a loop and put what you want to do in that loop
  // Or else this NS3 script will end to fast.
  // The server itself will end (buffer->flag will be set to 0), when there is 10 seconds
  // connections to the server
  while(buffer->flag){

    // This is an example of just getting the packet and printing it back onto the terminal
    // You can do whatever you want once you get the pacet into the payload buffer
    sleep(1);
    for (i = 0; i < buffer->bufferSize; i++) {
        if (buffer->buffer != NULL) {
            memset(payload, 0, PAYLOADSIZE);

            // HOW TO USE:
            // Passing in the cars, which is "buffer->buffer", then pass in the car number "i"
            // Then pass in your buffer that the packet will be copied to
            getCarPayload(buffer->buffer, i, payload);
            printf("Car %i payload: %s\n", i, payload);
        }
    }
    
  }

  Simulator::Run ();
  Simulator::Destroy ();
}


