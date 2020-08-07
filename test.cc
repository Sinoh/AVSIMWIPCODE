
#include "networks.h"

int main(int argc, char const *argv[]) 
{    
    int i;
    struct LinkedList *list = startSwitch();
    sleep(3);
    while(1){
        sleep(1.5);
		if (list->root != NULL){
			printf("Car speed: %i\n", getCarSpeed(list, 0));
            printf("Car posX: %i\n", getCarPosX(list, 0));
            printf("Car posY: %i\n", getCarPosX(list, 0));
            printf("Car posZ: %i\n", getCarPosX(list, 0));
		}
    }
    return 0;
    
} 