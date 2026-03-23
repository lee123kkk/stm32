/////////////////////////////////////////////////////////////
// HW2: STEP 및 DC Motor 구동용 펄스 발생
// 제출자: 2020130030 이수혁
// 제출일: 2025. 10. 31
// 주요 내용 및 구현 방법

//STEP Motor 제어: SW3 입력 횟수를 TIM5 외부 클럭(0~3)으로 카운트, TIM1_CH3 펄스(위치값의 2배) 출력.
// DC Motor 제어: 가변저항(ADC3) 전압을 0~3 Torque 명령값으로 변환, TIM14_CH1 PWM(400us 주기) 출력 (DR 0%~90% 가변).
// 시스템 모니터링: TIM4(100ms) 인터럽트를 이용하여 LCD에 위치 및 토크 명령값 표시
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
//void BEEP(void);
void _EXTI_Init(void);

void TIM5_Init(void);
void TIM4_Init(void);
void TIM1_Init(void);
void ADC3_Init(void);   
void TIM14_Init(void);
// 스텝 모터용 전역 변수 
volatile uint16_t pulse_count = 0; // 현재까지 발생한 펄스 수
volatile uint16_t pulse_goal = 0;  // 목표 펄스 수 (위치값 * 2)
// DC 모터용 전역 변수 
volatile uint16_t torque_command = 0; // 현재 토크 명령값 (0~3)

int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
	DisplayInitScreen();	  // LCD 초기화면

        TIM5_Init();    // TIM5 초기화 함수
        TIM4_Init();    // TIM4 초기화 (100ms 주기 설정)
        TIM1_Init();    // TIM1 초기화 함수 호출 추가 (스텝 모터)
        ADC3_Init();   
        TIM14_Init();

        while(1)
         {
          // 모든 동작은 인터럽트 핸들러에서 처리
           
         } 
     
    
}



/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8

        LCD_SetPenColor(RGB_BLACK);           // 그림 색상: 검은색
        LCD_DrawFillRect(0, 0, 128, 26);

        LCD_SetBackColor(RGB_BLACK);	// 글자배경색:검은색
	LCD_SetTextColor(RGB_WHITE);	// 글자색: 흰 글씨
        LCD_DisplayText(0,0,"Motor Control");   // HW2 제목 
        LCD_DisplayText(1,0,"2020130030 LSH"); // 학번/이름 

        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색:노란색 
	LCD_SetTextColor(RGB_BLACK);	// 글자색: 검은 글씨
        LCD_DisplayText(2,0,">Step Motor");    
        LCD_DisplayText(3,0,"Position:");   
        LCD_DisplayText(4,0,">DC Motor");     
        LCD_DisplayText(5,0,"Torque:");    

	LCD_SetTextColor(RGB_RED);	// 글자색: 빨간 글씨
        LCD_DisplayText(3,9,"0");  //  위치 명령값 
        LCD_DisplayText(5,7,"0"); //  토크 명령값 
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
        
        // Buzzer (GPIO F) 설정 : Output mode            //TIM14_CH1 사용을 위해 Output 설정은 삭제, 클럭 활성화는 유지
	RCC->AHB1ENR	|=  0x00000020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable
/*							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
*/
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
        
   
}


