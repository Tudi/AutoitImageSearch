# benchmark_imagesearch.py
# Usage: python benchmark_imagesearch.py
# Assumes images and imagesearchdll.py + ImageSearchDLL.dll are in the same folder.

from __future__ import annotations
import time
from statistics import mean
from enum import IntFlag

# Your wrapper around the DLL
from imagesearchdll import ImageSearchDLL, SADSearchRegionFlags
import imagesearchdll

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
    SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_GSAD,
    SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_GSAD2,
    SADSearchRegionFlags.SSRF_ST_Similar,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_SATD,
    SADSearchRegionFlags.SSRF_ST_ENFORCE_SAD_WITH_HASH,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_SATD,
    SADSearchRegionFlags.SSRF_ST_MAIN_CHECK_IS_HASH,
]

WARMUP = 1     # warmup iterations per (query, flag)
ITERATIONS = 1 # timed iterations per (query, flag)

# ---- Helpers ----
def flag_name(flag: SADSearchRegionFlags) -> str:
    return flag.name or f"0x{int(flag):08X}"

def xysad(res):
    # imagesearchdll.SingleResult typically has x, y, sad
    return (getattr(res, "x", -1), getattr(res, "y", -1), getattr(res, "sad", -1))

def or_flags_if_exist(base_value: int, *flag_names: str) -> int:
    """
    Safely OR-in enum flags if they exist on SADSearchRegionFlags.
    Returns the combined int value.
    """
    for nm in flag_names:
        f = getattr(SADSearchRegionFlags, nm, None)
        if f is not None:
            base_value |= int(f)
    return base_value

def main():
    dll = ImageSearchDLL()

    # Load the cached background screenshot once
    dll.LoadCacheOverScreenshot(CACHE_BMP, 0, 0)
    dll.BlurrImage(1)
    print(f"loaded as screenshot: {CACHE_BMP}")

    # Per-(bmp, flag) timings/results
    times = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}
    results = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}

    # Warmup stored separately (avoids hacky dict keys)
    warmup_times = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}
    warmup_results = {bmp: {flag: [] for flag in FLAGS_TO_TEST} for bmp in QUERY_BMPS}

    # Benchmark
    for bmp in QUERY_BMPS:
        print(f"=== Speed testing: {bmp} ===")
        for flag in FLAGS_TO_TEST:
            search_start_x = 0
            search_start_y = 0
            search_end_x = 1900
            search_end_y = 1000
            search_width = search_end_x - search_start_x
            search_height = search_end_y - search_start_y
            number_of_compares_made = search_height * search_width

            for itr in range(ITERATIONS):
                # reload image as hash will cache its data and would create an unfair advantage
                dll.LoadCacheOverScreenshot(CACHE_BMP, 0, 0)
                dll.BlurrImage(1)

                tflag = int(flag)
                if itr % 2 == 0:
                    # add/remove a meaningless bit to avoid any result caching keyed by flags
                    tflag |= (1 << 25)

                # ---- Warmup(s) ----
                for w in range(WARMUP):
                    t0 = time.perf_counter()
                    if flag != SADSearchRegionFlags.SSRF_ST_Similar:
                        res_w = dll.SearchImageInRegion(
                            bmp, 0, 0, 1900, 1000,
                            search_flags=tflag,
                            apply_mask=False,
                            no_new_screenshot=True,
                        )
                    else:
                        res_w = dll.SearchSimilar(bmp)
                    dt_w = time.perf_counter() - t0
                    warmup_times[bmp][flag].append(dt_w)
                    if w == 0:
                        warmup_results[bmp][flag].append(res_w)
                        avg_ms = mean(warmup_times[bmp][flag]) * 1000.0
                        total_ms = sum(warmup_times[bmp][flag]) * 1000.0
                        single_check_us = (avg_ms * 1000.0 / number_of_compares_made) if number_of_compares_made else float("nan")
                        print(
                            f"  {('warmup_'+flag_name(flag)):>30}:  "
                            f"avg {avg_ms:6.2f} ms, total {total_ms:7.2f} ms, "
                            f"single check {single_check_us:5.2f} us, res {warmup_results[bmp][flag][0]}"
                        )
            
                # ---- Timed run ----
                # Optionally include extra debug/info flags if they exist (all guarded)
                if itr == 0 and flag != SADSearchRegionFlags.SSRF_ST_Similar:
                    tflag_debug = tflag
                    tflag_debug = tflag_debug | SADSearchRegionFlags.SSRF_ST_INLCUDE_SATD_INFO
                    tflag_debug = tflag_debug | SADSearchRegionFlags.SSRF_ST_INLCUDE_HASH_INFO
                    tflag_debug = tflag_debug | SADSearchRegionFlags.SSRF_ST_PROCESS_INCLUDE_DIFF_INFO
                    tflag_debug = tflag_debug | SADSearchRegionFlags.SSRF_ST_REMOVE_BRIGHTNESS_FROM_SAD
                else:
                    tflag_debug = tflag

                t0 = time.perf_counter()
                if flag != SADSearchRegionFlags.SSRF_ST_Similar:
                    res = dll.SearchImageInRegion(
                        bmp, 0, 0, 1900, 1000,
                        search_flags=tflag_debug,
                        apply_mask=False,
                        no_new_screenshot=True,
                    )
                else:
                    res = dll.SearchSimilar(bmp)
                dt = time.perf_counter() - t0

                times[bmp][flag].append(dt)
                results[bmp][flag].append(res)

            avg_ms = mean(times[bmp][flag]) * 1000.0
            total_ms = sum(times[bmp][flag]) * 1000.0
            single_check_us = (avg_ms * 1000.0 / number_of_compares_made) if number_of_compares_made else float("nan")
            print(
                f"  {flag_name(flag):>30}:  "
                f"avg {avg_ms:6.2f} ms, total {total_ms:7.2f} ms, "
                f"single check {single_check_us:5.2f} us, res {results[bmp][flag][0]}"
            )

        # Verify results identical across flags for this query
        print("  Verifying result consistency across flags...")
        first_flag = FLAGS_TO_TEST[0]
        expected = xysad(results[bmp][first_flag][0])
        consistent = True
        for flag in FLAGS_TO_TEST:
            unique_vals = {xysad(r) for r in results[bmp][flag]}
            if len(unique_vals) != 1:
                consistent = False
                print(f"    [WARN] {bmp} with {flag_name(flag)} returned non-deterministic results across runs: {unique_vals}")
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
        avg_ms = mean(all_dts) * 1000.0 if all_dts else float("nan")
        print(f"  {flag_name(flag):>30}:  overall avg {avg_ms:8.2f} ms")

    # Optional: show warmup stats
    print("\n=== Warmup Averages (across queries) ===")
    for flag in FLAGS_TO_TEST:
        all_w = []
        for bmp in QUERY_BMPS:
            all_w.extend(warmup_times[bmp][flag])
        avg_ms_w = mean(all_w) * 1000.0 if all_w else float("nan")
        print(f"  {flag_name(flag):>30}:  warmup avg {avg_ms_w:8.2f} ms")

