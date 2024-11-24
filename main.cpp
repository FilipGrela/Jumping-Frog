#include <iostream>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
using namespace std;

#define PROJECT_NAME "Jumping Frog"
#define WINDOW_WIDTH (int)100
#define WINDOW_HEIGHT (int)26

typedef struct Frog {
    char skin = 'O';
    int x;
    int y;

    char getFrog() {
        return skin;
    }
};

typedef struct Game {
    int level;
    Frog frog;
};

void initGame(Game *game) {
    game->level = 1;
    game->frog.x = WINDOW_WIDTH/2;
    game->frog.y = WINDOW_HEIGHT;
}

void draw(WINDOW *win, Game game) {
    box(win, 0, 0);
    mvwprintw(win, 0, (WINDOW_WIDTH-sizeof(PROJECT_NAME)/sizeof(char))/2, PROJECT_NAME);
    mvwprintw(win, WINDOW_HEIGHT+1, 4*WINDOW_WIDTH/5, " Level: %d ", game.level);

    mvwprintw(win, game.frog.y, game.frog.x, "%c",game.frog.getFrog());
};


int main(int argc, char *argv[])
{
    initscr();
    noecho();
    curs_set(false);

    typedef struct Terminal{
        int width = 0;
        int height = 0;
    };
    Terminal terminal;

    Game game;
    initGame(&game);

    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win = newwin(WINDOW_HEIGHT+2, WINDOW_WIDTH+2, (terminal.height-WINDOW_HEIGHT-2)/2, (terminal.width-WINDOW_WIDTH-2)/2);

    draw(win, game);


    wgetch(win);
    endwin();
    return 0;
}