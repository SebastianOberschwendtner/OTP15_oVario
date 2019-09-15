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
	MD5_T md5[IGC_HASH_NUMBER];
}IGCINFO_T;
#pragma pack(pop)

//ipc structs
SDIO_T* sd;
GPS_T* GpsData;
datafusion_T* BaroData;
T_keypad p_ipc_gui_keypad_data;

//private structs
FILE_T IGC;
IGCINFO_T IgcInfo;
T_command IgcCmd;

//Hash start keys
unsigned long const g_key[16] = {
		0x1C80A301,0x9EB30b89,0x39CB2Afe,0x0D0FEA76 ,
		0x48327203,0x3948ebea,0x9a9b9c9e,0xb3bed89a ,
		0x67452301,0xefcdab89,0x98badcfe,0x10325476 ,
		0xc8e899e8,0x9321c28a,0x438eba12,0x8cbe0aee };

/*
 * Igc task.
 * Systime has to be up to date.
 */

void igc_task(void)
{
	//Check commands
	while(ipc_get_queue_bytes(did_IGC) > 9) // look for new command in keypad queue
	{
		ipc_queue_get(did_IGC,10,&IgcCmd); 	// get new command
		switch(IgcCmd.cmd)					// switch for command
		{
		case cmd_igc_stop_logging:			// stop logging

			//Check if logging is ongoing
			if(IgcInfo.open == IGC_RECORDING)
				IgcInfo.open = IGC_LANDING;
			break;
		default:
			break;
		}
	}

	//Perform igc task
	switch(IgcInfo.open)
	{
	case IGC_CLOSED:

		/*
		 * If gps has a fix start logging -> this is detected if the time is not 0
		 * Do not start logging if no sd-card is inserted
		 */
		if(get_seconds_utc() && (sd->state & SD_CARD_DETECTED))
		{
			//Create log
			igc_create();

			set_led_green(ON);
			//If start of logging was successful
			if(IgcInfo.open != IGC_ERROR)
			{
				IgcInfo.open = IGC_RECORDING;

				//Send infobox
				IgcCmd.did 			= did_IGC;
				IgcCmd.cmd			= cmd_gui_set_std_message;
				IgcCmd.data 		= data_info_logging_started;
				IgcCmd.timestamp 	= TIM5->CNT;
				ipc_queue_push(&IgcCmd, 10, did_GUI);
			}
			else
			{
				//Send infobox
				IgcCmd.did 			= did_IGC;
				IgcCmd.cmd			= cmd_gui_set_std_message;
				IgcCmd.data 		= data_info_error;
				IgcCmd.timestamp 	= TIM5->CNT;
				ipc_queue_push(&IgcCmd, 10, did_GUI);
			}
		}
		break;
	case IGC_RECORDING:
		igc_BRecord();
		set_led_green(TOGGLE);
		break;
	case IGC_LANDING:
		igc_close();
		sdio_set_inactive();
		set_led_green(OFF);

		IgcInfo.open = IGC_FINISHED;
		//Send infobox
		IgcCmd.did 			= did_IGC;
		IgcCmd.cmd			= cmd_gui_set_std_message;
		IgcCmd.data 		= data_info_logging_stopped;
		IgcCmd.timestamp 	= TIM5->CNT;
		ipc_queue_push(&IgcCmd, 10, did_GUI);
		break;
	case IGC_FINISHED:
		break;
	case IGC_ERROR:
		break;
	default:
		break;
	}
};

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
 * init igc
 */
void init_igc(void)
{
	//get sd handler
	sd = ipc_memory_get(did_SDIO);
	//get gps handler
	GpsData = ipc_memory_get(did_GPS);
	//get the datafusion handler
	BaroData = ipc_memory_get(did_DATAFUSION);

	//Register the command queue
	ipc_register_queue(20, did_IGC);
}

/*
 * Create new IGC log
 */
