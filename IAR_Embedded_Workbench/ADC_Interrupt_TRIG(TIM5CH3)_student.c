//////////////////////////////////////////////////////////////////////
// ADC Interrupt by Triggering of TIM5 CH3
// ADC1:CH1 (PA1, pin 41) : 가변저항
// 변환시작: TIM5 CH3를 이용한 일정 주기마다 start trigger 명령
// 변환완료때 EOC 인터럽트 발생하여 아날로그 전압값과 디지털값을 LCD에 표시
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
void TIMER5_Init(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

uint16_t ADC_Value, Voltage;
char str[20];

void ADC_IRQHandler(void)
{
	ADC1->SR &= ~(1<<1);		// EOC flag clear

	ADC_Value = ADC1->DR;		// Reading ADC result

       	sprintf(str,"%4d",ADC_Value);	// ADC result : 12bits (0~4095)
       	LCD_DisplayText(0,6,str);
	Voltage = ADC_Value * (3.3 * 100) / 4095;   // 3.3 : 4095 =  Volatge : ADC_Value 
                                                    // 100:  소수점아래 두자리까지 표시하기 위한 값  
	LCD_DisplayChar(1,9,Voltage/100 + 0x30);
	LCD_DisplayChar(1,11,Voltage%100/10 + 0x30);
	LCD_DisplayChar(1,12,Voltage%10 + 0x30);

	// NO SWSTART !!!
}

int main(void)
{
	LCD_Init();	// LCD 구동 함수
	DelayMS(10);	// LCD구동 딜레이
 	DisplayTitle();	//LCD 초기화면구동 함수
      
	_GPIO_Init();
	_ADC_Init();
	TIMER5_Init();

	// NO SWSTART !!!!
 	while(1)
	{          
	}
}

void _ADC_Init(void)
{   	// ADC1: PA1(pin 41)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// (1<<0) ENABLE GPIOA CLK (stm32f4xx.h 참조)
	GPIOA->MODER |= (3<<2*1);		// CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE
						
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;	// (1<<8) ENABLE ADC1 CLK (stm32f4xx.h 참조)

	ADC->CCR &= ~(0X1F<<0);		// MULTI[4:0]: ADC_Mode_Independent
	ADC->CCR |=  (1<<16); 		// 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 21MHz)
        
	ADC1->CR1 &= ~(3<<24);		// RES[1:0]= 0x00 : 12bit Resolution  //빈칸 문제
	ADC1->CR1 &= ~(1<<8);		// SCAN=0 : ADC_ScanCovMode Disable
	ADC1->CR1 |=  (1<<5);		// EOCIE=1: Interrupt enable for EOC //빈칸 문제

	ADC1->CR2 &= ~(1<<1);		// CONT=0: ADC_Continuous ConvMode Disable
	ADC1->CR2 |=  (3<<28);		// EXTEN[1:0]: ADC_ExternalTrigConvEdge_Enable(Both Edge)  //빈칸 문제
	ADC1->CR2 |= (0x0C<<24);	// _Tim5 CC3___________//timer5의 ch3로 cc이벤트를 만들어서 ad컨버터를 작동 시작하겠다
	ADC1->CR2 &= ~(1<<11);		// ALIGN=0: ADC_DataAlign_Right
	ADC1->CR2 &= ~(1<<10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC1->SQR1 &= ~(0xF<<20);	// L[3:0]=0b0000: ADC Regular channel sequece length 
					// 0b0000:1 conversion)
 	//Channel selection, The Conversion Sequence of PIN1(ADC1_CH1) is first, Config sequence Range is possible from 0 to 17
	ADC1->SQR3 |= (1<<0);		// SQ1[4:0]=0b0001 : CH1
	ADC1->SMPR2 |= (0x7<<(3*1));	// ADC1_CH1 Sample TIme_480Cycles (3*Channel_1)
 	//Channel selection, The Conversion Sequence of PIN1(ADC1_CH1) is first, Config sequence Range is possible from 0 to 17

	NVIC->ISER[0] |= (1<<18);	// Enable ADC global Interrupt

	ADC1->CR2 |= (1<<0);		// ADON: ADC ON
}

void TIMER5_Init(void)
{
// TIM5_CH3 (PA2) : 500ms 이벤트 발생
// Clock Enable : GPIOA & TIMER5
	RCC->AHB1ENR	|= (1<<0);	// GPIOA Enable
	RCC->APB1ENR 	|= (1<<3);	// TIMER5 Enable     //빈칸 문제
    						
// PA2을 출력설정하고 Alternate function(TIM5_CH3)으로 사용 선언 
	GPIOA->MODER 	|= (2<<2*2);	// PA2 Output Alternate function mode					
	GPIOA->OSPEEDR 	|= (3<<2*2);	// PA2 Output speed (100MHz High speed)
	GPIOA->OTYPER	&= ~(1<<2);	// PA2 Output type push-pull (reset state)
	GPIOA->AFR[0]	|= (2 <<8); 	// 0x00000200	(AFR[0].(11~8)=0b0010): Connect TIM5 pins(PA2) to AF2(TIM3..5) //빈칸 문제
					// PA2 ==> TIM5_CH3

	// Assign 'Interrupt Period' and 'Output Pulse Period'
	TIM5->PSC = 840-1;	// Prescaler 84MHz/840 = 0.1MHz (10us)   //원본 코드
	TIM5->ARR = 50000-1;	// Auto reload  : 10us * 50K = 500ms(period)  //빈칸 문제
        TIM5->ARR = 200000-1;	// Auto reload  : 10us * 200K = 2000ms(period)  //빈칸 문제 //수정한 코드 
                                                                                                                 //(timer5는 32bit이기 때문에 ARR의 값을 200000으로 설정 가능)

	// CR1 : Up counting
	TIM5->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)   //빈칸 문제
	TIM5->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events   //빈칸 문제
				//	- Counter Overflow/Underflow, 
				// 	- Setting the UG bit Set,
				//	- Update Generation through the slave mode controller 
	TIM5->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
				//	- Counter Overflow/Underflow, 
				// 	- Setting the UG bit Set,
				//	- Update Generation through the slave mode controller 
	TIM5->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM5->CR1 &= ~(1<<7);	// ARPE=0(ARR is NOT buffered) (reset state)
	TIM5->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM5->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
				// Center-aligned mode: The counter counts Up and DOWN alternatively

	// Event & Interrup Enable : UI  
	TIM5->EGR |= (1<<0);    // UG: Update generation    

	////////////////////////////////
	// Disable Tim5 Update interrupt
    
	// Define the corresponding pin by 'Output'  
	TIM5->CCER|= (1<<8);	// CC3E=1: CC3 channel Output Enable     //빈칸 문제
				// OC3(TIM5_CH3) Active: 해당핀을 통해 신호출력
	TIM5->CCER &= ~(1<<9);	// CC3P=0: CC3 channel Output Polarity (OCPolarity_High : OC3으로 반전없이 출력)     //빈칸 문제

	// 'Mode' Selection : Output mode, toggle  
	TIM5->CCMR2 &= ~(3<<0); // CC3S(CC3 channel) = '0b00' : Output      //빈칸 문제
	TIM5->CCMR2 &= ~(1<<3); // OC3P=0: Output Compare 3 preload disable
	TIM5->CCMR2 |= (3<<4);	// OC3M=0b011: Output Compare 3 Mode : toggle       //빈칸 문제
				// OC3REF toggles when CNT = CCR3

 	TIM5->CCR3 = 30000;	// TIM5 CCR3 TIM5_Pulse

	////////////////////////////////
	// Disable Tim5 CC3 interrupt
      
	TIM5->CR1 |= (1<<0);	// CEN: Enable the Tim5 Counter  					
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
	
	//NAVI.SW(PORT I) 설정
	RCC->AHB1ENR 	|= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER 	= 0x00000000;	// GPIOI PIN8~PIN15 Input mode (reset state)
	GPIOI->PUPDR    = 0x00000000;	// GPIOI PIN8~PIN15 Floating input (No Pull-up, pull-down) (reset state)
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
	LCD_SetFont(&Gulim10);		//폰트 
	LCD_SetBackColor(RGB_GREEN);	//글자배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색
       	LCD_DisplayText(0,0,"ADC1: 0000");
      	LCD_DisplayText(1,0,"Voltage: 0.00V");

	LCD_SetBackColor(RGB_YELLOW);	//글자배경색
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
