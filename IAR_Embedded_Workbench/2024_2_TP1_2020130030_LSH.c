/////////////////////////////////////////////////////////////
// 과제번호: 마이컴응용 2024 TP1
// 과제명: Smart Watch
// 과제개요: 3개의 기능(화면)을 갖는 smart watch를 제작함. 
//        각 기능(화면)은 스위치(SW7)를 입력하면 전환이 되도록 함
//        초기(1번) 화면은 ‘Alarm’, 2번 화면은 ‘Ball game‘, 3번 화면은‘Thermostat’ 임.
//        3번 화면에서 SW7을 입력하면 1번 화면(’Alarm’)으로 전환함.
// 사용한 하드웨어(기능): GPIO(SW, LED, Joy-stick, Buzzer, GLCD), EXTI, Fram, USART
// 제출일: 2024. 12. 12.
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

#define ACC_CS_ON()  GPIOA->ODR &= ~(1<<8)  // PA8: CS pin
#define ACC_CS_OFF() GPIOA->ODR |= (1<<8)

// ACC 레지스터 주소
#define CTRL_REG1    0x20
#define CTRL_REG4    0x23
#define OUT_X_L      0x28
#define OUT_X_H      0x29
#define OUT_Y_L      0x2A
#define OUT_Y_H      0x2B

void _GPIO_Init(void);
void _EXTI_Init(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);

void _TIM6_Init(void);
void _TIM13_Init(void);
void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void DisplayTime(void);
void USART1_Init(void);
void _SPI1_Init(void);
void ACC_Init(void);
void UpdateBallPosition(void);
void SwitchScreen(void);
void DisplayAlarm(void);
void CheckAlarm(void);
void SetAlarm(uint8_t hour);
void SaveAlarmToFRAM(uint8_t hour);   // FRAM 쓰기
void ACC_WriteReg(uint8_t reg, uint8_t data);
void ADC2_Init(void);// ADC2 초기화 함수
void TIM2_Init(void);// TIM2 초기화 함수 (ADC2 트리거용)
void TIM3_Init(void);// TIM3 초기화 함수 (PWM 신호 생성용)
void DisplayThermostat(void);// Thermostat 화면 표시 함수
void SetHeaterCoolerLevel(float temperature);// 온도에 따른 히터/쿨러 레벨 설정 함수
void SetPWM(uint8_t heater_level, uint8_t cooler_level);  // PWM 신호 설정 함수
void ThermostatMode(void);  // Thermostat 모드 메인 처리 함수
void UpdateBallGame(void);
void DisplayBallGame(void);

float MeasureTemperature(void);// 온도 측정 및 변환 함수

uint8_t SPI1_SendByte(uint8_t byte); 
uint8_t ReadAlarmFromFRAM(void);    // FRAM 읽기
uint8_t current_hour = 0xF;    //현재 시간
uint8_t current_minute = 0xA;  //현재 분
uint8_t alarm_hour = 0;  // 알람 시간
uint8_t alarm_minute = 0;  // 알람 분 (항상 0으로 고정)
uint8_t alarm_triggered = 0;  //알람 트리거
uint8_t led_state = 0; // 0: 둘 다 꺼짐, 1: LED5 켜짐, 2: LED6 켜짐

// Ball game 관련 변수
int16_t acc_x, acc_y;
float g_x, g_y;
uint8_t ball_x = 50, ball_y = 50;  // 공의 초기 위치

uint8_t current_screen = 1;  // 현재 화면 (1: Alarm, 2: Ball game, 3: Thermostat)

