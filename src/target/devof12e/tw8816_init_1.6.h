static const u8 reg_init[] = {
 //Index register #0 page selection
 0xFF,     0,
 //0x002 - Input Format (INFORM)
 0x02,  0x44, //(6) 1 = Input crystal clock frequency is 27MHz, (2-3) 01: YOUT = YIN1
 //0x006 - Analog Control Register (ACNTL)
 0x06,     3, //???
 //0x01C - Standard Selection (SDT)
 //0x1C,  0x0F, // (0-2) Standard = Auto detection, (3) 1 = Disable VACTIVE and HDELAY shadow registers value depending on standard
 0x07,  0x12, // (0-1)HACTIVE_HI=2, (2-3)HDELAY_HI=0, (4-5)VACTIVE_HI=1, (6-7)VDELAY_HI=0
 0x08,  0x12, // VDELAY_LO
 0x09,  0x20, // VACTIVE_LO = 288, number of lines in a half-frame
 0x0A,  0x0C, // HDELAY_LO
 0x0B,  0xD0, // HACTIVE_LO = 720
 //0x01C - Standard Selection (SDT)
 0x1C,  0x07, //(0-2) Standard = Auto detection, (3) 0 = Enable VACTIVE and HDELAY shadow registers value depending on standard (first change to any 50Hz standard)
 //0x02F - Miscellaneous Control III (MISC3)
 0x2F,  0xE6,
 //0x040 to 0x04F - Scaler Input Control Registers
 0x40,     0, //default 0x00
 0x41,  0x20, // (5)Select Explicit DE = Horizontal Active is defined by individual video source (it exclude 0x47 - 0x4D)
 0x42,     0, //???
 0x43,  0x20, // DEC_VS active hign, DEC_HS active low
 0x44,  0x0C, // Input format selection = RGB
 0x45,  0x84, //???
 0x46,  0x20, //default 0x20

 //0x060 to 0x06B - Zoom Control Registers
 0x60,  0xE3, // X-Scale Up Factor midle      [65536*TV_ScaleWidth/Panel_Hactive] 58240 (TV_ScaleWidth=711)
 0x61,  0x80, // X-Scale Down Factor          {128*TV_ScaleWidth/Panel_Hactive}, default=128 (TV_ScaleWidth=800)
 0x62,  0x96, // Y-Scale Up Factor midle      (65536*TV_ScaleHeight/Panel_Vactive) 65536*283/480=38639=0x96EF (283 lines per half-frame)
 0x63,  0x00, // Y-Scale Up Factor high (2-3) (65536*TV_ScaleHeight/Panel_Vactive)
              // X-Scale Down Factor high (1) {128*TV_ScaleWidth/Panel_Hactive}
              // X-Scale Up Factor high (0)   [65536*TV_ScaleWidth/Panel_Hactive] 58240
 0x64,     0, // X-Scale Up Offset
 0x65,  0x80, // Y-Scale Up Offset for Odd field, default=128 (presumably 128 = No scaling, or 128*TV_ScaleHeight/Panel_Vactive)
 0x66,     0, // Horizontal non-display pixel number
 0x67,     0, // Non-display left/right independent control
 0x68,     0, // Horizontal scale at the side of display in panorama scaling mode
 0x69,  0x80, // X-Scale Up Factor low        [65536*TV_ScaleWidth/Panel_Hactive] 58240
 0x6A,  0xEF, // Y-Scale Up Factor low        (65536*TV_ScaleHeight/Panel_Vactive) 65536*283/480=38639=0x96EF
 0x6B,  0x80, // Y-Scale Up Offset for Even field =??? (presumably 128 = No scaling, or 128*TV_ScaleHeight/Panel_Vactive)

 //0x0B0 to 0x0C6 - PANEL CONTROL
 0xB0,  0x48, // Set pin FPDE Active High, Invert pin FPCLK polarity
 0xB1,     0, // default 0x00
 0xB2,  0xF4, // FPHS Period low (1056), FPLL/(F_ihsync * Panel_Vactive/TV_ScaleHeight) 33300000/(15723*480/283)
 0xB3,  0x30, // FPHS Active Pulse Width, default 0x10 pixels
 0xB4,  0x1B, // Flat Panel Horizontal Back Porch Width, default 0x1B pixels
 0xB5,  0x20, // FPDE Horizontal Active Length 800 pixels, 0xB6(4-6)
 0xB6,  0x34, // FPHS Period high (0-3) FPLL/(F_ihsync * Panel_Vactive/TV_ScaleHeight) 33300000/(15723*480/283)
 0xB7,  0x0D, // FPVS Period low =525 lines
 0xB8,  0x06, // FPVS Active Pulse Width, default 6 lines
 0xB9,  0x10, // Flat Panel Vertical Back Porch Width, default 0x1F lines
 0xBA,  0xE0, // Flat Panel Vertical Active Length low = 480 lines
 0xBB,  0x12, // FPVS Period high (0-2) =525 lines, Flat Panel Vertical Active Length high (4-6) = 480 lines
              // The value written in this register does not come into effect until it is followed by a register write to index 0x0B7 or 0x0BA
 0xBC,     0, // default 0x00
 0xBD,  0x0C, // Output Vsync delay from Input Vsync
 0xBE,  0x40, // Force short

 //0x0D0 to 0x0D3 - Status and Interrupt Registers
 0xD0,  0xC1,
 0xD1,     8,
 //0x0D4 to 0x0D8 - Power Management Registers
 0xD5,  0x3F,

 //0x0F9 to 0x0FE - Spread Spectum Synthesizer Control Registers
 0xF9,  0x01, // FPLL Freq high (0-3)
 0xFA,  0x7A, // FPLL Freq midle (PFLL = 0x017AE2 -> 33.3 MHz * 576/480 = 39.96 MHz)
 0xFB,  0xE2, // FPLL Freq low (108MHz*FPLL/2^17/2^POST)
 0xFE,  0x50, // FPLL POST=1 & VCO=1

 //Index register #1 page selection
 0xFF,     1,
    0,     0,
 //0x1C0 - LLPLL Input Control Register
 0xC0,     5, // PLL Input Polarity (2)=1 Normal, Clock selection (0)=1 Select oscillator clock
 //0x1CB - SOG Threshold Register
 0xCB,  0x70, // PLL power down control (6)=1 = power up
};
