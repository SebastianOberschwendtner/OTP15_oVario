/*
 * arbiter.c
 *
 *  Created on: 16.08.2020
 *      Author: Sebastian
 */
#include "arbiter.h"

/********** Example Command Structure ************************************

void task_command(void)
{
    //Get arguments
    unsigned long* arg1 = arbiter_get_argument(&task_struct);
    unsigned long* arg2 = arg1 + 1;

    //Allocate memory
    unsigned long* mem1 = arbiter_malloc(&task_struct, n_mem);
    unsigned long* mem2 = mem1 + 1;

    //Perform the command action
    switch (arbiter_get_sequence(&task_struc))
    {
        case SEQUENCE_ENTRY:
            break;

        case SEQUENCE_FINISHED:
            if (command_finished)
            {
                //Exit command
                arbiter_return(&task_struct, return_value);
            }
            break;

        defaul:
            break;
    }
};

**************************************************************************
*/

/*
 * Clear the data of one task struct.
 */

void arbiter_clear_task(TASK_T* task)
{
    task->active_command = 0;
    task->wait_counter   = 0;
    task->halt_task      = 0;
    task->local_counter  = 0;
    task->allocated_arg  = 0;

    for(unsigned char count = 0; count < TASK_QUEUE; count++)
    {
        task->command[count]                = 0;
        task->sequence[count]               = 0;
        task->argument_pointer[count]     = 0;
        task->narg_in_stack[count]                 = 0;
        task->return_value[count]           = 0;
    }

    for(unsigned char count = 0; count < ARG_STACK_SIZE; count++)
        task->argument_stack[count]         = 0;
};

/*
 * Call a function using the arbiter. The function call is call-by-reference -> Input argument is a pointer.
 * Returns 1 when the called function is finished.
 */
unsigned char arbiter_callbyreference(TASK_T* task, unsigned char command, void* argument)
{
    //Check whether command queue is full
    if(task->active_command < (TASK_QUEUE - 1))
    {
        //Command queue not full
        //Check whether current command has already called another command and whether this command is finished
        if(task->sequence[task->active_command + 1] == SEQUENCE_FINISHED)
        {
            task->sequence[task->active_command + 1] = 0;
            return 1;
        }
        else if (task->sequence[task->active_command + 1] == 0)
        {
            // When no next command is active, set the next command
            task->active_command++;
            task->command           [task->active_command] = command;
            task->sequence          [task->active_command] = SEQUENCE_ENTRY;
            task->argument_pointer  [task->active_command] = argument;
            task->narg_in_stack     [task->active_command] = 0;
        }
    }
    return 0;
};

/*
 * Call a function using the arbiter. This function call is call-by-value -> Input argument is a value.
 * The value has to be assigned using the arbiter_allocate_argument function!
 * Returns 1 when the called function is finished.
 */
unsigned char arbiter_callbyvalue(TASK_T *task, unsigned char command)
{
    //Check whether command queue is full
    if (task->active_command < (TASK_QUEUE - 1))
    {
        //Command queue not full
        //Check whether current command has already called another command and whether this command is finished
        if (task->sequence[task->active_command + 1] == SEQUENCE_FINISHED)
        {
            //Free memory for arguments
			arbiter_free_arguments(task);
			
            task->sequence[task->active_command + 1] = 0;
            return 1;
        }
        else if (task->sequence[task->active_command + 1] == 0)
        {
            // When no next command is active, set the next command
            task->active_command++;
            task->command[task->active_command] = command;
            task->sequence[task->active_command] = SEQUENCE_ENTRY; 
        }
    }
    return 0;
};

/*
 * Allocate the memory for the input arguments and get the pointer to the memory.
 */
void* arbiter_allocate_arguments(TASK_T* task, unsigned long n_args)
{
    //Only when no arguments are allocated for next active command
    if (task->narg_in_stack[task->active_command + 1] == 0)
    {
        //Set current number of arguments in stack
        task->narg_in_stack[task->active_command + 1] = n_args;
        //Set the reference to the input arguments
        task->argument_pointer[task->active_command + 1] = &task->argument_stack[task->allocated_arg];
        //Set the total number of arguments in stack
        task->allocated_arg += n_args;
    }
    //Return the pointer to the beginning of the argument stack segment
    return (void*) task->argument_pointer[task->active_command + 1];
};

/*
 * Free the allocated arguments when call is finished.
 */
void arbiter_free_arguments(TASK_T* task)
{
    //Decrease the argument stack pointer, and reset nargs of the command
    task->allocated_arg -= task->narg_in_stack[task->active_command + 1];
    task->narg_in_stack[task->active_command + 1] = 0;
};

/*
 * Signal to the arbiter that the current command has finished. 
 * This return function can be called any time, it automatically sets the calling command as FINISHED.
 * Frees any memory in the stack.
 */
