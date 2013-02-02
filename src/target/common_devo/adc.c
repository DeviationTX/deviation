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
#include <libopencm3/stm32/f1/adc.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/dma.h>
#include "common.h"
#include "devo.h"
#include <stdlib.h>
#include <stdio.h>

u16 ADC_Read(u8 channel);
volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
#define WINDOW_SIZE 10
#define SAMPLE_COUNT NUM_ADC_CHANNELS * WINDOW_SIZE * ADC_OVERSAMPLE_WINDOW_COUNT
static volatile u16 adc_array_oversample[SAMPLE_COUNT];
void ADC_Init(void)
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, _RCC_APB2ENR_ADCEN);
    /* Make sure the ADC doesn't run during config. */
    adc_off(_ADC);
    rcc_peripheral_reset(&RCC_APB2RSTR, _RCC_APB2RSTR_ADCRST);
    rcc_peripheral_clear_reset(&RCC_APB2RSTR, _RCC_APB2RSTR_ADCRST);
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
    /* We configure to scan the entire group each time conversion is requested. */
    adc_enable_scan_mode(_ADC);
    adc_set_single_conversion_mode(_ADC);
    adc_disable_discontinuous_mode_regular(_ADC);
    adc_disable_external_trigger_regular(_ADC);
    adc_set_right_aligned(_ADC);

    /* We want to read the temperature sensor, so we have to enable it. */
    adc_enable_temperature_sensor(_ADC); 
    adc_set_sample_time_on_all_channels(_ADC, ADC_SMPR_SMP_13DOT5CYC);

    adc_power_on(_ADC);
    adc_reset_calibration(_ADC);
    adc_calibration(_ADC);

    //Build a RNG seed using ADC 14, 16, 17
    u32 seed = 0;
    for(int i = 0; i < 8; i++) {
        seed = seed << 4 | ((ADC_Read(16) & 0x03) << 2) | (ADC_Read(17) & 0x03); //Get 2bits of RNG from Temp and Vref
        seed ^= ADC_Read(14) << i; //Get a couple more random bits from Voltage sensor
    }
    adc_disable_temperature_sensor(_ADC); 
    printf("RNG Seed: %08x\n", (int)seed);
    srand(seed);

    /* The following is based on code from here: http://code.google.com/p/rayaairbot */
    /* Enable DMA clock */
    rcc_peripheral_enable_clock(&RCC_AHBENR, _RCC_AHBENR_DMAEN);
    /* no reconfig for every ADC group conversion */
    dma_enable_circular_mode(_DMA, _DMA_CHANNEL);
    /* the memory pointer has to be increased, and the peripheral not */
    dma_enable_memory_increment_mode(_DMA, _DMA_CHANNEL);
    /* ADC_DR is only 16bit wide in this mode */
    dma_set_peripheral_size(_DMA, _DMA_CHANNEL, DMA_CCR_PSIZE_16BIT);
    /*destination memory is also 16 bit wide */
    dma_set_memory_size(_DMA, _DMA_CHANNEL, DMA_CCR_MSIZE_16BIT);
    /* direction is from ADC to memory */
    dma_set_read_from_peripheral(_DMA, _DMA_CHANNEL);
    /* get the data from the ADC data register */
    dma_set_peripheral_address(_DMA, _DMA_CHANNEL,(u32) &ADC_DR(_ADC));
    /* put everything in this array */
    dma_set_memory_address(_DMA, _DMA_CHANNEL, (u32) &adc_array_oversample);
    /* we convert only 3 values in one adc-group */
    dma_set_number_of_data(_DMA, _DMA_CHANNEL, SAMPLE_COUNT);
    /* we want an interrupt after the adc is finished */
    //dma_enable_transfer_complete_interrupt(_DMA, _DMA_CHANNEL);

    /* dma ready to go. waiting til the peripheral gives the first data */
    dma_enable_channel(_DMA, _DMA_CHANNEL);

    adc_enable_dma(_ADC);
    adc_set_regular_sequence(_ADC, NUM_ADC_CHANNELS, (u8 *)adc_chan_sel);
    adc_set_continuous_conversion_mode(_ADC);
    adc_start_conversion_direct(_ADC);
}

u16 ADC_Read(u8 channel)
{
    u8 channel_array[1];
    /* Select the channel we want to convert. 16=temperature_sensor. */
    channel_array[0] = channel;
    adc_set_regular_sequence(_ADC, 1, channel_array);

    /*
     * If the ADC_CR2_ON bit is already set -> setting it another time
     * starts the conversion.
     */
    adc_start_conversion_direct(_ADC);

    /* Wait for end of conversion. */
    while (! adc_eoc(_ADC))
        ;
    return adc_read_regular(_ADC);
}

void ADC_StartCapture()
{
    //while (!(ADC_SR(_ADC) & ADC_SR_EOC));
    //adc_start_conversion_direct(_ADC);
}

#if 0
void _DMA_ISR()
{
    //medium_priority_cb();
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
