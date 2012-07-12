/*

    	    MIDI I/O Function Definitions

*/

extern long readlong(FILE *fp);
extern short readshort(FILE *fp);
extern vlint readVarLen(FILE *fp);
extern void readMidiFileHeader(FILE *fp, struct mhead *h);
extern void readMidiTrackHeader(FILE *fp, struct mtrack *t);

extern void writelong(FILE *fp, const long l);
extern void writeshort(FILE *fp, const short s);
extern void writeVarLen(FILE *fp, const vlint v);
extern void writeMidiFileHeader(FILE *fp, struct mhead *h);
extern void writeMidiTrackHeader(FILE *fp, struct mtrack *t);
