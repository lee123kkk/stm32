/////////////////////////////////////////////////////////////
// 과제명: HW4. 커피 자동판매기
// 과제개요:  커피를 자동으로 제조 및 판매하는 커피자판기 제어 프로그램을 작성함
// *******************************
// 사용한 하드웨어(기능): GPIO, Joy-stick, EXTI, GLCD ...
// 제출일: 2024. 6. 15
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

uint8_t   Coin = 0; // 변수선언 : 동전 
uint8_t   TOT;// 자판기 수입 
uint8_t   TOT1;// 자판기 수입 10의 자리 수를 저장
uint8_t   TOT2;// 자판기 수입 100의 자리 수를 저장
uint8_t   Choice = 0; //커피 선택을 저장
uint8_t   ChoiceB = 0; //블랙 커피 선택 수
uint8_t   ChoiceS = 0; //슈가 커피 선택 수
uint8_t   ChoiceM = 0; //믹스 커피 선택 수
uint8_t   Cup = 9; //컵 재고
uint8_t   Sugar = 5; //설탕 재고
uint8_t   Milk = 5; //우유 재고
uint8_t   Coffee = 9; //커피 재고
uint8_t   NoC = 0; //커피 판매수
uint8_t   NoC1 = 0; //커피 판매수 10의 자리
uint8_t   NoC2 = 0; //커피 판매수 1의 자리
uint8_t   W_F=0;     //커피 제조 표시 신호(1일때 제조중)
uint8_t   R_F=0;     //제고 표시 신호(1일때  제고가 0개인 품목이 있음, 리필 필요) 

