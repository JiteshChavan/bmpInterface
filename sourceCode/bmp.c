#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "bmp.h"

#define DIB_DEFAULTS_NOT_SPECIFIED 0
#define PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED 0
#define COLOR_SPACE_DEFAULTS_NOT_SPECIFIED 0
#define COMPRESSION_OFFSET_ARTIFACTS_NOT_SPECIFIED 0

#define UNINTIALIZED -1
#define PIXEL_ARRAY_UNINITIALIZED -1
#define CHANNEL_UNINITIALIZED -1
#define ALL_COLORS_IMPORTANT 0

typedef struct pixel {
    byte red;
    byte green;
    byte blue;
    byte alpha;
}pixel;
typedef pixel *pixelArray;

typedef byte *channelArray;

typedef struct bmp {
    DIBHeaderVersion DIBVersion;
    DWORD DIBHeaderSize;
    pixelFormat pixelFormat;
    LONG xRes;
    LONG yRes;
    WORD colorPlaneCount;
    WORD colorDepth;
    DWORD compression;
    DWORD imageSizeBytes;
    LONG printResX;
    LONG printResY;
    DWORD paletteColorCOunt;
    DWORD impColorCOunt;
    pixelArray pixelArray;
    colorSpace colorSpace;
} bmp;


typedef LONG row;
typedef LONG column;

typedef struct channel {
    LONG xRes;
    LONG yRes;
    unsigned long long resolution;
    channelArray channelArray;
} channel;


static DWORD ceiling (DWORD a, DWORD b);
static void verifyDIBVersion (DIBHeaderVersion version);
static void verifyColorSpace (colorSpace colorSpace);
static void verifyPixelFormat (pixelFormat pixelFormat);
static void verifyColorDepth (WORD colorDepth);
static void verifyCompression (DWORD compression, DIBHeaderVersion version);

static DWORD determineDIBSize (DIBHeaderVersion version);
static DIBHeaderVersion determineDIBVersion (DWORD dibSize);

// converts an unsigned int to a little endian string of bytes, returns length of the bytes string
static void toLittleEndianBytes (DWORD number, byte *bytes, LONG byteCount);
static void colorSpaceToLittleEndianBytes (colorSpace colorSpace, byte *bytes);
static DWORD toDWORD (byte *bytes, LONG byteCount);

static void setDFLTPixelArray (bmpPtr bitmap);
static void testHelperFunctions ();
static void testDetermineDIBSize ();
static void testCeiling ();
static void testSetDFLTPixelArray ();
static void testToDWORD ();
static void testToLittleEndianBytes ();
static void testColorSpaceToLittleEndianBytes ();

// returns current fileOffset
static LONG writeBmpFileHeader (FILE *targetImage, bmpPtr sample, LONG fileOffset);
static DWORD evaluatePixelArrayFileOffset (DIBHeaderVersion dibVersion);
static LONG writeDIBHeader (FILE *targetImage, bmpPtr sample, LONG fileOffset);
static LONG writePixelArray (FILE *targetImage, bmpPtr sample, LONG fileOffset);
static void writeBytes (FILE *targetImage, byte *bytes, LONG byteCOunt);
static void readBytes (FILE *targetImage, byte *bytes, LONG byteCount);

// proceeds reading/writing additional fields post 'impColorCount'
static LONG readAdditionalFields (FILE *targetImage, bmpPtr sample, LONG fileOffset);
static LONG writeAdditionalFields (bmpPtr sample, FILE * image, LONG fileOffset);

static pixelFormat determinePixelFormat (WORD colorDepth);

static void testWriteHelperFunctions ();
static void testEvaluatePixelArrayFileOffset ();
static void testWriteBmpFileHeader ();
static void testWriteDIBHeader ();
static void testWritePixelArray ();

void saveBitMap (bmpPtr sample, fileName imageName, relativePath destination) {
    // remove tests before shipping
    /*testWriteHelperFunctions ();*/
    assert (sample != NULL);
    int imageNameLength = strlen (imageName);
    int destinationLength = strlen (destination);
    assert ( imageNameLength + destinationLength < MAX_RELATIVE_PATH_LENGTH);
    // create targetFile
    relativePath ePath;
    strcpy (ePath, destination);
    char *imageOrigins = strcat (ePath, "/");
    imageOrigins = strcat (ePath, imageName);
    FILE *targetImage = fopen (imageOrigins, "wb");
    assert (targetImage != NULL);
    LONG fileOffset = 0;
    fileOffset = writeBmpFileHeader (targetImage, sample, fileOffset);
    assert (fileOffset == 14);
    
    fileOffset = writeDIBHeader (targetImage, sample, fileOffset);
    
    // since the offset to pixelArray depends on compression
    // eg: in case of BI_RGB offset to pixelArray is less by 16 compared to BI_BITFIELDS due to absence of 4 BITMASK DWORDS
    verifyCompression (sample->compression, sample->DIBVersion);
    if (sample->DIBVersion == BITMAPINFOHEADER) {
        if (sample->compression == BI_RGB) {
            assert (fileOffset == 54);
        } else {
            assert (COMPRESSION_OFFSET_ARTIFACTS_NOT_SPECIFIED);
        }
    } else if (sample->DIBVersion == BITMAPV4HEADER) {
        if (sample->compression == BI_BITFIELDS) {
            assert (fileOffset == evaluatePixelArrayFileOffset (BITMAPV4HEADER));
        } else {
            assert (COMPRESSION_OFFSET_ARTIFACTS_NOT_SPECIFIED);
        }
    }
    
    fileOffset = writePixelArray (targetImage, sample, fileOffset);
    DWORD cOffset = determineFileSizeInBytes (sample);
    assert (fileOffset == cOffset);
    fclose (targetImage);
    return;
}

