
    	    	     MIDI File CSV Editing Tools
			
			  by John Walker
		      http://www.fourmilab.ch/

This distribution contains tools which permit you to convert
standard MIDI files to a CSV (Comma-Separated Format)
equivalent which preserves all the information in the MIDI file
and then translate such CSV files back to standard MIDI files.

Exporting MIDI files as CSV makes it easy to manipulate them
with speadsheets, database utilities, or programs in languages
such as Perl and C.

The following files are included in the distribution:

    C Source Code
    	csv.c	    CSV input parser
	csv.h	    Definitions for csv.c
	csvmidi.c   CSV to MIDI translator
	getopt.c    Command line option parser
	getopt.h    Definitions for getopt.c
	midicsv.c   MIDI to CSV translator
	midifile.h  MIDI file definitions
	midio.c     MIDI file I/O routines
	midio.h     Definitions for midio.c
	types.h     Common type definitions
	version.h   Version number definition
	
    Build Utilities
    	Makefile    Make file
	
    Sample MIDI Manipulation Programs and Data
    	acomp.pl    Simple-minded algorithmic composition
	    	    example.
    	bad.csv     CSV file chock full of errors to test
	    	    csvmidi error detection and recovery.
    	ce3k.csv    Sample MIDI file in CSV format:
	    	    "Close Encounters of the Third Kind"
	count_events.pl  Perl program which counts events by type
	    	    in a CSV MIDI file and prints the results.
    	chorus.pl   Perl program which adds a one
	    	    octave lower chorus to all notes in
		    a MIDI file.
	drummer.pl  Dumbest possible drum machine; illustrates
	    	    MIDI generation from scratch.
	exchannel.pl Perl program which extracts all events on
	    	    a given channel (General MIDI percussion channel
		    as supplied), passing through meta-events
		    unchanged.
	general_midi.pl Perl include file which defines hashes
	    	    that allow General MIDI patch and percussion
		    note assignments to be specified symbolically.
	torture.pl  Perl program to generate csvmidi/midicsv
	    	    "torture test".  Include all event types and
		    verifies handling of long strings and byte
		    sequences with arbitrary content.
	transpose.pl Perl program which transposes a MIDI
	    	    file, shifting all notes down one octave
	test.mid    Sample MIDI file: "Silent Running" by Mike
	    	    and the Mechanics

    Documentation
    	README	    This file
	csvmidi.1   csvmidi manual page
	log.txt     Development log
	midicsv.1   midicsv manual page
	midicsv.5   MIDI CSV representation file format documentation
	
    WIN32 Executable Files
    	Csvmidi.exe Csvmidi WIN32 executable
	Midicsv.exe Midicsv WIN32 executable
	
    WIN32 Build Files (for Microsoft Visual Studio .NET)
    	Midicsv.sln	Solution (Workspace) for Csvmidi and Midicsv
	Csvmidi.vcproj	Csvmidi project file
	Midicsv.vcproj	Midicsv project file
	W32test.bat 	Test script for WIN32 programs
	
BUILDING AND INSTALLATION

I can't bring myself to burden such a small, simple set of programs as
this with a grotesque Autoconf script which would dwarf the source
code.  Just edit the Makefile and change the C compiler and options
if necessary, and the install directory tree if you wish to use
the install target.  The build the program with:

    make
    
and run the self-test with:

    make check

which should report "All tests passed." if all is well.  If you're
patient, have Perl installed on your system, and have lots of free
disc space, you can run the "torture test" with:

    make torture

You may then proceed to install the programs and manual pages
with

    make install
    
which, unless you've changed the INSTALL_DEST directory in the
Makefile, will install into /usr/local, which will require you
to perform the installation with super-user privilege.

COMPATIBILITY NOTES

These programs were originally developed on an MS-DOS system
with a compiler in which the "int" type was 16 bits, but it's
been a long time since they've been compiled with such a
compiler so it's unlikely they'll work with 16 bit ints without
some tweaking.  The programs don't care whether the "char" type
is signed or unsigned.  I've not tested the programs on a
system in which "int" or "long" is wider than 32 bits, but I
don't anticipate any problems.

On systems such as WIN32 which distinguish text from binary files,
it is essential that midicsv.c open its input file and csvmidi.c
open its output file in *binary* mode, otherwise the MIDI file will
be corrupted due to end of line sequence translation.  If input
and output file names are specified explicitly, the "b" option supplied
in the fopen() call should take care of this (and cause no damage on
modern Unix systems, which ignore this option).  When reading or writing
to standard input/output, however, the open file must be explicitly set
to binary mode, and the means for doing this vary among development
systems.  The midicsv.c and csvmidi.c contain code conditional on
_WIN32 which sets binary file mode with the mechanism provided by
the Microsoft Visual C/C++ library; if you're building with a different
compiler and library, you may need to change this code accordingly.

