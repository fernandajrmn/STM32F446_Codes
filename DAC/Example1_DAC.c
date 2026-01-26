#include "stm32f446xx.h"
#include <stdint.h>

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles--) { __NOP(); }
}

int main(void)
{
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

    /* tWAKEUP: give it a few microseconds.
       At 16 MHz, 200 cycles ~ 12.5 us (safe). */
    delay_cycles(200);

    /* STEP 4: Write 12-bit value */
    uint16_t code = 2048;                 // mid-scale
    DAC->DHR12R1 = (code & 0x0FFF);

    while (1)
    {
        /* You can update DAC->DHR12R1 live in debug and watch PA4 change */
    }
}
