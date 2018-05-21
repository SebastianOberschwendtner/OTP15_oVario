/*
 * vario.h
 *
 *  Created on: 06.04.2018
 *      Author: Admin
 */

#ifndef VARIO_H_
#define VARIO_H_
//Git ist scheiße!
//*********** Includes **************
#include "stm32f4xx.h"
#include "ipc.h"
#include "did.h"

//*********** Defines **************



//*********** Functions **************
void vario_init (void);
void vario_task (void);


#endif /* VARIO_H_ */