bmpPtr parseBitMap (relativePath srcFilePath) {
    FILE *source = fopen (srcFilePath, "rb");
    assert (source != NULL);

    LONG fileOffset = 0;
    // assert file type
    byte value[2];
    readBytes (source, value, 2);
    assert (value[0] == 'B' && value[1] == 'M');
    fileOffset += 2;

    // fileByteSize
    byte bytes[4];
    readBytes (source, bytes, 4);
    DWORD fileByteSize = toDWORD (bytes, 4);
    fileOffset += 4;

    // 4 reserved bytes
    readBytes (source, bytes, 4);
    fileOffset += 4;

    // pixelArrayFileOffset
    readBytes (source, bytes, 4);
    DWORD pixelArrayFileOffsetRead = toDWORD (bytes, 4);
    fileOffset += 4;
    
    assert (fileOffset == 14);

    // DIB Header size
    readBytes (source, bytes, 4);
    DWORD dibSize = toDWORD (bytes, 4);
    fileOffset += 4;

    DIBHeaderVersion dibVersion = determineDIBVersion (dibSize);

    // destination ADT
    bmpPtr sample = createBmp (dibVersion);

    assert (fileOffset == 18);
    
    // xRes
    readBytes (source, bytes, 4);
    LONG xRes = (LONG) toDWORD (bytes, 4);
    sample->xRes = xRes;
    fileOffset += 4;

    // yRes
    readBytes (source, bytes, 4);
    LONG yRes = (LONG) toDWORD (bytes, 4);
    sample->yRes = yRes;
    fileOffset += 4;

    // pixel array setup
    unsigned long long netRes = sample->xRes * sample->yRes;
    sample->pixelArray = (pixelArray) malloc (netRes * sizeof(pixel));

    assert (fileOffset == 26);

    // colorPlaneCount
    readBytes (source, bytes, 2);
    WORD colorPlaneCount = (WORD) toDWORD (bytes, 2);
    sample->colorPlaneCount = colorPlaneCount;
    fileOffset += 2;

    // colorDepth
    readBytes (source, bytes, 2);
    WORD colorDepth = (WORD) toDWORD (bytes, 2);    
    assert (colorDepth == 24 || colorDepth == 32);
    sample->colorDepth = colorDepth;
    sample->pixelFormat = determinePixelFormat (colorDepth);    
    fileOffset += 2;

    // compression
    readBytes (source, bytes, 4);
    DWORD compression = toDWORD (bytes, 4);
    verifyCompression (compression, sample->DIBVersion);
    sample->compression = compression;
    fileOffset += 4;

    // imageSizeBytes (RAW)
    readBytes (source, bytes, 4);
    DWORD imageSIzeBytes = toDWORD (bytes, 4);
    sample->imageSizeBytes = imageSIzeBytes;
    fileOffset += 4;

    assert (fileOffset == 38);

    //printResX
    readBytes (source, bytes, 4);
    LONG printReX = (LONG) toDWORD (bytes, 4);
    sample->printResX = printReX;
    fileOffset += 4;

    //printResY
    readBytes (source, bytes, 4);
    LONG printReY = (LONG) toDWORD (bytes, 4);
    sample->printResY = printReY;
    fileOffset += 4;

    assert (fileOffset == 46);

    // paletteColorCount
    readBytes (source, bytes, 4);
    DWORD paletteCC = toDWORD (bytes, 4);
    sample->paletteColorCOunt = paletteCC;
    fileOffset += 4;

    // impCC
    readBytes (source, bytes, 4);
    DWORD impCC = toDWORD (bytes, 4);
    sample->impColorCOunt = impCC;
    fileOffset += 4;

    // modify here to support other DIB Headers
    fileOffset = readAdditionalFields (source, sample, fileOffset);

    DWORD pixelArrayFileOffsetCalculated = evaluatePixelArrayFileOffset (sample->DIBVersion);
    assert (pixelArrayFileOffsetRead == pixelArrayFileOffsetCalculated);
    assert (fileOffset == pixelArrayFileOffsetCalculated);

    assert (sample->colorDepth == 24 || sample->colorDepth == 32);

    DWORD bytesPerRow = sample->xRes * (sample->colorDepth/8);
    DWORD paddingByteCount = (ceiling (bytesPerRow, 4) * 4) - bytesPerRow;
    byte *trash = (byte *) malloc (paddingByteCount * sizeof (byte));

    if (sample->DIBVersion == BITMAPINFOHEADER) {
        row cRow = sample->yRes - 1;
        while (cRow >= 0) {
            column cCol = 0;
            while (cCol < sample->xRes) {
                unsigned long long pixIndex = (cRow * sample->xRes) + cCol;
                byte blue = fgetc (source);
                byte green = fgetc (source);
                byte red = fgetc (source);
                (sample->pixelArray + pixIndex)->blue = blue;
                (sample->pixelArray + pixIndex)->green = green;
                (sample->pixelArray + pixIndex)->red = red;
                
                fileOffset += 3;
                cCol ++;
            }
            //skipPaddingBytes
            readBytes (source, trash, paddingByteCount);
            fileOffset += paddingByteCount;
            cRow --;
        }
    } else if (sample->DIBVersion ==  BITMAPV4HEADER) {
        row cRow = 0;
        while (cRow < sample->yRes) {
            column cCol = 0;
            while (cCol < sample->xRes) {
                unsigned long long pixIndex = (cRow * sample->xRes) + cCol;
                byte blue = fgetc (source);
                byte green = fgetc (source);
                byte red = fgetc (source);
                byte alpha = fgetc (source);
                (sample->pixelArray + pixIndex)->blue = blue;
                (sample->pixelArray + pixIndex)->green = green;
                (sample->pixelArray + pixIndex)->red = red;
                (sample->pixelArray + pixIndex)->alpha = alpha;
                fileOffset += 4;
                cCol ++;
            }
            //skipPaddingBytes
            readBytes (source, trash, paddingByteCount);
            fileOffset += paddingByteCount;
            cRow ++;
        }
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    assert (fileOffset == fileByteSize);
    free (trash);
    fclose (source);
    return sample;
}

static LONG readAdditionalFields (FILE *targetImage, bmpPtr sample, LONG fileOffset) {
    assert (targetImage != NULL);
    assert (sample != NULL);
    assert (fileOffset  == 54);

    verifyCompression (sample->compression, sample->DIBVersion);
    if (sample->DIBVersion == BITMAPINFOHEADER) {
        // no need to read additional fields into DIB since only BI_RGB compression is supported fo this DIB
    } else if (sample->DIBVersion == BITMAPV4HEADER) {
        // read 4 DWORDS for ARGB32/channel masks
        // read 4 bytes for LCS_WINDOWS_COLOR_SPACE
        // 24h = 36 bytes of CIEXYZTRIPLE Color Space end points which is unused for SUPPORTED LCS Color space
        // 4,4,4 = 12 bytes of red,green,blue gamma again its unused for SUPPORTED LCS Color space
        
        byte redChannelMask[4];
        readBytes (targetImage, redChannelMask, 4);
        DWORD redBitMask = toDWORD (redChannelMask, 4);
        assert (redBitMask == 0x00FF0000);
        fileOffset += 4;

        byte greenChannelMask[4];
        readBytes (targetImage, greenChannelMask, 4);
        DWORD greenBitMask = toDWORD (greenChannelMask, 4);
        assert (greenBitMask == 0x0000FF00);
        fileOffset += 4;

        byte blueChannelMask[4];
        readBytes (targetImage, blueChannelMask, 4);
        DWORD blueBitMask = toDWORD (blueChannelMask, 4);
        assert (blueBitMask == 0x000000FF);
        fileOffset += 4;

        byte alphaChannelMask[4];
        readBytes (targetImage, alphaChannelMask, 4);
        DWORD alphaBitMask = toDWORD (alphaChannelMask, 4);
        assert (alphaBitMask == 0xFF000000);
        fileOffset += 4;

        byte bytes[4];
        readBytes (targetImage, bytes, 4);
        // verify color space
        assert (bytes[0] == ' ' && bytes[1] == 'n' && bytes[2] == 'i' && bytes[3] == 'W');
        sample->colorSpace = LCS_WINDOWS_COLOR_SPACE;
        fileOffset += 4;

        int i = 0;
        byte cypherTrash;
        while (i < 0x24) {
            cypherTrash = fgetc (targetImage);
            i ++;
        }
        fileOffset += 0x24;

        // useless 3 DWORDS for RGB gamma, these fields are ununsed for LCS color space 
        byte temp[12];
        readBytes (targetImage, temp, 12);
        fileOffset += 12;
        assert (fileOffset == 122);
        assert (122 == evaluatePixelArrayFileOffset (sample->DIBVersion));

    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    return fileOffset;
}

static LONG writeBmpFileHeader (FILE *targetImage, bmpPtr sample, LONG fileOffset) {
    assert (fileOffset == 0);
    assert (targetImage != NULL);
    assert (sample != NULL);

    putc ('B', targetImage);
    putc ('M', targetImage);
    fileOffset += 2;

    DWORD fileSizeInBytes = determineFileSizeInBytes (sample);
    byte bytes[4];
    toLittleEndianBytes (fileSizeInBytes, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // 4 reserved bytes
    putc (0, targetImage);
    putc (0, targetImage);
    putc (0, targetImage);
    putc (0, targetImage);

    fileOffset += 4;

    DWORD pixelArrayFileOffset = evaluatePixelArrayFileOffset (sample->DIBVersion);
    toLittleEndianBytes (pixelArrayFileOffset, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;
    assert (fileOffset == 14);
    return fileOffset;
}

static LONG writeDIBHeader (FILE *targetImage, bmpPtr sample, LONG fileOffset) {
    assert (targetImage != NULL);
    assert (sample != NULL);
    verifyDIBVersion (sample->DIBVersion);
    assert (fileOffset == 14);

    // DIBHeaderSize
    byte bytes[4];
    toLittleEndianBytes (sample->DIBHeaderSize, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;
    
    // xRes
    toLittleEndianBytes (sample->xRes, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // yRes
    toLittleEndianBytes (sample->yRes, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // colorPlaneCount
    toLittleEndianBytes (sample->colorPlaneCount, bytes, 2);
    writeBytes (targetImage, bytes, 2);
    fileOffset += 2;

    // colorDepth
    toLittleEndianBytes (sample->colorDepth, bytes, 2);
    writeBytes (targetImage, bytes, 2);
    fileOffset += 2;

    // compression
    toLittleEndianBytes (sample->compression, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // raw imageSize
    toLittleEndianBytes (sample->imageSizeBytes, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // printResX
    toLittleEndianBytes (sample->printResX, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // printResY
    toLittleEndianBytes (sample->printResY, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // paletteColoCount
    toLittleEndianBytes (sample->paletteColorCOunt, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    // impColorCount
    toLittleEndianBytes (sample->impColorCOunt, bytes, 4);
    writeBytes (targetImage, bytes, 4);
    fileOffset += 4;

    fileOffset = writeAdditionalFields (sample, targetImage, fileOffset);

    assert (fileOffset == evaluatePixelArrayFileOffset (sample->DIBVersion));
    return fileOffset;    
}

static LONG writeAdditionalFields (bmpPtr sample, FILE * targetImage, LONG fileOffset) {
    assert (sample != NULL);
    assert (targetImage != NULL);
    assert (fileOffset == 54);
    
    verifyCompression (sample->compression, sample->DIBVersion);
    if (sample->DIBVersion == BITMAPINFOHEADER) {
        // no need to write anything for only BI_RGB is supported for BITMAPINFOHEADER
    } else if (sample->DIBVersion == BITMAPV4HEADER) {
        // 4 DWORDS for ARGB32/CHANNEL_MASKS
        // write 4 bytes for LCS_WINDOWS_COLOR_SPACE
        // 24h = 36 bytes of CIEXYZTRIPLE Color Space end points which is unused for SUPPORTED LCS Color space
        // 4,4,4 = 12 bytes of red,green,blue gamma again its unused for SUPPORTED LCS Color space

        byte channelMask[4];
        toLittleEndianBytes (RED_CHANNEL_MASK, channelMask, 4);
        writeBytes (targetImage, channelMask, 4);
        fileOffset += 4;
        
        toLittleEndianBytes (GREEN_CHANNEL_MASK, channelMask, 4);
        writeBytes (targetImage, channelMask, 4);
        fileOffset += 4;
        
        toLittleEndianBytes (BLUE_CHANNEL_MASK, channelMask, 4);
        writeBytes (targetImage, channelMask, 4);
        fileOffset += 4;

        toLittleEndianBytes (ALPHA_CHANNEL_MASK, channelMask, 4);
        writeBytes (targetImage, channelMask, 4);
        fileOffset += 4;


        byte bytes[4];
        colorSpaceToLittleEndianBytes (LCS_WINDOWS_COLOR_SPACE, bytes);
        writeBytes (targetImage, bytes, 4);
        fileOffset += 4;

        int i = 0;
        while (i < 0x24) {
            putc (0, targetImage);
            i ++;
        }
        fileOffset += 0x24;

        // 3 DWORDS FOR RGB channel Gamma ununsed in LCS color space
        byte trash[12] = {0};
        writeBytes (targetImage, trash, 12);
        fileOffset += 12;
        assert (fileOffset ==  evaluatePixelArrayFileOffset (sample->DIBVersion));
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    return fileOffset;
}

static LONG writePixelArray (FILE *targetImage, bmpPtr sample, LONG fileOffset) {
    assert (fileOffset == evaluatePixelArrayFileOffset (sample->DIBVersion));
    assert (targetImage != NULL);
    assert (sample != NULL);
    assert (sample->xRes > 0 && sample->yRes > 0);
    verifyDIBVersion (sample->DIBVersion);
    DWORD cOffset = evaluatePixelArrayFileOffset (sample->DIBVersion);
    assert (fileOffset == cOffset);

    assert (sample->colorDepth == 24 || sample->colorDepth ==32);
    DWORD bytesPerRow = sample->xRes * (sample->colorDepth/8);
    DWORD paddingByteCount = (ceiling (bytesPerRow, 4) * 4) - bytesPerRow;

    byte *trash = (byte *) malloc (paddingByteCount * sizeof (byte));
    DWORD i = 0;
    while (i < paddingByteCount) {
        trash[i] = 0;
        i ++;
    }

    if (sample->DIBVersion == BITMAPINFOHEADER) {
        row cRow = sample->yRes -1;
        while (cRow >= 0) {
            column cCol = 0;
            while (cCol < sample->xRes) {
                unsigned long long pixelIndex = cRow * sample->xRes + cCol;
                byte blue = (sample->pixelArray + pixelIndex)->blue;
                putc (blue, targetImage);
                byte green = (sample->pixelArray + pixelIndex)->green;
                putc (green, targetImage);
                byte red = (sample->pixelArray + pixelIndex)->red;
                putc (red, targetImage);
                fileOffset += 3;
                cCol ++;
            }
            writeBytes (targetImage, trash, paddingByteCount);
            fileOffset += paddingByteCount;
            cRow --;
        }
    } else if (sample->DIBVersion == BITMAPV4HEADER) {
        row cRow = 0;
        while (cRow < sample->yRes) {
            column cCol = 0;
            while (cCol < sample->xRes) {
                unsigned long long pixelIndex = cRow * sample->xRes + cCol;
                byte blue = (sample->pixelArray + pixelIndex)->blue;
                putc (blue, targetImage);
                byte green = (sample->pixelArray + pixelIndex)->green;
                putc (green, targetImage);
                byte red = (sample->pixelArray + pixelIndex)->red;
                putc (red, targetImage);
                byte alpha = (sample->pixelArray + pixelIndex)->alpha;
                putc (alpha, targetImage);
                fileOffset += 4;
                cCol ++;
            }
            writeBytes (targetImage, trash, paddingByteCount);
            fileOffset += paddingByteCount;
            cRow ++;
        }
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    DWORD fileSIzeInBytes = determineFileSizeInBytes (sample);
    assert (fileOffset == fileSIzeInBytes);
    free (trash);
    return fileOffset;
}

static void writeBytes (FILE *targetImage, byte *bytes, LONG byteCount) {
    assert (targetImage != NULL);
    assert (bytes != NULL);
    assert (byteCount >= 0);
    int i = 0;
    while (i < byteCount) {
        putc (bytes[i], targetImage);
        i ++;
    }
    return;
}

static void readBytes (FILE *targetImage, byte *bytes, LONG byteCount) {
    assert (targetImage != NULL);
    assert (bytes != NULL);
    assert (byteCount >= 0);
    int i = 0;
    while (i < byteCount) {
        bytes[i] = fgetc (targetImage);
        i ++;
    }
    return;
}

static DWORD evaluatePixelArrayFileOffset (DIBHeaderVersion version) {
    verifyDIBVersion (version);
    DWORD pixelArrayOffset =  determineDIBSize (version) + 14;
    return pixelArrayOffset;
}

static void testWriteHelperFunctions () {
    printf ("\t\t\t>Testing writeHelperFunctions\n");
    testEvaluatePixelArrayFileOffset ();
    testWriteBmpFileHeader ();
    testWriteDIBHeader ();
    testWritePixelArray ();
    printf ("\t\t\t>Woohoo!! all write helper functions passed! You are awesome\n");
    return;
}

static void testEvaluatePixelArrayFileOffset () {
    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    setCompression (sample, BI_RGB);
    DWORD answer = evaluatePixelArrayFileOffset (sample->DIBVersion);
    assert (answer == 54);

    setDIBHeaderVersion (sample, BITMAPV4HEADER);
    answer = evaluatePixelArrayFileOffset (sample->DIBVersion);
    assert (answer == 122);
    destroyBmp (sample);
    return;
}

static void testWriteBmpFileHeader () {
    printf ("\t\t\t\t>Testing writeBmpFileHeader()\n");
    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sample, RGB_24);
    LONG fileOffset = 0;
    FILE *temp = fopen ("./temp0.bmp","wb");
    fileOffset = writeBmpFileHeader (temp, sample, fileOffset);
    assert (fileOffset == 14);
    fclose (temp);
    temp = fopen ("./temp0.bmp", "rb");
    
    int cypherChar = fgetc (temp);
    assert (cypherChar == 'B');
    cypherChar = fgetc (temp);
    assert (cypherChar == 'M');
    
    // fileSize in bytes
    byte bytes[4];
    readBytes (temp, bytes, 4);
    DWORD fileByteSizeRead = toDWORD (bytes, 4);
    DWORD fileByteSizeCalculated = determineFileSizeInBytes (sample);
    assert (fileByteSizeRead == fileByteSizeCalculated);

    // skip 4 reserved bytes
    readBytes (temp, bytes, 4);

    // pixel array offset
    readBytes (temp, bytes, 4);
    DWORD pixelArrayOffsetRead = toDWORD (bytes, 4);
    DWORD pixelArrayOffsetCalculated = evaluatePixelArrayFileOffset (sample->DIBVersion);
    assert (pixelArrayOffsetRead == pixelArrayOffsetCalculated);
    fclose (temp);
    int retCode = remove ("./temp0.bmp");
    assert (retCode == 0);
    free (sample->pixelArray);
    free (sample);
    return;
}

static void testWriteDIBHeader () {
    printf ("\t\t\t\t>Testing writeDIBHeader () for BITMAPINFOHEADER\n");
    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sample, RGB_24);
    LONG fileOffset = 14;
    FILE *temp = fopen ("./temp1.bmp", "wb");
    fileOffset = writeDIBHeader (temp, sample, fileOffset);
    fclose (temp);
    assert (fileOffset == 54);
    temp = fopen ("./temp1.bmp", "rb");
    
    // DIB size (bytes)
    byte bytes[4];
    readBytes (temp, bytes, 4);
    DWORD DIBSizeRead = toDWORD (bytes, 4);
    assert (DIBSizeRead == sample->DIBHeaderSize);

    // xRes
    readBytes (temp, bytes, 4);
    LONG xRes = (LONG)toDWORD (bytes, 4);
    assert (xRes == sample->xRes);

    // yRes
    readBytes (temp, bytes, 4);
    LONG yRes = (LONG) toDWORD (bytes, 4);
    assert (yRes == sample->yRes);

    // colorPlaneCount
    readBytes (temp, bytes, 2);
    WORD colorPlaneCount = (WORD) toDWORD (bytes, 2);
    assert (colorPlaneCount == sample->colorPlaneCount);

    // colorDepth
    readBytes (temp, bytes, 2);
    WORD colorDepth = (WORD) toDWORD (bytes, 2);
    assert (colorDepth == sample->colorDepth);
    if (colorDepth == 24) {
        assert (sample->pixelFormat == RGB_24);
    } else if (colorDepth == 32) {
        assert (sample->pixelFormat == ARGB_32);
    }

    // compression
    readBytes (temp, bytes, 4);
    DWORD compression = toDWORD (bytes, 4);
    assert (compression == sample->compression);

    // rawBitmapData (bytes)
    readBytes (temp, bytes, 4);
    DWORD imageSizeBytesRead = toDWORD (bytes, 4);
    assert (imageSizeBytesRead == sample->imageSizeBytes);
    DWORD imageSizeBytesCalculated = evaluateRawImageSizeInBytes (sample);
    assert (imageSizeBytesRead == imageSizeBytesCalculated);

    // printResX
    readBytes (temp, bytes, 4);
    LONG printResX = (LONG) toDWORD (bytes, 4);
    assert (printResX == sample->printResX);

    // printResY
    readBytes (temp, bytes, 4);
    LONG printResY = (LONG) toDWORD (bytes, 4);
    assert (printResY == sample->printResY);

    // paletteColorCount
    readBytes (temp, bytes, 4);
    DWORD paletteColorCount = toDWORD (bytes, 4);
    assert (paletteColorCount == sample->paletteColorCOunt);

    // impColorCount
    readBytes (temp, bytes, 4);
    DWORD impColorCount = toDWORD (bytes, 4);
    assert (impColorCount == sample->impColorCOunt);

    fclose (temp);
    int retCode = remove ("./temp1.bmp");
    assert (retCode == 0);

    free (sample->pixelArray);
    free (sample);

    printf ("\t\t\t\t>Testing writeDIBHeader () for BITMAPV4HEADER\n");
    bmpPtr bitmap = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (bitmap, ARGB_32);
    fileOffset = 14;
    temp = fopen ("./temp1.bmp", "wb");
    fileOffset = writeDIBHeader (temp, bitmap, fileOffset);
    assert (fileOffset == 122);
    fclose (temp);
    
    temp = fopen ("./temp1.bmp", "rb");
    
    // DIB size (bytes)
    byte values[4];
    readBytes (temp, values, 4);
    DIBSizeRead = toDWORD (values, 4);
    assert (DIBSizeRead == bitmap->DIBHeaderSize);

    // xRes
    readBytes (temp, values, 4);
    xRes = (LONG)toDWORD (values, 4);
    assert (xRes == bitmap->xRes);

    // yRes
    readBytes (temp, values, 4);
    yRes = (LONG) toDWORD (values, 4);
    assert (yRes == bitmap->yRes);

    // colorPlaneCount
    readBytes (temp, values, 2);
    colorPlaneCount = (WORD) toDWORD (values, 2);
    assert (colorPlaneCount == bitmap->colorPlaneCount);

    // colorDepth
    readBytes (temp, values, 2);
    colorDepth = (WORD) toDWORD (values, 2);
    assert (colorDepth == bitmap->colorDepth);
    if (colorDepth == 24) {
        assert (bitmap->pixelFormat == RGB_24);
    } else if (colorDepth == 32) {
        assert (bitmap->pixelFormat == ARGB_32);
    }

    // compression
    readBytes (temp, values, 4);
    compression = toDWORD (values, 4);
    assert (compression == bitmap->compression);

    // rawBitmapData (bytes)
    readBytes (temp, values, 4);
    imageSizeBytesRead = toDWORD (values, 4);
    assert (imageSizeBytesRead == bitmap->imageSizeBytes);
    imageSizeBytesCalculated = evaluateRawImageSizeInBytes (bitmap);
    assert (imageSizeBytesRead == imageSizeBytesCalculated);

    // printResX
    readBytes (temp, values, 4);
    printResX = (LONG) toDWORD (values, 4);
    assert (printResX == bitmap->printResX);

    // printResY
    readBytes (temp, values, 4);
    printResY = (LONG) toDWORD (values, 4);
    assert (printResY == bitmap->printResY);

    // paletteColorCount
    readBytes (temp, values, 4);
    paletteColorCount = toDWORD (values, 4);
    assert (paletteColorCount == bitmap->paletteColorCOunt);

    // impColorCount
    readBytes (temp, values, 4);
    impColorCount = toDWORD (values, 4);
    assert (impColorCount == bitmap->impColorCOunt);

    byte channelMask[4];
    readBytes (temp, channelMask, 4);
    DWORD redChannelBitMask = toDWORD (channelMask, 4);
    assert (redChannelBitMask == RED_CHANNEL_MASK);

    readBytes (temp, channelMask, 4);
    DWORD greenChannelBitMask = toDWORD (channelMask, 4);
    assert (greenChannelBitMask == GREEN_CHANNEL_MASK);

    readBytes (temp, channelMask, 4);
    DWORD blueChannelBitMask = toDWORD (channelMask, 4);
    assert (blueChannelBitMask == BLUE_CHANNEL_MASK);

    readBytes (temp, channelMask, 4);
    DWORD alphaCHannelBitMask = toDWORD (channelMask, 4);
    assert (alphaCHannelBitMask == ALPHA_CHANNEL_MASK);

    // color space
    readBytes (temp, values , 4);
    assert (values[0] == ' ' && values[1] == 'n' && values[2] == 'i' && values[3] == 'W');
    
    fclose (temp);
    retCode = remove ("./temp1.bmp");
    assert (retCode == 0);

    free (bitmap->pixelArray);
    free (bitmap);

    return;
}

static void testWritePixelArray () {
    // hardcoded test for example RGB_24 image on wikipedia.
    printf ("\t\t\t\t>testing writePixelArray () for BITMAPINFOHEADER/RGB_24\n");

    bmpPtr sample = createBmp (BITMAPINFOHEADER);
    initializeBmpDFLT (sample, RGB_24);
    LONG fileOffset = 54;
    FILE *temp = fopen ("./temp2.bmp", "wb");
    fileOffset = writePixelArray (temp, sample, fileOffset);
    assert (fileOffset == determineFileSizeInBytes (sample));
    fclose (temp);

    temp = fopen ("./temp2.bmp", "rb");

    byte pixVal = fgetc (temp);
    assert (pixVal == 0);

    pixVal = fgetc (temp);
    assert (pixVal == 0);

    pixVal = fgetc (temp);
    assert (pixVal == 255);

    pixVal = fgetc (temp);
    assert (pixVal == 255);
    pixVal = fgetc (temp);
    assert (pixVal == 255);
    pixVal = fgetc (temp);
    assert (pixVal == 255);

    pixVal = fgetc (temp);
    assert (pixVal == 0);
    pixVal = fgetc (temp);
    assert (pixVal == 0);

    pixVal = fgetc (temp);
    assert (pixVal == 255);
    pixVal = fgetc (temp);
    assert (pixVal == 0);
    pixVal = fgetc (temp);
    assert (pixVal == 0);

    pixVal = fgetc (temp);
    assert (pixVal == 0);
    pixVal = fgetc (temp);
    assert (pixVal == 255);
    pixVal = fgetc (temp);
    assert (pixVal == 0);

    pixVal = fgetc (temp);
    assert (pixVal == 0);
    pixVal = fgetc (temp);
    assert (pixVal == 0);

    fclose (temp);
    int retCode = remove ("./temp2.bmp");
    assert (retCode == 0);
    free (sample->pixelArray);
    free (sample);

    printf ("\t\t\t\t>testing writePixelArray () for BITMAPV4HEADER/ARGB_32\n");
    sample = createBmp (BITMAPV4HEADER);
    initializeBmpDFLT (sample, ARGB_32);
    
    LONG fileIndex = 122;
    FILE *iFile = fopen ("./iFile.bmp", "wb");
    initializeBmpDFLT (sample, ARGB_32);
    fileIndex = writePixelArray (iFile, sample, fileIndex);
    assert (fileIndex == determineFileSizeInBytes (sample));
    fclose (iFile);

    iFile = fopen ("./iFile.bmp", "rb");
    byte value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 127);

    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 127);

    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 127);

    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 127);

    // row 2
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);

    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);

    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 0);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);

    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);
    value = fgetc (iFile);
    assert (value == 255);
    
    fclose (iFile);
    retCode = remove ("./iFile.bmp");
    assert (retCode == 0);
    free (sample->pixelArray);
    free (sample);
    return;
}

bmpPtr createBmp (DIBHeaderVersion version) {
    verifyDIBVersion (version);
    bmpPtr sample = (bmpPtr) malloc (sizeof (bmp));
    DWORD DIBSize = determineDIBSize (version);

    assert (DIBSize > 0);

    sample->DIBVersion = version;
    sample->DIBHeaderSize = DIBSize;

    if (version == BITMAPINFOHEADER) {
        sample->compression = BI_RGB;
    } else if (version == BITMAPV4HEADER) {
        sample->compression = BI_BITFIELDS;
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }

    sample->colorDepth = UNINTIALIZED;
    sample->colorPlaneCount = UNINTIALIZED;
    sample->imageSizeBytes = UNINTIALIZED;
    sample->impColorCOunt = ALL_COLORS_IMPORTANT;
    sample->paletteColorCOunt = UNINTIALIZED;
    sample->pixelArray = NULL;
    sample->pixelFormat = UNINTIALIZED;
    sample->printResX = UNINTIALIZED;
    sample->printResY = UNINTIALIZED;
    sample->xRes = UNINTIALIZED;
    sample->yRes = UNINTIALIZED;
    return sample;
}

void setUpPixelArray (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->xRes > 0 && sample->yRes >0);
    free (sample->pixelArray);
    unsigned long long netRes = sample->xRes * sample->yRes;
    sample->pixelArray = (pixelArray) malloc (netRes * sizeof (pixel));
    return;
}

channelPtr createChannel (LONG xRes, LONG yRes) {
    channelPtr targetChannel;
    targetChannel = (channelPtr) malloc (sizeof (channel));
    targetChannel->xRes = xRes;
    targetChannel->yRes = yRes;
    targetChannel->resolution = xRes * yRes;
    targetChannel->channelArray = (channelArray) malloc (targetChannel->resolution * sizeof (byte));
    return targetChannel;
}

void initializeBmpDFLT (bmpPtr bitmap, pixelFormat pixelFormat) {
    // remove tests before final compilation
    /*testHelperFunctions ();*/
    assert (bitmap != NULL);
    verifyDIBVersion (bitmap->DIBVersion);
    verifyPixelFormat (pixelFormat);

    if (bitmap->DIBVersion == BITMAPINFOHEADER) {
        bitmap->colorPlaneCount = DEFAULT_IH_COLOR_PLANE_COUNT;
        bitmap->compression = DEFAULT_IH_COMPRESSION;
        assert (bitmap->DIBHeaderSize == DEFAULT_IH_SIZE);
        bitmap->impColorCOunt = DEFAULT_IH_IMP_COLOR_COUNT;
        bitmap->paletteColorCOunt = DEFAULT_IH_PALETTE_CLR_COUNT;
        if (pixelFormat == RGB_24) {
            bitmap->pixelFormat = RGB_24;
            bitmap->imageSizeBytes = DEFAULT_IH_IMAGE_SIZE_RGB_24;
            bitmap->printResX = DEFAULT_IH_PRINT_RES_X;
            bitmap->printResY = DEFAULT_IH_PRINT_RES_Y;
            bitmap->colorDepth = DEFAULT_IH_COLOR_DEPTH_RGB_24;
            bitmap->xRes = DEFAULT_IH_XRES_RGB_24;
            bitmap->yRes = DEFAULT_IH_YRES_RGB_24;
            setDFLTPixelArray (bitmap);
        } else {
            // initializations for newly  supported pixel format UNDER THIS DIBHEADER would go here
            assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
            // update tests for each of the helper fucntions for newly supported pixel/DIB format
        }
    } else if (bitmap->DIBVersion == BITMAPV4HEADER) {
        bitmap->colorPlaneCount = DEFAULT_V4IH_COLOR_PLANE_COUNT;
        bitmap->compression = DEFAULT_V4IH_COMPRESSION;
        bitmap->DIBHeaderSize = DEFAULT_V4IH_SIZE;
        bitmap->impColorCOunt = DEFAULT_V4IH_IMP_COLOR_COUNT;
        bitmap->paletteColorCOunt = DEFAULT_V4IH_PALETTE_CLR_COUNT;
        if (pixelFormat == ARGB_32) {
            bitmap->pixelFormat = ARGB_32;
            bitmap->imageSizeBytes = DEFAULT_V4IH_IMAGE_SIZE_ARGB_32;
            bitmap->printResX = DEFAULT_V4IH_PRINT_RES_X;
            bitmap->printResY = DEFAULT_V4IH_PRINT_RES_Y;
            bitmap->colorDepth = DEFAULT_V4IH_COLOR_DEPTH_ARGB_32;
            bitmap->xRes = DEFAULT_V4IH_XRES_ARGB_32;
            bitmap->yRes = DEFAULT_V4IH_YRES_ARGB_32;
            bitmap->colorSpace = LCS_WINDOWS_COLOR_SPACE;
            setDFLTPixelArray (bitmap);
        } else {
            // initializations for newly  supported pixel format UNDER THIS DIBHEADER would go here
            assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
            // update tests for each of the helper fucntions for newly supported pixel/DIB format
        }
    } else {
        // initializations for newlySupported DIBversion would go here
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    return;    
}

void destroyBmp (bmpPtr sample) {
    free (sample->pixelArray);
    free (sample);
    return;
}

void destroyChannel (channelPtr targetChannel) {
    free (targetChannel->channelArray);
    free (targetChannel);
    return;
}

DIBHeaderVersion getDIBHeaderVersion (bmpPtr sample) {
    assert (sample != NULL);
    DIBHeaderVersion dibVersion = sample->DIBVersion;
    verifyDIBVersion (dibVersion);
    return dibVersion;
}

DWORD getDIBHeaderSize (bmpPtr sample) {
    assert (sample != NULL);
    verifyDIBVersion (sample->DIBVersion);
    DWORD dibSize = sample->DIBHeaderSize;
    DWORD a = determineDIBSize (sample->DIBVersion);
    assert (dibSize == a);
    return dibSize;
}

void setDIBHeaderVersion (bmpPtr sample, DIBHeaderVersion version) {
    assert (sample != NULL);
    verifyDIBVersion (version);
    DWORD dibSize = determineDIBSize (version);
    sample->DIBVersion = version;
    sample->DIBHeaderSize = dibSize;
    return;
}

void setPixelFormat (bmpPtr sample, pixelFormat pixelFormat) {
    assert (sample != NULL);
    verifyPixelFormat (pixelFormat);
    sample->pixelFormat = pixelFormat;
    if (pixelFormat == ARGB_32) {
        sample->colorDepth = BPP_32;
    } else if (pixelFormat == RGB_24) {
        sample->colorDepth = BPP_24;
    } else {
        assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
    }
    return;
}

pixelFormat getPixelFormat (bmpPtr sample) {
    assert (sample != NULL);
    verifyPixelFormat (sample->pixelFormat);
    pixelFormat format = sample->pixelFormat;
    return format;
}

LONG getXRes (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->xRes > 0);
    LONG xRes = sample->xRes;
    return xRes;
}

void setXRes (bmpPtr sample, LONG xRes) {
    assert (sample != NULL);
    assert (xRes > 0);
    sample->xRes = xRes;
    return;
}

LONG getYRes (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->yRes > 0);
    LONG yRes = sample->yRes;
    return yRes;
}

void setYRes (bmpPtr sample, LONG yRes) {
    assert (sample != NULL);
    assert (yRes > 0);
    sample->yRes = yRes;
    return;
}

WORD getColorPlaneCount (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->colorPlaneCount != UNINTIALIZED);
    assert (sample->colorPlaneCount == 1);
    WORD cPlaneCount = sample->colorPlaneCount;
    return cPlaneCount;
}

