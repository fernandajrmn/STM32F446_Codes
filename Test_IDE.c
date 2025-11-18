#include "stm32f446xx.h"
int main(void)
{
    // Habilitar reloj para GPIOA y GPIOC
    RCC->AHB1ENR |= (1 << 0); // GPIOA
    RCC->AHB1ENR |= (1 << 2); // GPIOC

    // PA5 como salida (LED)
    GPIOA->MODER &= ~(3 << (5 * 2));
    GPIOA->MODER |=  (1 << (5 * 2));

    // PC13 como entrada (Botón)
    GPIOC->MODER &= ~(3 << (13 * 2));

    while (1)
    {
        if (!(GPIOC->IDR & (1 << 13))) // Botón presionado (activo bajo)
            GPIOA->ODR |= (1 << 5);    // Enciende LED
        else
            GPIOA->ODR &= ~(1 << 5);   // Apaga LED
    }
}
