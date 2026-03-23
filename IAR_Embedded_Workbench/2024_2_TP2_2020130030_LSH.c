/////////////////////////////////////////////////////////////
// 과제번호: 마이컴응용 2024 TP2
// 과제명:  Remote-controlled Robot
// 과제개요: 두개의 원격제어기에서 USART통신으로 제어하는 이동로봇구현
//         원격제어기1: PC(MFC 프로그램)
//         원격제어기2: Mobilephone(LightBlue 프로그램)
//         이동로봇: F407 보드(user control program)
//         원격제어기의 역할: 이동로봇에 이동/정지/회전 명령을전송하고, 이동로봇으로부터 로봇상태(이동/정지/회전 상태)를 수신하여화면에표시함. 
//         이동로봇의 역할: 원격제어기로부터 이동/정지/회전 명령을수신하고실행(LCD, LED, Buzzer 작동)함. 
//         로봇상태(이동/정지/회전상태)를원격제어기에 송신함.
// 사용한 하드웨어(기능): GPIO(SW, LED, Joy-stick, Buzzer, GLCD), EXTI, Fram, USART, DMAm ADC
// 제출일: 2024. 12. 11.
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);             // GPIO 초기화
void _USART1_Init(void);           // USART1 초기화: PC와의 통신을 위한 USART1 설정
void _UART4_Init(void);            // UART4 초기화: 블루투스 모듈과의 통신을 위한 UART4 설정
void _ADC3_Init(void);             // ADC3 초기화: 가변저항 값 측정을 위한 ADC3 설정
void _DMA_Init(void);              // DMA 초기화: USART1 수신을 위한 DMA 설정
void _TIM7_Init(void);             // TIM7 초기화: 2초 주기의 인터럽트 생성을 위한 타이머 설정
void DisplayInitScreen(void);      // 초기 화면 표시
void BEEP(void);                   // 부저 동작
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void SerialSendChar(uint8_t c);    // 문자 전송: USART를 통해 단일 문자 전송
void SerialSendString(char *s);    // 문자열 전송: USART를 통해 문자열 전송
float ADC_Read(void);              // ADC 값 읽기: ADC3에서 가변저항 값을 읽어 전압으로 변환
void ProcessCommand(uint8_t command);  // 명령어 처리: 수신된 명령어에 따른 로봇 동작 처리
void UpdateRobotStatus(void);      // 로봇 상태 업데이트: 현재 로봇 상태를 원격 제어기에 전송

uint8_t rx_data[1];  // DMA로 수신할 데이터 버퍼
uint8_t robot_motion = 0x40;  // 초기 상태: 정지
uint8_t robot_turn = 0x02;    // 초기 상태: 직진
uint8_t last_command = 0x40; // 초기 상태: 정지
float adc_value = 0.0;


int main(void)
{
    _GPIO_Init(); 	// GPIO 초기화
    _USART1_Init();
    _UART4_Init();
    _ADC3_Init();
    _DMA_Init();
    _TIM7_Init();
    
    LCD_Init();
    DelayMS(10);
    DisplayInitScreen();



	while (1)
	{
         // 메인 루프에서는 주로 대기 상태
         // 대부분의 작업은 인터럽트 핸들러에서 처리됨
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


}

// USART1 초기화 : PC와의 통신을 위한 USART1 설정
void _USART1_Init(void)   
{
    // USART1 초기화 (PC 통신용)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = (SystemCoreClock / 4) / 9600;  // 9600 baud
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE; 
    
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 5);
}

// UART4 초기화: 블루투스 모듈과의 통신을 위한 UART4 설정
void _UART4_Init(void)
{
    // UART4 초기화 (블루투스 통신용)
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    UART4->BRR = (SystemCoreClock / 2) / 9600;  // 9600 baud
    UART4->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    
    NVIC_EnableIRQ(UART4_IRQn);
    NVIC_SetPriority(UART4_IRQn, 6);
}

// ADC3 초기화: 가변저항 값 측정을 위한 ADC3 설정
void _ADC3_Init(void)
{
    // ADC3 초기화
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
    ADC3->CR1 = ADC_CR1_EOCIE;
    ADC3->CR2 = ADC_CR2_ADON;
    ADC3->SQR3 = 1;  // ADC3_IN1 선택
    
    NVIC_EnableIRQ(ADC_IRQn);
    NVIC_SetPriority(ADC_IRQn, 7);
}

// DMA 초기화: USART1 수신을 위한 DMA 설정
void _DMA_Init(void)
{
    // DMA 초기화 (USART1 RX용)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    DMA2_Stream2->CR = (4 << 25) | DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_TCIE;
    DMA2_Stream2->PAR = (uint32_t)&USART1->DR;
    DMA2_Stream2->M0AR = (uint32_t)rx_data;
    DMA2_Stream2->NDTR = 1;
    
    NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    NVIC_SetPriority(DMA2_Stream2_IRQn, 6);
    
    DMA2_Stream2->CR |= DMA_SxCR_EN;
}

// TIM7 초기화: 2초 주기의 인터럽트 생성을 위한 타이머 설정
void _TIM7_Init(void)
{
    // TIM7 초기화 (2초 주기 인터럽트)
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 8399;  // 84MHz / 8400 = 10kHz
    TIM7->ARR = 19999;  // 10kHz / 20000 = 0.5Hz (2초)
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    
    NVIC_EnableIRQ(TIM7_IRQn);
    NVIC_SetPriority(TIM7_IRQn, 7);
}

