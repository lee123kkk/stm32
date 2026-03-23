//////////////////////////////////////////////////////////////////////
// DMA를 이용한 USART(RX) 통신 
// USART1: TX pin: PA9, RX pin: PA10 
//    - DMA 방식 (interrupt 사용 안함)
// PC(Hyper terminal: Herculus)에서 전송한 데이터를 USART1->DR에 저장하고, 
// DMA를 통해 이 데이터를 메모리(USART_value[ ])에 보내어 저장함
//////////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void DisplayTitle(void);
void _GPIO_Init(void);
void USART1_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);
void _DMA_Init(void);

void SerialPutChar(uint8_t c);
void Serial_PutString(char *s);

uint16_t KEY_Scan(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t USART_value[6];
char tx_ch[4] = {0,};

int main(void)
{
	_GPIO_Init();
	USART1_Init();
	_DMA_Init();
	LCD_Init();		// LCD 구동 함수
	DelayMS(100);	// LCD구동 딜레이
    
	GPIOG->ODR &= 0x00;// LED0~7 Off 
          
	//LCD 초기화면구동 함수
	DisplayTitle();
	GPIOB->ODR &= ~0x0200;
    
	while(1)
	{
//		LCD_DisplayChar(3,0, USART_value[0]); 
		LCD_DisplayText(3,0, USART_value); 
	}
}


void USART1_Init(void)
{
	RCC->APB2ENR |=	0x0010;	// RCC_APB2ENR USART1 Enable
	RCC->AHB1ENR |= 0x01;		// RCC_AHB1ENR GPIOA Enable
  
	//TX (PA9)
	GPIOA->MODER |= 0x00080000	;// GPIOA PIN9 Output Alternate function mode					
	GPIOA->OSPEEDR |= 0x000C0000;	// GPIOA PIN9 Output speed (100MHz Very High speed)
	GPIOA->OTYPER |= 0x00000000;	// GPIOA PIN9 Output type push-pull (reset state)
	GPIOA->PUPDR |= 0x00040000;	// GPIOA PIN9 Pull-up
	GPIOA->AFR[1] |= 0x0070;		// Connect GPIOA pin9 to AF7(USART1)
    
	//RX (PA10)
	GPIOA->MODER |= 0x200000;	// GPIOA PIN10 Output Alternate function mode
	GPIOA->OSPEEDR |= 0x00300000;	// GPIOA PIN10 Output speed (100MHz Very High speed
	GPIOA->AFR[1]	|= 0x0700;		//Connect GPIOA pin10 to AF7(USART1)
    
	USART_BRR_Configuration(9600);	// USART Baudrate Configuration
    
	USART1->CR1 |= 0x0000;		// USART_WordLength 8 Data bit
	USART1->CR1 |= 0x0000;		// USART_NoParity
	USART1->CR1 |= 0x0004;		// USART_Mode_RX Enable
	USART1->CR1 |= 0x0008;		// USART_Mode_Tx Enable
	USART1->CR2 |= 0x0000;		// USART_StopBits_1
	USART1->CR3 |= 0x0000;		// USART_HardwareFlowControl_None;
	USART1->CR3 |= (1<<6);		// DMA enable receiver
//	USART1->CR3 |= (1<<7);		// DMA enable transmitter
    
	USART1->CR1 |=	0x2000;		// USART1 Enable   
}

void _DMA_Init(void)
{
/*  DMA2 Stream2 CH4 를 USART1 Rx과 연결 */
	RCC->AHB1ENR |= (1<<22);			//DMA2 clock enable
	DMA2_Stream2->CR   |= (4<<25);		//DMA2 Stream2 channel 4 selected  
	DMA2_Stream2->PAR |= (uint32_t)&USART1->DR;    //Peripheral address - USART1 DR Address
	DMA2_Stream2->M0AR |= (uint32_t)USART_value;  	//Memory address - USART_Value
	DMA2_Stream2->NDTR = 4;			//DMA_BufferSize = 4
	  
	DMA2_Stream2->CR &= ~(3<<6);		//Data transfer direction : Peripheral-to-memory
	DMA2_Stream2->CR &= ~(1<<9);		//Peripheral increment mode - Peripheral address pointer is fixed
	DMA2_Stream2->CR |= (1<<10);		//Memory increment mode - Memory address pointer is incremented after each data transferd 
	  
	DMA2_Stream2->CR &= ~(3<<11);	//Peripheral data size -byte(8bit) 
	DMA2_Stream2->CR &= ~(3<<13);	//Memory data size - byte(8bit) : peripheral data size와 동일
	DMA2_Stream2->CR |= (1<<8);		//Circular mode enabled   
	DMA2_Stream2->CR |= (2<<16);		//Priority level - High

	DMA2_Stream2->FCR &= ~(1<<2);	//DMA_FIFO_direct mode enabled
//	DMA2_Stream2->FCR |= (1<<2);		//DMA_FIFO_direct mode disabled : FIFO mode enabled
//	DMA2_Stream2->FCR |= (3<<0);		//DMA_FIFO Threshold_HalfFull , Not used in direct mode
								// Threshold : 일정이상 쌓이면 꺼내가는것 크기를 정해줭함 FIFO의 배수( <=1)

	DMA2_Stream2->CR &= ~(3<<23);	//Memory burst transfer configuration - single transfer
	DMA2_Stream2->CR &= ~(3<<21);	//Peripheral burst transfer configuration - single transfer  
	DMA2_Stream2->CR |= (1<<0);		//DMA2_Stream2 enabled
}

void DisplayTitle(void)
{
	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim8);			//폰트 
	LCD_SetBackColor(RGB_GREEN);	//글자배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색
	LCD_DisplayText(0,0,"USART1-DMA: Rx");	// title

	LCD_SetBackColor(RGB_WHITE);	//글자배경색
}

