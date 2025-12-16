#include "stm32f446xx.h"
#include "clock.h"

void SystemClock_Config_180MHz(void)
{
    // 1. HSE ON (8 MHz de ST-LINK)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Power interface clock y escala de voltaje alta
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS; // High performance

    // 3. Configurar Flash: 5 wait states + caches
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;

    // 4. Configurar PLL (M=4, N=180, P=2, Q=4)
    RCC->PLLCFGR = (4u  << RCC_PLLCFGR_PLLM_Pos) |
                   (180u << RCC_PLLCFGR_PLLN_Pos) |
                   (0u   << RCC_PLLCFGR_PLLP_Pos) |  // P=2
                   (RCC_PLLCFGR_PLLSRC_HSE) |
                   (4u   << RCC_PLLCFGR_PLLQ_Pos);

    // 5. Encender PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 6. Configurar prescalers
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = 180 MHz
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // APB1 = 45 MHz
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;  // APB2 = 90 MHz

    // 7. Cambiar SYSCLK → PLL
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
