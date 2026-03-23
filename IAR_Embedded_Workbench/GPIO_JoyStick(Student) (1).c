//************************************//
// GPIO(INPUT mode: Joystick) 실습
//

#include "stm32f4xx.h"
#include "GLCD.h"

#define SW0_PUSH        0xFE00  //PH8
#define SW1_PUSH        0xFD00  //PH9
#define SW2_PUSH        0xFB00  //PH10
#define SW3_PUSH        0xF700  //PH11

// NO Joy-Button : 0x03E0 : 0000 0011 1110 0000 
			//Bit No      FEDC BA98 7654 3210
#define NAVI_PUSH	0x03C0	//PI5 0000 0011 1100 0000 
#define NAVI_UP		0x03A0	//PI6 0000 0011 1010 0000 
#define NAVI_DOWN	0x0360	//PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	0x02E0	//PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0	//PI9 0000 0001 1110 0000 

void _GPIO_Init(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);

void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t	SW0_Flag, SW1_Flag;

int main(void)
{
	_GPIO_Init();		// GPIO (LED & SW) 초기화
	LCD_Init();		// LCD 모듈 초기화
	DelayMS(100);
	BEEP();

	GPIOG->ODR = 0xFF00;	// LED 초기값: LED0~7 Off
	DisplayInitScreen();	// LCD 초기화면
 
	while(1)
	{
		switch(JOY_Scan())
		{
			case 0x01E0 :	// NAVI_LEFT
				if (SW0_Flag==0) {
					GPIOG->ODR |= 0x0001;	// LED0 ON		
					LCD_DisplayText(1,0,"0.LED0 ON ");
					SW0_Flag = 1;
				}
				else {
					GPIOG->ODR &= ~0x0001;	// LED0 OFF
					LCD_DisplayText(1,0,"0.LED0 OFF");
					SW0_Flag = 0;
				}
			break;
                        
			case 0x02E0:	// NAVI_RIGHT
				if (SW1_Flag==0) {
					GPIOG->ODR |= 0x0002;	// LED1 ON
					LCD_DisplayText(2,0,"1.LED1 ON ");
					SW1_Flag = 1;
				}
				else {
					GPIOG->ODR &= ~0x0002; // LED1 OFF
					LCD_DisplayText(2,0,"1.LED1 OFF");
					SW1_Flag = 0;
				}
			break;
                        
                        case 0x03A0:	// NAVI_up
				if (SW1_Flag==0) {
					GPIOG->ODR |= 0x0004;	// LED2 ON
					LCD_DisplayText(3,0,"2.LED2 ON ");
					SW1_Flag = 1;
				}
				else {
					GPIOG->ODR &= ~0x0004; // LED2 OFF
					LCD_DisplayText(3,0,"2.LED2 OFF");
					SW1_Flag = 0;
				}
			break;
                        
                        case 0x0360:	// NAVI_down
				if (SW1_Flag==0) {
					GPIOG->ODR |= 0x0008;	// LED3 ON
					LCD_DisplayText(4,0,"3.LED3 ON ");
					SW1_Flag = 1;
				}
				else {
					GPIOG->ODR &= ~0x0008; // LED3OFF
					LCD_DisplayText(4,0,"3.LED3 OFF");
					SW1_Flag = 0;
				}
			break;
                        
                         case 0x03C0:	// NAVI_push
				if (SW1_Flag==0) {
					GPIOG->ODR |= 0x0010;	// LED4 ON
					LCD_DisplayText(5,0,"4.LED4 ON ");
					SW1_Flag = 1;
				}
				else {
					GPIOG->ODR &= ~0x0010; // LED4OFF
					LCD_DisplayText(4,0,"4.LED4 OFF");
					SW1_Flag = 0;
				}
			break;
                        
                       /* case 0x03A0:	// NAVI_up
                          GPIOG->ODR |= 0x0004;	// LED2 ON
                          break;
                            case 0x0360:	// NAVI_down
                          GPIOG->ODR |= 0x0008;	// LED3ON
                          break;
                            case 0x03C0:	// NAVI_push
                          GPIOG->ODR |= 0x0010;	// LED4ON
                          break; */
		}  // switch(JOY_Scan())
	}  // while(1)
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joystick)) 초기 설정	*/
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
   
	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
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

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_GREEN);	// 글자배경색 : Green
	LCD_SetTextColor(RGB_BLACK);	// 글자색 : Black
	LCD_DisplayText(0,0,"MENU");	// Title

	LCD_SetBackColor(RGB_YELLOW);	//글자배경색 : Yellow
          
	LCD_DisplayText(1,0,"0.LED0 Off");
	LCD_DisplayText(2,0,"1.LED1 Off");
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
