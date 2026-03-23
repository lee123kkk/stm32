/////////////////////////////////////////////////////////////
// 과제번호: 마이컴응용 2024 HW4
// 과제명: 오목게임임(USART 이용)
// 과제개요: 10X10 바둑판에서 적돌과 청돌을 사용하여 동작하는 오목게임을 제작
//         바둑돌 착돌 좌표위치는 PC 또는 조이스틱으로 선택하고, 승패결과를
//        기록하고 FRAM에 승패결과를 저장
// 사용한 하드웨어(기능): GPIO(SW, LED, Joy-stick, Buzzer, GLCD), EXTI, Fram, USART
// 제출일: 2024. 11. 24.
// 제출자 클래스: 수요일반
// 학번: 2020130030
// 이름: 이수혁
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

#define NAVI_PUSH	0x03C0  //PI5 0000 0011 1100 0000 
#define NAVI_UP		0x03A0  //PI6 0000 0011 1010 0000 
#define NAVI_DOWN       0x0360  //PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	0x02E0  //PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0  //PI9 0000 0001 1110 0000 

void _GPIO_Init(void);
void _EXTI_Init(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);

void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void RSTMove(void);     // 적돌 이동
void BSTMove(void);     // 청돌 이동
void RSTPUT(void);      // 적돌 착돌
void BSTPUT(void);      // 청돌 착돌
void CheckHor(void);        // 가로 검사
void CheckVer(void);        // 세로 검사
void CheckDia(void);        // 대각선 검사
void RWin(void);        // 적돌 승리
void BWin(void);        // 청돌 승리
void DisplayScore(void);        // 스코어 출력
void ResetGame(void);       // 게임 리셋
void Winner(void);      // 승자 판별
void USART1_Init(void);  //USART1 초기화 함수
void USART1_SendChar(char ch);  
void USART1_SendString(char* str);


int RST_Flag = 0; // 적돌 선택 Flag
int BST_Flag = 0; // 청돌 선택 Flag
int RSTPUT_Flag = 0; // 적돌 착돌 Flag
int BSTPUT_Flag = 0; // 청돌 착돌 Flag
int RX = 5; // 적돌 x 좌표
int RY = 5; // 적돌 y 좌표
int BX = 5; // 청돌 x 좌표
int BY = 5; // 청돌 y 좌표
int RSX = 67; // 적돌 x 좌표값
int RSY = 67; // 적돌 y 좌표값
int BSX = 67; // 청돌 x 좌표값
int BSY = 67; // 청돌 y 좌표값
int stadium[10][10] = { 0 }; // 바둑판 배열
int turn_complete = 0; // 턴 종료 Flag
int winner = 0; // 승패 Flag
int Rscore; // 적돌 점수
int Bscore; // 청돌 점수

