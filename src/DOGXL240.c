/*
 * DOGXL240.c
 *
 *  Created on: 28.02.2018
 *      Author: Sebastian
 */

#include "DOGXL240.h"

/*
 * Initialize necessary peripherals and display
 */

void init_lcd(void)
{
	init_spi();

	/*Set GPIO [Clock already activated in spi_init()]
	 * PB14:	OUT   (C/D)
	 */
	GPIOB->MODER |= GPIO_MODER_MODER14_0;

	/*
	 * Initialize display
	 */
	lcd_set_cd(COMMAND);
	spi_send_char(SET_COM_END);			//Set last COM electrode to 127
	spi_send_char(127);
	spi_send_char(SET_PART_DISP_START);	//Set display start line to 0
	spi_send_char(0);
	spi_send_char(SET_PART_DISP_END);	//Set display end line to 127
	spi_send_char(127);
	spi_send_char(SET_POTI);			//Set Contrast
	spi_send_char(100);

	spi_send_char(0xF1);		//Set com end
	spi_send_char(0x7F);
	spi_send_char(0x25);		//Set Temp comp
	spi_send_char(0xC0);		//LCD Mapping
	spi_send_char(0x02);
	spi_send_char(0x81);		//Set Contrast
	spi_send_char(0x8F);
	spi_send_char(0xA9);		//Display enable
	spi_send_char(0xA5);		//All pixel on

}

/*
 * set C/D
 */
void lcd_set_cd(unsigned char ch_state)
{
	if(ch_state)
		GPIOB->BSRRL = GPIO_BSRR_BS_14;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_14>>16);
}
