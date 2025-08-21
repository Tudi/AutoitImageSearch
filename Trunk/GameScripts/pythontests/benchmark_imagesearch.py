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
         warmup_SSRF_ST_NO_FLAGS:  avg 1831.26 ms, total 1831.26 ms, single check  0.96 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 1871.63 ms, total 1871.63 ms, single check  0.99 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1056.13 ms, total 1056.13 ms, single check  0.56 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1104.44 ms, total 1104.44 ms, single check  0.58 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 533.52 ms, total  533.52 ms, single check  0.28 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 565.39 ms, total  565.39 ms, single check  0.30 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 293.31 ms, total  293.31 ms, single check  0.15 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 319.55 ms, total  319.55 ms, single check  0.17 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 313.03 ms, total  313.03 ms, single check  0.16 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 285.37 ms, total  285.37 ms, single check  0.15 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 977.23 ms, total  977.23 ms, single check  0.51 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=1992, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|1992', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg 148.75 ms, total  148.75 ms, single check  0.08 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=1992, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|1992', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2118.87 ms, total 2118.87 ms, single check  1.12 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2145.09 ms, total 2145.09 ms, single check  1.13 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 3227.48 ms, total 3227.48 ms, single check  1.70 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=13.26, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|13.26|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 1890.98 ms, total 1890.98 ms, single check  1.00 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19857.02 ms, total 19857.02 ms, single check 10.45 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19899.73 ms, total 19899.73 ms, single check 10.47 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=13.26, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|13.26|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 36181.76 ms, total 36181.76 ms, single check 19.04 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=14.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|14.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 16158.22 ms, total 16158.22 ms, single check  8.50 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=14.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|14.00|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 440.71 ms, total  440.71 ms, single check  0.23 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 492.41 ms, total  492.41 ms, single check  0.26 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 318.25 ms, total  318.25 ms, single check  0.17 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 343.39 ms, total  343.39 ms, single check  0.18 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 139.87 ms, total  139.87 ms, single check  0.07 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 168.01 ms, total  168.01 ms, single check  0.09 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 171.06 ms, total  171.06 ms, single check  0.09 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 225.83 ms, total  225.83 ms, single check  0.12 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 143.25 ms, total  143.25 ms, single check  0.08 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 172.60 ms, total  172.60 ms, single check  0.09 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 520.28 ms, total  520.28 ms, single check  0.27 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=295, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|295', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg  25.37 ms, total   25.37 ms, single check  0.01 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=295, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|295', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 436.22 ms, total  436.22 ms, single check  0.23 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 474.42 ms, total  474.42 ms, single check  0.25 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 5272.58 ms, total 5272.58 ms, single check  2.78 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7.16, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7.16|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 550.87 ms, total  550.87 ms, single check  0.29 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 14838.25 ms, total 14838.25 ms, single check  7.81 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 11612.14 ms, total 11612.14 ms, single check  6.11 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 10072.94 ms, total 10072.94 ms, single check  5.30 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7.16, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7.16|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 3409.20 ms, total 3409.20 ms, single check  1.79 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7.16, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7.16|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 2094.53 ms, total 2094.53 ms, single check  1.10 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 2117.64 ms, total 2117.64 ms, single check  1.11 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1142.38 ms, total 1142.38 ms, single check  0.60 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1181.18 ms, total 1181.18 ms, single check  0.62 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 553.38 ms, total  553.38 ms, single check  0.29 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 567.81 ms, total  567.81 ms, single check  0.30 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 310.24 ms, total  310.24 ms, single check  0.16 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 345.92 ms, total  345.92 ms, single check  0.18 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 244.39 ms, total  244.39 ms, single check  0.13 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 279.40 ms, total  279.40 ms, single check  0.15 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 596.74 ms, total  596.74 ms, single check  0.31 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=619, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|619', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg 154.46 ms, total  154.46 ms, single check  0.08 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=619, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|619', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2016.07 ms, total 2016.07 ms, single check  1.06 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2087.53 ms, total 2087.53 ms, single check  1.10 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2127.34 ms, total 2127.34 ms, single check  1.12 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=4.97, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|4.97|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2074.48 ms, total 2074.48 ms, single check  1.09 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19117.85 ms, total 19117.85 ms, single check 10.06 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21054.24 ms, total 21054.24 ms, single check 11.08 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 84861.08 ms, total 84861.08 ms, single check 44.66 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=4.97, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|4.97|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 35921.51 ms, total 35921.51 ms, single check 18.91 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=4.97, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|4.97|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1493.90 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  overall avg   876.34 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  overall avg   433.74 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  overall avg   297.10 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  overall avg   245.79 ms
                 SSRF_ST_Similar:  overall avg   109.53 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1569.02 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  1505.45 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 17522.04 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 18496.31 ms

=== Warmup Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  warmup avg  1455.50 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  warmup avg   838.92 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  warmup avg   408.92 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  warmup avg   258.20 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  warmup avg   233.56 ms
                 SSRF_ST_Similar:  warmup avg   698.08 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  warmup avg  1523.72 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  warmup avg  3542.46 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  warmup avg 17937.71 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  warmup avg 43705.26 ms
"""