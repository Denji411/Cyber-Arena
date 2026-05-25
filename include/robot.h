#ifndef ROBOT_H
#define ROBOT_H

#include "globals.h"

void *thread_robot(void *arg);

int radar_prossimita(StatoGioco *stato, int id);

int distanza(StatoGioco *stato, int id_a, int id_b);

bool muovi(StatoGioco *stato, int id, int dx, int dy);

bool attacca(StatoGioco *stato, int id);

void riposa(StatoGioco *stato, int id);

bool piazza_mina(StatoGioco *stato, int id, int x_prec, int y_prec);

#endif