int main(void)
{
    _GPIO_Init(); 	// GPIO 초기화
    LCD_Init();	// LCD 모듈 초기화
    DelayMS(10);
    Fram_Init();             // FRAM 초기화 H/W 초기화
    Fram_Status_Config();    // FRAM 초기화 S/W 초기화
    _EXTI_Init();	     // EXTI 초기화
    DisplayInitScreen();     // LCD 초기화면
    _TIM6_Init();            // TIM6 초기화
    _TIM13_Init();           // TIM13 초기화
    USART1_Init();           // USART1 초기화 추가
    _SPI1_Init();            // SPI1 초기화
    ACC_Init();              // ACC 초기화
    ADC2_Init();
    TIM2_Init();
    TIM3_Init();
    GPIOG->ODR &= ~0x00FF;    // LED 초기값: LED0~7 Off


    alarm_hour = ReadAlarmFromFRAM();
    if (alarm_hour > 0x0F)  // 유효하지 않은 값일 경우
    {
        alarm_hour = 0;
        SaveAlarmToFRAM(0);
    }
    DisplayAlarm();

   while (1)
   {
       CheckAlarm();
       switch(current_screen)
      {
          case 1:
              DisplayAlarm();
              break;
          case 2:
              DisplayBallGame();
              break;
          case 3:
              ThermostatMode();
              break;
      }

    // LED 상태 유지
    if (led_state == 1)
    {
        GPIOG->ODR |= 0x0020; //LED5 on
        GPIOG->ODR &= ~0x0040; // LED6 Off
    }
    else if (led_state == 2)
    {
        GPIOG->ODR |= 0x0040; //LED6 on
        GPIOG->ODR &= ~0x0020; // LED5 off
    }
    else
    {
        GPIOG->ODR &= ~0x0060; // LED5, LED6 off
    }

    DelayMS(100);
        }  

    
}



/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer) 초기 설정	*/
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

	// Buzzer (GPIO F) 설정 
	RCC->AHB1ENR |= 0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER |= 0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER &= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

        // SW (GPIO H) 설정 
        RCC->AHB1ENR |= 0x00000080;    // RCC_AHB1ENR : GPIOH(bit#7) Enable
        GPIOH->MODER &= ~0xC0000000;   // GPIOH 15 : Input mode (reset state)
        GPIOH->PUPDR &= ~0xC0000000;   // GPIOH 15 : Floating input (No Pull-up, pull-down)

}

//GLCD 초기화면 설정
void DisplayInitScreen(void)   
{
	 LCD_Clear(RGB_WHITE);		// 화면 클리어 : 흰색
	 LCD_SetFont(&Gulim10);		// 폰트 : 굴림 10
        
         LCD_SetBackColor(RGB_WHITE);	
         LCD_SetTextColor(RGB_BLACK);
         LCD_DisplayText(0, 0, "1.ALARM(LSH)");
         DisplayTime();  // 현재 시간 표시
         DisplayAlarm();

}

//초기 화면 표시 함수
void DisplayTime(void)   
{
    static uint8_t prev_hour = 0xFF, prev_minute = 0xFF;
    if (current_hour != prev_hour || current_minute != prev_minute)
    {
        char time_str[10];
        sprintf(time_str, "%X:%X", current_hour, current_minute);
        LCD_SetFont(&Gulim10);
        LCD_SetBackColor(RGB_WHITE);
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(0, 15, time_str);
        
        prev_hour = current_hour;
        prev_minute = current_minute;
    }
}


//현재 시간 표시 함수
void _TIM6_Init(void) 
{
    RCC->APB1ENR |= (1<<4);  // TIM6 clock enable
    TIM6->PSC = 8399;  // Prescaler 설정
    TIM6->ARR = 9999;  // Auto-reload 값 설정 (1초)
    TIM6->CR1 |= (1<<0);  // 카운터 활성화
    TIM6->DIER |= (1<<0);  // Update 인터럽트 활성화

    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    NVIC_SetPriority(TIM6_DAC_IRQn, 2);  // 인터럽트 우선순위 설정
}

// TIM6 인터럽트 핸들러 (1초마다 호출)
void TIM6_DAC_IRQHandler(void) 
{
     if (TIM6->SR & TIM_SR_UIF)
    {
        current_minute++;
        if (current_minute > 0xF)
        {
            current_minute = 0;
            current_hour++;
            if (current_hour > 0xF)
            {
                current_hour = 0;
            }
        }
        DisplayTime();  // 시간이 변경될 때마다 화면 업데이트
        TIM6->SR &= ~TIM_SR_UIF;  // 인터럽트 플래그 클리어
    }
}


