/////////////////////////////////////////////////////////////
// PR: Car Tracking system
// 제출자: 2020130030 이수혁
// 주요 내용
// - 거리센서(ADC): ADC3_IN1(PA1, 선도차), ADC3_IN9(PF3, 인도) Scan/DMA 방식 (TIM1_CH3 트리거 400ms)
// - 추종차 속도제어(엔진): TIM4_CH2(PB7) PWM 제어 (선도차 거리에 따른 Duty비 10~90% 가변)
// - 추종차 방향제어(핸들): TIM14_CH1(PF9) PWM 제어 (인도 거리에 따른 Duty비 가변, Buzzer 출력)
// - 시동 제어(Switch): SW4(EXTI12, Move), SW6(EXTI14, Stop) 외부 인터럽트 이용
// - 원격 제어 및 통신(PC): USART1 이용 (RX: 'M'/'S' 명령 수신, TX: 거리값 송신)
// - 상태 저장(FRAM): 1201번지에 주행 상태(M/S) 저장 및 리셋 후 상태 복구
// - 화면 표시(GLCD): 거리(D1, D2), 속도(SP), 방향(DIR) 및 막대그래프 표시
/////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"
#include <stdio.h>


void _GPIO_Init(void);             // GPIO 초기화
void DisplayInitScreen(void);           //초기 화면 표시
void _LCD_Update(int d1, int d2);  // 화면 갱신 (숫자, 막대)
void BEEP(void);                   // 부저 동작
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void _EXTI_Init(void);    // 외부 인터럽트 초기화 (SW4, SW6)
void _ADC_Init(void);  // ADC, DMA, Timer 설정을 한 번에 하는 함수
void _DMA_Init(void);              // DMA 초기화 (ADC 데이터 전송)
void _TIM1_Init(void);             // 타이머 초기화 (ADC 트리거용 400ms)
void _TIM4_Init(void);         //  엔진(속도) 제어용 타이머 초기화 (TIM4_CH2, PB7)
void _TIM14_Init(void);       //핸들(방향) 제어용 타이머 초기화 (TIM14_CH1, PF9)
void Control_Car(int , int ); //  자동차 제어 로직
void _USART1_Init(void);  //  USART1 초기화: PC 통신용
void USART1_SendChar(char c);  //  문자 1개 송신
void USART1_SendString(char *str);  //문자열 송신 함수
void USART1_IRQHandler(void); // USART1 인터럽트 핸들러
void Load_State_From_FRAM(void); // 부팅 시 FRAM에서 상태 복구



// ADC 변환 값을 저장할 배열 (DMA가 여기에 자동으로 값을 채움)
// ADC_Data[0]: 거리센서1 (PA1), ADC_Data[1]: 거리센서2 (PF3)
volatile uint16_t ADC_Data[2] = {0, };

volatile int System_State = 0; // 시스템 상태 변수 (0: STOP, 1: MOVE)


// =============================================================
int main(void)
{

    LCD_Init();  //lCD 초기화
    Fram_Init();                    // FRAM 초기화 H/W 초기화
    Fram_Status_Config();   // FRAM 초기화 S/W 초기화
    _GPIO_Init(); 	   
    _EXTI_Init();               
    DisplayInitScreen();       // 초기 UI 그리기 
    DelayMS(10);    

     Load_State_From_FRAM(); // FRAM에서 상태 읽어와서 덮어쓰기 (이전 상태 복구)
    _ADC_Init(); 
    _TIM4_Init();   // 엔진(속도)
    _TIM14_Init();  // 핸들(방향)
    _USART1_Init(); // 통신 초기화

    // 초기 LED 상태 (STOP 상태: LED6 ON)
    GPIOG->ODR &= ~(1 << 4); // LED4 OFF
    GPIOG->ODR |=  (1 << 6); // LED6 ON

    // 거리 계산을 위한 변수 선언
    float vol1, vol2;
    int dist1, dist2;

   char uart_buf[30];   // 송신용 버퍼

	while (1)
	{

           if (System_State == 1) // 1. MOVE 상태 (가변저항 값 반영)
        {
            // D1 거리 계산: 전압*5 + 3
            vol1 = (float)ADC_Data[0] * 3.3f / 1023.0f;
            dist1 = (int)(vol1 * 5) + 3;
            // 범위 제한 (3~19m)
            if(dist1 < 3) dist1 = 3;
            if(dist1 > 19) dist1 = 19;

            // D2 거리 계산: 전압 (소수점 버림)
            vol2 = (float)ADC_Data[1] * 3.3f / 1023.0f;
            dist2 = (int)vol2;
            // 범위 제한 (0~3m)
            if(dist2 < 0) dist2 = 0;
            if(dist2 > 3) dist2 = 3;


           //  PC로 거리값 전송 (MOVE 상태일 때만)
            // 포맷: "12m " (숫자 + m + 공백)
            sprintf(uart_buf, "%dm ", dist1);
            USART1_SendString(uart_buf);
        }
        else // 2. STOP 상태 (값 고정)
        {
            // STOP 모드에서는 센서 값 무시하고 초기값 유지
            dist1 = 0; // D1: 0m
            dist2 = 1; // D2: 1m
        }
          
          _LCD_Update(dist1, dist2);  //  화면 갱신 함수 호출
          Control_Car(dist1, dist2);   //자동차 제어 (PWM 출력 및 하단 정보 표시)

          DelayMS(200); // 0.2초 대기

        }  

    
}

