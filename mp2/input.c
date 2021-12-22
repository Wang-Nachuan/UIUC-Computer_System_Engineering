/*
 * tab:4
 *
 * input.c - source file for input control to maze game
 *
 * "Copyright (c) 2004-2009 by Steven S. Lumetta."
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
 * Version:       5
 * Creation Date: Thu Sep  9 22:25:48 2004
 * Filename:      input.c
 * History:
 *    SL    1    Thu Sep  9 22:25:48 2004
 *        First written.
 *    SL    2    Sat Sep 12 14:34:19 2009
 *        Integrated original release back into main code base.
 *    SL    3    Sun Sep 13 03:51:23 2009
 *        Replaced parallel port with Tux controller code for demo.
 *    SL    4    Sun Sep 13 12:49:02 2009
 *        Changed init_input order slightly to avoid leaving keyboard in odd state on failure.
 *    SL    5    Sun Sep 13 16:30:32 2009
 *        Added a reasonably robust direct Tux control for demo mode.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <termio.h>
#include <termios.h>
#include <unistd.h>

#include <time.h>

#include "assert.h"
#include "input.h"
#include "maze.h"
#include "module/tuxctl-ioctl.h"

/* set to 1 and compile this file by itself to test functionality */
#define TEST_INPUT_DRIVER  1

/* set to 1 to use tux controller; otherwise, uses keyboard input */
#define USE_TUX_CONTROLLER 1

/* stores original terminal settings */
static struct termios tio_orig;

/* 
 * init_input
 *   DESCRIPTION: Initializes the input controller.  As both keyboard and
 *                Tux controller control modes use the keyboard for the quit
 *                command, this function puts stdin into character mode
 *                rather than the usual terminal mode.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure 
 *   SIDE EFFECTS: changes terminal settings on stdin; prints an error
 *                 message on failure
 */
int init_input() {
    struct termios tio_new;

    /*
     * Set non-blocking mode so that stdin can be read without blocking
     * when no new keystrokes are available.
     */
    if (fcntl(fileno(stdin), F_SETFL, O_NONBLOCK) != 0) {
        perror("fcntl to make stdin non-blocking");
        return -1;
    }

    /*
     * Save current terminal attributes for stdin.
     */
    if (tcgetattr(fileno(stdin), &tio_orig) != 0) {
        perror ("tcgetattr to read stdin terminal settings");
        return -1;
    }

    /*
     * Turn off canonical (line-buffered) mode and echoing of keystrokes
     * to the monitor.  Set minimal character and timing parameters so as
     * to prevent delays in delivery of keystrokes to the program.
     */
    tio_new = tio_orig;
    tio_new.c_lflag &= ~(ICANON | ECHO);
    tio_new.c_cc[VMIN] = 1;
    tio_new.c_cc[VTIME] = 0;
    if (tcsetattr(fileno(stdin), TCSANOW, &tio_new) != 0) {
        perror("tcsetattr to set stdin terminal settings");
        return -1;
    }

    /* Return success. */
    return 0;
}

// /* 
//  * get_command
//  *   DESCRIPTION: Reads a command from the input controller.  As some
//  *                controllers provide only absolute input (e.g., go
//  *                right), the current direction is needed as an input
//  *                to this routine.
//  *   INPUTS: cur_dir -- current direction of motion
//  *   OUTPUTS: none
//  *   RETURN VALUE: command issued by the input controller
//  *   SIDE EFFECTS: drains any keyboard input
//  */
// cmd_t get_command(int fd, unsigned char bit_status) {

//     cmd_t command;
//     unsigned long *arg; 
//     unsigned long b = 0;
//     unsigned char bit_status_new = 0;
//     arg = &b;
    
//     /* Read all characters from stdin. */
//     ioctl(fd, TUX_BUTTONS, (unsigned long)arg);

//     c = (unsigned char) (*arg);

//     printf("%d\n", c);

