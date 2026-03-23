/////////////////////////////////////////////////////////////
// HW4: Golf game
// 제출자: 2020130030 이수혁
// 주요 내용 및 구현 방법 
// - SPI 통신 및 센서 데이터 처리: SPI1(PA5~8)과 TIM11(200ms)을 이용한 가속도 폴링
// - 시간 측정 및 게임 제어: TIM6(0.1s) 스톱워치 및 EXTI12(SW4) 게임 시작 트리거
// - 게임 물리 엔진: 가속도 기울기(Ax, Ay)를 속도로 변환하여 공 이동 및 벽 충돌 처리
// - 홀(Hole) 통과 로직: 1->2->3 순서 검사 및 성공 시 색상 변경(Yellow->Blue
/////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include <stdio.h>
#include <math.h>

// --- 매크로 정의 ---
#define ACC_CS_PIN   8                     
#define ACC_CS_ON()  GPIOA->ODR &= ~(1<<ACC_CS_PIN) 
#define ACC_CS_OFF() GPIOA->ODR |= (1<<ACC_CS_PIN)

// ACC 레지스터 주소
#define CTRL_REG1    0x20
#define CTRL_REG4    0x23
#define OUT_X_L      0x28
#define OUT_X_H      0x29
#define OUT_Y_L      0x2A
#define OUT_Y_H      0x2B
#define OUT_Z_L      0x2C
#define OUT_Z_H      0x2D

// 게임 상수
// --- Display Constants (160x128 Resolution) ---
#define LCD_WIDTH       160
#define LCD_HEIGHT      128
#define BALL_SIZE       5
#define HOLE_SIZE       10
#define GAME_X_START    2            // 왼쪽 여백 2픽셀
#define GAME_Y_START    20           // 상단 여백 20픽셀 (제목 표시용)
#define GAME_WIDTH      110          // 게임 박스 폭 (2 + 110 = 112, 우측에 약 48픽셀 남음)
#define GAME_HEIGHT     100          // 게임 박스 높이 (20 + 100 = 120, 하단에 8픽셀 남음)
#define GAME_X_END      (GAME_X_START + GAME_WIDTH)
#define GAME_Y_END      (GAME_Y_START + GAME_HEIGHT)

// --- 전역 변수 ---
int16_t acc_x_raw, acc_y_raw, acc_z_raw;
float g_x, g_y, g_z;

// Ball Position (초기 위치: 경기장 중앙)
int16_t ball_x = GAME_X_START + (GAME_WIDTH / 2) - (BALL_SIZE / 2);
int16_t ball_y = GAME_Y_START + (GAME_HEIGHT / 2) - (BALL_SIZE / 2);

// 시간 관련 변수
uint8_t time_sec = 0;
uint8_t time_dsec = 0; // 0.1초 단위
uint8_t game_running = 0; // 0: Stop, 1: Run, 2: Finish

/// Hole Status
uint8_t current_target_hole = 1; // 1 -> 2 -> 3

// Hole Coordinates (경기장 크기에 맞춰 W/4, H/2 비율로 배치)
int16_t hole_x[3];
int16_t hole_y[3];


// --- 함수 원형 ---
void System_Init(void);
void _GPIO_Init(void);
void _SPI1_Init(void);
void ACC_Init(void);
void _TIM6_Init(void);  // Stopwatch (0.1s)
void _TIM11_Init(void); // ACC Polling (200ms)
void _EXTI_Init(void);  // SW4 (Start/Reset)

uint8_t SPI1_SendByte(uint8_t byte);
void ACC_WriteReg(uint8_t reg, uint8_t data);
uint8_t ACC_ReadReg(uint8_t reg);
void ACC_ReadXYZ(void);

void DrawGameScreen(void);
void UpdateGameLogic(void);
void DisplayTime(void);
void DelayMS(unsigned short wMS);


// =============================================================

