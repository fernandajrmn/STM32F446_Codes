#include "stm32f446xx.h"
#include <stdint.h>

/*
  UART2 Polling example (USART2) on STM32F446RE
  Pins:
    PA2 = USART2_TX (AF7)
    PA3 = USART2_RX (AF7)

  Format: 115200, 8N1, oversampling by 16
*/


char c;
static void UART2_GPIO_Init(void);
static void UART2_Init(void);
static void UART2_WriteByte(uint8_t b);
static uint8_t UART2_ReadByte_Blocking(void);
static void UART2_WriteString(const char *s);

int main(void)
{

    UART2_GPIO_Init();
    UART2_Init();
    UART2_WriteString("UART2 polling ready.\r\nType something:\r\n");

    while (1)
    {
        c = UART2_ReadByte_Blocking();  // waits for RXNE
        UART2_WriteByte(c);             // echo back

    }
}

static void UART2_GPIO_Init(void)
{
    // 1) Enable GPIOA clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 2) PA2, PA3 -> Alternate Function mode (MODER = 10b)
    GPIOA->MODER &= ~((3u << (2u*2u)) | (3u << (3u*2u)));
    GPIOA->MODER |=  ((2u << (2u*2u)) | (2u << (3u*2u)));

    // 3) Select AF7 for PA2, PA3 (AFRL because pins 0..7)
    GPIOA->AFR[0] &= ~((0xFu << (2u*4u)) | (0xFu << (3u*4u)));
    GPIOA->AFR[0] |=  ((7u   << (2u*4u)) | (7u   << (3u*4u)));

}

static void UART2_Init(void)
{
    // 1) Enable USART2 clock on APB1
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // 2) Disable USART before config (UE=0)
    USART2->CR1 &= ~USART_CR1_UE;

    // 3) 8N1, oversampling by 16
    //    M=0 (8 data), PCE=0 (no parity), STOP=00 (1 stop), OVER8=0
    USART2->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_OVER8);
    USART2->CR2 &= ~(USART_CR2_STOP);

    // 4) Baud rate (OVER8=0 => fraction is 4 bits)
    //    BRR = (div_integer << 4) | (div_fraction)
    USART2->BRR  = (8 << 4) | (11); // for 16 MHz PCLK1 and 115200 baud
    //USART2->BRR  = (24 << 4) | (7); // for 45 MHz PCLK1 and 115200 baud

    // 5) Enable TX and RX
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

    // 6) Enable USART
    USART2->CR1 |= USART_CR1_UE;
}

static void UART2_WriteByte(uint8_t b)
{
    // Wait until DR is empty (TXE=1)
    while ((USART2->SR & USART_SR_TXE) == 0);

    // Write byte to data register
    USART2->DR = b;

    // Optional: wait until complete (TC=1) if you need "fully sent"
    // while ((USART2->SR & USART_SR_TC) == 0);
}

static uint8_t UART2_ReadByte_Blocking(void)
{
    // Wait until a byte is received (RXNE=1)
    while ((USART2->SR & USART_SR_RXNE) == 0);

    // Reading DR clears RXNE
    return (uint8_t)(USART2->DR & 0xFFu);
}

static void UART2_WriteString(const char *s)
{
    while (*s)
    {
        UART2_WriteByte((uint8_t)*s++);
    }
}