void igc_create(void)
{
	//only create igc if sd-card is detected
	if(sd->state & SD_CARD_DETECTED)
	{
		sdio_read_root();

		//calculate name
		char igc_name[9] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00};
		sys_num2str(igc_name, get_year_utc(), 2);
		sys_num2str(igc_name+2, get_month_utc(),2);
		sys_num2str(igc_name+4, get_day_utc(),2);

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
			md5_initialize(&IgcInfo.md5[count], (unsigned long*)(&(g_key[4*count]))); //DONE Is the construct with the const_cast desirable? -> works good
		//Write header as required in FAI Chapter A3.2.4
		//Manufacturer ID
		igc_NewRecord('A');
		igc_AppendString(IGC_MANUF_ID);
		igc_AppendString(IGC_LOGGER_ID);
		igc_WriteLine();

		/*
		 * FR type
		 * This has to come second, because this mimics the XCSOAR behaviour and enables that commas are included in G-records.
		 * Very important! Otherwise the G-record will be invalid.
		 */
		igc_NewRecord('H');
		igc_AppendString("FFTYFRTYPE:");
		igc_AppendString(IGC_FR_TYPE);
		igc_WriteLine();

		//Log date and flight number
		igc_NewRecord('H');
		igc_AppendString("FDTE");
		igc_AppendNumber(get_day_utc(), 2);
		igc_AppendNumber(get_month_utc(), 2);
		igc_AppendNumber(get_year_utc(), 2);
		igc_AppendString(",");
		igc_AppendNumber(l_count, 2);
		igc_WriteLine();

		//Pilot in Charge
		igc_NewRecord('H');
		igc_AppendString("FPLTPILOTINCHARGE:");
		igc_AppendString(IGC_PILOT_NAME);
		igc_WriteLine();

		//Co-Pilot --> not yet applicable :P
		igc_NewRecord('H');
		igc_AppendString("FCM2CREW2:");
		igc_AppendString("None");
		igc_WriteLine();

		//Glider type
		igc_NewRecord('H');
		igc_AppendString("FGTYGLIDERTYPE:");
		igc_AppendString(IGC_GLIDER_TYPE);
		igc_WriteLine();

		//Glider ID
		igc_NewRecord('H');
		igc_AppendString("FGIDGLIDERID:");
		igc_AppendString(IGC_GLIDER_ID);
		igc_WriteLine();

		//GPS DATUM
		igc_NewRecord('H');
		igc_AppendString("FDTMGPSDATUM:WG84");
		igc_WriteLine();

		//Firmware version
		igc_NewRecord('H');
		igc_AppendString("FRFWFIRMWAREVERSION:");
		igc_AppendString(IGC_FIRMWARE_VER);
		igc_WriteLine();

		//Hardware version
		igc_NewRecord('H');
		igc_AppendString("FRHWHARDWAREVERSION:");
		igc_AppendString(IGC_HARDWARE_VER);
		igc_WriteLine();

		//GPS receiver
		igc_NewRecord('H');
		igc_AppendString("FGPSRECEIVER:");
		igc_AppendString(IGC_GPS_RX);
		igc_WriteLine();

		//barometer
		igc_NewRecord('H');
		igc_AppendString("FPRSPRESSALTSENSOR:");
		igc_AppendString(IGC_BARO_MANUF);
		igc_WriteLine();

		//add fix accuracy to B-record from byte 36 to byte 38
		igc_NewRecord('I');
		igc_AppendString("013638FXA");
		igc_WriteLine();

		//Write file
		sdio_write_file(&IGC);
	}
	else
		IgcInfo.open = IGC_ERROR;
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
		for(unsigned char count = 0; count < IGC_HASH_NUMBER; count++)
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
	 * Just to be sure in case the FAI or XCSOAR decide that the LF is a valid character.
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
		sys_num2str(&IgcInfo.linebuffer[IgcInfo.linepointer], number, digits);
		IgcInfo.linepointer += digits;
	}
};

/*
 * Write F-record. This records contains the current count of satellites used and their id.
 * TODO Add id support
 */
void igc_FRecord(void)
{
	igc_NewRecord('F');
	//Write UTC time
	igc_AppendNumber(get_hours_utc(), 2);
	igc_AppendNumber(get_minutes_utc(), 2);
	igc_AppendNumber(get_seconds_utc(), 2);
	for(unsigned char count = 0; count < GpsData->n_sat; count++)
		igc_AppendNumber(count, 2);
	igc_WriteLine();
};

