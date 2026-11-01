#ifndef _BASE64_H
#define _BASE64_H

#define GETCONVSIZEFROM256(size) ((size/3)*4 + (((size%3) == 2)?3:((size%3)?2:0)))
#define GETCONVSIZEFROM64(size) (size/4)*3 + (((size%4) == 3)?2:(((size%4)==2)?1:0))

extern int ConvertBinToBase64ASCII(void *pBinIn, int inSize, char *pBase64Out, int outSize);
extern int ConvertBase64ASCIIToBin(char *pBase64In, int inSize, void *pBinOut, int outSize);

#endif //_BASE64_H