int main(void)
{
    _GPIO_Init(); 	// GPIO (LED,SW,Buzzer,Joy stick) 초기화
    LCD_Init();	// LCD 모듈 초기화
    DelayMS(10);
    Fram_Init();                    // FRAM 초기화 H/W 초기화
    Fram_Status_Config();   // FRAM 초기화 S/W 초기화
    _EXTI_Init();	// EXTI 초기화
    DisplayInitScreen();    // LCD 초기화면
    USART1_Init();  // USART1 초기화 추가
    GPIOG->ODR &= ~0x00FF;	// LED 초기값: LED0~7 Off

    Rscore = Fram_Read(300);  // FRAM에서 점수 읽기
    Bscore = Fram_Read(301);  // FRAM에서 점수 읽기
    DisplayScore(); // 점수 표시

	while (1)
	{
            switch(JOY_Scan())	// 입력된 Joystick 정보 분류
		{
                case NAVI_UP : 	// Joystick UP
                    if(RST_Flag == 1){      // 적돌 선택 플래그가 1일 경우
                        BEEP();
                        if(RY > 0){     // 적돌 y좌표가 0보다 클 경우
                            RY -= 1;    // 적돌 y좌표 -1
                            RSY -= 8;   // 적돌 y좌표값 -8
                        }
                        else {      // 적돌 y좌표가 0이하일 경우
                            RY = 0;     // 적돌 y좌표 0
                            RSY = 28;   // 적돌 y좌표값 28
                        }
                        RSTMove();      // 적돌 이동
                    }
                    if(BST_Flag == 1){      // 청돌 선택 플래그가 1일 경우
                        BEEP();
                        if(BY > 0){     // 청돌 y좌표가 0보다 클 경우
                            BY -= 1;        // 청돌 y좌표 -1
                            BSY -= 8;       // 청돌 y좌표값 -8
                        }     
                        else {      // 청돌 y좌표가 0이하일 경우
                            BY = 0;     // 청돌 y좌표 0
                            BSY = 28;       // 청돌 y좌표값 28
                        }
                        BSTMove();      // 청돌 이동
                    }
	        	break;
                case NAVI_DOWN:	// Joystick DOWN
                    if(RST_Flag == 1){      // 적돌 선택 플래그가 1일 경우
                        BEEP();
                        if(RY < 9){     // 적돌 y좌표가 9보다 작을 경우
                            RY += 1;        // 적돌 y좌표 +1
                            RSY += 8;       // 적돌 y좌표값 +8
                        }
                        else {      // 적돌 y좌표가 9이상일 경우
                            RY = 9;     // 적돌 y좌표 9
                            RSY = 100;      // 적돌 y좌표값 100
                        }
                        RSTMove();       // 적돌 이동                                                  
                    }
                    if(BST_Flag == 1){      // 청돌 선택 플래그가 1일 경우
                        BEEP();
                        if(BY < 9){     // 청돌 y좌표가 9보다 작을 경우
                            BY += 1;         // 청돌 y좌표 +1
                            BSY += 8;        // 청돌 y좌표값 +8   
                        }     
                        else {      // 청돌 y좌표가 9보다 클 경우
                            BY = 9;     // 청돌 y좌표 9
                            BSY = 100;      // 청돌 y좌표값 100
                        }
                        BSTMove();      // 청돌 이동
                    }
          		break;
                case NAVI_RIGHT:	// Joystick RIGHT	
                    if (RST_Flag == 1) {        // 적돌 선택 플래그가 1일 경우
                        BEEP();
                        if (RX < 9) {       // 적돌 X좌표가 9보다 작을 경우       
                            RX += 1;        // 적돌 X좌표 +1
                            RSX += 8;       // 적돌 X좌표값 +8
                        }
                        else {      
                            RX = 9;// 적돌 X좌표가 9이상 일 경우
                            RSX = 100;      // 적돌 X좌표값 100
                        }
                        RSTMove();        // 적돌 이동  
                    }
                    if (BST_Flag == 1) {        // 청돌 선택 플래그가 1일 경우
                        BEEP();
                        if (BX < 9) {       // 청돌 x좌표가 9보다 작을 경우
                            BX += 1;        // 청돌 x좌표 +1
                            BSX += 8;        // 청돌 x좌표값 +8  
                        }
                        else {              
                            BX = 9;      // 청돌 y좌표 9
                            BSX = 100;      // 청돌 y좌표값 100
                        }
                        BSTMove();      // 청돌 이동
                    }
          		break;
                case NAVI_LEFT:	// Joystick LEFT	
                    if (RST_Flag == 1) {        // 적돌 선택 플래그가 1일 경우
                        BEEP();
                        if (RX > 0) {          // 적돌 x좌표가 0보다 클 경우
                            RX -= 1;        // 적돌 x좌표 -1
                            RSX -= 8;       // 적돌 x좌표값 -8
                        }
                        else {      // 적돌 x좌표가 0이하일 경우
                            RX = 0;     // 적돌 x좌표 0
                            RSX = 28;       // 적돌 x좌표값 28
                        }   
                        RSTMove();      // 적돌 이동 
                    }
                    if (BST_Flag == 1) {        // 청돌 선택 플래그가 1일 경우
                        BEEP();
                        if (BX > 0) {       // 청돌 x좌표가 0보다 클 경우
                            BX -= 1;        // 청돌 x좌표 -1
                            BSX -= 8;        // 청돌 x좌표값 -8
                        }
                        else {      // 청돌 x좌표가 0이하일 경우
                            BX = 0;      // 청돌 x좌표 0
                            BSX = 28;       // 청돌 x좌표값 28
                        }
                        BSTMove();      // 청돌 이동
                    }
          		break;
        }  // switch(JOY_Scan())

    }
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joy stick)) 초기 설정	*/
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

	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR |= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER &= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}

