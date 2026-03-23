////////////////////////////////////////////////////////////////
// HW2: 디지털 시계 제작
// 과제명 : 디지털 시계 제작
// 제출자 : 
// 개요 : TIM10의 UP-Counting mode 와 TIM2-CH3 Output Compare mode를 활용한 시계제작
//		  기본 기능 => 시간(TIME)표시는 총 24초를 표시한다. 이 때 초기값은 20:0으로 지정한다.
//		  PSC 840분주를 이용한다.
//		  추가 기능 => 스탑워치 동작은 총 99.9초까지 표시한다. SW3을 누르면 스탑워치 동작이 실행된다.
//		  이 후 한번 더 누르면 동작이 중단되고 다시 누르면 동작을 재개한다. 중단된 상태에서 SW4를 누르면
//		  초기상태도 되돌아간다.
/////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
void _RCC_Init(void);
void _EXTI_Init(void);
void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
void BEEP(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void TIMER10_Init(void); //TIM10 설정함수
void TIMER2_OC_Init(void); //TIM2_OC mode 설정 함수

uint8_t	SW0_Flag, SW1_Flag;
static int MSEC,MSTW; // 100ms 단위 시간 변수
static int SEC,STW; // s 단위 시간 변수
int STW_Flag;  // 스탑워치 상태 플레그 변수

int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
	DisplayInitScreen();	// LCD 초기화면
	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off
	MSEC = 0; // 0으로 초기 설정
	SEC = 20; // 20으로 초기 설정
	MSTW = 0; // 0으로 초기 설정
	STW = 0; // 0으로 초기 설정
	STW_Flag = 0;
	TIMER10_Init();	//TIM10 설정 함수
	TIMER2_OC_Init();	//TIM2_OC mode 설정 함수
	while (1)
	{
	
	} // while
} // main

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetPenColor(RGB_BLACK); // 팬색 : black
	LCD_DrawRectangle(1, 1, 140, 40); //사각형
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8 
	LCD_SetTextColor(RGB_BLUE); // 글자색 : blue
	LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : yellow
	LCD_DrawText(3,3,"Digital Watch");  	// Title 
	LCD_DrawText(3, 83, "TIME");  	// Title 
	LCD_DrawText(3, 95, "STW");  	// Title 
	LCD_SetTextColor(RGB_GREEN); // 글자색 : green
	LCD_DrawText(3, 15, "20XXXXXXXX XXX");  	// Title 
	LCD_SetTextColor(RGB_BLACK); // 글자색 : black
	LCD_DrawChar(40, 95, (STW/10)+0x30);  	// 10의 자리 숫자
	LCD_DrawChar(48, 95, (STW %10)+0x30);  	// 1의 자리 숫자
	LCD_DrawText(56, 95, ":");  	// Title 
	LCD_DrawChar(64, 95, MSTW + 0x30);  	// 100ms 단위 숫자

}
void TIMER10_Init(void)
{
	RCC->APB2ENR |= 0x20000;	// RCC_APB2ENR TIMER10 Enable
 
	TIM10->CR1 &= ~(1 << 4);	// DIR=0 (Up counter)
	TIM10->CR1 &= ~(1 << 1);	// Update event enabled
	TIM10->CR1 &= ~(1 << 2);	// Any of the following events generate an update interrupt or DMA request if enabled
	TIM10->CR1 &= ~(1 << 3);    // The counter is not stopped at update event
	TIM10->CR1 &= ~(1 << 7);	// ARR is not buffered
	TIM10->CR1 &= ~(3 << 8);    // Tpts=Tck_int
	TIM10->CR1 &= ~(3 << 5);    // Edge-aligned mode
 	
	TIM10->PSC = 840 - 1;	// PSC 840분주
	TIM10->ARR = 20000 - 1;		// 200000Hz/20000 = 10Hz => 1/(10Hz) = 100ms
	
	TIM10->EGR |= (1 << 0);	// UG(Update generation)=1 
	  

// Setting an UI(UEV) Interrupt 
	NVIC->ISER[0] |= (1 << 25); 	// Enable Timer10 global Interrupt
	TIM10->DIER |= (1 << 0);	// Enable the Tim10 Update interrupt

	TIM10->CR1 |= (1 << 0);	// Enable the Tim10 Counter (clock enable)
}

