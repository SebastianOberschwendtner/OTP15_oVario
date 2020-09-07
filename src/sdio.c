/*
 * sdio.c
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */
#include "sdio.h"

SYS_T *sys;			 	//System struct with system information -> Date and Time is relevant
TASK_T task_sdio;	 	//Struct for the task status and memory of the sdio task
SDIO_T *SD;			 	//Struct for status information of the SD-card
FILE_T *dir;			//Is the global filehandler for directories
FILE_T *file;		 	//Filehandler for the currently openend file
unsigned long *args;	//Pointer to handle arguments for function calls which are call-by-value
T_command rxcmd_sdio;	//Command struct for received commands from other tasks via ipc
T_command txcmd_sdio;	//Command struct to transmit commands to other ipc tasks

/*
 * Register the memory for the sdio task
 */
void sdio_register_ipc(void)
{
	//Register and intialize struct for SDIO task
	SD = ipc_memory_register(sizeof(SDIO_T), did_SDIO);
	SD->status = SD_IS_BUSY; //Set card as busy
	SD->CardName[7] = 0;	 //End of String

	//Initialize task struct
	arbiter_clear_task(&task_sdio);
	arbiter_set_command(&task_sdio, SDIO_CMD_INIT);

	//Register and intialize the filehandler for the directory
	dir = ipc_memory_register(sizeof(FILE_T), did_DIRFILE);
	dir->name[11] = 0; //End of string

	//Register and intialize the filehandler for the active file
	file = ipc_memory_register(sizeof(FILE_T), did_FILEHANDLER);
	file->name[11] = 0; //End of string

	//Intialize the received command struct
	rxcmd_sdio.did			= did_SDIO;
	rxcmd_sdio.cmd 			= 0;
	rxcmd_sdio.data 		= 0;
	rxcmd_sdio.timestamp 	= 0;

	//Register command queue, 5 commands fit into queue
	ipc_register_queue(5 * sizeof(T_command), did_SDIO);
};

/*
 * Get everything relevant for IPC
 */
void sdio_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
	sys = ipc_memory_get(did_SYS);
};

/*
 * sdio system task
 */
void sdio_task(void)
{
	//Perform action according to active state
	switch (arbiter_get_command(&task_sdio))
	{
	case CMD_IDLE:
		sdio_check_commands();
		break;

	case SDIO_CMD_INIT:
		sdio_init();
		break;

	case SDIO_CMD_INIT_FILESYSTEM:
		sdio_init_filesystem();
		break;

	case SDIO_CMD_READ_ROOT:
		sdio_read_root();
		break;

	case SDIO_CMD_GET_NEXT_CLUSTER:
		sdio_get_next_cluster();
		break;

	case SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER:
		sdio_read_next_sector_of_cluster();
		break;

	case SDIO_CMD_GET_FILE:
		sdio_get_file();
		break;

	case SDIO_CMD_CLEAR_CLUSTER:
		sdio_clear_cluster();
		break;

	case SDIO_CMD_SET_CLUSTER:
		sdio_set_cluster();
		break;

	case SDIO_CMD_GET_EMPTY_ID:
		sdio_get_empty_id();
		break;

	case SDIO_CMD_SET_EMPTY_ID:
		sdio_set_empty_id();
		break;

	case SDIO_CMD_GET_EMPTY_CLUSTER:
		sdio_get_empty_cluster();
		break;

	case SDIO_CMD_GET_FILEID:
		sdio_get_fileid();
		break;

	case SDIO_CMD_FSEARCH:
		sdio_fsearch();
		break;

	case SDIO_CMD_CD:
		sdio_cd();
		break;

	case SDIO_CMD_FOPEN:
		sdio_fopen();
		break;

	case SDIO_CMD_FCLOSE:
		sdio_fclose();
		break;

	case SDIO_CMD_SET_FILESIZE:
		sdio_set_filesize();
		break;

	case SDIO_CMD_MAKE_ENTRY:
		sdio_make_entry();
		break;

	case SDIO_CMD_MKFILE:
		sdio_mkfile();
		break;

	case SDIO_CMD_MKDIR:
		sdio_mkdir();
		break;

	case SDIO_CMD_RM:
		sdio_rm();
		break;

	case SDIO_CMD_FCLEAR:
		sdio_fclear();
		break;

	case SDIO_CMD_WRITE_FILE:
		sdio_write_file();
		break;

	case SDIO_CMD_READ_ENDSECTOR_OF_FILE:
		sdio_read_end_sector_of_file();
		break;

	case SDIO_CMD_BYTE2FILE:
		sdio_byte2file();
		break;

	case SDIO_CMD_INT2FILE:
		sdio_int2file();
		break;

	case SDIO_CMD_LONG2FILE:
		sdio_long2file();
		break;

	case SDIO_CMD_STRING2FILE:
		sdio_string2file();
		break;

	case SDIO_CMD_NUM2FILE:
		sdio_num2file();
		break;

	case SDIO_CMD_NO_CARD:
		break;

	default:
		break;
	}
	// Check for errors
	sdio_check_error();
};

/***********************************************************
 * The idle state of the sd-card. 
 ***********************************************************
 * The task checks the ipc-queue for commands and executes
 * them. The status flags inside the SD-struct are set 
 * according to the command.
 * 
 * This command can/should not be called by the arbiter.
 ***********************************************************/
void sdio_check_commands(void)
{
	//When the task enters this state, the previous active command is finished
	SD->status |= SD_CMD_FINISHED;

	//When calling command was not the task itself
	if (rxcmd_sdio.did != did_SDIO)
	{
		//Send finished signal
		txcmd_sdio.did = did_SDIO;
		txcmd_sdio.cmd = cmd_ipc_signal_finished;
		txcmd_sdio.data = arbiter_get_return_value(&task_sdio);
		ipc_queue_push(&txcmd_sdio, sizeof(T_command), rxcmd_sdio.did); //Signal that command is finished to calling task

		//Reset calling command
		rxcmd_sdio.did = did_SDIO;
	}

	//the idle task assumes that all commands are finished, so reset all sequence states
	arbiter_reset_sequence(&task_sdio);

	//Check for new commands in queue
	if (ipc_get_queue_bytes(did_SDIO) >= 10)
	{
		//New commands are in the queue -> task is busy and command is not finished
		SD->status |= SD_IS_BUSY;
		SD->status &= ~SD_CMD_FINISHED;

		//Check the command in the queue and execute it
		if (ipc_queue_get(did_SDIO, 10, &rxcmd_sdio))
		{
			//Call-by-reference -> cmd.data has to have the address to the argument value.
			arbiter_callbyreference(&task_sdio, rxcmd_sdio.cmd, (void*)rxcmd_sdio.data);
		}
	}
	else
	{
		//No new commands, task is not busy
		SD->status &= ~SD_IS_BUSY;
	}
};

/***********************************************************
 * state for intializing the sdio and the sd-card.
 ***********************************************************

 * Argument:	none
 * Return:		none
 * 
 * call-by-value, nargs = none
 * call-by-reference
 ***********************************************************/
void sdio_init(void)
{
	// when the state wants to wait, decrease the counter. Otherwise perform the task
	if (task_sdio.wait_counter)
		task_sdio.wait_counter--;
	else
	{
		//read the current position in the state sequence
		switch (arbiter_get_sequence(&task_sdio))
		{
		case SEQUENCE_ENTRY:
			// initialize the peripherals: Clock, SDIO, DMA, Register Memory
			sdio_init_peripheral();
			// wait for a few taskticks
			task_sdio.wait_counter = ms2TASKTICKS(1);
			// set next sequence step
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_RESET_CARD);
			break;

		case SDIO_SEQUENCE_RESET_CARD:
			//Sent command to reset Card
			if (sdio_send_cmd(CMD0, 0))
			{
				//When command is sent, wait a few taskticks and goto next sequence
				task_sdio.wait_counter = ms2TASKTICKS(100);
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_SUPPLY);
			}
			break;

		case SDIO_SEQUENCE_SET_SUPPLY:
			//Send command to set supply voltage range
			if (sdio_send_cmd_R7(CMD8, CMD8_VOLTAGE_0 | CHECK_PATTERN))
			{
				//When command is sent, check response
				if ((SD->response & 0b11111111) == CHECK_PATTERN)
				{
					//Card is responding
					SD->status |= SD_CARD_DETECTED;
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_INIT_CARD_1);
				}
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //No Card was detected...
			}
			break;

		case SDIO_SEQUENCE_INIT_CARD_1:
			//Send command to goto subcommand
			if (sdio_send_cmd_R1(CMD55, 0))
			{
				//When command is sent, check whether subcommand was accepted
				if (SD->response & R1_APP_CMD)
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_INIT_CARD_2);
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //Command was not accepted
			}
			break;

		case SDIO_SEQUENCE_INIT_CARD_2:
			//Send command to set initialize sd-card
			if (sdio_send_cmd_R3(ACMD41, ACMD41_HCS | ACMD41_XPC | OCR_3_0V))
			{
				//When command is sent, check whether sd-card is busy (Card is NOT busy when bit is HIGH!)
				if (!(SD->response & ACMD41_BUSY))
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_INIT_CARD_1);
				else
				{
					// sd-card is not busy -> initialization is finished
					// Check whether SDHC-Card or not
					if (SD->response & ACMD41_CCS)
						SD->status |= SD_SDHC;
					// next sequence
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_RCA_1);
				}
			}
			break;

		case SDIO_SEQUENCE_GET_RCA_1:
			//Sent command to begin RCA transfer
			if (sdio_send_cmd_R2(CMD2, 0))
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_RCA_2); //Next sequence, when command is sent
			break;

		case SDIO_SEQUENCE_GET_RCA_2:
			//Sent command to Read RCA
			if (sdio_send_cmd_R6(CMD3, 0))
			{
				//When command is sent, read RCA and goto next sequence
				SD->RCA = (unsigned int)(SD->response >> 16);
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SELECT_CARD);
			}
			break;

		case SDIO_SEQUENCE_SELECT_CARD:
			//Send command to select card
			if (sdio_send_cmd_R1(CMD7, (SD->RCA << 16)))
			{
				//When command is sent, check response for error
				if (!(SD->response & R1_ERROR))
				{
					// No error -> card is selected
					SD->status |= SD_CARD_SELECTED;
#ifdef SDIO_4WIRE
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_4WIRE_1); //When 4-Wire mode should be enabled
#else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); // No 4-Wire mode -> Initialization is finished
#endif
				}
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //No Card was selected...
			}
			break;

		case SDIO_SEQUENCE_SET_4WIRE_1:
			//Send command to goto subcommand
			if (sdio_send_cmd_R1(CMD55, (SD->RCA << 16)))
			{
				//When command is sent, check whether subcommand was accepted
				if (SD->response & R1_APP_CMD)
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_4WIRE_2);
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //Command was not accepted
			}
			break;

		case SDIO_SEQUENCE_SET_4WIRE_2:
			//Send command to set 4-wire mode
			if (sdio_send_cmd_R1(ACMD6, 0b10))
			{
				//When command is sent, check for error
				if (!(SD->response & R1_ERROR))
				{
					// 4-wire mode was accepted
					task_sdio.wait_counter = ms2TASKTICKS(10);
					// Set 4-wire mode in sdio peripheral
					SDIO->CLKCR |= SDIO_CLKCR_WIDBUS_0;
				}
				// Card intialization is finished
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
			}
			break;

		case SEQUENCE_FINISHED:
			// Check whether card was initialized properly
			if (SD->status & (SD_CARD_DETECTED | SD_CARD_SELECTED))
			{
				// card was properly detected and selected
				sdio_set_clock(4000000);								   // set sdio clock to 4MHz
				arbiter_set_command(&task_sdio, SDIO_CMD_INIT_FILESYSTEM); // goto next state
			}
			else
			{
				// Card was not properly selected -> goto state NO_CARD
				arbiter_set_command(&task_sdio, SDIO_CMD_NO_CARD);
			}
			break;

		default:
			break;
		}
	}
};