int main(void)
{
	LCD_Init();	// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화
        Fram_Init();                    // FRAM 초기화 H/W 초기화
	Fram_Status_Config();   // FRAM 초기화 S/W 초기화
	DisplayInitScreen();	// LCD 초기화면

        //Fram 에 저장되어 있는 NoC, ToT, Coin값 불러오기 
        NoC1 = Fram_Read(50);    
        NoC2 =Fram_Read(51);;
        TOT1 = Fram_Read(70);
        TOT2 = Fram_Read(71);

        NoC = NoC1 * 10 + NoC2;                     
        TOT = TOT1 * 100 + TOT2 * 10;
        Coin = (Fram_Read(60) * 100) + (Fram_Read(61) * 10);



        //노란 배경 검은 글씨
         LCD_SetBackColor(RGB_YELLOW);	
	 LCD_SetTextColor(RGB_BLACK);	
         LCD_DisplayChar(8,17,Fram_Read(50)+0x30);	//FRAM 50번지 저장된 data(1byte) 읽어 LCD에 표시      
         LCD_DisplayChar(8,18,Fram_Read(51)+0x30);
         //검은 배경 노란 글씨
         LCD_SetBackColor(RGB_BLACK);       
         LCD_SetTextColor(RGB_YELLOW);
         LCD_DisplayChar(1,16,Fram_Read(60)+0x30);
         LCD_DisplayChar(1,17,Fram_Read(61)+0x30);
         LCD_DisplayChar(3,16,Fram_Read(70)+0x30);
         LCD_DisplayChar(3,17,Fram_Read(71)+0x30);

       while(1)
       {
          if(Cup==0 || Coffee==0 || Sugar ==0 || Milk ==0) //제고가 0인 물품이 있을때 리필 필요 표시
         {
                R_F=1;
                LCD_SetBrushColor(RGB_RED);     //빨간색 색칙 사각형
                LCD_DrawFillRect(140, 65, 9, 9);
                EXTI->IMR  |= 0x1000;		// EXTI12인터럽트 mask (Interrupt Enable) 설정
                
         }  
          else R_F=0;

          if(W_F==0)                         //커피 제조중일때 커피 추가 선택 불가
         {
             switch(JOY_Scan() )      //조이스틱 입력
            {
                   case 0x01E0 :	 // NAVI_LEFT    블랙커피 주문  10원
                          BEEP();
                          LCD_SetBackColor(RGB_BLUE);        
                          LCD_SetTextColor(RGB_RED);
                          LCD_DisplayText(2,1,"B");
                          Choice = Choice +1;
                          ChoiceB = ChoiceB +1;
                   break;
            

                   case 0x03A0:	 // NAVI_up       슈가커피 주문 20원
                          BEEP();
                          LCD_SetBackColor(RGB_BLUE);        
                          LCD_SetTextColor(RGB_RED);
                          LCD_DisplayText(2,3,"S");
                          Choice = Choice +1;
                          ChoiceS = ChoiceS +1;                      
	           break;


                   case 0x02E0 :	 // NAVI_RIGHT  믹스 커피 주문 30원
                          BEEP();
                          LCD_SetBackColor(RGB_BLUE);        
                          LCD_SetTextColor(RGB_RED);
                          LCD_DisplayText(2,5,"M");
                          Choice = Choice +1;
                          ChoiceM = ChoiceM +1;                        
                   break;
            }
         }
            switch(KEY_Scan())        //스위치 입력
            {
                   case 0xFB00 :                                         //SW2    커피제조 시작
                          if(Coin >= (ChoiceB * 10) + (ChoiceS * 20) + (ChoiceM * 30) )   //코인 입력이 커피 가격보다 많을때 
                          {
                                  if((Cup >= Choice) && (Coffee >= Choice) &&  (Sugar >= (ChoiceS + ChoiceM)) && (Milk >= ChoiceM)) //필요한 재료가 전부 있을때 동작 가능
                                  {
                                         if(Choice != 0)        //커피가 선택되었을 때 
                                         {
                                               W_F = 1;                     //제조 시작
                                               BEEP();
                                               GPIOG->ODR |= 0x00FF; //LED전부 켜기

                                               LCD_SetBackColor(RGB_RED);       //빨간 배경 흰 글씨
                                               LCD_SetTextColor(RGB_WHITE);
                                               LCD_DisplayText(3,3,"0");
                                               DelayMS(1000);
                                               LCD_DisplayText(3,3,"1");
                                               DelayMS(1000);
                                               LCD_DisplayText(3,3,"2");
                                               DelayMS(1000);
                                               LCD_DisplayText(3,3,"W");

                                               GPIOG->ODR &= ~0x00FF; //LED전부 끄기
                                               BEEP();
                                               DelayMS(500);
                                               BEEP();
                                               DelayMS(500);
                                               BEEP();
                                               DelayMS(500);

                                               Cup = Cup - Choice;                             //컵 재고 -1
                                               Coffee = Coffee - Choice;                      //커피 재고 -1
                                               Sugar = Sugar - ChoiceS - ChoiceM;  //설탕 재고 -1
                                               Milk = Milk - ChoiceM;                          //우유 재고 -1

                                               NoC = NoC + Choice;
                                               NoC1 = NoC/10;                                 //10의 자리 숫자
                                               NoC2 = (NoC%10);                            //1의 자리 숫자
                                               Fram_Write(50, NoC1);             //Fram 50번지에 NoC1 값 저장
                                               Fram_Write(51, NoC2);             //Fram 50번지에 NoC2 값 저장
                                               //노란 배경 검은 글씨
                                               LCD_SetBackColor(RGB_YELLOW);	
	                                       LCD_SetTextColor(RGB_BLACK);	
                                               LCD_DisplayChar(8,17,Fram_Read(50)+0x30);	//FRAM 50번지 저장된 data(1byte) 읽어 LCD에 표시                                              NoC2 = (NoC%10)/10;                        //1의 자리 숫자
                                               LCD_DisplayChar(8,18,Fram_Read(51)+0x30);

                                               TOT = TOT + (ChoiceB * 10) + (ChoiceS * 20) + (ChoiceM * 30);   //자판기 총수입

                                               Coin = Coin - TOT;                                                 //커피 가격 만큼 입력 액수 감소
                                               Fram_Write(60, Coin/100);                   //Fram60번지에 Coin의 100의 자리 값 저장
                                               Fram_Write(61, (Coin%100)/10);         //Fram61번지에 Coin의 10의 자리 값 저장
                                               //검은 배경 노란 글씨
                                               LCD_SetBackColor(RGB_BLACK);       
                                               LCD_SetTextColor(RGB_YELLOW);
                                               LCD_DisplayChar(1,16,Fram_Read(60)+0x30);
                                               LCD_DisplayChar(1,17,Fram_Read(61)+0x30);

                                               TOT1 = TOT/100;
                                               TOT2 = (TOT%10)/10;
                                               Fram_Write(70, TOT1);                        //Fram70번지에 TOT의 100의 자리 값 저장
                                               Fram_Write(71, TOT2);                        //Fram71번지에 TOT의 10의 자리 값 저장
                                               LCD_DisplayChar(3,16,Fram_Read(70)+0x30);
                                               LCD_DisplayChar(3,17,Fram_Read(71)+0x30);

                                               //파란 배경 흰 글씨
                                               LCD_SetBackColor(RGB_BLUE);        
                                               LCD_SetTextColor(RGB_WHITE);
                                               LCD_DisplayText(2,1,"B");
                                               LCD_DisplayText(2,3,"S");
                                               LCD_DisplayText(2,5,"M");

                                               LCD_SetFont(&Gulim10);                   // 글시체 크기 10
                                               LCD_SetBackColor(RGB_WHITE);     //흰 배경 검음 글씨
                                               LCD_SetTextColor(RGB_BLACK);
                                               LCD_DisplayChar(3, 1, (9-Choice)+0x30);
                                               LCD_DisplayChar(3, 4, (5-ChoiceS-ChoiceM)+0x30);
                                               LCD_DisplayChar(3, 7, (5-ChoiceM)+0x30);
                                               LCD_DisplayChar(3, 10, (9-Choice)+0x30);
                                               LCD_SetFont(&Gulim8);                   // 글시체 크기 8

                                               Choice = 0;
                                               ChoiceB =0;
                                               ChoiceS =0;
                                               ChoiceM =0;
                                               
                                               W_F = 0;  //제조 종료
                                        }   
                                  }
                            }
                   break;
            }

       }  
}



