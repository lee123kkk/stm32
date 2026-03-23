//////////////////////////////////////////////////////////////////////
// USART : BT 연결용 프로그램 
// UART4: TX pin: PC10, RX pin: PC11 
// TX: Polling 방식, RX: Interrupt 방식 
// 문자를 TX를 통해 BT module(Smartphone)로 전송하고, 
// BT module(Smartphone)에서 보내온 문자를 받아 LCD에 표시
// 2024. 11. 11
//////////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include "default.h"
#include "Util.h"

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

void UART4_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);

void SerialSendChar(uint8_t c);
void SerialSendString(char *s);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void BEEP(void);

UINT8 no, CRdata;
UINT8 Px, Py;
int main(void)
{
	LCD_Init();	// LCD 구동 함수
	DelayMS(1000);	// LCD구동 딜레이
    
	_GPIO_Init();
	UART4_Init();
	GPIOG->ODR &= 0x00;	// LED0~7 Off 
          
	DisplayTitle();	//LCD 초기화면구동 함수
	BEEP();
    
    
	while(1)
	{
		switch(KEY_Scan())
		{
		case SW0_PUSH : 	//SW0
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x01;// LED0 On
			SerialSendString("AT");
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT"); 
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW1_PUSH : 	//SW1
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x02;// LED1 On
			SerialSendString("AT+BTSCAN");
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT+BTSCAN");                       //팬딩 상태
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW2_PUSH : 	//SW2
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x04;// LED2 On
			SerialSendString("AT+BTCANCEL");                           //팬딩 상태 취소
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT+BTCANCEL"); 
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW3_PUSH : 	//SW3
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x08;// LED3 On
			SerialSendString("AT+BTNAME=TUE01");                 //이름 변경
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT+BTNAME=TUE01");        //이름 변경 확인
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW4_PUSH : 	//SW4                                            
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x10;// LED4 On
			SerialSendString("ATZ");
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"ATZ"); 
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW5_PUSH : 	//SW5                                              
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x20;// LED5 On
			SerialSendString("AT+BTNAME?");                           //이름
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT+BTNAME?"); 
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW6_PUSH : 	//SW6
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x40;// LED6 On
			SerialSendString("AT+BTSTS?" );
//			SerialSendString("AT+BTKEY?" );
			SerialSendChar(0x0D); // '\r' carriage return
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"AT+BTSTS?"); 
//			LCD_DisplayText(4,0,"AT+BTKEY?"); 
			LCD_DisplayText(6,0,"                               ");
			LCD_DisplayText(7,0,"                               ");
			Px= 0;  Py=0;
 		break;
		case SW7_PUSH : 	//SW7
			GPIOG->ODR &= 0x00;	// LED0~7 Off 
			GPIOG->ODR |= 0x80;// LED7 On
			SerialSendString("BT test");
			LCD_DisplayText(4,0,"                                    "); 
			LCD_DisplayText(4,0,"BT test"); 
 		break;
                }      
 	}
}


void UART4_IRQHandler(void)	
{       
	if ( (UART4->SR & USART_SR_RXNE) ) // USART_SR_RXNE= 1? RX Buffer Full?
    // #define  USART_SR_RXNE ((uint16_t)0x0020)    //  Read Data Register Not Empty(FULL)     
	{
		char ch;
		ch = UART4->DR;	// 수신된 문자 저장
		if ((ch != 0x0D)&&(ch != 0x0A)){
			LCD_DisplayChar(Py+6,Px++, ch);
		}
		if ((Px > 18)&&(Py==0)) {
			Px= 0;	Py= 1;	
		}
		else if ((Px > 18)&&(Py==1)) {
			Px= 0;	Py= 0;	
		}
	} 
        // DR 을 읽으면 SR.RXNE bit(flag bit)는 clear 된다. 즉 clear 할 필요없음 
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    	|=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    	RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}