/***********************************************************
 * State for initializing the filesystem (FAT).
 ***********************************************************
 *
 * Argument:	none
 * Return:		none
 * 
 * call-by-value, nargs = none
 * call-by-reference
 ***********************************************************/

// Variables needed for the initialization of the filesystem
//TODO Allocate memory inside the function for these variables.
unsigned long l_temp = 0, l_LBABegin = 0, l_RootDirSectors = 0, l_TotSec = 0, l_DataSec = 0, l_CountofCluster = 0;

void sdio_init_filesystem(void)
{
	//Perform action according to sequence
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		// Read first block of sd-card
		if (sdio_read_block(dir->buffer, 0))
		{
			if (sdio_read_int(dir->buffer, MBR_MAGIC_NUMBER_pos) == 0xAA55) //Check filesystem for corruption
			{
				//Check which parition table is used
				if (sdio_read_byte(dir->buffer, MBR_PART1_TYPE_pos) == 0xEE)
				{
					//EFI table is used
					l_temp = sdio_read_long(dir->buffer, MBR_PART1_LBA_BEGIN_pos); //Read EFI Begin Address
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_EFI_1);	   //Next sequence
				}
				else
				{
					//MBR is used (Not completely true, but this driver does only support EFI and MBR...)
					l_temp = sdio_read_long(dir->buffer, MBR_PART1_LBA_BEGIN_pos); //Read MBR Begin Address
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_BPB);	   //Next Sequence
				}
			}
			else
			{
				SD->err = SDIO_ERROR_CORRUPT_FILESYSTEM;			 //Set Error
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //File system is corrupt#
			}
		}
		break;

	case SDIO_SEQUENCE_READ_EFI_1:
		//Read EFI file system
		if (sdio_read_block(dir->buffer, l_temp))
		{
			l_temp = sdio_read_long(dir->buffer, EFI_TABLE_LBA_BEGIN_pos); //Get begin of file system
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_EFI_2);	   //Next sequence
		}
		break;

	case SDIO_SEQUENCE_READ_EFI_2:
		//Read EFI partition table
		if (sdio_read_block(dir->buffer, l_temp))
		{
			l_temp = sdio_read_long(dir->buffer, EFI_PART_LBA_BEGIN_pos); //Get begin of file system
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_BPB);	  //Next sequence
		}
		break;

	case SDIO_SEQUENCE_READ_BPB:
		//Read beginning of filesystem
		if (sdio_read_block(dir->buffer, l_temp))
		{
			//Check for errors and read filesystem variables
			if ((sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos) == SDIO_BLOCKLEN) && (sdio_read_byte(dir->buffer, BPB_NUM_FAT_pos) == 2))
			{
				//Save the LBA begin address
				l_LBABegin = l_temp;
				/************Determine the FAT-type according to Microsoft specifications****************************************************************************/
				//Determine the count of sectors occupied by the root directory
				l_RootDirSectors = ((sdio_read_int(dir->buffer, BPB_ROOT_ENT_CNT_pos) * 32) + (sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos) - 1)) / sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos);

				//Determine the count of sectors in the data region of the volume
				if (sdio_read_int(dir->buffer, BPB_FAT_SIZE_16_pos))
					SD->FATSz = sdio_read_int(dir->buffer, BPB_FAT_SIZE_16_pos);
				else
					SD->FATSz = sdio_read_long(dir->buffer, BPB_FAT_SIZE_32_pos);

				if (sdio_read_int(dir->buffer, BPB_TOT_SEC_16_pos))
					l_TotSec = sdio_read_int(dir->buffer, BPB_TOT_SEC_16_pos);
				else
					l_TotSec = sdio_read_long(dir->buffer, BPB_TOT_SEC_32_pos);

				l_DataSec = l_TotSec - (sdio_read_int(dir->buffer, BPB_RES_SEC_pos) + (sdio_read_byte(dir->buffer, BPB_NUM_FAT_pos) * SD->FATSz) + l_RootDirSectors);

				//determine the count of clusters as
				SD->SecPerClus = sdio_read_byte(dir->buffer, BPB_SEC_PER_CLUS_pos);
				l_CountofCluster = l_DataSec / SD->SecPerClus;

				if (l_CountofCluster < 4085)
				{
					/* Volume is FAT12 => not supported! */
					SD->err = SDIO_ERROR_WRONG_FAT;
				}
				else if (l_CountofCluster < 65525)
				{
					/* Volume is FAT16 */
					SD->status |= SD_IS_FAT16;
				}
				else
				{
					/* Volume is FAT32 */
				}
				/********Get the Fat begin address ********************************************************************/
				SD->LBAFATBegin = l_LBABegin + sdio_read_int(dir->buffer, BPB_RES_SEC_pos);

				/********Get sector number of root directory **********************************************************/
				SD->FirstDataSecNum = SD->LBAFATBegin + (2 * SD->FATSz) + l_RootDirSectors;

				if (!(SD->status & SD_IS_FAT16))
					SD->RootDirClus = sdio_read_long(dir->buffer, BPB_ROOT_DIR_CLUS_pos);
				else
					SD->RootDirClus = 0;

				SD->err = 0;
			}
			else
				SD->err = SDIO_ERROR_WRONG_FILESYSTEM;

			//Goto to sequence FINISHED
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		// Check whether an error occurred
		if (SD->err)
			arbiter_set_command(&task_sdio, SDIO_CMD_NO_CARD); //Error: Goto state NO_CARD
		else
			arbiter_set_command(&task_sdio, SDIO_CMD_READ_ROOT); //No Error: Goto state READ_ROOT
		break;

	default:
		break;
	}
};

/***********************************************************
 * Read the root directory of the sd-card.
 ***********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * call-by-value, nargs = none
 * call-by-reference
 ***********************************************************/
void sdio_read_root(void)
{
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		if (SD->status & SD_IS_FAT16) //FAT16 uses a specific region for the root directory
		{
			if (sdio_read_block(dir->buffer, SD->LBAFATBegin + (2 * SD->FATSz)))
			{
				dir->id = 0;
				dir->CurrentCluster = 0;
				dir->CurrentSector = 1;
				dir->DirCluster = 0;
				dir->StartCluster = 0;

				//Goto next sequence
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
			}
		}
		else
		{
			if (sdio_read_cluster(dir, SD->RootDirClus))
			{
				dir->id = 0;
				dir->DirCluster = SD->RootDirClus;
				dir->StartCluster = SD->RootDirClus;

				//Goto next sequence
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
				;
			}
		}
		break;

	case SEQUENCE_FINISHED:
		//get the arguments for the command sdio_get_file()
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = 0;				  //fileid
		args[1] = (unsigned long)dir; //filehandler

		//Call command
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILE))
		{
			//When the root directory was read, set the card name
			for (unsigned char count = 0; count < 8; count++)
				SD->CardName[count] = dir->name[count];

			//Exit the command
			arbiter_return(&task_sdio, 0);
		}
		break;

	default:
		break;
	}
};

/*
 * Initialize the peripherals for the sdio communication
 */
void sdio_init_peripheral(void)
{
	//Activate Clocks
	RCC->APB2ENR |= RCC_APB2ENR_SDIOEN; //Sdio adapter
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; //DMA2
	gpio_en(GPIO_C);
	gpio_en(GPIO_D);

	/*
	 * Configure Pins:
	 * PD2		SD_CMD	AF12	Open-drain	Pullup
	 * PC8		DAT0	AF12	Open-drain	Pullup
	 * PC9		DAT1	AF12	Open-drain	Pullup
	 * PC10		DAT2	AF12	Open-drain	Pullup
	 * PC11		DAT3	AF12	Open-drain	Pullup
	 * PC12 	SD_CLK	AF12	Push-pull	No Pull
	 */
	GPIOC->MODER |= GPIO_MODER_MODER12_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER10_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER8_1;
	//GPIOC->OTYPER	|= GPIO_OTYPER_OT_11 | GPIO_OTYPER_OT_10 | GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_8;
	GPIOC->PUPDR |= GPIO_PUPDR_PUPDR11_0 | GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR8_0;
	GPIOC->AFR[1] |= (12 << 16) | (12 << 12) | (12 << 8) | (12 << 4) | (12 << 0);

	GPIOD->MODER |= GPIO_MODER_MODER2_1;
	//GPIOD->OTYPER	= GPIO_OTYPER_OT_2;
	GPIOD->PUPDR = GPIO_PUPDR_PUPDR2_0;
	GPIOD->AFR[0] |= (12 << 8);

	/*
	 * Configure sdio
	 */
	sdio_set_clock(400000);									   //set sdio clock
	SDIO->CLKCR |= SDIO_CLKCR_CLKEN;						   //enable clock
	SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0; //Enable SD_CLK
	SDIO->DTIMER = 0xFFFFFFF;								   //Data timeout during data transfer, includes the program time of the sd-card memory -> adjust for slow cards
	SDIO->DLEN = SDIO_BLOCKLEN;								   //Numbers of bytes per block
	NVIC_EnableIRQ(SDIO_IRQn);								   //Enable the global SDIO interrupt

	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

	/*
	 * Configure DMA2 Stream 6 Channel 4:
	 * -Memorysize:	32bit	incremented
	 * -Peripheral:	32bit	fixed
	 * -Peripheral flow control
	 * -Peripheral Burst with 4 Beats
	 */
#define DMA_CONFIG_SDIO (DMA_SxCR_CHSEL_2 | DMA_SxCR_PBURST_0 | DMA_SxCR_PSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_PFCTRL)

	DMA2_Stream6->CR = DMA_CONFIG_SDIO;
	//DMA2_Stream3->NDTR = 128;							//128*4bytes to transmit
	DMA2_Stream6->PAR = (unsigned long)&SDIO->FIFO;		 //Peripheral address
	DMA2_Stream6->M0AR = (unsigned long)&dir->buffer[0]; //Buffer start address
	DMA2_Stream6->FCR = DMA_SxFCR_DMDIS;				 // | DMA_SxFCR_FTH_1 | DMA_SxFCR_FTH_0;
};

/*
 * set the clock speed to the sd card
 */
void sdio_set_clock(unsigned long l_clock)
{
	//Enable Clock and set to desired speed [400kHz : 25 MHz] clock inactive when bus inactive
	if (l_clock < 400000)
		l_clock = 400000;
	else if (l_clock > 25000000)
		l_clock = 25000000;

	//Save current CLKCR
	unsigned long config = SDIO->CLKCR;

	//Write new CLKDIV
	SDIO->CLKCR = (config & (127 << 8)) | ((SDIOCLK / l_clock) - 2);
};

