/*
 * gui.c
 *
 *  Created on: 10.03.2018
 *      Author: Admin
 */


//*********** Includes **************
#include "gui.h"
#include "oVario_Framework.h"
#include "DOGXL240.h"
#include "ipc.h"
#include "error.h"
#include "Variables.h"

//*********** struct typedefs ********
typedef struct
{
	uint8_t 	message;
	uint16_t	lifetime;
}InfoBox_T;

//*********** Variables **************
uint8_t 		menu 	= Gui_Vario;
//uint8_t			menu 	= Gui_BMS;
uint8_t 		submenu = 0;
InfoBox_T		InfoBox;
datafusion_T* 	p_ipc_gui_df_data;
GPS_T* 			p_ipc_gui_gps_data;
BMS_T* 			p_ipc_gui_bms_data;
ms5611_T* 		p_ipc_gui_ms5611_data;
SDIO_T*			p_ipc_gui_sd_data;
T_command 		GUI_cmd;

extern uint8_t state_sinktone;
extern unsigned long error_var;

//*********** Functions **************
void gui_init (void)
{
	p_ipc_gui_df_data 		= ipc_memory_get(did_DATAFUSION);
	p_ipc_gui_gps_data 		= ipc_memory_get(did_GPS);
	p_ipc_gui_bms_data 		= ipc_memory_get(did_BMS);
	p_ipc_gui_ms5611_data 	= ipc_memory_get(did_MS5611);
	p_ipc_gui_sd_data		= ipc_memory_get(did_SDIO);

	ipc_register_queue(200, did_GUI);
}


//*********** Task **************
void gui_task (void)
{
	lcd_clear_buffer();
	if (menu >= num_states)	menu = 0;

	gui_status_bar();

	switch(menu)
	{
	case Gui_Initscreen:
		//fkt_Initscreen();
		menu++;
		break;
	case Gui_Vario:
		fkt_Vario();
		break;
	case Gui_Menu:
		//fkt_Menu();
		menu++;
		break;
	case Gui_Settings:
		//fkt_Settings();
		menu++;
		break;
	case Gui_BMS:
		fkt_BMS();
		break;
	case Gui_GPS:
		fkt_GPS();
		break;
	case Gui_MS5611:
		fkt_MS5611();
		break;
	case Gui_Datafusion:
		fkt_Datafusion();
		break;
	default:
		break;
	}
	// Check for runtime errors
	fkt_runtime_errors();

	//Draw infobox
	fkt_infobox();

	// draw screen
	//	lcd_send_buffer();
	lcd_dma_enable();
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
			menu++;
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

uint16_t tempo = 0;
void fkt_Vario (void)
{

	uint8_t y = 20;

	y = y + 8;
	lcd_set_cursor(0, y);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->glide,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" gz");

	y = y + 20;
	lcd_set_cursor(0, y);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_gps_data->speed_kmh,2,1);
	//lcd_float2buffer(p_ipc_gui_df_data->Wind.W_mag,2,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" kmh");


	// Any indication what vario tone mode is active
	lcd_set_cursor(100, y);
	lcd_float2buffer((float)state_sinktone,1,0);
	///////////////////////


	y = y + 20;
	lcd_set_cursor(0, y);
	lcd_set_fontsize(2);
	//	lcd_float2buffer(p_ipc_gui_df_data->height,4,1);
	lcd_float2buffer(p_ipc_gui_gps_data->msl,4,1);
	lcd_set_fontsize(1);
	lcd_string2buffer(" m");

	//	lcd_float2buffer(p_ipc_gui_gps_data->heading_deg,3,0);

	y = y + 20;
	lcd_set_cursor(0, y);
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->climbrate_av,1,2);
	lcd_set_fontsize(1);
	lcd_string2buffer(" ms av");

	// Draw Heading Arrow

#define x_hdg 30
#define y_hdg 108
#define r_hdg 10

	lcd_set_cursor(0, y_hdg -10);
	lcd_string2buffer("HDG");
	lcd_circle2buffer(x_hdg, y_hdg, r_hdg);

	tempo = (uint16_t)p_ipc_gui_gps_data->heading_deg;//tempo + 3;
	//	if(tempo >359) tempo = 0;

	lcd_arrow2buffer(x_hdg, y_hdg, r_hdg, tempo);

	lcd_set_fontsize(1);
	lcd_set_cursor(x_hdg - 3, y_hdg - 10);
	lcd_string2buffer("N");
	lcd_set_cursor(x_hdg + 12, y_hdg + 5);
	lcd_string2buffer("O");
	lcd_set_cursor(x_hdg - 3, y_hdg + 20);
	lcd_string2buffer("S");
	lcd_set_cursor(x_hdg - 18 , y_hdg + 5);
	lcd_string2buffer("W");
	lcd_set_cursor(0, y_hdg + 20);
	lcd_float2buffer(p_ipc_gui_gps_data->heading_deg,3,0);

	// Draw wind speed


