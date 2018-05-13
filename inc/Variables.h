/*
 * Variables.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_


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
}BMS_T;
#pragma pack(pop)



#endif /* VARIABLES_H_ */
