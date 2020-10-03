/**
 ******************************************************************************
 * @file    DOGXL240.c
 * @author  SO
 * @version V1.0
 * @date    28-February-2018
 * @brief   Talks to the DOGXL display via the SPI bus.
 ******************************************************************************
 */

//****** Includes *****
#include "DOGXL240.h"
#include "font.h"

//****** Variables ******
unsigned char temp[4];

#pragma pack(push, 1)
typedef struct
{
	unsigned char cursor_x;
	unsigned char cursor_y;
	unsigned char ch_fontsize;		//Sets the fontsize as multiples pixel per bit of the font
	unsigned char ch_inverted;		//Inverts the written charcter
	unsigned char buffer[LCD_PIXEL_X*LCD_PIXEL_Y/8];
} lcd;
#pragma pack(pop)


lcd* plcd_DOGXL = 0;

extern const unsigned char font12x16[256][32];
extern const unsigned char font6x8[256][8];
extern const unsigned char battery12x8[5][16];
extern const unsigned char number16x20[11][40];

//****** Functions ******
/**
 * @brief Initialize necessary peripherals and display.
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
	plcd_DOGXL = ipc_memory_register(LCD_PIXEL_X*LCD_PIXEL_Y/8 + 4,did_LCD);

	lcd_set_cursor(0,0);
	lcd_set_inverted(0);
	lcd_set_fontsize(0);

	/*
	 * Initialize DMA
	 */
	//Enable Clocks
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	//Wait for DMA to be finished and write settings
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	DMA1_Stream4->CR = DMA_SxCR_MINC | DMA_SxCR_DIR_0;			//Memory: 8bit to Peripheral: 16bit
	DMA1_Stream4->NDTR = LCD_PIXEL_X*LCD_PIXEL_Y/8;				//Number of transfers for the buffer
	DMA1_Stream4->PAR = (unsigned long)&SPI2->DR;				//Set SPI2 as target peripheral address
	DMA1_Stream4->M0AR = (unsigned long)&plcd_DOGXL->buffer[0];	//Set display buffer as start address
	DMA1_Stream4->FCR = 0;//DMA_SxFCR_DMDIS;							//Enable FIFO with 4 bytes

	SPI2->CR2 |= SPI_CR2_TXDMAEN;								//Enable DMA request in SPI2
};

/**
 * @brief Enable DMA transfer
 */
void lcd_dma_enable(void)
{
	//Clear DMA interrupts
	DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

	lcd_set_cd(DATA);
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN))		//check if transfer is complete
	{
		DMA1_Stream4->NDTR = (LCD_PIXEL_X*LCD_PIXEL_Y/8);
		DMA1_Stream4->CR |= DMA_SxCR_EN;
	}
};

/**
 * @brief Reset display
 */
void lcd_reset(void)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SYS_RESET);
		wait_ms(5);
	}
};

/**
 * @brief Set C/D pin according to send data
 * @param ch_state New state of the pin
 */
void lcd_set_cd(unsigned char ch_state)
{
	if(ch_state)
		GPIOB->BSRRL = GPIO_BSRR_BS_14;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_14>>16);
};

/**
 * @brief Set the column address of RAM
 * @param ch_col The new column address
 */
void lcd_set_col_addr(unsigned char ch_col)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SET_COL_LSB | (ch_col & 0b1111));
		spi_send_char(SET_COL_MSB | (ch_col>>4));
	}
};

/**
 * @brief Set the page address of RAM
 * note: ch_page must be smaller than 16!
 * @param ch_page The new page address
 */
void lcd_set_page_addr(unsigned char ch_page)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SET_PAGE_LSB | (ch_page & 0b1111));
		spi_send_char(SET_PAGE_MSB | (ch_page>>4));
	}
};

/**
 * @brief Set the pattern number which is written into RAM.
 * @param ch_pat The pattern number
 */
