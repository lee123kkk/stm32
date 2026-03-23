#include "stm32f4xx.h"

// GPIO 포트 주소 정의
#define GPIOA ((GPIO_TypeDef *)0x40020000)
#define GPIOB ((GPIO_TypeDef *)0x40020400)
#define GPIOC ((GPIO_TypeDef *)0x40020800)
#define GPIOD ((GPIO_TypeDef *)0x40020C00)
#define GPIOE ((GPIO_TypeDef *)0x40021000)
#define GPIOF ((GPIO_TypeDef *)0x40021400)
#define GPIOG ((GPIO_TypeDef *)0x40021800)
#define GPIOH ((GPIO_TypeDef *)0x40021C00)
#define GPIOI ((GPIO_TypeDef *)0x40022000)

// GPIO 핀 설정
#define MOTOR1_PWM_PIN GPIO_PIN_6 // PA6
#define MOTOR2_PWM_PIN GPIO_PIN_7 // PA7
#define MOTOR1_DIR_PIN GPIO_PIN_0 // PB0
#define MOTOR2_DIR_PIN GPIO_PIN_1 // PB1
#define TOUCH_SENSOR1_PIN GPIO_PIN_0 // PC0
#define TOUCH_SENSOR2_PIN GPIO_PIN_1 // PC1
#define PRESSURE_SENSOR_PIN GPIO_PIN_0 // PA0
#define ULTRASONIC_TRIG1_PIN GPIO_PIN_8 // PA8
#define ULTRASONIC_TRIG2_PIN GPIO_PIN_9 // PA9
#define ULTRASONIC_ECHO1_PIN GPIO_PIN_6 // PB6
#define ULTRASONIC_ECHO2_PIN GPIO_PIN_7 // PB7
#define IMU_SCL_PIN GPIO_PIN_8 // PB8
#define IMU_SDA_PIN GPIO_PIN_9 // PB9
#define VIBRATION_MOTOR_PIN GPIO_PIN_10 // PA10
#define REED_SWITCH_PIN GPIO_PIN_2 // PC2
#define ENCODER_SENSOR_PIN GPIO_PIN_5 // PA5
#define LED_PIN GPIO_PIN_13 // PG13

volatile uint8_t ledFlag = 0;

void gpio_init(void) {
    // GPIO 클럭 활성화
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOGEN;

    // GPIOG 클럭 활성화
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;

    // GPIOA 설정
    // PA6, PA7: TIM3 PWM
    GPIOA->AFR[0] |= (2 << 24) | (2 << 28); // Alternate Function 2 (TIM3)
    GPIOA->MODER |= (2 << 12) | (2 << 14); // Alternate Function Mode

    // PA0: Analog Input (압력 센서)
    GPIOA->MODER &= ~(3 << 0); // Input Mode

    // PA8, PA9: Output (초음파 Trig)
    GPIOA->MODER |= (1 << 16) | (1 << 18); // Output Mode

    // PA10: Output (진동 모터)
    GPIOA->MODER |= (1 << 20); // Output Mode

    // GPIOB 설정
    // PB0, PB1: Output (모터 방향 제어)
    GPIOB->MODER |= (1 << 0) | (1 << 2); // Output Mode

    // PB6, PB7: Alternate Function (TIM4 Input Capture)
    GPIOB->AFR[0] |= (2 << 24) | (2 << 28); // Alternate Function 2 (TIM4)
    GPIOB->MODER |= (2 << 12) | (2 << 14); // Alternate Function Mode

    // PB8, PB9: Alternate Function (I2C1)
    GPIOB->AFR[0] |= (4 << 16) | (4 << 20); // Alternate Function 4 (I2C1)
    GPIOB->MODER |= (2 << 16) | (2 << 18); // Alternate Function Mode

    // GPIOC 설정
    // PC0, PC1: Input (터치 센서)
    GPIOC->MODER &= ~(3 << 0) & ~(3 << 2); // Input Mode

    // PC2: Input (리드 스위치)
    GPIOC->MODER &= ~(3 << 4); // Input Mode

    // EXTI 설정 (리드 스위치)
    SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~(0x0F << 8)) | (0x02 << 8); // EXTI2 (PC2) 설정
    EXTI->IMR |= EXTI_IMR_MR2; // EXTI2 인터럽트 활성화
    EXTI->FTSR |= EXTI_FTSR_TR2; // 폴링 엣지 설정

    // GPIOG 설정 (LED)
    GPIOG->MODER |= (1 << 26); // PG13을 출력으로 설정
}

void EXTI2_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR2) {
        // 리드 스위치가 작동하면 플래그 설정
        if (GPIOC->IDR & (1 << 2)) {
            ledFlag = 1; // LED 켜기 플래그 설정
        } else {
            ledFlag = 0; // LED 끄기 플래그 설정
        }
        EXTI->PR |= EXTI_PR_PR2; // 플래그 클리어
    }
}

int main(void) {
    // GPIO 초기화
    gpio_init();

    // EXTI 인터럽트 활성화
    NVIC_EnableIRQ(EXTI2_IRQn);

    while (1) {
        if (ledFlag) {
            GPIOG->BSRR = (1 << (13 + 16)); // LED 켜기
        } else {
            GPIOG->BSRR = (1 << 13); // LED 끄기
        }
    }
}