/*
 * i2c.c
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#include "i2c.h"

TASK_T task_i2c;							//Struct for arbiter
T_command rxcmd_i2c;						//Command struct for received commands from other tasks via ipc
T_command txcmd_i2c;						//Command struct to transmit commands to other ipc tasks
volatile unsigned int active_address = 0;	//Active i2c address for communication


/*
 * Register everything relevant for IPC
 */
void i2c_register_ipc(void)
{
	//Initialize task struct
	arbiter_clear_task(&task_i2c);
	arbiter_set_command(&task_i2c, I2C_CMD_INIT);

	//Intialize the received command struct
	rxcmd_i2c.did			= did_I2C;
	rxcmd_i2c.cmd 			= 0;
	rxcmd_i2c.data 			= 0;
	rxcmd_i2c.timestamp 	= 0;

	//Commands queue
	ipc_register_queue(10 * sizeof(T_command), did_I2C);
};

/*
 * Get everything relevant for IPC
 */
void i2c_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data

};

/**********************************************************
 * TASK I2C
 **********************************************************
 * The i2c task, which handles the data transfer and 
 * ipc communication.
 * 
 **********************************************************
 * Execution:	Non-interruptable
 * Wait: 		No
 * Halt: 		No
 **********************************************************/
unsigned char i2c_temp[4];
void i2c_task(void)
{
	//Perform task action
	switch (arbiter_get_command(&task_i2c))
	{
	case CMD_IDLE:
		i2c_idle();
		break;

	case I2C_CMD_INIT:
		i2c_init_peripheral();
		arbiter_set_command(&task_i2c, CMD_IDLE);
		break;

	case I2C_CMD_SEND_CHAR:
		i2c_send_char();
		break;

	case I2C_CMD_READ_INT:
		i2c_read_int();
		break;

	case I2C_CMD_READ_24BIT:
		i2c_read_24bit();
		break;

	case I2C_CMD_TEST:
		i2c_test();
		break;

	default:
		break;
	}
	//TODO Add error handling.
};

