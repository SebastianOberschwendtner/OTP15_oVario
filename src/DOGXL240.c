/*
 * DOGXL240.c
 *
 *  Created on: 28.02.2018
 *      Author: Sebastian
 */

#include "DOGXL240.h"
#include "font.h"

#pragma pack(push, 1)
typedef struct
{
	unsigned char cursor_x;
	unsigned char cursor_y;
	unsigned char buffer[LCD_PIXEL_X*LCD_PIXEL_Y/8];
} lcd;
#pragma pack(pop)


lcd* plcd_DOGXL = 0;

extern const unsigned char font12x16[256][32];
const unsigned char font6x8[256][8];
unsigned char ch_fontsize = 1;		//Sets the fontsize as multiples pixel per bit of the font

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
	spi_send_char(LCD_MAP_MY);
	spi_send_char(SET_LCD_BIAS | LCD_BIAS_11);		//Set bias ratio to 11
	spi_send_char(SET_POTI);							//Set Contrast to 143
	spi_send_char(143);
	spi_send_char(SET_DISP_EN | DISP_EN_DC2);		//Display enable
	spi_send_char(SET_DISP_PAT | DISP_PAT_DC5);		//Set 1bit per pixel in RAM
	spi_send_char(SET_RAM_ADDR_CTRL | RAM_ADDR_CTRL_AC1 | RAM_ADDR_CTRL_AC0); //Automatic wrap around in RAM
	spi_send_char(SET_WPC0);							//Set window programm starting column address
	spi_send_char(0);
	spi_send_char(SET_WPP0);							//Set window programm starting page address
	spi_send_char(0);
	spi_send_char(SET_WPC1);							//Set window programm end column address
	spi_send_char(239);
	spi_send_char(SET_WPP1);							//Set window programm end page address
	spi_send_char(15);
	spi_send_char(SET_WPP_EN);						//Enable window programming
	//spi_send_char(SET_ALL_ON | ALL_ON_DC1);		//All pixel on

	//Set column, page and pattern which sould be written
	lcd_set_page_addr(0);
	lcd_set_col_addr(0);
	lcd_set_write_pattern(PAGE_PATTERN0);

	//register buffer
	plcd_DOGXL = ipc_memory_register(LCD_PIXEL_X*LCD_PIXEL_Y/8 + 2,did_LCD);

	lcd_set_cursor(0,0);

	/*
	 * Initialize DMA
	 */
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	DMA1_Stream4->CR = DMA_SxCR_MINC | DMA_SxCR_DIR_0;
	DMA1_Stream4->NDTR = LCD_PIXEL_X*LCD_PIXEL_Y/8;

	DMA1_Stream4->PAR = (unsigned long)&SPI2->DR;
	DMA1_Stream4->M0AR = (unsigned long)&plcd_DOGXL->buffer[0];
	DMA1_Stream4->FCR = DMA_SxFCR_DMDIS;

	SPI2->CR2 |= SPI_CR2_TXDMAEN;
}
/*
 * Enable DMA transfer
 */
void lcd_dma_enable(void)
{
	lcd_set_cd(DATA);
	DMA1_Stream4->NDTR = LCD_PIXEL_X*LCD_PIXEL_Y/8;
	DMA1_Stream4->CR |= DMA_SxCR_EN;
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
/*
 * Set which pattern is displayed.
 * Range of ch_pat: [0 3]!
 */
void lcd_set_pattern(unsigned char ch_pat)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SET_DISP_PAT | DISP_PAT_DC5 | (ch_pat<<1));
}
/*
 * Switch display on and off
 */
void lcd_set_enable(unsigned char ch_state)
{
	lcd_set_cd(COMMAND);
	spi_send_char(SET_DISP_EN | ch_state);
}
/*
 * Set cursor in buffer
 * Range:	ch_x: [0 239]
 * 			ch_y: [0 127]
 */
void lcd_set_cursor(unsigned char ch_x, unsigned char ch_y)
{
	plcd_DOGXL->cursor_x = ch_x;
	plcd_DOGXL->cursor_y = ch_y;
}

/*
 * set the fontsize
 */
void lcd_set_fontsize(unsigned char ch_size)
{
	ch_fontsize = ch_size;
}
/*
 * Send the lcd buffer to display
 * This function assumens that the RAM cursor is at the beginning of display RAM
 */
void lcd_send_buffer(void)
{
	lcd_set_cd(DATA);
	for(unsigned long l_count=0;l_count<(LCD_PIXEL_X*LCD_PIXEL_Y/8);l_count++)
	{
		spi_send_char(plcd_DOGXL->buffer[l_count]);
	}
}
/*
 * Write specific pixel to buffer. Also clears pixel
 */
void lcd_pixel2buffer(unsigned char ch_x, unsigned char ch_y, unsigned char ch_val)
{
	unsigned char ch_shift = ch_y % 8;
	if(ch_val)
		plcd_DOGXL->buffer[ch_x*(LCD_PIXEL_Y/8)+(ch_y/8)] |= (1<<ch_shift);
	else
		plcd_DOGXL->buffer[ch_x*(LCD_PIXEL_Y/8)+(ch_y/8)] &= ~(1<<ch_shift);
}
/*
 * Write pixel data of a character to buffer at cursor position.
 * Cursors are incremented after each access.
 * The cursor points to the top left corner of character.
 * The size is fixed to 12x16, when ch_fontsize is 0.
 * For other ch_fontsize the size is variable.
 */
