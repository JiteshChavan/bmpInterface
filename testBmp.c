#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "testBmp.h"
#include "bmp.h"

static void testCreateBitMap ();
static void testAccessFunctions ();
static void testCompareChannels ();
static void testChannelAccessFunctions ();
static void testInitializeBmpDFLT ();
static void testMiscOps ();
static void testWriteAndParse ();
static void testDeterMineFileSizeInBytes ();
static void testEvaluateRawImageSizeInBytes ();

static void compareChannels (channelPtr before, channelPtr after);
void testBmp () {
    printf ("\n>Testing ADT:bmp\n");
    testCreateBitMap ();
    testAccessFunctions ();
    testCompareChannels ();
    testChannelAccessFunctions ();
    testInitializeBmpDFLT ();
    testWriteAndParse ();
    testMiscOps ();
    printf (">Woohoo!! All passed\n> MIC DROP!!!\n");
    return;
}

static void testCreateBitMap () {
    printf ("\t>testing createBmp ()\n");
    bmpPtr bitMap = createBmp (BITMAPINFOHEADER);
    DIBHeaderVersion version = getDIBHeaderVersion (bitMap);
    DWORD dibSize = getDIBHeaderSize (bitMap);
    assert (version == BITMAPINFOHEADER);
    assert (dibSize == DEFAULT_IH_SIZE);
    bmpPtr sample = createBmp (BITMAPV4HEADER);
    version = getDIBHeaderVersion (sample);
    dibSize = getDIBHeaderSize (sample);
    assert (version == BITMAPV4HEADER);
    assert (dibSize == DEFAULT_V4IH_SIZE);
    destroyBmp (sample);
    destroyBmp (bitMap);
    return;
}
static void testAccessFunctions () {
    printf ("\t>testing accessFunctions \n");
    
    bmpPtr bitMap = createBmp (BITMAPINFOHEADER);
    
    setDIBHeaderVersion (bitMap, BITMAPINFOHEADER);
    DIBHeaderVersion temp = getDIBHeaderVersion (bitMap);
    assert (temp == BITMAPINFOHEADER);
    DWORD dibSize = getDIBHeaderSize (bitMap);
    assert (dibSize == BITMAPINFOHEADER_SIZE);

    DWORD hSize = getDIBHeaderSize (bitMap);
    assert (hSize == BITMAPINFOHEADER_SIZE);
    
    setPixelFormat (bitMap, RGB_24);
    pixelFormat format = getPixelFormat (bitMap);
    assert (format == RGB_24);

    setXRes (bitMap, 5);
    LONG xRes = getXRes (bitMap);
    assert (xRes == 5);

    setYRes (bitMap, 4);
    LONG yres = getYRes (bitMap);
    assert (yres == 4);

    setColorPlaneCount (bitMap, 1);
    WORD t = getColorPlaneCount (bitMap);
    assert (t == COLOR_PLANCE_COUNT);

    setColorDepth (bitMap, BPP_24);
    WORD depth = getColorDepth (bitMap);
    assert (depth == BPP_24);

    setCompression (bitMap, BI_RGB);
    DWORD comp = getCompression (bitMap);
    assert (comp == BI_RGB);

    setImageSize (bitMap, 1);
    DWORD iSIze = getImageSize (bitMap);
    assert (iSIze == 1);

    setPrintResX (bitMap, 9);
    LONG pRex = getPrintResX (bitMap);
    assert (pRex == 9);

    setPrintResY (bitMap, 8);
    pRex = getPrintResY (bitMap);
    assert (pRex == 8);

    setPaletteColorCount (bitMap,12);
    DWORD xD = getPaletteColorCount (bitMap);
    assert (xD == 12);

    setImpColorCount (bitMap, 16);
    xD = getImpColorCount (bitMap);
    assert (xD == 16);

    setDIBHeaderVersion (bitMap, BITMAPV4HEADER);
    setColorSpace (bitMap, LCS_WINDOWS_COLOR_SPACE);
    colorSpace cSpace = getColorSpace (bitMap);
    assert (cSpace == LCS_WINDOWS_COLOR_SPACE);

    destroyBmp (bitMap);    

    return;
}

