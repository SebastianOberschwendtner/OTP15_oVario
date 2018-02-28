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

void lcd_init(void)
{
	spi_init();

	/*Set GPIO [Clock already activated in spi_init()]
	 * PB14:	OUT   (C/D)
	 */
	GPIOB->MODER |= GPIO_MODER_MODER14_0;

	/*
	 * Initialize display
	 */
	lcd_set_cd(COMMAND);
	spi_send_char(SET_COM_END);		//Set last COM electrode to 127
	spi_send_char(127);

}

/*
 * set C/D
 */
void lcd_set_cd(unsigned char ch_state)
{
	if(ch_state)
		GPIOB->BSRRL = GPIO_BSRR_BS_14;
	else
		GPIOB->BSRRH = GPIO_BSRR_BR_14;
}
