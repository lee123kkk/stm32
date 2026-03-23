#include "stm32f4xx.h"
#include "GLCD.h"

void RunMenu(void);
void _GPIO_Init(void);
void USART1_Init(void);
void USART_BRR_Configuration(uint32_t USART_BaudRate);

void SerialPutChar(uint8_t c);
void Serial_PutString(char *s);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void DMAInit(void);
uint8_t USART_value[4];

int main(void)
{
	_GPIO_Init();
	USART1_Init();
	DMAInit();
	LCD_Init();	// LCD 구동 함수
	DelayMS(1000);	// LCD구동 딜레이
    
	GPIOG->ODR &= 0x00;// LED0~7 Off 
          
	//LCD 초기화면구동 함수
	RunMenu();
	GPIOB->ODR &= ~0x0200;
    
//	char str[20];

	while(1)
	{
//		LCD_DisplayChar(3,0, USART_value[0]); //원본코드
                LCD_DisplayText(3,0, USART_value); //수정한 코드   

	}
}


void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR	|=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

void USART1_Init(void)
{
	RCC->APB2ENR |=	0x0010;	// RCC_APB2ENR USART1 Enable
  
	//TX(PA9)
	RCC->AHB1ENR |= 0x01;	// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER |= 0x00080000;// GPIOA PIN9 Output Alternate function mode					
	GPIOA->OSPEEDR |= 0x000C0000;// GPIOA PIN9 Output speed (100MHz Very High speed)
	GPIOA->OTYPER |= 0x00000000;// GPIOA PIN9 Output type push-pull (reset state)
	GPIOA->PUPDR |= 0x00040000;// GPIOA PIN9 Pull-up
	GPIOA->AFR[1] |= 0x70;	//Connect GPIOA pin9 to AF7(USART1)
    
	//RX(PA10)
	RCC->AHB1ENR |= 0x01;	// RCC_AHB1ENR GPIOA Enable    				   
	GPIOA->MODER |= 0x200000;// GPIOA PIN10 Output Alternate function mode
	GPIOA->OSPEEDR |= 0x00300000;// GPIOA PIN10 Output speed (100MHz Very High speed
	GPIOA->AFR[1]	|= 0x700;//Connect GPIOA pin10 to AF7(USART1)
    
	USART_BRR_Configuration(9600);	// USART Baudrate Configuration
    
	USART1->CR1 |= 0x0000;	// USART_WordLength 8 Data bit
	USART1->CR1 |= 0x0000;	// USART_Parity_No
	USART1->CR1 |= 0x0004;	// USART_Mode_RX Enable
	USART1->CR1 |= 0x0008;	// USART_Mode_Tx Enable
	USART1->CR2 |= 0x0000;	// USART_StopBits_1
	USART1->CR3 |= 0x0000;	// USART_HardwareFlowControl_None;
	USART1->CR3 |= (1<<6);	// DMA enable receiver
	USART1->CR3 |= (1<<7);	// DMA enable transmitter
    
	//NVIC->ISER[1] 	|= ( 1 << 5 ); 	// Enable Interrupt USART1
	USART1->CR1 |= 	0x20;	// RXNE interrupt Enable
	USART1->CR1 |=	0x2000;	// USART1 Enable   
}

void DMAInit(void)
{
	// DMA2 Stream0 channel2 configuration *************************************
	RCC->AHB1ENR |= (1<<22);			//DMA2 clock enable
	DMA2_Stream2->CR   |= (4<<25);		//DMA2 Stream2 channel 4 selected  100 
	DMA2_Stream2->PAR |= (uint32_t)&USART1->DR;    //Peripheral add   ress - ADC1 Regular data Address
	DMA2_Stream2->M0AR |= (uint32_t)USART_value;  //Memory address - ADC1 Value
	DMA2_Stream2->NDTR = 2;		//DMA_BufferSize = 2    한번에 보내는 글자 수 
	  
	DMA2_Stream2->CR &= ~(3<<6);	//Data transfer direction : Peripheral-to-memory
	DMA2_Stream2->CR &= ~(1<<9);	//Peripheral increment mode - Peripheral address pointer is fixed
	DMA2_Stream2->CR |= (1<<10);	//Memory increment mode - Memory address pointer is incremented after each data transferd 
	  
	DMA2_Stream2->CR &= ~(3<<11);	//Peripheral data size -byte(8bit) 
	DMA2_Stream2->CR &= ~(3<<13);	//Memory data size - byte(8bit) peripheral data size와 동일
	DMA2_Stream2->CR |= (1<<8);	//Circular mode enabled   
	DMA2_Stream2->CR |= (2<<16);	//Priority level - High

	DMA2_Stream2->FCR &= ~(1<<2);	//DMA_FIFO_direct mode enabled  큐 사용 안함 //원본코드 
//	DMA2_Stream2->FCR |= (1<<2);	// 큐 사용함 //수정한 코드 
//      DMA2_Stream2->FCR |= (3<<0);     //추가한 코드 

	DMA2_Stream2->FCR |= (1<<0);	//DMA_FIFO Threshold_HalfFull , Not used in direct mode
					// Threshold : 일정이상 쌓이면 꺼내가는것 크기를 정해줭함 FIFO의 배수( <=1)

	DMA2_Stream2->CR &= ~(3<<23);	//Memory burst transfer configuration - single transfer
	DMA2_Stream2->CR &= ~(3<<21);	//Peripheral burst transfer configuration - single transfer  
	DMA2_Stream2->CR |= (1<<0);	//DMA2_Stream2 enabled
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

void WriteTitle(char* str)
{
	LCD_Clear(RGB_WHITE);
	LCD_SetFont(&Gulim8);
	LCD_SetBackColor(RGB_GREEN);
	LCD_SetTextColor(RGB_BLACK);
    
	LCD_DisplayText(0,0,str);
}

void RunMenu(void)
{
	WriteTitle("USART1");

	LCD_SetFont(&Gulim8);		//폰트 
	LCD_SetBackColor(RGB_WHITE);	//글자배경색
	LCD_SetTextColor(RGB_BLACK);	//글자색

}

void SerialPutChar(uint8_t c) // 1문자 보내기 함수
{
	while((USART1->SR & 0x0080) == RESET); 	// 송신 가능한 상태까지 대기

	USART1->DR = (c & (uint16_t)0x01FF);	// 전송
}


void Serial_PutString(char *s) // 여러문자 보내기 함수
{
	while (*s != '\0') // 종결문자가 나오기 전까지 구동. 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialPutChar(*s); // 포인터가 가르키는 곳의 데이터를 송신
		s++; // 포인터 수치 증가
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
