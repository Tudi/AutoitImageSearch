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
    return satd;
}

// Complete SATD for RGBA block 8x8
size_t satd_8x8_rgb_reference(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst) {
    size_t satdR = satd_8x8_channel_scalar(src, ref, stride_src, stride_dst, 0);
    size_t satdG = satd_8x8_channel_scalar(src, ref, stride_src, stride_dst, 1);
    size_t satdB = satd_8x8_channel_scalar(src, ref, stride_src, stride_dst, 2);
//    size_t satdA = satd_8x8_channel_scalar(src, ref, stride_src, stride_dst, 3); // in theory this is 0 all the time
//    return satdR + satdG + satdB + satdA;
    return (satdR + satdG + satdB) >> 2;
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

inline static void Load_1_ChannelIntoRegisters(const uint8_t* src, const uint8_t* ref, size_t stride_src, 
    size_t stride_dst, size_t channel, __m256i *v) {
    const __m256i shuf_mask = _mm256_setr_epi8(
        0, -1, 4, -1,
        8, -1, 12, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1,
        0, -1, 4, -1,
        8, -1, 12, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1
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

        __m256i diff16 = _mm256_sub_epi16(src_shuf, ref_shuf);

        uint64_t val0 = _mm256_extract_epi64(diff16, 0);
        uint64_t val2 = _mm256_extract_epi64(diff16, 2);

        // Now create newr
        v[y] = _mm256_set_epi64x(0, 0, val2, val0);

        // Advance to next row
        src += stride_src;
        ref += stride_dst;
    }
}

size_t satd_8x8_rgb_avx2_v1(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst) {
    __m256i v[8];

    // Sum absolute values
    __m256i sum_lo = _mm256_setzero_si256();
    __m256i sum_hi = _mm256_setzero_si256();

    for (size_t channel_ = 0; channel_ < 3; channel_++)
    {
        Load_1_ChannelIntoRegisters(src, ref, stride_src, stride_dst, channel_, v);
        hadamard_1d_8_avx2(v);
        transpose_8x8_epi16_avx2(v);
        hadamard_1d_8_avx2(v);

        for (int i = 0; i < 8; ++i) {
            __m256i abs_v = _mm256_abs_epi16(v[i]);

            __m256i lo = _mm256_unpacklo_epi16(abs_v, _mm256_setzero_si256());
            __m256i hi = _mm256_unpackhi_epi16(abs_v, _mm256_setzero_si256());

            sum_lo = _mm256_add_epi32(sum_lo, lo);
            sum_hi = _mm256_add_epi32(sum_hi, hi);
        }
    }

    __m256i total = _mm256_add_epi32(sum_lo, sum_hi);
    total = _mm256_hadd_epi32(total, total);
    total = _mm256_hadd_epi32(total, total);

    size_t satd = _mm256_extract_epi32(total, 0);

    // Step 5: Normalize (divide by 4)
    return satd >> 2;
}

void transpose8x16_i16_avx2(const __m256i *v, __m256i *result) {
    // Step 1: Unpack 16-bit elements
    __m256i t0 = _mm256_unpacklo_epi16(v[0], v[1]); // r0_0 r1_0 r0_1 r1_1 ...
    __m256i t1 = _mm256_unpackhi_epi16(v[0], v[1]);
    __m256i t2 = _mm256_unpacklo_epi16(v[2], v[3]);
    __m256i t3 = _mm256_unpackhi_epi16(v[2], v[3]);
    __m256i t4 = _mm256_unpacklo_epi16(v[4], v[5]);
    __m256i t5 = _mm256_unpackhi_epi16(v[4], v[5]);
    __m256i t6 = _mm256_unpacklo_epi16(v[6], v[7]);
    __m256i t7 = _mm256_unpackhi_epi16(v[6], v[7]);

    // Step 2: Unpack 32-bit elements
    __m256i s0 = _mm256_unpacklo_epi32(t0, t2); // r0_0 r2_0 r1_0 r3_0 ...
    __m256i s1 = _mm256_unpackhi_epi32(t0, t2);
    __m256i s2 = _mm256_unpacklo_epi32(t1, t3);
    __m256i s3 = _mm256_unpackhi_epi32(t1, t3);
    __m256i s4 = _mm256_unpacklo_epi32(t4, t6);
    __m256i s5 = _mm256_unpackhi_epi32(t4, t6);
    __m256i s6 = _mm256_unpacklo_epi32(t5, t7);
    __m256i s7 = _mm256_unpackhi_epi32(t5, t7);

    // Step 3: Unpack 64-bit elements
    __m256i u0 = _mm256_unpacklo_epi64(s0, s4); // r0_0 r4_0 r2_0 r6_0 r1_0 r5_0 r3_0 r7_0
    __m256i u1 = _mm256_unpackhi_epi64(s0, s4);
    __m256i u2 = _mm256_unpacklo_epi64(s1, s5);
    __m256i u3 = _mm256_unpackhi_epi64(s1, s5);
    __m256i u4 = _mm256_unpacklo_epi64(s2, s6);
    __m256i u5 = _mm256_unpackhi_epi64(s2, s6);
    __m256i u6 = _mm256_unpacklo_epi64(s3, s7);
    __m256i u7 = _mm256_unpackhi_epi64(s3, s7);

    // Step 4: Extract 128-bit lanes and combine to form transposed columns
    result[0] = _mm256_permute2x128_si256(u0, u4, 0x20);
    result[1] = _mm256_permute2x128_si256(u1, u5, 0x20);
    result[2] = _mm256_permute2x128_si256(u2, u6, 0x20);
    result[3] = _mm256_permute2x128_si256(u3, u7, 0x20);
    result[4] = _mm256_permute2x128_si256(u0, u4, 0x31);
    result[5] = _mm256_permute2x128_si256(u1, u5, 0x31);
    result[6] = _mm256_permute2x128_si256(u2, u6, 0x31);
    result[7] = _mm256_permute2x128_si256(u3, u7, 0x31);
}

inline static void Load_2_ChannelIntoRegisters(const uint8_t* src, const uint8_t* ref, size_t stride_src,
    size_t stride_dst, __m256i* v) {
    const __m256i shuf_mask = _mm256_setr_epi8(
        0, -1, 4, -1,
        8, -1, 12, -1,
        1, -1, 5, -1,
        9, -1, 13, -1,
        0, -1, 4, -1,
        8, -1, 12, -1,
        1, -1, 5, -1,
        9, -1, 13, -1
        );

    for (size_t y = 0; y < 8; ++y) {
        // Load 32 bytes (8 pixels * 4 channels) from src and ref
        __m256i src_row = _mm256_loadu_si256((const __m256i*)(src));
        __m256i ref_row = _mm256_loadu_si256((const __m256i*)(ref));

        // Shuffle to extract the desired channel into i16 values
        __m256i src_shuf = _mm256_shuffle_epi8(src_row, shuf_mask);
        __m256i ref_shuf = _mm256_shuffle_epi8(ref_row, shuf_mask);

        __m256i diff16 = _mm256_sub_epi16(src_shuf, ref_shuf);

        uint64_t val0 = _mm256_extract_epi64(diff16, 0);
        uint64_t val1 = _mm256_extract_epi64(diff16, 1);
        uint64_t val2 = _mm256_extract_epi64(diff16, 2);
        uint64_t val3 = _mm256_extract_epi64(diff16, 3);

        // Now create newr
        v[y] = _mm256_set_epi64x(val3, val1, val2, val0);

        // Advance to next row
        src += stride_src;
        ref += stride_dst;
    }
}

size_t satd_8x8_3channel_AVX2(const uint8_t* src, const uint8_t* ref, size_t stride_src, 
    size_t stride_dst) {
    __m256i v[16];

    // Sum absolute values
    __m256i sum_lo = _mm256_setzero_si256();
    __m256i sum_hi = _mm256_setzero_si256();

    Load_1_ChannelIntoRegisters(src, ref, stride_src, stride_dst, 2, v);
    hadamard_1d_8_avx2(v);
    transpose_8x8_epi16_avx2(v);
    hadamard_1d_8_avx2(v);

    for (int i = 0; i < 8; ++i) {
        __m256i abs_v = _mm256_abs_epi16(v[i]);

        __m256i lo = _mm256_unpacklo_epi16(abs_v, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi16(abs_v, _mm256_setzero_si256());

        sum_lo = _mm256_add_epi32(sum_lo, lo);
        sum_hi = _mm256_add_epi32(sum_hi, hi);
    }

    Load_2_ChannelIntoRegisters(src, ref, stride_src, stride_dst, v);
    hadamard_1d_8_avx2(v); // does it on all 16 int16 on the row
    transpose8x16_i16_avx2(v, v);
    hadamard_1d_8_avx2(&v[0]); // only first 8 values of i16 are valid
    hadamard_1d_8_avx2(&v[8]);

    for (int i = 0; i < 16; ++i) {
        __m256i abs_v = _mm256_abs_epi16(v[i]);

        __m256i lo = _mm256_unpacklo_epi16(abs_v, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi16(abs_v, _mm256_setzero_si256());

        sum_lo = _mm256_add_epi32(sum_lo, lo);
        sum_hi = _mm256_add_epi32(sum_hi, hi);
    }

    __m256i total = _mm256_add_epi32(sum_lo, sum_hi);
    total = _mm256_hadd_epi32(total, total);
    total = _mm256_hadd_epi32(total, total);

    size_t satd = _mm256_extract_epi32(total, 0);

    // Step 5: Normalize (divide by 4)
    return (satd + 2) >> 2;
}

size_t satd_8x8_rgb_avx2_v2(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst)
{
    size_t satdRGB = satd_8x8_3channel_AVX2(src, ref, stride_src, stride_dst);
    return satdRGB;
}
