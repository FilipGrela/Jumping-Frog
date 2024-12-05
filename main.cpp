#define TEST true

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ncurses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>
#include <stdlib.h>

#define PROJECT_NAME "Jumping Frog"
#define DELAY 60000

#define COLOR_P_FROG 1
#define COLOR_P_EXIT 2
// #define COLOR_P_SAFE
#define COLOR_P_DANGER 3
#define COLOR_P_CAR_STOPPABLE 4
#define COLOR_P_CAR_FRIENDLY 5
#define COLOR_P_CACTUS 6
#define COLOR_P_TITLE_WIN 7
#define COLOR_P_TITLE_LOOSE 8
#define COLOR_P_BACKGROUND 9

#define SCOREBOARD_SIZE 5

#define CONFIG_PATH (char*) "../game-data.txt"

enum ObstacleType {
    NONE,
    CAR,
    CAR_STOPPABLE,
    CAR_FRIENDLY,
    CACTUS
};

struct Level {
    int height;
    int width;
    int cactus_num;
    int car_num;
};

struct Coordinate {
    int x;
    int y;
};

struct Obstacle {
    int color_pair = COLOR_P_DANGER;
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
    int level{};
    long start_time{};
    long play_time{};
    Exit exit;
    Frog frog;
    int obstacleCount = 0;
    Obstacle obstacle[50];  //TODO: dynamiczna alokacja miejsca
    Level **levels{};
    int level_count{};
    int board_height{};
    int board_width{};
    int *scores;
};

int getRandomNumber(int min, int max) {
    return rand() % (max + 1 - min) + min;

}

void addObstacle(Game *game, const Obstacle obstacle) {
    game->obstacle[game->obstacleCount] = obstacle;
    game->obstacleCount++;
}

void initGame(Game *game) {
    game->start_time = time(nullptr);
    game->level = 1;
}

long getElapsedTime(const long *ts) {
    const long end = time(nullptr);
    return (end - *ts);
};

void drawError(char msg[]) {
    Coordinate coord{};
    getmaxyx(stdscr, coord.y, coord.x);
    wattron(stdscr, COLOR_PAIR(COLOR_P_TITLE_LOOSE));
    mvwprintw(stdscr, 1, (coord.x - 7)/2, "%s", msg);
    wattroff(stdscr, COLOR_PAIR(COLOR_P_TITLE_LOOSE));
}

void drawCommunicates(WINDOW * win, bool win_game,  int board_height, int board_width, long play_time) {
    if (win_game) {
        wattron(win, COLOR_PAIR(COLOR_P_TITLE_WIN));
        mvwprintw(win, board_height/2, (board_width - 7)/2, "You won!");
        mvwprintw(win, board_height/2+1, (board_width - 28)/2, "Your game took %ld seconds!", play_time);
        wattroff(win, COLOR_PAIR(COLOR_P_TITLE_WIN));
    }else {
        wattron(win, COLOR_PAIR(COLOR_P_TITLE_LOOSE));
        mvwprintw(win, board_height/2, (board_width - 9)/2, "Game Over");
        wattroff(win, COLOR_PAIR(COLOR_P_TITLE_LOOSE));
    }
    mvwprintw(win, board_height-2, (board_width - 15)/2, "Press Q to quit");
};

WINDOW * initWindowCoords(int board_height, int board_width, int start_x, int start_y) {
    WINDOW *win = newwin(board_height, board_width, start_x, start_y);

    keypad(win, TRUE);
    keypad(stdscr, TRUE);
    return win;
};

WINDOW * initWindowCentered(int board_height, int board_width) {
    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win =initWindowCoords(board_height+2, board_width+2, (terminal.height-board_height-2)/2, (terminal.width-board_width-2)/2);;

    return win;
}

void draw_scoreboard(const Game & game, int scores[SCOREBOARD_SIZE]) {
    Coordinate terminalSize;
    getmaxyx(stdscr, terminalSize.y, terminalSize.x);
    WINDOW *win_scoreboard = initWindowCoords(SCOREBOARD_SIZE + 2, 15, (terminalSize.y - game.board_height-2)/2,  (terminalSize.x - game.board_width-2)/2 - (SCOREBOARD_SIZE + 15));
    box(win_scoreboard, 0, 0);
    mvwprintw(win_scoreboard, 0, 1, "Place");
    mvwprintw(win_scoreboard, 0, 9, "Score");

    for (int i = 1; i < SCOREBOARD_SIZE+1; i++) {
        mvwprintw(win_scoreboard, i, 1, " %d.   | %d", i, scores[i-1]);
    }

    wrefresh(win_scoreboard);
};