void TIM5_Init(void)
{
    // 1. GPIOH 클럭 활성화 (SW3 핀 PH11이 속한 포트)
    RCC->AHB1ENR |= (1 << 7); // GPIOH EN

    // 2. PH11 핀을 Alternate Function(AF) 모드로 설정
    GPIOH->MODER &= ~(3 << (11 * 2)); // PH11 MODER (비트 23, 22) 초기화 (00: Input)
    GPIOH->MODER |=  (2 << (11 * 2)); // PH11 MODER (비트 23, 22) 설정 (10: AF Mode)

    // 3. PH11 핀을 AF2 (TIM5_CH2) 기능으로 매핑
    //    PH11은 AFR[1] 레지스터의 (11-8)*4 = 12번 비트부터 사용
    GPIOH->AFR[1] &= ~(0xF << ((11 - 8) * 4)); // AFRH11 (비트 15:12) 초기화
    GPIOH->AFR[1] |=  (0x2 << ((11 - 8) * 4)); // AFRH11 (비트 15:12) = 0010 (AF2)

    // 4. TIM5 타이머 클럭 활성화 (APB1 버스)
    RCC->APB1ENR |= (1 << 3); // TIM5 EN

    // 5. TIM5 타이머 기본 설정
    TIM5->PSC = 0;     // 프리스케일러 사용 안 함 (외부 클럭 1개당 1 카운트)
    TIM5->ARR = 3;     // Auto-Reload Register: 3. 카운팅 범위 0~3
    TIM5->CNT = 0;     // 카운터 초기값 0으로 설정

    // 6. TIM5 채널 2 입력 캡처 설정 (CCMR1 레지스터)
    //    CC2S = 01: TIM5_CH2를 입력(TI2)으로 사용
    TIM5->CCMR1 &= ~(3 << 8); // CC2S = 00 (초기화)
    TIM5->CCMR1 |=  (1 << 8); // CC2S = 01 (설정)

    // 7. TIM5 캡처/비교 활성화 설정 (CCER 레지스터)
    //    Falling edge 감지
    TIM5->CCER &= ~(1 << 5); // CC2P = 0 (초기화, Rising edge)
    TIM5->CCER |=  (1 << 5); // CC2P = 1 (Falling edge 감지)
    TIM5->CCER &= ~(1 << 7); // CC2NP = 0 (초기화, non-inverted)

    // 8. TIM5 슬레이브 모드 컨트롤러 설정 (SMCR 레지스터)
    //    External Clock mode 1 설정
    TIM5->SMCR &= ~(7 << 4); // TS = 000 (트리거 선택 초기화)
    TIM5->SMCR |=  (6 << 4); // TS = 110 (TI2FP2 신호 선택)
    
    TIM5->SMCR &= ~(7 << 0); // SMS = 000 (슬레이브 모드 초기화)
    TIM5->SMCR |=  (7 << 0); // SMS = 111 (External Clock Mode 1)

    // 9. TIM5 카운터 활성화
    TIM5->CR1 |= (1 << 0); // CEN = 1 (Counter Enable)
}

void TIM4_Init(void)//TIM14 초기 설정
{
    // 1. TIM4 타이머 클럭 활성화 (APB1 버스)
    RCC->APB1ENR |= (1 << 2); // TIM4 EN

    // 2. TIM4 타이머 기본 설정 (100ms 주기)
    TIM4->PSC = 8400 - 1; // Prescaler  //    84,000,000 Hz / 8400 = 10,000 Hz (0.1ms)
    TIM4->ARR = 1000 - 1; // Auto-Reload Register //    10,000 Hz / 1000 = 10 Hz (100ms)

    // 3. TIM4 컨트롤 레지스터1 설정
    TIM4->CR1 &= ~(3 << 8); // CKD = 00 (Clock division)
    TIM4->CR1 &= ~(3 << 5); // CMS = 00 (Edge-aligned mode)
    TIM4->CR1 &= ~(1 << 4); // DIR = 0 (Up-counter)
    
    // 4. TIM4 인터럽트 활성화 (타이머 자체 설정)
    TIM4->DIER |= (1 << 0); // UIE = 1 (Update Interrupt Enable)

    // 5. TIM4 카운터 활성화
    TIM4->CR1 |= (1 << 0); // CEN = 1 (Counter Enable)
    
   // 6. NVIC (인터럽트 컨트롤러) 설정
    NVIC->IP[TIM4_IRQn] = (2 << 4);    // 우선순위 2로 설정 (0이 가장 높음), STM32F4는 상위 4비트만 사용 (2 << 4) = 0x20
    //NVIC_SetPriority(TIM4_IRQn, 2);
    NVIC->ISER[0] |= (1 << 30);     // TIM4 전역 인터럽트 활성화
    

}

