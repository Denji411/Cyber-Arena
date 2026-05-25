#ifndef GRAFICA_H
#define GRAFICA_H

#include "globals.h"

typedef struct {
    StatoGioco *stato;
} ArgGrafica;

void *thread_grafico(void *arg);

void disegna_mappa(StatoGioco *stato);

void disegna_classifica(StatoGioco *stato);

void disegna_eventi(const char *messaggio);

void init_colori(void);

int colore_hp(int hp);

#endif