//////////////////////////////////////////////////////////////////
// 과제번호: 마이컴 응용 2025 HW3
// 과제명: HW3. USART 통신을 이용한 센서 모니터링
// 제출자 :2020130030 이수혁
// 제출일: 2025. 11. 22
// 과제개요: 3개의 온도센서로부터 취득된 전압값을 AD 변환함
//        ADC 결과값으로부터 전압값과 온도값을 구하여 LCD에표시
//        USART를 이용하여 PC에 온도값을 업로딩
// 사용한 하드웨어(기능): GPIO, DMA, GLCD, ADC, TIM, USART, ADC
/////////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include <stdio.h>
// 함수 선언
void _GPIO_Init(void);
void DisplayInitScreen(void);
void _ADC_Init(void);
void _TIM1_Init(void);  
void _USART1_Init(void); 
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
void BEEP(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
// 전역 변수
volatile uint8_t rx_command = '0'; // 수신된 명령 저장 (초기값 '0')
volatile uint8_t cmd_received_flag = 0; // 명령 수신 플래그
volatile uint16_t ADC_Data[3] = {0, }; // ADC 변환 결과 저장용 배열 (순서: 센서1, 센서2, 센서3)
// 추가 함수 선언
void DisplayCommand(uint8_t cmd); // LCD에 명령 표시 함수
void USART1_SendChar(uint8_t ch); // 문자 송신 함수 (테스트용)
void LCD_DisplayValue(uint8_t , uint8_t , uint16_t ); //테스트용
void UpdateSensorData(void); // 센서 데이터 변환 및 화면 출력 함수
void SendTemperatureData(uint8_t command); // 온도값 계산 및 PC 송신 함수

// =============================================================
int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
        _USART1_Init(); // USART1 초기화 (9N1, Odd Parity, RX 인터럽트 활성화)
        _ADC_Init();   // // ADC 및 DMA 초기화
        _TIM1_Init();  // TIM1 초기화 (400ms 주기 ADC 트리거용)
	DisplayInitScreen();	// LCD 초기화면


	while (1)
	{


              // 인터럽트에서 명령을 받으면 LCD 갱신
              if (cmd_received_flag)
              {
              DisplayCommand(rx_command);    // 1. LCD 상단에 수신된 명령 표시    
              SendTemperatureData(rx_command);  // 2. 해당 센서의 온도값을 PC로 전송        
              cmd_received_flag = 0;  // 3. 플래그 초기화
              }

              UpdateSensorData(); // 센서 데이터 변환 및 LCD 갱신 (텍스트 + 막대그래프)

              DelayMS(200); // LCD 갱신 속도 조절

	} // while

} // main

// =============================================================
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

// =============================================================
// GLCD 초기화면 설정 함수 
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8

        LCD_SetPenColor(GET_RGB(0, 0, 128));           // 그림 색상:  어두운 파란색       
        LCD_DrawFillRect(0, 0, 128, 26);


        LCD_SetTextColor(RGB_YELLOW);  //글자 색: 노란색
 	LCD_SetBackColor(GET_RGB(0, 0, 128));	// 글자배경색 : 어두운 파란색
	LCD_DisplayText(0, 0,"TMP monitor");  
	LCD_DisplayText(1, 0,"LSH 2020130030");     
  
        LCD_SetTextColor(RGB_BLACK);  //글자 색: 검은색
 	LCD_SetBackColor(RGB_WHITE);	// 글자배경색 : 흰색
	LCD_DisplayText(2, 0,"S1:   C(    V)");  
        LCD_DisplayText(4, 0,"S2:   C(    V)");     
        LCD_DisplayText(6, 0,"S3:   C(    V)");  
 
        DisplayCommand('0');

}	

// =============================================================
// 명령값 갱신 함수
void DisplayCommand(uint8_t cmd)
{

    char buf[2];
    buf[0] = cmd;
    buf[1] = '\0';
    
    LCD_SetTextColor(RGB_RED);      // 눈에 띄게 빨간색으로 표시
    LCD_SetBackColor(RGB_WHITE);    //흰 색 배경
    LCD_DisplayText(1, 17, buf);   
}

