#include <iostream>
#include <ctime>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>

#define PROJECT_NAME "Jumping Frog"
// #define BOARD_WIDTH (int)60
// #define BOARD_HEIGHT (int)20
#define DELAY 60000

#define FROG_PAIR_COLOR 1
#define EXIT_PAIR_COLOR 2
#define COLOR_PAIR_SAFE 4
#define COLOR_PAIR_DANGER 5
#define COLOR_PAIR_CACTUS 6

#define CONFIG_PATH (char*) "../game-data.txt"

int board_width = 0;
int board_height = 0;

int getRandomNumber(int min, int max) {
    return rand() % (max + 1 - min) + min;

}

enum ObstacleType {
    NONE,
    CAR,
    CACTUS
};

struct Level {
    int height;
    int width;
    int cactus_num;
    int car_num;
};

struct Obstacle {
    int color_pair = COLOR_PAIR_DANGER;
    int y = -1;
    int x = -1;
    unsigned int speed = 0;
    int direction = 1;
    char skin = 'X';
    ObstacleType type = NONE;
};

struct Exit {
    int x = 1;
    int y = 1;
    char skin = '^';
};

struct Frog {
    const char skin = 'O';
    int x = 0;
    int y = 1;
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
    Level **levels;
    int level_count;
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
    wbkgd(win, COLOR_PAIR(COLOR_PAIR_SAFE));

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
        // Obstacle
        for (int i = 0; i < game.obstacleCount; i++) {
            Obstacle obstacle = game.obstacle[i];
            for (int x = 1; x <= board_width; x++) {
                wattron(win, COLOR_PAIR(obstacle.color_pair));
                mvwprintw(win, obstacle.y, x, " ");
            }
            mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
            wattron(win, COLOR_PAIR(obstacle.color_pair));

        }

        // Player
        wattron(win, COLOR_PAIR(EXIT_PAIR_COLOR));
        mvwprintw(win, game.exit.y, game.exit.x-1, "%c", game.exit.skin);
        wattroff(win, COLOR_PAIR(EXIT_PAIR_COLOR));

        wattron(win, COLOR_PAIR(FROG_PAIR_COLOR));
        mvwprintw(win, game.frog.y, game.frog.x-1, "%c",game.frog.skin);
        wattroff(win, COLOR_PAIR(FROG_PAIR_COLOR));
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
    //                          font color   bg color
    init_pair(FROG_PAIR_COLOR, COLOR_BLACK, COLOR_GREEN);
    init_pair(EXIT_PAIR_COLOR, COLOR_WHITE, COLOR_GREEN);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);

    init_pair(COLOR_PAIR_SAFE, COLOR_GREEN, COLOR_BLUE);
    init_pair(COLOR_PAIR_DANGER, COLOR_RED, COLOR_MAGENTA);

    init_pair(COLOR_PAIR_CACTUS, COLOR_RED, COLOR_YELLOW);
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



int *getLevelData(Game * game, int level) {

    int row_i = 0;

    Level current_level = *game->levels[level - 1];
    board_height = current_level.height;
    board_width = current_level.width;
    row_i = board_height - 3;

    int trap_rows[row_i];
    for (int i = 0; i < row_i; i++) {
        trap_rows[i] = -1;
    }
    fill_trap_rows(trap_rows,row_i, current_level.cactus_num, current_level.car_num);

    int * trap_rows_ptr =  (int *) malloc(sizeof(int) * (row_i));
    for (int i = 0; i < row_i; i++) {
        trap_rows_ptr[i] = trap_rows[i];
    }

    return trap_rows_ptr;
}

void levelInit(Game *game, int level) {
    int * trap_rows = getLevelData(game, level);
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
            obs.color_pair = COLOR_PAIR_DANGER;
            obs.skin = '*';
            obs.speed = getRandomNumber(1,3);

            addObstacle(game, obs);

        }else if (trap_rows[i] == CACTUS) {
            Obstacle obs;
            obs.x = getRandomNumber(2, board_width-1);
            obs.y = i+2;
            obs.type = CACTUS;
            obs.color_pair = COLOR_PAIR_CACTUS;
            obs.speed = 0;

            addObstacle(game, obs);
        }
    }

    free(trap_rows);
}


/** @brief Function checks if frog collides with obstacle
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

WINDOW *get_next_level(WINDOW **win, Game *game) {
    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle obstacle;
        game->obstacle[i] = obstacle;
    }
    game->obstacleCount = 0;

    levelInit(game, game->level);

    return (initWindow());
}

Level ** loadLevelsFile (char *fileName, int * level_count_p) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error opening file %s\n", fileName);
        // return '';
    }

    fscanf(file, "Level_count: %d", level_count_p);
    int level_count = *level_count_p;

    int lvl_width[level_count], lvl_height[level_count], lvl_car[level_count], lvl_cactus[level_count];

    Level **level_arr =  (Level **) malloc(sizeof(Level*) * (level_count));

    for (int i = 0; i < level_count; i++) {
        level_arr[i] = (Level *) malloc(sizeof(Level));
        fscanf(file, "%*s %d", &lvl_height[i]);
        fscanf(file, "%*s %d", &lvl_width[i]);
        fscanf(file, "%*s %d", &lvl_cactus[i]);
        fscanf(file, "%*s %d", &lvl_car[i]);

        level_arr[i]->height = lvl_height[i];
        level_arr[i]->width = lvl_width[i];
        level_arr[i]->car_num = lvl_cactus[i];
        level_arr[i]->cactus_num = lvl_car[i];
    }
    fclose(file);

    (Level(*)[128])level_arr;
    return level_arr;
}

void mem_free_lvl(Level *** level, int level_count) {
    for (int i = 0; i < level_count; i++) {
        free(level[i]);
    }
    free(level);
};

int main(int argc, char *argv[]) {
    srand(time(nullptr));
    cursesInit();
    WINDOW *win;

    Game game;
    game.levels = loadLevelsFile(CONFIG_PATH, &game.level_count);

    initGame(&game);

    levelInit(&game, game.level);
    win = initWindow();
    draw(win, game);

    int input_b;

    time_t forg_move_dt = 0;
    while (!game.end) {
        time_t startTime = time(nullptr);
        input_b = getch();

        if (input_b != -1)
            forg_move_dt = 0;

        if (forg_move_dt >= 5)
            game.end = true;

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
            if (game.level > 2) {
                game.end = true;
            }else {
                win = get_next_level(&win, &game);
                game.win = false;
            }
        }
        draw(win, game);

        forg_move_dt += time(nullptr) - startTime;
    }


    while (wgetch(win) != 'q') {}
    mem_free_lvl(&game.levels, game.level_count);
    endwin();
    return 0;
}

