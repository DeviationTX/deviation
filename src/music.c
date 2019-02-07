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
#include "extended_audio.h"
#include "config/model.h"
#include <stdlib.h>

static struct {u8 note; u8 duration;} Notes[100];
static u8 Volume;
static u8 next_note;
static u8 num_notes;

#if NUM_TIMERS > 4
#error "Number of timers is != 4.  This will cause the Alarm music to not work properly"
#endif
static const char *const sections[] = {
    "startup",
    "shutdown",
    "volume",
    "alarm1",
    "alarm2",
    "alarm3",
    "alarm4",
    "batt_alarm",
    "done_binding",
    "timer_warning",
    "key_pressing",
    "saving",
    "max_len",
    "telem_alarm1",
    "telem_alarm2",
    "telem_alarm3",
    "telem_alarm4",
    "telem_alarm5",
    "telem_alarm6",
    "inactivity_alarm",
};

#if HAS_EXTENDED_AUDIO
static const char *const audio_devices[] = {
    NULL,
    "all",
    "buzzer",
    "voice",
};

static u8 playback_device;
#endif
static u8 vibrate;

#define NUM_FREQ_ONE_SCALE 12
static const u16 freqs[] = {220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415};

static u16 get_freq(u8 note)
{
    if (note == 0)
        return 0;

    u16 freq = freqs[(note - 1) % NUM_FREQ_ONE_SCALE];
    for (int j = 0; j < (note - 1) / NUM_FREQ_ONE_SCALE; ++j)
        freq *= 2;

    return freq;
}

static u8 get_note(const char *value)
{
    u8 offset;
    s8 level = 0;
    u8 upper = 0;
    switch(value[0])
    {
        case 'c': offset = 0; break;
        case 'd': offset = 2; break;
        case 'e': offset = 4; break;
        case 'f': offset = 5; break;
        case 'g': offset = 7; break;
        case 'a': offset = 9; break;
        case 'b': offset = 11; break;
        default:
            return 0;
    }

    char b;
    if (value[1] == 'x'){
        upper = 1;
        b = value[2];
    }
    else if (value[1] == '\0')
        b = '\0';
    else
        b = value[1];

    if (b != '\0')
        level = b - '0' + 1;

    if (level > 5)
        return 0;

    return NUM_FREQ_ONE_SCALE * level + offset + upper - 8;
}

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    const char *requested_sec = (const char *)user;
    if (strcasecmp(section, requested_sec) == 0) {
#if HAS_EXTENDED_AUDIO
        if (strcasecmp("device", name) == 0) {
            for (u16 i = 1; i < AUDDEV_LAST; i++) {
                if (strcasecmp(audio_devices[i], value) == 0) {
                    playback_device = i;
                    break;
                }
            }
        }
#endif
        if (strcasecmp("vibrate", name) == 0) {
            if (strcasecmp(value, "off") == 0) {
                vibrate = 0;
            }
        }
        if (strcasecmp("volume", name) == 0) {
            Volume = atoi(value);
            if (Volume > 100)
                Volume = 100;
            // The music volume should be controlled by TX volume setting as well as sound.ini
            Volume = Transmitter.volume * Volume/10; // = Transmitter.volume * 10 * sound_volume/100;
        }
        Notes[num_notes].note = get_note(name);
        Notes[num_notes].duration = atoi(value) / 10; //convert from msec to centi-secs
        num_notes++;
        return 1;
    }
    return 1;
}

u16 next_note_cb() {
    if (next_note == num_notes)
        return 0;
    SOUND_SetFrequency(get_freq(Notes[next_note].note), Volume);
    return Notes[next_note++].duration * 10;
}

void MUSIC_Beep(char* note, u16 duration, u16 interval, u8 count)
{
    vibrate = 1; // Haptic sensor set to on as default
    u8 tone=0,i;
    next_note = 1;
    Volume = Transmitter.volume * 10;
    if(! count)
        return;
    if(count > sizeof(Notes)/2)
        count = sizeof(Notes)/2;
    tone = get_note(note);
    num_notes = count*2;
    for(i=0; i<count; i++) {
        Notes[i*2].note = tone;
        Notes[i*2].duration = duration / 10;
        Notes[(i*2)+1].note = 0;
        Notes[(i*2)+1].duration = interval / 10;
    }
    SOUND_SetFrequency(get_freq(Notes[0].note), Volume);
    SOUND_Start((u16)Notes[0].duration * 10, next_note_cb, vibrate);
}

u16 MUSIC_GetSound(u16 music) {
    num_notes = 0;
    next_note = 1;
    Volume = Transmitter.volume * 10;
    #ifdef _DEVO12_TARGET_H_
    static char filename[] = "media/sound.ini\0\0\0"; // placeholder for longer folder name
    static u8 checked;
        if(!checked) {
            FILE *fh;
            fh = fopen("mymedia/sound.ini", "r");
            if(fh) {
                sprintf(filename, "mymedia/sound.ini");
                fclose(fh);
            }
            checked = 1;
        }
    #else
    char filename[] = "media/sound.ini";
    #endif
    if (music >= MUSIC_TOTAL) {
        printf("ERROR: Music %d can not be found in sound.ini", music);
        return 1;
    }
    if(CONFIG_IniParse(filename, ini_handler, (void *)sections[music])) {
        printf("ERROR: Could not read %s\n", filename);
        return 1;
    }
    return 0;
}

