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
                    tflag_debug = or_flags_if_exist(
                        tflag,
                        "SSRF_ST_INCLUDE_SATD_INFO",
                        "SSRF_ST_INCLUDE_HASH_INFO",
                        "SSRF_ST_PROCESS_INCLUDE_DIFF_INFO",
                        "SSRF_ST_REMOVE_BRIGHTNESS_FROM_SAD",
                    )
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
         warmup_SSRF_ST_NO_FLAGS:  avg 1813.04 ms, total 1813.04 ms, single check  0.95 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|18446744073709551615|18446744073709551615|413888')
                SSRF_ST_NO_FLAGS:  avg 1882.56 ms, total 1882.56 ms, single check  0.99 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|18446744073709551615|18446744073709551615|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1234.42 ms, total 1234.42 ms, single check  0.65 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1265.82 ms, total 1265.82 ms, single check  0.67 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|18446744073709551615|18446744073709551615|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 548.22 ms, total  548.22 ms, single check  0.29 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 531.86 ms, total  531.86 ms, single check  0.28 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|18446744073709551615|18446744073709551615|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 303.10 ms, total  303.10 ms, single check  0.16 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 285.71 ms, total  285.71 ms, single check  0.15 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|18446744073709551615|18446744073709551615|413888')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 233.05 ms, total  233.05 ms, single check  0.12 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|18446744073709551615|18446744073709551615|413888')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 269.34 ms, total  269.34 ms, single check  0.14 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|18446744073709551615|18446744073709551615|413888')
          warmup_SSRF_ST_Similar:  avg 3374.58 ms, total 3374.58 ms, single check  1.78 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=29, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|29')
                 SSRF_ST_Similar:  avg 465.03 ms, total  465.03 ms, single check  0.24 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=29, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|29')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2026.98 ms, total 2026.98 ms, single check  1.07 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|743959|34|413888')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2119.13 ms, total 2119.13 ms, single check  1.12 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|743959|34|413888')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 4786.62 ms, total 4786.62 ms, single check  2.52 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|18446744073709551615|18446744073709551615|413888')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 4489.48 ms, total 4489.48 ms, single check  2.36 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|18446744073709551615|18446744073709551615|413888')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19957.21 ms, total 19957.21 ms, single check 10.50 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100|743959|34|413888')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19735.22 ms, total 19735.22 ms, single check 10.39 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=100, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|100|743959|34|413888')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 277808.77 ms, total 277808.77 ms, single check 146.22 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=8, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|8|18446744073709551615|18446744073709551615|413888')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 294781.96 ms, total 294781.96 ms, single check 155.15 us, res SingleResult(x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=8, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|8|18446744073709551615|18446744073709551615|413888')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 379.33 ms, total  379.33 ms, single check  0.20 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|18446744073709551615|18446744073709551615|55181')
                SSRF_ST_NO_FLAGS:  avg 437.84 ms, total  437.84 ms, single check  0.23 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|18446744073709551615|18446744073709551615|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 293.88 ms, total  293.88 ms, single check  0.15 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 284.43 ms, total  284.43 ms, single check  0.15 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|18446744073709551615|18446744073709551615|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 137.69 ms, total  137.69 ms, single check  0.07 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 161.68 ms, total  161.68 ms, single check  0.09 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|18446744073709551615|18446744073709551615|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 171.26 ms, total  171.26 ms, single check  0.09 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 154.12 ms, total  154.12 ms, single check  0.08 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|18446744073709551615|18446744073709551615|55181')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 130.98 ms, total  130.98 ms, single check  0.07 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|18446744073709551615|18446744073709551615|55181')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 136.11 ms, total  136.11 ms, single check  0.07 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|18446744073709551615|18446744073709551615|55181')
          warmup_SSRF_ST_Similar:  avg 1297.72 ms, total 1297.72 ms, single check  0.68 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=23, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|23')
                 SSRF_ST_Similar:  avg 242.08 ms, total  242.08 ms, single check  0.13 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=23, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|23')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 392.73 ms, total  392.73 ms, single check  0.21 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|125158|15|55181')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 389.04 ms, total  389.04 ms, single check  0.20 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|125158|15|55181')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 10980.31 ms, total 10980.31 ms, single check  5.78 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|18446744073709551615|18446744073709551615|55181')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 11465.61 ms, total 11465.61 ms, single check  6.03 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|18446744073709551615|18446744073709551615|55181')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7265.25 ms, total 7265.25 ms, single check  3.82 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100|125158|15|55181')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7073.84 ms, total 7073.84 ms, single check  3.72 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=100, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|100|125158|15|55181')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 98887.93 ms, total 98887.93 ms, single check 52.05 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=7, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|7|18446744073709551615|18446744073709551615|55181')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 99327.99 ms, total 99327.99 ms, single check 52.28 us, res SingleResult(x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=7, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|7|18446744073709551615|18446744073709551615|55181')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 2122.57 ms, total 2122.57 ms, single check  1.12 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|18446744073709551615|18446744073709551615|269873')
                SSRF_ST_NO_FLAGS:  avg 2109.44 ms, total 2109.44 ms, single check  1.11 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|18446744073709551615|18446744073709551615|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1288.42 ms, total 1288.42 ms, single check  0.68 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1264.49 ms, total 1264.49 ms, single check  0.67 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|18446744073709551615|18446744073709551615|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 617.87 ms, total  617.87 ms, single check  0.33 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 624.54 ms, total  624.54 ms, single check  0.33 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|18446744073709551615|18446744073709551615|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 332.46 ms, total  332.46 ms, single check  0.17 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 322.50 ms, total  322.50 ms, single check  0.17 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|18446744073709551615|18446744073709551615|269873')
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 252.45 ms, total  252.45 ms, single check  0.13 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|18446744073709551615|18446744073709551615|269873')
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 249.33 ms, total  249.33 ms, single check  0.13 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|18446744073709551615|18446744073709551615|269873')
          warmup_SSRF_ST_Similar:  avg 3080.68 ms, total 3080.68 ms, single check  1.62 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=274, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|274')
                 SSRF_ST_Similar:  avg 1980.69 ms, total 1980.69 ms, single check  1.04 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=274, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|274')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2152.80 ms, total 2152.80 ms, single check  1.13 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|542218|21|269873')
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2185.64 ms, total 2185.64 ms, single check  1.15 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|542218|21|269873')
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 15499.96 ms, total 15499.96 ms, single check  8.16 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|18446744073709551615|18446744073709551615|269873')
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 15460.32 ms, total 15460.32 ms, single check  8.14 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|18446744073709551615|18446744073709551615|269873')
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19597.97 ms, total 19597.97 ms, single check 10.31 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100|542218|21|269873')
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 19243.18 ms, total 19243.18 ms, single check 10.13 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=100, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|100|542218|21|269873')
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 293059.25 ms, total 293059.25 ms, single check 154.24 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=5, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|5|18446744073709551615|18446744073709551615|269873')
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 284788.84 ms, total 284788.84 ms, single check 149.89 us, res SingleResult(x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=5, satd=18446744073709551615, satd_per_pixel=18446744073709551615, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|5|18446744073709551615|18446744073709551615|269873')
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1476.61 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  overall avg   938.25 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  overall avg   439.36 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  overall avg   254.11 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  overall avg   218.26 ms
                 SSRF_ST_Similar:  overall avg   895.93 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1564.60 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg 10471.80 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 15350.75 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 226299.60 ms

=== Warmup Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  warmup avg  1438.31 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  warmup avg   938.91 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  warmup avg   434.59 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  warmup avg   268.94 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  warmup avg   205.49 ms
                 SSRF_ST_Similar:  warmup avg  2584.33 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  warmup avg  1524.17 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  warmup avg 10422.30 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  warmup avg 15606.81 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  warmup avg 223251.98 ms
"""