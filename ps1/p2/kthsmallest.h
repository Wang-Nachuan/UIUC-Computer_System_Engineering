/*
 * tab:2
 *
 * kthsmallest.h - kthSmallest function & helper function declarations
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
 * Filename:      kthsmallest.h
 * History:
 *    HA    1    Tue Jan 26 2021
 *        First written.
 */

#ifndef KTHSMALLEST_H
#define KTHSMALLEST_H

// swaps values at given addresses
extern void swap(int *a, int *b);

// kthSmallest_c
//   Finds the kth smallest element in the given array.
extern int kthSmallest_c(int arr[], int left, int right, int k);


// kthSmallest_c
//   Finds the kth smallest element in the given array.
// TODO: Implement this function in x86 Assembly in kthsmallest_asm.S
extern int kthSmallest_asm(int arr[], int left, int right, int k);

#endif
