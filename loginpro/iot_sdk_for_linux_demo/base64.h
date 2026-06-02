#ifndef __BASE64_H__
#define __BASE64_H__

/******************************
*名    称 : base64_encode()
*功    能 : base64的编码
*入口参数 : src为需要编码的字符串	dest为编码之后的字符串
*返回参数 :	0代表转换结束
*说    明 : 无
******************************/
int base64_encode(unsigned char *dest, const unsigned char *src);

/******************************
*名    称 : base64_decode()
*功    能 : base64的解码
*入口参数 : src为需要解码的字符串	dest为解码之后的字符串
*返回参数 :	0代表转换结束
*说    明 : 无
******************************/
int base64_decode(unsigned char *dest, const char *src);

#endif











