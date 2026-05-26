#define _XOPEN_SOURCE 600

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

#define NUM_VOCI        4
#define LARGH_MENU     13

static const char *TITOLO[] = {
    "   ____      _           _   ____              ",
    "  / ___|___ | |__   ___ | |_|  _ \\ ___  _ __   ",
    " | |   / _ \\| '_ \\ / _ \\| __| |_) / _ \\| '_ \\  ",
    " | |__| (_) | |_) | (_) | |_|  _ < (_) | |_) | ",
    "  \\____\\___/|_.__/ \\___/ \\__|_| \\_\\___/| .__/  ",
    "                                        |_|    ",
    "                                               "
};
#define TITOLO_RIGHE 7

static void init_mappa(StatoGioco *stato);
static void init_robot(StatoGioco *stato);
static void avvia_partita(void);
static int  menu_principale(void);

static void init_mappa(StatoGioco *stato) {
    for (int r = 0; r < RIGHE; r++)
        for (int c = 0; c < COLONNE; c++)
            stato->mappa[r][c] = CELLA_LIBERA;

    for (int c = 0; c < COLONNE; c++) {
        stato->mappa[0][c]        = CELLA_BORDO;
        stato->mappa[RIGHE - 1][c] = CELLA_BORDO;
    }
    for (int r = 0; r < RIGHE; r++) {
        stato->mappa[r][0]          = CELLA_BORDO;
        stato->mappa[r][COLONNE - 1] = CELLA_BORDO;
    }

    int ostacoli[][2] = {
        {5, 15}, {5, 35}, {10, 25}, {15, 10}, {15, 40}
    };
    for (int i = 0; i < 5; i++)
        stato->mappa[ostacoli[i][0]][ostacoli[i][1]] = CELLA_OSTACOLO;
}

static void init_robot(StatoGioco *stato) {
    int posizioni[MAX_ROBOT][2] = {
        {1,  1},
        {1,  COLONNE-2},
        {RIGHE/2, COLONNE/2},
        {RIGHE-2, 1},
        {RIGHE-2, COLONNE-2}
    };

    int velocita[MAX_ROBOT] = {
        200000,
        350000,
        250000,
        500000,
        150000
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

        stato->mappa[r->x][r->y] = r->simbolo;
    }
}

static void avvia_partita(void) {
    StatoGioco *stato = calloc(1, sizeof(StatoGioco));
    if (!stato) { endwin(); perror("calloc"); exit(1); }

    stato->partita_finita = false;
    stato->vincitore      = -1;
    strncpy(stato->ultimo_evento, "La battaglia sta per iniziare!",
            sizeof(stato->ultimo_evento) - 1);

    sem_init(&sem_mappa, 0, 1);
    init_mappa(stato);
    init_robot(stato);

    ArgRobot args_robot[MAX_ROBOT];
    for (int i = 0; i < MAX_ROBOT; i++) {
        args_robot[i].stato = stato;
        args_robot[i].id    = i;
    }

    ArgGrafica args_grafica;
    args_grafica.stato      = stato;

    ArgEventi args_eventi;
    args_eventi.stato      = stato;

    pthread_t t_robot[MAX_ROBOT];
    pthread_t t_eventi;

    pthread_create(&t_eventi, NULL, thread_eventi, &args_eventi);

    for (int i = 0; i < MAX_ROBOT; i++)
        pthread_create(&t_robot[i], NULL, thread_robot, &args_robot[i]);

    curs_set(0);
    nodelay(stdscr, TRUE); 
    init_colori();

    thread_grafico(&args_grafica);

    stato->partita_finita = true;

    for (int i = 0; i < MAX_ROBOT; i++)
        pthread_join(t_robot[i], NULL);

    pthread_join(t_eventi, NULL);

    nodelay(stdscr, FALSE);

    sem_destroy(&sem_mappa);
    free(stato);
}

static void schermata_comandi(void) {
    clear();
    int cx = COLS / 2;
    attron(A_BOLD);
    mvprintw(5,  cx - 7, "--- COMANDI ---");
    attroff(A_BOLD);
    mvprintw(8,  cx - 21, "La simulazione è completamente automatica:");
    mvprintw(10, cx - 40, "- %d robot combattono in simultanea su thread separati.", MAX_ROBOT);
    mvprintw(11, cx - 40, "- Ogni robot ha una personalità diversa (Aggressivo, Prudente, Casuale, Difensivo, Kamikaze).");
    mvprintw(12, cx - 40, "- Power-up (+ energia, S scudo) compaiono casualmente.");
    mvprintw(13, cx - 40, "- Le mine (M) esplodono al contatto.");
    mvprintw(14, cx - 40, "- Gli accessi alla mappa sono protetti da un semaforo.");
    mvprintw(15, cx - 40, "- Gli eventi vengono trasmessi via ZeroMQ PUB/SUB.");
    mvprintw(18, cx - 22, "Premi un tasto qualsiasi per tornare al menu.");
    refresh();
    getch();
    clear();
}

static void schermata_crediti(void) {
    clear();
    int cx = COLS / 2;
    attron(A_BOLD | COLOR_PAIR(2));
    mvprintw(5, cx - 7, "--- CREDITI ---");
    attroff(A_BOLD | COLOR_PAIR(2));
    mvprintw(8,  cx - 4, "CobotRop");
    mvprintw(10, cx - 22, "Andrea Mazzoli, Daniele Matranga & Saad Zahir");
    mvprintw(13, cx - 17, "Tecnologie usate:");
    mvprintw(14, cx - 13, "- POSIX Threads (pthread)");
    mvprintw(15, cx - 13, "- Semafori POSIX (sem_t)");
    mvprintw(16, cx - 13, "- ncurses");
    mvprintw(20, cx - 22, "Premi un tasto qualsiasi per tornare al menu.");
    refresh();
    getch();
    clear();
}

static int menu_principale(void) {
    const char *voci[] = { "  Gioca  ", " Comandi ", " Crediti ", "  Esci  " };
    int sel = 0;

    while (1) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        int col_titolo = (cols - 47) / 2;
        attron(A_BOLD | COLOR_PAIR(3));
        for (int i = 0; i < TITOLO_RIGHE; i++)
            mvprintw(2 + i, col_titolo, "%s", TITOLO[i]);
        attroff(A_BOLD | COLOR_PAIR(3));

        int riga_menu = (rows - NUM_VOCI) / 2 + 4;
        int col_menu  = (cols - LARGH_MENU) / 2;
        for (int i = 0; i < NUM_VOCI; i++) {
            if (i == sel) attron(A_REVERSE | A_BOLD);
            mvprintw(riga_menu + i, col_menu, "  %-*s  ", LARGH_MENU - 4, voci[i]);
            if (i == sel) attroff(A_REVERSE | A_BOLD);
        }

        mvprintw(rows - 2, (cols - 57) / 2, "Usa KEY_UP/KEY_DOWN per navigare, INVIO per selezionare");
        refresh();

        int k = getch();
        if (k == KEY_DOWN) sel = (sel + 1) % NUM_VOCI;
        else if (k == KEY_UP) sel = (sel + NUM_VOCI - 1) % NUM_VOCI;
        else if (k == '\n' || k == KEY_ENTER) {
            switch (sel) {
                case 0: return 1;
                case 1: schermata_comandi(); break;
                case 2: schermata_crediti(); break;
                case 3: return 0;
            }
        }
    }
}

int main(void) {
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
