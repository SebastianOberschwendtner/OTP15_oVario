/*
 * i2c.c
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#include "oVario_Framework.h"

/*
 * Initialize necessary peripherals
 */
void i2c_init(void)
{
	//Init clocks
	gpio_en(GPIO_B);
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	/*
	 * Set GPIO
	 * PB7: I2C1_SDA (AF4)
	 * PB6: I2C1_SCL (AF4)
	 */
	GPIOB->MODER |= GPIO_MODER_MODER7_1 | GPIO_MODER_MODER6_1;
	GPIOB->AFR[0] |= (4<<28) | (4<<24);

	//Init I2C1
	I2C1->CR2 = 42;	//Define APB1 clock speed (42 MHz)
	I2C1->CCR = I2C_CCR_FS | (42000000UL/(3*I2C_CLOCK));
	I2C1->TRISE = 0x28;
	I2C3->CR1 = I2C_CR1_PE;	//Activate I2C
}
/*
 * send char
 */
//TODO Add comments to send proceder especially why the wait conditions

void i2c_send_char(unsigned char ch_address, unsigned char ch_data)
{
	while(I2C1->SR2 & I2C_SR2_BUSY);
	I2C1->CR1 = I2C_CR1_START | I2C_CR1_PE;
	while(!(I2C1->SR1 & I2C_SR1_SB));
	I2C1->DR = ch_address;
	while(!((I2C3->SR1 & I2C_SR1_ADDR) && (I2C3->SR2 & I2C_SR2_MSL)));
	I2C3->DR = ch_data;
	while(!(I2C3->SR1 & I2C_SR1_TXE));
	while(!(I2C3->SR2 & I2C_SR2_MSL));
	I2C3->CR1 = I2C_CR1_STOP | I2C_CR1_PE;
};
