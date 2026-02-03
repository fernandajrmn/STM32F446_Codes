#include "stm32f446xx.h"
#include <stdint.h>

/* =========================
   Pin choices (edit if needed)
   ========================= */
#define CS_PORT      GPIOB
#define CS_PIN       6u

#define CS_LOW()   (CS_PORT->BSRR = (1u << (CS_PIN + 16u)))
#define CS_HIGH()  (CS_PORT->BSRR = (1u <<  CS_PIN))

/* MAX7219 registers */
#define MAX7219_REG_NOOP        0x00u
#define MAX7219_REG_DIGIT0      0x01u  // row 0
#define MAX7219_REG_DIGIT7      0x08u  // row 7
#define MAX7219_REG_DECODE      0x09u
#define MAX7219_REG_INTENSITY   0x0Au
#define MAX7219_REG_SCANLIMIT   0x0Bu
#define MAX7219_REG_SHUTDOWN    0x0Cu
#define MAX7219_REG_DISPLAYTEST 0x0Fu




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
static void SPI1_Init_Polling(uint32_t br_bits /* 0..7 for BR[2:0] */)
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
    SPI1->CR1 |= (uint16_t)((br_bits & 0x7u) << SPI_CR1_BR_Pos);
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
   (For MAX7219 we'll ignore the return.)
   ========================= */
static uint8_t SPI1_Transfer8(uint8_t tx)
{
    /* Wait until TXE=1 (transmit buffer empty) */
    while ((SPI1->SR & SPI_SR_TXE) == 0) { }

    /* Write to DR starts shifting */
    *(__IO uint8_t *)&SPI1->DR = tx;

    /* Wait until RXNE=1 (receive buffer not empty) */
    while ((SPI1->SR & SPI_SR_RXNE) == 0) { }

    /* Read DR clears RXNE */
    uint8_t rx = *(__IO uint8_t *)&SPI1->DR;

    return rx;
}

/* Wait until last bits fully sent on the wire (end-of-transfer) */
static void SPI1_WaitEnd(void)
{
    /* Wait TXE=1 then BSY=0 */
    while ((SPI1->SR & SPI_SR_TXE) == 0) { }
    while ((SPI1->SR & SPI_SR_BSY) != 0) { }
}

/* =========================
   MAX7219 transaction: 16-bit frame [addr][data]
   CS low -> shift 16 bits -> wait end -> CS high (latch)
   ========================= */
static void MAX7219_Write(uint8_t reg, uint8_t data)
{
    CS_LOW();

    (void)SPI1_Transfer8(reg);
    (void)SPI1_Transfer8(data);

    SPI1_WaitEnd();
    CS_HIGH();

    /* Small hold time (usually not required, but harmless) */
    delay_cycles(200);
}

/* =========================
   MAX7219 init for 8x8 matrix
   - decode off
   - scan all 8 digits (rows)
   - intensity medium
   - normal operation (shutdown=1)
   ========================= */
static void MAX7219_Init(void)
{
    /* Recommended startup sequence */
    MAX7219_Write(MAX7219_REG_DISPLAYTEST, 0x00); // test off
    MAX7219_Write(MAX7219_REG_DECODE,      0x00); // no decode for matrix
    MAX7219_Write(MAX7219_REG_SCANLIMIT,   0x07); // digits 0..7
    MAX7219_Write(MAX7219_REG_INTENSITY,   0x08); // 0x00..0x0F
    MAX7219_Write(MAX7219_REG_SHUTDOWN,    0x01); // normal operation

    /* Clear all rows */
    for (uint8_t r = 0; r < 8; r++)
    {
        MAX7219_Write((uint8_t)(MAX7219_REG_DIGIT0 + r), 0x00);
    }
}

/* Example: draw an X pattern on 8x8 */
static void MAX7219_DrawX(void)
{
    static const uint8_t rows[8] = {
        0b10000001,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00011000,
        0b00100100,
        0b01000010,
        0b10000001
    };

    for (uint8_t r = 0; r < 8; r++)
    {
        MAX7219_Write((uint8_t)(MAX7219_REG_DIGIT0 + r), rows[r]);
    }
}

/* =========================
   BR presets for your two clock modes (SPI1 on APB2)
   BR codes: 000=/2, 001=/4, 010=/8, 011=/16, 100=/32, 101=/64, 110=/128, 111=/256
   ========================= */

/* If PCLK2 = 16 MHz (default boot):
   /16 => 1 MHz (BR=0b011 = 3)
*/
#define BR_FOR_16MHZ_BOOT   3u

/* If PCLK2 = 90 MHz (typical with SYSCLK=180 and APB2=/2):
   /64 => 1.40625 MHz (BR=0b101 = 5)  [safe bring-up]
   /32 => 2.8125  MHz (BR=0b100 = 4)  [faster, still under MAX7219 10MHz]
*/
#define BR_FOR_180MHZ_SAFE  5u
/* #define BR_FOR_180MHZ_FAST  4u */

int main(void)
{
    /* 0) Optional: call your SystemClock_Config_180MHz() here if you want that mode.
          If you don't call it, you're likely at 16 MHz HSI boot.
    */

    SPI1_Clocks_Enable();
    SPI1_GPIO_Init();

    /* Choose one based on your clock mode */
    /* SPI1_Init_Polling(BR_FOR_16MHZ_BOOT); */
    SPI1_Init_Polling(BR_FOR_180MHZ_SAFE);

    MAX7219_Init();

    while (1)
    {
        MAX7219_DrawX();
        delay_cycles(3000000);

        /* clear */
        for (uint8_t r = 0; r < 8; r++)
            MAX7219_Write((uint8_t)(MAX7219_REG_DIGIT0 + r), 0x00);

        delay_cycles(3000000);
    }
}
