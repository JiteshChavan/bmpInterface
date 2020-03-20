# bmpInterface
A library to interface with .bmp file format
Support:
    more DIB headers can be integrated by updating the interface (bmp.h file) and including its functionality in bmp.c
    at multiple assertion stages there are verifications for supported DIBHeader versions, so keep that in mind
Abstraction and Extensibility:
    To add support for more DIB header versions:
        1.functions to test initializeDFLT (version) : for eg: assertBITMAPINFOHEADERDFLTS (bmpPtr image) should be provided in the interface of the unit tester for said
        version..which then should be plugged in testBmp.h :: testInitializeBmpDFLT ()
        since its the only function in bmp ADT that directly hsa something to do with the DIB header ADTs.
        2. DIB ADTs have complete control over pixelArrays

IMPORTANT :
    >Please note that all the unit testers have been disabled by /* */ style comments
    >finalBuild has all the unit tests disabled (its compiled without testBmp.c)
    >mk1Bmp has all the tests enabled.
    >tests have to be enabled on following lines:
        > main.c :: line 12
        > bmp.c :: line 105 and line 1098
    >tests can be enabled by compiling as follows:
        >gcc -Wall -O -o k-On main.c bmp.c testBmp.c
    >execute as ./k-On
    >imageGenerator.c is an illustration of how to use the interface bmp.h
        > it generates 2 images one RGB_24 and another ARGB_32
        > both of 1920x1080 resolution
    >main.c alose generates such two images but with very small resolution