/*
 * Check SDIO task for errors
 */
void sdio_check_error(void)
{
	// Any SDIO Error Occured?
	if (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_CTIMEOUT))
		SD->err = SDIO_ERROR_TRANSFER_ERROR;

	// Any DMA Error Occurred?
	//TODO Add DMA error check here!

	// Any File Error Occurred?
	//TODO Add File error Check here!

	// Change command when an error occurrred
	if (SD->err)
		arbiter_set_command(&task_sdio, SDIO_CMD_NO_CARD);
};

/*
 * Send command to sd-card without expected response. Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (no response required)
		if (!(SDIO->STA & SDIO_STA_CMDSENT))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDSENTC;
			return 1;
		}
	}
	return 0;
};

/*
 * Send command to sd-card with expected R1 response. Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd_R1(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (R1 response required)
		if (!(SDIO->STA & SDIO_STA_CMDREND))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDRENDC;
			// Save response
			SD->response = SDIO->RESP1;
			return 1;
		}
	}
	return 0;
};

/*
 * Send command to sd-card with expected R2 response (Only the first 48 bits of the response are checked). 
 * Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd_R2(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (R2 response required)
		if (!(SDIO->STA & SDIO_STA_CMDREND))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDRENDC;
			// Save response
			SD->response = SDIO->RESP1;
			return 1;
		}
	}
	return 0;
};

/*
 * Send command to sd-card with expected R3 response. Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd_R3(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (R3 response required, without matching CCRC)
		if (!(SDIO->STA & (SDIO_STA_CCRCFAIL | SDIO_STA_CMDREND)))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDRENDC | SDIO_ICR_CCRCFAILC;
			// Save response
			SD->response = SDIO->RESP1;
			return 1;
		}
	}
	return 0;
};

/*
 * Send command to sd-card with expected R6 response. Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd_R6(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (R6 response required)
		if (!(SDIO->STA & SDIO_STA_CMDREND))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDRENDC;
			// Save response
			SD->response = SDIO->RESP1;
			return 1;
		}
	}
	return 0;
};

/*
 * Send command to sd-card with expected R7 response. Returns 1 when communication is finished.
 */
unsigned char sdio_send_cmd_R7(unsigned char ch_cmd, unsigned long l_arg)
{
	// Command transfer in progress?
	if (!(SDIO->STA & SDIO_STA_CMDACT))
	{
		// Finished Sending Command? (R7 response required)
		if (!(SDIO->STA & SDIO_STA_CMDREND))
		{
			//No Active Command, so new command can be sent
			SDIO->ARG = l_arg;
			SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
		}
		else
		{
			// The command is finished, clear all flags
			SDIO->ICR = SDIO_ICR_CMDRENDC;
			// Save response
			SD->response = SDIO->RESP1;
			return 1;
		}
	}
	return 0;
};

/*
 * Set card inactive for save removal of card.
 * Returns 1 when card is ejected
 */
unsigned char sdio_set_inactive(void)
{
	// Only eject card, when card is selected and detected
	if (SD->status & (SD_CARD_DETECTED | SD_CARD_SELECTED))
	{
		//send command to go to inactive state
		if (sdio_send_cmd(CMD15, (SD->RCA << 16)))
		{
			//Delete flags for selected card when command is sent
			SD->status &= ~(SD_CARD_SELECTED | SD_CARD_DETECTED);
			return 1;
		}
	}
	else
		return 1;
	return 0;
};

/*
 * Read block data from card at specific block address. Data is stored in buffer specified at function call.
 * The addressing uses block number and adapts to SDSC and SDHC cards. Returns 1 when block transfer is finished.
 */
unsigned char sdio_read_block(unsigned long *databuffer, unsigned long l_block_address)
{
	//Data Transfer in Progress?
	if (!(SDIO->STA & SDIO_STA_RXACT))
	{
		//Data Transfer finished?
		if (SDIO->STA & SDIO_STA_DBCKEND)
		{
			//Block data was read
			if (!(DMA2_Stream6->CR & DMA_SxCR_EN)) // Wait until DMA Stream is finished
			{
				DMA2->HIFCR |= DMA_HIFCR_CTCIF6;					//Clear DMA Transfer Finished flag
				SDIO->ICR |= SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC; //Clear SDIO block transfer finished flag
				return 1;
			}
		}
		else
		{
			//No block transfer in progress
			if (!(DMA2_Stream6->CR & DMA_SxCR_EN))
				sdio_dma_start_receive(databuffer); //When Stream is not yet enabled, enable it

			//Get address of block data
			if (!(SD->status & SD_SDHC)) //If SDSC card, byte addressing is used
				l_block_address *= SDIO_BLOCKLEN;

			/* Send command to start block transfer and start data transfer when command was sent.
			 * The return value is not used, because in block transfer interrupts are used to manage the
			 * command transfer. */
			sdio_send_cmd_R1(CMD17, l_block_address);
		}
	}
	return 0;
};

/*
 * Interrupt when command transfer is finished and data transfer should be enabled.
 */
