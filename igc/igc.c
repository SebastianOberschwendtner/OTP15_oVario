/*
 * igc.c
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */
#include "igc.h"
#include "Variables.h"



//Typedef for IGC
#pragma pack(push, 1)
typedef struct
{
	char linebuffer[77];	//Maximum record length + LF (see FAI Appendix A2.1)
	unsigned char linepointer;
	unsigned char open;
	MD5_T md5[4];
}IGCINFO_T;
#pragma pack(pop)

//ipc structs
SDIO_T* sd;

//private structs
FILE_T IGC;
IGCINFO_T IgcInfo;

//Hash start keys
unsigned long const g_key[16] = {
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
		if (sys_strcmp(IGC_MANUF_ID, &in[1]))
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
		char igc_name[9] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00};
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

		//Set igc as open and terminate linebuffer
		IgcInfo.open = 1;
		IgcInfo.linepointer = 0;
		IgcInfo.linebuffer[76] = '\n';

		//initialize hashes with custom key
		for(unsigned char count = 0; count < 4; count++)
			md5_initialize(&IgcInfo.md5[count], (unsigned long*)(&(g_key[4*count]))); //TODO Is the construct with the const_cast desirable?
		//Write header
		//Manufacturer ID
		igc_NewRecord('A');
		igc_AppendString(IGC_MANUF_ID);
		igc_AppendString(IGC_LOGGER_ID);
		igc_WriteLine();

		//Log date
		igc_NewRecord('H');
		igc_AppendString("FDTEDATE");
		igc_AppendNumber(get_day(), 2);
		igc_AppendNumber(get_month(), 2);
		igc_AppendNumber(get_year(), 4);
		igc_WriteLine();

		sdio_write_file(&IGC);
		sdio_set_inactive();


	}
};

/*
 * Commit char to hash
 */
void igc_CommitCharacter(unsigned char character)
{
	//Check if character is valid
	if(igc_IsValidCharacter(character))
	{
		//Commit character to the hashes
		for(unsigned char count = 0; count < 4; count++)
			md5_append_char(&IgcInfo.md5[count], character);
	}
};

/*
 * Commit line of log to hash.
 * String has to be terminated with a LF (0x0A).
 */
void igc_CommitLine(char* line)
{
	//Check if current record should be included in hash
	if(igc_IncludeInGrecord(line))
	{
		while(*line != '\n')
		{
			//Check for end of string
			if(*line == 0)
				break;
			//Commit the characters of the string
			igc_CommitCharacter(*line);
			//Next character
			line++;
		}
	}
};

/*
 * Write line to file and commit to hash.
 * The LF at the end of the line is added automatically using the linepointer.
 */
void igc_WriteLine(void)
{
	/*
	 * Write LF at end of line and thus include it in the linecommit, although we know LF is an invalid character.
	 * Just to be sure in case the FAI or XCSOAR decide that the LF is valid character.
	 */
	IgcInfo.linebuffer[IgcInfo.linepointer] = '\n';
	IgcInfo.linepointer++;

	//Write string to file using the linepointer
	for(unsigned char count = 0; count < IgcInfo.linepointer; count++)
		sdio_byte2file(&IGC, IgcInfo.linebuffer[count]);

	//Commit line
	igc_CommitLine(IgcInfo.linebuffer);
};

/*
 * Add new record line to log.
 * The old line has to be finished with a LF.
 */
void igc_NewRecord(unsigned char type)
{
	IgcInfo.linepointer = 0;
	IgcInfo.linebuffer[0] = type;
	IgcInfo.linepointer=1;
}
/*
 * Append string to linebuffer and current record
 */
void igc_AppendString(char* string)
{
	//Check if linebuffer is full
	if(IgcInfo.linepointer < 76)
	{
		while(*string != 0)
		{
			IgcInfo.linebuffer[IgcInfo.linepointer] = *string;
			string++;
			IgcInfo.linepointer++;
		}
	}
	else
	{
		//linebuffer is full, just reset pointer => this will lead to datacorruption
		IgcInfo.linepointer = 0;
	}
};

/*
 * Append a number with a specific number of digits to the current record.
 * The sys_num2str function isn't used to save RAM and increase speed.
 */
void igc_AppendNumber(unsigned long number, unsigned char digits)
{
	//Check if the digits fit in the line buffer
	if(IgcInfo.linepointer < (76-digits))
	{
		//Compute the string content
		for(unsigned char ch_count = 0; ch_count<digits;ch_count++)
		{
			IgcInfo.linebuffer[IgcInfo.linepointer + digits - ch_count - 1] = (unsigned char)(number%10)+48;
			number /=10;
		}
		IgcInfo.linepointer += digits;
	}
};


