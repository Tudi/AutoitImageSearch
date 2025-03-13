#include "stdafx.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>

static void hadamard_1d_8_avx2(__m256i v[8]);
static void transpose_8x8_epi16_avx2(__m256i v[8]);

size_t satd_8x8_channel_scalar(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst, size_t channel) {
    int16_t diff[8][8];

    // Step 1: Compute difference block for the selected channel
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            size_t src_idx = y * stride_src + x * 4 + channel;
            size_t ref_idx = y * stride_dst + x * 4 + channel;
            diff[y][x] = (int16_t)src[src_idx] - (int16_t)ref[ref_idx];
        }
    }

    // Step 2: Horizontal Hadamard Transform (per row)
    int16_t m[8][8];
    for (size_t y = 0; y < 8; ++y) {
        int16_t* d = diff[y];
        int16_t s07 = d[0] + d[7];
        int16_t s16 = d[1] + d[6];
        int16_t s25 = d[2] + d[5];
        int16_t s34 = d[3] + d[4];
        int16_t d07 = d[0] - d[7];
        int16_t d16 = d[1] - d[6];
        int16_t d25 = d[2] - d[5];
        int16_t d34 = d[3] - d[4];

        int16_t a0 = s07 + s34;
        int16_t a1 = s16 + s25;
        int16_t a2 = s16 - s25;
        int16_t a3 = s07 - s34;
        int16_t a4 = d34 + d07;
        int16_t a5 = d25 + d16;
        int16_t a6 = d25 - d16;
        int16_t a7 = d34 - d07;

        m[y][0] = a0 + a1;
        m[y][1] = a3 + a2;
        m[y][2] = a3 - a2;
        m[y][3] = a0 - a1;
        m[y][4] = a4 + a5;
        m[y][5] = a7 + a6;
        m[y][6] = a7 - a6;
        m[y][7] = a4 - a5;
    }

    // Step 3: Vertical Hadamard Transform (per column)
    int16_t t[8][8];
    for (size_t x = 0; x < 8; ++x) {
        int16_t* m_col = (int16_t*)&m[0][x];
        int16_t s07 = m_col[0 * 8] + m_col[7 * 8];
        int16_t s16 = m_col[1 * 8] + m_col[6 * 8];
        int16_t s25 = m_col[2 * 8] + m_col[5 * 8];
        int16_t s34 = m_col[3 * 8] + m_col[4 * 8];
        int16_t d07 = m_col[0 * 8] - m_col[7 * 8];
        int16_t d16 = m_col[1 * 8] - m_col[6 * 8];
        int16_t d25 = m_col[2 * 8] - m_col[5 * 8];
        int16_t d34 = m_col[3 * 8] - m_col[4 * 8];

        int16_t a0 = s07 + s34;
        int16_t a1 = s16 + s25;
        int16_t a2 = s16 - s25;
        int16_t a3 = s07 - s34;
        int16_t a4 = d34 + d07;
        int16_t a5 = d25 + d16;
        int16_t a6 = d25 - d16;
        int16_t a7 = d34 - d07;

        t[0][x] = a0 + a1;
        t[1][x] = a3 + a2;
        t[2][x] = a3 - a2;
        t[3][x] = a0 - a1;
        t[4][x] = a4 + a5;
        t[5][x] = a7 + a6;
        t[6][x] = a7 - a6;
        t[7][x] = a4 - a5;
    }

    // Step 4: Sum absolute transform coefficients
    size_t satd = 0;
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            satd += abs(t[y][x]);
        }
    }

    // Step 5: Normalize (divide by 4)
    return (satd + 2) >> 2;
}