void SDIO_IRQHandler(void)
{
	// Clear Flags
	SDIO->ICR |= SDIO_ICR_CMDRENDC;
	// Disable interrupts
	SDIO->MASK = 0;

	//Check in which direction the data should be sent
	if (DMA2_Stream6->CR & DMA_SxCR_DIR_0)
		SDIO->DCTRL = (0b1001 << 4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN; //Start data transmit
	else
		SDIO->DCTRL = (0b1001 << 4) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN; //Start data receive
};

/*
 * Write block data to card at specific block address. Data is read from buffer specified at function call.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 * Returns 1 when communication is finished.
 */
unsigned char sdio_write_block(unsigned long *databuffer, unsigned long l_block_address)
{
	//Data Transfer in Progress?
	if (!(SDIO->STA & SDIO_STA_RXACT))
	{
		//Data Transfer finished?
		if (SDIO->STA & SDIO_STA_DBCKEND)
		{
			//Block data was read
			if (!(DMA2_Stream6->CR & DMA_SxCR_EN)) // Wait until DMA Stream is finished
			{
				DMA2->HIFCR |= DMA_HIFCR_CTCIF6;					//Clear DMA Transfer Finished flag
				SDIO->ICR |= SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC; //Clear SDIO block transfer finished flag
				return 1;
			}
		}
		else
		{
			//No block transfer in progress
			if (!(DMA2_Stream6->CR & DMA_SxCR_EN))
				sdio_dma_start_transmit(databuffer); //When Stream is not yet enabled, enable it

			//Get address of block data
			if (!(SD->status & SD_SDHC)) //If SDSC card, byte addressing is used
				l_block_address *= SDIO_BLOCKLEN;

			/* Send command to start block transfer and start data transfer when command was sent.
			 * The return value is not used, because in block transfer interrupts are used to manage the
			 * command transfer. */
			sdio_send_cmd_R1(CMD24, l_block_address);
		}
	}
	return 0;
};

/*
 * Enable the DMA for data receive and start the transfer. Data is stored in buffer specified at function call.
 * DMA must not be enabled when calling this function!
 */
void sdio_dma_start_receive(unsigned long *databuffer)
{
	//Enable interrupt for SDIO -> CMDREND
	SDIO->MASK = SDIO_MASK_CMDRENDIE;
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	//Buffer start address
	DMA2_Stream6->M0AR = (unsigned long)databuffer;
	//Start transfer
	DMA2_Stream6->CR = DMA_CONFIG_SDIO | DMA_SxCR_EN;
};

/*
 * Enable the DMA for data transmit. Data is read from buffer specified at function call.
 * DMA must not be enabled when calling this function!
 */
void sdio_dma_start_transmit(unsigned long *databuffer)
{
	//Enable interrupt for SDIO -> CMDREND
	SDIO->MASK = SDIO_MASK_CMDRENDIE;
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	//Buffer start address
	DMA2_Stream6->M0AR = (unsigned long)databuffer;
	//Start transfer
	DMA2_Stream6->CR = DMA_CONFIG_SDIO | DMA_SxCR_DIR_0 | DMA_SxCR_EN; //DMA Config + Dir: from memory to peripheral
};

/*
 * Compute the LBA begin address of a specific cluster
 * Cluster numbering begins at 2.
 */
unsigned long sdio_get_lba(unsigned long l_cluster)
{
	if (l_cluster >= 2)
		return SD->FirstDataSecNum + (l_cluster - 2) * (unsigned long)SD->SecPerClus;
	else										  //if cluster number is zero, only the root directory of FAT16 can have this value
		return SD->LBAFATBegin + (2 * SD->FATSz); //Root dir of FAT16
};

/*
 * Read the first sector of a specific cluster from sd-card.
 * Returns 1 when communication is finished
 */
unsigned char sdio_read_cluster(FILE_T *filehandler, unsigned long l_cluster)
{
	//Set the current sector and cluster
	filehandler->CurrentCluster = l_cluster;
	filehandler->CurrentSector = 1;

	//Get the LBA address of cluster and the read the corresponding first sector
	return sdio_read_block(&filehandler->buffer[0], sdio_get_lba(l_cluster));
};

/***********************************************************
 * Write cluster with zeros to empty space.
 ***********************************************************
 * 
 * Argument[0]:	unsigned long 	l_cluster
 * Argument[1]: unsigned long*	l_databuffer
 * Return:		none
 * 
 * call-by-value, nargs = 2
 ***********************************************************/
void sdio_clear_cluster(void)
{
	//Get arguments of command
	unsigned long *l_cluster = arbiter_get_argument(&task_sdio);						 //Argument[0]
	unsigned long *l_databuffer = (unsigned long *)arbiter_get_reference(&task_sdio, 1); //Argument[1]
	//Initialize command variables
	unsigned long current_block = 0;

	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//How many sectors should be cleared?
		task_sdio.local_counter = SD->SecPerClus;

		//Delete the current filebuffer
		for (unsigned int i_count = 0; i_count < (SDIO_BLOCKLEN / 4); i_count++)
			l_databuffer[i_count] = 0;

		//Goto next sequence
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Get cluster address and calculate the block address
		current_block = (*l_cluster) + (SD->SecPerClus - task_sdio.local_counter);

		//Delete one sector
		if (sdio_write_block(l_databuffer, current_block))
		{
			//When sector was deleted goto next sector
			task_sdio.local_counter--;

			//When all sectors of the cluster are deleted, exit the function
			if (task_sdio.local_counter == 0)
				arbiter_return(&task_sdio, 0);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Set the state of a cluster, writing the FAT.
 ***********************************************************
 *  
 * Argument[0]:	unsigned long	l_cluster
 * Argument[1]:	unsigned long	l_state
 * Argument[2]: unsigned long*	l_databuffer
 * Return:		none
 * 
 * call-by-value, nargs = 3
 ***********************************************************/
void sdio_set_cluster(void)
{
	//Get input arguments
	unsigned long *l_cluster = arbiter_get_argument(&task_sdio);						 //Argument[0]
	unsigned long *l_state = l_cluster + 1;												 //Argument[1]
	unsigned long *l_databuffer = (unsigned long *)arbiter_get_reference(&task_sdio, 2); //Argument[1]

	//Allocate memory for command
	unsigned long *l_lba = arbiter_malloc(&task_sdio, 1);

	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Get the LBA address of the cluster in the 1st FAT
		*l_lba = sdio_get_fat_sec(*l_cluster, 1);

		//Read the sector with the entry of the current cluster in the 1st FAT
		if (sdio_read_block(l_databuffer, *l_lba))
		{
			//Determine the FAT entry
			sdio_write_fat_pos(l_databuffer, sdio_get_fat_pos(*l_cluster), *l_state);

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_FAT_1);
		}
		break;

	case SDIO_SEQUENCE_SET_FAT_1:
		//Write 1st FAT to sd-card
		if (sdio_write_block(l_databuffer, *l_lba))
		{
			//Get the LBA address of the cluster in the 2nd FAT
			*l_lba = sdio_get_fat_sec(*l_cluster, 2);

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_FAT_2);
		}
		break;

	case SDIO_SEQUENCE_SET_FAT_2:
		//Read the sector with the entry of the current cluster in the 2nd FAT
		if (sdio_read_block(l_databuffer, *l_lba))
		{
			//Determine the FAT entry
			sdio_write_fat_pos(l_databuffer, sdio_get_fat_pos(*l_cluster), *l_state);
			//Goto sequence FINISHED
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Write 2nd FAT to sd-card
		if (sdio_read_block(l_databuffer, *l_lba))
		{
			//Exit command
			arbiter_return(&task_sdio, 0);
		}
		break;

	default:
		break;
	}
};

/*
 * Read a byte from buffer with the corresponding location in the sd-card memory
 */
unsigned char sdio_read_byte(unsigned long *databuffer, unsigned int i_address)
{
	unsigned char ch_address = (unsigned char)((i_address / 4)); //Get address of buffer
	unsigned char ch_byte = (unsigned char)(i_address % 4);		 //Get number of byte in buffer

	return (unsigned char)((databuffer[ch_address] >> (ch_byte * 8)) & 0xFF); //Write in buffer
};

/*
 * write a byte to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_byte(unsigned long *databuffer, unsigned int i_address, unsigned char ch_data)
{
	unsigned char ch_address = (unsigned char)((i_address / 4)); //Get adress of buffer
	unsigned char ch_byte = (unsigned char)(i_address % 4);		 //Get number of byte in buffer

	databuffer[ch_address] &= ~(((unsigned long)0xFF) << (ch_byte * 8));   //Delete old byte
	databuffer[ch_address] |= (((unsigned long)ch_data) << (ch_byte * 8)); //Write new byte
};

/*
 * Read a int from buffer with the corresponding location in the sd-card memory
 */
unsigned int sdio_read_int(unsigned long *databuffer, unsigned int i_address)
{
	unsigned int i_data = 0;
	i_data = (sdio_read_byte(databuffer, i_address) << 0);
	i_data |= (sdio_read_byte(databuffer, i_address + 1) << 8);
	return i_data;
};

/*
 * Write a int to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_int(unsigned long *databuffer, unsigned int i_address, unsigned int i_data)
{
	sdio_write_byte(databuffer, i_address, (unsigned char)(i_data & 0xFF));
	sdio_write_byte(databuffer, i_address + 1, (unsigned char)((i_data >> 8) & 0xFF));
};

/*
 * Read a word from buffer with the corresponding location in the sd-card memory
 */
unsigned long sdio_read_long(unsigned long *databuffer, unsigned int i_address)
{
	unsigned long l_data = 0;
	/*
	for(unsigned char ch_count = 0; ch_count<4; ch_count++)
		l_data |= (sdio_read_byte(i_adress+ch_count)<<(8*ch_count));
	 */
	l_data = (sdio_read_byte(databuffer, i_address) << 0);
	l_data |= (sdio_read_byte(databuffer, i_address + 1) << 8);
	l_data |= (sdio_read_byte(databuffer, i_address + 2) << 16);
	l_data |= (sdio_read_byte(databuffer, i_address + 3) << 24);

	return l_data;
};

/*
 * Write a word to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_long(unsigned long *databuffer, unsigned int i_address, unsigned long l_data)
{
	sdio_write_byte(databuffer, i_address, (unsigned char)(l_data & 0xFF));
	sdio_write_byte(databuffer, i_address + 1, (unsigned char)((l_data >> 8) & 0xFF));
	sdio_write_byte(databuffer, i_address + 2, (unsigned char)((l_data >> 16) & 0xFF));
	sdio_write_byte(databuffer, i_address + 3, (unsigned char)((l_data >> 24) & 0xFF));
};

/***********************************************************
 * Read a file/directory with fileid.
 ***********************************************************

 * Argument[0]:	unsigned long	file_id
 * Argument[1]:	FILE_T*			filehandler
 * Return:		none
 * 
 * call-by-value, nargs = 2
 ***********************************************************/
void sdio_get_file(void)
{
	// Get the argument for the command
	unsigned long *file_id = arbiter_get_argument(&task_sdio);			  //Argument[0]
	FILE_T *filehandler = (FILE_T *)arbiter_get_reference(&task_sdio, 1); //Argument[1]

	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read start cluster of directory
		if (sdio_read_cluster(dir, dir->StartCluster))
		{
			//Set local counter
			task_sdio.local_counter = (*file_id / (SDIO_BLOCKLEN / 32));
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//Get the cluster sector which corresponds to the given file id (one sector can contain SDIO_BLOCKLEN/32 entries, one entry is 32bytes)
		if (task_sdio.local_counter)
		{
			//Read the next sector of the cluster as often as local_count says
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//Decrement call counter
				task_sdio.local_counter--;
				//See whether next sector was found
				if (arbiter_get_return_value(&task_sdio) == 0)
					SD->err = SDIO_ERROR_NO_SUCH_FILEID;
			}
		}
		else
		{
			//The current sector contains the fileid
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Read the entry of the file
		/*
		 	 * The fileid is a consecutive number over all sectors of the directory. To get the specific entry within one sector,
		 	 * the remainder of the entries per sector has to be used to access the information in the sd-buffer.
		 	 */
		filehandler->id = *file_id;
		unsigned long l_fileid = *file_id % (SDIO_BLOCKLEN / 32);
		sdio_get_name(filehandler);																				  //Name
		filehandler->DirAttr = sdio_read_byte(dir->buffer, l_fileid * 32 + DIR_ATTR_pos);						  //Attribute Byte
		filehandler->DirCluster = dir->CurrentCluster;															  //Cluster in which entry can be found
		filehandler->StartCluster = (unsigned long)sdio_read_int(dir->buffer, l_fileid * 32 + DIR_FstClusLO_pos); //Cluster in which the file starts (FAT16)
		if (!(SD->status & SD_IS_FAT16))
			filehandler->StartCluster |= (((unsigned long)sdio_read_int(dir->buffer, l_fileid * 32 + DIR_FstClusHI_pos)) << 16); //Cluster in which the file starts (FAT32)
		filehandler->size = sdio_read_long(dir->buffer, l_fileid * 32 + DIR_FILESIZE_pos);

		//exit the command
		arbiter_return(&task_sdio, 0);
		break;

	default:
		break;
	}
};

/***********************************************************
 * Read the next sector of the current file.
 ***********************************************************
 *
 * Read the next sectors of a file until the end of file is
 * reached.
 * Returns 1 when the next setor was found and could be read.
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long	file_found
 * 
 * call-by-reference
 ***********************************************************/
void sdio_read_next_sector_of_cluster(void)
{
	//Get arguments
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio); //Argument[0]

	//intialize temp memory
	unsigned long l_FATEntry = 0;

	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Check whether next sector can be directly read
		//If the current sector is not the last sector of the current cluster, the next sector of the cluster is read
		if (filehandler->CurrentSector != SD->SecPerClus)
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR);
		else
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_NEXT_CLUSTER);
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR:
		//When communication is finished, exit the command
		if (sdio_read_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster) + filehandler->CurrentSector++))
			arbiter_return(&task_sdio, 0);
		break;

	case SDIO_SEQUENCE_GET_NEXT_CLUSTER:
		/*
	 	 * If the current sector is the last sector of the cluster, the next cluster is determined using the FAT.
	 	 * If the FAT gives an unexpected result (End-of-File, Bad-Sector or empty cluster) the transmission is aborted.
	 	 */
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = filehandler->CurrentCluster;
		args[1] = (unsigned long)filehandler->buffer;

		//Get the entry of the FAT table
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_NEXT_CLUSTER))
		{
			l_FATEntry = arbiter_get_return_value(&task_sdio);

			if (SD->status & SD_IS_FAT16) //If FAT16
			{
				if (l_FATEntry == 0xFFFF) //End of file
					arbiter_return(&task_sdio, 0);
				else if (l_FATEntry >= 0xFFF8) //Bad Sector
				{
					SD->err = SDIO_ERROR_BAD_SECTOR;
					arbiter_return(&task_sdio, 0);
				}
				else if (l_FATEntry == 0x0000) //Cluster empty
				{
					SD->err = SDIO_ERROR_FAT_CORRUPTED;
					arbiter_return(&task_sdio, 0);
				}
				else //Read the next cluster
				{
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
				}
			}
			else //If FAT32
			{
				if (l_FATEntry == 0xFFFFFFFF) //End of file
					arbiter_return(&task_sdio, 0);
				else if (l_FATEntry >= 0xFFFFFFF8) //Bad Sector
				{
					SD->err = SDIO_ERROR_BAD_SECTOR;
					arbiter_return(&task_sdio, 0);
				}
				else if (l_FATEntry == 0x00000000) //Cluster Empty
				{
					SD->err = SDIO_ERROR_FAT_CORRUPTED;
					arbiter_return(&task_sdio, 0);
				}
				else //Read the next cluster
				{
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
				}
			}
		}
		break;

	case SEQUENCE_FINISHED:
		//Read the next cluster address and read the first sector
		l_FATEntry = arbiter_get_return_value(&task_sdio);
		if (sdio_read_cluster(filehandler, l_FATEntry))
			arbiter_return(&task_sdio, 1);
		break;

	default:
		break;
	}
};

/***********************************************************
 * Get the next sector of the current cluster
 ***********************************************************
 * 
 * Get the next cluster of a current cluster, reading the FAT.
 * The next cluster is written to the return-value-pointer.
 * 
 * Argument[0]: unsigned long 	l_current_cluster
 * Argument[1]: unsigned long*	l_databuffer
 * Returns:		unsigned long	next_fat_sector
 * 
 * call-by-value, nargs = 2
 ***********************************************************/
void sdio_get_next_cluster(void)
{
	//Get the pointer to the argument data for the command
	unsigned long *l_currentcluster = arbiter_get_argument(&task_sdio);					 //Argument[0]
	unsigned long *l_databuffer = (unsigned long *)arbiter_get_reference(&task_sdio, 1); //Argument[1]

	//Perform the command sequences
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read the sector with the entry of the current cluster
		if (sdio_read_block(l_databuffer, sdio_get_fat_sec(*l_currentcluster, 1)))
		{
			//When block is read goto finished sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;
	case SEQUENCE_FINISHED:
		//Determine the FAT entry and return the function call
		arbiter_return(&task_sdio, sdio_read_fat_pos(l_databuffer, sdio_get_fat_pos(*l_currentcluster)));
		break;

	default:
		break;
	}
};

