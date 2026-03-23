/////////////////////////////////////////////////////////////
// HW1: 인터스텔라 시계 제작
// 제출자: 2020130030 이수혁
// 제출일: 2025. 9. 24
// 주요 내용 및 구현 방법
// -지구와 밀러행성의 시계를 표시
// -중력이 다른 밀러행성의 시계를 변경하는 기능
// -지구 시간: TIM5의 Up-Counting mode(100msec) 이용
// -밀러 시간: TIM4(CH2)의 OC mode의 CC2 Interrupt(100ms) 이용
// - 밀러 시계 증가 속도 변경: SW6(EXTI14) 입력으로 TIM4의 ARR 값 변경
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

void TIMER5_Init(void);   //Timer5
void TIMER4_OC_Init(void); //Timer4_OC mode 설정 함수

// 전역 변수 선언
uint8_t earth_s_count = 0; // 지구 시계 100ms 카운트
uint8_t earth_hour = 23;    // 지구 시계 시(hour)
uint8_t earth_min = 50;     // 지구 시계 분(min)

uint8_t miller_s_count = 0; // 밀러 시계 100ms 카운트
uint8_t miller_hour = 23;   // 밀러 시계 시(hour)
uint8_t miller_min = 50;    // 밀러 시계 분(min)

uint8_t miller_period_index = 0; // 밀러 시계 주기 인덱스 (0: 100ms, 1: 200ms, 2: 500ms)


int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
	DisplayInitScreen();	  // LCD 초기화면
        TIMER5_Init(); // 범용타이머(TIM5) 초기화 : up counting mode
        TIMER4_OC_Init();

        while(1)
         {
     
           
         } 
     
    
}



/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
//노란 배경 검은 글씨
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색:노란색 
	LCD_SetTextColor(RGB_BLACK);	// 글자색: 검은 글씨
	LCD_DisplayText(0,0,"LSH 2020130030");  //학번, 이름
        LCD_DisplayText(1,0,">EARTH");
        LCD_DisplayText(2,0,">MILLER");
        LCD_DisplayText(3,0," Int period    ms");

        LCD_SetTextColor(RGB_BLUE);	
        LCD_DisplayText(1,7,"23:50");   //지구 시계

        LCD_SetTextColor(RGB_RED);	//밀러 시계
        LCD_DisplayText(2,7,"23:50");
        LCD_DisplayText(3,12,"100");



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
        
        SYSCFG->EXTICR[3] &= ~0xFFFF;    //EXTI I2, 13, 14, 15에 대한 초기화         
	SYSCFG->EXTICR[3] |= 0x7700;	// EXTI14, EXTI15에 대한 소스 입력은 GPIOH로 설정 (7: GPIOH)               


        EXTI->FTSR |= (1 << 15);    // EXTI15에 대한 Falling Edge Trigger 활성화
        EXTI->FTSR |= (1 << 14);    // EXTI14에 대한 Falling Edge Trigger 활성화

        EXTI->IMR |= (1 << 15);     // EXTI15 인터럽트 마스크 (Interrupt Enable) 설정
        EXTI->IMR |= (1 << 14);     // EXTI14 인터럽트 마스크 (Interrupt Enable) 설정

 
        NVIC->ISER[1] |= (1 <<(40-32));  // EXTI15_10 전역 인터럽트 활성화
    
}




/* 타이머5 초기화 함수 (지구 시계) */
void TIMER5_Init(void)   
{
// Enable Timer CLK 
	RCC->APB1ENR |= (1 << 3);	// RCC_APB2ENR TIMER5 Enable

// Setting CR1 : 0x0000 
	TIM5->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state)   
	TIM5->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled): By one of following events
	TIM5->CR1 &= ~(1<<2);	// URS=0(Update Request Source  Selection): By one of following events
	TIM5->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM5->CR1 &= ~(1<<7);	// ARPE=0(ARR is NOT buffered) (reset state)
	TIM5->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM5->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
				// Center-aligned mode: The counter counts UP and DOWN alternatively

//  타이머 기본 설정
        TIM5->PSC = 42-1;	// Prescaler 84,000,000Hz/42 = 2MHz (0.5us)
        TIM5->ARR = 20000-1;   // Auto-Reload Register: 0.5us * 20,000 = 10ms 주기

//  카운터 초기화 및 레지스터 업데이트
	TIM5->EGR |= (1<<0);	// UG(Update generation)=1 
				// Re-initialize the counter(CNT=0) & generates an update of registers   

//  인터럽트 설정	
 	TIM5->DIER |= (1<<0);	// Enable the Tim5 Update interrupt
        NVIC->ISER[1] |= (1<<(50-32)); 	// Enable Timer5 global Interrupt  //50번
