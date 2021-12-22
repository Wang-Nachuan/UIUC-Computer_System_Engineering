
.data					# section declaration

        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

        SCREEN_SIZE = 1999

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
        
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret


/* void mp1_rtc_tasklet (void);
 *
 * Interface: Following C calling convention
 *    Inputs: None
 *   Outputs: Updated linked list
 * Variables: None
 */
mp1_rtc_tasklet:

        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        # 2. Leave space for local variables
        # Pass  

        /* Walk down the linked list */
        movl    mp1_list_head, %ebx
        Tasklet_loop:
                # Return when reach a NULL pointer
                cmpl    $0, %ebx
                je      Tasklet_teardown
                # Decrement countdown status
                movw    COUNTDOWN(%ebx), %ax
                addw    $-1, %ax
                movw    %ax, COUNTDOWN(%ebx)
                # Check current countdown value
                cmpw    $0, %ax
                jne     Tasklet_next
                # Examine status character
                movw    STATUS(%ebx), %ax
                cmpw    $1, %ax
                je      Tasklet_status_one
                cmpw    $0, %ax
                je      Tasklet_status_zero 
                jmp     Tasklet_next            # In case that there is invalid status
        
        Tasklet_status_one:
                # Change statust to 0
                movw    $0, STATUS(%ebx)
                # Call mp1_poke, display off_char character
                movl    $0, %eax
                movw    LOCATION(%ebx), %ax
                movl    $2, %edx
                mull    %edx
                movb    OFF_CHAR(%ebx), %cl
                pushl   %edx
                call    mp1_poke
                popl    %edx
                # Update countdown with off_length
                movw    OFF_LENGTH(%ebx), %ax
                movw    %ax, COUNTDOWN(%ebx)
                jmp     Tasklet_next

        Tasklet_status_zero:
                # Change status to 1
                movw    $1, STATUS(%ebx)
                # Call mp1_poke, display on_char character
                movl    $0, %eax
                movw    LOCATION(%ebx), %ax
                movl    $2, %edx
                mull    %edx
                movb    ON_CHAR(%ebx), %cl
                pushl   %edx
                call    mp1_poke
                popl    %edx
                # Update countdown with on_length
                movw    ON_LENGTH(%ebx), %ax
                movw    %ax, COUNTDOWN(%ebx)
                jmp     Tasklet_next

        Tasklet_next:
                # Finally, move to next item
                movl    NEXT(%ebx), %ebx
                jmp     Tasklet_loop  
        
        /* Callee tear down */
        Tasklet_teardown:
                # 4. Tear down stack frame
                # Pass
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave
        
	ret


/* int mp1_ioctl(unsigned long arg, unsigned long cmd);
 *
 * Interface: Following C calling convention 
 * Inputs: arg  -  4(%esp), argument that passes to one of four core functions
 *         cmd  -  8(%esp),selecting number of core function
 *   Outputs: -1 if cmd is invalid, none otherwiser
 * Registers: Clobbers EAX
 */
mp1_ioctl:
        # Check cmd
        movl    8(%esp), %eax
        cmpl    $0, %eax        # If cmd < 0, invalid
        jb      invalid_cmd
        cmpl    $3, %eax        # If cmd > 3, invalid
        ja      invalid_cmd

        # Jump to corresponding function in the jump table
        jmp     *jump_table(, %eax, 4)

        # If cmd is invalid, return -1
        invalid_cmd:
                movl    $-1, %eax
	        ret


/* int mp1_ioctl_add(unsigned long arg);
 *
 * Interface: Following C calling convention
 *    Inputs: arg  -  8(%ebp), user level pointer to a to a mp1_blink_struct structure
 *   Outputs: 0 if success
 * Variables: -16(%ebp), starting address of the allocated memory block
 *            -20(%ebp), return value of mp1_copy_from_user
 */
