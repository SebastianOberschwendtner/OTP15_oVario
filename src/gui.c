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
uint8_t menu = Gui_Vario;
uint8_t submenu = 0;
datafusion_T* 	p_ipc_gui_df_data;
GPS_T* 			p_ipc_gui_gps_data;
BMS_T* 			p_ipc_gui_bms_data;
T_command 		GUI_cmd;

//*********** Functions **************
void gui_init (void)
{
	p_ipc_gui_df_data 	= ipc_memory_get(did_DATAFUSION);
	p_ipc_gui_gps_data 	= ipc_memory_get(did_GPS);
	p_ipc_gui_bms_data 	= ipc_memory_get(did_BMS);

	ipc_register_queue(200, did_GUI);
}


//*********** Task **************
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
	case Gui_BMS:
		fkt_BMS();
		break;
	case Gui_GPS:
		fkt_GPS();
		break;
	default:
		break;
	}

	// draw screen
	lcd_send_buffer();
	//lcd_dma_enable();
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

	// Keypad
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command
		switch(GUI_cmd.data)					// switch for pad number
		{
		case data_KEYPAD_pad_LEFT:		// next screen
			menu = Gui_BMS;
			break;
		case data_KEYPAD_pad_DOWN:
			break;
		case data_KEYPAD_pad_UP:
			break;
		case data_KEYPAD_pad_RIGHT:
			break;
		default:
			break;
		}
	}




}

void fkt_Vario (void)
{
	//
	//	float temp1 = (uint32_t)p_ipc_gui_gps_data->time_utc / 10000;
	//	float temp2 = ((uint32_t)p_ipc_gui_gps_data->time_utc - (temp1) * 10000) / 100;
	//	float temp3 = (uint32_t)p_ipc_gui_gps_data->time_utc - (uint32_t)temp1 * 10000 - (uint32_t)temp2 * 100;

	lcd_set_cursor(190, 8);
	lcd_set_fontsize(1);
	lcd_num2buffer(p_ipc_gui_gps_data->hours,2);
	lcd_string2buffer(":");
	lcd_num2buffer(p_ipc_gui_gps_data->min,2);
	lcd_string2buffer(":");
	lcd_num2buffer(p_ipc_gui_gps_data->sec,2);



	lcd_set_cursor(0, 16);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->glide,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" gz");

	lcd_set_cursor(100, 12);
	lcd_float2buffer(p_ipc_gui_gps_data->msl,4,1);
	lcd_string2buffer(" m MSL");


	lcd_set_cursor(0, 36);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_gps_data->speed_kmh,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" kmh");

	lcd_set_cursor(0, 56);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->height,4,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" m");

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


	// Keypad
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command
		switch(GUI_cmd.data)					// switch for pad number
		{
		case data_KEYPAD_pad_LEFT:		// next screen
			menu = Gui_BMS;
			break;

		case data_KEYPAD_pad_DOWN:
			break;

		case data_KEYPAD_pad_UP:
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_igc_stop_logging;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_IGC);
			break;

		case data_KEYPAD_pad_RIGHT:		// toggle sinktone
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_vario_toggle_sinktone;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_VARIO);
			break;

		default:
			break;
		}


	}
}

uint8_t testipc = 0;