void setColorPlaneCount (bmpPtr sample, WORD colorPlanceCount) {
    assert (sample != NULL);
    assert (colorPlanceCount == 1);
    sample->colorPlaneCount = colorPlanceCount;
    return;
}

WORD getColorDepth (bmpPtr sample) {
    assert (sample != NULL);
    verifyColorDepth (sample->colorDepth);
    WORD clrDepth = sample->colorDepth;
    return clrDepth;
}
void setColorDepth (bmpPtr sample, WORD colorDepth) {
    assert (sample != NULL);
    verifyColorDepth (colorDepth);
    sample->colorDepth = colorDepth;
    if (colorDepth == BPP_24) {
        sample->pixelFormat = RGB_24;
    } else if (colorDepth == BPP_32) {
        sample->pixelFormat = ARGB_32;
    } else {
        // its a trap
        assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
    }
    return;
}

DWORD getCompression (bmpPtr sample) {
    assert (sample != NULL);
    verifyCompression (sample->compression, sample->DIBVersion);
    DWORD compression = sample->compression;
    return compression;
}
void setCompression (bmpPtr sample, DWORD compression) {
    assert (sample != NULL);
    verifyCompression (compression, sample->DIBVersion);
    sample->compression = compression;
    return;
}

DWORD getImageSize (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->imageSizeBytes > 0);

    DWORD a = evaluateRawImageSizeInBytes (sample);
    DWORD imageSize = sample->imageSizeBytes;
    return imageSize;
}
void setImageSize (bmpPtr sample, DWORD imageSize) {
    assert (sample != NULL);
    assert (imageSize > 0);
    sample->imageSizeBytes = imageSize;
    return;
}

