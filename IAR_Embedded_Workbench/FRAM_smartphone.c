#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

void _GPIO_Init(void);
uint16_t KEY_Scan(void);

void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t S_M, B_M;

int main(void)
{
	_GPIO_Init(); 	// GPIO (LED,SW,Buzzer,Joy stick) 초기화
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);

	Fram_Init();            // FRAM 초기화 H/W 초기화
	Fram_Status_Config();   // FRAM 초기화 S/W 초기화
 
	DisplayInitScreen();    // LCD 초기화면
	BEEP();

	GPIOG->ODR &= ~0x00FF;	// LED 초기값: LED0~7 Off

	S_M= Fram_Read(525);
	B_M= Fram_Read(526);
	if (S_M == 'S')
	{	LCD_SetTextColor(RGB_RED);
		LCD_DisplayChar(0,8,S_M); 
	}
	else if (S_M == 'V')
	{	LCD_SetTextColor(RGB_RED);
		LCD_DisplayChar(0,10,S_M); 
	}
	else if (S_M == 'N')
	{	LCD_SetTextColor(RGB_RED);
		LCD_DisplayChar(0,12,S_M); 
	}
	else 
	{  	LCD_SetTextColor(RGB_RED);
		S_M= 'S';
		LCD_DisplayChar(0,8,'S'); 
	}
	if ((B_M != 'O') && (B_M != 'X'))
	{
		B_M = 'O';
	}
	LCD_SetTextColor(RGB_RED);
	LCD_DisplayChar(1,5,B_M); 

	while(1)
	{
		switch(KEY_Scan())	// 입력된 Switch 정보 분류 		
		{
        		case 0xFE00 : 	//SW0 입력
	        	break;
        		case 0xFD00 : 	//SW1 입력
			if 	(S_M=='S')
			{  	S_M= 'V';
				LCD_SetTextColor(RGB_BLUE);
				LCD_DisplayChar(0,8,'S');
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(0,10,'V');
			}
			else if (S_M=='V')	
			{	S_M= 'N';
				LCD_SetTextColor(RGB_BLUE);
				LCD_DisplayChar(0,10,'V');
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(0,12,'N');
			}  
			else
			{	S_M= 'S';
				LCD_SetTextColor(RGB_BLUE);
				LCD_DisplayChar(0,12,'N');
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(0,8,'S');
			}
			Fram_Write(525,S_M);  // FRAM(525) 50번지에 1 저장 
				
		break;
        		case 0xFB00 : 	//SW2 입력
			if 	(B_M=='O')
			{  	B_M= 'X';
			}
			else
			{  	B_M= 'O';
			}
			LCD_SetTextColor(RGB_RED);
			LCD_DisplayChar(1,5,B_M);
			Fram_Write(526,B_M);  // FRAM(0~8191) 50번지에 2 저장 
	        	break;
        		case 0xF700 : 	//SW3 입력
          		break;
        	}  // switch(KEY_Scan())
    	}  // while(1)
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joy stick)) 초기 설정	*/
void _GPIO_Init(void)
{
    // LED (GPIO G) 설정
    RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    RCC->AHB1ENR    |=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER    |=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR    |= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER 	&=~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR    &=~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}	

/* GLCD 초기화면 설정 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : Green
	LCD_SetTextColor(RGB_BLUE);	// 글자색 : Black
	                          //0123456789ABC
	LCD_DisplayText(0,0,"SOUND : S V N");  // Title
	LCD_DisplayText(1,0,"BT : O");  // subtitle
}

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
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
		DelayUS(1000);         		// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
    	for(; Dly; Dly--);
}
