/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "AirSimConnect.h"

using namespace std;
namespace ns3 {

struct CarBuffer *initialize()
{
    pthread_t thread_id;
    struct CarBuffer *buffer = initCarBuffer();
    pthread_create(&thread_id, NULL, initServer, buffer);
    return buffer;
}


}


