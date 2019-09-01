//--------------------------------------------------------------
// File     : stm32_ub_adc1_single.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_ADC1_SINGLE_H
#define __STM32F4_UB_ADC1_SINGLE_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_adc.h"
#include "adc.h"

//--------------------------------------------------------------
// Liste aller ADC-Kanäle
// (keine Nummer doppelt und von 0 beginnend)
//--------------------------------------------------------------
typedef enum {
  ADC_PA6 = 0,  // PA6
  ADC_PA7 = 1   // PA7
}ADC1s_NAME_t;

#define  ADC1s_ANZ   2 // Anzahl von ADC1s_NAME_t




//--------------------------------------------------------------
// ADC-Clock
// Max-ADC-Frq = 36MHz
// Grundfrequenz = APB2 (APB2=84MHz)
// Moegliche Vorteiler = 2,4,6,8
//--------------------------------------------------------------

//#define ADC1s_VORTEILER     ADC_Prescaler_Div2 // Frq = 42 MHz
#define ADC1s_VORTEILER     ADC_Prescaler_Div4   // Frq = 21 MHz
//#define ADC1s_VORTEILER     ADC_Prescaler_Div6 // Frq = 14 MHz
//#define ADC1s_VORTEILER     ADC_Prescaler_Div8 // Frq = 10.5 MHz



//--------------------------------------------------------------
// Struktur eines ADC Kanals
//--------------------------------------------------------------
typedef struct {
  ADC1s_NAME_t ADC_NAME;  // Name
  GPIO_TypeDef* ADC_PORT; // Port
  const uint16_t ADC_PIN; // Pin
  const uint32_t ADC_CLK; // Clock
  const uint8_t ADC_CH;   // ADC-Kanal
}ADC1s_t;



//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------

void adc_task(void);
void UB_ADC1_SINGLE_Init(void);
uint16_t UB_ADC1_SINGLE_Read(ADC1s_NAME_t adc_name);
//uint16_t UB_ADC1_SINGLE_Read_MW(ADC1s_NAME_t adc_name);


//--------------------------------------------------------------
#endif // __STM32F4_UB_ADC1_SINGLE_H