int main(void)
{
    // 1. 시스템 및 주변기기 초기화
    System_Init();
    
    // 2. LCD 초기화
    LCD_Init(); 
    
    // 3. 홀 위치 계산 (상수 비율에 따라 자동 계산)
    // Hole 1: 하단 중앙
    hole_x[0] = GAME_X_START + (GAME_WIDTH / 2) - (HOLE_SIZE / 2);
    hole_y[0] = GAME_Y_END - 20;
    
    // Hole 2: 우측 중간
    hole_x[1] = GAME_X_END - (GAME_WIDTH / 4) - (HOLE_SIZE / 2);
    hole_y[1] = GAME_Y_START + (GAME_HEIGHT / 2) - (HOLE_SIZE / 2);
    
    // Hole 3: 좌측 상단
    hole_x[2] = GAME_X_START + (GAME_WIDTH / 4) - (HOLE_SIZE / 2);
    hole_y[2] = GAME_Y_START + 20;

    // 4. 초기 화면 그리기
    DrawGameScreen();
    
    // 5. 메인 루프 (인터럽트에 의해 동작하므로 비워둠)
    while(1)
    {
        
    }
}

// =============================================================

void System_Init(void)
{
    _GPIO_Init();
    _SPI1_Init();
    ACC_Init();
    _TIM6_Init();
    _TIM11_Init();
    _EXTI_Init();
}
// =============================================================

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer) 초기 설정	*/
void _GPIO_Init(void)
{
    // 1. Clock Enable
    RCC->AHB1ENR |= (1 << 0); // GPIOA Enable (SPI)
    RCC->AHB1ENR |= (1 << 7); // GPIOH Enable (SW)
    
    // 2. SPI1 Pins (PA5:SCK, PA6:MISO, PA7:MOSI) -> Alternate Function
    // PA5
    GPIOA->MODER &= ~(3 << 10); // Clear
    GPIOA->MODER |= (2 << 10);  // AF Mode
    // PA6
    GPIOA->MODER &= ~(3 << 12);
    GPIOA->MODER |= (2 << 12);
    // PA7
    GPIOA->MODER &= ~(3 << 14);
    GPIOA->MODER |= (2 << 14);
    
    // 3. CS Pin (PA8) -> Output
    GPIOA->MODER &= ~(3 << 16);
    GPIOA->MODER |= (1 << 16);  // Output Mode
    
    // 4. Output Speed (High Speed)
    GPIOA->OSPEEDR |= (3 << 10); // PA5
    GPIOA->OSPEEDR |= (3 << 12); // PA6
    GPIOA->OSPEEDR |= (3 << 14); // PA7
    GPIOA->OSPEEDR |= (3 << 16); // PA8
    
    // 5. AFR Setting (AF5 for SPI1)
    // AFRL covers pin 0-7. Each pin uses 4 bits.
    GPIOA->AFR[0] |= (5 << 20); // PA5 -> AF5
    GPIOA->AFR[0] |= (5 << 24); // PA6 -> AF5
    GPIOA->AFR[0] |= (5 << 28); // PA7 -> AF5
    
    // 6. SW4 (PH12) Input
    GPIOH->MODER &= ~(3 << 24); // Input mode (00)
    GPIOH->PUPDR &= ~(3 << 24); // Floating (No pull-up/down)
    
    // 7. CS Pin High (Idle state)
    GPIOA->ODR |= (1 << ACC_CS_PIN);
}

// =============================================================

void _SPI1_Init(void)
{
    // 1. Clock Enable
    RCC->APB2ENR |= (1 << 12); // SPI1 Clock Enable
    
    // 2. SPI CR1 Configuration
    SPI1->CR1 = 0;         // Reset
    SPI1->CR1 |= (1 << 2); // MSTR: Master configuration
    SPI1->CR1 |= (1 << 9); // SSM: Software slave management enabled
    SPI1->CR1 |= (1 << 8); // SSI: Internal slave select
    
    // Baud Rate: fPCLK/32
    // BR[2:0] (Bits 5:3) -> 100 (binary) = 4 -> Div 32
    SPI1->CR1 |= (1 << 5); 
    
    // Mode 3 (CPOL=1, CPHA=1) - Typical for MEMS sensors
    SPI1->CR1 |= (1 << 1); // CPOL
    SPI1->CR1 |= (1 << 0); // CPHA
    
    // 3. SPI Enable
    SPI1->CR1 |= (1 << 6); // SPE
}

