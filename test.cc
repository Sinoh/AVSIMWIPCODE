
#include "networks.h"

int main(int argc, char const *argv[])
{
    int i;
    struct CarBuffer *buffer = startSwitch();
    char payload[PAYLOADSIZE];
    while(1){
        sleep(0.8);
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
