#include "stdio.h"
#include "base64.h"

int mystrlen(const char *);
char *mystrchr(const char *, int);
int strchr_of_num(const char *, char );

const char *base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char end_char = '=';

int mystrlen(const char *s)
{
	const char *sr = s;
	while(*++s);
	return s - sr;
}

char *mystrchr(const char *s, int c)
{
	while(*s != '\0' && *s != c)
		s++;
	return (char *)(*s == c ? s : NULL);
}

int base64_encode(unsigned char *dest, const unsigned char *src)
{
	int i=0, j=0;
	unsigned char ind=0;
	const int datalength = mystrlen((const char*)src);

	for (i = 0; i < datalength; i += 3)
	{
		ind = ((src[i] >> 2) & 0x3f);
		dest[j++] = base64char[(int)ind];
		ind = ((src[i] << 4) & 0x30);
		if ((i + 1) < datalength)
		{
			ind |= ((src[i + 1] >> 4) & 0x0f);
			dest[j++] = base64char[(int)ind];
		}
		else
		{
			dest[j++] = base64char[(int)ind];
			dest[j++] = end_char;
			dest[j++] = end_char;
			break;
		}
		ind = ((src[i + 1] << 2) & 0x3c);
		if (i + 2 < datalength)
		{
			ind |= ((src[i + 2] >> 6) & 0x03);
			dest[j++] = base64char[(int)ind];
			ind = src[i + 2] & 0x3f;
			dest[j++] = base64char[(int)ind];
		}
		else
		{
			dest[j++] = base64char[(int)ind];
			dest[j++] = end_char;
			break;
		}
	}
	dest[j] = '\0'; 
	return 0;
}

int strchr_of_num(const char *str, char c)
{
	const char *ind = mystrchr(str, c);
	
	return (int)(ind != NULL) ? ind - str : -1;
}

int base64_decode(unsigned char *dest, const char *src)
{
	int i = 0, j=0;
	int trans[4] = {0};
	for(i = 0; src[i] != '\0'; i += 4)
	{
		trans[0] = strchr_of_num(base64char, src[i]);
		trans[1] = strchr_of_num(base64char, src[i+1]);
		dest[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1]>>4) & 0x03);
		if(src[i + 2] == '=')
			continue;
		else
			trans[2] = strchr_of_num(base64char, src[i + 2]);
		dest[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
		if(src[i + 3] == '=')
			continue;
		else
			trans[3] = strchr_of_num(base64char, src[i + 3]);
		dest[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
	}
	dest[j] = '\0';
	return 0;
}