#define x_we 70
#define y_we 108
#define r_we 10

	lcd_set_cursor(88, y_hdg - 10);
	lcd_string2buffer("Wind");
	lcd_circle2buffer(x_we, y_we, r_we);

	tempo = (uint16_t)p_ipc_gui_df_data->Wind.W_dir;

	lcd_arrow2buffer(x_we, y_we, r_we, tempo);

	lcd_set_fontsize(1);
	lcd_set_cursor(x_we - 3, y_we - 10);
	lcd_string2buffer("N");
	lcd_set_cursor(x_we + 12, y_we + 5);
	lcd_string2buffer("O ");
	lcd_set_fontsize(2);
	lcd_float2buffer(p_ipc_gui_df_data->Wind.W_mag * 3.6,2,0);
	lcd_set_fontsize(1);
	lcd_set_cursor(x_we - 3, y_we + 20);
	lcd_string2buffer("S");
	lcd_set_cursor(x_we - 18 , y_we + 5);
	lcd_string2buffer("W");

	lcd_set_cursor(45, y_we + 20);
	//lcd_float2buffer(p_ipc_gui_df_data->Wind.W_dir,3,0);

	lcd_set_cursor(85, y_we + 20);
	lcd_float2buffer(p_ipc_gui_df_data->Wind.W_mag * 3.6,2,1);

	lcd_set_cursor(45, y_we - 10);
	//lcd_float2buffer(p_ipc_gui_df_data->Wind.cnt,2,0);




	// Climbrate
	lcd_set_cursor(110, 40);
	lcd_set_fontsize(3);
	lcd_float2buffer(p_ipc_gui_df_data->climbrate_filt,2,2);
	lcd_set_fontsize(1);
	lcd_string2buffer(" ms");

	draw_graph(138, 57);

	//Get commands
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
				break;

			case data_KEYPAD_pad_DOWN:
				GUI_cmd.did 		= did_GUI;
				GUI_cmd.cmd 		= cmd_vario_toggle_sinktone;//cmd_vario_toggle_sinktone;
				GUI_cmd.timestamp 	= TIM5->CNT;
				ipc_queue_push((void*)&GUI_cmd, 10, did_VARIO);
				break;

			case data_KEYPAD_pad_UP:
				GUI_cmd.did 		= did_GUI;
				GUI_cmd.cmd 		= cmd_igc_stop_logging;
				GUI_cmd.timestamp 	= TIM5->CNT;
				ipc_queue_push((void*)&GUI_cmd, 10, did_IGC);
				break;

			case data_KEYPAD_pad_RIGHT:		// toggle sinktone
				GUI_cmd.did 		= did_GUI;
				GUI_cmd.cmd 		= cmd_vario_toggle_sinktone;//cmd_vario_toggle_sinktone;
				GUI_cmd.timestamp 	= TIM5->CNT;
				ipc_queue_push((void*)&GUI_cmd, 10, did_VARIO);
				break;

			default:
				break;
			}
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
			default:
				break;
		}
	}
}


void fkt_BMS(void)
{
	uint8_t y 	= 9;
#define	ls	10		// Line step width
#define	c1	100		// y Value of Value Column

	// Set Fontsize
	lcd_set_fontsize(1);

	// Write Data to Screen
	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("OTG Status: ");
	lcd_set_cursor(c1, y);

	if (p_ipc_gui_bms_data->charging_state & (1<<12))
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

	// check comamnds
	while(ipc_get_queue_bytes(did_GUI) > 9) 							// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI, 10, &GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
		}
	}
}

void fkt_GPS(void)
{
	uint8_t y 	= 9;
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

	/*y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("MSG current DMA:");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(p_ipc_gui_gps_data->currentDMA,6,0);*/

	// Check comamnds
	while(ipc_get_queue_bytes(did_GUI) > 9) 	// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
		}
	}
}


void fkt_MS5611 (void)
{

	uint8_t y 	= 9;
#define	ls	10		// Line step width
#define	c1	100		// y Value of Value Column

	// Set Fontsize
	lcd_set_fontsize(1);

	// Write Data to Screen
	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Pressure: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_ms5611_data->pressure,6,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Temperature: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(((float)p_ipc_gui_ms5611_data->temperature)/100,2,1);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Sens: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(((float)p_ipc_gui_ms5611_data->Sens),11,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Sens2: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(((float)p_ipc_gui_ms5611_data->Sens2),11,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Off: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(((float)p_ipc_gui_ms5611_data->Off),11,0);

	y +=ls;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Off2: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer(((float)p_ipc_gui_ms5611_data->Off2),11,0);

	//Get commands
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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
				break;

			default:
				break;
			}
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
			default:
				break;
		}
	}
}