// EXTI 초기화 함수
void _EXTI_Init(void)  
{
    RCC->AHB1ENR |= 0x00000080;    // RCC_AHB1ENR GPIOH Enable
    RCC->APB2ENR |= 0x00004000;    // Enable System Configuration Controller Clock

    SYSCFG->EXTICR[3] |= 0x7000;   // EXTI15에 대한 소스 입력은 GPIOH로 설정
    EXTI->FTSR |= 0x8000;          // Falling trigger enable (EXTI15)
    EXTI->IMR |= 0x8000;           // Interrupt mask register (EXTI15)

    NVIC->ISER[1] |= (1 << 8);     // Enable 'Global Interrupt EXTI15_10'
/*
    RCC->APB2ENR |= (1<<14);  // Enable SYSCFG
    
    SYSCFG->EXTICR[3] |= (7<<12);  // EXTI15 source: GPIOH
    EXTI->FTSR |= (1<<15);  // Falling trigger enable (EXTI15)
    EXTI->IMR |= (1<<15);   // Interrupt mask register (EXTI15)
    
    NVIC->ISER[1] |= (1<<8);  // Enable EXTI15_10 interrupt

*/
}


// EXTI 인터럽트 핸들러 (스위치 입력 처리)
void EXTI15_10_IRQHandler(void)  
{
   if (EXTI->PR & EXTI_PR_PR15) 
    {
        EXTI->PR |= EXTI_PR_PR15;   // 인터럽트 플래그 클리어
        SwitchScreen();
    }
}

// USART1 초기화 함수
void USART1_Init(void)   
{
 // USART1 설정: 9600 baud, 8-bit data, 1 stop bit, Even parity
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // USART1 클럭 활성화
//RCC->APB2ENR |= (1<<4); 

    USART1->BRR = (SystemCoreClock / 2) / 9600;  // 9600 baud rate 설정
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_PCE | USART_CR1_PS;
//    USART1->CR1 = (1<<13) | (1<<3) | (1<<2) | (1<<5) | (1<<10) | (1<<9);  // UE, TE, RE, RXNEIE, PCE, PS


    // USART1 인터럽트 활성화
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 5);  // 인터럽트 우선순위 설정
}



 //USART1 송수신 함수
void USART1_SendChar(char ch) 
{
  while(!(USART1->SR & USART_SR_TXE));  //while(!(USART1->SR & (1<<7)));  // TXE 플래그 확인
  USART1->DR = ch;
}

void USART1_SendString(char* str)
{
  while(*str) USART1_SendChar(*str++);
}

char USART1_ReceiveChar(void)   
{
  while(!(USART1->SR & USART_SR_RXNE));     //while(!(USART1->SR & (1<<5)));  // RXNE 플래그 확인
  return USART1->DR;
}


//USART1 인터럽트 핸들러  (알람 시간 설정 처리)
void USART1_IRQHandler(void) 
{
     if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t received_data = USART1->DR;
        if (received_data >= 0x00 && received_data <= 0x0F)
        {
            SetAlarm(received_data);
        }
        else
        {
            BEEP();
            DelayMS(500);
            BEEP();
        }
    }
}

// FRAM에 알람 시간 저장 함수
void SaveAlarmToFRAM(uint8_t hour)  
{     
        Fram_Write(1200, hour);
}

// FRAM에서 알람 시간 읽기 함수
uint8_t ReadAlarmFromFRAM(void)   
{
        return Fram_Read(1200);
}

