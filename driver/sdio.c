/*
 * sdio.c
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */
#include "sdio.h"
#include "Variables.h"
#include "ipc.h"

SYS_T* sys;
SDIO_T* SD;
FILE_T* dir; //Is the global filehandler for directories

/*
 * initialize sdio and sd-card
 */
void init_sdio(void)
{
	//Activate Clocks
	RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;		//Sdio adapter
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;		//DMA2
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
	GPIOC->MODER	|= GPIO_MODER_MODER12_1 | GPIO_MODER_MODER11_1 | GPIO_MODER_MODER10_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER8_1;
	//GPIOC->OTYPER	|= GPIO_OTYPER_OT_11 | GPIO_OTYPER_OT_10 | GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_8;
	GPIOC->PUPDR	|= GPIO_PUPDR_PUPDR11_0 | GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR8_0;
	GPIOC->AFR[1]	|= (12<<16) | (12<<12) | (12<<8) | (12<<4) | (12<<0);

	GPIOD->MODER	|= GPIO_MODER_MODER2_1;
	//GPIOD->OTYPER	= GPIO_OTYPER_OT_2;
	GPIOD->PUPDR	= GPIO_PUPDR_PUPDR2_0;
	GPIOD->AFR[0]	|= (12<<8);

	/*
	 * Configure sdio
	 */
	sdio_set_clock(400000);										//set sdio clock
	SDIO->CLKCR |= SDIO_CLKCR_CLKEN;							//enable clock
	SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0;	//Enable SD_CLK
	SDIO->DTIMER = 0xFFFFF;										//Data timeout during data transfer, includes the program time of the sd-card memory -> adjust for slow cards
	SDIO->DLEN	=	SDIO_BLOCKLEN;								//Numbers of bytes per block


	//Register Memory
	SD = ipc_memory_register(38,did_SDIO);
	dir = ipc_memory_register(556, did_DIRFILE);
	dir->name[11] = 0;	//End of string
	sys = ipc_memory_get(did_SYS);

	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

	/*
	 * Configure DMA2 Stream 6 Channel 4:
	 * -Memorysize:	32bit	incremented
	 * -Peripheral:	32bit	fixed
	 * -Peripheral flow control
	 */
#define DMA_CONFIG_SDIO (DMA_SxCR_CHSEL_2 | DMA_SxCR_PBURST_0 | DMA_SxCR_PSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_PFCTRL)

	DMA2_Stream6->CR = DMA_CONFIG_SDIO;
	//DMA2_Stream3->NDTR = 128;							//128*4bytes to transmit
	DMA2_Stream6->PAR = (unsigned long)&SDIO->FIFO;		//Peripheral address
	DMA2_Stream6->M0AR = (unsigned long)&dir->buffer[0];	//Buffer start address
	DMA2_Stream6->FCR = DMA_SxFCR_DMDIS;// | DMA_SxFCR_FTH_1 | DMA_SxFCR_FTH_0;

	wait_ms(1);			//Wait for powerup of card


	/*
	 * Begin card identification
	 */
	//send reset
	sdio_send_cmd(CMD0,0);
	wait_ms(1);			//wait for reset of card

	//send CMD8 to specify supply voltage
	SD->state = 0;
	sdio_set_wait(ON);
	SD->err = 0;
	SD->response = sdio_send_cmd_short(CMD8,CMD8_VOLTAGE_0 | CHECK_PATTERN);
	if(SD->response)	//Check response
	{
		if((SD->response & 0b11111111) == CHECK_PATTERN)	//Valid response, card is detected
			SD->state |= SD_CARD_DETECTED;
	}

	//If card is detected
	//DONE Optionally switch to 4-wire mode
	//TODO Read Card capacity

	if(SD->state & SD_CARD_DETECTED)
	{
		//send ACMD41 until initialization is complete
		do
		{
			SD->response = sdio_send_cmd_short(CMD55,0);
			if(SD->response & R1_APP_CMD)
				SD->response = sdio_send_cmd_short_no_crc(ACMD41,ACMD41_HCS | ACMD41_XPC | OCR_3_0V); //Set Voltage Range, Host Capacity Status and Power Control
		}while(!(SD->response & ACMD41_BUSY));

		//Check whether SDHC-Card or not
		if(SD->response  & ACMD41_CCS)
			SD->state |= SD_SDHC;

		//Get the RCA of the card
		if(sdio_send_cmd_long(CMD2,0))
			SD->RCA = (unsigned int)(sdio_send_cmd_short(CMD3,0)>>16);

		//Set Blocklen to 512 bytes
		//SD->response = sdio_send_cmd_short(CMD16,SDIO_BLOCKLEN);

		//Set bus mode to 4 bit

		sdio_select_card();
		if(SD->state & SD_CARD_SELECTED)
		{
			//send command for bus mode
			SD->response = sdio_send_cmd_short(CMD55,(SD->RCA<<16));
			if(SD->response & R1_APP_CMD)
				SD->response = sdio_send_cmd_short(ACMD6,0b10);
			//If bus mode is available, switch to this mode
			if(!(SD->response & R1_ERROR))
				SDIO->CLKCR |= SDIO_CLKCR_WIDBUS_0;
		}

		wait_ms(1);
		//Increase clock speed to 4 MHz
		sdio_set_clock(4000000);

		//initialize the filesystem
		sdio_init_filesystem();

		//Get card name
		sdio_read_root();
		sdio_get_file(dir, 0);
		for(unsigned char count = 0; count<8; count++)
			SD->CardName[count] = dir->name[count];
	}
};

/*
 * set the clock speed to the sd card
 */
void sdio_set_clock(unsigned long l_clock)
{
	//Enable Clock and set to desired speed [400kHz : 25 MHz] clock inactive when bus inactive
	if(l_clock<400000)
		l_clock = 400000;
	else if(l_clock > 25000000)
		l_clock = 25000000;

	//Save current CLKCR
	unsigned long config = SDIO->CLKCR;

	//Write new CLKDIV
	SDIO->CLKCR = (config & (127<<8)) | ((SDIOCLK/l_clock)-2);
}

/*
 * send command to card without expected response
 */
void sdio_send_cmd(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | (ch_cmd & 0b111111);
	while(!(SDIO->STA & SDIO_STA_CMDSENT));		//Wait until command is sent
	SDIO->ICR = SDIO_ICR_CMDSENTC;
};

/*
 * send command to card with expected short response
 */
unsigned long sdio_send_cmd_short(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT)));		//Wait until command response is received or timeout

	//If command timeout occurs set error and clear flags
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		SD->err = SD_ERROR_TRANSFER_ERROR;
		return 0;
	}

	//If no error return response and clear flags
	SDIO->ICR = SDIO_ICR_CMDRENDC;
	return SDIO->RESP1;
};

