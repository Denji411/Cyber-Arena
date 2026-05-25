#ifndef EVENTI_H
#define EVENTI_H

#include "globals.h"

typedef struct {
    StatoGioco *stato;
} ArgEventi;

void *thread_eventi(void *arg);

void spawna_powerup(StatoGioco *stato);

void muovi_ostacolo(StatoGioco *stato);

#endif