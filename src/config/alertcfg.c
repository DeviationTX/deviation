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

#if HAS_EXTENDED_AUDIO

void CONFIG_AlertParse(const char* filename)
{
    FILE *file;
    char *pt;
    int textlen;
    int val;
    u32 j = 0;
    music_map_entries = 0;
    memset(music_map,0,sizeof(music_map));
    char line[MAX_MUSICMAP_ENTRIES];
    file = fopen(filename, "r");
    if (!file) {
	printf("Can't open music mapping file: %s\n", filename);
        return;
    }

    while ((fgets(line, sizeof(line), file) != NULL) && music_map_entries < MAX_MUSICMAP_ENTRIES) {
        j=0;
        textlen = strlen(line);
        // strip LF or CRLF
        if (line[textlen-1] == '\n') {
            line[--textlen] = '\0';
            if ((textlen > 0) && (line[textlen-1] == '\r'))
                line[--textlen] = '\0';
        }
        // Ignore comments or empty lines
        if ((line[0] == '\0') || (line[0] == ';'))
            continue;
        pt = strtok(line,":");
        while(pt != NULL) {
            j++;
            val = atoi(pt);
            switch (j) {
              case 1: music_map[music_map_entries].musicid = val; break;
              case 2: music_map[music_map_entries].duration = val; break;
#if HAS_MUSIC_CONFIG
              case 3: strlcpy(music_map[music_map_entries].label,pt,MAX_MUSIC_LABEL);
#endif
            }
            pt = strtok(NULL, ":");
        }
            music_map_entries++;
    }
    fclose(file);
}
#endif
