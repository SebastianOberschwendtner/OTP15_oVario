/*
 * sdio.c
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */
#include "sdio.h"
#include "Variables.h"

SYS_T* sys;
SDIO_T* SD;
FILE_T* file;

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
	SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0;	//Enable SD_CLK
	SDIO->DTIMER = 0x404;										//Data timeout during data transfer
	SDIO->DLEN	=	SDIO_BLOCKLEN;								//Numbers of bytes per block


	//Register Memory
	SD = ipc_memory_register(548,did_SDIO);
	file = ipc_memory_register(26, did_FILE);
	file->name[11] = 0;	//End of string
	sys = ipc_memory_get(did_SYS);

	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

	/*
	 * Configure DMA2 Stream 6 Channel 4:
	 * -Memorysize:	32bit	incremented
	 * -Peripheral:	32bit	fixed
	 * -Peripheral flow control
	 */
	DMA2_Stream6->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_PBURST_0 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_PFCTRL;
	//DMA2_Stream3->NDTR = 128;							//128*4bytes to transmit
	DMA2_Stream6->PAR = (unsigned long)&SDIO->FIFO;		//Peripheral address
	DMA2_Stream6->M0AR = (unsigned long)&SD->buffer[0];	//Buffer start address
	DMA2_Stream6->FCR = DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_1 | DMA_SxFCR_FTH_0;

	wait_ms(1);			//Wait for powerup of card

	/*
	 * Begin card identification
	 */
	//send reset
	sdio_send_cmd(CMD0,0);
	wait_ms(1);			//wait for reset of card

	//send CMD8 to specify supply voltage
	SD->state = 0;
	SD->err = 0;
	SD->response = sdio_send_cmd_short(CMD8,CMD8_VOLTAGE_0 | CHECK_PATTERN);
	if(SD->response)	//Check response
	{
		if((SD->response & 0b11111111) == CHECK_PATTERN)	//Valid response, card is detected
			SD->state = SD_CARD_DETECTED;
	}

	//If card is detected
	//TODO Optionally switch to 4-wire mode
	//TODO Read Card capacity

	if(SD->state & SD_CARD_DETECTED)
	{
		//send ACMD41 until initialization is complete
		do
		{
			SD->response = sdio_send_cmd_short(CMD55,0);
			if(SD->response & R1_APP_CMD)
				SD->response = sdio_send_cmd_short_no_crc(ACMD41,OCR_3_0V);
		}while(!(SD->response & ACMD41_BUSY));

		//Check whether SDHC-Card or not
		if(SD->response  & ACMD41_CCS)
			SD->state |= SD_SDHC;

		//Get the RCA of the card
		if(sdio_send_cmd_long(CMD2,0))
			SD->RCA = (unsigned int)(sdio_send_cmd_short(CMD3,0)>>16);

		//Set Blocklen to 512 bytes
		SD->response = sdio_send_cmd_short(CMD16,512);

		/*Set bus mode to 4 bit
		if(!(sdio_send_cmd_short(CMD7,(SD->RCA<<16)) & R1_ERROR))
		{
			SD->response = sdio_send_cmd_short(CMD55,(SD->RCA<<16));
			if(SD->response & R1_APP_CMD)
				SD->response = sdio_send_cmd_short(ACMD6,0b10);
			SD->response = sdio_send_cmd_short(CMD7,(SD->RCA<<16));
		}*/

		wait_ms(1);
		//Read first sector
		sdio_select_card();
		sdio_init_filesystem();
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
	SDIO->CLKCR = SDIO_CLKCR_CLKEN | ((SDIOCLK/l_clock)-2);
}

/*
 * send command to card without espected response
 */
void sdio_send_cmd(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | (ch_cmd & 0b111111);
	while(!(SDIO->STA & SDIO_STA_CMDSENT));		//Wait until command is sent
	SDIO->ICR = SDIO_ICR_CMDSENTC;
};

/*
 * send command to card with espected short response
 */
unsigned long sdio_send_cmd_short(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & SDIO_STA_CMDREND))		//Wait until command response is received
	{
		if(SDIO->STA & SDIO_STA_CTIMEOUT)		//If no response is received
		{
			//TODO Add Error assignment here
			break;
		}
	}
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		return 0;
	}
	else
	{
		SDIO->ICR = SDIO_ICR_CMDRENDC;
		return SDIO->RESP1;
	}
};

