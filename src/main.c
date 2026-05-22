/* Necessario per usleep() con -std=c11 */
#define _XOPEN_SOURCE 600

/*
 * main.c  –  CobotRop
 * Punto di ingresso del programma.
 *
 * Responsabilità:
 *  1. Menu principale ncurses (riutilizza il vostro codice originale)
 *  2. Inizializzazione della mappa e dei robot
 *  3. Creazione del context ZeroMQ e delle socket PUB/SUB
 *  4. Lancio dei thread: 1 grafico + 1 eventi + 1 per robot
 *  5. Attesa del termine e cleanup
 *
 * Pattern ZeroMQ usato:  inproc PUB/SUB
 *   - Thread robot    → pubblicano su PUB (eventi di gioco)
 *   - Thread eventi   → pubblica su PUB  (spawn power-up, ostacoli)
 *   - Thread grafico  → si iscrive su SUB (riceve tutti i messaggi)
 *
 * Nota: con inproc le socket devono usare lo stesso zmq_ctx.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>

#include "globals.h"
#include "robot.h"
#include "grafica.h"
#include "eventi.h"

/* ─────────────────────────────────────────────
 * Costanti di layout per il menu
 * ───────────────────────────────────────────── */
#define NUM_VOCI        4
#define LARGH_MENU     20

/* ─────────────────────────────────────────────
 * ASCII art titolo (adattata per CobotRop)
 * ───────────────────────────────────────────── */
static const char *TITOLO[] = {
    "   ____      _           _   ____            ",
    "  / ___|___ | |__   ___ | |_|  _ \\ ___  _ __ ",
    " | |   / _ \\| '_ \\ / _ \\| __| |_) / _ \\| '_ \\",
    " | |__| (_) | |_) | (_) | |_|  _ < (_) | |_) |",
    "  \\____\\___/|_.__/ \\___/ \\__|_| \\_\\___/| .__/ ",
    "                                        |_|    ",
    "          C-2026 EXTREME EDITION               "
};
#define TITOLO_RIGHE 7

/* ─────────────────────────────────────────────
 * Prototipi interni
 * ───────────────────────────────────────────── */
static void init_mappa(StatoGioco *stato);
static void init_robot(StatoGioco *stato);
static void avvia_partita(void);
static int  menu_principale(void);

/* ─────────────────────────────────────────────
 * Inizializzazione mappa
 * ───────────────────────────────────────────── */
static void init_mappa(StatoGioco *stato) {
    /* Riempie con celle libere */
    for (int r = 0; r < RIGHE; r++)
        for (int c = 0; c < COLONNE; c++)
            stato->mappa[r][c] = CELLA_LIBERA;

    /* Disegna i bordi */
    for (int c = 0; c < COLONNE; c++) {
        stato->mappa[0][c]        = CELLA_BORDO;
        stato->mappa[RIGHE - 1][c] = CELLA_BORDO;
    }
    for (int r = 0; r < RIGHE; r++) {
        stato->mappa[r][0]          = CELLA_BORDO;
        stato->mappa[r][COLONNE - 1] = CELLA_BORDO;
    }

    /* Piazza qualche ostacolo fisso per rendere l'arena più interessante */
    int ostacoli[][2] = {
        {5, 15}, {5, 35}, {10, 25}, {15, 10}, {15, 40}
    };
    for (int i = 0; i < 5; i++)
        stato->mappa[ostacoli[i][0]][ostacoli[i][1]] = CELLA_OSTACOLO;
}

/* ─────────────────────────────────────────────
 * Inizializzazione robot
 * ───────────────────────────────────────────── */
