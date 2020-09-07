/*
 * sdio.h
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */

#ifndef SDIO_H_
#define SDIO_H_

//*********** Includes **************
#include "oVario_Framework.h"
#include "Variables.h"


//*********** Defines **************
//Clocks
#define SDIOCLK		    48000000UL

//Loop rate for sdio task
#define SDIO_TASK_us        50000
#define us2TASKTICKS(x)     (x/SDIO_TASK_us)
#define ms2TASKTICKS(x)     (x*1000/SDIO_TASK_us)

//Blocklength
#define SDIO_BLOCKLEN	512		//The blocklength isn't used everythere, so don't change it!!!
#define DBLOCKSIZE		9		//the power of the blocklen with the base 2 (2^9 = 512)

//Checksums
#define CHECK_PATTERN	0b10101
// SD Commands
#define CMD0		0
#define CMD2		2
#define CMD3		3
#define CMD4		4
#define CMD5		5
#define CMD6		6
#define CMD7		7		//Selected/deselect card
#define CMD8		8
#define CMD13		13
#define CMD15		15		//Set card inactive
#define CMD16		16
#define CMD17		17
#define CMD24		24
#define CMD55		55

#define ACMD6		6
#define ACMD41		41

//SD Comamnd bits
#define CMD8_VOLTAGE_0		(1<<8)		//Voltage Range 2.7V - 3.0V

#define ACMD41_BUSY			(1<<31)		//Card indicates whether initialization is completed
#define ACMD41_CCS			(1<<30)		//Card Capacity Status
#define ACMD41_HCS			(1<<30)		//Host Capacity Support
#define ACMD41_XPC			(1<<28)		//Power Control (0: 0.36W; 1: 0.54W)

//Response bits
#define R1_APP_CMD			(1<<5)		//Card will accept ACMD as next command
#define R1_ERROR			(1<<19)		//Generic error bit
#define R1_ILLEGAL_CMD		(1<<22)		//Illegal command
#define R1_READY_4_DATA		(1<<8)		//Card processed old data and is ready to receive new data


//SD Register bits
#define OCR_3_0V			(1<<17)

//Status bits
#define SD_CARD_DETECTED    (1<<0)	//Card inserted and responding
#define SD_SDHC				(1<<1)	//High Capacity Card
#define SD_CARD_SELECTED	(1<<2)	//Current card is selected and in transfer mode
#define SD_IS_FAT16			(1<<3)	//File system is FAT16, if not FAT32 is assumed
#define SD_IS_BUSY          (1<<4)  //Operation is on-going
#define SD_CMD_FINISHED     (1<<5)  //Operation finished successfully
#define SD_FILE_CREATED     (1<<6)  //File was created with MKDIR MKFILE

//Commands of sdio_task
#define SDIO_CMD_INIT                               1
#define SDIO_CMD_INIT_FILESYSTEM                    2
#define SDIO_CMD_READ_ROOT                          3
#define SDIO_CMD_GET_NEXT_CLUSTER                   4
#define SDIO_CMD_READ_NEXT_SECTOR_OF_CLUSTER        5
#define SDIO_CMD_GET_FILE                           6
#define SDIO_CMD_CLEAR_CLUSTER                      7
#define SDIO_CMD_SET_CLUSTER                        8
#define SDIO_CMD_GET_EMPTY_ID                       9
#define SDIO_CMD_SET_EMPTY_ID                       10
#define SDIO_CMD_GET_EMPTY_CLUSTER                  11
#define SDIO_CMD_GET_FILEID                         12
#define SDIO_CMD_FSEARCH                            13
#define SDIO_CMD_CD                                 14
#define SDIO_CMD_FOPEN                              15
#define SDIO_CMD_FCLOSE                             16
#define SDIO_CMD_SET_FILESIZE                       17
#define SDIO_CMD_MAKE_ENTRY                         18
#define SDIO_CMD_MKFILE                             19
#define SDIO_CMD_MKDIR                              20
#define SDIO_CMD_RM                                 21
#define SDIO_CMD_FCLEAR                             22
#define SDIO_CMD_WRITE_FILE                         23
#define SDIO_CMD_READ_ENDSECTOR_OF_FILE             24
#define SDIO_CMD_BYTE2FILE                          25
#define SDIO_CMD_INT2FILE                           26
#define SDIO_CMD_LONG2FILE                          27
#define SDIO_CMD_STRING2FILE                        28
#define SDIO_CMD_NUM2FILE                           29
#define SDIO_CMD_NO_CARD                            99

