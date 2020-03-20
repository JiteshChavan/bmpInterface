//  bmp.h
//
// created by  J.Chavan on 26th February,2020

#define MAX_IMAGE_NAME_LENGTH 256   // use '<' not '<='
#define MAX_RELATIVE_PATH_LENGTH 4097


// supported DIB header versions
// BITMAPINFOHEADER
#define BITMAPINFOHEADER 0
#define BITMAPINFOHEADER_SIZE 40
// BITMAPV4HEADER
#define BITMAPV4HEADER 1
#define BITMAPV4HEADER_SIZE 108

// supproted pixelFormats
// introduce redundancy along with colorDepth parameter (beware!)
#define RGB_24 0
#define ARGB_32 1

// suported color depths
#define BPP_24 24
#define BPP_32 32

// supported compression methods
// no compression
#define BI_RGB 0 
#define BI_BITFIELDS 3
// supported color spaces
// LCS windows color space
#define LCS_WINDOWS_COLOR_SPACE 0

// channel types
#define RED 0
#define GREEN 1
#define BLUE 2
#define ALPHA 3

#define MIN_RGB_VALUE 0
#define MAX_RGB_VALUE 255

#define COLOR_PLANCE_COUNT 1

typedef unsigned char byte;
typedef unsigned int DWORD;
typedef unsigned short int WORD;
typedef int LONG;

typedef int DIBHeaderVersion;
typedef int pixelFormat;
typedef DWORD colorSpace;
typedef int channelType;

typedef char relativePath [MAX_RELATIVE_PATH_LENGTH];
typedef char fileName [MAX_IMAGE_NAME_LENGTH];

typedef struct bmp *bmpPtr;
typedef struct channel *channelPtr;

// creates an instance of ADT 'bmp' (sets the DIBHEaderVersion and DIbSize) and returns a pointer to it
bmpPtr createBmp (DIBHeaderVersion version);
// creates an instance of ADT 'channel' of specified dimensions and returns a pointer to it
channelPtr createChannel (LONG xRes, LONG yRes);

// initializes the ADT with default values for the specified version.
// (the default values should be specified in the interface of the corresponding DIB header eg: BITMAPINFOHEADER.h)
// pixelFormat corresponds to color depth in some sense (RGB24/RGBA32 etc)
void initializeBmpDFLT (bmpPtr bitmap, pixelFormat pixelFormat);

// destroys the instance of ADT 'bmp' frees any memory associated with it
void destroyBmp (bmpPtr sampleImage);
// destroys the instance of ADT 'channel' frees any memory associated with it
void destroyChannel (channelPtr channel);

// reading and parsing go hand in hand while testing.
// parses a '.bmp' file. Stores its data in an isntance of ADT 'bmp' and returns a poitner to it
bmpPtr parseBitMap (relativePath srcFilePath);

// saves the specified bitmap image as a '.bmp' file on the hard drive
void saveBitMap (bmpPtr sampleBitmap, fileName imageFileName, relativePath destination);


// Acess functions ADT : 'bmp'

// returns the DIB header version of the bitmap.
DIBHeaderVersion getDIBHeaderVersion (bmpPtr bitMap);
// returns the size of the DIBHeader 
DWORD getDIBHeaderSize (bmpPtr bitMap);
// sets the DIB header version and hence the dibSize of the bitMap
void setDIBHeaderVersion (bmpPtr sample, DIBHeaderVersion version);

// sets the pixel format of thebitMap image
void setPixelFormat (bmpPtr bitMap, pixelFormat pixelFormat);
// returns the pixel format of thebitMap image
pixelFormat getPixelFormat (bmpPtr bitMap);

// returns the resolution 'width' (pixels) of the bitmap image
LONG getXRes (bmpPtr bitMap);
// sets the resolution 'width' (pixels) of the bitmap image
void setXRes (bmpPtr bitMap, LONG xRes);

// returns the resolution 'height' (pixels) of the bitmap image
LONG getYRes (bmpPtr bitMap);
// sets the resolution 'height' (pixels) of the bitmap image
void setYRes (bmpPtr bitMap, LONG yRes);

// allocates memory for pixel array (xRes and yRes must be initialized beforehand)
void setUpPixelArray (bmpPtr sample);

// returns the number of color planes of the bitmap image
// MUST BE 1 hence no fucntion to change the value.
WORD getColorPlaneCount (bmpPtr bitMap);
// assumes that argument colorPlanceCount is 1.
void setColorPlaneCount (bmpPtr sample, WORD colorPlanceCount);

// retunrs color depth (bits/pixel) of the image
WORD getColorDepth (bmpPtr bitMap);
// sets color depth of the image
void setColorDepth (bmpPtr bitMap, WORD colorDepth);

// ***supporetd compression methods are listed in this interface
// returns compression method enumeration
DWORD getCompression (bmpPtr bitMap);
// set compression method enumeration
void setCompression (bmpPtr bitMap, DWORD compression);

// returns (raw) image data size (corresponds to only pixels and padding)
DWORD getImageSize (bmpPtr bitMap);
// sets image data size (corresponds to only pixels and padding)
void setImageSize (bmpPtr bitMap, DWORD imageSize);

