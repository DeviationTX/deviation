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

static struct tx_configure_page * const cp = &pagemem.u.tx_configure_page;  // MACRO is not good when debugging

static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    char filename[13];
    int type;
    if (idx == 0) {
        return "English";
    }
    if (! FS_OpenDir("language"))
        return _tr("Unknown");
    while((type = FS_ReadDir(filename)) != 0) {
        if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
            idx--;
            if (idx == 0) {
                sprintf(tempstring, "language/%s", filename);
                break;
            }
        }
    }
    FS_CloseDir();
    if(idx == 0) {
        fh = fopen(tempstring, "r");
        if (fh) {
            if(fgets(tempstring, sizeof(tempstring), fh) == NULL)
                tempstring[0] = 0;
            fclose(fh);
            unsigned len = strlen(tempstring);
            if(len && tempstring[0] != ':') {
                tempstring[len-1] = '\0';
                if ((u8)tempstring[0] == 0xef
                    && (u8)tempstring[1] == 0xbb
                    && (u8)tempstring[2] == 0xbf)
                {
                    //Remove BOM
                    for(u32 i = 3; i < len; i++)
                        tempstring[i-3] = tempstring[i];
                    len -= 3;
                }
                return tempstring;
            }
        }
    }
    return _tr("Unknown");
}

