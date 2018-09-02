/*
 * igc.c
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */
#include "igc.h"
#include "Variables.h"

//ipc structs
SDIO_T* sd;

//private structs
FILE_T IGC;
MD5_T md5[4];

//Hash start keys
const unsigned long g_key[16] = {
		0x1C80A301,0x9EB30b89,0x39CB2Afe,0x0D0FEA76 ,
		0x48327203,0x3948ebea,0x9a9b9c9e,0xb3bed89a ,
		0x67452301,0xefcdab89,0x98badcfe,0x10325476 ,
		0xc8e899e8,0x9321c28a,0x438eba12,0x8cbe0aee };

/*
 * Check if current character is a valid igc character and therefore included in the grecord
 */
unsigned char igc_IsValidCharacter(unsigned char character)
{
	unsigned char valid = 0;

	//check for character range, the character 0x7E is excluded because it is a reserved character
	if((character < 0x7E) && (character >= 0x20))
		valid = 1;

	//check for reserved characters
	if(character == '$' || character == '*' || character == '!' || character == '\\' || character == '^')
		valid = 0;

	return valid;
};

/*
 * Check whether current record should be included in grecord or not
 */
unsigned char igc_IncludeInGrecord(char* in)
{
	unsigned char valid = false;

	switch (in[0]) {
	case 'L':
		if (sys_strcmp(IGC_CODE, in[1]))
			// only include L records made by XCS
			valid = true;
		break;

	case 'G':
		break;

	case 'H':
		if ((in[1] != 'O') && (in[1] != 'P'))
			valid = true;
		break;

	default:
		valid = true;
	}

	return valid;
};

/*
 * Create new IGC log
 */
void igc_create(void)
{
	//get sd handler
	sd = ipc_memory_get(did_SDIO);

	//only create igc if sd-card is detected
	if(sd->state & SD_CARD_DETECTED)
	{
		sdio_read_root();

		//calculate name
		char igc_name[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		sys_num2str(igc_name, get_day(), 2);
		sys_num2str(igc_name+2, get_month(),2);
		sys_num2str(igc_name+4, get_year(),2);

		//create file
		unsigned long l_count = 0;
		do
		{
			l_count++;
			sys_num2str(igc_name+6, l_count, 2);
		}while(sdio_get_fileid(&IGC, igc_name, "IGC"));
		sdio_mkfile(&IGC, igc_name, "IGC");
	}
};




