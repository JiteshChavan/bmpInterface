#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "bmp.h"
#include "testBmp.h"


int main (int argc, char *argv[]) {
    /*testBmp ();*/
    printf ("Hello World\n");
    printf ("\t>Please note that all the unit testers have been disabled by /* */ style comments\n");
    bmpPtr sampleBitmap = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sampleBitmap, RGB_24);
    saveBitMap (sampleBitmap, "helloWorld.bmp", ".");    
    destroyBmp (sampleBitmap);

    sampleBitmap = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sampleBitmap, ARGB_32);
    saveBitMap (sampleBitmap, "HelloWorld2.bmp", ".");
    destroyBmp (sampleBitmap);

    printf ("See you later\n");
    return EXIT_SUCCESS;
}