void TIM1_Init(void)//TIM1 초기 설정 (스텝 모터 펄스 발생용)   펄스 발생 및 인터럽트 발생 설정
{
    // 1. PE13 핀을 Alternate Function(AF) 모드로 설정 (TIM1_CH3)
    GPIOE->MODER &= ~(3 << (13 * 2)); // PE13 MODER  초기화
    GPIOE->MODER |=  (2 << (13 * 2)); // PE13 MODER = 10 (AF Mode)

    // 2. PE13 핀을 Alternate Function(AF) 모드로 설정 (TIM1_CH3)
    GPIOE->MODER &= ~(3 << (13 * 2)); // PE13 MODER (비트 27, 26) 초기화
    GPIOE->MODER |=  (2 << (13 * 2)); // PE13 MODER (비트 27, 26) = 10 (AF Mode)

    // 3. PE13 핀을 AF1 (TIM1_CH3) 기능으로 매핑
    GPIOE->AFR[1] &= ~(15 << ((13 - 8) * 4)); // AFRH13 (비트 23:20) 초기화
    GPIOE->AFR[1] |=  (1 << ((13 - 8) * 4)); // AFRH13 (비트 23:20) = 0001 (AF1)

    // 4. TIM1 타이머 클럭 활성화 (APB2 버스)
    RCC->APB2ENR |= (1 << 0); // TIM1 EN

    // 5. TIM1 타이머 기본 설정 (1초 주기)
    TIM1->PSC = 8400 - 1; 
    TIM1->ARR = 20000 - 1; 

    // 6. TIM1 채널 3을 Output Compare Toggle 모드로 설정
    TIM1->CCMR2 &= ~(7 << 0); // OC3M = 000 (초기화)
    TIM1->CCMR2 |=  (3 << 0); // OC3M = 011 (Toggle on match)

    // 7. 펄스 시작 시점 설정
    TIM1->CCR3 = 1; 

    // 8. TIM1 인터럽트 활성화 (타이머 자체 설정)
    TIM1->DIER |= (1 << 3); // CC3IE = 1 

    // 9. (중요) TIM1은 Advanced 타이머이므로 Main Output Enable 필요
    TIM1->BDTR |= (1 << 15); // MOE = 1

    // 10. PDF 힌트: 초기에는 타이머와 채널을 비활성화
    TIM1->CCER &= ~(1 << (4 * (3 - 1))); // CC3E = 0 (Disable)
    TIM1->CR1  &= ~(1 << 0);             // CEN = 0 (Disable)

    // 11. NVIC 설정 (우선순위 1)
    NVIC->IP[TIM1_CC_IRQn] = (1 << 4); // 우선순위 1
    NVIC->ISER[0] |= (1 << 27);        // I전역 인터럽트 활성화
}


void TIM4_IRQHandler(void)
{
    // 0. TIM4의 Update Interrupt Flag (UIF)가 발생했는지 확인
    if ( (TIM4->SR & (1 << 0)) != 0 )
    {
        // === 1. 스텝 모터 (Position) 처리 ===
        static uint16_t last_pos = 0;     // 이전 위치 값을 기억하기 위한 static 변수
        uint16_t current_pos = TIM5->CNT; // 현재 위치 명령값(0~3) 읽기
        char pos_str[2];                  // LCD 표시용 문자열 버퍼

        // 1-1. 위치 명령값이 이전 값과 다른지 확인
        if (current_pos != last_pos)
        {
            // 1-2. LCD에 현재 위치값(Position) 표시
            pos_str[0] = current_pos + '0'; // 숫자(0~3)를 문자('0'~'3')로 변환
            pos_str[1] = '\0';              // 문자열 종료
            
            LCD_SetBackColor(RGB_YELLOW);   // 글자 배경색
            LCD_SetTextColor(RGB_RED);      // 글자 색
            LCD_DisplayText(3, 9, pos_str); // "Position:" 옆에 숫자 표시
            
            // 1-3. 스텝 모터 구동을 위한 전역 변수 설정
            pulse_goal = current_pos * 2; // 목표 펄스 수 = 위치값 * 2
            pulse_count = 0;              // 현재 펄스 카운트 초기화
            TIM1->CNT = 0;                // TIM1 카운터 초기화
            
            // 1-4. 목표 펄스 수가 0보다 클 때만 TIM1 타이머 시작 
            if (pulse_goal > 0) 
            {
                TIM1->CCER |= (1 << (4 * (3 - 1))); // CC3E Enable (채널 활성화)
                TIM1->CR1  |= (1 << 0);             // CEN = 1 (카운터 시작)
            }

            // 1-5. 현재 위치값을 '이전 값'으로 저장
            last_pos = current_pos;
        }
        
        // 2. DC 모터 (Torque) 처리 
        static uint16_t last_torque = 0;          // 이전 토크 값을 기억하기 위한 static 변수
        uint16_t current_torque = torque_command; // ADC가 갱신한 전역 변수 읽기
        char torque_str[2];
        
        // 2-1. 토크 명령값이 이전 값과 다른지 확인
        if (current_torque != last_torque)
        {
            // 2-2. LCD에 현재 토크값(Torque) 표시
            torque_str[0] = current_torque + '0';
            torque_str[1] = '\0';
            LCD_SetBackColor(RGB_YELLOW);   
            LCD_SetTextColor(RGB_RED); 
            LCD_DisplayText(5, 7, torque_str); // "Torque:" 옆에 숫자 표시
            
            // 2-3. 현재 토크값을 '이전 값'으로 저장
            last_torque = current_torque;
        }
        ADC3->CR2 |= (1 << 30); // SWSTART = 1 //(다음 100ms 주기를 위해) ADC 변환 1회 시작

        // 3. TIM4 Update Interrupt Flag(UIF) 클리어
        //    (이 플래그를 클리어하지 않으면 인터럽트 핸들러가 무한 반복됨)
        TIM4->SR &= ~(1 << 0);
    }
}

