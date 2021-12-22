/*
 * tab:2
 *
 * kthsmallest_asm.S - Implementation of Assembly QuickSelect
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
 * Filename:      kthsmallest_asm.S
 * History:
 *    HA    1    Tue Jan 26 2021
 *        First written.
 */

.text
.global kthSmallest_asm

# kthsmallest ASM (Assembly)
# Finds the kth smallest element in an array
# Declaration is in kthsmallest.h
#
# Registers:
#   eax - Return Value
# 
kthSmallest_asm:

   # Argument address
   # arr[]  -  8(%ebp)
   # left   -  12(%ebp)
   # right  -  16(%ebp)
   # k      -  20(%ebp)

   # 0. Save old base pointer and get new one
   pushl %ebp
   movl %esp, %ebp

   # 1. Save callee-saved registers
   pushl %ebx  # base
   pushl %esi  # source index
   pushl %edi  # destination index

   # 2. Make space for local variables
   # pivot (addr = -16(%ebp))
   movl 8(%ebp), %ebx   # ebx <- *arr
   movl 16(%ebp), %edi  # edi <- right
   movl (%ebx, %edi, 4), %eax    # eax <- arr[right]
   pushl %eax
   # i (addr = -20(%ebp))
   movl 12(%ebp), %eax
   pushl %eax

   # 3. Excute the function body
   # -----------------------------------
   # Loop
   movl 12(%ebp), %ecx  # j <- left
Loop:
   # If j >= right, stop
   movl 16(%ebp), %eax
   cmpl %eax, %ecx
   jge Swap2   
   # If arr[j] > pivot, continue
   movl 8(%ebp), %ebx
   movl (%ebx, %ecx, 4), %eax   # eax <- arr[j]
   movl -16(%ebp), %ebx     # ebx <- pivot
   cmpl %ebx, %eax
   jg Incr
   # Call swap function
   # 0. Save caller save registers
   pushl %eax
   pushl %edx
   pushl %ecx
   pushfl
   # 1. Push arguments
   movl 8(%ebp), %ebx
   leal (%ebx, %ecx, 4), %eax
   push %eax   # push &arr[j]
   movl -20(%ebp), %edi
   leal (%ebx, %edi, 4), %eax
   pushl %eax   # push &arr[i]
   # 2. Call the function
   CALL swap
   # 3. Pop arguments
   addl $8, %esp
   # 4. restore caller saved registers
   popfl
   popl %ecx
   popl %edx
   popl %eax
   # Continue excution
   movl -20(%ebp), %eax
   addl $1, %eax
   movl %eax, -20(%ebp)
Incr:
   addl $1, %ecx
   jmp Loop

   # Call swap function
Swap2:
   # 0. Save caller save registers
   pushl %eax
   pushl %edx
   pushl %ecx
   pushfl
   # 1. Push arguments
   movl 8(%ebp), %ebx
   LEA (%ebx, %ecx, 4), %eax
   pushl %eax   # push &arr[j]
   movl -20(%ebp), %edi
   LEA (%ebx, %edi, 4), %eax
   pushl %eax   # push &arr[i]
   # 2. Call the function
   CALL swap
   # 3. Pop arguments
   addl $8, %esp
   # 4. restore caller saved registers
   popfl
   popl %ecx
   popl %edx
   popl %eax

   # If k==i, return
   movl 20(%ebp), %eax  # eax <- k
   movl -20(%ebp), %ebx  # ebx <- i
   cmpl %eax, %ebx
   je ReturnK
   jg ReturnLeft
   jl ReturnRight

ReturnK:
   movl 8(%ebp), %ebx
   movl 20(%ebp), %edi
   movl (%ebx, %edi, 4), %eax
   jmp Teardown
ReturnLeft:
   # 0. Save caller save registers
   pushl %edx
   pushl %ecx
   pushfl
   # 1. Push arguments
   movl 20(%ebp), %eax
   push %eax
   movl -20(%ebp), %eax
   subl $1, %eax
   push %eax
   movl 12(%ebp), %eax
   push %eax
   movl 8(%ebp), %eax
   push %eax
   # 2. Call the function
   CALL kthSmallest_asm
   # 3. Pop arguments
   addl $16, %esp
   # 4. restore caller saved registers
   popfl
   popl %ecx
   popl %edx
   jmp Teardown
ReturnRight:
   # 0. Save caller save registers
   pushl %edx
   pushl %ecx
   pushfl
   # 1. Push arguments
   movl 20(%ebp), %eax
   push %eax
   movl 16(%ebp), %eax
   push %eax
   movl -20(%ebp), %eax
   addl $1, %eax
   push %eax
   movl 8(%ebp), %eax
   push %eax
   # 2. Call the function
   CALL kthSmallest_asm
   # 3. Pop arguments
   addl $16, %esp
   # 4. restore caller saved registers
   popfl
   popl %ecx
   popl %edx
   jmp Teardown
   # -----------------------------------

Teardown:
   # 4. Tear down stack frame
   addl $8, %esp
   # 5. Restore callee-saved registers
   popl %edi
   popl %esi
   popl %ebx

   # movl $1, %eax ?

   leave
   ret
