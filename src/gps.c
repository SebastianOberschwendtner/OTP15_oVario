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

//****** Variables *******

uint8_t msg_buff[255];
uint8_t gps_state = gps_state_idle;
T_hdr_nmea *p_HDR_nmea = (void *)&msg_buff[0];
T_VTG_nmea act_VTG;

void *pDMABuff;
uint8_t pl_buf[50];

T_UBX_hdr *UBX_beg;

uint32_t Rd_Idx = 0;
uint32_t Rd_Cnt = 0;
uint8_t poll_cnt = 0;
volatile uint32_t Wr_Idx = 0;

uint32_t gps_baudrates[5] = {9600, 19200, 38400, 57600, 115200};
uint8_t baudcnt = 0;
uint8_t state_init = 0;

uint32_t velned_msg_cnt = 0;

//****** Puplic Variables ******
GPS_T *p_GPS_data;
SYS_T *sys;


//****** Functions ******

/**
 * @brief Register everything relevant for IPC
 */
void gps_register_ipc(void)
{
	//Register everything relevant for IPC
	// get memory
	pDMABuff = ipc_memory_register(dma_buf_size, did_GPS_DMA);
	p_GPS_data = ipc_memory_register(sizeof(GPS_T), did_GPS);
};

/**
 * @brief Get everything relevant for IPC
 */
void gps_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
	sys = ipc_memory_get(did_SYS);
};

uint8_t cnt = 0;
uint8_t decflag = 1;
uint8_t predec[10];
uint8_t dedec[10];
uint8_t predeccnt = 0;
uint8_t dedeccnt = 0;
uint8_t CFG_ACK = 3; // NACK = 0; ACK = 1; Nothing = 3
uint8_t deccnt = 0;

uint16_t currentDMA = 0;
uint16_t lastDMA = 0;

//uint8_t buff[50];

uint32_t TimVal1;

void *ptr = 0;

/**
 * @brief The gps task
 */
