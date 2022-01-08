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
 * @file    Variables.h
 * @author  SO
 * @version V1.1
 * @date    25-February-2018
 * @brief   Contains the typedefs for the IPC memory and important system
 * 			defines.
 ******************************************************************************
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_

#define true 1
#define false 0


// ******* QUEUE MESSAGES *******
#define cmd_ipc_signal_finished		255 //Generic signal finished command for all tasks
#define cmd_ipc_outta_time			254 //Generic signal to signal that the called command timed out

// KEYPAD, Do not change the number assignment since it
// is used in the gui to index the array with corresponding key function
#define data_KEYPAD_pad_LEFT		0
#define data_KEYPAD_pad_DOWN		1
#define data_KEYPAD_pad_UP			2
#define data_KEYPAD_pad_RIGHT		3

// BMS
#define cmd_BMS_OTG_OFF				0
#define cmd_BMS_OTG_ON				1
#define cmd_BMS_ResetCapacity		2

// Sound
#define cmd_sound_set_frequ 		1
#define cmd_sound_set_vol			2
#define cmd_sound_set_louder 		3
#define cmd_sound_set_quieter 		4
#define cmd_sound_set_mute			5
#define cmd_sound_set_unmute		6
#define cmd_sound_set_beep 			7
#define cmd_sound_set_beep_short	8
#define cmd_sound_set_cont			9
#define cmd_sound_set_period		10

// Vario
#define cmd_vario_set_sinktone		0
#define cmd_vario_clear_sinktone	1
#define cmd_vario_toggle_sinktone	2

//IGC Logging
#define cmd_igc_stop_logging		1
#define cmd_igc_eject_card			2
#define cmd_igc_start_logging		3

//MD5 calculation
#define cmd_md5_set_active_string	1

//GUI
#define cmd_gui_eval_keypad			1
#define cmd_gui_set_std_message		2

//Standard messages for infobox
#define data_info_logging_started	1
#define data_info_logging_stopped	2
#define data_info_error				3
#define data_info_bms_fault			4
#define data_info_coloumb_fault		5
#define data_info_baro_fault		6
#define data_info_sd_fault			7
#define data_info_otg_on_failure	8
#define data_info_keypad_0			9
#define data_info_keypad_1			10
#define data_info_keypad_2			11
#define data_info_keypad_3			12
#define data_info_msg_with_payload	13
#define data_info_igc_start_error	14
#define data_info_shutting_down		15
#define data_info_stopping_log		16
#define data_info_no_gps_fix		17


// ******* TYPEDEF *******
// Generic command for queue
#pragma pack(push, 1)
typedef struct
{
	uint8_t did;
	uint8_t cmd;
	uint32_t data;
	uint32_t timestamp;
}T_command;
#pragma pack(pop)

//Sound
#pragma pack(push, 1)
typedef struct
{
	uint8_t	mute;
	uint8_t volume;		//[%]
	uint16_t frequency; //[Hz]
	uint8_t period;		//[1/100s]
	uint8_t mode;
	uint8_t beep;
}T_sound_state;
#pragma pack(pop)


// Keypad
#pragma pack(push, 1)
typedef struct
{
	uint8_t	 pad;
	uint32_t timestamp;
}T_keypad;
#pragma pack(pop)


// Datafusion

#pragma pack(push, 1)
typedef struct{
	float W_mag;
	float W_dir;
	float W_Va;
	uint32_t iterations;
	uint8_t cnt;
	int8_t WE_x[61];
	int8_t WE_y[61];
}datafusion_T_Wind;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct{
	float 		hoehe;
	float 		climbrate;
	float 		climbrate_filt;
	float 		height;
	int32_t 	pressure;
	float		Time;
	float		ui1;
	float		yi1;
	float		yi2;
	float		sub;
	float		climbrate_av;
	float		glide;
	float		hist_clib[50];
	uint8_t 	hist_ptr;
	float 		hist_h[30];
	uint8_t 	histh_ptr;
	datafusion_T_Wind Wind;
}datafusion_T;
#pragma pack(pop)



