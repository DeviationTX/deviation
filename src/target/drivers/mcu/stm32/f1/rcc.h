#ifndef _DTX_STM32F1_RCC_H_
#define _DTX_STM32F1_RCC_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>

__attribute__((always_inline)) static inline enum rcc_periph_clken get_rcc_from_port(u32 port)
{
    switch (port) {
        case GPIOA:  return RCC_GPIOA;
        case GPIOB:  return RCC_GPIOB;
        case GPIOC:  return RCC_GPIOC;
        case GPIOD:  return RCC_GPIOD;
        case GPIOE:  return RCC_GPIOE;
        case GPIOF:  return RCC_GPIOF;
        case GPIOG:  return RCC_GPIOG;
        case SPI1:   return RCC_SPI1;
        case SPI2:   return RCC_SPI2;
        case SPI3:   return RCC_SPI3;
        case ADC1:   return RCC_ADC1;
        case ADC2:   return RCC_ADC2;
        case ADC3:   return RCC_ADC3;
        case DMA1:   return RCC_DMA1;
        case DMA2:   return RCC_DMA2;
        case I2C1:   return RCC_I2C1;
        case I2C2:   return RCC_I2C2;
        case TIM1:   return RCC_TIM1;
        case TIM2:   return RCC_TIM2;
        case TIM3:   return RCC_TIM3;
        case TIM4:   return RCC_TIM4;
        case TIM5:   return RCC_TIM5;
        case TIM6:   return RCC_TIM6;
        case TIM7:   return RCC_TIM7;
        case TIM8:   return RCC_TIM8;
        case TIM9:   return RCC_TIM9;
        case TIM10:  return RCC_TIM10;
        case TIM11:  return RCC_TIM11;
        case TIM12:  return RCC_TIM12;
        case TIM13:  return RCC_TIM13;
        case TIM14:  return RCC_TIM14;
        case USART1: return RCC_USART1;
        case UART5:  return RCC_UART5;
        default:    return ltassert();  // We should never get here
    }
}

#define get_rcc_from_pin(x) get_rcc_from_port((x).port)
// static inline  enum rcc_periph_clken get_rcc_from_pin(struct mcu_pin pin)
// {
//    return get_rcc_from_port(pin.port);
// }

#endif  // _DTX_STM32F1_RCC_H_
