/*
 * tab:4
 *
 * text.h - font data and text to mode X conversion utility header file
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
 * Version:       2
 * Creation Date: Thu Sep  9 22:08:16 2004
 * Filename:      text.h
 * History:
 *    SL    1    Thu Sep  9 22:08:16 2004
 *        First written.
 *    SL    2    Sat Sep 12 13:40:11 2009
 *        Integrated original release back into main code base.
 */

#ifndef TEXT_H
#define TEXT_H

/* The default VGA text mode font is 8x16 pixels. */
#define FONT_WIDTH   8
#define FONT_HEIGHT  16

/* Parameters definded more check point 1 functions */
#define MAX_LENTH   32      // Maximun number of characters of input string (320/(8+2)=32)
#define IMG_X       320     // x size of image buffer
#define IMG_Y       18      // y size of image buffer

/* Standard VGA text font. */
extern unsigned char font_data[256][16];

/*
 * text_to_image
 *   DESCRIPTION: text to graphics image generation routine, characters are 
 *                centerd at the bar 
 *   INPUTS: 
 *          str  --  an array of ASCII characters, the length is set to 30 for convinence 
 *          len  --  length of string (note: it is user's responsibility to make sure len <= 32)
 *          color_c  --  8-bit color value of text
 *          color_b  --  8-bit color value of background
 *          buf   --  an 18(row) x 320(col) buffer that hold the graphical image of ASCII character; left-up conner is the origin
 *   OUTPUTS: buf
 *   RETURN VALUE: none 
 *   IDE EFFECTS: none
 */
void text_to_image(char str[30], int len, unsigned char color_c, unsigned char color_b, unsigned char buf[IMG_Y*IMG_X]);

#endif /* TEXT_H */