void arbiter_return(TASK_T* task, unsigned long value)
{
    //Save the return value
    task->return_value[task->active_command] = value;

    //Free memory in stack
    arbiter_memfree(task);

    //Check whether the current command is the only command in the queue
    if(task->active_command == 0)
    {
        //There are other tasks in the queue
        //Goto to idle state

        task->command[task->active_command] = CMD_IDLE;
        task->sequence[task->active_command] = SEQUENCE_ENTRY;
    }
    else
    {
        //There are other tasks in the queue
        //Set current state to zero and mark it as finished
        task->command[task->active_command]  = 0;
        task->sequence[task->active_command] = SEQUENCE_FINISHED;

        //Set the new active command
        task->active_command--;
    }  
};

/*
 * Get the argument pointer for the current active command.
 * Returns ALWAYS a pointer regardless whether function was called by reference or value.
 * This enables the call-by-reference function to accept any kind of pointer. It is also more memory efficient.
 */
void* arbiter_get_argument(TASK_T* task)
{
    return task->argument_pointer[task->active_command];
};

/*
 * Get the adress of a reference, when call-by-value was used to forward the pointer address.
 * You have to specify which argument was the address.
 * 
 * Returns the pointer address as an unsigned long.
 */
unsigned long arbiter_get_reference(TASK_T* task, unsigned long arg)
{
    unsigned long* temp_pointer = task->argument_pointer[task->active_command];
    return *(temp_pointer + arg);
};

/*
 * Get the return value of the called command, when it is finished.
 */
unsigned long arbiter_get_return_value(TASK_T* task)
{
    return task->return_value[task->active_command+1];
};

/*
 * Set the active state of the arbiter.
 */
void arbiter_set_command(TASK_T* task, unsigned char next_state)
{
    //Reset sequence
    task->sequence[task->active_command] = SEQUENCE_ENTRY;
    //Set next command
    task->command[task->active_command]  = next_state;
};

/*
 * Set the sequence of the  active state of the arbiter.
 */
void arbiter_set_sequence(TASK_T* task, unsigned char next_sequence)
{
    task->sequence[task->active_command] = next_sequence;
};

/*
 * Reset all sequence states
 */
void arbiter_reset_sequence(TASK_T* task)
{
    //Set every sequence number to 0
    for (unsigned char i = 0; i < TASK_QUEUE; i++)
        task->sequence[i] = 0;
}

/*
 * Get the active state of the arbiter.
 */
unsigned char arbiter_get_command(TASK_T* task)
{
    return task->command[task->active_command];
};

/*
 * Get the sequence of the active state of the arbiter.
 */
unsigned char arbiter_get_sequence(TASK_T* task)
{
    return task->sequence[task->active_command];
};

/*
 * Allocate memory for the current command. Should not be called with memsize = 0.
 * For now the function can only assign static memory.
 * -> you can call the function multiple times, but the allocated size of the memory
 * has to stay the same, otherwise stack memory could be overwritten (for now at least).
 * 
 * Returns the pointer to the allocated memory.
 */
//TODO Add dynamic memory allocation
unsigned long* arbiter_malloc(TASK_T* task, unsigned char memsize)
{
    //Only allocate memory when no memory is allocated
    if(!((task->stack_pointer[task->active_command] + memsize) == task->allocated_stack))
    {
        //Set the current stack pointer to new address
        task->stack_pointer[task->active_command] = task->allocated_stack;
        
        //Save the allocated amount of memory
        task->allocated_stack += memsize;

        //Shift all following stack pointer to the new "empty" address
        for (unsigned char i = task->active_command+1; i < TASK_QUEUE; i++)
            task->stack_pointer[i] = task->allocated_stack; 
    }

    //Return the pointer to the memory
    return &task->stack[task->stack_pointer[task->active_command]];
};

/*
 * Free the memory currently allocated for the active command.
 * Does nothing when no memory is allocated.
 */
void arbiter_memfree(TASK_T* task)
{
    //When the active command is not the first in the stack
    if (task->active_command)
    {
        //When the stack pointer of the next command is greater than the current command
        //stack memory is allocated, otherwise not
        if (task->stack_pointer[task->active_command + 1] > task->stack_pointer[task->active_command])
        {
            //Save the new amount of allocated memory
            task->allocated_stack = task->stack_pointer[task->active_command];

            //memory is allocated, free it by shifting all following pointers
            for (unsigned char i = task->active_command; i < TASK_QUEUE; i++)
                task->stack_pointer[i] = task->stack_pointer[task->active_command];
        }
    }
    else
    {
        //when only one command is active, set the allocated memory to zero
        task->allocated_stack = 0;
        
        //Set all stack pointers to 0
        for (unsigned char i = 0; i < TASK_QUEUE; i++)
                task->stack_pointer[i] = 0;
    }
};

/*
 * Get the size in unsigned longs of the allocated memory of the currently active command.
 * Returns 0 when no memory is allocated for the command.
 */
unsigned char arbiter_get_memsize(TASK_T* task)
{
    //When the active command is not the first in the stack
    if (task->active_command)
    {
        //When the stack pointer of the current command is greater than the previous command
        //stack memory is allocated, otherwise not
        if (task->stack_pointer[task->active_command] > task->stack_pointer[task->active_command - 1])
            return task->allocated_stack - task->stack_pointer[task->active_command];
    }
    else
        return task->allocated_stack;
    return 0;
};