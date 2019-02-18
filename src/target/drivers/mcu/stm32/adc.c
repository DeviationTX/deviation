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
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include "target/drivers/mcu/stm32/adc.h"
#include "target/drivers/mcu/stm32/dma.h"
#include "target/drivers/mcu/stm32/rcc.h"

#include "target/tx/devo/common/devo.h"  // FIXME - Shouldn't rely on things from 'devo' here

unsigned ADC_Read(unsigned channel);
volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
#define WINDOW_SIZE 10
#define SAMPLE_COUNT NUM_ADC_CHANNELS * WINDOW_SIZE * ADC_OVERSAMPLE_WINDOW_COUNT
static volatile u16 adc_array_oversample[SAMPLE_COUNT];
void ADC_Init(void)
{
    rcc_periph_clock_enable(get_rcc_from_port(ADC_CFG.adc));
    adc_power_off(ADC_CFG.adc);
    ADC_reset(ADC_CFG.adc);
    adc_set_clk_prescale(ADC_CFG.prescalar);
    /* We configure to scan the entire group each time conversion is requested. */
    adc_enable_scan_mode(ADC_CFG.adc);
    adc_set_single_conversion_mode(ADC_CFG.adc);
    adc_disable_discontinuous_mode_regular(ADC_CFG.adc);
    adc_disable_external_trigger_regular(ADC_CFG.adc);
    adc_set_right_aligned(ADC_CFG.adc);

    /* We want to read the temperature sensor, so we have to enable it. */
    adc_enable_temperature_sensor();
    adc_set_sample_time_on_all_channels(ADC_CFG.adc, ADC_CFG.sampletime);

    adc_power_on(ADC_CFG.adc);
    ADC_calibrate(ADC_CFG.adc);

    //Build a RNG seed using ADC 14, 16, 17
    for(int i = 0; i < 8; i++) {
        u32 seed;
        seed = ((ADC_Read(16) & 0x03) << 2) | (ADC_Read(17) & 0x03); //Get 2bits of RNG from Temp and Vref
        seed ^= ADC_Read(adc_chan_sel[NUM_ADC_CHANNELS-1]) << i; //Get a couple more random bits from Voltage sensor
        rand32_r(0, seed);
    }
    //This is important.  We're using the temp value as a buffer because otherwise the channel data
    //Can bleed into the voltage-sense data.
    //By disabling the temperature, we always read a consistent value
    adc_disable_temperature_sensor();
    printf("RNG Seed: %08x\n", (int)rand32());

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
    dma_set_peripheral_address(_DMA, _DMA_CHANNEL, (u32) &ADC_DR(ADC_CFG.adc));
    /* put everything in this array */
    dma_set_memory_address(_DMA, _DMA_CHANNEL, (u32) &adc_array_oversample);
    /* we convert only 3 values in one adc-group */
    dma_set_number_of_data(_DMA, _DMA_CHANNEL, SAMPLE_COUNT);
    /* we want an interrupt after the adc is finished */
    //dma_enable_transfer_complete_interrupt(_DMA, _DMA_CHANNEL);

    /* dma ready to go. waiting til the peripheral gives the first data */
    dma_enable_channel(_DMA, _DMA_CHANNEL);

    adc_enable_dma(ADC_CFG.adc);
    adc_set_regular_sequence(ADC_CFG.adc, NUM_ADC_CHANNELS, (u8 *)adc_chan_sel);
    adc_set_continuous_conversion_mode(ADC_CFG.adc);
    adc_start_conversion_direct(ADC_CFG.adc);
}

unsigned ADC_Read(unsigned channel)
{
    u8 channel_array[1];
    /* Select the channel we want to convert. 16=temperature_sensor. */
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC_CFG.adc, 1, channel_array);

    /*
     * If the ADC_CR2_ON bit is already set -> setting it another time
     * starts the conversion.
     */
    adc_start_conversion_direct(ADC_CFG.adc);

    /* Wait for end of conversion. */
    while (!adc_eoc(ADC_CFG.adc))
        ;
    return adc_read_regular(ADC_CFG.adc);
}

void ADC_StartCapture()
{
    //while (!(ADC_SR(_ADC) & ADC_SR_EOC));
    adc_start_conversion_direct(ADC_CFG.adc);
}

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
        ADC_Filter();
        for(int i = 0; i < NUM_ADC_CHANNELS; i++) {
            u32 x = adc_array_raw[i];
            if (x > max[i])
                max[i] = x;
            if (x < min[i])
                min[i] = x;
        }
        u32 ms = CLOCK_getms();
        if((ms % 100) == 0 && ms != lastms) {
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
