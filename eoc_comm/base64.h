#ifndef _BASE64_H_
#define _BASE64_H_

#ifdef __cplusplus
extern "C"
{
#endif

int base64_encode(const unsigned char *in, unsigned long len,
                  unsigned char *out);

int base64_decode(const unsigned char *in, unsigned char *out);

//
#ifdef __cplusplus
}
#endif //__cplusplus
#endif //_BASE64_H_
