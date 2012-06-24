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
#include <libopencm3/stm32/f1/gpio.h>
#include "target.h"
#include "mixer.h"
#include "devo8.h"

const char *tx_input_str[NUM_TX_INPUTS] = {
    "THR",
    "RUD",
    "ELE",
    "AIL",
    "RUD_D/R",
    "ELE_D/R",
    "AIL D/R",
    "GEAR",
    "MIX0",
    "MIX1",
    "MIX2",
    "FMODE0",
    "FMODE1",
    "FMODE2",
};

void CHAN_Init()
{
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);

    // This is just to fill in something before calibration
    //Throttle (mode 1)
    Transmitter.calibration[0].max = 3698;
    Transmitter.calibration[0].min = 489;
    Transmitter.calibration[0].zero = (Transmitter.calibration[0].max + Transmitter.calibration[0].min) / 2;
    //Rudder
    Transmitter.calibration[1].max = 3867;
    Transmitter.calibration[1].min = 569;
    Transmitter.calibration[1].zero = (Transmitter.calibration[1].max + Transmitter.calibration[1].min) / 2;
    //Elevator (mode 1)
    Transmitter.calibration[2].max = 3549;
    Transmitter.calibration[2].min = 354;
    Transmitter.calibration[2].zero = (Transmitter.calibration[2].max + Transmitter.calibration[2].min) / 2;
    //Alileron
    Transmitter.calibration[3].max = 3813;
    Transmitter.calibration[3].min = 530;
    Transmitter.calibration[3].zero = (Transmitter.calibration[3].max + Transmitter.calibration[3].min) / 2;
}

s16 CHAN_ReadInput(int channel)
{
    u16 value;
    switch(channel) {
    case INP_THROTTLE: value = ADC1_Read(13); break;
    case INP_RUDDER:   value = ADC1_Read(11); break;
    case INP_ELEVATOR: value = ADC1_Read(10); break;
    case INP_AILERON:  value = ADC1_Read(12); break;
    }
    if(channel <= INP_HAS_CALIBRATION) {
        s32 max = Transmitter.calibration[channel - 1].max;
        s32 min = Transmitter.calibration[channel - 1].min;
        s32 zero = Transmitter.calibration[channel - 1].zero;
        if(! zero) {
            //If this input doesn't have a zero, calculate from max/min
            zero = ((u32)max + min) / 2;
        }
        max -= zero;
        min -= zero;
        if(value >= zero) {
            value = ((s32)value - zero) * CHAN_MAX_VALUE / max;
        } else {
            value = ((s32)value - zero) * CHAN_MIN_VALUE / min;
        }
    } else {
        value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    return value;
}
