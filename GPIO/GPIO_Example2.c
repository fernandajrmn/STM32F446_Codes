
#include "stm32f446xx.h"  // CMSIS device header
#include <stdint.h>

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


void GPIO_PC13_Input_Init(void)
{
    /* Enable GPIOC clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* PC13 input mode */
    GPIOC->MODER &= ~(3U << (13 * 2));

    /* Pull-up enabled (button active LOW) */
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR |=  (1U << (13 * 2));
}

void Button_LED_Control(void)
{
    if ((GPIOC->IDR & (1U << 13)) == 0)   // button pressed
    {
        LED_ON();
    }
    else
    {
        LED_OFF();
    }
}


int main(void) {
	GPIO_PA5_Output_Init();
	GPIO_PC13_Input_Init();


    while (1) {
    	Button_LED_Control();

    }
}
