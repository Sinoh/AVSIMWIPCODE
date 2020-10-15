
#include "networks.h"

int main(int argc, char const *argv[])
{
    // int i;
    struct CarBuffer *buffer = startSwitch();
    char payload[PAYLOADSIZE];
    sleep(5);
    memset(payload, 0, PAYLOADSIZE);
    getCarPayload(buffer->buffer, 0, payload);
    printf("Car %i payload: %s\n", 0, payload);
    memset(payload, 0, PAYLOADSIZE);
    getCarPayload(buffer->buffer, 1, payload);
    printf("Car %i payload: %s\n", 1, payload);
    sleep(5);

    getCarPayload(buffer->buffer, 0, payload);
    printf("Car %i payload: %s\n", 0, payload);
    memset(payload, 0, PAYLOADSIZE);
    getCarPayload(buffer->buffer, 1, payload);
    printf("Car %i payload: %s\n", 1, payload);
    // while(buffer->flag){
    //     sleep(1);
    //     for (i = 0; i < buffer->bufferSize; i++) {
    //         if (buffer->buffer != NULL) {
    //             memset(payload, 0, PAYLOADSIZE);
    //             getCarPayload(buffer->buffer, i, payload);
    //             printf("Car %i payload: %s\n", i, payload);
    //         }
    //     }
    // }
    return 0;

}