//  타이머 카운터 동작 시작
	TIM5->CR1 |= (1<<0);	// Enable the Tim5 Counter (clock enable)                       클락 동작 시작
}


/* TIM5_IRQHandler: 지구 시계 업데이트 (10ms 주기) */
void TIM5_IRQHandler(void)
{
    // Update Interrupt Flag(UIF) 확인 및 클리어
    if ((TIM5->SR & (1 << 0)) != 0)
    {
        TIM5->SR &= ~(1 << 0); // 플래그 클리어

        // 10ms 펄스를 10개 세어 100ms를 만듦
        earth_s_count++;
        if (earth_s_count >= 10)
        {
            earth_s_count = 0;
            
            // 1분(60초) 업데이트 (과제는 100ms마다 1분 증가를 요구)
            earth_min++;

            if (earth_min >= 60)
            {
                earth_min = 0;
                earth_hour++;

                if (earth_hour >= 24)
                {
                    earth_hour = 0;
                }
            }

            // LCD에 업데이트된 시간 표시
            char time_str[6];
            // snprintf 함수를 사용하여 시간과 분 변수를 "HH:MM" 형식의 문자열로 변환
            // (unsigned int)로 형 변환하여 컴파일러 경고 방지
            snprintf(time_str, sizeof(time_str), "%02u:%02u", earth_hour, earth_min);
            LCD_SetBackColor(RGB_YELLOW);
            LCD_SetTextColor(RGB_BLUE);
            LCD_DisplayText(1, 7, time_str);
        }
    }
}


/* 타이머4 초기화 함수 (밀러 행성 시계) */
void TIMER4_OC_Init(void)
{
    // PB7: TIM4_CH2
    // PB7을 출력설정하고 Alternate function(TIM4_CH2)으로 사용 선언
    RCC->AHB1ENR |= (1 << 1);        // GPIOB 클럭 활성화 (AHB1ENR.1)

    GPIOB->MODER |= (2 << 14);       // GPIOB PIN7을 Alternate function 모드로 설정 (MODER.15-14 = 0b10)
    GPIOB->OSPEEDR |= (3 << 14);     // GPIOB PIN7의 출력 속도를 High speed로 설정 (OSPEEDR.15-14 = 0b11)
    GPIOB->OTYPER &= ~(1 << 7);      // GPIOB PIN7의 출력 타입을 Push-pull로 설정 (OTYPER.7 = 0)
    GPIOB->PUPDR |= (1 << 14);       // GPIOB PIN7을 Pull-up으로 설정 (PUPDR.15-14 = 0b01)
    GPIOB->AFR[0] |= 0x20000000;     // GPIOB PIN7에 TIM4_CH2 기능 연결 (AFR[0].31-28 = 0b0010)

    // Time base 설정
    RCC->APB1ENR |= (1 << 2);        // TIM4 클럭 활성화 (APB1ENR.2)

    // CR1 레지스터 설정 (기본값 사용)
    TIM4->CR1 &= ~(1 << 4);          // DIR=0 (Up counter)
    TIM4->CR1 &= ~(1 << 1);          // UDIS=0 (Update event Enabled)
    TIM4->CR1 &= ~(1 << 2);          // URS=0 (Update Request Source Selection)
    TIM4->CR1 &= ~(1 << 3);          // OPM=0 (The counter is NOT stopped at update event)
    TIM4->CR1 |= (1 << 7);           // ARPE=1 (ARR is buffered)
    TIM4->CR1 &= ~(3 << 8);          // CKD=00 (Clock division)
    TIM4->CR1 &= ~(3 << 5);          // CMS=00 (Edge-aligned mode)

    // 타이머 주기 설정 (84MHz 클럭 기준, 100ms 주기)
    TIM4->PSC = 8400 - 1;            // 84MHz / 8400 = 10KHz (0.1ms 펄스)
    TIM4->ARR = 1000 - 1;            // 0.1ms * 1000 = 100ms 주기

    // 카운터 초기화 및 레지스터 업데이트
    TIM4->EGR |= (1 << 0);           // UG (Update generation) 비트를 설정하여 카운터 초기화

    // Output Compare 설정
    // CCMR1 (Capture/Compare Mode Register 1) 설정: 채널2에 대한 모드 설정
    TIM4->CCMR1 &= ~(3 << 8);        // CC2S=0b00 (Output 모드)
    TIM4->CCMR1 &= ~(1 << 10);       // OC2FE=0 (Output Compare 2 Fast disable)
    TIM4->CCMR1 &= ~(1 << 11);       // OC2PE=0 (Output Compare 2 preload disable)
    TIM4->CCMR1 |= (3 << 12);        // OC2M=0b011 (Toggle mode)

    // CCER (Capture/Compare Enable Register) 설정: 채널2 활성화
    TIM4->CCER |= (1 << 4);          // CC2E=1 (CC2 channel Output Enable)
    TIM4->CCER &= ~(1 << 5);         // CC2P=0 (CC2 channel Output Polarity)

    // CC2I (CC 인터럽트) 인터럽트 활성화
    TIM4->CCR2 = 100;                // TIM4->CNT가 100에 도달하면 인터럽트 발생 및 펄스 토글
    TIM4->DIER |= (1 << 2);          // CC2IE (CC2 인터럽트 활성화)

    // NVIC (Nested Vectored Interrupt Controller) 설정
    NVIC->ISER[0] |= (1 << 30);      // TIM4 전역 인터럽트 활성화 (벡터 포지션 30)

    // 타이머 카운터 동작 시작
    TIM4->CR1 |= (1 << 0);           // CEN (Counter Enable) 비트를 설정하여 타이머 시작
}

