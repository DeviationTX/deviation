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

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "model.h"
#include "../music.h"
#include "extended_audio.h"
#include "tx.h"
#include "voice.h"

#define MATCH_SECTION(s) (strcasecmp(section, s) == 0)
#define MATCH_KEY(s)     (strcasecmp(name,    s) == 0)

#if HAS_EXTENDED_AUDIO

const char SECTION_VOICE_GLOBAL[] = "global";
const char SECTION_VOICE_CUSTOM[] = "custom";

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    // ini_handler returns a requested mp3 label passed at *user to tempstring
    u16 req_id = *((long*)user);
    u16 id = atoi(name);
    const char* ptr = value;
    u8 k = 0;
    u16 duration = 0;

    while (*ptr && *ptr != ',') {
        ptr++;
        k++;
    }
    if (*ptr == ',') {
        duration = atoi(ptr + 1);
        }

#if HAS_MUSIC_CONFIG
    if ( k && (req_id != MAX_VOICEMAP_ENTRIES) && (req_id == id) ) {
        strlcpy(tempstring, value, k+1);
        tempstring[MAX_VOICE_LABEL] = '\0';  // limit label length
        return 1;
    }
#endif
    if ( req_id == MAX_VOICEMAP_ENTRIES ) {
        if (MATCH_SECTION(SECTION_VOICE_GLOBAL)) {
            for (int i = 0; i < CUSTOM_ALARM_ID; i++) {
                snprintf(tempstring, 4, "%d", i);
                if (MATCH_KEY(tempstring)) {
                    voice_map[i].duration = duration;
                    voice_map[i].id = i;
                    return 1;
                }
            }
        }
        if (MATCH_SECTION(SECTION_VOICE_CUSTOM)) {
            voice_map[voice_map_entries].duration = duration;
            voice_map[voice_map_entries].id = id;
            voice_map_entries++;
            return 1;
        }
        printf("Unknown entry in voice.ini: %s\n", value);
        return 0;
    }
    return 1;  // voice label ignored
}

const char* CONFIG_VoiceParse(unsigned id)
{
    #ifdef _DEVO12_TARGET_H_
    static char filename[] = "media/voice.ini\0\0\0"; // placeholder for longer folder name
    static u8 checked;
        if(!checked) {
            FILE *fh;
            fh = fopen("mymedia/voice.ini", "r");
            if(fh) {
                sprintf(filename, "mymedia/voice.ini");
                fclose(fh);
            }
            checked = 1;
        }
    #else
    char filename[] = "media/voice.ini";
    #endif
    if (id == MAX_VOICEMAP_ENTRIES) {  // initial parse of voice.ini
        voice_map_entries = CUSTOM_ALARM_ID;  // Reserve space in map for global alerts
        if (CONFIG_IniParse(filename, ini_handler, &id)) {
            printf("Failed to parse voice.ini\n");
            Transmitter.audio_player = AUDIO_NONE;  // disable external voice output
            tempstring[0] = '\0';
        }
    }
    if ( (id < MAX_VOICEMAP_ENTRIES) && (id >= CUSTOM_ALARM_ID) ) {
        if (CONFIG_IniParse(filename, ini_handler, &id)) {
            // ini handler will return tempstring with label of id
        }
    }
    return tempstring;
}
#endif
