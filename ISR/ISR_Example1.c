#include "stm32f446xx.h"
#include <stdint.h>

/*
  EXTI0 Interrupt Example (PA0 -> EXTI0)
  - PA0: input with pull-up, button to GND
  - PA5: output LED
  - EXTI0 on falling edge
*/

#define LED_PIN     5U     // PA5
#define BTN_PIN     0U     // PA0
#define EXTI_LINE0  0U

static void GPIO_LED_Init_PA5(void);
static void GPIO_Button_Init_PA0_PullUp(void);
static void EXTI0_Interrupt_Init_From_PA0_FallingEdge(void);

int main(void)
{

    GPIO_LED_Init_PA5();
    GPIO_Button_Init_PA0_PullUp();
    EXTI0_Interrupt_Init_From_PA0_FallingEdge();

    while (1)
    {

    }
}

/* ---------------- GPIO ---------------- */

static void GPIO_LED_Init_PA5(void)
{
    // Enable GPIOA clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA5 output mode (MODER5 = 01)
    GPIOA->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOA->MODER |=  (1U << (LED_PIN * 2U));

    // Push-pull (OTYPER5 = 0)
    GPIOA->OTYPER &= ~(1U << LED_PIN);

    // Medium speed (optional)
    GPIOA->OSPEEDR &= ~(3U << (LED_PIN * 2U));
    GPIOA->OSPEEDR |=  (1U << (LED_PIN * 2U));

    // No pull-up/pull-down on LED pin
    GPIOA->PUPDR &= ~(3U << (LED_PIN * 2U));
}

static void GPIO_Button_Init_PA0_PullUp(void)
{
    // Enable GPIOA clock (already enabled in LED init, but safe)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA0 input mode (MODER0 = 00)
    GPIOA->MODER &= ~(3U << (BTN_PIN * 2U));

    // Pull-up on PA0 (PUPDR0 = 01)
    // Button to GND -> released = 1, pressed = 0 -> falling edge when pressed
    GPIOA->PUPDR &= ~(3U << (BTN_PIN * 2U));
    GPIOA->PUPDR |=  (1U << (BTN_PIN * 2U));
}

/* ---------------- EXTI + NVIC ---------------- */

static void EXTI0_Interrupt_Init_From_PA0_FallingEdge(void)
{
    // 1) Enable SYSCFG clock (needed to route GPIO -> EXTI)
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 2) Map EXTI0 line to Port A in SYSCFG_EXTICR1
    // EXTICR1 bits [3:0] select source for EXTI0:
    // 0000 = PA0, 0001 = PB0, 0010 = PC0, ...
    SYSCFG->EXTICR[0] &= ~(0xFU << 0);  // EXTI0 = PA0

    // 3) Configure trigger: falling edge on line 0
    EXTI->RTSR &= ~(1U << EXTI_LINE0); // disable rising
    EXTI->FTSR |=  (1U << EXTI_LINE0); // enable falling

    // 4) Unmask interrupt line 0 (Interrupt mode)
    EXTI->IMR  |=  (1U << EXTI_LINE0);

    // (Optional) ensure event is disabled if you want "interrupt only"
    EXTI->EMR  &= ~(1U << EXTI_LINE0);

    // 5) Clear pending flag before enabling NVIC (important!)
    EXTI->PR = (1U << EXTI_LINE0);

    // 6) NVIC: set priority and enable EXTI0 IRQ
    // Lower number = higher priority
    NVIC_SetPriority(EXTI0_IRQn, 5U);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* ---------------- ISR ---------------- */

void EXTI0_IRQHandler(void)
{
    // Check pending flag for line 0
    if (EXTI->PR & (1U << EXTI_LINE0))
    {
        // Clear pending by writing 1
        EXTI->PR = (1U << EXTI_LINE0);

        // Action: toggle LED on PA5
        GPIOA->ODR ^= (1U << LED_PIN);
    }
}
