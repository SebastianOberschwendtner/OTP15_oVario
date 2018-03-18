/*
 * Variables.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_



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
	uint8_t volume;
	uint16_t frequency;
}T_sound_state;
#pragma pack(pop)





#pragma pack(push, 1)
typedef struct
{
	uint8_t	 pad;
	uint32_t timestamp;
}T_keypad;
#pragma pack(pop)


#endif /* VARIABLES_H_ */