// initialization tests for other pixelFormats should be written here
static void testInitializeBmpDFLT () {
    printf ("\t>testing initializeBmpDFLT ()\n\t\tfor BITMAPINFOHEADER/RGB24 defaults\n");
    
    bmpPtr bitMap = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (bitMap, RGB_24);

    DIBHeaderVersion version = getDIBHeaderVersion (bitMap);
    assert (version == BITMAPINFOHEADER);

    DWORD ihSize = getDIBHeaderSize (bitMap);
    assert (ihSize == DEFAULT_IH_SIZE);

    pixelFormat pixForm = getPixelFormat (bitMap);
    assert (pixForm == RGB_24);

    LONG xRes = getXRes (bitMap);
    assert (xRes == DEFAULT_IH_XRES_RGB_24);

    LONG yRes = getYRes (bitMap);
    assert (yRes == DEFAULT_IH_YRES_RGB_24);

    WORD cPlaneCount = getColorPlaneCount (bitMap);
    assert (cPlaneCount == DEFAULT_IH_COLOR_PLANE_COUNT);

    WORD colorDepth = getColorDepth (bitMap);
    assert (colorDepth == DEFAULT_IH_COLOR_DEPTH_RGB_24);
    
    DWORD compression = getCompression (bitMap);
    assert (compression == DEFAULT_IH_COMPRESSION);
    
    DWORD imageSize = getImageSize (bitMap);
    assert (imageSize == DEFAULT_IH_IMAGE_SIZE_RGB_24);

    LONG printResX = getPrintResX (bitMap);
    assert (printResX == DEFAULT_IH_PRINT_RES_X);

    LONG printResY = getPrintResY (bitMap);
    assert (printResY == DEFAULT_IH_PRINT_RES_Y);

    DWORD paletteColorCOunt, impColorCOunt;
    paletteColorCOunt = getPaletteColorCount (bitMap);
    impColorCOunt = getImpColorCount (bitMap);
    assert (paletteColorCOunt == DEFAULT_IH_PALETTE_CLR_COUNT);
    assert (impColorCOunt == DEFAULT_IH_IMP_COLOR_COUNT);

    // tests for rgb24 example image on wikipedia 
    // blue green
    // red white
    channelPtr red = getRedChannel (bitMap);
    channelPtr blue = getBlueChannel (bitMap);
    channelPtr green = getGreenChannel (bitMap);

    LONG xResCh = getChXRes (red);
    assert (xRes == xResCh);
    LONG yresCh = getChYRes (red);
    assert (yresCh == yRes);
    
    xResCh = getChXRes (blue);
    assert (xRes == xResCh);
    yresCh = getChYRes (blue);
    assert (yresCh == yRes);
    
    xResCh = getChXRes (green);
    assert (xRes == xResCh);
    yresCh = getChYRes (green);
    assert (yresCh == yRes);
    
    // 0 0 (BLUE)
    byte pixVal = getPixel (0,0,red);
    assert (pixVal == 0);
    pixVal = getPixel (0,0,blue);
    assert (pixVal == 255);

    pixVal = getPixel (0,0,green);
    assert (pixVal == 0);
    //0 1 (GREEN)
    pixVal = getPixel (0,1,red);
    assert (pixVal == 0);

    pixVal = getPixel (0,1,blue);
    assert (pixVal == 0);

    pixVal = getPixel (0,1,green);
    assert (pixVal == 255);
    //1 0 (RED)
    pixVal = getPixel (1,0,red);
    assert (pixVal == 255);

    pixVal = getPixel (1,0,blue);
    assert (pixVal == 0);

    pixVal = getPixel (1,0,green);
    assert (pixVal == 0);
    // 1 1 (WHITE)
    pixVal = getPixel (1,1,red);
    assert (pixVal == 255);

    pixVal = getPixel (1,1,blue);
    assert (pixVal == 255);

    pixVal = getPixel (1,1,green);
    assert (pixVal == 255);
    
    destroyBmp (bitMap);
    destroyChannel (red);
    destroyChannel (blue);
    destroyChannel (green);

    printf ("\t\t>for BITMAPV4HEADER/ARGB32 defaults\n");

    bmpPtr sample = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sample, ARGB_32);

    colorSpace cSpace = getColorSpace (sample);
    assert (cSpace == LCS_WINDOWS_COLOR_SPACE);

    version = getDIBHeaderVersion (sample);
    assert (version == BITMAPV4HEADER);

    ihSize = getDIBHeaderSize (sample);
    assert (ihSize == DEFAULT_V4IH_SIZE);

    pixForm = getPixelFormat (sample);
    assert (pixForm == ARGB_32);

    xRes = getXRes (sample);
    assert (xRes == DEFAULT_V4IH_XRES_ARGB_32);

    yRes = getYRes (sample);
    assert (yRes == DEFAULT_V4IH_YRES_ARGB_32);

    cPlaneCount = getColorPlaneCount (sample);
    assert (cPlaneCount == DEFAULT_V4IH_COLOR_PLANE_COUNT);

    colorDepth = getColorDepth (sample);
    assert (colorDepth == DEFAULT_V4IH_COLOR_DEPTH_ARGB_32);
    
    compression = getCompression (sample);
    assert (compression == DEFAULT_V4IH_COMPRESSION);
    
    imageSize = getImageSize (sample);
    assert (imageSize == DEFAULT_V4IH_IMAGE_SIZE_ARGB_32);

    printResX = getPrintResX (sample);
    assert (printResX == DEFAULT_V4IH_PRINT_RES_X);

    printResY = getPrintResY (sample);
    assert (printResY == DEFAULT_V4IH_PRINT_RES_Y);

    paletteColorCOunt = getPaletteColorCount (sample);
    impColorCOunt = getImpColorCount (sample);
    assert (paletteColorCOunt == DEFAULT_V4IH_PALETTE_CLR_COUNT);
    assert (impColorCOunt == DEFAULT_V4IH_IMP_COLOR_COUNT);

    // tests for argb32 example image on wikipedia 
    // blue green red white (ALPHA = 127)
    // blue green red white (ALPHA = 255)

    channelPtr alpha = getAlphaChannel (sample);
    red = getRedChannel (sample);
    blue = getBlueChannel (sample);
    green = getGreenChannel (sample);

    xResCh = getChXRes (red);
    assert (xRes == xResCh);
    yresCh = getChYRes (red);
    assert (yresCh == yRes);
    
    xResCh = getChXRes (blue);
    assert (xRes == xResCh);
    yresCh = getChYRes (blue);
    assert (yresCh == yRes);
    
    xResCh = getChXRes (green);
    assert (xRes == xResCh);
    yresCh = getChYRes (green);
    assert (yresCh == yRes);
    
    // 0 0 (BLUE/ALPHA 127)
    pixVal = getPixel (0,0,red);
    assert (pixVal == 0);
    pixVal = getPixel (0,0,blue);
    assert (pixVal == 255);
    pixVal = getPixel (0,0,green);
    assert (pixVal == 0);
    pixVal = getPixel (0,0, alpha);
    assert (pixVal == 127);

    // 0 1 (GREEN/ALPHA 127)
    pixVal = getPixel (0,1,red);
    assert (pixVal == 0);
    pixVal = getPixel (0,1,blue);
    assert (pixVal == 0);
    pixVal = getPixel (0,1,green);
    assert (pixVal == 255);
    pixVal = getPixel (0,1, alpha);
    assert (pixVal == 127);
    // 0 2 (RED/ALPHA 127)
    pixVal = getPixel (0,2,red);
    assert (pixVal == 255);
    pixVal = getPixel (0,2,blue);
    assert (pixVal == 0);
    pixVal = getPixel (0,2,green);
    assert (pixVal == 0);
    pixVal = getPixel (0,2, alpha);
    assert (pixVal == 127);
    // 0 3 (WHITE/ALPHA 127)
    pixVal = getPixel (0,3,red);
    assert (pixVal == 255);
    pixVal = getPixel (0,3,blue);
    assert (pixVal == 255);
    pixVal = getPixel (0,3,green);
    assert (pixVal == 255);
    pixVal = getPixel (0,3, alpha);
    assert (pixVal == 127);

    // 1 0 (BLUE/ALPHA 255)
    pixVal = getPixel (1,0,red);
    assert (pixVal == 0);
    pixVal = getPixel (1,0,blue);
    assert (pixVal == 255);
    pixVal = getPixel (1,0,green);
    assert (pixVal == 0);
    pixVal = getPixel (1,0, alpha);
    assert (pixVal == 255);

    // 1 1 (GREEN/ALPHA 255)
    pixVal = getPixel (1,1,red);
    assert (pixVal == 0);
    pixVal = getPixel (1,1,blue);
    assert (pixVal == 0);
    pixVal = getPixel (1,1,green);
    assert (pixVal == 255);
    pixVal = getPixel (1,1, alpha);
    assert (pixVal == 255);
    // 1 2 (RED/ALPHA 255)
    pixVal = getPixel (1,2,red);
    assert (pixVal == 255);
    pixVal = getPixel (1,2,blue);
    assert (pixVal == 0);
    pixVal = getPixel (1,2,green);
    assert (pixVal == 0);
    pixVal = getPixel (1,2, alpha);
    assert (pixVal == 255);
    // 1 3 (WHITE/ALPHA 255)
    pixVal = getPixel (1,3,red);
    assert (pixVal == 255);
    pixVal = getPixel (1,3,blue);
    assert (pixVal == 255);
    pixVal = getPixel (1,3,green);
    assert (pixVal == 255);
    pixVal = getPixel (1,3, alpha);
    assert (pixVal == 255);

    destroyBmp (sample);
    destroyChannel (red);
    destroyChannel (blue);
    destroyChannel (green);
    destroyChannel (alpha);
    return;
}

