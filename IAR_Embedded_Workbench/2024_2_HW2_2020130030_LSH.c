/////////////////////////////////////////////////////////////
// 과제번호: 마이컴 응용 2024 HW2
// 과제명:디지털 시계 제작(StopWatch 기능포함)
// 과제개요:  과제에 대한 간단한 설명과 사용되는 Timer 사양등을기재
// *******************************
// 사용한 하드웨어(기능): GPIO, Joy-stick, EXTI, GLCD, Update Interrupt
// 제출일: 2024. 9. 22
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////


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

void _RCC_Init(void);
void _GPIO_Init(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
uint16_t KEY_Scan(void);     
uint16_t JOY_Scan(void);
void DisplayInitScreen(void);
void BEEP(void);
void _EXTI_Init(void);
void TIMER10_Init(void);   //Timer10
void IMER2_OC_Init(void); //Timer2_OC mode 설정 함수

uint8_t	Second=20;   //1초 
uint8_t	mSecond=0;  //10ms
uint8_t	S_Second=0;   // stopwatch 1초 
uint8_t	S_mSecond=0;  //stopwatch 10ms
uint8_t	SW_S=0;  //stopwatch  상태


int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
	DisplayInitScreen();	  // LCD 초기화면
        TIMER10_Init(); // 범용타이머(TIM10) 초기화 : up counting mode


        while(1)
         {
           mSecond = TIM10->CNT;
           S_mSecond = TIM2->CNT;     
           
         } 
    
}



/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
//노란 배경 검은 글씨
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색
	LCD_SetTextColor(RGB_BLUE);	// 글자색
	LCD_DisplayText(0,0,"Digital Watch");  	//제목
        LCD_DisplayText(2,0,"Time");
        LCD_DisplayText(3,0,"STW");

        LCD_SetTextColor(RGB_GREEN);	// 글자색
        LCD_DisplayText(1,0,"2020130030 LSH");  //학번, 이름

        LCD_SetTextColor(RGB_BLACK);	// 글자색
        LCD_DisplayText(2,5,"20:0");
        LCD_DisplayText(3,5,"00:0");

        LCD_SetPenColor(RGB_BLACK);    //그림 색
        LCD_DrawRectangle(0, 0, 125, 25);   // 오목판 최외각 

}



/* GPIO 초기 설정	*/ 
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW(GPIO H) 설정 : Input mode
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7)에 Clock Enable							
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
 

        SYSCFG->EXTICR[2] &= ~0xFFFF;    //EXTI 8, 9, 10, 11에 대한 초기화                         
	SYSCFG->EXTICR[2] |= 0x7000;	// EXTI 11에 대한 소스 입력은 GPIOH로 설정,          

        SYSCFG->EXTICR[3] &= ~0xFFFF;    //EXTI I2, 13, 14, 15에 대한 초기화         
	SYSCFG->EXTICR[3] |= 0x0007;	// EXTI12에 대한 소스 입력은 GPIOH로 설정                




	EXTI->FTSR |= 0x1000;		// EXTI11: Falling Trigger Enable 
  	EXTI->FTSR |= 0x2000;		// EXTI 12 Falling Trigger Enable         
            

        EXTI->IMR  |= 0x0800;		// EXTI 11인터럽트 mask (Interrupt Enable) 설정
        EXTI->IMR  |= 0x1000;		// EXTI 12인터럽트 mask (Interrupt Enable) 설정
	

	NVIC->ISER[0] |= ( 1<<23  );	// Enable 'Global Interrupt EXTI8,9'
					// Vector table Position 참조
        NVIC->ISER[1] |= ( 1<<(40-32)  );   //40번       

    
}