// 알람 시간 설정 함수
void DisplayAlarm(void)     
{
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 20, 159, 107);  // 화면 아래쪽만 지우기

    LCD_SetFont(&Gulim10);		// 폰트 : 굴림 10  
    LCD_SetBackColor(RGB_WHITE);	
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "1.ALARM(LSH)");
    char alarm_str[10];
    sprintf(alarm_str, "%X:0", alarm_hour);
    LCD_SetFont(&Gulim10);
    LCD_SetBackColor(RGB_WHITE);    
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(1, 0, alarm_str);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(1, 2, "0");
}

// 알람 시간 설정 함수
void SetAlarm(uint8_t hour)    //알람 설정
{
    alarm_hour = hour;
    SaveAlarmToFRAM(hour);  // 1200번지에 새로운 알람 시간 저장
    DisplayAlarm();
    BEEP();
}

// 알람 체크 함수
void CheckAlarm(void)  
{
   if (current_hour == alarm_hour && current_minute == 0 && !alarm_triggered)
    {
        for (int i = 0; i < 3; i++)
        {
            BEEP();
            DelayMS(500);
        }
        alarm_triggered = 1; // 알람이 울렸음을 표시
    }
    else if (current_minute != 0)
    {
        alarm_triggered = 0; // 다음 시간대를 위해 리셋
    }
}

// SPI1 초기화 함수 (가속도 센서 통신용)
void _SPI1_Init(void)
{
    // SPI1 모듈 활성화
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // GPIO 설정 (PA5:SCK, PA6:MISO, PA7:MOSI, PA8:NSS)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER |= (2<<10) | (2<<12) | (2<<14) | (1<<16);  // Alternate function mode for PA5-7, Output mode for PA8
    GPIOA->AFR[0] |= (5<<20) | (5<<24) | (5<<28);  // AF5(SPI1)
    GPIOA->OSPEEDR |= (3<<10) | (3<<12) | (3<<14) | (3<<16);  // High speed

    // SPI1 설정
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_2;  // Master mode, Software slave management, Internal slave select, fPCLK/32
    SPI1->CR1 |= SPI_CR1_SPE;  // SPI 활성화
/*
    RCC->APB2ENR |= (1<<12);  // SPI1 clock enable
    RCC->AHB1ENR |= (1<<0);   // GPIOA clock enable
    
    GPIOA->MODER |= (2<<10) | (2<<12) | (2<<14) | (1<<16);
    GPIOA->AFR[0] |= (5<<20) | (5<<24) | (5<<28);
    GPIOA->OSPEEDR |= (3<<10) | (3<<12) | (3<<14) | (3<<16);
    
    SPI1->CR1 = (1<<2) | (1<<9) | (1<<8) | (1<<3);
    SPI1->CR1 |= (1<<6);
*/
}

// 가속도 센서 초기화 함수
void ACC_Init(void)  
{
    ACC_CS_OFF();
    DelayMS(1);

    // CTRL_REG1: 데이터 속도 100Hz, X/Y/Z축 활성화
    ACC_WriteReg(CTRL_REG1, 0x57);

    // CTRL_REG4: ±2g 범위 설정
    ACC_WriteReg(CTRL_REG4, 0x00);
}

// SPI를 통한 가속도 센서 레지스터 읽기 함수
uint8_t ACC_ReadReg(uint8_t reg)  
{
    uint8_t data;
    ACC_CS_ON();
    SPI1_SendByte(reg | 0x80);  // 읽기 명령
    data = SPI1_SendByte(0x00);
    ACC_CS_OFF();
    return data;
}

// SPI를 통한 가속도 센서 레지스터 쓰기 함수
void ACC_WriteReg(uint8_t reg, uint8_t data)  
{
    ACC_CS_ON();
    SPI1_SendByte(reg & 0x7F);  // 쓰기 명령
    SPI1_SendByte(data);
    ACC_CS_OFF();
}

