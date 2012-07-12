/*

	             Encode MIDI file into CSV format

	Designed and implemented in December of 1995 by John Walker.
	Revised and updated by John Walker in October 1998 and
	February 2004.

		       http://www.fourmilab.ch/
		       
                This program is in the public domain.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "version.h"
#include "types.h"
#include "midifile.h"
#include "midio.h"
#include "getopt.h"

#define FALSE	0
#define TRUE	1

static char *progname;		      /* Program name string */
static int verbose = FALSE; 	      /* Debug output */

/*  VLENGTH  --  Parse variable length item from in-memory track  */

static vlint vlength(byte **trk, long *trklen)
{
    vlint value;
    byte ch;
    byte *cp = *trk;

    trklen--;
    if ((value = *cp++) & 0x80) {
	value &= 0x7F;
	do {
	    value = (value << 7) | ((ch = *cp++) & 0x7F);
	    trklen--;
	} while (ch & 0x80);
    }
#ifdef DUMP
    fprintf(stderr, "Time lapse: %d bytes, %d\n", cp - *trk, value);
#endif
    *trk = cp;
    return value;
}

/*  TEXTCSV  --  Convert text field to CSV, quoting as necessary.

		 If CSV_Quote_ISO if defined ISO 8859-1 graphic
		 characters are permitted in text strings
		 without octal quoting.  Otherwise all
		 characters other than 7-bit ASCII graphics are
		 output as octal.
		 
		 Output is appended directly to output stream fo.
*/

#define CSV_Quote_ISO

static void textcsv(FILE *fo, const byte *t, const int len)
{
    byte c;
    int i;

    putc('"', fo);
    for (i = 0; i < len; i++) {
    	c = *t++;
        if ((c < ' ') ||
#ifdef CSV_Quote_ISO
	    ((c > '~') && (c <= 160))
#else
    	    (c > '~')
#endif
	   ) {
            putc('\\', fo);
            fprintf(fo, "%03o", c);
	} else {
            if (c == '"') {
                putc('"', fo);
            } else if (c == '\\') {
                putc('\\', fo);
	    }
	    putc(c, fo);
	}
    }
    putc('"', fo);
}

/*  TRACKCSV  --  Compile track into CSV written to fo.  */

