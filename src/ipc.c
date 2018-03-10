// ********* Includes *********
#include "ipc.h"


// ********* Variables *********

uint8_t ipc_memory[memory_size];
uint8_t memory_idx = 0;
void* memory_did[did_size];

extern uint8_t error;

//************************************
//*********** Functions **************

// register Data Memory and return pointer
void* ipc_memory_register(uint16_t no_bytes, uint8_t did)
{
	uint16_t temp 	= memory_idx;
	memory_idx 		+= no_bytes;

	if(memory_idx < memory_size)					// Check for enough Memory
	{
		memory_did[did] = (void*) &ipc_memory[temp];
		return (void*) &ipc_memory[temp];
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
	return memory_did[did];
}