void gps_task(void)
{
	switch (gps_state)
	{
	case gps_state_idle:
		gps_state = gps_state_init;
		break;
	case gps_state_init:
		if (gps_init())
		{
			gps_state = gps_state_device_found;
		}
		break;
	case gps_state_device_found:
		TimVal1 = TIM5->CNT / 10;

		if (TIM5->CNT / 10 - p_GPS_data->ts_LMessage > 10000)
		{
			p_GPS_data->ts_LMessage = TIM5->CNT / 10;
			gps_state = gps_state_init;
			state_init = 0;
		}

		break;
	case gps_state_running:
		break;
	case gps_state_lost:
		gps_state = gps_state_idle;
		break;
	default:
		gps_state = gps_state_idle;
		break;
	}

	// Send Poll Request

	poll_cnt++;
	if ((poll_cnt > 50) & (gps_state == gps_state_device_found))
	{
		poll_cnt = 0;
		gps_poll_nav_stats();
	}

	// Something else
	volatile uint16_t msg_cnt = 0;
	volatile uint16_t remDataUnits = DMA_GetCurrDataCounter(DMA1_Stream1);
	currentDMA = dma_buf_size - remDataUnits; // get current pos of dma pointer

	Rd_Cnt += (currentDMA + dma_buf_size - lastDMA) % dma_buf_size; // Increase Rd_Cnt for additional bytes

	if (Rd_Cnt > dma_buf_size)
		Rd_Cnt = dma_buf_size; // Limit Rd_Cnt to DMA Buff Size

	lastDMA = currentDMA; // Save position of DMA counter for next round

	Rd_Idx = (dma_buf_size + currentDMA - Rd_Cnt) % dma_buf_size; // calc Rd_Idx position

	for (msg_cnt = 0; msg_cnt < Rd_Cnt; msg_cnt++)
	{
		UBX_beg = (T_UBX_hdr *)(pDMABuff + ((msg_cnt + Rd_Idx) % dma_buf_size)); // Put on Mask

		if (UBX_beg->sync == 0x62B5) // look for beginning of message
		{
			if ((Rd_Cnt - msg_cnt) >= (UBX_beg->length + 8)) // look for complete message in buffer
			{
				if (UBX_beg->length < 255) // look for length of message smaller as buffer
				{
					for (uint8_t mcnt = 0; mcnt < UBX_beg->length; mcnt++) // copy to buffer for debugging
					{
						msg_buff[mcnt] = *(uint8_t *)(pDMABuff + ((msg_cnt + Rd_Idx + mcnt + 6) % dma_buf_size));
					}

					Rd_Cnt--; // decrease read count

					// look which class in message
					switch (UBX_beg->class)
					{
					case ubx_class_nav:
						gps_handle_nav();
						p_GPS_data->ts_LMessage = TIM5->CNT / 10;
					case ubx_class_ack:
						gps_handle_ack();
						p_GPS_data->ts_LMessage = TIM5->CNT / 10;
						break;
					default:;
					}
				}
			}
			else
			{
				p_GPS_data->debug++;
				break;
			}
		}
		else
		{
			Rd_Cnt--; // decrease read count
		}
	}

	if (p_GPS_data->fix) // if fix -> set Sys Date and timezone
	{
		set_timezone();
		gps_SetSysTime();
		gps_SetSysDate();
	}

	p_GPS_data->Rd_Idx = (float)Rd_Idx;
	p_GPS_data->Rd_cnt = (float)Rd_Cnt;
	p_GPS_data->currentDMA = (float)currentDMA;

	// Min/Max values
	if (p_GPS_data->msl > p_GPS_data->alt_max)
		p_GPS_data->alt_max = p_GPS_data->msl;
	if (p_GPS_data->msl < p_GPS_data->alt_min)
		p_GPS_data->alt_min = p_GPS_data->msl;
	if (p_GPS_data->speed_kmh < p_GPS_data->v_max)
		p_GPS_data->v_max = p_GPS_data->speed_kmh;
};

/**
 * @brief initialize the gps
 */

uint8_t gps_init(void)
{
	switch (state_init)
	{
	case 0:
		UART_Init(gps_baudrates[baudcnt]);
		p_GPS_data->baudrate = gps_baudrates[baudcnt];

		// Initialize p_GPS_data
		p_GPS_data->Rd_Idx = 0;
		p_GPS_data->Rd_cnt = 0;
		p_GPS_data->currentDMA = 0;

		p_GPS_data->alt_max = 0;
		p_GPS_data->alt_min = 9999;
		p_GPS_data->v_max = 0;

		// Config GPS
		gps_config(gps_baudrates[baudcnt]);
		state_init++;
		break;
	case 1: // gps_wait_for_ack
		if (CFG_ACK == 1)
		{
			state_init = 2;
		}
		else if (CFG_ACK == 0)
		{
			state_init = 0; // resend cfg message
		}
		else
		{
			CFG_ACK++;

			if((CFG_ACK % 5) == 0)
			{
				gps_config(gps_baudrates[baudcnt]);
			}

			if (CFG_ACK > 50)
			{
				state_init = 0; // Reinit GPS
				baudcnt++;

				if (baudcnt > 4)
				{
					baudcnt = 0;
				}
			}
		}
		break;
	case 2:
		gps_config_rates();
		return 1;
		break;
	default:
		break;
	}
	// char log_hdg[] = {"HDG"};
	// char log_spd[] = {"SPD"};
	// log_include(&p_GPS_data->heading_deg, 4 ,1, &log_hdg[0]);
	// log_include(&p_GPS_data->speed_kmh, 4 ,1, &log_spd[0]);
	return 0;
};

/**
 * @brief Handle GPS ACK
 */
uint8_t gps_handle_ack(void)
{
	switch (UBX_beg->id)
	{
	case ubx_ID_ack:
		CFG_ACK = 1;
		return 1;
		break;
	case ubx_ID_nack:
		CFG_ACK = 0;
		return 0;
		break;
	default:
		break;
	}
	return 0;
}