void fkt_Datafusion	(void)
{

	uint8_t y 	= 9;
#define	ls	10		// Line step width
#define	c1	100		// y Value of Value Column

	// Set Fontsize
	lcd_set_fontsize(1);

	// Write Data to Screen
	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Wind speed: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_df_data->Wind.W_mag,2,1);
	lcd_string2buffer(" m/s");

	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Wind direction: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_df_data->Wind.W_dir,3,0);
	lcd_string2buffer(" grad");

	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Aero speed: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_df_data->Wind.W_Va,2,1);
	lcd_string2buffer(" m/s");

	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Iterations: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_df_data->Wind.iterations,3,0);

	y +=ls - 1;
	lcd_set_cursor(0, y);
	lcd_string2buffer("Data cnt: ");
	lcd_set_cursor(c1, y);
	lcd_float2buffer((float)p_ipc_gui_df_data->Wind.cnt,3,0);


#define rwe 35
	lcd_circle2buffer(rwe + 2, y + rwe + 2, rwe);
	lcd_line2buffer(2,y + rwe + 2, rwe * 2 + 2, y + rwe + 2);
	lcd_line2buffer(rwe + 2,y + 2, rwe + 2, y + rwe * 2 + 2);

	for(uint8_t cntwe = 0; cntwe <60; cntwe++)
	{
		char tempx = rwe + 2;
		char tempy = rwe + y + 2;

		char tempxx = (char)((((float)(p_ipc_gui_df_data->Wind.WE_x[cntwe])) * rwe / 5 / 20) + (float)tempx);
		char tempyy = (char)((((float)(-p_ipc_gui_df_data->Wind.WE_y[cntwe])) * rwe / 5 / 20) + (float)tempy);

		lcd_pixel2buffer(tempxx, tempyy, 1);
	}




	//Get commands
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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

				break;

			default:
				break;
			}
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
			default:
				break;
		}
	}
}




void fkt_Menu (void)
{
	lcd_set_cursor(0, 12);
	lcd_set_fontsize(1);
	lcd_float2buffer(2.0f,4,1);

	//Get commands
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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

				break;

			default:
				break;
			}
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
			default:
				break;
		}
	}
}

void fkt_Settings(void)
{
	lcd_set_cursor(0, 12);
	lcd_set_fontsize(1);
	lcd_float2buffer(3.0f,4,1);

	//Get commands
	while(ipc_get_queue_bytes(did_GUI) > 9) 				// look for new command in keypad queue
	{
		ipc_queue_get(did_GUI,10,&GUI_cmd); 	// get new command

		//Switch for commad
		switch(GUI_cmd.cmd)
		{
		//Keypad
		case cmd_gui_eval_keypad:
			switch(GUI_cmd.data)					// switch for pad number
			{
			case data_KEYPAD_pad_LEFT:		// next screen
				menu++;
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

				break;

			default:
				break;
			}
			break;

			//Infobox
			case cmd_gui_set_std_message:
				InfoBox.message = (unsigned char)GUI_cmd.data;	//Standard message
				InfoBox.lifetime = 50;							//Lifetime is fixed to 5 seconds for now.
				break;
			default:
				break;
		}
	}
}

/*
 * Check for runtime errors and displayes the error as infobox
 */
void fkt_runtime_errors(void)
{
	//if error occured
	if(error_var)
	{
		switch(error_var)
		{
		case err_no_memory_left:
			InfoBox.message = data_info_error;			//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		case err_queue_overrun:
			InfoBox.message = data_info_error;			//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		case err_bms_fault:
			InfoBox.message = data_info_bms_fault;		//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		case err_coloumb_fault:
			InfoBox.message = data_info_coloumb_fault;	//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		case err_baro_fault:
			InfoBox.message = data_info_baro_fault;		//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		case err_sd_fault:
			InfoBox.message = data_info_sd_fault;		//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		default:
			InfoBox.message = data_info_error;			//Standard message
			InfoBox.lifetime = 50;						//Lifetime is fixed to 50 seconds for now.
			break;
		}
		error_var = 0;
	}
};

/*
 * This function displays an infobox on the screen with an expiration time.
 * The expiration time is fixed to 5 seconds for now.
 */
