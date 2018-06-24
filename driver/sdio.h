/*
 * sdio.h
 *
 *  Created on: 19.05.2018
 *      Author: Sebastian
 */

#ifndef SDIO_H_
#define SDIO_H_

//*********** Includes **************
#include "stm32f4xx.h"
#include "oVario_Framework.h"
#include "did.h"
#include "ipc.h"
#include "error.h"
#include "Variables.h"

//*********** Defines **************
//Clocks
#define SDIOCLK		48000000UL

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
#define CMD7		7
#define CMD8		8
#define CMD13		13
#define CMD16		16
#define CMD17		17
#define CMD24		24
#define CMD55		55

#define ACMD6		6
#define ACMD41		41

//SD Comamnd bits
#define CMD8_VOLTAGE_0		(1<<8)

#define ACMD41_BUSY			(1<<31)		//Card indicates whether initialization is completed
#define ACMD41_CCS			(1<<30)		//Card Capacity Status
#define ACMD41_HCS			(1<<30)		//Host Capacity Support

//Response bits
#define R1_APP_CMD			(1<<5)		//Card will accept ACMD as next command
#define R1_ERROR			(1<<19)		//Generic error bit
#define R1_ILLEGAL_CMD		(1<<22)
#define R1_READY_4_DATA		(1<<8)


//SD Register bits
#define OCR_3_0V			(1<<17)

//Status bits
#define SD_CARD_DETECTED		(1<<0)
#define SD_SDHC					(1<<1)
#define SD_CARD_SELECTED		(1<<2)
#define SD_IS_FAT16				(1<<3)
#define SD_WAIT_FOR_TRANSMIT	(1<<4)

/*
 * defines for filesystem
 */
//MBR
#define MBR_MAGIC_NUMBER_pos	0x1FE	//int
#define MBR_PART1_TYPE_pos		0x1C2	//char
#define MBR_PART1_LBA_BEGIN_pos	0x1C6	//long

//EFI
#define EFI_TABLE_LBA_BEGIN_pos	0x48	//long
#define EFI_PART_LBA_BEGIN_pos	0x20	//long

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
#define SD_ERROR_WRONG_FILESYSTEM		0x01
#define SD_ERROR_WRONG_FAT				0x02
#define SD_ERROR_BAD_SECTOR				0x03
#define SD_ERROR_FAT_CORRUPTED			0x04
#define SD_ERROR_NO_SUCH_FILEID			0x05
#define SD_ERROR_NO_SUCH_FILE			0x06
#define SD_ERROR_NOT_A_DIR				0x07
#define SD_ERROR_NOT_A_FILE				0x08
#define SD_ERROR_NO_EMPTY_ENTRY			0x09
#define SD_ERROR_NO_EMPTY_CLUSTER		0x0A
#define SD_ERROR_TRANSFER_ERROR			0x0B
#define SD_ERROR_FILE_ALLREADY_EXISTS	0x0C

//*********** Functions **************
void init_sdio(void);
void sdio_set_clock(unsigned long l_clock);
void sdio_send_cmd(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_short(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_short_no_crc(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_long(unsigned char ch_cmd, unsigned long l_arg);
void sdio_select_card(void);
void sdio_read_block(unsigned long l_block_address);
void sdio_write_block(unsigned long l_block_address);
void sdio_set_wait(unsigned char ch_state);
void sdio_dma_receive(void);
void sdio_dma_transmit(void);
unsigned char sdio_read_byte(unsigned int i_address);
void sdio_write_byte(unsigned int i_address, unsigned char ch_data);
unsigned int sdio_read_int(unsigned int i_address);
void sdio_write_int(unsigned int i_address, unsigned int i_data);
unsigned long sdio_read_long(unsigned int i_address);
void sdio_write_long(unsigned int i_address, unsigned long l_data);
void sdio_init_filesystem(void);
unsigned long sdio_get_lba(unsigned long l_cluster);
unsigned long sdio_get_fat_sec(unsigned long l_cluster, unsigned char ch_FATNum);
unsigned long sdio_get_fat_pos(unsigned long l_cluster);
unsigned long sdio_read_fat_pos(unsigned long l_pos);
void sdio_write_fat_pos(unsigned long l_pos, unsigned long l_data);
unsigned long sdio_get_next_cluster(void);
void sdio_set_cluster(unsigned long l_cluster, unsigned long l_state);
void sdio_read_root(void);
void sdio_read_cluster(unsigned long l_cluster);
void sdio_write_current_sector(void);
unsigned char sdio_read_next_sector_of_cluster(void);
void sdio_get_name(char* pch_name, unsigned long l_id);
unsigned char sdio_strcmp(char* pch_string1, char* pch_string2);
unsigned char sdio_check_filetype(FILE_T* filehandler, char* pch_type);
void sdio_get_file(FILE_T* filehandler, unsigned long l_fileid);
unsigned long sdio_get_empty_id(void);
void sdio_set_empty_id(unsigned long l_id);
unsigned long sdio_get_empty_cluster(void);
unsigned long sdio_get_fileid(char* pch_name, char* pch_extension);
unsigned int sdio_get_date(void);
unsigned int sdio_get_time(void);
void sdio_cd(FILE_T* filehandler, char* pch_dirname);
void sdio_fopen(FILE_T* filehandler, char* pch_name, char* pch_extension);
void sdio_mkfile(char* pch_name, char* pch_filetype);
void sdio_mkdir(char* pch_name);
void sdio_rm(FILE_T* filehandler);

#endif /* SDIO_H_ */
