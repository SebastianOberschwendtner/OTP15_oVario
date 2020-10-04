/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 ******************************************************************************
 * @file    ipc.c
 * @author  JK
 * @version V1.0
 * @date    10-March-2020
 * @brief   Handles the inter-process-communication -> IPC.
 ******************************************************************************
 */

#ifndef IPC_H_
#define IPC_H_

//*********** Includes **************
#include "stdint.h"

//*********** Defines **************
#define memory_data_size 	10000
#define memory_queue_size 	5000
#define did_size 			20

// ***** Prototypes *****
typedef struct
{
	uint8_t 	did;
	uint16_t 	size;
	uint8_t 	queue_wr_idx;
	uint8_t 	queue_rd_idx;
	uint8_t		cnt;
	void* 		p_memory;
}T_queue;

//*********** Functions **************
void* 		ipc_memory_register	(uint32_t no_bytes, uint8_t did);
void* 		ipc_memory_get		(uint8_t did);


void  		ipc_register_queue	(uint16_t size_queue, uint8_t did);
uint8_t 	ipc_queue_push		(void* p_data, uint8_t no_bytes, uint8_t did);
uint8_t 	ipc_queue_get		(uint8_t did, uint8_t no_bytes, void* pData);
void* 		ipc_queue_look		(uint8_t did);
uint16_t 	ipc_get_queue_bytes	(uint8_t did);

extern uint32_t error_var;


#endif /* IPC_H_ */