static void testMiscOps () {
    printf ("\t>testing miscOps ()\n");
    testDeterMineFileSizeInBytes ();
    testEvaluateRawImageSizeInBytes ();
    printf ("\t>miscOps successfully tested\n");
}
static void testDeterMineFileSizeInBytes () {
    printf ("\t\t>testing determineFileSizeInBytes ()\n");
    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sample, RGB_24);
    DWORD fileSize = determineFileSizeInBytes (sample);
    assert (fileSize == 70);
    destroyBmp (sample);

    sample = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sample, ARGB_32);
    fileSize = determineFileSizeInBytes (sample);
    assert (fileSize == 154);
    destroyBmp (sample);
    return;
}

static void testEvaluateRawImageSizeInBytes () {
    printf ("\t\t>testing evaluateRawImageSizeInBytes ()\n");
    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sample, RGB_24);
    DWORD imageSizeBytes = evaluateRawImageSizeInBytes (sample);
    assert (imageSizeBytes == 16);
    destroyBmp (sample);
    
    sample = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sample, ARGB_32);
    imageSizeBytes = evaluateRawImageSizeInBytes (sample);
    assert (imageSizeBytes == 32);
    destroyBmp (sample);
    return;
}

// hard coded testCase (subject within the same directory).
static void testWriteAndParse () {
    printf ("\t>testing parseBitMap () and saveBitMap () for BITMAPINFOHEADER/RGB24\n");
    bmpPtr bitMap = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (bitMap, RGB_24);

    // before writing to hardDrive
    DIBHeaderVersion versionBefore = getDIBHeaderVersion (bitMap);
    DWORD sizeBefore = getDIBHeaderSize (bitMap);
    LONG xResBefore = getXRes (bitMap);
    LONG yResBefore = getYRes (bitMap);
    WORD colorPlaneCountBefore = getColorPlaneCount (bitMap);
    WORD colorDepthBefore = getColorDepth (bitMap);
    DWORD compressionBefore =  getCompression (bitMap);
    DWORD imageSizeBefore = getImageSize (bitMap);
    LONG  printResXBefore = getPrintResX (bitMap);
    LONG  printResYBefore = getPrintResY (bitMap);
    DWORD paletteColorCountBefore = getPaletteColorCount (bitMap);
    DWORD impColorCountBefore = getImpColorCount (bitMap);

    channelPtr redBefore = getRedChannel (bitMap);
    channelPtr greenBefore = getGreenChannel (bitMap);
    channelPtr blueBefore = getBlueChannel (bitMap);

    saveBitMap (bitMap, "RGB24", ".");
    destroyBmp (bitMap);
    
    bmpPtr image = parseBitMap ("./RGB24");
    
    DIBHeaderVersion versionAfter = getDIBHeaderVersion (image);
    DWORD sizeAfter = getDIBHeaderSize (image);
    LONG xResAfter = getXRes (image);
    LONG yResAfter = getYRes (image);
    WORD colorPlaneCountAfter = getColorPlaneCount (image);
    WORD colorDepthAfter = getColorDepth (image);
    DWORD compressionAfter =  getCompression (image);
    DWORD imageSizeAfter = getImageSize (image);
    LONG  printResXAfter = getPrintResX (image);
    LONG  printResY = getPrintResY (image);
    DWORD paletteColorCountAfter = getPaletteColorCount (image);
    DWORD impColorCountAfter = getImpColorCount (image);

    channelPtr redAfter = getRedChannel (image);
    channelPtr greenAfter = getGreenChannel (image);
    channelPtr blueAfter = getBlueChannel (image);

    assert (sizeAfter == sizeBefore);
    assert (versionAfter == versionBefore);
    assert (xResAfter == xResBefore);
    assert (yResBefore == yResAfter);
    assert (colorPlaneCountAfter == colorPlaneCountBefore);
    assert (colorDepthAfter == colorDepthBefore);
    assert (compressionAfter == compressionBefore);
    assert (imageSizeAfter == imageSizeBefore);
    assert (printResXAfter == printResXBefore);
    assert (paletteColorCountAfter == paletteColorCountBefore);
    assert (impColorCountAfter == impColorCountBefore);
    
    compareChannels (redAfter, redBefore);
    compareChannels (greenAfter, greenBefore);
    compareChannels (blueAfter, blueBefore);

    destroyChannel (redBefore);
    destroyChannel (redAfter);
    destroyChannel (greenBefore);
    destroyChannel (greenAfter);
    destroyChannel (blueBefore);
    destroyChannel (blueAfter);
    destroyBmp (image);
    int retCode = remove ("./RGB24");
    assert (retCode == 0);

    printf ("\t>testing parseBitMap () and saveBitMap () for BITMAPV4HEADER/ARGB32\n");

    bitMap = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (bitMap, ARGB_32);
    // before writing to hardDrive
    versionBefore = getDIBHeaderVersion (bitMap);
    sizeBefore = getDIBHeaderSize (bitMap);
    xResBefore = getXRes (bitMap);
    yResBefore = getYRes (bitMap);
    colorPlaneCountBefore = getColorPlaneCount (bitMap);
    colorDepthBefore = getColorDepth (bitMap);
    compressionBefore =  getCompression (bitMap);
    imageSizeBefore = getImageSize (bitMap);
    printResXBefore = getPrintResX (bitMap);
    printResYBefore = getPrintResY (bitMap);
    paletteColorCountBefore = getPaletteColorCount (bitMap);
    impColorCountBefore = getImpColorCount (bitMap);
    colorSpace cSpaceBefore = getColorSpace (bitMap);
    
    redBefore = getRedChannel (bitMap);
    greenBefore = getGreenChannel (bitMap);
    blueBefore = getBlueChannel (bitMap);
    
    channelPtr alphaBefore = getAlphaChannel (bitMap);
    saveBitMap (bitMap, "ARGB32", ".");
    destroyBmp (bitMap);
    
    image = parseBitMap ("./ARGB32");
    versionAfter = getDIBHeaderVersion (image);
    sizeAfter = getDIBHeaderSize (image);
    xResAfter = getXRes (image);
    yResAfter = getYRes (image);
    colorPlaneCountAfter = getColorPlaneCount (image);
    colorDepthAfter = getColorDepth (image);
    compressionAfter =  getCompression (image);
    imageSizeAfter = getImageSize (image);
    printResXAfter = getPrintResX (image);
    printResY = getPrintResY (image);
    paletteColorCountAfter = getPaletteColorCount (image);
    impColorCountAfter = getImpColorCount (image);
    colorSpace cspaceAfter = getColorSpace (image);
    
    redAfter = getRedChannel (image);
    greenAfter = getGreenChannel (image);
    blueAfter = getBlueChannel (image);
    channelPtr alphaAfter = getAlphaChannel (image);

    assert (sizeAfter == sizeBefore);
    assert (versionAfter == versionBefore);
    assert (xResAfter == xResBefore);
    assert (yResBefore == yResAfter);
    assert (colorPlaneCountAfter == colorPlaneCountBefore);
    assert (colorDepthAfter == colorDepthBefore);
    assert (compressionAfter == compressionBefore);
    assert (imageSizeAfter == imageSizeBefore);
    assert (printResXAfter == printResXBefore);
    assert (paletteColorCountAfter == paletteColorCountBefore);
    assert (impColorCountAfter == impColorCountBefore);
    assert (cSpaceBefore == cspaceAfter);

    compareChannels (redAfter, redBefore);
    compareChannels (greenAfter, greenBefore);
    compareChannels (blueAfter, blueBefore);
    compareChannels (alphaAfter, alphaBefore);

    destroyChannel (redBefore);
    destroyChannel (redAfter);
    destroyChannel (greenBefore);
    destroyChannel (greenAfter);
    destroyChannel (blueBefore);
    destroyChannel (blueAfter);
    destroyChannel (alphaBefore);
    destroyChannel (alphaAfter);
    destroyBmp (image);
    retCode = remove ("./ARGB32");
    assert (retCode == 0);

    return;
}

