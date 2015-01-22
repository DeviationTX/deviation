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
    {"xx",    0}, {"a",  220}, {"ax", 233}, {"b",  247},

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
};

#define NUM_NOTES (sizeof(note_map) / sizeof(struct NoteMap))

    
static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    u16 i;
    const char *requested_sec = (const char *)user;
    if (strcasecmp(section, requested_sec) == 0) {
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

void MUSIC_Play(enum Music music)
{
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
    if(! num_notes)
        return;
    SOUND_SetFrequency(note_map[Notes[0].note].note, Volume);
    SOUND_Start((u16)Notes[0].duration * 10, next_note_cb);
}
