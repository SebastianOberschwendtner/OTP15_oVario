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
#define CMD16		16
#define CMD17		17
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
#define SD_CARD_DETECTED	(1<<0)
#define SD_SDHC				(1<<1)
#define SD_CARD_SELECTED	(1<<2)
#define SD_IS_FAT16			(1<<3)

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


/*
 * Defines for erros
 */
#define SD_ERROR_WRONG_FILESYSTEM	0x01
#define SD_ERROR_WRONG_FAT			0x02
#define SD_ERROR_BAD_SECTOR			0x03
#define SD_ERROR_FAT_CORRUPTED		0x04


//*********** Functions **************
void init_sdio(void);
void sdio_set_clock(unsigned long l_clock);
void sdio_send_cmd(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_short(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_short_no_crc(unsigned char ch_cmd, unsigned long l_arg);
unsigned long sdio_send_cmd_long(unsigned char ch_cmd, unsigned long l_arg);
void sdio_select_card(void);
void sdio_read_block(unsigned long l_block_address);
void sdio_dma_receive(void);
unsigned char sdio_read_byte(unsigned int i_address);
unsigned int sdio_read_int(unsigned int i_address);
unsigned long sdio_read_long(unsigned int i_address);
void sdio_init_filesystem(void);
unsigned long sdio_get_lba(unsigned long l_cluster);
unsigned long sdio_get_fat_sec(unsigned long l_cluster, unsigned char ch_FATNum);
unsigned long sdio_get_fat_pos(unsigned long l_cluster);
unsigned long sdio_read_fat_pos(unsigned long l_pos);
unsigned long sdio_get_next_cluster(void);
void sdio_read_cluster(unsigned long l_cluster);
unsigned char sdio_read_next_cluster(void);
void sdio_get_name(unsigned long l_fileid);

#endif /* SDIO_H_ */