// SPI 데이터 송수신 함수
uint8_t SPI1_SendByte(uint8_t byte)   
{
    while(!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = byte;
    while(!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

// 가속도 데이터 읽기 함수
void ACC_ReadXY(void)  
{
    uint8_t xh, xl, yh, yl;
    
    xl = ACC_ReadReg(OUT_X_L);
    xh = ACC_ReadReg(OUT_X_H);
    yl = ACC_ReadReg(OUT_Y_L);
    yh = ACC_ReadReg(OUT_Y_H);
    
    acc_x = (int16_t)((xh << 8) | xl);
    acc_y = (int16_t)((yh << 8) | yl);
    
    // 중력가속도 단위로 변환 (-2g ~ +2g)
    g_x = (float)acc_x / 32768.0f * 2.0f;
    g_y = (float)acc_y / 32768.0f * 2.0f;
    
    // 범위 제한 (-1.0g ~ +1.0g)
    if (g_x < -1.0f) g_x = -1.0f;
    if (g_x > 1.0f) g_x = 1.0f;
    if (g_y < -1.0f) g_y = -1.0f;
    if (g_y > 1.0f) g_y = 1.0f;
}

// Ball game 화면 표시 함수
void DisplayBallGame(void)       
{
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 20, 159, 107);  // 화면 아래쪽만 지우기
    LCD_SetFont(&Gulim10);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "2.Ball game");
    
    // 경기장 그리기
    LCD_SetPenColor(RGB_BLACK);
    LCD_DrawRectangle(0, 20, 100, 100);
    
    // 가속도 값 표시
    LCD_SetFont(&Gulim7);
    char acc_str[30];
    if (g_x >= 0) {
        sprintf(acc_str, "Ax:+%.1f", g_x);
    } else {
        sprintf(acc_str, "Ax:%.1f", g_x);
    }
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(3, 18, "Ax:");
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(3, 21, acc_str + 3);

    if (g_y >= 0) {
        sprintf(acc_str, "Ay:+%.1f", g_y);
    } else {
        sprintf(acc_str, "Ay:%.1f", g_y);
    }
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(4, 18, "Ay:");
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(4, 21, acc_str + 3);
    
    // 공 그리기
    LCD_SetPenColor(RGB_RED);
    LCD_DrawRectangle(ball_x, ball_y + 20, 5, 5);
}

// 공 위치 업데이트 함수
void UpdateBallPosition(void)      
{
    // 이전 공 위치 지우기
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(ball_x, ball_y + 20, 5, 5);

    // 가속도 값을 이용해 공의 위치 업데이트
    int8_t dx = (int8_t)(g_x * 5);  // 가속도에 따른 이동량 조절
    int8_t dy = (int8_t)(g_y * 5);
    
    // X축 이동
    if (ball_x + dx >= 0 && ball_x + dx <= 95) 
    {
        ball_x += dx;
    }
    
    // Y축 이동
    if (ball_y + dy >= 0 && ball_y + dy <= 95) 
    {
        ball_y += dy;
    }
    // 새 공 위치 그리기
    LCD_SetPenColor(RGB_RED);
    LCD_DrawRectangle(ball_x, ball_y + 20, 5, 5);
}

void UpdateBallGame(void)
{
    // 이전 공 위치 지우기
    LCD_SetBrushColor(RGB_WHITE);
    LCD_DrawFillRect(0, 21, 100, 99);  // 전체 게임 영역을 지움

    // 공 위치 업데이트
    UpdateBallPosition();

    // 새 공 위치 그리기
    LCD_SetPenColor(RGB_RED);
    LCD_DrawRectangle(ball_x, ball_y + 20, 5, 5);

    // 가속도 값 업데이트
    char acc_str[30];
    LCD_SetFont(&Gulim7);

    sprintf(acc_str, "Ax:%+.1f", g_x);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(3, 18, "Ax:");
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(3, 21, acc_str + 3);

    sprintf(acc_str, "Ay:%+.1f", g_y);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(4, 18, "Ay:");
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayText(4, 21, acc_str + 3);
}


// 화면 전환 함수
void SwitchScreen(void)        
{
    current_screen++;
    if (current_screen > 3) current_screen = 1;
    
    BEEP();
    LCD_Clear(RGB_WHITE);  // 화면 전체 지우기

    switch(current_screen) 
    {
        case 1:
            DisplayAlarm();
            break;
        case 2:
            DisplayBallGame();
            break;
        case 3:
            DisplayThermostat();
            break;
    }
}


// TIM13 초기화 함수 (가속도 센서 데이터 읽기용)
void _TIM13_Init(void)
{
// 150ms 주기로 인터럽트 발생
    RCC->APB1ENR |= RCC_APB1ENR_TIM13EN;
    TIM13->PSC = 8399;    // 84MHz / 8400 = 10kHz
    TIM13->ARR = 1499;    // 10kHz / 1500 = 150ms 주기
    TIM13->DIER |= TIM_DIER_UIE;
    TIM13->CR1 |= TIM_CR1_CEN;
    
    NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 2);
/*
RCC->APB1ENR |= (1<<7);  // TIM13 clock enable
    TIM13->PSC = 8399;
    TIM13->ARR = 1499;
    TIM13->DIER |= (1<<0);  // Update interrupt enable
    TIM13->CR1 |= (1<<0);   // Counter enable
*/
}

