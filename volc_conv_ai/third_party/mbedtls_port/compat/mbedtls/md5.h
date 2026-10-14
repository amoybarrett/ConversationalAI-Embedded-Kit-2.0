#ifndef VOLC_MBEDTLS_MD5_COMPAT_H
#define VOLC_MBEDTLS_MD5_COMPAT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int mbedtls_md5(const unsigned char *input, size_t ilen, unsigned char output[16]);

#ifdef __cplusplus
}
#endif

#endif
