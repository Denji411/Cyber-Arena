#ifndef GLOBALS_H
#define GLOBALS_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

/* ───────────── Dimensioni arena ───────────── */
#define RIGHE       20
#define COLONNE     50
#define MAX_ROBOT   5

/* ───────────── Celle speciali ───────────── */
#define CELLA_LIBERA    '.'
#define CELLA_BORDO     '#'
#define CELLA_MINA      'M'
#define CELLA_ENERGIA   '+'
#define CELLA_SCUDO     'S'
#define CELLA_OSTACOLO  'O'

/* ───────────── Costanti di gioco ───────────── */
#define HP_INIZIALE     100
#define DANNO_ATTACCO   15
#define RECUPERO_RIPOSO 10
#define BONUS_ENERGIA   25
#define DANNO_MINA      40
#define DURATA_SCUDO    5      /* turni di protezione */
#define RAGGIO_RADAR    6      /* distanza massima rilevamento nemici */

/* ───────────── Tipi di robot (personalità) ───────────── */
typedef enum {
    AGGRESSIVO = 0,
    PRUDENTE,
    CASUALE,
    DIFENSIVO,
    KAMIKAZE
} TipoRobot;

/* ───────────── Stato di un singolo robot ───────────── */
typedef struct {
    int  x;            /* riga nella mappa */
    int  y;            /* colonna nella mappa */
    int  hp;
    int  scudo;        /* turni di scudo rimasti */
    bool vivo;
    char simbolo;
    TipoRobot tipo;
    int  velocita;     /* microsecondi tra un'azione e l'altra */
    int  mine_piazzate;
} Robot;

/* ───────────── Stato globale condiviso ───────────── */
typedef struct {
    Robot    robot[MAX_ROBOT];
    char     mappa[RIGHE][COLONNE];
    int      num_robot;
    bool     partita_finita;
    int      vincitore;           /* indice del robot vincitore, -1 se ancora in corso */
    char     ultimo_evento[128];  /* messaggio da mostrare nell'interfaccia */
} StatoGioco;

/* ───────────── Argomenti per il thread robot ───────────── */
typedef struct {
    StatoGioco *stato;
    int         id;               /* indice nel vettore robot[] */
} ArgRobot;

/* ───────────── Semaforo globale per la mappa ───────────── */
extern sem_t sem_mappa;

/* ───────────── Mutex per i messaggi evento ───────────── */
extern pthread_mutex_t mtx_evento;

/* ───────────── Simboli robot ───────────── */
static const char SIMBOLI_ROBOT[MAX_ROBOT] = {'A','P','C','D','K'};

/* ───────────── Nomi robot ───────────── */
extern const char *NOMI_ROBOT[MAX_ROBOT];

#endif