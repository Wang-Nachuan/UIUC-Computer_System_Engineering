/*
 * tab:4
 *
 * mazegame.c - main source file for ECE398SSL maze game (F04 MP2)
 *
 * "Copyright (c) 2004 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:        Steve Lumetta
 * Version:       1
 * Creation Date: Fri Sep 10 09:57:54 2004
 * Filename:      mazegame.c
 * History:
 *    SL    1    Fri Sep 10 09:57:54 2004
 *        First written.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "blocks.h"
#include "maze.h"
#include "modex.h"
#include "text.h"
#include "module/tuxctl-ioctl.h"

// New Includes and Defines
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/io.h>
#include <termios.h>
#include <pthread.h>

// Add
#include <string.h>

#define BACKQUOTE 96
#define UP        65
#define DOWN      66
#define RIGHT     67
#define LEFT      68

// Button press
#define TUX_BLANK   0
#define TUX_START   1
#define TUX_A       2
#define TUX_B       4
#define TUX_C       8
#define TUX_UP      16
#define TUX_DOWN    32
#define TUX_LEFT    64
#define TUX_RIGHT   128

/*
 * If NDEBUG is not defined, we execute sanity checks to make sure that
 * changes to enumerations, bit maps, etc., have been made consistently.
 */
#if defined(NDEBUG)
#define sanity_check() 0
#else
static int sanity_check();
#endif


/* a few constants */
#define PAN_BORDER      5  /* pan when border in maze squares reaches 5    */
#define MAX_LEVEL       10 /* maximum level number                         */

/* outcome of each level, and of the game as a whole */
typedef enum {GAME_WON, GAME_LOST, GAME_QUIT} game_condition_t;

/* structure used to hold game information */
typedef struct {
    /* parameters varying by level   */
    int number;                  /* starts at 1...                   */
    int maze_x_dim, maze_y_dim;  /* min to max, in steps of 2        */
    int initial_fruit_count;     /* 1 to 6, in steps of 1/2          */
    int time_to_first_fruit;     /* 300 to 120, in steps of -30      */
    int time_between_fruits;     /* 300 to 60, in steps of -30       */
    int tick_usec;         /* 20000 to 5000, in steps of -1750 */
    
    /* dynamic values within a level -- you may want to add more... */
    unsigned int map_x, map_y;   /* current upper left display pixel */
} game_info_t;

static game_info_t game_info;

/* local functions--see function headers for details */
static int prepare_maze_level(int level);
static void move_up(int* ypos);
static void move_right(int* xpos);
static void move_down(int* ypos);
static void move_left(int* xpos);
static int unveil_around_player(int play_x, int play_y, int* fnum);
static void *rtc_thread(void *arg);
static void *keyboard_thread(void *arg);

/* 
 * prepare_maze_level
 *   DESCRIPTION: Prepare for a maze of a given level.  Fills the game_info
 *          structure, creates a maze, and initializes the display.
 *   INPUTS: level -- level to be used for selecting parameter values
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: writes entire game_info structure; changes maze;
 *                 initializes display
 */
static int prepare_maze_level(int level) {
    int i; /* loop index for drawing display */
    
    /*
     * Record level in game_info; other calculations use offset from
     * level 1.
     */
    game_info.number = level--;

    /* Set per-level parameter values. */
    if ((game_info.maze_x_dim = MAZE_MIN_X_DIM + 2 * level) > MAZE_MAX_X_DIM)
        game_info.maze_x_dim = MAZE_MAX_X_DIM;
    if ((game_info.maze_y_dim = MAZE_MIN_Y_DIM + 2 * level) > MAZE_MAX_Y_DIM)
        game_info.maze_y_dim = MAZE_MAX_Y_DIM;
    if ((game_info.initial_fruit_count = 1 + level / 2) > 6)
        game_info.initial_fruit_count = 6;
    if ((game_info.time_to_first_fruit = 300 - 30 * level) < 120)
        game_info.time_to_first_fruit = 120;
    if ((game_info.time_between_fruits = 300 - 60 * level) < 60)
        game_info.time_between_fruits = 60;
    if ((game_info.tick_usec = 20000 - 1750 * level) < 5000)
        game_info.tick_usec = 5000;

    /* Initialize dynamic values. */
    game_info.map_x = game_info.map_y = SHOW_MIN;

    /* Create a maze. */
    if (make_maze(game_info.maze_x_dim, game_info.maze_y_dim, game_info.initial_fruit_count) != 0)
        return -1;
    
    /* Set logical view and draw initial screen. */
    set_view_window(game_info.map_x, game_info.map_y);
    for (i = 0; i < SCROLL_Y_DIM; i++)
        (void)draw_horiz_line (i);

    /* Return success. */
    return 0;
}

