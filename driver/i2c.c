/*
 * i2c.c
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#include "i2c.h"

unsigned char ch_i2c_error = 0;
unsigned long l_timeout = 0;

//TODO Add communication error counts to every sensor

/*
 * Makro for wait with timeout
 */
#define TIMEOUT		100*(1000000/I2C_CLOCK)	//[us], wait for a transfer of 10 bits with current I2C-clock before timeout

#define WAIT_FOR(condition,timeout)		\
		l_timeout = 0;\
		while(condition)\
		{\
			if(l_timeout++ >= US2TICK(timeout))\
			{\
				ch_i2c_error=1;\
				return;\
			}\
		}

/*
 * Makro for wait with timeout and return value
 */
#define RETURN_WAIT_FOR(condition,timeout)		\
		l_timeout = 0;\
		while(condition)\
		{\
			if(l_timeout++ >= US2TICK(timeout))\
			{\
				ch_i2c_error=1;\
				return 0;\
			}\
		}

/*
 * Initialize necessary peripherals
 */
void init_i2c(void)
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

};

/*
 * Reset the i2c error
 */
void i2c_reset_error(void)
{
	ch_i2c_error = 0;
};

/*
 * read current error status
 */
unsigned char i2c_get_error(void)
{
	return ch_i2c_error;
};

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * All functions use MSB first unless otherwhise stated!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*
 * send char
 */
//TODO Add comments to send procedure especially why the wait conditions

void i2c_send_char(unsigned char ch_address, unsigned char ch_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START;  					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		I2C1->DR = ch_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;						// Stop generation
	}
};

/*
 * send char value to register
 */

void i2c_send_char_register(unsigned char ch_address, unsigned char ch_register, unsigned char ch_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START;  					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		//transmit register address
		I2C1->DR = ch_register;							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		//transmit byte
		I2C1->DR = ch_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;						// Stop generation
	}
};

/*
 * send int
 */

void i2c_send_int(unsigned char ch_address, unsigned int i_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START; 					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		//transmit 1st byte
		I2C1->DR = (i_data>>8);							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 2nd byte
		I2C1->DR = i_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;						// Stop generation
	}
};

/*
 * send int LSB first
 */

void i2c_send_int_LSB(unsigned char ch_address, unsigned int i_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START;  					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		//transmit 1st byte
		I2C1->DR = i_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 2nd byte
		I2C1->DR = (i_data>>8);							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;	// Stop generation
	}
};

/*
 * send int value to register
 */

void i2c_send_int_register(unsigned char ch_address, unsigned char ch_register, unsigned int i_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START;  					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		//transmit register address
		I2C1->DR = ch_register;							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 1st byte
		I2C1->DR = (i_data>>8);							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 2nd byte
		I2C1->DR = i_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;						// Stop generation
	}
};

/*
 * send int value to register, LSB first
 */

void i2c_send_int_register_LSB(unsigned char ch_address, unsigned char ch_register, unsigned int i_data)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		WAIT_FOR(I2C1->SR2 & I2C_SR2_BUSY,TIMEOUT);

		I2C1->CR1 |= I2C_CR1_START;  					// Start generation
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);	// Wait for start condition

		I2C1->DR = ch_address;							// Write Adress to register
		WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		//transmit register address
		I2C1->DR = ch_register;							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 1st byte
		I2C1->DR = i_data;								// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode
		//transmit 2nd byte
		I2C1->DR = (i_data>>8);							// Write Data to register
		WAIT_FOR(!(I2C1->SR1 & I2C_SR1_TXE),TIMEOUT);	// Data register empty
		WAIT_FOR(!(I2C1->SR2 & I2C_SR2_MSL),TIMEOUT);	// Interface in Master Mode

		I2C1->CR1 |= I2C_CR1_STOP;						// Stop generation
	}
};

/*
 * Read char
 */
