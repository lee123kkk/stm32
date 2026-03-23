/////////////////////////////////////////////////////////////
// 과제명: HW3. Great Escape(EXTI)
// 과제개요: ROOM 마다 비밀번호(1자리 숫자)를 눌러 탈출
// *******************************
// 사용한 하드웨어(기능): GPIO, Joy-stick, EXTI, GLCD ...
// 제출일: 2024. 6. 8
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////


#include "stm32f4xx.h"
#include "GLCD.h"

#define NAVI_DOWN	 0x0360	  //PI7 0000 0011 0110 0000 

void _GPIO_Init(void);
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
void BEEP(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t	SW0_Flag, SW1_Flag;



int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화

	DisplayInitScreen();	// LCD 초기화면

	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off
	GPIOG->ODR |= 0x0001;	// GPIOG->ODR.0 'H'(LED ON)
    
	while(1)
	{
		//EXTI Example(1) : SW0가 High에서 Low가 될 때(Falling edge Trigger mode) EXTI8 인터럽트 발생, LED0 toggle
		if(SW0_Flag)	// 인터럽트 (EXTI8) 발생
		{
			GPIOG->ODR ^= 0x01;	// LED0 Toggle
			SW0_Flag = 0;

		}
	}
}`



/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
        LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLUE);	// 글자색 : Blue
	LCD_DisplayText(0,0,"2020130030 LSH");  	// 학번 이름
        LCD_SetTextColor(RGB_BLACK);	// 글자색 : Black
	LCD_DisplayText(1, 0,"R 0 1 2 3 4 5 6 7");  	
        LCD_SetPenColor(RGB_BLACK);              // 테두리 검은색
        LCD_DrawRectangle(16, 27, 9, 9);        //첫번째 사각형
        LCD_DrawRectangle(32, 27, 9, 9); 
        LCD_DrawRectangle(48, 27, 9, 9); 
        LCD_DrawRectangle(64, 27, 9, 9); 
        LCD_DrawRectangle(80, 27, 9, 9); 
        LCD_DrawRectangle(96, 27, 9, 9); 
        LCD_DrawRectangle(112, 27, 9, 9); 
        LCD_DrawRectangle(128, 27, 9, 9); 
        LCD_DisplayText(9, 18,"S");  	
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

       //Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR	|= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)  (5~9번 조이스틱 관련)
	GPIOI->PUPDR	&= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

/* EXTI  초기 설정  */
void _EXTI_Init(void)
{  

	RCC->AHB1ENR 	|= 0x80;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x4000;	// Enable System Configuration Controller Clock
	
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)	
        GPIOI->MODER	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)  (5~9번 조이스틱 관련)

        SYSCFG->EXTICR[1] &= ~0xFFFF;    //EXTI 4, 5, 6, 7에 대한 초기화         
	SYSCFG->EXTICR[1] |= 0x8000;	// EXTI7에 대한 소스 입력은 GPIOi로 설정  

        SYSCFG->EXTICR[2] &= ~0xFFFF;    //EXTI 8, 9, 10, 11에 대한 초기화                         
	SYSCFG->EXTICR[2] |= 0x7077;	// EXTI 8, 9, 11에 대한 소스 입력은 GPIOH로 설정, EXTI 10에 대한 소스 입력은 GPIOI로 설정                 

        SYSCFG->EXTICR[3] &= ~0xFFFF;    //EXTI I2, 13, 14, 15에 대한 초기화         
	SYSCFG->EXTICR[3] |= 0x7777;	// EXTI12, 13, 14, 15에 대한 소스 입력은 GPIOH로 설정                




	EXTI->FTSR |= 0x0100;		// EXTI8: Falling Trigger Enable 
	EXTI->FTSR |= 0x0200;		// EXTI 9 Falling Trigger Enable 
 
       
        if(GPIOI->IDR |= 0x0360)
        {
         EXTI->FTSR |= 0x0080;		// EXTI 7  Falling Trigger Enable 
        }        	
    
        EXTI->FTSR |= 0x0800;		// EXTI 11: Falling Trigger Enable                 
        EXTI->FTSR |= 0x1000;		// EXTI 12: Falling Trigger Enable             
        EXTI->FTSR |= 0x2000;		// EXTI 13: Falling Trigger Enable          
        EXTI->FTSR |= 0x4000;		// EXTI 14: Falling Trigger Enable        
        EXTI->FTSR |= 0x8000;		// EXTI 15: Falling Trigger Enable         

        EXTI->IMR  |= 0x0100;		// EXTI8,인터럽트 mask (Interrupt Enable) 설정
	

	NVIC->ISER[0] |= ( 1<<23  );	// Enable 'Global Interrupt EXTI8,9'
					// Vector table Position 참조
        NVIC->ISER[1] |= ( 1<<(40-32)  );   //40번              
}



/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		

         if(EXTI->PR & 0x0100)			// EXTI8 Interrupt Pending(발생) 
	{
                LCD_DisplayText(9, 18,"W"); 
		EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		SW0_Flag = 1;			// SW0_Flag: EXTI8이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용) 
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(17, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x0100;  //EXTI8,인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x0200;		// EXTI9인터럽트 mask (Interrupt Enable) 설정
	}

        if(EXTI->PR & 0x0200)			// EXTI 9 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x0200;
                GPIOG->ODR ^= 0x02;		// LED1 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(33, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x0200;  //EXTI 9,인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x0080;		// EXTI 7인터럽트 mask (Interrupt Enable) 설정

        }

        if(EXTI->PR & 0x0080)			// EXTI 7 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x0080;       
	
                GPIOG->ODR ^= 0x04;		// LED2Toggle
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(49, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x0080;             //EXTI 7인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x0800;		// EXTI 11인터럽트 mask (Interrupt Enable) 설정
        }
}

void EXTI15_10_IRQHandler(void)                
{

       


         if(EXTI->PR & 0x0800)			// EXTI 11 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x0800;
                GPIOG->ODR ^= 0x08;		// LED3 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(65, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x0800;             //EXTI 11인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x1000;		// EXTI 12인터럽트 mask (Interrupt Enable) 설정
        }

         if(EXTI->PR & 0x1000)			// EXTI 12 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x1000;
                GPIOG->ODR ^= 0x10;		// LED4 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(81, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x1000;             //EXTI 12 인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x2000;		// EXTI 13 인터럽트 mask (Interrupt Enable) 설정
        }

         if(EXTI->PR & 0x2000)			// EXTI 13 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x2000;
                GPIOG->ODR ^= 0x20;		// LED5 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(97, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x2000;             //EXTI 13 인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x4000;		// EXTI 14 인터럽트 mask (Interrupt Enable) 설정
        }

         if(EXTI->PR & 0x4000)			// EXTI 14 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x4000;
                GPIOG->ODR ^= 0x40;		// LED6 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(113, 28, 8, 8);
                BEEP();
                EXTI->IMR &= ~0x4000;             //EXTI 14 인터럽트 mask (Interrupt disble) 설정
                EXTI->IMR  |= 0x8000;		// EXTI 15 인터럽트 mask (Interrupt Enable) 설정
        }

        if(EXTI->PR & 0x8000)			// EXTI 15 Interrupt Pending(발생) 
        {
                EXTI->PR |= 0x8000;
                GPIOG->ODR ^= 0x80;		// LED7 Toggle	
		LCD_SetBrushColor(GET_RGB(255, 0, 200));     //내부 분홍색
                LCD_DrawFillRect(129, 28, 8, 8);
                LCD_DisplayText(9, 18,"C"); 
                DelayMS(1000);      //1초 대기
                BEEP();
                DelayMS(500);
                BEEP();
                DelayMS(500);
                BEEP();
                EXTI->IMR &= ~0x8000;             //EXTI 14 인터럽트 mask (Interrupt disble) 설정
                DelayMS(3000);
                LCD_SetBrushColor(RGB_WHITE);  
                LCD_DrawFillRect(17, 28, 8, 8);
                LCD_DrawFillRect(33, 28, 8, 8);
                LCD_DrawFillRect(49, 28, 8, 8);
                LCD_DrawFillRect(65, 28, 8, 8);
                LCD_DrawFillRect(81, 28, 8, 8);
                LCD_DrawFillRect(97, 28, 8, 8);
                LCD_DrawFillRect(113, 28, 8, 8);
                LCD_DrawFillRect(129, 28, 8, 8);

                GPIOG->ODR &= ~0xFF;//모든 led off
                EXTI->IMR  |= 0x0100;	
                LCD_DisplayText(9, 18,"S"); 

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

uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{ 
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if(key == 0x03E0)		// if no key, check key off
	{	if(joy_flag == 0)
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