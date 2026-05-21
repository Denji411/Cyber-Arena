/* Necessario per usleep() con -std=c11 */
#define _XOPEN_SOURCE 600

/*
 * grafica.c
 * Thread grafico: aggiorna la schermata ncurses 10 volte al secondo.
 *
 * Legge stato->ultimo_evento (protetto da mtx_evento) per mostrare
 * i messaggi degli altri thread (robot, eventi speciali).
 *
 * Layout del terminale:
 *   ┌──────────────────────────────────────────────────┐ ┌───────────────┐
 *   │                    MAPPA 20x50                   │ │  CLASSIFICA   │
 *   │                                                  │ │  A: ████ 100  │
 *   │                                                  │ │  P: ██   45   │
 *   │                                                  │ │  ...          │
 *   └──────────────────────────────────────────────────┘ └───────────────┘
 *   [ EVENTO: Aggressore ha distrutto Prudente!                           ]
 */

#include <ncurses.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "grafica.h"

/* ─── Color pair ID ─── */
#define CP_BORDO       1   /* bianco su nero */
#define CP_HP_OK       2   /* verde */
#define CP_HP_MED      3   /* giallo */
#define CP_HP_LOW      4   /* rosso */
#define CP_EVENTO      5   /* ciano */
#define CP_ROBOT_A     6   /* magenta – Aggressivo */
#define CP_ROBOT_P     7   /* blu     – Prudente   */
#define CP_ROBOT_C     8   /* giallo  – Casuale    */
#define CP_ROBOT_D     9   /* verde   – Difensivo  */
#define CP_ROBOT_K    10   /* rosso   – Kamikaze   */
#define CP_MINA       11   /* rosso brillante */
#define CP_POWERUP    12   /* verde brillante */
#define CP_SCUDO      13   /* ciano brillante */

/* Colonne dove inizia la classifica (a destra della mappa) */
#define COL_CLASS (COLONNE + 3)
/* Riga dell'evento (sotto la mappa) */
#define RIGA_EVENTO (RIGHE + 1)

void init_colori(void) {
    start_color();
    use_default_colors();
    init_pair(CP_BORDO,   COLOR_WHITE,   COLOR_BLACK);
    init_pair(CP_HP_OK,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_HP_MED,  COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_HP_LOW,  COLOR_RED,     COLOR_BLACK);
    init_pair(CP_EVENTO,  COLOR_CYAN,    COLOR_BLACK);
    init_pair(CP_ROBOT_A, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CP_ROBOT_P, COLOR_BLUE,    COLOR_BLACK);
    init_pair(CP_ROBOT_C, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_ROBOT_D, COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_ROBOT_K, COLOR_RED,     COLOR_BLACK);
    init_pair(CP_MINA,    COLOR_RED,     COLOR_BLACK);
    init_pair(CP_POWERUP, COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_SCUDO,   COLOR_CYAN,    COLOR_BLACK);
}

int colore_hp(int hp) {
    if (hp > 60) return CP_HP_OK;
    if (hp > 30) return CP_HP_MED;
    return CP_HP_LOW;
}

/* Restituisce il color pair associato al simbolo del robot */
static int colore_robot(char simbolo) {
    switch (simbolo) {
        case 'A': return CP_ROBOT_A;
        case 'P': return CP_ROBOT_P;
        case 'C': return CP_ROBOT_C;
        case 'D': return CP_ROBOT_D;
        case 'K': return CP_ROBOT_K;
    }
    return CP_BORDO;
}

void disegna_mappa(StatoGioco *stato) {
    sem_wait(&sem_mappa);

    for (int r = 0; r < RIGHE; r++) {
        for (int c = 0; c < COLONNE; c++) {
            char ch = stato->mappa[r][c];
            int  cp = CP_BORDO;

            if (ch == CELLA_BORDO || ch == CELLA_OSTACOLO) {
                cp = CP_BORDO;
            } else if (ch == CELLA_MINA) {
                cp = CP_MINA;
            } else if (ch == CELLA_ENERGIA) {
                cp = CP_POWERUP;
            } else if (ch == CELLA_SCUDO) {
                cp = CP_SCUDO;
            } else {
                for (int i = 0; i < stato->num_robot; i++) {
                    if (stato->robot[i].vivo && stato->robot[i].simbolo == ch) {
                        cp = colore_robot(ch);
                        break;
                    }
                }
            }

            attron(COLOR_PAIR(cp));
            mvaddch(r, c, ch);
            attroff(COLOR_PAIR(cp));
        }
    }

    sem_post(&sem_mappa);
}

