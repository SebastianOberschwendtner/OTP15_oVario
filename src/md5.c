/*
 * md5.c
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */
#include "md5.h"

//***** Varaibles *****
TASK_T task_md5;		//Task struct for arbiter
T_command rxcmd_md5;	//Container for received ipc commands
T_command txcmd_md5;	//Container for commands to transmit via ipc
unsigned long* args;	//Pointer to handle arguments for arbiter

//TODO Can the active string be transmitted in another way? Maybe a command with 2 data arguments.
char* active_string;	//Points to the active string which should be appended to the hash

//*********** Constants for hash calculation **************
unsigned long const k[64] = {
		// k[i] := floor(abs(sin(i)) * (2 pow 32))
		// RLD should be sin(i + 1) but want compatibility
		3614090360UL, // k=0
		3905402710UL, // k=1
		606105819UL, // k=2
		3250441966UL, // k=3
		4118548399UL, // k=4
		1200080426UL, // k=5
		2821735955UL, // k=6
		4249261313UL, // k=7
		1770035416UL, // k=8
		2336552879UL, // k=9
		4294925233UL, // k=10
		2304563134UL, // k=11
		1804603682UL, // k=12
		4254626195UL, // k=13
		2792965006UL, // k=14
		1236535329UL, // k=15
		4129170786UL, // k=16
		3225465664UL, // k=17
		643717713UL, // k=18
		3921069994UL, // k=19
		3593408605UL, // k=20
		38016083UL, // k=21
		3634488961UL, // k=22
		3889429448UL, // k=23
		568446438UL, // k=24
		3275163606UL, // k=25
		4107603335UL, // k=26
		1163531501UL, // k=27
		2850285829UL, // k=28
		4243563512UL, // k=29
		1735328473UL, // k=30
		2368359562UL, // k=31
		4294588738UL, // k=32
		2272392833UL, // k=33
		1839030562UL, // k=34
		4259657740UL, // k=35
		2763975236UL, // k=36
		1272893353UL, // k=37
		4139469664UL, // k=38
		3200236656UL, // k=39
		681279174UL, // k=40
		3936430074UL, // k=41
		3572445317UL, // k=42
		76029189UL, // k=43
		3654602809UL, // k=44
		3873151461UL, // k=45
		530742520UL, // k=46
		3299628645UL, // k=47
		4096336452UL, // k=48
		1126891415UL, // k=49
		2878612391UL, // k=50
		4237533241UL, // k=51
		1700485571UL, // k=52
		2399980690UL, // k=53
		4293915773UL, // k=54
		2240044497UL, // k=55
		1873313359UL, // k=56
		4264355552UL, // k=57
		2734768916UL, // k=58
		1309151649UL, // k=59
		4149444226UL, // k=60
		3174756917UL, // k=61
		718787259UL, // k=62
		3951481745UL  // k=63
};

