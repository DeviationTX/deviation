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
#include "config/tx.h"
#include <stdlib.h>
#include <stdio.h>
#include "target/drivers/mcu/stm32/adc.h"
#include "target/drivers/mcu/stm32/dma.h"
#include "target/drivers/mcu/stm32/rcc.h"

#define NUM_ADC_CHANNELS (INP_HAS_CALIBRATION + 2)  // Inputs + Temprature + Voltage
#define WINDOW_SIZE 10
#define SAMPLE_COUNT NUM_ADC_CHANNELS * WINDOW_SIZE * ADC_OVERSAMPLE_WINDOW_COUNT

#define CHAN_INVERT -1
#define CHAN_NONINV  1

unsigned ADC_Read(unsigned channel);
volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
static volatile u16 adc_array_oversample[SAMPLE_COUNT];

#if 0
    // These are the valid ADC pins for an STM32
    ADC_CHAN(GPIOA, GPIO0),  /* ADC123_0  */ \
    ADC_CHAN(GPIOA, GPIO1),  /* ADC123_1  */ \
    ADC_CHAN(GPIOA, GPIO2),  /* ADC123_2  */ \
    ADC_CHAN(GPIOA, GPIO3),  /* ADC123_3  */ \
    ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   */ \
    ADC_CHAN(GPIOA, GPIO5),  /* ADC12_5   */ \
    ADC_CHAN(GPIOA, GPIO6),  /* ADC12_6   */ \
    ADC_CHAN(GPIOA, GPIO7),  /* ADC12_7   */ \
    ADC_CHAN(GPIOB, GPIO0),  /* ADC12_8   */ \
    ADC_CHAN(GPIOB, GPIO1),  /* ADC12_9   */ \
    ADC_CHAN(GPIOF, GPIO6),  /* ADC3_4    */ \
    ADC_CHAN(GPIOF, GPIO7),  /* ADC3_5    */ \
    ADC_CHAN(GPIOF, GPIO8),  /* ADC3_6    */ \
    ADC_CHAN(GPIOF, GPIO9),  /* ADC3_7    */ \
    ADC_CHAN(GPIOF, GPIO10), /* ADC3_8    */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    ADC_CHAN(GPIOC, GPIO5),  /* ADC12_15  */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    // End
#endif

