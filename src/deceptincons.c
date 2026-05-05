#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "decepticons.h"

#define ACTION_ARRAY_SIZE 10

void *decepticon(void* arg) {
    D_args *args = (D_args*) arg;
    Status *st = args -> st;
    int ID = args -> D_ID;

    int action_array[ACTION_ARRAY_SIZE] = {0};
    while(1) {
        if (st -> rHP < 20) {
            //TODO: probabilità riposo
        }
    }
    
    pthread_exit(NULL);
}