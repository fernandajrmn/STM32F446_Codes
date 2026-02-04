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



int main (void) {

    /* Ejemplo de uso de delay_ms */
    while (1)
    {
        // Hacer algo
        delay_ms(5000);  // Esperar 5 segundos
    }
}