LONG  getPrintResX (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->printResX > 0);
    LONG printResX = sample->printResX;
    return printResX;
}
void  setPrintResX (bmpPtr sample, LONG xPrintRes) {
    assert (sample != NULL);
    assert (xPrintRes > 0);
    sample->printResX = xPrintRes;
    return;
}

LONG  getPrintResY (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->printResY > 0);
    LONG printResY = sample->printResY;
    return printResY;
}
void  setPrintResY (bmpPtr sample, LONG yPrintRes) {
    assert (sample != NULL);
    assert (yPrintRes > 0);
    sample->printResY = yPrintRes;
    return;
}


DWORD getPaletteColorCount (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->paletteColorCOunt >= 0);
    DWORD paletteColorCount = sample->paletteColorCOunt;
    return paletteColorCount;
}
void setPaletteColorCount (bmpPtr sample, DWORD paletteColorCount) {
    assert (sample != NULL);
    assert (paletteColorCount >= 0);
    sample->paletteColorCOunt = paletteColorCount;
    return;
}

DWORD getImpColorCount (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->impColorCOunt >= 0);
    DWORD impClrCount = sample->impColorCOunt;
    return impClrCount;
}
void setImpColorCount (bmpPtr sample, DWORD impColorCount) {
    assert (sample != NULL);
    assert (impColorCount >= 0);
    sample->impColorCOunt = impColorCount;
    return;
}