void i2c_test(void)
{
	switch(arbiter_get_sequence(&task_i2c))
	{
		case SEQUENCE_ENTRY:
			active_address = i2c_addr_MS5611;
			i2c_temp[0] = 0x1E;
			if (arbiter_callbyreference(&task_i2c, I2C_CMD_SEND_CHAR, i2c_temp))
			{
				task_i2c.wait_counter = 1;
				arbiter_set_sequence(&task_i2c, I2C_SEQUENCE_WAIT);
			}
			break;

		case I2C_SEQUENCE_WAIT:
			if (task_i2c.wait_counter)
			{
				task_i2c.wait_counter--;
				break;
			}
			else
				arbiter_set_sequence(&task_i2c, I2C_SEQUENCE_REQUEST);

		case I2C_SEQUENCE_REQUEST:
			active_address = i2c_addr_MS5611;
			i2c_temp[0] = 0xA0;
			if (arbiter_callbyreference(&task_i2c, I2C_CMD_SEND_CHAR, i2c_temp))
				arbiter_set_sequence(&task_i2c, SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			active_address = i2c_addr_MS5611+1;
			if (i2c_receive_nbytes(i2c_temp,2))
				arbiter_return(&task_i2c, 1);
			break;

		default:
			break;
	}
};

/*
 * Initialize necessary peripherals
 */
void i2c_init_peripheral(void)
{
	//Init clocks
	gpio_en(GPIO_B);
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	/*
	 * Set GPIO
	 * PIN	AF				OUTTYPE
	 * PB7: I2C1_SDA (AF4)	Open drain
	 * PB6: I2C1_SCL (AF4)	Open drain
	 */
	GPIOB->MODER |= GPIO_MODER_MODER7_1 | GPIO_MODER_MODER6_1;
	GPIOB->AFR[0] |= (4<<28) | (4<<24);
	GPIOB->OTYPER |= GPIO_OTYPER_OT_7 | GPIO_OTYPER_OT_6;


	//Init I2C1
	I2C1->CR2 	= 42;	//Define APB1 clock speed (42 MHz)
	I2C1->CCR 	= I2C_CCR_FS | (42000000UL/(3*I2C_CLOCK));
	I2C1->TRISE = 14;
	I2C1->CR1 	= I2C_CR1_ACK | I2C_CR1_PE;	//Activate I2C

	//Enable the i2c interrupt
	NVIC_EnableIRQ(I2C1_EV_IRQn);

	//Init dma for transmitting
	/*
	 * Configure DMA1 Stream 6 Channel 1:
	 * -Memorysize:	8bit	incremented
	 * -Peripheral:	8bit	fixed
	 * -Memory-to-peripheral
	 * -Direct mode
	 * -Transfer Complete Interrupt enabled
	 */
#define DMA_CONFIG_I2C_TX (DMA_SxCR_CHSEL_0 | DMA_SxCR_MINC | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE)

	DMA1_Stream6->PAR = (unsigned long)&I2C1->DR;		 	//Peripheral address
	DMA1_Stream6->FCR = DMA_SxFCR_DMDIS;					//Set direct mode
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);						//Enable the global dma interrupt

	//Init dma for receiving
	/*
	 * Configure DMA1 Stream 5 Channel 1:
	 * -Memorysize:	8bit	incremented
	 * -Peripheral:	8bit	fixed
	 * -Peripheral-to-memory
	 * -Direct mode
	 * -Transfer Complete Interrupt enabled
	 */
#define DMA_CONFIG_I2C_RX (DMA_SxCR_CHSEL_0 | DMA_SxCR_MINC | DMA_SxCR_TCIE)

	DMA1_Stream5->PAR = (unsigned long)&I2C1->DR;		 	//Peripheral address
	DMA1_Stream5->FCR = DMA_SxFCR_DMDIS;					//Set direct mode
	NVIC_EnableIRQ(DMA1_Stream5_IRQn);						//Enable the global dma interrupt
};

/**********************************************************
 * Idle command for i2c
 **********************************************************
 * Checks for commands in queue and triggers the 
 * transmission.
 * 
 * Should/Can not be called via the arbiter!
 **********************************************************/
void i2c_idle(void)
{
	//When calling command was not the task itself
	if (rxcmd_i2c.did != did_I2C)
	{
		//Send finished signal
		txcmd_i2c.did = did_I2C;
		txcmd_i2c.cmd = cmd_ipc_signal_finished;
		txcmd_i2c.data = arbiter_get_return_value(&task_i2c);
		ipc_queue_push(&txcmd_i2c, sizeof(T_command), rxcmd_i2c.did); //Signal that command is finished to calling task

		//Reset calling command
		rxcmd_i2c.did = did_I2C;
	}

	//the idle task assumes that all commands are finished, so reset all sequence states
	arbiter_reset_sequence(&task_i2c);

	//Check for new commands in queue
	if (ipc_get_queue_bytes(did_I2C) >= sizeof(T_command))
	{
		//Check the command in the queue and execute it
		if (ipc_queue_get(did_I2C, sizeof(T_command), &rxcmd_i2c))
		{
			switch (rxcmd_i2c.cmd) // switch for command
			{
			case I2C_CMD_SEND_CHAR:
				//Decode i2c address from calling task
				active_address = i2c_decode_did(rxcmd_i2c.did);
				//Call the command
				arbiter_callbyreference(&task_i2c,I2C_CMD_SEND_CHAR,&rxcmd_i2c.data);
				break;

			case I2C_CMD_READ_INT:
				//Decode i2c address from calling task
				active_address = i2c_decode_did(rxcmd_i2c.did);
				//Call the command
				arbiter_callbyreference(&task_i2c,I2C_CMD_READ_INT,&rxcmd_i2c.data);
				break;

			case I2C_CMD_READ_24BIT:
				//Decode i2c address from calling task
				active_address = i2c_decode_did(rxcmd_i2c.did);
				//Call the command
				arbiter_callbyreference(&task_i2c,I2C_CMD_READ_24BIT,&rxcmd_i2c.data);
				break;

			default:
				break;
			}
		}
	}
};