// =============================================================
/* GPIO (GPIOG(LED), GPIOH(Switch), 초기 설정*/
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
	RCC->AHB1ENR |= 0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER |= 0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER &= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR |= 0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 

	// SW (GPIO H) 설정 
	RCC->AHB1ENR |= 0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR &= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

}

// =============================================================
// 초기 화면 표시: LCD에 초기 화면 정보 표시
void DisplayInitScreen(void)
{
// 1. 검정색 텍스트 (고정 라벨)
    LCD_Clear(RGB_WHITE);
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "LSH 2020130030");
    LCD_DisplayText(1, 0, "Tracking Car");
    LCD_DisplayText(2, 0, "D1:");
    LCD_DisplayText(3, 0, "D2:");
    LCD_SetFont(&Gulim7);
    LCD_DisplayText(5, 0, "SP(DR):  %  DIR(DR):");
    LCD_SetFont(&Gulim8);
// 2. 파란색 텍스트 (초기 STOP 값)
    LCD_SetTextColor(RGB_BLUE); // 글자색 파랑으로 변경
    LCD_DisplayText(1, 18, "S"); // 상태: STOP
    LCD_DisplayText(2, 17, "0");  // D1: 0m
    LCD_DisplayText(3, 17, "1");  // D2: 1m
    LCD_SetFont(&Gulim7);      
    LCD_DisplayText(5, 7,  "00"); // SP 값
    LCD_DisplayText(5, 20, "F");   // DIR 값 (Forward)
    LCD_SetFont(&Gulim8); 

// 3. 초기 막대 그래프 (D2: 1m 초록색)
    // 위치 설정
    int bar_x = 35;            // X 시작점
    int bar_y_d2 = (3*12)+4; // 3번째 줄(Index 3) Y좌표
    int bar_height = 6;        // 막대 높이
    int bar_len_init = 33;     // 1m 길이 (33%)

    //  초록색 막대 채우기
    LCD_SetBrushColor(RGB_GREEN);
    LCD_DrawFillRect(bar_x, bar_y_d2, bar_len_init, bar_height);
    
    // 색상 복구 (Text Color는 검정으로)
    LCD_SetTextColor(RGB_BLACK);
}
// =============================================================

void _EXTI_Init(void)
{
    // 1. SYSCFG 클럭 활성화
    RCC->APB2ENR |= (1 << 14);   

    // 2. EXTI 라인 연결 (SW4:PH12, SW6:PH14 -> EXTI12, EXTI14)
    // EXTI 12, 13, 14, 15는 EXTICR[3]에서 관리
    // SW4(EXTI12): [3:0]비트, SW6(EXTI14): [11:8]비트
    // Port H는 값 7 (0111)에 해당
    
    SYSCFG->EXTICR[3] &= ~0x0F0F;   // EXTI12, EXTI14 비트 클리어 (0으로 초기화)
    SYSCFG->EXTICR[3] |=  0x0707;   // EXTI12, EXTI14에 Port H(7) 연결 (0x0707)

    // 3. 인터럽트 마스크 해제 (IMR)
    // EXTI12(Bit 12) | EXTI14(Bit 14) 
    // 0x1000 | 0x4000 = 0x5000
    EXTI->IMR |= 0x5000;            // EXTI 12, 14 인터럽트 허용

    // 4. 하강 에지 트리거 설정 (FTSR)
    // 버튼을 누를 때 동작하므로 Falling Edge 사용
    EXTI->FTSR |= 0x5000;           // EXTI 12, 14 하강 에지 검출

    // 5. NVIC 우선순위 및 활성화 (레지스터 직접 제어)
    // EXTI15_10_IRQn 벡터 번호는 40번
    // ISER[0]: 0~31, ISER[1]: 32~63
    // 40번은 ISER[1]의 (40-32) = 8번째 비트
    
    NVIC->ISER[1] |= (1 << 8);      // EXTI15_10 인터럽트 활성화 (Enable)
}
// =============================================================

