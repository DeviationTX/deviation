The genspeech utility needs to be run under OSX or linux.  On OSX, it will use text-to-speech command "say"
to generate the aiff speech files.  On Linux, it will use text-to-speech command espeak to generate wave
speech files.  Then, it will use "lame" to convert aiff or wave files to mp3 format.  Please make sure
you've installed these command line utilities onto your system.

Usage:
1. For each model file, you can optionally provide a Switch/Trim Button/Auxillary Knob to Alert Message file.  For example,
   File: Model1Alert.txt
   Content: FMODE0:		Acro Mode
            FMODE1:		Angle Mode
            HOLD1:		GPS Hold Activated
            HOLD0:		GPS Hold Deactivated
            TRIMLV+:		Beeper On
            TRIMLV-:		Beeper Off
            TRIMLH+:		Taking Photo
            TRIMRV+_ON:		Lights On
            TRIMRV+_OFF:	Lights Off
            AUX4_UP:		Camera Up
            AUX4_DOWN:		Camera Down

   Description: The 1st field is the Switch, Trim Button or Auxillary Knob, followed by a colon.
                Then the message to be spoken.
                Switch: Switch name following by its postion, such as FMODE0, FMODE1, SW B0, etc.
                Trim Button: When trim buttons are used as virtual switches, they can also have voice feedback.
                             Trim buttons as ON/OFF switches, Momentary switches & Toggles switches are all
                             supported.  When used as ON/OFF switches, use only the Trim button name, such as
                             TRIMLV+, TRIMLV-, etc.  When used as Momentary switches or Toggles switches, each
                             Trim button can have 2 states, indicated by suffix "_ON" & "_OFF".  Example,
                             TRIMRV+_ON & TRIMRV+_OFF representing its toggle on/off or momentary on/off states.
                Auxillary Knob: Auxillary knob name followed by its controlling direction, "_UP" or "_DOWN"
                                (turning up or down).  For example, AUX4_UP indicating knob has been turned up;
                                and AUX4_DOWN indicating knob has been turned down.

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

