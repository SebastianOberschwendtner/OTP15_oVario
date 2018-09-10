/*
 * sound.h
 *
 *  Created on: 12.03.2018
 *      Author: Admin
 */

#ifndef SOUND_H_
#define SOUND_H_

//*********** Includes **************
#include "stm32f4xx.h"
#include "ipc.h"
#include "did.h"

//*********** Defines **************
#define sound_mode_cont	1
#define sound_mode_beep	2


//*********** Functions **************
void sound_init();
void sound_task();

#endif /* SOUND_H_ */