/* GLCD 초기화면 설정 */
void DisplayInitScreen(void)
{
	 LCD_Clear(RGB_YELLOW);		// 화면 클리어 : 노란색
	 LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8

     // Mecha-OMOK(PJS)
	 LCD_SetBackColor(RGB_YELLOW);		
	 LCD_SetTextColor(RGB_BLACK);	
	 LCD_DrawText(4, 4, "Mecha-OMOK(LSH)");  // 
	 // OMOK stadium
     LCD_SetPenColor(RGB_BLACK);			
     LCD_DrawHorLine(30,30,72);
     LCD_DrawHorLine(30,38,72);
     LCD_DrawHorLine(30,46,72);
     LCD_DrawHorLine(30,54,72);
     LCD_DrawHorLine(30,62,72);
     LCD_DrawHorLine(30,70,72);
     LCD_DrawHorLine(30,78,72);
     LCD_DrawHorLine(30,86,72);
     LCD_DrawHorLine(30,94,72);
     LCD_DrawHorLine(30,102,72);
     LCD_DrawVerLine(30,30,72);
     LCD_DrawVerLine(38,30,72);
     LCD_DrawVerLine(46,30,72);
     LCD_DrawVerLine(54,30,72);
     LCD_DrawVerLine(62,30,72);
     LCD_DrawVerLine(70,30,72);
     LCD_DrawVerLine(78,30,72);
     LCD_DrawVerLine(86,30,72);
     LCD_DrawVerLine(94,30,72);
     LCD_DrawVerLine(102,30,72);

     // node
     LCD_SetBrushColor(RGB_BLACK); 
     LCD_DrawFillRect(68,68,5,5);

     // Stadium Coordinates
     LCD_SetFont(&Gulim7);		
	 LCD_DrawText(22, 18, "0");  
	 LCD_DrawText(68, 18, "5");  
     LCD_DrawText(102, 18, "9");  
     LCD_DrawText(22, 66, "5");  
	 LCD_DrawText(22, 100, "9");  

	 // OMOK Red Stone Coordinate
     LCD_SetFont(&Gulim7);		
	 LCD_SetTextColor(RGB_RED);	
	 LCD_DrawText(10, 115, "(");  
	 LCD_DrawText(16, 115, "5");  
	 LCD_DrawText(22, 115, ",");  
	 LCD_DrawText(28, 115, "5");         
	 LCD_DrawText(34, 115, ")");  

     // OMOK Blue Stone Coordinate
     LCD_SetFont(&Gulim7);		
	 LCD_SetTextColor(RGB_BLUE);	
	 LCD_DrawText(116, 115, "(");  
	 LCD_DrawText(122, 115, "5");  
	 LCD_DrawText(128, 115, ",");  
	 LCD_DrawText(134, 115, "5");          
	 LCD_DrawText(140, 115, ")");  

	 //OMOK Score
     LCD_SetFont(&Gulim7);		// 폰트 : 굴림 7
	 LCD_SetTextColor(RGB_RED);	
     LCD_DrawChar(62, 115, Rscore + 0x30);  
     LCD_SetTextColor(RGB_BLACK);	
 	 LCD_DrawText(70, 115, "vs");  

	 LCD_SetTextColor(RGB_BLUE);	// 글자색 : BLUE
     LCD_DrawChar(84, 115, Bscore + 0x30);  // Title NoC : 0
}

// 승자 판별
void Winner(void) {
    CheckHor();
    CheckVer();
    CheckDia();
    DelayMS(100);
    if (winner == 1) {  // 적돌 승리
        RWin();
    }
    else if (winner == 2) {  // 청돌 승리
        BWin();
    }
    if (winner != 0) {      // 적(청)돌 승리 시 1초간격으로 buzzer 5회
        for (int i = 0; i < 5; i++) {
            DelayMS(1000);  // 1초 간격
            BEEP();
        }
        ResetGame();        // 리셋 게임
        winner = 0;     // 승자 초기화
    }
} 

