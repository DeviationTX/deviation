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
u16 voice_map_entries;
struct VoiceMap voice_map[MAX_VOICEMAP_ENTRIES];
u16 audio_queue[AUDIO_QUEUE_LENGTH];
u8 next_audio;
u8 num_audio;
u32 audio_queue_time;

#ifndef EMULATOR
static u8 player_buffer[] = {0x7E, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
#endif

// Initialize UART for extended-audio
void AUDIO_Init() {
    if (Transmitter.audio_player == AUDIO_DISABLED) {
        // Reload hardware.ini if audio_player had been temporarily disabled
        CONFIG_LoadHardware();
    }
    printf("Voice: Initializing UART for extended-audio\n");

#if HAS_AUDIO_UART
    if (Transmitter.audio_uart) {
        printf("Voice: UART5 already initialized\n");
        return;
    }
#endif

#ifndef _DEVO12_TARGET_H_
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) {
        printf("Voice: Cannot initialize UART for extended-audio, PPM in use\n");
        UART_SetDataRate(0);
    }
    else {
#endif // _DEVO12_TARGET_H_
        printf("Voice: Setting up UART for extended-audio\n");
        UART_SetDataRate(9600);
#ifndef _DEVO12_TARGET_H_
    }
#endif // _DEVO12_TARGET_H_
    AUDIO_SetVolume();
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

    // If we are just playing beeps....
    if (music == MUSIC_KEY_PRESSING || music == MUSIC_MAXLEN) {
        printf("Voice: beep only\n");
        return 0;
    }
    printf("Voice: Playing mp3 #%d\n", voice_map[music].id);

#ifdef EMULATOR     // On emulators call mpg123 to play mp3s
    char cmd[70];
    u16 vol_val = Transmitter.audio_vol * 32786/10;
#ifdef _WIN32
    sprintf(cmd, "start /B ..\\..\\mpg123 -f %d -q ..\\..\\mp3\\%04d*.mp3 > nul 2>&1", vol_val, voice_map[music].id);
#else
    sprintf(cmd, "mpg123 -f %d -q ../../mp3/%04d*.mp3 > /dev/null 2>&1 &", vol_val, voice_map[music].id);
#endif // _WIN32
    system(cmd);
    return 1;
#endif // EMULATOR

#ifndef EMULATOR
  switch (Transmitter.audio_player) {
    case AUDIO_DISABLED:
    case AUDIO_LAST: // Sigh. Shut up the warnings
    case AUDIO_NONE: return 0;	// Play beeps...
    case AUDIO_AUDIOFX: {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "#%d\n", voice_map[music].id);
      AUDIO_Print(buffer);
      break;
    }
    case AUDIO_DF_PLAYER:
        // Fill in track number and checksum
        player_buffer[3] = 0x12;
        u16ToArray(voice_map[music].id, player_buffer+5);
        u16ToArray(AUDIO_CalculateChecksum(player_buffer), player_buffer+7);
        AUDIO_Send(player_buffer, sizeof(player_buffer));
        break;
  }
  return 1;
#endif // EMULATOR
}

void AUDIO_SetVolume() {
#ifndef _DEVO12_TARGET_H_
#if HAS_AUDIO_UART
    if ( !Transmitter.audio_uart && (PPMin_Mode() || Model.protocol == PROTOCOL_PPM) ) {  // don't send volume command when using PPM port
#else
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) { // don't send volume command when using PPM port
#endif
        printf("Voice: PPM port in use, cannot set volume\n");
        return;
    }
#endif //_DEVO12_TARGET_H_
    printf("Voice: Setting external audio volume to %d\n", Transmitter.audio_vol);

#ifndef EMULATOR
    switch (Transmitter.audio_player) {
      case AUDIO_DISABLED:
      case AUDIO_LAST: // Sigh. Shut up the warnings
      case AUDIO_AUDIOFX: // AUDIOFX only allows up down selection of volume...not implemented
      case AUDIO_NONE:
          break;	// Play beeps...
      case AUDIO_DF_PLAYER:
          // Fill in volume and checksum
          player_buffer[3] = 0x06;
          u16ToArray(Transmitter.audio_vol * 3, player_buffer+5); // DFPlayer has volume range 0-30
          u16ToArray(AUDIO_CalculateChecksum(player_buffer), player_buffer+7);
          AUDIO_Send(player_buffer, sizeof(player_buffer));
          break;
    }
#endif
}

void AUDIO_CheckQueue() {
    u32 t = CLOCK_getms();
    if (next_audio < num_audio) {
        if (t > audio_queue_time) {
            AUDIO_Play(audio_queue[next_audio]);
            audio_queue_time = CLOCK_getms() + voice_map[audio_queue[next_audio]].duration;
            next_audio++;
        }
    } else if (num_audio && t > audio_queue_time) {
        printf("Voice: Queue finished, resetting.\n");
        num_audio = 0;
        next_audio = 0;
        AUDIO_SetVolume();
    }
}

int AUDIO_VoiceAvailable() {
#ifndef _DEVO12_TARGET_H_
#if HAS_AUDIO_UART
    if ( !Transmitter.audio_uart && (PPMin_Mode() || Model.protocol == PROTOCOL_PPM) ) {  // don't send play command when using PPM port
#else
    if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) { // don't send play command when using PPM port
#endif
        printf("Voice: PPM port in use\n");
        num_audio = 0; // Reset queue when audio not available
        next_audio = 0;
        return 0;
    }
#endif // _DEVO12_TARGET_H_

    if ( (Transmitter.audio_player == AUDIO_NONE) || (Transmitter.audio_player == AUDIO_DISABLED) || !Transmitter.audio_vol ) {
        num_audio = 0; // Reset queue when audio not available
        next_audio = 0;
        return 0;
    }

    return 1;
}

int AUDIO_AddQueue(u16 music) {
    if (num_audio == AUDIO_QUEUE_LENGTH) {
        printf("Voice: Queue full, cannot add new mp3 #%d\n",music);
        return 0;
    }
    if (!voice_map[music].duration) {
        printf("Voice: mp3 length is zero\n");
        return 0;
    }

    audio_queue[num_audio++] = music;
    return 1;
}

#endif