//Sequences of each command, the are in sequential number, so different commands can share the same sequence
//Command INIT:
#define SDIO_SEQUENCE_RESET_CARD                    1
#define SDIO_SEQUENCE_SET_SUPPLY                    2
#define SDIO_SEQUENCE_INIT_CARD_1                   3
#define SDIO_SEQUENCE_INIT_CARD_2                   4
#define SDIO_SEQUENCE_GET_RCA_1                     5
#define SDIO_SEQUENCE_GET_RCA_2                     6
#define SDIO_SEQUENCE_SELECT_CARD                   7
#define SDIO_SEQUENCE_SET_4WIRE_1                   8
#define SDIO_SEQUENCE_SET_4WIRE_2                   9
//Command INIT_FILESYSTEM:
#define SDIO_SEQUENCE_READ_EFI_1                    10
#define SDIO_SEQUENCE_READ_EFI_2                    11
#define SDIO_SEQUENCE_READ_BPB                      12
//Command READ_NEXT_SECTOR_OF_CLUSTER + (FCLEAR)
#define SDIO_SEQUENCE_READ_NEXT_SECTOR              13
#define SDIO_SEQUENCE_GET_NEXT_CLUSTER              14
//Command GET_FILE + GET_EMPTY_ID + SET_EMPTY_ID + GET_FILEID + FSEARCH + SET_FILESIZE
#define SDIO_SEQUENCE_READ_NEXT_SECTOR_OF_CLUSTER   15
//Command SET_CLUSTER
#define SDIO_SEQUENCE_SET_FAT_1                     16
#define SDIO_SEQUENCE_SET_FAT_2                     17
//Command GET_EMPTY_CLUSTER
#define SDIO_SEQUENCE_READ_FAT_UNTIL_END            18
//Command CD + FOPEN
#define SDIO_SEQUENCE_GET_FILEID                    19
#define SDIO_SEQUENCE_GET_FILE                      20
#define SDIO_SEQUENCE_READ_FIRST_CLUSTER            21
//Command MKFILE + MKDIR + (FCLEAR)
#define SDIO_SEQUENCE_READ_DIR_START_CLUSTER        22
#define SDIO_SEQUENCE_GET_EMPTY_CLUSTER             23
#define SDIO_SEQUENCE_GET_EMPTY_ID                  24
#define SDIO_SEQUENCE_MAKE_ENTRY                    25
#define SDIO_SEQUENCE_SET_CLUSTER                   26
#define SDIO_SEQUENCE_SET_NEXT_CLUSTER              27
#define SDIO_SEQUENCE_CLEAR_CLUSTER                 28
#define SDIO_SEQUENCE_READ_EMPTY_CLUSTER            29
#define SDIO_SEQUENCE_MAKE_ENTRY_DOT                30
#define SDIO_SEQUENCE_MAKE_ENTRY_DOTDOT             31
//Command RMFILE
#define SDIO_SEQUENCE_SET_EMPTY_ID                  32
#define SDIO_SEQUENCE_READ_FAT_POS                  33
//Command WRITE_FILE
#define SDIO_SEQUENCE_SET_FILESIZE                  34
#define SDIO_SEQUENCE_SET_EMPTY_CLUSTER             35

/*
 * defines for filesystem
 */
//MBR
#define MBR_MAGIC_NUMBER_pos	0x1FE	//int
#define MBR_PART1_TYPE_pos		0x1C2	//char
#define MBR_PART1_LBA_BEGIN_pos	0x1C6	//long

//EFI
#define EFI_TABLE_LBA_BEGIN_pos	0x48	//long
#define EFI_PART_LBA_BEGIN_pos	0x20	//long4

//BPB
#define BPB_BYTES_PER_SEC_pos	0x0B	//int
#define BPB_SEC_PER_CLUS_pos	0x0D	//char
#define BPB_RES_SEC_pos			0x0E	//int
#define BPB_NUM_FAT_pos			0x10	//byte
#define BPB_ROOT_ENT_CNT_pos	0x11	//int
#define BPB_TOT_SEC_16_pos		0x13	//int
#define BPB_FAT_SIZE_16_pos		0x16	//int
#define BPB_TOT_SEC_32_pos		0x20	//long
#define BPB_FAT_SIZE_32_pos		0x24	//long
#define BPB_ROOT_DIR_CLUS_pos	0x2C	//long

//DIR
#define DIR_ATTR_pos			0x0B	//char
#define DIR_CRT_TIME_pos		0x0E	//int
#define DIR_CRT_DATE_pos		0x10	//int
#define DIR_ACC_DATE_pos		0x12	//int
#define DIR_WRT_TIME_pos		0x16	//int
#define DIR_WRT_DATE_pos		0x18	//int
#define DIR_FstClusHI_pos		0x14	//int
#define DIR_FstClusLO_pos		0x1A	//int
#define DIR_FILESIZE_pos		0x1C	//long

//Attribute bits
#define DIR_ATTR_R				0x01
#define DIR_ATTR_HID			0x02
#define DIR_ATTR_SYS			0x04
#define DIR_ATTR_DIR			0x10
#define DIR_ATTR_ARCH			0x20

//Positions for date and time bits
#define FAT_DATE_YEAR_pos		9
#define FAT_DATE_MONTH_pos		5
#define FAT_DATE_DAY_pos		0

