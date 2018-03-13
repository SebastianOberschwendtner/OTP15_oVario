/*
 * ipc.h
 *
 *  Created on: 10.03.2018
 *      Author: Admin
 */

#ifndef IPC_H_
#define IPC_H_

//*********** Includes **************
#include "stm32f4xx.h"
#include "did.h"
#include "error.h"

//*********** Defines **************
#define memory_size 10000
#define did_size 	20


//*********** Functions **************

void* ipc_memory_register	(uint16_t no_bytes, uint8_t did);
void* ipc_memory_get		(uint8_t did);


#endif /* IPC_H_ */
