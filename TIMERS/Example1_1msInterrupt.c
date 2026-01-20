#include "stm32f446xx.h"
#include <stdint.h>

volatile uint32_t counter = 0;

static void TIM6_Init_1ms(void)
{
    // 1) Enable TIM6 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    // 2) Configure PSC to get 1 MHz counter tick
    //    fCNT = fTIM6 / (PSC+1) => PSC = (fTIM6 / 1e6) - 1
    TIM6->PSC = 16-1; // for 16 MHz clock
    //TIM6->PSC = 180-1; // for 180 MHz clock

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
        counter++;                 // 1 ms “system tick”
        // Periodic task here
    }

    // If you ever use DAC underrun interrupts, you’d also check/clear DAC flags here.
}

int main(void) {
    TIM6_Init_1ms();


    while (1) {

        // Main loop
    }
}