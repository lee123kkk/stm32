/////////////////////////////////////////////////////////////
// PR: Car Tracking System (기말고사 대비 최적화 버전)
// 구조: ADC 인터럽트(400ms) 기반 제어 (우수 과제 구조 반영)
// 
// [필수 암기 초기화 순서]
// 1. Hardware Init (LCD, FRAM)
// 2. GPIO & EXTI (입출력, 버튼)
// 3. Comm (UART 통신)
// 4. Actuators (PWM 타이머들)
// 5. Sensors (DMA -> Trigger Timer -> ADC)
/////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"   // Fram_Init, Fram_Write, Fram_Read
#include <stdio.h>

// ================= [전역 변수] =================
// ADC 값 저장용 (DMA가 자동으로 갱신)
// [0]: 거리센서1, [1]: 거리센서2
volatile uint16_t ADC_Data[2] = {0, };

// 상태 변수 (0: STOP, 1: MOVE)
volatile int System_State = 0; 

// ================= [함수 선언] =================
// 기능별로 묶어서 외우기 쉽게 정리
void System_Init_Sequence(void); // 전체 초기화 총괄
void _GPIO_EXTI_Init(void);      // LED, 스위치
void _UART_Init(void);           // PC 통신
void _PWM_Timers_Init(void);     // 모터/부저 제어 (TIM4, TIM14)
void _ADC_System_Init(void);     // 센서 시스템 (DMA+Timer1+ADC3)

void Load_State_From_FRAM(void); // FRAM 복구
void LCD_Display_Background(void); // 배경 화면
void UART_Send_String(char *str);  // 문자열 송신

// 유틸리티
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

// ================= [메인 함수] =================
// 우수 과제 스타일: main은 초기화만 하고 비워둠
int main(void)
{
    // 1. 하드웨어 라이브러리 초기화 (순서 중요!)
    LCD_Init();
    Fram_Init();           
    Fram_Status_Config();  
    
    // 2. 주변장치 설정 (함수 하나로 묶음)
    System_Init_Sequence();

    // 3. 초기 화면 및 상태 복구
    LCD_Display_Background();
    Load_State_From_FRAM(); 

    // 4. 무한 루프 (할 일 없음, 모든 일은 ADC_IRQHandler에서 처리)
    while (1)
    {
        // Sleep Mode로 들어가도 됨 (__WFI();)
    }
}

// ================= [초기화 그룹] =================
void System_Init_Sequence(void)
{
    _GPIO_EXTI_Init();   // 1. 버튼/LED
    _UART_Init();        // 2. 통신
    _PWM_Timers_Init();  // 3. 모터 (PWM)
    _ADC_System_Init();  // 4. 센서 (가장 중요, 마지막에 켜기)
}

// ================= [1. GPIO & EXTI] =================
void _GPIO_EXTI_Init(void)
{
    // --- [LED: PG4, PG6] ---
    // 1. 클럭 활성화 (GPIOG는 AHB1 버스, Bit 6)
    RCC->AHB1ENR |= (1 << 6); 

    // 2. 모드 설정 (Output Mode = 01)
    // PG4: Output Mode
    GPIOG->MODER |= (1 << (4 * 2)); 
    // PG6: Output Mode
    GPIOG->MODER |= (1 << (6 * 2)); 
    
    // --- [Switch: SW4(PH12), SW6(PH14)] ---
    // 1. 클럭 활성화 (GPIOH는 AHB1 버스, Bit 7)
    RCC->AHB1ENR |= (1 << 7); 

    // 2. 모드 설정 (Input Mode = 00)
    // PH12: Input Mode (기존 비트 클리어)
    GPIOH->MODER &= ~(3 << (12 * 2)); 
    // PH14: Input Mode (기존 비트 클리어)
    GPIOH->MODER &= ~(3 << (14 * 2)); 

    // --- [EXTI 설정] ---
    // 1. SYSCFG 클럭 활성화 (APB2 버스, Bit 14)
    RCC->APB2ENR |= (1 << 14); 
    
    // 2. EXTICR: 포트 연결 (Port H = 0x7)
    // [공식] EXTICR[n]: 핀번호/4 = 배열인덱스, (핀번호%4)*4 = 시프트양
    
    // EXTI12 (SW4) -> EXTICR[3]의 [0~3]비트 (12 % 4 = 0, 0 * 4 = 0)
    SYSCFG->EXTICR[3] &= ~(0xF << 0); // 기존 설정 클리어
    SYSCFG->EXTICR[3] |=  (0x7 << 0); // Port H(0x7) 연결

    // EXTI14 (SW6) -> EXTICR[3]의 [8~11]비트 (14 % 4 = 2, 2 * 4 = 8)
    SYSCFG->EXTICR[3] &= ~(0xF << 8); // 기존 설정 클리어
    SYSCFG->EXTICR[3] |=  (0x7 << 8); // Port H(0x7) 연결

    // 3. IMR: 인터럽트 마스크 해제
    EXTI->IMR |= (1 << 12); // EXTI12 허용
    EXTI->IMR |= (1 << 14); // EXTI14 허용

    // 4. FTSR: 하강 에지 트리거 (버튼 누를 때)
    EXTI->FTSR |= (1 << 12); // EXTI12 Falling Edge
    EXTI->FTSR |= (1 << 14); // EXTI14 Falling Edge

    // 5. NVIC: EXTI15_10 인터럽트 활성화 (Vector 40)
    // ISER[1]은 32~63번 인터럽트 관리. (40 - 32 = 8번째 비트)
    NVIC->ISER[1] |= (1 << 8); 
}

