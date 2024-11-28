#include <iostream>
#include <ctime>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>

#define PROJECT_NAME "Jumping Frog"
// #define BOARD_WIDTH (int)60
// #define BOARD_HEIGHT (int)20
#define DELAY 60000

int board_width = 0;
int board_height = 0;

int getRandomNumber(int min, int max) {
    return rand() % (max + 1 - min) + min;

}

enum ObstacleType {
    CAR,
    CACTUS
};

struct Obstacle {

    int y = -1;
    int x = -1;
    unsigned int speed;
    int direction = 1;
    char skin = 'X';
    ObstacleType type;
};

struct Exit {
    int x;
    int y;
    char skin = '^';
};

struct Frog {
    const char skin = 'O';
    int x;
    int y;
};

struct Game {
    int end = 0;
    bool win = false;
    int level;
    long start_time;
    long play_time;
    Exit exit;
    Frog frog;
    int obstacleCount = 0;
    Obstacle obstacle[50];  //TODO: dynamiczna alokacja miejsca
};

void addObstacle(Game *game, Obstacle obstacle) {
    game->obstacle[game->obstacleCount] = obstacle;
    game->obstacleCount++;
}

void initGame(Game *game) {
    game->start_time = time(nullptr);
    game->level = 1;
}

long getElapsedTime(const long *ts) {
    // clock_t end = clock();
    const long end = time(nullptr);
    return (end - *ts);
};

void draw(WINDOW *win, Game game) {
    werase(win);

    // Box and board setup
    box(win, 0, 0);
    mvwprintw(win, 0, (board_width-sizeof(PROJECT_NAME)/sizeof(char))/6 , PROJECT_NAME);
    mvwprintw(win, board_height+1, 1*board_width/10, " Level: %d ", game.level);
    mvwprintw(win, 0, 4*board_width/6, "Time: %ld", game.play_time);

    if (game.end) {
        if (game.win) {
            wattron(win, COLOR_PAIR(2));
            mvwprintw(win, board_height/2, (board_width - 7)/2, "You won!");
            mvwprintw(win, board_height/2+1, (board_width - 28)/2, "Your game took %ld seconds!", game.play_time);
            wattroff(win, COLOR_PAIR(2));
        }else {
            wattron(win, COLOR_PAIR(2));
            mvwprintw(win, board_height/2, (board_width - 9)/2, "Game Over");
            wattroff(win, COLOR_PAIR(2));
        }
        mvwprintw(win, board_height-2, (board_width - 15)/2, "Press Q to quit");
    }else {
        // Player

        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, game.exit.y, game.exit.x-1, "%c", game.exit.skin);
        wattroff(win, COLOR_PAIR(1));

        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, game.frog.y, game.frog.x-1, "%c",game.frog.skin);
        wattroff(win, COLOR_PAIR(1));
        // Obstacle
        for (int i = 0; i < game.obstacleCount; i++) {
            Obstacle obstacle = game.obstacle[i];
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
            wattroff(win, COLOR_PAIR(3));
        }
    }
    wrefresh(win);
    usleep(DELAY);
};

void cursesInit() {
    initscr();
    noecho();
    cbreak();
    timeout(FALSE);
    curs_set(FALSE);
    start_color();
    clear();
    refresh();

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_RED, COLOR_BLACK);
}

void fill_trap_rows(int * trap_rows, const int row_number, int cactus_row_num, int car_row_num){
    if (cactus_row_num + car_row_num > row_number) {
        throw std::out_of_range("Obstacle row number grater then row number");
    }

    // ustaw odpowiednia ilość kaktusów
    for (int i = 0; i < cactus_row_num; i++) {
        unsigned int row_num;
        do {
            row_num = getRandomNumber(0, row_number);
        } while (trap_rows[row_num] != -1);
        trap_rows[row_num] = CACTUS;
    }

    for (int i = 0; i < car_row_num; i++) {
        unsigned int row_num;
        do {
            row_num = getRandomNumber(0, row_number);
        } while (trap_rows[row_num] != -1);
        trap_rows[row_num] = CAR;
    }
}