/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
//노란 배경 검은 글씨
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색
	LCD_SetTextColor(RGB_BLACK);	// 글자색
	LCD_DisplayText(0,0,"LSH coffee");  	//제목
        LCD_DisplayText(8,17," 0");
//흰 배경 검은 글씨
        LCD_SetBackColor(RGB_WHITE);   
        LCD_DisplayText(0,16,"IN");
        LCD_DisplayText(0,12,"\\10");
        LCD_DisplayText(2,16,"TOT");
        LCD_DisplayText(2,12,"\\50");
        LCD_DisplayText(5,1,"cp sg mk cf");
        LCD_DisplayText(5,15,"RF");
        LCD_DisplayText(7,16,"NoC");
//검은 배경 노란 글씨
        LCD_SetBackColor(RGB_BLACK);       
        LCD_SetTextColor(RGB_YELLOW);
        LCD_DisplayText(1,16,"000");
        LCD_DisplayText(3,16,"000");   

        LCD_SetPenColor(RGB_GREEN);            //초록색 사각형
        LCD_DrawRectangle(104, 14, 9, 9);    
        LCD_DrawRectangle(104, 40, 9, 9);       

        LCD_SetBrushColor(RGB_GRAY);         //회색 색칠 사각형
        LCD_DrawFillRect(104, 14, 9, 9);
        LCD_DrawFillRect(104, 40, 9, 9);

        LCD_SetBrushColor(RGB_GREEN);     //초록색 색칙 사각형
        LCD_DrawFillRect(140, 65, 9, 9);
