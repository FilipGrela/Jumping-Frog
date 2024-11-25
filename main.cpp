#include <iostream>
#include<time.h>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>
using namespace std;

#define PROJECT_NAME "Jumping Frog"
#define BOARD_WIDTH (int)100
#define BOARD_HEIGHT (int)26
#define DELAY 1000

int getRandomNumber(int min, int max, unsigned int user_seed) {
    // Taking current time as seed
    unsigned int seed = time(0) * user_seed;
    return  rand_r(&seed) % (max - min + 1) + min;

}

struct Obstacle {
    enum ObstacleType {
        CAR
    };

    int y;
    int x;
    int speed;
    char skin = 'X';
    ObstacleType type;

    void initObstacle(int x, int y, int speed, ObstacleType type) {
        this->y = y;
        this->x = x;
        this->speed = speed;
        this->type = type;

        if (y < 0 || y > BOARD_HEIGHT || x < 0 || x > BOARD_WIDTH) {
            throw runtime_error("Blad podczas inicjalizacji, obiekt obstacle poza oknem!");
        }
    }
};

struct Frog {
    const char skin = 'O';
    int x;
    int y;

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
    int end = 0;
    int level;
    Frog frog;
    int obstacleCount = 0;
    Obstacle obstacle[10];  //TODO: dynamiczna alokacja miejsca

    void addObstacle(Obstacle obstacle) {
        this->obstacle[obstacleCount] = obstacle;
        obstacleCount++;
    }
};

void initGame(Game *game) {
    game->level = 1;
    game->frog.x = BOARD_WIDTH/2;
    game->frog.y = BOARD_HEIGHT;
}

void draw(WINDOW *win, Game game) {
    wclear(win);

    // Box and board setup
    box(win, 0, 0);
    mvwprintw(win, 0, (BOARD_WIDTH-sizeof(PROJECT_NAME)/sizeof(char))/2 , PROJECT_NAME);
    mvwprintw(win, BOARD_HEIGHT+1, 4*BOARD_WIDTH/5, " Level: %d ", game.level);

    if (game.end) {
        mvwprintw(win, BOARD_HEIGHT/2, (BOARD_WIDTH - 9)/2, "Game Over");
    }else {
        // Player
        mvwprintw(win, game.frog.y, game.frog.x-1, "%c",game.frog.skin);

        // Obstacle
        for (int i = 0; i < game.obstacleCount; i++) {
            Obstacle obstacle = game.obstacle[i];
            mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
        }
    }
    wrefresh(win);
    usleep(DELAY);
};

void cursesInit() {
    initscr();
    noecho();
    cbreak();
    timeout(DELAY);
    curs_set(FALSE);
    clear();
    refresh();
}

void levelInit(Game *game) {
    Obstacle test;
    for (int i = 0; i < 10; i++) {
        test.initObstacle(getRandomNumber(1, BOARD_WIDTH, i), getRandomNumber(1,BOARD_HEIGHT, i), 2, Obstacle::CAR);
        game->addObstacle(test);
    }
}


/** @brief Function checks if frog collides with obstacle
 *
 *
 *  @return true if frog collides with obstacle
 */
bool checkCollision(Game game) {

    for (int i = 0; i < game.obstacleCount; i++) {
        Obstacle obstacle = game.obstacle[i];
        if (obstacle.y == game.frog.y && obstacle.x+1 == game.frog.x) {
            return true;
        }
    }

    return false;
}


int main(int argc, char *argv[]) {
    cursesInit();

    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win = newwin(BOARD_HEIGHT+2, BOARD_WIDTH+2, (terminal.height-BOARD_HEIGHT-2)/2, (terminal.width-BOARD_WIDTH-2)/2);
    keypad(win, TRUE);

    Game game;
    initGame(&game);
    draw(win, game);

    levelInit(&game);
    draw(win, game);

    int input_b;

    while (!game.end) {
        input_b = wgetch(win);

        if (input_b == 'w' || input_b == KEY_UP) {
            game.frog.moveUp();
        } else if (input_b == 's' || input_b == KEY_DOWN) {
            game.frog.moveDown();
        } else if (input_b == 'a' || input_b == KEY_LEFT) {
            game.frog.moveLeft();
        }else if (input_b == 'd' || input_b == KEY_RIGHT) {
            game.frog.moveRight();
        } else if (input_b == 'q' || input_b == KEY_EXIT || input_b == 27) {
            game.end = 1;
        }

        if (checkCollision(game)) {
            game.end = 1;
        }

        draw(win, game);
    }


    wgetch(win);
    endwin();
    return 0;
}