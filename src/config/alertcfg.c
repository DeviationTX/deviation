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

#define MAX_LINE 80
#define MAX_NAME 50

#if HAS_EXTENDED_AUDIO

void CONFIG_AlertParse(const char* filename)
{
    FILE *file;
    char *pt;
    int textlen;
    int val;
    u8 j,k = 0;
    char line[MAX_LINE];
    char name[MAX_NAME];
    const char *button_name;

    file = fopen(filename, "r");
    if (!file) {
	printf("Can't open alert file: %s\n", filename);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        j=0;
        textlen = strlen(line);
        // strip LF or CRLF
        if (line[textlen-1] == '\n') {
            line[--textlen] = '\0';
            if ((textlen > 0) && (line[textlen-1] == '\r'))
                line[--textlen] = '\0';
        }
        // strip trailing spaces
        while ((textlen > 0) && ((line[textlen-1] == ' ') || (line[textlen-1] == '\t')))
            line[--textlen] = '\0';
        // Ignore comments or empty lines
        if ((line[0] == '\0') || (line[0] == ';'))
            continue;
        pt = strtok(line,":");
        while(pt != NULL) { 
            //printf(" \t%s",pt);
            j++;
            val = atoi(pt);
            if (j == 3) music_durations[k].duration = val;
            if (j == 2) {
                music_durations[k].music = val;
                for (int i = INP_HAS_CALIBRATION+1; i <= NUM_INPUTS; i++) {
                    INPUT_SourceName(name, i);
                    if (!strcmp(name, line)) {
                        Model.switch_music_no[i - INP_HAS_CALIBRATION - 1] = val;
                        break;
                    }
                }
                for (int i = 1; i <= NUM_TX_BUTTONS; i++) {
                    button_name = INPUT_ButtonName(i);
                    strcpy(name, button_name);
                    strcat(name, "_ON");
                    // Button name alone or with suffix "_ON" will be considered the same
                    if (!strcmp(button_name, line) || !strcmp(name, line)) {
                        Model.button_music_no[i-1].on_state_music = val;
                        break;
                    }
                    strcpy(name, button_name);
                    strcat(name, "_OFF");
                    if (!strcmp(name, line)) {
                        Model.button_music_no[i-1].off_state_music = val;
                        break;
                    }
                }
#if NUM_AUX_KNOBS
                for (int i = NUM_STICKS+1; i <= NUM_STICKS + NUM_AUX_KNOBS; i++) {
                    INPUT_SourceName(name, i);
                    strcat(name, "_UP");
                    if (!strcmp(name, line)) {
                        Model.aux_music_no[i - (NUM_STICKS+1)].up_state_music = val;
                        break;
                    }
                    INPUT_SourceName(name, i);
                    strcat(name, "_DOWN");
                    if (!strcmp(name, line)) {
                        Model.aux_music_no[i - (NUM_STICKS+1)].down_state_music = val;
                        break;
                    }
                }
#endif
            }
            pt = strtok(NULL, ":");
            
        }
        k++;    
    }
    fclose(file);
}
#endif
