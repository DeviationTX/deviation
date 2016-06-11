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

#define MAX_LINE 80
#define MAX_NAME 50

#if HAS_EXTENDED_AUDIO
void CONFIG_AlertParse(const char* filename, u16 *switch_music_no)
{
    FILE *file;
    char *pt;
    int textlen;
    int val;
    char line[MAX_LINE];
    char switch_name[MAX_NAME];

    file = fopen(filename, "r");
    if (!file) {
	printf("Can't open alert file: %s\n", filename);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
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
        if ((pt = strchr(line, ':')) != NULL) {
            *pt = '\0';
            pt++;
            // Trim spaces
            while ((*pt != '\0') && ((*pt == ' ') || (*pt == '\t')))
                pt++;
            textlen = strlen(line);
            // Trim trailing spaces
            while ((textlen > 0) && ((line[textlen-1] == ' ') || (line[textlen-1] == '\t')))
                line[--textlen] = '\0';
            if (*pt == '\0')
                continue;
            val = atoi(pt);
	    for (int i = INP_HAS_CALIBRATION+1; i <= NUM_INPUTS; i++) {
                INPUT_SourceName(switch_name, i);
		if (!strcmp(switch_name, line)) {
                    switch_music_no[i - INP_HAS_CALIBRATION - 1] = val;
                    break;
                }
            }
        }
    }
    fclose(file);
}
#endif
