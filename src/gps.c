/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 ******************************************************************************
 * @file    gps.c
 * @author  JK
 * @version V1.1
 * @date    01-May-2018
 * @brief   Talks to the GPS and module and reads its data.
 ******************************************************************************
 */

//****** Includes ******
#include "gps.h"

//****** Typedefs ******
#pragma pack(push, 1)
typedef struct{
	uint8_t start;
	uint8_t	tID[2];
	uint8_t msgContID[3];
}T_hdr_nmea;

typedef struct{
	float course;
	float v_knots;
	float v_kmh;
	uint8_t fix;
	uint16_t checksum;
}T_VTG_nmea;

typedef struct{
	uint16_t sync;
	uint8_t class;
	uint8_t id;
	uint16_t length;
}T_UBX_hdr;

typedef struct{
	uint32_t 	iTow;		// GPS Millisecond Time of Week
	int32_t		velN;		// NED north velocity
	int32_t		velE;		// NED east velocity
	int32_t		velD;		// NED down velocity
	uint32_t 	speed;		// Speed (3-D)
	uint32_t 	gspeed;		// Ground Speed (2-D)
	int32_t 	heading;	// Heading of motion 2-D
	uint32_t 	sAcc;		// Speed Accuracy Estimate
	uint32_t 	cAcc;		// Course / Heading Accuracy Estimate
}T_UBX_VELNED;


typedef struct{
	uint32_t 	iTow;
	uint32_t 	fTow;
	uint16_t 	week;
	uint8_t		GPSFix;
}T_UBX_SOL;

typedef struct{
	uint32_t iTow;
	int32_t lon;
	int32_t lat;
	int32_t height;	// Above Ellipsoid
	int32_t hMSL;	// MSL
	uint32_t hAcc;
	uint32_t vAcc;
}T_UBX_POSLLH;

typedef struct{
	uint32_t 	iTOW;
	uint32_t 	tAcc;
	int32_t 	nano;
	uint16_t 	year;
	uint8_t 	month;
	uint8_t 	day;
	uint8_t 	hour;
	uint8_t 	min;
	uint8_t 	sec;
	uint8_t	 	valid;
}T_UBX_TIMEUTC;


typedef struct{
	uint16_t sync;
	uint8_t class;
	uint8_t id;
	uint16_t length;
	uint8_t portID;
	uint8_t reserved;
	uint16_t txReady;
	uint32_t Mode;
	uint32_t baudrate;
	uint16_t inProtomask;
	uint16_t outProtomask;
	uint16_t flags;
	uint16_t reserved2;
	uint8_t CK_A;
	uint8_t CK_B;
}T_CFG_PRT;

#pragma pack(pop)

//****** Variables *******

uint8_t msg_buff[255];
T_hdr_nmea* p_HDR_nmea = (void*)&msg_buff[0];
T_VTG_nmea act_VTG;

void *pDMABuff;
uint8_t DMABuff[dma_buf_size];

T_UBX_hdr *UBX_beg;

uint32_t			Rd_Idx = 0;
uint32_t			Rd_Cnt = 0;
volatile uint32_t 	Wr_Idx = 0;

//****** Puplic Variables ******
GPS_T *p_GPS_data;
SYS_T* sys;

//****** Functions ******

/**
 * @brief Register everything relevant for IPC
 */
void gps_register_ipc(void)
{
	//Register everything relevant for IPC
	// get memory
	pDMABuff 	= ipc_memory_register(dma_buf_size, did_GPS_DMA);
	p_GPS_data 	= ipc_memory_register(sizeof(GPS_T), did_GPS);
};

/**
 * @brief Get everything relevant for IPC
 */
void gps_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
	sys = ipc_memory_get(did_SYS);
};

/**
 * @brief initialize the gps
 */
