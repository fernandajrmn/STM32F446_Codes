#include "stm32f446xx.h"
#include <stdint.h>

volatile uint32_t g_ms_ticks = 0;

#define LED_ON()   (GPIOA->BSRR = (1U << 5)) //Macros defined using register BSSR for pin PA5 to turn on the led. 
#define LED_OFF()  (GPIOA->BSRR = (1U << (5 + 16))) //Macros defined using same register BSSRD for turning off the led. 

static void TIM6_Init_1ms(uint32_t tim6_clk_hz)
{
    // 1) Enable TIM6 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // 2) Configure PSC to get 1 MHz counter tick
    //    fCNT = fTIM6 / (PSC+1) => PSC = (fTIM6 / 1e6) - 1
    TIM6->PSC = (uint16_t)((tim6_clk_hz / 1000000UL) - 1UL);

    // 3) ARR for 1 ms: 1000 ticks @ 1 MHz -> ARR = 1000-1
    TIM6->ARR = 1000 - 1;

    // 4) Enable update interrupt
    TIM6->DIER |= TIM_DIER_UIE;

    // 5) Clear pending update flag (good practice)
    TIM6->SR &= ~TIM_SR_UIF;

    // 6) Enable NVIC for TIM6_DAC
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    // 7) Start timer
    TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)        // Update event?
    {
        TIM6->SR &= ~TIM_SR_UIF;      // Clear flag (must do!)
        g_ms_ticks++;                 // 1 ms “system tick”
        // put your periodic task here
    }

    // If you ever use DAC underrun interrupts, you’d also check/clear DAC flags here.
}


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

int main(void) {
    TIM6_Init_1ms(16000000);
    GPIO_PA5_Output_Init();



    while (1) {
        if (g_ms_ticks >3000) {
            LED_ON();
        } else if (g_ms_ticks >6000) {
            LED_OFF();
            g_ms_ticks = 0;
        }

    }
}