void EXTI15_10_IRQHandler(void)                
{
 if(SW_S=0)         //중단 상태일때
 {
   if(EXTI->PR & 0x0800)			// EXTI11 Interrupt Pending(발생)
     {
        EXTI->PR = 0x0800;  // EXTI11 인터럽트 플래그 클리어
        GPIOG->ODR = 0x0008;	//  LED3 ON
        GPIOG->ODR &= ~0x0010;// LED4 OFF
        BEEP();
        SW_S=1;                                          //작동상태로 전환
        EXTI->IMR  &= ~0x1000;        //EXTI 12 인터럽트 마스크 disable
        
     
     }
  if(EXTI->PR & 0x1000)			// EXTI12 Interrupt Pending(발생)
     {

        S_Second=0;   
        S_mSecond=0; 
        LCD_DisplayText(3,5,"00:0"); 

        EXTI->PR = 0x1000;  // EXTI12 인터럽트 플래그 클리어

     }

 }

 if(SW_S=1)         //작동 상태일때
 {
   if(EXTI->PR & 0x0800)			// EXTI11 Interrupt Pending(발생)
     {

        GPIOG->ODR = 0x0010;	//  LED4 ON
        GPIOG->ODR &= ~0x0008;// LED3 OFF
        BEEP();
        SW_S=0;                                             //중단 상태로 전환
        EXTI->IMR  |= 0x1000;		// EXTI 12인터럽트 mask (Interrupt Enable) 설정 
        EXTI->PR = 0x0800;  // EXTI11 인터럽트 플래그 클리어
     } 
 }

}




void TIMER10_Init(void)   
{
// Enable Timer CLK 
	RCC->APB2ENR |= (1 << 17);	// RCC_APB1ENR TIMER10 Enable

// Setting CR1 : 0x0000 
	TIM10->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)   
	TIM10->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
	TIM10->CR1 &= ~(1<<2);	// URS=0(Update Request Source  Selection): By one of following events
	TIM10->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM10->CR1 &= ~(1<<7);	// ARPE=0(ARR is NOT buffered) (reset state)
	TIM10->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM10->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
				// Center-aligned mode: The counter counts UP and DOWN alternatively

// Deciding the Period

        TIM10->PSC = 840-1;	// Prescaler 84,000,000Hz/840 = 10,0000 Hz (0.01ms)  (1~65536) 
        TIM10->ARR = 10000-1;	 //0.01ms * 10000 =100ms(0.1초)

// Clear the Counter
	TIM10->EGR |= (1<<0);	// UG(Update generation)=1 
				// Re-initialize the counter(CNT=0) & generates an update of registers   

// Setting an UI(UEV) Interrupt 
	NVIC->ISER[0] |= (1<< 25); 	// Enable Timer10 global Interrupt
 	TIM10->DIER |= (1<<0);	// Enable the Tim10 Update interrupt

	TIM10->CR1 |= (1<<0);	// Enable the Tim10 Counter (clock enable)                       클락 동작 시작
}




void TIM10_IRQHandler(void)  	// 1ms Interrupt         
{

 if (TIM10->SR & (1<<0)) 
    {
        TIM10->SR &= ~(1<<0);   // Clear the interrupt flag
        
        if(mSecond<9)
        {
            mSecond++;
        } 
        else 
        {
            mSecond = 0;
            Second++;
            if(Second >= 24) {  // 24초에 반복
                Second = 0; 
            }
        }

        char Second_T[3];  
        snprintf(Second_T, sizeof(Second_T), "%02d", Second); // 문자열로 변경
        
        LCD_DisplayText(2, 5, Second_T); // 초 표시      
        LCD_DisplayChar(2, 8, mSecond + 0x30); // 0.1초 표시
    }

}


