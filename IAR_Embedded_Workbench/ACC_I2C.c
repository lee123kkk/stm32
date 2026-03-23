//////////////////////////////////////////////////////////////////////////
// 가속도센서와의 I2C통신을 application function 제공
//  ACC_I2C_Init(): 가속도센서 초기화, CTRL1,2,4에 초기값 전송/입력
//  ACC_I2C_GetData(UINT16 *pBuf) : 센서로부터 가속도값 X,Y,Z를 읽어옴, pBuf[]로 return
//  ACC_I2C_Process(UINT16 *pBuf) : 가속도값 읽고 16bit 형태로 변환,  pBuf[]로 return 
//  ACC_I2C_CalData(UINT16 *pBuf, UINT16 *pTxBuf) : 가속도값을 16bit 형태로 변환,
///////////////////////////////////////////////////////////////////////////
#define __ACC_I2C_C__
#include "ACC_I2C.h"
#undef  __ACC_I2C_C__

UINT16 ACC_I2C_GetData(UINT16 *pBuf);
void ACC_I2C_CalData(UINT16 *pBuf, UINT16 *pTxBuf);

////////////////////////////////// I2C  ////////////////////////////////////////////////

////////////////////////////////// I2C_Write  ////////////////////////////////////////////////
uint8_t I2C_WriteDeviceRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
	/* Begin the configuration sequence */
//	I2C_GenerateSTART(I2C2, ENABLE); // I2C_GenerateSTART() in stm32f4xx.i2c.c
	I2C2->CR1 |= I2C_CR1_START;
	
	/* Test on EV5 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB)) // I2C_GetFlagStatus() in stm32f4xx.i2c.c
//	{ }
	while ((I2C2->SR1 & I2C_SR1_SB) != I2C_SR1_SB)
	{ }
 
	/* Transmit the slave address and enable writing operation */
//	I2C_Send7bitAddress(I2C2, ACC_ADDR, I2C_Direction_Transmitter);
	I2C2->DR = ACC_ADDR & (~(uint8_t)I2C_OAR1_ADD0);
	  
	/* Test on EV6 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
//	{ }
	while ((I2C2->SR1 & I2C_SR1_ADDR) != I2C_SR1_ADDR)
	{ }
 
	/* Read status register 2 to clear ADDR flag */
	I2C2->SR2;
  
	/* Test on EV8_1 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
//	{ }
	while ((I2C2->SR1 & I2C_SR1_TXE) != I2C_SR1_TXE)
	{ }
 
	/* Transmit the first address for r/w operations */
//	I2C_SendData(I2C2, RegisterAddr);
	I2C2->DR = RegisterAddr;
  
	/* Test on EV8 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
//	{ }
 	while ((I2C2->SR1 & I2C_SR1_TXE) != I2C_SR1_TXE)
	{ }
 
	/* Prepare the register value to be sent */
//	I2C_SendData(I2C2, RegisterValue);
	I2C2->DR = RegisterValue;
  
	/* Test on EV8_2 and clear it */
//	while ((!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF)))
//	{ }
 	while ( ((I2C2->SR1 & I2C_SR1_TXE) != I2C_SR1_TXE) || ((I2C2->SR1 & I2C_SR1_BTF) != I2C_SR1_BTF) )
	{ }
 
	/* End the configuration sequence */
//	I2C_GenerateSTOP(I2C2, ENABLE);
	I2C2->CR1 |= I2C_CR1_STOP;
	
	return 0;
}