// =============================================================

void ACC_Init(void)
{
    DelayMS(10);
    // CTRL_REG1 (0x20): 100Hz(0101), Normal mode, XYZ enable(111) -> 0x57
    ACC_WriteReg(CTRL_REG1, 0x57); 
    
    // CTRL_REG4 (0x23): +/- 2g range -> 0x00
    ACC_WriteReg(CTRL_REG4, 0x00);
}
// =============================================================

void _TIM6_Init(void) // 0.1s Stopwatch
{
    // 1. Clock Enable
    RCC->APB1ENR |= (1 << 4); // TIM6 Enable
    
    // 2. Timing Calculation (APB1 = 42MHz, Timer Clock = 84MHz)
    // Target: 0.1s (10Hz)
    // PSC = 8400 - 1 -> 10kHz counter (0.1ms tick)
    TIM6->PSC = 8399;
    // ARR = 1000 - 1 -> 1000 * 0.1ms = 100ms
    TIM6->ARR = 999;
    
    // 3. Interrupt Enable
    TIM6->DIER |= (1 << 0); // UIE (Update Interrupt Enable)
    
    // 4. NVIC Config (TIM6_DAC_IRQn = 54)
    // 54 / 32 = 1, 54 % 32 = 22 -> ISER[1] bit 22
    NVIC->ISER[1] |= (1 << 22);
    
    // Note: Timer starts when game starts
}
// =============================================================


void _TIM11_Init(void) // 200ms ACC Polling
{
    // 1. Clock Enable (APB2 = 84MHz, Timer Clock = 168MHz usually)
    // Assuming 168MHz for calculation ease (typical for F407 APB2 timers)
    RCC->APB2ENR |= (1 << 18); // TIM11 Enable
    
    // 2. Timing Calculation
    // Target: 200ms (5Hz)
    // PSC = 16800 - 1 -> 10kHz counter
    TIM11->PSC = 16799; 
    // ARR = 2000 - 1 -> 2000 * 0.1ms = 200ms
    TIM11->ARR = 1999;
    
    // 3. Interrupt Enable
    TIM11->DIER |= (1 << 0); // UIE
    
    // 4. NVIC Config (TIM1_TRG_COM_TIM11_IRQn = 26)
    // 26 / 32 = 0, 26 % 32 = 26 -> ISER[0] bit 26
    NVIC->ISER[0] |= (1 << 26);
    
    // 5. Start Timer immediately (Sensor needs to be read continuously)
    TIM11->CR1 |= (1 << 0); 
}

// =============================================================


void _EXTI_Init(void) // SW4 (PH12)
{
    // 1. SYSCFG Clock Enable
    RCC->APB2ENR |= (1 << 14);
    
    // 2. Connect EXTI Line 12 to Port H
    // SYSCFG_EXTICR4 (EXTI 12~15)
    // EXTI12 is in EXTICR[3] bits 0-3. Port H is 0111 (7).
    SYSCFG->EXTICR[3] &= ~(0xF << 0); // Clear
    SYSCFG->EXTICR[3] |= (7 << 0);    // Set to Port H
    
    // 3. Interrupt Mask (IMR)
    EXTI->IMR |= (1 << 12);
    
    // 4. Falling Trigger (FTSR)
    EXTI->FTSR |= (1 << 12);
    
    // 5. NVIC Config (EXTI15_10_IRQn = 40)
    // 40 / 32 = 1, 40 % 32 = 8 -> ISER[1] bit 8
    NVIC->ISER[1] |= (1 << 8);
}


// =============================================================


// --- SPI & ACC Helper Functions ---
uint8_t SPI1_SendByte(uint8_t byte)
{
    // Wait for TX Empty
    while(!(SPI1->SR & (1 << 1))); 
    SPI1->DR = byte;
    // Wait for RX Not Empty
    while(!(SPI1->SR & (1 << 0)));
    return SPI1->DR;
}
// =============================================================