size_t satd_8x8_channel_(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst, size_t channel) {
#define _USE_CPU_
//#define _USE_AVX2_ // I think the hadamard transform on 8x8 does not give the expected output registers will have 16 values instead of 8 expected ?
#ifdef _USE_CPU_
    int16_t diff[8+1][8]; // extra storage to allow store to not cause stack corruption
#endif
#ifdef _USE_AVX2_
    __m256i v[8];
#endif

    const __m256i shuf_mask = _mm256_setr_epi8(
        0, 0x80, 4, 0x80,
        8, 0x80, 12, 0x80,
        0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80,
        0, 0x80, 4, 0x80,
        8, 0x80, 12, 0x80,
        0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80
    );

    // Step 1: Compute difference block for the selected channel
    src += channel;
    ref += channel;

    for (size_t y = 0; y < 8; ++y) {
        // Load 32 bytes (8 pixels * 4 channels) from src and ref
        __m256i src_row = _mm256_loadu_si256((const __m256i*)(src));
        __m256i ref_row = _mm256_loadu_si256((const __m256i*)(ref));

        // Shuffle to extract the desired channel into i16 values
        __m256i src_shuf = _mm256_shuffle_epi8(src_row, shuf_mask);
        __m256i ref_shuf = _mm256_shuffle_epi8(ref_row, shuf_mask);

#ifdef _USE_CPU_
        // Subtract src - ref
        __m256i diff_s16 = _mm256_sub_epi16(src_shuf, ref_shuf);

        // Store diff[y][x]
        __m128i diff_lo = _mm256_castsi256_si128(diff_s16);
        __m128i diff_hi = _mm256_extracti128_si256(diff_s16, 1);

        _mm_storeu_si128((__m128i*) & diff[y][0], diff_lo);
        _mm_storeu_si128((__m128i*) & diff[y][4], diff_hi);
#endif
#ifdef _USE_AVX2_
        v[y] = _mm256_sub_epi16(src_shuf, ref_shuf);
#endif
        // Advance to next row
        src += stride_src;
        ref += stride_dst;
    }

#ifdef _USE_CPU_
    // Step 2: Horizontal Hadamard Transform (per row)
    int16_t m[8][8];
    for (size_t y = 0; y < 8; ++y) {
        int16_t* d = diff[y];
        int16_t s07 = d[0] + d[7];
        int16_t s16 = d[1] + d[6];
        int16_t s25 = d[2] + d[5];
        int16_t s34 = d[3] + d[4];
        int16_t d07 = d[0] - d[7];
        int16_t d16 = d[1] - d[6];
        int16_t d25 = d[2] - d[5];
        int16_t d34 = d[3] - d[4];

        int16_t a0 = s07 + s34;
        int16_t a1 = s16 + s25;
        int16_t a2 = s16 - s25;
        int16_t a3 = s07 - s34;
        int16_t a4 = d34 + d07;
        int16_t a5 = d25 + d16;
        int16_t a6 = d25 - d16;
        int16_t a7 = d34 - d07;

        m[y][0] = a0 + a1;
        m[y][1] = a3 + a2;
        m[y][2] = a3 - a2;
        m[y][3] = a0 - a1;
        m[y][4] = a4 + a5;
        m[y][5] = a7 + a6;
        m[y][6] = a7 - a6;
        m[y][7] = a4 - a5;
    }

    // Step 3: Vertical Hadamard Transform (per column)
    int16_t t[8][8];
    for (size_t x = 0; x < 8; ++x) {
        int16_t* m_col = (int16_t*)&m[0][x];
        int16_t s07 = m_col[0 * 8] + m_col[7 * 8];
        int16_t s16 = m_col[1 * 8] + m_col[6 * 8];
        int16_t s25 = m_col[2 * 8] + m_col[5 * 8];
        int16_t s34 = m_col[3 * 8] + m_col[4 * 8];
        int16_t d07 = m_col[0 * 8] - m_col[7 * 8];
        int16_t d16 = m_col[1 * 8] - m_col[6 * 8];
        int16_t d25 = m_col[2 * 8] - m_col[5 * 8];
        int16_t d34 = m_col[3 * 8] - m_col[4 * 8];

        int16_t a0 = s07 + s34;
        int16_t a1 = s16 + s25;
        int16_t a2 = s16 - s25;
        int16_t a3 = s07 - s34;
        int16_t a4 = d34 + d07;
        int16_t a5 = d25 + d16;
        int16_t a6 = d25 - d16;
        int16_t a7 = d34 - d07;

        t[0][x] = a0 + a1;
        t[1][x] = a3 + a2;
        t[2][x] = a3 - a2;
        t[3][x] = a0 - a1;
        t[4][x] = a4 + a5;
        t[5][x] = a7 + a6;
        t[6][x] = a7 - a6;
        t[7][x] = a4 - a5;
    }
#endif
#ifdef _USE_AVX2_
    // Horizontal Hadamard
    hadamard_1d_8_avx2(v);
    transpose_8x8_epi16_avx2(v);
    // Vertical Hadamard
    hadamard_1d_8_avx2(v);
#endif

#ifdef _USE_CPU_
    // Step 4: Sum absolute transform coefficients
    size_t satd = 0;
    __m128i sum = _mm_setzero_si128();

    for (size_t y = 0; y < 8; ++y) {
        // Load 8 int16_t values into 128-bit vector
        __m128i row = _mm_loadu_si128((__m128i*)t[y]);

        // Absolute values
        __m128i abs_row = _mm_abs_epi16(row);

        // Accumulate
        sum = _mm_add_epi16(sum, abs_row);
    }

    // Now we have 8 16-bit sums in `sum`.
    // We need to horizontally add them.

    __m128i zero = _mm_setzero_si128();
    // Unpack to 32-bit ints to avoid overflow
    __m128i sum_lo = _mm_unpacklo_epi16(sum, zero);
    __m128i sum_hi = _mm_unpackhi_epi16(sum, zero);

    // Add low and high parts
    __m128i sum32 = _mm_add_epi32(sum_lo, sum_hi);

    // Horizontal add remaining elements
    sum32 = _mm_hadd_epi32(sum32, sum32);
    sum32 = _mm_hadd_epi32(sum32, sum32);

    satd = _mm_cvtsi128_si32(sum32);
#else
    // Sum absolute values
    __m256i sum_lo = _mm256_setzero_si256();
    __m256i sum_hi = _mm256_setzero_si256();

    for (int i = 0; i < 8; ++i) {
        __m256i abs_v = _mm256_abs_epi16(v[i]);

        __m256i lo = _mm256_unpacklo_epi16(abs_v, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi16(abs_v, _mm256_setzero_si256());

        sum_lo = _mm256_add_epi32(sum_lo, lo);
        sum_hi = _mm256_add_epi32(sum_hi, hi);
    }

    __m256i total = _mm256_add_epi32(sum_lo, sum_hi);

    // Horizontal sum across lanes
    __m128i total_lo = _mm256_castsi256_si128(total);
    __m128i total_hi = _mm256_extracti128_si256(total, 1);
    __m128i sum128 = _mm_add_epi32(total_lo, total_hi);

    sum128 = _mm_hadd_epi32(sum128, sum128);
    sum128 = _mm_hadd_epi32(sum128, sum128);

    size_t satd = _mm_cvtsi128_si32(sum128);
#endif

    // Step 5: Normalize (divide by 4)
    return (satd + 2) >> 2;
}

// Complete SATD for RGBA block 8x8
size_t satd_8x8_rgb_scalar(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst) {
    size_t satdR = satd_8x8_channel_(src, ref, stride_src, stride_dst, 0);
    size_t satdG = satd_8x8_channel_(src, ref, stride_src, stride_dst, 1);
    size_t satdB = satd_8x8_channel_(src, ref, stride_src, stride_dst, 2);
//    uint32_t satdA = satd_8x8_channel_scalar(src, ref, stride, 3);

    return satdR + satdG + satdB;
}

#include <smmintrin.h>  // SSE4.1 intrinsics (includes SSE2/SSSE3 as needed)
#include <stdint.h>
#include <cmath>        // for std::abs (if needed)

// Compute SATD for a 4x4 block of RGBA pixels using SSE4.1 intrinsics
uint32_t satd_4x4_RGBA_SSE41(const uint8_t* src, const uint8_t* ref, int stride) {
    // Intermediate difference values for each channel (16-bit to allow negatives)
    alignas(16) int16_t diffR[4][4], diffG[4][4], diffB[4][4], diffA[4][4];

    // 1. Load pixels and compute differences for each of the 4 rows
    for (int y = 0; y < 4; ++y) {
        // Load 4 pixels (16 bytes) from each image block row
        __m128i srcVec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + y * stride));
        __m128i refVec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ref + y * stride));
        // Convert 8-bit to 16-bit (zero-extend) to prepare for subtraction
        __m128i srcLow = _mm_cvtepu8_epi16(srcVec);               // lower 8 bytes -> 8 words
        __m128i srcHigh = _mm_cvtepu8_epi16(_mm_srli_si128(srcVec, 8));  // upper 8 bytes
        __m128i refLow = _mm_cvtepu8_epi16(refVec);
        __m128i refHigh = _mm_cvtepu8_epi16(_mm_srli_si128(refVec, 8));
        // Compute 16-bit differences in parallel
        __m128i diffLow = _mm_sub_epi16(srcLow, refLow);
        __m128i diffHigh = _mm_sub_epi16(srcHigh, refHigh);
        // Store differences into arrays, separating channels
        alignas(16) int16_t diffTemp[8];
        _mm_store_si128(reinterpret_cast<__m128i*>(diffTemp), diffLow);
        // diffTemp now has 8 values: R0,G0,B0,A0, R1,G1,B1,A1 (for pixels 0 and 1 of the row)
        diffR[y][0] = diffTemp[0];  diffG[y][0] = diffTemp[1];
        diffB[y][0] = diffTemp[2];  diffA[y][0] = diffTemp[3];
        diffR[y][1] = diffTemp[4];  diffG[y][1] = diffTemp[5];
        diffB[y][1] = diffTemp[6];  diffA[y][1] = diffTemp[7];
        _mm_store_si128(reinterpret_cast<__m128i*>(diffTemp), diffHigh);
        // diffTemp now has differences for pixels 2 and 3: R2,G2,B2,A2, R3,G3,B3,A3
        diffR[y][2] = diffTemp[0];  diffG[y][2] = diffTemp[1];
        diffB[y][2] = diffTemp[2];  diffA[y][2] = diffTemp[3];
        diffR[y][3] = diffTemp[4];  diffG[y][3] = diffTemp[5];
        diffB[y][3] = diffTemp[6];  diffA[y][3] = diffTemp[7];
    }

    // 2. Hadamard transform on the 4x4 difference block for each channel
    unsigned satdValue = 0;
    for (int c = 0; c < 4; ++c) {
        // Select channel diff matrix
        int16_t(*diff)[4];
        switch (c) {
        case 0: diff = diffR; break;  // Red channel
        case 1: diff = diffG; break;  // Green channel
        case 2: diff = diffB; break;  // Blue channel
        case 3: diff = diffA; break;  // Alpha channel
        }

        // Horizontal Hadamard transform for each row
        int16_t H[4][4];
        for (int y = 0; y < 4; ++y) {
            int16_t d0 = diff[y][0], d1 = diff[y][1];
            int16_t d2 = diff[y][2], d3 = diff[y][3];
            // First stage: pairwise addition and subtraction
            int16_t a0 = d0 + d3;
            int16_t a1 = d1 + d2;
            int16_t a2 = d1 - d2;
            int16_t a3 = d0 - d3;
            // Second stage: combine results for Hadamard
            H[y][0] = a0 + a1;
            H[y][1] = a2 + a3;
            H[y][2] = a0 - a1;
            H[y][3] = a3 - a2;  // (or a2 - a3, which is just negative of H[y][1] – absolute value will be same)
        }

        // Vertical Hadamard transform on the columns of H
        int16_t V[4][4];
        for (int x = 0; x < 4; ++x) {
            int16_t h0 = H[0][x], h1 = H[1][x];
            int16_t h2 = H[2][x], h3 = H[3][x];
            int16_t b0 = h0 + h3;
            int16_t b1 = h1 + h2;
            int16_t b2 = h1 - h2;
            int16_t b3 = h0 - h3;
            V[0][x] = b0 + b1;
            V[1][x] = b2 + b3;
            V[2][x] = b0 - b1;
            V[3][x] = b3 - b2;
        }

        // 3. Sum the absolute values of the transform coefficients for this channel
        __m128i sumVec = _mm_setzero_si128();
        // Process 8 coefficients at a time using SSE
        for (int i = 0; i < 4; i += 2) {
            // Load two rows of coefficients (8 values)
            __m128i coeffs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(V[i] + 0));
            // Compute absolute values of 16-bit coefficients
            __m128i absCoeffs = _mm_abs_epi16(coeffs);
            // Accumulate into sumVec (packed 16-bit addition with saturation won't overflow here)
            sumVec = _mm_adds_epi16(sumVec, absCoeffs);
        }
        // Now sumVec contains 8 partial sums (each up to 4 values summed). We need a total sum.
        // Use horizontal adds to sum these 8 values down to one value.
        __m128i sum4 = _mm_hadd_epi16(sumVec, sumVec);    // sum adjacent pairs -> 4 values
        __m128i sum2 = _mm_hadd_epi16(sum4, sum4);        // sum adjacent again -> 2 values
        __m128i sum1 = _mm_hadd_epi16(sum2, sum2);        // sum again -> 1 value (in lower 16 bits)
        unsigned channelSATD = static_cast<unsigned>(_mm_extract_epi16(sum1, 0));
        satdValue += channelSATD;
    }

    return satdValue;
}

