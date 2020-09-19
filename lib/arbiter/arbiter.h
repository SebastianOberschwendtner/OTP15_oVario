/**
 ******************************************************************************
 * @file    arbiter.h
 * @author  SO
 * @version V1.0
 * @date    19-September-2020
 * @brief   The arbiter is used to handle the command arbitration within one
 *          task. You can use the arbiter for different tasks, but each task
 *          has to have its own task-struct.
 ******************************************************************************
 */

#ifndef ARBITER_H_
#define ARBITER_H_

//*********** Defines **************

//The command queue size
#define TASK_QUEUE      6
//Size of the argument stack in unsigned longs
#define ARG_STACK_SIZE  2*TASK_QUEUE //Two arguments per task
//Stack size in unsigned longs
#define STACK_SIZE      16

//Calculate waiting times -> Wait time can only be a multiple of the scheduled time
#define US2TASKTICK(time,task_schedule)  (unsigned long)((time)/(task_schedule))
#define MS2TASKTICK(time,task_schedule)  (unsigned long)((time*1000)/(task_schedule))

//Structure for arbiter data within one task
#pragma pack(push, 1)
typedef struct
{
    unsigned char   active_command;
    unsigned char   command                 [TASK_QUEUE];
    unsigned char   sequence                [TASK_QUEUE];
    void*           argument_pointer        [TASK_QUEUE];
    unsigned char   stack_pointer           [TASK_QUEUE];
    unsigned char   narg_in_stack           [TASK_QUEUE];
    unsigned long   return_value            [TASK_QUEUE];
    unsigned long   argument_stack          [ARG_STACK_SIZE];
    unsigned long   stack                   [STACK_SIZE];
    unsigned char   allocated_arg;
    unsigned char   allocated_stack;
    unsigned long   wait_counter;
    unsigned long   halt_task;
    unsigned long   local_counter;
} TASK_T;
#pragma pack(pop)

//Common States
#define CMD_IDLE            255

//Common sequences
#define SEQUENCE_ENTRY      0
#define SEQUENCE_FINISHED   255

//Function prototypes
void            arbiter_clear_task          (TASK_T* task);
unsigned char   arbiter_callbyreference     (TASK_T* task, unsigned char command, void* argument);
unsigned char   arbiter_callbyvalue         (TASK_T *task, unsigned char command);
void*           arbiter_allocate_arguments  (TASK_T* task, unsigned long n_args);
void            arbiter_return              (TASK_T* task, unsigned long value);
void*           arbiter_get_argument        (TASK_T* task);
void            arbiter_free_arguments      (TASK_T* task);
unsigned long   arbiter_get_reference       (TASK_T* task, unsigned long arg);
unsigned long   arbiter_get_return_value    (TASK_T* task);
void            arbiter_set_command         (TASK_T* task, unsigned char next_state);
void            arbiter_set_sequence        (TASK_T* task, unsigned char next_sequence);
void            arbiter_reset_sequence      (TASK_T* task);
unsigned char   arbiter_get_command         (TASK_T* task);
unsigned char   arbiter_get_sequence        (TASK_T* task);
unsigned long*  arbiter_malloc              (TASK_T* task, unsigned char memsize);
void            arbiter_memfree             (TASK_T* task);
unsigned char   arbiter_get_memsize         (TASK_T* task);
#endif