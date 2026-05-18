#ifndef ROBOT_H
#define ROBOT_H

#include "globals.h"

/* ───────────── Thread principale di ogni robot ───────────── */
void *thread_robot(void *arg);

/* ───────────── Funzioni di strategia ───────────── */
/* Restituisce l'indice del nemico vivo più vicino, -1 se nessuno nel raggio */
int radar_prossimita(StatoGioco *stato, int id);

/* Calcola distanza Manhattan tra due robot */
int distanza(StatoGioco *stato, int id_a, int id_b);

/* ───────────── Azioni atomiche ───────────── */
/* Tutte le funzioni seguenti acquisiscono/rilasciano sem_mappa internamente */

/* Sposta il robot verso (dx, dy). Restituisce true se lo spostamento è avvenuto. */
bool muovi(StatoGioco *stato, int id, int dx, int dy);

/* Attacca il robot nemico adiacente con HP più bassi. Restituisce true se attacca. */
bool attacca(StatoGioco *stato, int id);

/* Il robot recupera HP (rimane fermo). */
void riposa(StatoGioco *stato, int id);

/* Piazza una mina nella cella appena lasciata. */
bool piazza_mina(StatoGioco *stato, int id, int x_prec, int y_prec);

/* ───────────── Invio evento via ZeroMQ ───────────── */
/* Pubblica un messaggio sulla socket ZMQ (usata internamente dai thread robot) */
void pubblica_evento(void *zmq_socket, const char *messaggio);

#endif /* ROBOT_H */
