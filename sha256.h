/* sha256.h
 *
 * Typical usage:
 *   struct sha256_ctx ctx;
 *   sha256_init(&ctx);                        // Initialize context
 *   sha256_update(&ctx, data_ptr, data_len);  // Process input data
 *   sha256_final(&ctx, out32);                // Produce final 32-byte hash
 *
 */

#ifndef SHA256_H
#define SHA256_H

/*
 * Minimal fixed-size integer types.
 * Since we can’t rely on <stdint.h> in bare-metal,
 * we typedef our own. Adjust sizes if your compiler/CPU
 * has different native widths.
 */
typedef unsigned char        u8;   //  8-bit unsigned integer
typedef unsigned int         u32;  // 32-bit unsigned integer
typedef unsigned long long   u64;  // 64-bit unsigned integer

/*
 * SHA-256 context structure
 * Holds the working state while hashing data incrementally.
 */
struct sha256_ctx {
    u32 h[8];          //Current hash state (8 words = 256 bits)
    u8  buffer[64];    // Data block buffer (512 bits per SHA-256 block)
    u32 buflen;        // Number of bytes currently in buffer (0..63)
    u64 bitlen;        //Total number of bits processed (used for padding)
};

/*
 * Public API functions
 *
 * sha256_init()
 * Initialize the SHA-256 context with standard constants.
 * Must be called before update/final.
 */
void sha256_init(struct sha256_ctx *ctx);

/* sha256_update()
 * Feed data into the hashing context.
 * Can be called multiple times with successive chunks.
 *
 * ctx  - hashing context
 * data - pointer to input bytes
 * len  - number of bytes to process
 */
void sha256_update(struct sha256_ctx *ctx, const u8 *data, u32 len);

/* sha256_final()
 * Finish hashing: add padding, process final block,
 * and write the 32-byte digest to out_hash32.
 *
 * ctx        - hashing context
 * out_hash32 - must point to at least 32 bytes
 */
void sha256_final(struct sha256_ctx *ctx, u8 out_hash32[32]);

/* sha256_to_hex()
 * Convert raw 32-byte binary hash to human-readable hex string.
 * hex_out must be at least 65 bytes (64 hex chars + null terminator).
 */
void sha256_to_hex(const u8 hash32[32], char hex_out[65]);

#endif 
