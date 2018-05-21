/*
 * gps.c
 *
 *  Created on: 01.05.2018
 *      Author: Admin
 */

#include "gps.h"

#include "stm32f4xx.h"
#include "ipc.h"
#include "did.h"
#include "oVario_Framework.h"
#include "Variables.h"
//Git ist scheiße!
// Variables
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

#pragma pack(pop)




uint8_t msg_buff[255];
T_hdr_nmea* p_HDR_nmea = (void*)&msg_buff[0];
T_VTG_nmea act_VTG;


GPS_T *p_GPS_data;

void *pDMABuff;
uint8_t DMABuff[dma_buf_size];



uint32_t			Rd_Idx = 0;
uint32_t			Rd_Cnt = 0;
volatile uint32_t 	Wr_Idx = 0;

uint8_t test1, test2, test3, test4, test5;


float value = 0;
float value2 = 1000;




// ***** Functions *****

void gps_init (){

	pDMABuff = ipc_memory_register(dma_buf_size, did_GPS);


	//Sbus_data = (struct Ausgabe*)Sbus_Buffer;

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

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


	/* USART configuration ***************************************/
	/* USART3 configured as follow:
		- BaudRate = 9600 baud
		- Word Length = 8 Bits
		- 1 Stop Bit
		- Even parity
		- Hardware flow control disabled (RTS and CTS signals)
		- Receive and transmit enabled
	 */
	/* Enable USART3 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
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
	DMA_InitStructure.DMA_FIFOThreshold 		= DMA_FIFOThreshold_Full;
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
	//	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)pDMABuff;
	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&DMABuff[0];
	DMA_Init(DMA1_Stream1,&DMA_InitStructure);

	// Enable DMA Stream Transfer Complete interrupt */
	DMA_ITConfig(DMA1_Stream1, DMA_IT_TC, ENABLE);

	// Enable the USART Rx DMA request */
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

	// Enable the DMA RX Stream */
	DMA_Cmd(DMA1_Stream1, ENABLE);


	p_GPS_data = ipc_memory_register(50, did_GPS);

}



uint8_t cnt 		= 0;
//uint8_t new_flag 	= 0;
uint8_t decflag 	= 1;
uint8_t predec[5];
uint8_t dedec[5];
uint8_t predeccnt = 0;
uint8_t dedeccnt = 0;

uint8_t deccnt = 0;
uint8_t gnflag = 0;

void gps_task ()
{
	uint16_t msg_cnt = 0;
	uint16_t msg_cnt2 = 0;

	while(msg_cnt <= Rd_Cnt)
	{
		if((*((uint8_t*)pDMABuff + (Rd_Idx + msg_cnt) % dma_buf_size)) == '$')	// find beginning of message
		{
			msg_cnt2 = msg_cnt;
			while(msg_cnt2 <= Rd_Cnt)
			{

				if((*((uint8_t*)pDMABuff + (Rd_Idx + msg_cnt2) % dma_buf_size)) == 0x0A)	// find end of message
				{
					test1 = *((uint8_t*)pDMABuff + (msg_cnt + Rd_Idx) % dma_buf_size);
					test2 = *((uint8_t*)pDMABuff + (msg_cnt2 + Rd_Idx) % dma_buf_size);

					// Parse message
					for (uint8_t cnt = 0; cnt < (msg_cnt2 - msg_cnt + 1); cnt++)	// copy message to buffer
					{
						msg_buff[cnt] = *((uint8_t*)pDMABuff + (msg_cnt + Rd_Idx + cnt)%dma_buf_size);
					}

					if((p_HDR_nmea->msgContID[0]== 'V') &&
							(p_HDR_nmea->msgContID[1]== 'T') &&
							(p_HDR_nmea->msgContID[2]== 'G'))
					{
						test3 = p_HDR_nmea->msgContID[0];
						test4 = p_HDR_nmea->msgContID[1];
						test5 = p_HDR_nmea->msgContID[2];



						cnt = 6;
						while((msg_buff[cnt] != '*')&&(cnt < 255)) 					// Solange vor Checksumme
						{

							if(msg_buff[cnt] == ',') // Wenn Komma, �berspringen
							{
								;
							}
							else if((msg_buff[cnt] >= 48)&&(msg_buff[cnt] <= 57)) // Wenn Zahl, dann hole Zahl
							{
								decflag = 1;
								predeccnt = 0;
								dedeccnt = 0;
								while(msg_buff[cnt] != ',')
								{
									if(msg_buff[cnt] == '.')
									{
										decflag = 0;
									}
									else
									{
										if(decflag == 1)
										{
											predec[predeccnt] = msg_buff[cnt]; 			// Vorkommastellen
											predeccnt++;
										}
										else
										{
											dedec[dedeccnt] = msg_buff[cnt];			// Nachkommastellen
											dedeccnt++;
										}
									}

									cnt++;

								}
								// Zahl done
								uint32_t pow = 10;
								value = 0;
								value2 = 0;

								for(uint8_t fcnt = 0; fcnt < predeccnt; fcnt++)
								{
									uint8_t temptest2 = predec[predeccnt - fcnt - 1]-48;
									value2 += ((float)temptest2)*(pow/10);
									pow *= 10;
								}

								pow = 10;

								for(uint8_t fcnt2 = 0; fcnt2 < dedeccnt; fcnt2++)
								{
									uint8_t temptest = dedec[fcnt2]-48;
									value += ((float)temptest)/(pow);
									pow *= 10;
								}

								value = value + value2;
								set_led_green(TOGGLE);

							}
							else if((msg_buff[cnt] >= 64)) // If Buchstabe, do something special
							{
								uint8_t id = msg_buff[cnt];
								switch(id)
								{
								case 'K':
									p_GPS_data->speed_kmh = value;
									break;
								case 'T':
									if(value <900)
										p_GPS_data->heading_deg = value;
									break;
								case 'A':
									p_GPS_data->fix = true;
									break;
								case 'V':
									p_GPS_data->fix = false;
									break;

								default:
									break;
								}
								value = 999;

							}

							cnt++;
						}
					}


					Rd_Idx = (Rd_Idx + msg_cnt2  + 1) % dma_buf_size;
					msg_cnt = 0;
					msg_cnt2 = 0;

					Rd_Cnt = Rd_Cnt - msg_cnt2;// if message found, decrease rd cnt
					break;
				}
				msg_cnt2++;
			}
			break;
		}
		msg_cnt++;
	}
}




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
		Rd_Cnt = (Rd_Cnt + dma_buf_size / 4);

		if(Rd_Cnt > dma_buf_size)
		{
			Rd_Cnt = dma_buf_size;
			Rd_Idx = (Wr_Idx + 1) % dma_buf_size ;
		}

		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_TCIF1);
	}
}
