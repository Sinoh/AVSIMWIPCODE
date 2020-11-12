/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PACKET_HELPER_H
#define PACKET_HELPER_H

#include "ns3/AirSimConnect.h"

namespace ns3 {

#define PAYLOADSIZE 1024
class carData{

    public:
        int sequenceNumber = 0;
        int packetFlag = 0;
        int Id = 0;
        int packetLength = 21;
        double speed = 0;
        double gear = 0;
        double posX = 0;
        double posY = 0;
        double posZ = 0;
        char *payload;
        void setData(int newSequenceNumber, int newPacketFlag, int newId, int newPacketLength, 
                    double newSpeed, double newGear, double newPosX, double newPosY, double newPosZ);
        int writePayload(char *buffer);
        void createCarPacket();
        void readCarPacket(char *packet);
        /*
        uint8_t *getPayload(){
            return this->payload;
        }
        int readSequenceNumber(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload, 4);
            return buffer;
        }
        int readPacketFlag(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 4, 1);
            return buffer;
        }
        int readPacketLength(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 5, 1);
            return buffer;
        }
        int readSpeed(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 6, 1);
            return buffer;
        }
        int readGear(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 7, 1);
            return buffer;
        }
        int readPosX(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 8, 4);
            return buffer;
        }
        int readPosY(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 12, 4);
            return buffer;
        }
        int readPosZ(){
            int buffer;
            memset(&buffer, 0, sizeof(buffer));
            memcpy(&buffer, this->payload + 16, 4);
            return buffer;
        }*/
};


void printBytes2(char *PDU);
// void sendPacketToAirsim(int socketNumber, uint8_t *packet);
}

#endif