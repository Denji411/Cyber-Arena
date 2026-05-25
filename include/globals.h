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

#define HP_INIZIALE     100
#define DANNO_ATTACCO   15
#define RECUPERO_RIPOSO 10
#define BONUS_ENERGIA   25
#define DANNO_MINA      40
#define DURATA_SCUDO    5
#define RAGGIO_RADAR    15

typedef enum {
    AGGRESSIVO = 0,
    PRUDENTE,
    CASUALE,
    DIFENSIVO,
    KAMIKAZE
} TipoRobot;

typedef struct {
    int  x;
    int  y;
    int  hp;
    int  scudo;
    bool vivo;
    char simbolo;
    TipoRobot tipo;
    int  velocita;
    int  mine_piazzate;
} Robot;

typedef struct {
    Robot    robot[MAX_ROBOT];
    char     mappa[RIGHE][COLONNE];
    int      num_robot;
    bool     partita_finita;
    int      vincitore;
    char     ultimo_evento[128];
} StatoGioco;

typedef struct {
    StatoGioco *stato;
    int         id;
} ArgRobot;

extern sem_t sem_mappa;

extern pthread_mutex_t mtx_evento;

static const char SIMBOLI_ROBOT[MAX_ROBOT] = {'A','P','C','D','K'};

extern const char *NOMI_ROBOT[MAX_ROBOT];

#endif