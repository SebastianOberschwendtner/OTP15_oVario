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

//*********** Defines **************
#define sound_cmd_set_frequ 	1
#define sound_cmd_set_vol		2
#define sound_cmd_set_louder 	3
#define sound_cmd_set_quieter 	4
#define sound_cmd_set_mute		5
#define sound_cmd_set_unmute	6


//*********** Functions **************
void sound_init();
void sound_task();

#endif /* SOUND_H_ */