/* 
 * move_up
 *   DESCRIPTION: Move the player up one pixel (assumed to be a legal move)
 *   INPUTS: ypos -- pointer to player's y position (pixel) in the maze
 *   OUTPUTS: *ypos -- reduced by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void move_up(int* ypos) {
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the upper pan border
     * while the top pixels of the maze are not on-screen.
     */
    if (--(*ypos) < game_info.map_y + BLOCK_Y_DIM * PAN_BORDER && game_info.map_y > SHOW_MIN) {
        /*
         * Shift the logical view upwards by one pixel and draw the
         * new line.
         */
        set_view_window(game_info.map_x, --game_info.map_y);
        (void)draw_horiz_line(0);
    }
}

/* 
 * move_right
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: xpos -- pointer to player's x position (pixel) in the maze
 *   OUTPUTS: *xpos -- increased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void move_right(int* xpos) {
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the right pan border
     * while the rightmost pixels of the maze are not on-screen.
     */
    if (++(*xpos) > game_info.map_x + SCROLL_X_DIM - BLOCK_X_DIM * (PAN_BORDER + 1) &&
        game_info.map_x + SCROLL_X_DIM < (2 * game_info.maze_x_dim + 1) * BLOCK_X_DIM - SHOW_MIN) {
        /*
         * Shift the logical view to the right by one pixel and draw the
         * new line.
         */
        set_view_window(++game_info.map_x, game_info.map_y);
        (void)draw_vert_line(SCROLL_X_DIM - 1);
    }
}

/* 
 * move_down
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: ypos -- pointer to player's y position (pixel) in the maze
 *   OUTPUTS: *ypos -- increased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void move_down(int* ypos) {
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the right pan border
     * while the bottom pixels of the maze are not on-screen.
     */
    if (++(*ypos) > game_info.map_y + SCROLL_Y_DIM - BLOCK_Y_DIM * (PAN_BORDER + 1) && 
        game_info.map_y + SCROLL_Y_DIM < (2 * game_info.maze_y_dim + 1) * BLOCK_Y_DIM - SHOW_MIN) {
        /*
         * Shift the logical view downwards by one pixel and draw the
         * new line.
         */
        set_view_window(game_info.map_x, ++game_info.map_y);
        (void)draw_horiz_line(SCROLL_Y_DIM - 1);
    }
}

/* 
 * move_left
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: xpos -- pointer to player's x position (pixel) in the maze
 *   OUTPUTS: *xpos -- decreased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void move_left(int* xpos) {
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the left pan border
     * while the leftmost pixels of the maze are not on-screen.
     */
    if (--(*xpos) < game_info.map_x + BLOCK_X_DIM * PAN_BORDER && game_info.map_x > SHOW_MIN) {
        /*
         * Shift the logical view to the left by one pixel and draw the
         * new line.
         */
        set_view_window(--game_info.map_x, game_info.map_y);
        (void)draw_vert_line (0);
    }
}

/* 
 * unveil_around_player
 *   DESCRIPTION: Show the maze squares in an area around the player.
 *                Consume any fruit under the player.  Check whether
 *                player has won the maze level.
 *   INPUTS: (play_x,play_y) -- player coordinates in pixels
 *           fnum -- type number of fruit
 *   OUTPUTS: none
 *   RETURN VALUE: 1 if player wins the level by entering the square
 *                 0 if not
 *   SIDE EFFECTS: draws maze squares for newly visible maze blocks,
 *                 consumed fruit, and maze exit; consumes fruit and
 *                 updates displayed fruit counts
 */
