#if ! HAS_MAPPED_GFX
    #include "../320x240x16/_xygraph.c"
#else
#define _ClearXYGraphBG(box, color) (void)1 //This is done for us
void _CreateXYGraph_Helper(guiXYGraph_t *graph)
{
    struct guiBox *box = &graph->header.box;
    LCD_CreateMappedWindow(1, box->x, box->y, box->width, box->height);
    box->x = 0;
    box->y = 0;
    box->width = box->width * CHAR_WIDTH;
    box->height = box->height * CHAR_HEIGHT;
    printf("w: %d h: %d\n", box->width, box->height);
}

void _DrawXYStart()
{
    LCD_SetMappedWindow(1);
}
void _DrawXYStop()
{
    LCD_SetMappedWindow(0);
}
#endif //HAS_MAPPED_GFX