void TIMER2_OC_Init(void)
{
// PB10: TIM2_CH3
// Pb10을 출력설정하고 Alternate function(TIM4_CH1)으로 사용 선언
	RCC->AHB1ENR	|= (1<<1);	// 0x02, RCC_AHB1ENR GPIOBEnable : 

	GPIOB->MODER    |= (2<<20);	//  Output Alternate function mode 					
	GPIOB->OSPEEDR 	|= (3<<20);	//  Output speed (100MHz High speed)
	GPIOB->OTYPER	&= ~(1<<10);	//  Output type push-pull (reset state)
	GPIOB->PUPDR    |= (1<<20); 	// Pull-up
	GPIOB->AFR[1]	|= 0x00000300;  // (AFR[1].(11~8)=0b0010): Connect TIM2 pins(PB10) to AF3(TIM8..10)
 
// Time base 설정
	RCC->APB1ENR |= (1<<1);	// 0x01, RCC_APB1ENR TIMER2 Enable

	// Setting CR1 : 0x0000 
	TIM2->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)
	TIM2->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
	TIM2->CR1 &= ~(1<<2);	// URS=0(Update event source Selection): one of following events
	TIM2->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM2->CR1 |= (1<<7);	// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM2->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM2->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	// Setting the Period
	TIM2->PSC = 16800-1;	
	TIM2->ARR = 500-1;	
	// Update(Clear) the Counter
	TIM2->EGR |= (1<<0);    // UG: Update generation    

// Output Compare 설정
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM2->CCMR1 &= ~(3<<0); // CC1S(CC1 channel) = '0b00' : Output 
	TIM2->CCMR1 &= ~(1<<2); // OC1FE=0: Output Compare 1 Fast disable 
	TIM2->CCMR1 &= ~(1<<3); // OC1PE=0: Output Compare 1 preload disable(CCR1에 언제든지 새로운 값을 loading 가능) 
	TIM2->CCMR1 |= (3<<4);	// OC1M=0b011 (Output Compare 1 Mode : toggle)  

				
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM2->CCER |= (1<<0);	// CC1E=1: CC1 channel Output Enable  
				// OC1(TIM2_CH1) Active: 해당핀(100번)을 통해 신호출력
	TIM2->CCER &= ~(1<<1);	// CC1P=0: CC1 channel Output Polarity (OCPolarity_High : OC1으로 반전없이 출력)  

	// CC1I(CC 인터럽트) 인터럽트 발생시각 또는 신호변화(토글)시기 결정: 신호의 위상(phase) 결정
	// 인터럽트 발생시간(10000 펄스)의 10%(1000) 시각에서 compare match 발생
	TIM2->CCR1 = 1000;	// TIM2 CCR1 TIM4_Pulse

	TIM2->DIER |= (1<<0);	// UIE: Enable Tim4 Update interrupt
	TIM2->DIER |= (1<<1);	// CC1IE: Enable the Tim2 CC1 interrupt

	NVIC->ISER[0] 	|= (1<<28);	// Enable Timer2global Interrupt on NVIC

	TIM2->CR1 |= (1<<0);	// CEN: Enable the Tim4 Counter  					
}


void TIM2_IRQHandler(void)      //RESET: 0
{
	if ((TIM2->SR & 0x01) != RESET)	// Update interrupt flag (10ms)
	{
		TIM2->SR &= ~(1<<0);	// Update Interrupt Claer
                 SW_S=0;
                if(S_mSecond<9)
               {
                  S_mSecond++;
               } 
               else 
               {
                  S_mSecond = 0;
                  S_Second++;
                  if(S_Second >= 99) { 
                      S_Second = 0; 
                  }
               }
               char S_Second_T[3];  
               snprintf(S_Second_T, sizeof(S_Second_T), "%02d", S_Second); // 문자열로 변경
        
              LCD_DisplayText(3, 5, S_Second_T); // 초 표시      
              LCD_DisplayChar(3, 8, S_mSecond + 0x30); // 0.1초 표시

	}

    
	if((TIM2->SR & 0x02) != RESET)	// Capture/Compare 1 interrupt flag
	{
		TIM2->SR &= ~(1<<1);	// CC 1 Interrupt Claer
                SW_S=1;
	}
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

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
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