
struct ImageMap TGLICO_GetImage(int idx)
{
    static u16 w = 0;
    if(w == 0) {
        u16 h;
	LCD_ImageDimensions(TOGGLE_FILE, &w, &h);
    }
    struct ImageMap img;
    if (idx == 0) {
        img.file = GRAY_FILE;
        img.x_off = 0;
        img.y_off = 0;
    } else {
        idx--;
        img.file = TOGGLE_FILE;
        img.x_off = (idx * TOGGLEICON_WIDTH) % w;
        img.y_off = ((idx * TOGGLEICON_WIDTH) / w) * TOGGLEICON_HEIGHT;
    }
    return img;
}
