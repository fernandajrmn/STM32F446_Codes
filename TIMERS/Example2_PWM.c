#include "stm32f446xx.h"
#include <stdint.h>

#define PWM_TICK_HZ   1000000UL   // 1 MHz internal timer tick (1 us per tick)
#define PWM_FREQ_HZ   1000UL      // 1 kHz PWM
#define PWM_ARR       ((PWM_TICK_HZ / PWM_FREQ_HZ) - 1UL)  // 999


int cutting(int x, int lo, int hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}


/**
 * TIM3 PWM on PB4 (TIM3_CH1), 1 kHz, duty in ticks (0..ARR).
 * - PWM Mode 1
 * - Active-high
 * - Preload enabled (ARPE + OC1PE)
 */
void TIM3_PWM_PB4_CH1_Init_1kHz(uint32_t tim3_clk_hz, uint16_t duty_ticks)
{
    /* ---------- 1) Enable clocks ---------- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   // GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;    // TIM3 clock

    /* ---------- 2) Configure PB4 as AF2 (TIM3_CH1) ---------- */
    // PB4 -> Alternate function mode (MODER4 = 10)
    GPIOB->MODER &= ~(3U << (4U * 2U));
    GPIOB->MODER |=  (2U << (4U * 2U));

    // Push-pull (default), but we can explicitly set:
    GPIOB->OTYPER &= ~(1U << 4U);

    // Optional: high speed for cleaner edges
    GPIOB->OSPEEDR &= ~(3U << (4U * 2U));
    GPIOB->OSPEEDR |=  (2U << (4U * 2U)); // 10 = high speed

    // No pull-up/down (optional)
    GPIOB->PUPDR &= ~(3U << (4U * 2U));

    // PB4 AF2: AFRL[19:16] = 0b0010
    GPIOB->AFR[0] &= ~(0xFU << (4U * 4U));
    GPIOB->AFR[0] |=  (0x2U << (4U * 4U));

    /* ---------- 3) Stop timer while configuring ---------- */
    TIM3->CR1 &= ~TIM_CR1_CEN;

    /* ---------- 4) Set time base: PSC and ARR for 1 kHz ---------- */
    // We want fCNT = 1 MHz => PSC = (fTIM3 / 1e6) - 1
    uint32_t psc = (tim3_clk_hz / PWM_TICK_HZ);
    if (psc == 0) psc = 1; // avoid underflow if someone passes tiny clock
    TIM3->PSC = (uint16_t)(psc - 1UL);

    TIM3->ARR = (uint16_t)PWM_ARR;

    // Enable ARR preload (buffered ARR)
    TIM3->CR1 |= TIM_CR1_ARPE;

    /* ---------- 5) Configure Channel 1 as PWM Mode 1 ---------- */
    // CCMR1:
    // - CC1S = 00 (output mode)
    // - OC1M = 110 (PWM mode 1)
    // - OC1PE = 1 (CCR1 preload enable)
    TIM3->CCMR1 &= ~(
        TIM_CCMR1_CC1S   |   // clear CC1S bits
        TIM_CCMR1_OC1M   |   // clear OC1M bits
        TIM_CCMR1_OC1PE      // clear OC1PE
    );

    TIM3->CCMR1 |= (
        (6U << TIM_CCMR1_OC1M_Pos) | // PWM mode 1 (110)
        TIM_CCMR1_OC1PE            // preload enable for CCR1
    );

    /* ---------- 6) Set duty (ticks) ---------- */
    // Valid range: 0..ARR+1 (practically 0..ARR for clean duty math)
    uint16_t max_ticks = (uint16_t)(TIM3->ARR + 1U);
    duty_ticks = cutting(duty_ticks, 0, max_ticks);
    TIM3->CCR1 = duty_ticks;

    /* ---------- 7) Enable channel output ---------- */
    // CCER:
    // - CC1E = 1 enable output
    // - CC1P = 0 active-high
    TIM3->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP); // ensure active high
    TIM3->CCER |=  TIM_CCER_CC1E;

    /* ---------- 8) Force update event to load preloads ---------- */
    TIM3->EGR |= TIM_EGR_UG;

    /* ---------- 9) Start timer ---------- */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/**
 * Update duty in ticks (0..ARR+1). With OC1PE=1 this updates cleanly
 * at the next update event (no mid-cycle glitch).
 */
void TIM3_PWM_SetDutyTicks(uint16_t duty_ticks)
{
    uint16_t max_ticks = (uint16_t)(TIM3->ARR + 1U);
    if (duty_ticks > max_ticks) duty_ticks = max_ticks;
    TIM3->CCR1 = duty_ticks;
}


int main(void)
{
    // Example: if your TIM3 clock real is 90 MHz:
    TIM3_PWM_PB4_CH1_Init_1kHz(90000000UL, 500); // ~50% with ARR=999

    while (1)
    {
        // // example: change duty
        // TIM3_PWM_SetDutyTicks(100);  // ~10%
        // for (volatile uint32_t i=0; i<800000; i++);
        // TIM3_PWM_SetDutyTicks(900);  // ~90%
        // for (volatile uint32_t i=0; i<800000; i++);
    }
}