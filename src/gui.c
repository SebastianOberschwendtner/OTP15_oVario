/*
 * gui.c
 *
 *  Created on: 10.03.2018
 *      Author: Admin
 */


//*********** Includes **************
#include "gui.h"
#include "DOGXL240.h"
#include "ipc.h"
#include "Variables.h"

//*********** Variables **************
uint8_t menu = Gui_Vario;//Gui_Initscreen;
uint8_t submenu = 0;
datafusion_T* p_ipc_gui_df_data;
GPS_T* p_ipc_gui_gps_data;
BMS_T* p_ipc_gui_bms_data;

//*********** Functions **************
void gui_init (void)
{
	p_ipc_gui_df_data = ipc_memory_get(did_DATAFUSION);
	p_ipc_gui_gps_data = ipc_memory_get(did_GPS);
	p_ipc_gui_bms_data = ipc_memory_get(did_BMS);
}


void gui_task (void)
{
	lcd_clear_buffer();
	switch(menu)
	{
	case Gui_Initscreen:
		fkt_Initscreen();
		break;

	case Gui_Vario:
		fkt_Vario();
		break;

	case Gui_Menu:
		fkt_Menu();
		break;

	case Gui_Settings:
		fkt_Settings();
		break;

	default:
		break;
	}

	// draw screen
	lcd_send_buffer();
}

void fkt_Initscreen (void)
{
	lcd_set_cursor(0, 12);
	lcd_set_fontsize(1);
	lcd_float2buffer(p_ipc_gui_df_data->height,4,1);
	lcd_char2buffer('m');


	lcd_set_cursor(0, 25);
	lcd_num2buffer(p_ipc_gui_df_data->pressure,6);
	lcd_string2buffer("hPa");

	lcd_set_cursor(0, 38);
	lcd_float2buffer(p_ipc_gui_gps_data->speed_kmh,2,1);
	lcd_string2buffer("km/h");

	lcd_set_cursor(0, 51);
	lcd_string2buffer("fix:");
	lcd_num2buffer(p_ipc_gui_gps_data->fix,1);

	lcd_set_cursor(0, 64);
	lcd_string2buffer("UBat:");
	lcd_num2buffer(p_ipc_gui_bms_data->battery_voltage,4);
	lcd_string2buffer("mV");

	lcd_set_cursor(0, 77);
	lcd_string2buffer("IBat:");
	lcd_signed_num2buffer(p_ipc_gui_bms_data->current,3);
	lcd_string2buffer("mA");


	lcd_set_cursor(0, 90);
	lcd_string2buffer("Glide:");
	lcd_float2buffer(p_ipc_gui_df_data->glide,2,1);


	lcd_set_cursor(80, 50);
	lcd_set_fontsize(3);
	lcd_float2buffer(p_ipc_gui_df_data->climbrate_filt,2,2);


	gui_gauge(p_ipc_gui_df_data->climbrate_filt, -2, 5);


}

void fkt_Vario (void)
{
	lcd_set_cursor(0, 16);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->height,4,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" m");

	lcd_set_cursor(0, 36);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_gps_data->speed_kmh,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" kmh");

	lcd_set_cursor(0, 56);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->glide,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" gz");

	lcd_set_cursor(0, 76);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_gps_data->heading_deg,3,0);
	lcd_set_fontsize(1);
	lcd_string2buffer(" hdg");

	lcd_set_cursor(0, 96);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->climbrate_av,1,2);
	lcd_set_fontsize(1);
	lcd_string2buffer(" ms av");



	lcd_set_fontsize(1);
	lcd_set_cursor(0, 116);
	lcd_string2buffer("UBat:");
	lcd_num2buffer(p_ipc_gui_bms_data->battery_voltage,4);
	lcd_string2buffer(" mV");



	lcd_set_cursor(110, 40);
	lcd_set_fontsize(3);
	lcd_float2buffer(p_ipc_gui_df_data->climbrate_filt,2,2);
	lcd_set_fontsize(1);
	lcd_string2buffer(" ms");

	draw_graph(138, 57);
}

void fkt_Menu (void)
{
	;
}

void fkt_Settings(void)
{
	;
}

void gui_gauge(float value, float min, float max)
{
	// 128 Pixel hoch
	// 240 Pixel breit
	// 10 Pixel breit

	if(value > max)
		value = max;

	if(value < min)
		value = min;

	float total = max - min;


	// draw scale

	//lcd_line2buffer(219,122,229,122);



	lcd_set_fontsize(1);
	lcd_line2buffer(219,6,239,6);
	lcd_set_cursor(205, 11);
	lcd_float2buffer(max, 1, 0);

	lcd_line2buffer(219,(122-6)/total*max+6,239,(122-6)/total*max+6); // 88
	lcd_set_cursor(205,(127-11)/total*max+11);

	lcd_float2buffer(0, 1, 0);

	lcd_line2buffer(219,122,239,122);
	lcd_set_cursor(205, 127);
	lcd_float2buffer(min, 1, 0);

	// Draw Block
	uint8_t zeroline = (122-6)/total*max+6;
	uint8_t maxline = 6;
	uint8_t minline = 122;
	uint8_t minzero = minline - zeroline;
	uint8_t zeromax = zeroline - maxline;
	uint8_t value_min = value/min*minzero;

	if(value > 0)
		lcd_block2buffer(229,(122-8)/total*max+7,value/max*(122-6)/total*max,10);
	else if(value < 0)
		lcd_block2buffer(229,zeroline + value_min,value_min,10);


}


void draw_graph(uint8_t x, uint8_t y)
{
	float min = -1;
	float max = 1;
	uint8_t height = 61;
	uint8_t xpixel = 0;
	uint8_t ypixel = 0;
	float temp = 0;



	for (uint8_t dcnt = 0; dcnt < 50; dcnt++)
	{
		temp = p_ipc_gui_df_data->hist_clib[(p_ipc_gui_df_data->hist_ptr + dcnt) % 50];

		if(temp < min)
			temp = min;

		if(temp > max)
			temp = max;

		xpixel = x + dcnt*2 + 1;
		ypixel = y + 1 + height/2 - (int8_t)(temp / max * height / 2);
		lcd_pixel2buffer(xpixel, ypixel,1);
		lcd_pixel2buffer(xpixel, ypixel-1,1);
		lcd_pixel2buffer(xpixel, ypixel+1,1);

		lcd_pixel2buffer(xpixel+1, ypixel,1);
		lcd_pixel2buffer(xpixel+1, ypixel-1,1);
		lcd_pixel2buffer(xpixel+1, ypixel+1,1);
	}

	// Draw vertical lines
	lcd_line2buffer(x, y, x, y + height + 2);
	lcd_line2buffer(x + 100 + 1, y, x + 100 + 1, y + height + 2);

	// Draw horizontal lines
	lcd_line2buffer(x, y + height + 1, x + 100 + 1, y + height + 1); 			// Bottom line
	lcd_line2buffer(x, y , x + 100 + 1, y);										// Top line
	lcd_line2buffer(x, y + height / 2 + 1 , x + 100 + 1, y + height / 2 + 1);	// Middle line


	lcd_set_cursor(x-25, y+8);
	lcd_set_fontsize(1);
	lcd_float2buffer((float)max,1,0);
	lcd_string2buffer("ms");

	lcd_set_cursor(x-25, y + height/2+6);
	lcd_float2buffer((float)0,1,0);
	lcd_string2buffer("ms");



	lcd_set_cursor(x-25, y + height+3);
	lcd_float2buffer((float)min,1,0);
	lcd_string2buffer("ms");



}