colorSpace getColorSpace (bmpPtr sample) {
    assert (sample->DIBVersion == BITMAPV4HEADER);
    colorSpace space = sample->colorSpace;
    verifyColorSpace (space);
    return space;
}

void setColorSpace (bmpPtr sample, colorSpace colorSpace) {
    assert (sample->DIBVersion == BITMAPV4HEADER);
    verifyColorSpace (colorSpace);
    sample->colorSpace = colorSpace;
    return;
}

// Access functions ADT : 'channel'

// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the RED channel and returns a pointer to it.
channelPtr getRedChannel (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->xRes > 0 && sample-> yRes > 0);
    assert (sample->pixelArray != NULL);
    channelPtr red;
    red = (channelPtr) malloc (sizeof (channel));
    red->xRes = sample->xRes;
    red->yRes = sample->yRes;
    red->resolution = sample->xRes * sample->yRes;
    red->channelArray = (channelArray) malloc (red->resolution * sizeof (byte));
    int i = 0;
    while (i < red->resolution) {
        red->channelArray[i] = (sample->pixelArray+i)->red;
        i ++;
    }
    return red;
}
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the GREEN channel and returns a pointer to it.
channelPtr getGreenChannel (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->xRes > 0 && sample-> yRes > 0);
    assert (sample->pixelArray != NULL);
    channelPtr green;
    green = (channelPtr) malloc (sizeof (channel));
    green->xRes = sample->xRes;
    green->yRes = sample->yRes;
    green->resolution = sample->xRes * sample->yRes;
    green->channelArray = (channelArray) malloc (green->resolution * sizeof (byte));
    int i = 0;
    while (i < green->resolution) {
        green->channelArray[i] = (sample->pixelArray+i)->green;
        i ++;
    }
    return green;
}
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the BLUE channel and returns a pointer to it.
channelPtr getBlueChannel (bmpPtr sample) {
    assert (sample != NULL);
    assert (sample->xRes > 0 && sample-> yRes > 0);
    assert (sample->pixelArray != NULL);
    channelPtr blue;
    blue = (channelPtr) malloc (sizeof (channel));
    blue->xRes = sample->xRes;
    blue->yRes = sample->yRes;
    blue->resolution = sample->xRes * sample->yRes;
    blue->channelArray = (channelArray) malloc (blue->resolution * sizeof (byte));
    int i = 0;
    while (i < blue->resolution) {
        blue->channelArray[i] = (sample->pixelArray+i)->blue;
        i ++;
    }
    return blue;
}
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the ALPHA channel and returns a pointer to it.
// provided the there exists alpha channel in the pixel format 
channelPtr getAlphaChannel (bmpPtr sample) {

    assert (sample != NULL);
    assert (sample->xRes > 0 && sample-> yRes > 0);
    assert (sample->pixelArray != NULL);
    assert (sample->pixelFormat == ARGB_32);
    channelPtr alpha;
    alpha = (channelPtr) malloc (sizeof (channel));
    alpha->xRes = sample->xRes;
    alpha->yRes = sample->yRes;
    alpha->resolution = sample->xRes * sample->yRes;
    alpha->channelArray = (channelArray) malloc (alpha->resolution * sizeof (byte));
    int i = 0;
    while (i < alpha->resolution) {
        alpha->channelArray[i] = (sample->pixelArray+i)->alpha;
        i ++;
    }
    return alpha;
}

