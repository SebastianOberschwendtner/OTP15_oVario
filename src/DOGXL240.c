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

	//Set NSS low at the beginning, display stays selected the whole time
	GPIOB->BSRRH = GPIO_BSRR_BS_12;
	wait_ms(150);
	spi_send_char(SYS_RESET);
	wait_ms(5);

	spi_send_char(SET_COM_END);						//Set com end to 127
	spi_send_char(127);
	spi_send_char(SET_PART_DISP_START);				//Set partial display start to 0
	spi_send_char(0);
	spi_send_char(SET_PART_DISP_END);				//Set partial display start to 127
	spi_send_char(127);
	spi_send_char(SET_TEMP_COMP | TEMP_COMP_010);	//Set Temp comp to -0.10 % per C
	spi_send_char(SET_LCD_MAP);						//LCD Mapping
	spi_send_char(0);
	spi_send_char(SET_LCD_BIAS | LCD_BIAS_11);		//Set bias ratio to 11
	spi_send_char(SET_POTI);						//Set Contrast to 143
	spi_send_char(143);
	spi_send_char(SET_DISP_EN | DISP_EN_DC2);		//Display enable
	spi_send_char(SET_DISP_PAT | DISP_PAT_DC5);		//Set 1bit per pixel in RAM
	spi_send_char(SET_RAM_ADDR_CTRL | RAM_ADDR_CTRL_AC1 | RAM_ADDR_CTRL_AC0); //Automatic wrap around in RAM
	spi_send_char(SET_WPC0);						//Set window programm starting column address
	spi_send_char(0);
	spi_send_char(SET_WPP0);						//Set window programm starting page address
	spi_send_char(0);
	spi_send_char(SET_WPC1);						//Set window programm end column address
	spi_send_char(239);
	spi_send_char(SET_WPP1);						//Set window programm end page address
	spi_send_char(15);
	spi_send_char(SET_WPP_EN);						//Enable window programming
	//spi_send_char(SET_ALL_ON | ALL_ON_DC1);			//All pixel on


	lcd_set_page_addr(15);
	lcd_set_col_addr(239);
	lcd_set_write_pattern(PAGE_PATTERN0);
}
/*
 * Reset display
 */
void lcd_reset(void)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SYS_RESET);
	wait_ms(5);
}
/*
 * Set C/D pin according to send data
 */
void lcd_set_cd(unsigned char ch_state)
{
	if(ch_state)
		GPIOB->BSRRL = GPIO_BSRR_BS_14;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_14>>16);
}
/*
 * Set the column address of RAM
 */
void lcd_set_col_addr(unsigned char ch_col)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SET_COL_LSB | (ch_col & 0b1111));
	spi_send_char(SET_COL_MSB | (ch_col>>4));
}
/*
 * Set the page address of RAM
 * note: ch_page must be smaller than 16!
 */
void lcd_set_page_addr(unsigned char ch_page)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SET_PAGE_LSB | (ch_page & 0b1111));
	spi_send_char(SET_PAGE_MSB | (ch_page>>4));
}
/*
 * Set the pattern number which is written into RAM
 */
void lcd_set_write_pattern(unsigned char ch_pat)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SET_PAGE_MSB | ch_pat);
}
