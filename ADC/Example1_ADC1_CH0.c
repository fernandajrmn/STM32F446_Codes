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

/* ----------------------------- User knobs ------------------------------ */
#define VREF_VOLTS      3.3f     // Adjust if you measure a different VDDA/VREF+
#define ADC_TIMEOUT     2000000u // Simple safety timeout for polling loops

/* -------------------------- Step-by-step API --------------------------- */
static void GPIO_PA0_Analog_Init(void);
static void ADC1_CommonClock_Init(void);
static void ADC1_RegularSingleChannel_Init_CH0(void);
static uint16_t ADC1_Read_CH0_Polling(void);

/* Optional helper */
static float ADC_CodeToVolts(uint16_t code);

/* ------------------------------ Main ----------------------------------- */
int main(void)
{
    /* Step 1A: Enable GPIOA clock + configure PA0 as analog */
    GPIO_PA0_Analog_Init();

    /* Step 3: Configure ADC common clock prescaler (ADCCLK) */
    ADC1_CommonClock_Init();

    /* Steps 4–7: Configure ADC1 for Regular, CH0, single conversion, polling */
    ADC1_RegularSingleChannel_Init_CH0();

    while (1)
    {
        /* Step 8: Start conversion + poll EOC + read DR */
        uint16_t adc_raw = ADC1_Read_CH0_Polling();
        float vin = ADC_CodeToVolts(adc_raw);

        (void)vin; // Place breakpoint here to watch adc_raw and vin
    }
}

/* ---------------------------------------------------------------------- */
/* Step 2: GPIO setup                                                     */
/* ---------------------------------------------------------------------- */
static void GPIO_PA0_Analog_Init(void)
{
    /* Step 1A: Enable GPIOA clock (AHB1) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR; // readback to ensure clock is on before config

    /* Step 2: Configure PA0 as Analog mode: MODER0 = 11 */
    GPIOA->MODER &= ~(3u << (0u * 2u));
    GPIOA->MODER |=  (3u << (0u * 2u));

    /* Optional good practice: no pull-up / pull-down on analog pin */
    GPIOA->PUPDR &= ~(3u << (0u * 2u));
}

/* ---------------------------------------------------------------------- */
/* Step 3: ADC common clock (ADCCLK prescaler)                             */
/* ---------------------------------------------------------------------- */
static void ADC1_CommonClock_Init(void)
{
    /* Step 1B: Enable ADC1 clock (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    /*
     * Step 3: Set ADCCLK = PCLK2 / prescaler using ADC common register ADC->CCR.
     * ADCPRE encoding (F4 family):
     *   00: /2
     *   01: /4
     *   10: /6
     *   11: /8
     *
     * For a first lab demo, /4 is a safe, stable default.
     */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  ADC_CCR_ADCPRE_0; // 01 => /4
}

/* ---------------------------------------------------------------------- */
/* Steps 4–7: ADC1 configuration for Example A                              */
/* ---------------------------------------------------------------------- */
static void ADC1_RegularSingleChannel_Init_CH0(void)
{
    /* Step 4: Make sure ADC is off before configuring (ADON = 0) */
    ADC1->CR2 &= ~ADC_CR2_ADON;

    /* Small delay (optional) to ensure proper power-down before reconfig */
    for (volatile uint32_t i = 0; i < 1000; i++) { __NOP(); }

    /* Step 5: Sampling time for channel 0 (PA0/IN0) in SMPR2.SMP0[2:0]
     * Encoding:
     *  000=3  cycles
     *  001=15 cycles
     *  010=28 cycles
     *  011=56 cycles
     *  100=84 cycles
     *  101=112 cycles
     *  110=144 cycles
     *  111=480 cycles
     *
     * Use 56 cycles as a robust “first example” default.
     */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;
    ADC1->SMPR2 |=  (3u << ADC_SMPR2_SMP0_Pos); // 0b011 => 56 cycles

    /* Step 6A: Regular sequence length = 1 conversion => SQR1.L = 0 */
    ADC1->SQR1 &= ~ADC_SQR1_L;

    /* Step 6B: Regular rank 1 channel = 0 => SQR3.SQ1 = 0 */
    ADC1->SQR3 &= ~ADC_SQR3_SQ1; // channel 0 already encoded as 0

    /* Step 7: ADC control settings (CR1/CR2)
     * - Regular group
     * - Single conversion: CONT = 0
     * - Software trigger: EXTEN = 00 (disabled), use SWSTART
     * - EOCS = 1: EOC set at end of each conversion (simple for polling)
     * - Right alignment default
     * - 12-bit resolution default (CR1.RES = 00)
     */
    ADC1->CR1 = 0; // SCAN=0, RES=00 (12-bit), no interrupts here

    ADC1->CR2 = 0;
    ADC1->CR2 |= ADC_CR2_EOCS;       // EOC after each conversion (good for polling)
    ADC1->CR2 &= ~ADC_CR2_CONT;      // single conversion
    ADC1->CR2 &= ~ADC_CR2_EXTEN;     // no external trigger
    /* ADC_CR2.ALIGN = 0 => right alignment (default) */

    /* Step 4 (final): Enable ADC (ADON=1) */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* Optional short startup delay */
    for (volatile uint32_t i = 0; i < 1000; i++) { __NOP(); }
}

/* ---------------------------------------------------------------------- */
/* Step 8: Start conversion + Poll EOC + Read DR                            */
/* ---------------------------------------------------------------------- */
static uint16_t ADC1_Read_CH0_Polling(void)
{
    /* Clear potential stale flags by reading SR (and DR if needed) */
    (void)ADC1->SR;
    (void)ADC1->DR;

    /* Start regular conversion: SWSTART = 1 */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* Poll EOC */
    uint32_t timeout = ADC_TIMEOUT;
    while (((ADC1->SR & ADC_SR_EOC) == 0u) && timeout--)
    {
        /* busy-wait */
    }

    /* If timeout hit, return 0 as a safe default (you can assert instead) */
    if (timeout == 0u)
    {
        return 0u;
    }

    /* Read DR:
     * - returns the converted result
     * - also clears EOC (common handshake behavior)
     */
    uint16_t result = (uint16_t)(ADC1->DR & 0xFFFFu);

    /* Optional: if OVR set, clear it by software (reading DR may not be enough in every scenario) */
    if (ADC1->SR & ADC_SR_OVR)
    {
        ADC1->SR &= ~ADC_SR_OVR;
    }

    return result;
}

/* ---------------------------------------------------------------------- */
/* Optional helper: convert raw ADC code to volts                           */
/* ---------------------------------------------------------------------- */
static float ADC_CodeToVolts(uint16_t code)
{
    /* 12-bit right-aligned: 0..4095 */
    return ((float)code * VREF_VOLTS) / 4095.0f;
}