// 가로 검사
void CheckHor(void) {    
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 6; x++) {
            if (stadium[x][y] == 1 && stadium[x + 1][y] == 1 && stadium[x + 2][y] == 1 && stadium[x + 3][y] == 1 && stadium[x + 4][y] == 1) {
                winner = 1;
                return;
            }
            if (stadium[x][y] == 2 && stadium[x + 1][y] == 2 && stadium[x + 2][y] == 2 && stadium[x + 3][y] == 2 && stadium[x + 4][y] == 2) {
                winner = 2;
                return;
            }
        }
    }
}

// 세로 검사
void CheckVer(void) {        
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 6; y++) {
            if (stadium[x][y] == 1 && stadium[x][y + 1] == 1 && stadium[x][y + 2] == 1 && stadium[x][y + 3] == 1 && stadium[x][y + 4] == 1) {
                winner = 1;
                return;
            }
            if (stadium[x][y] == 2 && stadium[x][y + 1] == 2 && stadium[x][y + 2] == 2 && stadium[x][y + 3] == 2 && stadium[x][y + 4] == 2) {
                winner = 2;
                return;
            }
        }
    }
}

// 대각선 검사
void CheckDia(void) {
    // 왼쪽 위에서 오른쪽 아래
    for (int x = 0; x < 6; x++) {
        for (int y = 0; y < 6; y++) {
            if (stadium[x][y] == 1 && stadium[x + 1][y + 1] == 1 && stadium[x + 2][y + 2] == 1 && stadium[x + 3][y + 3] == 1 && stadium[x + 4][y + 4] == 1) {
                winner = 1;
                return;
            }
            if (stadium[x][y] == 2 && stadium[x + 1][y + 1] == 2 && stadium[x + 2][y + 2] == 2 && stadium[x + 3][y + 3] == 2 && stadium[x + 4][y + 4] == 2) {
                winner = 2;
                return;
            }
        }
    }

    // 오른쪽 위에서 왼쪽 아래
    for (int x = 4; x < 10; x++) {
        for (int y = 0; y < 6; y++) {
            if (stadium[x][y] == 1 && stadium[x - 1][y + 1] == 1 && stadium[x - 2][y + 2] == 1 && stadium[x - 3][y + 3] == 1 && stadium[x - 4][y + 4] == 1) {
                winner = 1;
                return;
            }
            if (stadium[x][y] == 2 && stadium[x - 1][y + 1] == 2 && stadium[x - 2][y + 2] == 2 && stadium[x - 3][y + 3] == 2 && stadium[x - 4][y + 4] == 2) {
                winner = 2;
                return;
            }
        }
    }
}

// 적돌 승리
void RWin(void) {
    Rscore = Fram_Read(300);  // FRAM(300)에서 점수 읽기
    Rscore = (Rscore + 1) % 10;    // 점수 증가(0~9까지)
    Fram_Write(300, Rscore);      // FRAM(300)에 점수 저장
    DisplayScore();
}

// 청돌 승리
void BWin(void) {
    Bscore = Fram_Read(301);  // FRAM(301)에서 점수 읽기
    Bscore = (Bscore + 1) % 10;    // 점수 증가(0~9까지)
    Fram_Write(301, Bscore);      // FRAM(301)에 점수 저장
    DisplayScore();

}

// 스코어 출력
void DisplayScore(void) {
    LCD_SetTextColor(RGB_RED);
    LCD_DrawChar(62, 115, Rscore + 0x30);  // 적돌 점수 표시
    LCD_SetTextColor(RGB_BLUE);
    LCD_DrawChar(84, 115, Bscore + 0x30);  // 청돌 점수 표시
}

// 리셋 게임
void ResetGame(void) {
    // 배열 초기화
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            stadium[i][j] = 0;
       }
    }
    GPIOG->ODR &= ~0x00FF;	// LED 초기값: LED0~7 Off
    RX = RY = BX = BY = 5;  // 돌 위치 초기화
    RSX = RSY = BSX = BSY = 67;  // 좌표 초기화
    turn_complete = 0;      // 턴 초기화
    Rscore = Fram_Read(300);  // FRAM에서 점수 읽기
    Bscore = Fram_Read(301);  // FRAM에서 점수 읽기
    DisplayInitScreen();    // LCD 초기화면
    DisplayScore();     // FRAM에 저장된 점수 표시

}