void TIMER2_OC_Init(void) // PB10: TIM2_CH3
{

	RCC->AHB1ENR |= (1 << 1);		// GPIOB Enable

	GPIOB->MODER |= (2 << 20);		// PB10을 Output Alternate function mode로 설정  					
	GPIOB->OSPEEDR |= (3 << 20);	// GPIOB PIN10 Output speed (100MHz High speed)	
	GPIOB->OTYPER &= ~(1 << 10);	// GPIOB PIN10 Output type push-pull (reset state)	
	GPIOB->PUPDR |= (1 << 20); 		// GPIOB PIN10 Pull-up
	GPIOB->AFR[1] |= 0x100;		// Connect TIM2 Pins(PB10) to AF1

	RCC->APB1ENR |= (1 << 0);       // RCC_APB1ENR TIMER2 Enable
 
	TIM2->CR1 &= ~(1 << 4);	 // DIR=0 (Up counter)
	TIM2->CR1 &= ~(1 << 1);	// Update event enabled
	TIM2->CR1 &= ~(1 << 2);	// Any of the following events generate an update interrupt or DMA request if enabled
	TIM2->CR1 &= ~(1 << 3);	// The counter is not stopped at update event
	TIM2->CR1 |= (1 << 7);	// ARR is buffered
	TIM2->CR1 &= ~(3 << 8);	// Tpts=Tck_int	
	TIM2->CR1 &= ~(3 << 5);	// Edge-aligned mode	

	TIM2->PSC = 16800 - 1;	// PSC 16800분주
	TIM2->ARR = 500 - 1;	// 5000 / 500 = 10Hz => 1/(10Hz) = 100ms

	TIM2->EGR |= (1 << 0);	// UG(Update generation)=1 

	TIM2->CCMR2 &= ~(3 << 0);		//CC3 channel is configured as output
	TIM2->CCMR2 &= ~(1 << 2);		//Output compare3 fast disable
	TIM2->CCMR2 &= ~(1 << 3);		//Output compare3 preload disable
	TIM2->CCMR2 |= (3 << 4);		//Toggle mode

	TIM2->CCER |= (1 << 8);		//cc3 output enable
	TIM2->CCER &= ~(1 << 9);		//C1P = 0 => 반전 없이 출력

	TIM2->CCR3 = 500;			// 인터럽트 발생시간의 100%시각에서 compare match 발생

	TIM2->DIER |= (1 << 0);		// Enable TIM2 Update interrupt
	TIM2->DIER |= (1 << 3);		// Enable TIM2 CC3 interrupt

	NVIC->ISER[0] |= (1 << 28);		// Enable TIM2 global interrupt on NVIC

	TIM2->CR1 &= ~(1 << 0);		// disable the TIM2 Counter
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)
{
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	// LED (GPIO G) 설정 : Output mode
	GPIOG->MODER 	&= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)						
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						

	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	

	GPIOG->OSPEEDR 	&= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
	// PUPDR : Default (floating) 
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

	// Joy Stick 설정 : Input mode
	RCC->AHB1ENR |= 0x00000100;  // RCC_AHB!ENR : GPIOI Enable
	GPIOI->MODER &= ~0x000FFC00; // GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR &= ~0x000FFC00; // GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

/* EXTI 초기 설정  */
void _EXTI_Init(void)
{
	RCC->AHB1ENR 	|= 0x0080;	// RCC_AHB1ENR GPIOH,GPIOI Enable
	RCC->APB2ENR 	|= 0x4000;	// Enable System Configuration Controller Clock
					 

	SYSCFG->EXTICR[2] &= ~0xFFFF; //  비트에 0이 있어서  클리어가 필요하다
	SYSCFG->EXTICR[2] |= 0x7000;	// EXTI8에 대한 소스 입력은 GPIOH로 설정
					// EXTI11 <- PH11
					// EXTICR3(EXTICR[2])를 이용 
					// reset value: 0x0000	
	SYSCFG->EXTICR[3] &= ~0xFFFF; //  비트에 0이 있어서  클리어가 필요하다
	SYSCFG->EXTICR[3] |= 0x0007;	// EXTI15에 대한 소스 입력은 GPIOH로 설정
					// EXTI12 <- PH12
					// EXTICR4(EXTICR[3])를 이용 
					// reset value: 0x0000
	
	EXTI->FTSR |= 0x1800;		// EXTI5,8,15: Rising Trigger  Enable 
	EXTI->IMR  |= 0x800;		// EXTI5,8,15 인터럽트 mask (Interrupt Enable) 설정
		
	NVIC->ISER[1] |= (1 << 8);	//  vector table 40
	// Vector table Position 참조
}

