#include "stdafx.h"

#include <stdint.h>

#define STRIDE_8x8 32

void TestSATDCorrectness()
{
    // 8x8 source RGBA block
    static const uint8_t src_block_rgba_8x8[8 * STRIDE_8x8] = {
        // Row 0
        52, 20, 70, 255, 55, 21, 71, 254, 61, 22, 72, 253, 66, 23, 73, 252, 50, 24, 74, 251, 45, 25, 75, 250, 40, 26, 76, 249, 35, 27, 77, 248,
        // Row 1
        70, 28, 78, 247, 61, 29, 79, 246, 64, 30, 80, 245, 73, 31, 81, 244, 68, 32, 82, 243, 58, 33, 83, 242, 48, 34, 84, 241, 38, 35, 85, 240,
        // Row 2
        63, 36, 86, 239, 59, 37, 87, 238, 55, 38, 88, 237, 90, 39, 89, 236, 80, 40, 90, 235, 70, 41, 91, 234, 60, 42, 92, 233, 50, 43, 93, 232,
        // Row 3
        67, 44, 94, 231, 61, 45, 95, 230, 68, 46, 96, 229, 104, 47, 97, 228, 94, 48, 98, 227, 84, 49, 99, 226, 74, 50, 100, 225, 64, 51, 101, 224,
        // Row 4
        60, 52, 102, 223, 65, 53, 103, 222, 70, 54, 104, 221, 75, 55, 105, 220, 80, 56, 106, 219, 85, 57, 107, 218, 90, 58, 108, 217, 95, 59, 109, 216,
        // Row 5
        55, 60, 110, 215, 60, 61, 111, 214, 65, 62, 112, 213, 70, 63, 113, 212, 75, 64, 114, 211, 80, 65, 115, 210, 85, 66, 116, 209, 90, 67, 117, 208,
        // Row 6
        50, 68, 118, 207, 55, 69, 119, 206, 60, 70, 120, 205, 65, 71, 121, 204, 70, 72, 122, 203, 75, 73, 123, 202, 80, 74, 124, 201, 85, 75, 125, 200,
        // Row 7
        45, 76, 126, 199, 50, 77, 127, 198, 55, 78, 128, 197, 60, 79, 129, 196, 65, 80, 130, 195, 70, 81, 131, 194, 75, 82, 132, 193, 80, 83, 133, 192
    };

    // 8x8 reference RGBA block
    static const uint8_t ref_block_rgba_8x8[8 * STRIDE_8x8] = {
        // Row 0
        50, 19, 69, 254, 54, 20, 70, 253, 60, 21, 71, 252, 65, 22, 72, 251, 49, 23, 73, 250, 44, 24, 74, 249, 39, 25, 75, 248, 34, 26, 76, 247,
        // Row 1
        69, 27, 77, 246, 60, 28, 78, 245, 63, 29, 79, 244, 72, 30, 80, 243, 67, 31, 81, 242, 57, 32, 82, 241, 47, 33, 83, 240, 37, 34, 84, 239,
        // Row 2
        62, 35, 85, 238, 58, 36, 86, 237, 54, 37, 87, 236, 89, 38, 88, 235, 79, 39, 89, 234, 69, 40, 90, 233, 59, 41, 91, 232, 49, 42, 92, 231,
        // Row 3
        66, 43, 93, 230, 60, 44, 94, 229, 67, 45, 95, 228, 103, 46, 96, 227, 93, 47, 97, 226, 83, 48, 98, 225, 73, 49, 99, 224, 63, 50, 100, 223,
        // Row 4
        59, 51, 101, 222, 64, 52, 102, 221, 69, 53, 103, 220, 74, 54, 104, 219, 79, 55, 105, 218, 84, 56, 106, 217, 89, 57, 107, 216, 94, 58, 108, 215,
        // Row 5
        54, 59, 109, 214, 59, 60, 110, 213, 64, 61, 111, 212, 69, 62, 112, 211, 74, 63, 113, 210, 79, 64, 114, 209, 84, 65, 115, 208, 89, 66, 116, 207,
        // Row 6
        49, 67, 117, 206, 54, 68, 118, 205, 59, 69, 119, 204, 64, 70, 120, 203, 69, 71, 121, 202, 74, 72, 122, 201, 79, 73, 123, 200, 84, 74, 124, 199,
        // Row 7
        44, 75, 125, 198, 49, 76, 126, 197, 54, 77, 127, 196, 59, 78, 128, 195, 64, 79, 129, 194, 69, 80, 130, 193, 74, 81, 131, 192, 79, 82, 132, 191
    };

/* {
        int cpuInfo[4] = { 0 };

        // Check AVX-512F
        __cpuid(cpuInfo, 7);
        int avx512f = (cpuInfo[1] & (1 << 16)) != 0;
        int avx512bw = (cpuInfo[1] & (1 << 30)) != 0;

        printf("AVX-512F : %s\n", avx512f ? "Yes" : "No");
        printf("AVX-512BW: %s\n", avx512bw ? "Yes" : "No");
    }/**/

    int stride = 32;
    size_t satd_scalar = satd_8x8_rgb_reference(src_block_rgba_8x8, ref_block_rgba_8x8, stride, stride);
    size_t satd_SAD = 0;
    size_t satd_nm = satd_nxm((LPCOLORREF)src_block_rgba_8x8, (LPCOLORREF)ref_block_rgba_8x8, 8, 8, stride / 4, stride / 4);
    size_t satd_v1 = satd_8x8_rgb_avx2_v1(src_block_rgba_8x8, ref_block_rgba_8x8, stride, stride);
    size_t satd_v2 = satd_8x8_rgb_avx2_v2(src_block_rgba_8x8, ref_block_rgba_8x8, stride, stride);
#if !defined(_DEBUG) || 1
    #define RETEAT_TEST_COUNT 10000000
    uint8_t* ref_block_rgba_8x8_ = (uint8_t *)malloc(8 * STRIDE_8x8 + 32*4);
    uint8_t* src_block_rgba_8x8_ = (uint8_t*)malloc(8 * STRIDE_8x8 + 32*4);
    memset(ref_block_rgba_8x8_, 0, 8 * STRIDE_8x8 + 32 * 4);
    memset(src_block_rgba_8x8_, 0, 8 * STRIDE_8x8 + 32 * 4);

    memcpy(ref_block_rgba_8x8_, ref_block_rgba_8x8, 8 * STRIDE_8x8);
    memcpy(src_block_rgba_8x8_, src_block_rgba_8x8, 8 * STRIDE_8x8);
    __int64 startscalar = GetTickCount();
    for (size_t i = 0; i < RETEAT_TEST_COUNT; i++)
    {
        src_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        ref_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        satd_scalar += satd_8x8_rgb_reference(src_block_rgba_8x8_, ref_block_rgba_8x8_, stride, stride);
    }
    __int64 endscalar = GetTickCount();

    memcpy(ref_block_rgba_8x8_, ref_block_rgba_8x8, 8 * STRIDE_8x8);
    memcpy(src_block_rgba_8x8_, src_block_rgba_8x8, 8 * STRIDE_8x8);
    __int64 startscalar2 = GetTickCount();
    for (size_t i = 0; i < RETEAT_TEST_COUNT; i++)
    {
        src_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        ref_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        satd_nm += satd_nxm((LPCOLORREF)src_block_rgba_8x8_, (LPCOLORREF)ref_block_rgba_8x8_, 8, 8, stride / 4, stride / 4);
    }
    __int64 endscalar2 = GetTickCount();

    memcpy(ref_block_rgba_8x8_, ref_block_rgba_8x8, 8 * STRIDE_8x8);
    memcpy(src_block_rgba_8x8_, src_block_rgba_8x8, 8 * STRIDE_8x8);
    __int64 startsse = GetTickCount();
    for (size_t i = 0; i < RETEAT_TEST_COUNT; i++)
    {
        src_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        ref_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        satd_v1 += satd_8x8_rgb_avx2_v1(src_block_rgba_8x8_, ref_block_rgba_8x8_, stride, stride);
    }
    __int64 endsse = GetTickCount();

    memcpy(ref_block_rgba_8x8_, ref_block_rgba_8x8, 8 * STRIDE_8x8);
    memcpy(src_block_rgba_8x8_, src_block_rgba_8x8, 8 * STRIDE_8x8);
    __int64 startavx = GetTickCount();
    for (size_t i = 0; i < RETEAT_TEST_COUNT; i++)
    {
        src_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        ref_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        satd_v2 += satd_8x8_rgb_avx2_v2(src_block_rgba_8x8_, ref_block_rgba_8x8_, stride, stride);
    }
    __int64 endavx = GetTickCount();

    memcpy(ref_block_rgba_8x8_, ref_block_rgba_8x8, 8 * STRIDE_8x8);
    memcpy(src_block_rgba_8x8_, src_block_rgba_8x8, 8 * STRIDE_8x8);
    __int64 startSAD = GetTickCount();
    for (size_t i = 0; i < RETEAT_TEST_COUNT; i++)
    {
        src_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        ref_block_rgba_8x8_[i % sizeof(src_block_rgba_8x8)] = (uint8_t)i;
        satd_SAD += ImageSad((LPCOLORREF)src_block_rgba_8x8_, stride / 4, (LPCOLORREF)ref_block_rgba_8x8_, stride / 4, 8, 8);
    }
    __int64 endSAD = GetTickCount();

    /*
    SAD time  16
    Reference 4469
    mn        1594
    v1        1516
    v2        1484
    */
    printf("SAD time  %lld\n", endSAD - startSAD);
    printf("Reference %lld\n", endscalar - startscalar);
    printf("mn        %lld\n", endscalar2 - startscalar2);
    printf("v1        %lld\n", endsse - startsse);
    printf("v2        %lld\n", endavx - startavx);
#endif
    printf("Expected SATD RGBA: %llu\n", satd_scalar);
    printf("mxn               : %llu\n", satd_nm);
    printf("v1                : %llu\n", satd_v1);
    printf("v2                : %llu\n", satd_v2);
}