/*
 * send command to card with expected short response and no CRC ckeck
 */
unsigned long sdio_send_cmd_short_no_crc(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & (SDIO_STA_CCRCFAIL | SDIO_STA_CTIMEOUT)));		//Wait until command response is received, CCRCFAIL indicated a received command response with failed CRC

	//If command timeout occurs set error and clear flags
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		SD->err = SD_ERROR_TRANSFER_ERROR;
		return 0;
	}
	//If no error return response and clear flags
	SDIO->ICR = SDIO_ICR_CCRCFAILC;
	return SDIO->RESP1;
};

/*
 * send command to card with expected long response. The long response isn't returned!
 */
unsigned long sdio_send_cmd_long(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT)));		//Wait until command response is received or timeout

	//If command timeout occurs set error and clear flags
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		SD->err = SD_ERROR_TRANSFER_ERROR;
		return 0;
	}
	//If no error return first register of response and clear flags
	SDIO->ICR = SDIO_ICR_CMDRENDC;
	return SDIO->RESP1;
};

/*
 * Select card for data transfer
 * This library only supports one sd-card at a time!
 */
void sdio_select_card(void)
{
	//First check whether card is already selected
	if(!(SD->state & SD_CARD_SELECTED))
	{
		SD->response = sdio_send_cmd_short(CMD7,(SD->RCA<<16)); 	//Select Card
		if(!(SD->response & R1_ERROR))								//Check for Error
			SD->state |= SD_CARD_SELECTED;
	}
};
/*
 * Set card inactive for save removal of card
 */
void sdio_set_inactive(void)
{
	//got to inactive state
	sdio_send_cmd(CMD15,(SD->RCA<<16)); 	//Send command
	//Delete flags for seleted card
	SD->state &= ~(SD_CARD_SELECTED | SD_CARD_DETECTED);
}
/*
 * Read block data from card at specific block address. Data is stored in buffer specified at function call.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 */
void sdio_read_block(unsigned long* databuffer, unsigned long l_block_address)
{
	if(!(SD->state & SD_SDHC))				//If SDSC card, byte addressing is used
		l_block_address *= SDIO_BLOCKLEN;
	SD->response = sdio_send_cmd_short(CMD17,l_block_address);

	sdio_dma_receive(databuffer);
	SDIO->DCTRL = (0b1001<<4) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
	while(!(SDIO->STA & (SDIO_STA_DBCKEND | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)));	//Wait for transfer to finish

	//Set error if transfer timeout occurred
	if(SDIO->STA & SDIO_STA_DTIMEOUT)
	{
		SD->err = SD_ERROR_TRANSFER_ERROR;
		SDIO->ICR = SDIO_STA_DTIMEOUT;
	}
	//Clear other pending flags
	SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;
};

/*
 * Write block data to card at specific block address. Data is read from buffer specified at function call.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 */