////////////////////////////////// I2C_Read (8-bit)  ////////////////////////////////////////////////
uint8_t ACC_I2C_ReadDeviceRegister(uint8_t RegisterAddr)
{
	uint8_t tmp = 0;

	/* Enable Acknowledgement to be ready for reception */
//  I2C_AcknowledgeConfig(I2C2, ENABLE);
	I2C2->CR1 |= I2C_CR1_ACK;  

	/* Enable the I2C peripheral */
//	I2C_GenerateSTART(I2C2, ENABLE);
	I2C2->CR1 |= I2C_CR1_START;
  
	/* Test on EV5 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
//	{}
	while ((I2C2->SR1 & I2C_SR1_SB) != I2C_SR1_SB)
	{ }
  
	/* Transmit the slave address and enable writing operation */
//	I2C_Send7bitAddress(I2C2, ACC_ADDR, I2C_Direction_Transmitter);
	I2C2->DR = ACC_ADDR & (~(uint8_t)I2C_OAR1_ADD0);
  
	/* Test on EV6 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
//	{}
	while ((I2C2->SR1 & I2C_SR1_ADDR) != I2C_SR1_ADDR)
	{ }
  
	/* Read status register 2 to clear ADDR flag */
	I2C2->SR2;
  
	/* Test on EV8 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
//	{}
	while ((I2C2->SR1 & I2C_SR1_TXE) != I2C_SR1_TXE)
	{ }
  
	/* Transmit the first address for r/w operations */
//	I2C_SendData(I2C2, RegisterAddr);
	I2C2->DR = RegisterAddr;
  
	/* Test on EV8 and clear it */
//	while ((!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF)))
//	{}
 	while ( ((I2C2->SR1 & I2C_SR1_TXE) != I2C_SR1_TXE) || ((I2C2->SR1 & I2C_SR1_BTF) != I2C_SR1_BTF) )
	{ }
   
	/* Regenerate a start condition */
//	I2C_GenerateSTART(I2C2, ENABLE);
	I2C2->CR1 |= I2C_CR1_START;
  
  	/* Test on EV5 and clear it */
//	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
//	{}
	while ((I2C2->SR1 & I2C_SR1_SB) != I2C_SR1_SB)
	{ }
  
  	/* Transmit the slave address and enable Reding operation */
	I2C_Send7bitAddress(I2C2, ACC_ADDR, I2C_Direction_Receiver);
  
  	/* Test on EV6 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	{}
  
  	/* Disable Acknowledgement */
	I2C_AcknowledgeConfig(I2C2, DISABLE);

  	/* Read status register 2 to clear ADDR flag */
	I2C2->SR2;
  
  	/* End the configuration sequence */
	I2C_GenerateSTOP(I2C2, ENABLE);
  
  	/* Test on EV7 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_RXNE))
	{ }
          
  	/* Load the register value */
	tmp = I2C_ReceiveData(I2C2);
  
  	/* Enable Acknowledgement to be ready for another reception */
	I2C_AcknowledgeConfig(I2C2, ENABLE);
  
  	/* Return the read value */
	return tmp;
  
}

////////////////////////////////// I2C_Read (16-bit)  ////////////////////////////////////////////////
uint16_t I2C_ReadDataBuffer(uint32_t RegisterAddr)
{
	uint8_t ACC_BufferRX[2] = {0x00, 0x00};  
  
 	/* Enable the I2C peripheral */
	I2C_GenerateSTART(I2C2, ENABLE);
 
  	/* Test on EV5 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
	{ }
   
  	/* Send device address for write */
	I2C_Send7bitAddress(I2C2, ACC_ADDR, I2C_Direction_Transmitter);
  
  	/* Test on EV6 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	{ }
  
  	/* Read status register 2 to clear ADDR flag */
	I2C2->SR2;
  
  	/* Test on EV8 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
	{ }
  
	/* Send the device's internal address to write to */
	I2C_SendData(I2C2, RegisterAddr);  
    
	/* Send START condition a second time */  
	I2C_GenerateSTART(I2C2, ENABLE);
  
	/* Test on EV5 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
	{ }
  
	/* Send IO Expander address for read */
	I2C_Send7bitAddress(I2C2, ACC_ADDR, I2C_Direction_Receiver);
  
	/* Test on EV6 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	{ }
 
	/* Disable Acknowledgement and set Pos bit */
	I2C_AcknowledgeConfig(I2C2, DISABLE);       
	I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
  
	/* Read status register 2 to clear ADDR flag */
	I2C2->SR2;

	/* Test on EV7 and clear it */
	while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF))
	{ }
 
	/* Send STOP Condition */
	I2C_GenerateSTOP(I2C2, ENABLE);
   
	/* Read the first byte from the IO Expander */
	ACC_BufferRX[1] = I2C_ReceiveData(I2C2);
    
	/* Read the second byte from the IO Expander */
	ACC_BufferRX[0] = I2C_ReceiveData(I2C2);
                                         
	/* Enable Acknowledgement and reset POS bit to be ready for another reception */
	I2C_AcknowledgeConfig(I2C2, ENABLE);
	I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
   
	/* return the data */
	return ((uint16_t) ACC_BufferRX[0] | ((uint16_t)ACC_BufferRX[1]<< 8));

}

