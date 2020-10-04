/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 ******************************************************************************
 * @file    spi.c
 * @author  SO
 * @version V1.0
 * @date    27-February-2018
 * @brief   Handles the SPI communication.
 ******************************************************************************
 */

//****** Includes ******
#include "spi.h"

//****** Functions ******
/**
 * @brief Initialize necessary peripherals
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
	//Init SPI2, SCK = 5.25 MHz
	//Set baudrate, Master mode and frameformat (clock high idle, sample on rising edge)
	//TODO Increase Baudrate
	SPI2->CR1 = (0b011<<3) | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
	SPI2->CR2 = SPI_CR2_SSOE;
	SPI2->CR1 |= SPI_CR1_SPE;   //Enable SPI

	//TODO Setup DMA (maybe in display framework?)
}

/**
 * @brief Send char with wait state until transfer complete.
 * @param ch_data The data to be sent
 */
void spi_send_char(unsigned char ch_data)
{
	SPI2->DR = ch_data;
	while(!(SPI2->SR & SPI_SR_TXE));
}
