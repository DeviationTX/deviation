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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "common.h"
#include "target/common/stm32/dma.h"
#include "target/common/stm32/nvic.h"

//FIXME
#define SSER_TX_TIM           TIM8
#define SSER_TX_DMA_ISR       dma2_stream1_isr

#define SSER_RX_PIN  ((struct mcu_pin){GPIOA, GPIO10})

//Configure for Tim1: DMA2, Stream1, Channel6
#define JREXT_DMA ((struct dma_config) { \
    .dma = DMA2,                         \
    .stream = DMA_STREAM1,               \
    .channel = DMA_SxCR_CHSEL_7,         \
    })

//transmit: 100kbps 8e2 requires max 10 transitions per byte
static u16 jrext_bits[26*10];
volatile u8 sser_transmitting;
static unsigned count_per_bit = 10;  // 10 clocks per bit (1Mhz -> 100kbaud)

void SSER_InitializeTx()
{
    sser_transmitting = 0;
    rcc_periph_clock_enable(RCC_TIM8);
    rcc_periph_clock_enable(RCC_DMA2);

    // connect timer compare output to pin
    PORT_pin_setup_pushpull_af(EXTMODULE_PIN, SSER_TX_TIM);

    rcc_periph_reset_pulse(RST_TIM8);

    // Timer global mode: No divider, Alignment edge, Direction up, auto-preload buffered
    timer_set_mode(SSER_TX_TIM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    //SSER_TX_TIM is on APB@@60MHz
    timer_set_prescaler(SSER_TX_TIM, 60 - 1);

    /* compare output setup. compare register must match i/o pin */
    timer_set_oc_mode(SSER_TX_TIM, TIM_OC1, TIM_OCM_FORCE_HIGH); // output force high

    // further specific initialization in PPM_Enable and PXX_Enable
}

void SSER_StopTx()
{
    timer_disable_counter(SSER_TX_TIM);
    nvic_disable_irq(NVIC_DMA2_STREAM1_IRQ);
    dma_disable_transfer_complete_interrupt(JREXT_DMA.dma, JREXT_DMA.stream);
    dma_disable_stream(JREXT_DMA.dma, JREXT_DMA.stream);
}

static u16 * add_bit(u16 *ptr, unsigned bit, unsigned last)
{
    if (bit == last) {
        *ptr += count_per_bit;
    } else {
        (*ptr)--;   //1 extra cycle to reset counter
        ptr++;
        *ptr = count_per_bit;
    }
    return ptr;
}

void SSER_Transmit(u8 *packet, int len)
{
    if (!packet) return;
    while(sser_transmitting) {
        //Wait for tramission to complete
    }
    u16 *ptr = jrext_bits;
    jrext_bits[0] = 2 * count_per_bit;
    for (int i = 0; i < len; i ++) {
        u8 val = packet[i];
        ptr = add_bit(ptr, 0, 1);   // add start bit
        int ones = 0;
        unsigned last = 0;
        for(int j = 0; j < 8; j++) {
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
#ifdef JREXT_NO_DMA
    *ptr = 0;
    jrptr = jrext_bits+1;
    timer_disable_counter(SSER_TX_TIM);
    timer_continuous_mode(SSER_TX_TIM);
    timer_disable_preload(SSER_TX_TIM);
    timer_set_period(SSER_TX_TIM, jrext_bits[0]);
    timer_set_oc_value(SSER_TX_TIM, TIM_OC1, 0);           // 8 us high for PXX
    timer_set_oc_mode(SSER_TX_TIM, TIM_OC1, TIM_OCM_TOGGLE); // output active while counter below compare
    timer_enable_oc_output(SSER_TX_TIM, TIM_OC1);          // enable OCx to pin
    timer_enable_oc_output(SSER_TX_TIM, TIM_OC1N);          // enable OCx to pin
    timer_enable_break_main_output(SSER_TX_TIM);               // master output enable
    nvic_enable_irq(NVIC_SSER_TX_TIM_CC_IRQ);
    timer_enable_irq(SSER_TX_TIM, TIM_DIER_CC1IE);
    timer_set_counter(SSER_TX_TIM, 0);                          //don't fire for 1st count
    timer_enable_counter(SSER_TX_TIM);
#else
    dma_disable_stream(JREXT_DMA.dma, JREXT_DMA.stream);
    timer_disable_counter(SSER_TX_TIM);
    timer_set_oc_mode(SSER_TX_TIM, TIM_OC1, TIM_OCM_FORCE_HIGH); // output force high
    // Setup DMA
    nvic_set_priority(NVIC_DMA2_STREAM1_IRQ, 3);    // DMA interrupt on transfer complete
    nvic_enable_irq(NVIC_DMA2_STREAM1_IRQ);

    DMA_stream_channel_reset(JREXT_DMA.dma, JREXT_DMA.stream, JREXT_DMA.channel);
    DMA_channel_select(JREXT_DMA.dma, JREXT_DMA.stream, JREXT_DMA.channel);
    DMA_set_priority(JREXT_DMA.dma, JREXT_DMA.stream, DMA_SxCR_PL_VERY_HIGH);
    DMA_set_transfer_mode(JREXT_DMA.dma, JREXT_DMA.stream, JREXT_DMA.channel, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_set_peripheral_address(JREXT_DMA.dma, JREXT_DMA.stream, (u32) &TIM_ARR(SSER_TX_TIM));
    dma_set_peripheral_size(JREXT_DMA.dma, JREXT_DMA.stream, DMA_SxCR_PSIZE_16BIT);
    dma_disable_peripheral_increment_mode(JREXT_DMA.dma, JREXT_DMA.stream);
    dma_set_memory_address(JREXT_DMA.dma, JREXT_DMA.stream, (u32)&jrext_bits[1]);
    dma_set_memory_size(JREXT_DMA.dma, JREXT_DMA.stream, DMA_SxCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(JREXT_DMA.dma, JREXT_DMA.stream);
    dma_set_number_of_data(JREXT_DMA.dma, JREXT_DMA.stream, ((u32)ptr - (u32)jrext_bits) / sizeof( u16 ) );
    dma_enable_transfer_complete_interrupt(JREXT_DMA.dma, JREXT_DMA.stream);
    dma_enable_stream(JREXT_DMA.dma, JREXT_DMA.stream);    // dma ready to go

    timer_continuous_mode(SSER_TX_TIM);
    timer_disable_oc_output(SSER_TX_TIM, TIM_OC2 | TIM_OC3 | TIM_OC4);
    timer_disable_oc_clear(SSER_TX_TIM, TIM_OC1);
    timer_disable_oc_preload(SSER_TX_TIM, TIM_OC1);
    timer_update_on_overflow(SSER_TX_TIM);

    timer_disable_preload(SSER_TX_TIM);
    timer_set_period(SSER_TX_TIM, jrext_bits[0]);              //1st period should remain high (2 stop bits are enforced)
    timer_set_counter(SSER_TX_TIM, 0);
    timer_set_oc_value(SSER_TX_TIM, TIM_OC1, 0);               // toggle on counter reset
    timer_set_oc_mode(SSER_TX_TIM, TIM_OC1, TIM_OCM_TOGGLE);   // toggle on counter reset
    timer_enable_oc_output(SSER_TX_TIM, TIM_OC1);              // Need both OC1 and OC1N enabled
    timer_enable_oc_output(SSER_TX_TIM, TIM_OC1N);
    timer_enable_break_main_output(SSER_TX_TIM);               // master output enable
    timer_enable_irq(SSER_TX_TIM, TIM_DIER_UDE);        // enable timer dma request

    sser_transmitting = 1;
    timer_enable_counter(SSER_TX_TIM);
#endif
}

//////////////////////////////////////////////////////
// soft serial receiver for s.port data
// receive inverted data (high input volgate = 0, low is 1)
// 57600 bps, no stop, 8 data bits, 1 stop bit, lsb first
// 1) Enable rising edge interrupt on RX line to search for start bit
// 2) On rising edge, set timer for half a bit time to get to bit center
// 3) Verify start bit, set timer for interrupts at full bit time
// 4) Collect 8 data bits, verify stop bit
// 5) Process byte, then repeat
u8 in_byte;
u8 data_byte;
u8 bit_pos;
sser_callback_t *soft_rx_callback;

void SSER_InitializeRx()
{
    if (SSER_RX_PIN.port == GPIOA && SSER_RX_PIN.pin == GPIO10)
        UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))

    in_byte = 0;
    bit_pos = 0;
    data_byte = 0;

    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOA);

    // Set RX pin mode to pull up
    PORT_pin_setup_input_pullup(SSER_RX_PIN);

    // Interrupt on input rising edge to find start bit
    exti_select_source(EXTI10, GPIOA);
    exti_set_trigger(EXTI10, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI10);

    // Configure bit timer
    rcc_periph_clock_enable(RCC_TIM6);
    rcc_periph_reset_pulse(RST_TIM6);
    nvic_set_priority(NVIC_TIM6_IRQ, 8);
    timer_set_prescaler(TIM6, 0);
    timer_enable_irq(TIM6, TIM_DIER_UIE);
}

void SSER_StartReceive(sser_callback_t *isr_callback)
{
    soft_rx_callback = isr_callback;

    if (isr_callback) {
        nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    } else {
        nvic_disable_irq(NVIC_EXTI15_10_IRQ);
        nvic_disable_irq(NVIC_TIM6_IRQ);
    }
}

void SSER_StopRx()
{
    SSER_StartReceive(NULL);
    timer_disable_counter(TIM6);
    exti_disable_request(EXTI10);
}
