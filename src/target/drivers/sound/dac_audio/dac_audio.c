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

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/i2c.h>

#include "common.h"
#include "target/common/stm32/nvic.h"
#include "target/common/stm32/dma.h"
#include "target/common/stm32/rcc.h"

enum {
    AUDIO_DONE = 0,
    AUDIO_FIRST_HALF = 1,
    AUDIO_SECOND_HALF = 2,
    AUDIO_READY = 3,
};
static uint8_t waveform[256];
static volatile uint8_t load_audio;
static FILE *  wav_fh;
static uint32_t bytes_remaining;
static uint8_t sample_size;
static uint8_t channel_size;

#define _uint32_le(x) (*(uint32_t *)(x))  // FIXME: need to do something different for Big-Endian
#define _uint16_le(x) (*(uint16_t *)(x))  // FIXME: need to do something different for Big-Endian
#define TIMER_TICKS_PER_SEC rcc_apb1_frequency

void AUDIODAC_Init(unsigned period)
{
    /* Enable TIM2 clock. */
    rcc_periph_clock_enable(get_rcc_from_port(AUDIODAC_TIM.tim));
    rcc_periph_reset_pulse(AUDIODAC_TIM.rst);
    /* Timer global mode: - No divider, Alignment edge, Direction up */
    timer_set_mode(AUDIODAC_TIM.tim, TIM_CR1_CKD_CK_INT,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_continuous_mode(AUDIODAC_TIM.tim);
    timer_set_period(AUDIODAC_TIM.tim, period);
    timer_disable_oc_output(AUDIODAC_TIM.tim, TIM_OC2 | TIM_OC3 | TIM_OC4);
    timer_enable_oc_output(AUDIODAC_TIM.tim, TIM_OC1);
    timer_disable_oc_clear(AUDIODAC_TIM.tim, TIM_OC1);
    timer_disable_oc_preload(AUDIODAC_TIM.tim, TIM_OC1);
    timer_set_oc_slow_mode(AUDIODAC_TIM.tim, TIM_OC1);
    timer_set_oc_mode(AUDIODAC_TIM.tim, TIM_OC1, TIM_OCM_TOGGLE);
    timer_set_oc_value(AUDIODAC_TIM.tim, TIM_OC1, 500);
    timer_disable_preload(AUDIODAC_TIM.tim);
    /* Set the timer trigger output (for the DAC) to the channel 1 output
       compare */
    timer_set_master_mode(AUDIODAC_TIM.tim, TIM_CR2_MMS_COMPARE_OC1REF);
    timer_enable_counter(AUDIODAC_TIM.tim);

    /* DAC channel 1 uses DMA controller 1 Stream 5 Channel 7. */
    /* Enable AUDIODAC_DMA.dma clock and IRQ */
        NVIC_enable_dma_irq(AUDIODAC_DMA);
    DMA_stream_channel_reset(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, AUDIODAC_DMA.channel);
    DMA_set_priority(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, DMA_SxCR_PL_LOW);
    dma_set_memory_size(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, DMA_SxCR_MSIZE_8BIT);
    dma_set_peripheral_size(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, DMA_SxCR_PSIZE_8BIT);
    dma_enable_memory_increment_mode(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);
    dma_enable_circular_mode(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);
    DMA_set_transfer_mode(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, AUDIODAC_DMA.channel,
                DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    /* The register to target is the DAC1 8-bit right justified data
       register */
    dma_set_peripheral_address(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, (uint32_t) &DAC_DHR8R1);
    /* The array v[] is filled with the waveform data to be output */
    dma_set_memory_address(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, (uint32_t) waveform);
    dma_set_number_of_data(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, 256);
    dma_enable_half_transfer_interrupt(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);
    dma_enable_transfer_complete_interrupt(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);
    DMA_channel_select(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream, AUDIODAC_DMA.channel);
    DMA_enable_stream(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);

    /* Enable the DAC clock on APB1 */
    rcc_periph_clock_enable(RCC_DAC);
    /* Setup the DAC channel 1, with timer 2 as trigger source.
     * Assume the DAC has woken up by the time the first transfer occurs */
    dac_trigger_enable(CHANNEL_1);
    dac_set_trigger_source(DAC_CR_TSEL1_T2);
    dac_dma_enable(CHANNEL_1);
    dac_enable(CHANNEL_1);
}

void I2CVOLUME_SET(unsigned volume) {
    const u8 volumeScale[] = {
      0,  1,  2,  3,  5,  9,  13,  17,  22,  27,  33,  40,
      64, 82, 96, 105, 112, 117, 120, 122, 124, 125, 126, 127
    };
    if (volume >= sizeof(volumeScale)) {
        volume = sizeof(volumeScale) - 1;
    }
    u8 vol[2];
    vol[0] = 0x00;
    vol[1] = volumeScale[volume];
    // FIXME: This hangs indefinitely
    // i2c_transfer7(I2C_CFG.i2c, I2C_ADDRESS_VOLUME, vol, 2,  NULL, 0);
    return;
}


static uint32_t handle_fmt(uint32_t chunk_size)
{
    uint8_t data[16];
    if (chunk_size < 16) {
        fseek(wav_fh, chunk_size, SEEK_CUR);
        return 0;
    }
    fread(data, 16, 1, wav_fh);

    unsigned format = _uint16_le(data);
    uint32_t sample_rate = _uint32_le(data + 4);
    sample_size = _uint16_le(data + 12);
    channel_size = _uint16_le(data + 14) / 8;
    if (chunk_size > 16)
        fseek(wav_fh, chunk_size, SEEK_CUR);
    if (format != 1)
        return 0;
    unsigned period = TIMER_TICKS_PER_SEC / sample_rate;
    return period;
}

void AUDIODAC_Loop()
{
    u8 data[sizeof(waveform) / 2];
    u8 *wavptr;
    if (load_audio == AUDIO_DONE || load_audio == AUDIO_READY)
        return;
    wavptr = waveform + ((load_audio == AUDIO_FIRST_HALF) ? 0 : sizeof(waveform) / 2);
    for (unsigned i = 0; i < channel_size; i++) {
        unsigned len = sizeof(data);
        if (bytes_remaining < len)
            len = bytes_remaining;
        unsigned bytes = fread(data, 1, len, wav_fh);
        bytes_remaining -= bytes;
        printf("br: %d %d\n", bytes_remaining, CLOCK_getms());
        for (unsigned j = channel_size - 1; j < bytes; j += sample_size)
            *wavptr++ = data[j];

        if (!bytes_remaining) {
            load_audio = AUDIO_DONE;
            fclose(wav_fh);
            wav_fh = NULL;
            return;
        }
    }
    load_audio = AUDIO_READY;
    return;
}

void DAC_play(const char *filename)
{
    load_audio = AUDIO_DONE;
    wav_fh = fopen(filename, "r");
    if (!wav_fh) {
        printf("Failed to open wav\n");
        return;
    }
    u8 data[12];
    fread(data, 12, 1, wav_fh);
    if (memcmp(data, "RIFF", 4)) {
        printf("BAD WAV header\n");
        return;
    }
    u32 chunk_size;
    u32 period = 0;
    int ok;
    while ((ok = fread(data, 8, 1, wav_fh))) {
        chunk_size = _uint32_le(data + 4);
        printf("read: %c%c%c%c %d\n", data[0], data[1], data[2], data[3], chunk_size);
        if (memcmp(data, "fmt ", 4) == 0) {
            period = handle_fmt(chunk_size);
        } else if (memcmp(data, "data", 4) != 0) {
            fseek(wav_fh, chunk_size, SEEK_CUR);
        } else {
            break;
        }
    }
    if (!ok || !period) {
        printf("Missing data header\n");
        return;
    }
    bytes_remaining = chunk_size;
    printf("period: %d ss: %d cs: %d count: %d\n", period, sample_size, channel_size, bytes_remaining);
    load_audio = AUDIO_FIRST_HALF;
    AUDIODAC_Loop();
    load_audio = AUDIO_SECOND_HALF;
    AUDIODAC_Loop();
    AUDIODAC_Init(period);
}
