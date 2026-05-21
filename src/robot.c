/* Necessario per usleep() con -std=c11 */
#define _XOPEN_SOURCE 600

/*
 * robot.c
 * Implementa il thread di ogni robot e la sua intelligenza artificiale.
 *
 * Ogni robot gira nel proprio thread POSIX. La mappa e lo stato condiviso
 * sono protetti dal semaforo sem_mappa: ogni accesso in scrittura (muovi,
 * attacca, piazza mina) fa una sem_wait prima e una sem_post dopo.
 *
 * Gli eventi vengono scritti in stato->ultimo_evento (protetto da mtx_evento).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

#include "globals.h"
#include "robot.h"

/* ─────────────────────────────────────────────
 * Utilità interne
 * ───────────────────────────────────────────── */

/* Calcola distanza Manhattan tra robot id_a e id_b */
int distanza(StatoGioco *stato, int id_a, int id_b) {
    return abs(stato->robot[id_a].x - stato->robot[id_b].x)
         + abs(stato->robot[id_a].y - stato->robot[id_b].y);
}

/* Restituisce l'indice del nemico vivo più vicino entro RAGGIO_RADAR, -1 se nessuno */
int radar_prossimita(StatoGioco *stato, int id) {
    int min_dist = RAGGIO_RADAR + 1;
    int bersaglio = -1;
    for (int i = 0; i < stato->num_robot; i++) {
        if (i == id || !stato->robot[i].vivo) continue;
        int d = distanza(stato, id, i);
        if (d < min_dist) {
            min_dist = d;
            bersaglio = i;
        }
    }
    return bersaglio;
}

/* ─────────────────────────────────────────────
 * Azioni atomiche (sem_mappa acquisito internamente)
 * ───────────────────────────────────────────── */

bool muovi(StatoGioco *stato, int id, int dx, int dy) {
    Robot *r = &stato->robot[id];
    int nx = r->x + dx;
    int ny = r->y + dy;

    sem_wait(&sem_mappa);

    /* Controlla bordi */
    if (nx <= 0 || nx >= RIGHE - 1 || ny <= 0 || ny >= COLONNE - 1) {
        sem_post(&sem_mappa);
        return false;
    }
    char cella = stato->mappa[nx][ny];
    if (cella == CELLA_BORDO || cella == CELLA_OSTACOLO) {
        sem_post(&sem_mappa);
        return false;
    }

    /* Controlla se un altro robot occupa già la cella */
    for (int i = 0; i < stato->num_robot; i++) {
        if (i != id && stato->robot[i].vivo &&
            stato->robot[i].x == nx && stato->robot[i].y == ny) {
            sem_post(&sem_mappa);
            return false;
        }
    }

    /* Gestisce cella speciale */
    if (cella == CELLA_MINA && r->scudo == 0) {
        r->hp -= DANNO_MINA;
        stato->mappa[nx][ny] = CELLA_LIBERA;
        char msg[128];
        snprintf(msg, sizeof(msg), "%s ha colpito una mina! -%d HP",
                 NOMI_ROBOT[id], DANNO_MINA);
        pthread_mutex_lock(&mtx_evento);
        strncpy(stato->ultimo_evento, msg, sizeof(stato->ultimo_evento) - 1);
        pthread_mutex_unlock(&mtx_evento);
    } else if (cella == CELLA_MINA && r->scudo > 0) {
        /* Lo scudo assorbe la mina */
        r->scudo = 0;
        stato->mappa[nx][ny] = CELLA_LIBERA;
    } else if (cella == CELLA_ENERGIA) {
        r->hp = (r->hp + BONUS_ENERGIA > HP_INIZIALE) ? HP_INIZIALE : r->hp + BONUS_ENERGIA;
        stato->mappa[nx][ny] = CELLA_LIBERA;
    } else if (cella == CELLA_SCUDO) {
        r->scudo = DURATA_SCUDO;
        stato->mappa[nx][ny] = CELLA_LIBERA;
    }

    /* Aggiorna posizione nella mappa */
    stato->mappa[r->x][r->y] = CELLA_LIBERA;
    r->x = nx;
    r->y = ny;
    stato->mappa[nx][ny] = r->simbolo;

    /* Controlla se il robot è morto per la mina */
    if (r->hp <= 0) {
        r->hp = 0;
        r->vivo = false;
        stato->mappa[nx][ny] = CELLA_LIBERA;
    }

    sem_post(&sem_mappa);
    return true;
}