// to access dimensions of channel
LONG getChXRes (channelPtr channel) {
    assert (channel != NULL);
    assert (channel->xRes > 0);
    LONG chXRes = channel->xRes;
    return chXRes;
}
// to access dimensions of channel
LONG getChYRes (channelPtr channel) {
    assert (channel != NULL);
    assert (channel->yRes > 0);
    LONG yRes = channel->yRes;
    return yRes;
}

// to access specified pixel of specified channel
byte getPixel (LONG row, LONG column, channelPtr channel) {
    assert (channel != NULL);
    assert (row >= 0 && column >= 0);
    assert (row < channel->yRes && column < channel->xRes);
    unsigned long long i = channel->xRes * row + column;
    byte pixVal = channel->channelArray[i];
    return pixVal;
}
// to access specified pixel of specified channel
void setPixel (LONG row, LONG column, channelPtr channel, byte pixVal) {
    assert (channel != NULL);
    assert (row >= 0 && column >= 0);
    assert (row < channel->yRes && column < channel->xRes);
    assert (pixVal >= 0 && pixVal < 256);
    int i = channel->xRes * row + column;
    channel->channelArray[i] = pixVal;
    return;
}

// sets the channel of specified type in specified bitMap
void setChannel (channelType channelType, bmpPtr sample, channelPtr srcChannel) {
    assert (channelType == RED || channelType == GREEN || channelType == BLUE || channelType == ALPHA);
    if (channelType == ALPHA) {
        assert (sample->pixelFormat == ARGB_32 && sample->colorDepth == 32);
    }
    assert (srcChannel->xRes == sample->xRes && sample->yRes == srcChannel->yRes);
    if (channelType == RED) {
        int i = 0;
        while (i < srcChannel->resolution) {
            ((sample->pixelArray)+i)->red = srcChannel->channelArray[i];
            i ++;
        }
    } else if (channelType == BLUE) {
        int i = 0;
        while (i < srcChannel->resolution) {
            ((sample->pixelArray)+i)->blue = srcChannel->channelArray[i];
            i ++;
        }
    } else if (channelType == GREEN) {
        int i = 0;
        while (i < srcChannel->resolution) {
            ((sample->pixelArray)+i)->green = srcChannel->channelArray[i];
            i ++;
        }
    } else if (channelType == ALPHA) {
        int i = 0;
        while (i < srcChannel->resolution) {
            ((sample->pixelArray)+i)->alpha = srcChannel->channelArray[i];
            i ++;
        }
    }
    return;
}