unsigned long const r[64] = {
		7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
		5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
		4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
		6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

/*
 * Task to calculate the hash
 */

void md5_task(void)
{
	 //When the task wants to wait
	 if (task_md5.wait_counter)
		 task_md5.wait_counter--; //Decrease the wait counter
	 else						  //Execute task when wait is over
	 {
		 //Perform command action when the task does not wait for other tasks to finish
		 if (task_md5.halt_task == 0)
		 {
			 //Perform the task action
			 switch (arbiter_get_command(&task_md5))
			 {
			 case CMD_IDLE:
				 md5_check_commands();
				 break;

			 case MD5_CMD_APPEND_CHAR:
				 md5_append_char();
				 break;

			 case MD5_CMD_APPEND_STRING:
				 md5_append_string();
				 break;

			 default:
				 break;
			 }
		 }
	 }
};

/**********************************************************
 * Idle command of the task.
 **********************************************************
 * Checks the ipc command queue for new commands and 
 * handles them.
 * 
 * This task can/should not be called via the arbiter.
 **********************************************************/
void md5_check_commands(void)
{
	//When calling command was not the task itself
	if (rxcmd_md5.did != did_MD5)
	{
		//Send finished signal
		txcmd_md5.did = did_MD5;
		txcmd_md5.cmd = cmd_ipc_signal_finished;
		txcmd_md5.data = arbiter_get_return_value(&task_md5);
		ipc_queue_push(&txcmd_md5, sizeof(T_command), rxcmd_md5.did); //Signal that command is finished to calling task

		//Reset calling command
		rxcmd_md5.did = did_MD5;
	}

	//the idle task assumes that all commands are finished, so reset all sequence states
	arbiter_reset_sequence(&task_md5);

	//Check for new commands in queue
	if (ipc_get_queue_bytes(did_MD5) >= sizeof(T_command))
	{
		//Check the command in the queue and execute it
		if (ipc_queue_get(did_MD5, sizeof(T_command), &rxcmd_md5))
		{
			switch (rxcmd_md5.cmd) // switch for command
			{
				case cmd_md5_set_active_string:
					//Set the new address of the active string
					active_string = (char*)rxcmd_md5.data;
					break;
				
				case MD5_CMD_APPEND_STRING:
					//Set arguments
					args =arbiter_allocate_arguments(&task_md5,2);
					args[0] = (unsigned long)active_string;
					args[1] = rxcmd_md5.data;

					//Append the active string to the hash
					arbiter_callbyvalue(&task_md5, MD5_CMD_APPEND_STRING);
					break;

				case MD5_CMD_FINALIZE:
					md5_finalize((MD5_T*)rxcmd_md5.data);
					break;
				
				default:
					break;
			}
		}
	}
};

/*
 * Register memory for hash calculation.
 */
void md5_register_ipc(void)
{
	//Register command queue, can hold 5 commands
	ipc_register_queue(5 * sizeof(T_command), did_MD5);

	//Clear task
	arbiter_clear_task(&task_md5);
	arbiter_set_command(&task_md5, CMD_IDLE);

	//Initialize struct for received commands
	rxcmd_md5.did			= did_MD5;
	rxcmd_md5.cmd 			= 0;
	rxcmd_md5.data 			= 0;
	rxcmd_md5.timestamp 	= 0;
};

/*
 * initialize md5 hash with custom key
 */
void md5_initialize(MD5_T* hash, unsigned long* key)
{
	//Set initial message length to 0
	hash->message_length = 0;
	//Copy the input keys to the state of the hash
	sys_memcpy(hash->state,key,16);
	//Initialize the buffer with all 0s
	for(unsigned char count = 0; count<64; count++)
		hash->buff512[count] = 0;
};

/*
 * leftrotate input bits.
 */
unsigned long md5_leftrotate(unsigned long x, unsigned long c)
{
	return (x << c) | (x >> (32 - c));
};

/**********************************************************
 * Append character to hash.
 **********************************************************
 * Returns 1 when character is appended to hash.
 * 
 * Argument[0]:		unsigned char	ch_character
 * Argument[1]:		MD5_T*			hash
 * Return:			unsigned long	l_character_appended
 * 
 * call-by-value, nargs = 2
 **********************************************************/
void md5_append_char(void)
{
	//Get arguments
	unsigned char* ch_character = (unsigned char*)arbiter_get_argument(&task_md5);
	MD5_T* hash = (MD5_T*)arbiter_get_reference(&task_md5,1);

	//Allocate memory
	unsigned char* ch_position = (unsigned char*)arbiter_malloc(&task_md5,1);
	
	//Perform the command action
	switch (arbiter_get_sequence(&task_md5))
	{
	case SEQUENCE_ENTRY:
		//Get position of buffer to be written to
		*ch_position = (unsigned char)(hash->message_length % 64);
		//Write character
		hash->buff512[*ch_position] = *ch_character;
		//Increase message length
		hash->message_length++;
		//Goto next sequence without break
		arbiter_set_sequence(&task_md5, SEQUENCE_FINISHED);

	case SEQUENCE_FINISHED:
		//If buffer is full, calculate hash
		if (*ch_position == 63)
			md5_process512(hash);
		//exit the command
		arbiter_return(&task_md5, 1);
		break;

	default:
		break;
	}
};

/**********************************************************
 * Append string to hash.
 **********************************************************
 * Returns 1 when string is appended to hash.
 * 
 * Argument[0]:	char*			ch_string
 * Argument[1]:	MD5_T*			hash
 * Return:		unsigned long 	l_string_appended
 * 
 * call-by-value, nargs = 2
 *********************************************************/
void md5_append_string(void)
{
	//Get arguments
	char* ch_string = (char*)arbiter_get_reference(&task_md5,0);
	MD5_T* hash     = (MD5_T*)arbiter_get_reference(&task_md5,1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_md5))
	{
		case SEQUENCE_ENTRY:
			//Initialize the string counter
			task_md5.local_counter = 0;
			//go directly to next sequence
			arbiter_set_sequence(&task_md5, SEQUENCE_FINISHED);

		case SEQUENCE_FINISHED:
			//When end of string is not reached
			if (ch_string[task_md5.local_counter])
			{
				args = arbiter_allocate_arguments(&task_md5, 2);
				args[0] = (unsigned long)ch_string[task_md5.local_counter];
				args[1] = (unsigned long)hash;
				//Append character and increase string counter when character is appended
				if (arbiter_callbyvalue(&task_md5,MD5_CMD_APPEND_CHAR))
					task_md5.local_counter++;
			}
			else
				arbiter_return(&task_md5,1); //Exit the command
			break;

		default:
			break;
	}
}

