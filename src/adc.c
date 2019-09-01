//--------------------------------------------------------------
// File     : stm32_ub_adc1_single.c
// Datum    : 16.02.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.0
// Module   : GPIO,ADC
// Funktion : AD-Wandler (ADC1 im Single-Conversion-Mode)
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include <adc.h>


//--------------------------------------------------------------
// Definition der benutzten ADC Pins
// Reihenfolge wie bei ADC1s_NAME_t
//--------------------------------------------------------------
ADC1s_t ADC1s[] = {
  //NAME  ,PORT , PIN      , CLOCK              , Kanal        , Mittelwerte
  {ADC_PA6,GPIOA,GPIO_Pin_6,RCC_AHB1Periph_GPIOA,ADC_Channel_6},   // ADC an PA6 = ADC1_IN6
  {ADC_PA7,GPIOA,GPIO_Pin_7,RCC_AHB1Periph_GPIOA,ADC_Channel_7}, // ADC an PA7 = ADC1_IN7
};



//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_ADC1s_InitIO(void);
void P_ADC1s_InitADC(void);


//--------------------------------------------------------------
// init vom ADC1 im Single-Conversion-Mode
//--------------------------------------------------------------
void UB_ADC1_SINGLE_Init(void)
{
  P_ADC1s_InitIO();
  P_ADC1s_InitADC();
}



float U_Bat = 0;
float U_Prog = 0;
float I_Bat  = 0;
void adc_task()
{
	U_Bat 	= ((float)UB_ADC1_SINGLE_Read(ADC_PA7)) / 4095 * 3.3 * 2;		// Measure battery voltage [V]
	U_Prog 	= ((float)UB_ADC1_SINGLE_Read(ADC_PA6)) / 4095 * 3.3;			// Measure prog voltage [V]
	I_Bat   = U_Prog / 5000 * 1000;
}



//--------------------------------------------------------------
// starten einer Messung
// und auslesen des Messwertes
//--------------------------------------------------------------
uint16_t UB_ADC1_SINGLE_Read(ADC1s_NAME_t adc_name)
{
  uint16_t messwert=0;

  // Messkanal einrichten
  ADC_RegularChannelConfig(ADC1,ADC1s[adc_name].ADC_CH,1,ADC_SampleTime_3Cycles);
  // Messung starten
  ADC_SoftwareStartConv(ADC1);
  // warte bis Messung fertig ist
  while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
  // Messwert auslesen
  messwert = ADC_GetConversionValue(ADC1);

  return(messwert);
}

//--------------------------------------------------------------
// interne Funktion
// Init aller IO-Pins
//--------------------------------------------------------------
void P_ADC1s_InitIO(void) {
  GPIO_InitTypeDef  GPIO_InitStructure;
  ADC1s_NAME_t 		adc_name;

  for(adc_name=0; adc_name < ADC1s_ANZ; adc_name++)
  {
    // Clock Enable
    RCC_AHB1PeriphClockCmd(ADC1s[adc_name].ADC_CLK, ENABLE);

    // Config des Pins als Analog-Eingang
    GPIO_InitStructure.GPIO_Pin 	= ADC1s[adc_name].ADC_PIN;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL ;
    GPIO_Init(ADC1s[adc_name].ADC_PORT, &GPIO_InitStructure);
  }
}

//--------------------------------------------------------------
// interne Funktion
// Init von ADC Nr.1
//--------------------------------------------------------------
void P_ADC1s_InitADC(void)
{
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  ADC_InitTypeDef       ADC_InitStructure;

  // Clock Enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  // ADC-Config
  ADC_CommonInitStructure.ADC_Mode 				= ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler 		= ADC1s_VORTEILER;
  ADC_CommonInitStructure.ADC_DMAAccessMode 	= ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay 	= ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  ADC_InitStructure.ADC_Resolution 				= ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode 			= DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode 		= DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge 	= ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign 				= ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion 		= 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  // ADC-Enable
  ADC_Cmd(ADC1, ENABLE);
}


