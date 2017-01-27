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

#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include "common.h"

#include "devo.h"
#include "music.h"
#include "config/tx.h"
#include "config/model.h"

#if HAS_EXTENDED_AUDIO
void
AUDIO_send_char(uint8_t c) {
#if HAS_AUDIO_UART5
    if (Transmitter.audio_uart5) {
        usart_send_blocking(UART5, c);
    }
    else {
        usart_send_blocking(_USART, c);
    }
      
#else
    usart_send_blocking(_USART, c);
#endif
}
#endif