unsigned char i2c_read_char(unsigned char ch_address, unsigned char ch_command)
{
	//Only if no error occured
	if(!ch_i2c_error)
	{
		i2c_send_char(ch_address, ch_command);

		I2C1->CR1 |= I2C_CR1_START;  							// Start generation
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);		// Wait for start condition

		I2C1->DR = ch_address+1;								// Read Adress to register
		RETURN_WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data
		I2C1->CR1 &= ~I2C_CR1_ACK;								//After received data send NACK
		I2C1->CR1 |= I2C_CR1_STOP;								//Send stop condition

		return I2C1->DR;
	}
	return 0;
}

/*
 * Read int
 */
unsigned int i2c_read_int(unsigned char ch_address, unsigned char ch_command)
{
	unsigned int i_temp = 0;

	//Only if no error occured
	if(!ch_i2c_error)
	{
		i2c_send_char(ch_address, ch_command);

		I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;  				// Start generation
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);		// Wait for start condition

		I2C1->DR = ch_address+1;								// Read Adress to register
		RETURN_WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data, 1st byte
		i_temp = (I2C1->DR<<8);

		I2C1->CR1 &= ~I2C_CR1_ACK;								//After received data send NACK
		I2C1->CR1 |= I2C_CR1_STOP;								//Send stop condition
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data, 2nd byte
		i_temp |= I2C1->DR;
	}
	return i_temp;
}

/*
 * Read int LSB first
 */
unsigned int i2c_read_int_LSB(unsigned char ch_address, unsigned char ch_command)
{
	unsigned int i_temp = 0;

	//Only if no error occured
	if(!ch_i2c_error)
	{
		i2c_send_char(ch_address, ch_command);

		I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START; 				// Start generation
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);		// Wait for start condition

		I2C1->DR = ch_address+1;								// Read Adress to register
		RETURN_WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode

		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data, 1st byte
		i_temp = I2C1->DR;

		I2C1->CR1 &= ~I2C_CR1_ACK;								//After received data send NACK
		I2C1->CR1 |= I2C_CR1_STOP;								//Send stop condition
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data, 2nd byte
		i_temp |= (I2C1->DR<<8);
	}
	return i_temp;
}

/*
 * Read 24bit (3x8bit), meant for MS5611
 */
unsigned long i2c_read_24bit(unsigned char ch_address, unsigned char ch_command)
{
	unsigned long l_temp = 0;

	//Only if no error occured
	if(!ch_i2c_error)
	{
		i2c_send_char(ch_address, ch_command);

		I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;  					// Start generation
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);			// Wait for start condition

		I2C1->DR = ch_address+1;									// Read Adress to register
		RETURN_WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode
		for(unsigned char ch_count = 2; ch_count>0; ch_count--)
		{
			RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data
			l_temp |= (I2C1->DR<<ch_count*8);
		}
		I2C1->CR1 &= ~I2C_CR1_ACK;									//After received data send NACK
		I2C1->CR1 |= I2C_CR1_STOP;									//Send stop condition
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);		//Wait for received data
		l_temp |= I2C1->DR;
	}
	return l_temp;
}

/*
 * Read long
 */
unsigned long i2c_read_long(unsigned char ch_address, unsigned char ch_command)
{
	unsigned long l_temp = 0;

	//Only if no error occured
	if(!ch_i2c_error)
	{
		i2c_send_char(ch_address, ch_command);

		I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;					// Start generation
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_SB),TIMEOUT);			// Wait for start condition

		I2C1->DR = ch_address+1;									// Read Adress to register
		RETURN_WAIT_FOR(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL)),TIMEOUT); // Adress sent and Master Mode
		for(unsigned char ch_count = 3; ch_count>0; ch_count--)
		{
			RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);	//Wait for received data
			l_temp |= (I2C1->DR<<(ch_count)*8);
		}
		I2C1->CR1 &= ~I2C_CR1_ACK;									//After received data send NACK
		I2C1->CR1 |= I2C_CR1_STOP;									//Send stop condition
		RETURN_WAIT_FOR(!(I2C1->SR1 & I2C_SR1_RXNE),TIMEOUT);		//Wait for received data
		l_temp |= I2C1->DR;
	}
	return l_temp;
}
