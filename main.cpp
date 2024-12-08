#define TEST false

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curses.h>
#include <unistd.h>
#include <math.h>

#define PROJECT_NAME "Jumping Frog"
#define DELAY 60000

#define COL_P_FROG 1
#define COL_P_EXIT 2
#define COL_P_DANGER 3
#define COL_P_CAR_STOPPABLE 4
#define COL_P_CAR_FRIENDLY_on 5
#define COL_P_CAR_FRIENDLY_off 6
#define COL_P_CACTUS 7
#define COL_P_TITLE_WIN 8
#define COL_P_TITLE_LOOSE 9
#define COL_P_BACKGROUND 10
#define COL_P_BIRD 11
#define COL_P_STORK 12

#define FROG_DESC "FROG: Use arrows to navigate."
#define EXIT_DESC "EXIT: Your destination point."
#define CAR_DESC "CAR: Be careful, it's dangerous."
#define CAR_STOPPABLE_DESC "GOOD CAR: Driver will stop to let you pass, but you shouldn't touch it!"
#define CAR_FRIENDLY_DESC "BUS: This car can give you a ride! Press E to make it CAR."
#define CACTUS_DESC "CACTUS: It's spiky, don't touch it!"
#define STORK_DESC "STORK: A hungry bird looking for a frog to eat."
#define BIRD_DESC "BIRD: A lazy bird, it'll eat you when you get too close!"

#define SCOREBOARD_SIZE 5
#define SCOREBOARD_PATH "scoreboard.txt"


#define CAR_LINE_CHANGE_CHANCE 20
#define CAR_SPEED_CHANGE_CHANCE 20

#define CONF_PATH (char*) "../game-data.txt"
/*
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
   ::                                                  ::
   ::                 Structures and Enums             ::
   ::                                                  ::
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/

enum ObstacleType {
    FIRST, //Has to be first in order, DON'T CHANGE
    NONE,
    CAR,
    CAR_STOPPABLE,
    CAR_FRIENDLY,
    CACTUS,
    STORK,
    BIRD,
    LAST //Has to be last in order, DON'T CHANGE
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

    struct Bird {
        double angle = 0.0; // Current angle (radians)
        int radius = 0; // Circle radius
        int center_x = 0; // Circle center (x)
        int center_y = 0; // Circle center (y)
    } bird;
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
    // board dimensions without border (only playable area)
    int board_height = 1;
    int board_width = 51;
    long long frame = 0;

    Exit exit;
    Frog frog;
    Obstacle obstacles[50];
    int obs_count = 0;

    Level **levels{};
    int lvl_count{};
    int curr_lvl{};

    bool win = false;
    bool end = false;
    bool friendly_car_ride = true;

    long start_time{};
    long play_time{};

    int *scores_tabele = nullptr;
};

/*
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
   ::                                                  ::
   ::                  Utility Functions               ::
   ::                                                  ::
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/

/**
 * @return random numebr in range (min to max)
 */
int get_random_number(int min, int max) {
    return rand() % (max + 1 - min) + min;
}


/**
 * @return squareroot of a given number
 */
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

/**
 * @brief Computes the power of a number.
 * @param base The base value.
 * @param exp The exponent value.
 * @return The result of base raised to the power of exp.
 */
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


/**
 * @brief Calculates the factorial of a given number.
 * @param n The number for which the factorial is calculated.
 * @return The factorial of the number n.
 */

double factorial(int n) {
    double result = 1.0;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}


/**
 * @brief Computes the sine of a given angle (in radians) using a Taylor series approximation.
 * @param angle The angle in radians.
 * @return The sine of the angle x.
 */

double sin(const double angle) {
    double term = angle;
    double sum = term;
    int n = 3;

    for (int i = 1; i <= 10; i++) {
        term = -term * angle * angle / ((n - 1) * n);
        sum += term;
        n += 2;
    }

    return sum;
}

/**
 * @brief Computes the cos of a given angle (in radians) using a Taylor series approximation.
 * @param angle The angle in radians.
 * @return The cos of the angle x.
 */

double cos(double angle) {
    double term = 1.0;
    double sum = term;
    int n = 2;

    for (int i = 1; i <= 10; i++) {
        term = -term * angle * angle / ((n - 1) * n);
        sum += term;
        n += 2;
    }

    return sum;
}

/**
 * @brief Calculates the Euclidean distance between two points in a 2D space.
 * @param x1 The x-coordinate of the first point.
 * @param y1 The y-coordinate of the first point.
 * @param x2 The x-coordinate of the second point.
 * @param y2 The y-coordinate of the second point.
 * @return The Euclidean distance between the points (x1, y1) and (x2, y2).
 */