//TIM1 Capture/Compare 인터럽트 핸들러   
//펄스가 발생할 때마다(정확히는 Compare Match마다) 실행됨
// 목표한 펄스 수에 도달하면 타이머를 스스로 중지시킴
void TIM1_CC_IRQHandler(void)
{
    // TIM1 Capture/Compare 3 Interrupt Flag (CC3IF)가 떴는지 확인
    if ( (TIM1->SR & (1 << 3)) != 0 )
    {
        pulse_count++; // 펄스 카운트 증가

        // 목표한 펄스 수(pulse_goal)에 도달했는지 확인 
        if (pulse_count >= pulse_goal)
        {
            // PDF 힌트 [81, 82]: TIM1 채널과 카운터 비활성화
            TIM1->CCER &= ~(1 << (4 * (3 - 1))); // CC3E Disable 
            TIM1->CR1  &= ~(1 << 0);             // CEN = 0 (Disable) 
            pulse_count = 0; // 카운터 초기화
        }

        // (중요) TIM1 CC3IF 플래그 클리어
        TIM1->SR &= ~(1 << 3);
    }
}

void ADC3_Init(void)  //ADC3 초기 설정 (가변저항 PA1 입력용)
{
    // 1. GPIOA 클럭 활성화 (PA1 핀이 속한 포트)
    RCC->AHB1ENR |= (1 << 0); // GPIOA EN

    // 2. PA1 핀을 아날로그 모드로 설정
    GPIOA->MODER |= (3 << (1 * 2)); // PA1 MODER (비트 3, 2) = 11 (Analog)

    // 3. ADC3 타이머 클럭 활성화 (APB2 버스)
    RCC->APB2ENR |= (1 << 10); // ADC3 EN

    // 4. ADC3 컨트롤 레지스터1 설정 (10비트, EOC 인터럽트)
    ADC3->CR1 &= ~(3 << 24); // RES = 00 (초기화)
    ADC3->CR1 |=  (1 << 24); // RES = 10 (10비트 해상도)
    ADC3->CR1 |=  (1 << 5);  // EOCIE = 1 

    // 5. ADC3 컨트롤 레지스터2 설정 (연속 변환)
  //  ADC3->CR2 |=  (1 << 1);  // CONT = 1 (연속 변환)
    ADC3->CR2 &= ~(1 << 11); // ALIGN = 0 
    ADC3->CR2 |=  (1 << 0);  // ADON = 1 

    // 6. ADC3 채널 1을 첫 번째 변환 순서로 설정
    ADC3->SQR1 &= ~(0xF << 20); // L = 0000 (1개 채널)
    ADC3->SQR3 &= ~(0x1F << 0); // SQ1 = 00000 (초기화)
    ADC3->SQR3 |=  (1 << 0);    // SQ1 = 00001 (ADC_IN1)

    // 7. ADC 안정화 시간 대기
    DelayMS(10); 

    // 8. NVIC 설정 (우선순위 1)
    NVIC->IP[ADC_IRQn] = (1 << 4); // 우선순위 1
    NVIC->ISER[0] |= (1 << 18);    // IRQ 18 활성화

    // 9. ADC 변환 시작 (연속 변환 모드)
    ADC3->CR2 |= (1 << 30); // SWSTART = 1 
}

