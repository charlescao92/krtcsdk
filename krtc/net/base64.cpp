/**
 * @file    base64.cpp
 * @brief   BASE64编解码方法
 * @author  xiangwangfeng <xiangwangfeng@gmail.com>
 * @data	2011-4-23
 * @website www.xiangwangfeng.com
 */

#include "standard_header.h"
#include "base64.h"
#include <string>


NAMESPACE_BEGIN(Util)

const int kdecode_error	=	-1;

static char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int	base64_encode(const char *data, int size, char **str)
{
    char *s, *p;
    int i;
    int c;
    const unsigned char *q;

    p = s = new char[size * 4 / 3 + 4];
    if (p == 0)
        return -1;
    q = (const unsigned char *) data;
    i = 0;
    for (i = 0; i < size;) {
        c = q[i++];
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        p[0] = base64_chars[(c & 0x00fc0000) >> 18];
        p[1] = base64_chars[(c & 0x0003f000) >> 12];
        p[2] = base64_chars[(c & 0x00000fc0) >> 6];
        p[3] = base64_chars[(c & 0x0000003f) >> 0];
        if (i > size)
            p[3] = '=';
        if (i > size + 1)
            p[2] = '=';
        p += 4;
    }
    *p = 0;
    *str = s;
    return strlen(s);
}



static int	pos(char c)
{
    char *p;
    for (p = base64_chars; *p; p++)
        if (*p == c)
            return p - base64_chars;
    return -1;
}



static int	token_decode(const char *token)
{
    int i;
    unsigned int val = 0;
    int marker = 0;
    if (strlen(token) < 4)
        return kdecode_error;
    for (i = 0; i < 4; i++) {
        val *= 64;
        if (token[i] == '=')
            marker++;
        else if (marker > 0)
            return kdecode_error;
        else
            val += pos(token[i]);
    }
    if (marker > 2)
        return kdecode_error;
    return (marker << 24) | val;
}

int	base64_decode(const char *str, char *data)
{
    const char *p;
    unsigned char *q;

    q = (unsigned char*)data;
    for (p = str; *p && (*p == '=' || strchr(base64_chars, *p)); p += 4) {
        int val = token_decode(p);
        unsigned int marker = (val >> 24) & 0xff;
        if (val == kdecode_error)
            return -1;
        *q++ = (val >> 16) & 0xff;
        if (marker < 2)
            *q++ = (val >> 8) & 0xff;
        if (marker < 1)
            *q++ = val & 0xff;
    }
    return q - (unsigned char *) data;
}

void	base64Encode(const std::string& input,std::string& output)
{
	char* buff = 0;
	int length = base64_encode(input.c_str(),input.size(),&buff);
	if (length != -1&& buff !=0)
	{
		output = std::string(buff,length);
		delete []buff;
	}
	else
	{
		assert(false);
		output.clear();
	}
}

void	base64Decode(const std::string& input,std::string& output)
{
	char* buffer	= new char[input.size()];
	int length		= base64_decode(input.c_str(),buffer);
	if (length != -1)
	{
		output = std::string(buffer,length);
	}
	else
	{
		assert(false);
		output.clear();
	}
	delete []buffer;
}


NAMESPACE_END(Util)