// ================= [2. UART (PC통신)] =================
void _UART_Init(void)
{
    // PA9(TX), PA10(RX) 사용
    // 1. 클럭 활성화
    RCC->AHB1ENR |= (1 << 0); // GPIOA 클럭 (AHB1)
    RCC->APB2ENR |= (1 << 4); // USART1 클럭 (APB2)

    // 2. GPIO 모드 설정 (Alternate Function = 10)
    // PA9: AF Mode
    GPIOA->MODER |= (2 << (9 * 2));
    // PA10: AF Mode
    GPIOA->MODER |= (2 << (10 * 2));
    
    // 3. AFR 설정 (AF7 = USART1)
    // 핀번호가 8 이상이므로 AFR[1] 사용. (핀번호 - 8) * 4 만큼 시프트
    // PA9 -> AF7
    GPIOA->AFR[1] |= (7 << (1 * 4)); 
    // PA10 -> AF7
    GPIOA->AFR[1] |= (7 << (2 * 4)); 

    // 4. Baudrate 설정 (19200bps)
    // 84MHz / 19200 = 4375
    USART1->BRR = 4375;

    // 5. Control Register 설정 (CR1)
    USART1->CR1 |= (1 << 13); // UE: USART Enable
    USART1->CR1 |= (1 << 3);  // TE: Transmitter Enable
    USART1->CR1 |= (1 << 2);  // RE: Receiver Enable
    USART1->CR1 |= (1 << 5);  // RXNEIE: RX Interrupt Enable

    // 6. NVIC: USART1 인터럽트 활성화 (Vector 37)
    // 37 - 32 = 5번째 비트
    NVIC->ISER[1] |= (1 << 5);
}