void sdio_write_block(unsigned long* databuffer, unsigned long l_block_address)
{
	if(!(SD->state & SD_SDHC))				//If SDSC card, byte addressing is used
		l_block_address *= SDIO_BLOCKLEN;

	//Clear pending interrupt flags
	if(SDIO->STA)
		SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;

	//Send start address and check whether card is ready for data
	if(sdio_send_cmd_short(CMD24,l_block_address) & R1_READY_4_DATA)
	{
		sdio_dma_transmit(databuffer);
		SDIO->DCTRL = (0b1001<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

		//If wait for transfer finish is activated
		if(SD->state & SD_WAIT_FOR_TRANSMIT)
		{
			while((SDIO->STA & SDIO_STA_TXACT) && !(SDIO->STA & (SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT)));	//Wait for transfer to finish

			//Set error if transfer timeout occurred
			if(SDIO->STA & SDIO_STA_DTIMEOUT)
			{
				SD->err = SD_ERROR_TRANSFER_ERROR;
				SDIO->ICR = SDIO_STA_DTIMEOUT;
			}
			//Clear other pending flags
			SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;
		}
	}
	else
		SD->state = SD_ERROR_TRANSFER_ERROR;
};

/*
 * Enable the wait for finishing a transmit
 */
void sdio_set_wait(unsigned char ch_state)
{
	if(ch_state == ON)
		SD->state |= SD_WAIT_FOR_TRANSMIT;
	else
		SD->state &= ~SD_WAIT_FOR_TRANSMIT;
}

/*
 * Enable the DMA for data receive. Data is stored in buffer specified at function call.
 */
void sdio_dma_receive(unsigned long* databuffer)
{
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	if(!(DMA2_Stream6->CR & DMA_SxCR_EN))		//check if transfer is complete
	{
		DMA2_Stream6->M0AR = (unsigned long)databuffer;		//Buffer start address
		DMA2_Stream6->CR = DMA_CONFIG_SDIO | DMA_SxCR_EN;
	}
};

/*
 * Enable the DMA for data transmit. Data is read from buffer specified at function call.
 */
void sdio_dma_transmit(unsigned long* databuffer)
{
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	if(!(DMA2_Stream6->CR & DMA_SxCR_EN))		//check if transfer is complete
	{
		DMA2_Stream6->M0AR = (unsigned long)databuffer;		//Buffer start address
		DMA2_Stream6->CR = DMA_CONFIG_SDIO | DMA_SxCR_DIR_0 | DMA_SxCR_EN; //DMA Config + Dir: from memory to peripheral
	}
};

/*
 * Read a byte from buffer with the corresponding location in the sd-card memory
 */
unsigned char sdio_read_byte(unsigned long* databuffer, unsigned int i_address)
{
	unsigned char ch_address = (unsigned char)((i_address/4));					//Get address of buffer
	unsigned char ch_byte = (unsigned char)(i_address%4);						//Get number of byte in buffer

	return (unsigned char)( (databuffer[ch_address] >> (ch_byte*8)) & 0xFF);	//Write in buffer
};

/*
 * write a byte to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_byte(unsigned long* databuffer, unsigned int i_address, unsigned char ch_data)
{
	unsigned char ch_address = (unsigned char)((i_address/4));				//Get adress of buffer
	unsigned char ch_byte = (unsigned char)(i_address%4);					//Get number of byte in buffer

	databuffer[ch_address] &= ~( ((unsigned long) 0xFF) << (ch_byte*8) );	//Delete old byte
	databuffer[ch_address] |= ( ((unsigned long) ch_data) << (ch_byte*8) );	//Write new byte
};

/*
 * Read a int from buffer with the corresponding location in the sd-card memory
 */
unsigned int sdio_read_int(unsigned long* databuffer, unsigned int i_address)
{
	unsigned int i_data = 0;
	i_data = (sdio_read_byte(databuffer, i_address)<<0);
	i_data |= (sdio_read_byte(databuffer, i_address+1)<<8);
	return i_data;
};

/*
 * Write a int to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_int(unsigned long* databuffer, unsigned int i_address, unsigned int i_data)
{
	sdio_write_byte(databuffer, i_address,(unsigned char)(i_data&0xFF));
	sdio_write_byte(databuffer, i_address+1,(unsigned char)((i_data>>8)&0xFF));
};

/*
 * Read a word from buffer with the corresponding location in the sd-card memory
 */
unsigned long sdio_read_long(unsigned long* databuffer, unsigned int i_address)
{
	unsigned long l_data = 0;
	/*
	for(unsigned char ch_count = 0; ch_count<4; ch_count++)
		l_data |= (sdio_read_byte(i_adress+ch_count)<<(8*ch_count));
	 */
	l_data = (sdio_read_byte(databuffer, i_address)<<0);
	l_data |= (sdio_read_byte(databuffer, i_address+1)<<8);
	l_data |= (sdio_read_byte(databuffer, i_address+2)<<16);
	l_data |= (sdio_read_byte(databuffer, i_address+3)<<24);

	return l_data;
};

/*
 * Write a word to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_long(unsigned long* databuffer, unsigned int i_address, unsigned long l_data)
{
	sdio_write_byte(databuffer, i_address,(unsigned char)(l_data&0xFF));
	sdio_write_byte(databuffer, i_address+1,(unsigned char)((l_data>>8)&0xFF));
	sdio_write_byte(databuffer, i_address+2,(unsigned char)((l_data>>16)&0xFF));
	sdio_write_byte(databuffer, i_address+3,(unsigned char)((l_data>>24)&0xFF));
};

/*
 * initialize filehandler
 */

FILE_T* sdio_register_handler(unsigned char did)
{
	// Register memory
	FILE_T* temp = ipc_memory_register(556, did);
	// Set important values
	temp->name[11] = 0;			//End of name string
	temp->CurrentCluster = 0;
	temp->CurrentSector = 0;
	temp->id = 0;

	return temp;
}
/*
 * Initialize file system.
 * Gets the type and begin of the file system
 */
void sdio_init_filesystem(void)
{
	unsigned char ch_part_type = 0;
	unsigned long l_temp = 0,l_LBABegin = 0, l_RootDirSectors = 0, l_TotSec = 0, l_DataSec = 0, l_CountofCluster = 0;

	sdio_read_block(dir->buffer,0);
	if(sdio_read_int(dir->buffer,MBR_MAGIC_NUMBER_pos) == 0xAA55)	//Check filesystem for corruption
	{
		ch_part_type = sdio_read_byte(dir->buffer, MBR_PART1_TYPE_pos);				//Save partition type
		l_temp = sdio_read_long(dir->buffer, MBR_PART1_LBA_BEGIN_pos);				//Read beginn address of filesystem
		sdio_read_block(dir->buffer, l_temp);										//read partition table

		if(ch_part_type == 0xEE)										//EFI filesystem
		{
			l_temp = sdio_read_long(dir->buffer, EFI_TABLE_LBA_BEGIN_pos);
			sdio_read_block(dir->buffer, l_temp);									//Read EFI partition table
			l_temp = sdio_read_long(dir->buffer, EFI_PART_LBA_BEGIN_pos);			//Get begin of file system
		}
		sdio_read_block(dir->buffer, l_temp);										//Read BPB
		//Check for errors and read filesystem variables
		if((sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos) == SDIO_BLOCKLEN) && (sdio_read_byte(dir->buffer, BPB_NUM_FAT_pos) == 2))
		{
			//Save the LBA begin address
			l_LBABegin = l_temp;
			/************Determine the FAT-type according to Microsoft specifications****************************************************************************/
			//Determine the count of sectors occupied by the root directory
			l_RootDirSectors = ( (sdio_read_int(dir->buffer, BPB_ROOT_ENT_CNT_pos) * 32) + (sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos) - 1)) / sdio_read_int(dir->buffer, BPB_BYTES_PER_SEC_pos);

			//Determine the count of sectors in the data region of the volume
			if(sdio_read_int(dir->buffer, BPB_FAT_SIZE_16_pos))
				SD->FATSz = sdio_read_int(dir->buffer, BPB_FAT_SIZE_16_pos);
			else
				SD->FATSz = sdio_read_long(dir->buffer, BPB_FAT_SIZE_32_pos);

			if(sdio_read_int(dir->buffer, BPB_TOT_SEC_16_pos))
				l_TotSec = sdio_read_int(dir->buffer, BPB_TOT_SEC_16_pos);
			else
				l_TotSec = sdio_read_long(dir->buffer, BPB_TOT_SEC_32_pos);

			l_DataSec = l_TotSec - (sdio_read_int(dir->buffer, BPB_RES_SEC_pos) + (sdio_read_byte(dir->buffer, BPB_NUM_FAT_pos) * SD->FATSz) + l_RootDirSectors);

			//determine the count of clusters as
			SD->SecPerClus = sdio_read_byte(dir->buffer, BPB_SEC_PER_CLUS_pos);
			l_CountofCluster = l_DataSec / SD->SecPerClus;

			if(l_CountofCluster < 4085)
			{
				/* Volume is FAT12 => not supported! */
				SD->err = SD_ERROR_WRONG_FAT;
			}
			else if(l_CountofCluster < 65525)
			{
				/* Volume is FAT16 */
				SD->state |= SD_IS_FAT16;
			}
			else
			{
				/* Volume is FAT32 */
			}
			/********Get the Fat begin address ********************************************************************/
			SD->LBAFATBegin = l_LBABegin + sdio_read_int(dir->buffer, BPB_RES_SEC_pos);

			/********Get sector number of root directory **********************************************************/
			SD->FirstDataSecNum = SD->LBAFATBegin + (2 * SD->FATSz) + l_RootDirSectors;

			if(!(SD->state & SD_IS_FAT16))
				SD->RootDirClus = sdio_read_long(dir->buffer, BPB_ROOT_DIR_CLUS_pos);
			else
				SD->RootDirClus = 0;

			SD->err = 0;
		}
		else
			SD->err = SD_ERROR_WRONG_FILESYSTEM;
	}
};

/*
 * Compute the LBA begin address of a specific cluster
 * Cluster numbering begins at 2.
 */