void UART4_Init(void)
{
	// UART4 : TX(PC10)
	RCC->AHB1ENR	|= (1<<2);	// RCC_AHB1ENR GPIOC Enable
	GPIOC->MODER	|= (2<<2*10);	// GPIOC PIN10 Output Alternate function mode					
	GPIOC->OSPEEDR	|= (3<<2*10);	// GPIOC PIN10 Output speed (100MHz Very High speed)
	GPIOC->AFR[1]	|= (8<<4*(10-8));// Connect GPIOC pin10 to AF8(USART1)
    
	// UART4 : RX(PC11)
	GPIOC->MODER 	|= (2<<2*11);	// GPIOC PIN11 Output Alternate function mode
	GPIOC->OSPEEDR	|= (3<<2*11);	// GPIOC PIN11 Output speed (100MHz Very High speed
	GPIOC->AFR[1]	|= (8<<4*(11-8));// Connect GPIOC pin11 to AF8(USART1)

	// BT RESET (PC13) : GPIO
	GPIOC->MODER 	|= (1<<2*13);	// GPIOC PIN13 Output mode
	GPIOC->OSPEEDR  |= (3<<2*13);
	GPIOC->ODR	|= (1<<13);	// BT Reset
 
	RCC->APB1ENR	|= (1<<19);	// RCC_APB1ENR UART4 Enable
    
	USART_BRR_Configuration(9600); // USART Baud rate Configuration
    
	UART4->CR1	&= ~(1<<12);	// USART_WordLength 8 Data bit
	UART4->CR1	&= ~(1<<10);	// NO USART_Parity

	UART4->CR1	|= (1<<2);	// 0x0004, USART_Mode_RX Enable
	UART4->CR1	|= (1<<3);	// 0x0008, USART_Mode_Tx Enable
	UART4->CR2	&= ~(3<<12);	// 0b00, USART_StopBits_1
	UART4->CR3	= 0x0000;	// No HardwareFlowControl, No DMA
    
	UART4->CR1 	|= (1<<5);	// 0x0020, RXNE interrupt Enable
	NVIC->ISER[1]	|= (1<<(52-32));// Enable Interrupt USART1 (NVIC 52번)
	UART4->CR1 	|= (1<<13);	//  0x2000, USART1 Enable
}

void SerialSendChar(uint8_t Ch) // 1문자 보내기 함수
{
	// USART_SR_TXE(1<<7)=0?, TX Buffer NOT Empty? 
	// TX buffer Empty되지 않으면 계속 대기(송신 가능한 상태까지 대기)
    	while((UART4->SR & USART_SR_TXE) == RESET); 
	UART4->DR = (Ch & 0x01FF);	// 전송 (최대 9bit 이므로 0x01FF과 masking)
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
	uint32_t APB1clock = 42000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	// Find the integer part 
	if ((UART4->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
        //  #define  USART_CR1_OVER8 ((uint16_t)0x8000) // USART Oversampling by 8 enable   
	{       // UART4->CR1.OVER8 = 1 (8 oversampling)
		// Computing 'Integer part' when the oversampling mode is 8 Samples 
		integerdivider = ((25 * APB1clock) / (2 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)  
	}
	else  // USART1->CR1.OVER8 = 0 (16 oversampling)
	{	// Computing 'Integer part' when the oversampling mode is 16 Samples 
		integerdivider = ((25 * APB1clock) / (4 * USART_BaudRate));  // 공식에 100을 곱한 곳임(소수점 두번째자리까지 유지하기 위함)    
	}
	tmpreg = (integerdivider / 100) << 4;
  
	// Find the fractional part 
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	// Implement the fractional part in the register 
	if ((UART4->CR1 & USART_CR1_OVER8) != 0)	
	{	// 8 oversampling
		tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
	}
	else	// 16 oversampling
	{
		tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
	}

	// Write to USART BRR register
	UART4->BRR = (uint16_t)tmpreg;
}

void DisplayTitle(void)
{	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim8);
	LCD_SetBackColor(RGB_GREEN);	//배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색
	LCD_DisplayText(0,0,"BLUETOOTH(UART4)");

	LCD_SetBackColor(RGB_WHITE);	//글자배경색
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
{ 	GPIOF->ODR |= (1<<9);	// PF9 'H' Buzzer on
	DelayMS(20);		// Delay 20 ms
	GPIOF->ODR &= ~(1<<9);	// PF9 'L' Buzzer off
}