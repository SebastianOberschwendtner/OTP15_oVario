/*
 * igc.c
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */
#include "igc.h"

//Typedef for IGC
#pragma pack(push, 1)
typedef struct
{
    char linebuffer[77]; //Maximum record length + LF (see FAI Appendix A2.1)
    char hashbuffer[77]; //Buffer which contains all valid characters for the hash
    unsigned char linepointer;
    unsigned char hashpointer;
    unsigned char open;
    MD5_T md5[IGC_HASH_NUMBER];
} IGCINFO_T;
#pragma pack(pop)

//ipc structs
SDIO_T *sd;
GPS_T *GpsData;
datafusion_T *BaroData;
T_keypad p_ipc_gui_keypad_data;

//private structs
IGCINFO_T IgcInfo;
T_command rxcmd_igc;    //Command struct to receive ipc commands
T_command txcmd_igc;    //Command struct to sent ipc commands
TASK_T task_igc;

//Hash start keys
unsigned long const g_key[16] = {
    0x1C80A301, 0x9EB30b89, 0x39CB2Afe, 0x0D0FEA76,
    0x48327203, 0x3948ebea, 0x9a9b9c9e, 0xb3bed89a,
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,
    0xc8e899e8, 0x9321c28a, 0x438eba12, 0x8cbe0aee};

/*
 * register memory for igc
 */
void igc_register_ipc(void)
{
    //Initialize task struct
    arbiter_clear_task(&task_igc);
    arbiter_set_command(&task_igc, CMD_IDLE);

    //Initialize receive command struct
    rxcmd_igc.did           = did_IGC;
    rxcmd_igc.cmd           = 0;
    rxcmd_igc.data          = 0;
    rxcmd_igc.timestamp     = 0;

    //Initialize IGC data
    IgcInfo.open = IGC_LOG_CLOSED;

    //Register the command queue, holds 5 commands
    ipc_register_queue(5 * sizeof(T_command), did_IGC);
};

/*
 * Get everything relevant for IPC
 */
void igc_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
    //get sd handler
    sd = ipc_memory_get(did_SDIO);
    //get gps handler
    GpsData = ipc_memory_get(did_GPS);
    //get the datafusion handler
    BaroData = ipc_memory_get(did_DATAFUSION);
};

/***********************************************************
 * TASK IGC
 ***********************************************************
 * Systime has to be up to date!
 * 
 ***********************************************************
 * Execution:	interruptable
 * Wait: 		Yes
 * Halt: 		Yes
 **********************************************************/
void igc_task(void)
{
    //Check commands in queue
    igc_check_commands();

    //When the task wants to wait
    if(task_igc.wait_counter)
        task_igc.wait_counter--; //Decrease the wait counter
    else    //Execute task when wait is over
    {
        //Perform command action when the task does not wait for other tasks to finish
        if (task_igc.halt_task == 0)
        {
            //Perform action according to active state
            switch (arbiter_get_command(&task_igc))
            {
            case CMD_IDLE:
                igc_idle();
                break;

            case IGC_CMD_CREATE_LOG:
                igc_create_log();
                break;

            case IGC_CMD_CREATE_HEADER:
                igc_create_header();
                break;

            case IGC_CMD_WRITE_LOG:
                // igc_BRecord();
                break;

            case IGC_CMD_FINISH_LOG:
                igc_close();
                break;

            default:
                break;
            }
        }
    }
    // Check for errors
    // igc_check_error();
};

/*
 * Check the commands in the igc queue and call the corresponding command
 */