/* EXTI10~15 인터럽트 핸들러 */
void EXTI15_10_IRQHandler(void)
{
		if (EXTI->PR & 0x800)		// EXTI11 Interrupt Pending(발생) 여부?
		{
			EXTI->PR |= 0x800;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
			if (STW_Flag == 0) {	// 초기 상태일 경우
				STW_Flag = 1;		// 플레그 1로 설정
				GPIOG->ODR |= 0x0018;	// LED 3,4 ON
				BEEP();			// BEEP
				EXTI->IMR &= ~0x1000;	// EXTI12 인터럽트 mask (Interrupt Disable) 설정
				TIM2->CR1 |= (1 << 0);	// Enable the TIM2 Counter
			}
			else if (STW_Flag == 1) {	// 동작 상태일 경우
				STW_Flag = 2;			// 플레그 2로 설정
				GPIOG->ODR &= ~(0x0008);// LED3 OFF
				BEEP();			// BEEP
				EXTI->IMR |= 0x1000;	// EXTI12 인터럽트 mask (Interrupt Enable) 설정
				TIM2->CR1 &= ~(1 << 0);	// disable the TIM2 Counter
			}
			else if (STW_Flag == 2) {	// 정지 상태일 경우
				STW_Flag = 1;		// 플레그 1로 설정
				GPIOG->ODR |= 0x0018;	// LED 3,4 ON
				BEEP();			// BEEP
				EXTI->IMR &= ~0x1000;   // EXTI12 인터럽트 mask (Interrupt Disable) 설정
				TIM2->CR1 |= (1 << 0);	// Enable the TIM2 Counter
			}
		}
		else if (EXTI->PR & 0x1000)			// EXTI12 Interrupt Pending(발생) 여부?
		{
			EXTI->PR |= 0x1000;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
			GPIOG->ODR &= ~0x00FF;	// LED OFF
			STW_Flag = 0;			// 플레그 0으로 설정
			BEEP();				// BEEP
			EXTI->IMR &= ~0x1000;	// EXTI12 인터럽트 mask (Interrupt Disable) 설정
			MSTW = 0;			// 스탑워치 시간 0으로 초기화
			STW = 0;				// 스탑워치 시간 0으로 초기화
			LCD_SetTextColor(RGB_BLACK); // 글자색 : black
			LCD_DrawChar(40, 95, (STW / 10) + 0x30);  	// Title 
			LCD_DrawChar(48, 95, (STW % 10) + 0x30);  	// Title 
			LCD_DrawText(56, 95, ":");  	// Title 
			LCD_DrawChar(64, 95, MSTW + 0x30);  	// Title
		}
}

void TIM1_UP_TIM10_IRQHandler(void) {
	TIM10->SR &= ~(1 << 0);		// Interrupt flag clear
	MSEC++;						// 100ms 시간 증가
	if (MSEC > 9) {				// 9보다 큰경우
		SEC++;					// s 시간 증가
		if (SEC > 23)SEC = 0;	// 23보다 큰 경우 0으로 변경
		MSEC = 0;				// 100ms 시간 0으로 변경
	}
	LCD_SetTextColor(RGB_BLACK); // 글자색 : black
	LCD_DrawChar(40, 83, (SEC / 10) + 0x30);  	// Title 
	LCD_DrawChar(48, 83, (SEC % 10) + 0x30);  	// Title 
	LCD_DrawText(56, 83, ":");  	// Title 
	LCD_DrawChar(64, 83, MSEC + 0x30);  	// Title
}

void TIM2_IRQHandler(void) {
	if ((TIM2->SR & 0x01) != RESET) {	// Update interrupt flag 
		TIM2->SR &= ~(1<<0);			// Update interrupt clear
		LCD_SetTextColor(RGB_BLACK); // 글자색 : black
		LCD_DrawChar(40, 95, (STW / 10) + 0x30);  	// Title 
		LCD_DrawChar(48, 95, (STW % 10) + 0x30);  	// Title 
		LCD_DrawText(56, 95, ":");  	// Title 
		LCD_DrawChar(64, 95, MSTW + 0x30);  	// Title
	}if ((TIM2->SR & 0x08) != RESET) {	// Capture/Compare 3 interrupt flag
		TIM2->SR &= ~(1 << 3);			// CC3 interrupt clear
		MSTW++;							// 100ms 시간 증가
		if (MSTW > 9) {					// 9보다 큰 경우
			STW++;						// s 시간 증가
			if (STW > 99)STW = 0;		// 99보다 큰 경우 0으로 설정
			MSTW = 0;					// 100ms 시간 0으로 설정
		}
		LCD_SetTextColor(RGB_BLACK); // 글자색 : black
		LCD_DrawChar(40, 95, (STW / 10) + 0x30);  	// Title 
		LCD_DrawChar(48, 95, (STW % 10) + 0x30);  	// Title 
		LCD_DrawText(56, 95, ":");  	// Title 
		LCD_DrawChar(64, 95, MSTW + 0x30);  	// Title
	}
}

/* Switch가 입력되었는지 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{	if(key_flag == 0)
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

uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if (key == 0x03E0)		// if no key, check key off
	{
		if (joy_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			joy_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (joy_flag != 0)	// if continuous key, treat as no key input
			return 0x03E0;
		else			// if new key,delay for debounce
		{
			joy_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}

/* Buzzer: Beep for 30 ms */
void BEEP(void)			
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
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
