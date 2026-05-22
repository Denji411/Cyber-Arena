#ifndef GRAFICA_H
#define GRAFICA_H

#include "globals.h"

/* Argomenti per il thread grafico */
typedef struct {
    StatoGioco *stato;
} ArgGrafica;

/* ───────────── Thread grafico ───────────── */
/* Aggiorna continuamente la finestra ncurses finché la partita è in corso */
void *thread_grafico(void *arg);

/* ───────────── Funzioni di rendering ───────────── */
/* Disegna i bordi e la mappa */
void disegna_mappa(StatoGioco *stato);

/* Disegna la classifica laterale con HP e stato dei robot */
void disegna_classifica(StatoGioco *stato);

/* Disegna il riquadro eventi in basso */
void disegna_eventi(const char *messaggio);

/* Inizializza i color pair ncurses */
void init_colori(void);

/* Restituisce il color pair in base agli HP (verde > 60, giallo > 30, rosso altrimenti) */
int colore_hp(int hp);

#endif