//////////////////////////////////////////////////////////////////////////
// 가속도센서와의 SPI통신을  위한 application function 제공
//  SPI1_Send(uint8_t byte): 1 byte 데이터를 가속도센서에 전송 
//  ACC_Init(): 가속도센서 초기화, CTRL1,2,4에 초기값 전송/입력
//  SPI1_GetData(UINT8 *pBuf) : 센서로부터 가속도값 X,Y,Z를 읽어옴, pBuf[]로 return
//  SPI1_Process(UINT16 *pBuf) : 가속도값 읽고 16bit 형태로 변환,  pBuf[]로 return 
//  SPI1_CalData(UINT8 *pBuf, UINT16 *pTxBuf) : 가속도값을 16bit 형태로 변환,
///////////////////////////////////////////////////////////////////////////
#define __ACC_C__
#include "ACC.h"
#undef  __ACC_C__

UINT8 SPI1_GetData(UINT8 *pBuf);
void SPI1_CalData(UINT8 *pBuf, int16 *pTxBuf);

#define DUMMY   0x00		// 의미없는 데이터, 수신만을 위해 전송하는 데이터 

uint8_t SPI1_Send(uint8_t byte)		// 1 byte 데이터를 가속도센서에 전송 
{
        // Polling: Loop while DR register in not empty 
        while ((SPI1->SR & (1<<1)) == RESET);		// SR.TXE = 1 ?	
        
        // Send a byte through the SPI1 peripheral(ACC sensor)
        SPI1->DR = byte;

        // Polling: Wait to receive a byte 
        while ((SPI1->SR & (1<<0)) == RESET);		// SR.RXNE = 1 ?	
	
        // Return the byte read from the SPI bus (ACC sensor) 
        return(SPI1->DR);
}

/////// ACC_Init(void)	////////////////////////////////////////
// CTRL1 초기값 입력
// 	HR : high resolution (Value:1)
// 	ODR[2:0] : 800Hz (Value:110) Output Data Rate
// 	ZEN : Z-axis enabled (Value:1)
// 	YEN : Y-axis enabled (Value:1)
// 	XEN : X-axis enabled (Value:1)
// CTRL2 초기값 입력
//	DFC[1:0] : ODR 9 (Value:10) 
//	   (HighPass cutoff freqency 사용:ODR에서 선택된 주파수)
// CTRL4 초기값 입력
//	BW[2:1] : 400Hz (Value:00)  
// 	FS : +/-2g (Value:00)
// 	BW_SCALE_ODR : automatically(Value:0)
// 	IF_ADD_INC : enabled (Value:1)
//	I2C_DISABLE : Disabled (Value:1)
//	SIM : 4-wire interface (Value:0)
////////////////////////////////////////////////////////////////////
void ACC_Init(void)	
{
        SET_ACC_NSS1();         // NSS : H

	// Control Reg 1 
        CLR_ACC_NSS1();	        // NSS : L (enable: 통신 시작)
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL1);	// Master -> Slave(sensor)
						// writing a Command to CTRL1(20h) 
        SPI1_Send(HR_HIGH_RESOLUTION		// Command
                | ODR_800HZ
                | BDU_CONTINUOUS_UPDATE
                | ZEN_Z_ENABLE  
                | YEN_Y_ENABLE
                | XEN_X_ENABLE);
        SET_ACC_NSS1();         // NSS : H
        UTIL_DelayUS(1);
  
	// Control Reg 2 
        CLR_ACC_NSS1();	       // NSS : L (enable: 통신 시작)
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL2);	// Master -> Slave(sensor)
                                              	// writing a Command to CTRL2(21h)   
        SPI1_Send(ZERO                        	// command
                | DFC_ODR_9
		| FDS_FILTER_BYPASSED
		| HPIS1_FILTER_BYPASSED  
		| HPIS2_FILTER_BYPASSED);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);

	// Control Reg 3   
        CLR_ACC_NSS1();	
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL3);  
        SPI1_Send(0x00);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);

	// Control Reg 4   
        CLR_ACC_NSS1();	
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL4); 	// Master -> Slave(sensor)
                                             	// writing a Command to CTRL4(24h) 
        SPI1_Send(BW_400HZ 
		| FS_2G
		| BW_SCALE_ODR_AUTOMATICALLY
		| IF_ADD_INC_ENABLE  
		| I2C_DISABLE
		| SIM_4WIRE);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);

	// Control Reg 5   
        CLR_ACC_NSS1();	
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL5);  
        SPI1_Send(0x00);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);
  
	// Control Reg 6 
        CLR_ACC_NSS1();	
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL6);  
        SPI1_Send(0x00);
        SET_ACC_NSS1();
  
	// Control Reg 7 
        CLR_ACC_NSS1();	
        SPI1_Send(CMD_WRITE | CMD_REG_CTRL7);  
        SPI1_Send(0x00);
        SET_ACC_NSS1();
  
        UTIL_DelayUS(100);
}