void ADC_Init(void)
{
    #define ADC_CHAN(x, y, z) (x ? get_rcc_from_port(x) : 0)
    const uint32_t adc_rcc[NUM_ADC_CHANNELS] = ADC_CHANNELS;
    #undef ADC_CHAN
    #define ADC_CHAN(x, y, z) {(x), (y)}
    const struct mcu_pin adc_pins[NUM_ADC_CHANNELS] = ADC_CHANNELS;
    #undef ADC_CHAN
    #define ADC_CHAN(x, y, z) ADC_PIN_TO_CHAN((x), (y))
    static const u8 adc_chan_sel[NUM_ADC_CHANNELS] = ADC_CHANNELS;
    #undef ADC_CHAN
    for (unsigned i = 0; i < NUM_ADC_CHANNELS; i++) {
        if (!HAS_PIN(adc_pins[i]))
            continue;
        rcc_periph_clock_enable(adc_rcc[i]);
        GPIO_setup_input(adc_pins[i], ITYPE_ANALOG);
    }
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
    rcc_periph_clock_enable(get_rcc_from_port(ADC_DMA.dma));
    /* 1) Disable DMA and wait for it to complete */
    DMA_stream_reset(ADC_DMA);
    /* 2) get the data from the ADC data register */
    dma_set_peripheral_address(ADC_DMA.dma, ADC_DMA.stream, (u32)&ADC_DR(ADC_CFG.adc));
    /* 3) put everything in this array */
    dma_set_memory_address(ADC_DMA.dma, ADC_DMA.stream, (u32)&adc_array_oversample);
    /* 4) Number of elements in the memory array */
    dma_set_number_of_data(ADC_DMA.dma, ADC_DMA.stream, SAMPLE_COUNT);
    /* 5) Select dma channel*/
    DMA_channel_select(ADC_DMA);
    /* 6) Let the DMA controller manage flow-control (required for cirular mode) */
    DMA_set_dma_flow_control(ADC_DMA);
    /* 7) priority = Very High */
    DMA_set_priority(ADC_DMA, DMA_SxCR_PL_VERY_HIGH);
    /* 8) Enable direct-mode (disable FIFO) */
    DMA_enable_direct_mode(ADC_DMA);
    /* 9) Setup everything else */
    /* the memory pointer has to be increased, and the peripheral not */
    dma_enable_memory_increment_mode(ADC_DMA.dma, ADC_DMA.stream);
    /* ADC_DR is only 16bit wide in this mode */
    dma_set_peripheral_size(ADC_DMA.dma, ADC_DMA.stream, DMA_SxCR_PSIZE_16BIT);
    /*destination memory is also 16 bit wide */
    dma_set_memory_size(ADC_DMA.dma, ADC_DMA.stream, DMA_SxCR_MSIZE_16BIT);
    /* direction is from ADC to memory */
    DMA_set_transfer_mode(ADC_DMA, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    /* no double buffering */
    DMA_disable_double_buffer_mode(ADC_DMA);
    /* continuous operation */
    dma_enable_circular_mode(ADC_DMA.dma, ADC_DMA.stream);
    /* we want an interrupt after the adc is finished */
    // dma_enable_transfer_complete_interrupt(ADC_DMA.dma, ADC_DMA.stream);

    /* dma ready to go. waiting til the peripheral gives the first data */
    DMA_enable_stream(ADC_DMA);

    adc_enable_dma(ADC_CFG.adc);
    adc_set_dma_continue(ADC_CFG.adc);
    adc_set_regular_sequence(ADC_CFG.adc, NUM_ADC_CHANNELS, (u8 *)adc_chan_sel);
    adc_set_continuous_conversion_mode(ADC_CFG.adc);
    ADC_start_conversion(ADC_CFG.adc);
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
    ADC_start_conversion(ADC_CFG.adc);

    /* Wait for end of conversion. */
    while (!adc_eoc(ADC_CFG.adc))
        ;
    return adc_read_regular(ADC_CFG.adc);
}

void ADC_StartCapture()
{
    //while (!(ADC_SR(_ADC) & ADC_SR_EOC));
    ADC_start_conversion(ADC_CFG.adc);
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

s32 ADC_ReadRawInput(int channel)
{
    if TX_HAS_SRC(channel)
        return adc_array_raw[channel-1];
    return 0;
}

s32 ADC_NormalizeChannel(int channel)
{
    s32 value = ADC_ReadRawInput(channel);
    s32 max = Transmitter.calibration[channel - 1].max;
    s32 min = Transmitter.calibration[channel - 1].min;
    s32 zero = Transmitter.calibration[channel - 1].zero;
    if (!zero) {
        // If this input doesn't have a zero, calculate from max/min
        zero = ((u32)max + min) / 2;
    }
    // Derate min and max by 1% to ensure we can get all the way to 100%
    max = (max - zero) * 99 / 100;
    min = (min - zero) * 99 / 100;
    if (value >= zero) {
        value = (value - zero) * CHAN_MAX_VALUE / max;
    } else {
        value = (value - zero) * CHAN_MIN_VALUE / min;
    }
    // Bound output
    if (value > CHAN_MAX_VALUE)
        value = CHAN_MAX_VALUE;
    if (value < CHAN_MIN_VALUE)
        value = CHAN_MIN_VALUE;

    #define ADC_CHAN(x, y, z) (z)
    const s8 chan_inverted[NUM_ADC_CHANNELS] = ADC_CHANNELS;
    #undef ADC_CHAN
    value = value * chan_inverted[channel - 1];
    return value;
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

/* Return milivolts */
unsigned PWR_ReadVoltage(void)
{
    u32 v = adc_array_raw[NUM_ADC_CHANNELS-1];
    /* Multily the above by 1000 to get milivolts */
    v = v * VOLTAGE_NUMERATOR / 100 + VOLTAGE_OFFSET;
    return v;
}

