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
                SSRF_ST_NO_FLAGS:  avg  1787.75 ms, total 1787.7492001280189 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=2147483647, raw='1|58|228|413888|19|0|0|0|100|2147483647|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  1978.09 ms, total 1978.0942997895181 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=743959, raw='1|58|228|413888|19|0|0|0|100|743959|413888')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  4043.72 ms, total 4043.715599924326 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, sad_brightness_corrected=2147483647, raw='1|58|228|413888|19|0|0|0|8|2147483647|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 18066.65 ms, total 18066.65250007063 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=743959, raw='1|58|228|413888|19|0|0|0|100|743959|413888')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 247849.58 ms, total 247849.58190005273 ms, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, sad_brightness_corrected=2147483647, raw='1|58|228|413888|19|0|0|0|8|2147483647|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg   377.43 ms, total 377.4314997717738 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=2147483647, raw='1|583|286|55181|6|0|0|0|100|2147483647|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg   366.26 ms, total 366.2604000419378 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=125158, raw='1|583|286|55181|6|0|0|0|100|125158|55181')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 10766.63 ms, total 10766.634800005704 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, sad_brightness_corrected=2147483647, raw='1|583|286|55181|6|0|0|0|7|2147483647|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg  6520.37 ms, total 6520.374699961394 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=125158, raw='1|583|286|55181|6|0|0|0|100|125158|55181')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 97056.27 ms, total 97056.27309996635 ms, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, sad_brightness_corrected=2147483647, raw='1|583|286|55181|6|0|0|0|7|2147483647|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
=========== Speed testing: SSRF_ST_NO_FLAGS ===
                SSRF_ST_NO_FLAGS:  avg  2027.62 ms, total 2027.6229004375637 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=2147483647, raw='1|1560|345|269873|10|0|0|0|100|2147483647|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_SATD ===
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg  1997.28 ms, total 1997.2785003483295 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=542218, raw='1|1560|345|269873|10|0|0|0|100|542218|269873')
=========== Speed testing: SSRF_ST_ENFORCE_SAD_WITH_HASH ===
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg  2056.13 ms, total 2056.1305000446737 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, sad_brightness_corrected=2147483647, raw='1|1560|345|269873|10|0|0|0|5|2147483647|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_HASH ===
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 17625.06 ms, total 17625.06360001862 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, sad_brightness_corrected=542218, raw='1|1560|345|269873|10|0|0|0|100|542218|269873')
=========== Speed testing: SSRF_ST_MAIN_CHECK_IS_SATD ===
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 332962.75 ms, total 332962.74559991434 ms, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, sad_brightness_corrected=2147483647, raw='1|1560|345|269873|10|0|0|0|5|2147483647|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1397.60 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1447.21 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  5622.16 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 14070.70 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 225956.20 ms
"""