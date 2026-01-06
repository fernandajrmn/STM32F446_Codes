#include "stm32f446xx.h"  // CMSIS device header
#include <stdint.h>
#include "clock.h"

/* LED control */
#define LED_ON()   (GPIOA->BSRR = (1U << 5))
#define LED_OFF()  (GPIOA->BSRR = (1U << (5 + 16)))



void GPIO_PA5_Output_Init(void)
{
    /* 1. Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Set PA5 as output (MODER5 = 01) */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    /* 3. Push-pull output (OTYPER5 = 0) */
    GPIOA->OTYPER &= ~(1U << 5);

    /* 4. Medium speed (OSPEEDR5 = 01) */
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));
    GPIOA->OSPEEDR |=  (1U << (5 * 2));

    /* 5. No pull-up / pull-down (PUPDR5 = 00) */
    GPIOA->PUPDR &= ~(3U << (5 * 2));
}



void delay_us(uint32_t us)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // Habilitar TIM2

    // APB1 = 45 MHz → timer clock = 2×45 MHz = 90 MHz (porque APB1 prescaler != 1)
    // Queremos 1 tick = 1 µs → PSC = 90 - 1
    TIM2->PSC = 90 - 1;
    TIM2->ARR = us;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;

    while (!(TIM2->SR & TIM_SR_UIF)); // Esperar fin
    TIM2->CR1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
}


int main(void) {
	SystemClock_Config_180MHz();
	GPIO_PA5_Output_Init();


    while (1) {
    	LED_ON();
    	delay_us(5000000);
    	LED_OFF();
    	delay_us(5000000);

    }
}
