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
         warmup_SSRF_ST_NO_FLAGS:  avg 2120.36 ms, total 2120.36 ms, single check  1.12 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 2032.51 ms, total 2032.51 ms, single check  1.07 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1282.68 ms, total 1282.68 ms, single check  0.68 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1536.63 ms, total 1536.63 ms, single check  0.81 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 882.31 ms, total  882.31 ms, single check  0.46 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 822.41 ms, total  822.41 ms, single check  0.43 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 357.27 ms, total  357.27 ms, single check  0.19 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 387.32 ms, total  387.32 ms, single check  0.20 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 270.89 ms, total  270.89 ms, single check  0.14 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 289.99 ms, total  289.99 ms, single check  0.15 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 1079.16 ms, total 1079.16 ms, single check  0.57 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=1992, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|1992', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg 155.41 ms, total  155.41 ms, single check  0.08 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=1992, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|58|228|413888|1992', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2285.22 ms, total 2285.22 ms, single check  1.20 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2353.21 ms, total 2353.21 ms, single check  1.24 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 3413.65 ms, total 3413.65 ms, single check  1.80 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=28.93, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|28.93|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2316.42 ms, total 2316.42 ms, single check  1.22 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21313.18 ms, total 21313.18 ms, single check 11.22 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|100.00|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21231.54 ms, total 21231.54 ms, single check 11.17 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 185600.95 ms, total 185600.95 ms, single check 97.68 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=28.93, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=413888, raw='1|58|228|413888|19|0|0|0|28.93|-1|-1|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 69837.98 ms, total 69837.98 ms, single check 36.76 us, res SingleResult(found_result=1, x=58, y=228, sad=413888, sad_per_pixel=19, avg_color_diff=54, color_diff_count=7610, color_diff_pct=35, hash_diff_pct=28.93, satd=743959, satd_per_pixel=34, sad_brightness_corrected=413888, raw='1|58|228|413888|19|54|7610|35|28.93|743959|34|413888', img_name='Screenshot_0000_0058_0228_0155_0047.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (58, 228, 413888)