void MUSIC_Play(u16 music)
{
#if HAS_EXTENDED_AUDIO
    // Play audio for switch
    if ( music > MUSIC_TOTAL ) {
            AUDIO_AddQueue(music);
        return;
    }
    playback_device = AUDDEV_UNDEF;
#endif
    vibrate = 1;	// Haptic sensor set to on as default

    /* NOTE: We need to do all this even if volume is zero, because
       the haptic sensor may be enabled */

    if (MUSIC_GetSound(music)) return;


#if HAS_EXTENDED_AUDIO
    if ( !(playback_device == AUDDEV_BUZZER) ) {
        if (AUDIO_AddQueue(music)) {
            if ((playback_device == AUDDEV_EXTAUDIO) || (playback_device == AUDDEV_UNDEF)) {
                Volume = 0;
                return;
            }
        }
    }
#endif

    if(! num_notes) return;
    SOUND_SetFrequency(get_freq(Notes[next_note].note), Volume);
    SOUND_Start((u16)Notes[0].duration * 10, next_note_cb, vibrate);
}

#if HAS_EXTENDED_AUDIO

u16 MUSIC_GetTelemetryAlarm(enum Music music) {
    if ( Model.voice.telemetry[music - MUSIC_TELEMALARM1].music > 0 )
        return Model.voice.telemetry[music - MUSIC_TELEMALARM1].music;
    return music;
}

void MUSIC_PlayValue(u16 music, s32 value, u8 unit, u8 prec)
{
    u32 i;
    char digits[6]; // Do we need more?
    char thousands = 0;
    u8 digit_count = 0;

    //quick fix for enabling device=buzzer on playValue
    if (music < MUSIC_TOTAL) {
        MUSIC_GetSound(music);
        if ( (playback_device == AUDDEV_BUZZER) || (playback_device == AUDDEV_ALL) ) {
            MUSIC_Play(music);
            return;
        }
    }

    if ( !AUDIO_AddQueue(music) && (music < MUSIC_TOTAL) ) {
        MUSIC_Play(music);
        return;
    }

    // Play minutes/hours/seconds for timers
    if (unit == VOICE_UNIT_TIME) {
        if (value >= 3600) {
            i = value / 3600;
            value %= 3600;
            AUDIO_AddQueue(i + MUSIC_TOTAL);
            AUDIO_AddQueue(VOICE_UNIT_HOURS + VOICE_UNIT_OFFSET);
        }
        if (value >= 60) {
            i = value / 60;
            value %= 60;
            AUDIO_AddQueue(i + MUSIC_TOTAL);
            AUDIO_AddQueue(VOICE_UNIT_MINUTES + VOICE_UNIT_OFFSET);
        }
        if (value > 0) {
            AUDIO_AddQueue(value + MUSIC_TOTAL);
            AUDIO_AddQueue(VOICE_UNIT_SECONDS + VOICE_UNIT_OFFSET);
        }
        return;
    }

    // Add minus sign for negative number
    if (value < 0) {
        AUDIO_AddQueue(VOICE_UNIT_MINUS + VOICE_UNIT_OFFSET);
        value = -value;
    }

    //Add precision digits
    for (i=0; i < prec; i++) {
        digits[digit_count++] = value % 10;
        value /=10;
    }
    //Add decimal seperator
    if (prec > 0) {
        digits[digit_count++] = VOICE_DEC_SEP;
    }

    // Special case value == 0
    if (value == 0)
        digits[digit_count++] = 0;
    // Get single digits from remaining value
    while (value > 0) {
        if (value > 999) {
            thousands = value / 1000;
            value %= 1000;
        }
        if (value > 100) {
            digits[digit_count++] = value % 100;
            value /= 100;
            digits[digit_count++] = value + 99;
            if (thousands) {
                digits[digit_count++] = 109;  // MP3 for "thousands"
                digits[digit_count++] = thousands;
            }
            break;
        }
        if (value < 101 && value > 0) {
            digits[digit_count++] = value;
            break;
        }
        else {
            if (thousands) {
                digits[digit_count++] = 109;  // MP3 for "thousands"
                digits[digit_count++] = thousands;
            }
        }
    }

    // Fill music queue with digits
    for (i = digit_count; i > 0; i--) {
        AUDIO_AddQueue(digits[i-1] + MUSIC_TOTAL);
    }
    // Add unit for value if specified
    if (unit > VOICE_UNIT_NONE)
        AUDIO_AddQueue(unit + VOICE_UNIT_OFFSET);
}
#endif
#define TESTNAME music
#include <tests.h>