void gps_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* USART IOs configuration ***********************************/
	/* Enable GPIOC clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* Connect PC10 to USART3_Tx */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);

	/* Connect PC11 to USART3_Rx*/
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

	/* Configure USART3_Tx and USART3_Rx as alternate function */
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	/* Enable USART3 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate 				= 9600;
	USART_InitStructure.USART_WordLength 			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits 				= USART_StopBits_1;
	USART_InitStructure.USART_Parity 				= USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl 	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode 					= USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	/* Enable USART3 */
	USART_Cmd(USART3, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Configure the Priority Group to 2 bits */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	// Enable the USART3 RX DMA Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel 						= DMA1_Stream1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd 					= ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	// Enable DMA2's AHB1 interface clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA1_Stream1);
	DMA_InitStructure.DMA_BufferSize 			= dma_buf_size;
	DMA_InitStructure.DMA_FIFOMode 				= DMA_FIFOMode_Enable;//DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold 		= DMA_FIFOThreshold_1QuarterFull;
	DMA_InitStructure.DMA_MemoryBurst			= DMA_MemoryBurst_Single ;
	DMA_InitStructure.DMA_MemoryDataSize 		= DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc 			= DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode 					= DMA_Mode_Circular;
	DMA_InitStructure.DMA_PeripheralBaseAddr 	= (uint32_t)&USART3->DR;
	DMA_InitStructure.DMA_PeripheralBurst 		= DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralDataSize 	= DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc 		= DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority 				= DMA_Priority_High;
	DMA_InitStructure.DMA_Channel 				= DMA_Channel_4;
	DMA_InitStructure.DMA_DIR 					= DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)pDMABuff;
	//	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&DMABuff[0];
	DMA_Init(DMA1_Stream1,&DMA_InitStructure);

	// Enable DMA Stream Transfer Complete interrupt */
	DMA_ITConfig(DMA1_Stream1, DMA_IT_TC, ENABLE);

	// Enable the USART Rx DMA request */
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

	// Enable the DMA RX Stream */
	DMA_Cmd(DMA1_Stream1, ENABLE);


	// Initialize p_GPS_data
	p_GPS_data->Rd_Idx 	= 0;
	p_GPS_data->Rd_cnt = 0;
	p_GPS_data->currentDMA = 0;

	p_GPS_data->alt_max = 0;
	p_GPS_data->alt_min = 9999;
	p_GPS_data->v_max 	= 0;


	// Config GPS
	gps_config();

	// char log_hdg[] = {"HDG"};
	// char log_spd[] = {"SPD"};
	// log_include(&p_GPS_data->heading_deg, 4 ,1, &log_hdg[0]);
	// log_include(&p_GPS_data->speed_kmh, 4 ,1, &log_spd[0]);
};

uint8_t cnt 		= 0;
uint8_t decflag 	= 1;
uint8_t predec[10];
uint8_t dedec[10];
uint8_t predeccnt = 0;
uint8_t dedeccnt = 0;

uint8_t deccnt = 0;

uint16_t currentDMA = 0;
uint16_t lastDMA = 0;


uint8_t buff[50];


void* ptr = 0;

/**
 * @brief The gps task
 */