if __name__ == "__main__":
    main()

    
    
"""
loaded as screenshot: Screenshot_0000_0000_0000_1906_1011.bmp
=== Speed testing: Screenshot_0000_0058_0228_0155_0047.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 1961.20 ms, total 1961.20 ms, single check  1.03 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|18446744073709551615|18446744073709551615|413888')
                SSRF_ST_NO_FLAGS:  avg 1954.05 ms, total 1954.05 ms, single check  1.03 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1154.87 ms, total 1154.87 ms, single check  0.61 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1142.16 ms, total 1142.16 ms, single check  0.60 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 573.56 ms, total  573.56 ms, single check  0.30 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 582.80 ms, total  582.80 ms, single check  0.31 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 284.58 ms, total  284.58 ms, single check  0.15 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 285.16 ms, total  285.16 ms, single check  0.15 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 261.58 ms, total  261.58 ms, single check  0.14 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 236.05 ms, total  236.05 ms, single check  0.12 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
          warmup_SSRF_ST_Similar:  avg 890.41 ms, total  890.41 ms, single check  0.47 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=10, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|10')
                 SSRF_ST_Similar:  avg 109.08 ms, total  109.08 ms, single check  0.06 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=10, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|10')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2119.25 ms, total 2119.25 ms, single check  1.12 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2115.07 ms, total 2115.07 ms, single check  1.11 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 4326.24 ms, total 4326.24 ms, single check  2.28 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8.53, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8.53|18446744073709551615|18446744073709551615|413888')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 5150.33 ms, total 5150.33 ms, single check  2.71 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21472.46 ms, total 21472.46 ms, single check 11.30 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21017.70 ms, total 21017.70 ms, single check 11.06 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 299255.54 ms, total 299255.54 ms, single check 157.50 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8.53, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8.53|18446744073709551615|18446744073709551615|413888')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 304360.43 ms, total 304360.43 ms, single check 160.19 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8.53, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8.53|743959|34|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 370.58 ms, total  370.58 ms, single check  0.20 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|18446744073709551615|18446744073709551615|55181')
                SSRF_ST_NO_FLAGS:  avg 392.90 ms, total  392.90 ms, single check  0.21 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 286.16 ms, total  286.16 ms, single check  0.15 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 282.12 ms, total  282.12 ms, single check  0.15 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 119.18 ms, total  119.18 ms, single check  0.06 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 128.87 ms, total  128.87 ms, single check  0.07 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 149.66 ms, total  149.66 ms, single check  0.08 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 147.67 ms, total  147.67 ms, single check  0.08 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 126.45 ms, total  126.45 ms, single check  0.07 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 119.96 ms, total  119.96 ms, single check  0.06 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
          warmup_SSRF_ST_Similar:  avg 452.19 ms, total  452.19 ms, single check  0.24 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=8, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|8')
                 SSRF_ST_Similar:  avg  25.74 ms, total   25.74 ms, single check  0.01 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=8, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|8')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 378.48 ms, total  378.48 ms, single check  0.20 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 388.07 ms, total  388.07 ms, single check  0.20 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 10919.93 ms, total 10919.93 ms, single check  5.75 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7.02, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7.02|18446744073709551615|18446744073709551615|55181')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 11304.11 ms, total 11304.11 ms, single check  5.95 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7416.51 ms, total 7416.51 ms, single check  3.90 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7487.02 ms, total 7487.02 ms, single check  3.94 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 110647.18 ms, total 110647.18 ms, single check 58.24 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7.02, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7.02|18446744073709551615|18446744073709551615|55181')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 105938.16 ms, total 105938.16 ms, single check 55.76 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.02, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.02|125158|15|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 2031.91 ms, total 2031.91 ms, single check  1.07 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|18446744073709551615|18446744073709551615|269873')
                SSRF_ST_NO_FLAGS:  avg 2068.33 ms, total 2068.33 ms, single check  1.09 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1160.06 ms, total 1160.06 ms, single check  0.61 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1234.23 ms, total 1234.23 ms, single check  0.65 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 575.96 ms, total  575.96 ms, single check  0.30 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 562.73 ms, total  562.73 ms, single check  0.30 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 333.57 ms, total  333.57 ms, single check  0.18 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 327.43 ms, total  327.43 ms, single check  0.17 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 252.41 ms, total  252.41 ms, single check  0.13 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 276.09 ms, total  276.09 ms, single check  0.15 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
          warmup_SSRF_ST_Similar:  avg 620.94 ms, total  620.94 ms, single check  0.33 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=11, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|11')
                 SSRF_ST_Similar:  avg 177.45 ms, total  177.45 ms, single check  0.09 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=11, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|11')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2013.63 ms, total 2013.63 ms, single check  1.06 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2043.26 ms, total 2043.26 ms, single check  1.08 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2038.58 ms, total 2038.58 ms, single check  1.07 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5.28, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5.28|18446744073709551615|18446744073709551615|269873')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2086.44 ms, total 2086.44 ms, single check  1.10 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 22546.68 ms, total 22546.68 ms, single check 11.87 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21573.64 ms, total 21573.64 ms, single check 11.35 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 333990.84 ms, total 333990.84 ms, single check 175.78 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5.28, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5.28|18446744073709551615|18446744073709551615|269873')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 326749.50 ms, total 326749.50 ms, single check 171.97 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5.28, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5.28|542218|21|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1471.76 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  overall avg   886.17 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  overall avg   424.80 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  overall avg   253.42 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  overall avg   210.70 ms
                 SSRF_ST_Similar:  overall avg   104.09 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1515.47 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  6180.29 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 16692.78 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 245682.69 ms

=== Warmup Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  warmup avg  1454.56 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  warmup avg   867.03 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  warmup avg   422.90 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  warmup avg   255.93 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  warmup avg   213.48 ms
                 SSRF_ST_Similar:  warmup avg   654.51 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  warmup avg  1503.79 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  warmup avg  5761.58 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  warmup avg 17145.22 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  warmup avg 247964.52 ms
"""