void disegna_classifica(StatoGioco *stato) {
    int col = COL_CLASS;

    attron(COLOR_PAIR(CP_BORDO) | A_BOLD);
    mvprintw(0, col, ".______________.");
    mvprintw(1, col, "|  CLASSIFICA  |");
    mvprintw(2, col, "|______________|");
    attroff(COLOR_PAIR(CP_BORDO) | A_BOLD);

    sem_wait(&sem_mappa);
    for (int i = 0; i < stato->num_robot; i++) {
        Robot *r = &stato->robot[i];
        int riga = 3 + i * 3;

        int cp_robot = colore_robot(r->simbolo);
        attron(COLOR_PAIR(cp_robot) | A_BOLD);
        mvprintw(riga, col, "| %c %-10s |", r->simbolo,
                 r->vivo ? NOMI_ROBOT[i] : "DISTRUTTO ");
        attroff(COLOR_PAIR(cp_robot) | A_BOLD);

        if (r->vivo) {
            int barre = r->hp / 10;
            attron(COLOR_PAIR(colore_hp(r->hp)));
            mvprintw(riga + 1, col, "| HP:");
            for (int b = 0; b < 10; b++) {
                addch(b < barre ? '#' : '.');
            }
            printw(" %-3d |", r->hp);
            attroff(COLOR_PAIR(colore_hp(r->hp)));

            if (r->scudo > 0) {
                attron(COLOR_PAIR(CP_SCUDO));
                mvprintw(riga + 2, col, "|  [SCUDO x%d]   |", r->scudo);
                attroff(COLOR_PAIR(CP_SCUDO));
            } else {
                mvprintw(riga + 2, col, "|______________|");
            }
        } else {
            mvprintw(riga + 1, col, "|              |");
            mvprintw(riga + 2, col, "|______________|");
        }
    }
    sem_post(&sem_mappa);

    int riga_fine = 3 + stato->num_robot * 3;
    mvprintw(riga_fine, col, "|______________|");
}

void disegna_eventi(const char *messaggio) {
    pthread_mutex_lock(&mtx_evento);

    attron(COLOR_PAIR(CP_EVENTO));
    mvprintw(RIGA_EVENTO, 0, "[ EVENTO ] %-*s", COLONNE - 12, messaggio);
    attroff(COLOR_PAIR(CP_EVENTO));

    pthread_mutex_unlock(&mtx_evento);
}

/* ─────────────────────────────────────────────
 * Thread grafico
 * ───────────────────────────────────────────── */

 void *thread_grafico(void *arg) {
    ArgGrafica *args  = (ArgGrafica *)arg;
    StatoGioco *stato = args->stato;

    while (!stato->partita_finita) {
        /* Permette uscita anticipata con il tasto 'q' */
        int tasto = getch();
        if (tasto == 'q' || tasto == 'Q') {
            stato->partita_finita = true;
            stato->vincitore = -1;
            break;
        }

        /* Ridisegna */
        erase();
        disegna_mappa(stato);
        disegna_classifica(stato);

        /* Chiamata pulita: il mutex viene preso direttamente dentro la funzione */
        disegna_eventi(stato->ultimo_evento);

        refresh();

        /* ~10 aggiornamenti al secondo */
        usleep(100000);
    }

    /* Schermata finale */
    erase();
    disegna_mappa(stato);
    disegna_classifica(stato);

    attron(COLOR_PAIR(CP_HP_OK) | A_BOLD);
    if (stato->vincitore >= 0) {
        mvprintw(RIGA_EVENTO, 0,
                 "VINCITORE: %s (%c)  --  Premi un tasto per uscire.",
                 NOMI_ROBOT[stato->vincitore],
                 SIMBOLI_ROBOT[stato->vincitore]);
    } else {
        mvprintw(RIGA_EVENTO, 0, "Nessun vincitore. Premi un tasto per uscire.");
    }
    attroff(COLOR_PAIR(CP_HP_OK) | A_BOLD);

    refresh();
    nodelay(stdscr, FALSE);
    getch();

    return NULL;
}