void EXTI15_10_IRQHandler(void)
{
    // 1. SW4 (MOVE) 눌림 확인 (Bit 12)
    if (EXTI->PR & (1 << 12)) 
    {

        System_State = 1;   // 상태 변수 변경: MOVE
        // 화면 갱신: (1, 18) 위치에 파란색 'M'
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(1, 18 ,"M");

        // LED 제어: LED4 ON, LED6 OFF
        GPIOG->ODR |=  (1 << 4);
        GPIOG->ODR &= ~(1 << 6);

        Fram_Write(0x1201, 'M');  //  상태 저장

        // 플래그 클리어
        EXTI->PR |= (1 << 12); 
    }

    // 2. SW6 (STOP) 눌림 확인 (Bit 14)
    if (EXTI->PR & (1 << 14)) 
    {

        System_State = 0; // [추가] 상태 변수 변경: STOP
        // 화면 갱신: (1, 18) 위치에 파란색 'S'
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(1, 18, "S");

        // LED 제어: LED4 OFF, LED6 ON
        GPIOG->ODR &= ~(1 << 4);
        GPIOG->ODR |=  (1 << 6);

         Fram_Write(0x1201, 'S');  //  상태 저장

        // 플래그 클리어
        EXTI->PR |= (1 << 14);
    }
}

