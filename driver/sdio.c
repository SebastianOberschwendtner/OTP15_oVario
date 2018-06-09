/*
 * sdio.c
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */
#include "sdio.h"
#include "Variables.h"

SDIO_T* SD;

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
	SDIO->DTIMER = 0xFFFF;										//Data timeout during data transfer
	SDIO->DLEN	=	SDIO_BLOCKLEN;								//Numbers of bytes per block


	//Register Memory
	SD = ipc_memory_register(522,did_SDIO);

	//Clear DMA interrupts
	DMA2->HIFCR = DMA_LIFCR_CTCIF3 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

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
	SD->response = sdio_send_cmd_short(CMD8,CMD8_VOLTAGE_0 | CHECK_PATTERN);
	if(SD->response)	//Check response
	{
		if((SD->response & 0b11111111) == CHECK_PATTERN)	//Valid response, card is detected
			SD->state = SD_CARD_DETECTED;
	}

	//If card is detected
	//TODO Optionally switch to 4-wire mode
	//TODO Read Card capacity
	//TODO Set block length to be sure.
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
			SD->RCA = (sdio_send_cmd_short(CMD3,0)>>16);

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
		sdio_read_block(0);
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
 * Read block data from card at specific block address.
 * The addressing uses block number and adapts to SDSC and SDHC cards.
 */
void sdio_read_block(unsigned long l_block_address)
{
	//unsigned char ch_buffer_count = 127;

	if(!(SD->state & SD_SDHC))				//If SDSC card, byte addressing is used
		l_block_address *= SDIO_BLOCKLEN;

	//Select card
	SD->response = sdio_send_cmd_short(CMD7,(SD->RCA<<16));
	if(!(SD->response & R1_ERROR))
	{
		SD->response = sdio_send_cmd_short(CMD16,512);
		SD->response = sdio_send_cmd_short(CMD17,l_block_address);
		sdio_dma_receive();
		SDIO->DCTRL = (0b1001<<4) | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

		//unsigned ch_buffer_count = 127;
		while(!(SDIO->STA & SDIO_STA_DBCKEND));/*
		{
			if(SDIO->STA & SDIO_STA_RXFIFOHF)
			{
				for(unsigned char ch_count = 0; ch_count < 8;ch_count++)
					SD->buffer[ch_buffer_count - ch_count] = SDIO->FIFO;
				ch_buffer_count -= 8;
				SD->response += 8;
			}
		}*/
		SDIO->ICR = SDIO_ICR_DBCKENDC | SDIO_ICR_DATAENDC;

	}
};

/*
 * Enable the DMA for data receive
 */
void sdio_dma_receive(void)
{
	if(!(DMA2_Stream6->CR & DMA_SxCR_EN))		//check if transfer is complete
		DMA2_Stream6->CR |= DMA_SxCR_EN;
};