static void init_robot(StatoGioco *stato) {
    /* Posizioni di partenza distribuite ai 4 angoli + centro */
    int posizioni[MAX_ROBOT][2] = {
        {1,  1},          /* Aggressivo – angolo in alto a sinistra */
        {1,  COLONNE-2},  /* Prudente   – angolo in alto a destra  */
        {RIGHE/2, COLONNE/2}, /* Casuale – centro                  */
        {RIGHE-2, 1},     /* Difensivo  – angolo in basso a sinistra */
        {RIGHE-2, COLONNE-2}  /* Kamikaze – angolo in basso a destra */
    };

    /* Velocità (microsecondi tra un'azione e l'altra):
     * valori più bassi = robot più veloci */
    int velocita[MAX_ROBOT] = {
        200000,   /* Aggressivo: veloce */
        350000,   /* Prudente:   medio  */
        250000,   /* Casuale:    abbastanza veloce */
        500000,   /* Difensivo:  lento ma solido   */
        150000    /* Kamikaze:   rapidissimo        */
    };

    stato->num_robot = MAX_ROBOT;

    for (int i = 0; i < MAX_ROBOT; i++) {
        Robot *r        = &stato->robot[i];
        r->x            = posizioni[i][0];
        r->y            = posizioni[i][1];
        r->hp           = HP_INIZIALE;
        r->scudo        = 0;
        r->vivo         = true;
        r->simbolo      = SIMBOLI_ROBOT[i];
        r->tipo         = (TipoRobot)i;
        r->velocita     = velocita[i];
        r->mine_piazzate = 0;

        /* Posiziona il simbolo sulla mappa */
        stato->mappa[r->x][r->y] = r->simbolo;
    }
}

/* ─────────────────────────────────────────────
 * Avvio della partita (thread + ZMQ)
 * ───────────────────────────────────────────── */
static void avvia_partita(void) {
    /* ── Alloca e inizializza lo stato di gioco ── */
    StatoGioco *stato = calloc(1, sizeof(StatoGioco));
    if (!stato) { endwin(); perror("calloc"); exit(1); }

    stato->partita_finita = false;
    stato->vincitore      = -1;
    strncpy(stato->ultimo_evento, "La battaglia sta per iniziare!",
            sizeof(stato->ultimo_evento) - 1);

    /* Inizializza semaforo e mappa */
    sem_init(&sem_mappa, 0, 1);
    init_mappa(stato);
    init_robot(stato);

    /* ── Prepara argomenti thread ── */

    /* Thread robot */
    ArgRobot args_robot[MAX_ROBOT];
    for (int i = 0; i < MAX_ROBOT; i++) {
        args_robot[i].stato = stato;
        args_robot[i].id    = i;
    }

    /* Thread grafico */
    ArgGrafica args_grafica;
    args_grafica.stato      = stato;

    /* Thread eventi */
    ArgEventi args_eventi;
    args_eventi.stato      = stato;

    /* ── Lancia i thread secondari ── */
    pthread_t t_robot[MAX_ROBOT];
    pthread_t t_eventi;

    pthread_create(&t_eventi, NULL, thread_eventi, &args_eventi);

    for (int i = 0; i < MAX_ROBOT; i++)
        pthread_create(&t_robot[i], NULL, thread_robot, &args_robot[i]);

    /* ── Il thread MAIN gestisce ncurses
     * ncurses non è thread-safe: un solo thread alla volta può disegnare.
     * La soluzione corretta è far girare il loop grafico sul main thread,
     * e spostare robot/eventi su thread secondari.                       */
    curs_set(0);
    nodelay(stdscr, TRUE);   /* getch() non bloccante durante la partita */
    init_colori();

    /* Chiama direttamente la funzione grafica (bloccante fino a fine partita) */
    thread_grafico(&args_grafica);

    /* ── Attende i thread secondari ── */
    stato->partita_finita = true;   /* garanzia extra */

    for (int i = 0; i < MAX_ROBOT; i++)
        pthread_join(t_robot[i], NULL);

    pthread_join(t_eventi, NULL);

    /* Ripristina getch() bloccante per il menu */
    nodelay(stdscr, FALSE);

    sem_destroy(&sem_mappa);
    free(stato);
}

/* ─────────────────────────────────────────────
 * Schermate del menu
 * ───────────────────────────────────────────── */
