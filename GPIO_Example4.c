#include "stm32f446xx.h"  // CMSIS device header
#include <stdint.h>

void GPIO_TIM3_CH1_Init(void)
{
    /* 1. Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Alternate function mode */
    GPIOA->MODER &= ~(3U << (6 * 2));
    GPIOA->MODER |=  (2U << (6 * 2));

    /* 3. Very high speed */
    GPIOA->OSPEEDR |= (3U << (6 * 2));

    /* 4. No pull-up / pull-down */
    GPIOA->PUPDR &= ~(3U << (6 * 2));

    /* 5. AF2 for TIM3 */
    GPIOA->AFR[0] &= ~(0xF << (6 * 4));
    GPIOA->AFR[0] |=  (2U << (6 * 4));
}



int main(void) {
	GPIO_TIM3_CH1_Init();

    while (1) {

    }
}
