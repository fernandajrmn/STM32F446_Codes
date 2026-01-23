/**
 * Example A (STM32F446RE) — ADC Regular Group, Single Channel, Single Conversion, Polling
 *
 * Goal:
 *  - Read an analog voltage on PA0 (ADC1_IN0)
 *  - Regular group
 *  - Single conversion (CONT = 0)
 *  - Software start (SWSTART)
 *  - Poll EOC, then read DR (reading DR clears EOC)
 *
 * Wiring (recommended):
 *  - Potentiometer end pins: 3.3V and GND
 *  - Potentiometer wiper: PA0
 *
 * Notes:
 *  - ADC input voltage must stay within [VREF- .. VREF+]. On Nucleo, VREF+ is typically ~3.3V.
 *  - This is register-level (no HAL).
 */
#include "stm32f446xx.h"
#include <stdint.h>

static inline void FPU_Enable(void)
{
    SCB->CPACR |= (0xFu << 20);  // Full access to CP10 and CP11
    __DSB();
    __ISB();
}

volatile uint16_t adc_raw = 0;
volatile float adc_voltage = 0;

int main(void)
{
	FPU_Enable();

    /* 1) GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2) PA0 as Analog mode (MODER0 = 11) */
    GPIOA->MODER &= ~(3u << (0u * 2u));
    GPIOA->MODER |=  (3u << (0u * 2u));
    GPIOA->PUPDR &= ~(3u << (0u * 2u));   // no pull

    /* 3) ADC1 clock (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* 4) ADC common prescaler: ADCCLK = PCLK2 / 4 (safe default)
          00:/2  01:/4  10:/6  11:/8
    */
   //For 16 MHz, the recommended prescaler is /2 (ADCCLK=8 MHz). Use /4 (ADCCLK=4 MHz)for higher precision 
   //For 16 MHz, the recommended prescaler is /4 (ADCCLK=22.5 MHz). Use /6 (ADCCLK=15 MHz)for higher precision 
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  ADC_CCR_ADCPRE_0;        // /4

    /* 5) Ensure ADC OFF before config */
    ADC1->CR2 &= ~ADC_CR2_ADON;

    /* 6) Sampling time for channel 0: SMP0 = 56 cycles (robust default) */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;
    ADC1->SMPR2 |=  (3u << ADC_SMPR2_SMP0_Pos);   // 0b011 = 56 cycles

    /* 7) Regular sequence length = 1 conversion => L=0 */
    ADC1->SQR1 &= ~ADC_SQR1_L;

    /* 8) Rank 1 channel = IN0 => SQ1 = 0 (PA0) */
    ADC1->SQR3 &= ~ADC_SQR3_SQ1;          // channel 0 encoded as 0

    /* 9) Control: single conversion, no external trigger, EOC after each conversion */
    ADC1->CR1 = 0;
    ADC1->CR2 = 0;
    ADC1->CR2 |= ADC_CR2_EOCS;            // EOC at end of each conversion
    /* CONT=0 by default, EXTEN=00 by default */

    /* 10) Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;

    while (1)
    {
        /* Optional: clear stale EOC by reading DR */
        (void)ADC1->SR;
        (void)ADC1->DR;

        /* 11) Start conversion */
        ADC1->CR2 |= ADC_CR2_SWSTART;

        /* 12) Wait until EOC becomes 1 */
        while ((ADC1->SR & ADC_SR_EOC) == 0u) {
            ; // wait
         }

        /* 13) Read result (reading DR clears EOC) */
        adc_raw = (uint16_t)ADC1->DR;
        adc_voltage = adc_raw*3.3f /4095.0f;
    }
}