static void compareChannels (channelPtr before, channelPtr after) {
    printf ("\t\t\t>comparing channels\n");
    
    LONG xResBefore = getChXRes (before);
    LONG xResAfter = getChXRes (after);
    assert (xResAfter == xResBefore);

    LONG yResBefore = getChYRes (before);
    LONG yResAfter = getChYRes (after);
    assert (yResAfter == yResBefore);
    int row = 0;
    while (row < yResAfter) {
        int column = 0;
        while (column < xResAfter) {
            byte pixValBefore = getPixel (row, column, before);
            byte pixValAfter = getPixel (row, column, after);
            assert (pixValAfter == pixValBefore);
            column ++;
        }
        row ++;
    }
    printf ("\t\t\t>channel comparision successfull\n");
    return;
}


static void testChannelAccessFunctions () {

    printf ("\ttesting channel access functions:\n");

    // for RGB24/R/G/B channels
    bmpPtr bitMap = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (bitMap, RGB_24);

    channelPtr red = createChannel (DEFAULT_IH_XRES_RGB_24, DEFAULT_IH_YRES_RGB_24);
    LONG res = getChXRes (red);
    assert (res == DEFAULT_IH_XRES_RGB_24);
    res = getChYRes (red);
    assert (res == DEFAULT_IH_YRES_RGB_24);

    setPixel (0,0,red,240);
    byte pixVal = getPixel (0,0,red);
    assert (pixVal == 240);
    LONG i = 0;
    while (i < DEFAULT_IH_YRES_RGB_24) {
        LONG j = 0;
        while (j < DEFAULT_IH_XRES_RGB_24) {
            setPixel (i,j,red, i);
            j ++;
        }
        i ++;
    }

    setChannel (RED, bitMap, red);
    channelPtr extractedChannel = getRedChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);

    i = 0;
    while (i < DEFAULT_IH_YRES_RGB_24) {
        LONG j = 0;
        while (j < DEFAULT_IH_XRES_RGB_24) {
            setPixel (i,j,red, j);
            j ++;
        }
        i ++;
    }
    setChannel (GREEN, bitMap, red);
    extractedChannel = getGreenChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);
    setChannel (BLUE, bitMap, red);
    extractedChannel = getBlueChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);
    
    destroyChannel (red);
    destroyBmp (bitMap);

    // for ARGB32/R/G/B/A channels
    bitMap = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (bitMap, ARGB_32);
    red = createChannel (DEFAULT_V4IH_XRES_ARGB_32, DEFAULT_V4IH_YRES_ARGB_32);
    res = getChXRes (red);
    assert (res == DEFAULT_V4IH_XRES_ARGB_32);
    res = getChYRes (red);
    assert (res == DEFAULT_V4IH_YRES_ARGB_32);

    setPixel (0,0,red,240);
    pixVal = getPixel (0,0,red);
    assert (pixVal == 240);
    i = 0;
    while (i < DEFAULT_V4IH_YRES_ARGB_32) {
        LONG j = 0;
        while (j < DEFAULT_V4IH_XRES_ARGB_32) {
            setPixel (i,j,red, i);
            j ++;
        }
        i ++;
    }

    setChannel (RED, bitMap, red);
    extractedChannel = getRedChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);
    i = 0;
    while (i < DEFAULT_V4IH_YRES_ARGB_32) {
        LONG j = 0;
        while (j < DEFAULT_V4IH_XRES_ARGB_32) {
            setPixel (i,j,red, j);
            j ++;
        }
        i ++;
    }
    setChannel (GREEN, bitMap, red);
    extractedChannel = getGreenChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);
    setChannel (BLUE, bitMap, red);
    extractedChannel = getBlueChannel (bitMap);
    compareChannels (extractedChannel, red);
    destroyChannel (extractedChannel);
    
    setChannel (ALPHA, bitMap, red);

    extractedChannel = getAlphaChannel (bitMap);
    compareChannels (extractedChannel, red);

    destroyChannel (extractedChannel);
    destroyChannel (red);
    destroyBmp (bitMap);

    return;
}