void ADC_IRQHandler(void)
{
    // ADC3의 End of Conversion (EOC) 플래그가 떴는지 확인
    if ( (ADC3->SR & (1 << 1)) != 0 )
    {
        uint16_t adc_val;
        uint16_t new_ccr;

        // 1. ADC 변환 값 읽기 (10비트: 0 ~ 1023)
        //    플래그는 DR 레지스터를 읽으면 자동으로 클리어됨
        adc_val = ADC3->DR; 

        // 2. 전압 값(0~3.3V) 기준으로 0~3 명령값 변환
        //    10비트(1023) 기준 전압 임계값 계산:
        //    0.80V -> (0.80 / 3.3) * 1023 = 248
        //    1.60V -> (1.60 / 3.3) * 1023 = 496
        //    2.40V -> (2.40 / 3.3) * 1023 = 744
        if (adc_val < 248) 
          {
              torque_command = 0;
          } 
        else if (adc_val < 496) 
          {
              torque_command = 1;
          } 
        else if (adc_val < 744)         
          {
              torque_command = 2;
          } 
        else
          {
              torque_command = 3;
          }
        
        // 3. 'torque_command'에 따라 TIM14의 Duty Ratio (CCR1) 갱신
        //    ARR = 80 (0~79)
        switch (torque_command)
        {
            case 0: // 0% Duty Ratio
                new_ccr = 0;
                break;
            case 1: // 30% Duty Ratio
                new_ccr = 24; // 80 * 0.3 = 24
                break;
            case 2: // 60% Duty Ratio
                new_ccr = 48; // 80 * 0.6 = 48
                break;
            case 3: // 90% Duty Ratio
                new_ccr = 72; // 80 * 0.9 = 72
                break;
            default:
                new_ccr = 0;
                break;
        }
        
        TIM14->CCR1 = new_ccr; // PWM Duty 값 즉시 갱신
    }
}
void TIM14_Init(void) //TIM14 초기 설정 (DC 모터 PWM 발생용)
{
    // 1. PF9 핀을 Alternate Function(AF) 모드로 설정 (GPIO F 클럭은 _GPIO_Init에서 활성화)
    GPIOF->MODER &= ~(3 << (9 * 2)); // PF9 MODER (비트 19, 18) 초기화
    GPIOF->MODER |=  (2 << (9 * 2)); // PF9 MODER (비트 19, 18) = 10 (AF Mode) 

    // 2. PF9 핀을 AF9 (TIM14_CH1) 기능으로 매핑 [cite: 106]
    //    PF9는 AFR[1] 레지스터의 (9-8)*4 = 4번 비트부터 사용
    GPIOF->AFR[1] &= ~(0xF << ((9 - 8) * 4)); // AFRH9 (비트 7:4) 초기화
    GPIOF->AFR[1] |=  (9 << ((9 - 8) * 4)); // AFRH9 (비트 7:4) = 1001 (AF9)

    // 3. TIM14 타이머 클럭 활성화 (APB1 버스)
    RCC->APB1ENR |= (1 << 8); // TIM14 EN

    // 4. TIM14 타이머 기본 설정 
    TIM14->PSC = 420 - 1; // Prescaler  84M / 420 = 200kHz (5us) 
    TIM14->ARR = 80 - 1;  // Auto-Reload Register  80 * 5us = 400us 

    // 5. TIM14 채널 1을 PWM 2 Mode로 설정
    TIM14->CCMR1 &= ~(7 << 4); // OC1M (비트 6:4) = 000 (초기화)
    TIM14->CCMR1 |=  (6 << 4); // OC1M (비트 6:4) = 110 (PWM 1 Mode) 
    TIM14->CCMR1 |=  (1 << 3); // OC1PE = 1 (Output Compare 1 Preload Enable)

    // 6. 초기 Duty Ratio 0% 설정 (CCR1 = 0)
    //    (PWM 2 Mode는 CNT < CCR1 일 때 Active)
    TIM14->CCR1 = 0; 

    // 7. TIM14 채널 1 출력 활성화
    TIM14->CCER |= (1 << 0); // CC1E = 1 (Channel 1 Output Enable)

    // 8. TIM14 카운터 활성화
    TIM14->CR1 |= (1 << 7); // ARPE = 1 (Auto-Reload Preload Enable)
    TIM14->CR1 |= (1 << 0); // CEN = 1 (Counter Enable)
}


//--------------------------------------------------------

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
