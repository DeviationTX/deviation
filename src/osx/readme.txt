The genspeech utility needs to be run under OSX.  It will use OSX text-to-speech command "say" to generate
the aiff speech files.  Then, it will use "lame" (if installed) to convert aiff files to mp3 format.
If you don't have lame installed, you'll need to convert the aiff files to mp3 format by yourself.

Usage:
1. For each model file, you can optionally provide a Switch to Alert Message file.  For example,
   File: Model1Alert.txt
   Content: FMODE0:		Acro Mode
            FMODE1:		Angle Mode
            HOLD1:		GPS Hold Activated
            HOLD0:		GPS Hold Deactivated

   Description: The 1st field is the Switch name, followed by a colon.  Then the message to be spoken.

2. Place all the Switch to Alert Message files under "models" directory.  There are 2 sample Alert Message files
   provided in the "models" directory.  You can remove them before placing your own files.

3. Run "genspeech" and it will generate a mapping file for each Switch to Alert Message file, such as
   model1.map, model2.map, etc.  The mapping file contains the mapping of Switches and their corresponding
   voice message number.  For example, below model1.map will be generated from the above Model1Alert.txt file.
   File: model1.map
   Content: FMODE0:0100
            FMODE1:0101
            HOLD1:0102
            HOLD0:0103

4. All the mapping files above will need to be copied to your Transmitter under "models" directory.

5. A new directory named "mp3" will also be generated with various voice files in it.  These voice files will
   need to be moved to your DFPlayer Mini or other hardware players.