void igc_check_commands(void)
{
    //Check commands
    if (ipc_get_queue_bytes(did_IGC) >= 10) // look for new command in keypad queue
    {
        ipc_queue_get(did_IGC, sizeof(T_command), &rxcmd_igc); // get new command
        switch (rxcmd_igc.cmd)                  // switch for command
        {
        case cmd_ipc_signal_finished:
            //Called task is finished
            task_igc.halt_task -= rxcmd_igc.did;
            break;

        case cmd_igc_start_logging: // create logfile and start logging
            //Only when no log is active
            if(IgcInfo.open == IGC_LOG_CLOSED)
            {
                IgcInfo.open = IGC_LOG_BUSY;
                arbiter_set_command(&task_igc, IGC_CMD_CREATE_LOG);
            }
            break;

        case cmd_igc_stop_logging: // stop logging
            //Only when a log is active
            if (IgcInfo.open == IGC_LOGGING)
            {
                IgcInfo.open = IGC_LOG_BUSY;
                arbiter_set_command(&task_igc, IGC_CMD_FINISH_LOG);
            }
            break;

        default:
            break;
        }
    }
};

/***********************************************************
 * The idle state of the task.
 ***********************************************************
 *
 * It checks whether the logging state can be entered or not.
 * After a log file is created, the task automatically
 *  starts logging.
 * 
 * Argument:    none
 * Return:      none
 * 
 * call-by-reference
 ***********************************************************/
void igc_idle(void)
{
    //When the log file is created, start logging
    if (IgcInfo.open == IGC_LOG_OPEN)
    {
        //Log is now logging
        IgcInfo.open = IGC_LOGGING;
        //Got logging command
        arbiter_callbyreference(&task_igc, IGC_CMD_WRITE_LOG,0);
    }
};

/*
 * Check if current character is a valid igc character and therefore included in the grecord
 */
unsigned char igc_IsValidCharacter(unsigned char character)
{
    unsigned char valid = 0;

    //check for character range, the character 0x7E is excluded because it is a reserved character
    if ((character < 0x7E) && (character >= 0x20))
        valid = 1;

    //check for reserved characters
    if (character == '$' || character == '*' || character == '!' || character == '\\' || character == '^')
        valid = 0;

    return valid;
};

/*
 * Check whether current record should be included in grecord or not
 */
unsigned char igc_IncludeInGrecord(char *in)
{
    unsigned char valid = false;

    switch (in[0])
    {
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
 * Call a other task via the ipc queue
 */
void igc_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
    //Set the command and data for the target task
    txcmd_igc.did = did_IGC;
    txcmd_igc.cmd = cmd;
    txcmd_igc.data = data;

    //Push the command
    ipc_queue_push(&txcmd_igc, sizeof(T_command), did_target);

    //Set wait counter to wait for called task to finish
    task_igc.halt_task += did_target;
};

/*
 * Get the return value of the last finished ipc task which was called.
 */
inline unsigned long igc_get_call_return(void)
{
    return rxcmd_igc.data;
};

/***********************************************************
 * Create new IGC log.
 ***********************************************************
 * 
 * Return 1 when the log is created
 * 
 * Arguments:   none
 * Return:      unsigned long l_log_created
 * 
 * call-by-reference
 ***********************************************************/
void igc_create_log(void)
{
    //Allocate memory
    char *igc_name = (char *)arbiter_malloc(&task_igc, 3);

    //Log is not created yet
    IgcInfo.open = IGC_LOG_BUSY;

    //Perform the command action
    switch (arbiter_get_sequence(&task_igc))
    {
    case SEQUENCE_ENTRY:
        //Initialize filename
        sys_num2str(igc_name, get_year_utc(), 2);
        sys_num2str(igc_name + 2, get_month_utc(), 2);
        sys_num2str(igc_name + 4, get_day_utc(), 2);
        sys_num2str(igc_name + 6, task_igc.local_counter + 1, 2);
        igc_name[8]  = 'I';
        igc_name[9]  = 'G';
        igc_name[10] = 'C';
        igc_name[11] = 0x00;

        //Push command to search for file in ipc queue
        igc_call_task(SDIO_CMD_MKFILE, (unsigned long)igc_name, did_SDIO);

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_CREATE_FILENAME);
        break;

    case IGC_SEQUENCE_CREATE_FILENAME:
        //Check whether MKFILE is finished and created the file
        if (sd->status & SD_CMD_FINISHED)
        {
            if (igc_get_call_return())
            {
                //initialize hashes with custom keys
                for (unsigned char count = 0; count < 4; count++)
                    md5_initialize(&IgcInfo.md5[count], (unsigned long *)(&(g_key[4 * count])));

                //Tell the md5 task the address of the active string
                igc_call_task(cmd_md5_set_active_string,(unsigned long)IgcInfo.hashbuffer,did_MD5);
                
                //Goto next sequence
                arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
            }
            else
            {
                //Increase file counter
                task_igc.local_counter++;
                //File already exists, so start procedure again
                arbiter_set_sequence(&task_igc, SEQUENCE_ENTRY);
            }
        }
        break;

    case SEQUENCE_FINISHED:
        //Create the header of the file
        if (arbiter_callbyreference(&task_igc, IGC_CMD_CREATE_HEADER, 0))
        {
            //Log is created
            IgcInfo.open = IGC_LOG_OPEN;

            //Write file to sd-card
            igc_call_task(SDIO_CMD_WRITE_FILE, (unsigned long)ipc_memory_get(did_FILEHANDLER), did_SDIO);

            //exit command
            arbiter_return(&task_igc, 1);
        }

    default:
        break;
    }
};

