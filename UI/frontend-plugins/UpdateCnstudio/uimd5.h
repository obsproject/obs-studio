/*****************************************************************************

file name :uc_md5.h
Describe  :Encryption Libarary encrpyt the data using 
the md5
Author    :jgw
Data      :2003/3/26
Place     :Longmaster corporation,Yuanchenxin Building

****************************************************************************/

#ifndef _INCLUDE_MD5_H
#define _INCLUDE_MD5_H

#include <string>

using namespace std;
/* MD5 Class. */
class CUIMD5
{
public:
	CUIMD5();
	virtual ~CUIMD5();
	void	Encrypt(char * apInput, int aiInputLen, unsigned char apOutput[16]);
	void	Encrypt2String(char * apInput, int aiInputLen, char apOutput[33]);
	static	string MD5(const char* aszText, int aiLen = -1);
	static  string MD5(ifstream & astrStream);
	static  string MD5(ifstream & astrStream, unsigned long long aiTotalLen);
	static  string MD5File(const char * aszFileName);
private:
	void MD5Init ();
	void MD5Update ( unsigned char *input, unsigned int inputLen);
	void MD5Final (unsigned char digest[16]);
	void MD5Transform (unsigned long int state[4], unsigned char block[64]);
	void MD5_memcpy (unsigned char* output, unsigned char* input,unsigned int len);
	void Encode (unsigned char *output, unsigned long int *input,unsigned int len);
	void Decode (unsigned long int *output, unsigned char *input, unsigned int len);
	void MD5_memset (unsigned char* output,int value,unsigned int len);
	void ConvertString(char *apIn,char * alpOut);

	unsigned long int state[4];					/* state (ABCD) */
	unsigned long int count[2];					/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];       /* input buffer */
	unsigned char PADDING[64];		/* What? */
};

#endif
