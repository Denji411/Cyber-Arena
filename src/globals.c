/*
 * globals.c
 * Definizione delle variabili globali dichiarate come extern in globals.h.
 * Centralizzare qui le variabili evita problemi di "multiple definition"
 * quando si linkano più unità di compilazione.
 */

#include "globals.h"
#include <semaphore.h>
#include <pthread.h>

/* Semaforo binario che protegge l'accesso alla mappa e ai dati dei robot.
 * Valore iniziale 1 → un solo thread alla volta può modificare lo stato. */
sem_t sem_mappa;

/* Mutex per aggiornare il campo ultimo_evento in modo sicuro. */
pthread_mutex_t mtx_evento = PTHREAD_MUTEX_INITIALIZER;

/* Nomi dei robot in ordine di tipo (corrisponde all'enum TipoRobot) */
const char *NOMI_ROBOT[MAX_ROBOT] = {
    "Aggressore", "Prudente", "Casuale", "Difensore", "Kamikaze"
};
