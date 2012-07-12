/*

	Comma separated value database format scanner

	This function implements a somewhat extended flavour
	of CSV.  In addition to the standard quoted fields,
	permitting embedded commas, with embedded quotes
        represented as "", backslash escaped characters
	expressed as three octal digits are also permitted,
	with a double backslash representing an embedded
	backslash.  This is necessary to permit fields
	which include end-of-line delimiters which would
	otherwise truncate the record when it is read.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "csv.h"

#define BufferInitial	256	    /* Initial buffer size */
#define BufferExpansion 1024	    /* Buffer expansion increment */

#define EOS     '\0'

#define FALSE	0
#define TRUE	1

static const char *csptr;   	    /* CSV scan pointer */

/*  CSVSCANINIT  --  Initialise scanning of a CSV record.  */

void CSVscanInit(const char *s)
{
    csptr = s;
}

/*  CSVSCANFIELD  --  Scan next field from a CSV record.  The actual
		      length of the field is placed in the
		      global variable CSVfieldLength.  If the
		      field is not quoted, leading and trailing
		      spaces are discarded.
		      
		      The argument b_f is a pointer to a 
*/

int CSVfieldLength = 0;     	/* Length of CSV field scanned */

#define f   	(*b_f)
#define flen	(*b_flen)

static void expand_buf(char **b_f, int *b_flen)
{
    if ((f == NULL) || (flen == 0)) {
    	f = (char *) malloc(BufferInitial);
	if (f == NULL) {
	    fprintf(stderr, "Unable to allocate %d byte CSV field buffer.\n", BufferInitial);
	    abort();
    	}
	flen = BufferInitial;
    } else {
    	flen += BufferExpansion;
    	f = (char *) realloc(f, flen);
	if (f == NULL) {
	    fprintf(stderr, "Unable to expand CSV field buffer to %d bytes.\n", flen);
	    abort();
    	}	
    }
}

#define store(c) if (CSVfieldLength >= (flen - 1)) \
    	    	    { expand_buf(b_f, b_flen); }   \
		 f[CSVfieldLength] = c;            \
		 CSVfieldLength++;

int CSVscanField(char **b_f, int *b_flen)
{
    int foundfield = FALSE, quoted = FALSE;

    CSVfieldLength = 0;
    if (*csptr != EOS) {
	foundfield = TRUE;
	while ((*csptr != EOS) && isspace(*csptr)) {
	    csptr++;
	}
        if (*csptr == '"') {
	    quoted = TRUE;
	    csptr++;
	    while (*csptr != EOS) {
                if (*csptr == '"') {
                    if (csptr[1] == '"') {
                        store('"');
			csptr += 2;
		    } else {
			csptr++;
			break;
		    }
                } else if (*csptr == '\\') {
                    if (csptr[1] == '\\') {
                        store('\\');
			csptr += 2;
		    } else {
			unsigned int v = 0;
			int i;

    	    	    	for (i = 0; i < 3; i++) {
			    csptr++;
			    if ((*csptr >= '0') && (*csptr <= '7')) {
			    	v = (v << 3) | (*csptr - '0');
			    } else {
			    	csptr--;
    	    	    	    }
			}
			csptr++;
			store((char) v);
		    }
		} else {
		    char c = *csptr++;
		    store(c);
		}
	    }
	}
        while (*csptr != ',' && *csptr != EOS) {
	    char c = *csptr++;
	    store(c);
	}
        if (*csptr == ',') {
	    csptr++;
	}
    }
    f[CSVfieldLength] = EOS;	    	/* Append C string terminator */
    
    /*	If the field wasn't quoted, elide any trailing spaces.  */
    
    if (foundfield && !quoted) {
	while (CSVfieldLength > 0 && isspace(f[CSVfieldLength - 1])) {
	    f[--CSVfieldLength] = EOS;
	}
    }
    return foundfield;
}
#undef f
#undef flen
