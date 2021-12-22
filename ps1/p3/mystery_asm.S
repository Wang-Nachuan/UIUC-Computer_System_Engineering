/*
 * tab:2
 *
 * mystery_asm.S - Assembly mystery
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
 * Filename:      mystery_asm.S
 * History:
 *    AH    1    Sun Jan 26 2020
 *        First written.
 */

.data

.text
.global mystery_asm

# mystery (Assembly)
# Calculates the mystery function of the two input numbers
# 
# Registers:
#   eax - For devision op & Return Value
#   ebx - x
#   ecx - y
#   edx - For division op
#   edi - greater number

mystery_asm:
  # 0. Save old base pointer and get new one
  pushl %ebp
  movl %esp, %ebp

  # 1. Save callee-saved registers
  pushl %ebx
  pushl %edi

  # Load inputs
  movl 12(%ebp), %edi   # edi <- y
  movl 12(%ebp), %ebx   # ebx <- y
  movl 8(%ebp), %ecx    # ecx <- x

  cmpl $0, %ebx   # y - 0
  jle invalid_input   # if y <= 0, invalid

  cmpl $0, %ecx
  jle invalid_input   # if x <= 0, invalid

  cmpl %ebx, %ecx   
  je invalid_input    # if x == y, invalid
  jl op_loop

  movl 8(%ebp), %edi  # edi(y) <- x

op_loop:
  xorl %edx, %edx
  movl %edi, %eax   # eax <- z
  idiv %ebx         # eax <- z/y, edx <- remainder
  cmpl $0, %edx
  jne repeat 

  xorl %edx, %edx
  movl %edi, %eax
  idiv %ecx
  cmpl $0, %edx
  jne repeat

  movl %edi, %eax
  jmp finish 

repeat:
  incl %edi
  jmp op_loop

invalid_input:
  movl $-1, %eax

finish:
  popl %edi
  popl %ebx

  leave
  ret