//     if (c == p_1) {
//         command = C; 
//     } else if (c == p_2) {
//         command = B;
//     } else if (c == p_3) {
//         command = A;
//     } else if (c == p_4) {
//         command = START;
//     } else if (c == p_5) {
//         command = RIGHT;
//     } else if (c == p_6) {
//         command = DOWN;
//     } else if (c == p_7) {
//         command =LEFT;
//     } else if (c == p_8) {
//         command = UP;
//     } else {
//         command = BLAKN;
//     }

//     return command;
// }

/* 
 * shutdown_input
 *   DESCRIPTION: Cleans up state associated with input control.  Restores
 *                original terminal settings.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: restores original terminal settings
 */
void shutdown_input() {
    (void)tcsetattr(fileno(stdin), TCSANOW, &tio_orig);
}

/* 
 * display_time_on_tux
 *   DESCRIPTION: Show number of elapsed seconds as minutes:seconds
 *                on the Tux controller's 7-segment displays.
 *   INPUTS: num_seconds -- total seconds elapsed so far
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: changes state of controller's display
 */
// void display_time_on_tux(int num_seconds) {
// #if (USE_TUX_CONTROLLER != 0)
// #error "Tux controller code is not operational yet."
// #endif
// }

#if (TEST_INPUT_DRIVER == 1)
int main() {
    unsigned long LED_counter = 0;
    int fd;
    unsigned long i;
    unsigned char button_status = 0xFF;         // last status of all tux button
    unsigned char button_status_new = 0xFF;     // current status of all tux button
    unsigned char button_press = 0x0;           // last pressed button
    unsigned char button_press_new = 0x0;       // current pressed button
    unsigned char mask;

    unsigned long *arg; 
    unsigned long b = 0;
    arg = &b;
    int ch;

    int idx = 0;
    // LED Test 1: display
    unsigned long test_LED_1[16] = {
        // Digit and decimal point display
        0x000F1234,
        0x010F5678,
        0x020F9ABC,
        0x030FDEF0,
        // Mask
        0x04004321,
        0x05014322,
        0x06024323,
        0x07044324,
        0x08084325,
        0x09034326,
        0x0A044327,
        0x0B054328,
        0x0C064329,
        0x0D07432A,
        0x0E09432B,
        0x0F0D432C
    };
    

    // Open tux controller 
    fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    int ldisc_num = N_MOUSE;
    ioctl(fd, TIOCSETD, &ldisc_num);

    init_input();


    printf("Check point 1\n");

    ioctl(fd, TUX_INIT, arg);

    while (1) {

        ch = getc(stdin);
        if (ch == '`')
            break;

        // LED Test 2: spam
        for (LED_counter = 0; LED_counter < 0xFFFFFFFF; LED_counter++) {
            ioctl(fd, TUX_SET_LED, LED_counter);
        }

        // ioctl(fd, TUX_BUTTONS, (unsigned long)arg);
        // button_status_new = (unsigned char) *arg;

        // // Button test
        // // printf("Button status: %02x\n", (unsigned char)~button_status_new);

        // // Find the newly pressed bottun, i.e. the first botton that change from 1 to 0
        // for (i = 0; i < 8; i++) {
        //     mask = 0x01 << i;
        //     if (((button_status & mask) != 0) && ((button_status_new & mask) == 0)) {
        //         button_press_new = mask;
        //         break;
        //     }
        // }

        // button_status = button_status_new;

        // // Compare two adjacent press, if they are the same, just continue
        // if (button_press_new == button_press) {
        //     continue;
        // } else {
        //     button_press = button_press_new;
        // }

        // if (button_press == RIGHT) {
        //     idx = (idx + 1) % 16;
        // } else if (button_press == LEFT) {
        //     idx = (idx - 1) % 16;
        // } else {}
            

        // switch (button_press) {
        //     case A:
        //         break;
        //     case B:
        //         break;
        //     case C:
        //         break;
        //     case START:
        //         break;
        //     case RIGHT:
        //         idx = (idx + 1) % 16;
        //         break;
        //     case DOWN:
        //         break;
        //     case LEFT:
        //         idx = (idx - 1) % 16;
        //         break;
        //     case UP:
        //         break;
        //     default: 
        //         break;
        // }

        // ioctl(fd, TUX_SET_LED, test_LED_1[idx]);
    }
    
    shutdown_input();
    return 0;
}
#endif