/*
 * Write the current sector of the current cluster to sd-card.
 * Returns 1 when the communication is finished
 */
unsigned char sdio_write_current_sector(FILE_T *filehandler)
{
	//Write sector to LBA sector address. The current sector is decremented by 1, because the sector count starts counting at 1.
	return sdio_write_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster) + (filehandler->CurrentSector - 1));
};

/*
 * Get the FAT sector for a specific cluster
 */
unsigned long sdio_get_fat_sec(unsigned long l_cluster, unsigned char ch_FATNum)
{
	unsigned long l_FATOffset = 0;

	if (SD->status & SD_IS_FAT16)
		l_FATOffset = l_cluster * 2;
	else
		l_FATOffset = l_cluster * 4;

	return SD->LBAFATBegin + (l_FATOffset / SDIO_BLOCKLEN) + (SD->FATSz * (ch_FATNum - 1));
};

/*
 * Get the byte position of a cluster withing a FAT sector
 */
unsigned long sdio_get_fat_pos(unsigned long l_cluster)
{
	unsigned long l_FATOffset = 0;

	if (SD->status & SD_IS_FAT16)
		l_FATOffset = l_cluster * 2;
	else
		l_FATOffset = l_cluster * 4;

	return l_FATOffset % SDIO_BLOCKLEN;
};

/*
 * Read the FAT entry of the loaded Sector and return the content
 */
unsigned long sdio_read_fat_pos(unsigned long *databuffer, unsigned long l_pos)
{
	if (SD->status & SD_IS_FAT16)
		return (unsigned long)sdio_read_int(databuffer, l_pos);
	else
		return sdio_read_long(databuffer, l_pos);
};

/*
 * Read the name of a file or directory. The sector of the file id has to be loaded.
 */
void sdio_get_name(FILE_T *filehandler)
{
	unsigned long l_id = filehandler->id % (SDIO_BLOCKLEN / 32); //one sector can contain 16 files or directories
	//Read the name string
	for (unsigned char ch_count = 0; ch_count < 11; ch_count++)
		filehandler->name[ch_count] = sdio_read_byte(dir->buffer, l_id * 32 + ch_count);
};

/*
 * Write the FAT entry of the loaded Sector
 */
void sdio_write_fat_pos(unsigned long *databuffer, unsigned long l_pos, unsigned long l_data)
{
	if (SD->status & SD_IS_FAT16)
		sdio_write_int(databuffer, (unsigned int)l_pos, (unsigned int)l_data);
	else
		sdio_write_long(databuffer, (unsigned int)l_pos, l_data);
};

/*
 * Compare two strings. The first string sets the compare length.
 */
unsigned char sdio_strcmp(char *pch_string1, char *pch_string2)
{
	while (*pch_string1 != 0)
	{
		if (*pch_string1++ != *pch_string2++)
			return 0;
	}
	return 1;
};

/*
 * check the file type of the current file
 */
unsigned char sdio_check_filetype(FILE_T *filehandler, char *pch_type)
{
	for (unsigned char ch_count = 0; ch_count < 3; ch_count++)
	{
		if (filehandler->name[8 + ch_count] != *pch_type++)
			return 0;
	}
	return 1;
};

/*
 * Get date in FAT format
 */
unsigned int sdio_get_date(void)
{
	return sys->date;
};

/*
 * Get time in FAT format
 */
unsigned int sdio_get_time(void)
{
	//In FAT the time is only stored with a resolution of 2 seconds. Bit shifting the current time by 1 (dividing by 2) converts to this format.
	return (unsigned int)(sys->time >> 1);
};

/***********************************************************
 * Get fileid of an empty file entry.
 ***********************************************************
 * 
 * The directory cluster of the file/directory has to be 
 * loaded in the buffer!
 * 
 * Argument:	none
 * return:		unsigned long l_empty_id
 * 
 * call-by-value, nargs = none
 * call-by-reference
 ***********************************************************/
void sdio_get_empty_id(void)
{
	//Allocate memory
	unsigned long *l_empty_id = arbiter_malloc(&task_sdio, 2);
	unsigned long *l_entry = l_empty_id + 1;

	//Perform command actions
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read current cluster
		if (sdio_read_cluster(dir, dir->CurrentCluster))
		{
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//Read the sector in which the current file id lays
		if (((*l_empty_id % (SDIO_BLOCKLEN / 32)) == 0) && *l_empty_id)
		{
			// Read next sector when id is in the next secor
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//When next sector is read, check whether the id was found
				if (!arbiter_get_return_value(&task_sdio))
					SD->err = SDIO_ERROR_NO_SUCH_FILEID;

				//Goto next sequence
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
			}
		}
		else
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //Goto next sequence when the right sector was read
		break;

	case SEQUENCE_FINISHED:
		//Read the first byte of the filename
		*l_entry = (unsigned long)sdio_read_byte(dir->buffer, (*l_empty_id % (SDIO_BLOCKLEN / 32)) * 32);

		//0x00 or 0xE5 represent empty entries
		if ((*l_entry == 0) || (*l_entry == 0xE5))
			arbiter_return(&task_sdio, *l_empty_id); //Command is finished
		else
		{
			//If current entry is not empty, try next entry
			*l_empty_id = *l_empty_id + 1;
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Set fileid of an empty file entry.
 ***********************************************************
 * 
 * The directory cluster of the file/directory has to be 
 * loaded in the buffer!
 * 
 * Argument:	unsigned long l_id
 * Return:		none
 * 
 * call-by-value, nargs = 1
 ***********************************************************/
void sdio_set_empty_id(void)
{
	//Allocate memory
	unsigned long *l_count = arbiter_malloc(&task_sdio, 1);

	//Get argument
	unsigned long *l_id = arbiter_get_argument(&task_sdio);

	//Perform command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read the current cluster
		if (sdio_read_cluster(dir, dir->CurrentCluster))
		{
			//Reset counter variable
			*l_count = 0;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//Read the sector in which the current file id lays
		if (*l_count < (*l_id / (SDIO_BLOCKLEN / 32)))
		{
			// Read next sector when id is in the next secor
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//Count when sector was read
				*l_count += 1;

				//When next sector is read, check whether the id was found
				if (!arbiter_get_return_value(&task_sdio))
					SD->err = SDIO_ERROR_NO_SUCH_FILEID;
			}
		}
		else
		{
			//Set the current id as empty entry
			sdio_write_byte(dir->buffer, (*l_id % (SDIO_BLOCKLEN / 32)) * 32, 0xE5);
			//Goto next sequence when the right sector was read
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Write the sector to sd-card and exit the command
		if (sdio_write_current_sector(dir))
			arbiter_return(&task_sdio, 0);
		break;

	default:
		break;
	}
};

/***********************************************************
 * Get the next empty cluster.
 ***********************************************************
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long 	l_empty_cluster
 * 
 * call-by-reference
 ***********************************************************/
void sdio_get_empty_cluster(void)
{
	//Get arguments
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio); //Argument[0];

	//Allocate memory
	unsigned long *l_empty_cluster = arbiter_malloc(&task_sdio, 2);
	unsigned long *l_limit = l_empty_cluster + 1;

	//Perform command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		// Initialize memory
		*l_empty_cluster = 2;

		//Determine the limit of clusters occupied by one FAT according to FAT-type
		if (SD->status & SD_IS_FAT16) //FAT16
			*l_limit = SD->FATSz * (SDIO_BLOCKLEN / 16);
		else //FAT32
			*l_limit = SD->FATSz * (SDIO_BLOCKLEN / 32);

		//Goto next sequence
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//End of FAT reached?
		if (*l_empty_cluster <= *l_limit)
		{
			//End is not reached
			//Read the block with the cluster entry
			if (sdio_read_block(filehandler->buffer, sdio_get_fat_sec(*l_empty_cluster, 1)))
			{
				//Determine the cluster state, if the function returns 0 the cluster is empty
				if (!sdio_read_fat_pos(filehandler->buffer, sdio_get_fat_pos(*l_empty_cluster)))
					arbiter_return(&task_sdio, *l_empty_cluster);
				else
					*l_empty_cluster += 1; //If cluster is occupied try next cluster
			}
		}
		else
			SD->err = SDIO_ERROR_NO_EMPTY_CLUSTER; //End is reached
		break;

	default:
		break;
	}
};

/***********************************************************
 * Get fileid of specific file or directory. 
 ***********************************************************
 * The fileid is automatically saved in the active file handler.
 * The directory cluster of the file/directory has to be loaded
 * in the buffer! The input name string has to include the 
 * filename + file extension!
 * When the file does not exist, SD->err is set 
 * with SDIO_ERROR_NO_SUCH_FILEID.
 * 
 * Argument[0]:	FILE_T*			filehandler
 * Argument[1]:	char* 			ch_namestring
 * return:		unsigned long 	l_fileid
 * 
 * call-by-value, nargs = 2
 ***********************************************************/
void sdio_get_fileid(void)
{
	//allocate memory
	unsigned long *l_fileid = arbiter_malloc(&task_sdio, 1);

	//Get arguments: Filehandler
	FILE_T *filehandler = (FILE_T *)arbiter_get_reference(&task_sdio, 0); //Argument[0];
	//Get arguments: Pointer to filename + extension
	char *ch_namestring = (char *)arbiter_get_reference(&task_sdio, 1); //Argument[1]

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read the start cluster of the file
		if (sdio_read_cluster(dir, dir->StartCluster))
		{
			//Initialize memory
			*l_fileid = 0;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//Set the file id of the read file
		filehandler->id = *l_fileid;
		//Read the sector in which the current file id lays
		if (((*l_fileid % (SDIO_BLOCKLEN / 32)) == 0) && *l_fileid)
		{
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//When next sector was read, check whether a next sector exits
				if (!arbiter_get_return_value(&task_sdio))
					SD->err = SDIO_ERROR_NO_SUCH_FILEID;
				//Goto next sequence
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
			}
		}
		else
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Read filename
		sdio_get_name(filehandler);

		//Compare the entry name to the input string and break if the names match
		if (sdio_strcmp(ch_namestring, filehandler->name))
		{
			//File was found return the file id
			arbiter_return(&task_sdio, *l_fileid);
		}
		else
		{
			//If the names do not match try the next entry
			*l_fileid += 1;

			//Start procedure again
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Check whether a filename is existing or not.
 ***********************************************************
 * 
 * This is a robust function to determine whether a file 
 * exists or not. The other function throw an error when the
 * file does not exist, this functions simply return 0 or 1.
 * 
 * Returns 1 when the filename exists.
 * 
 * Argument[0]:	FILE_T*			filehandler
 * Argument[1]:	char*			ch_namestring
 * Return:		unsigned long	l_filefound
 * 
 * call-by-value, nargs = 2
 ***********************************************************/
void sdio_fsearch(void)
{
	//allocate memory
	unsigned long *l_fileid = arbiter_malloc(&task_sdio, 1);

	//Get arguments: Filehandler
	FILE_T *filehandler = (FILE_T *)arbiter_get_reference(&task_sdio, 0); //Argument[0]
	//Get arguments: Pointer to filename + extension
	char *ch_namestring = (char *)arbiter_get_reference(&task_sdio, 1); //Argument[1]

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read the start cluster of the file
		if (sdio_read_cluster(dir, dir->StartCluster))
		{
			//Initialize memory
			*l_fileid = 0;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//Set current fileid
		filehandler->id = *l_fileid;

		//Read the sector in which the current file id lays
		if (((*l_fileid % (SDIO_BLOCKLEN / 32)) == 0) && *l_fileid)
		{
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//When next sector was read, check whether a next sector exits
				if (arbiter_get_return_value(&task_sdio))
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //Goto next sequence
				else
					arbiter_return(&task_sdio, 0); //File does not exist}
			}
		}
		else
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);

		break;

	case SEQUENCE_FINISHED:
		//Read filename
		sdio_get_name(filehandler);

		//Compare the entry name to the input string and break if the names match
		if (sdio_strcmp(ch_namestring, filehandler->name))
		{
			//exit command
			arbiter_return(&task_sdio, 1);
		}
		else
		{
			//If the names do not match try the next entry
			*l_fileid += 1;

			//Start procedure again
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}

	default:
		break;
	}
};

