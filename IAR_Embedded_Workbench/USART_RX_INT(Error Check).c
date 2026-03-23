//////////////////////////////////////////////////////////////////////
// USART RX : Error check 실습
// USART1: TX pin: PA9, RX pin: PA10 
// - 인위적으로 Overrun, Frame, Parity error를 발생케하여 
//    데이터 수신시 error check 함 
// - 테스트 데이터는 PC(Hercules)에서 전송
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

void DisplayTitle(void);
void _GPIO_Init(void);
uint16_t KEY_Scan(void);

void USART1_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);

void SerialSendChar(uint8_t c);
void SerialSendString(char *s);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

uint8_t overrun;

int main(void)
{
	LCD_Init();	// LCD 구동 함수
	DelayMS(1000);	// LCD구동 딜레이
    
	_GPIO_Init();
	USART1_Init();
	GPIOG->ODR &= 0x00;	// LED0~7 Off 
          
	DisplayTitle();	//LCD 초기화면구동 함수
	BEEP();
    
	while(1)
	{
		switch(KEY_Scan())
		{
			case SW0_PUSH : 		//SW0
				GPIOG->ODR |= 0x01;	// LED0 On
				SerialSendString("HELLO! "); // USART1을 통해 PC로 “HELLO! ” 문자열을 전송
			break;
			case SW6_PUSH : 	//SW6 (overrun 삭제)
				overrun= 0;
			break;
			case SW7_PUSH : 	//SW7 (overrun 발생)
				overrun= 1;
			break;
                }      
 	}
}

void USART1_IRQHandler(void)	
{       
	if (USART1->SR & USART_SR_PE) // 0x0001
		LCD_DisplayText(2,0,"Parity Error! ");			  
	if (USART1->SR & USART_SR_ORE) // 0x0008
		LCD_DisplayText(3,0,"Overrun Error! ");			  
	if (USART1->SR & USART_SR_FE) // 0x0002
		LCD_DisplayText(4,0,"Frame Error! ");			  
	if (overrun == 0) {
		if ( (USART1->SR & USART_SR_RXNE) ) // USART_SR_RXNE= 1? RX Buffer Full?
		// #define  USART_SR_RXNE ((uint16_t)0x0020)    //  Read Data Register Not Empty     
		{
			char ch;
			ch = (uint16_t)(USART1->DR & (uint16_t)0x01FF);	// 수신된 문자 저장
			LCD_DisplayChar(1,0,ch); 	// 수신된 문자를 LCD에 display
		} 
	}
	// DR 을 읽으면 SR.RXNE bit(flag bit)는 clear 된다. 즉 clear 할 필요없음 
}

void USART1_Init(void)
{
	RCC->AHB1ENR	|= (1<<0);		// RCC_AHB1ENR GPIOA Enable

	// USART1 : TX(PA9)
	GPIOA->MODER	|= (2<<2*9);	// GPIOA PIN9 Output Alternate function mode					
	GPIOA->OSPEEDR |= (3<<2*9);	// GPIOA PIN9 Output speed (100MHz Very High speed)
	GPIOA->AFR[1] |= (7<<4);		// Connect GPIOA pin9 to AF7(USART1)
    
	// USART1 : RX(PA10)
	GPIOA->MODER 	|= (2<<2*10);	// GPIOA PIN10 Output Alternate function mode
	GPIOA->OSPEEDR |= (3<<2*10);	// GPIOA PIN10 Output speed (100MHz Very High speed
	GPIOA->AFR[1] |= (7<<8);		// Connect GPIOA pin10 to AF7(USART1)

	RCC->APB2ENR	|= (1<<4);		// RCC_APB2ENR USART1 Enable
    
//	USART_BRR_Configuration(9600);	 // USART Baud rate Configuration
	USART_BRR_Configuration(19200);	 // USART Baud rate Configuration
    
//	USART1->CR1	&= ~(1<<12);	// USART_WordLength 8 Data bit
	USART1->CR1	|= (1<<12);		// USART_WordLength 9 Data bit

//	USART1->CR1	&= ~(1<<10);	// NO USART_Parity
	USART1->CR1	|= (1<<10);		// USART_Parity Enable

	USART1->CR1	&= ~(1<<9);	// Even USART_Parity
//	USART1->CR1	|= (1<<9);		// Odd USART_Parity

	USART1->CR1	|= (1<<2);		// 0x0004, USART_Mode_RX Enable
	USART1->CR1	|= (1<<3);		// 0x0008, USART_Mode_Tx Enable

	USART1->CR2	&= ~(3<<12);	// 0b00, USART_StopBits_1
	USART1->CR3	= 0x0000;		// No HardwareFlowControl, No DMA
    
	USART1->CR1 	|= (1<<5);		// 0x0020, RXNE interrupt Enable
	NVIC->ISER[1]	|= (1<<(37-32));	// Enable Interrupt USART1 (NVIC 37번)

	USART1->CR1 	|= (1<<13);		// 0x2000, USART1 Enable
}