void SerialPutChar(uint8_t ch) // 1문자 보내기 함수
{
	while((USART1->SR & 0x0080) == RESET); 	// 송신 가능한 상태까지 대기

	USART1->DR = (ch & (uint16_t)0x01FF);	// 전송
}

void Serial_PutString(char *s) // 여러문자 보내기 함수
{
	while (*s != '\0') // 종결문자가 나오기 전까지 구동. 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialPutChar(*s); 	// 포인터가 가르키는 곳의 데이터를 송신
		s++; 			// 포인터 수치 증가
	}
}

void USART_BRR_Configuration(uint32_t USART_BaudRate)
{ 
	uint32_t tmpreg = 0x00;
	uint32_t apbclock = 84000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	/* Determine the integer part */
	if ((USART1->CR1 & 0x8000) != 0)
	{
		/* Integer part computing in case Oversampling mode is 8 Samples */
		integerdivider = ((25 * apbclock) / (2 * (USART_BaudRate)));    
	}
	else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
	{
		/* Integer part computing in case Oversampling mode is 16 Samples */
		integerdivider = ((25 * apbclock) / (4 * (USART_BaudRate)));    
	}
	tmpreg = (integerdivider / 100) << 4;
  
	/* Determine the fractional part */
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	/* Implement the fractional part in the register */
	if ((USART1->CR1 & USART_CR1_OVER8) != 0)
	{
		tmpreg |= ((((fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
	}
	else /* if ((USARTx->CR1 & USART_CR1_OVER8) == 0) */
	{
		tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
	}
	/* Write to USART BRR register */
	USART1->BRR = (uint16_t)tmpreg;
    
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
	GPIOF->MODER 	|=    0x00040000;	// GPIOF 9 : Output mode (0b01)					
	GPIOF->OTYPER 	&= ~0x0200;		// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)
 	GPIOF->OSPEEDR |=  0x00040000;		// GPIOF 9 : Output speed 25MHZ Medium speed 
	
	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR 	|= 0x00000100;		// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER 	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR   &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

void DelayMS(unsigned short wMS)
{
	register unsigned short i;

	for (i=0; i<wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
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


