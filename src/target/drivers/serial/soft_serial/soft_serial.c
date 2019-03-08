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

#include <libopencm3/stm32/usart.h>

#include "common.h"

#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "target/drivers/mcu/stm32/exti.h"
#include "target/drivers/mcu/stm32/nvic.h"
#include "target/drivers/mcu/stm32/dma.h"

// soft serial receiver for s.port data
// receive inverted data (high input volgate = 0, low is 1)
// 57600 bps, no stop, 8 data bits, 1 stop bit, lsb first
// 1) Enable rising edge interrupt on RX line to search for start bit
// 2) On rising edge, set timer for half a bit time to get to bit center
// 3) Verify start bit, set timer for interrupts at full bit time
// 4) Collect 8 data bits, verify stop bit
// 5) Process byte, then repeat

#ifndef MODULAR

u8 in_byte;
u8 data_byte;
u8 bit_pos;
sser_callback_t *soft_rx_callback;

void SSER_Initialize()
{
    if (!UART_CFG.uart)
        return;
    if (PWM_TIMER.pin.pin == UART_CFG.tx.pin && PWM_TIMER.pin.port == UART_CFG.tx.port) {
        UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))
    }
    in_byte = 0;
    bit_pos = 0;
    data_byte = 0;

    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(get_rcc_from_pin(UART_CFG.rx));

    // Set RX pin mode to pull up
    GPIO_setup_input(UART_CFG.rx, ITYPE_PULLUP);

    // Interrupt on input rising edge to find start bit
    exti_select_source(EXTIx(UART_CFG.rx), UART_CFG.rx.port);
    exti_set_trigger(EXTIx(UART_CFG.rx), EXTI_TRIGGER_RISING);
    exti_enable_request(EXTIx(UART_CFG.rx));

    // Configure bit timer
    rcc_periph_clock_enable(get_rcc_from_port(SSER_TIM.tim));
    rcc_periph_reset_pulse(RST_TIMx(SSER_TIM.tim));
    nvic_set_priority(get_nvic_irq(SSER_TIM.tim), 8);
    timer_set_prescaler(SSER_TIM.tim, 0);
    timer_enable_irq(SSER_TIM.tim, TIM_DIER_UIE);
}

void SSER_StartReceive(sser_callback_t *isr_callback)
{
    if (!UART_CFG.uart)
        return;
    soft_rx_callback = isr_callback;

    if (isr_callback) {
        nvic_enable_irq(NVIC_EXTIx_IRQ(UART_CFG.rx));
    } else {
        nvic_disable_irq(NVIC_EXTIx_IRQ(UART_CFG.rx));
        nvic_disable_irq(get_nvic_irq(SSER_TIM.tim));
    }
}

void SSER_Stop()
{
    if (!UART_CFG.uart)
        return;
    SSER_StartReceive(NULL);
    timer_disable_counter(SSER_TIM.tim);
    exti_disable_request(EXTIx(UART_CFG.rx));
}

#ifdef HAS_SSER_TX
// transmit: 100kbps 8e2 requires max 10 transitions per byte
static u16 jrext_bits[26*10];
volatile u8 sser_transmitting;
static unsigned count_per_bit = 10;  // 10 clocks per bit (1Mhz -> 100kbaud)

void SSER_InitializeTx()
{
    sser_transmitting = 0;
    rcc_periph_clock_enable(get_rcc_from_port(SSER_TX_TIM.tim));
    rcc_periph_clock_enable(get_rcc_from_port(SSER_TX_DMA.dma));

    // connect timer compare output to pin
    GPIO_setup_output_af(SSER_TX_TIM.pin, OTYPE_PUSHPULL, SSER_TX_TIM.tim);

    rcc_periph_reset_pulse(RST_TIMx(SSER_TX_TIM.tim));

    // Timer global mode: No divider, Alignment edge, Direction up, auto-preload buffered
    timer_set_mode(SSER_TX_TIM.tim, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    // SSER_TX_TIM is on APB@@60MHz
    timer_set_prescaler(SSER_TX_TIM.tim, TIM_FREQ_MHz(SSER_TX_TIM.tim) - 1);

    /* compare output setup. compare register must match i/o pin */
    timer_set_oc_mode(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), TIM_OCM_FORCE_HIGH);  // output force high

    // further specific initialization in PPM_Enable and PXX_Enable
}

