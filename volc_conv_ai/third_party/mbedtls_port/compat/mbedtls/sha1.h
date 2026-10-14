#ifndef VOLC_MBEDTLS_SHA1_COMPAT_H
#define VOLC_MBEDTLS_SHA1_COMPAT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int mbedtls_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

#ifdef __cplusplus
}
#endif

#endif
