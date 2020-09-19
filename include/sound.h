/**
 ******************************************************************************
 * @file    sound.c
 * @author  JK
 * @version V1.1
 * @date    12-March-2018
 * @brief   Produces the sound of the vario.
 ******************************************************************************
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
