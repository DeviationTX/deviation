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
#include "music.h"
#include "config/tx.h"
#include "config/model.h"
#include "extended_audio.h"
#include "stdlib.h"

#ifndef EMULATOR
#include <libopencm3/stm32/usart.h>
#endif // EMULATOR

#if HAS_EXTENDED_AUDIO

static u32 audio_queue_time = 0;

// Initialize UART for extended-audio
void AUDIO_Init() {
#ifdef BUILDTYPE_DEV
    printf("Audio: Initializing UART for extended-audio\n");
#endif

#if HAS_AUDIO_UART5
    if (Transmitter.audio_uart5) {
#ifdef BUILDTYPE_DEV
        printf("Audio: UART5 already initialized\n");
#endif
        return;
    }
#endif

#ifndef EMULATOR
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) {
#ifdef BUILDTYPE_DEV
        printf("Audio: Cannot initialize USART for extended-audio, PPM in use\n");
#endif
        usart_disable(_USART);
        usart_set_baudrate(_USART, 115200);
        usart_enable(_USART);
    }
    else {
#ifdef BUILDTYPE_DEV
        printf("Audio: Setting up USART for extended-audio\n");
#endif
        usart_disable(_USART);
        usart_set_baudrate(_USART, 9600);
        usart_enable(_USART);
    }
#endif // EMULATOR
}

#ifndef EMULATOR
// Send a block of len bytes to the Audio device.
void
AUDIO_Send(u8 *data, int len) {
  void AUDIO_send_char(char c);
  for (u16 iter = 0; iter < len; iter += 1) {
    AUDIO_send_char(data[iter]);
  }
}

// Send a string  to the Audio device.
void
AUDIO_Print(char *string) {
  AUDIO_Send((u8 *)string, strlen(string));
}
#endif // EMULATOR

void u16ToArray(u16 value, u8 *array){
    *array = (u8)(value>>8);
    *(array+1) = (u8)value;
}

// generate Checksum for DFPlyer commands
u16 AUDIO_CalculateChecksum(u8 *buffer) {
    u16 sum = 0;
    for (int i=1; i < 7; i += 1)
        sum += buffer[i];
    return -sum;
}

// Generate a string to play.
int AUDIO_Play(u16 music) {
#if HAS_AUDIO_UART5
    // If we are just playing beeps....
    if (music == MUSIC_KEY_PRESSING || music == MUSIC_MAXLEN) {
#else
    // If we are using the PPM port or are just playing beeps anyway....
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM
    || music == MUSIC_KEY_PRESSING || music == MUSIC_MAXLEN) {
#ifdef BUILDTYPE_DEV
        printf("Audio: PPM port in use\n");
#endif
#endif
        return 0;
    }

#ifdef BUILDTYPE_DEV
    printf("Audio: Playing music #%d (%s)\n", music_map[music].musicid, music_map[music].label);
#endif

#ifdef EMULATOR     // On emulators call mpg123 to play mp3s
    char cmd[70];
    u16 vol_val = Transmitter.audio_vol * 32786/30;
#ifdef _WIN32
    sprintf(cmd, "start /B ..\\..\\mpg123 -f %d -q ..\\..\\mp3\\%04d*.mp3 > nul 2>&1", vol_val, music_map[music].musicid);
#else
    sprintf(cmd, "mpg123 -f %d -q ../../mp3/%04d*.mp3 > /dev/null 2>&1 &", vol_val, music_map[music].musicid);
#endif // _WIN32
    system(cmd);
    return 1;
#endif // EMULATOR

#ifndef EMULATOR
  switch (Transmitter.audio_player) {
    case AUDIO_LAST: // Sigh. Shut up the warnings
    case AUDIO_NONE: return 0;	// Play beeps...
    case AUDIO_AUDIOFX: {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "#%d\n", music_map[music].musicid);
      AUDIO_Print(buffer);
      break;
    }
    case AUDIO_DF_PLAYER: {
        static u8 buffer[] =
            {0x7E, 0xFF, 0x06, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
        // Fill in track number and checksum
        u16ToArray(music_map[music].musicid, buffer+5);
        u16ToArray(AUDIO_CalculateChecksum(buffer), buffer+7);
        AUDIO_Send(buffer, sizeof(buffer));
        break;
    }
  }
  return 1;
 #endif // EMULATOR
}

void AUDIO_SetVolume() {
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) { // don't send volume command when using PPM port
#ifdef BUILDTYPE_DEV
        printf("Audio: PPM port in use\n");
#endif
        return;
    }
#ifdef BUILDTYPE_DEV
    printf("Audio: Setting external audio volume to %d\n", Transmitter.audio_vol);
#endif
#ifndef EMULATOR
    switch (Transmitter.audio_player) {
      case AUDIO_LAST: // Sigh. Shut up the warnings
      case AUDIO_AUDIOFX: // AUDIOFX only allows up down selection of volume...not implemented
      case AUDIO_NONE: break;	// Play beeps...
      case AUDIO_DF_PLAYER: {
          static u8 buffer[] =
              {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
          // Fill in volume and checksum
          u16ToArray(Transmitter.audio_vol, buffer+5);
          u16ToArray(AUDIO_CalculateChecksum(buffer), buffer+7);
          AUDIO_Send(buffer, sizeof(buffer));
          break;
      }
    }
#endif
}

void AUDIO_CheckQueue() {
    u32 t = CLOCK_getms();
    if (next_audio < num_audio) {
        if (t > audio_queue_time) {
            AUDIO_Play(audio_queue[next_audio]);
            audio_queue_time = CLOCK_getms() + music_map[audio_queue[next_audio]].duration;
            next_audio++;
        }
    } else if (num_audio && t > audio_queue_time) {
#ifdef BUILDTYPE_DEV
        printf("Audio: Queue finished, resetting.\n");
#endif
        num_audio = 0;
        next_audio = 0;
    }
}

void AUDIO_AddQueue(u16 music) {
    if (num_audio == AUDIO_QUEUE_LENGTH) {
#ifdef BUILDTYPE_DEV
        printf("Audio: Queue full, cannot add new music #%d\n",music);
#endif
        return;
    }
    audio_queue[num_audio++] = music;
}

#endif
