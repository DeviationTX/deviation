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
#ifdef EMULATOR    
    {"xx",   0},    // 10 Hz tone makes the emulator crash
#else
    {"xx",   10},   // 0 Hz doesn't work well on actual hardware
#endif
    {"a",  220}, {"ax", 233}, {"b",  247},
    {"c0", 262}, {"cx0",277}, {"d0", 294}, {"dx0",311}, {"e0", 330}, {"f0", 349},
    {"fx0",370}, {"g0", 392}, {"gx0",415}, {"a0", 440}, {"ax0",466}, {"b0", 494},

    {"c1", 523}, {"cx1",554}, {"d1", 587}, {"dx1",622}, {"e1", 659}, {"f1", 698},
    {"fx1",740}, {"g1", 784}, {"gx1",831}, {"a1", 880}, {"ax1",932}, {"b1", 988},

    {"c2", 1047},{"cx2",1109},{"d2", 1175},{"dx2",1245},{"e2", 1319},{"f2", 1397},
    {"fx2",1480},{"g2", 1568},{"gx2",1661},{"a2", 1760},{"ax2",1865},{"b2", 1976},

    {"c3", 2093},{"cx3",2217},{"d3", 2349},{"dx3",2489},{"e3", 2637},{"f3", 2794},
    {"fx3",2960},{"g3", 3136},{"gx3",3322},{"a3", 3520},{"ax3",3729},{"b3", 3951},

    {"c4", 4186},{"cx4",4435},{"d4", 4699},{"dx4",4978},{"e4", 5274},{"f4", 5588},
    {"fx4",5920},{"g4", 6272},{"gx4",6645},{"a4", 7080},{"ax4",7459},{"b4", 7902},
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
    "extaudio",
};

static u8 playback_device;
static u16 music_queue[10];
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
#if HAS_EXTENDED_AUDIO
    if (Transmitter.audio_player) {
        if ((playback_device == AUDDEV_EXTAUDIO) || (playback_device == AUDDEV_UNDEF)) {
            AUDIO_Play(music_queue[next_note]);
            return MUSIC_GetDuration(music_queue[next_note++]);
        }      
    }
#endif
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

void MUSIC_Play(enum Music music)
{
#if HAS_EXTENDED_AUDIO
    // Play audio for switch
    if (Transmitter.audio_player && (music > MUSIC_TOTAL)) {
        AUDIO_Play(music);
        return;
    }
    playback_device = AUDDEV_UNDEF;
#endif
    vibrate = 1;	// Haptic sensor set to on as default

    /* NOTE: We need to do all this even if volume is zero, because
       the haptic sensor may be enabled */
    num_notes = 0;
    next_note = 1;
    Volume = Transmitter.volume * 10;
    char filename[] = "media/sound.ini\0\0\0"; // placeholder for longer folder name
    #ifdef _DEVO12_TARGET_H_
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
    #endif
    if(CONFIG_IniParse(filename, ini_handler, (void *)sections[music])) {
        printf("ERROR: Could not read %s\n", filename);
        return;
    }
#if HAS_EXTENDED_AUDIO
    if (Transmitter.audio_player) {
        if ((playback_device == AUDDEV_EXTAUDIO) || (playback_device == AUDDEV_UNDEF)) {
            AUDIO_Play(music);
            Volume = 0;	// Just activate the haptic sensor, no buzzer
        } else if (playback_device == AUDDEV_ALL) {
            AUDIO_Play(music);
        }
    }
#endif
    if(! num_notes)
        return;
    SOUND_SetFrequency(note_map[Notes[0].note].note, Volume);
    SOUND_Start((u16)Notes[0].duration * 10, next_note_cb, vibrate);
}

#if HAS_EXTENDED_AUDIO 
u16 MUSIC_GetDuration(u16 music)
{
    u8 i;
    for ( i = 0; i < sizeof(music_durations)/sizeof(music_durations[0]);i++) {
        if ( music_durations[i].music == music ) return music_durations[i].duration;
    }
    return 0;
          
}

void MUSIC_TelemValue(enum Music music, s32 telem_val, u16 telem_unit)
{
    u8 i,j;
    char digits[6];
    Volume = 0;
    next_note = 1;
    num_notes = 0;
    j = 1;
    
    // Get single digits from telemetry value
    u32 abs_telem_val = (u32)telem_val;
    while (abs_telem_val > 0) {
        digits[num_notes] = abs_telem_val % 10;
        abs_telem_val /= 10;
        ++num_notes;
    }
    
    // Fill music queue
    num_notes++;
    music_queue[0] = music;

    for (i=num_notes; i > 1; i--) {
        music_queue[j] = digits[i-2] + 1000;
        j++;
    }
    // Add decimal seperator for 
    if (telem_unit == 2) { 
        num_notes++;
        music_queue[j] = music_queue[j-1];
        music_queue[j-1] = 1010;
        j++;
    }
    if (telem_unit > 0) {
        num_notes++;
        music_queue[j] = telem_unit + 1010;
    }  

    if (Transmitter.audio_player) {
        if ((playback_device == AUDDEV_EXTAUDIO) || (playback_device == AUDDEV_UNDEF)) {
            SOUND_Start(100, next_note_cb, vibrate);
#ifdef BUILDTYPE_DEV
            for (i=0;i<num_notes;i++) {
                printf("Playing music %d for %d ms\n",music_queue[i], MUSIC_GetDuration(music_queue[i]));
            }
#endif
         }
    }
    
}
#endif
