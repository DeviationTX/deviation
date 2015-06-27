
#if ! HAS_MAPPED_GFX
    #include "../320x240x16/_mapped_gfx.h"
#else
#define _GUI_ClearMappedBox(box, color) (void)1 //This is done for us
#define _GUI_UnmapWindow(x) LCD_UnmapWindow(x)
void _GUI_CreateMappedItem_Helper(guiObject_t *graph);
void _GUI_DrawMappedStart();
void _GUI_DrawMappedStop();
#endif //HAS_MAPPED_GFX