#define FAT_TIME_HOUR_pos		11
#define FAT_TIME_MINUTE_pos		5
#define FAT_TIME_SECONDS_pos	0

/*
 * Defines for erros
 */
#define SDIO_ERROR_WRONG_FILESYSTEM		    0x01
#define SDIO_ERROR_WRONG_FAT				0x02
#define SDIO_ERROR_BAD_SECTOR				0x03
#define SDIO_ERROR_FAT_CORRUPTED			0x04
#define SDIO_ERROR_NO_SUCH_FILEID			0x05
#define SDIO_ERROR_NO_SUCH_FILE			    0x06
#define SDIO_ERROR_NOT_A_DIR				0x07
#define SDIO_ERROR_NOT_A_FILE				0x08
#define SDIO_ERROR_NO_EMPTY_ENTRY			0x09
#define SDIO_ERROR_NO_EMPTY_CLUSTER		    0x0A
#define SDIO_ERROR_TRANSFER_ERROR			0x0B
#define SDIO_ERROR_FILE_ALLREADY_EXISTS 	0x0C
#define SDIO_ERROR_CORRUPT_FILESYSTEM       0x0D

//*********** Functions **************
void            sdio_register_ipc                   (void);
void            sdio_get_ipc                        (void);
void            sdio_task                           (void);
void            sdio_check_commands                 (void);
void            sdio_init                           (void);
void            sdio_init_filesystem                (void);
void            sdio_read_root                      (void); 
void            sdio_init_peripheral                (void);
void            sdio_set_clock                      (unsigned long l_clock);
void            sdio_check_error                    (void);
unsigned char   sdio_send_cmd                       (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_send_cmd_R1                    (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_send_cmd_R2                    (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_send_cmd_R3                    (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_send_cmd_R6                    (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_send_cmd_R7                    (unsigned char ch_cmd, unsigned long l_arg);
unsigned char   sdio_set_inactive                   (void);
unsigned char   sdio_read_block                     (unsigned long* databuffer, unsigned long l_block_address);
unsigned char   sdio_write_block                    (unsigned long* databuffer, unsigned long l_block_address);
void            sdio_dma_start_receive              (unsigned long *databuffer);
void            sdio_dma_start_transmit             (unsigned long* databuffer);
unsigned long   sdio_get_lba                        (unsigned long l_cluster);
unsigned char   sdio_read_cluster                   (FILE_T *filehandler, unsigned long l_cluster); 
void            sdio_clear_cluster                  (void);
void            sdio_set_cluster                    (void);
unsigned char   sdio_read_byte                      (unsigned long *databuffer, unsigned int i_address);
void            sdio_write_byte                     (unsigned long *databuffer, unsigned int i_address, unsigned char ch_data);
unsigned int    sdio_read_int                       (unsigned long* databuffer, unsigned int i_address);
void            sdio_write_int                      (unsigned long* databuffer, unsigned int i_address, unsigned int i_data);
unsigned long   sdio_read_long                      (unsigned long* databuffer, unsigned int i_address);
void            sdio_write_long                     (unsigned long* databuffer, unsigned int i_address, unsigned long l_data);
void            sdio_get_file                       (void);
void            sdio_read_next_sector_of_cluster    (void);
void            sdio_get_next_cluster               (void);
unsigned char   sdio_write_current_sector           (FILE_T* filehandler);
unsigned long   sdio_get_fat_sec                    (unsigned long l_cluster, unsigned char ch_FATNum);
unsigned long   sdio_get_fat_pos                    (unsigned long l_cluster);
unsigned long   sdio_read_fat_pos                   (unsigned long* databuffer,unsigned long l_pos);
void            sdio_get_name                       (FILE_T* filehandler);
void            sdio_write_fat_pos                  (unsigned long* databuffer, unsigned long l_pos, unsigned long l_data);
unsigned char   sdio_strcmp                         (char* pch_string1, char* pch_string2);
unsigned char   sdio_check_filetype                 (FILE_T* filehandler, char* pch_type);
unsigned int    sdio_get_date                       (void);
unsigned int    sdio_get_time                       (void);
void            sdio_get_empty_id                   (void);
void            sdio_set_empty_id                   (void);
void            sdio_get_empty_cluster              (void);
void            sdio_get_fileid                     (void);
void            sdio_fsearch                        (void);
void            sdio_cd                             (void);
void            sdio_fopen                          (void);
void            sdio_fclose                         (void);
void            sdio_make_entry                     (void);
void            sdio_mkfile                         (void);
void            sdio_mkdir                          (void);
void            sdio_rm                             (void);
void            sdio_set_filesize                   (void);
void            sdio_fclear                         (void);
void            sdio_write_file                     (void);
void            sdio_read_end_sector_of_file        (void);
void            sdio_byte2file                      (void);
void            sdio_int2file                       (void);
void            sdio_long2file                      (void);
void            sdio_string2file                    (void);
void            sdio_num2file                       (void);
#endif /* SDIO_H_ */
