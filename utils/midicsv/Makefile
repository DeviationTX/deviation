
CC = gcc
CFLAGS = -g -Wall

INSTALL_DEST = /usr/local

#	You shouldn't need to change anything after this line

VERSION = 1.1
PROGRAMS = midicsv csvmidi
MANPAGES = $(PROGRAMS:%=%.1) midicsv.5
DOC = README log.txt
BUILD = Makefile
SOURCE = csv.c csvmidi.c midicsv.c midio.c getopt.c getopt.h
HEADERS = csv.h midifile.h midio.h types.h version.h
EXAMPLES = test.mid bad.csv ce3k.csv acomp.pl chorus.pl \
	count_events.pl drummer.pl exchannel.pl general_midi.pl \
	transpose.pl torture.pl
WIN32EXE = Midicsv.exe Csvmidi.exe
WIN32 = $(WIN32EXE) Midicsv.sln Midicsv.vcproj Csvmidi.vcproj W32test.bat
DISTRIBUTION = $(DOC) $(BUILD) $(SOURCE) $(MANPAGES) $(HEADERS) $(EXAMPLES) $(WIN32)

all:	$(PROGRAMS)

MIDICSV_OBJ = midicsv.o midio.o getopt.o

midicsv:    $(MIDICSV_OBJ)
	$(CC) $(CFLAGS) -o midicsv midicsv.o midio.o getopt.o
	
Midicsv.exe: $(MIDICSV_OBJ:%.o=%.c)
	@echo 'Yar!  Midicsv.exe needs to be rebuilt on WIN32!'
	@exit 1

CSVMIDI_OBJ = csvmidi.o midio.o csv.o getopt.o

csvmidi:    $(CSVMIDI_OBJ)
	$(CC) $(CFLAGS) -o csvmidi csvmidi.o midio.o csv.o getopt.o
	
Csvmidi.exe: $(CSVMIDI_OBJ:%.o=%.c)
	@echo 'Yar!  Csvmidi.exe needs to be rebuilt on WIN32!'
	@exit 1

check:	all
	@./midicsv test.mid /tmp/test.csv
	@./csvmidi /tmp/test.csv /tmp/w.mid
	@./midicsv /tmp/w.mid /tmp/w1.csv
	@-cmp -s test.mid /tmp/w.mid ; if test $$? -ne 0  ; then \
	    echo '** midicsv/csvmidi: MIDI file comparison failed. **' ; else \
	diff -q /tmp/test.csv /tmp/w1.csv ; if test $$? -ne 0  ; then \
	    echo '** midicsv/csvmidi: CSV file comparison failed. **' ; else \
	    echo 'All tests passed.' ; fi ; fi
	@rm -f /tmp/test.csv /tmp/w.mid /tmp/w1.csv

pipetest: all
	./midicsv test.mid | tee /tmp/test.csv | ./csvmidi | ./midicsv - /tmp/w1.csv
	diff /tmp/test.csv /tmp/w1.csv
	rm /tmp/test.csv /tmp/w1.csv
	
torture: all
	perl torture.pl | ./csvmidi | tee /tmp/w.mid | ./midicsv | ./csvmidi >/tmp/w1.mid
	@cmp /tmp/w.mid /tmp/w1.mid ; if test $$? -ne 0  ; then \
	    echo '** midicsv/csvmidi: Torture test CSV file comparison failed. **' ; else \
	    echo 'Torture test passed.' ; fi
	@rm /tmp/w.mid /tmp/w1.mid
	
install:	all
	install -d -m 755 $(INSTALL_DEST)/bin
	install -m 755 $(PROGRAMS) $(INSTALL_DEST)/bin
	install -d -m 755 $(INSTALL_DEST)/man/man1
	install -m 644 midicsv.1 csvmidi.1 $(INSTALL_DEST)/man/man1
	install -d -m 755 $(INSTALL_DEST)/man/man5
	install -m 644 midicsv.5 $(INSTALL_DEST)/man/man5
	
uninstall:
	rm -f $(INSTALL_DEST)/bin/csvmidi $(INSTALL_DEST)/bin/midicsv
	rm -f $(INSTALL_DEST)/man/man1/csvmidi.1 $(INSTALL_DEST)/man/man1/midicsv.1
	rm -f $(INSTALL_DEST)/man/man5/midicsv.5
	
dist:	$(WIN32EXE)
	rm -f midicsv*.tar midicsv*.tar.gz
	tar cfv midicsv.tar $(DISTRIBUTION)
	mkdir midicsv-$(VERSION)
	( cd midicsv-$(VERSION) ; tar xfv ../midicsv.tar )
	rm -f midicsv.tar
	tar cfv midicsv-$(VERSION).tar midicsv-$(VERSION)
	gzip midicsv-$(VERSION).tar
	rm -rf midicsv-$(VERSION)
	rm -f midicsv-$(VERSION).zip
	zip midicsv-$(VERSION).zip $(WIN32EXE)

#	Zipped archive for building WIN32 version	
winarch:
	rm -f midicsv.zip
	zip midicsv.zip $(DISTRIBUTION)
	
#	Publish distribution on Web page (Fourmilab specific)
WEBDIR = $(HOME)/ftp/webtools/midicsv

publish: dist
	cp -p midicsv-$(VERSION).tar.gz midicsv-$(VERSION).zip $(WEBDIR)

clean:
	rm -f $(PROGRAMS) *.o *.bak core core.* *.out midicsv.zip