static DWORD ceiling (DWORD a, DWORD b) {
    assert (b != 0);
    DWORD answer;
    if (a % b != 0) {
        answer = (DWORD) (a/b) + 1;
    } else {
        answer = a/b;
    }
    return answer;
}

static void toLittleEndianBytes (DWORD number, byte *bytes, LONG byteCount) {
    assert (bytes != NULL);
    assert (byteCount > 0);
    int i = 0;
    while (i < byteCount) {
        bytes[i] = (number >> (8*i)) & 255;
        i ++;
    }
    return;
}

static void colorSpaceToLittleEndianBytes (colorSpace colorSpace, byte *bytes) {
    verifyColorSpace (colorSpace);
    if (colorSpace == LCS_WINDOWS_COLOR_SPACE) {
        bytes [0] = ' ';
        bytes [1] = 'n';
        bytes [2] = 'i';
        bytes [3] = 'W';
    } else {
        assert (COLOR_SPACE_DEFAULTS_NOT_SPECIFIED);
    }
    return;
}

static DWORD toDWORD (byte *bytes, LONG byteCount) {
    assert (bytes != NULL);
    assert (byteCount > 0);

    DWORD number = 0;
    int i = 0;
    while (i < byteCount) {
        number += bytes[i] << (8 * i);
        i ++;
    }
    return number;
}

static void verifyDIBVersion (DIBHeaderVersion version) {
    assert (version == BITMAPINFOHEADER || version == BITMAPV4HEADER);
    return;
}

static void verifyColorSpace (colorSpace colorSpace) {
    assert (colorSpace == LCS_WINDOWS_COLOR_SPACE);
    return;
}

static void verifyPixelFormat (pixelFormat pixelFormat) {
    assert (pixelFormat == RGB_24 || pixelFormat == ARGB_32);
    return;
}

static void verifyColorDepth (WORD colorDepth) {
    assert (colorDepth == BPP_24 || colorDepth == BPP_32);
    return;
}

static void verifyCompression (DWORD compression, DIBHeaderVersion version) {
    verifyDIBVersion (version);
    assert (compression == BI_RGB || compression == BI_BITFIELDS);
    if (version == BITMAPINFOHEADER) {
        assert (compression == BI_RGB);
    } else if (version == BITMAPV4HEADER) {
        assert (compression == BI_BITFIELDS);
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);
    }
    return;
}

static DWORD determineDIBSize (DIBHeaderVersion version) {
    verifyDIBVersion (version);
    DWORD dibSize;
    if (version == BITMAPINFOHEADER) {
        dibSize = DEFAULT_IH_SIZE;
    } else if (version == BITMAPV4HEADER) {
        dibSize = DEFAULT_V4IH_SIZE;
    } else {
        assert (DIB_DEFAULTS_NOT_SPECIFIED);        
    }
    return dibSize;
}

static DIBHeaderVersion determineDIBVersion (DWORD dibSize) {
    assert (dibSize > 0);
    assert (dibSize == DEFAULT_IH_SIZE || dibSize == DEFAULT_V4IH_SIZE);
    DIBHeaderVersion version;
    if (dibSize == DEFAULT_IH_SIZE) {
        version = BITMAPINFOHEADER;
    } else if (dibSize == DEFAULT_V4IH_SIZE) {
        version = BITMAPV4HEADER;
    }
    return version; 
}

DWORD evaluateRawImageSizeInBytes (bmpPtr sample) {
    assert (sample != NULL);
    verifyDIBVersion (sample->DIBVersion);
    assert (sample->xRes > 0 && sample->yRes > 0);
    assert (sample->colorDepth == BPP_24 || sample->colorDepth == BPP_32);
    int bytesPerPixel = sample->colorDepth / 8;

    DWORD bytesPerRow = sample->xRes * bytesPerPixel;
    bytesPerRow = ceiling (bytesPerRow, 4) * 4;
    DWORD totalBytes = bytesPerRow * sample->yRes;
    return totalBytes;
}

DWORD determineFileSizeInBytes (bmpPtr sample) {
    DWORD totalBytes = evaluateRawImageSizeInBytes (sample);
    totalBytes += 14 + determineDIBSize (sample->DIBVersion);
    return totalBytes;
}