int *getLevelData(int level) {

    int row_i = 0;

    if (level == 1) {
        board_height = 10;
        board_width = 35;
        row_i = board_height - 3;

        int trap_rows[row_i];
        for (int i = 0; i < row_i; i++) {
            trap_rows[i] = -1;
        }
        fill_trap_rows(trap_rows,row_i, 3, 2);

        int * trap_rows_ptr =  (int *) malloc(sizeof(int) * (row_i));
        for (int i = 0; i < row_i; i++) {
            trap_rows_ptr[i] = trap_rows[i];
        }

        return trap_rows_ptr;
    }else if (level == 2) {
        board_height = 32;
        board_width = 51;

        row_i = board_height - 3;
        int trap_rows[row_i];
        for (int i = 0; i < row_i; i++) {
            trap_rows[i] = -1;
        }
        fill_trap_rows(trap_rows,row_i, 8, 8);

        int * trap_rows_ptr =  (int *) malloc(sizeof(int) * (row_i));
        for (int i = 0; i < row_i; i++) {
            trap_rows_ptr[i] = trap_rows[i];
        }

        return  trap_rows_ptr;
    }

    return nullptr;
}

void levelInit(Game *game, int level) {
    int * trap_rows = getLevelData(level);
    int trap_row_i = board_height - 2;

    Exit exit;
    exit.x = board_width/2;
    exit.y = 1;
    game->exit = exit;

    game->frog.x = board_width/2;
    game->frog.y = board_height;


    for (int i = 0; i < trap_row_i; i++) {
        if (trap_rows[i] == CAR) {
            Obstacle obs;
            obs.x = getRandomNumber(2, board_width-1);
            obs.y = i+2;
            obs.type = CAR;
            obs.skin = '*';
            obs.speed = getRandomNumber(1,3);

            addObstacle(game, obs);

        }else if (trap_rows[i] == CACTUS) {
            Obstacle obs;
            obs.x = getRandomNumber(2, board_width-1);
            obs.y = i+2;
            obs.type = CACTUS;
            obs.speed = 0;

            addObstacle(game, obs);
        }
    }

    free(trap_rows);
}


/** @brief Function checks if frog collides with obstacle
 *
 *
 *  @return true if frog collides with obstacle
 */
void checkCollision(Game *game) {

    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle obstacle = game->obstacle[i];
        if (obstacle.y == game->frog.y && obstacle.x+1 == game->frog.x) {
            game->end = true;
        }
    }
}

bool checkWin(Game *game) {
    if (game->frog.x == game->exit.x && game->frog.y == game->exit.y) {
        game->win = true;
        game->level++;
        return true;
    }
    return false;
}


void moveObstacles(Game *game) {
    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle *obstacle = &(game->obstacle[i]);
        if (obstacle->type == CAR) {
            for (int j = 0; j < obstacle->speed; j++) {
                if (obstacle->x <= 1 || obstacle->x >= board_width) obstacle->direction *=-1;
                obstacle->x += 1*obstacle->direction;
                checkCollision(game);
            }
        }
    }
}

WINDOW * initWindow() {
    clear();
    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win = newwin(board_height+2, board_width+2, (terminal.height-board_height-2)/2, (terminal.width-board_width-2)/2);
    keypad(win, TRUE);
    keypad(stdscr, TRUE);
    return win;
};

int main(int argc, char *argv[]) {

    srand(time(nullptr));
    cursesInit();
    WINDOW *win;

    Game game;
    initGame(&game);

    levelInit(&game, game.level);
    win = initWindow();
    draw(win, game);

    int input_b;

    while (!game.end) {
        input_b = getch();

        if (input_b == 'w' || input_b == KEY_UP) {
            if (game.frog.y > 1)
                game.frog.y--;
        } else if (input_b == 's' || input_b == KEY_DOWN) {
            if (game.frog.y < board_height)
                game.frog.y++;
        } else if (input_b == 'a' || input_b == KEY_LEFT) {
            if (game.frog.x > 2)
                game.frog.x--;
        }else if (input_b == 'd' || input_b == KEY_RIGHT) {
            if (game.frog.x < board_width)
                game.frog.x++;
        } else if (input_b == 'q' || input_b == KEY_EXIT || input_b == 27) {
            game.end = 1;
        }


        moveObstacles(&game);
        checkWin(&game);
        game.play_time = getElapsedTime(&game.start_time);

        if (game.win) {
            game.end = 1;
        }
        draw(win, game);
    }


    while (wgetch(win) != 'q') {}
    endwin();
    return 0;
}