void ACC_WriteReg(uint8_t reg, uint8_t data)
{
    GPIOA->ODR &= ~(1 << ACC_CS_PIN); // CS Low
    SPI1_SendByte(reg & 0x7F); // Write mode
    SPI1_SendByte(data);
    GPIOA->ODR |= (1 << ACC_CS_PIN); // CS High
}
// =============================================================
uint8_t ACC_ReadReg(uint8_t reg)
{
    uint8_t val;
    GPIOA->ODR &= ~(1 << ACC_CS_PIN); // CS Low
    SPI1_SendByte(reg | 0x80); // Read mode
    val = SPI1_SendByte(0x00); // Dummy
    GPIOA->ODR |= (1 << ACC_CS_PIN); // CS High
    return val;
}
// =============================================================
void ACC_ReadXYZ(void)
{
    uint8_t xl, xh, yl, yh, zl, zh;
    
    xl = ACC_ReadReg(OUT_X_L);
    xh = ACC_ReadReg(OUT_X_H);
    yl = ACC_ReadReg(OUT_Y_L);
    yh = ACC_ReadReg(OUT_Y_H);
    zl = ACC_ReadReg(OUT_Z_L);
    zh = ACC_ReadReg(OUT_Z_H);
    
    acc_x_raw = (int16_t)((xh << 8) | xl);
    acc_y_raw = (int16_t)((yh << 8) | yl);
    acc_z_raw = (int16_t)((zh << 8) | zl);
    
    // Convert to g (assuming +/- 2g range, 16bit)
    // Factor: 2.0 / 32768.0
    g_x = (float)acc_x_raw * (2.0f / 32768.0f);
    g_y = (float)acc_y_raw * (2.0f / 32768.0f);
    g_z = (float)acc_z_raw * (2.0f / 32768.0f);
}
// =============================================================


// --- Game Logic ---
void DrawGameScreen(void)
{
    LCD_Clear(RGB_WHITE);
    
    // 1. Title
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "Golf:LSH202013030"); // 학번/이름
    
    // 2. Wall (Blue)
    LCD_SetPenColor(RGB_BLUE);
    LCD_DrawRectangle(GAME_X_START, GAME_Y_START, GAME_WIDTH, GAME_HEIGHT);
    
    // 3. Holes
    for(int i=0; i<3; i++) {
        if (i < current_target_hole - 1) 
             LCD_SetBrushColor(RGB_BLUE); // 완료
        else 
             LCD_SetBrushColor(RGB_YELLOW); // 미완료
             
        LCD_SetPenColor(RGB_BLACK);
        LCD_DrawFillRect(hole_x[i], hole_y[i], HOLE_SIZE, HOLE_SIZE);
    }
    
    // 4. Initial Text Labels (Right Side)
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(3, 15, "Ax:"); 
    LCD_DisplayText(5, 15, "Ay:");
    LCD_DisplayText(7, 15, "T:");
    
    DisplayTime();
}
// =============================================================

void DisplayTime(void)
{
    char buf[10];
    sprintf(buf, "%02d:%d", time_sec, time_dsec);
    
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE);
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(7, 17, buf); // Line 7, Col 17
}
// =============================================================