// ================= [3. PWM Timers] =================
void _PWM_Timers_Init(void)
{
    // --- [TIM4 CH2: PB7 (엔진)] ---
    // 1. 클럭 활성화
    RCC->AHB1ENR |= (1 << 1); // GPIOB 클럭 (AHB1)
    RCC->APB1ENR |= (1 << 2); // TIM4 클럭 (APB1)

    // 2. GPIO 설정 (PB7 -> AF2)
    GPIOB->MODER |= (2 << (7 * 2)); // AF Mode
    // 7번 핀은 AFR[0] 사용 (0~7번)
    GPIOB->AFR[0] |= (2 << (7 * 4)); // AF2 (TIM4) 연결

    // 3. 시간 설정 (10kHz, 5초 주기)
    TIM4->PSC = 8400 - 1;  // 분주비: 84MHz / 8400 = 10kHz (0.1ms)
    TIM4->ARR = 50000 - 1; // 주기: 0.1ms * 50000 = 5000ms (5초)
    
    // 4. PWM 모드 설정 (CH2)
    // CH2는 CCMR1의 [15:8] 비트 사용
    TIM4->CCMR1 |= (6 << 12); // OC2M: PWM Mode 1 (110)
    TIM4->CCMR1 |= (1 << 11); // OC2PE: Preload Enable
    
    // 5. 출력 활성화 및 시작
    TIM4->CCER  |= (1 << 4);  // CC2E: Output Enable
    TIM4->CR1   |= (1 << 0);  // CEN: Counter Enable

    // --- [TIM14 CH1: PF9 (핸들)] ---
    // 1. 클럭 활성화
    RCC->AHB1ENR |= (1 << 5); // GPIOF 클럭 (AHB1)
    RCC->APB1ENR |= (1 << 8); // TIM14 클럭 (APB1)

    // 2. GPIO 설정 (PF9 -> AF9)
    GPIOF->MODER |= (2 << (9 * 2)); // AF Mode
    // 9번 핀은 AFR[1] 사용 (8~15번)
    GPIOF->AFR[1] |= (9 << (1 * 4)); // AF9 (TIM14) 연결

    // 3. 시간 설정 (200kHz, 400us 주기)
    TIM14->PSC = 420 - 1; // 분주비: 84MHz / 420 = 200kHz (5us)
    TIM14->ARR = 80 - 1;  // 주기: 5us * 80 = 400us

    // 4. PWM 모드 설정 (CH1)
    // CH1은 CCMR1의 [7:0] 비트 사용
    TIM14->CCMR1 |= (6 << 4); // OC1M: PWM Mode 1 (110)
    TIM14->CCMR1 |= (1 << 3); // OC1PE: Preload Enable
    
    // 5. 출력 활성화 및 시작
    TIM14->CCER  |= (1 << 0); // CC1E: Output Enable
    TIM14->CR1   |= (1 << 0); // CEN: Counter Enable
}

// ================= [4. ADC System (DMA+Trig+ADC)] =================
void _ADC_System_Init(void)
{
    // 1. DMA (Data Mover) - ADC3는 DMA2 Stream0 Ch2
    RCC->AHB1ENR |= (1 << 22); // DMA2 클럭
    
    DMA2_Stream0->CR &= ~(1 << 0); // 먼저 DMA 끄기 (설정 변경 위함)
    while(DMA2_Stream0->CR & 1);   // 꺼질 때까지 대기

    DMA2_Stream0->PAR = (uint32_t)&ADC3->DR; // 출발지: ADC 데이터 레지스터
    DMA2_Stream0->M0AR = (uint32_t)ADC_Data; // 도착지: 메모리 배열
    DMA2_Stream0->NDTR = 2; // 데이터 개수 (센서 2개)
    
    // DMA 모드 설정
    DMA2_Stream0->CR |= (2 << 25); // CHSEL: Channel 2 선택
    DMA2_Stream0->CR |= (1 << 11); // PSIZE: 16bit (Half-word)
    DMA2_Stream0->CR |= (1 << 13); // MSIZE: 16bit (Half-word)
    DMA2_Stream0->CR |= (1 << 10); // MINC: 메모리 주소 증가 (배열 저장)
    DMA2_Stream0->CR |= (1 << 8);  // CIRC: 순환 모드 (계속 받기)
    // DIR: Peripheral-to-Memory (00) -> 기본값이므로 설정 안 해도 됨
    
    DMA2_Stream0->CR |= (1 << 0); // DMA 활성화

    // 2. Trigger Timer (TIM1 CH3: PE13) - 400ms 주기 신호 생성
    RCC->AHB1ENR |= (1 << 4); // GPIOE 클럭
    RCC->APB2ENR |= (1 << 0); // TIM1 클럭 (APB2)

    // PE13 설정 (AF1 = TIM1)
    GPIOE->MODER |= (2 << (13 * 2)); // AF Mode
    GPIOE->AFR[1] |= (1 << (5 * 4)); // AF1 연결 (13 - 8 = 5)

    // 시간 설정 (20kHz, 400ms 주기)
    TIM1->PSC = 8400 - 1; // 168MHz / 8400 = 20kHz
    TIM1->ARR = 8000 - 1; // 0.05ms * 8000 = 400ms
    
    // PWM 모드 (트리거 신호용)
    TIM1->CCMR2 |= (6 << 4); // CH3 PWM Mode 1
    TIM1->CCR3 = 4000;       // Duty 50%
    TIM1->CCER |= (1 << 8);  // CC3E: Output Enable
    TIM1->BDTR |= (1 << 15); // MOE: Main Output Enable (TIM1 필수)
    TIM1->CR1 |= (1 << 0);   // Timer Start

    // 3. ADC3 Init (PA1, PF3)
    RCC->AHB1ENR |= (1 << 0); // GPIOA 클럭
    RCC->AHB1ENR |= (1 << 5); // GPIOF 클럭
    
    // 핀 설정 (Analog Mode = 11)
    GPIOA->MODER |= (3 << (1 * 2)); // PA1 Analog
    GPIOF->MODER |= (3 << (3 * 2)); // PF3 Analog

    RCC->APB2ENR |= (1 << 10); // ADC3 클럭
    ADC->CCR |= (1 << 16); // Prescaler /4 (84MHz/4 = 21MHz)

    // CR1 설정
    ADC3->CR1 |= (1 << 8);  // SCAN Mode Enable
    ADC3->CR1 |= (1 << 24); // RES: 10-bit (01)
    ADC3->CR1 |= (1 << 5);  // EOCIE: 인터럽트 활성화 [핵심!]

    // CR2 설정 (Trigger & DMA)
    ADC3->CR2 |= (3 << 28); // EXTEN: Both Edge Trigger
    ADC3->CR2 |= (2 << 24); // EXTSEL: TIM1 CC3 (0010)
    ADC3->CR2 |= (1 << 8);  // DMA Enable
    ADC3->CR2 |= (1 << 9);  // DDS: DMA Disable Selection (Circular용)

    // Sequence (SQR) - 순서 중요!
    ADC3->SQR1 |= (1 << 20); // L=1 (총 2개 변환, 2-1=1)
    ADC3->SQR3 |= (1 << 0);  // 1순위: CH1 (PA1)
    ADC3->SQR3 |= (9 << 5);  // 2순위: CH9 (PF3) -> 5비트씩 밀림

    // 인터럽트 및 ADC 시작
    NVIC->ISER[0] |= (1 << 18); // ADC Global Interrupt 활성화
    ADC3->CR2 |= (1 << 0);      // ADON: ADC 켜기
}

