#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#define Altezza 25
#define Larghezza 50
#define num_voci 4

int larghezza_menu = 15;

char titleScreen[8][Larghezza + 1] = {
    "   _____      _           _     _____             ",
    "  / ____|    | |         | |   |  __ \\            ",
    " | |     ___ | |__   ___ | |_  | |__) |___  _ __  ",
    " | |    / _ \\| '_ \\ / _ \\| __| |  _  // _ \\| '_ \\ ",
    " | |___| (_) | |_) | (_) | |_  | | \\ \\ (_) | |_) |",
    "  \\_____\\___/|_.__/ \\___/ \\__| |_|  \\_\\___/| .__/ ",
    "                                           | |    ",
    "                                           |_|    "
};
void draw_title() {
    for (int i = 0; i < Altezza; i++) {
        mvprintw(10 + i, 67 , titleScreen[i]);
    }
}

int Gioca() {
    bool exit = false;
    return exit;
 }

void Comandi() {
     clear();
     refresh();
     attron(COLOR_PAIR(1)); // Assicurati che il testo sia visibile sullo sfondo bianco
     mvprintw(18, (COLS - 15) / 2, "--- Comandi ---"); // Centra il titolo
     mvprintw(20, (COLS - 30) / 2, "E' una simlazione fratello"); // Centra i nomi
     attroff(COLOR_PAIR(1));
     getch(); // Attendi la pressione di un tasto
     clear();  // Pulisci lo schermo prima di tornare al menu
     refresh(); // Aggiorna lo schermo
 }

int Crediti() {
     clear();
     refresh();
     attron(COLOR_PAIR(1)); // Assicurati che il testo sia visibile sullo sfondo bianco
     mvprintw(18, (COLS - 15) / 2, "--- Crediti ---"); // Centra il titolo
     mvprintw(20, (COLS - 14) / 2, "Cobot Rop by:"); // Centra i nomi
     mvprintw(22, (COLS - 30) / 2, "Andrea Mazzoli e Daniele Matranga.");
     mvprintw(26, (COLS - 30) / 2, "Premi un tasto per tornare al menu."); // Istruzione per l'utente
     attroff(COLOR_PAIR(1));
     getch(); // Attendi la pressione di un tasto
     clear();  // Pulisci lo schermo prima di tornare al menu
     refresh(); // Aggiorna lo schermo
 }

int Esci() {
    bool exit = true;
    return exit;
 }

int scroll_menu(WINDOW **items, int count, int menu_start_col) {
     int key;
     int selected = 0;
     while (1) {
         key = getch();
         if (key == KEY_DOWN || key == KEY_UP) {
             wbkgd(items[selected + 1], COLOR_PAIR(2));
             wnoutrefresh(items[selected + 1]);
             if (key == KEY_DOWN) {
                 selected = (selected + 1) % count;
             } else {
                 selected = (selected + count - 1) % count;
             }
             wbkgd(items[selected + 1], COLOR_PAIR(1));
             wnoutrefresh(items[selected + 1]);
             doupdate();
         }
         // Aggiungi qui la logica per gestire la selezione (es. pressione di Enter)
         if (key == 10) {
             break; // Esci dal menu a scorrimento dopo la selezione
         }
     }
     return selected; // Restituisci l'indice dell'elemento selezionato
 }

char *menuItems[] = {
         "Gioca",
         "Comandi",
         "Crediti",
         "Esci",
         NULL
     };

int menu() {
    // ... (inizializzazione) ...
    int scelta_scroll = 0; // Inizializza la scelta corrente

    // ... (pulizia) ...

     // Se stai usando ncurses, devi inizializzarlo e de-inizializzarlo correttamente.
     initscr();
     cbreak();
     noecho();
     keypad(stdscr, TRUE);
     start_color();
     init_pair(1, COLOR_BLACK, COLOR_WHITE); // Testo nero, sfondo bianco per la selezione
     init_pair(2, COLOR_WHITE, COLOR_BLACK); // Testo bianco, sfondo nero per le altre voci
    do{
     // Imposta lo sfondo predefinito a bianco
     bkgd(COLOR_PAIR(1));
     refresh();

     WINDOW *menu_items[num_voci + 1]; // +1 per il bordo
     int y, x, i;
     bool in_menu = true; // Flag per controllare se siamo nel menu
    bool exit = false;

     getmaxyx(stdscr, y, x);
     int inizio_y = (y - num_voci - 1) / 2; // Centra verticalmente
     int inizio_x = (x - larghezza_menu) / 2; // Centra orizzontalmente

     WINDOW *bordi = newwin(num_voci + 2, larghezza_menu + 2, inizio_y, inizio_x);
     draw_title();
     wbkgd(bordi, COLOR_PAIR(1)); // Sfondo bianco per il bordo
     box(bordi, 0, 0);
     wnoutrefresh(bordi);

     for (i = 0; i < num_voci; i++) {
         menu_items[i + 1] = newwin(1, larghezza_menu, inizio_y + i + 1, inizio_x + 1);
         mvwprintw(menu_items[i + 1], 0, 1, "%s", menuItems[i]);
         wbkgd(menu_items[i + 1], COLOR_PAIR(2)); // Sfondo nero per le voci non selezionate
         wnoutrefresh(menu_items[i + 1]);
     }

     wbkgd(menu_items[1], COLOR_PAIR(1)); // Sfondo bianco per la prima voce (selezionata inizialmente)
     wnoutrefresh(menu_items[1]);
     doupdate();

    while (in_menu) {
         scelta_scroll = scroll_menu(menu_items, num_voci, inizio_x + 1);

         // Esegui l'azione in base alla scelta da scroll_menu
         if (scelta_scroll == 0) {
             exit = Gioca();
             return exit;
             clear(); // Pulisci lo schermo dopo Gioca
             refresh();
         } else if (scelta_scroll == 1) {
             Comandi();
         } else if (scelta_scroll == 2) {
             Crediti(); // La funzione Crediti ora pulisce lo schermo al ritorno
         } else if (scelta_scroll == 3) {
             in_menu = false; // Imposta il flag per uscire dal ciclo
             exit = Esci();
             return exit;
         }

         // Ridisegna il menu dopo essere tornati da un'opzione (tranne Esci)
    if(in_menu){
        draw_title();
        wbkgd(bordi, COLOR_PAIR(1)); // Sfondo bianco per il bordo
        box(bordi, 0, 0);
        wnoutrefresh(bordi);

        for (i = 0; i < num_voci; i++) {
            menu_items[i + 1] = newwin(1, larghezza_menu, inizio_y + i + 1, inizio_x + 1);
            mvwprintw(menu_items[i + 1], 0, 1, "%s", menuItems[i]);
            wbkgd(menu_items[i + 1], COLOR_PAIR(2)); // Sfondo nero per le voci non selezionate
            wnoutrefresh(menu_items[i + 1]);
        }

        wbkgd(menu_items[1], COLOR_PAIR(1)); // Sfondo bianco per la prima voce (selezionata inizialmente)
        wnoutrefresh(menu_items[1]);
        doupdate();
        }
    }
    // Pulisci ncurses
    delwin(bordi);
    for (i = 1; i <= num_voci; i++) {
        delwin(menu_items[i]);
    }
    }while(1);
 }

 int main(){
    int X, Y;
    bool win, exit, play;
    while(!exit){
        exit = menu();
        clear();
        refresh();

        }
    endwin();
    return 0;
}