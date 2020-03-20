#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "bmp.h"

#define INVALID_PIXEL_FORMAT 0

void setupForImage (bmpPtr masterPiece, LONG xRes, LONG yRes, pixelFormat pixelFormat);

int main (int argc, char *argv) {
    printf (">Hello World\n");

    LONG xRes = 1920;
    LONG yRes = 1080;

    bmpPtr masterPiece;
    masterPiece = createBmp (BITMAPV4HEADER);
    setupForImage (masterPiece, xRes, yRes, ARGB_32);
    
    channelPtr myRed = createChannel (xRes, yRes);
    channelPtr myGreen = createChannel (xRes, yRes);
    channelPtr myBlue = createChannel (xRes, yRes);
    channelPtr myAlpha = createChannel (xRes, yRes);

    time_t t;
    srand (time (&t));

    int row = 0;
    while (row < yRes) {
        int col = 0;
        while (col < xRes) {
            byte redVal = rand () % 256;
            byte greenVal = rand () % 256;
            byte blueVal = rand () % 256;
            byte alphaVal = 150+ (rand () % 90);
            setPixel (row, col, myRed, redVal);
            setPixel (row, col, myGreen, greenVal);
            setPixel (row, col, myBlue, blueVal);
            setPixel (row, col, myAlpha, alphaVal);
            col ++;
        }
        row ++;
    }
    setChannel (RED, masterPiece, myRed);
    setChannel (GREEN, masterPiece, myGreen);
    setChannel (BLUE, masterPiece, myBlue);
    setChannel (ALPHA, masterPiece, myAlpha);

    destroyChannel (myRed);
    destroyChannel (myGreen);
    destroyChannel (myBlue);
    destroyChannel (myAlpha);

    saveBitMap (masterPiece, "apple.bmp", "..");
    destroyBmp (masterPiece);

    bmpPtr art = createBmp (BITMAPINFOHEADER);
    setupForImage (art, xRes, yRes, RGB_24);

    myRed = createChannel (xRes, yRes);
    myGreen = createChannel (xRes, yRes);
    myBlue = createChannel (xRes, yRes);
    myAlpha = createChannel (xRes, yRes);

    row = 0;
    while (row < yRes) {
        int col = 0;
        while (col < xRes) {
            byte redVal = rand () % 256;
            byte greenVal = rand () % 256;
            byte blueVal = rand () % 256;
            setPixel (row, col, myRed, redVal);
            setPixel (row, col, myGreen, greenVal);
            setPixel (row, col, myBlue, blueVal);
            col ++;
        }
        row ++;
    }

    setChannel (RED, masterPiece, myRed);
    setChannel (GREEN, masterPiece, myGreen);
    setChannel (BLUE, masterPiece, myBlue);

    destroyChannel (myRed);
    destroyChannel (myGreen);
    destroyChannel (myBlue);

    saveBitMap (art, "art.bmp", "..");
    destroyBmp (art);

    printf (">See you later\n");
    return EXIT_SUCCESS;
}

void setupForImage (bmpPtr masterPiece, LONG xRes, LONG yRes, pixelFormat pixelFormat) {
    DIBHeaderVersion version = getDIBHeaderVersion (masterPiece);
    if (pixelFormat == ARGB_32) {
        assert (version == BITMAPV4HEADER);
        setPixelFormat (masterPiece, ARGB_32);
        setColorDepth (masterPiece, BPP_32);
        setCompression (masterPiece, BI_BITFIELDS);
        setColorSpace (masterPiece, DEFAULT_V4IH_COLOR_SPACE);
        setPrintResX (masterPiece, DEFAULT_V4IH_PRINT_RES_X);
        setPrintResY (masterPiece, DEFAULT_V4IH_PRINT_RES_Y);
        setPaletteColorCount (masterPiece, DEFAULT_V4IH_PALETTE_CLR_COUNT);
        setImpColorCount (masterPiece, DEFAULT_V4IH_IMP_COLOR_COUNT);
    } else if (pixelFormat == RGB_24) {
        assert (version == BITMAPINFOHEADER);
        setPixelFormat (masterPiece, RGB_24);
        setColorDepth (masterPiece, BPP_24);
        setCompression (masterPiece, BI_RGB);
        setPrintResX (masterPiece, DEFAULT_IH_PRINT_RES_X);
        setPrintResY (masterPiece, DEFAULT_IH_PRINT_RES_Y);
        setPaletteColorCount (masterPiece, DEFAULT_IH_PALETTE_CLR_COUNT);
        setImpColorCount (masterPiece, DEFAULT_IH_IMP_COLOR_COUNT);
    } else {
        assert (INVALID_PIXEL_FORMAT);
    }
    setXRes (masterPiece, xRes);
    setYRes (masterPiece, yRes);
    setUpPixelArray (masterPiece);
    setColorPlaneCount (masterPiece, 1);
    DWORD imageSizeInBytes = evaluateRawImageSizeInBytes (masterPiece);
    setImageSize (masterPiece, imageSizeInBytes);
    return;
}