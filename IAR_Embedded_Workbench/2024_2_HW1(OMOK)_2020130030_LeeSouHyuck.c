/////////////////////////////////////////////////////////////
// 과제번호: 마이컴 응용 2024 HW1
// 과제명: 오목 게임
// 과제개요:  오목을 하고 승패를 판단하는 프로그램을 작성함
// *******************************
// 사용한 하드웨어(기능): GPIO, Joy-stick, EXTI, GLCD, Fram
// 제출일: 2024. 9. 22
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////


#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

void _GPIO_Init(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
uint16_t KEY_Scan(void);     
uint16_t JOY_Scan(void);
void DisplayInitScreen(void);
void BEEP(void);
void _EXTI_Init(void);


uint8_t   B_F=0;     //파란색 신호
uint8_t   R_F=0;     //빨간색 신호
uint8_t   R_X=5;     //빨간색 x좌표 값
uint8_t   R_Y=5;     //빨간색 Y좌표 값
uint8_t   B_X=5;     //파란색 x좌표 값
uint8_t   B_Y=5;     //파란색 Y좌표 값
uint8_t   empty=0;     // 비어있을떄 0, 
uint8_t   RED=1;     // 빨간 돌이 있을 때 1
uint8_t   BLUE=2;     // 파란돌이 있을떄 2
uint8_t   board[10][10] = {0}; // 0으로 초기화 (빈 칸)
uint8_t   x;
uint8_t   y;
uint8_t   count;
uint8_t   player;
uint16_t CheckWin(uint8_t);// 승리 판정 알고리즘
uint8_t   B_W=0;     //파란색 승리 회수
uint8_t   R_W=0;     //빨간색 승리 회수


int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
        Fram_Init();                    // FRAM 초기화 H/W 초기화
	Fram_Status_Config();     // FRAM 초기화 S/W 초기화
	DisplayInitScreen();	  // LCD 초기화면
        R_W=Fram_Read(300);   //  빨간 돌 승리 회수 불러오기
        B_W=Fram_Read(301);  //  파란 돌 승리 회수 불러오기
//노란 배경 빨간 글씨
        LCD_SetBackColor(RGB_YELLOW);        
        LCD_SetTextColor(RGB_RED);
        LCD_DisplayChar(9,8,Fram_Read(300)+0x30);
 //노란 배경 파랑 글씨
        LCD_SetBackColor(RGB_YELLOW);       
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayChar(9,11,Fram_Read(301)+0x30);


        while(1)
         {
             if(R_F ==1)         //빨간 돌 순서
                {
                 switch(JOY_Scan())
		     {
			case 0x01E0 :	// NAVI_LEFT
                                     if(R_X !=0) 
                                       {
                                         R_X= R_X-1;      //빨간색 왼쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_RED);
                                         LCD_DisplayChar(9,2, (R_X)+0x30);
                                       } 
                                     else R_X= R_X;
;

			break;
                        
			case 0x02E0:	// NAVI_RIGHT
                                     if(R_X !=9) 
                                       {
                                         R_X= R_X+1;      //빨간색 오른쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_RED);
                                         LCD_DisplayChar(9,2, (R_X)+0x30);
                                       }  
                                     else R_X= R_X;

			break;
                        
                        case 0x03A0:	// NAVI_up
                                     if(R_Y !=0) 
                                       {
                                         R_Y= R_Y-1;      //빨간색 위으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_RED);
                                         LCD_DisplayChar(9,4, (R_Y)+0x30);
                                       }  
                                     else R_Y= R_Y;

			break;
                        
                        case 0x0360:	// NAVI_down
                                     if(R_Y !=9)
                                       {
                                         R_Y= R_Y+1;      //빨간색 아래쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_RED);
                                         LCD_DisplayChar(9,4, (R_Y)+0x30);
                                       }  
                                     else R_Y= R_Y;

			break;
                        
                         case 0x03C0:	// NAVI_push
                                    if(board[R_X][R_Y] != empty) 
                                        {DelayMS(1000); BEEP(); BEEP(); BEEP();} 
                                    else 
                                         LCD_SetBrushColor(RGB_RED);    //그림 색
                                         LCD_DrawFillRect(33+8*R_X-2, 25+8*R_Y-2, 5, 5);  
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_RED);
                                         LCD_DisplayText(9,0," ");
                                         GPIOG->ODR &= ~0x0001;	// LED0 OFF
                                         BEEP();
                                         board[R_X][R_Y] = RED;  //빨간 돌 착수

                                         uint8_t winner = CheckWin(RED);     //빨간색 승리 여부 확인
                                         if (winner == RED)
                                              {
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();

                                               LCD_SetBackColor(RGB_YELLOW);        
                                               LCD_SetTextColor(RGB_RED);
                                               if(R_W != 9)
                                                   {
                                                     R_W=R_W + 1;
                                                   }
                                               else  R_W=0;                //이긴 횟수가 9보다 크면 0으로 초기화
                                               Fram_Write(300, R_W);            //Fram에  빨간 돌 승리 회수 저장
                                               LCD_DisplayChar(9, 8, (R_W)+0x30);  //승리 횟수 표시

	                                       _GPIO_Init();	// GPIO 초기화
	                                       _EXTI_Init();	       // EXTI 초기화
	                                       DisplayInitScreen();	  // LCD 초기화면
                                              uint8_t   B_F=0;   
                                              uint8_t   R_F=0;     
                                              uint8_t   R_X=5;    
                                              uint8_t   R_Y=5;     
                                              uint8_t   B_X=5;     
                                              uint8_t   B_Y=5;  
 
                                              //노란 배경 빨간 글씨
                                              LCD_SetBackColor(RGB_YELLOW);        
                                              LCD_SetTextColor(RGB_RED);
                                              LCD_DisplayChar(9,8,Fram_Read(300)+0x30);
                                              //노란 배경 파랑 글씨
                                              LCD_SetBackColor(RGB_YELLOW);       
                                              LCD_SetTextColor(RGB_BLUE);
                                              LCD_DisplayChar(9,11,Fram_Read(301)+0x30);
                                              }
                                        else
                                              {
                                               EXTI->IMR &= ~0x0100;  //EXTI8인터럽트 mask (Interrupt disble) 설정
                                               EXTI->IMR |= 0x8000;   // EXTI15인터럽트 mask (Interrupt Enable) 설정
                                               R_F=0;
                                              }
			break;
                     }   

		}  // switch(JOY_Scan())


            if(B_F ==1)             //파란 돌 순서
                {
                 switch(JOY_Scan())
		     {
			case 0x01E0 :	// NAVI_LEFT
                                     if(B_X !=0) 
                                       {
                                         B_X= B_X-1;      //파란색 왼쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_BLUE);
                                         LCD_DisplayChar(9,15, (B_X)+0x30);
                                       } 
                                     else B_X= B_X;
;

			break;
                        
			case 0x02E0:	// NAVI_RIGHT
                                     if(B_X !=9) 
                                       {
                                         B_X= B_X+1;      //파란색 오른쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_BLUE);
                                         LCD_DisplayChar(9,15, (B_X)+0x30);
                                       }  
                                     else B_X= B_X;

			break;
                        
                        case 0x03A0:	// NAVI_up
                                     if(B_Y !=0) 
                                       {
                                         B_Y= B_Y-1;      //파란색 위으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_BLUE);
                                         LCD_DisplayChar(9,17, (B_Y)+0x30);
                                       }  
                                     else B_Y= B_Y;

			break;
                        
                        case 0x0360:	// NAVI_down
                                     if(B_Y !=9)
                                       {
                                         B_Y= B_Y+1;      //파란색 아래쪽으로 한칸 이동
                                         BEEP();
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_BLUE);
                                         LCD_DisplayChar(9,17, (B_Y)+0x30);
                                       }  
                                     else B_Y= B_Y;

			break;
                        
                         case 0x03C0:	// NAVI_push
                                    if(board[B_X][B_Y] != empty) 
                                        {DelayMS(1000); BEEP(); BEEP(); BEEP();} 
                                    else 
                                         LCD_SetBrushColor(RGB_BLUE);    //그림 색
                                         LCD_DrawFillRect(33+8*B_X-2, 25+8*B_Y-2, 5, 5);  
                                         LCD_SetBackColor(RGB_YELLOW);        
                                         LCD_SetTextColor(RGB_BLUE);
                                         LCD_DisplayText(9,19," ");
                                         GPIOG->ODR &= ~0x0080;	// LED7 OFF
                                         BEEP();
                                         board[B_X][B_Y] = BLUE;   //파란 돌 착수
                                         uint8_t winner = CheckWin(BLUE);     //파란색 승리 여부 확인
                                         if (winner == BLUE)
                                              {
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               DelayMS(1000);
                                               BEEP();
                                               LCD_SetBackColor(RGB_YELLOW);        
                                               LCD_SetTextColor(RGB_BLUE);
                                               if(B_W != 9)                        
                                                   {
                                                     B_W=B_W + 1;                    //파란 돌 승리 횟수 추가
                                                   }
                                               else  B_W=0;                //이긴 횟수가 9보다 크면 0으로 초기화

                                               Fram_Write(301, B_W);            //Fram에  파란 돌 승리 회수 저장
                                               LCD_DisplayChar(9, 11, (B_W)+0x30);  //승리 횟수 표시

	                                       _GPIO_Init();	// GPIO 초기화
	                                       _EXTI_Init();	       // EXTI 초기화
	                                       DisplayInitScreen();	  // LCD 초기화면
                                               uint8_t   B_F=0;   
                                               uint8_t   R_F=0;     
                                               uint8_t   R_X=5;    
                                               uint8_t   R_Y=5;     
                                               uint8_t   B_X=5;     
                                               uint8_t   B_Y=5;   

                                              //노란 배경 빨간 글씨
                                              LCD_SetBackColor(RGB_YELLOW);        
                                              LCD_SetTextColor(RGB_RED);
                                              LCD_DisplayChar(9,8,Fram_Read(300)+0x30);
                                              //노란 배경 파랑 글씨
                                              LCD_SetBackColor(RGB_YELLOW);       
                                              LCD_SetTextColor(RGB_BLUE);
                                              LCD_DisplayChar(9,11,Fram_Read(301)+0x30);
                                              }


                                         EXTI->IMR &= ~0x8000;     // EXTI15인터럽트 mask(Interrupt disble) 설정
                                         EXTI->IMR |= 0x0100;     //EXTI8인터럽트 mask(Interrupt Enable) 설정

                                         B_F=0;
			break;
                     }   

		} 
        
            
         } 
    
}


 uint16_t CheckWin(uint8_t player)    //승패 판단 알고리즘
{
 // 가로 검사
    for (y = 0; y < 10; y++) 
        {
        count = 0;
        for (x = 0; x < 10; x++) 
            {
            if (board[x][y] == player)
               {
                  count++;
                  if (count == 5) return player; // 5개 연속
               } 
            else {count = 0;}
            }
         }

 // 세로 검사
    for ( x = 0; x < 10; x++) 
        {
        count = 0;
        for (y = 0; y < 10; y++) 
            {
            if (board[x][y] == player) 
                {
                count++;
                if (count == 5) return player; // 5개 연속
                } 
            else {count = 0;}
            }
       }

// 대각선 검사 (왼쪽 위에서 오른쪽 아래로)
    for (x = 0; x < 10; x++) 
       {
        for ( y = 0; y < 10; y++) 
            {
            if (x <= 5 && y <= 5) // 대각선 아래쪽
               { 
                count = 0;
                for (uint8_t i = 0; i < 5; i++)
                   {
                    if (board[x+i][y+i] == player) 
                        {
                        count++;
                        if (count == 5) return player; // 5개 연속
                        }
                   }
               }
           }
       }

    // 대각선 검사 (오른쪽 위에서 왼쪽 아래로)
    for (x = 0; x < 10; x++) 
        {
        for (y = 0; y < 10; y++) 
           {
            if (x >= 4 && y <= 5)  // 대각선 위쪽
               { 
                count = 0;
                for (uint8_t i = 0; i < 5; i++) 
                    {
                    if (board[x-i][y+i] == player) 
                        {
                        count++;
                        if (count == 5) return player; // 5개 연속
                        }
                    }
              }
          }
      }

    return 0; // 승자가 없으면 0 반환

}




