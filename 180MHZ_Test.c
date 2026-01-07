#include "stm32f446xx.h"

void SystemClock_Config(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

int main(void)
{
    SystemClock_Config();

    // Habilitar GPIOA (LED en PA5)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~(3 << (5 * 2));
    GPIOA->MODER |=  (1 << (5 * 2)); // PA5 como salida

    // LED blink test
    while (1)
    {
        GPIOA->ODR ^= (1 << 5); // Toggle LED
        delay_ms(5000);          // Esperar 500 ms
    }
}

/* =====================================================
   Configura el sistema a 180 MHz usando HSE (8 MHz)
   ===================================================== */
void SystemClock_Config(void)
{
    // 1. Habilitar HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Activar PWR y setear voltaje alto rendimiento
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS; // VOS = 11

    // 3. Flash: 5 wait states + caches
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;

    // 4. Configurar PLL (M=4, N=180, P=2, Q=4)
    RCC->PLLCFGR = (4 << RCC_PLLCFGR_PLLM_Pos)  |
                   (180 << RCC_PLLCFGR_PLLN_Pos) |
                   (0 << RCC_PLLCFGR_PLLP_Pos)   | // P=2
                   (RCC_PLLCFGR_PLLSRC_HSE)      |
                   (4 << RCC_PLLCFGR_PLLQ_Pos);

    // 5. Encender PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 6. Configurar prescalers
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = /1 (180 MHz)
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // APB1 = /4 (45 MHz)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;  // APB2 = /2 (90 MHz)

    // 7. Cambiar SYSCLK a PLL
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/* =====================================================
   Delay en microsegundos usando TIM2
   ===================================================== */
void delay_us(uint32_t us)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // Habilitar TIM2

    // APB1 = 45 MHz → timer clock = 2×45 MHz = 90 MHz (porque APB1 prescaler != 1)
    // Queremos 1 tick = 1 µs → PSC = 90 - 1
    TIM2->PSC = 90 - 1;
    TIM2->ARR = us;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;

    while (!(TIM2->SR & TIM_SR_UIF)); // Esperar fin
    TIM2->CR1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
}

/* =====================================================
   Delay en milisegundos basado en delay_us
   ===================================================== */
void delay_ms(uint32_t ms)
{
    while (ms--)
        delay_us(1000);
}
