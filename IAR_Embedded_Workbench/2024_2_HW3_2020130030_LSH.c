//////////////////////////////////////////////////////////////////
// 과제번호: 마이컴 응용 2024 HW3
// 과제명: HW3. 온도(가변저항)연동 냉방기 제어
// 과제개요: 온도센서(가변저항)로부터 출력되는 아날로그신호의 전압을AD변환함.
//        변환 결과값(디지털값)으로부터 센서의 전압값 및 온도값을계산하여LCD에 표시
//        온도에 비례하여 냉방기의 냉각기를 제어함
// 사용한 하드웨어(기능): GPIO, , EXTI, GLCD, ADC, PWM
// 제출일: 2024. 11. 11
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
/////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
void _RCC_Init(void);
void _EXTI_Init(void);
void DisplayInitScreen(void);
void _ADC_Init(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
void BEEP(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void TIMER1_Init(void);
void TIMER14_PWM_Init(void);
void UpdateLCD(void);

float_t voltage = 0.00;
float_t temperature = 0.0;
float_t dutyRatio = 0.0;
uint8_t  LCD_update_flag= 0;
volatile uint8_t pwmEnabled = 0;

//uint8_t	SW0_Flag, SW1_Flag;
//static int MSEC,MSTW; // 100ms 단위 시간 변수
//static int SEC,STW; // s 단위 시간 변수
//int STW_Flag;  // 스탑워치 상태 플레그 변수

int main(void)//수정필요//
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
        _ADC_Init();   // ADC 초기화
	DisplayInitScreen();	// LCD 초기화면
        TIMER1_Init();  
        TIMER14_PWM_Init(); 


	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off

	while (1)
	{
	    if (LCD_update_flag)
           {
                 UpdateLCD();
                 LCD_update_flag = 0;
           }
	} // while

} // main

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim7);		// 폰트 : 굴림 7 
        LCD_SetTextColor(RGB_BLACK);
	LCD_SetPenColor(RGB_GREEN); // 그림 색 : 초록색
        LCD_SetBrushColor(RGB_GREEN);   //초록색
	LCD_SetBackColor(RGB_WHITE);	// 글자배경색 : 흰색
	LCD_DisplayText(0, 0,"Air Conditioner Control"); // 제목
        LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8 
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : 노란색
	LCD_DisplayText(1, 0,"2020130030 LSH");  
	LCD_DisplayText(2, 0,"VOL : 0.00V");
 	LCD_DisplayText(3, 0,"TMP : 0.00C"); 
	LCD_DisplayText(4, 0,"DR :  0.0%");

        LCD_DrawRectangle(10, 100, 110, 10); //DR값 퍼센트  테두리


}

