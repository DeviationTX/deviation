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
#include <stdlib.h>
#include "tx.h"
#include "ini.h"

#ifndef SUPPORT_LANG_V2
#define SUPPORT_LANG_V2 0
#endif

#ifndef SUPPORT_DYNAMIC_LOCSTR
#define SUPPORT_DYNAMIC_LOCSTR 0
#endif

/* String long enough to hold every used string in the code
   defined as extern char tempstring[TEMPSTRINGLENGTH] in common.h */
char tempstring[TEMPSTRINGLENGTH];

#if !SUPPORT_MULTI_LANGUAGE
int CONFIG_IniParse(const char* filename,
         int (*handler)(void*, const char*, const char*, const char*),
         void* user)
{
    return ini_parse(filename, handler, user);
}
void CONFIG_ReadLang(u8 idx) {(void)idx;}
void CONFIG_EnableLanguage(int state) {(void)state;}
#else
static u16 fnv_16_str(const char *str);
static u16 table_size;
static unsigned fix_crlf(char *str);

#define MAX_LINE 300
#define MAX_STRINGS 485

#if !SUPPORT_DYNAMIC_LOCSTR
    static char strings[8192];
#else
    #define MAX_STRING_BUFFER 128

    static char strcache[MAX_STRING_BUFFER];
    static u16 str_ptr = 0;
    static FATFS LangFAT;
    static FILE * fh;
#endif

/* tempstring[] must be at least long as line[], otherwise they are too small/big to fit in each other */
#if MAX_LINE > TEMPSTRINGLENGTH
    #error "MAX_LINE > TEMPSTRINGLENGTH in language.c - CRITICAL - check length of tmpstring[] here and in common.h"
#endif

#define dbg_printf if(0) printf
static struct str_map {
    u16 hash;
    u16 pos;
} lookupmap[MAX_STRINGS];

static const char* LoadString(u16 pos, const char *str)
{
#if !SUPPORT_DYNAMIC_LOCSTR
    (void)str;
    return strings + pos;
#else
    char *ret = &strcache[str_ptr];
    char buf[MAX_LINE];
    fseek(fh, pos, SEEK_SET);
    if (fgets(buf, MAX_LINE, fh) == NULL)
        return str;

    fix_crlf(buf);
    unsigned len = strlen(buf);
    if (len + str_ptr >= MAX_STRING_BUFFER - 1)
    {
        str_ptr = 0;
        ret = &strcache[0];
    }

    strlcpy(ret, buf, MAX_STRING_BUFFER - str_ptr - 1);
    str_ptr+= len + 1;

    return ret;
#endif
}

const char *_tr(const char *str)
{
    u16 min, max, i;

    if (table_size == 0) {
        return str;
    }
    u16 hash = fnv_16_str(str);
    dbg_printf("%d: %s\n", hash, str);

    min = 0;
    max = table_size;
    while (min <= max)
    {
        i = (min + max) / 2;
        if (hash == lookupmap[i].hash)
            return LoadString(lookupmap[i].pos, str);
        else if (hash > lookupmap[i].hash)
            min = i + 1;
        else
            max = i - 1;
    }
    return str;
}

static unsigned fix_crlf(char *str)
{
    unsigned len = strlen(str);
    unsigned i, j;
    for (i = 0; i < len; i++) {
        char replace = '\0';
        if (str[i] == '\r' || str[i] == '\n') {
            str[i] = '\0';
            return i;
        }
        if (str[i] == '\\') {
            if (str[i+1] == 'n') {
                replace = '\n';
            } else if(str[i+1] == 't') {
                replace = '\t';
            }
        }
        if (replace) {
            str[i] = replace;
            for(j = i + 2; j < len; j++)
                str[j - 1] = str[j];
            len--;
        }
    }
    return len;
}

static void ReadLangV1(FILE* fh)
{
    struct str_map *lookup = lookupmap;
    unsigned pos = 0;
    unsigned len = 0;

    while (fgets(tempstring, sizeof(tempstring), fh) != NULL) {
        u16 hash;
        if(tempstring[0] == ':') {
            CLOCK_ResetWatchdog();
            fix_crlf(tempstring+1);
            if (lookup - lookupmap == MAX_STRINGS - 1) {
                printf("Only %d strings are supported aborting @ %s\n", MAX_STRINGS, tempstring);
                break;
            }
            hash = fnv_16_str(tempstring + 1);
            dbg_printf("%d: %s\n", hash, tempstring);
#if SUPPORT_DYNAMIC_LOCSTR
            pos = ftell(fh);
            if (fgets(tempstring, sizeof(tempstring), fh) == NULL) {
                break;
            }
#else
            if (fgets(tempstring, sizeof(tempstring), fh) == NULL) {
                break;
            }
            tempstring[MAX_LINE-1] = 0;
            len = fix_crlf(tempstring) + 1;
            if (pos + len > sizeof(strings)) {
                printf("Out of space processing %s\n", tempstring);
                break;
            }
            strlcpy(strings + pos, tempstring, len);
            dbg_printf("\t: %s\n", tempstring);
#endif
            // sorted insert based on hash, assume no hash conflict
            struct str_map *lookup0 = lookupmap;
            while (lookup0 < lookup && lookup0->hash < hash) {
                lookup0++;
            }

            // free up one slot to insert
            if (lookup - lookup0 > 0) {
                memmove(lookup0+1, lookup0, (lookup - lookup0)* sizeof(struct str_map));
            }

            // insert the current string
            lookup0->hash = hash;
            lookup0->pos = pos;

            lookup++;
            pos += len;
        }
    }
    table_size = lookup - lookupmap;
}