// =============================================================
// DMA 초기화 함수: ADC3의 데이터를 메모리(ADC_Data)로 자동 전송
// DMA2, Stream0, Channel2 사용 (Datasheet 기준 ADC3 연결)
void _DMA_Init(void)
{
    // 1. DMA2 클럭 활성화
    RCC->AHB1ENR |= (1 << 22);      // DMA2 Clock Enable (Bit 22)

    // 2. DMA 스트림 비활성화 (설정 변경 전 필수)
    DMA2_Stream0->CR &= ~(1 << 0);  // EN=0: Stream Disable
    while(DMA2_Stream0->CR & (1 << 0)); // 꺼질 때까지 대기

    // 3. 채널 선택 (Channel 2가 ADC3에 연결됨)
    DMA2_Stream0->CR &= ~(7 << 25); // CHSEL 클리어
    DMA2_Stream0->CR |=  (2 << 25); // CHSEL=2: Channel 2 선택

    // 4. 주소 설정
    DMA2_Stream0->PAR  = (uint32_t)&ADC3->DR; // Peripheral Address: ADC3 데이터 레지스터
    DMA2_Stream0->M0AR = (uint32_t)ADC_Data;  // Memory Address: 데이터를 저장할 배열 주소

    // 5. 데이터 전송 방향 및 모드 설정
    DMA2_Stream0->CR &= ~(3 << 6);  // DIR 클리어
    DMA2_Stream0->CR &= ~(1 << 6);  // DIR=00: Peripheral-to-Memory (P->M)

    DMA2_Stream0->NDTR = 2;         // 전송할 데이터 개수 (거리센서 2개)

    // 6. 주소 증가 모드
    DMA2_Stream0->CR &= ~(1 << 9);  // PINC=0: Peripheral(ADC) 주소 고정
    DMA2_Stream0->CR |=  (1 << 10); // MINC=1: Memory(배열) 주소 자동 증가

    // 7. 데이터 크기 (Half-word: 16bit)
    DMA2_Stream0->CR &= ~(3 << 11); // PSIZE 클리어
    DMA2_Stream0->CR |=  (1 << 11); // PSIZE=01: 16-bit (Half-word)
    DMA2_Stream0->CR &= ~(3 << 13); // MSIZE 클리어
    DMA2_Stream0->CR |=  (1 << 13); // MSIZE=01: 16-bit (Half-word)

    // 8. 순환 모드 (Circular Mode)
    DMA2_Stream0->CR |=  (1 << 8);  // CIRC=1: Circular mode enable (계속 측정하므로)

    // 9. 우선순위 및 기타
    DMA2_Stream0->CR |=  (2 << 16); // PL=10: Priority Level High
    DMA2_Stream0->FCR &= ~(1 << 2); // DMDIS=0: Direct mode enable (FIFO 사용 안 함)

    // 10. DMA 스트림 활성화
    DMA2_Stream0->CR |=  (1 << 0);  // EN=1: Stream Enable
}
// =============================================================
// 타이머 초기화 함수: ADC 트리거용 400ms 주기 신호 생성
// TIM1_CH3 (PE13) 사용
void _TIM1_Init(void)
{
    // 1. GPIOE (PE13) 설정 - 확인용 (TIM1_CH3 출력)
    RCC->AHB1ENR   |= (1 << 4);         // GPIOE Clock Enable
    
    // PE13 Alternate Function 설정
    GPIOE->MODER   &= ~(3 << (13 * 2)); // 초기화
    GPIOE->MODER   |=  (2 << (13 * 2)); // Mode=10: Alternate Function
    GPIOE->OSPEEDR |=  (3 << (13 * 2)); // Speed=11: High Speed
    GPIOE->AFR[1]  |=  (1 << (4 * (13 - 8))); // AF1 선택 (TIM1)

    // 2. TIM1 클럭 활성화
    RCC->APB2ENR   |= (1 << 0);         // TIM1 Clock Enable

    // 3. 주기 및 분주비 설정 (목표: 400ms)
    // SystemCoreClock 168MHz, APB2 84MHz -> TIM1 Kernel Clock 168MHz
    // PSC: 8400 -> 168MHz / 8400 = 20kHz (T = 0.05ms)
    TIM1->PSC = 8400 - 1;               // Prescaler 설정 (PDF 요구사항)

    // ARR: 400ms / 0.05ms = 8000
    TIM1->ARR = 8000 - 1;               // Auto Reload Register (주기 400ms)

    // 4. PWM 모드 설정 (CH3) - 트리거 신호 생성용
    TIM1->CCMR2 &= ~(3 << 0);           // CC3S=00: Output Mode
    TIM1->CCMR2 |=  (6 << 4);           // OC3M=110: PWM Mode 1 (Active -> Inactive)
    TIM1->CCMR2 |=  (1 << 3);           // OC3PE=1: Preload Enable

    TIM1->CCR3   = 4000;                // Pulse Width (50% Duty, 트리거 시점은 상관없음)

    // 5. 출력 극성 및 활성화
    TIM1->CCER  &= ~(1 << 9);           // CC3P=0: Active High
    TIM1->CCER  |=  (1 << 8);           // CC3E=1: Output Enable

    TIM1->BDTR  |=  (1 << 15);          // MOE=1: Main Output Enable (고급 타이머 필수)

    // 6. 타이머 카운터 활성화
    TIM1->CR1   |=  (1 << 0);           // CEN=1: Counter Enable
}