/**********************************************************
 * Send char via i2c
 **********************************************************
 * 
 * Argument:	unsigned char* 	data
 * Return:		unsigned long	l_char_sent
 * 
 * call-by-reference
 **********************************************************/
void i2c_send_char(void)
{
	//Get Arguments
	unsigned char* data = (unsigned char*) arbiter_get_argument(&task_i2c);

	//Perform the command action
	switch (arbiter_get_sequence(&task_i2c))
	{
		case SEQUENCE_ENTRY:
			//Send byte and go next state when finished
			if (i2c_transmit_nbytes(data,1))
				arbiter_set_sequence(&task_i2c, SEQUENCE_FINISHED);
			break;
		
		case SEQUENCE_FINISHED:
			//byte is sent
			arbiter_return(&task_i2c, 1);
			break;
		
		default:
			break;
	}
};

/**********************************************************
 * Read int via i2c
 **********************************************************
 * Reads an int from a specified device register. The
 * return value is an unsigned long with the value of
 * the returned int value. -> Byte swapping from MSB to
 * LSB is performed within the function.
 * 
 * Argument:	unsigned char* 	data
 * Return:		unsigned long	l_value
 * 
 * call-by-reference
 **********************************************************/
void i2c_read_int(void)
{
	//Get arguments
	unsigned char* data = (unsigned char*)arbiter_get_argument(&task_i2c);

	//Allocate memory
	unsigned long* l_value = arbiter_malloc(&task_i2c,2);
	unsigned char* rx_buffer = (unsigned char*)(l_value+1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_i2c))
	{
	case SEQUENCE_ENTRY:
		//Send the register address of the desired to the slave
		if (arbiter_callbyreference(&task_i2c, I2C_CMD_SEND_CHAR, data))
			arbiter_set_sequence(&task_i2c, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Set the read flag in the address
		active_address |= 1;
		//Read the value
		if (i2c_receive_nbytes(rx_buffer, 2))
		{
			*l_value = 0;
			//Perform the byte swap from MSB to LSB
			for (unsigned char i = 0; i < 2; i++)
				*l_value += (rx_buffer[1-i]<<(8*i));
			//Return the received value
			arbiter_return(&task_i2c, *l_value);
		}
		break;

	default:
		break;
	}
};

/**********************************************************
 * Read 24bit via i2c
 **********************************************************
 * Reads 24bits from a specified device register. The
 * return value is an unsigned long with the value of
 * the returned value. -> Byte swapping from MSB to
 * LSB is performed within the function.
 * 
 * Argument:	unsigned char* 	data
 * Return:		unsigned long	l_value
 * 
 * call-by-reference
 **********************************************************/
void i2c_read_24bit(void)
{
	//Get arguments
	unsigned char* data = (unsigned char*)arbiter_get_argument(&task_i2c);

	//Allocate memory
	unsigned long* l_value = arbiter_malloc(&task_i2c,2);
	unsigned char* rx_buffer = (unsigned char*)(l_value+1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_i2c))
	{
	case SEQUENCE_ENTRY:
		//Send the register address of the desired to the slave
		if (arbiter_callbyreference(&task_i2c, I2C_CMD_SEND_CHAR, data))
			arbiter_set_sequence(&task_i2c, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Set the read flag in the address
		active_address |= 1;
		//Read the value
		if (i2c_receive_nbytes(rx_buffer, 3))
		{
			*l_value = 0;
			//Perform the byte swap from MSB to LSB
			for (unsigned char i = 0; i < 3; i++)
				*l_value += (rx_buffer[2-i]<<(8*i));
			//Return the received value
			arbiter_return(&task_i2c, *l_value);
		}
		break;

	default:
		break;
	}
};

