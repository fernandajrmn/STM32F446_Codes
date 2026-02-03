#include "stm32f446xx.h"
#include <stdint.h>

/* =========================
   Pin choices (edit if needed)
   ========================= */
#define CS_PORT      GPIOB
#define CS_PIN       6u

#define CS_LOW()   (CS_PORT->BSRR = (1u << (CS_PIN + 16u)))
#define CS_HIGH()  (CS_PORT->BSRR = (1u <<  CS_PIN))


/* =========================
   Step 1: RCC clocks
   ========================= */
static void SPI1_Clocks_Enable(void)
{
    /* GPIOA for SCK/MISO/MOSI, GPIOB for CS */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* SPI1 is on APB2 */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
}

/* =========================
   Step 2: GPIO AF for SPI pins + CS as output
   SPI1 AF = AF5
   PA5=SCK, PA7=MOSI (PA6=MISO optional)
   ========================= */
static void SPI1_GPIO_Init(void)
{
    /* ---- PA5 (SCK), PA7 (MOSI) as AF5 ---- */
    /* MODER: 10 = Alternate Function */
    GPIOA->MODER &= ~((3u << (5u*2u)) | (3u << (7u*2u)));
    GPIOA->MODER |=  ((2u << (5u*2u)) | (2u << (7u*2u)));

    /* AFRL (pins 0..7): set AF5 (0101) for PA5 and PA7 */
    GPIOA->AFR[0] |=  ((5u   << (5u*4u)) | (5u   << (7u*4u)));

    /* Speed: High/Very High recommended for SCK/MOSI */
    GPIOA->OSPEEDR |= (3u << (5u*2u)) | (3u << (7u*2u));

    /* Output type: push-pull (default 0) */
    GPIOA->OTYPER &= ~((1u << 5u) | (1u << 7u));

    /* Pull-up/down: usually none for SCK/MOSI (depends on wiring) */
    GPIOA->PUPDR &= ~((3u << (5u*2u)) | (3u << (7u*2u)));

    /* ---- PB6 as GPIO output for CS ---- */
    GPIOB->MODER &= ~(3u << (CS_PIN*2u));
    GPIOB->MODER |=  (1u << (CS_PIN*2u));  // 01 = output

    GPIOB->OSPEEDR |= (3u << (CS_PIN*2u)); // fast edge OK for CS
    GPIOB->OTYPER  &= ~(1u << CS_PIN);     // push-pull
    GPIOB->PUPDR   &= ~(3u << (CS_PIN*2u)); // no pull

    CS_HIGH(); // idle high (CS active-low)
}

/* =========================
   Step 3-6: SPI1 config (Polling baseline)
   Mode 0: CPOL=0, CPHA=0
   8-bit, MSB first
   Software NSS: SSM=1, SSI=1
   ========================= */
static void SPI1_Init_Polling(void)
{
    /* Disable SPI before configuring */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /* CR1:
       - MSTR = 1 (master)
       - BR[2:0] = br_bits
       - CPOL=0, CPHA=0 (mode 0)
       - DFF=0 (8-bit)
       - LSBFIRST=0 (MSB first)
       - SSM=1, SSI=1 (software NSS)
    */
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;



    /* =========================
    BR presets for your two clock modes (SPI1 on APB2)
   BR codes: 000=/2, 001=/4, 010=/8, 011=/16, 100=/32, 101=/64, 110=/128, 111=/256
   =========================
  

   If PCLK2 = 16 MHz (default boot): 
   /16 => 1 MHz (BR=0b011 = 3) 
   
   
   If PCLK2 = 90 MHz (typical with SYSCLK=180 and APB2=/2):
   /64 => 1.40625 MHz (BR=0b101 = 5)  [safe bring-up]
   /32 => 2.8125  MHz (BR=0b100 = 4)  [faster, still safe]
    =========================   
    */
    
    SPI1->CR1 |= (5u << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;

    /* Full-duplex default: BIDIMODE=0, RXONLY=0 */
    SPI1->CR1 &= ~(SPI_CR1_BIDIMODE | SPI_CR1_RXONLY);

    /* CR2: polling baseline (no interrupts, no DMA, Motorola FRF=0) */
    SPI1->CR2 = 0;

    /* Enable SPI */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* =========================
   Step 7-9: Polling transfer primitive
   Sends 1 byte and returns the simultaneously received byte.
   ========================= */
static uint8_t SPI1_Transfer8(uint8_t tx)
{
    /* Wait until TXE=1 (transmit buffer empty) */
    while ((SPI1->SR & SPI_SR_TXE) == 0);

    /* Write to DR starts shifting */
    SPI1->DR = tx;

    /* Wait until RXNE=1 (receive buffer not empty) */
    while ((SPI1->SR & SPI_SR_RXNE) == 0);

    /* Read DR clears RXNE */
    uint8_t rx = SPI1->DR;

    return rx;
}

/* Wait until last bits fully sent on the wire (end-of-transfer) */
static void SPI1_WaitEnd(void)
{
    /* Wait TXE=1 then BSY=0 */
    while ((SPI1->SR & SPI_SR_TXE) == 0);
    while ((SPI1->SR & SPI_SR_BSY) != 0);
}


int main(void)
{
    /* 0) Optional: call your SystemClock_Config_180MHz() here if you want that mode.
          If you don't call it, you're at 16 MHz HSI boot.
    */

    SPI1_Clocks_Enable();
    SPI1_GPIO_Init();

    /* Choose one based on your clock mode */
    /* SPI1_Init_Polling(BR_FOR_16MHZ_BOOT); */
    SPI1_Init_Polling();

    while (1)
    {

    }
}
