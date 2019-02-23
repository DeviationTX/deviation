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
#include "target/drivers/mcu/stm32/rcc.h"

void VIBRATINGMOTOR_Init()
{
    if (!HAS_VIBRATINGMOTOR)
        return;
    rcc_periph_clock_enable(get_rcc_from_pin(HAPTIC_PIN));
    GPIO_setup_output(HAPTIC_PIN, OTYPE_PUSHPULL);
    GPIO_pin_clear(HAPTIC_PIN);
}

void VIBRATINGMOTOR_Start()
{
    if (!HAS_VIBRATINGMOTOR || !Transmitter.vibration_state)
        return;
    GPIO_pin_set(HAPTIC_PIN);
}

void VIBRATINGMOTOR_Stop()
{
    if (!HAS_VIBRATINGMOTOR)
        return;
    GPIO_pin_clear(HAPTIC_PIN);
}
