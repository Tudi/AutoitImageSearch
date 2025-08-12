# benchmark_imagesearch.py
# Usage: python benchmark_imagesearch.py
# Assumes images and imagesearchdll.py + ImageSearchDLL.dll are in the same folder.

from __future__ import annotations
import time
from statistics import mean
from enum import IntFlag

# Your wrapper around the DLL
from imagesearchdll import ImageSearchDLL,SADSearchRegionFlags

# ---- Config ----
CACHE_BMP = "Screenshot_0000_0000_0000_1906_1011.bmp"
QUERY_BMPS = [
    "Screenshot_0000_0058_0228_0155_0047.bmp",
    "Screenshot_0001_0583_0286_0103_0028.bmp",
    "Screenshot_0002_1560_0345_0197_0043.bmp",
]

FLAGS_TO_TEST = [
    SADSearchRegionFlags.SSRF_ST_NO_FLAGS,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_SATD,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_HASH,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_HASH,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_SATD,
]

WARMUP = 0     # warmup iterations per (query, flag)
ITERATIONS = 1 # timed iterations per (query, flag)

# ---- Helpers ----
def normalize_result(res):
    """
    Normalize the search result so we can compare across flags even if
    the wrapper returns different shapes (tuple, dict, custom object).
    Fallback to string if we can't detect a structure.
    """
    if res is None:
        return None
    # Common patterns: (x, y, score), (left, top, right, bottom, score), dicts, etc.
    if isinstance(res, (tuple, list)):
        return tuple(res)
    if isinstance(res, dict):
        # Keep key order stable
        return tuple(sorted(res.items()))
    # For custom objects try coords/score attributes if present
    for attrs in (("x", "y", "score"), ("left", "top", "right", "bottom", "score")):
        if all(hasattr(res, a) for a in attrs):
            return tuple(getattr(res, a) for a in attrs)
    return str(res)

def flag_name(flag: SADSearchRegionFlags) -> str:
    return flag.name

def xysad(res):
    # SingleResult from imagesearchdll.py has x, y, sad
    return (getattr(res, "x", -1), getattr(res, "y", -1), getattr(res, "sad", -1))
    
def main():
    dll = ImageSearchDLL()

    # Load the cached background screenshot once
    dll.LoadCacheOverScreenshot(CACHE_BMP, 0, 0)
    dll.BlurrImage(1)
    print(f"loaded as screenshot: {CACHE_BMP}")

    # Data structures to store timings and results
    times = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}
    results = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}

    # Benchmark
    for bmp in QUERY_BMPS:
        print(f"=== Speed testing: {bmp} ===")
        for flag in FLAGS_TO_TEST:
            print(f"=========== Speed testing: {flag_name(flag)} ===")
            # Warmup
            for itr in range(WARMUP):
                dll.SearchImageInRegion(bmp, 0, 0, 1900, 1000, int(flag), False, True)

            # Timed runs
            for itr in range(ITERATIONS):
                tflag = int(flag)
                if itr % 2 == 0:
                    tflag = int(flag) | ( 1 << 25 ) # add/remove useless flag as search flag to avoid search returning a cached result
                t0 = time.perf_counter()
                res = dll.SearchImageInRegion(bmp, 0, 0, 1900, 1000, int(tflag), False, True)
                dt = time.perf_counter() - t0

                times[bmp][flag].append(dt)
#                results[bmp][flag].append(normalize_result(res))
                results[bmp][flag].append(res)

            avg_ms = mean(times[bmp][flag]) * 1000.0
            total_ms = sum(times[bmp][flag]) * 1000.0
            print(f"  {flag_name(flag):>30}:  avg {avg_ms:8.2f} ms, total {total_ms} ms, res {results[bmp][flag][0]}")

        # Verify results identical across flags for this query
        print("  Verifying result consistency across flags...")
        # Take the first flag’s first result as “expected”
        first_flag = FLAGS_TO_TEST[0]
#        expected = results[bmp][first_flag][0]
        expected = xysad(results[bmp][first_flag][0])
        consistent = True
        for flag in FLAGS_TO_TEST:
            # It’s possible results vary slightly per iteration; check set uniqueness
#            unique_vals = set(results[bmp][flag])
            unique_vals = { xysad(r) for r in results[bmp][flag] }
            if len(unique_vals) != 1:
                consistent = False
                print(f"    [WARN] {bmp} with {flag_name(flag)} returned non-deterministic results across runs: {unique_vals}")
            # Compare against expected
            for unique_val in unique_vals:
                if unique_val != expected:
                    consistent = False
                    print(f"    [DIFF] {bmp}: {flag_name(flag)} != {flag_name(first_flag)} result")
                    print(f"           got  : {unique_val}")
                    print(f"           expect: {expected}")

        if consistent:
            print(f"  [OK] All flags produced identical (deterministic) results for this query. : {expected}")
        else:
            print("  [ATTN] Some flags produced different results for this query (see notes above).")

    # Per-flag overall averages across all queries
    print("\n=== Overall Averages (across queries) ===")
    for flag in FLAGS_TO_TEST:
        all_dts = []
        for bmp in QUERY_BMPS:
            all_dts.extend(times[bmp][flag])
        avg_ms = mean(all_dts) * 1000.0
        print(f"  {flag_name(flag):>30}:  overall avg {avg_ms:8.2f} ms")

if __name__ == "__main__":
    main()
    
    
"""
loaded as screenshot: Screenshot_0000_0000_0000_1906_1011.bmp
=== Speed testing: Screenshot_0000_0058_0228_0155_0047.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg  1969.86 ms, total 1969.8573998175561 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|2147483647|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  2192.24 ms, total 2192.241400014609 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=743959, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|743959|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  5139.18 ms, total 5139.179099816829 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|2147483647|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 19027.34 ms, total 19027.338500134647 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=743959, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|743959|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 296146.78 ms, total 296146.77549991757 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|2147483647|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg   394.98 ms, total 394.98250000178814 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|2147483647|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg   409.24 ms, total 409.2442002147436 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=125158, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|125158|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 11437.49 ms, total 11437.48759990558 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|2147483647|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg  6862.15 ms, total 6862.1457000263035 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=125158, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|125158|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 105917.28 ms, total 105917.28009982035 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|2147483647|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg  2204.64 ms, total 2204.6409002505243 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|2147483647|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  2169.89 ms, total 2169.8920000344515 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=542218, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|542218|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  2250.29 ms, total 2250.285899732262 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|2147483647|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 18153.89 ms, total 18153.88649981469 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=542218, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|542218|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 336069.38 ms, total 336069.38329990953 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|2147483647|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1523.16 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1590.46 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  6275.65 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 14681.12 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 246044.48 ms
"""