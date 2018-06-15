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


// Sound
#pragma pack(push, 1)
typedef struct
{
	uint8_t command;
	uint32_t data;
}T_sound_command;
#pragma pack(pop)

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
}datafusion_T;
#pragma pack(pop)


// MS5611
#pragma pack(push, 1)
typedef struct{
	int32_t pressure;
	int32_t temperature;
	uint32_t timestamp;
}ms5611_T;
#pragma pack(pop)


// GPS
#pragma pack(push, 1)
typedef struct{
	float speed_kmh;
	float heading_deg;
	uint8_t fix;
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
}BMS_T;
#pragma pack(pop)

//SDIO
#pragma pack(push, 1)
typedef struct
{
	unsigned long buffer[128];
	unsigned long response;
	unsigned long state;
	unsigned int RCA;
	unsigned long LBAFATBegin;
	unsigned long FATsSz;
	unsigned long ThisFATSecNum;
	unsigned long ThisFATEntOffset;
	unsigned long FirstRootDirSecNum;
	unsigned char SecPerClus;
	unsigned char err;
}SDIO_T;
#pragma pack(pop)

#endif /* VARIABLES_H_ */
