#define TEST true

#include <clocale>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curses.h>			/* ncurses.h includes stdio.h */
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#define PROJECT_NAME "Jumping Frog"
#define DELAY 60000

#define COL_P_FROG 1
#define COL_P_EXIT 2
// #define COL_P_SAFE
#define COL_P_DANGER 3
#define COL_P_CAR_STOPPABLE 4
#define COL_P_CAR_FRIENDLY 5
#define COL_P_CACTUS 6
#define COL_P_TITLE_WIN 7
#define COL_P_TITLE_LOOSE 8
#define COL_P_BACKGROUND 9
#define COL_P_BIRD 10
#define COL_P_STORK 11

#define SCOREBOARD_SIZE 5
#define SCOREBOARD_PATH "scoreboard.txt"


#define CAR_LINE_CHANGE_CHANCE 20
#define CAR_SPEED_CHANGE_CHANCE 20

#define CONF_PATH (char*) "../game-data.txt"

enum ObstacleType {
    NONE,
    CAR,
    CAR_STOPPABLE,
    CAR_FRIENDLY,
    CACTUS,
    STORK,
    BIRD
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
    int color_pair = COL_P_DANGER;
    int y = -1;
    int x = -1;
    unsigned int speed = 0;
    int direction = 1;
    char skin = 'X';
    ObstacleType type = NONE;

    // Parametry ruchu po okręgu (dla BIRDO)
    struct Bird {
        double angle = 0.0;  // Aktualny kąt (w radianach)
        int radius = 0;      // Promień okręgu
        int center_x = 0;    // Środek okręgu (x)
        int center_y = 0;    // Środek okręgu (y)
    }bird;
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
    long long frame;
    int end = 0;
    bool win = false;
    int lvl{};
    long start_time{};
    long play_time{};
    Exit exit;
    Frog frog;
    int obs_count = 0;
    Obstacle obstacles[50];  //TODO: dynamiczna alokacja miejsca
    Level **levels{};
    int lvl_count{};
    int board_height{};
    int board_width{};
    int *scores = nullptr;
};

int get_random_number(int min, int max) {
    return rand() % (max + 1 - min) + min;

}

void add_bird_obstacle(Obstacle *obs, int center_x, int center_y, int radius, double start_angle, int speed, int color) {
    obs->skin = 'B';
    obs->type = BIRD;
    obs->bird.center_x = center_x;
    obs->bird.center_y = center_y;
    obs->bird.radius = radius;
    obs->bird.angle = start_angle;
    obs->speed = speed;
    obs->color_pair = color;
}

void add_obstacle(Game &game, const Obstacle obstacle) {
    game.obstacles[game.obs_count] = obstacle;
    game.obs_count++;
}

void init_game(Game *game) {
    game->start_time = time(nullptr);
    game->lvl = 1;
}

long get_elapsed_time(const long *ts) {
    const long end = time(nullptr);
    return (end - *ts);
};

void draw_error(char msg[]) {
    Coordinate coord{};
    getmaxyx(stdscr, coord.y, coord.x);
    wattron(stdscr, COLOR_PAIR(COL_P_TITLE_LOOSE));
    mvwprintw(stdscr, 1, (coord.x - 7)/2, "%s", msg);
    wattroff(stdscr, COLOR_PAIR(COL_P_TITLE_LOOSE));
}

void draw_communicates(WINDOW * win, bool win_game,  int board_height, int board_width, long play_time) {
    if (win_game) {
        wattron(win, COLOR_PAIR(COL_P_TITLE_WIN));
        mvwprintw(win, board_height/2, (board_width - 7)/2, "You won!");
        mvwprintw(win, board_height/2+1, (board_width - 28)/2, "Your game took %ld seconds!", play_time);
        wattroff(win, COLOR_PAIR(COL_P_TITLE_WIN));
    }else {
        wattron(win, COLOR_PAIR(COL_P_TITLE_LOOSE));
        mvwprintw(win, board_height/2, (board_width - 9)/2, "Game Over");
        wattroff(win, COLOR_PAIR(COL_P_TITLE_LOOSE));
    }
    mvwprintw(win, board_height-2, (board_width - 15)/2, "Press Q to quit");
};

WINDOW * init_window_coords(int board_height, int board_width, int start_x, int start_y) {
    WINDOW *win = newwin(board_height, board_width, start_x, start_y);

    keypad(win, TRUE);
    keypad(stdscr, TRUE);
    return win;
};

