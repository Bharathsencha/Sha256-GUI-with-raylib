#include "sha256.h"

// rotate-right: rotates the bits of x to the right by n positions 
static inline u32 rotr(u32 x, u32 n) {
    return (x >> n) | (x << (32 - n));
}

// SHA-256 logical functions (as defined in the FIPS-180-4 standard)
#define CH(x,y,z)  ((x & y) ^ (~x & z))        // choose: picks y or z based on x
#define MAJ(x,y,z) ((x & y) ^ (x & z) ^ (y & z)) // majority: majority vote among x,y,z
#define BSIG0(x)   (rotr(x,2) ^ rotr(x,13) ^ rotr(x,22)) // big sigma 0
#define BSIG1(x)   (rotr(x,6) ^ rotr(x,11) ^ rotr(x,25)) // big sigma 1
#define SSIG0(x)   (rotr(x,7) ^ rotr(x,18) ^ (x >> 3))   // small sigma 0
#define SSIG1(x)   (rotr(x,17) ^ rotr(x,19) ^ (x >> 10)) // small sigma 1

// Round constants: first 32 bits of cube roots of first 64 primes 
static const u32 K[64] = {
  0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
  0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
  0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
  0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
  0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
  0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
  0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
  0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
  0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
  0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
  0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
  0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
  0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
  0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
  0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
  0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

/* Process one 512-bit (64-byte) block of input.
 * This is the "heart" of SHA-256, where the compression function runs.
 * If we mess up here it's going to break everything.
 */
static void sha256_transform(struct sha256_ctx *ctx, const u8 block[64]) {
    u32 W[64];      // message schedule array
    u32 a,b,c,d,e,f,g,h; // working variables
    u32 t1, t2;
    u32 i;

    // Step 1: Prepare the message schedule W[0..63]
    for (i = 0; i < 16; ++i) {
        // Convert 4 bytes from input block into a 32-bit word (big-endian)
        u32 j = i * 4;
        W[i] = ((u32)block[j] << 24) | ((u32)block[j+1] << 16)
             | ((u32)block[j+2] << 8) | ((u32)block[j+3]);
    }
    // Extend first 16 words into remaining 48 words
    for (i = 16; i < 64; ++i) {
        W[i] = SSIG1(W[i-2]) + W[i-7] + SSIG0(W[i-15]) + W[i-16];
    }

    // Step 2: Initialize working variables with current hash state
    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];

    // Step 3: Main compression loop (64 rounds)
    for (i = 0; i < 64; ++i) {
        t1 = h + BSIG1(e) + CH(e,f,g) + K[i] + W[i];
        t2 = BSIG0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Step 4: Add this block's hash to the cumulative state
    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
    ctx->h[5] += f;
    ctx->h[6] += g;
    ctx->h[7] += h;
}

// Initialize SHA-256 context with standard initial hash values
void sha256_init(struct sha256_ctx *ctx) {
    ctx->h[0] = 0x6a09e667u;
    ctx->h[1] = 0xbb67ae85u;
    ctx->h[2] = 0x3c6ef372u;
    ctx->h[3] = 0xa54ff53au;
    ctx->h[4] = 0x510e527fu;
    ctx->h[5] = 0x9b05688cu;
    ctx->h[6] = 0x1f83d9abu;
    ctx->h[7] = 0x5be0cd19u;

    ctx->buflen = 0;  // no data yet
    ctx->bitlen = 0;  // processed length = 0
}

// Byte copy (manual memcpy replacement)
static void memcopy_bytes(u8 *dst, const u8 *src, u32 n) {
    u32 i;
    for (i = 0; i < n; ++i) dst[i] = src[i];
}

/*
 * Process input data: can be called repeatedly.
 * Buffers input, processes full 64-byte blocks.
 */
void sha256_update(struct sha256_ctx *ctx, const u8 *data, u32 len) {
    u32 i = 0;

    ctx->bitlen += ((u64)len) * 8ull; // track total length in bits

    // If buffer already has data, try to fill it to 64 bytes
    if (ctx->buflen > 0) {
        u32 need = 64 - ctx->buflen;
        if (len < need) {
            memcopy_bytes(&ctx->buffer[ctx->buflen], data, len);
            ctx->buflen += len;
            return; // not enough to process a full block yet
        } else {
            memcopy_bytes(&ctx->buffer[ctx->buflen], data, need);
            sha256_transform(ctx, ctx->buffer); // process full buffer
            ctx->buflen = 0;
            i += need;
        }
    }

    // Process direct full 64-byte blocks from input
    // Fixed: Changed condition to prevent buffer overflow
    while (i + 64 <= len) {
        sha256_transform(ctx, &data[i]);
        i += 64;
    }

    // Copy any leftover bytes into buffer
    if (i < len) {
        u32 rem = len - i;
        memcopy_bytes(ctx->buffer, &data[i], rem);
        ctx->buflen = rem;
    }
}

/*
 * Finalize the hash computation:
 * - Apply SHA-256 padding
 * - Append message length
 * - Output final 32-byte hash
 */
void sha256_final(struct sha256_ctx *ctx, u8 out_hash32[32]) {
    u32 i;
    u64 bits = ctx->bitlen;

    // Append padding: start with 0x80
    ctx->buffer[ctx->buflen] = 0x80;
    ctx->buflen++;

    // Handle case where padding doesn't fit in current block
    if (ctx->buflen > 56) {
        while (ctx->buflen < 64) {
            ctx->buffer[ctx->buflen] = 0;
            ctx->buflen++;
        }
        sha256_transform(ctx, ctx->buffer);
        ctx->buflen = 0;
    }

    // Pad remaining space with zeros
    while (ctx->buflen < 56) {
        ctx->buffer[ctx->buflen] = 0;
        ctx->buflen++;
    }

    // Append 64-bit length in big-endian
    ctx->buffer[56] = (u8)(bits >> 56);
    ctx->buffer[57] = (u8)(bits >> 48);
    ctx->buffer[58] = (u8)(bits >> 40);
    ctx->buffer[59] = (u8)(bits >> 32);
    ctx->buffer[60] = (u8)(bits >> 24);
    ctx->buffer[61] = (u8)(bits >> 16);
    ctx->buffer[62] = (u8)(bits >> 8);
    ctx->buffer[63] = (u8)bits;

    // Process final block
    sha256_transform(ctx, ctx->buffer);

    // Output hash
    for (i = 0; i < 8; ++i) {
        u32 val = ctx->h[i];
        out_hash32[i*4 + 0] = (u8)(val >> 24);
        out_hash32[i*4 + 1] = (u8)(val >> 16);
        out_hash32[i*4 + 2] = (u8)(val >> 8);
        out_hash32[i*4 + 3] = (u8)val;
    }
}

// Convert binary digest into human-readable hex string
void sha256_to_hex(const u8 hash32[32], char hex_out[65]) {
    static const char hexchars[] = "0123456789abcdef";
    u32 i;
    for (i = 0; i < 32; ++i) {
        u8 b = hash32[i];
        hex_out[i*2 + 0] = hexchars[(b >> 4) & 0xF]; // high nibble
        hex_out[i*2 + 1] = hexchars[b & 0xF];        // low nibble
    }
    hex_out[64] = '\0'; // null-terminate
}