void gps_task(void)
{
	// Something else
	volatile uint16_t msg_cnt = 0;

	currentDMA = dma_buf_size - DMA_GetCurrDataCounter(DMA1_Stream1);	// get current pos of dma pointer

	Rd_Cnt += (currentDMA + dma_buf_size - lastDMA) % dma_buf_size;		// Increase Rd_Cnt for additional bytes

	if(Rd_Cnt > dma_buf_size) Rd_Cnt = dma_buf_size;					// Limit Rd_Cnt to DMA Buff Size

	lastDMA = currentDMA;												// Save position of DMA counter for next round

	Rd_Idx = (dma_buf_size + currentDMA - Rd_Cnt) % dma_buf_size;		// calc Rd_Idx position


	for(msg_cnt = 0; msg_cnt < Rd_Cnt; msg_cnt++)
	{
		UBX_beg = (T_UBX_hdr*)(pDMABuff + ((msg_cnt + Rd_Idx) % dma_buf_size));		// Put on Mask

		if(UBX_beg->sync == 0x62B5)													// look for beginning of message
		{
			if((Rd_Cnt - msg_cnt) > (UBX_beg->length + 8))							// look for complete message in buffer
			{
				if(UBX_beg->length < 255)											// look for length of message smaller as buffer
				{
					for(uint8_t mcnt = 0; mcnt < UBX_beg->length; mcnt++)			// copy to buffer for debugging
					{
						msg_buff[mcnt] = *(uint8_t*)(pDMABuff + ((msg_cnt + Rd_Idx + mcnt + 6) % dma_buf_size));
					}

					Rd_Cnt--;														// decrease read count

					// look which class in message
					switch(UBX_beg->class)
					{
					case nav:
						gps_handle_nav();
						break;
					default:
						;
					}
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			Rd_Cnt--;																// decrease read count
		}
	}

	if(p_GPS_data->fix) // if fix -> set Sys Date and timezone
	{
		set_timezone();
		gps_SetSysTime();
		gps_SetSysDate();
	}

	p_GPS_data->Rd_Idx 		= (float)Rd_Idx;
	p_GPS_data->Rd_cnt 		= (float)Rd_Cnt;
	p_GPS_data->currentDMA 	= (float)currentDMA;

	// Min/Max values
	if(p_GPS_data->msl > p_GPS_data->alt_max) 		p_GPS_data->alt_max 	= p_GPS_data->msl;
	if(p_GPS_data->msl < p_GPS_data->alt_min) 		p_GPS_data->alt_min 	= p_GPS_data->msl;
	if(p_GPS_data->speed_kmh < p_GPS_data->v_max) 	p_GPS_data->v_max 		= p_GPS_data->speed_kmh;
};

/**
 * @brief Handle GPS NAV
 */
void gps_handle_nav(void)
{
	T_UBX_VELNED* pVN;
	T_UBX_SOL* pSOL;
	T_UBX_POSLLH* pPOS;
	T_UBX_TIMEUTC* pTIME;

	switch(UBX_beg->id)
	{
	case velned:
		pVN = (void*)&msg_buff[0];
		p_GPS_data->speed_kmh 	= (float)pVN->gspeed * 3.6/100;
		p_GPS_data->heading_deg = (float)pVN->heading / 100000;

		uint32_t temp = pVN->iTow/1000;
		p_GPS_data->sec 	= (float)(temp % 60);
		p_GPS_data->min 	= (float)((temp - (uint32_t)p_GPS_data->sec) / 60 % 60);
		p_GPS_data->hours   = (float)(((temp - (uint32_t)p_GPS_data->sec - (uint32_t)p_GPS_data->min * 60) / 3600 ) % 24);
		break;
	case sol:
		pSOL = (void*)&msg_buff[0];
		p_GPS_data->fix = pSOL->GPSFix;
		break;
	case posllh:
		pPOS = (void*)&msg_buff[0];
		p_GPS_data->lat 	= ((float)pPOS->lat) / 10000000;
		p_GPS_data->lon 	= ((float)pPOS->lon) / 10000000;
		p_GPS_data->msl 	= ((float)pPOS->hMSL) / 1000;
		p_GPS_data->height  = ((float)pPOS->height) / 1000;
		p_GPS_data->hAcc 	= ((float)pPOS->hAcc) / 1000;
		break;
	case timeutc:
		pTIME = (void*)&msg_buff[0];
		p_GPS_data->year 	= pTIME->year;
		p_GPS_data->month 	= pTIME->month;
		p_GPS_data->day 	= pTIME->day;
		break;
	default:
		;
	}
};

/**
 * @brief Configure the gps
 */
void gps_config(void)
{
	// Set new baudrate
	//const unsigned baudrates[] = {9600, 19200, 38400, 57600, 115200};

	wait_ms(100);
	//gps_set_baud(baudrates[j]);

	T_CFG_PRT* cfgmsg = (void*)&buff[0];
	cfgmsg->sync 			= 0x62B5;
	cfgmsg->class 			= cfg;
	cfgmsg->id 				= id_cfg;
	cfgmsg->length 			= 20;
	cfgmsg->portID 			= 1;
	cfgmsg->txReady 		= 0;
	cfgmsg->Mode 			= 0x000008D0;
	cfgmsg->baudrate 		= 9600;
	cfgmsg->inProtomask 	= 1;
	cfgmsg->outProtomask 	= 1;
	cfgmsg->flags 			= 0;

	// Calc Checksum
	uint8_t CK_A 	= 0;
	uint8_t CK_B 	= 0;

	for(uint8_t I = 2; I < 26; I++)
	{
		CK_A = CK_A + buff[I];
		CK_B = CK_B + CK_A;
	}

	cfgmsg->CK_A 			= CK_A;
	cfgmsg->CK_B 			= CK_B;


	gps_send_bytes(buff, 28);
	wait_ms(100);

	// Set new rates for UBX messages
	wait_ms(100);
	gps_set_msg_rate(nav, posllh, 1);
	wait_ms(60);

	gps_set_msg_rate(nav, timeutc, 1);
	wait_ms(60);

	//	gps_set_msg_rate(nav, clock, 1);
	//	wait_ms(60);
	//
	gps_set_msg_rate(nav, sol, 1);
	wait_ms(60);

	gps_set_msg_rate(nav, velned, 1);
	wait_ms(60);

	//	gps_set_msg_rate(nav, svinfo, 1);
	//	wait_ms(60);

	//	gps_set_msg_rate(mon, hw, 1);
	//	wait_ms(60);
};

/**
 * @brief Set the message rate of the gps.
 * @param msg_class The class of the message.
 * @param msg_id The id of the message.
 * @param rate The new message rate.
 */
void gps_set_msg_rate(uint8_t msg_class, uint8_t msg_id, uint8_t rate)
{

	// Set MSG Rate
	buff[0] = 0xB5;			// Sync
	buff[1] = 0x62;			// Sync
	buff[2] = 0x06;			// Class
	buff[3] = 0x01;			// ID

	buff[4] = 3;			// Length
	buff[5] = 0x00;			// Length

	buff[6] = msg_class;	// msg class
	buff[7] = msg_id;		// msg id

	buff[8] = rate;			// rate


	// Checksum
	uint8_t CK_A 	= 0;
	uint8_t CK_B 	= 0;

	for(uint8_t I = 2; I < 9 ; I++)
	{
		CK_A = CK_A + buff[I];
		CK_B = CK_B + CK_A;
	}

	buff[9] = CK_A;
	buff[10] = CK_B;

	gps_send_bytes(buff, 11);
};

/**
 * @brief Send data to the gps.
 * @param ptr The pointer to the data to be sent.
 * @param no_bytes The number of bytes to be sent.
 */
void gps_send_bytes(void* ptr, uint8_t no_bytes)
{
	for(uint8_t i = 0; i < no_bytes; i++)
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);	// wait for TX Data empty
		USART_SendData(USART3, *(uint8_t*)(ptr + i));					// send data
	}
};