/*
 * Calculate hash.
 * Code adapted from XCSOAR project.
 * Only works on little endian MCUs!
 * (STM32 is little endian)
 */
void md5_process512(MD5_T* hash)
{
	// assume exactly 512 bits

	// copy the 64 chars into the 16 longs
	unsigned long w[16];
	sys_memcpy(w, hash->buff512, 64);

	// Initialize hash value for this chunk:
	unsigned long a = hash->state[0], b = hash->state[1], c = hash->state[2], d = hash->state[3];

	// Main loop:
	for(int i = 0; i < 64; i++)
	{
		unsigned long f, g;
		if (i <= 15)
		{
			f = (b & c) | ((~b) & d);
			g = i;
		}
		else if (i <= 31)
		{
			f = (d & b) | ((~d) & c);
			g = (5 * i + 1) % 16;
		}
		else if (i <= 47)
		{
			f = b ^ c ^ d;
			g = (3 * i + 5) % 16;
		}
		else
		{
			f = c ^ (b | (~d));
			g = (7 * i) % 16;
		}

		unsigned long temp = d;
		d = c;
		c = b;
		b += md5_leftrotate((a + f + k[i] + w[g]), r[i]);
		a = temp;
	}

	// Add this chunk's hash to result so far:
	hash->state[0] += a;
	hash->state[1] += b;
	hash->state[2] += c;
	hash->state[3] += d;
};

/*
 * Write message length in bits to the last 8 bytes of buffer
 */
void md5_WriteLength(MD5_T* hash)
{
	//Compute bit length
	unsigned long long BitLength = hash->message_length*8;

	//Write length to buffer
	unsigned long long* pointer = (unsigned long long*)&hash->buff512[56];
	*pointer = BitLength;
};

/*
 * Finalize buffer for hash computation
 */
void md5_finalize(MD5_T* hash)
{
	//Get current buffer position
	unsigned char BufferPosition = (unsigned char)(hash->message_length%64);

	//Fill unused buffer with 0s
	for(unsigned char count = 63; count > BufferPosition; count--)
		hash->buff512[count] = 0;

	//Write 0x80 at end of buffer
	hash->buff512[BufferPosition] = 0x80;

	//Decide whether message length fits in remaining buffer or not
	if(BufferPosition >= 56)
	{
		//Message length does not fit
		//Process buffer
		md5_process512(hash);

		//Fill buffer with all 0s
		for(unsigned char count = 0; count < 64; count++)
			hash->buff512[count] = 0;
	}

	//Append message length to last bytes of buffer
	md5_WriteLength(hash);

	//Process new buffer
	md5_process512(hash);
};

/*
 * Get digest of hash after finalization of buffer.
 * Digest is saved to digest string of input. The string has to be 32 characters long.
 */
void md5_GetDigest(MD5_T* hash, char* digest)
{
	/*
	 * Pointer for hash.
	 * The pointer is 8bit long and used to swap the bytes of the 32bit hash.
	 */
	unsigned char* HashPointer = 0;

	//For every entry of the hash state
	for(unsigned char state = 0; state < 4; state++)
	{
		HashPointer = (unsigned char*)&hash->state[state];

		//Compute the string content, every state occupies 4x2 characters
		for(unsigned char byte = 0; byte < 4; byte++)
		{
			sys_hex2str(digest + 2*byte, *HashPointer, 2);
			HashPointer++;
		}
	}
};