/***********************************************************
 * Create the header of the igc file. 
 ***********************************************************
 * 
 * The local counter variable is used to determine the 
 * flight number.
 * Returns 1 when header is created.
 * 
 * Argument:    none
 * Return:      unsigned long   l_header_created
 * 
 * call-by-reference
 ***********************************************************/
void igc_create_header(void)
{
    //Perform Command action
    switch (arbiter_get_sequence(&task_igc))
    {
    case SEQUENCE_ENTRY:
        //initialize line buffer
        IgcInfo.linepointer = 0;
        IgcInfo.linebuffer[76] = 0x00;

        //Set next sequence and immediately enter it
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_1);

    case IGC_SEQUENCE_LINE_1:
        //Write header as required in FAI Chapter A3.2.4
        //Manufacturer ID
        igc_NewRecord('A');
        igc_AppendString(IGC_MANUF_ID);
        igc_AppendString(IGC_LOGGER_ID);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_2);
        break;

    case IGC_SEQUENCE_LINE_2:
        /*
            * FR type
            * This has to come second, because this mimics the XCSOAR behaviour and enables that commas are included in G - records.
            * Very important !Otherwise the G - record will be invalid.
            */
        igc_NewRecord('H');
        igc_AppendString("FFTYFRTYPE:");
        igc_AppendString(IGC_FR_TYPE);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_3);
        break;

    case IGC_SEQUENCE_LINE_3:
        //Log date and flight number
        igc_NewRecord('H');
        igc_AppendString("FDTE");
        igc_AppendNumber(get_day_utc(), 2);
        igc_AppendNumber(get_month_utc(), 2);
        igc_AppendNumber(get_year_utc(), 2);
        igc_AppendString(",");
        igc_AppendNumber(task_igc.local_counter + 1, 2);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_4);
        break;

    case IGC_SEQUENCE_LINE_4:
        //Pilot in Charge
        igc_NewRecord('H');
        igc_AppendString("FPLTPILOTINCHARGE:");
        igc_AppendString(IGC_PILOT_NAME);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_5);
        break;

    case IGC_SEQUENCE_LINE_5:
        //Co-Pilot --> not yet applicable :P
        igc_NewRecord('H');
        igc_AppendString("FCM2CREW2:");
        igc_AppendString("None");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_6);
        break;

    case IGC_SEQUENCE_LINE_6:
        //Glider type
        igc_NewRecord('H');
        igc_AppendString("FGTYGLIDERTYPE:");
        igc_AppendString(IGC_GLIDER_TYPE);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_7);
        break;

    case IGC_SEQUENCE_LINE_7:
        //Glider ID
        igc_NewRecord('H');
        igc_AppendString("FGIDGLIDERID:");
        igc_AppendString(IGC_GLIDER_ID);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_8);
        break;

    case IGC_SEQUENCE_LINE_8:
        //GPS DATUM
        igc_NewRecord('H');
        igc_AppendString("FDTMGPSDATUM:WG84");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_9);
        break;

    case IGC_SEQUENCE_LINE_9:
        //Firmware version
        igc_NewRecord('H');
        igc_AppendString("FRFWFIRMWAREVERSION:");
        igc_AppendString(IGC_FIRMWARE_VER);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_10);
        break;

    case IGC_SEQUENCE_LINE_10:
        //Hardware version
        igc_NewRecord('H');
        igc_AppendString("FRHWHARDWAREVERSION:");
        igc_AppendString(IGC_HARDWARE_VER);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_11);
        break;

    case IGC_SEQUENCE_LINE_11:
        //GPS receiver
        igc_NewRecord('H');
        igc_AppendString("FGPSRECEIVER:");
        igc_AppendString(IGC_GPS_RX);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_12);
        break;

    case IGC_SEQUENCE_LINE_12:
        //barometer
        igc_NewRecord('H');
        igc_AppendString("FPRSPRESSALTSENSOR:");
        igc_AppendString(IGC_BARO_MANUF);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_13);
        break;

    case IGC_SEQUENCE_LINE_13:
        //add fix accuracy to B-record from byte 36 to byte 38
        igc_NewRecord('I');
        igc_AppendString("013638FXA");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
        break;

    case SEQUENCE_FINISHED:
        //Exit command
        arbiter_return(&task_igc, 1);
        break;

    default:
        break;
    }
};

