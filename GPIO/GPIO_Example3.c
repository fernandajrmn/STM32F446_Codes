#include "stm32f446xx.h"  // CMSIS device header
#include <stdint.h>
#include "clock.h"


void GPIO_USART2_Init(void)
{
    /* 1. Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Alternate function mode for PA2, PA3 */
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));

    /* 3. High speed */
    GPIOA->OSPEEDR |= ((3U << (2 * 2)) | (3U << (3 * 2)));

    /* 4. Pull-up on RX (PA3) */
    GPIOA->PUPDR &= ~(3U << (3 * 2));
    GPIOA->PUPDR |=  (1U << (3 * 2));

    /* 5. AF7 for USART2 */
    GPIOA->AFR[0] &= ~((0 << (2 * 4)) | (0 << (3 * 4)));
    GPIOA->AFR[0] |=  ((7U << (2 * 4)) | (7U << (3 * 4)));
}



int main(void) {
	GPIO_USART2_Init();

    while (1) {
    }
}
