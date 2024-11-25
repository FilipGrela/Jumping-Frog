#include <iostream>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>
using namespace std;

#define PROJECT_NAME "Jumping Frog"
#define BOARD_WIDTH (int)100
#define BOARD_HEIGHT (int)26
#define DELAY 1000

struct Frog {
    char skin = 'O';
    int x;
    int y;

    char getFrog() {
        return skin;
    }

    void moveUp(){
        if (y > 1)
            y--;
    }

    void moveDown() {
        if (y < BOARD_HEIGHT)
            y++;
    }

    void moveLeft() {
        if (x > 1)
            x--;
    }
    void moveRight() {
        if (x < BOARD_WIDTH)
            x++;
    }
};

struct Game {
    int level;
    Frog frog;
};

void initGame(Game *game) {
    game->level = 1;
    game->frog.x = BOARD_WIDTH/2;
    game->frog.y = BOARD_HEIGHT;
}

void draw(WINDOW *win, Game game) {

    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 0, (BOARD_WIDTH-sizeof(PROJECT_NAME)/sizeof(char))/2 , PROJECT_NAME);
    mvwprintw(win, BOARD_HEIGHT+1, 4*BOARD_WIDTH/5, " Level: %d ", game.level);

    mvwprintw(win, game.frog.y, game.frog.x, "%c",game.frog.getFrog());

    wrefresh(win);
    usleep(DELAY);
};

void cursesInit() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    cbreak();
    timeout(DELAY);
    curs_set(FALSE);
    clear();
    refresh();
}


int main(int argc, char *argv[]) {
    cursesInit();

    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    Game game;

    initGame(&game);

    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win = newwin(BOARD_HEIGHT+2, BOARD_WIDTH+2, (terminal.height-BOARD_HEIGHT-2)/2, (terminal.width-BOARD_WIDTH-2)/2);
    draw(win, game);


    int input_b;
    while (1) {
        input_b = wgetch(win);

        if (input_b == 'w' || input_b == KEY_UP) {
            game.frog.moveUp();
        } else if (input_b == 's' || input_b == KEY_DOWN) {
            game.frog.moveDown();
        } else if (input_b == 'a' || input_b == KEY_LEFT) {
            game.frog.moveLeft();
        }else if (input_b == 'd' || input_b == KEY_RIGHT) {
            game.frog.moveRight();
        }


        draw(win, game);
    }

    endwin();
    return 0;
}