/*
 * Get the i2c address from the received did.
 */
unsigned char i2c_decode_did(unsigned char did)
{
	//Switch for the did and get the i2c address
	switch (did)
	{
	case did_BMS:
		//BMS chip
		return i2c_addr_BMS;

	case did_MS5611:
		//Baro
		return i2c_addr_MS5611;

	default:
		return 0;
	}
};

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * All functions use MSB first unless otherwhise stated!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*
 * Send bytes via the i2c bus. The bytes are sent to the currently
 * active bus address. The number of bytes and data pointer have to be specified.
 * 
 * Returns 1, when the bytes are sent.
 */
unsigned long i2c_transmit_nbytes(unsigned char* data, unsigned long nbytes)
{
	//Data transfer ongoing?
	if (!(I2C1->SR2 & I2C_SR2_BUSY))
	{
		// Data transfer finished?
		if (DMA1->HISR & DMA_HISR_TCIF6)
		{
			//Reset DMA flag and return
			DMA1->HIFCR = DMA_HIFCR_CTCIF6;
			return 1;
		}
		else
		{
			//Enable the dma stream, transmit nbytes
			i2c_dma_transmit(data,nbytes);
			//Enable interrupt to catch when the start condition was generated and enable DMA request
			I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_ITEVTEN;
			//Transfer has not started, send start condition
			I2C1->CR1 |= I2C_CR1_START;
		}
	}

	//return 0, bytes are not send
	return 0;
};

/*
 * Enable the dma to transmit data. The data is defined by the pointer which is passed.
 * You can set the number of bytes to be transmitted.
 */
void i2c_dma_transmit(void* data, unsigned long nbytes)
{
	//Set the memory address
	DMA1_Stream6->M0AR = (unsigned long)data;
	//Set number of bytes to transmit
	DMA1_Stream6->NDTR = nbytes+1;
	//Clear DMA interrupts
	DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
	//Enable dma
	DMA1_Stream6->CR = DMA_CONFIG_I2C_TX | DMA_SxCR_EN;
};

/*
 * Interrupthandler for the i2c bus to handle the communication.
 * The handler assumes that is is only called after the generation of the start condition.
 * The rest of the communication is done via the DMA.
 */
void I2C1_EV_IRQHandler(void)
{
	// Make sure that start condition is sent (also the read of SR1 is part of clearing the SB flag)
	if (I2C1->SR1 & I2C_SR1_SB)
	{
		//Write the address
		I2C1->DR = active_address;
	}
	else if (I2C1->SR1 & I2C_SR1_ADDR)
	{
		//Clear the ADDR flag and set the LAST flag when receiving data
		if (!(I2C1->SR2 & I2C_SR2_TRA))
			I2C1->CR2 |= I2C_CR2_LAST;
		//disable the i2c interrupt
		// if (I2C1->SR2 & I2C_SR2_MSL)
		I2C1->CR2 &= ~I2C_CR2_ITEVTEN;
	}
};

/*
 * DMA transfer finished interrupt, to generate stop condition,
 * when bytes are transmitted.
 */
void DMA1_Stream6_IRQHandler(void)
{
	//Disable DMA interrupt, but let the flag be set
	DMA1_Stream6->CR &= ~DMA_SxCR_TCIE;
	//Disable dma requests for i2c
	I2C1->CR2 &= ~I2C_CR2_DMAEN;
	//Send stop condition
	I2C1->CR1 |= I2C_CR1_STOP;
};

/*
 * Receive bytes via the i2c bus. 
 * The number of bytes and data pointer have to be specified.
 * 
 * Returns 1, when the bytes are received.
 */