// TIM13 인터럽트 핸들러 (150ms마다 호출)
void TIM8_UP_TIM13_IRQHandler(void)
{
if (TIM13->SR & TIM_SR_UIF)
    {
        TIM13->SR &= ~TIM_SR_UIF;
        ACC_ReadXY();
        if (current_screen == 2)
        {
            UpdateBallGame();
        }
    }
}

// ADC2 초기화 함수
void ADC2_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;  // ADC2 클럭 활성화
    ADC2->CR1 = ADC_CR1_EOCIE;  // EOC 인터럽트 활성화
    ADC2->CR2 = ADC_CR2_ADON | ADC_CR2_EXTEN_0 | ADC_CR2_EXTSEL_3;  // ADC 활성화, 외부 트리거 상승 엣지, TIM2 CH4 이벤트로 트리거
    ADC2->SQR3 = 1;  // ADC2_IN1 선택
    
    NVIC_EnableIRQ(ADC_IRQn);
    NVIC_SetPriority(ADC_IRQn, 2);

/*
    RCC->APB2ENR |= (1<<9);  // ADC2 clock enable
    ADC2->CR1 = (1<<5);  // EOC interrupt enable
    ADC2->CR2 = (1<<0) | (1<<28) | (1<<29);  // ADON, EXTEN, EXTSEL
*/
}

// TIM2 초기화 함수 (ADC2 트리거용)
void TIM2_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 8399;  // 84MHz / 8400 = 10kHz
    TIM2->ARR = 4499;  // 10kHz / 4500 = 450ms 주기
    TIM2->CCR4 = 2250;  // 50% 듀티 사이클
    TIM2->CCMR2 = TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2;  // PWM 모드 1
    TIM2->CCER = TIM_CCER_CC4E;  // CC4 출력 활성화
    TIM2->CR1 = TIM_CR1_CEN;  // 타이머 활성화
}

// TIM3 초기화 함수 (PWM 신호 생성용)
void TIM3_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    
    GPIOB->MODER |= GPIO_MODER_MODER0_1;  // PB0를 AF 모드로 설정
    GPIOB->AFR[0] |= 2 << 0;  // PB0를 AF2(TIM3)로 설정
    
    TIM3->PSC = 16799;  // 84MHz / 16800 = 5kHz
    TIM3->ARR = 9999;  // 5kHz / 10000 = 2초 주기
    TIM3->CCR3 = 1000;  // 초기 듀티 사이클 10%
    TIM3->CCMR2 = TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;  // PWM 모드 1
    TIM3->CCER = TIM_CCER_CC3E;  // CC3 출력 활성화
    TIM3->CR1 = TIM_CR1_CEN;  // 타이머 활성화
}