/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
//노란 배경 검은 글씨
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색
	LCD_SetTextColor(RGB_BLACK);	// 글자색
        LCD_SetPenColor(RGB_BLACK);    //그림 색

	LCD_DisplayText(0,0,"Mecha-OMOK(LSH)");  	//제목
        LCD_DisplayText(1,3,"0     5    9");
        LCD_DisplayText(5,3,"5");
        LCD_DisplayText(8,3,"9");
        LCD_DisplayText(9,9,"vs");

        LCD_DrawRectangle(33, 25, 80, 80);   // 오목판 최외각 

        LCD_DrawRectangle(41, 25, 8, 80);   // 오목판 세로줄
        LCD_DrawRectangle(49, 25, 8, 80);
        LCD_DrawRectangle(57, 25, 8, 80);
        LCD_DrawRectangle(65, 25, 8, 80);
        LCD_DrawRectangle(73, 25, 8, 80);
        LCD_DrawRectangle(81, 25, 8, 80);
        LCD_DrawRectangle(89, 25, 8, 80);
        LCD_DrawRectangle(97, 25, 8, 80);

        LCD_DrawRectangle(33, 33, 80, 8);   //오목판 가로줄 
        LCD_DrawRectangle(33, 41, 80, 8);
        LCD_DrawRectangle(33, 49, 80, 8);
        LCD_DrawRectangle(33, 57, 80, 8);
        LCD_DrawRectangle(33, 65, 80, 8);
        LCD_DrawRectangle(33, 73, 80, 8);
        LCD_DrawRectangle(33, 81, 80, 8);
        LCD_DrawRectangle(33, 89, 80, 8);

        LCD_DrawFillRect(33+8*5-1, 25+8*5-1, 3, 3);   // (5,5)지점