void fkt_BMS(void)
{
	uint8_t y 	= 0;
#define	ls	10		// Line step width
#define	c1	100		// y Value of Value Column

	// Set Fontsize
	lcd_set_fontsize(1);

	// Write Data to Screen
	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("OTG Status: ");
	lcd_set_cursor(c1, y);

	if (p_ipc_gui_bms_data->charging_state == ON)
		lcd_string2buffer("ON");
	else
		lcd_string2buffer("OFF");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("UBat: ");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->battery_voltage,4);
	lcd_string2buffer(" mV");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I charge: ");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->charge_current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I: ");
	lcd_set_cursor(c1, y);
	lcd_signed_num2buffer(p_ipc_gui_bms_data->current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I Discharge: ");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->discharge_current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("C Discharged:");
	lcd_set_cursor(c1, y);
	lcd_signed_num2buffer(p_ipc_gui_bms_data->discharged_capacity,4);
	lcd_string2buffer(" mAh");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I Input:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->input_current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("U Input:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->input_voltage,4);
	lcd_string2buffer(" mV");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I max charge:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->max_charge_current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("I OTG:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->otg_current,4);
	lcd_string2buffer(" mA");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("U OTG:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->otg_voltage,4);
	lcd_string2buffer(" mV");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Temperature:");
	lcd_set_cursor(c1, y);
	lcd_num2buffer(p_ipc_gui_bms_data->temperature,4);
	lcd_string2buffer(" C");


	// Keypad
	testipc = ipc_get_queue_bytes(did_GUI);
	while(testipc > 9) 							// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI, 10, &GUI_cmd); 	// get new command
		testipc = ipc_get_queue_bytes(did_GUI);
		switch(GUI_cmd.data)					// switch for pad number
		{
		case data_KEYPAD_pad_LEFT:		// next screen
			menu = Gui_GPS;
			break;
		case data_KEYPAD_pad_DOWN:
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_BMS_OTG_ON;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_BMS);
			break;
		case data_KEYPAD_pad_UP:
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_BMS_OTG_OFF;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_BMS);
			break;
		case data_KEYPAD_pad_RIGHT:
			break;
		default:
			break;
		}

	}
}


void fkt_GPS(void)
{
	uint8_t y 	= 0;
#define	ls	10		// Line step width
#define	c1	100		// y Value of Value Column

	// Set Fontsize
	lcd_set_fontsize(1);

	// Write Data to Screen
	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Fix Status: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_gps_data->fix,1,0);


	//	if (p_ipc_gui_gps_data->fix != 0)
	//		lcd_string2buffer("Fix");
	//	else
	//		lcd_string2buffer("No Fix");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Latitude: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->lat,3,7);

	if(p_ipc_gui_gps_data->lat > 0)
		lcd_string2buffer(" North");
	else
		lcd_string2buffer(" South");


	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Longitude: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->lon,3,7);

	if(p_ipc_gui_gps_data->lon > 0)
		lcd_string2buffer(" East");
	else
		lcd_string2buffer(" West");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("MSL: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->msl,4,1);
	lcd_string2buffer(" m");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Num Sat: ");
	lcd_set_cursor(c1, y);
	lcd_num2buffer ((unsigned long)p_ipc_gui_gps_data->n_sat,2);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Time:");
	lcd_set_cursor(c1, y);
	//	lcd_float2buffer(p_ipc_gui_gps_data->time_utc,6,0);
	//	lcd_string2buffer(" s");
	//
	//	lcd_set_cursor(190, 8);
	//	lcd_set_fontsize(1);
	lcd_num2buffer(p_ipc_gui_gps_data->hours,2);
	lcd_string2buffer(":");
	lcd_num2buffer(p_ipc_gui_gps_data->min,2);
	lcd_string2buffer(":");
	lcd_num2buffer(p_ipc_gui_gps_data->sec,2);



	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("GPS vel:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->speed_kmh,2,1);
	lcd_string2buffer(" kmh");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("AltRef:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->Altref,4,4);
	lcd_string2buffer(" m");

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("HDOP:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->HDOP,4,4);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("HDG:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->heading_deg,3,1);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("MSG RdIdx:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->Rd_Idx,6,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("MSG Rd_cnt:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->Rd_cnt,6,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("MSG current DMA:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->currentDMA,6,0);


	// Keypad
	testipc = ipc_get_queue_bytes(did_GUI);
	while(testipc > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command
		testipc = ipc_get_queue_bytes(did_GUI);
		switch(GUI_cmd.data)					// switch for pad number
		{
		case data_KEYPAD_pad_LEFT:		// next screen
			menu = Gui_Vario;
			break;
		case data_KEYPAD_pad_DOWN:
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_BMS_OTG_ON;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_BMS);
			break;
		case data_KEYPAD_pad_UP:
			GUI_cmd.did 		= did_GUI;
			GUI_cmd.cmd 		= cmd_BMS_OTG_OFF;
			GUI_cmd.timestamp 	= TIM5->CNT;
			ipc_queue_push((void*)&GUI_cmd, 10, did_BMS);
			break;
		case data_KEYPAD_pad_RIGHT:
			break;
		default:
			break;
		}

	}
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
	float min 		= -1;
	float max 		= 1;
	uint8_t height 	= 61;
	uint8_t xpixel 	= 0;
	uint8_t ypixel 	= 0;
	float temp 		= 0;

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



