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
                if itr == 0 and (tflag & SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_HASH) == 0 and (tflag & SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_HASH) == 0:
                    tflag = tflag | SADSearchRegionFlags.SSRF_ST_INLCUDE_HASH_INFO
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
                SSRF_ST_NO_FLAGS:  avg  1878.26 ms, total 1878.2628001645207 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|2147483647|2147483647|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  2274.73 ms, total 2274.733600206673 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=743959, satd_per_pixel=2933, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|743959|2933|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  4558.10 ms, total 4558.095599990338 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|2147483647|2147483647|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 256189.74 ms, total 256189.74280031398 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|2147483647|2147483647|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 18195.85 ms, total 18195.84759976715 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=743959, satd_per_pixel=2933, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|743959|2933|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg   375.86 ms, total 375.85780024528503 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|2147483647|2147483647|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg   384.39 ms, total 384.3946997076273 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=125158, satd_per_pixel=804, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|125158|804|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 10844.84 ms, total 10844.840600155294 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|2147483647|2147483647|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 100416.89 ms, total 100416.89289966598 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|2147483647|2147483647|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg  6513.53 ms, total 6513.528800103813 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=125158, satd_per_pixel=804, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|125158|804|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg  2016.50 ms, total 2016.5040995925665 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|2147483647|2147483647|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  2178.88 ms, total 2178.8769997656345 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=542218, satd_per_pixel=1989, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|542218|1989|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  2276.94 ms, total 2276.943000033498 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|2147483647|2147483647|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 321418.25 ms, total 321418.25439967215 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=2147483647, satd_per_pixel=2147483647, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|2147483647|2147483647|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 17842.55 ms, total 17842.546300031245 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=542218, satd_per_pixel=1989, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|542218|1989|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1423.54 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1612.67 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  5893.29 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 226008.30 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 14183.97 ms
"""