static void testCompareChannels () {
    printf ("\t\t>testing compare channels\n");
    bmpPtr image = createBmp (BITMAPINFOHEADER);
    bmpPtr reflection = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (image, RGB_24);
    initializeBmpDFLT (reflection, RGB_24);

    channelPtr red = getRedChannel (image);
    channelPtr reflectedRed = getRedChannel (reflection);

    compareChannels (red, reflectedRed);

    channelPtr green = getGreenChannel (image);
    channelPtr reflectedGreen = getGreenChannel (reflection);

    compareChannels (green, reflectedGreen);

    channelPtr blue = getBlueChannel (image);
    channelPtr reflectedBlue = getBlueChannel (reflection);

    compareChannels (blue, reflectedBlue);
    
    destroyChannel (blue);
    destroyChannel (reflectedBlue);

    destroyChannel (green);
    destroyChannel (reflectedGreen);

    destroyChannel (red);
    destroyChannel (reflectedRed);

    destroyBmp (reflection);
    destroyBmp (image);

    bmpPtr sample = createBmp (BITMAPV4HEADER);
    bmpPtr reflectSample = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sample, ARGB_32);
    initializeBmpDFLT (reflectSample, ARGB_32);

    red = getRedChannel (sample);
    reflectedRed = getRedChannel (reflectSample);
    compareChannels (red, reflectedRed);

    green = getGreenChannel (sample);
    reflectedGreen = getGreenChannel (reflectSample);
    compareChannels (green, reflectedGreen);

    blue = getBlueChannel (sample);
    reflectedBlue = getBlueChannel (reflectSample);
    compareChannels (blue, reflectedBlue);
    
    channelPtr alpha = getAlphaChannel (sample);
    channelPtr reflectedAlpha = getAlphaChannel (reflectSample);
    compareChannels (alpha, reflectedAlpha);

    destroyChannel (blue);
    destroyChannel (reflectedBlue);

    destroyChannel (green);
    destroyChannel (reflectedGreen);

    destroyChannel (red);
    destroyChannel (reflectedRed);

    destroyChannel (alpha);
    destroyChannel (reflectedAlpha);
    
    destroyBmp (sample);
    destroyBmp (reflectSample);


    return;
}