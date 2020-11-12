#include "Packet-helper.h"
#include <vector>
#include <iostream>
#include <string>  
namespace ns3 {

void carData::setData(int newSequenceNumber, int newPacketFlag, int newId, int newPacketLength, 
					double newSpeed, double newGear, double newPosX, double newPosY, double newPosZ){
	sequenceNumber = newSequenceNumber;
	packetFlag = newPacketFlag;
	Id = newId;
	packetLength = newPacketLength;
	speed = newSpeed;
	gear = newGear;
	posX = newPosX;
	posY = newPosY;
	posZ = newPosZ;
}

int carData::writePayload(char *buffer){
    return sprintf(buffer, ":%f:%f:%f:%f:%f:\r", speed, gear, posX, posY, posZ);
}
void carData::createCarPacket(){
	char buffer[1024];
	this->writePayload(buffer);

	memset(this->payload, 0, 6);
	memcpy(this->payload, (uint32_t*)&this->sequenceNumber, 4);
	memcpy(this->payload + 4, &this->packetFlag, 1);
	memcpy(this->payload + 5, &this->Id, 1);
	memcpy(this->payload + 6, &this->packetLength, 1);
	memcpy(this->payload + 7, buffer, packetLength);
	memset(this->payload + packetLength, 0, 1);
}

void carData::readCarPacket(char *packet){
	uint32_t tempSequenceNumber;
	std::string str = packet + 7;
	std::string delimiter = ":";
	std::string token;

	
	str = packet + 7;

	auto start = 0U;
    auto end = str.find(delimiter);
	double attributes[5];
	int i = 0;
	int length;
	start = end + delimiter.length();
	end = str.find(delimiter, start);

	while (end != std::string::npos){
        token = str.substr(start, end - start);
		length = token.length();
		char tempBuf[length];
		strcpy(tempBuf, token.c_str());
		attributes[i] = atof(tempBuf);
        start = end + delimiter.length();
        end = str.find(delimiter, start);
		i++;
    }
	memcpy(&tempSequenceNumber, packet, 4);
	setData(tempSequenceNumber, packet[4], packet[5], packet[6], 
			attributes[0], attributes[1],attributes[2] ,attributes[3] ,attributes[4]);
}

// For DEBUG purposes only. Prints every byte in a buffer in Hex, Num, and Char
void printBytes2(char *PDU){
	int i;

	printf("\tHex\t|\tNum\t|\tChar\n");
	for (i = 0; i < 100; i++){
		printf("%i:\t%x\t|\t%i\t|\t%c\n", i, PDU[i] & 0xffff, PDU[i], PDU[i]);
	}
}

// void sendPacketToAirsim(int socketNumber, class Packet packet){
//     safeSend(socketNumber, (char*) packet.getPayload(), packet.readPacketFlag());
// }


}