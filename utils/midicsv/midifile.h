/*

                       MIDI File Definitions  

*/

/*  MIDI command codes  */

typedef enum {

    /* Channel voice messages */

    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyphonicKeyPressure = 0xA0,
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend = 0xE0,

    /* Channel mode messages */

    ChannelMode = 0xB8,

    /* System messages */

    SystemExclusive = 0xF0,
    SystemCommon = 0xF0,
    SystemExclusivePacket = 0xF7,
    SystemRealTime = 0xF8,
    SystemStartCurrentSequence = 0xFA,
    SystemContinueCurrentSequence = 0xFB,
    SystemStop = 0xFC,

    /* MIDI file-only messages */

    FileMetaEvent = 0xFF
} midi_command;

/*  MIDI file meta-event codes  */

typedef enum {
    SequenceNumberMetaEvent = 0,
    TextMetaEvent = 1,
    CopyrightMetaEvent = 2,
    TrackTitleMetaEvent = 3,
    TrackInstrumentNameMetaEvent = 4,
    LyricMetaEvent = 5,
    MarkerMetaEvent = 6,
    CuePointMetaEvent = 7,

    ChannelPrefixMetaEvent = 0x20,
    PortMetaEvent = 0x21,
    EndTrackMetaEvent = 0x2F,

    SetTempoMetaEvent = 0x51,
    SMPTEOffsetMetaEvent = 0x54,
    TimeSignatureMetaEvent = 0x58,
    KeySignatureMetaEvent = 0x59,

    SequencerSpecificMetaEvent = 0x7F
} midifile_meta_event;

/*  The following structures are for in-memory manipulation of MIDI
    file components and must not be used for reading or writing
    MIDI files to external media.  MIDI files must be written in
    big-endian byte order with no padding to word boundaries and
    I/O code must comply with this format regardless of the host's
    in-memory representation.  */

/*  MIDI file header  */

#define MIDI_Header_Sentinel "MThd"

struct mhead {
    char chunktype[4];                /* Chunk type: "MThd" */
    long length;                      /* Length: 6 */
    short format;                     /* File format */
    short ntrks;                      /* Number of tracks in file */
    short division;                   /* Time division */
};

/*  MIDI track header  */

#define MIDI_Track_Sentinel "MTrk"

struct mtrack {
    char chunktype[4];                /* Chunk type: "MTrk" */
    long length;                      /* Length of track */
};