// 초기 화면 표시: LCD에 초기 화면 정보 표시
void DisplayInitScreen(void)
{
    LCD_Clear(RGB_WHITE);
    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_YELLOW);
    LCD_SetTextColor(RGB_BLACK);
    LCD_DisplayText(0, 0, "RC Robot(LSH)");
    

    LCD_SetFont(&Gulim8);
    LCD_SetBackColor(RGB_WHITE);
    LCD_DisplayText(1, 0, "Rx(PC):");
    LCD_DisplayText(2, 0, "Rx(MP):");
    LCD_DisplayText(3, 0, "Motion:");
    LCD_DisplayText(4, 0, "Turn:");
    LCD_DisplayText(5, 0, "ADC(V):");
    
    LCD_SetTextColor(RGB_BLUE);
    LCD_DisplayText(1, 9, "0x40");
    LCD_DisplayText(2, 9, "0x40");
    LCD_DisplayText(3, 9, "Stop");
    LCD_DisplayText(4, 7, "Straight");
    LCD_DisplayText(5, 9, "0.0");
}

// 명령어 처리: 수신된 명령어에 따른 로봇 동작 처리
void ProcessCommand(uint8_t command)
{
    last_command = command; // 최근 명령 저장
    switch(command)
    {
        case 0x80:  // 이동
            robot_motion = 0x80;
            GPIOG->ODR |= 0x80;  // LED7 ON
            BEEP(); DelayMS(500); BEEP();
            LCD_DisplayText(3, 9, "Move ");
            break;
        case 0x40:  // 정지
            robot_motion = 0x40;
            GPIOG->ODR &= ~0x80;  // LED7 OFF
            BEEP(); DelayMS(500); BEEP(); DelayMS(500); BEEP();
            LCD_DisplayText(3, 9, "Stop ");
            break;
        case 0x04:  // 우회전
            if(robot_motion == 0x80) 
            {
                robot_turn = 0x04;
                GPIOG->ODR = (GPIOG->ODR & ~0x07) | 0x04;  // LED2 ON, LED0,1 OFF
                BEEP();
                LCD_DisplayText(4, 7, "Right   ");
            }
            break;
        case 0x02:  // 직진
            if(robot_motion == 0x80) 
            {
                robot_turn = 0x02;
                GPIOG->ODR = (GPIOG->ODR & ~0x07) | 0x02;  // LED1 ON, LED0,2 OFF
                BEEP();
                LCD_DisplayText(4, 7, "Straight");
            }
            break;
        case 0x01:  // 좌회전
            if(robot_motion == 0x80) 
           {
                robot_turn = 0x01;
                GPIOG->ODR = (GPIOG->ODR & ~0x07) | 0x01;  // LED0 ON, LED1,2 OFF
                BEEP();
                LCD_DisplayText(4, 7, "Left    ");
            }
            break;
    }
}

// 로봇 상태 업데이트: 현재 로봇 상태를 원격 제어기에 전송
void UpdateRobotStatus(void)
{
    char status[10];
    if(robot_motion == 0x80) 
    {
        switch(robot_turn) 
        {
            case 0x80:strcpy(status, "Move "); break;
            case 0x04: strcpy(status, "Right "); break;
            case 0x02: strcpy(status, "Straight "); break;
            case 0x01: strcpy(status, "Left "); break;
        }
    } 
    else 
    {
        strcpy(status, "Stop ");
    }
    
    SerialSendString(status);
    UART4->DR = status[0];  // 첫 글자만 블루투스로 전송
}


// ADC 값 읽기: ADC3에서 가변저항 값을 읽어 전압으로 변환
float ADC_Read(void)
{
    ADC3->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC3->SR & ADC_SR_EOC));
    return (ADC3->DR * 3.3f) / 4095.0f;
}


// USART1 인터럽트 핸들러: PC로부터의 명령어 수신 처리
void USART1_IRQHandler(void)
{
    if(USART1->SR & USART_SR_RXNE)
    {
        uint8_t data = USART1->DR;
        ProcessCommand(data);
        char buf[5];
        sprintf(buf, "0x%02X", data);
        LCD_DisplayText(1, 9, buf);
    }
}


// UART4 인터럽트 핸들러: 블루투스 모듈로부터의 명령어 수신 처리
void UART4_IRQHandler(void)
{
    if(UART4->SR & USART_SR_RXNE)
    {
        uint8_t data = UART4->DR;
        ProcessCommand(data);
        char buf[5];
        sprintf(buf, "0x%02X", data);
        LCD_DisplayText(2, 9, buf);
    }
}

// ADC 인터럽트 핸들러: ADC 변환 완료 시 처리
void ADC_IRQHandler(void)
{
    if(ADC3->SR & ADC_SR_EOC)
    {
        adc_value = ADC_Read();
        char buf[10];
        sprintf(buf, "%.1fV", adc_value);
        LCD_DisplayText(5, 9, buf);
        
        // PC에 ADC 값 전송
        SerialSendString(buf);
    }
}

// TIM7 인터럽트 핸들러: 2초마다 로봇 상태 업데이트
void TIM7_IRQHandler(void)
{
    if(TIM7->SR & TIM_SR_UIF)
    {
        TIM7->SR = ~TIM_SR_UIF;
        UpdateRobotStatus();
    }
}

// 문자 전송: USART를 통해 단일 문자 전송
void SerialSendChar(uint8_t c)
{
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

// 문자열 전송: USART를 통해 문자열 전송
void SerialSendString(char *s)
{
    while(*s)
        SerialSendChar(*s++);
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