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

/*************************************/
/* Trims */
#define VTRIM_W 10
#define VTRIM_H 140
#define HTRIM_W 125
#define HTRIM_H 10

#define INTRIM_1_X 130
#define INTRIM_2_X (320 - INTRIM_1_X - VTRIM_W)
#define OUTTRIM_1_X 16
#define OUTTRIM_2_X (320 - OUTTRIM_1_X - VTRIM_W)
#define TRIM_12_Y 75

#define TRIM_3_X 5
#define TRIM_4_X (320 - TRIM_3_X - HTRIM_W)
#define TRIM_34_Y 220

#define TRIM_5_X 145
#define TRIM_6_X (320 - TRIM_5_X - VTRIM_W)
#define TRIM_56_Y 40
/*************************************/


#define BOX0123_X 16
#define BOX4567_X 204
#define BOX_W     100
#define BOX0145_H 40
#define BOX2367_H 24

#define BOX04_Y 40
#define BOX15_Y 90
#define BOX26_Y 150
#define BOX37_Y 185

/*************************************/
/* Model Icon */
#define MODEL_ICO_X 205
#define MODEL_ICO_Y 40
#define MODEL_ICO_W 96
#define MODEL_ICO_H 96
/*************************************/

#define GRAPH_H 59

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

#endif