/////// ACC_I2C_Init(void)	////////////////////////////////////////
// CTRL1 초기값 입력
// 	HR : high resolution (Value:1)
// 	ODR[2:0] : 800Hz (Value:110) Output Data Rate
// 	ZEN : Z-axis enabled (Value:1)
// 	YEN : Y-axis enabled (Value:1)
// 	XEN : X-axis enabled (Value:1)
// CTRL2 초기값 입력
//	DFC[1:0] : ODR 9 (Value:10) 
//	(HighPass cutoff freqency 사용:ODR에서 선택된 주파수)
// CTRL4 초기값 입력
//	BW[2:1] : 400Hz (Value:00)  
// 	FS : +/-2g (Value:00)
// 	BW_SCALE_ODR : automatically(Value:0)
// 	IF_ADD_INC : enabled (Value:1)
//	I2C_ENABLE : Enabled (Value:0) <- I2C 통신 사용 시  enable
//	SIM : 4-wire interface (Value:0)
////////////////////////////////////////////////////////////////////

void ACC_I2C_Init(void)	
{
	// Control Reg 1 
	// writing a Command to CTRL1(20h) 
	I2C_WriteDeviceRegister(CMD_REG_CTRL1, HR_HIGH_RESOLUTION	// Command
						| ODR_800HZ
						| BDU_CONTINUOUS_UPDATE
						| ZEN_Z_ENABLE  
						| YEN_Y_ENABLE
						| XEN_X_ENABLE);

	// Control Reg 2 
	I2C_WriteDeviceRegister(CMD_REG_CTRL2, ZERO		// command
						| DFC_ODR_9
						| FDS_FILTER_BYPASSED
						| HPIS1_FILTER_BYPASSED  
						| HPIS2_FILTER_BYPASSED);

	// Control Reg 3   
	I2C_WriteDeviceRegister(CMD_REG_CTRL3, 0x00);

	// Control Reg 4   
	I2C_WriteDeviceRegister(CMD_REG_CTRL4, BW_400HZ 
						| FS_2G
						| BW_SCALE_ODR_AUTOMATICALLY
						| IF_ADD_INC_ENABLE
						| I2C_ENABLE); // I2C 통신 사용 시 Enable( SPI 사용 시 Disable)
        
	// Control Reg 5   
	I2C_WriteDeviceRegister(CMD_REG_CTRL5, 0x00);
  
	// Control Reg 6 
	I2C_WriteDeviceRegister(CMD_REG_CTRL6, 0x00);

	// Control Reg 7 
	I2C_WriteDeviceRegister(CMD_REG_CTRL7, 0x00);

}

/////////////////////////////////////////////////////
// 센서로부터 가속도값 X,Y,Z를 읽어옴, PBuf[]를 return
// X값: pBuf[0] OUT_X_L - OUT_X_H
// Y값: pBuf[1} OUT_Y_L - OUT_Y_H
// Z값: pBuf[2] OUT_Z_L - OUT_Z_H
UINT16 ACC_I2C_GetData(UINT16 *pBuf)
{
	// Get ACC_X
	pBuf[0] = I2C_ReadDataBuffer(CMD_REG_OUT_X_L);
          
	// Get ACC_Y        
	pBuf[1] = I2C_ReadDataBuffer(CMD_REG_OUT_Y_L);
  
	// Get ACC_Z         
	pBuf[2] = I2C_ReadDataBuffer(CMD_REG_OUT_Z_L);
       
	return true;
}


//   가속도값 읽고 16bit 형태로 변환,  pBuf[]로 return 
void ACC_I2C_Process(UINT16 *pBuf)
{
	UINT16 buffer[3];
  
	ACC_I2C_GetData(&buffer[0]);		// Get ACC value(X,Y,Z)
  
	ACC_I2C_CalData(&buffer[0],&pBuf[0]);	//  출력 byte 위치 변경
}


// 가속도값을 16bit 형태로 변환
// 입력 pBuf[2..0]: 16bit 3개의 값		(X_L,X_H, Y_L,Y_H,  Z_L,Z_H)
// 출력 pTxBuf[2..0] : 16bit 3개의 값	(X_H,X_L, Y_H,Y_L,  Z_H,Z_L)
void ACC_I2C_CalData(UINT16 *pBuf, UINT16 *pTxBuf) 
{
        int16 TempX0, TempY0, TempZ0;

        TempX0 = (int16)(((int16)pBuf[0]<<8) | ((int16)pBuf[0]&0xFF));
        TempY0 = (int16)(((int16)pBuf[1]<<8) | ((int16)pBuf[1]&0xFF));
        TempZ0 = (int16)(((int16)pBuf[2]<<8) | ((int16)pBuf[2]&0xFF));
  
        pTxBuf[0] = abs(TempX0);
        pTxBuf[1] = abs(TempY0);
        pTxBuf[2] = abs(TempZ0);
}