/* TIM4_IRQHandler: 밀러 행성 시계 업데이트 (100ms 주기) */
void TIM4_IRQHandler(void)
{
    // CC2 인터럽트 플래그 확인 및 클리어
    if ((TIM4->SR & (1 << 2)) != 0)
    {
        TIM4->SR &= ~(1 << 2); // 플래그 클리어
        
        // 밀러 행성 시간 업데이트
        miller_min++;

        if (miller_min >= 60)
        {
            miller_min = 0;
            miller_hour++;

            if (miller_hour >= 24)
            {
                miller_hour = 0;
            }
        }

        // LCD에 업데이트된 시간 표시
        char time_str[6];
        // snprintf 함수를 사용하여 시간과 분 변수를 "HH:MM" 형식의 문자열로 변환
        // (unsigned int)로 형 변환하여 컴파일러 경고 방지
        snprintf(time_str, sizeof(time_str), "%02u:%02u", (unsigned int)miller_hour, (unsigned int)miller_min);
        LCD_SetBackColor(RGB_YELLOW);
        LCD_SetTextColor(RGB_RED);
        LCD_DisplayText(2, 7, time_str);
    }
}


/* EXTI15_10_IRQHandler: 스위치 입력 처리 */
void EXTI15_10_IRQHandler(void)
{
    // SW7(PH15 / EXTI15): 시간 리셋
    if ((EXTI->PR & (1 << 15)) != 0)
    {
        EXTI->PR |= (1 << 15); 

        // 타이머 정지
        TIM5->CR1 &= ~(1 << 0); 
        TIM4->CR1 &= ~(1 << 0); 

        // LED7 ON, 부저 1회
        GPIOG->ODR |= (1 << 7); // LED7 ON
        BEEP();

        // 3초 딜레이 및 LED7 OFF
        DelayMS(3000);
        GPIOG->ODR &= ~(1 << 7); // LED7 OFF

        // 시간 변수 초기화
        earth_s_count = 0;
        earth_hour = 0;
        earth_min = 0;
        miller_s_count = 0;
        miller_hour = 0;
        miller_min = 0;

        // LCD에 00:00 표시
        LCD_SetBackColor(RGB_YELLOW);
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(1, 7, "00:00");
        LCD_SetTextColor(RGB_RED);
        LCD_DisplayText(2, 7, "00:00");

        // 타이머 카운터 값 초기화
        TIM5->CNT = 0; 
        TIM4->CNT = 0; 

        // 타이머 다시 시작
        TIM5->CR1 |= (1 << 0); 
        TIM4->CR1 |= (1 << 0); 
    }

    // SW6(PH14 / EXTI14): 밀러 시계 속도 변경
    if ((EXTI->PR & (1 << 14)) != 0)
    {
        EXTI->PR |= (1 << 14);

        // 타이머4 정지
        TIM4->CR1 &= ~(1 << 0); 

        // 주기 변경 로직
        miller_period_index = (miller_period_index + 1) % 3;
        
        switch(miller_period_index)
        {

            case 0: // 100ms
                TIM4->PSC = 8400 - 1;
                TIM4->ARR = 1000 - 1;
                LCD_SetTextColor(RGB_RED);
                LCD_DisplayText(3, 12, "100");
                break;
            case 1: // 200ms
                TIM4->PSC = 8400 - 1;
                TIM4->ARR = 2000 - 1;
                LCD_SetTextColor(RGB_RED);
                LCD_DisplayText(3, 12, "200");
                break;
            case 2: // 500ms
                TIM4->PSC = 8400 - 1;
                TIM4->ARR = 5000 - 1;
                LCD_SetTextColor(RGB_RED);
                LCD_DisplayText(3, 12, "500");
                break;
        }

        TIM4->CNT = 0; 
        
        // 타이머4 다시 시작
        TIM4->CR1 |= (1 << 0);
    }
}




//---------------------------

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