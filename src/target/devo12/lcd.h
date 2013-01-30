#define LCD_00_PRODCODE   0x00 // Product Code Register
#define LCD_02_CONF_READ  0x02 // Configuration Readback Register
#define LCD_04_PLL_DDIV   0x04 // PLL D-Divider Register
#define LCD_06_PLL0       0x06 // PLL Setting Register 0
#define LCD_08_PLL1       0x08 // PLL Setting Register 1
#define LCD_0A_PLL2       0x0A // PLL Setting Register 2
#define LCD_0C_NDIV       0x0C // PLL N-Divider Register
#define LCD_0E_SSCON1     0x0E // SS Control Register 0
#define LCD_10_SSCON1     0x10 // SS Control Register 1
#define LCD_12_CLK_SRC    0x12 // Clock Source Select Register
#define LCD_14_PANELTYPE  0x14 // LCD Panel Type Register
#define LCD_16_HORIZ_WID  0x16 // Horizontal Display Width Register (HDISP)
#define LCD_18_HORIZ_NDP  0x18 // Horizontal Non-Display Period Register (HNDP)
#define LCD_1A_VERT_HGHT0  0x1A // Vertical Display Height Register 0 (VDISP)
#define LCD_1C_VERT_HGHT1  0x1C // Vertical Display Height Register 1 (VDISP)
#define LCD_1E_VERT_NDP   0x1E // Vertical Non-Display Period Register (VNDP)
#define LCD_20_PHS_HSW    0x20 // PHS Pulse Width Register (HSW)
#define LCD_22_PHS_HPS    0x22 // PHS Pulse Start Position Register (HPS)
#define LCD_24_PVS_VSW    0x24 // PVS Pulse Width Register (VSW)
#define LCD_26_PVS_VPS    0x26 // PVS Pulse Start Position Register (VPS)
#define LCD_28_PCLK_POL   0x28 // PCLK Polarity Register
#define LCD_2A_DISPMODE   0x2A // Display Mode Register
#define LCD_2C_PIP1_DISP_START0  0x2C // PIP1 Display Start Address Register 0
#define LCD_2E_PIP1_DISP_START1  0x2E // PIP1 Display Start Address Register 1
#define LCD_30_PIP1_DISP_START2  0x30 // PIP1 Display Start Address Register 2
#define LCD_32_PIP1_XSTART   0x32 // PIP1 Window X Start Position Register
#define LCD_34_PIP1_YSTART0  0x34 // PIP1 Window Y Start Position Register 0
#define LCD_36_PIP1_YSTART1  0x36 // PIP1 Window Y Start Position Register 1
#define LCD_38_PIP1_XEND     0x38 // PIP1 Window X End Position Register
#define LCD_3A_PIP1_YEND0    0x3A // PIP1 Window Y End Position Register 0
#define LCD_3C_PIP1_YEND1    0x3C // PIP1 Window Y End Position Register 1
#define LCD_3E_PIP2_DISP_START0  0x3E // PIP2 Display Start Address Register 0
#define LCD_40_PIP2_DISP_START1  0x40 // PIP2 Display Start Address Register 1
#define LCD_42_PIP2_DISP_START2  0x42 // PIP2 Display Start Address Register 2
#define LCD_44_PIP2_XSTART   0x44 // PIP2 Window X Start Position Register
#define LCD_46_PIP2_YSTART0  0x46 // PIP2 Window Y Start Position Register 0
#define LCD_48_PIP2_YSTART1  0x48 // PIP2 Window Y Start Position Register 1
#define LCD_4A_PIP2_XEND     0x4A // PIP2 Window X End Position Register
#define LCD_4C_PIP2_YEND0    0x4C // PIP2 Window Y End Position Register 0
#define LCD_4E_PIP2_YEND1    0x4E // PIP2 Window Y End Position Register 1
#define LCD_50_DISPCON       0x50 // Display Control Register
#define LCD_52_INP_MODE      0x52 // Input Mode Register
#define LCD_54_TRANS_KEY_RED    0x54 // Transparency Key Color Red Register
#define LCD_56_TRANS_KEY_GREEN  0x56 // Transparency Key Color Green Register
#define LCD_58_TRANS_KEY_BLUE   0x58 // Transparency Key Color Blue Register
#define LCD_5A_WRWIN_XSTART   0x5A // Write Window X Start Position Register
#define LCD_5C_WRWIN_YSTART0  0x5C // Write Window Y Start Position Register 0
#define LCD_5E_WRWIN_YSTART1  0x5E // Write Window Y Start Position Register 1
#define LCD_60_WRWIN_XEND     0x60 // Write Window X End Position Register
#define LCD_62_WRWIN_YEND0    0x62 // Write Window Y End Position Register 0
#define LCD_64_WRWIN_YEND1    0x64 // Write Window Y End Position Register 1
#define LCD_66_MEM_PORT0      0x66 // Memory Data Port Register 0
#define LCD_67_MEM_PORT1      0x67 // Memory Data Port Register 1
#define LCD_68_PWR_SAVE       0x68 // Power Save Register
#define LCD_6A_NONDISP_PERIOD  0x6A // Non-Display Period Control / Status Register
#define LCD_6C_GPOUT0         0x6C // General Purpose Output Register 0
#define LCD_6E_GPOUT1         0x6E // General Purpose Output Register 1
#define LCD_70_PWM_CTRL       0x70 // PWM Control Register
#define LCD_72_PWM_HIGH0      0x72 // PWM High Duty Cycle Register 0
#define LCD_74_PWM_HIGH1      0x74 // PWM High Duty Cycle Register 1
#define LCD_76_PWM_HIGH2      0x76 // PWM High Duty Cycle Register 2
#define LCD_78_PWM_HIGH3      0x78 // PWM High Duty Cycle Register 3
#define LCD_7A_PWM_LOW0       0x7A // PWM Low Duty Cycle Register 0
#define LCD_7C_PWM_LOW1       0x7C // PWM Low Duty Cycle Register 1
#define LCD_7E_PWM_LOW2       0x7E // PWM Low Duty Cycle Register 2
#define LCD_80_PWM_LOW3       0x80 // PWM Low Duty Cycle Register 3
#define LCD_82_SDRAM_CTRL     0x82 // SDRAM Control Register
#define LCD_84_SDRAM_STATUS0  0x84 // SDRAM Status Register 0
#define LCD_86_SDRAM_STATUS1  0x86 // SDRAM Status Register 1
#define LCD_88_SDRAM_MRS0     0x88 // SDRAM MRS Value Register 0
#define LCD_8A_SDRAM_MRS1     0x8A // SDRAM MRS Value Register 1
#define LCD_8C_SDRAM_RFRSH_CNT0  0x8C // SDRAM Refresh Counter Register 0
#define LCD_8E_SDRAM_RFRSH_CMT1  0x8E // SDRAM Refresh Counter Register 1
#define LCD_90_SDRAM_BUFSIZE  0x90 // SDRAM Write Buffer Memory Size Register 0
#define LCD_92_SDRAM_DEBUG    0x92 // SDRAM Debug Register
#define LCD_94_ALPHA_CTRL     0x94 // Alpha-Blend Control Register
#define LCD_96_RESERVED       0x96 // is Reserved
#define LCD_98_ALPHA_HSIZE    0x98 // Alpha-Blend Horizontal Size Register
#define LCD_9A_ALPHA_VSIZE0   0x9A // Alpha-Blend Vertical Size Register 0
#define LCD_9C_ALPHA_VSIZE1   0x9C // Alpha-Blend Vertical Size Register 1
#define LCD_9E_ALPHA_VALUE    0x9E // Alpha-Blend Value Register
#define LCD_A0_ALPHA_INP1_START0  0xA0 // Alpha-Blend Input Image 1 Start Address Register 0
#define LCD_A2_ALPHA_INP1_START1  0xA2 // Alpha-Blend Input Image 1 Start Address Register 1
#define LCD_A4_ALPHA_INP1_START2  0xA4 // Alpha-Blend Input Image 1 Start Address Register 2
#define LCD_A6_ALPHA_INP2_START0  0xA6 // Alpha-Blend Input Image 2 Start Address Register 0
#define LCD_A8_ALPHA_INP2_START1  0xA8 // Alpha-Blend Input Image 2 Start Address Register 1
#define LCD_AA_ALPHA_INP2_START2  0xAA // Alpha-Blend Input Image 2 Start Address Register 2
#define LCD_AC_ALPHA_OUT_START0   0xAC // Alpha-Blend Output Image Start Address Register 0
#define LCD_AE_ALPHA_OUT_START1   0xAE // Alpha-Blend Output Image Start Address Register 1
#define LCD_B0_ALPHA_OUT_START2   0xB0 //Alpha-Blend Output Image Start Address Register 2
#define LCD_B2_INTRPT_CTRL    0xB2 // Interrupt Control Register
#define LCD_B4_INTRPT_STATUE  0xB4 // Interrupt Status Register
#define LCD_B6_INTRPT_CLEAR   0xB6 // Interrupt Clear Register