unsigned long i2c_receive_nbytes(unsigned char* data, unsigned long nbytes)
{
	//Data transfer ongoing?
	if (!(I2C1->SR2 & I2C_SR2_BUSY))
	{
		// Data transfer finished?
		if (DMA1->HISR & DMA_HISR_TCIF5)
		{
			//Reset DMA flag and return
			DMA1->HIFCR = DMA_HIFCR_CTCIF5;
			return 1;
		}
		else
		{
			//Enable the dma stream, transmit nbytes
			i2c_dma_receive(data,nbytes);
			//Enable interrupt to catch when the start condition was generated and enable DMA request
			I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_ITEVTEN;
			//Transfer has not started, send start condition
			I2C1->CR1 |= I2C_CR1_START;
		}
	}

	//return 0, bytes are not send
	return 0;
};

/*
 * Enable the dma to receive data. The data is defined by the pointer which is passed.
 * You can set the number of bytes to be transmitted.
 */
void i2c_dma_receive(void* data, unsigned long nbytes)
{
	//Set the memory address
	DMA1_Stream5->M0AR = (unsigned long)data;
	//Set number of bytes to transmit
	DMA1_Stream5->NDTR = nbytes;
	//Clear DMA interrupts
	DMA1->HIFCR = DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5;
	//Enable dma
	DMA1_Stream5->CR = DMA_CONFIG_I2C_RX | DMA_SxCR_EN;
};

/*
 * DMA transfer finished interrupt, to generate stop condition,
 * when bytes are transmitted.
 */
void DMA1_Stream5_IRQHandler(void)
{
	//Disable DMA interrupt, but let the flag be set
	DMA1_Stream5->CR &= ~DMA_SxCR_TCIE;
	//Disable dma requests for i2c
	I2C1->CR2 &= ~I2C_CR2_DMAEN;
	//Send stop condition
	I2C1->CR1 |= I2C_CR1_STOP;
};

/*
 * send char value to register
 */

// void i2c_send_char_register(unsigned char ch_address, unsigned char ch_register, unsigned char ch_data)
// {

// };

// /*
//  * send int
//  */

// void i2c_send_int(unsigned char ch_address, unsigned int i_data)
// {

// };

// /*
//  * send int LSB first
//  */

// void i2c_send_int_LSB(unsigned char ch_address, unsigned int i_data)
// {

// };

// /*
//  * send int value to register
//  */

// void i2c_send_int_register(unsigned char ch_address, unsigned char ch_register, unsigned int i_data)
// {

// };

// /*
//  * send int value to register, LSB first
//  */

// void i2c_send_int_register_LSB(unsigned char ch_address, unsigned char ch_register, unsigned int i_data)
// {

// };

// /*
//  * Read char
//  */
// unsigned char i2c_read_char(unsigned char ch_address, unsigned char ch_command)
// {

// }

// /*
//  * Read int
//  */
// unsigned int i2c_read_int(unsigned char ch_address, unsigned char ch_command)
// {

// }

// /*
//  * Read int LSB first
//  */
// unsigned int i2c_read_int_LSB(unsigned char ch_address, unsigned char ch_command)
// {

// }

// /*
//  * Read 24bit (3x8bit), meant for MS5611
//  */
// unsigned long i2c_read_24bit(unsigned char ch_address, unsigned char ch_command)
// {

// }

// /*
//  * Read long
//  */
// unsigned long i2c_read_long(unsigned char ch_address, unsigned char ch_command)
// {

// };

// /*
//  * Read an array of specific size
//  */
// unsigned char i2c_read_array(unsigned char ch_address, unsigned char ch_command, unsigned char* p_array, unsigned char array_size)
// {

// };

// /*
//  * Write an array of specific size
//  */
// unsigned char i2c_send_array(unsigned char ch_address, unsigned char* p_array, unsigned char array_size)
// {

// };

// /*
//  * Write an array and tead an array of specific size
//  */
// unsigned char i2c_send_read_array(unsigned char ch_address, unsigned char* array_in, unsigned char size_in, unsigned char* array_out, unsigned char size_out)
// {

// };