// =============================================================
// ADC3 초기화 함수: GPIO, ADC, DMA, Timer 설정을 총괄
// PA1 PF3사용 / 10bit / Timer Trigger
void _ADC_Init(void)
{
    // 1. 선행 모듈 초기화 (DMA 및 타이머)
    _DMA_Init();
    _TIM1_Init();

    // 2. GPIO 초기화 (PA1, PF3 -> Analog Mode)
    RCC->AHB1ENR |= (1 << 0);           // GPIOA Clock Enable
    RCC->AHB1ENR |= (1 << 5);           // GPIOF Clock Enable

    // PA1 (ADC3_IN1)
    GPIOA->MODER |= (3 << (1 * 2));     // PA1 Mode=11: Analog Mode
    // PF3 (ADC3_IN9)
    GPIOF->MODER |= (3 << (3 * 2));     // PF3 Mode=11: Analog Mode

    // 3. ADC 공통 설정 (Common Init)
    RCC->APB2ENR |= (1 << 10);          // ADC3 Clock Enable

    // ADC Prescaler: PCLK2 / 4 (84MHz / 4 = 21MHz)
    ADC->CCR &= ~(3 << 16);             // ADCPRE 클리어
    ADC->CCR |=  (1 << 16);             // ADCPRE=01: Div 4

    // 4. ADC3 세부 설정 (CR1)
    // 해상도 10-bit (PDF 요구사항)
    ADC3->CR1 &= ~(3 << 24);            // RES 클리어
    ADC3->CR1 |=  (1 << 24);            // RES=01: 10-bit Resolution
    ADC3->CR1 |=  (1 << 8);             // SCAN=1: Scan Mode Enable

    // 5. ADC3 트리거 및 DMA 설정 (CR2)
    // 외부 트리거 활성화 (Timer1 CC3)
    ADC3->CR2 &= ~(3 << 28);            // EXTEN 클리어
    ADC3->CR2 |=  (3 << 28);            // EXTEN=11: Trigger detection on both edges (PDF 요구사항)

    ADC3->CR2 &= ~(0xF << 24);          // EXTSEL 클리어
    ADC3->CR2 |=  (2 << 24);            // EXTSEL=0010: Timer 1 CC3 event

    // DMA 사용 설정
    ADC3->CR2 |=  (1 << 8);             // DMA=1: DMA Mode Enable
    ADC3->CR2 |=  (1 << 9);             // DDS=1: DMA Disable Selection (Circular 모드에서 필수)
    ADC3->CR2 &= ~(1 << 11);            // ALIGN=0: Right Alignment

    // 6. 채널 순서 및 샘플링 타임 설정
    // 시퀀스 길이: 2개
    ADC3->SQR1 &= ~(0xF << 20);         // L 클리어
    ADC3->SQR1 |=  (1 << 20);           // L=0001: 2 conversions (N-1 = 1)

    // 순서 1: IN1 (PA1)
    ADC3->SQR3 &= ~(0x1F << 0);         // Rank 1 클리어
    ADC3->SQR3 |=  (1 << 0);            // Rank 1 = Channel 1

    // 순서 2: IN9 (PF3)
    ADC3->SQR3 &= ~(0x1F << 5);         // Rank 2 클리어
    ADC3->SQR3 |=  (9 << 5);            // Rank 2 = Channel 9

    // 샘플링 타임: 480 Cycles (안정적인 측정을 위해 최대값)
    ADC3->SMPR2 |= (7 << (3 * 1));      // Channel 1: 480 cycles
    ADC3->SMPR2 |= (7 << (3 * 9));      // Channel 9: 480 cycles

    // 7. ADC 켜기
    ADC3->CR2 |= (1 << 0);              // ADON=1: Enable ADC
}



// =============================================================