// 적돌 이동
void RSTMove(void){
     LCD_SetFont(&Gulim7);		
	 LCD_SetTextColor(RGB_RED);	
	 LCD_DrawText(10, 115, "(");  
	 LCD_DrawChar(16, 115, RX + 0x30); 
	 LCD_DrawText(22, 115, ",");  
	 LCD_DrawChar(28, 115, RY + 0x30);   
	 LCD_DrawText(34, 115, ")");  
}

// 청돌 이동
void BSTMove(void){
     LCD_SetFont(&Gulim7);		
	 LCD_SetTextColor(RGB_BLUE);	
	 LCD_DrawText(116, 115, "("); 
	 LCD_DrawChar(122, 115, BX + 0x30);  
	 LCD_DrawText(128, 115, ",");  
	 LCD_DrawChar(134, 115, BY + 0x30);       
	 LCD_DrawText(140, 115, ")");  
}


// 적돌 착돌
void RSTPUT(void) {
    LCD_SetBrushColor(RGB_RED); 
    LCD_DrawFillRect(RSX, RSY, 7, 7);
}

// 청돌 착돌
void BSTPUT(void) {
    LCD_SetBrushColor(RGB_BLUE); 
    LCD_DrawFillRect(BSX, BSY, 7, 7);
}

void _EXTI_Init(void)
{
	RCC->AHB1ENR |= 0x00000080;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= 0x00004000;	// Enable System Configuration Controller Clock

    SYSCFG->EXTICR[1] |= 0x0080;	// EXTI5에 대한 소스 입력은 GPIOH로 설정
	SYSCFG->EXTICR[2] |= 0x0007;	// EXTI8에 대한 소스 입력은 GPIOH로 설정
	SYSCFG->EXTICR[3] |= 0x7000;	// EXTI15에 대한 소스 입력은 GPIOH로 설정
	// EXTI8,15<- PHx

	EXTI->FTSR |= 0x8120;		// Falling trigger enable 5, 8, 15
	EXTI->IMR |= 0x8100;		// mask 설정 - 8, 15 mask Enable

	NVIC->ISER[0] |= (1 << 23);	// Enable 'Global Interrupt EXTI5,8'
	NVIC->ISER[1] |= (1 << 8);	// Enable 'Global Interrupt EXTI11~15'
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & 0x0020)			// EXTI5 Interrupt Pending(발생)
    {
        EXTI->PR |= 0x0020;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
        if (RST_Flag == 1) {
            if (turn_complete == 1) {       // 적돌이 착돌 된 상태에서 한 번 더 착돌 시 buzzer 3회
                DelayMS(1000);
                for (int i = 0; i < 3; i++) {
                    BEEP();
                    DelayMS(300);
                    USART1_SendString("Not running ");
                }
                return;
            }
            if (stadium[RX][RY] == 0) {     // 착돌되지 않은 곳이면 착돌
                RSTPUT();
                BEEP();		
                stadium[RX][RY] = 1;
                turn_complete = 1;

                char msg[5] = {'R', RX + '0', RY + '0', ' ', '\0'};
                USART1_SendString(msg);
            }
            else {      // 착돌 된 곳에 착돌 시 buzzer 3회
                DelayMS(1000);
                for (int i = 0; i < 3; i++) {
                    BEEP();
                    DelayMS(300);
                    USART1_SendString("Not running ");
                    
                }
            }
        }
        else if (BST_Flag == 1) {
            if (turn_complete == 2) {  // 청돌이 착돌 된 상태에서 한 번 더 착돌 시 buzzer 3회
                DelayMS(1000);
                for (int i = 0; i < 3; i++) {
                    BEEP();  
                    DelayMS(300);
                    USART1_SendString("Not running ");
                }
                return;
            }
            if (stadium[BX][BY] == 0) {     // 착돌되지 않은 곳이면 착돌
                BSTPUT();
                BEEP();		
                stadium[BX][BY] = 2;
                turn_complete = 2;

                char msg[5] = {'B', BX + '0', BY + '0', ' ', '\0'};
                USART1_SendString(msg);

            }
            else {       // 착돌 된 곳에 착돌 시 buzzer 3회
                DelayMS(1000);
                for (int i = 0; i < 3; i++) {
                    BEEP();
                    DelayMS(300);
                    USART1_SendString("Not running ");
                }
            }
        }
        Winner();       // 승자 판별
    }

    if (EXTI->PR & 0x0100)			// EXTI8 Interrupt Pending(발생)
    {
        EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
        GPIOG->ODR |= 0x0001;       // LED0 ON
        GPIOG->ODR &= ~0x0080;      // LED7 OFF
        LCD_SetFont(&Gulim7);	
        LCD_SetBrushColor(RGB_RED); 
        LCD_SetTextColor(RGB_RED);	
        LCD_DrawText(2, 115, "*");  
        LCD_SetTextColor(RGB_YELLOW);	
        LCD_DrawText(152, 115, "*");  
        RST_Flag = 1;      
        BST_Flag = 0;
        EXTI->IMR |= 0x8120;		// mask 설정 - 5, 8, 15 mask Enable
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & 0x8000)		// EXTI15 Interrupt Pending(발생) 여부?
    {
        EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
        GPIOG->ODR |= 0x0080;       // LED7 ON
        GPIOG->ODR &= ~0x0001;       // LED0 OFF
        LCD_SetFont(&Gulim7);		
        LCD_SetBrushColor(RGB_BLUE); 
        LCD_SetTextColor(RGB_BLUE);	
        LCD_DrawText(152, 115, "*");  
        LCD_SetTextColor(RGB_YELLOW);	
        LCD_DrawText(2, 115, "*");  
        BST_Flag = 1;
        RST_Flag = 0;
        EXTI->IMR |= 0x8120;		// mask 설정 - 5, 8, 15 mask Enable
    }
}


