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
struct NoteMap {
    const char str[4];
    u16 note;
};
static const struct NoteMap note_map[] = {
    {"xx",   0}, {"a",  220}, {"ax", 233}, {"b",  247},

    {"c0", 262}, {"cx0",277}, {"d0", 294}, {"dx0",311}, {"e0", 330}, {"f0", 349},
    {"fx0",370}, {"g0", 392}, {"gx0",415}, {"a0", 440}, {"ax0",466}, {"b0", 494},

    {"c1", 523}, {"cx1",554}, {"d1", 587}, {"dx1",622}, {"e1", 659}, {"f1", 698},
    {"fx1",740}, {"g1", 784}, {"gx1",831}, {"a1", 880}, {"ax1",932}, {"b1", 988},

    {"c2", 1047},{"cx2",1109},{"d2", 1175},{"dx2",1245},{"e2", 1319},{"f2", 1397},
    {"fx2",1480},{"g2", 1568},{"gx2",1661},{"a2", 1760},{"ax2",1865},{"b2", 1976},

    {"c3", 2093},{"cx3",2217},{"d3", 2349},{"dx3",2489},{"e3", 2637},{"f3", 2794},
    {"fx3",2960},{"g3", 3136},{"gx3",3322},{"a3", 3520},{"ax3",3729},{"b3", 3951},

    {"c4", 4186},{"cx4",4435},{"d4", 4699},{"dx4",4978},{"e4", 5274},{"f4", 5588},
    {"fx4",5920},{"g4", 6272},{"gx4",6645},{"a4", 7040},{"ax4",7459},{"b4", 7902},
};

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

#define NUM_NOTES (sizeof(note_map) / sizeof(struct NoteMap))


static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    u16 i;
    const char *requested_sec = (const char *)user;
    if (strcasecmp(section, requested_sec) == 0) {
#if HAS_EXTENDED_AUDIO
        if (strcasecmp("device", name) == 0) {
            for (i = 1; i < AUDDEV_LAST; i++) {
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
        for(i = 0; i < NUM_NOTES; i++) {
            if(strcasecmp(note_map[i].str, name) == 0) {
                Notes[num_notes].note = i;
                Notes[num_notes].duration = atoi(value) / 10; //convert from msec to centi-secs
                num_notes++;
                return 1;
            }
        }
    }
    return 1;
}
u16 next_note_cb() {
    if (next_note == num_notes)
        return 0;
    SOUND_SetFrequency(note_map[Notes[next_note].note].note, Volume);
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
    for(i = 0; i < NUM_NOTES; i++) {
        if(strcasecmp(note_map[i].str, note) == 0) {
            tone = i;
            break;
        }
    }
    num_notes = count*2;
    for(i=0; i<count; i++) {
        Notes[i*2].note = tone;
        Notes[i*2].duration = duration / 10;
        Notes[(i*2)+1].note = 0;
        Notes[(i*2)+1].duration = interval / 10;
    }
    SOUND_SetFrequency(note_map[Notes[0].note].note, Volume);
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
    char filename[] = "media/sound.ini\0\0\0"; // placeholder for longer folder name
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
        if (AUDIO_VoiceAvailable())
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
        if (  AUDIO_VoiceAvailable() && AUDIO_AddQueue(music) ) {
            if ((playback_device == AUDDEV_EXTAUDIO) || (playback_device == AUDDEV_UNDEF)) {
                Volume = 0;
                return;
            }
        }
    }
#endif

    if(! num_notes) return;
    SOUND_SetFrequency(note_map[Notes[0].note].note, Volume);
    SOUND_Start((u16)Notes[0].duration * 10, next_note_cb, vibrate);
}

#if HAS_EXTENDED_AUDIO

u16 MUSIC_GetTelemetryAlarm(enum Music music) {
    if ( Model.voice.telemetry[music - MUSIC_TELEMALARM1].music > 0 )
        return Model.voice.telemetry[music - MUSIC_TELEMALARM1].music;
    return music;
}

u16 MUSIC_GetTimerAlarm(enum Music music) {
    if ( Model.voice.timer[music - MUSIC_ALARM1].music > 0 )
        return Model.voice.timer[music - MUSIC_ALARM1].music;
    return music;
}

void MUSIC_PlayValue(u16 music, s32 value, u8 unit, u8 prec)
{
    u32 i;
    char digits[6]; // Do we need more?
    char thousands = 0;
    u8 digit_count = 0;

    if ( !AUDIO_VoiceAvailable() || !AUDIO_AddQueue(music)) {
        if (music < MUSIC_TOTAL)
            MUSIC_Play(music);
        return;
    }

    // Play minutes/hours/seconds for timers
    if (unit == VOICE_UNIT_TIME) {
        if (value >= 3600) {
            i = value / 3600;
            AUDIO_AddQueue(i + MUSIC_TOTAL);
            AUDIO_AddQueue(VOICE_UNIT_HOURS + VOICE_UNIT_OFFSET);
            value %= 3600;
        }
        if (value >= 60) {
            i = value / 60;
            AUDIO_AddQueue(i + MUSIC_TOTAL);
            AUDIO_AddQueue(VOICE_UNIT_MINUTES + VOICE_UNIT_OFFSET);
            value %= 60;
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
        value *= -1;
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

    // Special case value == 0 and not playing TIME
    if (value == 0 && unit != VOICE_UNIT_TIME)
        digits[digit_count++] = 0;
    // Get single digits from remaining value
    while (value > 0) {
        if(value > 999) {
            thousands = value / 1000;
            value %= 1000;
        }
        if(value > 100) {
            digits[digit_count++] = value % 100;
            value /= 100;
            digits[digit_count++] = value + 99;
            if (thousands){
              digits[digit_count++] = 109; // MP3 for "thousands"
              digits[digit_count++] = thousands;
            }
            break;
        }
        if(value < 101 && value > 0) {
            digits[digit_count++] = value;
            break;
        }
        else {
            if (thousands){
                digits[digit_count++] = 109; // MP3 for "thousands"
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
