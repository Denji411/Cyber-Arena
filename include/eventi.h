#ifndef EVENTI_H
#define EVENTI_H

#include "globals.h"

/* Argomenti per il thread eventi */
typedef struct {
    StatoGioco *stato;
} ArgEventi;

/* ───────────── Thread eventi speciali ───────────── */
/* Fa comparire periodicamente power-up, mine e ostacoli mobili */
void *thread_eventi(void *arg);

/* ───────────── Funzioni helper ───────────── */
/* Piazza un power-up casuale in una cella libera */
void spawna_powerup(StatoGioco *stato);

/* Sposta un ostacolo esistente in una cella adiacente libera */
void muovi_ostacolo(StatoGioco *stato);

/* Hook per future espansioni (mine a tempo) */
void pulisci_mine(StatoGioco *stato);

#endif