WINDOW * init_window_centered(int board_height, int board_width) {
    struct Terminal{
        int width = 0;
        int height = 0;
    };

    Terminal terminal;
    getmaxyx(stdscr, terminal.height, terminal.width);
    WINDOW *win =init_window_coords(board_height+2, board_width+2, (terminal.height-board_height-2)/2, (terminal.width-board_width-2)/2);;

    return win;
}

void draw_scoreboard(const Game & game, int scores[SCOREBOARD_SIZE]) {
    Coordinate terminalSize{};
    getmaxyx(stdscr, terminalSize.y, terminalSize.x);
    WINDOW *win_scoreboard = init_window_coords(SCOREBOARD_SIZE + 2, 17, (terminalSize.y - game.board_height-2)/2,  (terminalSize.x - game.board_width-2)/2 - (SCOREBOARD_SIZE + 15));
    box(win_scoreboard, '|', '-');
    mvwprintw(win_scoreboard, 0, 1, "Place");
    mvwprintw(win_scoreboard, 0, 9, "Score");
    for (int i = 1; i < SCOREBOARD_SIZE+1; i++) {
        if (scores[i-1] > 0) {
            mvwprintw(win_scoreboard, i, 1, " %d.   | %ds", i, scores[i-1]);
        }else {
            mvwprintw(win_scoreboard, i, 1, " %d.   |", i);
        }
    }

    wrefresh(win_scoreboard);
};

void draw_board(const Game &game , WINDOW * win) {
    box(win, 0, 0);
    mvwprintw(win, 0, (game.board_height-sizeof(PROJECT_NAME)/sizeof(char))/6 , PROJECT_NAME);
    mvwprintw(win, game.board_height+1, 1*game.board_width/10, " Level: %d ", game.lvl);
    mvwprintw(win, 0, 4*game.board_width/6, "Time: %ld", game.play_time);
}

void draw(WINDOW *win, const Game game) {
    draw_scoreboard(game, game.scores);
    werase(win);
    wbkgd(win, COLOR_PAIR(COL_P_BACKGROUND));

    // Box and board setup
    draw_board(game, win);

    if (game.end) {
        draw_communicates(win,game.win, game.board_height, game.board_width, game.play_time);
    }else {
        // Obstacle
        Obstacle bird;
        Obstacle stork;
        for (int i = 0; i < game.obs_count; i++) {
            Obstacle obstacle = game.obstacles[i];
            if (obstacle.type == BIRD) {
                bird = obstacle;
                continue;
            }else if (obstacle.type == STORK) {
                stork = obstacle;
                continue;
            }
            wattron(win, COLOR_PAIR(obstacle.color_pair));
            for (int x = 1; x <= game.board_width; x++) {
                mvwprintw(win, obstacle.y, x, " ");
            }
            mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
            wattron(win, COLOR_PAIR(obstacle.color_pair));

        }
        // Draw birds always on top
        mvwaddch(win, bird.y, bird.x, bird.skin | COLOR_PAIR(bird.color_pair));
        mvwaddch(win, stork.y, stork.x, stork.skin | COLOR_PAIR(stork.color_pair));

        // Exit
        wattron(win, COLOR_PAIR(COL_P_EXIT));
        mvwprintw(win, game.exit.y, game.exit.x-1, "%c", game.exit.skin);
        wattroff(win, COLOR_PAIR(COL_P_EXIT));
        // Player
        wattron(win, COLOR_PAIR(COL_P_FROG));
        mvwprintw(win, game.frog.y, game.frog.x-1, "%c",game.frog.skin);
        wattroff(win, COLOR_PAIR(COL_P_FROG));
    }
    wrefresh(win);
    usleep(DELAY);
};

