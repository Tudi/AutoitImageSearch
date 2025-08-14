# imagesearch_au3.py
# Windows-only. Python 3.8+.
from __future__ import annotations
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
from typing import List, Tuple, Optional
from enum import IntFlag

class SADSearchRegionFlags(IntFlag):
    SSRF_ST_NO_FLAGS = 0
    SSRF_ST_PROCESS_INCLUDE_DIFF_INFO = 1 << 0  # result will include the number of pixels, colors that changed
    SSRF_ST_ENFORCE_SAD_WITH_HASH = 1 << 1      # hash uses image features, 10x sower than SAD
    SSRF_ST_ENFORCE_SAD_WITH_SATD = 1 << 2      # better correlation, slower, barelly slower than SAD
#    SSRF_ST_ENFORCE_SAD_WITH_SSD = 1 << 3       # squared differences
#    SSRF_ST_ENFORCE_SAD_WITH_ZNCC = 1 << 4      # Zero-mean normalized cross-correlation
#    SSRF_ST_ENFORCE_SAD_WITH_EDGE_COLOR = 1 << 5
#    SSRF_ST_ENFORCE_SAD_WITH_EDGE_GRAY = 1 << 6
#    SSRF_ST_ENFORCE_SAD_WITH_EDGE_SSIM = 1 << 7
    SSRF_ST_REMOVE_BRIGHTNESS_FROM_SAD = 1 << 8 # will try to adjust brightness so that SAD is performed on similar lightning condition
    SSRF_ST_MAIN_CHECK_IS_HASH = 1 << 9 # instead of SAD, use perceptual hashing - 100x slower than SAD
    SSRF_ST_MAIN_CHECK_IS_SATD = 1 << 10 # istead of SAD, use SATD - 10x slower than SAD ?
    SSRF_ST_INLCUDE_SATD_INFO = 1 << 11
    SSRF_ST_INLCUDE_HASH_INFO = 1 << 12
    SSRF_ST_ALLOW_MULTI_STAGE_SAD2 = 1 << 13 # faster but less precise
    SSRF_ST_ALLOW_MULTI_STAGE_SAD4 = 1 << 14 # faster but less precise
    SSRF_ST_ALLOW_MULTI_STAGE_SAD9 = 1 << 15 # faster but less precise
    SSRF_ST_ALLOW_MULTI_STAGE_GSAD = 1 << 16 # grayscale preSAD before SAD. Min cache width 32
    SSRF_ST_ALLOW_MULTI_STAGE_GSAD2 = 1 << 17 # grayscale preSAD before SAD. Min cache width 64
    
# ------------------
# DLL + prototypes
# ------------------
class _DLL:
    def __init__(self, path: str = "./ImageSearchDLL.dll"):
        self.h = ctypes.WinDLL(path)

        self.SetScreehotFPSLimit = ctypes.WINFUNCTYPE(None, ctypes.c_int)(("SetScreehotFPSLimit", self.h))
        self.TakeScreenshot = ctypes.WINFUNCTYPE(None, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int)(("TakeScreenshot", self.h))
        self.SaveScreenshot = ctypes.WINFUNCTYPE(None)(("SaveScreenshot", self.h))
        self.LoadCacheOverScreenshot = ctypes.WINFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int, ctypes.c_int)(("LoadCacheOverScreenshot", self.h))
        self.ApplyColorBitmask = ctypes.WINFUNCTYPE(None, ctypes.c_uint)(("ApplyColorBitmask", self.h))
        self.ImageSearchRegion = ctypes.WINFUNCTYPE(
            ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
            ctypes.c_uint
        )(("ImageSearch_SAD_Region", self.h))     
        self.BlurrImage = ctypes.WINFUNCTYPE(None, ctypes.c_uint)(("BlurrImage", self.h))

_DEFAULT_MASK = 0x00C0C0C0  # same default as AU3 script
_CHECK_FILE_EXISTS = False   # AU3 popped a MsgBox; python version just skips it

@dataclass
class SingleResult:
    x: int = -1
    y: int = -1
    sad: int = -1
    sad_per_pixel: int = -1
    avg_color_diff: int = -1
    color_diff_count: int = -1
    color_diff_pct: int = -1
    hash_diff_pct: int = -1
    satd: int = -1
    satd_per_pixel: int = -1
    sad_brightness_corrected: int = -1
    raw: str = ""