/**
 * @brief Handle GPS NAV
 */

uint32_t dTimer = 0;

void gps_handle_nav(void)
{
	T_UBX_VELNED *pVN;
	T_UBX_SOL *pSOL;
	T_UBX_POSLLH *pPOS;
	T_UBX_TIMEUTC *pTIME;
	T_UBX_STATUS *pSTATS;

	switch (UBX_beg->id)
	{
	case ubx_ID_velned:
		pVN = (void *)&msg_buff[0];
		p_GPS_data->speed_kmh = (float)pVN->gspeed * 3.6 / 100;
		p_GPS_data->heading_deg = (float)pVN->heading / 100000;

		uint32_t temp = pVN->iTow / 1000;
		p_GPS_data->sec = (float)(temp % 60);
		p_GPS_data->min = (float)((temp - (uint32_t)p_GPS_data->sec) / 60 % 60);
		p_GPS_data->hours = (float)(((temp - (uint32_t)p_GPS_data->sec - (uint32_t)p_GPS_data->min * 60) / 3600) % 24);
		
		volatile uint32_t aTimer = TIM5->CNT / 10 ;
		p_GPS_data->dT = aTimer - dTimer;
		p_GPS_data->t_current = aTimer;
		p_GPS_data->t_last = dTimer;

		dTimer = aTimer;
		p_GPS_data->velned_msg_cnt = velned_msg_cnt++;
		
		break;
	case ubx_ID_sol:
		pSOL = (void *)&msg_buff[0];
		p_GPS_data->fix = pSOL->GPSFix;
		break;
	case ubx_ID_posllh:
		pPOS = (void *)&msg_buff[0];
		p_GPS_data->lat = ((float)pPOS->lat) / 10000000;
		p_GPS_data->lon = ((float)pPOS->lon) / 10000000;
		p_GPS_data->msl = ((float)pPOS->hMSL) / 1000;
		p_GPS_data->height = ((float)pPOS->height) / 1000;
		p_GPS_data->hAcc = ((float)pPOS->hAcc) / 1000;
		break;
	case ubx_ID_timeutc:
		pTIME = (void *)&msg_buff[0];
		p_GPS_data->year = pTIME->year;
		p_GPS_data->month = pTIME->month;
		p_GPS_data->day = pTIME->day;
		break;
	case ubx_ID_navstatus:
		pSTATS = (void *)&msg_buff[0];
		p_GPS_data->Startup_ms = pSTATS->msss;
		break;
	default:;
	}
};

/**
 * @brief Configure the gps
 */
void gps_config(uint32_t baudrate)
{

	wait_ms(100);

	T_CFG_PRT *cfgmsg = (void *)&pl_buf[0];

	cfgmsg->portID = 1;
	cfgmsg->txReady = 0;
	cfgmsg->Mode = 0x000008D0;
	cfgmsg->baudrate = baudrate;
	cfgmsg->inProtomask = 1;
	cfgmsg->outProtomask = 1;
	cfgmsg->flags = 0;

	gps_send_UBX(ubx_class_cfg, ubx_ID_cfg, sizeof(T_CFG_PRT), &pl_buf[0]);
	CFG_ACK = 3;
	wait_ms(100);
};

void gps_config_rates(void)
{
	// Set new rates for UBX messages

	wait_ms(100);
	gps_set_meas_rate(500);

	wait_ms(100);
	gps_set_msg_rate(ubx_class_nav, ubx_ID_posllh, 1);
	wait_ms(60);

	gps_set_msg_rate(ubx_class_nav, ubx_ID_timeutc, 2);
	wait_ms(60);

	gps_set_msg_rate(ubx_class_nav, ubx_ID_sol, 5);
	wait_ms(60);

	gps_set_msg_rate(ubx_class_nav, ubx_ID_velned, 1);
	wait_ms(60);
}