/*
 * send command to card with espected short response and no CRC ckeck
 */
unsigned long sdio_send_cmd_short_no_crc(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & SDIO_STA_CCRCFAIL))		//Wait until command response is received, CCRCFAIL indicated a received command response with failed CRC
	{
		if(SDIO->STA & SDIO_STA_CTIMEOUT)		//If no response is received
		{
			//TODO Add Error assignment here
			break;
		}
	}
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		return 0;
	}
	else
	{
		SDIO->ICR = SDIO_ICR_CCRCFAILC;
		return SDIO->RESP1;
	}
};

/*
 * send command to card with espected long response. The long response isn't returned!
 */
unsigned long sdio_send_cmd_long(unsigned char ch_cmd, unsigned long l_arg)
{
	SDIO->ARG = l_arg;
	SDIO->CMD = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0 | (ch_cmd & 0b111111);
	while(!(SDIO->STA & SDIO_STA_CMDREND))		//Wait until command response is received
	{
		if(SDIO->STA & SDIO_STA_CTIMEOUT)		//If no response is received
		{
			//TODO Add Error assignment here
			break;
		}
	}
	if(SDIO->STA & SDIO_STA_CTIMEOUT)
	{
		SDIO->ICR = SDIO_ICR_CTIMEOUTC;
		return 0;
	}
	else
	{
		SDIO->ICR = SDIO_ICR_CMDRENDC;
		return SDIO->RESP1;
	}
};

/*
 * Select card for data transfer
 * This library only supports one sd-card at a time!
 */
void sdio_select_card(void)
{
	if(!(SD->state & SD_CARD_SELECTED))
	{
		SD->response = sdio_send_cmd_short(CMD7,(SD->RCA<<16)); //Select Card
		if(!(SD->response & R1_ERROR))								//Check for Error
			SD->state |= SD_CARD_SELECTED;
	}
};

/*
 * Read block data from card at specific block address.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 */
void sdio_read_block(unsigned long l_block_address)
{
	if(!(SD->state & SD_SDHC))				//If SDSC card, byte addressing is used
		l_block_address *= SDIO_BLOCKLEN;
	SD->response = sdio_send_cmd_short(CMD17,l_block_address);

	sdio_dma_receive();
	SDIO->DCTRL = (0b1001<<4) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
	while(!(SDIO->STA & (SDIO_STA_DBCKEND | SDIO_STA_DTIMEOUT)));	//Wait for transfer to finish
	//TODO Add timeout
	SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;
};

/*
 * Write block data to card at specific block address.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 */