=== Speed testing: Screenshot_0001_0583_0286_0103_0028.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 507.22 ms, total  507.22 ms, single check  0.27 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 563.03 ms, total  563.03 ms, single check  0.30 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 294.03 ms, total  294.03 ms, single check  0.15 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 336.92 ms, total  336.92 ms, single check  0.18 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 141.88 ms, total  141.88 ms, single check  0.07 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 165.35 ms, total  165.35 ms, single check  0.09 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 158.85 ms, total  158.85 ms, single check  0.08 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 187.92 ms, total  187.92 ms, single check  0.10 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 136.30 ms, total  136.30 ms, single check  0.07 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 168.96 ms, total  168.96 ms, single check  0.09 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 567.41 ms, total  567.41 ms, single check  0.30 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=295, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|295', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg  25.14 ms, total   25.14 ms, single check  0.01 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=295, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|583|286|55181|295', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 496.50 ms, total  496.50 ms, single check  0.26 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 582.77 ms, total  582.77 ms, single check  0.31 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 6200.22 ms, total 6200.22 ms, single check  3.26 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=21.77, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|21.77|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2059.46 ms, total 2059.46 ms, single check  1.08 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7471.93 ms, total 7471.93 ms, single check  3.93 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|100.00|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 7577.62 ms, total 7577.62 ms, single check  3.99 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 72489.91 ms, total 72489.91 ms, single check 38.15 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=21.77, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=55181, raw='1|583|286|55181|6|0|0|0|21.77|-1|-1|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 27815.67 ms, total 27815.67 ms, single check 14.64 us, res SingleResult(found_result=1, x=583, y=286, sad=55181, sad_per_pixel=6, avg_color_diff=25, color_diff_count=2162, color_diff_pct=26, hash_diff_pct=21.77, satd=125158, satd_per_pixel=15, sad_brightness_corrected=55181, raw='1|583|286|55181|6|25|2162|26|21.77|125158|15|55181', img_name='Screenshot_0001_0583_0286_0103_0028.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (583, 286, 55181)
=== Speed testing: Screenshot_0002_1560_0345_0197_0043.bmp ===
         warmup_SSRF_ST_NO_FLAGS:  avg 2381.44 ms, total 2381.44 ms, single check  1.25 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
                SSRF_ST_NO_FLAGS:  avg 2332.47 ms, total 2332.47 ms, single check  1.23 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1353.47 ms, total 1353.47 ms, single check  0.71 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  avg 1394.46 ms, total 1394.46 ms, single check  0.73 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 657.07 ms, total  657.07 ms, single check  0.35 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  avg 677.44 ms, total  677.44 ms, single check  0.36 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 426.87 ms, total  426.87 ms, single check  0.22 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  avg 514.84 ms, total  514.84 ms, single check  0.27 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 338.52 ms, total  338.52 ms, single check  0.18 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  avg 364.46 ms, total  364.46 ms, single check  0.19 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
          warmup_SSRF_ST_Similar:  avg 766.40 ms, total  766.40 ms, single check  0.40 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=619, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|619', img_name='', found_it=False, img_width=-1, img_height=-1)
                 SSRF_ST_Similar:  avg 178.43 ms, total  178.43 ms, single check  0.09 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=619, avg_color_diff=-1, color_diff_count=-1, color_diff_pct=-1, hash_diff_pct=-1, satd=-1, satd_per_pixel=-1, sad_brightness_corrected=-1, raw='1|1560|345|269873|619', img_name='', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2240.02 ms, total 2240.02 ms, single check  1.18 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  avg 2345.64 ms, total 2345.64 ms, single check  1.23 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2327.80 ms, total 2327.80 ms, single check  1.23 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=15.74, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|15.74|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  avg 2468.32 ms, total 2468.32 ms, single check  1.30 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_SATD:  avg 20914.60 ms, total 20914.60 ms, single check 11.01 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=100.0, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|100.00|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_SATD:  avg 21497.28 ms, total 21497.28 ms, single check 11.31 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  warmup_SSRF_ST_MAIN_CHECK_IS_HASH:  avg 249864.46 ms, total 249864.46 ms, single check 131.51 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=0, color_diff_count=0, color_diff_pct=0, hash_diff_pct=15.74, satd=18446744073709551615, satd_per_pixel=-1, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|0|0|0|15.74|-1|-1|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
      SSRF_ST_MAIN_CHECK_IS_HASH:  avg 125103.71 ms, total 125103.71 ms, single check 65.84 us, res SingleResult(found_result=1, x=1560, y=345, sad=269873, sad_per_pixel=10, avg_color_diff=44, color_diff_count=6084, color_diff_pct=24, hash_diff_pct=15.74, satd=542218, satd_per_pixel=21, sad_brightness_corrected=269873, raw='1|1560|345|269873|10|44|6084|24|15.74|542218|21|269873', img_name='Screenshot_0002_1560_0345_0197_0043.bmp', found_it=False, img_width=-1, img_height=-1)
  Verifying result consistency across flags...
  [OK] All flags produced identical (deterministic) results for this query. : (1560, 345, 269873)

=== Overall Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  overall avg  1642.67 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  overall avg  1089.34 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  overall avg   555.07 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  overall avg   363.36 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  overall avg   274.47 ms
                 SSRF_ST_Similar:  overall avg   119.66 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  overall avg  1760.54 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  overall avg  2281.40 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  overall avg 16768.81 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  overall avg 74252.45 ms

=== Warmup Averages (across queries) ===
                SSRF_ST_NO_FLAGS:  warmup avg  1669.67 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD2:  warmup avg   976.73 ms
  SSRF_ST_ALLOW_MULTI_STAGE_SAD4:  warmup avg   560.42 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD:  warmup avg   314.33 ms
  SSRF_ST_ALLOW_MULTI_STAGE_GSAD2:  warmup avg   248.57 ms
                 SSRF_ST_Similar:  warmup avg   804.33 ms
   SSRF_ST_ENFORCE_SAD_WITH_SATD:  warmup avg  1673.91 ms
   SSRF_ST_ENFORCE_SAD_WITH_HASH:  warmup avg  3980.56 ms
      SSRF_ST_MAIN_CHECK_IS_SATD:  warmup avg 16566.57 ms
      SSRF_ST_MAIN_CHECK_IS_HASH:  warmup avg 169318.44 ms
"""