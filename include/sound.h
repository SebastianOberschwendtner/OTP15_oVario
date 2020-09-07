/*
 * sound.h
 *
 *  Created on: 12.03.2018
 *      Author: Admin
 */

#ifndef SOUND_H_
#define SOUND_H_

//*********** Includes **************
#include "oVario_Framework.h"

//*********** Defines **************
#define sound_mode_cont			1
#define sound_mode_beep			2
#define sound_mode_beep_short	3


//*********** Functions **************
void sound_register_ipc (void);
void sound_init         ();
void sound_task         ();

#endif /* SOUND_H_ */
