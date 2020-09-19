/**
 ******************************************************************************
 * @file    spi.h
 * @author  SO
 * @version V1.0
 * @date    27-February-2018
 * @brief   Handles the SPI communication.
 ******************************************************************************
 */

#ifndef SPI_H_
#define SPI_H_
#include "oVario_Framework.h"

void init_spi(void);
void spi_send_char(unsigned char ch_data);

#endif /* SPI_H_ */