void lcd_set_write_pattern(unsigned char ch_pat)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SET_PAGE_MSB | ch_pat);
	}
};

/**
 * @brief Set which pattern is displayed.
 * Range of ch_pat: [0 3]!
 * @param ch_pat The pattern number
 */
void lcd_set_pattern(unsigned char ch_pat)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SET_DISP_PAT | DISP_PAT_DC5 | (ch_pat<<1));
	}
};

/**
 * @brief Switch display on and off.
 * @param ch_state The new state of the display.
 */
void lcd_set_enable(unsigned char ch_state)
{
	if(!(DMA1_Stream4->CR & DMA_SxCR_EN)) //Wait for DMA transfer to finish
	{
		lcd_set_cd(COMMAND);
		spi_send_char(SET_DISP_EN | ch_state);
	}
};

/**
 * @brief Set cursor in buffer
 * Range:	ch_x: [0 239]
 * 			ch_y: [0 127]
 * @param ch_x New X-Position of cursor.
 * @param ch_y New Y-Position of cursor.
 */
void lcd_set_cursor(unsigned char ch_x, unsigned char ch_y)
{
	plcd_DOGXL->cursor_x = ch_x;
	plcd_DOGXL->cursor_y = ch_y;
};

/**
 * @brief Shift the cursor according to ch_pos_x and ch_pos_y.
 * Both directions can be negative.
 * @param ch_pos_x The shift length in X-direction.
 * @param ch_pos_y The shift length in Y-direction.
 */
void lcd_shift_cursor(signed char ch_pos_x, signed char ch_pos_y)
{
	plcd_DOGXL->cursor_x += ch_pos_x;
	plcd_DOGXL->cursor_y += ch_pos_y;

	if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
		plcd_DOGXL->cursor_x = 0;
	if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
		plcd_DOGXL->cursor_y = 0;
};

/**
 * @brief set the fontsize
 * @param ch_size New size
 */
void lcd_set_fontsize(unsigned char ch_size)
{
	plcd_DOGXL->ch_fontsize = ch_size;
};

/**
 * @brief Set the invertion state.
 * @param ch_state New state
 */
void lcd_set_inverted(unsigned char ch_state)
{
	plcd_DOGXL->ch_inverted = ch_state;
};

/**
 * @brief Send the lcd buffer to display.
 * This function assumes that the RAM cursor is at the beginning of display RAM!
 */
void lcd_send_buffer(void)
{
	lcd_set_cd(DATA);
	for(unsigned long l_count=0;l_count<(LCD_PIXEL_X*LCD_PIXEL_Y/8);l_count++)
	{
		spi_send_char(plcd_DOGXL->buffer[l_count]);
	}
};

/**
 * @brief Write specific pixel to buffer. Also clears pixel.
 * @param ch_x X-Position of the pixel.
 * @param ch_y Y-Position of the pixel.
 * @param ch_val Value of the pixel.
 */
void lcd_pixel2buffer(unsigned char ch_x, unsigned char ch_y, unsigned char ch_val)
{
	unsigned char ch_shift = ch_y % 8;
	ch_val= (ch_val && 1) ^ plcd_DOGXL->ch_inverted;
	if(ch_val)
		plcd_DOGXL->buffer[ch_x*(LCD_PIXEL_Y/8)+(ch_y/8)] |= (1<<ch_shift);
	else
		plcd_DOGXL->buffer[ch_x*(LCD_PIXEL_Y/8)+(ch_y/8)] &= ~(1<<ch_shift);
};

/**
 * @brief Write pixel data of a character to buffer at cursor position.
 * Cursors are incremented after each access.
 * The cursor points to the bottom left corner of character.
 * The size is fixed to 12x16, when ch_fontsize is 0.
 * For other ch_fontsize the size is variable as a multiple of 6x8.
 * @param ch_data Value to written
 * @todo //TODO The function checks the maximum cursor size after writing to buffer => this can cause problem if cursor is manually increased to the maximum position
 */
