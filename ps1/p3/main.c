/*
 * tab:2
 *
 * main.c - I/O and mystery problem setup
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
 * Author:        Aamir Hasan
 * Version:       1
 * Creation Date: Sun Jan 26 2020
 * Filename:      main.c
 * History:
 *    AH    1    Fri Jan 26 2020
 *        First written.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mystery.h"

int import(char* infile, int* a, int* b) {
  FILE* f = fopen(infile, "r");
  if(f == NULL) return -1;
  fscanf(f, "%d %d\n", a, b);
  fclose(f);
  return 0;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: ./mystery <input file>\n");
    return -1;
  }
  int a = 0;
  int b = 0;
  import(argv[1], &a, &b);
  printf("Running C Code\n");
  printf("Mystery(%d, %d) = %d\n\n", a, b, mystery_c(a, b));
  printf("Running ASM Code\n");
  printf("Mystery(%d, %d) = %d\n", a, b, mystery_asm(a, b));
  return 0;
}
