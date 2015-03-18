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
#include "protospi.h"
#include "config/model.h"
#include "protocol/interface.h"

u8 last_module = TX_MODULE_LAST;
void PACTL_Init()
{
    /* Enable Power Amp Rx/Tx signals */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    PORT_mode_setup(PA_TXEN, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(PA_RXEN, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(NRF24L01_PAEN, GPIO_MODE_ANALOG, GPIO_PUPD_NONE);
    PROTOSPI_pin_clear(PA_TXEN);
    PROTOSPI_pin_clear(PA_RXEN);
    /* Configure aomparator on PA.1 */
    comp_select_input(COMP1, COMP_CSR_INSEL_1_4_VREFINT);
    comp_select_speed(COMP1, COMP_CSR_SPEED_HIGH);
    comp_select_output(COMP1, COMP_CSR_OUTSEL_NONE);
    comp_select_hyst(COMP1, COMP_CSR_HYST_NO);
    exti_set_trigger(EXTI21, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI21);
    //comp_enable(COMP1);
    nvic_set_priority(NVIC_ADC_COMP_IRQ, 0 << 6);
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

void PACTL_SetNRF24L01_CE(int state)
{
    if(state) {
        PROTOSPI_pin_set(NRF24L01_CE);
    } else {
        PROTOSPI_pin_clear(NRF24L01_CE);
    }
}

int PACTL_SetSwitch(int module) {
    PACTL_SetNRF24L01_CE(0);
    switch (module) {
        case CYRF6936: /* Port 3 */ PROTOSPI_pin_clear(RF_MUXSEL1);    PROTOSPI_pin_set(RF_MUXSEL2); break;
        case CC2500:   /* Port 4 */   PROTOSPI_pin_set(RF_MUXSEL1);    PROTOSPI_pin_set(RF_MUXSEL2); break;
        case NRF24L01: /* Port 2 */   PROTOSPI_pin_set(RF_MUXSEL1);  PROTOSPI_pin_clear(RF_MUXSEL2); break;
        case A7105:    /* Port 1 */ PROTOSPI_pin_clear(RF_MUXSEL1);  PROTOSPI_pin_clear(RF_MUXSEL2); break;
    }
    last_module = module;
    return 0;
}

void PACTL_SetTxRxMode(int mode)
{
    if (mode == TXRX_OFF) {
        comp_disable(COMP1);
        nvic_disable_irq(NVIC_ADC_COMP_IRQ);
        PROTOSPI_pin_clear(PA_RXEN);
        PROTOSPI_pin_clear(PA_TXEN);
        return;
    }
    if (last_module == NRF24L01) {
        comp_enable(COMP1);
        nvic_enable_irq(NVIC_ADC_COMP_IRQ);
    }
    if (mode == TX_EN) {
        PROTOSPI_pin_clear(PA_RXEN);
        PROTOSPI_pin_set(PA_TXEN);
    } else {
        PROTOSPI_pin_set(PA_RXEN);
        PROTOSPI_pin_clear(PA_TXEN);
    }
    printf("Mode(%d): %d (%d,%d,%d,%d)\n", last_module, mode, PORT_pin_get(RF_MUXSEL1), PORT_pin_get(RF_MUXSEL2), PORT_pin_get(PA_TXEN), PORT_pin_get(PA_RXEN));
#if DISCOVERY
    if (last_module == CYRF6936) {
        //BUYCHINA_SetTxRxMode
        if(mode == TX_EN) {
            CYRF_WriteRegister(0x0E,0x20);
        } else if (mode == RX_EN) {
            CYRF_WriteRegister(0x0E,0x80);
        }
    }
#endif
}