void UpdateGameLogic(void)
{
    if (game_running != 1) return;

    // 1. 이전 공 지우기
    LCD_SetBrushColor(RGB_WHITE);
    LCD_SetPenColor(RGB_WHITE);
    LCD_DrawFillRect(ball_x, ball_y, BALL_SIZE, BALL_SIZE);
    
    // 2. 물리 연산 (Sensitivity 조절: * 10.0f)
    // 보드를 기울이는 방향과 화면 좌표계 매칭 필요 (실험적 조절 필요)
    int16_t dx = (int16_t)(g_x * 10.0f); 
    int16_t dy = (int16_t)(-g_y * 10.0f); // Y축 반전
    
    int16_t next_x = ball_x + dx;
    int16_t next_y = ball_y + dy;
    
    // 3. 충돌 검사 (벽)
    // X축 제한
    if (next_x < GAME_X_START + 1) next_x = GAME_X_START + 1;
    if (next_x + BALL_SIZE > GAME_X_END - 1) next_x = GAME_X_END - 1 - BALL_SIZE;
    
    // Y축 제한
    if (next_y < GAME_Y_START + 1) next_y = GAME_Y_START + 1;
    if (next_y + BALL_SIZE > GAME_Y_END - 1) next_y = GAME_Y_END - 1 - BALL_SIZE;
    
    ball_x = next_x;
    ball_y = next_y;
    
    // 4. 새 공 그리기
    LCD_SetBrushColor(RGB_RED);
    LCD_SetPenColor(RGB_RED);
    LCD_DrawFillRect(ball_x, ball_y, BALL_SIZE, BALL_SIZE);
    
    // 5. 홀 로직 (AABB Collision)
    int target_idx = current_target_hole - 1;
    if (target_idx < 3) {
        int hx = hole_x[target_idx];
        int hy = hole_y[target_idx];
        
        // 공이 홀 영역과 겹치는지 확인
        if (ball_x < hx + HOLE_SIZE &&
            ball_x + BALL_SIZE > hx &&
            ball_y < hy + HOLE_SIZE &&
            ball_y + BALL_SIZE > hy) 
        {
            // Hole In! -> 색상 변경
            LCD_SetBrushColor(RGB_BLUE);
            LCD_SetPenColor(RGB_BLACK);
            LCD_DrawFillRect(hx, hy, HOLE_SIZE, HOLE_SIZE);
            
            current_target_hole++;
            
            // Game Finish
            if (current_target_hole > 3) {
                game_running = 2; // Finish State
                TIM6->CR1 &= ~(1 << 0); // Timer Stop
            }
        }
    }
    
    // 6. 정보창 업데이트 (Ax, Ay)
    char acc_str[10];
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE); 
    
    // Ax
    if(g_x >= 0) LCD_SetTextColor(RGB_RED);
    else LCD_SetTextColor(RGB_BLUE);
    sprintf(acc_str, "%+0.1f", g_x);
    LCD_DisplayText(4, 15, acc_str);
    
    // Ay
    if(g_y >= 0) LCD_SetTextColor(RGB_RED);
    else LCD_SetTextColor(RGB_BLUE);
    sprintf(acc_str, "%+0.1f", g_y);
    LCD_DisplayText(6, 15, acc_str);
}
// =============================================================


// --- Interrupt Handlers ---

// TIM6 : 0.1s Stopwatch
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & (1 << 0)) // UIF Check
    {
        TIM6->SR &= ~(1 << 0); // Clear UIF
        
        if (game_running == 1) {
            time_dsec++;
            if (time_dsec >= 10) {
                time_dsec = 0;
                time_sec++;
                if (time_sec >= 60) time_sec = 0;
            }
            DisplayTime();
        }
    }
}
// =============================================================

// TIM11 : 200ms ACC Read
void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    if (TIM11->SR & (1 << 0))
    {
        TIM11->SR &= ~(1 << 0);
        
        ACC_ReadXYZ();
        UpdateGameLogic(); // 센서 읽을 때마다 게임 화면 갱신
    }
}
// =============================================================

// EXTI15_10 : SW4 (Start/Reset)
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & (1 << 12)) // PR12 Check
    {
        EXTI->PR |= (1 << 12); // Clear PR
        
        // Reset Game Variables
        game_running = 1; // Start
        time_sec = 0;
        time_dsec = 0;
        current_target_hole = 1;
        
        // Reset Position
        ball_x = GAME_X_START + (GAME_WIDTH / 2) - (BALL_SIZE / 2);
        ball_y = GAME_Y_START + (GAME_HEIGHT / 2) - (BALL_SIZE / 2);
        
        // Redraw Screen
        DrawGameScreen();
        
        // Start Timer
        TIM6->CR1 |= (1 << 0);
    }
}
// =============================================================

// Helper: Delay
void DelayMS(unsigned short wMS)
{
    unsigned int i, j;
    for(i=0; i<wMS; i++)
        for(j=0; j<1000; j++); 
}






