/*
 * scheduler.h
 *
 *  Created on: 17.04.2020
 *      Author: Sebastian
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

//Includes
// #include "system.h"

//How many tasks do you want?
#define NUM_TASKS	3

//struct for task information
typedef struct
{
	unsigned char active[NUM_TASKS];	//Flag whether task should be executed at all
	unsigned char flag[NUM_TASKS];		//Flag when task wants to run
	unsigned int timer[NUM_TASKS];		//Timer for each task
	unsigned int schedule[NUM_TASKS];	//Timer reload value of each task, determines the rate the tasks are executed
	unsigned char loop_ovf;				//indicates when one task was started, when the loop time was already over
} schedule_t;

//defines for task bits
#define TASK0		0
#define TASK1		1
#define TASK2		2
#define TASK3		3
#define TASK4		4
#define TASK5		5
#define TASK6		6
#define TASK7		7

//State defines
#define INACTIVE	0
#define ACTIVE		1

#if NUM_TASKS > 8
#warning "Caution! You requested more tasks, than the scheduler is intended for!"
#endif

/*
 * Functional protoypes
 */
void			init_scheduler		(void);
void			schedule			(unsigned char task, unsigned int schedule);
void 			set_task			(unsigned char task, unsigned char state);
unsigned char 	get_task			(unsigned char task);
void 			count_task			(unsigned char task);
void 			run_scheduler		(void);
unsigned char 	run					(unsigned char task, unsigned char tickstate);
unsigned char 	schedule_overflow	(void);

#endif /* SCHEDULER_H_ */
