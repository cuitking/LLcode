#include "svr_auth.h"
#include "md5.h"
#include "Rijndael.h"

static void
tohex(const char bin[], char output[], int n) {
	static char hex[] = "0123456789abcdef";
	int i;
	for (i=0;i<n;i++) {
		output[i*2] = hex[bin[i] >> 4];
		output[i*2+1] = hex[bin[i] & 0xf];
	}
}

#define HEX(v,c) { char tmp = (char) c; if (tmp >= '0' && tmp <= '9') { v = tmp-'0'; } else { v = tmp - 'a' + 10; } }

static void
fromhex(const char input[], char output[], int n) {
	int i;
	for (i=0;i<n;i++) {
		char hi,low;
		HEX(hi, input[i*2]);
		HEX(low, input[i*2+1]);
		output[i] = hi<<4 | low;
	}
}

int ejoy_decrypt_auth_token(const char* token, unsigned int token_sz, const char* svr_pass, 
							unsigned int svr_pass_sz, const char* svr_sig, unsigned int svr_sig_sz, struct ejoy_token* t)
{
	if (token_sz != 160)
	{
		//token长度无效
		return 1;
	}

	char ts[128];
	memcpy(ts, token, 128);
	char tss[16];
	fromhex(token+128, tss, 16);

	MD5_CTX md5;  
	MD5Init(&md5);           
	char decrypt[16];      
	MD5Update(&md5, (unsigned char*)ts, 128); 
	MD5Update(&md5, (unsigned char*)svr_sig, svr_sig_sz);  
	MD5Final(&md5, (unsigned char*)decrypt); 

	for (int i = 0; i < 16; i++)
	{
		if (tss[i] != decrypt[i])
		{
			//token验证失败
			return 1;
		}
	}

	char tsss[64];
	fromhex(token, tsss, 64);
	CRijndael oRijndael;
	oRijndael.MakeKey((char*)svr_pass, NULL, svr_pass_sz, svr_pass_sz);
	char ret[64];
	oRijndael.Decrypt(tsss, ret, 64, CRijndael::ECB);

	char temp[3][64] = {0};
	int index = 0;
	int pos = 0;
	for (int i = 0; i < 64; i++)
	{
		if (ret[i] == '\n')
		{
			index++;
			pos = 0;
		}
		else
		{
			temp[index][pos++] = ret[i]; 
		}
	}

	//token验证成功
	memcpy(t->uid, temp[0], sizeof(t->uid));
	t->timestamp = atol(temp[1]);
	t->randomkey = atol(temp[2]);

	return 0;
}