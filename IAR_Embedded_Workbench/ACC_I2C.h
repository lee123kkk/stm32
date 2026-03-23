#ifndef __ACC_I2C_H__
#define __ACC_I2C_H__

#include "Util.h"

#ifdef __ACC_I2C_C__
    #define ACC_I2C_EXT
#else
    #define ACC_I2C_EXT extern
#endif

//------------------------ SPI----------------------//

#define SET_ACC_NSS1()      GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET); 	
			// GPIOA->ODR |= (1<<8) 와 같은 역할, NSS핀을 'High';
#define CLR_ACC_NSS1()      GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
			// GPIOA->ODR &= ~(1<<8);
// void GPIO_WriteBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BitAction BitVal)
// : stm32f4xx_gpio.c에 정의되어 있음, GPIOx의 핀(GPIO_Pin)을 'H' or 'L'로 지정함

#define CMD_WRITE           0x00
#define CMD_READ            0x80
#define CMD_READ_INCREMENT  0xC0

//------------------------ I2C ----------------------//
#define ACC_ADDR				0x3A		//가속도 센서 Address

//------------------------ REGISTER ----------------------//
#define CMD_REG_TEMP_L      0x0B
#define CMD_REG_TEMP_H      0x0C

#define CMD_REG_WHO_AM_I    0x0F

#define CMD_ACT_THS         0x1E
#define CMD_ACT_DUR         0x1F

#define CMD_REG_CTRL1       0x20
#define CMD_REG_CTRL2       0x21
#define CMD_REG_CTRL3       0x22
#define CMD_REG_CTRL4       0x23
#define CMD_REG_CTRL5       0x24
#define CMD_REG_CTRL6       0x25
#define CMD_REG_CTRL7       0x26

#define CMD_REG_STATUS      0x27

#define CMD_REG_OUT_X_L     0x28
#define CMD_REG_OUT_X_H     0x29
#define CMD_REG_OUT_Y_L     0x2A
#define CMD_REG_OUT_Y_H     0x2B
#define CMD_REG_OUT_Z_L     0x2C
#define CMD_REG_OUT_Z_H     0x2D

#define CMD_REG_FIFO_CTRL
#define CMD_REG_FIFO_SRC

#define CMD_REG_IG_CFG1
#define CMD_REG_IG_SRC1

//------------------------ CTRL1(20h) ----------------------//
#define HR_HIGH_RESOLUTION          0x80
#define ODR_800HZ                   0x60
#define BDU_CONTINUOUS_UPDATE       0x00
#define ZEN_Z_ENABLE                0x04
#define YEN_Y_ENABLE                0x02
#define XEN_X_ENABLE                0x01

//------------------------ CTRL2(21h) ----------------------//
#define ZERO                        0x00
#define DFC_ODR_9                   0x40
#define HPM_NOMAL_MODE              0x00
#define FDS_FILTER_BYPASSED         0x00
#define HPIS1_FILTER_BYPASSED       0x00
#define HPIS2_FILTER_BYPASSED       0x00

//------------------------ CTRL2(22h) ----------------------//
#define FIFO_DISABLE                0x00

//------------------------ CTRL3(23h) ----------------------//

//------------------------ CTRL4(24h) ----------------------//
#define BW_400HZ                    0x00
#define BW_200HZ                    0x40
#define FS_2G                       0x00
#define FS_4G                       0x20
#define FS_8G                       0x30
#define BW_SCALE_ODR_AUTOMATICALLY  0x00
#define IF_ADD_INC_ENABLE           0x04
#define I2C_DISABLE                 0x02
#define I2C_ENABLE                 0x00
#define SIM_4WIRE                   0x00

//------------------------ CTRL5(25h) ----------------------//

//------------------------ CTRL6(26h) ----------------------//

//------------------------ CTRL7(27h) ----------------------//

#define SCALE_2G                    0x00
#define SCALE_4G                    0x10
#define SCALE_8G                    0x20

#define SPI_4WIRE                   0x00

#define HIGH_RESOUTION              0x08


ACC_I2C_EXT void ACC_I2C_Init(void);
ACC_I2C_EXT void ACC_I2C_Process(UINT16 *pBuf);

uint8_t  I2C_WriteDeviceRegister(uint8_t RegisterAddr, uint8_t RegisterValue);
uint8_t  I2C_ReadDeviceRegister(uint8_t RegisterAddr);
uint16_t I2C_ReadDataBuffer(uint32_t RegisterAddr);
#endif