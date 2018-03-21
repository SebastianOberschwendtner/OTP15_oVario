/*
 * i2c.c
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#include "i2c.h"

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


	/*
	I2C_InitTypeDef I2C_InitStruct;
	I2C_InitStruct.I2C_ClockSpeed   		= 100000;
	I2C_InitStruct.I2C_Mode 				= I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle 			= I2C_DutyCycle_2;
	I2C_InitStruct.I2C_Ack 					= I2C_Ack_Disable;
	I2C_InitStruct.I2C_AcknowledgedAddress 	= I2C_AcknowledgedAddress_7bit;

	I2C_Init(I2C1,&I2C_InitStruct);
	 */


}
/*
 * send char
 */
//TODO Add comments to send procedure especially why the wait conditions

void i2c_send_char(unsigned char ch_address, unsigned char ch_data)
{
	while(I2C1->SR2 & I2C_SR2_BUSY);

	I2C1->CR1 |= I2C_CR1_START;  // Start generation
	while(!(I2C1->SR1 & I2C_SR1_SB));		// Wait for start condition

	I2C1->DR = ch_address;					// Write Adress to register
	while(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL))); // Adress sent and Master Mode

	I2C1->DR = ch_data;						// Write Data to register
	while(!(I2C1->SR1 & I2C_SR1_TXE));		// Data register empty
	while(!(I2C1->SR2 & I2C_SR2_MSL));		// Interface in Master Mode

	I2C1->CR1 |= I2C_CR1_STOP;	// Stop generation
};

/*
 * Read char
 */
unsigned char i2c_read_char(unsigned char ch_address, unsigned char ch_command)
{
	i2c_send_char(ch_address, ch_command);

	I2C1->CR1 |= I2C_CR1_START;  // Start generation
	while(!(I2C1->SR1 & I2C_SR1_SB));		// Wait for start condition

	I2C1->DR = ch_address+1;					// Read Adress to register
	while(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL))); // Adress sent and Master Mode
	while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data
	I2C1->CR1 &= ~I2C_CR1_ACK;				//After received data send NACK
	I2C1->CR1 |= I2C_CR1_STOP;				//Send stop condition

	return I2C1->DR;
}

/*
 * Read int
 */
unsigned int i2c_read_int(unsigned char ch_address, unsigned char ch_command)
{
	unsigned int i_temp = 0;
	i2c_send_char(ch_address, ch_command);

	I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;  // Start generation
	while(!(I2C1->SR1 & I2C_SR1_SB));		// Wait for start condition

	I2C1->DR = ch_address+1;					// Read Adress to register
	while(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL))); // Adress sent and Master Mode

	while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data, 1st byte
	i_temp = (I2C1->DR<<8);

	I2C1->CR1 &= ~I2C_CR1_ACK;				//After received data send NACK
	I2C1->CR1 |= I2C_CR1_STOP;				//Send stop condition
	while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data, 2nd byte
	i_temp |= I2C1->DR;
	return i_temp;
}

/*
 * Read 24bit (3x8bit), meant for MS5611
 */
unsigned long i2c_read_24bit(unsigned char ch_address, unsigned char ch_command)
{
	unsigned long l_temp = 0;
	i2c_send_char(ch_address, ch_command);

	I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;  // Start generation
	while(!(I2C1->SR1 & I2C_SR1_SB));		// Wait for start condition

	I2C1->DR = ch_address+1;					// Read Adress to register
	while(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL))); // Adress sent and Master Mode
	for(unsigned char ch_count = 2; ch_count>0; ch_count--)
	{
		while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data
		l_temp |= (I2C1->DR<<ch_count*8);
	}
	I2C1->CR1 &= ~I2C_CR1_ACK;				//After received data send NACK
	I2C1->CR1 |= I2C_CR1_STOP;				//Send stop condition
	while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data
	l_temp |= I2C1->DR;
	return l_temp;
}

/*
 * Read long
 */
unsigned long i2c_read_long(unsigned char ch_address, unsigned char ch_command)
{
	unsigned long l_temp = 0;
	i2c_send_char(ch_address, ch_command);

	I2C1->CR1 |= I2C_CR1_ACK | I2C_CR1_START;// Start generation
	while(!(I2C1->SR1 & I2C_SR1_SB));		// Wait for start condition

	I2C1->DR = ch_address+1;					// Read Adress to register
	while(!((I2C1->SR1 & I2C_SR1_ADDR) && (I2C1->SR2 & I2C_SR2_MSL))); // Adress sent and Master Mode
	for(unsigned char ch_count = 3; ch_count>0; ch_count--)
	{
		while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data
		l_temp |= (I2C1->DR<<(ch_count)*8);
	}
	I2C1->CR1 &= ~I2C_CR1_ACK;				//After received data send NACK
	I2C1->CR1 |= I2C_CR1_STOP;				//Send stop condition
	while(!(I2C1->SR1 & I2C_SR1_RXNE));		//Wait for received data
	l_temp |= I2C1->DR;
	return l_temp;
}
