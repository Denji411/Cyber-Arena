#include "globals.h"
#include <semaphore.h>
#include <pthread.h>

sem_t sem_mappa;
 
pthread_mutex_t mtx_evento = PTHREAD_MUTEX_INITIALIZER;
 
const char *NOMI_ROBOT[MAX_ROBOT] = {
    "Aggressore", "Prudente", "Casuale", "Difensore", "Kamikaze"
};