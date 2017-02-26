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
#include "target.h"
#include "gui/gui.h"
#include "music.h"
#include "tx.h"

#ifndef EMULATOR
#include <libopencm3/stm32/usart.h>
#endif // EMULATOR

#include <stdlib.h>
#include <string.h>

#if HAS_EXTRA_SWITCHES
const char SWITCH_CFG[] = "extra-switches";
#endif
#if HAS_EXTRA_BUTTONS
const char BUTTON_CFG[] = "extra-buttons";
#endif
const char HAPTIC_ENABLE[] = "enable-haptic";
#if HAS_EXTENDED_AUDIO
const char AUDIO_CFG[] = "extended-audio";
static char * const AUDIO_PLAYER[AUDIO_LAST] = {
    [AUDIO_AUDIOFX] = "audiofx",
    [AUDIO_DF_PLAYER] = "dfplayer",
    };
#endif
#if HAS_AUDIO_UART5
const char UART5_ENABLE[] = "extended-audio-uart5";
#endif
/* Section: TX Module */
static const char SECTION_MODULES[] = "modules";
static const char MODULE_ENABLE_PIN[] = "enable";
static const char MODULE_HAS_PA[] = "has_pa";
static const char MODULE_CONFIG[] = "config";
const char * const MODULE_NAME[TX_MODULE_LAST] = {
      [CYRF6936] = "CYRF6936",
      [A7105]    = "A7105",
      [CC2500]   = "CC2500",
      [NRF24L01] = "NRF24l01",
      [MULTIMOD] = "MultiMod",
      };

#define MATCH_SECTION(s) strcasecmp(section, s) == 0
#define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
#define MATCH_KEY(s)     strcasecmp(name,    s) == 0

static int get_module_index(const char *str)
{
    for(int i = 0; i < TX_MODULE_LAST; i++) {
        if (strcasecmp(str, MODULE_NAME[i]) == 0)
            return i;
    }
    return -1;
}

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    (void)user;
    s32 value_int = atoi(value);
    if (section[0] == '\0') {
#if HAS_EXTRA_SWITCHES
        if (MATCH_KEY(SWITCH_CFG)) {
            CHAN_SetSwitchCfg(value);
        }
#endif
#if HAS_EXTRA_BUTTONS
        if (MATCH_KEY(BUTTON_CFG)) {
            CHAN_SetButtonCfg(value);
        }
#endif
#if HAS_VIBRATINGMOTOR == OPTIONAL
        //Only configure this if the motor isn't stock
        if (MATCH_KEY(HAPTIC_ENABLE)) {
            if (value_int)
                Transmitter.extra_hardware |= VIBRATING_MOTOR;
        }
#endif
#if HAS_EXTENDED_AUDIO
        if (MATCH_KEY(AUDIO_CFG)) {
            for (int i = 1; i < AUDIO_LAST; i += 1) {
                unsigned len = strlen(AUDIO_PLAYER[i]);
                if (strncasecmp(value, AUDIO_PLAYER[i], len) == 0) {
                    Transmitter.audio_player = i;
                    if (strlen(value) == len)
                        Transmitter.audio_2way = 0;
                    else if (strcasecmp(value + len, "-2") == 0)
                        Transmitter.audio_2way = 1;
                    // Ignore invalid values
                    else Transmitter.audio_player = AUDIO_NONE;
                }
            }
        }
#endif
#if HAS_AUDIO_UART5
        if (MATCH_KEY(UART5_ENABLE)) {
            if (value_int)
                Transmitter.audio_uart5 = 1;
        }
#endif
        if(MATCH_KEY("txid")) {
            Transmitter.txid = strtol(value, NULL, 16);
        }
        return 1;
    }
    if(MATCH_SECTION(SECTION_MODULES)) {
        if(MATCH_KEY("force")) {
            if (value_int)
                Transmitter.extra_hardware |= FORCE_MODULES;
            return 1;
        }
        if(MATCH_START(name, MODULE_ENABLE_PIN)) {
            int pin = get_module_index(name+sizeof(MODULE_ENABLE_PIN));
            if(pin >= 0) {
                if (MCU_SetPin(&Transmitter.module_enable[pin], value))
                    return 1;
            }
        }
        if(MATCH_START(name, MODULE_HAS_PA)) {
            int pin = get_module_index(name+sizeof(MODULE_HAS_PA));
            if(pin >= 0) {
               int v = value_int ? 1 : 0;
               Transmitter.module_poweramp = (Transmitter.module_poweramp & ~(1 << pin)) | (v << pin);
               return 1;
            }
        }
        if(MATCH_START(name, MODULE_CONFIG)) {
            int module = get_module_index(name+sizeof(MODULE_CONFIG));
            if(module >= 0) {
               Transmitter.module_config[module] = strtol(value, NULL, 16);
               return 1;
            }
        }
    }
    return 0;
}

void CONFIG_LoadHardware()
{
    CONFIG_IniParse("hardware.ini", ini_handler, NULL);
}
