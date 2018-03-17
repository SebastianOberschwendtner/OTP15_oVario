// ********* Includes *********
#include "ipc.h"


// ********* Variables *********

uint8_t ipc_data_memory[memory_data_size];
uint8_t ipc_queue_memory[memory_queue_size];
uint8_t memory_data_idx = 0;
uint8_t memory_queue_idx = 0;

uint8_t queue_cnt = 0;
T_queue queues[did_size];


void* memory_data_did[did_size];
void* memory_queue_did[did_size];

extern uint8_t error;

//************************************
//*********** Functions **************

// register Data Memory and return pointer
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
		error = err_no_memory_left;
		return 0;
	}
}


// get pointer of registered Memory
void* ipc_memory_get(uint8_t did)
{
	return memory_data_did[did];
}


// register queue memory
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
		error = err_no_memory_left;
	}
}


// push data into queue
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
		queue_error = err_queue_overrun;
	}

	queues[did].queue_wr_idx = (queues[did].queue_wr_idx + no_bytes) % queues[did].size;
	return queue_error;
}


// fetch data from queue
void* ipc_queue_get(uint8_t did, uint8_t no_bytes)
{
	if(queues[did].cnt < no_bytes)
		return 0;
	else
	{
		uint8_t temp = queues[did].queue_rd_idx;
		queues[did].queue_rd_idx = (queues[did].queue_rd_idx + no_bytes) % queues[did].size;
		queues[did].cnt -= no_bytes;
		return (queues[did].p_memory + temp);
	}
}


// look into queue
void* ipc_queue_look(uint8_t did)
{
	return (queues[did].p_memory + queues[did].queue_rd_idx);
}


// return no bytes in queue
uint8_t ipc_get_queue_bytes(uint8_t did)
{
	return queues[did].cnt;
}