uint32_t satd_8x8_RGBA_SSE41(const uint8_t* src, const uint8_t* ref, int stride) {
    uint32_t satd = 0;

    // Top-left 4x4 block (rows 0-3, cols 0-3)
    satd += satd_4x4_RGBA_SSE41(src, ref, stride);

    // Top-right 4x4 block (rows 0-3, cols 4-7)
    satd += satd_4x4_RGBA_SSE41(src + 4 * 4, ref + 4 * 4, stride);

    // Bottom-left 4x4 block (rows 4-7, cols 0-3)
    satd += satd_4x4_RGBA_SSE41(src + 4 * stride, ref + 4 * stride, stride);

    // Bottom-right 4x4 block (rows 4-7, cols 4-7)
    satd += satd_4x4_RGBA_SSE41(src + 4 * stride + 4 * 4, ref + 4 * stride + 4 * 4, stride);

    return satd;
}

#include <stdint.h>
#include <emmintrin.h>   // SSE2
#include <tmmintrin.h>   // SSSE3 (for abs)

static void load_rgba_channel_8x8_sse(const uint8_t* src, const uint8_t* ref, int stride, char channel, __m128i out[8]) {
    const __m128i shuf_mask = _mm_set_epi8(
        channel + 28, channel + 24, channel + 20, channel + 16,
        channel + 12, channel + 8, channel + 4, channel + 0,
        channel + 28, channel + 24, channel + 20, channel + 16,
        channel + 12, channel + 8, channel + 4, channel + 0
    );

    for (int y = 0; y < 8; ++y) {
        const uint8_t* src_row = src + y * stride;
        const uint8_t* ref_row = ref + y * stride;

        // Load full 32 bytes from src and ref
        __m128i src0 = _mm_loadu_si128((const __m128i*)(src_row));
        __m128i src1 = _mm_loadu_si128((const __m128i*)(src_row + 16));
        __m128i ref0 = _mm_loadu_si128((const __m128i*)(ref_row));
        __m128i ref1 = _mm_loadu_si128((const __m128i*)(ref_row + 16));

        // Combine lower/upper parts (32 bytes) into 16 lanes (128 bits)
        __m128i src_combined = _mm_alignr_epi8(src1, src0, 0);
        __m128i ref_combined = _mm_alignr_epi8(ref1, ref0, 0);

        // Gather channel bytes using shuffle
        __m128i src_ch = _mm_shuffle_epi8(src_combined, shuf_mask);
        __m128i ref_ch = _mm_shuffle_epi8(ref_combined, shuf_mask);

        // Convert to 16-bit
        __m128i src16 = _mm_unpacklo_epi8(src_ch, _mm_setzero_si128());
        __m128i ref16 = _mm_unpacklo_epi8(ref_ch, _mm_setzero_si128());

        // Subtract src - ref
        out[y] = _mm_sub_epi16(src16, ref16);
    }
}