def _parse_single_result(raw: str) -> SingleResult:
    if not raw:
        return SingleResult(raw="")

    toks = [t for t in (x.strip() for x in raw.split("|")) if t != ""]
    try:    # be flexible about short/partial strings
        return SingleResult(
            x               = int(toks[1]) if len(toks) > 1 else -1,
            y               = int(toks[2]) if len(toks) > 2 else -1,
            sad             = int(toks[3]) if len(toks) > 3 else -1,
            sad_per_pixel   = int(toks[4]) if len(toks) > 4 else -1,
            avg_color_diff  = int(toks[5]) if len(toks) > 5 else -1,
            color_diff_count= int(toks[6]) if len(toks) > 6 else -1,
            color_diff_pct  = int(toks[7]) if len(toks) > 7 else -1,
            hash_diff_pct   = int(toks[8]) if len(toks) > 8 else -1,
            satd            = int(toks[9]) if len(toks) > 9 else -1,
            satd_per_pixel   = int(toks[10]) if len(toks) > 10 else -1,
            sad_brightness_corrected   = int(toks[11]) if len(toks) > 11 else -1,
            raw=raw
        )
    except ValueError:
        return SingleResult(raw=raw)

def _get_coords_from_image_filename(path: str) -> Tuple[int,int,int,int]:
    name = path.replace("\\", "/").split("/")[-1]
    parts = name.split("_")
    if len(parts) < 4:
        return (0,0,0,0)
    try:
        x      = int(float(parts[-4]))
        y      = int(float(parts[-3]))
        width  = int(float(parts[-2]))
        height = int(float(parts[-1].split(".")[0]))
        return (x,y,width,height)
    except Exception:
        return (0,0,0,0)

class ImageSearchDLL:
    def __init__(self, dll_path: str = "./ImageSearchDLL.dll"):
        self._dll = _DLL(dll_path)
        self._mask_default = _DEFAULT_MASK

    def SetScreenshotMaxFPS(self, max_fps: int) -> None:
        self._dll.SetScreehotFPSLimit(int(max_fps))

    def LoadCacheOverScreenshot(self, img_name: str, start_x: int, start_y: int, take_screenshot: bool = True) -> None:
        if take_screenshot == True:
            self._dll.TakeScreenshot(-1, -1, -1, -1)
        path_b = img_name.encode("mbcs", errors="replace")
        self._dll.LoadCacheOverScreenshot(path_b,start_x,start_y)

    def TakeScreenshotRegionAndSaveit(self, start_x: int, start_y: int, end_x: int, end_y: int,
                                      color_mask: int = None) -> None:
        self._dll.TakeScreenshot(int(start_x), int(start_y), int(end_x), int(end_y))
        if color_mask is None:
            color_mask = self._mask_default
        self._dll.ApplyColorBitmask(ctypes.c_uint(color_mask))
        self._dll.SaveScreenshot()

    # --- Image search (region) ---
    def SearchImageInRegion(self, img_name: str,
                        start_x: int = -1, start_y: int = -1,
                        end_x: int = -1, end_y: int = -1,
                        search_flags: int = SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_GSAD2, # in case you catch an instance where this is not accurate, remove it
                        apply_mask: bool = True,
                        search_radius_for_auto_taget = 2,   # when the search location is extracted from the file name, we will search an area to make sure there is no target movement
                        no_new_screenshot = False) -> SingleResult:

        # optional filename-derived coordinates
        if start_x < 0 or start_y < 0 or end_x < 0 or end_y < 0:
            x, y, w, h = _get_coords_from_image_filename(img_name)
            if x or y or w or h:
                start_x = x - search_radius_for_auto_taget
                start_y = y - search_radius_for_auto_taget
                end_x   = x + search_radius_for_auto_taget
                end_y   = y + search_radius_for_auto_taget

        if no_new_screenshot == False:
            # take a (possibly throttled) screenshot of “full screen” (-1,-1,-1,-1)
            self._dll.TakeScreenshot(-1, -1, -1, -1)
            # optional mask to smooth color blending
            if apply_mask:
                self._dll.ApplyColorBitmask(ctypes.c_uint(self._mask_default))

        # call the search; DLL expects ANSI/MBCS path (change to utf-8 if your DLL does)
        path_b = img_name.encode("mbcs", errors="replace")
        raw_ptr = self._dll.ImageSearchRegion(
            path_b, int(start_x), int(start_y), int(end_x), int(end_y),
            ctypes.c_uint(search_flags)
        )
        raw = (ctypes.cast(raw_ptr, ctypes.c_char_p).value or b"").decode("mbcs", errors="replace")
        return _parse_single_result(raw)

    def BlurrImage(self, half_kernel_size: int) -> None:
        self._dll.BlurrImage(int(half_kernel_size))