//////////////////////////////////////////////////////////////////////
// Mode: PWM Mode & Original Counting(Counter) Mode
// 사용하는 Timer: PWM(Pulse & Dir): Timer5 CH1
//               COUNTING(Encoder): Timer4 CH1, CH2
// 동작: OC1을 통한 PWM Pulse(DC motor 구동) 출력과 Encoder에 의한 모터회전수 측정 
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

// Joystick Define
#define NAVI_PUSH	0x03C0  //PI5 0000 0011 1100 0000 
#define NAVI_UP		0x03A0  //PI6 0000 0011 1010 0000 
#define NAVI_DOWN	0x0360  //PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	0x02E0  //PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0  //PI9 0000 0001 1110 0000 

void _GPIO_Init(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);	// Joystick 입력 함수
void DisplayInitScreen(void);

void TIMER5_PWM_Init(void);
void TIMER4_ENCODER_Init(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

uint16_t ENCODER;
uint8_t ENC[5];

int main(void)
{
	_GPIO_Init();
	LCD_Init();		// LCD 구동 함수
	DelayMS(10);		// LCD구동 딜레이
	DisplayInitScreen();	// LCD 초기화면구동 함수
	BEEP();

	TIMER5_PWM_Init();	// TIM5 Init (PWM mode)
        TIMER4_ENCODER_Init(); // TIM4 Init (Encoder mode)

	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off
	GPIOA->ODR |= (1<<3);	// 초기방향: CCW
	GPIOG->ODR |= 0x01; 	// LED0 ON
        
	while(1)
	{
		switch(KEY_Scan())	// SW0~SW3 Duty 변경
		{
			case SW0_PUSH : 	//SW0
				TIM5->CCR1 = 10;		// DR: 10%	  
				LCD_DisplayText(1,6,"10");
 				GPIOG->ODR &= ~0x00FF;		// All LED Off
				GPIOG->ODR |= (1<<0);		// LED0 ON 
 			break;
			case SW1_PUSH : 	//SW1
				TIM5->CCR1 = 35;		// DR: 35%	  
				LCD_DisplayText(1,6,"35");
				GPIOG->ODR &= ~0x00FF;		// All LED Off
				GPIOG->ODR |= (1<<1);		// LED1 ON 
 			break;
                        case SW2_PUSH  : 	//SW2
				TIM5->CCR1 = 65;		// DR: 65%	  
				LCD_DisplayText(1,6,"65");
				GPIOG->ODR &= ~0x00FF;		// All LED Off
				GPIOG->ODR |= (1<<2);		// LED2 ON 
 			break;
                        case SW3_PUSH  : 	//SW3
				TIM5->CCR1 = 90;		// DR: 90%	  
				LCD_DisplayText(1,6,"90");
				GPIOG->ODR &= ~0x00FF;		// All LED Off
				GPIOG->ODR |= (1<<3);		// LED3 ON 
 			break;
			case SW7_PUSH  : 	//SW7
				ENCODER = TIM4->CNT;		// TIM4 CNT 값(Encoder 펄스 수) 읽음
				ENC[4]= ENCODER/10000;		// 10000 자리
				ENC[3]= ENCODER%10000/1000;	// 1000 자리
				ENC[2]= ENCODER%1000/100;	// 100 자리
				ENC[1]= ENCODER%100/10;		// 10 자리
				ENC[0]= ENCODER%10;		// 1 자리
		 		LCD_DisplayChar(3,5,ENC[4]+0x30);
				LCD_DisplayChar(3,6,ENC[3]+0x30);
				LCD_DisplayChar(3,7,ENC[2]+0x30);
				LCD_DisplayChar(3,8,ENC[1]+0x30);
				LCD_DisplayChar(3,9,ENC[0]+0x30);
				GPIOG->ODR |= (1<<7);		// LED7 ON 
 			break;
                 }      
      
		switch(JOY_Scan())	// Joystick 입력으로 모터 DIR 변경
		{
			case NAVI_LEFT : 	// Joystick LEFT
				GPIOA->ODR |= (1<<3);   //DIR = CCW(HIGH) (PA3)
				LCD_DisplayText(2,6," CCW");	   
			break;
			case NAVI_RIGHT:	// Joystick RIGHT	
				GPIOA->ODR &= ~(1<<3); 	//DIR = CW(LOW) (PA3)
				LCD_DisplayText(2,6,"  CW");
			break;
		}  // switch(JOY_Scan())
	}
}

void TIMER4_ENCODER_Init(void)
{  
// Encoder 입력(Counting) 핀: 
//   Encoder_A: PD12(TIM4 CH1) 
//   Encoder_B: PD13(TIM4 CH2)
// Clock Enable : GPIOD & TIMER4
	RCC->AHB1ENR	|= (1<<3);	// GPIOD Enable   0x08
	RCC->APB1ENR 	|= (1<<2);	// TIMER4 Enable   0x04

// PD12: TIM4_CH1
// PD12을 출력설정하고 Alternate function(TIM4_CH1)으로 사용 선언
	GPIOD->MODER 	|= (2<<24);	// GPIOD PIN12 Output Alternate function mode 	0x02000000				
	GPIOD->OSPEEDR 	|= (2<<24);	// GPIOD PIN12 Output speed (50MHz High speed)
	GPIOD->PUPDR	&= ~(3<<24); 	// GPIOD PIN12 NO Pull-up
  					// PD12 ==> TIM4_CH1
	GPIOD->AFR[1]	|= (2<<16);	// Connect TIM4 pins(PD12) to AF2(TIM3..5)
 
// PD13: TIM4_CH2
// PD13을 출력설정하고 Alternate function(TIM4_CH2)으로 사용 선언
	GPIOD->MODER 	|= (2<<26);	// GPIOD PIN13 Output Alternate function mode 	 0x08000000				
	GPIOD->OSPEEDR 	|= (2<<26);	// GPIOD PIN13 Output speed (50MHz High speed)
	GPIOD->PUPDR	&= ~(3<<26); 	// GPIOD PIN13 NO Pull-up
  					// PD13 ==> TIM4_CH2
	GPIOD->AFR[1]	|= (2<<20);	// Connect TIM4 pins(PD13) to AF2(TIM3..5)

// Timerbase Mode
	// Setting CR1 : 0x0000 
	TIM4->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM4->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
	TIM4->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
	TIM4->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 |=  (1<<7);	// ARPE=1(ARR Preload Enable)
	TIM4->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
				// Center-aligned mode: The counter counts Up and DOWN alternatively
        // CR2 
        TIM4->CR2 &= ~(1<<7);	// TI1S=0(TIM4_CH1 pin is connected to TI1 input)
 
	// PSC, ARR
	TIM4->PSC = 1-1;	// Prescaler=1
	TIM4->ARR = 10000;	// Auto reload  :  엔코더 count값 범위: 0~10000

	// Update(Clear) the Counter
	TIM4->EGR |= (1<<0);    // UG: Update generation  , REG's Update (CNT clear) 
       
// Output Compare Mode
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM4->CCMR1 |= (1<<0); 	// CC1S(CC1 channel) = '0b01' : Input 
	TIM4->CCMR1 &= ~(15<<4);   // IC1F='0b0000: No Input Filter 

    	TIM4->CCMR1 |= (1<<8); 	// CC2S(CC2 channel) = '0b01' : Input 
	TIM4->CCMR1 &= ~(15<<12); // IC2F='0b0000: No Input Filter 

	// CCER(Capture/Compare Enable Register) : Ch1 & Ch2 Polarity : Noninverting / Rising Edge
	// : TI1FP1 NonInverting / Rising Edge
	TIM4->CCER &= ~(1<<1);	// CC1P=0          //채널1: 1,3번비트
	TIM4->CCER &= ~(1<<3);	// CC1NP=0   
	// : TI2FP2 NonInverting / Rising Edge  
      	TIM4->CCER &= ~(1<<5);	// CC2P=0      //채널2: 5, 7번비트
	TIM4->CCER &= ~(1<<7);	// CC2NP=0   

        
	// SMCR(Slave Mode Control Reg.) : Encoder Mode 1 
	TIM4->SMCR |= (1<<0);	// SMS(Slave Mode Selection) 
				// : Encoder 1 mode (TI1FP1 기반으로 TI2FP2 펄스의 수를 counting)
	TIM4->CR1 |= (1<<0);	// CEN: Enable the Tim4 Counter  	
 }

void TIMER5_PWM_Init(void)
{  
// 모터펄스(PWM)핀:PA0(TIM5 CH1), 모터방향(DIR)핀:PA3
// Clock Enable : GPIOA & TIMER5
	RCC->AHB1ENR	|= (1<<0);	// GPIOA Enable
	RCC->APB1ENR 	|= (1<<3);	// TIMER5 Enable 
    						
// PA0을 출력설정하고 Alternate function(TIM5_CH1)으로 사용 선언 : PWM 출력
	GPIOA->MODER 	|= (2<<0);	// PA0 Output Alternate function mode					
	GPIOA->OSPEEDR 	|= (3<<0);	// PA0 Output speed (100MHz High speed)
	GPIOA->OTYPER	&= ~(1<<0);	// PA0 Output type push-pull (reset state)
	GPIOA->AFR[0]	|= (2<<0); 	// 0x00000002	(AFR[0].(3~0)=0b0010): Connect TIM5 pins(PA0) to AF2(TIM3..5)
					// PA0 ==> TIM5_CH1
    
// PA3을 GPIO  출력설정 : Dir (모터방향)
	GPIOA->MODER 	|= (1<<6);	// PA3 Output  mode					
	GPIOA->OSPEEDR 	|= (1<<6);	// PA3 Output speed (25MHz High speed)
	GPIOA->OTYPER	&= ~(1<<3);	// PA3 Output type push-pull (reset state)
        
// TIM5 Channel 1 : PWM 1 mode
	// Assign 'PWM Pulse Period'
	TIM5->PSC	= 8400-1;	// Prescaler 84,000,000Hz/8400 = 10,000 Hz(0.1ms)  (1~65536)
	TIM5->ARR	= 100-1;	// Auto reload  (0.1ms * 100 = 10ms : PWM Period)
    
	// Define the corresponding pin by 'Output'  
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM5->CCER	|= (1<<0);	// CC1E=1: OC1(TIM5_CH1) Active(Capture/Compare 1 output enable)
					// 해당핀(40번)을 통해 신호출력
	TIM5->CCER	&= ~(1<<1);	// CC1P=0: CC1 output Polarity High (OC1으로 반전없이 출력)

	// Duty Ratio 
	TIM5->CCR1	= 10;		// CCR1 value

	// 'Mode' Selection : Output mode, PWM 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM5->CCMR1 	&= ~(3<<0); 	// CC1S(CC1 channel)='0b00' : Output 
	TIM5->CCMR1 	|= (1<<3); 	// OC1PE=1: Output Compare 1 preload Enable

	TIM5->CCMR1	|= (6<<4);	// OC1M: Output compare 1 mode: PWM 1 mode
	TIM5->CCMR1	|= (1<<7);	// OC1CE: Output compare 1 Clear enable

	// CR1 : Up counting & Counter TIM5 enable
	TIM5->CR1 	&= ~(1<<4);	// DIR: Countermode = Upcounter (reset state)
	TIM5->CR1 	&= ~(3<<8);	// CKD: Clock division = 1 (reset state)
	TIM5->CR1 	&= ~(3<<5); 	// CMS(Center-aligned mode Sel): No(reset state)
	TIM5->CR1	|= (1<<7);	// ARPE: Auto-reload preload enable
	TIM5->CR1	|= (1<<0);	// CEN: Counter TIM5 enable
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
{ 	GPIOF->ODR |= 0x0200;	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
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

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_GREEN);	// 글자배경색 : Green
	LCD_SetTextColor(RGB_BLACK);	// 글자색 : Black

	LCD_DisplayText(0,0,"PWM  & Encoder" );  // Title

	LCD_SetBackColor(RGB_YELLOW);	//글자배경색
	LCD_DisplayText(1,0,"DUTY: 10%");
	LCD_DisplayText(2,0,"DIR :  CCW");
	LCD_DisplayText(3,0,"ENC :00000 ");
}

/* Joystick switch가 입력되었는지를 여부와 어떤 Joystick switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{ 
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if(key == 0x03E0)		// if no key, check key off
	{  	if(joy_flag == 0)
        		return key;
      		else
		{	DelayMS(10);
        		joy_flag = 0;
        		return key;
        	}
    	}
  	else				// if key input, check continuous key
	{	if(joy_flag != 0)	// if continuous key, treat as no key input
        		return 0x03E0;
      		else			// if new key,delay for debounce
		{	joy_flag = 1;
			DelayMS(10);
 			return key;
        	}
	}
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
