#include "mbedtls/entropy.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/ssl.h"

#include "esp_rom_md5.h"
#include "psa/crypto.h"

#include "esp_random.h"

void mbedtls_entropy_init(mbedtls_entropy_context *ctx) {
    (void)ctx;
}

void mbedtls_entropy_free(mbedtls_entropy_context *ctx) {
    (void)ctx;
}

int mbedtls_entropy_func(void *ctx, unsigned char *output, size_t len) {
    (void)ctx;
    if (output == NULL || len == 0) {
        return -1;
    }
    esp_fill_random(output, len);
    return 0;
}

// Mbed TLS 4 uses PSA Crypto's RNG and removed this configuration API. The
// function remains as a no-op so the prebuilt Volcano RTC Lite archive links.
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng) {
    (void)conf;
    (void)f_rng;
    (void)p_rng;
}

int mbedtls_md5(const unsigned char *input, size_t ilen, unsigned char output[16]) {
    md5_context_t context;
    esp_rom_md5_init(&context);
    esp_rom_md5_update(&context, input, (uint32_t)ilen);
    esp_rom_md5_final(output, &context);
    return 0;
}

int mbedtls_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]) {
    size_t output_len = 0;
    if (psa_crypto_init() != PSA_SUCCESS) {
        return -1;
    }
    if (psa_hash_compute(PSA_ALG_SHA_1, input, ilen, output, 20, &output_len) !=
            PSA_SUCCESS || output_len != 20) {
        return -1;
    }
    return 0;
}
