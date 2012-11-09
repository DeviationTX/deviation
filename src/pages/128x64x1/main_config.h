#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

enum MainWidget {
    TRIM1,
    TRIM2,
    TRIM3,
    TRIM4,
    TRIM5,
    TRIM6,
    TOGGLE1,
    TOGGLE2,
    TOGGLE3,
    TOGGLE4,
    BOX1,
    BOX2,
    BOX3,
    BOX4,
    BOX5,
    BOX6,
    BOX7,
    BOX8,
    MODEL_ICO,
    BAR1,
    BAR2,
    BAR3,
    BAR4,
    BAR5,
    BAR6,
    BAR7,
    BAR8,
};

#define TOGGLE_FILE "media/toggle.bmp"

/*************************************/
/* Trims */
#define OUTTRIM_OFFSET 24

#define VTRIM_W 4 //10
#define VTRIM_H 53 // 140
#define HTRIM_W 53 //125
#define HTRIM_H 4 //10

#define INTRIM_1_X 58 //130
#define INTRIM_2_X (LCD_WIDTH - INTRIM_1_X - VTRIM_W)
#define OUTTRIM_1_X 2 //16
#define OUTTRIM_2_X (LCD_WIDTH - OUTTRIM_1_X - VTRIM_W)
#define TRIM_12_Y 10 // 75

#define TRIM_3_X 3 //5
#define TRIM_4_X (LCD_WIDTH - TRIM_3_X - HTRIM_W)
#define TRIM_34_Y 59 //220

#define TRIM_5_X 145
#define TRIM_6_X (320 - TRIM_5_X - VTRIM_W)
#define TRIM_56_Y 40
/*************************************/


#define BOX0123_X 3 //16
#define BOX4567_X 66 //204
#define BOX_W     40 //100
#define BOX0145_H 9 //40
#define BOX2367_H 9 //24

#define BOX04_Y 22 //40
#define BOX15_Y 31 //90
#define BOX26_Y 39 //150
#define BOX37_Y 49 //185

/*************************************/
/* Model Icon */
#define MODEL_ICO_X 75
#define MODEL_ICO_Y 20
#define MODEL_ICO_W 52
#define MODEL_ICO_H 36
/*************************************/

#define GRAPH1_X BOX0123_X
#define GRAPH2_X (320 - BOX_W - GRAPH1_X)
#define GRAPH_Y BOX26_Y
#define GRAPH_H 59
#define GRAPH_W 10
#define GRAPH_SPACE ((BOX_W - GRAPH_W) / 3)

/*************************************/
/* Toggle Icons */
#define TOGGLE_W 32
#define TOGGLE_H 32

#define TOGGLE_CNTR_X 145
#define TOGGLE_CNTR_Y 40
#define TOGGLE_CNTR_SPACE 48

#define TOGGLE_LR_Y 182
#define TOGGLE_LR_SPACE 40
#define TOGGLE_L_X 10
#define TOGGLE_R_X (320 - TOGGLE_L_X - 2 * TOGGLE_LR_SPACE - TOGGLE_W)
/*************************************/

u8 MAINPAGE_GetWidgetLoc(enum MainWidget widget, u16 *x, u16 *y, u16 *w, u16 *h);

#endif
