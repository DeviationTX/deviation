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
AUDIO_Send(uint8_t *data, int len) {
  void AUDIO_send_char(char c);
  for (uint16_t iter = 0; iter < len; iter += 1) {
    AUDIO_send_char(data[iter]);
  }
}

// Send a string  to the Audio device.
void
AUDIO_Print(char *string) {
  AUDIO_Send((uint8_t *)string, strlen(string));
}
#endif

// Generate a string to play.
int AUDIO_Play(u16 music) {
  #ifdef BUILDTYPE_DEV
  // dev builds log to the serial port, so just report it.
  printf("Playing alert #%d (%s)\n", music, music_map[music].label);
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
      snprintf(buffer, sizeof(buffer), "#%d\n", music);
      AUDIO_Print(buffer);
      break;
    }
    case AUDIO_DF_PLAYER: {
      static uint8_t buffer[] =
        {0x7E, 0xFF, 0x06, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};

      // Fill in track number
      buffer[5] = (uint8_t)(music >> 8);
      buffer[6] = (uint8_t)music;

      // And the checksum
      uint16_t sum = 0;
      for (int i=1; i < 7; i += 1)
        sum += buffer[i];
      sum = -sum;
      buffer[7] = (uint8_t)(sum >> 8);
      buffer[8] = (uint8_t)sum;

      AUDIO_Send(buffer, sizeof(buffer));
      break;
    }
  }

  #endif
  return 1;
}

void AUDIO_SetVolume(void) {
  //sending volume to audio player not implented yet.
#ifdef BUILDTYPE_DEV
    printf("Setting external audio volume to %d", Transmitter.audio_vol);
#endif
}

#endif