void lcd_char2buffer(unsigned char ch_data)
{
	if(plcd_DOGXL->ch_fontsize){
		plcd_DOGXL->cursor_y -= plcd_DOGXL->ch_fontsize*FONT_Y;
		for (unsigned char ch_fonty = 0; ch_fonty<(FONT_Y*plcd_DOGXL->ch_fontsize);ch_fonty=ch_fonty+plcd_DOGXL->ch_fontsize)
		{
			for (unsigned char ch_fontx = 0; ch_fontx<(FONT_X*plcd_DOGXL->ch_fontsize);ch_fontx=ch_fontx+plcd_DOGXL->ch_fontsize)
			{
				for(unsigned char ch_addpixelx = 0;ch_addpixelx<plcd_DOGXL->ch_fontsize;ch_addpixelx++)
				{
					for(unsigned char ch_addpixely = 0;ch_addpixely<plcd_DOGXL->ch_fontsize;ch_addpixely++)
					{
						lcd_pixel2buffer(plcd_DOGXL->cursor_x+ch_fontx+ch_addpixelx,plcd_DOGXL->cursor_y+ch_fonty+ch_addpixely,
								(font6x8[ch_data][(ch_fonty/plcd_DOGXL->ch_fontsize)] & (0x20>>((ch_fontx/plcd_DOGXL->ch_fontsize)))));
					}
				}
			}
		}
		plcd_DOGXL->cursor_x += (FONT_X*plcd_DOGXL->ch_fontsize);
		plcd_DOGXL->cursor_y += plcd_DOGXL->ch_fontsize*FONT_Y;
		if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
		{
			plcd_DOGXL->cursor_x = 0;
			plcd_DOGXL->cursor_y += (FONT_Y*plcd_DOGXL->ch_fontsize);
			if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
			{
				plcd_DOGXL->cursor_y = 0;
			}
		}
	}
	else
	{
		plcd_DOGXL->cursor_y -= 16;
		for (unsigned char ch_fonty = 0; ch_fonty<16;ch_fonty++)
		{
			for (unsigned char ch_fontx = 0; ch_fontx<12;ch_fontx++)
			{
				lcd_pixel2buffer(plcd_DOGXL->cursor_x+11-ch_fontx,plcd_DOGXL->cursor_y+ch_fonty,(font12x16[ch_data][(2*ch_fonty)+(ch_fontx/8)] & (0x80>>(ch_fontx%8))));
			}
		}
		plcd_DOGXL->cursor_x += 12;
		plcd_DOGXL->cursor_y += 16;
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
};

/**
 * @brief Clear buffer
 */
void lcd_clear_buffer(void)
{
	for(unsigned long l_count=0;l_count<(LCD_PIXEL_X*LCD_PIXEL_Y/8);l_count++)
	{
		plcd_DOGXL->buffer[l_count]=0;
	}
};

/**
 * @brief Write string to buffer
 * @param pch_string Pointer to the string to be written.
 */
void lcd_string2buffer(char* pch_string)
{
	while(*pch_string != 0)
	{
		lcd_char2buffer(*pch_string);
		pch_string++;
	}
};

/**
 * @brief Write number to buffer using ascii font.
 * Enter data, the number of places to be displayed.
 * Because of the calculation of the digits (LSB first), the cursor has to be shifted to fit the MSB first.
 * @param l_number Value to be written.
 * @param ch_predecimal The number of digits to be written.
 * @todo Discuss whether the cursor shifting is the best solution.
 */
void lcd_num2buffer(unsigned long l_number,unsigned char ch_predecimal)
{
	if(plcd_DOGXL->ch_fontsize)
		plcd_DOGXL->cursor_x += (ch_predecimal-1)*FONT_X*plcd_DOGXL->ch_fontsize;
	else
		plcd_DOGXL->cursor_x += (ch_predecimal-1)*12;

	for(unsigned char ch_count = 0; ch_count<ch_predecimal;ch_count++)
	{
		lcd_char2buffer((unsigned char)(l_number%10)+48);
		l_number /=10;
		if(plcd_DOGXL->ch_fontsize)
			plcd_DOGXL->cursor_x -= 2*FONT_X*plcd_DOGXL->ch_fontsize;
		else
			plcd_DOGXL->cursor_x -= 2*12;
	}

	if(plcd_DOGXL->ch_fontsize)
		plcd_DOGXL->cursor_x += (ch_predecimal+1)*FONT_X*plcd_DOGXL->ch_fontsize;
	else
		plcd_DOGXL->cursor_x += (ch_predecimal+1)*12;
}

/**
 * @brief Write number with special number font.
 * @param ch_data The value to be written.
 */
void lcd_digit2buffer(unsigned char ch_data)
{
	unsigned char ch_digitsize = 1;
	if(plcd_DOGXL->ch_fontsize > 1)
		ch_digitsize = plcd_DOGXL->ch_fontsize;
	plcd_DOGXL->cursor_y -= 20;
	for (unsigned char ch_fonty = 0; ch_fonty<(20*ch_digitsize);ch_fonty=ch_fonty+ch_digitsize)
	{
		for (unsigned char ch_fontx = 0; ch_fontx<(16*ch_digitsize);ch_fontx=ch_fontx+ch_digitsize)
		{
			for(unsigned char ch_addpixelx = 0;ch_addpixelx<ch_digitsize;ch_addpixelx++)
			{
				for(unsigned char ch_addpixely = 0;ch_addpixely<ch_digitsize;ch_addpixely++)
				{
					lcd_pixel2buffer(plcd_DOGXL->cursor_x+ch_fontx+ch_addpixelx,plcd_DOGXL->cursor_y+ch_fonty+ch_addpixely,(number16x20[ch_data][(2*ch_fonty/ch_digitsize)+(ch_fontx/(8*ch_digitsize))] & (1<<((ch_fontx/ch_digitsize)%8))));
				}
			}
		}
	}
	plcd_DOGXL->cursor_x += (16*ch_digitsize);
	plcd_DOGXL->cursor_y += 20;
	if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
	{
		plcd_DOGXL->cursor_x = 0;
		plcd_DOGXL->cursor_y += (20*ch_digitsize);
		if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
		{
			plcd_DOGXL->cursor_y = 0;
		}
	}
};

/**
 * @brief Write float number to buffer.
 * @param f_number The value to be written.
 * @param ch_predecimal The leading significant digits.
 * @param ch_dedecimal The trailing significant digits.
 */
void lcd_float2buffer(float f_number, unsigned char ch_predecimal, unsigned char ch_dedecimal)
{
	if(f_number >= 0)
		lcd_char2buffer('+');
	else
	{
		lcd_char2buffer('-');
		f_number = -f_number;
	}


	unsigned long E = 1;
	for(unsigned char cnt = 0; cnt < ch_dedecimal; cnt++)
	{
		E *= 10;
	}


	unsigned long predec = (unsigned long)f_number;
	unsigned long dedec  =  (unsigned long)((f_number - predec) * E);

	lcd_num2buffer(predec,ch_predecimal);
	if(ch_dedecimal > 0)
	{
		lcd_char2buffer('.');
		lcd_num2buffer(dedec,ch_dedecimal);
	}

};

/**
 * @brief Write battery symbol to buffer.
 * @param ch_stat The new SOC of the battery.
 */
void lcd_bat2buffer(unsigned char ch_stat)
{
	plcd_DOGXL->cursor_y -= 8;
	for (unsigned char ch_fonty = 0; ch_fonty<8;ch_fonty++)
	{
		for (unsigned char ch_fontx = 0; ch_fontx<12;ch_fontx++)
		{
			lcd_pixel2buffer(plcd_DOGXL->cursor_x+ch_fontx,plcd_DOGXL->cursor_y+ch_fonty,(battery12x8[ch_stat][(2*ch_fonty)+(ch_fontx/8)] & (1<<(7 - (ch_fontx%8)))));
		}
	}
	plcd_DOGXL->cursor_x += 12;
	plcd_DOGXL->cursor_y += 8;
	if(plcd_DOGXL->cursor_x > LCD_PIXEL_X)
	{
		plcd_DOGXL->cursor_x = 0;
		plcd_DOGXL->cursor_y += 8;
		if(plcd_DOGXL->cursor_y > LCD_PIXEL_Y/8)
		{
			plcd_DOGXL->cursor_y = 0;
		}
	}
};

/**
 * @brief Write line into buffer
 * Start values must be smaller than end values!
 * @param ch_x_start Start of line in X-direction.
 * @param ch_y_start Start of line in Y-direction.
 * @param ch_x_end End of line in X-direction.
 * @param ch_y_end End of line in Y-direction.
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
		signed long l_gradient = (ch_y_end-ch_y_start)*1000/(ch_x_end-ch_x_start);
		for(unsigned char ch_count=0;ch_count<(ch_x_end-ch_x_start);ch_count++)
		{
			lcd_pixel2buffer(ch_x_start+ch_count,(unsigned char)(l_gradient*ch_count/1000+ch_y_start),1);
		}
		lcd_pixel2buffer(ch_x_end,ch_y_end,1);
	}
};

/**
 * @brief Write circle to buffer.
 * @param ch_x_center Center of circle in X-direction.
 * @param ch_y_center Center of circle in Y-direction.
 * @param ch_radius Radius of the circle.
 */
void lcd_circle2buffer(unsigned char ch_x_center, unsigned char ch_y_center, unsigned char ch_radius)
{
	uint8_t x = ch_x_center;
	uint8_t y = ch_y_center;
	uint8_t r = ch_radius;
	float pTemp;

	for(uint8_t i = 0; i <= r; i++)
	{
		arm_sqrt_f32((float)(r * r - i * i), &pTemp);
		lcd_pixel2buffer(x + i, y + (uint8_t)pTemp,1);
		lcd_pixel2buffer(x - i, y + (uint8_t)pTemp, 1);
		lcd_pixel2buffer(x + i, y - (uint8_t)pTemp,1);
		lcd_pixel2buffer(x - i, y - (uint8_t)pTemp, 1);
	}
};

/**
 * @brief Write Arrow to buffer
 * @param ch_x_center Center of circle in X-direction.
 * @param ch_y_center Center of circle in Y-direction.
 * @param ch_radius Radius of the circle.
 * @param heading Direction where the arrow is pointing.
 */
void lcd_arrow2buffer(unsigned char ch_x_center, unsigned char ch_y_center, unsigned char ch_radius, uint16_t heading)
{
	uint8_t r = ch_radius;
	uint8_t xt = 0;
	uint8_t yt = 0;
	float ctemp = 0;
	float stemp = 0;
	float temp = (float)heading / 180 * 3.1415926f;

	ctemp = cos(temp);
	stemp = sin(temp);

	yt = (int8_t)((float)r * ctemp);
	xt = (int8_t)((float)r * stemp);

	uint8_t x1, x2, y1, y2;
	x1 = (uint8_t)(ch_x_center + xt);
	x2 = (uint8_t)(ch_x_center - xt);
	y1 = (uint8_t)(ch_y_center - yt);
	y2 = (uint8_t)(ch_y_center + yt);

	if(x2 > x1)
	{
		lcd_line2buffer(x1, y1, x2, y2);
	}
	else
	{
		lcd_line2buffer(x2, y2, x1, y1);
	}
	yt = (int8_t)((float)(r-3) * ctemp);
	xt = (int8_t)((float)(r-3) * stemp);

	lcd_circle2buffer((uint8_t)(ch_x_center + xt), (uint8_t)(ch_y_center - yt),1);
	lcd_circle2buffer((uint8_t)(ch_x_center + xt), (uint8_t)(ch_y_center - yt),2);
};

/**
 * @brief Draw Block; Coordinates are bottom left corner
 * @param ch_x The X-Position of the bottom left corner of the block.
 * @param ch_y The Y-Position of the bottom left corner of the block.
 * @param ch_height The height of the block.
 * @param ch_width The width of the block.
 */
void lcd_block2buffer(unsigned char ch_x, unsigned char ch_y, unsigned char ch_height, unsigned char ch_width)
{
	if(ch_height != 0)
	{
		for(unsigned char x_cnt = 0; x_cnt < ch_width; x_cnt++)
		{
			for(unsigned char y_cnt = 0; y_cnt < ch_height; y_cnt++)
			{
				lcd_pixel2buffer(ch_x + x_cnt, ch_y - y_cnt,1);
			}
		}
	}
};

/**
 * @brief Write a signed number to buffer using ascii font.
 * Enter data, the number of places to be displayed.
 * Because of the calculation of the digits (LSB first), the cursor has to be shifted to fit the MSB first.
 * @param l_number The value to be written.
 * @param ch_predecimal The number of significant digits.
 */
void lcd_signed_num2buffer(signed long l_number,unsigned char ch_predecimal)
{
	if(l_number < 0)
	{
		lcd_char2buffer('-');
		l_number *= -1;
	}
	else
		lcd_char2buffer('+');

	if(plcd_DOGXL->ch_fontsize)
		plcd_DOGXL->cursor_x += (ch_predecimal-1)*FONT_X*plcd_DOGXL->ch_fontsize;
	else
		plcd_DOGXL->cursor_x += (ch_predecimal-1)*12;
	for(unsigned char ch_count = 0; ch_count<ch_predecimal;ch_count++)
	{
		lcd_char2buffer((unsigned char)(l_number%10)+48);
		l_number /=10;
		if(plcd_DOGXL->ch_fontsize)
			plcd_DOGXL->cursor_x -= 2*FONT_X*plcd_DOGXL->ch_fontsize;
		else
			plcd_DOGXL->cursor_x -= 2*12;
	}
	if(plcd_DOGXL->ch_fontsize)
		plcd_DOGXL->cursor_x += (ch_predecimal+1)*FONT_X*plcd_DOGXL->ch_fontsize;
	else
		plcd_DOGXL->cursor_x += (ch_predecimal+1)*12;
};

/**
 * @brief Write an empty box with a border in the buffer.
 * The box is centered in the middle of the screen.
 * @param width The width of the box.
 * @param height The height of the box.
 * @param thickness The thickness of the border of the box.
 */
void lcd_box2buffer(unsigned char width, unsigned char height, unsigned char thickness)
{
	//get coordinates of box
	unsigned char x_start 	= (LCD_PIXEL_X/2) - (width/2);
	unsigned char x_end 	= (LCD_PIXEL_X/2) + (width/2);
	unsigned char y_start  	= (LCD_PIXEL_Y/2) - (height/2);
	unsigned char y_end		= (LCD_PIXEL_Y/2) + (height/2);

	//Draw border
	for(unsigned char border = 0; border < thickness; border++)
	{
		//horizontal lines
		lcd_line2buffer(x_start, y_start + border, x_end, y_start + border);
		lcd_line2buffer(x_start, y_end - border, x_end, y_end - border);

		//vertical lines
		lcd_line2buffer(x_start + border, y_start, x_start + border, y_end);
		lcd_line2buffer(x_end - border, y_start, x_end - border, y_end);
	}

	//Fill with empty space
	//X-direction
	for(unsigned char x_count = x_start+thickness; x_count <= x_end-thickness; x_count++)
	{
		//Y-direction
		for(unsigned char y_count = y_start+thickness; y_count <= y_end-thickness; y_count++)
		{
			lcd_pixel2buffer(x_count, y_count, 0);
		}
	}
};