static void setDFLTPixelArray (bmpPtr bitmap) {
    verifyPixelFormat (bitmap->pixelFormat);
    free (bitmap->pixelArray);
    if (bitmap->pixelFormat == RGB_24) {
        assert (bitmap->DIBVersion == BITMAPINFOHEADER);
        bitmap->pixelArray = (pixelArray) malloc (DEFAULT_IH_XRES_RGB_24 * DEFAULT_IH_YRES_RGB_24 * sizeof (pixel));
        
        bitmap->pixelArray->blue = MAX_RGB_VALUE;
        bitmap->pixelArray->green = MIN_RGB_VALUE;
        bitmap->pixelArray->red = MIN_RGB_VALUE;

        (bitmap->pixelArray + 1)->red = MIN_RGB_VALUE;
        (bitmap->pixelArray + 1)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray + 1)->blue = MIN_RGB_VALUE;

        (bitmap->pixelArray + 2)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray + 2)->green = MIN_RGB_VALUE;
        (bitmap->pixelArray + 2)->blue = MIN_RGB_VALUE;

        (bitmap->pixelArray + 3)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray + 3)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray + 3)->blue = MAX_RGB_VALUE;
    } else if (bitmap->pixelFormat == ARGB_32) {
        assert (bitmap->DIBVersion == BITMAPV4HEADER);
        bitmap->pixelArray = (pixelArray) malloc (DEFAULT_V4IH_XRES_ARGB_32 * DEFAULT_V4IH_YRES_ARGB_32 * sizeof (pixel));
        
        // 0 0
        bitmap->pixelArray->blue = MAX_RGB_VALUE;
        bitmap->pixelArray->green = MIN_RGB_VALUE;
        bitmap->pixelArray->red = MIN_RGB_VALUE;
        bitmap->pixelArray->alpha = 127;

        // 0 1
        (bitmap->pixelArray+1)->blue = MIN_RGB_VALUE;
        (bitmap->pixelArray+1)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray+1)->red = MIN_RGB_VALUE;
        (bitmap->pixelArray+1)->alpha = 127;

        // 0 2
        (bitmap->pixelArray+2)->blue = MIN_RGB_VALUE;
        (bitmap->pixelArray+2)->green = MIN_RGB_VALUE;
        (bitmap->pixelArray+2)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray+2)->alpha = 127;

        // 0 3
        (bitmap->pixelArray+3)->blue = MAX_RGB_VALUE;
        (bitmap->pixelArray+3)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray+3)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray+3)->alpha = 127;

        // 1 0
        (bitmap->pixelArray+4)->blue = MAX_RGB_VALUE;
        (bitmap->pixelArray+4)->green = MIN_RGB_VALUE;
        (bitmap->pixelArray+4)->red = MIN_RGB_VALUE;
        (bitmap->pixelArray+4)->alpha = 255;

        // 1 1
        (bitmap->pixelArray+5)->blue = MIN_RGB_VALUE;
        (bitmap->pixelArray+5)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray+5)->red = MIN_RGB_VALUE;
        (bitmap->pixelArray+5)->alpha = 255;

        // 1 2
        (bitmap->pixelArray+6)->blue = MIN_RGB_VALUE;
        (bitmap->pixelArray+6)->green = MIN_RGB_VALUE;
        (bitmap->pixelArray+6)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray+6)->alpha = 255;

        // 1 3
        (bitmap->pixelArray+7)->blue = MAX_RGB_VALUE;
        (bitmap->pixelArray+7)->green = MAX_RGB_VALUE;
        (bitmap->pixelArray+7)->red = MAX_RGB_VALUE;
        (bitmap->pixelArray+7)->alpha = 255;
    } else {
        printf ("\t\t\t>Default pixelArray for specified pixel format is not specified\n");
        assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
        // examples for other pixel formats shoud be setup here
    }
    return;
}


static void testHelperFunctions () {
    printf ("\t\t> testing \"Helper FUnctions\" in bmp.c\n");
    testDetermineDIBSize ();
    testSetDFLTPixelArray ();
    testCeiling ();
    testToDWORD ();
    testToLittleEndianBytes ();
    testColorSpaceToLittleEndianBytes ();
    printf ("\t\t> Whooohooo all helpers in bmp.c passed\n");
    return;
}

static void testDetermineDIBSize () {
    printf ("\t\t\t> testing determineDIBSize ()\n");
    DWORD temp = determineDIBSize (BITMAPINFOHEADER);
    assert (temp == DEFAULT_IH_SIZE);
    temp = determineDIBSize (BITMAPV4HEADER);
    assert (temp == DEFAULT_V4IH_SIZE);
    return;
}

static void testSetDFLTPixelArray () {
    printf ("\t\t\t> testing setDFLTPixelArray ()\n");
    bmpPtr sample;
    sample = createBmp (BITMAPINFOHEADER);
    sample->pixelFormat = RGB_24;
    setDFLTPixelArray (sample);
    assert (sample->pixelArray->blue == MAX_RGB_VALUE);
    assert (sample->pixelArray->green == MIN_RGB_VALUE);
    assert (sample->pixelArray->red == MIN_RGB_VALUE);

    assert ((sample->pixelArray+1)->red == MIN_RGB_VALUE);
    assert ((sample->pixelArray+1)->green == MAX_RGB_VALUE);
    assert ((sample->pixelArray+1)->blue == MIN_RGB_VALUE);

    assert ((sample->pixelArray+2)->red == MAX_RGB_VALUE);
    assert ((sample->pixelArray+2)->green == MIN_RGB_VALUE);
    assert ((sample->pixelArray+2)->blue == MIN_RGB_VALUE);

    assert ((sample->pixelArray+3)->red == MAX_RGB_VALUE);
    assert ((sample->pixelArray+3)->green == MAX_RGB_VALUE);
    assert ((sample->pixelArray+3)->blue == MAX_RGB_VALUE);
    
    destroyBmp (sample);

    bmpPtr bitmap;
    bitmap = createBmp (BITMAPV4HEADER);
    bitmap->pixelFormat = ARGB_32;
    setDFLTPixelArray (bitmap);

    assert (bitmap->pixelArray->blue == MAX_RGB_VALUE);
    assert (bitmap->pixelArray->green == MIN_RGB_VALUE);
    assert (bitmap->pixelArray->red == MIN_RGB_VALUE);
    assert (bitmap->pixelArray->alpha == 127);

    // 0 1
    assert ((bitmap->pixelArray+1)->blue == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+1)->green == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+1)->red == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+1)->alpha == 127);

    // 0 2
    assert ((bitmap->pixelArray+2)->blue == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+2)->green == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+2)->red == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+2)->alpha == 127);

    // 0 3
    assert ((bitmap->pixelArray+3)->blue == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+3)->green == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+3)->red == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+3)->alpha == 127);

    // 1 0
    assert ((bitmap->pixelArray+4)->blue == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+4)->green == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+4)->red == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+4)->alpha == 255);

    // 1 1
    assert ((bitmap->pixelArray+5)->blue == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+5)->green == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+5)->red == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+5)->alpha == 255);

    // 1 2
    assert ((bitmap->pixelArray+6)->blue == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+6)->green == MIN_RGB_VALUE);
    assert ((bitmap->pixelArray+6)->red == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+6)->alpha == 255);

    // 1 3
    assert ((bitmap->pixelArray+7)->blue == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+7)->green == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+7)->red == MAX_RGB_VALUE);
    assert ((bitmap->pixelArray+7)->alpha == 255);
    destroyBmp (bitmap);
    return;
}

static void testCeiling () {
    printf ("\t\t\t> testing ceilling ()\n");
    DWORD answer = ceiling (4,5);
    assert (answer == 1);
    
    answer = ceiling (5,4);
    assert (answer == 2);

    answer = ceiling (0,2);
    assert (answer == 0);
    
    answer = ceiling (4,2);
    assert (answer == 2);

    answer = ceiling (16,4);
    assert (answer == 4);

    answer = ceiling (81,9);
    assert (answer == 9);

    return;
}

static void testToDWORD () {
    printf ("\t\t\t> testing toDWORD ()\n");
    byte bytes[4] = {255,0,0,0};
    DWORD number = toDWORD (bytes, 4);
    assert (number == 255);

    bytes[0] = 255;
    bytes[1] = 255;
    bytes[2] = 255;
    bytes[3] = 255;

    number = toDWORD (bytes, 4);
    assert (number == 4294967295);
    return;
}

static void testToLittleEndianBytes () {
    printf ("\t\t\t > testing toLittleEndiaBytes ()\n");
    DWORD number = 4294967295;
    byte bytes[4];
    LONG byteCount = 4;
    toLittleEndianBytes (number, bytes, byteCount);
    assert (bytes[0] == 255 && bytes[1] == 255 && bytes[2] == 255 && bytes[3] == 255);

    number = 256*256;
    toLittleEndianBytes (number, bytes, byteCount);
    assert (bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 1 && bytes[3] == 0);
    
    return;
}

static void testColorSpaceToLittleEndianBytes () {
    printf ("\t\t\t > testing colorSpaceToLittleEndianBytes ()\n");
    byte bytes[4];
    colorSpace cSpace = LCS_WINDOWS_COLOR_SPACE;
    colorSpaceToLittleEndianBytes (cSpace, bytes);
    assert (bytes[0] == ' ');
    assert (bytes[1] == 'n');
    assert (bytes[2] == 'i');
    assert (bytes[3] == 'W');
    return;
}

static pixelFormat determinePixelFormat (WORD colorDepth) {
    assert (colorDepth == BPP_32 || colorDepth == BPP_24);
    pixelFormat form;
    if (colorDepth == BPP_24) {
        form = RGB_24;
    } else if (colorDepth == BPP_32) {
        form = ARGB_32;
    } else {
        assert (PIXEL_FORMAT_DEFAULTS_NOT_SPECIFIED);
    }
    return form;
}