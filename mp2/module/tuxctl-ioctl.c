/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)s

/*-------------------- File domain variables --------------------*/
static unsigned char button_status = 0x0;						// status of button at last check, form: 7 {right, left, down, up, c, b, a, start} 0
static unsigned char LED_status[4] = {0x0, 0x0, 0x0, 0x0};		// status of 4 LED light, form: {LED0, LED1, LED2, LED3}
static volatile int is_busy = FREE;						// flag that indicate wether the device is busy
static spinlock_t the_lock = SPIN_LOCK_UNLOCKED;		// lock that protect is_busy
static unsigned long flags;								// store the IF value

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	unsigned char button_status_new;		// new status of button
	unsigned char bit_down, bit_left;		// store some temperate bits
	unsigned char buf[8];			    	// buffer that stores instructions

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

	switch (a) {
		
		/* Receive MTCP_ACK */
		case MTCP_ACK: {

			spin_lock_irqsave(&the_lock, flags);
			is_busy = FREE;
			spin_unlock_irqrestore(&the_lock, flags);

			// printk("(Test point) MTCP_ACK recived, is_busy: %d\n", is_busy);
			break;
		}
		
		/* Receive MTCP_BIOC_EVENT */ 
		case MTCP_BIOC_EVENT: {

			// Get the new status of button
			button_status_new = ((c << 4) & 0xF0) | (b & 0x0F);

			// Switch the down and left bit
			bit_down = (c & 0x04) << 3;
			bit_left = (c & 0x02) << 5;
			button_status_new = (button_status_new & 0xDF) | bit_down;		// 0xDF -> b'1101 1111
			button_status_new = (button_status_new & 0xBF) | bit_left;		// 0xBF -> b'1011 1111

			// Assign it to current button status
			button_status = button_status_new;

			break;
		}

		/* Receive MTCP_RESET */
		case MTCP_RESET: {

			// printk("(Test point) MTCP_RESET recived\n");

			// Initialize the tux
			buf[0] = MTCP_BIOC_ON;
			buf[1] = MTCP_LED_USR;
			tuxctl_ldisc_put(tty, buf, 2);

			// Restore the LED display
			buf[0] = MTCP_LED_SET;
			buf[1] = 0x0F;
			buf[2] = LED_status[0];
			buf[3] = LED_status[1];
			buf[4] = LED_status[2];
			buf[5] = LED_status[3];
			// Check whether the device is busy
			spin_lock_irqsave(&the_lock, flags);
			if (is_busy == BUSY) {
				spin_unlock_irqrestore(&the_lock, flags);
				return;
			}
			is_busy = BUSY;
			spin_unlock_irqrestore(&the_lock, flags);
			// If so, issue the instruction
			tuxctl_ldisc_put(tty, buf, 6);
			break;
		}

		default:
			break;
	}
    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
	unsigned char buf[6];		// buffer that stores instructions
	unsigned char LED_mask;		// low four bit is the mask of 4 LED
	int val;					// value to be displayed on LED
	int i;						// counter for loop
	unsigned char LED[16] = {
		0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 
		0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8
	};							// 7-segment display information for hex value (0-F, without dot)

    switch (cmd) {

		/* Initializes any variables associated with the driver and returns 0 */

		case TUX_INIT: {

			// printk("(Test point) TUX_INIT started\n");

			// Return if the device is currently busy
			spin_lock_irqsave(&the_lock, flags);
			is_busy = BUSY;
			spin_unlock_irqrestore(&the_lock, flags);
			
			// Send instruction
			buf[0] = MTCP_BIOC_ON;
			buf[1] = MTCP_LED_USR;
			buf[2] = MTCP_LED_SET;
			buf[3] = 0x0F;
			buf[4] = 0x0;
			buf[5] = 0x0;
			buf[6] = 0x0;
			buf[7] = 0x0;
			tuxctl_ldisc_put(tty, buf, 8);

			// Also initialize inner status
			LED_status[0] = 0x0;
			LED_status[1] = 0x0;
			LED_status[2] = 0x0;
			LED_status[3] = 0x0;

			// printk("(Test point) TUX_INIT finished\n");

			return 0;
		}
			
		/* Get the button status fo tux */
		case TUX_BUTTONS: {
			
			// printk("(Test point) TUX_BUTTONS started\n");

			// Check the input pointer
			if (arg == 0)
				return -EINVAL;
			
			// Set the value of pointer
			buf[0] = button_status;
			copy_to_user((unsigned long*)arg, buf, 1);

			// printk("(Test point) button status: %x\n", button_status);
			// printk("(Test point) TUX_BUTTONS finished\n");

			return 0;
		}
			
		/* Set the LED */
		case TUX_SET_LED: {
			
			// printk("(Test point) TUX_SET_LED started\n");

			// Set the LED to user mode
			buf[0] = MTCP_LED_SET;
			// Specify the LEDs to be set (The low 4 bits of the third byte)
			LED_mask = (arg >> 16) & 0x0F;
			buf[1] = 0x0F;
			// Convert given value to LED display numbers
			for (i = 0; i < 4; i++) {
				// If the i-th LED needs to be turn on, add corresponding LED value to buffer
				if (((LED_mask >> i) & 0x01) == 1) {
					val = (arg >> (4*i)) & 0x0F;
					buf[i+2] = LED[val];
					LED_status[i] = LED[val];
				} else {
					// Otherwise, set the LED to blank
					buf[i+2] = 0x00;
					LED_status[i] = 0x00;
				}
				// Check whether the decimal should be displayed
				if (((arg >> (24+i)) & 0x01) == 1) {
					buf[i+2] = buf[i+2] | 0x10;
					LED_status[i] =  LED_status[i] | 0x10;	// Record latest LED status
				}
			}

			// Return if the device is currently busy
			spin_lock_irqsave(&the_lock, flags);
			if (is_busy == BUSY) {
				spin_unlock_irqrestore(&the_lock, flags);
				return 0;
			}
			is_busy = BUSY;
			spin_unlock_irqrestore(&the_lock, flags);
			
			// Pass instruction
			tuxctl_ldisc_put(tty, buf, 6);

			// printk("(Test point) TUX_SET_LED finished\n");

			return 0;
		}

		/* Ignoring following three cases in this mp  */
		case TUX_LED_ACK:
			return 0;
		case TUX_LED_REQUEST:
			return 0;
		case TUX_READ_LED:
			return 0;
		default:
			return -EINVAL;
    }
}
