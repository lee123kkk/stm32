//////////////////////////////////////////////////////////////////////////
// SPI 통신을 이용한 FRAM 인터페이스
//   NSS(/CS) pin:  PI0 (GPIO 기능으로 작동) Active LOW
//   NWP(/WP) pin:  PI4 (GPIO 기능으로 작동) Active LOW
//   SCK pin:  PI1 (SPI2_SCK)
//   MOSI pin: PI2 (SPI2_MOSI)
//   MISO pin: PI3 (SPI2_MISO)
// SPI mode: MASTER
// FRAM(MB85RS64APNF, Slave mode) SRAM과 FLASH memory의 특징을 갖는 메모리
//     (1byte 단위로 access, 실제로는 1bit씩 aceess) 
//////////////////////////////////////////////////////////////////////////

#define _FRAM_RTC_C_
  #include "FRAM.h"
#undef  _FRAM_RTC_C_

#define FRAM_CS_LOW      GPIOI->ODR &= ~(1<<0); // GPIO_WriteBit(GPIOI, GPIO_Pin_0 ,0)
#define FRAM_CS_HIGH     GPIOI->ODR |=  (1<<0); // GPIO_WriteBit(GPIOI, GPIO_Pin_0 ,1) 

#define FRAM_WP_LOW      GPIOI->ODR &= ~(1<<4); // GPIO_WriteBit(GPIOI, GPIO_Pin_4 ,0)
#define FRAM_WP_HIGH     GPIOI->ODR |=  (1<<4); // GPIO_WriteBit(GPIOI, GPIO_Pin_4 ,1) 

uint8_t SPI2_Send(uint8_t byte)
{
        // Polling: Loop while DR register in not empty 
        while ((SPI2->SR & (1<<1)) == RESET);	// SR.TXE = 1 ?	
        
        // Send a byte through the SPI2 peripheral(ACC sensor)
        SPI2->DR = byte;

        // Polling: Wait to receive a byte 
        while ((SPI2->SR & (1<<0)) == RESET);	// SR.RXNE = 1 ?	
	
        // Return the byte read from the SPI bus (ACC sensor) 
        return(SPI2->DR);
}