unsigned long sdio_get_lba(unsigned long l_cluster)
{
	if(l_cluster >=2 )
		return SD->FirstDataSecNum + (l_cluster -2) * (unsigned long)SD->SecPerClus;
	else	//if cluster number is zero, only the root directory of FAT16 can have this value
		return SD->LBAFATBegin + (2 * SD->FATSz);		//Root dir of FAT16
};

/*
 * Get the FAT sector for a specific cluster
 */
unsigned long sdio_get_fat_sec(unsigned long l_cluster, unsigned char ch_FATNum)
{
	unsigned long l_FATOffset = 0;

	if(SD->state & SD_IS_FAT16)
		l_FATOffset = l_cluster * 2;
	else
		l_FATOffset = l_cluster * 4;

	return SD->LBAFATBegin + (l_FATOffset / SDIO_BLOCKLEN) + (SD->FATSz * (ch_FATNum-1));
};

/*
 * Get the byte position of a cluster withing a FAT sector
 */
unsigned long sdio_get_fat_pos(unsigned long l_cluster)
{
	unsigned long l_FATOffset = 0;

	if(SD->state & SD_IS_FAT16)
		l_FATOffset = l_cluster * 2;
	else
		l_FATOffset = l_cluster * 4;

	return l_FATOffset % SDIO_BLOCKLEN;
};

/*
 * Read the FAT entry of the loaded Sector and return the content
 */
unsigned long sdio_read_fat_pos(unsigned long* databuffer,unsigned long l_pos)
{
	if(SD->state & SD_IS_FAT16)
		return (unsigned long)sdio_read_int(databuffer, l_pos);
	else
		return sdio_read_long(databuffer, l_pos);
};

/*
 * Write the FAT entry of the loaded Sector
 */
void sdio_write_fat_pos(unsigned long* databuffer, unsigned long l_pos, unsigned long l_data)
{
	if(SD->state & SD_IS_FAT16)
		sdio_write_int(databuffer, (unsigned int)l_pos,(unsigned int)l_data);
	else
		sdio_write_long(databuffer, (unsigned int)l_pos,l_data);
};

/*
 * Get the next cluster of a current cluster, reading the FAT
 */
unsigned long sdio_get_next_cluster(unsigned long* databuffer, unsigned long l_currentcluster)
{
	//Read the sector with the entry of the current cluster
	sdio_read_block(databuffer, sdio_get_fat_sec(l_currentcluster,1));

	//Determine the FAT entry
	return sdio_read_fat_pos(databuffer, sdio_get_fat_pos(l_currentcluster));
};

/*
 * Set the state of a cluster, writing the FAT
 */
void sdio_set_cluster(unsigned long* databuffer, unsigned long l_cluster, unsigned long l_state)
{
	//Get the LBA address of the cluster
	unsigned long l_lba = sdio_get_fat_sec(l_cluster,1);
	//Read the sector with the entry of the current cluster
	sdio_read_block(databuffer, l_lba);

	//Determine the FAT entry
	sdio_write_fat_pos(databuffer, sdio_get_fat_pos(l_cluster),l_state);

	//Write FAT to sd-card
	sdio_write_block(databuffer, l_lba);

	//Repeat for 2nd FAT
	l_lba = sdio_get_fat_sec(l_cluster,2);
	sdio_read_block(databuffer, l_lba);
	sdio_write_fat_pos(databuffer, sdio_get_fat_pos(l_cluster),l_state);
	sdio_write_block(databuffer, l_lba);
};

/*
 * Write cluster with zeros to empty space
 */
void sdio_clear_cluster(unsigned long* databuffer, unsigned long l_cluster)
{
	//Get LBA of cluster
	l_cluster = sdio_get_lba(l_cluster);
	//Clear buffer
	for(unsigned int i_count = 0; i_count < (SDIO_BLOCKLEN/4); i_count++)
		databuffer[i_count] = 0;

	//Clear Sectors until all sectors of one cluster a zero
	for(unsigned int i_count = 0; i_count < SD->SecPerClus; i_count++)
		sdio_write_block(databuffer, l_cluster+ (unsigned long)i_count);
};

/*
 * Read root directory
 */
unsigned char sdio_read_root(void)
{
	if(SD->state & SD_IS_FAT16)	//FAT16 uses a specific region for the root directory
	{
		sdio_read_block(dir->buffer, SD->LBAFATBegin + (2 * SD->FATSz));
		dir->CurrentCluster = 0;
		dir->CurrentSector = 1;
		dir->DirCluster = 0;
		dir->StartCluster = 0;
	}
	else
	{
		sdio_read_cluster(dir, SD->RootDirClus);
		dir->DirCluster = SD->RootDirClus;
		dir->StartCluster = SD->RootDirClus;
	}

	//Return 0 if error occurred
	if(SD->err)
		return 0;
	//Return 1 if read was successful
	return 1;
};

/*
 * Read the first sector of a specific cluster from sd-card
 */
void sdio_read_cluster(FILE_T* filehandler, unsigned long l_cluster)
{
	//Get the LBA address of cluster and the read the corresponding first sector
	sdio_read_block(&filehandler->buffer[0], sdio_get_lba(l_cluster));
	//Set the current sector and cluster
	filehandler->CurrentCluster = l_cluster;
	filehandler->CurrentSector = 1;
};

/*
 * Write the current sector of the current cluster to sd-card
 */
void sdio_write_current_sector(FILE_T* filehandler)
{
	//Write sector to LBA sector address. The current sector is decremented by 1, because the sector count starts counting at 1.
	sdio_write_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster)+(filehandler->CurrentSector-1));
};

/*
 * Read the next sectors of a file until the end of file is reached
 */
unsigned char sdio_read_next_sector_of_cluster(FILE_T* filehandler)
{
	unsigned long l_FATEntry = 0;

	//If the current sector is not the last sector of the current cluster, the next sector of the cluster is read
	if(filehandler->CurrentSector != SD->SecPerClus)
	{
		sdio_read_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster)+filehandler->CurrentSector++);
		return 1;
	}
	/*
	 * If the current sector is the last sector of the cluster, the next cluster is determined using the FAT.
	 * If the FAT gives an unexpected result (End-of-File, Bad-Sector or empty cluster) the transmission is aborted.
	 */
	else
	{
		//Get the entry of the FAT table
		l_FATEntry = sdio_get_next_cluster(filehandler->buffer, filehandler->CurrentCluster);

		if(SD->state & SD_IS_FAT16)					//If FAT16
		{
			if(l_FATEntry == 0xFFFF)				//End of file
				return 0;
			else if(l_FATEntry >= 0xFFF8)			//Bad Sector
			{
				SD->err = SD_ERROR_BAD_SECTOR;
				return 0;
			}
			else if(l_FATEntry == 0x0000)			//Cluster empty
			{
				SD->err = SD_ERROR_FAT_CORRUPTED;
				return 0;
			}
			else									//Read the next cluster
			{
				sdio_read_cluster(filehandler, l_FATEntry);
				return 1;
			}
		}
		else										//If FAT32
		{
			if(l_FATEntry == 0xFFFFFFFF)			//End of file
				return 0;
			else if(l_FATEntry >= 0xFFFFFFF8)		//Bad Sector
			{
				SD->err = SD_ERROR_BAD_SECTOR;
				return 0;
			}
			else if(l_FATEntry == 0x00000000)		//Cluster Empty
			{
				SD->err = SD_ERROR_FAT_CORRUPTED;
				return 0;
			}
			else									//Read the next cluster
			{
				sdio_read_cluster(filehandler, l_FATEntry);
				return 1;
			}
		}
	}
};