// returns horizontal print resolution of target device (pixels/metre)
LONG  getPrintResX (bmpPtr bitMap);
// sets horizontal print resolution of target device (pixels/metre)
void  setPrintResX (bmpPtr bitMap, LONG xPrintRes);

// returns vertical print resolution of target device (pixels/metre)
LONG  getPrintResY (bmpPtr bitMap);
// sets vertical print resolution of target device (pixels/metre)
void  setPrintResY (bmpPtr bitMap, LONG yPrintRes);

// returns the number of colors in color palette
DWORD getPaletteColorCount (bmpPtr bitMap);
// sets the number of colors in color palette
void setPaletteColorCount (bmpPtr bitMap, DWORD paletteColorCount);

// returns the number of important colors used (generally ignored)
DWORD getImpColorCount (bmpPtr bitMap);
// sets the same thing
void setImpColorCount (bmpPtr bitMap, DWORD impColorCount);

// sets the color space of the bitmap (valid function for only BITMAPV4HEADER)
// ***supported color space are listed in this interface
void setColorSpace (bmpPtr sample, colorSpace colorSpace);
// returns the color space of specified bitmap
colorSpace getColorSpace (bmpPtr sample);

// misc. ops
DWORD determineFileSizeInBytes (bmpPtr sample);
DWORD evaluateRawImageSizeInBytes (bmpPtr sample);

// Access functions ADT : 'channel'
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the RED channel and returns a pointer to it.
channelPtr getRedChannel (bmpPtr bitMap);
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the GREEN channel and returns a pointer to it.
channelPtr getGreenChannel (bmpPtr bitMap);
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the BLUE channel and returns a pointer to it.
channelPtr getBlueChannel (bmpPtr bitMap);
// takes an 'initialized bmp ADT instance reference' as input, creates and initializes the ALPHA channel and returns a pointer to it.
// provided the there exists alpha channel in the pixel format 
channelPtr getAlphaChannel (bmpPtr bitMap);


// to access dimensions of channel
LONG getChXRes (channelPtr channel);
// to access dimensions of channel
LONG getChYRes (channelPtr channel);


// to access specified pixel of specified channel
byte getPixel (LONG row, LONG column, channelPtr channel);
// to access specified pixel of specified channel
void setPixel (LONG row, LONG column, channelPtr channel, byte pixelVal);

// sets the channel of specified type in specified bitMap
void setChannel (channelType channelType, bmpPtr bitMap, channelPtr srcChannel);





// defaults for example BITMAPINFOHEADER bitmap image 
#define DEFAULT_IH_SIZE BITMAPINFOHEADER_SIZE
#define DEFAULT_IH_COMPRESSION BI_RGB
#define DEFAULT_IH_COLOR_PLANE_COUNT 1
#define DEFAULT_IH_PRINT_RES_X 2835
#define DEFAULT_IH_PRINT_RES_Y 2835
#define DEFAULT_IH_PALETTE_CLR_COUNT 0
#define DEFAULT_IH_IMP_COLOR_COUNT 0

//(RGB24 example)
#define DEFAULT_IH_XRES_RGB_24 2
#define DEFAULT_IH_YRES_RGB_24 2

#define DEFAULT_IH_COLOR_DEPTH_RGB_24 24
#define DEFAULT_IH_IMAGE_SIZE_RGB_24 16


// pixelArray for example image 2x2
// 0 0 255 red
// 255 255 255 white
// 0 0 padding
// 255 0 0 blue
// 0 255 0 green
// 0 0 padding

// defaults for example BITMAPV4HEADER bitmap image
#define DEFAULT_V4IH_SIZE BITMAPV4HEADER_SIZE
#define DEFAULT_V4IH_COMPRESSION BI_BITFIELDS
#define DEFAULT_V4IH_COLOR_PLANE_COUNT 1
#define DEFAULT_V4IH_PRINT_RES_X 2835
#define DEFAULT_V4IH_PRINT_RES_Y 2835
#define DEFAULT_V4IH_PALETTE_CLR_COUNT 0
#define DEFAULT_V4IH_IMP_COLOR_COUNT 0
#define DEFAULT_V4IH_COLOR_SPACE LCS_WINDOWS_COLOR_SPACE 
//(ARGB32 example)
#define DEFAULT_V4IH_XRES_ARGB_32 4
#define DEFAULT_V4IH_YRES_ARGB_32 2

#define DEFAULT_V4IH_COLOR_DEPTH_ARGB_32 32
#define DEFAULT_V4IH_IMAGE_SIZE_ARGB_32 32

// ARGB32/CHANNEL_MASKS
#define RED_CHANNEL_MASK 0x00FF0000
#define GREEN_CHANNEL_MASK 0x0000FF00
#define BLUE_CHANNEL_MASK 0x000000FF
#define ALPHA_CHANNEL_MASK 0xFF000000


// pixelArray for example image 2x4
// B G R A
// 255 0 0 255  (1,0) (BLUE)
// 0 255 0 255 (1,1) (GREEN)
// 0 0 255 255 (1,2) (RED)
// 255 255 255 127 (1,3) (WHITE)
// 255 0 0 127 (0,0) (BLUE)
// 0 255 0 127 (0,1) (GREEN)
// 0 0 255 127 (0,2) (RED)
// 255 255 255 127 (0,3) (WHITE)