///////////////////////////////////////////////////////
// SPI2 (APB1ENR.14) 
// Msater mode, Full-duplex, 8bit frame(MSB first), 
// fPCLK/32 Baud rate, Software slave management EN
void Fram_Init(void)  
{
	/*!< Clock Enable  *********************************************************/
        RCC->APB1ENR 	|= (1<<14);	// SPI2 Clock EN
        RCC->AHB1ENR 	|= (1<<8);	// GPIOI Clock EN		
  
        /*!< SPI2 pins configuration ************************************************/
	
        /*!< SPI2 NSS pin(PI4) configuration : GPIO 핀  */
        GPIOI->MODER 	|= (1<<(2*4));	// PI4 Output mode
        GPIOI->OTYPER 	&= ~(1<<4); 	// PI4 push-pull(reset state)
        GPIOI->OSPEEDR 	|= (2<<(2*4));	// PI4 Output speed (50MHZ) 
        GPIOI->PUPDR 	&= ~(3<<(2*4));	// PI4 NO Pullup Pulldown(reset state)

        /*!< SPI2 NWP pin(PI0) configuration : GPIO 핀  */
        GPIOI->MODER 	|= (1<<(2*0));	// PI0 Output mode
        GPIOI->OTYPER 	&= ~(1<<0); 	// PI0 push-pull(reset state)
        GPIOI->OSPEEDR 	|= (2<<(2*0));	// PI0 Output speed (50MHZ) 
        GPIOI->PUPDR 	&= ~(3<<(2*0));	// PI0 NO Pullup Pulldown(reset state)
    
        /*!< SPI2 SCK pin(PI1) configuration : SPI2_SCK */
        GPIOI->MODER 	|= (2<<(2*1)); 	// PI1 Alternate function mode
        GPIOI->OTYPER 	&= ~(1<<1); 	// PI1 Output type push-pull (reset state)
        GPIOI->OSPEEDR 	|= (2<<(2*1));	// PI1 Output speed (50MHz)
        GPIOI->PUPDR 	|= (2<<(2*1)); 	// PI1 Pull-down
        GPIOI->AFR[0] 	|= (5<<(4*1));	// Connect PI1 to AF5(SPI2)
    
        /*!< SPI2 MOSI pin(PI2) configuration : SPI2_MOSI */    
        GPIOI->MODER 	|= (2<<(2*2));	// PI2 Alternate function mode
        GPIOI->OTYPER	&= ~(1<<2);	// PI2 Output type push-pull (reset state)
        GPIOI->OSPEEDR 	|= (2<<(2*2));	// PI2 Output speed (50MHz)
        GPIOI->PUPDR 	|= (2<<(2*2)); 	// PI2 Pull-down
        GPIOI->AFR[0] 	|= (5<<(4*2));	// Connect PI2 to AF5(SPI2)
    
        /*!< SPI2 MISO pin(PI3) configuration : SPI2_MISO */
        GPIOI->MODER 	|= (2<<(2*3));	// PI3 Alternate function mode
        GPIOI->OTYPER 	&= ~(1<<3);	// PI3 Output type push-pull (reset state)
        GPIOI->OSPEEDR 	|= (2<<(2*3));	// PI3 Output speed (50MHz)
        GPIOI->PUPDR 	|= (2<<(2*3));	// PI3 Pull-down
        GPIOI->AFR[0] 	|= (5<<(4*3));	// Connect PI3 to AF5(SPI2)

        SPI2->CR1 |= (1<<2);	// MSTR(Master selection)=1, Master mode
        SPI2->CR1 &= ~(1<<15);	// SPI_Direction_2 Lines_FullDuplex
        SPI2->CR1 &= ~(1<<11);	// SPI_DataSize_8bit
        SPI2->CR1 |= (1<<9);  	// SSM(Software slave management)=1, 
				// NSS 핀 상태가 코딩에 의해 결정
        SPI2->CR1 |= (1<<8);   	// SSI(Internal_slave_select)=1,
				// 현재 MCU를 Master가 이므로 NSS 상태는 'High' 
        SPI2->CR1 &= ~(1<<7);	// LSBFirst=0, MSB transmitted first    
        SPI2->CR1 |= (4<<3);	// BR(BaudRate)=0b100, fPCLK/32 (84MHz/32 = 2.625MHz)
        SPI2->CR1 |= (1<<1);	// CPOL(Clock polarity)=1, CK is 'High' when idle
        SPI2->CR1 |= (1<<0);	// CPHA(Clock phase)=1, 두 번째 edge 에서 데이터가 샘플링

        SPI2->CR1 |= (1<<6);	// SPE=1, SPI2 Enable 
}

void Fram_Latch(char command)
{
	FRAM_CS_LOW;
	SPI2_Send(command);
	FRAM_CS_HIGH; 
}

// Reset Status Reg.
void Fram_Status_Config(void)
{
	uint8_t WriteBuf = 0x00;
	uint8_t data = 0;
	char command = WRSR;
  
	FRAM_WP_HIGH;  
    
	FRAM_CS_LOW;
	SPI2_Send(command);	// Write Status Reg.
	SPI2_Send(WriteBuf);	// Clear Status Reg. 
	FRAM_CS_HIGH;
 
	data = RDSR;
 
	FRAM_CS_LOW;
	SPI2_Send(data);	// Read Status Reg,
	FRAM_CS_HIGH;
}

void Fram_Write(short addr,unsigned char data)
{
	uint8_t Data[4];
  
	Fram_Latch(WREN);
  
	Data[0] = WRITE;
	Data[1] = addr>>8;
	Data[2] = addr&0xff;
	Data[3] = data;
  
	FRAM_CS_LOW;
	SPI2_Send(Data[0]);
	SPI2_Send(Data[1]);
	SPI2_Send(Data[2]);
	SPI2_Send(Data[3]);
	FRAM_CS_HIGH;
}

unsigned char Fram_Read(short addr)
{
	uint8_t data[3],read_data; 
	Fram_Latch(WRDI);
  
	data[0] = READ;
	data[1] = addr>>8;
	data[2] = addr&0xff;
  
	FRAM_CS_LOW;
	SPI2_Send(data[0]);
	SPI2_Send(data[1]);
	SPI2_Send(data[2]);
	read_data = SPI2_Send(0xff);
	FRAM_CS_HIGH;
  
	return read_data;
}