static int unveil_around_player(int play_x, int play_y, int* fnum) {
    int x = play_x / BLOCK_X_DIM; /* player's maze lattice position */
    int y = play_y / BLOCK_Y_DIM;
    int i, j;            /* loop indices for unveiling maze squares */

    /* Check for fruit at the player's position. */
    *fnum = check_for_fruit (x, y) - 1;
    if (*fnum < 0) {
        *fnum = 0;
    }

    /* Unveil spaces around the player. */
    for (i = -1; i < 2; i++)
        for (j = -1; j < 2; j++)
            unveil_space(x + i, y + j);
        unveil_space(x, y - 2);
        unveil_space(x + 2, y);
        unveil_space(x, y + 2);
        unveil_space(x - 2, y);

    /* Check whether the player has won the maze level. */
    return check_for_win (x, y);
}

#ifndef NDEBUG
/* 
 * sanity_check 
 *   DESCRIPTION: Perform checks on changes to constants and enumerated values.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if checks pass, -1 if any fail
 *   SIDE EFFECTS: none
 */
static int sanity_check() {
    /* 
     * Automatically detect when fruits have been added in blocks.h
     * without allocating enough bits to identify all types of fruit
     * uniquely (along with 0, which means no fruit).
     */
    if (((2 * LAST_MAZE_FRUIT_BIT) / MAZE_FRUIT_1) < NUM_FRUIT_TYPES + 1) {
        puts("You need to allocate more bits in maze_bit_t to encode fruit.");
        return -1;
    }
    return 0;
}
#endif /* !defined(NDEBUG) */

// Shared Global Variables
int quit_flag = 0;
int winner= 0;
int next_dir = UP;
int play_x, play_y, last_dir, dir;
int move_cnt = 0;
int fd;
int fd_tux;
unsigned long data;
static struct termios tio_orig;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Additional global variables for tux control
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int tux_press_flag = 0;     // flag that indicate the press of tux
// Variable needed to update button press
unsigned char button_status = 0xFF;         // last status of all tux button
unsigned char button_status_new = 0xFF;     // current status of all tux button
unsigned char button_press = 0x0;           // last pressed button
unsigned char button_press_new = 0x0;       // current pressed button

/*
 * keyboard_thread
 *   DESCRIPTION: Thread that handles keyboard inputs
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void *keyboard_thread(void *arg) {
    char key;
    int state = 0;
    // Break only on win or quit input - '`'
    while (winner == 0) {        
        // Get Keyboard Input
        key = getc(stdin);
        
        // Check for '`' to quit
        pthread_mutex_lock(&mtx);
        if (key == BACKQUOTE) {
            quit_flag = 1;
            tux_press_flag = 1;
            // Signal the tux thread to stop
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mtx);
            break;
        }
        pthread_mutex_unlock(&mtx);

        // Compare and Set next_dir
        // Arrow keys deliver 27, 91, ##
        if (key == 27) {
            state = 1;
        }
        else if (key == 91 && state == 1) {
            state = 2;
        }
        else {    
            if (key >= UP && key <= LEFT && state == 2) {
                pthread_mutex_lock(&mtx);
                switch(key) {
                    case UP:
                        next_dir = DIR_UP;
                        break;
                    case DOWN:
                        next_dir = DIR_DOWN;
                        break;
                    case RIGHT:
                        next_dir = DIR_RIGHT;
                        break;
                    case LEFT:
                        next_dir = DIR_LEFT;
                        break;
                }
                pthread_mutex_unlock(&mtx);
            }
            state = 0;
        }
    }

    return 0;
}

/*------------------------------ Add ------------------------------*/