static void trackcsv(FILE *fo, const int trackno,
    	    	     byte *trk, long trklen, const int ppq)
{
    int levt = 0, evt, channel, note, vel, control, value,
	type;
    vlint len;
    byte *titem;
    vlint abstime = 0;		      /* Absolute time in track */
#ifdef XDD
byte *strk = trk;
#endif

    while (trklen > 0) {
	vlint tlapse = vlength(&trk, &trklen);
	abstime += tlapse;

        fprintf(fo, "%d, %ld, ", trackno, abstime);

	/* Handle running status; if the next byte is a data byte,
	   reuse the last command seen in the track. */

	if (*trk & 0x80) {
#ifdef XDD
fprintf(fo, " (Trk: %02x NS: %02X : %d) ", *trk, evt, trk - strk);
#endif
	    evt = *trk++;
	    
	    /* One subtlety: we only save channel voice messages
	       for running status.  System messages and file
	       meta-events (all of which are in the 0xF0-0xFF
	       range) are not saved, as it is possible to carry a
	       running status across them.  You may have never seen
	       this done in a MIDI file, but I have. */
	       
	    if ((evt & 0xF0) != 0xF0) {
	    	levt = evt;
	    }
	    trklen--;
	} else {
	    evt = levt;
#ifdef XDD
fprintf(fo, " (Trk: %02x RS: %02X : %d) ", *trk, evt, trk - strk);
#endif
	}

	channel = evt & 0xF;

	/* Channel messages */

	switch (evt & 0xF0) {

	    case NoteOff:	 /* Note off */
		if (trklen < 2) {
		    return;
		}
		trklen -= 2;
		note = *trk++;
		vel = *trk++;
                fprintf(fo, "Note_off_c, %d, %d, %d\n", channel, note, vel);
		continue;

	    case NoteOn:	 /* Note on */
		if (trklen < 2) {
		    return;
		}
		trklen -= 2;
		note = *trk++;
		vel = *trk++;
		/*  A note on with a velocity of 0 is actually a note
		    off.  We do not translate it to a Note_off record
		    in order to preserve the original structure of the
		    MIDI file.	*/
                fprintf(fo, "Note_on_c, %d, %d, %d\n", channel, note, vel);
		continue;

	    case PolyphonicKeyPressure: /* Aftertouch */
		if (trklen < 2) {
		    return;
		}
		trklen -= 2;
		note = *trk++;
		vel = *trk++;
                fprintf(fo, "Poly_aftertouch_c, %d, %d, %d\n", channel, note, vel);
		continue;

	    case ControlChange:  /* Control change */
		if (trklen < 2) {
		    return;
		}
		trklen -= 2;
		control = *trk++;
		value = *trk++;
                fprintf(fo, "Control_c, %d, %d, %d\n", channel, control, value);
		continue;

	    case ProgramChange:  /* Program change */
		if (trklen < 1) {
		  return;
		}
		trklen--;
		note = *trk++;
                fprintf(fo, "Program_c, %d, %d\n", channel, note);
		continue;

	    case ChannelPressure: /* Channel pressure (aftertouch) */
		if (trklen < 1) {
		    return;
		}
		trklen--;
		vel = *trk++;
                fprintf(fo, "Channel_aftertouch_c, %d, %d\n", channel, vel);
		continue;

	    case PitchBend:	 /* Pitch bend */
	       if (trklen < 1) {
		   return;
		}
		trklen--;
		value = *trk++;
		value = value | ((*trk++) << 7);
                fprintf(fo, "Pitch_bend_c, %d, %d\n", channel, value);
		continue;

	    default:
		break;
	}

	switch (evt) {

	    /* System exclusive messages */

	    case SystemExclusive:
	    case SystemExclusivePacket:
		len = vlength(&trk, &trklen);
                fprintf(fo, "System_exclusive%s, %lu",
                    evt == SystemExclusivePacket ? "_packet" : "",
		    len);
		{
		    vlint i;

		    for (i = 0; i < len; i++) {
                        fprintf(fo, ", %d", *trk++);
		    }
                    fprintf(fo, "\n");
		}
		break;

	    /* File meta-events */

	    case FileMetaEvent:

		if (trklen < 2) {
		    return;
		}
		trklen -= 2;
		type = *trk++;
		len = vlength(&trk, &trklen);
		titem = trk;
		trk += len;
		trklen -= len;

		switch (type) {
		    case SequenceNumberMetaEvent:
                        fprintf(fo, "Sequence_number, %d\n", (titem[0] << 8) | titem[1]);
			break;

		    case TextMetaEvent:
#ifdef XDD
fprintf(fo, " (Len=%ld  Trk=%02x) ", len, *trk);
#endif
    	    	    	fputs("Text_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case CopyrightMetaEvent:
    	    	    	fputs("Copyright_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case TrackTitleMetaEvent:
    	    	    	fputs("Title_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case TrackInstrumentNameMetaEvent:
    	    	    	fputs("Instrument_name_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case LyricMetaEvent:
    	    	    	fputs("Lyric_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case MarkerMetaEvent:
    	    	    	fputs("Marker_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case CuePointMetaEvent:
    	    	    	fputs("Cue_point_t, ", fo);
			textcsv(fo, titem, len);
			putc('\n', fo);
			break;

		    case ChannelPrefixMetaEvent:
                        fprintf(fo, "Channel_prefix, %d\n", titem[0]);
			break;

		    case PortMetaEvent:
                        fprintf(fo, "MIDI_port, %d\n", titem[0]);
			break;

		    case EndTrackMetaEvent:
                        fprintf(fo, "End_track\n");
			trklen = -1;
			break;

		    case SetTempoMetaEvent:
                        fprintf(fo, "Tempo, %d\n", (titem[0] << 16) |
			       (titem[1] << 8) | titem[2]);
			break;

		    case SMPTEOffsetMetaEvent:
                        fprintf(fo, "SMPTE_offset, %d, %d, %d, %d, %d\n",
			    titem[0], titem[1], titem[2], titem[3], titem[4]);
			break;

		    case TimeSignatureMetaEvent:
                        fprintf(fo, "Time_signature, %d, %d, %d, %d\n",
				titem[0], titem[1], titem[2], titem[3]);
			break;

		    case KeySignatureMetaEvent:
                        fprintf(fo, "Key_signature, %d, \"%s\"\n", ((signed char) titem[0]),
                                titem[1] ? "minor" : "major");
			break;

		    case SequencerSpecificMetaEvent:
                        fprintf(fo, "Sequencer_specific, %lu", len);
			{
			    vlint i;

			    for (i = 0; i < len; i++) {
                                fprintf(fo, ", %d", titem[i]);
			    }
                            fprintf(fo, "\n");
			}
			break;

		    default:
			if (verbose) {
                            fprintf(stderr, "Unknown meta event type 0x%02X, %ld bytes of data.\n",
				    type, len);
			}
                        fprintf(fo, "Unknown_meta_event, %d, %lu", type, len);
			{
			    vlint i;

			    for (i = 0; i < len; i++) {
                                fprintf(fo, ", %d", titem[i]);
			    }
                            fprintf(fo, "\n");
			}
			break;
	      }
	      break;

	   default:
	      if (verbose) {
                  fprintf(stderr, "Unknown event type 0x%02X.\n", evt);
	      }
              fprintf(fo, "Unknown_event, %02Xx\n", evt);
	      break;
	}
    }
}

/*  Main program.  */

int main(int argc, char *argv[])
{
    struct mhead mh;
    FILE *fp, *fo;
    long track1;
    int i, n, track1l;

    fp = stdin;
    fo = stdout;
    progname = argv[0];
    while ((n = getopt(argc, argv, "uv")) != -1) {
	switch (n) {
            case 'u':
                fprintf(stderr, "Usage: %s [ options ] [ midi_file ] [ csv_file ]\n", progname);
                fprintf(stderr, "       Options:\n");
                fprintf(stderr, "           -u            Print this message\n");
                fprintf(stderr, "           -v            Verbose: dump header and track information\n");
		fprintf(stderr, "Version %s\n", VERSION);
		return 0;

            case 'v':
		verbose = TRUE;
		break;

            case '?':
                fprintf(stderr, "%s: undefined option -%c specified.\n",
		    progname, n);
		return 2;
	}
    }

    i = 0;
    while (optind < argc) {
	switch (i++) {
	    case 0:
                if (strcmp(argv[optind], "-") != 0) {
                    fp = fopen(argv[optind], "rb");
		    if (fp == NULL) {
                        fprintf(stderr, "%s: Unable to to open MIDI input file %s\n",
				progname, argv[optind]);
			return 2;
		    }
		}
		break;

	    case 1:
                if (strcmp(argv[optind], "-") != 0) {
                    fo = fopen(argv[optind], "w");
		    if (fo == NULL) {
                        fprintf(stderr, "%s: Unable to to create CSV output file %s\n",
				progname, argv[optind]);
			return 2;
		    }
		}
		break;
	}
	optind++;
    }
#ifdef _WIN32

    /*  If input is from standard input, set the input file
    	mode to binary.  */

    if (fp == stdin) {
	_setmode(_fileno(fp), _O_BINARY);
    }
#endif

    /* Read and validate header */

    readMidiFileHeader(fp, &mh);
    if (memcmp(mh.chunktype, "MThd", sizeof mh.chunktype) != 0) {
        fprintf(stderr, "%s is not a Standard MIDI File.\n", argv[1]);
	return 2;
    }
    if (verbose) {
        fprintf(stderr, "Format %d MIDI file.  %d tracks, %d ticks per quarter note.\n",
	      mh.format, mh.ntrks, mh.division);
    }

    /*	Output header  */

    fprintf(fo, "0, 0, Header, %d, %d, %d\n", mh.format, mh.ntrks, mh.division);

    /*	Process tracks */

    for (i = 0; i < mh.ntrks; i++) {
	struct mtrack mt;
	byte *trk;

	if (i == 0) {
	    track1 = ftell(fp);
	}

	readMidiTrackHeader(fp, &mt);
        if (memcmp(mt.chunktype, "MTrk", sizeof mt.chunktype) != 0) {
            fprintf(stderr, "Track %d header is invalid.\n", i + 1);
	    return 2;
	}

	if (verbose) {
            fprintf(stderr, "Track %d: length %ld.\n", i + 1, mt.length);
	}
        fprintf(fo, "%d, 0, Start_track\n", i + 1);

	trk = (byte *) malloc(mt.length);
	if (trk == NULL) {
             fprintf(stderr, "%s: Cannot allocate %ld bytes for track.\n",
		progname, mt.length);
	     return 2;
	}

	fread((char *) trk, (int) mt.length, 1, fp);
	if (i == 0) {
	    track1l = (int) (ftell(fp) - track1);
	}

	trackcsv(fo, i + 1, trk, mt.length, mh.division);
	free(trk);
    }
    fprintf(fo, "0, 0, End_of_file\n");
    return 0;
}