void SSER_StopTx()
{
    timer_disable_counter(SSER_TX_TIM.tim);
    nvic_disable_irq(get_nvic_dma_irq(SSER_TX_DMA));
    dma_disable_transfer_complete_interrupt(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    dma_disable_stream(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
}

static u16 * add_bit(u16 *ptr, unsigned bit, unsigned last)
{
    if (bit == last) {
        *ptr += count_per_bit;
    } else {
        (*ptr)--;   // 1 extra cycle to reset counter
        ptr++;
        *ptr = count_per_bit;
    }
    return ptr;
}

void SSER_Transmit(u8 *packet, int len)
{
    if (!packet) return;
    while (sser_transmitting) {
        // Wait for tramission to complete
    }
    u16 *ptr = jrext_bits;
    jrext_bits[0] = 2 * count_per_bit;
    for (int i = 0; i < len; i ++) {
        u8 val = packet[i];
        ptr = add_bit(ptr, 0, 1);   // add start bit
        int ones = 0;
        unsigned last = 0;
        for (int j = 0; j < 8; j++) {
            unsigned bit = val & 0x01;
            ptr = add_bit(ptr, bit, last);
            last = bit;
            if (bit) {
                ones++;
            }
            val >>= 1;
        }
        unsigned parity = ones & 0x01;     // even parity
        ptr = add_bit(ptr, parity, last);  // parity bit
        ptr = add_bit(ptr, 1, parity);     // 1st stop bit
        ptr = add_bit(ptr, 1, 1);          // 2nd stop bit
    }
#ifdef SSER_TX_NO_DMA
    *ptr = 0;
    jrptr = jrext_bits+1;
    timer_disable_counter(SSER_TX_TIM.tim);
    timer_continuous_mode(SSER_TX_TIM.tim);
    timer_disable_preload(SSER_TX_TIM.tim);
    timer_set_period(SSER_TX_TIM.tim, jrext_bits[0]);
    timer_set_oc_value(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), 0);                 // 8 us high for PXX
    timer_set_oc_mode(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), TIM_OCM_TOGGLE);     // output active while counter below compare
    timer_enable_oc_output(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch));                // enable OCx to pin
    if (SSER_TX_TIM.chn)
        timer_enable_oc_output(SSER_TX_TIM.tim, TIM_OCxN(SSER_TX_TIM.chn));          // enable OCx to pin
    timer_enable_break_main_output(SSER_TX_TIM.tim);                                 // master output enable
    nvic_enable_irq(NVIC_SSER_TX_TIM_CC_IRQ);
    timer_enable_irq(SSER_TX_TIM.tim, TIM_DIER_CCxDE(SSER_TX_TIM.ch));
    timer_set_counter(SSER_TX_TIM.tim, 0);                                           // don't fire for 1st count
    timer_enable_counter(SSER_TX_TIM.tim);
#else
    dma_disable_stream(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    timer_disable_counter(SSER_TX_TIM.tim);
    timer_set_oc_mode(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), TIM_OCM_FORCE_HIGH);  // output force high
    // Setup DMA
    nvic_set_priority(NVIC_DMA2_STREAM1_IRQ, 3);                                      // DMA interrupt on transfer complete
    nvic_enable_irq(NVIC_DMA2_STREAM1_IRQ);

    DMA_stream_reset(SSER_TX_DMA);
    DMA_channel_select(SSER_TX_DMA);
    DMA_set_priority(SSER_TX_DMA, DMA_SxCR_PL_VERY_HIGH);
    DMA_set_transfer_mode(SSER_TX_DMA, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_set_peripheral_address(SSER_TX_DMA.dma, SSER_TX_DMA.stream, (u32) &TIM_ARR(SSER_TX_TIM.tim));
    dma_set_peripheral_size(SSER_TX_DMA.dma, SSER_TX_DMA.stream, DMA_SxCR_PSIZE_16BIT);
    dma_disable_peripheral_increment_mode(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    dma_set_memory_address(SSER_TX_DMA.dma, SSER_TX_DMA.stream, (u32)&jrext_bits[1]);
    dma_set_memory_size(SSER_TX_DMA.dma, SSER_TX_DMA.stream, DMA_SxCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    dma_set_number_of_data(SSER_TX_DMA.dma, SSER_TX_DMA.stream, ((u32)ptr - (u32)jrext_bits) / sizeof( u16 ) );
    dma_enable_transfer_complete_interrupt(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    dma_enable_stream(SSER_TX_DMA.dma, SSER_TX_DMA.stream);                          // dma ready to go

    timer_continuous_mode(SSER_TX_TIM.tim);
    timer_disable_oc_output(SSER_TX_TIM.tim, TIM_OC1 | TIM_OC2 | TIM_OC3 | TIM_OC4);
    timer_disable_oc_clear(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch));
    timer_disable_oc_preload(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch));
    timer_update_on_overflow(SSER_TX_TIM.tim);

    timer_disable_preload(SSER_TX_TIM.tim);
    timer_set_period(SSER_TX_TIM.tim, jrext_bits[0]);                                // 1st period should remain high (2 stop bits are enforced)
    timer_set_counter(SSER_TX_TIM.tim, 0);
    timer_set_oc_value(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), 0);                 // toggle on counter reset
    timer_set_oc_mode(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch), TIM_OCM_TOGGLE);     // toggle on counter reset
    timer_enable_oc_output(SSER_TX_TIM.tim, TIM_OCx(SSER_TX_TIM.ch));                // Need both OC1 and OC1N enabled
    if (SSER_TX_TIM.chn)
        timer_enable_oc_output(SSER_TX_TIM.tim, TIM_OCxN(SSER_TX_TIM.chn));
    timer_enable_break_main_output(SSER_TX_TIM.tim);                                 // master output enable
    timer_enable_irq(SSER_TX_TIM.tim, TIM_DIER_UDE);                                 // enable timer dma request

    sser_transmitting = 1;
    timer_enable_counter(SSER_TX_TIM.tim);
#endif
}
#endif  // HAS_SSER_TX
#endif  // MODULAR