/*
 * Read the name of a file or directory. The sector of the file id has to be loaded.
 */
void sdio_get_name(FILE_T* filehandler)
{
	unsigned long l_id = filehandler->id % (SDIO_BLOCKLEN/32);		//one sector can contain 16 files or directories
	//Read the name string
	for(unsigned char ch_count = 0; ch_count<11; ch_count++)
		filehandler->name[ch_count] = sdio_read_byte(dir->buffer, l_id*32 + ch_count);
};

/*
 * Compare two strings. The first string sets the compare length
 */
unsigned char sdio_strcmp(char* pch_string1, char* pch_string2)
{
	while(*pch_string1 != 0)
	{
		if(*pch_string1++ != *pch_string2++)
			return 0;
	}
	return 1;
};

/*
 * check the file type of the current file
 */
unsigned char sdio_check_filetype(FILE_T* filehandler, char* pch_type)
{
	for(unsigned char ch_count = 0; ch_count<3; ch_count++)
	{
		if(filehandler->name[8+ch_count] != *pch_type++)
			return 0;
	}
	return 1;
};

/*
 * Read a file/directory with fileid
 */
void sdio_get_file(FILE_T* filehandler, unsigned long l_fileid)
{
	//Read start cluster of directory
	sdio_read_cluster(dir, dir->StartCluster);

	//Get the cluster sector which corresponds to the given file id (one sector can contain SDIO_BLOCKLEN/32 entries, one entry is 32bytes)
	for(unsigned long l_count = 0; l_count<(l_fileid/(SDIO_BLOCKLEN/32)); l_count++)
	{
		if(!sdio_read_next_sector_of_cluster(dir))
			SD->err = SD_ERROR_NO_SUCH_FILEID;
	}
	//Read the entry of the file
	if(!SD->err)
	{
		filehandler->id = l_fileid;																					//File id

		/*
		 * The fileid is a consecutive number over all sectors of the directory. To get the specific entry within one sector,
		 * the remainder of the entries per sector has to be used to access the information in the sd-buffer.
		 */
		l_fileid %= (SDIO_BLOCKLEN/32);
		sdio_get_name(filehandler);																										//Name
		filehandler->DirAttr = sdio_read_byte(dir->buffer, l_fileid*32 + DIR_ATTR_pos);											//Attribute Byte
		filehandler->DirCluster = dir->CurrentCluster;																			//Cluster in which entry can be found
		filehandler->StartCluster = (unsigned long)sdio_read_int(dir->buffer, l_fileid*32 + DIR_FstClusLO_pos);					//Cluster in which the file starts (FAT16)
		if(!(SD->state & SD_IS_FAT16))
			filehandler->StartCluster |= ( ((unsigned long)sdio_read_int(dir->buffer, l_fileid*32 + DIR_FstClusHI_pos)) << 16);	//Cluster in which the file starts (FAT32)
		filehandler->size = sdio_read_long(dir->buffer, l_fileid*32 + DIR_FILESIZE_pos);
	}
}

/*
 * Get fileid of an empty file entry.
 * The directory cluster of the file/directory has to be loaded in the buffer!
 */
unsigned long sdio_get_empty_id(void)
{
	unsigned long l_id = 0;
	char ch_entry = 0;

	sdio_read_cluster(dir, dir->CurrentCluster);
	//Read the entries of the directory until all sectors are read
	while(!SD->err)
	{
		//Read the sector in which the current file id lays
		if(((l_id%(SDIO_BLOCKLEN/32)) == 0) && l_id)
		{
			if(!sdio_read_next_sector_of_cluster(dir))
				SD->err = SD_ERROR_NO_SUCH_FILEID;
		}

		//Read the first byte of the filename
		ch_entry = sdio_read_byte(dir->buffer, (l_id%(SDIO_BLOCKLEN/32))*32);

		//0x00 or 0xE5 represent empty entries
		if((ch_entry == 0) || (ch_entry == 0xE5))
			return l_id;

		//If current entry is not empty, try next entry
		l_id++;
	}

	SD->err = SD_ERROR_NO_EMPTY_ENTRY;
	return 0;
};

/*
 * Set fileid of an empty file entry.
 * The directory cluster of the file/directory has to be loaded in the buffer!
 */
void sdio_set_empty_id(unsigned long l_id)
{
	sdio_read_cluster(dir, dir->CurrentCluster);

	//Read the sector in which the current file id lays
	for(unsigned long l_count = 0; l_count<(l_id/(SDIO_BLOCKLEN/32)); l_count++)
	{
		if(!sdio_read_next_sector_of_cluster(dir))
			SD->err = SD_ERROR_NO_SUCH_FILEID;
	}

	//Set the current id as empty entry
	sdio_write_byte(dir->buffer, (l_id%(SDIO_BLOCKLEN/32))*32,0xE5);

	//Write the sector to sd-card
	sdio_write_current_sector(dir);
};

/*
 * Get the next empty cluster
 */
unsigned long sdio_get_empty_cluster(FILE_T* filehandler)
{
	unsigned long l_cluster = 2;
	unsigned long l_limit = 0;

	//Determine the limit of clusters occupied by one FAT according to FAT-type
	if(SD->state & SD_IS_FAT16)						//FAT16
		l_limit = SD->FATSz * (SDIO_BLOCKLEN/16);
	else											//FAT32
		l_limit = SD->FATSz * (SDIO_BLOCKLEN/32);

	//Read FAT until end is reached
	while(l_cluster <= l_limit)
	{
		//Read the block with the cluster entry
		sdio_read_block(filehandler->buffer, sdio_get_fat_sec(l_cluster,1));

		//Determine the cluster state, if the function returns 0 the cluster is empty
		if(!sdio_read_fat_pos(filehandler->buffer, sdio_get_fat_pos(l_cluster)))
			return l_cluster;

		//If cluster is occupied try next cluster
		l_cluster++;
	}
	SD->err = SD_ERROR_NO_EMPTY_CLUSTER;
	return 0;
};

