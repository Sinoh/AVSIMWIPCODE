/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "AirSimConnect-helper.h"
#include "Networks-helper.h"

namespace ns3 {

// Used to initialize the a Car struct for a carBuffer. The
struct Car *initCar(struct CarBuffer *carBuffer, int socketNumber, int carNumber, char *payload){
	struct Car *car = (struct Car*)sCalloc(1, sizeof(struct Car));
	car->socketNumber = socketNumber;
	car->carNumber = carNumber;
	memset(car->payload, 0, PAYLOADSIZE);
	memcpy(car->payload, payload, PAYLOADSIZE);
	car->nextCar = NULL;

	return car;

}

// Called when the server is first spun up. Will create a struct that holds a flag to determine when the server
// ends, bufferSize of the list of cars, and a linked list of Car structs
struct CarBuffer *initCarBuffer(){
	struct CarBuffer *carBuffer = (struct CarBuffer*)malloc(sizeof(struct CarBuffer));
	carBuffer->bufferSize = 0;
	carBuffer->buffer = NULL;
	carBuffer->flag = 1;

	return carBuffer;
}

// Adds a Car struct to the CarBuffer, given the first car in the linked list and the new car to be added
struct Car *addCar(struct Car *carList, struct Car * car){
	if (carList != NULL) {
		carList->nextCar = addCar(carList->nextCar, car);
		return carList;
	}
	return car;

}

// Returns a speicifc Car struct you are trying to find
struct Car *findCar(struct CarBuffer *carBuffer, int socketNumber) {
	struct Car *tempCar = carBuffer->buffer;
	while (tempCar != NULL) {
		if (socketNumber == tempCar->socketNumber) {
			return tempCar;
		}
		tempCar = tempCar->nextCar;
	}
	return 0;
}

// Updates the payload/data the car is holding
void updateCar(struct Car *carList, int socketNumber, char *payload) {
	if (carList->socketNumber != socketNumber) {
		updateCar(carList->nextCar, socketNumber, payload);
	}
	else {
		memset(carList->payload, 0, PAYLOADSIZE);
		memcpy(carList->payload, payload, PAYLOADSIZE);
	}
}

//  Returns the payload/data the car is holding at the time
int getCarPayload(struct Car *carList, int carNumber, char *payload) {
	if (carList == NULL){
		return 0;
	}else if (carList->carNumber != carNumber) {
		return getCarPayload(carList->nextCar, carNumber, payload);
	}else{
		memset(payload, 0, PAYLOADSIZE);
		memcpy(payload, carList->payload, PAYLOADSIZE);
		return 1;
	}
}

// Removes a specifc car from the linkedlist. This will handle reconnecting the linkedlist
// and updating the bufferSize
struct Car *removeCar(int SocketNumber, struct Car *car, struct CarBuffer *carBuffer){
	if (car == NULL){
		return NULL;
	}else if (car->socketNumber == SocketNumber){
		memset(car->payload, 0, PAYLOADSIZE);
		printf("Car on socket %d terminted\n", SocketNumber);
		carBuffer->bufferSize--;
		return car->nextCar;
	}else{
		car->nextCar = removeCar(SocketNumber, car->nextCar, carBuffer);
		return car;
	}
	return 0;
}

// Helper function for testing purposes
// This will print the linked list in the following format
// 1(5) -> 2(6) -> NULL
void printCarList(struct CarBuffer *list) {
	// For debugging purposes
	if (list->buffer != NULL) {
		printCars(list->buffer);
	}
}

// Helper function for the printCarList function
// Prints the current car and the socket number it is connected to
void printCars(struct Car *car) {
	// Helper function for printLinkedList
	printf("%d(%d) -> ", car->carNumber, car->socketNumber);
	if (car->nextCar == NULL) {
		printf("NULL\n");
	}
	else {
		printCars(car->nextCar);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// Returns the current speed of the vehicle
// double getSpeed(){
// }

}