/*
 * tux_thread
 *   DESCRIPTION: Thread that handles tux inputs
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void *tux_thread(void *arg) {

    while (winner == 0) {   

        
        pthread_mutex_lock(&mtx);
        // Sleep to wait the button press
        while (tux_press_flag == 0) {
            pthread_cond_wait(&cond, &mtx);
        }
        // Check quit conditon
        if (quit_flag == 1) {
            pthread_mutex_unlock(&mtx);
            break;
        }

        // Reset flag
        tux_press_flag = 0;
        // Update the game state based on button press
        switch (button_press) {
            case TUX_RIGHT:
                next_dir = DIR_RIGHT;
                break;
            case TUX_DOWN:
                next_dir = DIR_DOWN;
                break;
            case TUX_LEFT:
                next_dir = DIR_LEFT;
                break;
            case TUX_UP:
                next_dir = DIR_UP;
                break;
            default:
                break;
        }
        pthread_mutex_unlock(&mtx);
    }

    return NULL;
}

/*
 * display_time
 *   DESCRIPTION: function that displays time in tux
 *   INPUTS: time - time from the starting of this level (in sec)
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: displays time in tux
 */
static void display_time(int time) {
    
    // Calculate second and minutes
    unsigned long ss = time % 60;       // second
    unsigned long ss_1 = ss % 10;       // low second
    unsigned long ss_2 = ss / 10;       // hight second
    unsigned long mm = time / 60;       // minute
    unsigned long arg = 0x04070000;     // Enable LED0, LED1, LED2. set decimal point at the LED2

    // Organize input value
    arg = arg | ss_1;
    arg = arg | (ss_2 << 4);
    arg = arg | (mm << 8);

    // Send instruction
    ioctl(fd_tux, TUX_SET_LED, arg);
}



/*------------------------------ End ------------------------------*/




/* some stats about how often we take longer than a single timer tick */
static int goodcount = 0;
static int badcount = 0;
static int total = 0;

