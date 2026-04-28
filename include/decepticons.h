#ifndef DECEPTICONS_H
#define DECEPTICONS_H

#include <pthread.h>

#define MAX_ROBOT 5

typedef struct {

    int rX[MAX_ROBOT];
    int rY[MAX_ROBOT];
    int rHP[MAX_ROBOT];
    int rStato[MAX_ROBOT];
    char rSimbolo[MAX_ROBOT];

}Status;

void* decepticon(void*);

#endif