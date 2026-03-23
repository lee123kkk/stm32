/////////////////////////////////////////////////////////////
// 과제명: HW2 엘리베이터

// 과제개요: 
//층 입력 명령에 따라 엘리베이터 이동 동작을 실행함Ø 층 입력 명령 방법
// (1) SW(0~7)에 의한 층 명령 입력
// (2) JS(Joy-Stick)(LEFT, RIGHT)에 의한 층 명령 입력Ø 층 이동은 LED(0~7)의 점멸로 표현
//Ø Reset State: 0층 (0층부터 시작)
//Ø LED 점멸 명령은 반드시 다음 중 선택해서 사용할것 (1) GPIOGaODR |= 0x?? , GPIOGaODR &= 0x??
//(2) GPIOGaBSRRL = 0x??, GPIOGaBSRRH = 0x??
// (3) GPIOGaODR <<= 1; (Shift 연산자 이용)
// * GPIOGaODR = 0x?? 와 같은 구문은 사용하지 말것!

// 사용한 하드웨어(기능): GPIO, Joy-stick,... // 제출일: 2024. 5. 20
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////


#include "stm32f4xx.h"

#define SW0_PUSH        0xFE00  //PH8
#define SW1_PUSH        0xFD00  //PH9
#define SW2_PUSH        0xFB00  //PH10
#define SW3_PUSH        0xF700  //PH11
#define SW4_PUSH        0xEF00  //PH12
#define SW5_PUSH        0xDF00  //PH13
#define SW6_PUSH        0xBF00  //PH14
#define SW7_PUSH        0x7F00  //PH15

#define NAVI_RIGHT	0x02E0	//PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0	//PI9 0000 0001 1110 0000 

void _GPIO_Init(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
uint16_t KEY_Scan(void);     
uint16_t JOY_Scan(void);
void BEEP(void);

//int current_led = 0;

int main(void)
{
  _GPIO_Init();		// GPIO (LED & SW) 초기화
	DelayMS(100);
	BEEP();
        int i;                  //변수
	GPIOG->ODR = 0x0001;	// LED 초기값: LED0 ON
  
        while(1)
        {
          switch(JOY_Scan())                            //Joy-Stick(JS)에 의한 층 입력 모드
          {
            case 0x01E0 :	// NAVI_LEFT
               if(GPIOG->ODR != 0x0001){  // 0번 led가 켜져있지 않을 때
                 DelayMS(500);
                 GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                 BEEP();
               }
               else{
                  BEEP();
                  DelayMS(200);
                  BEEP();
               }
            break;
            
            case 0x02E0 :	// NAVI_RIGHT
               if(GPIOG->ODR != 0x0080){  // 7번 led가 켜져있지 않을 때
                 DelayMS(500);
                 GPIOG->ODR <<=1;     //비트 한칸 씩 왼쪽으로 쉬프트
                 BEEP();
               }
               else{
                  BEEP();
                  DelayMS(200);
                  BEEP();
               }
            break;
          }
          switch(KEY_Scan())                     // SW에 의한 층 입력 모드
          {
              case 0xFE00  :                                  	//SW0
                if(GPIOG->ODR == 0x0001)  //LED0 ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0001)
                {
                  BEEP();
                  while (GPIOG->ODR != 0x0001)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
              break;
              
              case 0xFD00  : 	                                        //SW1
                if(GPIOG->ODR == 0x0002)  //LED1ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0002)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0002)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0002)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
              case 0xFB00  : 	                                         //SW2
                if(GPIOG->ODR == 0x0004)  //LED2ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0004)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0004)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0004)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
              case 0xF700  : 	                                         //SW3
                if(GPIOG->ODR == 0x0008)  //LED3ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0008)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0008)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0008)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
              case 0xEF00  : 	                                         //SW4
                if(GPIOG->ODR == 0x0010)  //LED4ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0010)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0010)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0010)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
              case 0xDF00  : 	                                         //SW5
                if(GPIOG->ODR == 0x0020)  //LED5ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0020)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0020)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0020)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
              case 0xBF00  : 	                                         //SW6
                if(GPIOG->ODR == 0x0040)  //LED6ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0040)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0040)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0040)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
               case 0x7F00  : 	                                         //SW7
                if(GPIOG->ODR == 0x0080)  //LED7ON
                {
                 BEEP();
                 DelayMS(200);
                 BEEP();
                }
                else if(GPIOG->ODR > 0x0080)
                {
                  BEEP();
                  while (GPIOG->ODR > 0x0080)
                  {
                   DelayMS(500);
                   GPIOG->ODR >>=1;  //비트 한칸 씩 오른쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }
                else
                {
                BEEP();
                  while (GPIOG->ODR < 0x0080)
                  {
                   DelayMS(500);
                   GPIOG->ODR <<=1;  //비트 한칸 씩 왼쪽으로 쉬프트
                  }
                  BEEP();
                  DelayMS(500);
                  BEEP();
                  DelayMS(500);
                  BEEP();
                }  
              break;
              
          }
 
          
          
          
        }
  
}

/* GPIO (GPIOG(LED), GPIOH(Switch)) 초기 설정	*/
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