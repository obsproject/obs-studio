/******************************************************************************
*  Copyright (C) 2000 by Robert Hubley.                                      *
*  All rights reserved.                                                      *
*                                                                            *
*  This software is provided ``AS IS'' and any express or implied            *
*  warranties, including, but not limited to, the implied warranties of      *
*  merchantability and fitness for a particular purpose, are disclaimed.     *
*  In no event shall the authors be liable for any direct, indirect,         *
*  incidental, special, exemplary, or consequential damages (including, but  *
*  not limited to, procurement of substitute goods or services; loss of use, *
*  data, or profits; or business interruption) however caused and on any     *
*  theory of liability, whether in contract, strict liability, or tort       *
*  (including negligence or otherwise) arising in any way out of the use of  *
*  this software, even if advised of the possibility of such damage.         *
*                                                                            *
******************************************************************************

	MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm

	Port to Win32 DLL by Robert Hubley 1/5/2000

	Original Copyright:

	Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
	rights reserved.

	License to copy and use this software is granted provided that it
	is identified as the "RSA Data Security, Inc. MD5 Message-Digest
	Algorithm" in all material mentioning or referencing this software
	or this function.
		  
	License is also granted to make and use derivative works provided
	that such works are identified as "derived from the RSA Data
	Security, Inc. MD5 Message-Digest Algorithm" in all material
	mentioning or referencing the derived work.
	
	RSA Data Security, Inc. makes no representations concerning either
	the merchantability of this software or the suitability of this
	software for any particular purpose. It is provided "as is"
	without express or implied warranty of any kind.
		  
	These notices must be retained in any copies of any part of this
	documentation and/or software.
*/
#include "uimd5.h"
#include <fstream>

using namespace std;

/* Constants for MD5Transform routine.
*/
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

