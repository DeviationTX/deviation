#include <stdlib.h>

#include "common.h"
#include "emu.h"
#include "CuTest.h"

#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER
{
    u16 bfType;  //specifies the file type
    u32 bfSize;  //specifies the size in bytes of the bitmap file
    u16 bfReserved1;  //reserved; must be 0
    u16 bfReserved2;  //reserved; must be 0
    u32 bfOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    u32 biSize;  //specifies the number of bytes required by the struct
    u32 biWidth;  //specifies width in pixels
    u32 biHeight;  //species height in pixels
    u16 biPlanes; //specifies the number of color planes, must be 1
    u16 biBitCount; //specifies the number of bit per pixel
    u32 biCompression;//spcifies the type of compression
    u32 biSizeImage;  //size of image in bytes
    u32 biXPelsPerMeter;  //number of pixels per meter in x axis
    u32 biYPelsPerMeter;  //number of pixels per meter in y axis
    u32 biClrUsed;  //number of colors used by th ebitmap
    u32 biClrImportant;  //number of colors that are important
}BITMAPINFOHEADER;

#pragma pack(pop)

static void WriteBMP(const char *filename)
{
    FILE *f;
    const int w = IMAGE_X;
    const int h = IMAGE_Y;
    const u8* img = gui.image;

    BITMAPFILEHEADER header;
    BITMAPINFOHEADER info;
    unsigned char bmppad[3] = {0,0,0};

    memset(&header, 0, sizeof(header));
    memset(&info, 0, sizeof(info));

    header.bfType = 0x4D42; // "BM"
    header.bfSize = sizeof(header) + sizeof(info) + 3 * w * h;

    info.biSize = sizeof(info);
    info.biWidth = w;
    info.biHeight = h;
    info.biPlanes = 1;
    info.biBitCount = 24;

    f = fopen(filename,"wb");
    if (!f) {
        return;
    }
    fwrite(&header,1,sizeof(header),f);
    fwrite(&info,1,sizeof(info),f);
    for(int i=0; i<h; i++)
    {
        fwrite(img+(w*(h-i-1)*3),3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }

    fclose(f);
}

void AssertScreenshot(CuTest* t, const char* filename)
{
    FILE *f;
    const int w = IMAGE_X;
    const int h = IMAGE_Y;
    const u8* img = gui.image;

    char filepath[100] = "../../tests/320x240x16/";
    strcat(filepath, filename);
    f = fopen(filepath, "rb");
    if (f == NULL)
    {
        printf("==Missing validation picture. ==\nPlease add %s to git.\n", filepath);
        WriteBMP(filepath);
    }
    else
    {
        BITMAPFILEHEADER header;
        BITMAPINFOHEADER info;
        u8 buf[3 * IMAGE_X];

        fread(&header, 1, sizeof(header), f);
        fread(&info, 1, sizeof(info), f);

        CuAssertIntEquals(t, IMAGE_X, info.biWidth);
        CuAssertIntEquals(t, IMAGE_Y, info.biHeight);

        int match = 1;
        // validate picture
        for(int i=0; i<h; i++)
        {
            fread(buf, 3, w, f);
            if (memcmp(buf, img+(w*(h-i-1)*3), 3 * w) != 0) {
                match = 0;
                break;
            }
            fread(buf, 1, (4-(w*3)%4)%4, f);
        }

        if (!match)
        {
            char buf[100];
            sprintf(buf, "Screenshot %s content mismatch", filename);
            WriteBMP(filename);
            CuFail(t, buf);
        }
    }
}