void SerialSendChar(uint8_t Ch) // 1문자 보내기 함수
{
	// USART_SR_TXE(1<<7)=0?, TX Buffer NOT Empty? 
	// TX buffer Empty되지 않으면 계속 대기(송신 가능한 상태까지 대기)
	while((USART1->SR & USART_SR_TXE) == RESET); 
	USART1->DR = (Ch & 0x01FF);	// 전송 (최대 9bit 이므로 0x01FF과 masking)
}

void SerialSendString(char *str) // 여러문자 보내기 함수
{
	while (*str != '\0') // 종결문자가 나오기 전까지 구동, 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialSendChar(*str);	// 포인터가 가르키는 곳의 데이터를 송신
		str++; 			// 포인터 수치 증가
	}
}

// Baud rate 설정
void USART_BRR_Configuration(uint32_t USART_BaudRate)
{ 
	uint32_t tmpreg = 0x00;
	uint32_t APB2clock = 84000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	// Find the integer part 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
	//  #define  USART_CR1_OVER8 ((uint16_t)0x8000) // USART Oversampling by 8 enable   
	{       // USART1->CR1.OVER8 = 1 (8 oversampling)
		// Computing 'Integer part' when the oversampling mode is 8 Samples 
		integerdivider = ((25 * APB2clock) / (2 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)  
	}
	else  // USART1->CR1.OVER8 = 0 (16 oversampling)
	{	// Computing 'Integer part' when the oversampling mode is 16 Samples 
		integerdivider = ((25 * APB2clock) / (4 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)    
	}								     // 100*(f_CK) / (8*2*Buadrate) = (25*f_CK)/(4*Buadrate)	
	tmpreg = (integerdivider / 100) << 4;
  
	// Find the fractional part 
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	// Implement the fractional part in the register 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0)	
	{	// 8 oversampling
		tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
	}
	else	// 16 oversampling
	{
		tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
	}

	// Write to USART BRR register
	USART1->BRR = (uint16_t)tmpreg;
}

void DisplayTitle(void)
{	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim8);
	LCD_SetBackColor(RGB_GREEN);	//배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색
	LCD_DisplayText(0,0,"USART1 Rx: Error check");

	LCD_SetBackColor(RGB_WHITE);	//글자배경색
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable					
	GPIOG->MODER &= 0xFFFF0000;	// GPIOG 0~7 : Clear (0b00)			
	GPIOG->MODER |=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)					
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR &= ~0x0000FFFF;	// GPIOG 0~7 : Clear (0b00)		 	
	GPIOG->OSPEEDR |=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR	|=  0x00000080;		// RCC_AHB1ENR : GPIOH(bit#7) Enable					
	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
	RCC->AHB1ENR	|=  0x00000020; 		// RCC_AHB1ENR : GPIOF(bit#5) Enable					
	GPIOF->MODER 	&= ~0x000C0000;	// GPIOF 9 : Clear (0b00)
	GPIOF->MODER 	|=   0x00040000;		// GPIOF 9 : Output mode (0b01)					
	GPIOF->OTYPER 	&= ~0x0200;		// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)
 	GPIOF->OSPEEDR |=  0x00040000;		// GPIOF 9 : Output speed 25MHZ Medium speed 
	
	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR 	|= 0x00000100;		// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER 	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR    &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

void DelayMS(unsigned short wMS)
{	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);  // 1000us => 1ms
}
void DelayUS(unsigned short wUS)
{	volatile int Dly = (int)wUS*17;
	for(; Dly; Dly--);
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
void BEEP(void)			// Beep for 20 ms 
{	GPIOF->ODR |= (1<<9);	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~(1<<9);	// PF9 'L' Buzzer off
}