// =============================================================
//  USART1 초기화 함수 (9비트, Odd Parity, 38400bps)
void _USART1_Init(void)
{
    //  [1] USART1 핀(PA9, PA10) 설정 
    
    // 1. GPIOA 클럭 활성화
    RCC->AHB1ENR |= (1 << 0);   // GPIOA EN (Bit 0)

    // 2. PA9 (TX) 설정: Alternate Function (AF) 모드
    GPIOA->MODER &= ~(3 << 18); // PA9 기존 설정 초기화 (MODER9[1:0] = 00)
    GPIOA->MODER |= (2 << 18);  // PA9 AF 모드 설정 (MODER9[1:0] = 10)

    // 3. PA10 (RX) 설정: Alternate Function (AF) 모드
    GPIOA->MODER &= ~(3 << 20); // PA10 기존 설정 초기화
    GPIOA->MODER |= (2 << 20);  // PA10 AF 모드 설정

    // 4. PA9를 AF7 (USART1_TX)로 연결
    GPIOA->AFR[1] &= ~(0xF << 4); // AFRH9 초기화 (핀 9번은 AFR[1]의 4~7비트)
    GPIOA->AFR[1] |= (7 << 4);    // AFRH9 = 0111 (AF7)

    // 5. PA10을 AF7 (USART1_RX)로 연결
    GPIOA->AFR[1] &= ~(0xF << 8); // AFRH10 초기화 (핀 10번은 AFR[1]의 8~11비트)
    GPIOA->AFR[1] |= (7 << 8);    // AFRH10 = 0111 (AF7)


    // --- [2] USART1 통신 모듈 설정 ---

    // 1. USART1 클럭 활성화
    RCC->APB2ENR |= (1 << 4);   // USART1 EN

    // 2. Baud Rate 설정: 38400bps (PCLK2=84MHz 기준)
    // BRR = 0x88C (Mantissa=136, Fraction=12)
    USART1->BRR = 0x088C;

    // 3. Word Length 설정: 9 Data bits (Parity 비트 포함)
    USART1->CR1 &= ~(1 << 12);  // M bit 초기화
    USART1->CR1 |= (1 << 12);   // M = 1 (9 bits)

    // 4. Parity 설정: Odd Parity
    USART1->CR1 |= (1 << 10);   // PCE = 1 (Parity 활성화)
    USART1->CR1 |= (1 << 9);    // PS = 1 (Odd Parity)

    // 5. 송수신 모드 활성화
    USART1->CR1 |= (1 << 3);    // TE = 1 (Transmitter Enable)
    USART1->CR1 |= (1 << 2);    // RE = 1 (Receiver Enable)

    // 6. RX 인터럽트 활성화
    USART1->CR1 |= (1 << 5);    // RXNEIE = 1 (RX Not Empty Interrupt Enable)

    // 7. NVIC 인터럽트 설정 (USART1_IRQn = 37)
    NVIC->ISER[1] |= (1 << 5);  // ISER[1]의 5번째 비트 = 37번 인터럽트 활성화

    // 8. USART1 최종 활성화
    USART1->CR1 |= (1 << 13);   // UE = 1 (USART Enable)
}

// =============================================================
//  USART1 인터럽트 핸들러 (데이터 수신 처리)
void USART1_IRQHandler(void)
{
    // 수신 데이터가 있는지 확인 (RXNE 비트)
    if (USART1->SR & (1 << 5)) 
    {
        // 데이터 읽기 (DR 레지스터 읽으면 RXNE 클리어됨)
        uint8_t data = USART1->DR & 0xFF; // 하위 8비트만 유효 (9번째 비트는 패리티)

        // 명령값 확인 ('1', '2', '3')
        // 과제 요구사항: 초기에는 '0', 수신된 값 표시
        if (data == '1' || data == '2' || data == '3')
        {
            rx_command = data;
            cmd_received_flag = 1; // 메인 루프에서 LCD 갱신하도록 플래그 설정
        }
    }
}

// =============================================================
//  송신용 함수 (Polling 방식 - Step 5에서 사용 예정)
void USART1_SendChar(uint8_t ch)
{
    // TXE (Transmit Data Register Empty) 플래그 대기
    while (!(USART1->SR & (1 << 7))); // Wait until TXE = 1
    
    USART1->DR = ch;
}