/* F, G, H and I are basic MD5 functions.
*/
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
*/
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
*/
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (unsigned long int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (unsigned long int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (unsigned long int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (unsigned long int)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}


/* MD5 initialization. Begins an MD5 operation, writing a new context.
*/

CUIMD5::CUIMD5()
{
	MD5Init ();
}

CUIMD5::~CUIMD5()
{
}

void CUIMD5::MD5Init ()
{
	count[0] = count[1] = 0;
	/* Load magic initialization constants.*/
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
	/* Add by Liguangyi */
	MD5_memset(PADDING, 0, sizeof(PADDING));
	*PADDING=0x80;
	//PADDING = {
	//	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	};
}

/* MD5 block update operation. Continues an MD5 message-digest
operation, processing another message block, and updating the
context.
*/
void CUIMD5::MD5Update (unsigned char *input,unsigned int inputLen)
{
	unsigned int i, index, partLen;
	
	/* Compute number of bytes mod 64 */
	index = (unsigned int)((this->count[0] >> 3) & 0x3F);
	
	/* Update number of bits */
	if ((this->count[0] += ((unsigned long int)inputLen << 3))
		< ((unsigned long int)inputLen << 3))
		this->count[1]++;
	this->count[1] += ((unsigned long int)inputLen >> 29);
	
	partLen = 64 - index;
	
	/* Transform as many times as possible.
	*/
	if (inputLen >= partLen) {
		MD5_memcpy((unsigned char*)&this->buffer[index], 
			(unsigned char*)input, partLen);
		MD5Transform (this->state, this->buffer);
		
		for (i = partLen; i + 63 < inputLen; i += 64)
			MD5Transform (this->state, &input[i]);
		
		index = 0;
	}
	else
		i = 0;
	
	/* Buffer remaining input */
	MD5_memcpy ((unsigned char*)&this->buffer[index], (unsigned char*)&input[i], inputLen-i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
the message digest and zeroizing the context.
*/
void CUIMD5::MD5Final (unsigned char digest[16])
{
	unsigned char bits[8];
	unsigned int index, padLen;
	
	/* Save number of bits */
	Encode (bits, this->count, 8);
	
	/* Pad out to 56 mod 64.
	*/
	index = (unsigned int)((this->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5Update ( PADDING, padLen);
	
	/* Append length (before padding) */
	MD5Update (bits, 8);
	/* Store state in digest */
	Encode (digest, this->state, 16);
	
	/* Zeroize sensitive information.
	*/
	MD5_memset ((unsigned char*)this, 0, sizeof (*this));
	this->MD5Init();
}

/* MD5 basic transformation. Transforms state based on block.
*/
void CUIMD5::MD5Transform (unsigned long int state[4], unsigned char block[64])
{
	unsigned long int a = state[0], b = state[1], c = state[2], d = state[3], x[16];
	
	Decode (x, block, 64);
	
	/* Round 1 */
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */
	
	/* Round 2 */
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */
	
	/* Round 3 */
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */
	
	/* Round 4 */
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */
	
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	
	/* Zeroize sensitive information.
	*/
	MD5_memset ((unsigned char*)x, 0, sizeof (x));
}

/* Encodes input (unsigned long int) into output (unsigned char). Assumes len is
a multiple of 4.
*/
void CUIMD5::Encode (unsigned char *output, unsigned long int *input,unsigned int len)
{
	unsigned int i, j;
	
	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

/* Decodes input (unsigned char) into output (unsigned long int). Assumes len is
a multiple of 4.
*/
void CUIMD5::Decode (unsigned long int *output, unsigned char *input, unsigned int len)
{
	unsigned int i, j;
	
	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((unsigned long int)input[j]) | (((unsigned long int)input[j+1]) << 8) |
		(((unsigned long int)input[j+2]) << 16) | (((unsigned long int)input[j+3]) << 24);
}

/* Note: Replace "for loop" with standard memcpy if possible.
*/

void CUIMD5::MD5_memcpy (unsigned char* output, unsigned char* input,unsigned int len)
{
	unsigned int i;
	
	for (i = 0; i < len; i++)
		output[i] = input[i];
}

/* Note: Replace "for loop" with standard memset if possible.
*/
void CUIMD5::MD5_memset (unsigned char* output,int value,unsigned int len)
{
	unsigned int i;
	
	for (i = 0; i < len; i++)
		((char *)output)[i] = (char)value;
}


void CUIMD5::Encrypt(char * apInput, int aiInputLen, unsigned char apOutput[16])
{
	MD5Update((unsigned char *)apInput, aiInputLen);
	MD5Final(apOutput);
}
void CUIMD5::Encrypt2String(char * apInput, int aiInputLen, char apOutput[33])
{
	unsigned char lpMD5[16];
	Encrypt(apInput,aiInputLen,lpMD5);
	ConvertString((char *)lpMD5,(char *)apOutput);

}
void CUIMD5::ConvertString(char *apIn,char * alpOut)
{
	unsigned char lcTemp=0;
	for(int i=0;i<16;i++){
		lcTemp=apIn[i];
		lcTemp=lcTemp&0xF0;
		lcTemp=lcTemp>>4;
		if (lcTemp<10)
			alpOut[2*i]=lcTemp+'0';
		else
			alpOut[2*i]=lcTemp+87;

		lcTemp=apIn[i];
		lcTemp=lcTemp&0x0F;
		if (lcTemp<10)
			alpOut[2*i+1]=lcTemp+'0';
		else
			alpOut[2*i+1]=lcTemp+87;
	}
	alpOut[32]=0;
}

string CUIMD5::MD5(const char* aszText, int aiLen)
{
	char lszMd5[32+1];
	CUIMD5 loMD5;	
	memset(lszMd5, 0, 32+1);
	int liLen = aiLen;
	if (liLen<=0)
	{
		liLen = strlen(aszText);
	}
	loMD5.Encrypt2String((char *)aszText, liLen, lszMd5);
	return lszMd5;

}

string CUIMD5::MD5(ifstream & astrStream)
{
	unsigned char lpMD5[16] = {0};
	char buff[8000];
	CUIMD5 loMD5;
	int liRead = 0;
	do 
	{
		astrStream.read(buff, 8000);
		liRead = astrStream.gcount();
		loMD5.MD5Update((unsigned char*)buff, liRead);
	} while (liRead>0);

	loMD5.MD5Final(lpMD5);
	char apOutput[33] = {0};
	loMD5.ConvertString((char *)lpMD5, (char *)apOutput);

	return apOutput;
}

// 怎么都没找到min，max的头文件-_-
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
string CUIMD5::MD5(ifstream & astrStream, unsigned long long aiTotalLen)
{
	unsigned char lpMD5[16] = {0};
	int liBuffLen = 200 * 1000;
	char * lpbuff = new char[liBuffLen];
	CUIMD5 loMD5;
	bool lbBreak = false;
	unsigned long long liRead = 0;
	do 
	{
		astrStream.read(lpbuff, min(liBuffLen, aiTotalLen));
		liRead = astrStream.gcount();
		if (liRead < liBuffLen||liRead > aiTotalLen)
		{
			//最后一块
			liRead = min(aiTotalLen,liRead);
			lbBreak = true;
		}
		loMD5.MD5Update((unsigned char*)lpbuff, liRead);
		if (lbBreak)
		{
			break;
		}
		aiTotalLen -= liRead;
	} while (liRead>0);

	loMD5.MD5Final(lpMD5);
	char apOutput[33] = {0};
	loMD5.ConvertString((char *)lpMD5, (char *)apOutput);

	delete [] lpbuff;
	return apOutput;
}

string CUIMD5::MD5File(const char * aszFileName)
{
	//立即文件校验
	ifstream loFileStream(aszFileName, ios::binary|ios::in);
	string lstrNewMD5;
	if (loFileStream.is_open())
	{
		lstrNewMD5 = CUIMD5::MD5(loFileStream);
		loFileStream.close();
	}
	return lstrNewMD5;
}