/*
 * Get fileid of specific file or directory.
 * The directory cluster of the file/directory has to be loaded in the buffer!
 */
unsigned char sdio_get_fileid(FILE_T* filehandler, char* pch_name, char* pch_extension)
{
	unsigned long l_id = 0;

	//Temporary name string, pre-filled with spaces and end-of-string identifier (0x00)
	char pch_in[12] = { 0x20,  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

	//Add the filename and extension to one string
	while(pch_name[l_id])
	{
		pch_in[l_id]=pch_name[l_id];
		l_id++;
		if(l_id > 7)
			break;
	}
	l_id = 0;
	while(pch_extension[l_id])
	{
		pch_in[l_id+8]=pch_extension[l_id];
		l_id++;
		if(l_id > 2)
			break;
	}
	l_id = 0;

	sdio_read_cluster(dir,dir->StartCluster);

	//Read entries until the end of the directory is reched
	while(!SD->err)
	{
		filehandler->id = l_id;
		//Read the sector in which the current file id lays
		if(((l_id%(SDIO_BLOCKLEN/32)) == 0) && l_id)
		{
			if(!sdio_read_next_sector_of_cluster(dir))
				SD->err = SD_ERROR_NO_SUCH_FILEID;
		}

		//If the file id exists, read the name of the id
		if(!SD->err)
			sdio_get_name(filehandler);

		//Compare the entry name to the input string and break if the names match
		if(sdio_strcmp(pch_in, filehandler->name))
			break;

		//If the names do not match try the next entry
		l_id++;
	}

	//An error at this point means that there is no such file in the directory
	if(SD->err)
	{
		SD->err = SD_ERROR_NO_SUCH_FILE;
		filehandler->id = 0;
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
	return (unsigned int)(sys->time>>1);
};

/*
 * Change the current directory within one directory.
 * For longer paths the function has to be called repeatedly.
 */
unsigned char sdio_cd(char* pch_dirname)
{
	//Get the file id of the desired directory
	sdio_get_fileid(dir, pch_dirname, " ");

	//If directory exists read the entry
	if(!SD->err)
		sdio_get_file(dir, dir->id);

	//If entry could be read, read the start cluster of the directory
	if(!SD->err)
	{
		//Check whether entry is a directory, before accessing the start cluster
		if(!(dir->DirAttr & DIR_ATTR_DIR))
			SD->err = SD_ERROR_NOT_A_DIR;
		else
		{
			sdio_read_cluster(dir, dir->StartCluster);
			return 1;
		}
	}
	return 0;
}

/*
 * Open file in current directory
 */
unsigned char sdio_fopen(FILE_T* filehandler, char* pch_name, char* pch_extension)
{
	//Get the file id of the desired file
	sdio_get_fileid(filehandler, pch_name, pch_extension);

	//If file exists read the entry
	if(!SD->err)
		sdio_get_file(filehandler, filehandler->id);

	//If entry could be read, read the start cluster of the file
	if(!SD->err)
	{
		//Check whether entry is a valid file (not a directory and not a system file), before accessing the file and reading the last sector
		if(filehandler->DirAttr & (DIR_ATTR_DIR | DIR_ATTR_SYS))
			SD->err = SD_ERROR_NOT_A_FILE;
		else
		{
			//Read first cluster of file
			sdio_read_cluster(filehandler, filehandler->StartCluster);
			//With first cluster loaded the end-of-file can be determined
			sdio_read_end_sector_of_file(filehandler);
			return 1;
		}
	}
	return 0;
};

/*
 * Close current file
 */
void sdio_fclose(FILE_T* filehandler)
{
	//Save file to sd-card
	sdio_write_file(filehandler);

	//Clear the filehandler
	filehandler->CurrentByte = 0;
	filehandler->id = 0;
	filehandler->DirAttr = 0;
	filehandler->DirCluster = 0;
	filehandler->StartCluster = 0;
	filehandler->name[0] = 0;
	filehandler->size = 0;
};

/*
 * Create a new entry in the current cluster
 */
void sdio_make_entry(unsigned long l_emptyid, char* pch_name, char* pch_filetype, unsigned long l_emptycluster, unsigned char ch_attribute)
{
	//Get the address within one sector for the current file id, see sdio_get_file() for more explanation
	unsigned int i_address = (unsigned int)(l_emptyid%(SDIO_BLOCKLEN/32)*32);

	/*
	 * Make entry in directory
	 */
	//Reset filename with spaces
	for(unsigned char ch_count = 0; ch_count<11; ch_count++)
		sdio_write_byte(dir->buffer, i_address+ch_count, 0x20);

	//Write name
	for(unsigned char ch_count = 0; ch_count<8; ch_count++)
	{
		if(*pch_name)
			sdio_write_byte(dir->buffer, i_address+ch_count, *pch_name++);
	}

	//Write file extension
	for(unsigned char ch_count = 0; ch_count<3; ch_count++)
	{
		if(*pch_filetype)
			sdio_write_byte(dir->buffer, i_address+8+ch_count, *pch_filetype++);
	}

	//Write file attribute
	sdio_write_byte(dir->buffer, i_address + DIR_ATTR_pos, ch_attribute);

	//Write time and dates
	sdio_write_int(dir->buffer, i_address+DIR_CRT_TIME_pos,sdio_get_time());
	sdio_write_int(dir->buffer, i_address+DIR_WRT_TIME_pos,sdio_get_time());

	sdio_write_int(dir->buffer, i_address+DIR_CRT_DATE_pos,sdio_get_date());
	sdio_write_int(dir->buffer, i_address+DIR_WRT_DATE_pos,sdio_get_date());
	sdio_write_int(dir->buffer, i_address+DIR_ACC_DATE_pos,sdio_get_date());

	//Write first cluster with the number of the allocated empty cluster
	sdio_write_int(dir->buffer, i_address + DIR_FstClusLO_pos, (l_emptycluster&0xFF));		//FAT16
	if(!(SD->state & SD_IS_FAT16))
		sdio_write_int(dir->buffer, i_address + DIR_FstClusHI_pos, (l_emptycluster>>16));	//FAT32 uses 32bit cluster addressing

	//Write file size, 0 for empty file. When writing to file, the file size has to be updated! Otherwise OEMs will assume the file is still empty.
	sdio_write_long(dir->buffer, i_address+DIR_FILESIZE_pos,0);

	//Write Entry to sd-card
	sdio_write_current_sector(dir);
}
/*
 * Create new file in current directory
 */
unsigned char sdio_mkfile(FILE_T* filehandler, char* pch_name, char* pch_filetype)
{
	//Check if file already exists
	sdio_get_fileid(filehandler, pch_name, pch_filetype);

	//Create file if it does not exist
	if(SD->err == SD_ERROR_NO_SUCH_FILE)
	{
		sdio_read_cluster(dir, dir->StartCluster);
		SD->err = 0;
		//Get empty space
		unsigned long l_emptycluster = sdio_get_empty_cluster(filehandler);
		filehandler->id = sdio_get_empty_id();

		//Make entry in directory
		sdio_make_entry(filehandler->id, pch_name, pch_filetype, l_emptycluster, DIR_ATTR_ARCH);

		//Load and set the entry in FAT of new cluster to end of file
		if(SD->state & SD_IS_FAT16)							//FAT16
			sdio_set_cluster(filehandler->buffer, l_emptycluster, 0xFFFF);
		else												//FAT32
			sdio_set_cluster(filehandler->buffer, l_emptycluster, 0xFFFFFFFF);

		//Clear allocated Cluster
		/*
		 * => Not necessary anymore since the filesize i used to determine the end-of-file
		sdio_clear_cluster(l_emptycluster);
		 */
		//Open file and read first cluster of file
		sdio_get_file(filehandler, filehandler->id);
		sdio_read_cluster(filehandler, filehandler->StartCluster);

		//exit function
		return 1;
	}
	else
		SD->err = SD_ERROR_FILE_ALLREADY_EXISTS;
	return 0;
};

/*
 * Create new directory in current directory
 */
unsigned char sdio_mkdir(char* pch_name)
{
	//remember root cluster
	unsigned long l_rootcluster = dir->CurrentCluster;

	//Check if file already exists
	sdio_get_fileid(dir, pch_name, " ");

	//Create file if it does not exist
	if(SD->err == SD_ERROR_NO_SUCH_FILE)
	{
		sdio_read_cluster(dir, l_rootcluster);
		SD->err = 0;
		//Get empty space
		unsigned long l_emptycluster = sdio_get_empty_cluster(dir);
		dir->id = sdio_get_empty_id();
		/*
		 * Make entry in directory
		 */
		sdio_make_entry(dir->id, pch_name, " ", l_emptycluster, DIR_ATTR_DIR);

		//Load and set the entry in FAT of new cluster to end of file
		if(SD->state & SD_IS_FAT16)
			sdio_set_cluster(dir->buffer, l_emptycluster, 0xFFFF);
		else
			sdio_set_cluster(dir->buffer, l_emptycluster, 0xFFFFFFFF);

		//Clear the allocated cluster
		sdio_clear_cluster(dir->buffer, l_emptycluster);

		sdio_read_cluster(dir, l_emptycluster);

		//Make the first two entries in cluster
		sdio_make_entry(0, ".", " ", l_emptycluster,DIR_ATTR_DIR | DIR_ATTR_HID | DIR_ATTR_R);
		sdio_make_entry(1, "..", " ", l_rootcluster,DIR_ATTR_DIR | DIR_ATTR_HID | DIR_ATTR_R);

		//Read root cluster again
		sdio_read_cluster(dir, l_rootcluster);
		return 1;
	}
	else
		SD->err = SD_ERROR_FILE_ALLREADY_EXISTS;
	return 0;
};

/*
 * Remove file or directory.
 * The content of a directory isn't checked!
 */
void sdio_rm(FILE_T* filehandler)
{
	unsigned long l_cluster = 0, l_cluster_next = 0;

	//Read the file entry of the file which should be removed
	if(!SD->err)
	{
		sdio_read_cluster(dir, filehandler->DirCluster);
		sdio_get_file(filehandler, filehandler->id);
	}

	//If file exists remove it.
	if(!SD->err)
	{
		//Set the entry as empty space
		sdio_set_empty_id(filehandler->id);

		//Remember the allocated start cluster of the file
		l_cluster = filehandler->StartCluster;

		//Set all cluster of file as empty space
		do
		{
			//Read the sector with the entry of start cluster of the file
			sdio_read_block(filehandler->buffer, sdio_get_fat_sec(l_cluster,1));

			//Determine the FAT entry
			l_cluster_next = sdio_read_fat_pos(filehandler->buffer, sdio_get_fat_pos(l_cluster));

			//Set current cluster as free space
			sdio_set_cluster(filehandler->buffer, l_cluster, 0);

			//Determine whether current cluster was end-of-file or whether other clusters are still allocated
			if(SD->state & SD_IS_FAT16)
			{
				if(l_cluster_next < 0xFFF8)
					l_cluster = l_cluster_next;
				else
					l_cluster = 0;
			}
			else
			{
				if(l_cluster_next < 0xFFFFFFF8)
					l_cluster = l_cluster_next;
				else
					l_cluster = 0;
			}
		}while(l_cluster && !SD->err);
	}
};
/*
 * Set file size of file
 */
void sdio_set_filesize(FILE_T* filehandler)
{
	//Get the address within one sector for the current file id, see sdio_get_file() for more explanation
	unsigned int i_address = (unsigned int)(filehandler->id%(SDIO_BLOCKLEN/32)*32);

	//Read root cluster of file
	sdio_read_cluster(dir, filehandler->DirCluster);

	//Read sector with entry
	for(unsigned long l_count = 0; l_count<(filehandler->id/(SDIO_BLOCKLEN/32)); l_count++)
	{
		if(!sdio_read_next_sector_of_cluster(dir))
			SD->err = SD_ERROR_NO_SUCH_FILEID;
	}

	//Write file size in entry
	sdio_write_long(dir->buffer, i_address+DIR_FILESIZE_pos,filehandler->size);

	//Write entry
	sdio_write_current_sector(dir);
};

/*
 * Set current file as empty
 */
void sdio_fclear(FILE_T* filehandler)
{
	//Set entry in directory
	filehandler->CurrentByte = 0;
	filehandler->size = 1;
	sdio_set_filesize(filehandler);

	//Clear FAT
	unsigned long l_nextcluster = sdio_get_next_cluster(filehandler->buffer, filehandler->StartCluster);
	if(SD->state & SD_IS_FAT16)						//If FAT16
	{
		//Set start cluster as end-of-file
		sdio_set_cluster(filehandler->buffer, filehandler->StartCluster, 0xFFFF);

		while(l_nextcluster != 0xFFFF)				//While not end-of-file
		{
			//Set current cluster empty
			sdio_set_cluster(filehandler->buffer, l_nextcluster, 0);

			//Read next cluster
			l_nextcluster = sdio_get_next_cluster(filehandler->buffer, l_nextcluster);
		}

	}
	else											//If FAT32
	{
		//Set start cluster as end-of-file
		sdio_set_cluster(filehandler->buffer, filehandler->StartCluster, 0xFFFFFFFF);

		while(l_nextcluster != 0xFFFFFFFF)			//While not end-of-file
		{
			//Set current cluster empty
			sdio_set_cluster(filehandler->buffer, l_nextcluster, 0);

			//Read next cluster
			l_nextcluster = sdio_get_next_cluster(filehandler->buffer, l_nextcluster);
		}
	}

	//Read first cluster of file
	sdio_read_cluster(filehandler, filehandler->StartCluster);
}

/*
 * Read the sector of a file which contains the current end of file using the stored filesize.
 * Works only after sdio_fopen() or when only the first sector of the file has been read from sd-card!
 */
void sdio_read_end_sector_of_file(FILE_T* filehandler)
{
	unsigned long l_sector = 0, l_usedcluster = 0, l_cluster = filehandler->StartCluster;
	//First get the cluster in which the end sector of the file is located
	for(unsigned long l_count = 0; l_count<((filehandler->size/SDIO_BLOCKLEN)/SD->SecPerClus); l_count++)
	{
		l_cluster = sdio_get_next_cluster(filehandler->buffer, l_cluster);
		l_usedcluster++;
	}
	//Second get the sector of the end cluster in which the file ends
	l_sector = ( (filehandler->size / SDIO_BLOCKLEN) % SD->SecPerClus );

	//Read the end sector of the file
	sdio_read_block(filehandler->buffer, sdio_get_lba(l_cluster)+l_sector);
	filehandler->CurrentCluster = l_cluster;
	filehandler->CurrentSector = l_sector+1;

	//Set the current byte pointer to the first empty byte
	filehandler->CurrentByte = (unsigned int) (filehandler->size - ( ( l_usedcluster*SD->SecPerClus + l_sector) * SDIO_BLOCKLEN ));
};

/*
 * Write current file content onto sd-card. If cluster or sector is full, the memory is extended.
 * Filesize has to be current!
 */
void sdio_write_file(FILE_T* filehandler)
{
	//Save current position in filesystem
	unsigned long l_emptycluster = 0, l_currentcluster = filehandler->CurrentCluster, l_currentsector = filehandler->CurrentSector;

	//Write the current buffer to sd-card
	sdio_write_current_sector(filehandler);

	//Set the filesize in the directory entry of the file to the current size
	sdio_set_filesize(filehandler);

	//If the current sector is full, check whether the cluster is full and allocate the next cluster if necessary
	if(filehandler->CurrentByte == SDIO_BLOCKLEN)
	{
		//If current sector equals the amount of sectors per cluster
		if( l_currentsector == SD->SecPerClus )
		{
			//Search for empty cluster number
			l_emptycluster = sdio_get_empty_cluster(filehandler);

			//Save empty cluster number to old end-of-file cluster
			sdio_set_cluster(filehandler->buffer, l_currentcluster, l_emptycluster);

			//Load and set the entry in FAT of new cluster to end of file
			if(SD->state & SD_IS_FAT16)							//FAT16
				sdio_set_cluster(filehandler->buffer, l_emptycluster, 0xFFFF);
			else												//FAT32
				sdio_set_cluster(filehandler->buffer, l_emptycluster, 0xFFFFFFFF);

			//Save the new write position of the file
			filehandler->CurrentSector = 1;
			filehandler->CurrentCluster = l_emptycluster;
		}
		//If cluster is not full, just read the next sector
		else
		{
			filehandler->CurrentSector = l_currentsector + 1;
			filehandler->CurrentCluster = l_currentcluster;
		}
		//Set current byte of file to beginning of cluster
		filehandler->CurrentByte = 0;

	}
	//If neither the sector or the cluster were full, just read the same sector again
	else
	{
		filehandler->CurrentSector = l_currentsector;
		filehandler->CurrentCluster = l_currentcluster;
	}
	//Read the block containing the new end-of-file byte
	sdio_read_block(filehandler->buffer, sdio_get_lba(filehandler->CurrentCluster) + filehandler->CurrentSector - 1 );
};

/*
 * Write byte to sd-buffer at current byte position after file is opened
 */
void sdio_byte2file(FILE_T* filehandler, unsigned char ch_byte)
{
	//Write data to buffer
	sdio_write_byte(filehandler->buffer, filehandler->CurrentByte, ch_byte);

	//Update the file properties
	filehandler->CurrentByte++;
	filehandler->size++;

	//If the buffer is full, write data to sd-card
	if(filehandler->CurrentByte == SDIO_BLOCKLEN)
		sdio_write_file(filehandler);
};

/*
 * Write integer to sd-buffer at current byte position after file is opened
 */
void sdio_int2file(FILE_T* filehandler, unsigned int i_data)
{
	//Write first 8 bytes of data
	sdio_byte2file(filehandler, (i_data>>8) && 0xFFFF);

	//Write second 8 bytes of data
	sdio_byte2file(filehandler, (i_data>>0) && 0xFFFF);
};

/*
 * Write 32bit to sd-buffer at current byte position after file is opened
 */
void sdio_long2file(FILE_T* filehandler, unsigned long l_data)
{
	//Write first 8 bytes of data
	sdio_byte2file(filehandler, (l_data>>24) && 0xFFFF);

	//Write second 8 bytes of data
	sdio_byte2file(filehandler, (l_data>>16) && 0xFFFF);

	//Write third 8 bytes of data
	sdio_byte2file(filehandler, (l_data>>8) && 0xFFFF);

	//Write forth 8 bytes of data
	sdio_byte2file(filehandler, (l_data>>0) && 0xFFFF);
};

/*
 * Write string to sd-buffer at current byte position after file is opened
 */
void sdio_string2file(FILE_T* filehandler, char* ch_string)
{
	//While the end of the string is not reched
	while(*ch_string)
	{
		//Write the content of string
		sdio_byte2file(filehandler, *ch_string);

		//Get the next character of the string
		ch_string++;
	}
};

/*
 * Write number as string to file
 */
void sdio_num2file(FILE_T* filehandler, unsigned long l_number, unsigned char ch_predecimal)
{
	//String for number
	char ch_number[ch_predecimal+1];
	ch_number[ch_predecimal] = 0;

	//Compute the string content
	for(unsigned char ch_count = 0; ch_count<ch_predecimal;ch_count++)
	{
		ch_number[ch_predecimal - ch_count -1] = (unsigned char)(l_number%10)+48;
		l_number /=10;
	}

	//Write to file
	for(unsigned char ch_count = 0; ch_count<ch_predecimal;ch_count++)
		sdio_byte2file(filehandler, ch_number[ch_count]);
}
