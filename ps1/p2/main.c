/*
 * tab:2
 *
 * main.c - I/O and problem setup
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
 * Author:        Harsh Agrawal
 * Version:       1
 * Creation Date: Tue Jan 26 2021
 * Filename:      main.c
 * History:
 *    HA    1    Tue Jan 26 2021
 *        First written.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "kthsmallest.h"

int* import_arr(char* fname, int* num_elements) {
  FILE* f = fopen(fname, "r");
  int i, element;

  fscanf(f, "%d\n", num_elements);

  int *arr = malloc((*num_elements) * sizeof(int));
  
  for (i = 0; i < *num_elements; i++) {
    fscanf(f, "%d\n", &element);
    arr[i] = element;
  }

  fclose(f);
  return arr;
}


int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: ./kthsmallest <input_file>\n");
    return -1;
  }
  char fname[20];
  
  int k;
  
  FILE* f = fopen(argv[1], "r");
  fscanf(f, "%s %d", fname, &k);
  fclose(f);

  int num_elements;
  int *arr = import_arr(fname, &num_elements);
  printf("Read array of size %d\n", num_elements);

  printf("\nBeginning C search\n");
  printf("kthSmallest_c find %d: %d\n", k, kthSmallest_c(arr, 0, num_elements-1, k));
  free(arr);

  arr = import_arr(fname, &num_elements);
  printf("\nBeginning ASM search\n");
  printf("kthSmallest_asm find %d: %d\n", k, kthSmallest_asm(arr, 0, num_elements-1, k));
  free(arr);

  return 0;
}