//파란 배경 흰 글씨
        LCD_SetBackColor(RGB_BLUE);        
        LCD_SetTextColor(RGB_WHITE);
        LCD_DisplayText(2,1,"B");
        LCD_DisplayText(2,3,"S");
        LCD_DisplayText(2,5,"M");
 //빨간 배경 흰 글씨
        LCD_SetBackColor(RGB_RED);       
        LCD_SetTextColor(RGB_WHITE);
        LCD_DisplayText(3,3,"W");

        LCD_SetFont(&Gulim10);                   // 글시체 크기 10
        LCD_SetBackColor(RGB_WHITE);
        LCD_SetTextColor(RGB_BLACK);
        LCD_DisplayText(3,1,"9  5  5  9");
        LCD_SetFont(&Gulim8);                   // 글시체 크기 8

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
	SYSCFG->EXTICR[2] |= 0x7077;	// EXTI 8, 9, 11에 대한 소스 입력은 GPIOH로 설정,          

        SYSCFG->EXTICR[3] &= ~0xFFFF;    //EXTI I2, 13, 14, 15에 대한 초기화         
	SYSCFG->EXTICR[3] |= 0x0077;	// EXTI12, 13에 대한 소스 입력은 GPIOH로 설정                




	EXTI->FTSR |= 0x0100;		// EXTI8: Falling Trigger Enable 
	EXTI->FTSR |= 0x0200;		// EXTI 9 Falling Trigger Enable 
  	
    
        EXTI->FTSR |= 0x0800;		// EXTI 11: Falling Trigger Enable                 
        EXTI->FTSR |= 0x1000;		// EXTI 12: Falling Trigger Enable             
        EXTI->FTSR |= 0x2000;		// EXTI 13: Falling Trigger Enable          
     

        EXTI->IMR  |= 0x2B00;		// EXTI8, 9, 11,13인터럽트 mask (Interrupt Enable) 설정
	

	NVIC->ISER[0] |= ( 1<<23  );	// Enable 'Global Interrupt EXTI8,9'
					// Vector table Position 참조
        NVIC->ISER[1] |= ( 1<<(40-32)  );   //40번              
}



/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
   if((W_F==0) &&(R_F==0))                                             //제조 중이거나 재고가 없을때  추가 동전 투입 불가능
  {
         if(EXTI->PR & 0x0100)			// EXTI8 Interrupt Pending(발생)     10원 투입
	{
		EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
                Coin = Coin + 10;
                
                LCD_SetBrushColor(RGB_YELLOW);         
                LCD_DrawFillRect(104, 14, 9, 9);
                LCD_SetBackColor(RGB_BLACK);	// 검은 배경
	        LCD_SetTextColor(RGB_YELLOW);	// 노란 글씨
 
                if(Coin>=200)                                                     //200원
                {
                 Coin = 200;
                 LCD_DisplayText(1,16,"200");   
                }

                 else if(Coin==200)                                         //200원
                {
                 LCD_DisplayText(1,16,"200");   
                }

                else if(Coin>=100)                                            //100~190원
                {

                 LCD_DisplayChar(1, 16, (Coin/100)+0x30);             //투입한 돈의 100의 자리
                 LCD_DisplayChar(1, 17, ((Coin%100)/10)+0x30);   //투입한 돈의 10의 자리
                 LCD_DisplayChar(1, 18, 0+0x30);                          //투입한 돈의 1의 자리 
                }

                else if(Coin>=10)
                {
                 LCD_DisplayChar(1, 16, 0+0x30);                         //10~90원
                 LCD_DisplayChar(1, 17, (Coin/10)+0x30);
                }

                Fram_Write(60, Coin/100);                   //Fram60번지에 Coin의 100의 자리 값 저장
                Fram_Write(61, (Coin%100)/10);         //Fram61번지에 Coin의 10의 자리 값 저장

                BEEP();
                DelayMS(1000);

                LCD_SetBrushColor(RGB_GRAY);         
                LCD_DrawFillRect(104, 14, 9, 9);
	}

        if(EXTI->PR & 0x0200)			// EXTI 9 Interrupt Pending(발생)   50원 투입
        {
                EXTI->PR |= 0x0200;
                Coin = Coin + 50;

                LCD_SetBrushColor(RGB_YELLOW);         
                LCD_DrawFillRect(104, 40, 9, 9);

                LCD_SetBackColor(RGB_BLACK);	// 검은 배경
	        LCD_SetTextColor(RGB_YELLOW);	// 노란 글씨

                 if(Coin>=200)                                                     //200원
                {
                 Coin = 200;
                 LCD_DisplayText(1,16,"200");   
                }

                else if(Coin>=100)                                            //100~190원
                {

                 LCD_DisplayChar(1, 16, (Coin/100)+0x30);            //투입한 돈의 100의 자리
                 LCD_DisplayChar(1, 17, ((Coin%100)/10)+0x30);    //투입한 돈의 10의 자리
                 LCD_DisplayChar(1, 18, 0+0x30);                            //투입한 돈의 1의 자리
                }

                else if(Coin>=50)
                {
                 LCD_DisplayChar(1, 16, 0+0x30);                         //10~90원
                 LCD_DisplayChar(1, 17, (Coin/10)+0x30);
                }             


                BEEP();
                DelayMS(1000);

                LCD_SetBrushColor(RGB_GRAY);         
                LCD_DrawFillRect(104, 40, 9, 9);
        }
  }
}