/***********************************************************
 * Change the current directory within one directory.
 ***********************************************************
 *
 * The filehandler for directories is automatically used.
 * For longer paths the function has to be called repeatedly.
 * Returns 1 when the directory was found and could be opened.
 * 
 * Argument:	char* ch_dirname
 * Return:		unsigned long l_dir_opened
 * 
 * call-by-reference
 ***********************************************************/
void sdio_cd(void)
{
	//Get the argument of the command
	char *ch_dirname = (char *)arbiter_get_argument(&task_sdio);

	//Allocate memory for the name string 3 x 4 Bytes = 12 Bytes
	char *ch_filename = (char *)arbiter_malloc(&task_sdio, 3);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//initialize the name string
		for (unsigned char i = 0; i < 11; i++)
			ch_filename[i] = 0x20; //Space
		ch_filename[11] = 0;	   //String terminator

		//Assemble the filename, the dirname can have up to 8 characters
		for (unsigned char i = 0; i < 8; i++)
		{
			//When dir string is not finished
			if (ch_dirname[i])
				ch_filename[i] = ch_dirname[i];
			else
				break; //Stop when dir string is finished
		}

		//Goto next sequence
		arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_FILEID);
		break;

	case SDIO_SEQUENCE_GET_FILEID:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = (unsigned long)dir,
		args[1] = (unsigned long)ch_filename;

		//Get the file id of the desired directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILEID))
		{
			//Set the new fileid
			dir->id = arbiter_get_return_value(&task_sdio);
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_CMD_GET_FILE);
		}
		break;

	case SDIO_SEQUENCE_GET_FILE:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = dir->id;
		args[1] = (unsigned long)dir;

		//Read the directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILE))
		{
			//Check whether entry is a directory, before accessing the start cluster
			if (!(dir->DirAttr & DIR_ATTR_DIR))
				SD->err = SDIO_ERROR_NOT_A_DIR;
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Read the start cluster
		if (sdio_read_cluster(dir, dir->StartCluster))
			arbiter_return(&task_sdio, 1); //Return and signal that the directory is opened
		break;

	default:
		break;
	}
};

/***********************************************************
 * Open file in the current directory. 
 ***********************************************************
 *
 * The filehandler for the active file is automatically used.
 * The input name string has to include the filename + file extension!
 * Returns 1 when file was found and could be read.
 * 
 * Argument:	char* filename
 * Return:		unsigned long file_opened
 * 
 * call-by-reference
 ***********************************************************/
void sdio_fopen(void)
{
	//Get arguments
	char *ch_filename = (char *)arbiter_get_argument(&task_sdio);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = (unsigned long)file;
		args[1] = (unsigned long)ch_filename;

		//Get the file id of the desired file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILEID))
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_FILE);
		break;

	case SDIO_SEQUENCE_GET_FILE:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = file->id;
		args[1] = (unsigned long)file;

		//Read the entry for the file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILE))
		{
			//Check whether entry is a valid file (not a directory and not a system file), before accessing the file and reading the last sector
			if (file->DirAttr & (DIR_ATTR_DIR | DIR_ATTR_SYS))
				SD->err = SDIO_ERROR_NOT_A_FILE;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_FIRST_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_FIRST_CLUSTER:
		// Read the first cluster of the file
		if (sdio_read_cluster(file, file->StartCluster))
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //Goto next state
		break;

	case SEQUENCE_FINISHED:
		//With first cluster loaded the end-of-file can be determined
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_ENDSECTOR_OF_FILE, file))
			arbiter_return(&task_sdio, 1); //Return and signal that the file is opened
		break;

	default:
		break;
	}
};

/***********************************************************
 * Close the current file in the active filehandler.
 ***********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * call-by-value, nargs = none
 * call-by-reference
 ***********************************************************/
void sdio_fclose(void)
{
	//perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Save file to sd-card
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_WRITE_FILE, file))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Clear the filehandler
		file->id = 0;
		file->CurrentByte = 0;
		file->DirAttr = 0;
		file->DirCluster = 0;
		file->StartCluster = 0;
		file->name[0] = 0;
		file->size = 0;

		//exit the command
		arbiter_return(&task_sdio, 1);
		break;

	default:
		break;
	}
};

/***********************************************************
 * Create a new entry in the current cluster.
 ***********************************************************
 * 
 * Argument[0]:	unsigned long 	l_empty_id
 * Argument[1]: char*			ch_filename
 * Argument[2]:	unsigned long	l_empty_cluster
 * Argument[3]: unsigned char 	ch_attribute
 * Return:		none
 * 
 * call-by-value, nargs = 4
 ***********************************************************/
void sdio_make_entry(void)
{
	//Get Arguments
	unsigned long *l_empty_id = arbiter_get_argument(&task_sdio);	  //Argument[0]
	char *ch_filename = (char *)arbiter_get_reference(&task_sdio, 1); //Argument[1]
	unsigned long *l_empty_cluster = l_empty_id + 2;				  //Argument[2]
	unsigned char *ch_attribute = (unsigned char *)(l_empty_id + 3);  //Argument[3]

	//Allocate memory
	unsigned int *i_address = (unsigned int *)arbiter_malloc(&task_sdio, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Get the address within one sector for the current file id, see sdio_get_file() for more explanation
		*i_address = (unsigned int)(*l_empty_id % (SDIO_BLOCKLEN / 32) * 32);

		/*
	 	* Make entry in directory
	 	*/
		//Reset filename with spaces
		for (unsigned char ch_count = 0; ch_count < 11; ch_count++)
			sdio_write_byte(dir->buffer, *i_address + ch_count, 0x20);

		//Write name + extension
		for (unsigned char ch_count = 0; ch_count < 12; ch_count++)
		{
			if (*ch_filename)
				sdio_write_byte(dir->buffer, *i_address + ch_count, *ch_filename++);
		}

		//Write file attribute
		sdio_write_byte(dir->buffer, *i_address + DIR_ATTR_pos, *ch_attribute);

		//Write time and dates
		sdio_write_int(dir->buffer, *i_address + DIR_CRT_TIME_pos, sdio_get_time());
		sdio_write_int(dir->buffer, *i_address + DIR_WRT_TIME_pos, sdio_get_time());

		sdio_write_int(dir->buffer, *i_address + DIR_CRT_DATE_pos, sdio_get_date());
		sdio_write_int(dir->buffer, *i_address + DIR_WRT_DATE_pos, sdio_get_date());
		sdio_write_int(dir->buffer, *i_address + DIR_ACC_DATE_pos, sdio_get_date());

		//Write first cluster with the number of the allocated empty cluster
		sdio_write_int(dir->buffer, *i_address + DIR_FstClusLO_pos, (*l_empty_cluster & 0xFF)); //FAT16
		if (!(SD->status & SD_IS_FAT16))
			sdio_write_int(dir->buffer, *i_address + DIR_FstClusHI_pos, (*l_empty_cluster >> 16)); //FAT32 uses 32bit cluster addressing

		//Write file size, 0 for empty file. When writing to file, the file size has to be updated! Otherwise OEMs will assume the file is still empty.
		sdio_write_long(dir->buffer, *i_address + DIR_FILESIZE_pos, 0);

		//Goto next sequence
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Write Entry to sd-card
		if (sdio_write_current_sector(dir))
			arbiter_return(&task_sdio, 0); //Exit the command when sector was written
		break;

	default:
		break;
	}
};

/***********************************************************
 * Create new file in current directory.
 ***********************************************************
 *
 * Return 1 when file could be created.
 * 
 * Argument:	char*			ch_filename
 * Return:		unsigned long	l_file_created
 * 
 * call-by-reference
 ***********************************************************/