void lcd_char2buffer(unsigned char ch_data)
{
	if(ch_fontsize){
		for (unsigned char ch_fonty = 0; ch_fonty<(FONT_Y*ch_fontsize);ch_fonty=ch_fonty+ch_fontsize)
		{
			for (unsigned char ch_fontx = 0; ch_fontx<(FONT_X*ch_fontsize);ch_fontx=ch_fontx+ch_fontsize)
			{
				for(unsigned char ch_addpixelx = 0;ch_addpixelx<ch_fontsize;ch_addpixelx++)
				{
					for(unsigned char ch_addpixely = 0;ch_addpixely<ch_fontsize;ch_addpixely++)
					{
						lcd_pixel2buffer(plcd_DOGXL->cursor_x+ch_fontx+ch_addpixelx,plcd_DOGXL->cursor_y+ch_fonty+ch_addpixely,(font6x8[ch_data][(ch_fonty/ch_fontsize)] & (0x20>>((ch_fontx/ch_fontsize)))));
					}
				}
			}
		}
		plcd_DOGXL->cursor_x += (FONT_X*ch_fontsize);
		if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
		{
			plcd_DOGXL->cursor_x = 0;
			plcd_DOGXL->cursor_y += (FONT_Y*ch_fontsize);
			if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
			{
				plcd_DOGXL->cursor_y = 0;
			}
		}
	}
	else
	{
		for (unsigned char ch_fonty = 0; ch_fonty<16;ch_fonty++)
		{
			for (unsigned char ch_fontx = 0; ch_fontx<12;ch_fontx++)
			{
				lcd_pixel2buffer(plcd_DOGXL->cursor_x+11-ch_fontx,plcd_DOGXL->cursor_y+ch_fonty,(font12x16[ch_data][(2*ch_fonty)+(ch_fontx/8)] & (0x80>>(ch_fontx%8))));
			}
		}
		plcd_DOGXL->cursor_x += 12;
		if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
		{
			plcd_DOGXL->cursor_x = 0;
			plcd_DOGXL->cursor_y += 16;
			if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
			{
				plcd_DOGXL->cursor_y = 0;
			}
		}
	}
}

/*
 * Clear buffer
 */
void lcd_clear_buffer(void)
{
	for(unsigned long l_count=0;l_count<(LCD_PIXEL_X*LCD_PIXEL_Y/8);l_count++)
	{
		plcd_DOGXL->buffer[l_count]=0;
	}
}

/*
 * Write string to buffer
 */
void lcd_string2buffer(char* pch_string)
{
	while(*pch_string != 0)
	{
		lcd_char2buffer(*pch_string);
		pch_string++;
	}
}

/*
 * Write number to buffer using ascii font.
 * Enter data, the number of places to be displayed.
 * Because of the calculation of the digits (LSB first), the cursor has to be shifted to fit the MSB first.
//TODO Discuss whether the cursor shifting is the best solution.
 */
void lcd_num2buffer(unsigned long l_number,unsigned char ch_predecimal)
{
	plcd_DOGXL->cursor_x += (ch_predecimal-1)*FONT_X;
	for(unsigned char ch_count = 0; ch_count<ch_predecimal;ch_count++)
	{
		lcd_char2buffer((l_number%10)+48);
		l_number /=10;
		plcd_DOGXL->cursor_x -= 2*FONT_X;
	}
	plcd_DOGXL->cursor_x += ch_predecimal*FONT_X;
}

/*
 * Write number with special number font.
 */
void lcd_digit2buffer(unsigned long l_number, unsigned char ch_predecimal)
{

}

/*
 * Write line into buffer
 * Start values must be smaller than end values!
 */
void lcd_line2buffer(unsigned char ch_x_start,unsigned char ch_y_start,unsigned char ch_x_end,unsigned char ch_y_end)
{
	if(ch_x_start == ch_x_end)
	{
		for(unsigned char ch_count=0;ch_count<(ch_y_end-ch_y_start);ch_count++)
		{
			lcd_pixel2buffer(ch_x_start,ch_y_start+ch_count,1);
		}
	}
	else if(ch_y_start == ch_y_end)
	{
		for(unsigned char ch_count=0;ch_count<(ch_x_end-ch_x_start);ch_count++)
		{
			lcd_pixel2buffer(ch_x_start+ch_count,ch_y_start,1);
		}
	}
	else
	{
		unsigned long l_gradient = (ch_y_end-ch_y_start)*1000/(ch_x_end-ch_x_start);
		for(unsigned char ch_count=0;ch_count<(ch_x_end-ch_x_start);ch_count++)
		{
			lcd_pixel2buffer(ch_x_start+ch_count,l_gradient*ch_count/1000+ch_y_start,1);
		}
		lcd_pixel2buffer(ch_x_end,ch_y_end,1);
	}
}

/*
 * Write circle to buffer
 */
void lcd_circle2buffer(unsigned char ch_x_center, unsigned char ch_y_center, unsigned char ch_radius)
{

}