/////////////////////////////////////////////////////
// 센서로부터 가속도값 X,Y,Z를 읽어옴, PBuf[]를 return
// X값: pBuf[1], pBuf[0]
// Y값: pBuf[3], pBuf[2]
// Z값: pBuf[5], pBuf[4]
UINT8 SPI1_GetData(UINT8 *pBuf)
{
	// Get ACC_X
        CLR_ACC_NSS1();  
        SPI1_Send(CMD_READ | CMD_REG_OUT_X_L);	// Master <- Slave(sensor)
                                             	// reading a data from OUT_X
        pBuf[0] = SPI1_Send(DUMMY);		// 가속도 X 값은 16비트이므로 두개의 dummy를 보냄
        pBuf[1] = SPI1_Send(DUMMY);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);
  
	// Get ACC_Y
        CLR_ACC_NSS1();  
        SPI1_Send(CMD_READ | CMD_REG_OUT_Y_L);	// Master <- Slave(sensor)
                                             	// reading a data from OUT_Y
        pBuf[2] = SPI1_Send(DUMMY);		// 가속도 Y 값은 16비트이므로 두개의 dummy를 보냄
        pBuf[3] = SPI1_Send(DUMMY);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);
  
	// Get ACC_Z
        CLR_ACC_NSS1();  
        SPI1_Send(CMD_READ | CMD_REG_OUT_Z_L);	// Master <- Slave(sensor)
                                             	// reading a data from OUT_Z
        pBuf[4] = SPI1_Send(DUMMY);		// 가속도 Z 값은 16비트이므로 두개의 dummy를 보냄
        pBuf[5] = SPI1_Send(DUMMY);
        SET_ACC_NSS1();
        UTIL_DelayUS(1);  
  
        return true;
}

//   가속도값 읽고 16bit 형태로 변환,  pBuf[]로 return 
void SPI1_Process(int16 *pBuf)
{
        UINT8 buffer[6];
  
        SPI1_GetData(&buffer[0]);		// Get ACC value(X,Y,Z)
  
        SPI1_CalData(&buffer[0],&pBuf[0]);	// 8 bit data --> 16 bit data
}

// 가속도값을 16bit 형태로 변환 
// 입력 pBuf[5..0]: 8bit 6개의 값(X_H,X_L, Y_H,Y_L,  Z_H,Z_L)
// 출력 pTxBuf[2..0] : 16bit 3개의 값 (X, Y, Z)
void SPI1_CalData(UINT8 *pBuf, int16 *pTxBuf) 
{
        UINT16 TempUniX0, TempUniY0, TempUniZ0;

        int16 TempX0, TempY0, TempZ0;

        TempX0 = (int16)(((int16)pBuf[1]<<8) | ((int16)pBuf[0]&0xFF));
        TempY0 = (int16)(((int16)pBuf[3]<<8) | ((int16)pBuf[2]&0xFF));
        TempZ0 = (int16)(((int16)pBuf[5]<<8) | ((int16)pBuf[4]&0xFF));
  
 //       pTxBuf[0] = abs(TempX0);
 //       pTxBuf[1] = abs(TempY0);
 //       pTxBuf[2] = abs(TempZ0);
		pTxBuf[0] = TempX0;
		pTxBuf[1] = TempY0;
		pTxBuf[2] = TempZ0;

}


