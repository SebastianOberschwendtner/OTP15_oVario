/**
 ******************************************************************************
 * @file    ipc.c
 * @author  JK
 * @version V1.0
 * @date    10-March-2020
 * @brief   Handles the inter-process-communication -> IPC.
 ******************************************************************************
 */

// ********* Includes *********
#include "ipc.h"


// ********* Variables *********

uint8_t ipc_data_memory[memory_data_size];
uint8_t ipc_queue_memory[memory_queue_size];

uint32_t memory_data_idx = 0;

uint32_t memory_queue_idx = 0;

uint8_t queue_cnt = 0;
T_queue queues[did_size];


void* memory_data_did[did_size];
void* memory_queue_did[did_size];

//extern uint8_t error;

//*********** Functions **************
/**
 * @brief register Data Memory and return pointer
 * @param no_bytes The number of bytes of the memory to be registered.
 * @param did The did of the memory.
 * @return The pointer address of the memory.
 */
void* ipc_memory_register(uint32_t no_bytes, uint8_t did)
{
	uint16_t temp 	= memory_data_idx;
	memory_data_idx += no_bytes;

	if(memory_data_idx < memory_data_size)					// Check for enough Memory
	{
		memory_data_did[did] = (void*) &ipc_data_memory[temp];
		return (void*) &ipc_data_memory[temp];
	}
	else
	{
	//	error = err_no_memory_left;
		return 0;
	}
};

/**
 * @brief Get pointer of registered Memory.
 * @param did The did of the memory
 * @return The pointer address of the memory.
 */
void* ipc_memory_get(uint8_t did)
{
	return memory_data_did[did];
};

/**
 * @brief Register queue memory
 * @param size_queue The size of the queue to be registered
 * @param did The did of the queue.
 */
void ipc_register_queue(uint16_t size_queue, uint8_t did)
{
	queues[did].did 	= did;
	queues[did].size 	= size_queue;
	queues[did].cnt		= 0;

	uint16_t temp 		= memory_queue_idx;
	memory_queue_idx 	+= size_queue;

	if(memory_queue_idx < memory_queue_size)					// Check for enough Memory
	{
		queues[did].p_memory = (void*) &ipc_queue_memory[temp];
	}
	else
	{
		;//error = err_no_memory_left;
	}
};

/**
 * @brief Push data into queue
 * @param p_data The pointer to the data
 * @param no_bytes The number of bytes to be pushed into the queue
 * @param did The did of the queue to be pushed into
 * @return Returns 0 when no error occurred or an error code.
 */
uint8_t ipc_queue_push(void* p_data, uint8_t no_bytes, uint8_t did)
{
	uint8_t temp;
	uint32_t queue_error = 0;

	for(uint8_t i = 0; i < no_bytes; i++)
	{
		temp = (queues[did].queue_wr_idx + i) % queues[did].size;
		*((uint8_t*)queues[did].p_memory + temp) = *((uint8_t*)p_data + i);
	}


	queues[did].cnt = (queues[did].cnt + no_bytes);
	if(queues[did].cnt >= queues[did].size)
	{
		queues[did].cnt = queues[did].size;
		queue_error = (1<<1);
	}

	queues[did].queue_wr_idx = (queues[did].queue_wr_idx + no_bytes) % queues[did].size;
	return queue_error;
};

/**
 * @brief Fetch data from queue. Deletes the element from the queue!
 * @param did The did of the queue
 * @param no_bytes The number of bytes to be fetched
 * @param pData The pointer where the data is pushed to.
 * @return Returns 0 when no error occurred or an error code.
 */
uint8_t ipc_queue_get(uint8_t did, uint8_t no_bytes, void* pData)
{
	if(queues[did].cnt >= no_bytes)
	{
		uint8_t temp 				= queues[did].queue_rd_idx;
		queues[did].queue_rd_idx 	= (queues[did].queue_rd_idx + no_bytes) % queues[did].size;
		queues[did].cnt 		   -= no_bytes;

		for(uint8_t cnt = 0; cnt < no_bytes; cnt++)
		{
			uint8_t* ptemp = (pData + cnt);
			*ptemp = *((uint8_t*)((queues[did].p_memory + ((temp + cnt) % queues[did].size))));
		}
		return 1;
	}
	else
	{
		return 0;
	}
};

/**
 * @brief Look into queue and do not delete the data in the queue.
 * @param did The did of the queue
 * @return The pointer to the current queue element.
 */
void* ipc_queue_look(uint8_t did)
{
	return (queues[did].p_memory + queues[did].queue_rd_idx);
};

/**
 * @brief Get the number of bytes in the queue.
 * @param did The did of the queue.
 * @return The number of bytes which are currently in the queue.
 */
uint16_t ipc_get_queue_bytes(uint8_t did)
{
	return queues[did].cnt;
};