// =============================================================
// TIM1 초기화 : 400ms 주기설정, ADC 트리거(CH2) 생성
void _TIM1_Init(void)
{
    // 1. TIM1 클럭 활성화
    RCC->APB2ENR |= (1 << 0);   // TIM1 Enable

    // 2. Prescaler 설정 (84MHz -> 10kHz)
    // 84,000,000 / 8400 = 10,000Hz (0.1ms 단위)
    TIM1->PSC = 8400-1;

    // 3. Auto Reload Register (ARR) 설정 (10kHz -> 2.5Hz = 400ms)
    // 10,000 / 4000 = 2.5Hz (주기 400ms)
    TIM1->ARR = 4000-1;

    // 4. PWM 모드 설정 (CH2를 ADC 트리거로 사용)
    // CC2S = 00 (Output), OC2M = 110 (PWM mode 1) 또는 Toggle
    // 여기서는 확실한 에지 생성을 위해 PWM 모드 1 사용
    TIM1->CCMR1 &= ~(3 << 8);   // CC2S = 00 (Output)
    TIM1->CCMR1 |= (6 << 12);   // OC2M = 110 (PWM mode 1)

    // 5. Capture/Compare 2 Enable
    TIM1->CCER |= (1 << 4);     // CC2E = 1 (Output enable)

    // 6. Pulse Width 설정 (트리거 시점)
    // ARR보다 작은 값이면 됨. 중간 지점인 2000 설정
    TIM1->CCR2 = 2000;

    // 7. TIM1 Main Output Enable (MOE) - 고급 타이머는 필요
    TIM1->BDTR |= (1 << 15);    // MOE = 1

    // 8. TIM1 카운터 활성화
    TIM1->CR1 |= (1 << 0);      // CEN = 1
}
// =============================================================
// 숫자를 LCD에 표시하는 도우미 함수
void LCD_DisplayValue(uint8_t ln, uint8_t col, uint16_t val)
{
    char buf[10];
    sprintf(buf, "%4d", val); // 숫자를 4자리 문자열로 변환 (예: 123 -> " 123")
    LCD_DisplayText(ln, col, buf);
}