bool attacca(StatoGioco *stato, int id) {
    Robot *r = &stato->robot[id];
    int dx[] = {-1, 1,  0, 0};
    int dy[] = { 0, 0, -1, 1};

    sem_wait(&sem_mappa);

    for (int dir = 0; dir < 4; dir++) {
        int nx = r->x + dx[dir];
        int ny = r->y + dy[dir];
        for (int i = 0; i < stato->num_robot; i++) {
            if (i == id || !stato->robot[i].vivo) continue;
            if (stato->robot[i].x == nx && stato->robot[i].y == ny) {
                int danno = (r->tipo == KAMIKAZE) ? DANNO_ATTACCO * 2 : DANNO_ATTACCO;
                if (stato->robot[i].scudo > 0) {
                    stato->robot[i].scudo--;
                    danno = 0; /* scudo assorbe */
                } else {
                    stato->robot[i].hp -= danno;
                }

                char msg[128];
                if (stato->robot[i].hp <= 0) {
                    stato->robot[i].hp = 0;
                    stato->robot[i].vivo = false;
                    stato->mappa[nx][ny] = CELLA_LIBERA;
                    snprintf(msg, sizeof(msg), "%s ha distrutto %s!",
                             NOMI_ROBOT[id], NOMI_ROBOT[i]);
                } else {
                    snprintf(msg, sizeof(msg), "%s attacca %s (-%d HP)",
                             NOMI_ROBOT[id], NOMI_ROBOT[i], danno);
                }
                pthread_mutex_lock(&mtx_evento);
                strncpy(stato->ultimo_evento, msg, sizeof(stato->ultimo_evento) - 1);
                pthread_mutex_unlock(&mtx_evento);

                sem_post(&sem_mappa);
                return true;
            }
        }
    }

    sem_post(&sem_mappa);
    return false;
}

void riposa(StatoGioco *stato, int id) {
    sem_wait(&sem_mappa);
    Robot *r = &stato->robot[id];
    r->hp = (r->hp + RECUPERO_RIPOSO > HP_INIZIALE) ? HP_INIZIALE : r->hp + RECUPERO_RIPOSO;
    sem_post(&sem_mappa);
}

bool piazza_mina(StatoGioco *stato, int id, int x_prec, int y_prec) {
    sem_wait(&sem_mappa);
    if (stato->mappa[x_prec][y_prec] == CELLA_LIBERA) {
        stato->mappa[x_prec][y_prec] = CELLA_MINA;
        stato->robot[id].mine_piazzate++;
        sem_post(&sem_mappa);
        return true;
    }
    sem_post(&sem_mappa);
    return false;
}

/* ─────────────────────────────────────────────
 * Strategie per tipo di robot
 * ───────────────────────────────────────────── */

static int DX[] = {-1, 1,  0, 0};
static int DY[] = { 0, 0, -1, 1};

/* Muove il robot verso il bersaglio (id_target) */
static void avvicina(StatoGioco *stato, int id, int id_target) {
    Robot *r  = &stato->robot[id];
    Robot *t  = &stato->robot[id_target];
    int dx = (t->x > r->x) ? 1 : (t->x < r->x) ? -1 : 0;
    int dy = (t->y > r->y) ? 1 : (t->y < r->y) ? -1 : 0;
    if (dx != 0 && !muovi(stato, id, dx, 0)) muovi(stato, id, 0, dy);
    else if (dy != 0) muovi(stato, id, 0, dy);
}

/* Allontana il robot dal bersaglio */
static void allontana(StatoGioco *stato, int id, int id_target) {
    Robot *r  = &stato->robot[id];
    Robot *t  = &stato->robot[id_target];
    int dx = (t->x > r->x) ? -1 : 1;
    int dy = (t->y > r->y) ? -1 : 1;
    if (!muovi(stato, id, dx, 0)) muovi(stato, id, 0, dy);
}