double calculate_distance(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

/**
 * @brief Calculates the elapsed time in seconds since the given timestamp.
 * @param ts A pointer to the timestamp (in seconds).
 * @return The elapsed time in seconds since the timestamp.
 */

long get_elapsed_time(const time_t *ts) {
    const time_t end = time(nullptr);
    return (end - *ts);
};

/**
 * @brief Function sorts an array in ascending order, treating negative numbers as the largest value.
 *
 * @param array A pointer to the integer array to be sorted.
 * @param n The number of elements in the array.
 */
void bubble_sort(int *array, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            // We check if both elements are different from -1
            if (array[j] != -1 && array[j + 1] != -1 && array[j] > array[j + 1]) {
                // Swapping elements
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
            // If array[j] is -1, we do not swap with array[j+1], even if array[j+1] < array[j]
            else if (array[j] == -1 && array[j + 1] != -1) {
                // Move -1 to the end of the array
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

/**
 * @brief Displays an error message at the center of the screen.
 * @param msg The error message to be displayed.
 */
void draw_error(char msg[]) {
    Coordinate coord{};
    getmaxyx(stdscr, coord.y, coord.x);
    wattron(stdscr, COLOR_PAIR(COL_P_TITLE_LOOSE));
    mvwprintw(stdscr, 1, (coord.x - 7) / 2, "%s", msg);
    wattroff(stdscr, COLOR_PAIR(COL_P_TITLE_LOOSE));
}

/*
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
   ::                                                  ::
   ::        Initialization and object creation        ::
   ::                                                  ::
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/

/**
 * @brief Sorts an array and writes the sorted values to a file.
 * @param file_name The name of the file where the sorted array will be written.
 * @param array A pointer to the array to be sorted.
 * @param n The number of elements in the array.
 */
void sorted_array_to_file(const char *file_name, int *array, int n) {
    bubble_sort(array, n + 1);

    FILE *file = fopen(file_name, "w");
    if (!file) {
        return;
    }

    for (int i = 0; i < n; i++) {
        fprintf(file, "%d\n", array[i]);
    }

    fclose(file);
}

/**
 * @brief Initializes the ncurses library, sets terminal configurations, and defines color pairs.
 */
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
    init_pair(COL_P_DANGER, COLOR_BLACK, COLOR_RED);
    init_pair(COL_P_CACTUS, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(COL_P_TITLE_WIN, COLOR_BLACK, COLOR_GREEN);
    init_pair(COL_P_TITLE_LOOSE, COLOR_RED, COLOR_BLACK);
    init_pair(COL_P_BACKGROUND, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COL_P_CAR_STOPPABLE, COLOR_WHITE, COLOR_RED);
    init_pair(COL_P_CAR_FRIENDLY_on, COLOR_GREEN, COLOR_RED);
    init_pair(COL_P_CAR_FRIENDLY_off, COLOR_YELLOW, COLOR_RED);
    init_pair(COL_P_BIRD, COLOR_BLACK, COLOR_WHITE);
    init_pair(COL_P_STORK, COLOR_RED, COLOR_WHITE);
}

/**
 * @brief Loads level data from a file and returns an array of Level structures.
 * @param file_name The name of the file containing the level data.
 * @param level_count_p A pointer to store the number of levels loaded from the file.
 * @return A pointer to an array of Level structures containing the loaded data.
 */
Level **load_levels_file(const char *file_name, int *level_count_p) {
    FILE *file = fopen(file_name, "r");
    if (file == nullptr) {
        draw_error((char *) "Error opening file");
    }

    fscanf(file, "Level_count: %d", level_count_p);
    int level_count = *level_count_p;

    Level **level_arr = (Level **) malloc(sizeof(Level *) * (level_count));

    char line[256];
    int current_level = 0;

    while (fgets(line, sizeof(line), file)) {
        // Skipping lines starting with '#'
        if (line[0] == '#') continue;

        // Loading level data
        if (strstr(line, "board_height:")) {
            level_arr[current_level] = (Level *) malloc(sizeof(Level));
            sscanf(line, "board_height: %d", &level_arr[current_level]->height);
        } else if (strstr(line, "board_width:")) {
            sscanf(line, "board_width: %d", &level_arr[current_level]->width);
        } else if (strstr(line, "cactus_num:")) {
            sscanf(line, "cactus_num: %d", &level_arr[current_level]->cactus_num);
        } else if (strstr(line, "car_num:")) {
            sscanf(line, "car_num: %d", &level_arr[current_level]->car_num);
            current_level++;
        }
    }


    return level_arr;
}

/**
 * @brief Reads an array of integers from a file or creates a new file if it doesn't exist.
 * @param file_name The name of the file to read from or write to.
 * @param n The number of integers to read into the array.
 * @return A pointer to the array of integers read from the file or filled with -1 if the file is not found.
 */
int *read_array(const char *file_name, int n) {
    FILE *file = fopen(file_name, "r");
    int *array = (int *) malloc((n + 1) * sizeof(int));

    if (!array) {
        return nullptr;
    }

    if (file) {
        // File exists, read data
        int i = 0;
        while (i < n && fscanf(file, "%d", &array[i]) == 1) {
            i++;
        }

        // If not enough data read, fill remaining spaces -1
        for (int j = i; j < n; j++) {
            array[j] = -1;
        }

        // If it doesn't read all the data, save the missing data to a file
        if (i < n) {
            file = freopen(file_name, "w", file); // Reset file to write mode
            if (file) {
                for (int j = 0; j < n; j++) {
                    fprintf(file, "%d\n", array[j]);
                }
            }
        }

        fclose(file);
        bubble_sort(array, n);
    } else {
        // File does not exist, create new one and fill array -1
        file = fopen(file_name, "w");
        if (!file) {
            free(array);
            return nullptr;
        }

        for (int i = 0; i < n; i++) {
            array[i] = -1;
            fprintf(file, "%d\n", array[i]);
        }

        fclose(file);
    }

    return array;
}

void init_game(Game *game) {
    game->start_time = time(nullptr);
    game->curr_lvl = 1;
}

/**
 * @brief Fills the trap rows with obstacles (cacti, cars, and a bird).
 * @param trap_rows An array representing the rows, where each element indicates the type of obstacle.
 * @param row_number The total number of rows available.
 * @param cactus_row_num The number of cactus obstacles to place.
 * @param car_row_num The number of car obstacles to place.
 */
void fill_trap_rows(int *trap_rows, const int row_number, int cactus_row_num, int car_row_num) {
    if (cactus_row_num + car_row_num > row_number) {
        // draw_error("Obstacle row number grater then row number!");
    } else {
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

/**
 * @brief Retrieves the level data and populates the trap rows with obstacles.
 * @param game A pointer to the main game structure.
 * @param level The level number to load data for.
 * @return A pointer to an array representing the obstacle placement for the given level.
 */
int *get_level_data(Game *game, int level) {
    int row_i = 0;

    Level current_level = *game->levels[level - 1];
    game->board_height = current_level.height;
    game->board_width = current_level.width;
    row_i = game->board_height - 3;

    int trap_rows[row_i];
    for (int i = 0; i < row_i; i++) {
        trap_rows[i] = -1;
    }
    fill_trap_rows(trap_rows, row_i, current_level.cactus_num, current_level.car_num);

    int *trap_rows_ptr = (int *) malloc(sizeof(int) * (row_i));
    for (int i = 0; i < row_i; i++) {
        trap_rows_ptr[i] = trap_rows[i];
    }

    return trap_rows_ptr;
}

/**
 * @brief Adds a bird obstacle to the given obstacle structure.
 * @param obs A pointer to the obstacle structure to be updated.
 * @param center_x The x-coordinate of the bird's center.
 * @param center_y The y-coordinate of the bird's center.
 * @param radius The radius of the bird's movement area.
 * @param start_angle The starting angle of the bird's movement.
 * @param speed The speed of the bird's movement.
 * @param color The color associated with the bird obstacle.
 */
void add_bird_obstacle(Obstacle *obs, int center_x, int center_y, int radius, double start_angle, int speed,
                       int color) {
    obs->skin = 'B';
    obs->type = BIRD;
    obs->bird.center_x = center_x;
    obs->bird.center_y = center_y;
    obs->bird.radius = radius;
    obs->bird.angle = start_angle;
    obs->speed = speed;
    obs->color_pair = color;
}

/**
 * @brief Adds an obstacle to the game's obstacle array.
 * @param game A reference to the game structure where the obstacle will be added.
 * @param obstacle The obstacle to be added to the game.
 */
void add_obstacle(Game &game, const Obstacle &obstacle) {
    game.obstacles[game.obs_count] = obstacle;
    game.obs_count++;
}

/**
 * @brief Initializes a car obstacle with random class and adds it to the game.
 * @param game A reference to the game structure where the obstacle will be added.
 * @param obs_board_row The row where the obstacle will appear.
 */
void init_car_obstacle(Game &game, int obs_board_row) {
    Obstacle obs;
    obs.x = get_random_number(2, game.board_width - 1);
    obs.y = obs_board_row + 2;
    obs.type = CAR;

    int random_num = get_random_number(1, 100);

    obs.color_pair = COL_P_DANGER;

    if (33 <= random_num && random_num <= 66) {
        obs.type = CAR_STOPPABLE;
        obs.color_pair = COL_P_CAR_STOPPABLE;
    } else if (random_num > 66) {
        obs.type = CAR_FRIENDLY;
        obs.color_pair = COL_P_CAR_FRIENDLY_on;
    }

    obs.skin = '*';
    obs.speed = get_random_number(1, 3);
    add_obstacle(game, obs);
};

/**
 * @brief Adds a stork obstacle to the game.
 * @param game A reference to the game structure where the stork will be added.
 */
void add_stork(Game &game) {
    Obstacle obs;
    obs.speed = 'S';
    obs.type = STORK;
    obs.x = game.frog.x - 1;
    obs.y = game.board_height + 5;
    obs.speed = 8; // Stork will move every n frame
    obs.color_pair = COL_P_STORK;
    add_obstacle(game, obs);
};

/**
 * @brief Initializes the level, including setting up the frog, exit, and obstacles.
 * @param game A pointer to the game structure to initialize the level for.
 * @param level The level number to initialize.
 */
void level_init(Game *game, int level) {
    int *trap_rows = get_level_data(game, level);
    int trap_row_i = game->board_height - 2;

    Exit exit;
    exit.x = game->board_width / 2;
    exit.y = 1;
    game->exit = exit;

    game->frog.x = game->board_width / 2;
    game->frog.y = game->board_height;


    for (int i = 0; i < trap_row_i; i++) {
        if (trap_rows[i] == CAR) {
            init_car_obstacle(*game, i);
        } else if (trap_rows[i] == CACTUS) {
            Obstacle obs;
            obs.x = get_random_number(2, game->board_width - 1);
            obs.y = i + 2;
            obs.type = CACTUS;
            obs.skin = 'X';
            obs.color_pair = COL_P_CACTUS;

            add_obstacle(*game, obs);
        } else if (trap_rows[i] == BIRD) {
            Obstacle obs;
            obs.type = BIRD;
            obs.bird.center_x = game->board_width / 2;
            obs.bird.center_y = game->board_height / 2;
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

/**
 * @brief Initializes a new window with the given dimensions and draw starting position.
 * @param board_height The height of the window.
 * @param board_width The width of the window.
 * @param start_x The x-coordinate of the drawing starting position.
 * @param start_y The y-coordinate of the drawing starting position.
 * @return A pointer to the initialized window.
 */
WINDOW *init_window_coords(int board_height, int board_width, int start_x, int start_y) {
    WINDOW *win = newwin(board_height, board_width, start_x, start_y);

    keypad(win, TRUE);
    keypad(stdscr, TRUE);
    return win;
};

/**
 * @brief Initializes a new window centered in the terminal with the given dimensions.
 * @param board_height The height of the window.
 * @param board_width The width of the window.
 * @return A pointer to the initialized, centered window.
 */
WINDOW *init_window_centered(int board_height, int board_width) {
    Coordinate terminal{};
    getmaxyx(stdscr, terminal.y, terminal.x);
    WINDOW *win = init_window_coords(board_height + 2, board_width + 2, (terminal.y - board_height - 2) / 2,
                                     (terminal.x - board_width - 2) / 2);;

    return win;
}

/**
 * @brief Resets the obstacles, initializes the next level, and creates a centered window for the next level.
 * @param game A pointer to the game structure to initialize the next level for.
 * @return A pointer to the initialized window for the next level.
 */

WINDOW *get_next_level_window(Game *game) {
    clear();
    for (int i = 0; i < game->obs_count; i++) {
        Obstacle obstacle;
        game->obstacles[i] = obstacle;
    }
    game->obs_count = 0;

    level_init(game, game->curr_lvl);

    return (init_window_centered(game->board_height, game->board_width));
}

/**
 * @brief Handles the game win condition, either proceeding to the next level or ending the game.
 * @param win A pointer to the current window.
 * @param game A pointer to the game structure to handle the win condition.
 * @return A pointer to the updated window, either for the next level or the game over screen.
 */
WINDOW *handle_game_win(WINDOW *win, Game *game) {
    if (game->win) {
        if (game->curr_lvl > game->lvl_count) {
            game->end = true;
            game->scores_tabele[SCOREBOARD_SIZE] = (int) game->play_time;
            sorted_array_to_file(SCOREBOARD_PATH, game->scores_tabele, SCOREBOARD_SIZE);
        } else {
            win = get_next_level_window(game);
            game->win = false;
        }
    }
    return win;
}


/*
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
   ::                                                  ::
   ::         Obstacle movement and collisions         ::
   ::                                                  ::
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/
/**
 * @brief Handles player controls for the frog movement and other actions.
 * @param input_b The input key pressed by the player.
 * @param game A pointer to the game structure to update based on the controls.
 */

void handle_controls(int input_b, Game *game) {
    if (input_b == 'w' || input_b == KEY_UP) {
        if (game->frog.y > 1)
            game->frog.y--;
    } else if (input_b == 's' || input_b == KEY_DOWN) {
        if (game->frog.y < game->board_height)
            game->frog.y++;
    } else if (input_b == 'a' || input_b == KEY_LEFT) {
        if (game->frog.x > 2)
            game->frog.x--;
    } else if (input_b == 'd' || input_b == KEY_RIGHT) {
        if (game->frog.x < game->board_width)
            game->frog.x++;
    } else if (input_b == 'e') {
        game->friendly_car_ride = !game->friendly_car_ride;
    } else if (input_b == 'q' || input_b == KEY_EXIT || input_b == 27) {
        game->end = 1;
    }
}

/**
 * @brief Updates the position of a bird obstacle based on its movement around a circular path.
 * @param bird A pointer to the obstacle structure representing the bird.
 */
void update_bird_position(Obstacle *bird) {
    if (bird->type != BIRD) return;

    // Angle update based on speed
    bird->bird.angle += bird->speed * 0.01;
    if (bird->bird.angle >= 2 * M_PI) bird->bird.angle -= 2 * M_PI;

    // Calculate new position on the circle
    bird->x = bird->bird.center_x + (int) (bird->bird.radius * cos(bird->bird.angle));
    bird->y = bird->bird.center_y + (int) (bird->bird.radius * sin(bird->bird.angle) * 0.5);
    // Scaling for console aspect ratio
}

/**
 * @brief Moves a car obstacle to a different lane, ensuring the lane is free and does not overlap with the player.
 * @param obstacles An array of all obstacles in the game.
 * @param player_coordinate The current position of the player.
 * @param obs_count The total number of obstacles in the game.
 * @param curr_obs_id The index of the current obstacle to move.
 * @param lane_number The total number of available lanes.
 */
void move_to_different_lane(Obstacle *obstacles, Coordinate player_coordinate, int obs_count, int curr_obs_id,
                            int lane_number) {
    if (obstacles[curr_obs_id].type != CAR) return;

    int free_lanes[lane_number];
    for (int i = 0; i < lane_number; i++) {
        free_lanes[i] = -1;
    }
    free_lanes[0] = 1;
    free_lanes[lane_number - 1] = 1;

    for (int i = 0; i < obs_count; i++) {
        free_lanes[obstacles[i].y - 1] = obstacles[i].type;
    }
    free_lanes[player_coordinate.y - 1] = 1;

    clock_t start_t = clock();
    int new_y;
    do {
        if ((start_t - clock()) / CLOCKS_PER_SEC > 0.1) return; // In case of endless loop

        new_y = get_random_number(1, lane_number - 1);
    } while (free_lanes[new_y] != -1);
    obstacles[curr_obs_id].y = new_y + 1;
}

/**
 * @brief Adjusts the car obstacle's speed and lane based on certain probabilities when it reaches the screen boundary.
 * @param game A pointer to the game structure to access game data.
 * @param obstacle A pointer to the car obstacle to adjust.
 * @param obstacle_index The index of the current car obstacle in the game.
 */
void adjust_car_speed_and_lane(Game *game, Obstacle *obstacle, int obstacle_index) {
    if (obstacle->x <= 1 || obstacle->x >= game->board_width) {
        obstacle->direction *= -1;

        // Change line with some probability
        if (get_random_number(1, 100) <= CAR_LINE_CHANGE_CHANCE) {
            const Coordinate player_coords{game->frog.x, game->frog.y};
            move_to_different_lane(game->obstacles, player_coords, game->obs_count, obstacle_index, game->board_height);
        }

        // Change speed with some probability
        if (get_random_number(1, 100) <= CAR_SPEED_CHANGE_CHANCE) {
            obstacle->speed = get_random_number(1, 3);
        }
    }
}

/**
 * @brief Checks for collisions between the frog and obstacles, and handles the consequences based on the obstacle type.
 * @param game A pointer to the game structure to check collisions and update the game state.
 */
void check_collision(Game *game) {
    for (int i = 0; i < game->obs_count; i++) {
        Obstacle obstacle = game->obstacles[i];

        // Check if the obstacle is in the same position as the frog
        if (obstacle.y != game->frog.y || obstacle.x + 1 != game->frog.x) {
            continue;
        }

        // Handle collision based on obstacle type
        if (obstacle.type != CAR_FRIENDLY || (obstacle.type == CAR_FRIENDLY && !game->friendly_car_ride)) {
            game->end = true;
            return;
        }

        // Move the frog based on the obstacle's direction
        game->frog.x = (obstacle.direction == -1) ? obstacle.x : obstacle.x + 2;
        game->frog.y = obstacle.y;

        // Keep the frog within the board boundaries
        if (game->frog.x <= 1) {
            game->frog.x += 2;
        } else if (game->frog.x >= game->board_width) {
            game->frog.x = game->board_width;
        }
    }
}

/**
 * @brief Handles the behavior of a car obstacle, including stopping, moving, and adjusting speed and lane based on the car type and game conditions.
 * @param game A pointer to the game structure to manage game data.
 * @param obstacle A pointer to the car obstacle being handled.
 * @param obstacle_index The index of the current car obstacle in the game.
 */
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

/**
 * @brief Moves the stork obstacle towards the frog's position.
 * @param game A reference to the game structure to manage game data.
 * @param stork A pointer to the stork obstacle being moved.
 */
void move_stork(Game &game, Obstacle *stork) {
    if (game.frame % stork->speed != 0) return;
    int target_x = game.frog.x - 1;
    int target_y = game.frog.y;

    if (stork->x < target_x) stork->x++;
    else if (stork->x > target_x) stork->x--;

    if (stork->y < target_y) stork->y++;
    else if (stork->y > target_y) stork->y--;
}

/**
 * @brief Moves all the obstacles in the game, including birds, cars, and storks.
 *        Each type of obstacle has its own movement logic.
 * @param game A pointer to the game structure that holds the list of obstacles.
 */
void move_obstacles(Game *game) {
    for (int i = 0; i < game->obs_count; i++) {
        Obstacle *obstacle = &(game->obstacles[i]);

        if (obstacle->type == BIRD) {
            update_bird_position(obstacle);
        } else if (obstacle->type == CAR || obstacle->type == CAR_STOPPABLE || obstacle->type == CAR_FRIENDLY) {
            handle_car(game, obstacle, i);
        } else if (obstacle->type == STORK) {
            move_stork(*game, obstacle);
        }
    }
}

/**
 * @brief Checks if the player has reached the exit and won the level.
 *        Updates the game state and progresses to the next level if the player has won.
 * @param game A pointer to the game structure that holds the player's position and the exit.
 * @return true if the player has reached the exit and won the level, false otherwise.
 */
bool check_win(Game *game) {
    if (game->frog.x == game->exit.x && game->frog.y == game->exit.y) {
        game->win = true;
        game->curr_lvl++;
        return true;
    }
    return false;
}

/*
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
   ::                                                  ::
   ::                  Display handling                ::
   ::                                                  ::
   ::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/

/**
 * @brief Draws the author information at the top of the terminal screen.
 *        This includes the author's name and their student index.
 */
void draw_autor() {
    mvwprintw(stdscr, 1, 4, "Author: Filip Grela");
    mvwprintw(stdscr, 2, 4, "Index: 203850");
}

/**
 * @brief Displays the "Wait for Start" screen with instructions on how to begin the game.
 *        It also provides options to display help or quit the game.
 * @param wind The window where the instructions and prompts will be drawn.
 */
void draw_wait_for_start(WINDOW *wind) {
    clear();
    draw_autor();
    box(wind, 0, 0);
    int width, height;
    getmaxyx(wind, height, width);
    mvwprintw(wind, height / 2, (width - 51) / 2 + 1, "Press any button to start, H for help, Q to quit.");
    refresh();
};

/**
 * @brief Draws the information screen with game obstacle and object descriptions and their corresponding colors.
 */
void draw_info() {
    Coordinate terminalSize{};
    getmaxyx(stdscr, terminalSize.y, terminalSize.x);
    WINDOW *win = init_window_coords(11, 90, (terminalSize.y - 11) / 2, (terminalSize.x - 90) / 2);
    box(win, 0, 0);
    mvwprintw(win, terminalSize.y / 2, terminalSize.x, "Help");

    struct TableEntry {
        char character;
        int color;
        const char *description;
    };
    TableEntry table[] = {
        {'0', COL_P_FROG, FROG_DESC},
        {'^', COL_P_EXIT, EXIT_DESC},
        {'*', COL_P_DANGER, CAR_DESC},
        {'*', COL_P_CAR_STOPPABLE, CAR_STOPPABLE_DESC},
        {'*', COL_P_CAR_FRIENDLY_on, CAR_FRIENDLY_DESC},
        {'X', COL_P_CACTUS, CACTUS_DESC},
        {'S', COL_P_BIRD, STORK_DESC},
        {'B', COL_P_STORK, BIRD_DESC}
    };


    for (int i = 0; i < sizeof(table) / sizeof(TableEntry); i++) {
        wattron(win, COLOR_PAIR(table[i].color));
        mvwprintw(win, i + 1, 2, "%c", table[i].character);
        wattroff(win, COLOR_PAIR(table[i].color));
        mvwprintw(win, i + 1, 3, ": %s", table[i].description);
    }

    wrefresh(win);
}

/**
 * @brief Draws the game board, including the level and play time.
 *
 * @param game The current game state.
 * @param win The window where the board is drawn.
 */
void draw_board(const Game &game, WINDOW *win) {
    box(win, 0, 0);
    mvwprintw(win, 0, (game.board_height - sizeof(PROJECT_NAME) / sizeof(char)) / 6, PROJECT_NAME);
    mvwprintw(win, game.board_height + 1, 1 * game.board_width / 10, " Level: %d ", game.curr_lvl);
    mvwprintw(win, 0, 4 * game.board_width / 6, "Time: %ld", game.play_time);
}

/**
 * @brief Draws the scoreboard with player rankings and their scores.
 *
 * @param game The current game state.
 * @param scores The array of scores to display in the scoreboard.
 */
void draw_scoreboard(const Game &game, int scores[SCOREBOARD_SIZE]) {
    Coordinate terminalSize{};
    getmaxyx(stdscr, terminalSize.y, terminalSize.x);
    WINDOW *win_scoreboard = init_window_coords(SCOREBOARD_SIZE + 2, 17, (terminalSize.y - game.board_height - 2) / 2,
                                                (terminalSize.x - game.board_width - 2) / 2 - (SCOREBOARD_SIZE + 15));
    box(win_scoreboard, '|', '-');
    mvwprintw(win_scoreboard, 0, 1, "Place");
    mvwprintw(win_scoreboard, 0, 9, "Score");
    for (int i = 1; i < SCOREBOARD_SIZE + 1; i++) {
        if (scores[i - 1] > 0) {
            mvwprintw(win_scoreboard, i, 1, " %d.   | %ds", i, scores[i - 1]);
        } else {
            mvwprintw(win_scoreboard, i, 1, " %d.   |", i);
        }
    }
    wrefresh(win_scoreboard);
};

/** @brief Function draws communicates in widnow.
 *
 * @param win Window that comunicates will display on
 * @param win_game is game won
 * @param board_height Height of playable area
 * @param board_width Width of playable area
 * @param play_time Time from game start.
 */
void draw_communicates(WINDOW *win, bool win_game, int board_height, int board_width, long play_time) {
    if (win_game) {
        wattron(win, COLOR_PAIR(COL_P_TITLE_WIN));
        mvwprintw(win, board_height / 2, (board_width - 7) / 2, "You won!");
        mvwprintw(win, board_height / 2 + 1, (board_width - 28) / 2, "Your game took %ld seconds!", play_time);
        wattroff(win, COLOR_PAIR(COL_P_TITLE_WIN));
    } else {
        wattron(win, COLOR_PAIR(COL_P_TITLE_LOOSE));
        mvwprintw(win, board_height / 2, (board_width - 9) / 2, "Game Over");
        wattroff(win, COLOR_PAIR(COL_P_TITLE_LOOSE));
    }
    mvwprintw(win, board_height - 2, (board_width - 15) / 2, "Press Q to quit");
};

/**
 * @brief Draws all the obstacles (such as birds, storks, cars) on the game board.
 *
 * This function iterates through all obstacles and draws them at their respective positions,
 * using appropriate colors and symbols for different types of obstacles (e.g., cars, birds, storks).
 *
 * @param win The window to draw the obstacles on.
 * @param game The current game state, including the list of obstacles and the game board dimensions.
 */
void draw_obstacles(WINDOW *win, const Game *game) {
    Obstacle bird, stork;

    for (int i = 0; i < game->obs_count; i++) {
        Obstacle obstacle = game->obstacles[i];
        if (obstacle.type == BIRD) {
            bird = obstacle;
            continue;
        } else if (obstacle.type == STORK) {
            stork = obstacle;
            continue;
        } else if (obstacle.type == CAR_FRIENDLY) {
            obstacle.color_pair = game->friendly_car_ride ? COL_P_CAR_FRIENDLY_on : COL_P_CAR_FRIENDLY_off;
        }

        wattron(win, COLOR_PAIR(obstacle.color_pair));
        for (int x = 1; x <= game->board_width; x++) {
            mvwprintw(win, obstacle.y, x, " ");
        }
        mvwprintw(win, obstacle.y, obstacle.x, "%c", obstacle.skin);
        wattron(win, COLOR_PAIR(obstacle.color_pair));
    }
    // Draw birds always on top
    mvwaddch(win, bird.y, bird.x, bird.skin | COLOR_PAIR(bird.color_pair));
    mvwaddch(win, stork.y, stork.x, stork.skin | COLOR_PAIR(stork.color_pair));
}

/**
 * @brief Renders the game state to the screen.
 *
 * This function draws the entire game interface, including the scoreboard, player, obstacles,
 * exit, and other relevant game data. It updates the game window based on the current game state,
 * and displays the correct visuals depending on whether the game is over or still ongoing.
 *
 * @param win The window where the game is rendered.
 * @param game The current game state that determines what is drawn on the screen.
 */
void draw(WINDOW *win, const Game game) {
    draw_scoreboard(game, game.scores_tabele);
    draw_autor();
    werase(win);
    wbkgd(win, COLOR_PAIR(COL_P_BACKGROUND));

    // Box and board setup
    draw_board(game, win);

    if (game.end) {
        draw_communicates(win, game.win, game.board_height, game.board_width, game.play_time);
    } else {
        // Obstacle
        draw_obstacles(win, &game);

        // Exit
        wattron(win, COLOR_PAIR(COL_P_EXIT));
        mvwprintw(win, game.exit.y, game.exit.x - 1, "%c", game.exit.skin);
        wattroff(win, COLOR_PAIR(COL_P_EXIT));
        // Player
        wattron(win, COLOR_PAIR(COL_P_FROG));
        mvwprintw(win, game.frog.y, game.frog.x - 1, "%c", game.frog.skin);
        wattroff(win, COLOR_PAIR(COL_P_FROG));
    }
    wrefresh(win);
    usleep(DELAY);
};


/**
 * @brief Frees memory allocated for game levels.
 *
 * This function deallocates memory for the levels in the game. It frees the memory used
 * by each individual level and then frees the array that holds pointers to the levels.
 *
 * @param level A pointer to a pointer to an array of pointers to the levels.
 * @param level_count The total number of levels in the game.
 */
void free_mem(Level ***level, int level_count) {
    for (int i = 0; i < level_count; i++) {
        free(level[0][i]);
    }
    free(*level);
};

void hdl_info_screen(WINDOW *wind, int *ch) {
#if !TEST
    int last_ch = -1;
    while ((*ch == -1 && *ch != 'q') || *ch == 'h') {
        switch (*ch) {
            case 'h':
                clear();
                draw_info();
                *ch = wgetch(wind);
                break;
            default:
                *ch = wgetch(wind);
        }
    }
#endif
};

/**
 * @brief Main function of the game.
*/
int main(int argc, char *argv[]) {
    char file_name[] = SCOREBOARD_PATH;

    srand(time(nullptr));
    ncurses_init();

    Game game;
    game.levels = load_levels_file(CONF_PATH, &game.lvl_count);
    game.scores_tabele = read_array(file_name, SCOREBOARD_SIZE);

    WINDOW *win = init_window_centered(game.board_height, game.board_width);
    init_game(&game);

    level_init(&game, game.curr_lvl);

    draw_wait_for_start(win);
    int ch = wgetch(win);
    hdl_info_screen(win, &ch);

    time_t forg_move_dt = 0;
    int last_input_b = -1;


    clear();
    while (!game.end && ch != 'q') {
        time_t startTime = time(nullptr);
        game.frame++;

        win = init_window_centered(game.board_height, game.board_width);
        int input_b = getch();
        flushinp();


        if (input_b != last_input_b && input_b != -1) {
            forg_move_dt = 0;
            handle_controls(input_b, &game);
        }

#if !TEST
        if (forg_move_dt >= 5)
            game.end = true;
#endif

        move_obstacles(&game);
        check_win(&game);
        game.play_time = get_elapsed_time(&game.start_time);

        win = handle_game_win(win, &game);
        draw(win, game);

        forg_move_dt += time(nullptr) - startTime;
        last_input_b = input_b;
    }

    free_mem(&game.levels, game.lvl_count);
    while (ch != 'q') {
        ch = wgetch(win);
    }

    endwin();
    return 0;
}