void _ADC_Init(void)
{	// ADC3: PA1(pin 41)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	// (1<<0) ENABLE GPIOA CLK 
	GPIOA->MODER |= (3<<2*1);		// CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE
						
	RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;	// (1<<8) ENABLE ADC3 CLK 

	ADC->CCR &= ~(0X1F<<0);		// MULTI[4:0]: ADC_Mode_Independent
	ADC->CCR |=  (1<<16); 		// 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 21MHz)
        
	ADC3->CR1 &= ~(3<<24);		// RES[1:0]=0b00 : 12bit Resolution
	ADC3->CR1 &= ~(1<<8);		// SCAN=0 : ADC_ScanCovMode Disable
	ADC3->CR1 |=  (1<<5);		// EOCIE=1: Interrupt enable for EOC   //EOC 인터럽트 활성화

	ADC3->CR2 &= ~(1<<1);		// CONT=0: ADC_Continuous ConvMode Disable  //연속실행 비활성화

	ADC3->CR2 &= ~(3<<28);		// EXTEN[1:0]=0b00:  // 외부 트리거 비활성화 제거
        ADC3->CR2 |= (1<<28);           // 외부 트리거 상승 에지 활성화
        ADC3->CR2 |= (2<<24);           // 외부 트리거 소스를 TIM1_CC3로 설정

	ADC3->CR2 &= ~(1<<11);		// ALIGN=0: ADC_DataAlign_Right
	ADC3->CR2 &= ~(1<<10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC3->SQR1 &= ~(0xF<<20);	// L[3:0]=0b0000: ADC Regular channel sequece length 
    
	ADC3->SMPR2 |= (0x4<<(3*1));	// 샘플링 시간 조정: 84 사이클로 설정 (약 1μs @ 84MHz)

	ADC3->SQR3 |= (1<<0);		// 채널 선택  SQ1[4:0]=0b0001 : CH1

	NVIC->ISER[0] |= (1<<18);	// ADC 전역 인터럽트 활성화

	ADC3->CR2 |= (1<<0);		// ADON: ADC ON

}



void TIMER1_Init(void)  //PB1
{
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;	// RCC_APB2ENR TIMER1 Enable
 
	TIM1->CR1 &= ~(1 << 4);	// DIR=0 (Up counter)
	TIM1->CR1 &= ~(1 << 1);	// Update event enabled
	TIM1->CR1 &= ~(1 << 2);	// Any of the following events generate an update interrupt or DMA request if enabled
	TIM1->CR1 &= ~(1 << 3);    // The counter is not stopped at update event
	TIM1->CR1 |= (1 << 7);	// ARR 프리로드 활성화
	TIM1->CR1 &= ~(3 << 8);    // Tpts=Tck_int
	TIM1->CR1 &= ~(3 << 5);    // Edge-aligned mode
 	
	TIM1->PSC = 8400 - 1;	// PSC 84MHz / 8400 = 10kHz 분주
	TIM1->ARR = 2000 - 1;	// 10kHz/2000 = 5Hz (200ms)
	
	TIM1->EGR |= (1 << 0);	// UG(Update generation)=1 
	  
       TIM1->CCMR2 &= ~TIM_CCMR2_CC3S;  // CC3 채널을 출력으로 설정
       TIM1->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;  // PWM 모드 1
       TIM1->CCMR2 |= TIM_CCMR2_OC3PE;  // 프리로드 활성화
       TIM1->CCER |= TIM_CCER_CC3E;  // CC3 출력 활성화
       TIM1->CCER &= ~TIM_CCER_CC3P; // 활성 high

       TIM1->CCR3 = 1000;  // 50% 듀티 사이클 (1000 / 2000)
       TIM1->CR1 |= TIM_CR1_CEN; // 타이머 활성화

// Setting an UI(UEV) Interrupt 
	NVIC->ISER[0] |= (1 << 25); 	// Enable Timer10 global Interrupt
	TIM1->DIER |= (1 << 0);	// Enable the Tim1 Update interrupt

	TIM1->CR1 |= (1 << 0);	// Enable the Tim1 Counter (clock enable)
}


void TIMER14_PWM_Init(void) //PF1
{
// Clock Enable : GPIOF & TIMER14
	RCC->AHB1ENR	|= (1<<5);	// GPIOF CLOCK Enable
	RCC->APB1ENR 	|= (1<<8);	// TIMER14 CLOCK Enable 
    						
// PF1을 출력설정하고 Alternate function(TIM14_CH1)으로 사용 선언 : PWM 출력
	GPIOF->MODER 	|= (2<<2);	//  Output Alternate function mode					
	GPIOF->OSPEEDR 	|= (3<<2);	//  Output speed (100MHz High speed)
	GPIOF->OTYPER	&= ~(1<<1);	//  Output type push-pull (reset state)
	GPIOF->PUPDR	|= (1<<2);	// 0x00010000 PB8 Pull-up
 	GPIOF->AFR[1]	|= (9<<4);	//  AF9 for TIM14
    
	TIM14->PSC	= 84-1;	//  84MHz / 84 = 1MHz
	TIM14->ARR	= 1000-1;	// Auto reload  1MHz/1000 = 1kHz

// PWM 모드 설정
    TIM14->CCMR1 &= ~TIM_CCMR1_CC1S;  // CC1 채널을 출력으로 설정
    TIM14->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;  // PWM 모드 1
    TIM14->CCMR1 |= TIM_CCMR1_OC1PE;  // 프리로드 활성화

    // 출력 설정
    TIM14->CCER |= TIM_CCER_CC1E;  // CC1 출력 활성화
    TIM14->CCER &= ~TIM_CCER_CC1P;  //  high 

    // CR1 설정
    TIM14->CR1 &= ~TIM_CR1_DIR;  // 업 카운터
    TIM14->CR1 |= TIM_CR1_ARPE;  // ARR 프리로드 활성화
    TIM14->CR1 &= ~TIM_CR1_CKD;  // 클럭 분주 없음

    TIM14->CCR1 = 0;// 초기 듀티 사이클 설정 (0%)
   
    TIM14->CR1 |= TIM_CR1_CEN;  // 타이머 활성화
    TIM14->CCER &= ~TIM_CCER_CC1E;  // 초기에 PWM 출력 비활성화
}



/* GPIO , GPIOH(Switch), GPIOF(Buzzer)) 	*/
void _GPIO_Init(void)
{
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
//	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
//	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state
        GPIOH->MODER &= ~(3UL<<(2*7));  // GPIOH 7번 핀 입력 모드
        GPIOH->PUPDR |= (1UL<<(2*7));   // GPIOH 7번 핀 풀업 설정

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


void _EXTI_Init(void)//함수: SW7에 대한 외부 인터럽트
{
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;      // SYSCFG 클럭 활성화
        SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI7_PH;  // EXTI7 라인을 PH7에 연결
        EXTI->FTSR |= EXTI_FTSR_TR7;     // 하강 에지에서 트리거
        EXTI->IMR |= EXTI_IMR_MR7;           // EXTI7 인터럽트 마스크 해제

        NVIC_EnableIRQ(EXTI9_5_IRQn);       // EXTI 9-5 인터럽트 활성화
        NVIC_SetPriority(EXTI9_5_IRQn, 2);  // 우선순위 설정
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR7) 
{
        EXTI->PR |= EXTI_PR_PR7;  // 인터럽트 플래그 클리어

        DelayMS(10);         // 디바운싱을 위한 딜레이

        if (!(GPIOH->IDR & GPIO_IDR_IDR_7)) // SW7이 여전히 눌려있는지 확인
        {
            pwmEnabled = !pwmEnabled; // PWM 상태 토글

            if (pwmEnabled)
            {
                TIM14->CCER |= TIM_CCER_CC1E; // PWM 출력 활성화
            }
            else
            {
                TIM14->CCER &= ~TIM_CCER_CC1E; // PWM 출력 비활성화
            }
        }
    }
}

void ADC_IRQHandler(void)
{
    if (ADC3->SR & ADC_SR_EOC) // ADC 변환 완료 확인
    {  
        uint16_t adcValue = ADC3->DR;  // ADC 변환 결과 읽기
        voltage = (float)adcValue * (3.3f / 4095.0f);  // 전압으로 변환
        temperature = 3.5f * voltage * voltage + 1.0f;  // 온도 계산
        
        // PWM 듀티 비 계산
        dutyRatio = temperature * 2.0f;
        if (dutyRatio > 100.0f) dutyRatio = 100.0f;
        
        if (pwmEnabled)
        {
            TIM14->CCR1 = (uint32_t)((dutyRatio / 100.0f) * (TIM14->ARR + 1));
        }
        else
        {
            TIM14->CCR1 = 0;  // PWM 비활성화 상태에서는 듀티 사이클을 0으로 설정
        }
        LCD_update_flag = 1;


    }
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF)  // 업데이트 인터럽트 발생 확인
    {
        TIM1->SR &= ~TIM_SR_UIF;// 인터럽트 플래그 클리어

        ADC3->CR2 |= ADC_CR2_SWSTART;  // 200ms마다 ADC 변환 시작
    }
}

// LCD 업데이트 함수
void UpdateLCD(void)
{
    char buffer[20];
    
    LCD_SetTextColor(RGB_BLUE);  // 글자색을 파란색으로 설정
    
    // VOL 값 표시
    sprintf(buffer, "%.2f", voltage);
    LCD_DisplayText(2, 6, buffer);
    
    // TMP 값 표시
    sprintf(buffer, "%.1f", temperature);
    LCD_DisplayText(3, 6, buffer);
    
    // DR 값 표시
    sprintf(buffer, "%.1f", dutyRatio);
    LCD_DisplayText(4, 6, buffer);
    
    // 온도에 따른 그래프 업데이트
    LCD_SetBrushColor(RGB_YELLOW);
    LCD_DrawFillRect(10, 100, 110, 10);  // 배경을 노란색으로 채움
    
    LCD_SetBrushColor(RGB_GREEN);
    int barWidth = (int)(dutyRatio / 100.0f * 140);
    int temp = (int)temperature; // 온도의 소수점 무시
    if (temp < 1) temp = 1;
    if (temp > 39) temp = 39;
    LCD_DrawFillRect(10, 99, barWidth, 10);  // DR 값에 따라 녹색으로 채움
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