void sdio_mkfile(void)
{
	//Get arguments
	char *ch_filename = (char *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_empty_cluster = arbiter_malloc(&task_sdio, 1);

	//File is not yet created
	SD->status &= ~SD_FILE_CREATED;

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = (unsigned long)file;
		args[1] = (unsigned long)ch_filename;
		//Check if file already exists
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_FSEARCH))
		{
			//When file does not exist create it
			if (arbiter_get_return_value(&task_sdio))
				arbiter_return(&task_sdio, 0);
			else
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_DIR_START_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_DIR_START_CLUSTER:
		//Read the start cluster of the current directory
		if (sdio_read_cluster(dir, dir->StartCluster))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_EMPTY_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_GET_EMPTY_CLUSTER:
		//Get the next empty cluster
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_GET_EMPTY_CLUSTER, file))
		{
			//Save the id of the empty cluster
			*l_empty_cluster = arbiter_get_return_value(&task_sdio);
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_EMPTY_ID);
		}
		break;

	case SDIO_SEQUENCE_GET_EMPTY_ID:
		//Set the id of the file
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_GET_EMPTY_ID, 0))
		{
			//Save the empty id to the filehandler
			file->id = arbiter_get_return_value(&task_sdio);
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_MAKE_ENTRY);
		}
		break;

	case SDIO_SEQUENCE_MAKE_ENTRY:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 4);
		args[0] = file->id;
		args[1] = (unsigned long)ch_filename;
		args[2] = *l_empty_cluster;
		args[3] = DIR_ATTR_ARCH;
		//Make the entry of the file in the current directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_MAKE_ENTRY))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_CLUSTER:

		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_empty_cluster;
		args[2] = (unsigned long)file->buffer;

		//Set args[1] according to the used file system
		if (SD->status & SD_IS_FAT16) //FAT16
			args[1] = 0xFFFF;
		else //FAT32
			args[1] = 0xFFFFFFFF;

		//Load and set the entry in FAT of new cluster to end of file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_FILE);
		}
		break;

	case SDIO_SEQUENCE_GET_FILE:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = file->id;
		args[1] = (unsigned long)file;
		//Open file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILE))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Read first cluster of file
		if (sdio_read_cluster(file, file->StartCluster))
		{
			//File is created
			SD->status |= SD_FILE_CREATED;

			//When cluster is read return
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Create new directory in current directory.
 ***********************************************************
 * 
 * Returns 1 when directory could be created.
 * 
 * Argument:	char*			ch_dirname
 * Return:		unsigned long	l_dir_created
 * 
 * call-by-reference
 ***********************************************************/
void sdio_mkdir(void)
{
	//get arguments
	char *ch_dirname = (char *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_rootcluster = arbiter_malloc(&task_sdio, 6);
	unsigned long *l_empty_cluster = l_rootcluster + 1;
	char *ch_tempname = (char *)(l_rootcluster + 2);

	//File is not yet created
	SD->status &= ~SD_FILE_CREATED;

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//remember root cluster
		*l_rootcluster = dir->CurrentCluster;

		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = (unsigned long)dir;
		args[1] = (unsigned long)ch_dirname;

		//Check if file already exists
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_FSEARCH))
		{
			//When file does not exist create it
			if (arbiter_get_return_value(&task_sdio))
				arbiter_return(&task_sdio, 0);
			else
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_DIR_START_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_DIR_START_CLUSTER:
		//Read the root cluster again
		if (sdio_read_cluster(dir, *l_rootcluster))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_EMPTY_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_GET_EMPTY_CLUSTER:
		//Get the next empty cluster
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_GET_EMPTY_CLUSTER, dir))
		{
			//Save the id of the empty cluster
			*l_empty_cluster = arbiter_get_return_value(&task_sdio);
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_CMD_GET_EMPTY_ID);
		}
		break;

	case SDIO_SEQUENCE_GET_EMPTY_ID:
		//Set the id of the file
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_GET_EMPTY_ID, 0))
		{
			//Save the empty id to the filehandler
			dir->id = arbiter_get_return_value(&task_sdio);
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_MAKE_ENTRY);
		}
		break;

	case SDIO_SEQUENCE_MAKE_ENTRY:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 4);
		args[0] = dir->id;
		args[1] = (unsigned long)ch_dirname;
		args[2] = *l_empty_cluster;
		args[3] = DIR_ATTR_DIR;

		//Make the entry of the file in the current directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_MAKE_ENTRY))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_empty_cluster;
		args[2] = (unsigned long)dir->buffer;

		//Load and set the entry in FAT of new cluster to end of file
		if (SD->status & SD_IS_FAT16) //FAT16
			args[1] = 0xFFFF;
		else //FAT32
			args[1] = 0xFFFFFFFF;

		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_CLEAR_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_CLEAR_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = *l_empty_cluster;
		args[1] = (unsigned long)dir->buffer;

		//Clear the allocated cluster
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_CLEAR_CLUSTER))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_EMPTY_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_EMPTY_CLUSTER:
		//Read the first cluster of the new directory
		if (sdio_read_cluster(dir, *l_empty_cluster))
		{
			//Create the temporary name string for the dir entries
			ch_tempname[0] = '.';
			for (unsigned char i = 1; i < 11; i++)
				ch_tempname[i] = 0x20;
			ch_tempname[11] = 0x00;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_MAKE_ENTRY_DOT);
		}
		break;

	case SDIO_SEQUENCE_MAKE_ENTRY_DOT:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 4);
		args[0] = 0;
		args[1] = (unsigned long)ch_tempname;
		args[2] = *l_empty_cluster;
		args[3] = DIR_ATTR_DIR | DIR_ATTR_HID | DIR_ATTR_R;

		//Make the entry for the current directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_MAKE_ENTRY))
		{
			//Change the temporary name string for the dir entries
			ch_tempname[1] = '.';

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_MAKE_ENTRY_DOTDOT);
		}
		break;

	case SDIO_SEQUENCE_MAKE_ENTRY_DOTDOT:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 4);
		args[0] = 1;
		args[1] = (unsigned long)ch_tempname;
		args[2] = *l_empty_cluster;
		args[3] = DIR_ATTR_DIR | DIR_ATTR_HID | DIR_ATTR_R;

		//Make the entry for the current directory
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_MAKE_ENTRY))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Read root cluster again
		if (sdio_read_cluster(dir, *l_rootcluster))
		{
			//Directory is created
			SD->status |= SD_FILE_CREATED;

			//Exit the command and return
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Remove file or directory.
 ***********************************************************
 * 
 * The content of a directory isn't checked!
 * Returns 1 when file is removed.
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long	l_file_removed
 * 
 * call-by-reference
 ***********************************************************/
void sdio_rm(void)
{
	//Get argument
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_cluster = arbiter_malloc(&task_sdio, 2);
	unsigned long *l_cluster_next = l_cluster + 1;

	//Perform command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Read the file entry of the file which should be removed
		if (sdio_read_cluster(dir, filehandler->DirCluster))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_FILE);
		}
		break;

	case SDIO_SEQUENCE_GET_FILE:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = filehandler->id;
		args[1] = (unsigned long)filehandler;

		//Get the file data
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_FILE))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_EMPTY_ID);
		}
		break;

	case SDIO_SEQUENCE_SET_EMPTY_ID:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 1);
		args[0] = filehandler->id;

		//Set the entry as empty space
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_EMPTY_ID))
		{
			//Remember the allocated start cluster of the file
			*l_cluster = filehandler->StartCluster;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_FAT_POS);
		}
		break;

	case SDIO_SEQUENCE_READ_FAT_POS:
		//Read the sector with the entry of start cluster of the file
		if (sdio_read_block(filehandler->buffer, sdio_get_fat_sec(*l_cluster, 1)))
		{
			//Determine the FAT entry
			*l_cluster_next = sdio_read_fat_pos(filehandler->buffer, sdio_get_fat_pos(*l_cluster));

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_cluster;
		args[1] = 0;
		args[2] = (unsigned long)filehandler->buffer;

		//Set current cluster as free space
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Determine whether current cluster was end-of-file or whether other clusters are still allocated
		if (SD->status & SD_IS_FAT16)
		{
			if (*l_cluster_next < 0xFFF8)
			{
				//Set next cluster to clear
				*l_cluster = *l_cluster_next;
				//Start sequence again
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_FAT_POS);
			}
			else
				arbiter_return(&task_sdio, 1);
		}
		else
		{
			if (*l_cluster_next < 0xFFFFFFF8)
			{
				//Set next cluster to clear
				*l_cluster = *l_cluster_next;
				//Start sequence again
				arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_FAT_POS);
			}
			else
				arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Set file size of file.
 ***********************************************************
 * 
 * Returns 1 when filesize is set.
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long	l_size_set
 * 
 * call-by-reference
 ***********************************************************/
void sdio_set_filesize(void)
{
	//Get arguments
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_count = arbiter_malloc(&task_sdio, 2);
	unsigned int *i_address = (unsigned int *)(l_count + 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Get the address within one sector for the current file id, see sdio_get_file() for more explanation
		*i_address = (unsigned int)(filehandler->id % (SDIO_BLOCKLEN / 32) * 32);

		//Read root cluster of file
		if (sdio_read_cluster(dir, filehandler->DirCluster))
		{
			//Get the sector in which the file id is located
			*l_count = (filehandler->id / (SDIO_BLOCKLEN / 32));
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER:
		//read the next sector until the sector with the current file entry is read
		if (*l_count)
		{
			//Read next sector of cluster
			if (arbiter_callbyreference(&task_sdio, SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER, dir))
			{
				//Check whether sector was found
				if (!arbiter_get_return_value(&task_sdio))
					SD->err = SDIO_ERROR_NO_SUCH_FILEID;
				else
					*l_count -= 1;
			}
		}
		else
		{
			//Write file size in entry
			sdio_write_long(dir->buffer, *i_address + DIR_FILESIZE_pos, filehandler->size);
			//Goto next state
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Write entry
		if (sdio_write_current_sector(dir))
			arbiter_return(&task_sdio, 1); //Exit command
		break;

	default:
		break;
	}
};

/***********************************************************
 * Set current file as empty.
 ***********************************************************
 * 
 * Returns one when file is cleared
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long	l_file_cleared
 * 
 * call-by-reference
 ***********************************************************/
void sdio_fclear(void)
{
	//Get arguments
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_state_cluster = arbiter_malloc(&task_sdio, 1);
	unsigned long *l_current_cluster = l_state_cluster + 1;
	unsigned long *l_next_cluster = l_state_cluster + 2;

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Set entry in directory
		filehandler->CurrentByte = 0;
		filehandler->size = 1;
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_SET_FILESIZE, filehandler))
		{
			//Check filetype
			if (SD->status & SD_IS_FAT16)
				*l_state_cluster = 0xFFFF; //FAT16
			else
				*l_state_cluster = 0xFFFFFFFF; //FAT32

			//Set the Startcluster as first cluster to be deleted
			*l_current_cluster = filehandler->StartCluster;

			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_NEXT_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_GET_NEXT_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 2);
		args[0] = *l_current_cluster;
		args[1] = (unsigned long)filehandler->buffer;

		//Get the next cluster which is occupied by the file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_NEXT_CLUSTER))
		{
			//Save next cluster
			*l_next_cluster = arbiter_get_return_value(&task_sdio);
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_current_cluster;
		args[1] = *l_state_cluster;
		args[2] = (unsigned long)filehandler->buffer;

		//Set start cluster as end-of-file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			if (SD->status & SD_IS_FAT16)
			{
				//Check whether the file does not occupy any more memory
				if (*l_next_cluster != 0xFFFF)
				{
					//Delete the next occupied cluster
					*l_current_cluster = *l_next_cluster;
					*l_state_cluster = 0;
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_NEXT_CLUSTER);
				}
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //File is deleted
			}
			else
			{
				//Check whether the file does not occupy any more memory
				if (*l_next_cluster != 0xFFFFFFFF)
				{
					//Delete the next occupied cluster
					*l_current_cluster = *l_next_cluster;
					*l_state_cluster = 0;
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_NEXT_CLUSTER);
				}
				else
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED); //File is deleted
			}
		}
		break;

	case SEQUENCE_FINISHED:
		//Read first cluster of file
		if (sdio_read_cluster(filehandler, filehandler->StartCluster))
			arbiter_return(&task_sdio, 1); //Exit command
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write current file content onto sd-card. 
 ***********************************************************
 *
 * If cluster or sector is full, the memory is extended.
 * Filesize has to be current!
 * Returns 1 when the file is written.
 * 
 * Argument: 	FILE_T*			filehandler
 * Return:		unsigned long 	l_file_written
 * 
 * call-by-reference
 ***********************************************************/
