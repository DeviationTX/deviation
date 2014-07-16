/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/comparator.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

#define EXTI21				(1 << 21)
static void clock_setup(void)
{
	/* Enable GPIOC clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOA);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO8/9 on GPIO port C for LEDs. */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8 | GPIO7 | GPIO9);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10 | GPIO11);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);

	comp_select_input(COMP1, COMP_CSR_INSEL_1_4_VREFINT);
        comp_select_speed(COMP1, COMP_CSR_SPEED_HIGH);
	comp_select_output(COMP1, COMP_CSR_OUTSEL_NONE);
        comp_select_hyst(COMP1, COMP_CSR_HYST_NO);
	exti_set_trigger(EXTI21, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI21);
	comp_enable(COMP1);
	nvic_enable_irq(NVIC_ADC_COMP_IRQ);
}

void adc_comp_isr()
{
	if(COMP_CSR1 & COMP_CSR_OUT) {
		gpio_set(GPIOB, GPIO10);
		gpio_clear(GPIOB, GPIO11);
		gpio_set(GPIOC, GPIO8);
	} else {
		gpio_clear(GPIOB, GPIO10);
		gpio_set(GPIOB, GPIO11);
		gpio_clear(GPIOC, GPIO8);
	}
	exti_reset_request(EXTI21);
}
int main(void)
{
    PWR_Init();
    clock_setup();
    gpio_setup();
    UART_Initialize();
        
	//usart_setup();
	printf("Power Up\n");
	/* Blink the LED (PD12) on the board with every transmitted byte. */
	while (1) {
		if(gpio_get(GPIOA, GPIO0)) {
			gpio_set(GPIOC, GPIO7);
		} else {
			gpio_clear(GPIOC, GPIO7);
		}
		if(COMP_CSR1 & COMP_CSR_OUT) {
			gpio_set(GPIOC, GPIO9);
		} else {
			gpio_clear(GPIOC, GPIO9);
		}
	}

	return 0;
}