void ncurses_init() {
    initscr();
    noecho();
    cbreak();
    timeout(FALSE);
    curs_set(FALSE);
    start_color();
    clear();
    refresh();

    //                    font color   bg color
    init_pair(COL_P_FROG, COLOR_WHITE, COLOR_GREEN);
    init_pair(COL_P_EXIT, COLOR_GREEN, COLOR_BLACK);
    // init_pair(COLOR_P_SAFE, COLOR_WHITE, COLOR_YELLOW);
    init_pair(COL_P_DANGER, COLOR_BLACK, COLOR_RED);
    init_pair(COL_P_CACTUS, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(COL_P_TITLE_WIN, COLOR_BLACK, COLOR_GREEN);
    init_pair(COL_P_TITLE_LOOSE, COLOR_RED, COLOR_BLACK);
    init_pair(COL_P_BACKGROUND, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COL_P_CAR_STOPPABLE, COLOR_WHITE, COLOR_RED);
    init_pair(COL_P_CAR_FRIENDLY, COLOR_GREEN, COLOR_RED);
    init_pair(COL_P_BIRD, COLOR_BLACK, COLOR_WHITE);
    init_pair(COL_P_STORK, COLOR_RED, COLOR_WHITE);

}

void fill_trap_rows(int * trap_rows, const int row_number, int cactus_row_num, int car_row_num){
    if (cactus_row_num + car_row_num > row_number) {
        // draw_error("Obstacle row number grater then row number!");
    }else {
        // ustaw odpowiednia ilość kaktusów
        for (int i = 0; i < cactus_row_num; i++) {
            unsigned int row_num;
            do {
                row_num = get_random_number(0, row_number);
            } while (trap_rows[row_num] != -1);
            trap_rows[row_num] = CACTUS;
        }

        for (int i = 0; i < car_row_num; i++) {
            unsigned int row_num;
            do {
                row_num = get_random_number(0, row_number);
            } while (trap_rows[row_num] != -1);
            trap_rows[row_num] = CAR;
        }
        unsigned int row_num;
        // do {
            row_num = 5;
        // } while (trap_rows[row_num] != -1);
        trap_rows[row_num] = BIRD;
    }
}

int *get_level_data(Game * game, int level) {

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

void init_obstacle(Game & game, int obs_board_row) {
    Obstacle obs;
    obs.x = get_random_number(2, game.board_width-1);
    obs.y = obs_board_row+2;
    obs.type = get_random_number(1,100) <= 50 ? CAR_STOPPABLE : CAR;
    obs.type = get_random_number(1,100) <= 50 ? CAR_STOPPABLE : CAR;

    int random_num = get_random_number(1,100);

    obs.color_pair = COL_P_DANGER;
    if (random_num < 33) {
        obs.type = CAR;
    } else if (33<=random_num && random_num<=66) {
        obs.type = CAR_STOPPABLE;
        obs.color_pair = COL_P_CAR_STOPPABLE;
    } else if (random_num > 66) {
        obs.type = CAR_FRIENDLY;
        obs.color_pair = COL_P_CAR_FRIENDLY;
    }

    obs.skin = '*';
    obs.speed = get_random_number(1,3);
    add_obstacle(game, obs);
};

void add_stork(Game &game) {
    Obstacle obs;
    obs.speed = 'B';
    obs.type = STORK;
    obs.x = game.frog.x - 1;
    obs.y = game.board_height +3;
    obs.speed = 1;
    obs.color_pair = COL_P_STORK;
    add_obstacle(game, obs);
};

void level_init(Game *game, int level) {
    int * trap_rows = get_level_data(game, level);
    int trap_row_i = game->board_height - 2;

    Exit exit;
    exit.x = game->board_width/2;
    exit.y = 1;
    game->exit = exit;

    game->frog.x = game->board_width/2;
    game->frog.y = game->board_height;


    for (int i = 0; i < trap_row_i; i++) {
        if (trap_rows[i] == CAR) {
            init_obstacle(*game, i);

        }else if (trap_rows[i] == CACTUS) {
            Obstacle obs;
            obs.x = get_random_number(2, game->board_width-1);
            obs.y = i+2;
            obs.type = CACTUS;
            obs.skin = 'X';
            obs.color_pair = COL_P_CACTUS;

            add_obstacle(*game, obs);
        }else if (trap_rows[i] == BIRD) {
            Obstacle obs;
            obs.speed = '-';
            obs.type = BIRD;
            obs.bird.center_x = game->board_width/2;
            obs.bird.center_y = game->board_height/2;
            obs.bird.radius = 5;
            obs.bird.angle = 0;
            obs.speed = 10;
            obs.color_pair = COL_P_BIRD;

            add_obstacle(*game, obs);
        }
    }
    add_stork(*game);

    free(trap_rows);
}

WINDOW *get_next_level(Game *game) {
    clear();
    for (int i = 0; i < game->obs_count; i++) {
        Obstacle obstacle;
        game->obstacles[i] = obstacle;
    }
    game->obs_count = 0;

    level_init(game, game->lvl);

    return (init_window_centered(game->board_height, game->board_width));
}

/** @brief Function checks if frog collides with obstacle
 *
 *  @return true if frog collides with obstacle
 */
void check_collision(Game *game) {

    for (int i = 0; i < game->obs_count; i++) {
        Obstacle obstacle = game->obstacles[i];
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

bool check_win(Game *game) {
    if (game->frog.x == game->exit.x && game->frog.y == game->exit.y) {
        game->win = true;
        game->lvl++;
        return true;
    }
    return false;
}

void move_to_different_lane(Obstacle * obstacles, Coordinate player_coordinate, int obs_count, int curr_obs_id, int lane_number) {
    if (obstacles[curr_obs_id].type != CAR) return;

    int free_lanes[lane_number];
    for (int i = 0; i < lane_number; i++) {
        free_lanes[i] = -1;
    }
    free_lanes[0] = 1;
    free_lanes[lane_number-1] = 1;

    for (int i = 0; i < obs_count; i++) {
        free_lanes[obstacles[i].y-1] = obstacles[i].type;
    }
    free_lanes[player_coordinate.y-1] = 1;

    int new_y;
    do{
        new_y = get_random_number(1, lane_number-1);

    } while (free_lanes[new_y] != -1);
    obstacles[curr_obs_id].y = new_y+1;
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


/**
 * @brief Function sorts an array in ascending order, treating negative numbers as the largest value.
 *
 * @param array A pointer to the integer array to be sorted.
 * @param n The number of elements in the array.
 */
void bubble_sort(int* array, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            // Sprawdzamy, czy oba elementy są różne od -1
            if (array[j] != -1 && array[j + 1] != -1 && array[j] > array[j + 1]) {
                // Zamiana elementów
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
            // Jeśli array[j] jest -1, nie zamieniamy miejscami z array[j+1], nawet jeśli array[j+1] < array[j]
            else if (array[j] == -1 && array[j + 1] != -1) {
                // Przesuń -1 na koniec tablicy
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

void sorted_array_to_file(const char* file_name, int* array, int n) {
    // Sortowanie tablicy — sortowanie bąbelkowe
    bubble_sort(array, n+1);

    // Zapis posortowanej tablicy do pliku
    FILE* file = fopen(file_name, "w");
    if (!file) {
        return;
    }

    for (int i = 0; i < n; i++) {
        fprintf(file, "%d\n", array[i]);
    }

    fclose(file);
}

int* read_array(const char* file_name, int n) {
    FILE* file = fopen(file_name, "r");
    int* array = (int*)malloc((n+1) * sizeof(int));

    if (!array) {
        return nullptr;
    }

    if (file) {
        // Plik istnieje, odczytaj dane
        int i = 0;
        while (i < n && fscanf(file, "%d", &array[i]) == 1) {
            i++;
        }

        // Jeśli nie odczytano wystarczającej liczby danych, uzupełnij pozostałe miejsca -1
        for (int j = i; j < n; j++) {
            array[j] = -1;
        }

        // Jeśli nie odczytano wszystkich danych, zapisz brakujące do pliku
        if (i < n) {
            file = freopen(file_name, "w", file);  // Zresetuj plik do trybu zapisu
            if (file) {
                for (int j = 0; j < n; j++) {
                    fprintf(file, "%d\n", array[j]);
                }
            }
        }

        fclose(file);
        bubble_sort(array, n);
    } else {
        // Plik nie istnieje, utwórz nowy i wypełnij tablicę -1
        file = fopen(file_name, "w");
        if (!file) {
            free(array);
            return nullptr;
        }

        for (int i = 0; i < n; i++) {
            array[i] = -1;
            fprintf(file, "%d\n", array[i]); // Zapisz do pliku
        }

        fclose(file);
    }

    return array;
}

void update_bird_position(Obstacle *obs) {
    if (obs->type != BIRD) return;

    // Aktualizacja kąta na podstawie prędkości
    obs->bird.angle += obs->speed * 0.01;
    if (obs->bird.angle >= 2 * M_PI) obs->bird.angle -= 2 * M_PI;

    // Oblicz nową pozycję na okręgu
    obs->x = obs->bird.center_x + (int)(obs->bird.radius * cos(obs->bird.angle));
    obs->y = obs->bird.center_y + (int)(obs->bird.radius * sin(obs->bird.angle) * 0.5); // Skalowanie dla proporcji konsoli
}

void adjust_car_speed_and_lane(Game *game, Obstacle *obstacle, int obstacle_index) {
    if (obstacle->x <= 1 || obstacle->x >= game->board_width) {
        obstacle->direction *= -1;

        // Zmień linię z pewnym prawdopodobieństwem
        if (get_random_number(1, 100) <= CAR_LINE_CHANGE_CHANCE) {
            const Coordinate player_coords{game->frog.x, game->frog.y};
            move_to_different_lane(game->obstacles, player_coords, game->obs_count, obstacle_index, game->board_height);
        }

        // Zmień prędkość z pewnym prawdopodobieństwem
        if (get_random_number(1, 100) <= CAR_SPEED_CHANGE_CHANCE) {
            obstacle->speed = get_random_number(1, 3);
        }
    }
}

void handle_car(Game *game, Obstacle *obstacle, int obstacle_index) {
    unsigned int speed_buff = obstacle->speed;
    double distance_to_frog = calculate_distance(game->frog.x, game->frog.y, obstacle->x, obstacle->y);

    if (obstacle->type == CAR_STOPPABLE && distance_to_frog <= 3) {
        obstacle->speed = 0;
    }

#if TEST
    if (obstacle->type == CAR_STOPPABLE) {
        char str[16];
        sprintf(str, "%f", distance_to_frog);
        draw_error(str);
    }
#endif

    for (int j = 0; j < obstacle->speed; j++) {
        adjust_car_speed_and_lane(game, obstacle, obstacle_index);
        obstacle->x += 1 * obstacle->direction;
        check_collision(game);
    }

    obstacle->speed = speed_buff; // Przywróć oryginalną prędkość
}

void move_stork(Game & game, Obstacle *stork) {
    if (game.frame % 7 != 0) return;
    int target_x = game.frog.x-1;
    int target_y = game.frog.y;

    if (stork->x < target_x) stork->x++;
    else if (stork->x > target_x) stork->x--;

    if (stork->y < target_y) stork->y++;
    else if (stork->y > target_y) stork->y--;
}

void move_obstacles(Game *game) {
    for (int i = 0; i < game->obs_count; i++) {
        Obstacle *obstacle = &(game->obstacles[i]);

        if (obstacle->type == BIRD) {
            update_bird_position(obstacle);
        } else if (obstacle->type == CAR || obstacle->type == CAR_STOPPABLE || obstacle->type == CAR_FRIENDLY) {
            handle_car(game, obstacle, i);
        }else if (obstacle->type == STORK) {
            move_stork(*game, obstacle);
        }
    }
}

Level ** load_levels_file (const char *file_name, int * level_count_p) {
    FILE *file = fopen(file_name, "r");
    if (file == nullptr) {
        // draw_error("Error opening file");
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

void handle_controls(int input_b, Game * game) {
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

WINDOW * handle_game_win(WINDOW *win, Game *game) {
    if (game->win) {
        if (game->lvl > game->lvl_count) {
            game->end = true;
            game->scores[SCOREBOARD_SIZE] = (int) game->play_time;
            sorted_array_to_file(SCOREBOARD_PATH, game->scores, SCOREBOARD_SIZE);
        }else {
            win = get_next_level(game);
            game->win = false;
        }
    }
    return win;
}

int main(int argc, char *argv[]) {
    char file_name[] = SCOREBOARD_PATH;

    srand(time(nullptr));
    ncurses_init();
    WINDOW *wind;

    Game game;
    game.levels = load_levels_file(CONF_PATH, &game.lvl_count);
    game.scores = read_array(file_name, SCOREBOARD_SIZE);

    init_game(&game);

    level_init(&game, game.lvl);
    wind = init_window_centered(game.board_height, game.board_width);

    // draw(wind, game);

    time_t forg_move_dt = 0;
    int last_input_b = -1;

    while (!game.end) {
        game.frame++;

        wind = init_window_centered(game.board_height, game.board_width);
        time_t startTime = time(nullptr);
        int input_b = getch();

        if (input_b != -1)
            forg_move_dt = 0;

        #if !TEST
        if (forg_move_dt >= 5)
            game.end = true;
        #endif

        if (input_b != last_input_b && input_b != -1) {
            handle_controls(input_b, &game);
        }


        move_obstacles(&game);
        check_win(&game);
        game.play_time = get_elapsed_time(&game.start_time);

        wind = handle_game_win(wind, &game);
        draw(wind, game);

        forg_move_dt += time(nullptr) - startTime;
        last_input_b = input_b;
    }

    mem_free_lvl(&game.levels, game.lvl_count);
    int ch = wgetch(wind);
    while (ch != 'q') {
        ch = wgetch(wind);
    }

    endwin();
    return 0;
}