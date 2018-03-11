/*
 * spi.c
 *
 *  Created on: 27.02.2018
 *      Author: Sebastian
 */

#include "spi.h"

/*
 * Initialize necessary peripherals
 */
void init_spi(void)
{
	//Init clocks
	gpio_en(GPIO_B);
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	/*
	 * Set GPIO
	 * PB12: SPI2_NSS (AF5)
	 * PB13: SPI2_SCK  (AF5)
	 * PB15: SPI2_MOSI (AF5)
	 */
	GPIOB->MODER |= GPIO_MODER_MODER15_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER12_0;
	GPIOB->AFR[1] |= (5<<28) | (5<<20) | (5<<16);

	GPIOB->BSRRL  = GPIO_BSRR_BS_12;
	//Init SPI2, SCK = 1.3125 MHz
	//Set baudrate, Master mode and frameformat (clock high idle, sample on rising edge)
	//TODO Increase Baudrate
	SPI2->CR1 = (0b101<<3) | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
	SPI2->CR2 = SPI_CR2_SSOE;
	SPI2->CR1 |= SPI_CR1_SPE;   //Enable SPI

	//TODO Setup DMA (maybe in display framework?)
}

/*
 * send char with wait state until transfer complete
 */
void spi_send_char(unsigned char ch_data)
{
	SPI2->DR = ch_data;
	while(!(SPI2->SR & SPI_SR_TXE));
}
