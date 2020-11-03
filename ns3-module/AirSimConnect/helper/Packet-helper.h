/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PACKET_HELPER_H
#define PACKET_HELPER_H

#include "ns3/AirSimConnect.h"

namespace ns3 {
#define PAYLOADSIZE 1024
class Packet{
    int sequenceNumber;
    int packetFlag;
    int packetLength;
    int speed;
    int gear;
    float posX;
    float posY;
    float posZ;
    uint8_t *payload;

    public:
        void updateSequenceNumber(int NewNumber){
            this->sequenceNumber = NewNumber;
        }
        void updatePacketFlag(int NewFlag){
            this->packetFlag = NewFlag;

        }
        void updatePacketLength(int NewLength){
            this->packetLength = NewLength;
        }
        void updateSpeed(int NewSpeed){
            this->speed = NewSpeed;
        }
        void updateGear(int NewGear){
            this->gear = NewGear;
        }
        void updatePosX(int NewPosX){
            this->posX = NewPosX;
        }
        void updatePosY(int NewPosY){
            this->posY = NewPosY;
        }
        void updatePosZ(int NewPosZ){
            this->posZ = NewPosZ;
        }
        void writePayload(){
            uint32_t netOrdSequenceNumber = htonl(this->sequenceNumber);
            memset(this->payload, 0, PAYLOADSIZE + 1);
            memcpy(this->payload, &netOrdSequenceNumber, 4);
            memcpy(this->payload + 6, &this->packetFlag, 1);
	        memcpy(this->payload + 7, &this->packetLength, 1);
            memcpy(this->payload + 8, &this->speed, 1);
            memcpy(this->payload + 9, &this->gear, 1);
            memcpy(this->payload + 10, &this->posX, 1);
            memcpy(this->payload + 11, &this->posY, 1);
            memcpy(this->payload + 12, &this->posZ, 1);
        }
        uint8_t *getPayload(){
            return payload;
        }

};
}

#endif