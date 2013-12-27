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
#include <libopencm3/stm32/f2/adc.h>
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/dma.h>
#include "common.h"
#include "../common/devo/devo.h"
#include <stdlib.h>
#include <stdio.h>

u16 ADC_Read(u8 channel);
volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
#define WINDOW_SIZE 10
#define SAMPLE_COUNT NUM_ADC_CHANNELS * WINDOW_SIZE * ADC_OVERSAMPLE_WINDOW_COUNT
static volatile u16 adc_array_oversample[SAMPLE_COUNT];

void ADC_Init(void)
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);
    /* Make sure the ADC doesn't run during config. */
    adc_off(ADC1);
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY2);
    /* We configure to scan the entire group each time conversion is requested. */
    adc_enable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_disable_discontinuous_mode_regular(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);

    /* We want to read the temperature sensor, so we have to enable it. */
    adc_enable_temperature_sensor(); 
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_144CYC);

    adc_power_on(ADC1);

    //Build a RNG seed using ADC 14, 16, 17
    u32 seed = 0;
    for(int i = 0; i < 8; i++) {
        seed = seed << 4 | ((ADC_Read(16) & 0x03) << 2) | (ADC_Read(17) & 0x03); //Get 2bits of RNG from Temp and Vref
        seed ^= ADC_Read(adc_chan_sel[NUM_ADC_CHANNELS-1]) << i; //Get a couple more random bits from Voltage sensor
    }
    //This is important.  We're using the temp value as a buffer because otherwise the channel data
    //Can bleed into the voltage-sense data.
    //By disabling the temperature, we always read a consistent value
    adc_disable_temperature_sensor();
    printf("RNG Seed: %08x\n", (int)seed);
    srand(seed);
    /* The following is based on the procedure documented in the
     * STM32F2 Reference manual 9.3.17 (Stream Configuration Procedure)
     */
    /* Enable DMA clock */
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_DMA2EN);
    /* 1) Disable DMA and wait for it to complete */
    dma_stream_reset(DMA2, DMA_STREAM0);
    /* 2) get the data from the ADC data register */
    dma_set_peripheral_address(DMA2, DMA_STREAM0,(u32) &ADC_DR(ADC1));
    /* 3) put everything in this array */
    dma_set_memory_address(DMA2, DMA_STREAM0, (u32) &adc_array_oversample);
    /* 4) Number of elements in the memory array */
    dma_set_number_of_data(DMA2, DMA_STREAM0, SAMPLE_COUNT);
    /* 5) Select dma channel (ADC1)*/
    dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
    /* 6) Let the DMA controller manage flow-control (required for cirular mode) */
    dma_set_dma_flow_control(DMA2, DMA_STREAM0);
    /* 7) priority = Very High */
    dma_set_priority(DMA2, DMA_STREAM0, DMA_SxCR_PL_VERY_HIGH);
    /* 8) Enable direct-mode (disable FIFO) */
    dma_enable_direct_mode(DMA2, DMA_STREAM0);
    /* 9) Setup everything else */
    /* direction is from ADC to memory */
    dma_set_transfer_mode(DMA2, DMA_STREAM0, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    /* the memory pointer has to be increased, and the peripheral not */
    dma_enable_memory_increment_mode(DMA2, DMA_STREAM0);
    /* ADC_DR is only 16bit wide in this mode */
    dma_set_peripheral_size(DMA2, DMA_STREAM0, DMA_SxCR_PSIZE_16BIT);
    /*destination memory is also 16 bit wide */
    dma_set_memory_size(DMA2, DMA_STREAM0, DMA_SxCR_MSIZE_16BIT);
    /* no double buffering */
    dma_disable_double_buffer_mode(DMA2, DMA_STREAM0);
    /* continuous operation */
    dma_enable_circular_mode(DMA2, DMA_STREAM0);
    /* we want an interrupt after the adc is finished */
    //dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM0);

    /* dma ready to go. waiting til the peripheral gives the first data */
    dma_enable_stream(DMA2, DMA_STREAM0);

    adc_enable_dma(ADC1);
    adc_set_dma_continue(ADC1);
    adc_set_regular_sequence(ADC1, NUM_ADC_CHANNELS, (u8 *)adc_chan_sel);
    adc_set_continuous_conversion_mode(ADC1);
    adc_start_conversion_regular(ADC1);
}

u16 ADC_Read(u8 channel)
{
    u8 channel_array[1];
    /* Select the channel we want to convert. 16=temperature_sensor. */
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);

    /*
     * If the ADC_CR2_ON bit is already set -> setting it another time
     * starts the conversion.
     */
    adc_start_conversion_regular(ADC1);

    /* Wait for end of conversion. */
    while (! adc_eoc(ADC1))
        ;
    return adc_read_regular(ADC1);
}

void ADC_StartCapture()
{
    //while (!(ADC_SR(ADC1) & ADC_SR_EOC));
    adc_start_conversion_regular(ADC1);
}

#if 0
void _DMA_ISR()
{
    ADC_Filter();
    medium_priority_cb();
    /* clear the interrupt flag */
    DMA_IFCR(_DMA) |= _DMA_IFCR_CGIF;
}
#endif

void ADC_Filter()
{
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        u32 result = 0;
        int idx = i;
        for(int j = 0; j < WINDOW_SIZE * ADC_OVERSAMPLE_WINDOW_COUNT; j++) {
            result += adc_array_oversample[idx];
            idx += NUM_ADC_CHANNELS;
        }
        result /= ADC_OVERSAMPLE_WINDOW_COUNT * WINDOW_SIZE;
        adc_array_raw[i] = result;
    }
}

void ADC_ScanChannels()
{
    u32 lastms = 0;
    u16 max[NUM_ADC_CHANNELS];
    u16 min[NUM_ADC_CHANNELS];
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        max[i] = 0;
        min[i] = 0xffff;
    }
    while(1) {
        if(PWR_CheckPowerSwitch()) PWR_Shutdown();
        //for(int i = 0; i < SAMPLE_COUNT; i++) {
        //    adc_array_oversample[i] = ADC_Read(adc_chan_sel[i % NUM_ADC_CHANNELS]);
        //}
        ADC_Filter();
        for(int i = 0; i < NUM_ADC_CHANNELS; i++) {
            u32 x = adc_array_raw[i];
            if (x > max[i])
                max[i] = x;
            if (x < min[i])
                min[i] = x;
        }
        u32 ms = CLOCK_getms();
        if((ms % 1000) == 0 && ms != lastms) {
            lastms = ms;
            printf("max:");
            for(int i = 0; i < NUM_ADC_CHANNELS; i++) printf(" %04x", max[i]);
            printf(" %d\n", max[NUM_ADC_CHANNELS-1]);
            printf("min:");
            for(int i = 0; i < NUM_ADC_CHANNELS; i++) printf(" %04x", min[i]);
            printf(" %d\n", min[NUM_ADC_CHANNELS-1]);
            printf("    ");
            for(int i = 0; i < NUM_ADC_CHANNELS; i++) printf(" %04x", adc_array_raw[i]);
            printf(" %d\n\n", adc_array_raw[NUM_ADC_CHANNELS-1]);
            memset(max, 0, sizeof(max));
            memset(min, 0xFF, sizeof(min));
        }
    }
}
