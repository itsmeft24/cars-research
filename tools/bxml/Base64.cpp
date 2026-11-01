#include "Base64.h"

unsigned char gBin64ToASCII[64] = {
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
   'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
   'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
   'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
   'w', 'x', 'y', 'z', '0', '1', '2', '3',
   '4', '5', '6', '7', '8', '9', '+', '/'
};

unsigned char gASCIIToBin64[256] = {
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,  
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,      
   255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,  
   255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,    
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 
};

int ConvertBinToBase64ASCII(void *pBinIn, int inSize, char *pBase64Out, int outSize)
{
   int i, chunks;
   unsigned char *pIn, *pOut;

   if (outSize < GETCONVSIZEFROM256(inSize)) {
      return -1;
   }
   
   chunks = inSize /3;
   pIn = (unsigned char*)pBinIn;
   pOut = (unsigned char *)pBase64Out;
   for (i =0; i < chunks; i++) {
      pOut[0] = gBin64ToASCII[pIn[0] & 0x3F];
      pOut[1] = gBin64ToASCII[(pIn[0] >> 6) | ((pIn[1]<<2) & 0x3C)];
      pOut[2] = gBin64ToASCII[(pIn[1] >> 4) | ((pIn[2]<<4) & 0x30)];
      pOut[3] = gBin64ToASCII[(pIn[2] >> 2)];

      pIn += 3;
      pOut += 4;
   }

   switch(inSize%3) {
      case 2:
         pOut[0] = gBin64ToASCII[pIn[0] & 0x3F];
         pOut[1] = gBin64ToASCII[(pIn[0] >> 6) | ((pIn[1]<<2) & 0x3C)];
         pOut[2] = gBin64ToASCII[(pIn[1] >> 4)];
         break;
      case 1:
         pOut[0] = gBin64ToASCII[pIn[0] & 0x3F];
         pOut[1] = gBin64ToASCII[(pIn[0] >> 6)];
      default:
         break;
   }

   int realSize = GETCONVSIZEFROM256(inSize);
   if (outSize > realSize) {
      pBase64Out[realSize] = 0;
   }

   return realSize;
}

int ConvertBase64ASCIIToBin(char *pBase64In, int inSize, void *pBinOut, int outSize)
{
   int i, chunks;
   unsigned char *pIn, *pOut, tmp[4];

   if (outSize < GETCONVSIZEFROM64(inSize)) {
      return -1;
   }

   chunks = inSize /4;
   pIn = (unsigned char*)pBase64In;
   pOut = (unsigned char*)pBinOut;
   for (i =0; i < chunks; i++) {
      tmp[0] = gASCIIToBin64[pIn[0]];
      tmp[1] = gASCIIToBin64[pIn[1]];
      tmp[2] = gASCIIToBin64[pIn[2]];
      tmp[3] = gASCIIToBin64[pIn[3]];
      
      if (tmp[0] > 63 || tmp[1] > 63 || tmp[2] > 63 || tmp[3] > 63 ) {
         //invalid character
         return -1;
      }

      pOut[0] = tmp[0] | ((tmp[1]<<6) & 0xC0);
      pOut[1] = (tmp[1]>>2) | ((tmp[2] << 4) & 0xF0);
      pOut[2] = (tmp[2]>>4) | (tmp[3]<<2);
      
      pIn += 4;
      pOut += 3;
   }

   switch(inSize%4) {
      case 3:
         tmp[0] = gASCIIToBin64[pIn[0]];
         tmp[1] = gASCIIToBin64[pIn[1]];
         tmp[2] = gASCIIToBin64[pIn[2]];
         
         //only 4 bits used in tmp[2]
         if (tmp[0] > 63 || tmp[1] > 63 || tmp[2] > 15) {
            //invalid character
            return -1;
         }

         pOut[0] = tmp[0] | ((tmp[1]<<6) & 0xC0);
         pOut[1] = (tmp[1]>>2) | ((tmp[2] << 4) & 0xF0);
         break;
      case 2:
         tmp[0] = gASCIIToBin64[pIn[0]];
         tmp[1] = gASCIIToBin64[pIn[1]];
         
         //only 2 bits used in tmp[1]
         if (tmp[0] > 63 || tmp[1] > 3) {
            //invalid character
            return -1;
         }

         pOut[0] = tmp[0] | ((tmp[1]<<6) & 0xC0);
      case 0:
         break;
      default://invalid: not multiple of 8 bits
         return -1;
   }

   return GETCONVSIZEFROM64(inSize);
}