void _LCD_Update(int d1, int d2)
{
    char buf[10]; // 문자열 변환 버퍼

    // 막대그래프 설정
    int bar_max_width = 80; 
    int bar_height = 6;      
    int bar_x = 35;          

    // [D1 처리] 3번째 줄 (Index 2)
    int d1_y = (2 * 12) + 2; 

    // (1) 텍스트 출력
    LCD_SetBackColor(RGB_WHITE);
    LCD_SetTextColor(RGB_BLUE);
    sprintf(buf, "%2d", d1);       
    LCD_DisplayText(2, 17, buf);   

    // (2) 막대 계산 (3m~19m -> 0~100%)
    float ratio_d1 = (float)(d1 - 3) / 16.0f; 
    if(ratio_d1 < 0.0f) ratio_d1 = 0.0f;
    if(ratio_d1 > 1.0f) ratio_d1 = 1.0f;
    
    int bar_len1 = (int)(ratio_d1 * bar_max_width);

    // (3) 막대 그리기
    LCD_SetBrushColor(RGB_WHITE); // 배경 지우기
    LCD_DrawFillRect(bar_x, d1_y, bar_max_width, bar_height);

    if(bar_len1 > 0) {
        LCD_SetBrushColor(RGB_RED); // 실제 값 채우기
        LCD_DrawFillRect(bar_x, d1_y, bar_len1, bar_height);
    }

    // [D2 처리] 4번째 줄 (Index 3)
    int d2_y = (3 * 12) + 4; 

    // (1) 텍스트 출력
    LCD_SetBackColor(RGB_WHITE); // 배경색 재설정
    LCD_SetTextColor(RGB_BLUE);
    sprintf(buf, "%2d", d2);
    LCD_DisplayText(3, 17, buf);

    // (2) 막대 계산 (0m~3m -> 0~100%)
    float ratio_d2 = (float)d2 / 3.0f;
    if(ratio_d2 > 1.0f) ratio_d2 = 1.0f;

    int bar_len2 = (int)(ratio_d2 * bar_max_width);

    // (3) 막대 그리기
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(bar_x, d2_y, bar_max_width, bar_height);

    if(bar_len2 > 0) {
        LCD_SetBrushColor(RGB_GREEN);
        LCD_DrawFillRect(bar_x, d2_y, bar_len2, bar_height);
    }

    // 색상 복구
    LCD_SetTextColor(RGB_BLACK);
    LCD_SetBackColor(RGB_WHITE);
}
// =============================================================
//  엔진(속도) 제어용 타이머 초기화 (TIM4_CH2, PB7)
// 주파수: 0.2Hz (주기 5초), 분주비 8400 (PDF 요구사항)
void _TIM4_Init(void)
{
    // 1. GPIOB (PB7) 설정 - TIM4_CH2
    RCC->AHB1ENR   |= (1 << 1);          // GPIOB Clock Enable (Bit 1)
    
    // PB7 Alternate Function 설정
    GPIOB->MODER   &= ~(3 << (7 * 2));   // 초기화
    GPIOB->MODER   |=  (2 << (7 * 2));   // Mode=10: Alternate Function
    GPIOB->OSPEEDR |=  (3 << (7 * 2));   // Speed=11: High Speed
    GPIOB->AFR[0]  |=  (2 << (4 * 7));   // AF2 선택 (TIM4) - (AFR[0]은 PIN 0~7 담당)

    // 2. TIM4 클럭 활성화 (APB1)
    RCC->APB1ENR   |= (1 << 2);          // TIM4 Clock Enable (Bit 2)

    // 3. 주기 및 분주비 설정 (0.2Hz = 5초)
    // APB1 Timer Clock = 84MHz
    // PSC: 8400 -> 84MHz / 8400 = 10kHz (0.1ms)
    TIM4->PSC = 8400 - 1;               
    
    // ARR: 5초 / 0.1ms = 50000
    TIM4->ARR = 50000 - 1;              

    // 4. PWM 모드 설정 (CH2)
    TIM4->CCMR1 &= ~(3 << 8);           // CC2S=00: Output Mode
    TIM4->CCMR1 |=  (6 << 12);          // OC2M=110: PWM Mode 1
    TIM4->CCMR1 |=  (1 << 11);          // OC2PE=1: Preload Enable

    TIM4->CCR2   = 0;                   // 초기 Duty 0%

    // 5. 출력 활성화
    TIM4->CCER  &= ~(1 << 5);           // CC2P=0: Active High
    TIM4->CCER  |=  (1 << 4);           // CC2E=1: Output Enable

    // 6. 타이머 카운터 활성화
    TIM4->CR1   |=  (1 << 0);           // CEN=1: Counter Enable
}
// =============================================================
//  핸들(방향) 제어용 타이머 초기화 (TIM14_CH1, PF9)
// 주파수: 2.5kHz (주기 400us), 분주비 420 (PDF 요구사항)
void _TIM14_Init(void)
{
    // 1. GPIOF (PF9) 설정 - TIM14_CH1 (Buzzer 연결)
    RCC->AHB1ENR   |= (1 << 5);          // GPIOF Clock Enable (Bit 5)
    
    // PF9 Alternate Function 설정
    GPIOF->MODER   &= ~(3 << (9 * 2));   
    GPIOF->MODER   |=  (2 << (9 * 2));   // Mode=10: AF
    GPIOF->OSPEEDR |=  (3 << (9 * 2));   
    // PF9는 AFR[1] (PIN 8~15 담당)
    GPIOF->AFR[1]  |=  (9 << (4 * (9 - 8))); // AF9 선택 (TIM14)

    // 2. TIM14 클럭 활성화 (APB1)
    RCC->APB1ENR   |= (1 << 8);          // TIM14 Clock Enable (Bit 8)

    // 3. 주기 및 분주비 설정 (2.5kHz)
    // APB1 Timer Clock = 84MHz
    // PSC: 420 -> 84MHz / 420 = 200kHz (5us)
    TIM14->PSC = 420 - 1;               
    
    // ARR: 400us / 5us = 80
    TIM14->ARR = 80 - 1;              

    // 4. PWM 모드 설정 (CH1)
    TIM14->CCMR1 &= ~(3 << 0);           // CC1S=00: Output
    TIM14->CCMR1 |=  (6 << 4);           // OC1M=110: PWM Mode 1
    TIM14->CCMR1 |=  (1 << 3);           // OC1PE=1: Preload Enable

    TIM14->CCR1   = 0;                   // 초기 Duty 0%

    // 5. 출력 활성화
    TIM14->CCER  &= ~(1 << 1);           // CC1P=0: Active High
    TIM14->CCER  |=  (1 << 0);           // CC1E=1: Output Enable

    // 6. 타이머 카운터 활성화
    TIM14->CR1   |=  (1 << 0);           // CEN=1: Counter Enable
}



