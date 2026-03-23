//////////////////////////////////////////////////////////////////////
// ADC polling 
// ADC1:CH1 (PA1, pin 41)
// 가변저항으로 부터 들어오는 아날로그 전압신호를 Polling 방법으로 ADC로 입력하여 디지털로 전환 
// 아날로그 전압값으로 변환하여 LCD에 표시
//////////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

#define SW0_PUSH        0xFE00  //PH8
#define SW1_PUSH        0xFD00  //PH9
#define SW2_PUSH        0xFB00  //PH10
#define SW3_PUSH        0xF700  //PH11
#define SW4_PUSH        0xEF00  //PH12
#define SW5_PUSH        0xDF00  //PH13
#define SW6_PUSH        0xBF00  //PH14
#define SW7_PUSH        0x7F00  //PH15

void _GPIO_Init(void);
void DisplayTitle(void);

void _ADC_Init(void);
uint16_t KEY_Scan(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

uint16_t ADC_Value, Voltage;
char str[20];

int main(void)
{
	LCD_Init();	// LCD 구동 함수
	DelayMS(10);		// LCD구동 딜레이
	DisplayTitle();		//LCD 초기화면구동 함수
	_GPIO_Init();
	_ADC_Init();
    
	while(1)
	{
                //Starts conversion of regular channels
                ADC1->CR2 |= ADC_CR2_SWSTART; 	// 0x40000000 (1<<30)      //이 문장을 한번만 시행하기 위해서는 whille문 밖으로 옮겨야 함
        
        	//Wait until conversion is done,  ADC_SR_EOC = 0x02
        	while( (ADC1->SR & ADC_SR_EOC) != ADC_SR_EOC );
  
         	ADC_Value = ADC1->DR;   	// Read ADC result value from ADC Data Reg(ADC1->DR) 
        
        	sprintf(str,"%4d",ADC_Value);	// ADC result : 12bits (0~4095)
        	LCD_DisplayText(0,6,str);
               Voltage = ADC_Value * (3.3 * 100) / 4095;   // 3.3V: 4095 =  Volatge : ADC_Value      //원본코드
                                                            // 100:  소수점아래 두자리까지 표시하기 위한 값  

     //            Voltage = ADC_Value * (3.3 * 100) / 255;  //수정한 코드 (8bit용)

                LCD_DisplayChar(1,9,Voltage/100 + 0x30);
                LCD_DisplayChar(1,11,Voltage%100/10 + 0x30);
                LCD_DisplayChar(1,12,Voltage%10 + 0x30);
 	}
}

void _ADC_Init(void)
{   	// ADC1: PA1(pin 41)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// (1<<0) ENABLE GPIOA CLK (stm32f4xx.h 참조)
	GPIOA->MODER |= (3<<2*1);		// CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE

	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;	// (1<<8) ENABLE ADC1 CLK (stm32f4xx.h 참조)

	ADC->CCR &= ~(0x1F<<0);		// MULTI[4:0]: ADC_Mode_Independent
	ADC->CCR |= (1<<16); 		// 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 21MHz)

	ADC1->CR1 &= ~(3<<24);		// RES[1:0]=0b00 : 12bit Resolution   //원본 코드
//       ADC1->CR1 |= (2<<24);	//8bit Resolution //수정한 코드 (한번만 실행)
	ADC1->CR1 &= ~(1<<8);		// SCAN=0 : ADC_ScanCovMode Disable

	ADC1->CR2 &= ~(1<<1);		// CONT=0: ADC_Continuous ConvMode Disable   //원본코드
 //       ADC1->CR2 |= (1<<1);	          //수정한 코드

	ADC1->CR2 &= ~(3<<28);		// EXTEN[1:0]=0b00: ADC_ExternalTrigConvEdge_None
	ADC1->CR2 &= ~(1<<11);		// ALIGN=0: ADC_DataAlign_Right
	ADC1->CR2 |= (1<<10);		// EOCS=1: The EOC bit is set at the end of each regular conversion//원본 코드
  //      ADC1->CR2 &= ~(1<<10);                 //수정한 코드   (한번만 실행)
	ADC1->SQR1 &= ~(0xF<<20);	// L[3:0]=0b0000: ADC Regular channel sequece length 
					// 0b0000:1 conversion)
    
	// Channel selection, The Conversion Sequence of PIN1(ADC1_CH1) is first, 
	// Config sequence Range is possible from 0 to 17
	ADC1->SQR3 |= (1<<0);		// SQ1[4:0]=0b0001 : CH1   //1번 채널 등록

	ADC1->CR2 |= (1<<0);		// ADON=1: ADC ON
	
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    	RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
	
	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR 	|= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER 	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR    &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

void BEEP(void)			// Beep for 20 ms 
{ 	GPIOF->ODR |= (1<<9);	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~(1<<9);	// PF9 'L' Buzzer off
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);   // 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
		for(; Dly; Dly--);
}

void DisplayTitle(void)
{
	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim10);			//폰트 
	LCD_SetBackColor(RGB_GREEN);		//글자배경색
	LCD_SetTextColor(RGB_BLACK);		//글자색
       	LCD_DisplayText(0,0,"ADC1: 0000");	// Title
      	LCD_DisplayText(1,0,"Voltage: 0.00V");

	LCD_SetBackColor(RGB_YELLOW);		//글자배경색
}

uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
        		return key;
      		else
		{	DelayMS(10);
        		key_flag = 0;
        		return key;
        	}
    	}
  	else				// if key input, check continuous key
	{	if(key_flag != 0)	// if continuous key, treat as no key input
        		return 0xFF00;
      		else			// if new key,delay for debounce
		{	key_flag = 1;
			DelayMS(10);
 			return key;
        	}
	}
}


