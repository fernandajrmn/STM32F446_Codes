#include "stm32f446xx.h"
#include "clock.h"

void SystemClock_Config_180MHz(void)
{
    // 1. HSE ON (8 MHz de ST-LINK)
    RCC->CR |= RCC_CR_HSEON; //Turning on the HSE (External Oscillator).
    while (!(RCC->CR & RCC_CR_HSERDY)); //(Wait until HSE is ready).

    // 2. Power interface clock y escala de voltaje alta
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; //(Enabling clock for PWR Module on APB1 bus). 
    PWR->CR |= PWR_CR_VOS; // High performance voltage configuration (Scale 1).

    // 3. Configurar Flash: 5 wait states + caches
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS; //Flash cache is enabled for instructions and data, also 5 wait states (wait states) latency is configured. 

    // 4. Configurar PLL (M=4, N=180, P=2, Q=4)
    RCC->PLLCFGR = (4u  << RCC_PLLCFGR_PLLM_Pos) | //VCO source (HSE = 8 MHz) is divided for better accuracy. 8 MHz/4 = 2 MHz. 
                   (180u << RCC_PLLCFGR_PLLN_Pos) | //Input frequency for VOC (2 MHz) is multiplied by 180 = 360 MHz. 
                   (0u   << RCC_PLLCFGR_PLLP_Pos) |  // The output frequency of PLL will be VCO / 2 = 180 MHz. (0 in this register means dividing by 2). 
                   (RCC_PLLCFGR_PLLSRC_HSE) | //PLL source is now configured to HSE. The previous steps only configured how the input frequency will be managed, but did not choose yet which source. 
                   (4u   << RCC_PLLCFGR_PLLQ_Pos); //USB bus will be configured to be at 90 MHz, dividing 360 MHz by 4. 

    // 5. Encender PLL
    RCC->CR |= RCC_CR_PLLON; //PLL module is turned on.
    while (!(RCC->CR & RCC_CR_PLLRDY)); //Wait until PLL is ready. 

    // 6. Configurar prescalers
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB = 180 MHz
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // APB1 = 45 MHz
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;  // APB2 = 90 MHz

    // 7. Cambiar SYSCLK → PLL
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL; //THE SYSCLK is switched to PLL source. 
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); //Waits until PLL is source has been properly connected.
}