/*
 * Commit char to hash
 */
void igc_CommitCharacter(unsigned char character)
{
    //Check if character is valid
    if (igc_IsValidCharacter(character))
    {
        //Commit character to the hashbuffer
        IgcInfo.hashbuffer[IgcInfo.hashpointer++] = character;
    }
};

/*
 * Commit line of log to hash.
 * String has to be terminated with a LF (0x0A).
 */
void igc_CommitLine(char *line)
{
    //Counter for string
    unsigned char ch_count = 0;

	//Check if current record should be included in hash
	if(igc_IncludeInGrecord(line))
	{
		while(line[ch_count] != '\n')
		{
			//Check for end of string
			if(line[ch_count] == 0)
				break;
			//Commit the characters of the string
			igc_CommitCharacter(line[ch_count]);
			//Next character
			ch_count++;
		}
        //Terminate hash buffer
        IgcInfo.hashbuffer[IgcInfo.hashpointer++] = 0x00;

        //Tell md5 task to append the hash
        for (unsigned char i = 0; i < IGC_HASH_NUMBER; i++)
            igc_call_task(MD5_CMD_APPEND_STRING, (unsigned long)&IgcInfo.md5[i], did_MD5);
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
    IgcInfo.linebuffer[IgcInfo.linepointer++] = '\n';

    //Terminate line string
    IgcInfo.linebuffer[IgcInfo.linepointer++] = 0x00;

    //Write string to fileIgc
    igc_call_task(SDIO_CMD_STRING2FILE, (unsigned long)IgcInfo.linebuffer, did_SDIO);

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
    IgcInfo.linepointer = 1;
    IgcInfo.hashpointer = 0;
}
/*
 * Append string to linebuffer and current record
 */
void igc_AppendString(char *string)
{
    //Check if linebuffer is full
    if (IgcInfo.linepointer < 76)
    {
        while (*string != 0)
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
 * The sys_num2str function is used to save RAM and increase speed.
 */
void igc_AppendNumber(unsigned long number, unsigned char digits)
{
    //Check if the digits fit in the line buffer
    if (IgcInfo.linepointer < (76 - digits))
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
    for (unsigned char count = 0; count < GpsData->n_sat; count++)
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
    if (GpsData->lat >= 0)
    {
        //degrees
        temp = GpsData->lat;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 2);

        //minutes
        temp = (temp - TempInt) * 60000;
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
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("S");
    }

    //Write longitude
    if (GpsData->lon >= 0)
    {
        //degrees
        temp = GpsData->lon;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 3);

        //minutes
        temp = (temp - TempInt) * 60000;
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
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("W");
    }

    //Write fix validity
    if (GpsData->fix == 3)
        igc_AppendString("A"); //3D fix
    else
        igc_AppendString("V"); //2D fix or no GPS data

    //Write pressure altitude
    igc_AppendNumber((unsigned long)BaroData->height, 5);

    //Write GNSS altitude
    igc_AppendNumber((unsigned long)GpsData->height, 5); //DONE Get height above ellipsoid

    //Write fix accuracy
    igc_AppendNumber((unsigned long)GpsData->hAcc, 3); //DONE Get real accuracy

    //Write line
    igc_WriteLine();
};


/**********************************************************
 * Close IGC log, sign it and write to sd-card.
 **********************************************************
 * Returns 1 when the file is closed. Also sets the log
 * status entry.
 * 
 * Argument:    none
 * Return:      unsigned long l_log_closed
 * 
 * call-by-reference
 **********************************************************/
void igc_close(void)
{
    //Log is not yet closed
    IgcInfo.open = IGC_LOG_BUSY;

    //Allocate memory
    unsigned long* l_count      = arbiter_malloc(&task_igc,2);
    unsigned long* l_LineCount  = l_count + 1;
    
    //Perform the command action
    switch (arbiter_get_sequence(&task_igc))
    {
        case SEQUENCE_ENTRY:
            //Tell the md5 task to finalize the hashes
            for (unsigned char i = 0; i < IGC_HASH_NUMBER; i++)
                igc_call_task(MD5_CMD_FINALIZE, (unsigned long)&IgcInfo.md5[i], did_MD5);
            
            //Reset Hash counter
            *l_count     = 0;

            //Goto next sequence
            arbiter_set_sequence(&task_igc,IGC_SEQUENCE_GET_DIGEST);
            // arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_FILE);
            break;

        case IGC_SEQUENCE_GET_DIGEST:
            //Save digest to hashbuffer
            md5_GetDigest(&IgcInfo.md5[*l_count], IgcInfo.hashbuffer);

            //Reset line counter
            *l_LineCount = 0;

            //Goto directly to next sequence
            arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_DIGEST);

        case IGC_SEQUENCE_WRITE_DIGEST:
            //Create G-Record
            igc_NewRecord('G');
            //Write one junk of the digest
            for (unsigned char character = 0; character < IGC_DIGEST_CHARPERLINE; character++)
                IgcInfo.linebuffer[character + 1] = IgcInfo.hashbuffer[character + *l_LineCount * IGC_DIGEST_CHARPERLINE];

            //Terminate string with LF and zero afterwards
            IgcInfo.linebuffer[IGC_DIGEST_CHARPERLINE + 1] = '\n';
            IgcInfo.linebuffer[IGC_DIGEST_CHARPERLINE + 2] = 0x00;

            //Tell sdio task to write the new string
            igc_call_task(SDIO_CMD_STRING2FILE, (unsigned long)IgcInfo.linebuffer, did_SDIO);

            //Increase line counter
            *l_LineCount += 1;

            //check whether all hashes are written
            if (*l_LineCount == (32/IGC_DIGEST_CHARPERLINE))
            {
                //Current hash is written, goto next hash
                *l_count += 1;

                //Check whether all hashes are written
                if (*l_count == IGC_HASH_NUMBER)
                    arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_FILE);
                else
                    arbiter_set_sequence(&task_igc, IGC_SEQUENCE_GET_DIGEST);
            }
            break;

        case IGC_SEQUENCE_WRITE_FILE:
            //Write file to sd-card
            igc_call_task(SDIO_CMD_FCLOSE, 0, did_SDIO);

            //Goto next sequence
            arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
            break;

        case SEQUENCE_FINISHED:
            //Log is closed
            IgcInfo.open = IGC_LOG_CLOSED;

            //Exit command
            arbiter_return(&task_igc,1);
            break;
    }

};

/*
 * Return the current state of the IGC Logging.
 * This is done, because only for the state it seems not to be necessary
 * to register the 2.8kB of the whole IGC struct in the ipc memory.
 * Maybe it is also good, that the state is read-only this way...
 */
unsigned char igc_get_state(void)
{
    return IgcInfo.open;
}
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
