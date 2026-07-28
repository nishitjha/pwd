
#include "sha1.h"
#include "stdlib.h"
#include "string.h"

void hmac_sha1(const uint8_t *key, size_t key_len,
               const uint8_t *msg, size_t msg_len,
               uint8_t out_digest[20])
{
    uint8_t k[64] = {0};
    uint8_t o_key_pad[64], i_key_pad[64];
    uint8_t inner_hash[20];
    SHA1_CTX ctx;

    if (key_len > 64)
    {
        SHA1Init(&ctx);
        SHA1Update(&ctx, key, key_len);
        SHA1Final(k, &ctx);
    }
    else
    {
        memcpy(k, key, key_len);
    }

    for (int i = 0; i < 64; i++)
    {
        o_key_pad[i] = k[i] ^ 0x5c;
        i_key_pad[i] = k[i] ^ 0x36;
    }

    SHA1Init(&ctx);
    SHA1Update(&ctx, i_key_pad, 64);
    SHA1Update(&ctx, msg, msg_len);
    SHA1Final(inner_hash, &ctx);

    SHA1Init(&ctx);
    SHA1Update(&ctx, o_key_pad, 64);
    SHA1Update(&ctx, inner_hash, 20);
    SHA1Final(out_digest, &ctx);
}