void USART1_Init(void)   //USART1 초기화 함수
{
  // USART1 설정: 38400 baud, 9-bit data, Odd parity
  USART1->BRR = SystemCoreClock / 38400;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_M | USART_CR1_PCE | USART_CR1_PS;
  USART1->CR2 = 0;
  USART1->CR3 = 0;
  
  // USART1 인터럽트 활성화
  NVIC_EnableIRQ(USART1_IRQn);
}



 //USART1 송수신 함수
void USART1_SendChar(char ch) 
{
  while(!(USART1->SR & USART_SR_TXE));
  USART1->DR = ch;
}

void USART1_SendString(char* str)
{
  while(*str) USART1_SendChar(*str++);
}

char USART1_ReceiveChar(void)
{
  while(!(USART1->SR & USART_SR_RXNE));
  return USART1->DR;
}



void USART1_IRQHandler(void) //USART1 인터럽트 핸들러
{
  if(USART1->SR & USART_SR_RXNE)
  {
    char received = USART1_ReceiveChar();
    if(received == 'R' || received == 'B')
    {
      char x = USART1_ReceiveChar();
      char y = USART1_ReceiveChar();
      
      if(x >= '0' && x <= '9' && y >= '0' && y <= '9')
      {
        int newX = x - '0';
        int newY = y - '0';
        
        if(received == 'R')
        {
          RX = newX;
          RY = newY;
          RSX = 28 + RX * 8;
          RSY = 28 + RY * 8;
          RST_Flag = 1;
          BST_Flag = 0;
          LCD_SetTextColor(RGB_RED);
          LCD_DrawText(2, 115, "*");
          LCD_SetTextColor(RGB_YELLOW);
          LCD_DrawText(152, 115, "*");
          GPIOG->ODR |= 0x0001;  // LED0 ON
          GPIOG->ODR &= ~0x0080; // LED7 OFF
        }
        else
        {
          BX = newX;
          BY = newY;
          BSX = 28 + BX * 8;
          BSY = 28 + BY * 8;
          BST_Flag = 1;
          RST_Flag = 0;
          LCD_SetTextColor(RGB_BLUE);
          LCD_DrawText(152, 115, "*");
          LCD_SetTextColor(RGB_YELLOW);
          LCD_DrawText(2, 115, "*");
          GPIOG->ODR |= 0x0080;  // LED7 ON
          GPIOG->ODR &= ~0x0001; // LED0 OFF
        }
        
        RSTMove();
        BSTMove();
      }
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