void sdio_write_block(unsigned long l_block_address)
{
	if(!(SD->state & SD_SDHC))				//If SDSC card, byte addressing is used
		l_block_address *= SDIO_BLOCKLEN;
	SD->response = sdio_send_cmd_short(CMD24,l_block_address);

	sdio_dma_transmit();
	SDIO->DCTRL = (0b1001<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
	while(!(SDIO->STA & (SDIO_STA_DBCKEND | SDIO_STA_DTIMEOUT)));	//Wait for transfer to finish
	//TODO Add timeout
	SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;
};

/*
 * Enable the DMA for data receive
 */
void sdio_dma_receive(void)
{
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	if(!(DMA2_Stream6->CR & DMA_SxCR_EN))		//check if transfer is complete
		DMA2_Stream6->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_PBURST_0 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_PFCTRL |
		DMA_SxCR_EN;
};

/*
 * Enable the DMA for data transmit
 */
void sdio_dma_transmit(void)
{
	//Clear DMA interrupts
	DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	if(!(DMA2_Stream6->CR & DMA_SxCR_EN))		//check if transfer is complete
		DMA2_Stream6->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_PBURST_0 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1 | DMA_SxCR_MINC | DMA_SxCR_PFCTRL |
		DMA_SxCR_DIR_0 | DMA_SxCR_EN;
};

/*
 * Read a byte from buffer with the corresponding location in the sd-card memory
 */
unsigned char sdio_read_byte(unsigned int i_address)
{
	unsigned char ch_address = (unsigned char)((i_address/4));	//Get adress of buffer
	unsigned char ch_byte = (unsigned char)(i_address%4);		//Get number of byte in buffer

	return (unsigned char)( (SD->buffer[ch_address] >> (ch_byte*8)) & 0xFF);
};

/*
 * write a byte to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_byte(unsigned int i_address, unsigned char ch_data)
{
	unsigned char ch_address = (unsigned char)((i_address/4));	//Get adress of buffer
	unsigned char ch_byte = (unsigned char)(i_address%4);		//Get number of byte in buffer

	SD->buffer[ch_address] = ( ((unsigned long) ch_data) << (ch_byte*8) );
};

/*
 * Read a int from buffer with the corresponding location in the sd-card memory
 */
unsigned int sdio_read_int(unsigned int i_address)
{
	unsigned int i_data = 0;
	i_data = (sdio_read_byte(i_address)<<0);
	i_data |= (sdio_read_byte(i_address+1)<<8);
	return i_data;
};

/*
 * Write a int to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_int(unsigned int i_address, unsigned int i_data)
{
	sdio_write_byte(i_address,(unsigned char)(i_data&0xFF));
	sdio_write_byte(i_address+1,(unsigned char)((i_data>>8)&0xFF));
};

/*
 * Read a word from buffer with the corresponding location in the sd-card memory
 */
unsigned long sdio_read_long(unsigned int i_address)
{
	unsigned long l_data = 0;
	/*
	for(unsigned char ch_count = 0; ch_count<4; ch_count++)
		l_data |= (sdio_read_byte(i_adress+ch_count)<<(8*ch_count));
	 */
	l_data = (sdio_read_byte(i_address)<<0);
	l_data |= (sdio_read_byte(i_address+1)<<8);
	l_data |= (sdio_read_byte(i_address+2)<<16);
	l_data |= (sdio_read_byte(i_address+3)<<24);

	return l_data;
};

/*
 * Write a word to buffer with the corresponding location in the sd-card memory
 */
void sdio_write_long(unsigned int i_address, unsigned long l_data)
{
	sdio_write_byte(i_address,(unsigned char)(l_data&0xFF));
	sdio_write_byte(i_address+1,(unsigned char)((l_data>>8)&0xFF));
	sdio_write_byte(i_address+2,(unsigned char)((l_data>>16)&0xFF));
	sdio_write_byte(i_address+3,(unsigned char)((l_data>>24)&0xFF));
};

/*
 * Initialize file system.
 * Gets the type and begin of the file system
 */
void sdio_init_filesystem(void)
{
	unsigned char ch_part_type = 0;
	unsigned long l_temp = 0,l_LBABegin = 0, l_RootDirSectors = 0, l_TotSec = 0, l_DataSec = 0, l_CountofCluster = 0;

	sdio_read_block(0);
	if(sdio_read_int(MBR_MAGIC_NUMBER_pos) == 0xAA55)	//Check filesystem for corruption
	{
		ch_part_type = sdio_read_byte(MBR_PART1_TYPE_pos);				//Save partition type
		l_temp = sdio_read_long(MBR_PART1_LBA_BEGIN_pos);
		sdio_read_block(l_temp);		//read partition table

		if(ch_part_type == 0xEE)										//EFI filesystem
		{
			l_temp = sdio_read_long(EFI_TABLE_LBA_BEGIN_pos);
			sdio_read_block(l_temp);	//Read EFI partition table
			l_temp = sdio_read_long(EFI_PART_LBA_BEGIN_pos);	//Get begin of file system
		}
		sdio_read_block(l_temp);	//Read BPB
		//Check for errors and read filesystem variables
		if((sdio_read_int(BPB_BYTES_PER_SEC_pos) == SDIO_BLOCKLEN) && (sdio_read_byte(BPB_NUM_FAT_pos) == 2))
		{
			//Save the LBA begin address
			l_LBABegin = l_temp;
			/************Determine the FAT-type according to Microsoft specifications****************************************************************************/
			//Determine the count of sectors occupied by the root directory
			l_RootDirSectors = ( (sdio_read_int(BPB_ROOT_ENT_CNT_pos) * 32) + (sdio_read_int(BPB_BYTES_PER_SEC_pos) - 1)) / sdio_read_int(BPB_BYTES_PER_SEC_pos);

			//Determine the count of sectors in the data region of the volume
			if(sdio_read_int(BPB_FAT_SIZE_16_pos))
				SD->FATSz = sdio_read_int(BPB_FAT_SIZE_16_pos);
			else
				SD->FATSz = sdio_read_long(BPB_FAT_SIZE_32_pos);

			if(sdio_read_int(BPB_TOT_SEC_16_pos))
				l_TotSec = sdio_read_int(BPB_TOT_SEC_16_pos);
			else
				l_TotSec = sdio_read_long(BPB_TOT_SEC_32_pos);

			l_DataSec = l_TotSec - (sdio_read_int(BPB_RES_SEC_pos) + (sdio_read_byte(BPB_NUM_FAT_pos) * SD->FATSz) + l_RootDirSectors);

			//determine the count of clusters as
			SD->SecPerClus = sdio_read_byte(BPB_SEC_PER_CLUS_pos);
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
			SD->LBAFATBegin = l_LBABegin + sdio_read_int(BPB_RES_SEC_pos);

			/********Get sector number of root directory **********************************************************/
			SD->FirstRootDirSecNum = SD->LBAFATBegin + (sdio_read_byte(BPB_NUM_FAT_pos) * SD->FATSz);

			if(!(SD->state & SD_IS_FAT16))
				SD->FirstRootDirSecNum = sdio_get_lba(sdio_read_long(BPB_ROOT_DIR_CLUS_pos));

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
	return SD->FirstRootDirSecNum + (l_cluster -2) * SD->SecPerClus;
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

	if(ch_FATNum == 1)
		return SD->LBAFATBegin + (l_FATOffset / SDIO_BLOCKLEN);
	else
		return SD->LBAFATBegin + (l_FATOffset / SDIO_BLOCKLEN) + (SD->FATSz * ch_FATNum);
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
unsigned long sdio_read_fat_pos(unsigned long l_pos)
{
	if(SD->state & SD_IS_FAT16)
		return (unsigned long)sdio_read_int(l_pos);
	else
		return sdio_read_long(l_pos);
};

/*
 * Wrote the FAT entry of the loaded Sector
 */
void sdio_write_fat_pos(unsigned long l_pos, unsigned long l_data)
{
	if(SD->state & SD_IS_FAT16)
		sdio_write_int(l_pos,(unsigned int)l_data);
	else
		sdio_write_long(l_pos,l_data);
};

/*
 * Get the next cluster of a current cluster, reading the FAT
 */
unsigned long sdio_get_next_cluster(void)
{
	//Read the sector with the entry of the current cluster
	sdio_read_block(sdio_get_fat_sec(SD->CurrentCluster,1));

	//Determine the FAT entry
	return sdio_read_fat_pos(sdio_get_fat_pos(SD->CurrentCluster));
};

/*
 * Set the state a cluster, writing the FAT
 */
void sdio_set_cluster(unsigned long l_cluster, unsigned long l_state)
{
	//Read the sector with the entry of the current cluster
	sdio_read_block(sdio_get_fat_sec(l_cluster,1));

	//Determine the FAT entry
	sdio_write_fat_pos(sdio_get_fat_pos(l_cluster),l_state);

	//Write FAT to sd-card
	sdio_write_block(sdio_get_fat_sec(l_cluster,1));

	//Repeat for 2nd FAT
	sdio_read_block(sdio_get_fat_sec(l_cluster,2));
	sdio_write_fat_pos(sdio_get_fat_pos(l_cluster),l_state);
	sdio_write_block(sdio_get_fat_sec(l_cluster,2));
};

/*
 * Read the first sector of a specific cluster from sd-card
 */
void sdio_read_cluster(unsigned long l_cluster)
{
	sdio_read_block(sdio_get_lba(l_cluster));
	SD->CurrentCluster = l_cluster;
	SD->CurrentSector = 1;
};

/*
 * Write the current sector of the current cluster to sd-card
 */
void sdio_write_current_sector(void)
{
	//Write sector to LBA sector address. The current sector is decremented by 1, because the sector count starts counting at 1.
	sdio_write_block(sdio_get_lba(SD->CurrentCluster)+(SD->CurrentSector-1));
};

/*
 * Read the next sectors of a file until the end of file is reached
 */
unsigned char sdio_read_next_sector_of_cluster(void)
{
	unsigned long l_FATEntry = 0;

	if(SD->CurrentSector != SD->SecPerClus)
	{
		sdio_read_block(sdio_get_fat_sec(SD->CurrentCluster, 1)+SD->CurrentSector++);
		return 1;
	}
	else
	{
		//Get the entry of the FAT table
		l_FATEntry = sdio_get_next_cluster();

		if(SD->state & SD_IS_FAT16)					//If FAT16
		{
			if(l_FATEntry == 0xFFFF)				//End of file
				return 0;
			else if(l_FATEntry >= 0xFFF8)			//Bad Sector
			{
				SD->err = SD_ERROR_BAD_SECTOR;
				return 0;
			}
			else if(l_FATEntry == 0x0000)			//Sector Empty
			{
				SD->err = SD_ERROR_FAT_CORRUPTED;
				return 0;
			}
			else									//Read the next cluster
			{
				sdio_read_cluster(l_FATEntry);
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
			else if(l_FATEntry == 0x00000000)		//Sector Empty
			{
				SD->err = SD_ERROR_FAT_CORRUPTED;
				return 0;
			}
			else									//Read the next cluster
			{
				sdio_read_cluster(l_FATEntry);
				return 1;
			}
		}
	}
};

/*
 * Read the name of a file or directory
 */
void sdio_get_name(FILE_T* filehandler)
{
	unsigned long l_fileid = filehandler->id % (SDIO_BLOCKLEN/32);		//one sector can contain 16 files or directories
	//Read the name string
	for(unsigned char ch_count = 0; ch_count<11; ch_count++)
		filehandler->name[ch_count] = sdio_read_byte(l_fileid*32 + ch_count);
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
	for(unsigned char ch_count = 0; ch_count<(l_fileid/(SDIO_BLOCKLEN/32)); ch_count++)
	{
		if(!sdio_read_next_sector_of_cluster())
			SD->err = SD_ERROR_NO_SUCH_FILEID;
	}
	if(!SD->err)
	{
		filehandler->id = l_fileid;
		l_fileid %= (SDIO_BLOCKLEN/32);
		sdio_get_name(filehandler);
		filehandler->DirAttr = sdio_read_byte(l_fileid*32 + DIR_ATTR_pos);
		filehandler->DirCluster = SD->CurrentCluster;
		filehandler->StartCluster = sdio_read_int(l_fileid*32 + DIR_FstClusLO_pos);
		if(!(SD->state & SD_IS_FAT16))
			filehandler->StartCluster |= (sdio_read_int(l_fileid*32 + DIR_FstClusHI_pos)<<16);
	}
}

/*
 * Get fileid of specific file or directory.
 * The directory cluster of the file/directory has to be loaded in the buffer!
 */
unsigned long sdio_get_empty_id(void)
{
	unsigned long l_id = 0;
	char ch_entry = 0;

	sdio_read_cluster(SD->CurrentCluster);
	while(!SD->err)
	{
		for(unsigned char ch_count = 0; ch_count<(l_id/(SDIO_BLOCKLEN/32)); ch_count++)
		{
			if(!sdio_read_next_sector_of_cluster())
				SD->err = SD_ERROR_NO_SUCH_FILEID;
		}
		ch_entry = sdio_read_byte((l_id%(SDIO_BLOCKLEN/32))*32);
		if((ch_entry == 0) || (ch_entry == 0xE5))
			return l_id;
		l_id++;
	}

	SD->err = SD_ERROR_NO_EMPTY_ENTRY;
	return 0;
};

/*
 * Get the next empty cluster
 */
unsigned long sdio_get_empty_cluster(void)
{
	unsigned long l_cluster = 2;
	unsigned long l_limit = 0;
	if(SD->state & SD_IS_FAT16)
		l_limit = SD->FATSz * (SDIO_BLOCKLEN/16);
	else
		l_limit = SD->FATSz * (SDIO_BLOCKLEN/32);
	while(l_cluster <= l_limit)
	{
		//Read the block with the cluster entry
		sdio_read_block(sdio_get_fat_sec(l_cluster,1));
		//Determine the cluster state
		if(!sdio_read_fat_pos(sdio_get_fat_pos(l_cluster)))
			return l_cluster;
		l_cluster++;
	}
	SD->err = SD_ERROR_NO_EMPTY_CLUSTER;
	return 0;
}

/*
 * Get fileid of specific file or directory.
 * The directory cluster of the file/directory has to be loaded in the buffer!
 */
void sdio_get_fileid(FILE_T* filehandler, char* pch_name)
{
	unsigned long l_id = 0;

	sdio_read_cluster(SD->CurrentCluster);
	while(!SD->err)
	{
		sdio_get_file(filehandler,l_id);
		if(sdio_strcmp(pch_name, file->name))
			break;
		l_id++;
	}

	if(SD->err)
	{
		SD->err = SD_ERROR_NO_SUCH_FILE;
		filehandler->id = 0;
	}
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
	return (unsigned int)(sys->time>>1);
};

/*
 * Change the current directory within one directory.
 * For longer paths the function has to be called repeatedly.
 */
void sdio_cd(char* pch_dirname)
{
	sdio_get_fileid(file, pch_dirname);
	if(!SD->err)
	{
		if(!(file->DirAttr & DIR_ATTR_DIR))
			SD->err = SD_ERROR_NOT_A_DIR;
		else
			sdio_read_cluster(file->StartCluster);
	}
}

/*
 * Open file in current directory
 */
void sdio_fopen(FILE_T* filehandler, char* pch_name)
{
	sdio_get_fileid(filehandler, pch_name);
	if(!SD->err)
	{
		if(filehandler->DirAttr & (DIR_ATTR_DIR | DIR_ATTR_SYS))
			SD->err = SD_ERROR_NOT_A_FILE;
		else
			sdio_read_cluster(filehandler->StartCluster);
	}
};

/*
 * Create new file in current directory
 */
void sdio_mkfile(char* pch_name, char* pch_filetype)
{
	//Get empty space
	unsigned long l_emptycluster = sdio_get_empty_cluster();
	unsigned long l_emptyid = sdio_get_empty_id();
	unsigned int i_address = (unsigned int)(l_emptyid%(SDIO_BLOCKLEN/32)*32);
	/*
	 * Make entry in directory
	 */
	//Reset filename with spaces
	for(unsigned char ch_count = 0; ch_count<11; ch_count++)
		sdio_write_byte(i_address+ch_count, 0x20);

	//Write name
	for(unsigned char ch_count = 0; ch_count<8; ch_count++)
	{
		if(*pch_name)
			sdio_write_byte(i_address+8+ch_count, *pch_name++);
	}

	//Write file extension
	for(unsigned char ch_count = 0; ch_count<3; ch_count++)
	{
		if(*pch_filetype)
			sdio_write_byte(i_address+ch_count, *pch_filetype++);
	}

	//Write file attribute
	sdio_write_byte(i_address + DIR_ATTR_pos, 0x20);

	//Write time and dates
	sdio_write_int(i_address+DIR_CRT_TIME_pos,sdio_get_time());
	sdio_write_int(i_address+DIR_WRT_TIME_pos,sdio_get_time());

	sdio_write_int(i_address+DIR_CRT_DATE_pos,sdio_get_date());
	sdio_write_int(i_address+DIR_WRT_DATE_pos,sdio_get_date());
	sdio_write_int(i_address+DIR_ACC_DATE_pos,sdio_get_date());

	//Write first cluster
	sdio_write_int(i_address + DIR_FstClusLO_pos, (l_emptycluster&0xFF));
	if(!(SD->state & SD_IS_FAT16))
		sdio_write_int(i_address + DIR_FstClusHI_pos, (l_emptycluster>>16));

	//Write filesize
	sdio_write_long(i_address+DIR_FILESIZE_pos,0);

	//Write Entry to sd-card
	sdio_write_current_sector();

	//Load and set the entry in FAT of new cluster to end of file
	if(SD->state & SD_IS_FAT16)
		sdio_set_cluster(l_emptycluster, 0xFFFF);
	else
		sdio_set_cluster(l_emptycluster, 0xFFFFFFFF);
};
