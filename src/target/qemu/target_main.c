#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include "common.h"
#include "mixer.h"
#include "config/tx.h"
#include "config/model.h"
#include "../common/devo/devo.h"

void run_profile();
void init_profile();
int main()
{
    SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT; //sleep immediate on WFI
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    systick_set_reload(0x00FFFFFF);
    systick_counter_enable();
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOC, GPIO12);
    gpio_set(GPIOC, GPIO12);
    gpio_clear(GPIOC, GPIO12);

    init_profile();
    run_profile();
}

void init_profile() {
    for (int i = 0; i < INP_HAS_CALIBRATION; i++)
    {
        Transmitter.calibration[i].min = CHAN_MIN_VALUE;
        Transmitter.calibration[i].max = CHAN_MAX_VALUE;
        Transmitter.calibration[i].zero = 0;
    }
    memset(Model.mixers, 0, sizeof(Model.mixers));
    for (unsigned i = 0; i < 1; i++) {
        Model.mixers[i].src = 1;
        Model.mixers[i].dest = (2 + i) % 5;
        Model.mixers[i].scalar = 100;
        Model.mixers[i].flags = MUX_REPLACE;
    }
    Model.num_channels = 12;
}

void __attribute__ ((noinline)) run_profile() {
    MIXER_CalcChannels();
}