// ================= [인터럽트 핸들러 (Core Logic)] =================

// 1. ADC 핸들러: 모든 제어 로직을 여기서 처리 (400ms 주기)
void ADC_IRQHandler(void)
{
    // Flag Clear
    ADC3->SR &= ~(1 << 1); 

    // STOP 상태면 아무것도 안 함 (또는 0으로 덮어쓰기)
    if (System_State == 0) return; 

    // --- A. 거리 계산 ---
    // D1 (선도차): 3~19m
    float vol1 = (float)ADC_Data[0] * 3.3f / 1023.0f;
    int dist1 = (int)(vol1 * 5) + 3;
    // 범위 제한
    if(dist1 < 3) dist1 = 3; 
    if(dist1 > 19) dist1 = 19;

    // D2 (인도): 0~3m
    float vol2 = (float)ADC_Data[1] * 3.3f / 1023.0f;
    int dist2 = (int)vol2;
    // 범위 제한
    if(dist2 > 3) dist2 = 3;

    // --- B. 모터 제어 ---
    // 속도 (D1)
    int sp_per = 0;
    if (dist1 >= 19) sp_per = 90;
    else if (dist1 < 3) sp_per = 10;
    else sp_per = ((dist1 - 1) / 2) * 10;
    TIM4->CCR2 = (50000 * sp_per) / 100; // PWM Update

    // 방향 (D2)
    int dir_per = 0;
    char *dir_str = "F";
    if (dist2 == 0) { dir_per = 30; dir_str = "R"; }
    else if (dist2 == 1) { dir_per = 0; dir_str = "F"; }
    else { dir_per = 90; dir_str = "L"; }
    TIM14->CCR1 = (80 * dir_per) / 100; // PWM Update

    // --- C. LCD 그리기 (우수과제 방식: Whiteout) ---
    // D1 (Red Bar)
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(35, (2*12)+2, 80, 6); // 지우기
    LCD_SetBrushColor(RGB_RED);
    float r1 = (float)(dist1 - 3) / 16.0f;
    LCD_DrawFillRect(35, (2*12)+2, (int)(r1*80), 6); // 그리기

    // D2 (Green Bar)
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(35, (3*12)+4, 80, 6); // 지우기
    LCD_SetBrushColor(RGB_GREEN);
    float r2 = (float)dist2 / 3.0f;
    LCD_DrawFillRect(35, (3*12)+4, (int)(r2*80), 6); // 그리기

    // 텍스트 갱신
    char buf[20];
    LCD_SetBackColor(RGB_WHITE); LCD_SetTextColor(RGB_BLUE);
    sprintf(buf, "%2d", dist1); LCD_DisplayText(2, 17, buf);
    sprintf(buf, "%2d", dist2); LCD_DisplayText(3, 17, buf);
    
    LCD_SetFont(&Gulim7);
    sprintf(buf, "%2d", sp_per); LCD_DisplayText(5, 7, buf);
    LCD_DisplayText(5, 20, dir_str);
    LCD_SetFont(&Gulim8);

    // --- D. PC 전송 ---
    sprintf(buf, "%dm ", dist1);
    UART_Send_String(buf);
}

