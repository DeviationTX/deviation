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

#if HAS_EXTENDED_AUDIO

#if HAS_AUDIO_UART5 || !defined(BUILDTYPE_DEV)

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
#endif

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
  #ifdef BUILDTYPE_DEV
  // dev builds log to the serial port, so just report it.
  printf("Playing alert #%d (%s)\n", music_map[music].musicid, music_map[music].label);
  #endif

  #if !defined(BUILDTYPE_DEV) || HAS_AUDIO_UART5

#if HAS_AUDIO_UART5
  // If we are just playing beeps....
  if (music == MUSIC_KEY_PRESSING || music == MUSIC_MAXLEN)
#else
  // If we are using the PPM port or are just playing beeps anyway....
  if ( PPMin_Mode() || Model.protocol == PROTOCOL_PPM
  || music == MUSIC_KEY_PRESSING || music == MUSIC_MAXLEN)
#endif
    return 0;

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
  #endif
  return 1;
}

void AUDIO_SetVolume(void) {
#ifdef BUILDTYPE_DEV
    printf("Setting external audio volume to %d\n", Transmitter.audio_vol);
#else
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

#endif
