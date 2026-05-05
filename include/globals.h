#ifndef GLOBALS_H
#define GLOBALS_H

#define LONG 20
#define LAT 50
#define MAX_ROBOT 5

typedef struct {

    int rX[MAX_ROBOT];
    int rY[MAX_ROBOT];
    int rHP[MAX_ROBOT];
    int rStato[MAX_ROBOT];
    char rSimbolo[MAX_ROBOT];

}Status;

typedef struct {
    Status *st;
    int D_ID;
} D_args;

extern char mappa[LONG][LAT];
extern int sem_map[LONG][LAT];

#endif