// =============================================================
// ADC1 및 DMA 초기화 (Scan mode, TIM1 Trigger)
void _ADC_Init(void)
{
    // --- [1] GPIO 설정 (아날로그 모드) ---
    // PA1 (ADC1_IN1), PB0 (ADC1_IN8)
    
    // GPIOA, GPIOB 클럭 활성화
    RCC->AHB1ENR |= (1 << 0);   // GPIOA EN
    RCC->AHB1ENR |= (1 << 1);   // GPIOB EN

    // PA1 Analog mode
    GPIOA->MODER |= (3 << 2);   // MODER1 = 11 (Analog)
    
    // PB0 Analog mode
    GPIOB->MODER |= (3 << 0);   // MODER0 = 11 (Analog)


    // --- [2] DMA2 설정 (ADC1은 DMA2 Stream0 Channel0 사용) ---
    
    // 1. DMA2 클럭 활성화
    RCC->AHB1ENR |= (1 << 22);  // DMA2 EN

    // 2. Stream0 비활성화 (설정 전 꺼야 함)
    DMA2_Stream0->CR &= ~(1 << 0); // EN = 0
    while(DMA2_Stream0->CR & (1 << 0)); // 꺼질 때까지 대기

    // 3. 주소 설정
    DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;    // 주변장치 주소 (ADC 데이터 레지스터)
    DMA2_Stream0->M0AR = (uint32_t)ADC_Data;    // 메모리 주소 (결과 저장 배열)

    // 4. 데이터 개수 설정 (3개 채널)
    DMA2_Stream0->NDTR = 3;

    // 5. 채널 선택 (Channel 0)
    DMA2_Stream0->CR &= ~(7 << 25); // CHSEL = 000

    // 6. 메모리 주소 증가 모드 (MINC)
    DMA2_Stream0->CR |= (1 << 10);  // MINC = 1 (배열 인덱스 증가)

    // 7. Circular 모드 (계속 반복)
    DMA2_Stream0->CR |= (1 << 8);   // CIRC = 1

    // 8. 데이터 크기 (Half-word: 16bit)
    DMA2_Stream0->CR |= (1 << 13);  // MSIZE = 01 (16bit)
    DMA2_Stream0->CR |= (1 << 11);  // PSIZE = 01 (16bit)

    // 9. DMA Stream0 활성화
    DMA2_Stream0->CR |= (1 << 0);   // EN = 1


    // --- [3] ADC1 설정 ---

    // 1. ADC1 클럭 활성화
    RCC->APB2ENR |= (1 << 8);   // ADC1 EN

    // 2. 해상도 설정 (기본 12bit라 생략 가능하지만 명시)
    ADC1->CR1 &= ~(3 << 24);    // RES = 00 (12bit)

    // 3. Scan Mode 활성화 (여러 채널 변환)
    ADC1->CR1 |= (1 << 8);      // SCAN = 1

    // 4. 외부 트리거 활성화 (TIM1 CC2 Event: Rising Edge)
    ADC1->CR2 |= (1 << 28);     // EXTEN = 01 (Rising Edge detection)
    
    // 5. 외부 트리거 소스 선택 (TIM1_CC2 = 0001)
    ADC1->CR2 &= ~(0xF << 24);  // EXTSEL 초기화
    ADC1->CR2 |= (1 << 24);     // EXTSEL = 0001 (TIM1 CH2)

    // 6. DMA 활성화 및 Continuous Request
    ADC1->CR2 |= (1 << 8);      // DMA = 1
    ADC1->CR2 |= (1 << 9);      // DDS = 1 (DMA 요청 계속 발생 - Circular용)

    // 7. 변환 순서 및 채널 개수 설정 (SQR)
    // 총 변환 개수: 3개 (L[3:0] = 2 -> 3개)
    ADC1->SQR1 &= ~(0xF << 20);
    ADC1->SQR1 |= (2 << 20);    // L = 0010 (3 conversions)

    // 순서 1: ADC1_IN1 (PA1 - 가변저항)
    ADC1->SQR3 &= ~(0x1F << 0);
    ADC1->SQR3 |= (1 << 0);     // SQ1 = 1

    // 순서 2: ADC1_IN8 (PB0 - 외부센서)
    ADC1->SQR3 &= ~(0x1F << 5);
    ADC1->SQR3 |= (8 << 5);     // SQ2 = 8

    // 순서 3: ADC1_IN16 (내부 온도센서)
    ADC1->SQR3 &= ~(0x1F << 10);
    ADC1->SQR3 |= (16 << 10);   // SQ3 = 16

    // 8. 내부 온도센서 활성화 (Common Control Register)
    ADC->CCR |= (1 << 23);      // TSVREFE = 1 (Temp Sensor & Vrefint Enable)

    // 9. ADC1 활성화
    ADC1->CR2 |= (1 << 0);      // ADON = 1
}

// =============================================================
// 센서 데이터 변환 및 LCD 출력 (텍스트 + 막대그래프)

