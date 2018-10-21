/*
 * Variables.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_

#define true 1
#define false 0


// ******* QUEUE MESSAGES *******
// KEYPAD
#define data_KEYPAD_pad_LEFT	0
#define data_KEYPAD_pad_DOWN	1
#define data_KEYPAD_pad_UP		2
#define data_KEYPAD_pad_RIGHT	3

// BMS
#define cmd_BMS_OTG_OFF			0
#define cmd_BMS_OTG_ON			1

// Sound
#define cmd_sound_set_frequ 	1
#define cmd_sound_set_vol		2
#define cmd_sound_set_louder 	3
#define cmd_sound_set_quieter 	4
#define cmd_sound_set_mute		5
#define cmd_sound_set_unmute	6
#define cmd_sound_set_beep 		7
#define cmd_sound_set_cont		8
#define cmd_sound_set_period	9

// Vario
#define cmd_vario_set_sinktone		0
#define cmd_vario_clear_sinktone	1
#define cmd_vario_toggle_sinktone	2

//IGC Logging
#define cmd_igc_stop_logging		1

//GUI
#define cmd_gui_eval_keypad			1
#define cmd_gui_set_std_message		2

//Standard messages for infobox
#define data_info_logging_started	1
#define data_info_logging_stopped	2
#define data_info_error				3


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
}datafusion_T;
#pragma pack(pop)


// MS5611
#pragma pack(push, 1)
typedef struct{
	int32_t pressure;
	int32_t temperature;
	uint32_t timestamp;
	uint8_t com_err;
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
	signed int discharged_capacity;		//[mA]
	unsigned int temperature;			//[degC]
	unsigned char com_err;
}BMS_T;
#pragma pack(pop)

//SDIO
//TODO Add SDIO_BLOCKLEN to buffer defines.
#pragma pack(push, 1)
typedef struct
{
	unsigned long response;
	unsigned long state;
	unsigned int RCA;
	unsigned long LBAFATBegin;
	unsigned long FATSz;
	unsigned long FirstDataSecNum;
	unsigned long RootDirClus;
	unsigned char SecPerClus;
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