static void ReadLangV2(FILE* fh)
{
    u16 hash;
    unsigned pos = 0;
    struct str_map *lookup = lookupmap;

    while (fread(&hash, 2, 1, fh) == 1) {
        CLOCK_ResetWatchdog();
        if (lookup - lookupmap == MAX_STRINGS - 1) {
            printf("Only %d strings are supported aborting @ %s\n", MAX_STRINGS, tempstring);
            break;
        }

#if !SUPPORT_DYNAMIC_LOCSTR
        if (fgets(tempstring, sizeof(tempstring), fh) == NULL) {
            break;
        }
        tempstring[MAX_LINE-1] = 0;
        unsigned len = fix_crlf(tempstring) + 1;
        if (pos + len > sizeof(strings)) {
            printf("Out of space processing %s\n", tempstring);
            break;
        }
        dbg_printf("%d\t: %s\n", hash, tempstring);
        strlcpy(strings + pos, tempstring, len);
        lookup->pos = pos;
        pos += len;
#else
        pos = ftell(fh);
        lookup->pos = pos;
        if (fgets(tempstring, sizeof(tempstring), fh) == NULL) {
            break;
        }
#endif
        lookup->hash = hash;
        lookup++;
    }
    table_size = lookup - lookupmap;
}

static int ReadLang(const char *file)
{
#if !SUPPORT_DYNAMIC_LOCSTR
    FILE *fh = fopen(file, "r");
#else
    finit(&LangFAT, "language");
    fh = fopen2(&LangFAT, file, "r");
#endif

    if (! fh) {
        printf("Couldn't open language file: %s\n", file);
        return 0;
    }

    // first line of langauge name, ignore it
    fgets(tempstring, sizeof(tempstring), fh);

    if (SUPPORT_LANG_V2) {
        // Try to detect the version
        if (fread(tempstring, 1, 1, fh) == 1)
        {
            // move file cursor 1 byte back
            fseek(fh, -1, SEEK_CUR);
            // check the value of the next character to detect version
            if (tempstring[0] == ':')
                ReadLangV1(fh);
            else
                ReadLangV2(fh);
        }
    } else {
        ReadLangV1(fh);
    }

    if (!SUPPORT_DYNAMIC_LOCSTR)
        fclose(fh);

    return 1;
}

void CONFIG_ReadLang(u8 idx)
{
    u8 cnt = 0;
    char file[30];
    char filename[13];
    int type;

    table_size = 0;
    Transmitter.language = 0;
    if (! idx)
        return;
    if (! FS_OpenDir("language"))
        return;
    while((type = FS_ReadDir(filename)) != 0) {
        if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
            cnt++;
            if (cnt == idx) {
                sprintf(file, "language/%s", filename);
                break;
            }
        }
    }
    FS_CloseDir();
    if(cnt != idx)
        return;

    if (ReadLang(file))
        Transmitter.language = idx;
}

void CONFIG_EnableLanguage(int state)
{
    static u16 disable = 0;
    if (! state) {
        disable = table_size;
        table_size = 0;
    } else {
        table_size = disable;
    }
}

int CONFIG_IniParse(const char* filename,
         int (*handler)(void*, const char*, const char*, const char*),
         void* user)
{
    u16 tmpval = table_size;
    table_size = 0; //Disable Language parsing
    int result = ini_parse(filename, handler, user);
    table_size = tmpval;
    return result;
}

/* The following section containing the FNV hash is in the public domain
 *
 * hash_32 - 32 bit Fowler/Noll/Vo hash code
 *
 * @(#) $Revision: 5.1 $
 ***
 * Fowler/Noll/Vo hash
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * To use the recommended 32 bit FNV-1 hash, pass FNV1_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 ***
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 */

/*
 * 32 bit magic FNV-0 and FNV-1 prime
 */
#define FNV_32_PRIME ((u32)0x01000193)
#define FNV1_32_INIT ((u32)0x811c9dc5)
#define MASK_16 (((u32)1<<16)-1) /* i.e., (u_int32_t)0xffff */

/*
 * fnv_16_str - perform a 32 bit Fowler/Noll/Vo hash on a string then fold to 16bit
 *
 * input:
 *	str	- string to hash
 *
 * returns:
 *	16 bit hash as a static hash type
 *
 */
static u16 fnv_16_str(const char *str)
{
    unsigned char *s = (unsigned char *)str;	/* unsigned string */
    u32 hval = FNV1_32_INIT;
    /*
     * FNV-1 hash each octet in the buffer
     */
    while (*s) {

	/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif

	/* xor the bottom with the current octet */
	hval ^= (u32)*s++;
    }

    /* fold to 16bits (don't do this if you want 32bits */
    hval = (hval>>16) ^ (hval & MASK_16);
    /* return our new hash value */
    return hval;
}
#endif

#define TESTNAME language
#include "tests.h"
