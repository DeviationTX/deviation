/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/comparator.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

#include "common.h"
#include "config/model.h"
#include "protocol/interface.h"

void PACTL_Init()
{
    /* Enable Power Amp Rx/Tx signals */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10 | GPIO11); //PB.10 => TX_EN, PB.11 => RX_EN
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);           //PA.1  => NRF_PAEN
    gpio_clear(GPIOB, GPIO10 | GPIO11);                                        //TX_EN => OFF, RX_EN => OFF
    /* Configure aomparator on PA.1 */
    comp_select_input(COMP1, COMP_CSR_INSEL_1_4_VREFINT);
    comp_select_speed(COMP1, COMP_CSR_SPEED_HIGH);
    comp_select_output(COMP1, COMP_CSR_OUTSEL_NONE);
    comp_select_hyst(COMP1, COMP_CSR_HYST_NO);
    exti_set_trigger(EXTI21, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI21);
    //comp_enable(COMP1);
    //nvic_enable_irq(NVIC_ADC_COMP_IRQ);
    PACTL_SetTxRxMode(TXRX_OFF);
}

void adc_comp_isr()
{
    // If the PA_EN bit is >= 1.8V, TX_EN == 1, RX_EN == 0
    // else TX_EN == 0, RX_EN == 1
    if(COMP_CSR1 & COMP_CSR_OUT) {
        gpio_set(GPIOB, GPIO10);
        gpio_clear(GPIOB, GPIO11);
    } else {
        gpio_clear(GPIOB, GPIO10);
       gpio_set(GPIOB, GPIO11);
    }
    exti_reset_request(EXTI21);
}

void PACTL_SetTxRxMode(int mode)
{
    if (mode == TXRX_OFF) {
        comp_disable(COMP1);
        nvic_disable_irq(NVIC_ADC_COMP_IRQ);
        gpio_clear(GPIOB, GPIO10 | GPIO11);
    } else {
        if (Model.module == NRF24L01) {
            comp_enable(COMP1);
            nvic_enable_irq(NVIC_ADC_COMP_IRQ);
        }
        if (mode == TX_EN) {
            gpio_set(GPIOB, GPIO10);
            gpio_clear(GPIOB, GPIO11);
        } else {
            gpio_clear(GPIOB, GPIO10);
            gpio_set(GPIOB, GPIO11);
        }
    }
}
