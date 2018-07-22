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
#define memory_data_size 10000
#define memory_queue_size 1000
#define did_size 	20

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
void* ipc_memory_register	(uint32_t no_bytes, uint8_t did);
void* ipc_memory_get		(uint8_t did);


void  	ipc_register_queue	(uint16_t size_queue, uint8_t did);
uint8_t ipc_queue_push		(void* p_data, uint8_t no_bytes, uint8_t did);
uint8_t ipc_queue_get(uint8_t did, uint8_t no_bytes, void* pData);
void* 	ipc_queue_look		(uint8_t did);
uint16_t ipc_get_queue_bytes	(uint8_t did);

extern uint32_t error_var;


#endif /* IPC_H_ */