//TODO Add adjustable lifetime.
void fkt_infobox(void)
{
	//Check if infobox is expired
	if(InfoBox.lifetime)
	{
		lcd_box2buffer(170, 40, 2);
		switch(InfoBox.message)
		{
		case data_info_logging_started:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("IGC Started!");
			break;
		case data_info_logging_stopped:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("IGC Finished!");
			break;
		case data_info_error:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Error!");
			break;
		case data_info_bms_fault:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("BMS Fault!");
			break;
		case data_info_coloumb_fault:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("BatGauge Fault!");
			break;
		case data_info_baro_fault:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Barometer Fault!");
			break;
		case data_info_sd_fault:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("SD Card Fault!");
			break;
		case data_info_otg_on_failure:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("OTG On Fault!");
			break;
		case data_info_keypad_0:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Keypad 0");
			break;
		case data_info_keypad_1:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Keypad 1");
			break;
		case data_info_keypad_2:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Keypad 2");
			break;
		case data_info_keypad_3:
			lcd_set_cursor(40, 73);
			lcd_set_fontsize(2);
			lcd_string2buffer("Keypad 3");
			break;



		default:
			break;
		}

		//After displaying decrease lifetime of box
		InfoBox.lifetime--;
	}
};

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
};

/*
 * Draws the bootlogo of the vario.
 */


void gui_bootlogo(void)
{

	lcd_clear_buffer();
	lcd_box2buffer(220, 120, 3);
	lcd_set_cursor(15, 30);
	lcd_set_fontsize(0);
	lcd_string2buffer("OBITec");
	lcd_set_fontsize(1);
	lcd_string2buffer(" and ");
	lcd_set_fontsize(0);
	lcd_string2buffer("JK Design");
	lcd_set_cursor(99, 45);
	lcd_set_fontsize(1);
	lcd_string2buffer("present");
	lcd_set_cursor(39, 85);
	lcd_set_fontsize(3);
	lcd_set_inverted(1);
	lcd_string2buffer(" joVario ");
	lcd_set_cursor(84, 105);
	lcd_set_fontsize(0);
	lcd_set_inverted(0);
	lcd_string2buffer("V 1.00");
	lcd_send_buffer();
};

/*
 * Draws the status bar of the vario
 */
unsigned char ch_status_counter = 0;

void gui_status_bar(void)
{
	/*
	 * Display current menu name
	 */
	lcd_set_cursor(0, 8);
	lcd_set_fontsize(1);
	lcd_set_inverted(0);
	switch(menu)
	{
	case Gui_Vario:
		lcd_string2buffer("Vario");
		break;
	case Gui_GPS:
		lcd_string2buffer("GPS");
		break;
	case Gui_BMS:
		lcd_string2buffer("Battery");
		break;
	case Gui_MS5611:
		lcd_string2buffer("Baro");
		break;
	case Gui_Datafusion:
		lcd_string2buffer("Datafusion");
		break;
	default:
		lcd_string2buffer("home");
		break;
	}

	/*
	 * display current height Baro
	 */
	lcd_set_cursor(35, 8);
	lcd_float2buffer(p_ipc_gui_df_data->height,4,1);
	lcd_string2buffer(" m Baro");


	/*
	 * Sat Symbol
	 */
	lcd_set_cursor(135, 8);
	if(p_ipc_gui_gps_data->fix == 3)
		lcd_set_inverted(0);
	else
		lcd_set_inverted(1);

	lcd_char2buffer(0xFB); //Sat symbol
	lcd_set_inverted(0);


	/*
	 * SD-Card information
	 */
	lcd_set_cursor(142, 8);
	lcd_char2buffer(0xFA); //SD-Card symbol
	// If card is detected
	if(p_ipc_gui_sd_data->state & 1)
	{
		for(unsigned char count = 0; count < 8; count++)
			lcd_char2buffer(p_ipc_gui_sd_data->CardName[count]);
	}
	else
		lcd_string2buffer("No Card");

	/*
	 * Display Battery voltage
	 */
	lcd_set_cursor(196, 8);
	//if not charging
	if(p_ipc_gui_bms_data->charging_state & (1<<14))
	{
		lcd_bat2buffer(ch_status_counter/10);
		ch_status_counter++;
		if(ch_status_counter >= 50)
			ch_status_counter = 0;
	}
	else
	{
		if(p_ipc_gui_bms_data->battery_voltage >= 4000)
			lcd_bat2buffer(3);
		else if(p_ipc_gui_bms_data->battery_voltage >= 3500)
			lcd_bat2buffer(2);
		else if(p_ipc_gui_bms_data->battery_voltage >= 3200)
			lcd_bat2buffer(1);
		else
			lcd_bat2buffer(0);
	}

	/*
	 * Display Clock
	 */
	lcd_set_cursor(209, 8);
	lcd_set_fontsize(1);
	lcd_num2buffer(get_hours_lct(),2);
	lcd_string2buffer(":");
	lcd_num2buffer(get_minutes_lct(),2);

	/*
	 * Display seperation line
	 */
	lcd_line2buffer(0, 8, 239, 8);

};