/**
 * @brief DMAInterrupt Handling
 * @details interrupt handler
 */
void DMA1_Stream1_IRQHandler(void) // USART3_RX
{
	if (DMA_GetITStatus(DMA1_Stream1, DMA_IT_FEIF1))
	{
		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_FEIF1);
	}

	if (DMA_GetITStatus(DMA1_Stream1, DMA_IT_HTIF1))
	{
		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_HTIF1);
	}

	/* Test on DMA Stream Transfer Complete interrupt */
	if (DMA_GetITStatus(DMA1_Stream1, DMA_IT_TCIF1))
	{
		Wr_Idx = (Wr_Idx + dma_buf_size / 4) % dma_buf_size;

		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_TCIF1);
	}
};

/**
 * @brief Set the baudrate of the gps.
 * @param baud The new baudrate
 */
void gps_set_baud(unsigned int baud)
{
	USART_Cmd(USART3, DISABLE);

	//USART_DeInit(USART3);
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate 				= baud;
	USART_InitStructure.USART_WordLength 			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits 				= USART_StopBits_1;
	USART_InitStructure.USART_Parity 				= USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl 	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode 					= USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	USART_Cmd(USART3, ENABLE);
};

/**
 * @brief This function sets the system time according to the gps time.
 * Timezone is UTC.
 */
void gps_SetSysTime(void)
{
	//Hour
	unsigned char ch_hour = (unsigned char)(p_GPS_data->hours);
	//Minutes
	unsigned char ch_minute = (unsigned char)(p_GPS_data->min);
	//Seconds
	unsigned char ch_second = (unsigned char)(p_GPS_data->sec);
	set_time(ch_hour, ch_minute, ch_second);
};

/**
 * @brief This function sets the system date according to the gps date.
 */
void gps_SetSysDate(void)
{
	//Year
	unsigned int i_year = (unsigned int)(p_GPS_data->year);
	//Month
	unsigned char ch_month = (unsigned char)(p_GPS_data->month);
	//Day
	unsigned char ch_day = (unsigned char)(p_GPS_data->day);
	set_date(ch_day, ch_month, i_year);
};