// =============================================================
//  자동차 제어 로직 (속도 및 방향 결정)
// main 루프에서 호출
void Control_Car(int d1, int d2)
{
    char buf[10];
    int speed_pwm_percent = 0;
    
    // 1. 속도 제어 (D1 거리 -> TIM4 PWM Duty)
    // 규칙: 3~4m(10%), 5~6m(20%), ... 17~18m(80%), 19m~(90%)
    // 공식 유도: ((거리 - 1) / 2) * 10
    if (d1 >= 19) speed_pwm_percent = 90;
    else if (d1 < 3) speed_pwm_percent = 10; // 3m 미만도 최소 10% 유지 (또는 0% 등 정책 필요 시 수정)
    else speed_pwm_percent = ((d1 - 1) / 2) * 10;
    
    // STOP 상태일 경우 강제 0% (main에서 호출 시 상태 체크해서 0 넘겨줘도 됨)
    if(System_State == 0) speed_pwm_percent = 0;

    // TIM4 CCR 값 적용 (ARR=50000 기준)
    TIM4->CCR2 = (50000 * speed_pwm_percent) / 100;

    // LCD에 SP 값 표시 (5번째 줄)
    LCD_SetFont(&Gulim7);
    sprintf(buf, "%2d", speed_pwm_percent);
    LCD_SetTextColor(RGB_BLUE);
    LCD_SetBackColor(RGB_WHITE);
    LCD_DisplayText(5, 7, buf);


    // 2. 방향 제어 (D2 거리 -> TIM14 PWM Duty & 화면 표시)
    // 규칙: 0m(우회전 30%), 1m(직진 0%), 2~3m(좌회전 90%)
    int dir_pwm_percent = 0;
    char *dir_char = "F"; // 기본 직진

    if(System_State == 0) // STOP 상태
    {
         dir_pwm_percent = 0; // 0%
         dir_char = "F";
    }
    else // MOVE 상태
    {
        switch(d2)
        {
            case 0: // 우회전
                dir_pwm_percent = 30;
                dir_char = "R";
                break;
            case 1: // 직진
                dir_pwm_percent = 0;
                dir_char = "F";
                break;
            default: // 2~3m 좌회전
                dir_pwm_percent = 90;
                dir_char = "L";
                break;
        }
    }

    // TIM14 CCR 값 적용 (ARR=80 기준)
    TIM14->CCR1 = (80 * dir_pwm_percent) / 100;

    // LCD에 DIR 값 표시 (5번째 줄 끝)
    LCD_DisplayText(5, 20, dir_char);
    LCD_SetFont(&Gulim8); // 폰트 원복
    LCD_SetTextColor(RGB_BLACK);
}
// =============================================================
//  USART1 초기화: PC 통신용 (19200bps, 8N1)
// PA9(TX), PA10(RX) 사용
void _USART1_Init(void)
{
    // 1. GPIOA, USART1 클럭 활성화
    RCC->AHB1ENR |= (1 << 0);       // GPIOA Clock Enable
    RCC->APB2ENR |= (1 << 4);       // USART1 Clock Enable (APB2)

    // 2. GPIO Alternate Function 설정 (PA9, PA10 -> AF7)
    // PA9 (TX)
    GPIOA->MODER   &= ~(3 << (9 * 2));   // Clear
    GPIOA->MODER   |=  (2 << (9 * 2));   // Mode=10: AF
    GPIOA->AFR[1]  |=  (7 << (4 * (9 - 8))); // AF7 (USART1) - AFRH(AFR[1])

    // PA10 (RX)
    GPIOA->MODER   &= ~(3 << (10 * 2));  // Clear
    GPIOA->MODER   |=  (2 << (10 * 2));  // Mode=10: AF
    GPIOA->AFR[1]  |=  (7 << (4 * (10 - 8))); // AF7 (USART1)

    // 3. Baudrate 설정 (19200bps)
    // f_PCLK2(APB2) = 84MHz
    // USARTDIV = 84,000,000 / 19200 = 4375
    USART1->BRR = 4375;

    // 4. 제어 레지스터 설정 (CR1)
    USART1->CR1 = 0;             // 초기화
    USART1->CR1 |= (1 << 13);    // UE=1: USART Enable
    USART1->CR1 |= (1 << 3);     // TE=1: Transmitter Enable
    USART1->CR1 |= (1 << 2);     // RE=1: Receiver Enable
    USART1->CR1 |= (1 << 5);     // RXNEIE=1: RX Interrupt Enable

    // 5. NVIC 인터럽트 활성화
    // USART1_IRQn = 37 (Vector Table 참조)
    // 37 - 32 = 5 (ISER[1]의 5번째 비트)
    NVIC->ISER[1] |= (1 << 5);
}
// =============================================================
//  문자 1개 송신 (Polling 방식)
void USART1_SendChar(char c)
{
    // TXE(Transmit data register empty, Bit 7)가 1이 될 때까지 대기
    while (!(USART1->SR & (1 << 7))); 
    USART1->DR = c; // 데이터 전송
}
// =============================================================
//  문자열 송신 함수
void USART1_SendString(char *str)
{
    while (*str)
    {
        USART1_SendChar(*str++);
    }
}
// =============================================================
//  USART1 인터럽트 핸들러 (수신 처리)
void USART1_IRQHandler(void)
{
    // RXNE(Read data register not empty, Bit 5) 플래그 확인
    if (USART1->SR & (1 << 5))
    {
        char rx_data = USART1->DR; // 데이터 읽기 (플래그 자동 클리어)

        // 'M' 또는 'm' 수신 시 MOVE 모드 전환
        if (rx_data == 'M' || rx_data == 'm')
        {
            System_State = 1; 

            // 화면 및 LED 업데이트 (EXTI 핸들러와 동일 로직)
            LCD_SetTextColor(RGB_BLUE);
            LCD_DisplayText(1, 18, "M");
            GPIOG->ODR |=  (1 << 4); // LED4 ON
            GPIOG->ODR &= ~(1 << 6); // LED6 OFF

            Fram_Write(0x1201, 'M');  //  상태 저장
        }
        // 'S' 또는 's' 수신 시 STOP 모드 전환
        else if (rx_data == 'S' || rx_data == 's')
        {
            System_State = 0;

            LCD_SetTextColor(RGB_BLUE);
            LCD_DisplayText(1, 18, "S");
            GPIOG->ODR &= ~(1 << 4); // LED4 OFF
            GPIOG->ODR |=  (1 << 6); // LED6 ON

            Fram_Write(0x1201, 'S');   //  상태 저장
        }
    }
}
// =============================================================
//  FRAM에서 상태를 읽어와 시스템 초기화
void Load_State_From_FRAM(void)
{
    // 1201번지에서 값 읽기
    unsigned char state = Fram_Read(0x1201); 

    if (state == 'M') 
    {
        // 1. MOVE 상태 복구
        System_State = 1;
        
        // 화면 표시
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(1, 18, "M");
        
        // LED 제어 (LED4 ON, LED6 OFF)
        GPIOG->ODR |=  (1 << 4);
        GPIOG->ODR &= ~(1 << 6);
    }
    else 
    {
        // 2. STOP 상태 복구 (값이 'S'이거나, 초기 쓰레기값일 경우)
        System_State = 0;
        
        // 만약 'S'가 아닌 이상한 값이면 'S'로 초기화하여 저장
        if (state != 'S') 
        {
            Fram_Write(0x1201, 'S');
        }

        // 화면 표시
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(1, 18, "S");
        
        // LED 제어 (LED4 OFF, LED6 ON)
        GPIOG->ODR &= ~(1 << 4);
        GPIOG->ODR |=  (1 << 6);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////

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

void BEEP(void)			/* beep for 30 ms */
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

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
	{  	if(joy_flag == 0)
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