// Horizontal 1D Hadamard Transform (in-place on 8 SSE vectors)
static void hadamard_1d_8_sse(__m128i v[8]) {
    __m128i a0 = _mm_add_epi16(v[0], v[7]);
    __m128i a1 = _mm_add_epi16(v[1], v[6]);
    __m128i a2 = _mm_add_epi16(v[2], v[5]);
    __m128i a3 = _mm_add_epi16(v[3], v[4]);
    __m128i a4 = _mm_sub_epi16(v[3], v[4]);
    __m128i a5 = _mm_sub_epi16(v[2], v[5]);
    __m128i a6 = _mm_sub_epi16(v[1], v[6]);
    __m128i a7 = _mm_sub_epi16(v[0], v[7]);

    __m128i b0 = _mm_add_epi16(a0, a3);
    __m128i b1 = _mm_add_epi16(a1, a2);
    __m128i b2 = _mm_sub_epi16(a1, a2);
    __m128i b3 = _mm_sub_epi16(a0, a3);
    __m128i b4 = _mm_add_epi16(a4, a7);
    __m128i b5 = _mm_add_epi16(a5, a6);
    __m128i b6 = _mm_sub_epi16(a5, a6);
    __m128i b7 = _mm_sub_epi16(a4, a7);

    v[0] = _mm_add_epi16(b0, b1);
    v[1] = _mm_add_epi16(b3, b2);
    v[2] = _mm_sub_epi16(b3, b2);
    v[3] = _mm_sub_epi16(b0, b1);
    v[4] = _mm_add_epi16(b4, b5);
    v[5] = _mm_add_epi16(b7, b6);
    v[6] = _mm_sub_epi16(b7, b6);
    v[7] = _mm_sub_epi16(b4, b5);
}

