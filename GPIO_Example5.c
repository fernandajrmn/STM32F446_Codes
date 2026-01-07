#include "stm32f446xx.h"  // CMSIS device header
#include <stdint.h>

void GPIO_PC13_EXTI_Init(void)
{
    /* 1. Enable GPIOC and SYSCFG clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* 2. Input mode */
    GPIOC->MODER &= ~(3U << (13 * 2));

    /* 3. Pull-up */
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR |=  (1U << (13 * 2));

    /* 4. Map EXTI13 to PC13 */
    SYSCFG->EXTICR[3] &= ~(0xF << 4);
    SYSCFG->EXTICR[3] |=  (0x2 << 4);   // Port C
}


int main(void) {
	GPIO_PC13_EXTI_Init();

    while (1) {

    }
}