// 2. EXTI 핸들러 (버튼)
void EXTI15_10_IRQHandler(void)
{
    // SW4 (MOVE) - Bit 12
    if (EXTI->PR & (1 << 12)) 
    {
        EXTI->PR |= (1 << 12); // Clear
        System_State = 1;
        Fram_Write(0x1201, 'M');
        
        // 즉시 반응 (LED/LCD)
        GPIOG->ODR |= (1<<4); 
        GPIOG->ODR &= ~(1<<6);
        LCD_SetTextColor(RGB_BLUE); 
        LCD_DisplayText(1, 18, "M");
    }

    // SW6 (STOP) - Bit 14
    if (EXTI->PR & (1 << 14)) 
    {
        EXTI->PR |= (1 << 14); // Clear
        System_State = 0;
        Fram_Write(0x1201, 'S');

        // 즉시 반응
        GPIOG->ODR &= ~(1<<4); 
        GPIOG->ODR |= (1<<6);
        LCD_SetTextColor(RGB_BLUE); 
        LCD_DisplayText(1, 18, "S");
        
        // STOP 시 초기화
        TIM4->CCR2 = 0; 
        TIM14->CCR1 = 0; // 모터 정지
        
        // 화면 초기화
        LCD_DisplayText(2, 17, "0 "); 
        LCD_DisplayText(3, 17, "1 ");
        
        // 그래프 지우기
        LCD_SetBrushColor(RGB_WHITE); 
        LCD_DrawFillRect(35, (2*12)+2, 80, 6);
        LCD_DrawFillRect(35, (3*12)+4, 80, 6);
        
        // D2=1m 표시
        LCD_SetBrushColor(RGB_GREEN); 
        LCD_DrawFillRect(35, (3*12)+4, 26, 6);
    }
}

// 3. UART 핸들러 (명령 수신)
void USART1_IRQHandler(void)
{
    if (USART1->SR & (1 << 5)) // RXNE
    {
        char rx = USART1->DR;
        if (rx == 'M' || rx == 'm') { 
            // EXTI의 SW4 로직과 동일하게 호출하거나 플래그 설정
            // 여기서는 코드 중복을 피하기 위해 플래그만 설정
            EXTI->SWIER |= (1 << 12); // SW4 인터럽트 강제 발생 (유용한 팁!)
        }
        else if (rx == 'S' || rx == 's') {
            EXTI->SWIER |= (1 << 14); // SW6 인터럽트 강제 발생
        }
    }
}

// ================= [Helper Functions] =================
void Load_State_From_FRAM(void)
{
    unsigned char state = Fram_Read(0x1201);
    if (state == 'M') EXTI->SWIER |= (1 << 12); // MOVE 강제 실행
    else              EXTI->SWIER |= (1 << 14); // STOP 강제 실행
}

void LCD_Display_Background(void)
{
    LCD_Clear(RGB_WHITE);
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE); LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "LSH 2020130030");
    LCD_DisplayText(1, 0, "Tracking Car");
    LCD_DisplayText(2, 0, "D1:");
    LCD_DisplayText(3, 0, "D2:");
    LCD_SetFont(&Gulim7);
    LCD_DisplayText(5, 0, "SP(DR):  %  DIR(DR):");
    LCD_SetFont(&Gulim8);
}

void UART_Send_String(char *str)
{
    while (*str) {
        while (!(USART1->SR & (1 << 7))); // Wait TXE
        USART1->DR = *str++;
    }
}

void DelayMS(unsigned short wMS) {
    register unsigned short i;
    for (i=0; i<wMS; i++) DelayUS(1000);
}

void DelayUS(unsigned short wUS) {
    volatile int Dly = (int)wUS*17;
    for(; Dly; Dly--);
}