// Transpose an 8x8 matrix of int16_t in 8 __m128i registers
static void transpose_8x8_epi16_sse(__m128i v[8]) {
    __m128i t0 = _mm_unpacklo_epi16(v[0], v[1]);
    __m128i t1 = _mm_unpackhi_epi16(v[0], v[1]);
    __m128i t2 = _mm_unpacklo_epi16(v[2], v[3]);
    __m128i t3 = _mm_unpackhi_epi16(v[2], v[3]);
    __m128i t4 = _mm_unpacklo_epi16(v[4], v[5]);
    __m128i t5 = _mm_unpackhi_epi16(v[4], v[5]);
    __m128i t6 = _mm_unpacklo_epi16(v[6], v[7]);
    __m128i t7 = _mm_unpackhi_epi16(v[6], v[7]);

    __m128i tt0 = _mm_unpacklo_epi32(t0, t2);
    __m128i tt1 = _mm_unpackhi_epi32(t0, t2);
    __m128i tt2 = _mm_unpacklo_epi32(t1, t3);
    __m128i tt3 = _mm_unpackhi_epi32(t1, t3);
    __m128i tt4 = _mm_unpacklo_epi32(t4, t6);
    __m128i tt5 = _mm_unpackhi_epi32(t4, t6);
    __m128i tt6 = _mm_unpacklo_epi32(t5, t7);
    __m128i tt7 = _mm_unpackhi_epi32(t5, t7);

    v[0] = _mm_unpacklo_epi64(tt0, tt4);
    v[1] = _mm_unpackhi_epi64(tt0, tt4);
    v[2] = _mm_unpacklo_epi64(tt1, tt5);
    v[3] = _mm_unpackhi_epi64(tt1, tt5);
    v[4] = _mm_unpacklo_epi64(tt2, tt6);
    v[5] = _mm_unpackhi_epi64(tt2, tt6);
    v[6] = _mm_unpacklo_epi64(tt3, tt7);
    v[7] = _mm_unpackhi_epi64(tt3, tt7);
}

// Compute SATD on a single channel (8x8 block)
static uint32_t satd_8x8_channel_sse(const uint8_t* src, const uint8_t* ref, int stride, char channel) {
    __m128i v[8];

    // Step 1: Load diff data
    load_rgba_channel_8x8_sse(src, ref, stride, channel, v);

    // Step 2: Horizontal Hadamard
    hadamard_1d_8_sse(v);

    // Step 3: Transpose
    transpose_8x8_epi16_sse(v);

    // Step 4: Vertical Hadamard
    hadamard_1d_8_sse(v);

    // Step 5: Sum absolute values
    __m128i sum = _mm_setzero_si128();
    for (int i = 0; i < 8; ++i) {
        __m128i abs_v = _mm_abs_epi16(v[i]);
        sum = _mm_add_epi16(sum, abs_v);
    }

    // Step 6: Widen to 32-bit, sum horizontally
    __m128i zero = _mm_setzero_si128();
    __m128i lo = _mm_unpacklo_epi16(sum, zero);
    __m128i hi = _mm_unpackhi_epi16(sum, zero);
    __m128i total = _mm_add_epi32(lo, hi);

    __m128i sum64 = _mm_add_epi32(total, _mm_shuffle_epi32(total, _MM_SHUFFLE(1, 0, 3, 2)));
    __m128i sum32 = _mm_add_epi32(sum64, _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1)));

    uint32_t satd = _mm_cvtsi128_si32(sum32);

    // Step 7: Normalize (divide by 4)
    return (satd + 2) >> 2;
}