/*
 * rtc_thread
 *   DESCRIPTION: Thread that handles updating the screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void *rtc_thread(void *arg) {
    int ticks = 0;
    int level;
    int ret;
    int open[NUM_DIRS];
    int need_redraw = 0;
    int goto_next_level = 0;

    /*------------------------------ Add ------------------------------*/
    // RGB glow color for player
    unsigned char palyer_glow_RGB[9][3] = {
        {0x3F, 0x3F, 0x3F},
        {0x4F, 0x4F, 0x3F},
        {0x5F, 0x5F, 0x3F},
        {0x6F, 0x6F, 0x3F},
        {0x7F, 0x7F, 0x3F},
        {0x6F, 0x6F, 0x3F},
        {0x5F, 0x5F, 0x3F},
        {0x4F, 0x4F, 0x3F},
        {0x3F, 0x3F, 0x3F}
    };

    // RGB color for level
    unsigned char level_RBG[10][3] = {
        {0x00, 0x00, 0x3F},
        {0x00, 0x64, 0x00},
        {0x8B, 0x86, 0x4E},
        {0x00, 0x00, 0x3F},
        {0x00, 0x64, 0x00},
        {0x8B, 0x86, 0x4E},
        {0x00, 0x00, 0x3F},
        {0x00, 0x64, 0x00},
        {0x8B, 0x86, 0x4E},
        {0x00, 0x00, 0x3F}
    };
    /*------------------------------ End ------------------------------*/

    // Loop over levels until a level is lost or quit.
    for (level = 1; (level <= MAX_LEVEL) && (quit_flag == 0); level++) {

        /*------------------------------ Add ------------------------------*/
        // Things need to do in each level
        // Additional variables
        int start = time(NULL);             // start time of current level
        int glow_t_loop = (int)clock();     // time counter for glow effect (in ms)
        int glow_c_count = 0;               // color counter for glow effect (0-9)
        unsigned char buf[BLOCK_Y_DIM*BLOCK_X_DIM];     // buffter that store the old value of masked pixel
        int temp_i;                                 // counter in following loop
        int num_fruit;                              // hold the number of fruit at the latest check
        int *type_fruit = NULL;                     // type of last found fruit, can be used directly as the index of text array
        int temp;                                   // temp value to initialize the pointer
        unsigned char TranText_img[IMG_Y*IMG_X];    // row major image information of transparent text
        unsigned char TranText_buf[IMG_Y*IMG_X];    // buffer that hold the old color
        int TranText_startT = 0;                    // starting time of displaying transparent text
        int TranText_Dflage = 0;                    // 1 means the text is currently being displayed
        int TranText_x, TranText_y;                 // coordinate of left-up cornner of text
        unsigned char RGB_white[3] = {0x3F, 0x3F, 0x3F};    // RGB value of white color
        static char* fruit_strings[8] = {
            "an apple!", "ew, grapes", "eh, it's a peach", "a strawberry", "A BANANA!", "melonwater", "Uh...Dew?", "Hidden fruit!"
        };      // Text for each fruit

        // For tux
        unsigned long *arg;     // argument to pass in tux driver
        unsigned long b = 0;    // used to initialize arg
        unsigned char mask;     // bit bask
        int i;                  // loop counter
        arg = &b;

        type_fruit = &temp;
        *type_fruit = 0;

        // Initialize the buffer
        for (temp_i = 0; temp_i < BLOCK_Y_DIM*BLOCK_X_DIM; temp_i++) {
            buf[temp_i] = 0;
        }

        // Update the color palette for wall and status bar
        set_color(0x22, level_RBG[level-1]);     // For simplicity we user same color index for wall and status bar
        
        // Set the color of text to white
        set_color(0x40, RGB_white);    // 0x40 is the color index of text, set it to white
        /*------------------------------ End ------------------------------*/

        // Prepare for the level.  If we fail, just let the player win.
        if (prepare_maze_level(level) != 0)
            break;
        goto_next_level = 0;

        // Start the player at (1,1)
        play_x = BLOCK_X_DIM;
        play_y = BLOCK_Y_DIM;

        // move_cnt tracks moves remaining between maze squares.
        // When not moving, it should be 0.
        move_cnt = 0;

        // Initialize last direction moved to up
        last_dir = DIR_UP;

        // Initialize the current direction of motion to stopped
        dir = DIR_STOP;
        next_dir = DIR_STOP;

        // Show maze around the player's original position
        (void)unveil_around_player(play_x, play_y, type_fruit);

        /*------------------------------ Modified ------------------------------*/
        // Old: draw_full_block(play_x, play_y, get_player_block(last_dir));
        draw_masked_block(play_x, play_y, get_player_block(last_dir), get_player_mask(last_dir), buf);    
        /*-------------------------------- End --------------------------------*/
        show_screen();
        /*------------------------------ Add ------------------------------*/
        // Erase player immediately after showing the screen
        undraw_masked_block(play_x, play_y, get_player_mask(last_dir), buf);
        /*------------------------------ End ------------------------------*/

        /*------------------------------ Add ------------------------------*/
        // Draw the status bar
        show_bar(level, get_n_fruits(), time(NULL) - start);

        num_fruit = get_n_fruits();
        /*------------------------------ End ------------------------------*/

        // get first Periodic Interrupt
        ret = read(fd, &data, sizeof(unsigned long));

        while ((quit_flag == 0) && (goto_next_level == 0)) {
            // Wait for Periodic Interrupt
            ret = read(fd, &data, sizeof(unsigned long));

            /*------------------------------ Add ------------------------------*/
            // Things need to do in each loop
            // Draw the status bar
            show_bar(level, get_n_fruits(), time(NULL) - start);

            // Update the head color of player in every 100000us
            if ((int)clock() - glow_t_loop > 100000) {
                set_color(0x20, palyer_glow_RGB[glow_c_count]);  // 0x20 is the index of color in player's head
                glow_c_count = (glow_c_count + 1) % 9;
                glow_t_loop = (int)clock();
            }

            // Check whether num_fruit has decreased, if so, generate text to be display
            if (get_n_fruits() < num_fruit) {
                num_fruit = get_n_fruits();
                TranText_startT = time(NULL);
                TranText_Dflage = 1;
                text_to_image(fruit_strings[*type_fruit], strlen(fruit_strings[*type_fruit]), 1, 0, TranText_img);      // 1 means have a pixel, 0 means not
            }   
            /*------------------------------ End ------------------------------*/
        
            // Update tick to keep track of time.  If we missed some
            // interrupts we want to update the player multiple times so
            // that player velocity is smooth
            ticks = data >> 8;    

            total += ticks;

            // If the system is completely overwhelmed we better slow down:
            if (ticks > 8) ticks = 8;

            if (ticks > 1) {
                badcount++;
            }
            else {
                goodcount++;
            }

            while (ticks--) {

                /*------------------------------ Add ------------------------------*/
                // Check button press in tux;
                ioctl(fd_tux, TUX_BUTTONS, (unsigned long)arg);
                button_status_new = (unsigned char) *arg;
                
                // Find the newly pressed bottun, i.e. the first botton that change from 1 to 0
                for (i = 0; i < 8; i++) {
                    mask = 0x01 << i;
                    if (((button_status & mask) != 0) && ((button_status_new & mask) == 0)) {
                        button_press_new = mask;
                        tux_press_flag = 1;
                        break;
                    }
                }

                button_status = button_status_new;

                // Compare two adjacent press, if they are the same, just continue
                if (button_press_new != button_press) {
                    button_press = button_press_new;
                }
                
                // Wake up the tux thread
                pthread_mutex_lock(&mtx);
                if (tux_press_flag != 0) {
                    pthread_cond_signal(&cond);
                }
                pthread_mutex_unlock(&mtx);

                /*------------------------------ End ------------------------------*/

                // Lock the mutex
                pthread_mutex_lock(&mtx);

                // Check to see if a key has been pressed
                if (next_dir != dir) {
                    // Check if new direction is backwards...if so, do immediately
                    if ((dir == DIR_UP && next_dir == DIR_DOWN) ||
                        (dir == DIR_DOWN && next_dir == DIR_UP) ||
                        (dir == DIR_LEFT && next_dir == DIR_RIGHT) ||
                        (dir == DIR_RIGHT && next_dir == DIR_LEFT)) {
                        if (move_cnt > 0) {
                            if (dir == DIR_UP || dir == DIR_DOWN)
                                move_cnt = BLOCK_Y_DIM - move_cnt;
                            else
                                move_cnt = BLOCK_X_DIM - move_cnt;
                        }
                        dir = next_dir;
                    }
                }
                // New Maze Square!
                if (move_cnt == 0) {
                    // The player has reached a new maze square; unveil nearby maze
                    // squares and check whether the player has won the level.
                    if (unveil_around_player(play_x, play_y, type_fruit)) {
                        pthread_mutex_unlock(&mtx);
                        goto_next_level = 1;
                        break;
                    }
                
                    // Record directions open to motion.
                    find_open_directions (play_x / BLOCK_X_DIM, play_y / BLOCK_Y_DIM, open);
        
                    // Change dir to next_dir if next_dir is open 
                    if (open[next_dir]) {
                        dir = next_dir;
                    }
    
                    // The direction may not be open to motion...
                    //   1) ran into a wall
                    //   2) initial direction and its opposite both face walls
                    if (dir != DIR_STOP) {
                        if (!open[dir]) {
                            dir = DIR_STOP;
                        }
                        else if (dir == DIR_UP || dir == DIR_DOWN) {    
                            move_cnt = BLOCK_Y_DIM;
                        }
                        else {
                            move_cnt = BLOCK_X_DIM;
                        }
                    }
                }
                // Unlock the mutex
                pthread_mutex_unlock(&mtx);
        
                if (dir != DIR_STOP) {
                    // move in chosen direction
                    last_dir = dir;
                    move_cnt--;    
                    switch (dir) {
                        case DIR_UP:    
                            move_up(&play_y);    
                            break;
                        case DIR_RIGHT: 
                            move_right(&play_x); 
                            break;
                        case DIR_DOWN:  
                            move_down(&play_y);  
                            break;
                        case DIR_LEFT:  
                            move_left(&play_x);  
                            break;
                    } 
                }
                
            }

            
            /*------------------------------ Add ------------------------------*/
            // Old: draw_full_block(play_x, play_y, get_player_block(last_dir));
            draw_masked_block(play_x, play_y, get_player_block(last_dir), get_player_mask(last_dir), buf);

            // Display the floating text
            if (TranText_Dflage == 1) {
                TranText_x = play_x - 150;
                TranText_y = play_y - 15;
                if (TranText_y < 5)
                    TranText_y = 5;
                draw_float_text(TranText_x, TranText_y, TranText_img, IMG_X, IMG_Y, TranText_buf);
            }    
            /*------------------------------ End ------------------------------*/
            show_screen();
            /*------------------------------ Add ------------------------------*/
            // Erase the floating text
            if (TranText_Dflage == 1) {
                // If time is over (here it last for 5 seconds), stop displaying and restore variables
                if (time(NULL) - TranText_startT > 5)
                    TranText_Dflage = 0;
                undraw_float_text(TranText_x, TranText_y, TranText_img, IMG_X, IMG_Y, TranText_buf);
            }

            // Erase player immediately after showing the screen */
            undraw_masked_block(play_x, play_y, get_player_mask(last_dir), buf);

            display_time(time(NULL) - start);
            /*------------------------------ End ------------------------------*/
            need_redraw = 0;
        }    
    }
    if (quit_flag == 0)
        winner = 1;
    
    return 0;
}