/* Mossa casuale */
static void muovi_casuale(StatoGioco *stato, int id) {
    int dir = rand() % 4;
    muovi(stato, id, DX[dir], DY[dir]);
}

/* Logica AGGRESSIVO: insegue e attacca sempre */
static void comportamento_aggressivo(StatoGioco *stato, int id) {
    if (attacca(stato, id)) return;
    int bersaglio = radar_prossimita(stato, id);
    if (bersaglio >= 0) avvicina(stato, id, bersaglio);
    else muovi_casuale(stato, id);
}

/* Logica PRUDENTE: scappa se HP bassi, altrimenti cerca bonus o attacca */
static void comportamento_prudente(StatoGioco *stato, int id) {
    Robot *r = &stato->robot[id];
    int bersaglio = radar_prossimita(stato, id);
    if (r->hp < 40) {
        if (bersaglio >= 0) allontana(stato, id, bersaglio);
        else riposa(stato, id);
    } else {
        if (attacca(stato, id)) return;
        if (bersaglio >= 0) avvicina(stato, id, bersaglio);
        else muovi_casuale(stato, id);
    }
}

/* Logica CASUALE: azioni completamente random */
static void comportamento_casuale(StatoGioco *stato, int id) {
    int scelta = rand() % 4;
    switch (scelta) {
        case 0: muovi_casuale(stato, id); break;
        case 1: attacca(stato, id);       break;
        case 2: riposa(stato, id);        break;
        default: muovi_casuale(stato, id); break;
    }
}

/* Logica DIFENSIVO: rimane in zona, attacca se qualcuno si avvicina, piazza mine */
static void comportamento_difensivo(StatoGioco *stato, int id) {
    Robot *r = &stato->robot[id];
    int bersaglio = radar_prossimita(stato, id);
    if (bersaglio >= 0 && distanza(stato, id, bersaglio) <= 2) {
        if (!attacca(stato, id)) {
            int xp = r->x, yp = r->y;
            muovi_casuale(stato, id);
            piazza_mina(stato, id, xp, yp);
        }
    } else {
        riposa(stato, id);
    }
}

/* Logica KAMIKAZE: carica il nemico più vicino ignorando i danni */
static void comportamento_kamikaze(StatoGioco *stato, int id) {
    int bersaglio = radar_prossimita(stato, id);
    if (bersaglio >= 0) {
        avvicina(stato, id, bersaglio);
        attacca(stato, id);
    } else {
        muovi_casuale(stato, id);
    }
}

/* ─────────────────────────────────────────────
 * Thread principale del robot
 * ───────────────────────────────────────────── */

void *thread_robot(void *arg) {
    ArgRobot   *args  = (ArgRobot *)arg;
    StatoGioco *stato = args->stato;
    int         id    = args->id;
    Robot      *r     = &stato->robot[id];

    /* Seme random univoco per ogni robot */
    srand((unsigned)(id * 1234 + 5678));

    while (!stato->partita_finita && r->vivo) {
        /* Controlla se è rimasto un solo robot vivo */
        int vivi = 0, ultimo = -1;
        sem_wait(&sem_mappa);
        for (int i = 0; i < stato->num_robot; i++) {
            if (stato->robot[i].vivo) { vivi++; ultimo = i; }
        }
        sem_post(&sem_mappa);
        if (vivi <= 1) {
            stato->partita_finita = true;
            stato->vincitore = ultimo;
            break;
        }

        /* Esegue il comportamento in base alla personalità */
        switch (r->tipo) {
            case AGGRESSIVO: comportamento_aggressivo(stato, id); break;
            case PRUDENTE:   comportamento_prudente  (stato, id); break;
            case CASUALE:    comportamento_casuale   (stato, id); break;
            case DIFENSIVO:  comportamento_difensivo (stato, id); break;
            case KAMIKAZE:   comportamento_kamikaze  (stato, id); break;
        }

        /* Pausa dipendente dalla velocità del robot */
        usleep(r->velocita);
    }

    pthread_exit(NULL);
}