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

/*
 * defines for filesystem
 */
//MBR
#define MBR_MAGIC_NUMBER_pos	0x1FE
#define MBR_PART1_TYPE_pos		0x1C2
#define MBR_PART1_LBA_BEGIN_pos	0x1C6

//EFI
#define EFI_TABLE_LBA_BEGIN_pos	0x48
#define EFI_PART_LBA_BEGIN_pos	0x20


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
unsigned char sdio_read_byte(unsigned int i_adress);
unsigned int sdio_read_int(unsigned int i_adress);
unsigned long sdio_read_long(unsigned int i_adress);
void sdio_init_file(void);

#endif /* SDIO_H_ */
