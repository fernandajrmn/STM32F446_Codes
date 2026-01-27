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



void DAC_Configuration() {
	/* STEP 1: Clocks */
	    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // GPIOA clock
	    RCC->APB1ENR |= RCC_APB1ENR_DACEN;     // DAC clock

	    /* STEP 2: PA4 = Analog mode (DAC_OUT1) */
	    GPIOA->MODER &= ~(3u << (4u * 2u));
	    GPIOA->MODER |=  (3u << (4u * 2u));   // 11: analog
	    GPIOA->PUPDR &= ~(3u << (4u * 2u));   // no pull

	    /* STEP 3: DAC channel 1 config */
	    DAC->CR &= ~DAC_CR_EN1;               // (optional) disable while configuring

	    DAC->CR &= ~DAC_CR_TEN1;              // TEN1=0: trigger disabled (software update)
	    DAC->CR &= ~DAC_CR_BOFF1;             // BOFF1=0: output buffer enabled

	    DAC->CR |= DAC_CR_EN1;                // EN1=1: enable DAC ch1
}
int main(void)
{
	TIM6_Init_1ms();
	DAC_Configuration();


    while (1)
    {

    	if (counter > 10000) {
    		counter = 0;
    		DAC->DHR12R1 = 4095; // full-scale

    	} else if (counter > 6000) {
    		DAC->DHR12R1 = 2048; // mid-scale
    	} else if (counter > 3000) {
    		DAC->DHR12R1 = 1024; // low-scale
    	}
        /* You can update DAC->DHR12R1 live in debug and watch PA4 change */
    }
}
