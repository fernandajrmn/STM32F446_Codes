#include "stm32f446xx.h"
#include <stdint.h>

/*
  EXTI0 Event Example (PA0 -> EXTI0 EVENT)
  - PA0: input with pull-up, button to GND
  - PA5: output LED
  - EXTI0 in EVENT mode on falling edge
  - No NVIC, no ISR
  - CPU sleeps with WFE and resumes after event
*/

#define LED_PIN     5U     // PA5
#define BTN_PIN     0U     // PA0
#define EXTI_LINE0  0U

static void GPIO_LED_Init_PA5(void);
static void GPIO_Button_Init_PA0_PullUp(void);
static void EXTI0_Event_Init_From_PA0_FallingEdge(void);

int main(void)
{
    GPIO_LED_Init_PA5();
    GPIO_Button_Init_PA0_PullUp();
    EXTI0_Event_Init_From_PA0_FallingEdge();

    while (1)
    {
        /*
          Wait For Event:
          - CPU sleeps until an event occurs.
          - EXTI in event mode can wake it up without an ISR.
        */
        __WFE();

        /*
          After waking up, you execute normal C code here.
          We can toggle the LED to prove the event happened.
        */
        GPIOA->ODR ^= (1U << LED_PIN);

        /*
          Optional: clear PR to avoid repeated wakeups if PR is still set.
          (Not always required for pure event usage, but it's a good practice
           when you want "one wake per edge" behavior.)
        */
        if (EXTI->PR & (1U << EXTI_LINE0))
        {
            EXTI->PR = (1U << EXTI_LINE0);
        }
    }
}

/* ---------------- GPIO ---------------- */

static void GPIO_LED_Init_PA5(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOA->MODER |=  (1U << (LED_PIN * 2U));  // output

    GPIOA->OTYPER &= ~(1U << LED_PIN);        // push-pull
    GPIOA->PUPDR  &= ~(3U << (LED_PIN * 2U)); // no pull
}

static void GPIO_Button_Init_PA0_PullUp(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (BTN_PIN * 2U));  // input
    GPIOA->PUPDR &= ~(3U << (BTN_PIN * 2U));
    GPIOA->PUPDR |=  (1U << (BTN_PIN * 2U));  // pull-up
}

/* ---------------- EXTI EVENT MODE ---------------- */

static void EXTI0_Event_Init_From_PA0_FallingEdge(void)
{
    // 1) Enable SYSCFG clock (needed for EXTI mapping)
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 2) Map EXTI0 to PA0
    SYSCFG->EXTICR[0] &= ~(0xFU << 0); // 0000 = Port A for EXTI0

    // 3) Select trigger edge: falling edge
    EXTI->RTSR &= ~(1U << EXTI_LINE0);
    EXTI->FTSR |=  (1U << EXTI_LINE0);

    // 4) EVENT mode: unmask event, mask interrupt
    EXTI->EMR |=  (1U << EXTI_LINE0);  // enable event request
    EXTI->IMR &= ~(1U << EXTI_LINE0);  // disable interrupt request (no ISR)

    // 5) Clear any pending flag before entering WFE loop
    EXTI->PR = (1U << EXTI_LINE0);

    // 6) NVIC is NOT enabled (on purpose)
    // NVIC_DisableIRQ(EXTI0_IRQn); // optional explicit disable
}