void draw(WINDOW *win, const Game game) {
    draw_scoreboard(game, game.scores);
    werase(win);
    wbkgd(win, COLOR_PAIR(COLOR_P_BACKGROUND));

    // Box and board setup
    box(win, 0, 0);
    mvwprintw(win, 0, (game.board_height-sizeof(PROJECT_NAME)/sizeof(char))/6 , PROJECT_NAME);
    mvwprintw(win, game.board_height+1, 1*game.board_width/10, " Level: %d ", game.level);
    mvwprintw(win, 0, 4*game.board_width/6, "Time: %ld", game.play_time);

    if (game.end) {
        drawCommunicates(win,game.win, game.board_height, game.board_width, game.play_time);
    }else {
        // Obstacle
        for (int i = 0; i < game.obstacleCount; i++) {
            Obstacle obstacle = game.obstacle[i];
            for (int x = 1; x <= game.board_width; x++) {
                wattron(win, COLOR_PAIR(obstacle.color_pair));
                mvwprintw(win, obstacle.y, x, " ");
            }
            mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
            wattron(win, COLOR_PAIR(obstacle.color_pair));

        }

        // Exit
        wattron(win, COLOR_PAIR(COLOR_P_EXIT));
        mvwprintw(win, game.exit.y, game.exit.x-1, "%c", game.exit.skin);
        wattroff(win, COLOR_PAIR(COLOR_P_EXIT));
        // Player
        wattron(win, COLOR_PAIR(COLOR_P_FROG));
        mvwprintw(win, game.frog.y, game.frog.x-1, "%c",game.frog.skin);
        wattroff(win, COLOR_PAIR(COLOR_P_FROG));
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

    //                      font color     bg color
    init_pair(COLOR_P_FROG, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOR_P_EXIT, COLOR_GREEN, COLOR_BLACK);
    // init_pair(COLOR_P_SAFE, COLOR_WHITE, COLOR_YELLOW);
    init_pair(COLOR_P_DANGER, COLOR_BLACK, COLOR_RED);
    init_pair(COLOR_P_CACTUS, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(COLOR_P_TITLE_WIN, COLOR_BLACK, COLOR_GREEN);
    init_pair(COLOR_P_TITLE_LOOSE, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_P_BACKGROUND, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_P_CAR_STOPPABLE, COLOR_WHITE, COLOR_RED);
    init_pair(COLOR_P_CAR_FRIENDLY, COLOR_GREEN, COLOR_RED);

}

void fill_trap_rows(int * trap_rows, const int row_number, int cactus_row_num, int car_row_num){
    if (cactus_row_num + car_row_num > row_number) {
        // drawError("Obstacle row number grater then row number!");
    }else {
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
}

int *getLevelData(Game * game, int level) {

    int row_i = 0;

    Level current_level = *game->levels[level - 1];
    game->board_height = current_level.height;
    game->board_width = current_level.width;
    row_i = game->board_height - 3;

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
    int trap_row_i = game->board_height - 2;

    Exit exit;
    exit.x = game->board_width/2;
    exit.y = 1;
    game->exit = exit;

    game->frog.x = game->board_width/2;
    game->frog.y = game->board_height;


    for (int i = 0; i < trap_row_i; i++) {
        if (trap_rows[i] == CAR) {
            Obstacle obs;
            obs.x = getRandomNumber(2, game->board_width-1);
            obs.y = i+2;
            obs.type = getRandomNumber(1,100) <= 50 ? CAR_STOPPABLE : CAR;
            obs.type = getRandomNumber(1,100) <= 50 ? CAR_STOPPABLE : CAR;

            int random_num = getRandomNumber(1,100);

            obs.color_pair = COLOR_P_DANGER;
            if (random_num < 33) {
                obs.type = CAR;
            } else if (33<=random_num && random_num<=66) {
                obs.type = CAR_STOPPABLE;
                obs.color_pair = COLOR_P_CAR_STOPPABLE;
            } else if (random_num > 66) {
                obs.type = CAR_FRIENDLY;
                obs.color_pair = COLOR_P_CAR_FRIENDLY;
            }

            obs.skin = '*';
            obs.speed = getRandomNumber(1,3);
            addObstacle(game, obs);

        }else if (trap_rows[i] == CACTUS) {
            Obstacle obs;
            obs.x = getRandomNumber(2, game->board_width-1);
            obs.y = i+2;
            obs.type = CACTUS;
            obs.skin = 'X';
            obs.color_pair = COLOR_P_CACTUS;

            addObstacle(game, obs);
        }
    }

    free(trap_rows);
}

WINDOW *get_next_level(WINDOW **win, Game *game) {
    clear();
    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle obstacle;
        game->obstacle[i] = obstacle;
    }
    game->obstacleCount = 0;

    levelInit(game, game->level);

    return (initWindowCentered(game->board_height, game->board_width));
}

/** @brief Function checks if frog collides with obstacle
 *
 *  @return true if frog collides with obstacle
 */
void checkCollision(Game *game) {

    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle obstacle = game->obstacle[i];
        if (obstacle.y == game->frog.y && obstacle.x+1 == game->frog.x) {
            if (obstacle.type !=
                CAR_FRIENDLY) {
                game->end = true;
            }else {

                game->frog.x = (obstacle.direction == -1) ? obstacle.x : obstacle.x+2;
                game->frog.y = obstacle.y;
                if (game->frog.x <= 1) {
                    game->frog.x = game->frog.x + 2;
                }else if (game->frog.x >= game->board_width) {
                    game->frog.x = game->board_width;
                }
            }

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

void moveToDifferentLane(Obstacle * obstacles, Coordinate player_coordinate, int obs_count, int curr_obs_id, int lane_number) {
    int free_lanes[lane_number];
    for (int i = 0; i < lane_number; i++) {
        free_lanes[i] = -1;
    }

    for (int i = 0; i < obs_count; i++) {
        free_lanes[obstacles[i].y] = obstacles->type;
    }
    int new_y;
    do{
        new_y = getRandomNumber(2, lane_number+2);

    } while (free_lanes[new_y] != -1 && new_y != player_coordinate.y);
    obstacles[curr_obs_id].y = new_y;
}

double sqrt(double x) {
    if (x < 0) return -1;
    if (x == 0 || x == 1) return x;

    double approx = x;
    double better_approx = 0.5 * (approx + x / approx);

    while (approx - better_approx > 1e-6 || better_approx - approx > 1e-6) {
        approx = better_approx;
        better_approx = 0.5 * (approx + x / approx);
    }

    return better_approx;
}

double pow(double base, int exp) {
    if (exp == 0) return 1;
    if (exp < 0) return 1 / pow(base, -exp);

    double half = pow(base, exp / 2);
    if (exp % 2 == 0) {
        return half * half;
    } else {
        return half * half * base;
    }
}

double calculate_distance(int x1, int y1, int x2, int y2) {
    return   sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

void bubbleSort(int* array, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (array[j] > array[j + 1]) {
                // Zamiana elementów
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

void saveSortedArrayToFile(const char* filename, int* array, int n) {
    // Sortowanie tablicy — sortowanie bąbelkowe
    bubbleSort(array, n);

    // Zapis posortowanej tablicy do pliku
    FILE* file = fopen(filename, "w");
    if (!file) {
        return;
    }

    for (int i = 0; i < n; i++) {
        fprintf(file, "%d\n", array[i]);
    }

    fclose(file);
}

int* readArray(const char* filename, int n) {
    FILE* file = fopen(filename, "r");
    int* array = (int*)malloc(n * sizeof(int));

    if (!array) {
        return nullptr;
    }

    if (file) {
        // Plik istnieje, odczytaj dane
        for (int i = 0; i < n; i++) {
            if (fscanf(file, "%d", &array[i]) != 1) {
                free(array);
                return nullptr;
            }
        }
    } else {
        // Plik nie istnieje, utwórz nowy
        file = fopen(filename, "w");
        if (!file) {
            free(array);
            return nullptr;
        }

        for (int i = 0; i < n; i++) {
            array[i] = -1;  // Wypełnij tablicę
        }
    }
    fclose(file);

    bubbleSort(array, n);
    return array;
}

void moveObstacles(Game *game) {
    for (int i = 0; i < game->obstacleCount; i++) {
        Obstacle *obstacle = &(game->obstacle[i]);
        if (obstacle->type == CAR|| obstacle->type == CAR_STOPPABLE || obstacle->type == CAR_FRIENDLY) {
            unsigned int speed_buff = obstacle->speed;
            double distance_to_frog = calculate_distance(game->frog.x, game->frog.y, obstacle->x, obstacle->y);
            if (obstacle->type == CAR_STOPPABLE && distance_to_frog <= 3) {
                obstacle->speed = 0;
            }

            #if TEST
            if (obstacle->type == CAR_STOPPABLE) {
                char str[16];
                sprintf(str, "%f",distance_to_frog);
                drawError(str);
            }
            #endif


            for (int j = 0; j < obstacle->speed; j++) {
                if (obstacle->x <= 1 || obstacle->x >= game->board_width) {
                    obstacle->direction *=-1;
                    if (getRandomNumber(1, 10) == 1) {
                        const Coordinate player_coords{game->frog.x ,game->frog.y};
                        moveToDifferentLane(game->obstacle, player_coords, game->obstacleCount,i, game->board_height-3);
                    }
                    if (getRandomNumber(1, 2) == 1) {
                        obstacle->speed = getRandomNumber(1, 3);
                    }
                }
                obstacle->x += 1*obstacle->direction;
                checkCollision(game);
            }
            obstacle->speed = speed_buff;
        }
    }
}

Level ** loadLevelsFile (const char *fileName, int * level_count_p) {
    FILE *file = fopen(fileName, "r");
    if (file == nullptr) {
        // drawError("Error opening file");
    }

    fscanf(file, "Level_count: %d", level_count_p);
    int level_count = *level_count_p;

    Level **level_arr =  (Level **) malloc(sizeof(Level*) * (level_count));

    char line[256];
    int current_level = 0;

    while (fgets(line, sizeof(line), file)) {
        // Pomijanie linii zaczynających się od '#'
        if (line[0] == '#') continue;

        // Wczytanie danych poziomu
        if (strstr(line, "board_height:")) {
            level_arr[current_level] = (Level *)malloc(sizeof(Level));
            sscanf(line, "board_height: %d", &level_arr[current_level]->height);
        } else if (strstr(line, "board_width:")) {
            sscanf(line, "board_width: %d", &level_arr[current_level]->width);
        } else if (strstr(line, "cactus_num:")) {
            sscanf(line, "cactus_num: %d", &level_arr[current_level]->cactus_num);
        } else if (strstr(line, "car_num:")) {
            sscanf(line, "car_num: %d", &level_arr[current_level]->car_num);
            current_level++; // Przechodzimy do następnego poziomu po wczytaniu wszystkich danych
        }
    }


    return level_arr;
}

void mem_free_lvl(Level *** level, int level_count) {
    for (int i = 0; i < level_count; i++) {
        free(level[0][i]);
    }
    free(*level);
};

void handleControls(int input_b, Game * game) {
    if (input_b == 'w' || input_b == KEY_UP) {
        if (game->frog.y > 1)
            game->frog.y--;
    } else if (input_b == 's' || input_b == KEY_DOWN) {
        if (game->frog.y < game->board_height)
            game->frog.y++;
    } else if (input_b == 'a' || input_b == KEY_LEFT) {
        if (game->frog.x > 2)
            game->frog.x--;
    }else if (input_b == 'd' || input_b == KEY_RIGHT) {
        if (game->frog.x < game->board_width)
            game->frog.x++;
    } else if (input_b == 'q' || input_b == KEY_EXIT || input_b == 27) {
        game->end = 1;
    }
}

int main(int argc, char *argv[]) {
    char fileName[] = "scoreboard.txt";

    int arr[SCOREBOARD_SIZE] = {5,6,4,2,1};
    saveSortedArrayToFile(fileName, arr,SCOREBOARD_SIZE);

    srand(time(nullptr));
    cursesInit();
    WINDOW *win;

    Game game;
    game.levels = loadLevelsFile(CONFIG_PATH, &game.level_count);
    game.scores = readArray(fileName, SCOREBOARD_SIZE);

    initGame(&game);

    levelInit(&game, game.level);
    win = initWindowCentered(game.board_height, game.board_width);

    draw(win, game);

    time_t forg_move_dt = 0;
    int last_input_b = -1;
    while (!game.end) {
        time_t startTime = time(nullptr);
        int input_b = getch();

        if (input_b != -1)
            forg_move_dt = 0;

        #if !TEST
        if (forg_move_dt >= 5)
            game.end = true;
        #endif

        if (input_b != last_input_b && input_b != -1) {
            handleControls(input_b, &game);
        }


        moveObstacles(&game);
        checkWin(&game);
        game.play_time = getElapsedTime(&game.start_time);

        if (game.win) {
            if (game.level > game.level_count) {
                game.end = true;
            }else {
                win = get_next_level(&win, &game);
                game.win = false;
            }
        }
        draw(win, game);

        forg_move_dt += time(nullptr) - startTime;
        last_input_b = input_b;
    }

    mem_free_lvl(&game.levels, game.level_count);
    while (wgetch(win) != 'q') {}

    endwin();
    return 0;
}