void EXTI15_10_IRQHandler(void)                
{

     if(W_F==0)        //제조 중일 때 동작 불가
    {
         if(EXTI->PR & 0x0800)			// EXTI 11 Interrupt Pending(발생)   SW3 잔돈반환
        {

               EXTI->PR |= 0x0800;
               Coin=0;
               Fram_Write(60, 0);                   //Fram60번지에 Coin의 100의 자리 값 저장
               Fram_Write(61, 0);                 //Fram61번지에 Coin의 10의 자리 값 저장
               //검은 배경 노란 글씨
               LCD_SetBackColor(RGB_BLACK);       
               LCD_SetTextColor(RGB_YELLOW);
               LCD_DisplayChar(1,16,Fram_Read(60)+0x30);
               LCD_DisplayChar(1,17,Fram_Read(61)+0x30);
        }


         if(EXTI->PR & 0x2000)			// EXTI 13 Interrupt Pending(발생)   SW5 리셋
        {
              NoC = 0;
              NoC1 = 0;
              NoC2 = 0;
              Fram_Write(50, NoC1);             //Fram 50번지에 NoC1 값 저장
              Fram_Write(51, NoC2);             //Fram 50번지에 NoC2 값 저장
              //노란 배경 검은 글씨
              LCD_SetBackColor(RGB_YELLOW);	
	      LCD_SetTextColor(RGB_BLACK);	
              LCD_DisplayChar(8,17,Fram_Read(50)+0x30);	//FRAM 50번지 저장된 data(1byte) 읽어 LCD에 표시                                              NoC2 = (NoC%10)/10;                        //1의 자리 숫자
              LCD_DisplayChar(8,18,Fram_Read(51)+0x30);

              TOT = 0;
              TOT1 = 0;
              TOT2 = 0;
              Fram_Write(70, TOT1);             //Fram70번지에 TOT의 100의 자리 값 저장
              Fram_Write(71, TOT2);             //Fram71번지에 TOT의 10의 자리 값 저장
              //검은 배경 노란 글씨
              LCD_SetBackColor(RGB_BLACK);       
              LCD_SetTextColor(RGB_YELLOW);
              LCD_DisplayChar(3,16,Fram_Read(70)+0x30);
              LCD_DisplayChar(3,17,Fram_Read(71)+0x30);
        }
    }

    if(R_F==1) //재고 부족 
   {
         if(EXTI->PR & 0x1000)			// EXTI 12 Interrupt Pending(발생)   SW4 리필
        {
              LCD_SetFont(&Gulim10);                   // 글시체 크기 10
              LCD_SetBackColor(RGB_WHITE);
              LCD_SetTextColor(RGB_BLACK);
              if(Cup==0) 
             {    
                   Cup=9;
                   LCD_DisplayText(3,1,"9");
             }
              if(Coffee==0) 
             {
                   Coffee=9;
                   LCD_DisplayText(3,10,"9");
             }
              if(Sugar==0) 
             {
                   Sugar=5;
                   LCD_DisplayText(3,4,"5");
             }
              if(Milk==0) 
             {
                   Milk=5;
                   LCD_DisplayText(3,7,"5");
             }

              LCD_SetFont(&Gulim8);                   // 글시체 크기 8

              BEEP();
              DelayMS(500);
              BEEP();

              LCD_SetBrushColor(RGB_GREEN);     //초록색 색칙 사각형
              LCD_DrawFillRect(140, 65, 9, 9);

              EXTI->IMR &= ~0x1000;  //EXTI12,인터럽트 mask (Interrupt disble) 설정
              R_F=0;  //재고 있음
        }
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