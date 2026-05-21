/* Necessario per usleep() con -std=c11 */
#define _XOPEN_SOURCE 600

/*
 * eventi.c
 * Thread eventi speciali: ogni tot secondi fa comparire casualmente
 * power-up (energia, scudo) e muove gli ostacoli presenti sulla mappa.
 *
 * Tutti gli accessi alla mappa sono protetti da sem_mappa.
 * Gli eventi vengono scritti in stato->ultimo_evento (protetto da mtx_evento).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "eventi.h"

/* Intervallo tra spawn di power-up (microsecondi) */
#define INTERVALLO_SPAWN    3000000  /* 3 secondi */
#define INTERVALLO_OSTACOLO 5000000  /* 5 secondi */

/* ─────────────────────────────────────────────
 * Funzioni helper
 * ───────────────────────────────────────────── */

/* Trova una cella libera casuale nella mappa; restituisce false se non trovata */
static bool cella_libera_casuale(StatoGioco *stato, int *out_r, int *out_c) {
    int tentativi = 50;
    while (tentativi-- > 0) {
        int r = 1 + rand() % (RIGHE - 2);
        int c = 1 + rand() % (COLONNE - 2);
        if (stato->mappa[r][c] == CELLA_LIBERA) {
            *out_r = r;
            *out_c = c;
            return true;
        }
    }
    return false;
}

void spawna_powerup(StatoGioco *stato) {
    sem_wait(&sem_mappa);

    int r, c;
    if (!cella_libera_casuale(stato, &r, &c)) {
        sem_post(&sem_mappa);
        return;
    }

    /* Sceglie casualmente energia o scudo (70% / 30%) */
    char simbolo;
    const char *nome;
    if (rand() % 10 < 7) {
        simbolo = CELLA_ENERGIA;
        nome    = "energia (+25 HP)";
    } else {
        simbolo = CELLA_SCUDO;
        nome    = "scudo temporaneo";
    }

    stato->mappa[r][c] = simbolo;

    /* Pubblica evento tramite stato condiviso */
    char msg[128];
    snprintf(msg, sizeof(msg), "Power-up [%s] apparso in (%d,%d)!", nome, r, c);
    pthread_mutex_lock(&mtx_evento);
    strncpy(stato->ultimo_evento, msg, sizeof(stato->ultimo_evento) - 1);
    pthread_mutex_unlock(&mtx_evento);

    sem_post(&sem_mappa);
}

void muovi_ostacolo(StatoGioco *stato) {
    int dx[] = {-1, 1,  0, 0};
    int dy[] = { 0, 0, -1, 1};

    sem_wait(&sem_mappa);

    /* Cerca un ostacolo esistente */
    for (int r = 1; r < RIGHE - 1; r++) {
        for (int c = 1; c < COLONNE - 1; c++) {
            if (stato->mappa[r][c] == CELLA_OSTACOLO) {
                /* Prova a spostarlo in una direzione casuale */
                int dir = rand() % 4;
                int nr = r + dx[dir];
                int nc = c + dy[dir];
                if (nr > 0 && nr < RIGHE - 1 && nc > 0 && nc < COLONNE - 1
                    && stato->mappa[nr][nc] == CELLA_LIBERA) {
                    stato->mappa[r][c]   = CELLA_LIBERA;
                    stato->mappa[nr][nc] = CELLA_OSTACOLO;
                }
                sem_post(&sem_mappa);
                return;
            }
        }
    }

    /* Nessun ostacolo trovato: piazza uno nuovo */
    int r, c;
    if (cella_libera_casuale(stato, &r, &c)) {
        stato->mappa[r][c] = CELLA_OSTACOLO;

        char msg[64];
        snprintf(msg, sizeof(msg), "Nuovo ostacolo apparso in (%d,%d)!", r, c);
        pthread_mutex_lock(&mtx_evento);
        strncpy(stato->ultimo_evento, msg, sizeof(stato->ultimo_evento) - 1);
        pthread_mutex_unlock(&mtx_evento);
    }

    sem_post(&sem_mappa);
}

void pulisci_mine(StatoGioco *stato) {
    /* Le mine vengono già rimosse in muovi() quando esplodono.
     * Questa funzione è un hook per future espansioni (mine a tempo). */
    (void)stato;
}

/* ─────────────────────────────────────────────
 * Thread eventi speciali
 * ───────────────────────────────────────────── */

void *thread_eventi(void *arg) {
    ArgEventi  *args  = (ArgEventi *)arg;
    StatoGioco *stato = args->stato;

    unsigned long elapsed = 0;

    while (!stato->partita_finita) {
        usleep(500000); /* controlla ogni 0.5 secondi */
        elapsed += 500000;

        if (elapsed % INTERVALLO_SPAWN == 0) {
            spawna_powerup(stato);
        }

        if (elapsed % INTERVALLO_OSTACOLO == 0) {
            muovi_ostacolo(stato);
        }

        pulisci_mine(stato);
    }

    pthread_exit(NULL);
}