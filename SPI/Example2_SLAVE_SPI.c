#include "stm32f446xx.h"
#include <stdint.h>

/* =========================
   SPI1 SLAVE pins (AF5)
   PA4 = NSS
   PA5 = SCK
   PA6 = MISO
   PA7 = MOSI
   ========================= */



/* Step 1: clocks */
static void SPI1_Clocks_Enable(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // GPIOA for SPI1 pins
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;    // SPI1 clock

}

/* Step 2: GPIO AF5 for PA4..PA7 */
static void SPI1_GPIO_Init_Slave(void)
{
    /* MODER: AF (10) for PA4,5,6,7 */
    GPIOA->MODER &= ~((3u << (4u*2u)) | (3u << (5u*2u)) | (3u << (6u*2u)) | (3u << (7u*2u)));
    GPIOA->MODER |=  ((2u << (4u*2u)) | (2u << (5u*2u)) | (2u << (6u*2u)) | (2u << (7u*2u)));

    /* AFRL: AF5 for PA4..PA7 */
    GPIOA->AFR[0] &= ~((0xFu << (4u*4u)) | (0xFu << (5u*4u)) | (0xFu << (6u*4u)) | (0xFu << (7u*4u)));
    GPIOA->AFR[0] |=  ((5u   << (4u*4u)) | (5u   << (5u*4u)) | (5u   << (6u*4u)) | (5u   << (7u*4u)));

    /* Speed: high */
    GPIOA->OSPEEDR |= (3u << (4u*2u)) | (3u << (5u*2u)) | (3u << (6u*2u)) | (3u << (7u*2u));

    /* Push-pull for outputs (SCK/MOSI are inputs in slave, MISO is output from slave) */
    GPIOA->OTYPER &= ~(1u << 6u);  // MISO push-pull

    /* Pulls:
       - NSS: pull-up recommended (idle high when not selected)
       - SCK/MOSI/MISO: usually no pull
    */
    GPIOA->PUPDR &= ~((3u << (4u*2u)) | (3u << (5u*2u)) | (3u << (6u*2u)) | (3u << (7u*2u)));
    GPIOA->PUPDR |=  (1u << (4u*2u));   // PA4 pull-up (01)
}

/* Step 3-6: SPI1 SLAVE config (Mode 0, HW NSS, polling) */
static void SPI1_Init_Slave_Polling_Mode0_HWNSS(void)
{
    /* Disable SPI */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /* CR1:
       - MSTR=0 (slave)
       - CPOL=0, CPHA=0 (Mode 0)
       - DFF=0 (8-bit)
       - LSBFIRST=0 (MSB first)
       - SSM=0 (hardware NSS)
       - BIDIMODE=0, RXONLY=0 (full-duplex)
    */
    SPI1->CR1 = 0;
    SPI1->CR1 &= ~(SPI_CR1_MSTR);
    SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI1->CR1 &= ~(SPI_CR1_DFF | SPI_CR1_LSBFIRST);
    SPI1->CR1 &= ~(SPI_CR1_SSM);
    SPI1->CR1 &= ~(SPI_CR1_BIDIMODE | SPI_CR1_RXONLY);

    /* CR2: Motorola frame format (FRF=0), no IRQ/DMA */
    SPI1->CR2 = 0;

    /* Enable SPI (do this BEFORE master starts clocking) */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* 8-bit safe DR access */
static inline void SPI1_WriteDR(uint8_t v)
{
    SPI1->DR = v;
}
static inline uint8_t SPI1_ReadDR(void)
{
    return SPI1->DR;
}

/* Clear OVR if it happens: read DR then SR */
static void SPI1_ClearOVR(void)
{
    (void)SPI1->DR;
    (void)SPI1->SR;
}

/* Optional: wait until selected (NSS low) by reading the NSS pin level */
static void WaitFor_NSS_Low(void)
{
    while (GPIOA->IDR & (1u << 4u)) { }  // PA4 high = not selected
}
static void WaitFor_NSS_High(void)
{
    while ((GPIOA->IDR & (1u << 4u)) == 0) { } // PA4 low = selected
}

int main(void)
{
    SPI1_Clocks_Enable();
    SPI1_GPIO_Init_Slave();
    SPI1_Init_Slave_Polling_Mode0_HWNSS();

    /* Example B fixed response bytes */
    const uint8_t tx0 = 0x3Cu;
    const uint8_t tx1 = 0xC3u;

    while (1)
    {
        /* ---- Session start (master pulls NSS low) ---- */
        WaitFor_NSS_Low();

        /* Preload first response BEFORE the first clocks (best practice) */
        while ((SPI1->SR & SPI_SR_TXE) == 0) { }
        SPI1_WriteDR(tx0);

        /* Receive byte #1 (master clocks it) */
        while ((SPI1->SR & SPI_SR_RXNE) == 0) { }
        uint8_t rx0 = SPI1_ReadDR();   (void)rx0;

        /* Load response #2 as soon as TXE allows */
        while ((SPI1->SR & SPI_SR_TXE) == 0) { }
        SPI1_WriteDR(tx1);

        /* Receive byte #2 */
        while ((SPI1->SR & SPI_SR_RXNE) == 0) { }
        uint8_t rx1 = SPI1_ReadDR();   (void)rx1;

        /* If overrun happened (debug safety) */
        if (SPI1->SR & SPI_SR_OVR)
        {
            SPI1_ClearOVR();
        }

        /* Wait end-of-session (master releases NSS high) */
        WaitFor_NSS_High();

        /* Loop back and be ready for next session */
    }
}
