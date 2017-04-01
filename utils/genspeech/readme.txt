The genspeech utility needs to be run under OSX or linux.  On OSX, it will use text-to-speech command "say"
to generate the aiff speech files.  On Linux, it will use text-to-speech commands pico2wave or espeak to generate wave speech files. Then, it will use "lame" to convert aiff or wave files to mp3 format. Once generated, the duration of each audio file gets checked with mplayer (linux) or afinfo (OSX). Please make sure you've installed these command line utilities onto your system.

Usage:
1. For global alerts as well as numerical values, decimal seperator and units the file globalAlert.txt is processed. Please note you can not change the labels defined but only the texts after ":" in each line.

2. For custom alerts a file modelAlert.txt will be read to generate model specific messages selectable in the voice menu. 

3. Run "genspeech" and it will generate a mapping file named "voice.ini" which contains all messages created as well as each files duration.

4. The voice.ini will need to be copied to your Transmitter under "media" directory.

5. A new directory named "mp3" will also be generated with the voice files in it.  These voice files will
   need to be moved to your DFPlayer Mini or other hardware players.
   
6. A voice.zip file containing voice.ini and the mp3 directory for further use (for example uploading to deviationtx.com).

7. You can supply an command-line option which will be appended to all filenames created. For example:
   genspeech EN
   will generate all files from modelAlertEN.txt and globalAlertEN.txt, generate voice.ini and mp3 folders and zip it
   all into voiceEN.ini. This especially useful for creating "language" packs from the sample files in the genspeech folder.
