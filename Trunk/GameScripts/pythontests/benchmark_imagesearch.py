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
    SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_SAD2,
    SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_SAD4,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_SATD,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_HASH,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_SATD,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_HASH,
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
            #print(f"=========== Speed testing: {flag_name(flag)} ===")
            # Warmup
            for itr in range(WARMUP):
                dll.SearchImageInRegion(bmp, 0, 0, 1900, 1000, int(flag), False, True)

            # Timed runs
            search_start_x = 0
            search_start_y = 0
            search_end_x = 1900
            search_end_y = 1000
            search_width = search_end_x - search_start_x
            search_height = search_end_y - search_start_y
            number_of_compares_made = search_height * search_width
            for itr in range(ITERATIONS):
                # reload image as hash will cache it's data and would create an unfair advantage in comparison
                dll.LoadCacheOverScreenshot(CACHE_BMP, 0, 0)
                dll.BlurrImage(1)
                tflag = int(flag)
                if itr % 2 == 0:
                    tflag = int(flag) | ( 1 << 25 ) # add/remove useless flag as search flag to avoid search returning a cached result
                t0 = time.perf_counter()
                res = dll.SearchImageInRegion(bmp, 0, 0, 1900, 1000, search_flags = int(tflag), apply_mask = False, no_new_screenshot = True)
                dt = time.perf_counter() - t0
                # helps with sanity tests. Show as much info as possible so we can see if anything changes ( should not )
                if itr == 0:
                    tflag = tflag | SADSearchRegionFlags.SSRF_ST_INLCUDE_SATD_INFO | SADSearchRegionFlags.SSRF_ST_INLCUDE_HASH_INFO | SADSearchRegionFlags.SSRF_ST_PROCESS_INCLUDE_DIFF_INFO | SADSearchRegionFlags.SSRF_ST_REMOVE_BRIGHTNESS_FROM_SAD
                    res = dll.SearchImageInRegion(bmp, 0, 0, 1900, 1000, search_flags = int(tflag), apply_mask = False, no_new_screenshot = True)

                times[bmp][flag].append(dt)
#                results[bmp][flag].append(normalize_result(res))
                results[bmp][flag].append(res)

            avg_ms = mean(times[bmp][flag]) * 1000.0
            total_ms = sum(times[bmp][flag]) * 1000.0
            print(f"  {flag_name(flag):>30}:  avg {avg_ms:4.2f} ms, total {total_ms:6.2f} ms, single check {(avg_ms*1000/number_of_compares_made):1.2f} us, res {results[bmp][flag][0]}")

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
                SSRF_ST_NO_FLAGS:  avg 1884.71 ms, total 1884.71 ms, single check 0.99 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1079.60 ms, total 1079.60 ms, single check 0.57 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 554.26 ms, total 554.26 ms, single check 0.29 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2095.85 ms, total 2095.85 ms, single check 1.10 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 4578.21 ms, total 4578.21 ms, single check 2.41 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 20771.94 ms, total 20771.94 ms, single check 10.93 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 275959.79 ms, total 275959.79 ms, single check 145.24 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|743959|34|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
                SSRF_ST_NO_FLAGS:  avg 866.07 ms, total 866.07 ms, single check 0.46 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 538.87 ms, total 538.87 ms, single check 0.28 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 348.07 ms, total 348.07 ms, single check 0.18 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 941.58 ms, total 941.58 ms, single check 0.50 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 24682.86 ms, total 24682.86 ms, single check 12.99 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 12422.24 ms, total 12422.24 ms, single check 6.54 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 147426.76 ms, total 147426.76 ms, single check 77.59 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|125158|15|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
                SSRF_ST_NO_FLAGS:  avg 2407.17 ms, total 2407.17 ms, single check 1.27 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1460.23 ms, total 1460.23 ms, single check 0.77 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 742.32 ms, total 742.32 ms, single check 0.39 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2529.25 ms, total 2529.25 ms, single check 1.33 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2628.37 ms, total 2628.37 ms, single check 1.38 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 28587.37 ms, total 28587.37 ms, single check 15.05 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 370673.24 ms, total 370673.24 ms, single check 195.09 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|542218|21|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1719.32 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  overall avg  1026.23 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  overall avg   548.22 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1855.56 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg 10629.81 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 20593.85 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 264686.60 ms
"""