/*
 * Write B-record. This records the gps fixes.
 */
void igc_BRecord(void)
{
	unsigned long TempInt = 0;
	float temp = 0;

	igc_NewRecord('B');
	//Write UTC time
	igc_AppendNumber(get_hours_utc(), 2);
	igc_AppendNumber(get_minutes_utc(), 2);
	igc_AppendNumber(get_seconds_utc(), 2);

	//Write latitude
	//DONE Compute coordinates in minutes and seconds
	if(GpsData->lat >= 0)
	{
		//degrees
		temp = GpsData->lat;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 2);

		//minutes
		temp = (temp-TempInt)*60000;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 5);

		igc_AppendString("N"); //DONE How do i know whether S or N? -> through the sign of the float
	}
	else
	{
		//degrees
		temp = GpsData->lat;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 2);

		//minutes
		temp = (temp-TempInt)*60000;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 5);

		igc_AppendString("S");
	}

	//Write longitude
	if(GpsData->lon >= 0)
	{
		//degrees
		temp = GpsData->lon;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 3);

		//minutes
		temp = (temp-TempInt)*60000;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 5);

		igc_AppendString("E"); //DONE How do i know whether W or E? -> through the sign of the float
	}
	else
	{

		//degrees
		temp = GpsData->lon;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 3);

		//minutes
		temp = (temp-TempInt)*60000;
		TempInt = (unsigned long)temp;
		igc_AppendNumber(TempInt, 5);

		igc_AppendString("W");
	}

	//Write fix validity
	if(GpsData->fix == 3)
		igc_AppendString("A");	//3D fix
	else
		igc_AppendString("V");	//2D fix or no GPS data

	//Write pressure altitude
	igc_AppendNumber((unsigned long)BaroData->height, 5);

	//Write GNSS altitude
	igc_AppendNumber((unsigned long)GpsData->height, 5);	//DONE Get height above ellipsoid

	//Write fix accuracy
	igc_AppendNumber((unsigned long)GpsData->hAcc, 3); //DONE Get real accuracy

	//Write line
	igc_WriteLine();
};

/*
 * Sign log with G-Record
 */
void igc_sign(void)
{
	//Finalize buffers
	for(unsigned char count = 0; count < IGC_HASH_NUMBER; count++)
		md5_finalize(&IgcInfo.md5[count]);

	//Get digest and write to file
	for(unsigned char count = 0; count < IGC_HASH_NUMBER; count++)
	{
		md5_GetDigest(&IgcInfo.md5[count], IgcInfo.linebuffer);

		//Every digest is split into n lines
		for(unsigned char LineCount = 0; LineCount < 32/IGC_DIGEST_CHARPERLINE; LineCount++)
		{
			//Write G-Record with IGC_DIGEST_CHARPERLINE characters until the complete digest is written.
			sdio_byte2file(&IGC, 'G');
			for(unsigned char character = 0; character < IGC_DIGEST_CHARPERLINE; character++)
				sdio_byte2file(&IGC, IgcInfo.linebuffer[character + LineCount*IGC_DIGEST_CHARPERLINE]);
			sdio_byte2file(&IGC, '\n');
		}
	}
};

/*
 * Close IGC log, sign it and write to sd-card.
 */
void igc_close(void)
{
	//Sign log
	igc_sign();

	//Write file
	sdio_write_file(&IGC);

	//Close file
	sdio_fclose(&IGC);

	//Set state
	IgcInfo.open = IGC_FINISHED;
};

/*
 * Plot current line in display for debugging
 */
//#include "DOGXL240.h"
//unsigned char linecount = 1;
//void igc_PlotLine(void)
//{
//	if(linecount == 17)
//	{
//		lcd_clear_buffer();
//		linecount = 1;
//	}
//	lcd_set_cursor(0, linecount*8);
//	for(unsigned char count= 0; count < IgcInfo.linepointer; count++)
//	{
//		if(count>39)
//			break;
//		lcd_char2buffer(IgcInfo.linebuffer[count]);
//	}
//	linecount++;
//	lcd_send_buffer();
//	wait_systick(5);
//};