void UpdateSensorData(void)
{
    float voltage[3];   // 변환된 전압값
    int temp[3];        // 변환된 온도값 (정수)
    char buf[30];       // LCD 출력용 문자열 버퍼
    
    // 막대그래프 색상 (S1:빨강, S2:초록, S3:파랑)
    uint16_t bar_colors[3] = {RGB_RED, RGB_GREEN, RGB_BLUE}; 
    
    // 텍스트가 표시될 줄 번호 (S1, S2, S3)
    int text_lines[3] = {2, 4, 6}; 
    
    // 막대그래프가 그려질 Y 좌표 (텍스트 아랫줄)
    // Gulim8 폰트 기준 대략적인 픽셀 위치 계산: 

    int bar_y_pos[3] = {40, 67, 94}; 

    // --- 1. ADC 데이터를 전압 및 온도로 변환 ---
    for(int i = 0; i < 3; i++)
    {
        // 1) 전압 계산: 0 ~ 4095 -> 0.0 ~ 3.3V
        voltage[i] = (float)ADC_Data[i] * 3.3f / 4095.0f;
    }

    // 2) 온도 계산
    // 센서 1, 2 (공식: T = 3.5 * V^2 + 1)
    temp[0] = (int)(3.5f * voltage[0] * voltage[0] + 1.0f);
    temp[1] = (int)(3.5f * voltage[1] * voltage[1] + 1.0f);

    // 센서 3 (내부 온도센서 공식)
    // T = (Vsense - V25) / Avg_Slope + 25
    // V25 = 0.76V, Avg_Slope = 0.0025 V/C
    temp[2] = (int)((voltage[2] - 0.76f) / 0.0025f + 25.0f);

    // 3) 온도 범위 제한 (1 ~ 39)
    for(int i = 0; i < 3; i++)
    {
        if(temp[i] < 1) temp[i] = 1;
        if(temp[i] > 39) temp[i] = 39;
    }


    // --- 2. LCD 디스플레이 갱신 ---
   for(int i = 0; i < 3; i++)
    {
        // 배경색은 공통적으로 흰색
        LCD_SetBackColor(RGB_WHITE);

        // [Step 1] "S1: " 출력 (검은색)
        LCD_SetTextColor(RGB_BLACK);
        sprintf(buf, "S%d: ", i + 1);
        LCD_DisplayText(text_lines[i], 0, buf); // 0번 칸부터 시작

        // [Step 2] 온도값 출력 (빨간색)
        LCD_SetTextColor(RGB_RED);
        sprintf(buf, "%2d", temp[i]);
        LCD_DisplayText(text_lines[i], 4, buf); // "S1: " 뒤인 4번 칸

        // [Step 3] "C(" 출력 (검은색)
        LCD_SetTextColor(RGB_BLACK);
        LCD_DisplayText(text_lines[i], 6, "C("); // 6번 칸

        // [Step 4] 전압값 출력 (빨간색)
        LCD_SetTextColor(RGB_RED);
        sprintf(buf, "%.1f", voltage[i]);
        LCD_DisplayText(text_lines[i], 8, buf); // 8번 칸

        // [Step 5] "V)" 출력 (검은색)
        LCD_SetTextColor(RGB_BLACK);
        LCD_DisplayText(text_lines[i], 12, "V)  "); // 12번 칸 (뒤에 공백으로 잔상 제거)


        // --- [Step 6] 막대그래프 표시 (좌우 여백 10pixel) ---
        
        // 1. 막대 배경 지우기 (흰색)
        // X범위: 10 ~ 149 (총 139픽셀 너비)
        LCD_SetBrushColor(RGB_WHITE);
        LCD_DrawFillRect(10, bar_y_pos[i], 139, 8); 

        // 2. 막대 길이 계산
        // 최대길이 = 159 - 10(좌) - 10(우) = 139 pixel
        int max_bar_width = 139;
        int bar_width = (int)((temp[i] / 39.0f) * (float)max_bar_width);
        
        // 3. 색상 설정 및 그리기 (시작점 X=10)
        LCD_SetBrushColor(bar_colors[i]);
        LCD_DrawFillRect(10, bar_y_pos[i], bar_width, 8); 
    }
}

// =============================================================
// 요청받은 센서의 온도값을 계산하여 PC로 전송
// 전송 형식: "XX " (십의자리 + 일의자리 + Space)

void SendTemperatureData(uint8_t command)
{
    int sensor_idx = command - '1'; // '1' -> 0, '2' -> 1, '3' -> 2
    
    // 잘못된 명령이면 함수 종료
    if (sensor_idx < 0 || sensor_idx > 2) return;

    // 1. ADC 데이터에서 현재 온도 다시 계산
    // (UpdateSensorData와 동일한 공식 사용)
    float voltage = (float)ADC_Data[sensor_idx] * 3.3f / 4095.0f;
    int temp;

    if (sensor_idx < 2) // S1, S2
    {
        temp = (int)(3.5f * voltage * voltage + 1.0f);
    }
    else // S3 (내부 센서)
    {
        temp = (int)((voltage - 0.76f) / 0.0025f + 25.0f);
    }

    // 2. 범위 제한 (1 ~ 39)
    if (temp < 1) temp = 1;
    if (temp > 39) temp = 39;

    // 3. UART 송신 (Polling 방식)
    // 형식: 십의자리, 일의자리, 공백
    
    // 3-1. 십의 자리 ('0' ~ '3')
    USART1_SendChar((temp / 10) + '0');
    
    // 3-2. 일의 자리 ('0' ~ '9')
    USART1_SendChar((temp % 10) + '0');
    
    // 3-3. 공백 문자 (' ')
    USART1_SendChar(' ');
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
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