static void schermata_comandi(void) {
    clear();
    int cx = COLS / 2;
    attron(A_BOLD);
    mvprintw(5,  cx - 8, "--- COMANDI ---");
    attroff(A_BOLD);
    mvprintw(8,  4, "La simulazione è completamente automatica:");
    mvprintw(10, 4, "- %d robot combattono in simultanea su thread separati.", MAX_ROBOT);
    mvprintw(11, 4, "- Ogni robot ha una personalità diversa (Aggressivo, Prudente,");
    mvprintw(12, 4, "  Casuale, Difensivo, Kamikaze).");
    mvprintw(13, 4, "- Power-up (+ energia, S scudo) compaiono casualmente.");
    mvprintw(14, 4, "- Le mine (M) esplodono al contatto.");
    mvprintw(15, 4, "- Gli accessi alla mappa sono protetti da un semaforo.");
    mvprintw(16, 4, "- Gli eventi vengono trasmessi via ZeroMQ PUB/SUB.");
    mvprintw(19, cx - 18, "Premi un tasto qualsiasi per tornare al menu.");
    refresh();
    getch();
    clear();
}

static void schermata_crediti(void) {
    clear();
    int cx = COLS / 2;
    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(5, cx - 9, "--- CREDITI ---");
    attroff(A_BOLD | COLOR_PAIR(2));
    mvprintw(8,  cx - 17, "CobotRop  –  C-2026 Extreme Edition");
    mvprintw(10, cx - 17, "Progetto TPSIT 4a superiore");
    mvprintw(12, cx - 17, "Andrea Mazzoli  &  Daniele Matranga");
    mvprintw(15, cx - 17, "Tecnologie usate:");
    mvprintw(16, cx - 13, "- POSIX Threads (pthread)");
    mvprintw(17, cx - 13, "- Semafori POSIX (sem_t)");
    mvprintw(19, cx - 13, "- ncurses");
    mvprintw(22, cx - 18, "Premi un tasto qualsiasi per tornare al menu.");
    refresh();
    getch();
    clear();
}

/* ─────────────────────────────────────────────
 * Menu principale (scroll con frecce)
 * ───────────────────────────────────────────── */
static int menu_principale(void) {
    const char *voci[] = { "  Gioca  ", " Comandi ", " Crediti ", "  Esci   " };
    int sel = 0;

    while (1) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        /* Titolo */
        int col_titolo = (cols - 47) / 2;
        attron(A_BOLD | COLOR_PAIR(3));
        for (int i = 0; i < TITOLO_RIGHE; i++)
            mvprintw(2 + i, col_titolo, "%s", TITOLO[i]);
        attroff(A_BOLD | COLOR_PAIR(3));

        /* Voci di menu */
        int riga_menu = (rows - NUM_VOCI) / 2 + 4;
        int col_menu  = (cols - LARGH_MENU) / 2;
        for (int i = 0; i < NUM_VOCI; i++) {
            if (i == sel) attron(A_REVERSE | A_BOLD);
            mvprintw(riga_menu + i, col_menu, "  %-*s  ", LARGH_MENU - 4, voci[i]);
            if (i == sel) attroff(A_REVERSE | A_BOLD);
        }

        mvprintw(rows - 2, (cols - 34) / 2, "Usa KEY_UP/KEY_DOWN per navigare, INVIO per selezionare");
        refresh();

        int k = getch();
        if (k == KEY_DOWN) sel = (sel + 1) % NUM_VOCI;
        else if (k == KEY_UP) sel = (sel + NUM_VOCI - 1) % NUM_VOCI;
        else if (k == '\n' || k == KEY_ENTER) {
            switch (sel) {
                case 0: return 1; /* Gioca */
                case 1: schermata_comandi(); break;
                case 2: schermata_crediti(); break;
                case 3: return 0; /* Esci */
            }
        }
    }
}

/* ─────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────── */
int main(void) {
    /* Inizializza ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colori();

    while(1){
        int gioca = menu_principale();
        if (gioca) {
            avvia_partita();
        } else {
            break;
        }
    }

    endwin();
    return EXIT_SUCCESS;
}