void sdio_write_file(void)
{
	//Get arguments
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_empty_cluster = arbiter_malloc(&task_sdio, 3);
	unsigned long *l_current_cluster = l_empty_cluster + 1;
	unsigned long *l_current_sector = l_empty_cluster + 2;

	//Perform command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Save current position in filesystem
		*l_empty_cluster = 0;
		*l_current_cluster = filehandler->CurrentCluster;
		*l_current_sector = filehandler->CurrentSector;

		//Write the current buffer to sd-card
		if (sdio_write_current_sector(filehandler))
		{
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_FILESIZE);
		}
		break;
	case SDIO_SEQUENCE_SET_FILESIZE:
		//Set the filesize in the directory entry of the file to the current size
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_SET_FILESIZE, filehandler))
		{
			//If the current sector is full, check whether the cluster is full and allocate the next cluster if necessary
			if (filehandler->CurrentByte == SDIO_BLOCKLEN)
			{
				//If current sector equals the amount of sectors per cluster
				if (*l_current_sector == SD->SecPerClus)
				{
					//Goto next sequence
					arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_EMPTY_CLUSTER);
				}
				else
				{
					//If cluster is not full, just read the next sector
					filehandler->CurrentSector = *l_current_sector + 1;
					filehandler->CurrentCluster = *l_current_cluster;
					//Set current byte of file to beginning of cluster
					filehandler->CurrentByte = 0;
					//Goto next sequence
					arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
				}
			}
			else
			{
				//If neither the sector or the cluster were full, just read the same sector again
				filehandler->CurrentSector = *l_current_sector;
				filehandler->CurrentCluster = *l_current_cluster;

				//goto next sequence
				arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
			}
		}
		break;

	case SDIO_SEQUENCE_GET_EMPTY_CLUSTER:
		//Search for empty cluster number
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_GET_EMPTY_CLUSTER, filehandler))
		{
			//Get next empty cluster
			*l_empty_cluster = arbiter_get_return_value(&task_sdio);
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_EMPTY_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_EMPTY_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_current_cluster;
		args[1] = *l_empty_cluster;
		args[2] = (unsigned long)filehandler->buffer;

		//Save empty cluster number to old end-of-file cluster
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_SET_CLUSTER);
		}
		break;

	case SDIO_SEQUENCE_SET_CLUSTER:
		//Allocate and set the arguments of the called command
		args = arbiter_allocate_arguments(&task_sdio, 3);
		args[0] = *l_empty_cluster;
		args[2] = (unsigned long)filehandler->buffer;

		//Check filesytem
		if (SD->status & SD_IS_FAT16)
			args[1] = 0xFFFF;
		else
			args[1] = 0xFFFFFFFF;

		//Load and set the entry in FAT of new cluster to end of file
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_SET_CLUSTER))
		{
			//Save the new write position of the file
			filehandler->CurrentSector = 1;
			filehandler->CurrentCluster = *l_empty_cluster;
			//Set current byte of file to beginning of cluster
			filehandler->CurrentByte = 0;
			//Goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}
		break;

	case SEQUENCE_FINISHED:
		//Read the block containing the new end-of-file byte
		if (sdio_read_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster) + filehandler->CurrentSector - 1))
		{
			//Exit the command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Read the end sector of the opened file.
 ***********************************************************
 * 
 * Read the sector of a file which contains the current end
 * of file using the stored filesize. Works only after 
 * sdio_fopen() or when only the first sector of the file 
 * has been read from sd-card!
 * Returns 1 when sector could be read.
 * 
 * Argument:	FILE_T*			filehandler
 * Return:		unsigned long	l_sector_read
 * 
 * call-by-reference
 ***********************************************************/
void sdio_read_end_sector_of_file(void)
{
	//Get argument
	FILE_T *filehandler = (FILE_T *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_sector = arbiter_malloc(&task_sdio, 4);
	unsigned long *l_used_cluster = l_sector + 1;
	unsigned long *l_cluster = l_sector + 2;
	unsigned long *l_count = l_sector + 3;

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Initialize memory
		*l_sector = 0;
		*l_used_cluster = 0;
		*l_cluster = filehandler->StartCluster;

		//Get the sector in which the file is located
		*l_count = ((filehandler->size / SDIO_BLOCKLEN) / SD->SecPerClus);

		//Get next sequence and immediately jump into the sequence -> break is intentionally left out
		arbiter_set_sequence(&task_sdio, SDIO_SEQUENCE_GET_NEXT_CLUSTER);

	case SDIO_SEQUENCE_GET_NEXT_CLUSTER:
		//When sector with file is not yet read
		if (*l_count)
		{
			//Allocate and set the arguments of the called command
			args = arbiter_allocate_arguments(&task_sdio, 2);
			args[0] = *l_cluster;
			args[1] = (unsigned long)filehandler->buffer;

			//Read the next sector of the cluster
			if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_GET_NEXT_CLUSTER))
			{
				//get the next cluster
				*l_cluster = arbiter_get_return_value(&task_sdio);
				//decrease the call counter
				*l_count -= 1;
			}
		}
		else
		{
			//Second get the sector of the end cluster in which the file ends
			*l_sector = ((filehandler->size / SDIO_BLOCKLEN) % SD->SecPerClus);
			//goto next sequence
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		}

	case SEQUENCE_FINISHED:
		//Read the end sector of the file
		if (sdio_read_block(filehandler->buffer, sdio_get_lba(*l_cluster) + *l_sector))
		{
			filehandler->CurrentCluster = *l_cluster;
			filehandler->CurrentSector = *l_sector + 1;

			//Set the current byte pointer to the first empty byte
			filehandler->CurrentByte = (unsigned int)(filehandler->size - ((*l_used_cluster * SD->SecPerClus + *l_sector) * SDIO_BLOCKLEN));

			//Exit the command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write byte to file.
 ***********************************************************
 * 
 * Write byte to currently opened file at current byte position.
 * Returns 1 when byte is written to file.
 * 
 * Argument:	unsigned char	ch_byte
 * Return:		unsigned long	l_byte_written
 * 
 * call-by-value, nargs = 1
 ***********************************************************/
void sdio_byte2file(void)
{
	//Get arguments
	unsigned char *ch_byte = arbiter_get_argument(&task_sdio);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Write data to buffer
		sdio_write_byte(file->buffer, file->CurrentByte, *ch_byte);

		//Update the file properties
		file->CurrentByte++;
		file->size++;

		//If the buffer is full, write data to sd-card
		if (file->CurrentByte == SDIO_BLOCKLEN)
			arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);
		else
			arbiter_return(&task_sdio, 1); //Exit command
		break;

	case SEQUENCE_FINISHED:
		//Buffer was full, so write SD-card
		if (arbiter_callbyreference(&task_sdio, SDIO_CMD_WRITE_FILE, file))
		{
			//File is written, exit command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write integer to file
 ***********************************************************
 *
 * Write integer to currently opened file at current byte position.
 * Returns 1 when integer is written to file.
 * 
 * Argument:	unsigned int	i_data
 * Return:		unsigned long	l_integer_written
 * 
 * call-by-value, nargs = 1
 ***********************************************************/
void sdio_int2file(void)
{
	//Get argument
	unsigned int *i_data = arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_count = arbiter_malloc(&task_sdio, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Get number of bytes to write
		*l_count = 2;
		//Get next sequence and immediately jump into the sequence -> break is intentionally left out
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);

	case SEQUENCE_FINISHED:
		//repeat for every byte
		if (*l_count)
		{
			//Allocate and set the arguments of the called command
			unsigned long *args = arbiter_allocate_arguments(&task_sdio, 1);
			args[0] = ((*i_data) >> ((*l_count - 1) * 8));
			//Write byte
			if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_BYTE2FILE))
				*l_count -= 1; //Decrease byte counter
		}
		else
		{
			//All bytes are sent, exit the command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write 32-bit to the file.
 ***********************************************************
 * 
 * Write long to currently opened file at current byte position.
 * Returns 1 when long is written to file.
 * 
 * Argument:	unsigned long	l_data
 * Return:		unsigned long	l_long_written
 * 
 * call-by-value, nargs = 1
 ***********************************************************/
void sdio_long2file(void)
{
	//Get argument
	unsigned long *l_data = arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_count = arbiter_malloc(&task_sdio, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Get number of bytes to write
		*l_count = 4;
		//Get next sequence and immediately jump into the sequence -> break is intentionally left out
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);

	case SEQUENCE_FINISHED:
		//repeat for every byte
		if (*l_count)
		{
			//Allocate and set the arguments of the called command
			unsigned long *args = arbiter_allocate_arguments(&task_sdio, 1);
			args[0] = ((*l_data) >> ((*l_count - 1) * 8));
			//Write byte
			if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_BYTE2FILE))
				*l_count -= 1; //Decrease byte counter
		}
		else
		{
			//All bytes are sent, exit the command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write a string to the file.
 ***********************************************************
 *
 * Write string to currently opened file at current byte position.
 * Returns 1 when string is written to file.
 * 
 * Argument:	char*			ch_string
 * Return:		unsigned long	l_string_written
 * 
 * call-by-reference
 ***********************************************************/
void sdio_string2file(void)
{
	//Get argument
	char *ch_string = (char *)arbiter_get_argument(&task_sdio);

	//Allocate memory
	unsigned long *l_count = arbiter_malloc(&task_sdio, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Initialize byte counter
		*l_count = 0;
		//Get next sequence and immediately jump into the sequence -> break is intentionally left out
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);

	case SEQUENCE_FINISHED:
		//repeat for every byte
		if (ch_string[*l_count])
		{
			//Allocate and set the arguments of the called command
			char *args = arbiter_allocate_arguments(&task_sdio, 1);
			args[0] = ch_string[*l_count];

			//Write byte
			if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_BYTE2FILE))
				*l_count += 1; //goto next byte in string
		}
		else
		{
			//All bytes are sent, exit the command
			arbiter_return(&task_sdio, 1);
		}
		break;

	default:
		break;
	}
};

/***********************************************************
 * Write a number to the file.
 ***********************************************************
 *
 * Write number as string to currently opened file at 
 * current byte position.
 * Returns 1 when number is written to file.
 * 
 * Argument[0]:	unsigned long	l_number
 * Argument[1]:	unsigned long	l_decimal_places
 * Return:		unsigned long	l_number_written
 * 
 * call-by-value, nargs = 2;
 ***********************************************************/
void sdio_num2file(void)
{
	//Get arguments
	unsigned long *l_number = arbiter_get_argument(&task_sdio);
	unsigned long *l_decimal_places = l_number + 1;

	//Allocate memory
	unsigned long *l_temp_value = arbiter_malloc(&task_sdio, 2);
	unsigned char *ch_character = (unsigned char *)(l_temp_value + 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_sdio))
	{
	case SEQUENCE_ENTRY:
		//Calculate the current string character of the number
		*l_temp_value = *l_number;
		for (unsigned char i = 1; i < *l_decimal_places; i++)
			*l_temp_value /= 10;
		*ch_character = (unsigned char)(*l_temp_value % 10) + 48;

		//Get next sequence and immediately jump into the sequence -> break is intentionally left out
		arbiter_set_sequence(&task_sdio, SEQUENCE_FINISHED);

	case SEQUENCE_FINISHED:
		//Write the current character of the number
		args = arbiter_allocate_arguments(&task_sdio, 1);
		args[0] = (unsigned long)*ch_character;

		//Call the command to write the byte
		if (arbiter_callbyvalue(&task_sdio, SDIO_CMD_BYTE2FILE))
		{
			//Check whether all decimal places are written
			if (*l_decimal_places)
			{
				//Write next descimal place
				*l_decimal_places -= 1;
				arbiter_set_sequence(&task_sdio, SEQUENCE_ENTRY);
			}
			else
				arbiter_return(&task_sdio, 1); //Exit the command
		}
		break;

	default:
		break;
	}
};