// Main SATD for RGBA 8x8 block
uint32_t satd_8x8_rgba_sse(const uint8_t* src, const uint8_t* ref, int stride) {
    uint32_t satdR = satd_8x8_channel_sse(src, ref, stride, 0);
    uint32_t satdG = satd_8x8_channel_sse(src, ref, stride, 1);
    uint32_t satdB = satd_8x8_channel_sse(src, ref, stride, 2);
//    uint32_t satdA = satd_8x8_channel_sse(src, ref, stride, 3);

    return satdR + satdG + satdB;
}

#include <immintrin.h>
#include <stdint.h>

// Load 8x8 block and split RGBA channels
static inline void load_rgba_8x8(
    const uint8_t* src, const uint8_t* ref, int stride,
    __m512i r[8], __m512i g[8], __m512i b[8], __m512i a[8]
) {
    for (int y = 0; y < 8; ++y) {
        const __m256i src_pix = _mm256_loadu_si256((const __m256i*)(src + y * stride));
        const __m256i ref_pix = _mm256_loadu_si256((const __m256i*)(ref + y * stride));

        __m512i src16 = _mm512_cvtepu8_epi16(src_pix);
        __m512i ref16 = _mm512_cvtepu8_epi16(ref_pix);

        __m512i diff16 = _mm512_sub_epi16(src16, ref16);

        // RGBA interleaved -> Extract channels
        __m512i shuf_mask = _mm512_set_epi16(
            62, 58, 54, 50, 46, 42, 38, 34,
            30, 26, 22, 18, 14, 10, 6, 2,
            60, 56, 52, 48, 44, 40, 36, 32,
            28, 24, 20, 16, 12, 8, 4, 0
        );

        __m512i permuted = _mm512_permutexvar_epi16(shuf_mask, diff16);

        r[y] = _mm512_maskz_compress_epi16(0x11111111, permuted);
        g[y] = _mm512_maskz_compress_epi16(0x22222222, permuted);
        b[y] = _mm512_maskz_compress_epi16(0x44444444, permuted);
        a[y] = _mm512_maskz_compress_epi16(0x88888888, permuted);
    }
}

// Vectorized Hadamard 8x8 transform (row + column), returns SATD
static uint32_t hadamard_satd_8x8_avx512(const __m512i rows[8]) {
    __m512i tmp[8];

    // Step 1: Horizontal Hadamard transform (row-wise)
    for (int i = 0; i < 8; ++i) {
        __m512i x0 = _mm512_add_epi16(rows[i], _mm512_shuffle_epi32(rows[i], _MM_PERM_BADC));
        __m512i x1 = _mm512_sub_epi16(rows[i], _mm512_shuffle_epi32(rows[i], _MM_PERM_BADC));

        tmp[i] = _mm512_unpacklo_epi16(x0, x1); // interleave
    }

    // Step 2: Transpose the 8x8 matrix (in registers)
    __m512i t0 = _mm512_unpacklo_epi16(tmp[0], tmp[1]);
    __m512i t1 = _mm512_unpackhi_epi16(tmp[0], tmp[1]);
    __m512i t2 = _mm512_unpacklo_epi16(tmp[2], tmp[3]);
    __m512i t3 = _mm512_unpackhi_epi16(tmp[2], tmp[3]);
    __m512i t4 = _mm512_unpacklo_epi16(tmp[4], tmp[5]);
    __m512i t5 = _mm512_unpackhi_epi16(tmp[4], tmp[5]);
    __m512i t6 = _mm512_unpacklo_epi16(tmp[6], tmp[7]);
    __m512i t7 = _mm512_unpackhi_epi16(tmp[6], tmp[7]);

    __m512i tt0 = _mm512_shuffle_i32x4(t0, t2, 0x44);
    __m512i tt1 = _mm512_shuffle_i32x4(t1, t3, 0x44);
    __m512i tt2 = _mm512_shuffle_i32x4(t0, t2, 0xEE);
    __m512i tt3 = _mm512_shuffle_i32x4(t1, t3, 0xEE);
    __m512i tt4 = _mm512_shuffle_i32x4(t4, t6, 0x44);
    __m512i tt5 = _mm512_shuffle_i32x4(t5, t7, 0x44);
    __m512i tt6 = _mm512_shuffle_i32x4(t4, t6, 0xEE);
    __m512i tt7 = _mm512_shuffle_i32x4(t5, t7, 0xEE);

    tmp[0] = _mm512_shuffle_i64x2(tt0, tt4, 0x88);
    tmp[1] = _mm512_shuffle_i64x2(tt1, tt5, 0x88);
    tmp[2] = _mm512_shuffle_i64x2(tt2, tt6, 0x88);
    tmp[3] = _mm512_shuffle_i64x2(tt3, tt7, 0x88);
    tmp[4] = _mm512_shuffle_i64x2(tt0, tt4, 0xDD);
    tmp[5] = _mm512_shuffle_i64x2(tt1, tt5, 0xDD);
    tmp[6] = _mm512_shuffle_i64x2(tt2, tt6, 0xDD);
    tmp[7] = _mm512_shuffle_i64x2(tt3, tt7, 0xDD);

    // Step 3: Vertical Hadamard transform (column-wise)
    for (int i = 0; i < 8; ++i) {
        __m512i x0 = _mm512_add_epi16(tmp[i], _mm512_shuffle_epi32(tmp[i], _MM_PERM_BADC));
        __m512i x1 = _mm512_sub_epi16(tmp[i], _mm512_shuffle_epi32(tmp[i], _MM_PERM_BADC));

        tmp[i] = _mm512_unpacklo_epi16(x0, x1);
    }

    // Step 4: Absolute values of the transform coefficients
    for (int i = 0; i < 8; ++i) {
        tmp[i] = _mm512_abs_epi16(tmp[i]);
    }

    // Step 5: Sum across all rows
    __m512i sum = _mm512_setzero_si512();
    for (int i = 0; i < 8; ++i)
        sum = _mm512_add_epi16(sum, tmp[i]);

    // Step 6: Convert to 32-bit to prevent overflow and accumulate
    __m512i zero = _mm512_setzero_si512();
    __m512i sum_lo = _mm512_unpacklo_epi16(sum, zero);
    __m512i sum_hi = _mm512_unpackhi_epi16(sum, zero);
    __m512i total32 = _mm512_add_epi32(sum_lo, sum_hi);

    // Step 7: Horizontal reduction into scalar
    __m256i total_lo256 = _mm512_castsi512_si256(total32);
    __m256i total_hi256 = _mm512_extracti64x4_epi64(total32, 1);
    __m256i sum256 = _mm256_add_epi32(total_lo256, total_hi256);

    __m128i sum128_lo = _mm256_castsi256_si128(sum256);
    __m128i sum128_hi = _mm256_extracti128_si256(sum256, 1);
    __m128i sum128 = _mm_add_epi32(sum128_lo, sum128_hi);

    __m128i sum64 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2)));
    __m128i sum32 = _mm_add_epi32(sum64, _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1)));

    uint32_t satd = _mm_cvtsi128_si32(sum32);

    // Normalize SATD result
    return (satd + 1) >> 1;
}

