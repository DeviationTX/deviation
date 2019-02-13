#include <stdlib.h>

#include "common.h"
#include "emu.h"
#include "CuTest.h"

#include "pnglite.h"

/*
 * write current screen to a png file
 * the screen is organized in the format as plane
 */
static void WriteScreen(const char *filename)
{
    png_t png;
    const int w = IMAGE_X;
    const int h = IMAGE_Y;
    u8* img = gui.image;

    png_init(NULL, NULL);

    png_open_file_write(&png, filename);
    png_set_data(&png, w, h, 8, PNG_TRUECOLOR, img);
    png_close_file(&png);
}

void AssertScreenshot(CuTest* t, const char* filename)
{
    png_t png;
    const u8* img = gui.image;

    png_init(NULL, NULL);

    char filepath[100];
    snprintf(filepath, sizeof(filepath), "../../tests/320x240x16/%s.png", filename);

    if (png_open_file_read(&png, filepath) != PNG_NO_ERROR)
    {
        printf("Missing expected screenshot. Please add %s to git repo.\n", filepath);
        WriteScreen(filepath);
    }
    else
    {
        u8 buf[IMAGE_X * IMAGE_X * 3];

        CuAssertIntEquals(t, IMAGE_X, png.width);
        CuAssertIntEquals(t, IMAGE_Y, png.height);

        // validate picture
        png_get_data(&png, buf);

        if (memcmp(buf, img, IMAGE_X * IMAGE_Y * 3) != 0)
        {
            char buf[100];
            snprintf(buf, sizeof(buf), "Screenshot %s content mismatch! Check filesystem/test/ for actual content.\n", filename);
            snprintf(filepath, sizeof(filepath), "%s.png", filename);
            WriteScreen(filepath);
            CuFail(t, buf);
        }
    }
}