// Thermostat 화면 표시 함수
void DisplayThermostat(void)
{
    LCD_Clear(RGB_WHITE);
    LCD_SetFont(&Gulim10);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "3.Thermostat");
    LCD_DisplayText(1, 0, "T:");
    LCD_DisplayText(2, 0, "H:");
    LCD_DisplayText(2, 4, "C:");
}

// 온도 측정 및 변환 함수
float MeasureTemperature(void)
{
    float voltage = (float)ADC2->DR / 4095 * 3.3;
    return voltage * 40 / 3.3 - 10;  // -10도 ~ 30도로 변환
}

// 온도에 따른 히터/쿨러 레벨 설정 함수
void SetHeaterCoolerLevel(float temperature)
{
    uint8_t heater_level = 0, cooler_level = 0;
    
    if (temperature <= 0) {
        heater_level = 2;
    } else if (temperature <= 10) {
        heater_level = 1;
    } else if (temperature > 20) {
        cooler_level = 1;
    }
    
    LCD_SetTextColor(RGB_RED);
    LCD_DisplayChar(2, 2, heater_level + '0');
    LCD_SetTextColor(RGB_BLUE);
    LCD_DisplayChar(2, 6, cooler_level + '0');
    
    SetPWM(heater_level, cooler_level);
}

// PWM 신호 설정 함수
void SetPWM(uint8_t heater_level, uint8_t cooler_level)
{
    if (heater_level == 2) 
    {
        TIM3->CCR3 = 9000;       // 90% 듀티 사이클
        GPIOG->ODR |= 0x0020;    //LED5 on
        GPIOG->ODR &=~0x0040;    // LED6 Off
        led_state = 1;
    } 
    else if (heater_level == 1) 
    {
        TIM3->CCR3 = 1000;      // 10% 듀티 사이클
        GPIOG->ODR |= 0x0020;   //LED5 on
        GPIOG->ODR &=~0x0040;   // LED6 Off
        led_state = 1;
    } 
    else if (cooler_level == 1) 
    {
        TIM3->CCR3 = 9000;            // 90% 듀티 사이클
        GPIOG->ODR |=0x0040;          //LED6 on
        GPIOG->ODR &=~0x0020;         // LED5 off
        led_state = 2;
    } 
    else 
    {
        TIM3->CCR3 = 1000;            // 10% 듀티 사이클
        GPIOG->ODR &= ~0x0060; // LED5, LED6 off
        led_state = 0;
    }
}

// Thermostat 모드 메인 처리 함수
void ThermostatMode(void)
{
    float temperature = MeasureTemperature();
    
    LCD_SetTextColor(RGB_GREEN);
    char temp_str[10];
    sprintf(temp_str, "%.1f", temperature);
    LCD_DisplayText(1, 3, temp_str);
    
  // 온도 막대 그래프 표시
    int bar_length = (int)(temperature + 11); // -10도에서 30도까지의 범위를 0에서 40으로 매핑
    if (bar_length < 1) bar_length = 1;
    if (bar_length > 41) bar_length = 41;

    LCD_SetPenColor(RGB_GREEN);
    LCD_DrawRectangle(80, 20, bar_length, 10); // 막대 그래프의 외곽선

    if (temperature <= 0) {
        LCD_SetBrushColor(RGB_BLUE);
    } else if (temperature <= 10) {
        LCD_SetBrushColor(GET_RGB(0, 255, 255)); // 하늘색
    } else if (temperature <= 20) {
        LCD_SetBrushColor(RGB_GREEN);
    } else {
        LCD_SetBrushColor(RGB_RED);
    }
    LCD_DrawFillRect(80, 20, bar_length, 10);

    SetHeaterCoolerLevel(temperature);
}

// ADC 인터럽트 핸들러
void ADC_IRQHandler(void)
{
    if (ADC2->SR & ADC_SR_EOC) {
        if (current_screen == 3) {
            ThermostatMode();
        }
        ADC2->SR &= ~ADC_SR_EOC;
    }
}


//////////////////////////////////

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