// Full SATD computation for 8x8 RGBA block
uint32_t satd_8x8_rgba_avx512(const uint8_t* src, const uint8_t* ref, int stride) {
    __m512i r[8], g[8], b[8], a[8];

    // Load data and split channels
    load_rgba_8x8(src, ref, stride, r, g, b, a);

    // Hadamard transform and SATD for each channel
    uint32_t satdR = hadamard_satd_8x8_avx512(r);
    uint32_t satdG = hadamard_satd_8x8_avx512(g);
    uint32_t satdB = hadamard_satd_8x8_avx512(b);
//    uint32_t satdA = hadamard_satd_8x8_avx512(a);

    return satdR + satdG + satdB;
}


#include <immintrin.h>
#include <stdint.h>

// === Step 1: Load Channel ===
// src/ref are RGBA buffers with 8x8 blocks (stride = bytes per row, typically 32)
// channel: 0=R, 1=G, 2=B, 3=A
static void load_rgba_channel_8x8_avx2(const uint8_t* src, const uint8_t* ref, int stride, char channel, __m256i out[8]) {
    // Shuffle mask to extract one channel from 8 RGBA pixels (32 bytes)
    const __m128i shuffle_mask = _mm_set_epi8(
        channel + 28, channel + 24, channel + 20, channel + 16,
        channel + 12, channel + 8, channel + 4, channel + 0,
        channel + 28, channel + 24, channel + 20, channel + 16,
        channel + 12, channel + 8, channel + 4, channel + 0
    );

    for (int y = 0; y < 8; ++y) {
        const uint8_t* src_row = src + y * stride;
        const uint8_t* ref_row = ref + y * stride;

        __m128i src_lo = _mm_loadu_si128((const __m128i*)src_row);
        __m128i ref_lo = _mm_loadu_si128((const __m128i*)ref_row);

        __m128i src_ch = _mm_shuffle_epi8(src_lo, shuffle_mask);
        __m128i ref_ch = _mm_shuffle_epi8(ref_lo, shuffle_mask);

        __m128i src16 = _mm_unpacklo_epi8(src_ch, _mm_setzero_si128());
        __m128i ref16 = _mm_unpacklo_epi8(ref_ch, _mm_setzero_si128());

        out[y] = _mm256_set_m128i(_mm_setzero_si128(), _mm_sub_epi16(src16, ref16));
    }
}

