#include <iostream>
#include<time.h>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>
using namespace std;

#define PROJECT_NAME "Jumping Frog"
#define BOARD_WIDTH (int)100
#define BOARD_HEIGHT (int)26
#define DELAY 100

int getRandomNumber(int min, int max) {
    return rand() % (max + 1 - min) + min;

}

struct Obstacle {
    enum ObstacleType {
        CAR,
        CACTUS
    };

    int y;
    int x;
    int speed;
    char skin = 'X';
    ObstacleType type;
};

struct Frog {
    const char skin = 'O';
    int x;
    int y;
};

struct Game {
    int end = 0;
    int level;
    Frog frog;
    int obstacleCount = 0;
    Obstacle obstacle[10];  //TODO: dynamiczna alokacja miejsca
};

void addObstacle(Game *game, Obstacle obstacle) {
    game->obstacle[game->obstacleCount] = obstacle;
    game->obstacleCount++;
}

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

    // stationary
    for (int i = 0; i < 5; i++) {
        Obstacle test;
        test.x = getRandomNumber(1, BOARD_WIDTH);
        test.y = getRandomNumber(1,BOARD_HEIGHT-1);
        test.type = Obstacle::CACTUS;
        test.speed = 0;

        addObstacle(game, test);
    }

    // Moving ones
    for (int i = 0; i < 5; i++) {
        Obstacle test;
        test.x = getRandomNumber(1, BOARD_WIDTH);
        test.y = getRandomNumber(1,BOARD_HEIGHT);
        test.speed = 1;
        test.skin = '*';
        test.type = Obstacle::CAR;

        addObstacle(game, test);
    }

    // test.x = 50;
    // test.y = 20;
    // test.speed = 1;
    // test.skin = '*';
    // test.type = Obstacle::CAR;
    //
    // addObstacle(game, test);
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


void moveObstacles(Game *game) {
    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle *obstacle = &(game->obstacle[i]);
        if (obstacle->type == Obstacle::CAR) {
            if (obstacle->x <= 1 || obstacle->x >= BOARD_WIDTH) obstacle->speed *= -1;
            obstacle->x += obstacle->speed;
        }
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    cursesInit();

    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win = newwin(BOARD_HEIGHT+2, BOARD_WIDTH+2, (terminal.height-BOARD_HEIGHT-2)/2, (terminal.width-BOARD_WIDTH-2)/2);
    keypad(win, TRUE);
    keypad(stdscr, TRUE);

    Game game;
    initGame(&game);
    draw(win, game);

    levelInit(&game);
    draw(win, game);

    int input_b;

    while (!game.end) {
        input_b = getch();

        if (input_b == 'w' || input_b == KEY_UP) {

            if (game.frog.y > 1)
                game.frog.y--;
        } else if (input_b == 's' || input_b == KEY_DOWN) {
            if (game.frog.y < BOARD_HEIGHT)
                game.frog.y++;
        } else if (input_b == 'a' || input_b == KEY_LEFT) {
            if (game.frog.x > 1)
                game.frog.x--;
        }else if (input_b == 'd' || input_b == KEY_RIGHT) {
            if (game.frog.x < BOARD_WIDTH)
                game.frog.x++;
        } else if (input_b == 'q' || input_b == KEY_EXIT || input_b == 27) {
            game.end = 1;
        }

        moveObstacles(&game);

        if (checkCollision(game)) {
            game.end = 1;
        }
        draw(win, game);
    }


    wgetch(win);
    endwin();
    return 0;
}