mp1_ioctl_add:

        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        # 2. Initialize local variables
        movl    $0, %eax
        pushl   %eax
        pushl   %eax

        /* !!! Error check O (invalid input pointer) !!! */
        movl    8(%ebp), %eax
        cmpl    $0, %eax
        je      Add_return_neg

        /* Allocate memory */
        # 0. Caller-saved registers
        pushl   %eax
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    $STRUCT_SIZE, %eax
        pushl   %eax
        # 2. Make the call
        call    mp1_malloc
        # 3. Pop argument, save return value
        addl    $4, %esp
        movl    %eax, -16(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx
        popl    %eax

        /* !!! Error check I (memory allocation failed) !!! */
        movl   -16(%ebp), %eax
        cmpl    $0, %eax
        je      Add_fail

        /* Copy structure to kenerl space */
        # 0. Caller-saved registers
        pushl   %eax
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    $STRUCT_SIZE, %eax
        pushl   %eax            # Push n
        movl    8(%ebp), %eax
        pushl   %eax            # Push *from
        movl    -16(%ebp), %eaxhalt
        pushl   %eax            # Push *to
        # 2. Make the call
        call    mp1_copy_from_user
        # 3. Pop argument, save return value
        addl    $12, %esp
        movl    %eax, -20(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx
        popl    %eax

        /* !!! Error check II (copy failed) !!! */
        movl   -20(%ebp), %eax
        cmpl    $0, %eax
        jne     Add_fail

        /* !!! Error check III (invalid location) !!! */
        movl    -16(%ebp), %ebx
        movw    LOCATION(%ebx), %ax
        cmpw    $0, %ax
        jb      Add_fail
        cmpw    $1999, %ax      # 1999 = 80*25 - 1
        ja      Add_fail

        /* Set countdown and status */
        # Move on_length to AX
        movl    -16(%ebp), %ebx
        movw    ON_LENGTH(%ebx), %ax
        # Set countdown to on_length 
        movw    %ax, COUNTDOWN(%ebx)
        # Set status to 1
        movw    $1, STATUS(%ebx)

        /* Insert the structure at head of linked list */
        # Set next
        movl    mp1_list_head, %eax
        movl    %eax, NEXT(%ebx)
        # Set head to new structure
        movl    %ebx, mp1_list_head

        /* Call mp1_poke to update the screen */
        movl    $0, %eax
        movw    LOCATION(%ebx), %ax
        movl    $2, %edx
        mull    %edx
        movb    ON_CHAR(%ebx), %cl
        pushl   %edx
        call    mp1_poke
        popl    %edx

        /* Return 0 if success */
        movl    $0, %eax
        jmp     Add_teardown
        
        /* Return -1 if fail */
        Add_fail:
                # If a memory block is allocated, free it
                movl    -16(%ebp), %eax
                cmpl    $0, %eax
                je      Add_return_neg
                # 0. Caller-saved registers
                pushl   %eax
                pushl   %edx
                pushl   %ecx
                pushfl
                # 1. Push argument
                movl    -16(%ebp), %eax
                pushl   %eax
                # 2. Make the call
                call    mp1_free
                # 3. Pop argument
                addl    $4, %esp
                # 4. Restore caller-saved registers
                popfl
                popl    %ecx
                popl    %edx
                popl    %eax
        Add_return_neg:
                movl    $-1, %eax
                jmp     Add_teardown

        /* Callee tear down */
        Add_teardown:
                # 4. Tear down stack frame
                addl    $8, %esp
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave

	ret


/* int mp1_ioctl_remove(unsigned long arg);
 *
 * Interface: Following C calling convention
 *    Inputs: arg - 8(%ebp), location of target structure
 *   Outputs: 0 if success, -1 if failed
 * Variables: -16(%ebp), location value, also pionter of the structure
 */
mp1_ioctl_remove:
        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        # 2. Save input
        movl    $0, %eax
        movw    8(%ebp), %ax
        pushl   %eax

        /* !!! Error check III (invalid location) !!! */
        cmpw    $0, %ax
        jb      Remove_fail
        cmpw    $SCREEN_SIZE, %ax
        ja      Remove_fail

        /* Search for the structure */
        # 0. Caller-saved registers
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        pushl   %eax
        # 2. Make the call
        call    mp1_help_search
        # 3. Pop argument, save return value
        addl    $4, %esp
        movl    %eax, -16(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx

        /* !!! Error check IV (location not found) !!! */
        cmpl    $0, %eax
        je      Remove_fail

        /* Find the previous structure */
        # Skip this part when it is the head pointer
        movl    mp1_list_head, %ebx
        cmpl    %eax, %ebx
        je      Remove_head
        # Otherwise, loop to find the previous structure
        Remove_Loop:
                movl    NEXT(%ebx), %edx        # EBX always store base address of current structure
                cmpl    %eax, %edx
                je      Remove_normal           # When %ebx->NEXT = %eax, find it
                cmpl    $0, %edx                
                je      Remove_fail             # When %ebx->NEXT = NULL, reach the end
                movl    NEXT(%ebx), %ebx
                jmp     Remove_Loop

        Remove_head:
                movl    NEXT(%eax), %edx
                movl    %edx, mp1_list_head     # Head <= Head->next
                jmp     Remove_free

        Remove_normal:
                movl    NEXT(%eax), %edx
                movl    %edx, NEXT(%ebx)        # %ebx->NEXT <= %eax->NEXT
                jmp     Remove_free
                

        /* Free the memory block */
        Remove_free:
                # 0. Caller-saved registers
                pushl   %edx
                pushl   %ecx
                pushfl
                # 1. Push argument
                pushl   %eax
                # 2. Make the call
                call    mp1_free
                # 3. Pop argument
                addl    $4, %esp
                # 4. Restore caller-saved registers
                popfl
                popl    %ecx
                popl    %edx
        
        /* Return 0 if success */
        movl    $0, %eax
        jmp     Remove_teardown
        
        /* Return -1 if fail */
        Remove_fail:
                movl    $-1, %eax
                jmp     Remove_teardown

        /* Callee tear down */
        Remove_teardown:
                # 4. Tear down stack frame
                addl    $4, %esp
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave
        
	ret


/* int mp1_ioctl_find(unsigned long arg);
 *
 * Interface: Following C calling convention
 *    Inputs: arg  -  8(%ebp), user level pointer to a to a mp1_blink_struct structure
 *   Outputs: 0 if success, -1 if failed
 * Variables: -16(%ebp), starting address of the allocated memory block
 *            -20(%ebp), return value of mp1_copy_from_user
 *            -24(%ebp), pointer to the node found in linked list
 */
mp1_ioctl_find:

        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        # 2. Initialize local variables
        movl    $0, %eax
        pushl   %eax
        pushl   %eax
        pushl   %eax

        /* !!! Error check O (invalid input pointer) !!! */
        movl    8(%ebp), %eax
        cmpl    $0, %eax
        je      Find_fail

        /* Allocate memory */
        # 0. Caller-saved registers
        pushl   %eax
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    $STRUCT_SIZE, %eax
        pushl   %eax
        # 2. Make the call
        call    mp1_malloc
        # 3. Pop argument, save return value
        addl    $4, %esp
        movl    %eax, -16(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx
        popl    %eax

        /* !!! Error check I (memory allocation failed) !!! */
        movl   -16(%ebp), %eax
        cmpl    $0, %eax
        je      Find_fail

        /* Copy structure to kenerl space */
        # 0. Caller-saved registers
        pushl   %eax
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    $STRUCT_SIZE, %eax
        pushl   %eax            # Push n
        movl    8(%ebp), %eax
        pushl   %eax            # Push *from
        movl    -16(%ebp), %eax
        pushl   %eax            # Push *to
        # 2. Make the call
        call    mp1_copy_from_user
        # 3. Pop argument, save return value
        addl    $12, %esp
        movl    %eax, -20(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx
        popl    %eax

        /* !!! Error check II (copy failed) !!! */
        movl   -20(%ebp), %eax
        cmpl    $0, %eax
        jne     Find_fail

        /* !!! Error check III (invalid location) !!! */
        movl    -16(%ebp), %ebx
        movl    $0, %eax        # Clean EAX
        movw    LOCATION(%ebx), %ax
        cmpw    $0, %ax
        jb      Find_fail
        cmpw    $SCREEN_SIZE, %ax
        ja      Find_fail

        /* Search for the structure in linked list */
        # 0. Caller-saved registers
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        pushl   %eax
        # 2. Make the call
        call    mp1_help_search
        # 3. Pop argument
        addl    $4, %esp
        movl    %eax, -24(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx
        
        /* !!! Error check IV (location not found) !!! */
        cmpl    $0, %eax
        je      Find_fail

        /* Copy the structure back to input pointers */
        # 0. Caller-saved registers
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    $STRUCT_SIZE, %eax
        pushl   %eax            # Push n
        movl    -24(%ebp), %eax
        pushl   %eax            # Push *from
        movl    8(%ebp), %eax
        pushl   %eax            # Push *to
        # 2. Make the call
        call    mp1_copy_to_user
        # 3. Pop argument, save return value
        addl    $12, %esp
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx

        /* !!! Error check II (copy failed) !!! */
        cmpl    $0, %eax
        jne     Find_fail

        /* Return 0 if success */
        movl    $0, %eax
        jmp     Find_free

        /* Return -1 if fail */
        Find_fail:
                movl    $-1, %eax
                jmp     Find_free

        /* Free the allocated memory block */
        Find_free:
                # Check if it is successfully allocated
                movl    -16(%ebp), %ebx
                cmpl    $0, %ebx
                je      Find_teardown
                # 0. Caller-saved registers
                pushl   %eax
                pushl   %edx
                pushl   %ecx
                pushfl
                # 1. Push argument
                movl    -16(%ebp), %eax
                pushl   %eax
                # 2. Make the call
                call    mp1_free
                # 3. Pop argument
                addl    $4, %esp
                # 4. Restore caller-saved registers
                popfl
                popl    %ecx
                popl    %edx
                popl    %eax

        /* Callee tear down */
        Find_teardown:
                # 4. Tear down stack frame
                addl    $12, %esp
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave

	ret


/* int mp1 ioctl sync(unsigned long arg);
 *
 * Interface: Following C calling convention
 *    Inputs: arg  -  8(%ebp), package of two two-byte location numbers
 *   Outputs: 0 if success, -1 if failed
 * Variables: -16(%ebp), first location (arg[32:16]), also pointer to first character
 *            -20(%ebp), second location (arg[15:0]), to be synchronized, also pointer to second character
 */
mp1_ioctl_sync:
        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        # 2. Leave space for local variables
        movl    $0, %eax
        pushl   %eax
        pushl   %eax

        /* !!! Error check III (invalid location) !!! */
        movw    10(%ebp), %ax           # Upper 16 bits to AX
        movw    8(%ebp), %bx            # Lower 16 bits to BX
        cmpw    $0, %ax
        jb      Sync_fail
        cmpw    $SCREEN_SIZE, %ax
        ja      Sync_fail
        cmpw    $0, %bx
        jb      Sync_fail
        cmpw    $SCREEN_SIZE, %bx
        ja      Sync_fail
        # Store valid variables
        movw    %ax, -16(%ebp)
        movw    %bx, -20(%ebp)

        /* Search for First structure */
        # 0. Caller-saved registers
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    -16(%ebp), %eax
        pushl   %eax
        # 2. Make the call
        call    mp1_help_search
        # 3. Pop argument
        addl    $4, %esp
        movl    %eax, -16(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx

        /* !!! Error check IV (location not found) !!! */
        cmpl    $0, %eax
        je      Sync_fail

        /* Search for Second structure */
        # 0. Caller-saved registers
        pushl   %edx
        pushl   %ecx
        pushfl
        # 1. Push argument
        movl    -20(%ebp), %eax
        pushl   %eax
        # 2. Make the call
        call    mp1_help_search
        # 3. Pop argument
        addl    $4, %esp
        movl    %eax, -20(%ebp)
        # 4. Restore caller-saved registers
        popfl
        popl    %ecx
        popl    %edx

        /* !!! Error check IV (location not found) !!! */
        cmpl    $0, %eax
        je      Sync_fail

        /* Copy timing information from first structure to second structure */
        movl    -16(%ebp), %eax         # EAX point to first structure
        movl    -20(%ebp), %ebx         # EBX point to second structure
        movw    ON_LENGTH(%eax), %dx
        movw    %dx, ON_LENGTH(%ebx)
        movw    OFF_LENGTH(%eax), %dx
        movw    %dx, OFF_LENGTH(%ebx)
        movw    COUNTDOWN(%eax), %dx
        movw    %dx, COUNTDOWN(%ebx)
        movw    STATUS(%eax), %dx
        movw    %dx, STATUS(%ebx)

        /* Call mp1_poke to update the screen */
        movl    $0, %eax
        movw    LOCATION(%ebx), %ax
        movl    $2, %edx
        mull    %edx
        # Check the state
        movw    STATUS(%ebx), %dx
        cmpw    $0, %dx
        je      Sync_state_off
        movb    ON_CHAR(%ebx), %cl
        jmp     Sync_call_poke
        Sync_state_off: 
        movb    OFF_CHAR(%ebx), %cl
        Sync_call_poke:
        pushl   %edx
        call    mp1_poke
        popl    %edx

        /* If success, return 0 */
        movl    $0, %eax
        jmp     Sync_teardown

        /* If fail, return -1 */
        Sync_fail:
                movl    $-1, %eax

        /* Callee tear down */
        Sync_teardown:
                # 4. Tear down stack frame
                addl    $8, %esp
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave
	ret


/* ---------- Assistant functions and parameters ---------- */

/* Search the node at  given location
 *
 * Interface: Following C calling convention
 *    Inputs: 8(%ebp), location of character
 *   Outputs: pointer to the node if success, 0 if not find
 */
mp1_help_search:
        /* Callee setup */
        # 0. Update pointer
        pushl   %ebp
        movl    %esp, %ebp
        # 1. Callee-saved registers
        pushl   %ebx
        pushl   %esi
        pushl   %edi

        /* Loop to find the node */
        movl    mp1_list_head, %ebx
        movl    $0, %eax                # Set default output to 0
        movl    8(%ebp), %ecx           # Store target location to ECX
        Search_loop:
                # Return when reach a NULL pointer
                cmpl    $0, %ebx
                je      Search_teardown
                # Check the location value
                movw    LOCATION(%ebx), %dx
                cmpw    %dx, %cx
                je      Search_success
                # Otherwise, move to next node
                movl    NEXT(%ebx), %ebx
                jmp     Search_loop   

        Search_success:
                movl    %ebx, %eax

        /* Callee tear down */
        Search_teardown:
                # 5. Restore callee-saved registers
                popl    %edi
                popl    %esi
                popl    %ebx
                # 6. Load old base pointer
                leave

        ret


# Jump table
jump_table:
        .long   mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync


.end
