//////////////////////////////////////////////////////////////////////
// DMA processing between ADC and memory (peripheral to memory)
// Peripheral: ADC1_CH1 (PA1) : 가변저항
//                ADC1_CH0 (PA0) : 거리센서
// Memory: ADC_value[2]; [0] 가변저항, [1] 거리센서
// 동작개요: 두 아날로그 신호(전압값) SQR에 순서대로 등록(CH1우선)하고, 
//              SCAN mode로 연속 ADC하여 
//              DMA를 통하여 메모리(ADC_value[2])에 저장하고 전압값변환하여 LCD에 표시
// ADC설정: ADC1, SCAN mode, CONT mode, 2 ch 변환, DMA mode, 12 bit RES, 
// DMA 설정: DMA2 Stream0 channel0(ADC1), Peripheral-to-memory (P=>M), DMA_BufferSize = 2 (ADC_Value[2])
//               Circular mode, Priority level - High, FIFO_direct mode, 
//               Peripheral address pointer is fixed, Memory address pointer is incremented after each data transferd 
//////////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void RunMenu(void);
void _ADC_Init(void);
void DMAInit(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t ADC_result[2];
char str[20];

int main(void)
{
	_ADC_Init();
	DMAInit();
    
	LCD_Init();		// LCD 구동 함수
	DelayMS(500);		// LCD구동 딜레이
          
	//LCD 초기화면구동 함수
	RunMenu();
   
 	while(1)
	{
		LCD_SetTextColor(RGB_BLACK);    
  
		///// ADC2_CH0 거리센서  전압값 표시
		sprintf(str,"Value : %4d",ADC_result[0]);
		LCD_DisplayText(3,0,str);     
		sprintf(str,"Voltage : %2.2f[V]",(3.3/4095)*ADC_result[0]);  
		LCD_DisplayText(4,0,str);

		///// ADC2_CH1 가변저항 전압값 표시
		sprintf(str,"Value : %4d",ADC_result[1]);
		LCD_DisplayText(6,0,str);     
		sprintf(str,"Voltage : %2.2f[V]",(3.3/4095)*ADC_result[1]); //ADC1_CH0 거리센서 전압값 표시
		LCD_DisplayText(7,0,str);
	}
}


void _ADC_Init(void)
{
	/* 1st Analog signal */
	RCC->AHB1ENR |= 0x01;  	// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER |= 0x0C;	// GPIOA PIN1(PA1) 가변저항 : Analog mode
  
	/* 2nd Analog signal */
	GPIOA->MODER |= 0x03;	// GPIOA PIN0(PA0) 거리센서: Analog mode

	/* ADC Common Init **********************************************************/
	RCC->APB2ENR |= 0x0200;	// RCC_APB2ENR ADC2 Enable
  
	ADC->CCR &= ~0X0000001F;// ADC_Mode_Independent
	ADC->CCR |= 0x00010000;	// ADC_Prescaler_Div4 (ADC MAX Clock 36Mhz, 84Mhz(APB2)/4 = 21Mhz
//	ADC->CCR |= 0x00004000;	// ADC_DMAAccessMode_1
//	ADC->CCR |= 0x00000F00;	// ADC_TwoSamplingDelay_20Cycles

	/* ADC1 Init ****************************************************************/
	ADC2->CR1 &= ~(3<<24);	// RES[1:0]=0b00 : 12bit Resolution
	ADC2->CR1 |= 0x00000100;	// ADC_ScanCovMode Enable (SCAN=1)
	ADC2->CR2 |= 0x00000002;	// ADC_ContinuousConvMode ENABLE (CONT=1)
	ADC2->CR2 &= ~(3<<28);	// EXTEN[1:0]=0b00: ADC_ExternalTrigConvEdge_None (외부트리거 사용안함)

	ADC2->CR2 &= ~(1<<11);	// ALIGN=0: ADC_DataAlign_Right
	ADC2->CR2 &= ~(1<<10);	// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC2->SQR1 |= 0x00100000; // ADC Regular channel sequece length = 2 conversion

	/* ADC_RegularChannelConfig *********************************************/
	ADC2->SMPR2 |= 0x07 << (3*1);	// ADC1_CH1 Sample TIme_480Cycles (3*Channel_1)
	ADC2->SQR3 |= 0x01 << (5*(2-1));	// ADC1_CH1 << (5 * (Rank - 1)),  Rank = 1 (1순위로 변환: 가변저항)
    
	ADC2->SMPR2 |= 0x07 << (3*0);	//ADC1_CH0 Sample Time_480Cycles (3*Channel_0)
	ADC2->SQR3 &= ~(0x1F << (5*(1-1)));//ADC1_CH0 << (5*(Rank-1)), Rank = 2 (2순위로 변환: 거리센서)

	/* Enable DMA request after last transfer (Single-ADC mode) */
	ADC2->CR2 |= 0x00000200;	// DDS: DMA requests are issued as long as data are converted and DMA=1	
				// for single ADC mode
	/* Enable ADC1 DMA */
	ADC2->CR2 |= 0x00000100;	// DMA mode enabled  (DMA=1)
	ADC2->CR2 |= 0x00000001;	// Enable ADC1:  ADON=1
  
	/* Start ADC Software Conversion */ 
	ADC2->CR2 |= 0x40000000;   // SWSTART=1
}

void DMAInit(void)
{
 	// DMA2 Stream0 channel0 configuration *************************************
	RCC->AHB1ENR |= (1<<22);		//DMA2 clock enable
	DMA2_Stream2->CR &= ~(7<<25);	//DMA2 Stream2 channel  Clear
	DMA2_Stream2->CR |= (1<<25);	//DMA2 Stream2 channel 1 selected

	// ADC1->DR(Peripheral) ==> ADC_vlaue(Memory)
	DMA2_Stream2->PAR |= (uint32_t)&ADC2->DR;	   //Peripheral address - ADC1->DR(Regular data) Address
	DMA2_Stream2->M0AR |= (uint32_t)ADC_result; //Memory address - ADC_Value address 
	DMA2_Stream2->CR &= ~(3<<6);	  //Data transfer direction : Peripheral-to-memory (P=>M)
	DMA2_Stream2->NDTR = 2;			  //DMA_BufferSize = 2 (ADC_Value[2])

	DMA2_Stream2->CR &= ~(1<<9); 	//Peripheral increment mode  - Peripheral address pointer is fixed
	DMA2_Stream2->CR |= (1<<10);	//Memory increment mode - Memory address pointer is incremented after each data transferd 
	DMA2_Stream2->CR |= (1<<11);	//Peripheral data size - halfword(16bit)
	DMA2_Stream2->CR |= (1<<13);	//Memory data size - halfword(16bit)   
	DMA2_Stream2->CR |= (1<<8);	//Circular mode enabled   
	DMA2_Stream2->CR |= (2<<16);	//Priority level - High

	DMA2_Stream2->FCR &= ~(1<<2);	//DMA_FIFO_direct mode enabled
	DMA2_Stream2->FCR |= (1<<0);	//DMA_FIFO Threshold_HalfFull , Not used in direct mode

	DMA2_Stream2->CR &= ~(3<<23);	//Memory burst transfer configuration - single transfer
	DMA2_Stream2->CR &= ~(3<<21);	//Peripheral burst transfer configuration - single transfer  
	DMA2_Stream2->CR |= (1<<0);	//DMA2_Stream0 enabled
}

void RunMenu(void)
{
	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim8);
	LCD_SetBackColor(RGB_GREEN);
	LCD_SetTextColor(RGB_BLACK);
 	LCD_DisplayText(0,0,"ADC");

	LCD_SetBackColor(RGB_WHITE);	//글자배경색
	LCD_SetTextColor(RGB_RED);

	LCD_DisplayText(1,0,"ADC 12bit");
	LCD_DisplayText(2,0,"REF 3.3V");

}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);         	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
	for(; Dly; Dly--);
}

