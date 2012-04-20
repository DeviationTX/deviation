/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include "target.h"

void Initialize_Channels()
{
  int i;
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);
  /* Make sure the ADC doesn't run during config. */
  adc_off(ADC1);


        rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);

        /* Make sure the ADC doesn't run during config. */
        adc_off(ADC1);

  /* configure channels for analog */
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);

        /* We configure everything for one single conversion. */
        adc_disable_scan_mode(ADC1);
        adc_set_single_conversion_mode(ADC1);
        adc_enable_discontinous_mode_regular(ADC1);
        adc_disable_external_trigger_regular(ADC1);
        adc_set_right_aligned(ADC1);

        /* We want to read the temperature sensor, so we have to enable it. */
        adc_set_conversion_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

        adc_on(ADC1);

        /* Wait for ADC starting up. */
        for (i = 0; i < 800000; i++)    /* Wait a bit. */
                __asm__("nop");

        adc_reset_calibration(ADC1);
        adc_calibration(ADC1);
}

u16 read_adc1(u8 channel)
{
    u8 channel_array[16];
    /* Select the channel we want to convert. 16=temperature_sensor. */
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);

    /*
     * If the ADC_CR2_ON bit is already set -> setting it another time
     * starts the conversion.
     */
    adc_on(ADC1);

    /* Wait for end of conversion. */
    while (!(ADC_SR(ADC1) & ADC_SR_EOC));
    return(ADC_DR(ADC1));
}

u16 ReadThrottle()
{
    return read_adc1(13);
}

u16 ReadRudder()
{
    return read_adc1(11);
}

u16 ReadElevator()
{
    return read_adc1(10);
}

u16 ReadAileron()
{
    return read_adc1(12);
}