/**
 * @brief Set the message rate of the gps.
 * @param msg_class The class of the message.
 * @param msg_id The id of the message.
 * @param rate The new message rate.
 */
void gps_set_msg_rate(uint8_t msg_class, uint8_t msg_id, uint8_t rate)
{
	T_CFG_MSG *cfgmsg = (void *)&pl_buf[0];

	cfgmsg->msgClass = msg_class;
	cfgmsg->msgID = msg_id;
	cfgmsg->rate = rate;
	
	gps_send_UBX(ubx_class_cfg, ubx_ID_msg, sizeof(T_CFG_MSG), &pl_buf[0]);
};

/**
 * @brief Set the measurement rate of the gps.
 * @param rate The new measurement rate.
 */
void gps_set_meas_rate(uint16_t rate)
{
	T_CFG_RATE *cfgmsg = (void *)&pl_buf[0];

	cfgmsg->measRate = rate;
	cfgmsg->navRate = 1;
	cfgmsg->timeRef = 0;

	gps_send_UBX(ubx_class_cfg, ubx_ID_rate, sizeof(T_CFG_RATE), &pl_buf[0]);	
};

/**
 * @brief Get the measurement rate of the gps.
 */
void gps_get_meas_rate(void)
{
	gps_send_UBX(ubx_class_cfg, 8, 0, 0);
};

/**
 * @brief Send data to the gps.
 * @param ptr The pointer to the data to be sent.
 * @param no_bytes The number of bytes to be sent.
 */
void gps_send_bytes(void *ptr, uint8_t no_bytes)
{
	for (uint8_t i = 0; i < no_bytes; i++)
	{
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
			;										   // wait for TX Data empty
		USART_SendData(USART3, *(uint8_t *)(ptr + i)); // send data
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
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
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

void UART_Init(uint32_t Baudrate)
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
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Enable USART3 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = Baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	/* Enable USART3 */
	USART_Cmd(USART3, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	// Configure the Priority Group to 2 bits */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	// Enable the USART3 RX DMA Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Enable DMA2's AHB1 interface clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA1_Stream1);
	DMA_InitStructure.DMA_BufferSize = dma_buf_size;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable; //DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)pDMABuff;
	//	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&DMABuff[0];
	DMA_Init(DMA1_Stream1, &DMA_InitStructure);

	// Enable DMA Stream Transfer Complete interrupt */
	DMA_ITConfig(DMA1_Stream1, DMA_IT_TC, ENABLE);

	// Enable the USART Rx DMA request */
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

	// Enable the DMA RX Stream */
	DMA_Cmd(DMA1_Stream1, ENABLE);
}

void gps_poll_nav_stats(void)
{
	gps_send_UBX(ubx_class_nav, ubx_ID_navstatus, 0, 0);
}

void gps_send_UBX(uint8_t class, uint8_t ID, uint8_t pl_length, uint8_t *data)
{
	uint8_t cnt = 0;
	uint8_t buff[50];

	// Set MSG Rate
	buff[0] = 0xB5;	 // Sync
	buff[1] = 0x62;	 // Sync
	buff[2] = class; // Class
	buff[3] = ID;	 // ID

	buff[4] = pl_length; // Length
	buff[5] = 0x00;		 // Length

	if (data != 0)
	{
		while (cnt < pl_length)
		{
			cnt++;
			buff[5 + cnt] = *(data + cnt - 1); // meas rate
		}
	}

	// Checksum
	uint8_t CK_A = 0;
	uint8_t CK_B = 0;

	for (uint8_t I = 2; I < 6 + pl_length; I++)
	{
		CK_A = CK_A + buff[I];
		CK_B = CK_B + CK_A;
	}

	buff[5 + cnt + 1] = CK_A;
	buff[5 + cnt + 2] = CK_B;

	gps_send_bytes(buff, 6 + cnt + 2);
}