// === Step 2: Horizontal Hadamard ===
static void hadamard_1d_8_avx2(__m256i v[8]) {
    __m256i s07 = _mm256_add_epi16(v[0], v[7]);
    __m256i s16 = _mm256_add_epi16(v[1], v[6]);
    __m256i s25 = _mm256_add_epi16(v[2], v[5]);
    __m256i s34 = _mm256_add_epi16(v[3], v[4]);
    __m256i d07 = _mm256_sub_epi16(v[0], v[7]);
    __m256i d16 = _mm256_sub_epi16(v[1], v[6]);
    __m256i d25 = _mm256_sub_epi16(v[2], v[5]);
    __m256i d34 = _mm256_sub_epi16(v[3], v[4]);

    __m256i b0 = _mm256_add_epi16(s07, s34);
    __m256i b1 = _mm256_add_epi16(s16, s25);
    __m256i b2 = _mm256_sub_epi16(s16, s25);
    __m256i b3 = _mm256_sub_epi16(s07, s34);
    __m256i b4 = _mm256_add_epi16(d34, d07);
    __m256i b5 = _mm256_add_epi16(d25, d16);
    __m256i b6 = _mm256_sub_epi16(d25, d16);
    __m256i b7 = _mm256_sub_epi16(d34, d07);

    v[0] = _mm256_add_epi16(b0, b1);
    v[1] = _mm256_add_epi16(b3, b2);
    v[2] = _mm256_sub_epi16(b3, b2);
    v[3] = _mm256_sub_epi16(b0, b1);
    v[4] = _mm256_add_epi16(b4, b5);
    v[5] = _mm256_add_epi16(b7, b6);
    v[6] = _mm256_sub_epi16(b7, b6);
    v[7] = _mm256_sub_epi16(b4, b5);
}

// === Step 3: Transpose 8x8 Matrix of 16-bit Elements ===
static void transpose_8x8_epi16_avx2(__m256i v[8]) {
    __m256i t0 = _mm256_unpacklo_epi16(v[0], v[1]);
    __m256i t1 = _mm256_unpackhi_epi16(v[0], v[1]);
    __m256i t2 = _mm256_unpacklo_epi16(v[2], v[3]);
    __m256i t3 = _mm256_unpackhi_epi16(v[2], v[3]);
    __m256i t4 = _mm256_unpacklo_epi16(v[4], v[5]);
    __m256i t5 = _mm256_unpackhi_epi16(v[4], v[5]);
    __m256i t6 = _mm256_unpacklo_epi16(v[6], v[7]);
    __m256i t7 = _mm256_unpackhi_epi16(v[6], v[7]);

    __m256i tt0 = _mm256_unpacklo_epi32(t0, t2);
    __m256i tt1 = _mm256_unpackhi_epi32(t0, t2);
    __m256i tt2 = _mm256_unpacklo_epi32(t1, t3);
    __m256i tt3 = _mm256_unpackhi_epi32(t1, t3);
    __m256i tt4 = _mm256_unpacklo_epi32(t4, t6);
    __m256i tt5 = _mm256_unpackhi_epi32(t4, t6);
    __m256i tt6 = _mm256_unpacklo_epi32(t5, t7);
    __m256i tt7 = _mm256_unpackhi_epi32(t5, t7);

    v[0] = _mm256_unpacklo_epi64(tt0, tt4);
    v[1] = _mm256_unpackhi_epi64(tt0, tt4);
    v[2] = _mm256_unpacklo_epi64(tt1, tt5);
    v[3] = _mm256_unpackhi_epi64(tt1, tt5);
    v[4] = _mm256_unpacklo_epi64(tt2, tt6);
    v[5] = _mm256_unpackhi_epi64(tt2, tt6);
    v[6] = _mm256_unpacklo_epi64(tt3, tt7);
    v[7] = _mm256_unpackhi_epi64(tt3, tt7);
}

// === Step 4: Compute SATD for One Channel ===
static uint32_t satd_8x8_channel_avx2(const uint8_t* src, const uint8_t* ref, int stride, char channel) {
    __m256i v[8];

    // Load diff data
    load_rgba_channel_8x8_avx2(src, ref, stride, channel, v);

    // Horizontal Hadamard
    hadamard_1d_8_avx2(v);

    // Transpose
    transpose_8x8_epi16_avx2(v);

    // Vertical Hadamard
    hadamard_1d_8_avx2(v);

    // Sum absolute values
    __m256i sum_lo = _mm256_setzero_si256();
    __m256i sum_hi = _mm256_setzero_si256();

    for (int i = 0; i < 8; ++i) {
        __m256i abs_v = _mm256_abs_epi16(v[i]);

        __m256i lo = _mm256_unpacklo_epi16(abs_v, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi16(abs_v, _mm256_setzero_si256());

        sum_lo = _mm256_add_epi32(sum_lo, lo);
        sum_hi = _mm256_add_epi32(sum_hi, hi);
    }

    __m256i total = _mm256_add_epi32(sum_lo, sum_hi);

    // Horizontal sum across lanes
    __m128i total_lo = _mm256_castsi256_si128(total);
    __m128i total_hi = _mm256_extracti128_si256(total, 1);
    __m128i sum128 = _mm_add_epi32(total_lo, total_hi);

    sum128 = _mm_hadd_epi32(sum128, sum128);
    sum128 = _mm_hadd_epi32(sum128, sum128);

    uint32_t satd = _mm_cvtsi128_si32(sum128);

    // Normalize
    return (satd + 2) >> 2;
}

// === Step 5: Complete RGBA SATD ===
uint32_t satd_8x8_rgba_avx2(const uint8_t* src, const uint8_t* ref, int stride) {
    uint32_t satdR = satd_8x8_channel_avx2(src, ref, stride, 0);
    uint32_t satdG = satd_8x8_channel_avx2(src, ref, stride, 1);
    uint32_t satdB = satd_8x8_channel_avx2(src, ref, stride, 2);

    return satdR + satdG + satdB;
}