/*
 * main
 *   DESCRIPTION: Initializes and runs the two threads
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int main() {
    int ret;
    struct termios tio_new;
    unsigned long update_rate = 32; /* in Hz */

    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;     // tux thread

    // Initialize RTC
    fd = open("/dev/rtc", O_RDONLY, 0);

    // Open tux controller 
    fd_tux = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    int ldisc_num = N_MOUSE;
    ioctl(fd_tux, TIOCSETD, &ldisc_num);

    ioctl(fd_tux, TUX_INIT, 0);

    /* Grant ourselves permission to use ports 0-1023 */
    // if (ioperm(0, 1024, 1) == -1) {
    //     perror("ioperm");
    //     return 3;
    // }
    
    // Enable RTC periodic interrupts at update_rate Hz
    // Default max is 64...must change in /proc/sys/dev/rtc/max-user-freq
    ret = ioctl(fd, RTC_IRQP_SET, update_rate);    
    ret = ioctl(fd, RTC_PIE_ON, 0);

    // Initialize Keyboard
    // Turn on non-blocking mode
    if (fcntl(fileno(stdin), F_SETFL, O_NONBLOCK) != 0) {
        perror("fcntl to make stdin non-blocking");
        return -1;
    }
    
    // Save current terminal attributes for stdin.
    if (tcgetattr(fileno(stdin), &tio_orig) != 0) {
        perror("tcgetattr to read stdin terminal settings");
        return -1;
    }
    
    // Turn off canonical (line-buffered) mode and echoing of keystrokes
    // Set minimal character and timing parameters so as
    tio_new = tio_orig;
    tio_new.c_lflag &= ~(ICANON | ECHO);
    tio_new.c_cc[VMIN] = 1;
    tio_new.c_cc[VTIME] = 0;
    if (tcsetattr(fileno(stdin), TCSANOW, &tio_new) != 0) {
        perror("tcsetattr to set stdin terminal settings");
        return -1;
    }

    // Perform Sanity Checks and then initialize input and display
    if ((sanity_check() != 0) || (set_mode_X(fill_horiz_buffer, fill_vert_buffer) != 0)){
        return 3;
    }

    // Create the threads
    pthread_create(&tid1, NULL, rtc_thread, NULL);
    pthread_create(&tid2, NULL, keyboard_thread, NULL);
    pthread_create(&tid3, NULL, tux_thread, NULL);
    
    // Wait for all the threads to end
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    // Shutdown Display
    clear_mode_X();
    
    // Close Keyboard
    (void)tcsetattr(fileno(stdin), TCSANOW, &tio_orig);
        
    // Close RTC
    close(fd);
    close(fd_tux);

    // Print outcome of the game
    if (winner == 1) {    
        printf("You win the game! CONGRATULATIONS!\n");
    }
    else if (quit_flag == 1) {
        printf("Quitter!\n");
    }
    else {
        printf ("Sorry, you lose...\n");
    }

    // Return success
    return 0;
}