// MS5611
#pragma pack(push, 1)
typedef struct{
	int32_t pressure;
	int32_t temperature;
	uint32_t timestamp;
	uint8_t com_err;
	int64_t Off;
	int64_t Off2;
	int64_t Sens;
	uint64_t Sens2;
}ms5611_T;
#pragma pack(pop)


// GPS
#pragma pack(push, 1)
typedef struct{
	float speed_kmh;
	float heading_deg;
	float lat;
	float lon;
	float time_utc;
	uint8_t n_sat;
	float HDOP;
	float msl;		// height above MSL			[m]
	float height; 	// height above ellipsoid 	[m]
	float Altref;
	uint8_t fix;
	float Rd_cnt;
	float Rd_Idx;
	float currentDMA;
	float hours;
	float min;
	float sec;
	float alt_max;
	float alt_min;
	float v_max;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	float 	hAcc;	//[m]
	uint32_t ts_LMessage;
	uint32_t dT;
	uint32_t debug;
	uint32_t Startup_ms;
	uint32_t velned_msg_cnt;
	uint32_t t_current;
	uint32_t t_last;
	uint32_t baudrate;
}GPS_T;
#pragma pack(pop)

//BMS
#pragma pack(push, 1)
typedef struct
{
	unsigned int charge_current;		//[mA]
	unsigned int discharge_current;		//[mA]
	unsigned int battery_voltage;		//[mV]
	unsigned int input_voltage;			//[mV]
	unsigned int input_current;			//[mA]
	unsigned int charging_state;
	unsigned int otg_voltage;			//[mV]
	unsigned int otg_current;			//[mV]
	unsigned int max_charge_current;	//[mA]
	signed int current;
	signed int discharged_capacity;		//[mAh]
	signed int old_capacity;			//[mAh] Old discharged capacity at previous shutdown
	unsigned int temperature;			//[mK]
	unsigned char com_err;
	unsigned char len;
	unsigned char crc;
}BMS_T;
#pragma pack(pop)

//SDIO
//TODO Add SDIO_BLOCKLEN to buffer defines.
#pragma pack(push, 1)
typedef struct
{
	unsigned char status;
	unsigned long response;
	unsigned int RCA;
	unsigned long LBAFATBegin;
	unsigned long FATSz;
	unsigned long FirstDataSecNum;
	unsigned long RootDirClus;
	unsigned char SecPerClus;
	unsigned char CardName[8];
	unsigned char err;
}SDIO_T;
#pragma pack(pop)

//filehandler
#pragma pack(push, 1)
typedef struct
{
	unsigned long id;
	unsigned char DirAttr;
	unsigned long DirCluster;
	unsigned long StartCluster;
	unsigned long size;
	unsigned int CurrentByte;
	char name[12];
	unsigned long buffer[128];
	unsigned long CurrentCluster;
	unsigned char CurrentSector;
}FILE_T;
#pragma pack(pop)

//System variables
#pragma pack(push, 1)
typedef struct
{
	unsigned int date;
	unsigned long time;
	signed char TimeOffset;
}SYS_T;
#pragma pack(pop)

// GUI CMD
#pragma pack(push, 1)
typedef struct
{
	uint8_t	 did;
	uint8_t  cmd;
	uint32_t timestamp;
}T_GUI_cmd;
#pragma pack(pop)

//Sensor information for logging
#pragma pack(push, 1)
typedef struct
{
	char* address;
	unsigned char size;
	unsigned char intervall;
}LOG_SENSOR_T;
#pragma pack(pop)

//Log information for logging
#pragma pack(push, 1)
typedef struct
{
	unsigned char ch_sensor_count;
	unsigned char ch_log_open;
	unsigned char max_intervall;
	char header[9*3];
}LOG_INFO_T;
#pragma pack(pop)

//States for md5 hash
#pragma pack(push, 1)
typedef struct
{
	unsigned char buff512[64];
	unsigned long state[4];
	unsigned long long message_length;
}MD5_T;
#pragma pack(pop)



#endif /* VARIABLES_H_ */