//노란 배경 빨간 글씨
        LCD_SetBackColor(RGB_YELLOW);        
        LCD_SetTextColor(RGB_RED);
        LCD_DisplayText(9,1,"(5,5)");
        LCD_DisplayText(9,8,"0");

 //노란 배경 파랑 글씨
        LCD_SetBackColor(RGB_YELLOW);       
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(9,14,"(5,5)");
        LCD_DisplayText(9,11,"0");

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



/* EXTI  초기 설정  */        
void _EXTI_Init(void)
{  

	RCC->AHB1ENR 	|= 0x80;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x4000;	// Enable System Configuration Controller Clock
	
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)	
        GPIOI->MODER	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)  (5~9번 조이스틱 관련)
 

        SYSCFG->EXTICR[2] &= ~0xFFFF;    //EXTI 8, 9, 10, 11에 대한 초기화                         
	SYSCFG->EXTICR[2] |= 0x0007;	// EXTI 8에 대한 소스 입력은 GPIOH로 설정,          

        SYSCFG->EXTICR[3] &= ~0xFFFF;    //EXTI I2, 13, 14, 15에 대한 초기화         
	SYSCFG->EXTICR[3] |= 0x7000;	// EXTI15에 대한 소스 입력은 GPIOH로 설정                




	EXTI->FTSR |= 0x0100;		// EXTI8: Falling Trigger Enable 
  	EXTI->FTSR |= 0x8000;		// EXTI 15: Falling Trigger Enable         
            

        EXTI->IMR  |= 0x0100;		// EXTI8인터럽트 mask (Interrupt Enable) 설정
	

	NVIC->ISER[0] |= ( 1<<23  );	// Enable 'Global Interrupt EXTI8,9'
					// Vector table Position 참조
        NVIC->ISER[1] |= ( 1<<(40-32)  );   //40번       

    
}


/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
    if(EXTI->PR & 0x0100)			// EXTI8 Interrupt Pending(발생)
     {

        GPIOG->ODR = 0x0001;	//  LED0 ON
        BEEP();
        DelayMS(100);
        R_F=1;

        EXTI->PR = 0x0100;  // EXTI8 인터럽트 플래그 클리어

        LCD_SetBackColor(RGB_YELLOW);        
        LCD_SetTextColor(RGB_RED);
        LCD_DisplayText(9,0,"*");
     }
}

void EXTI15_10_IRQHandler(void)                
{
    if(EXTI->PR & 0x8000)			// EXTI15 Interrupt Pending(발생)
     {

        GPIOG->ODR = 0x0080;	//  LED7 ON
        BEEP();
        DelayMS(100);
        B_F=1;

        EXTI->PR = 0x8000;  // EXTI15 인터럽트 플래그 클리어

        LCD_SetBackColor(RGB_YELLOW);        
        LCD_SetTextColor(RGB_BLUE);
        LCD_DisplayText(9,19,"*");
     }
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