# imagesearch_au3.py
# Windows-only. Python 3.8+.
from __future__ import annotations
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
from typing import List, Tuple, Optional

# ------------------
# DLL + prototypes
# ------------------
class _DLL:
    def __init__(self, path: str = "ImageSearchDLL.dll"):
        self.h = ctypes.WinDLL(path)

        # void WINAPI TakeScreenshot(int l, int t, int r, int b)
        self.TakeScreenshot = ctypes.WINFUNCTYPE(
            None, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int
        )(("TakeScreenshot", self.h))

        # void WINAPI SaveScreenshot()
        self.SaveScreenshot = ctypes.WINFUNCTYPE(None)(("SaveScreenshot", self.h))

        # void WINAPI ApplyColorBitmask(unsigned int mask)
        try:
            self.ApplyColorBitmask = ctypes.WINFUNCTYPE(None, ctypes.c_uint)(
                ("ApplyColorBitmask", self.h)
            )
        except AttributeError:
            self.ApplyColorBitmask = None

        # char* WINAPI ImageSearch_SAD_Region(const char* file, int l,t,r,b, unsigned int flags)
        self.ImageSearch_SAD_Region = ctypes.WINFUNCTYPE(
            ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
            ctypes.c_uint
        )(("ImageSearch_SAD_Region", self.h))

        # void WINAPI SetScreehotFPSLimit(int fps)   (note: function name spelled like this in AU3)
        try:
            self.SetScreehotFPSLimit = ctypes.WINFUNCTYPE(None, ctypes.c_int)(
                ("SetScreehotFPSLimit", self.h)
            )
        except AttributeError:
            self.SetScreehotFPSLimit = None


# ------------------
# AU3-compat facade
# ------------------
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
    color_diff_pct: int = 100
    hash_diff_pct: int = -1
    raw: str = ""

def _parse_single_result(raw: str) -> SingleResult:
    """
    AU3 logic expects: token1=resCount | token2=x | token3=y | token4=SAD | token5=SAD/px
                       | token6=avgDiff | token7=diffCount | token8=diffPct | token9=hashPct
    We mirror that, tolerantly.
    """
    if not raw:
        return SingleResult(raw="")

    toks = [t for t in (x.strip() for x in raw.split("|")) if t != ""]
    # AutoIt's StringSplit stores count in array[0]; AU3 code reads actual first token at index 1.
    # Here toks[0] is the first token (resCount).
    try:    # be flexible about short/partial strings
        return SingleResult(
            x               = int(toks[1]) if len(toks) > 1 else -1,
            y               = int(toks[2]) if len(toks) > 2 else -1,
            sad             = int(toks[3]) if len(toks) > 3 else -1,
            sad_per_pixel   = int(toks[4]) if len(toks) > 4 else -1,
            avg_color_diff  = int(toks[5]) if len(toks) > 5 else -1,
            color_diff_count= int(toks[6]) if len(toks) > 6 else -1,
            color_diff_pct  = int(toks[7]) if len(toks) > 7 else 100,
            hash_diff_pct   = int(toks[8]) if len(toks) > 8 else -1,
            raw=raw
        )
    except ValueError:
        return SingleResult(raw=raw)

def _parse_multi_result(raw: str) -> List[Tuple[int,int,int]]:
    """
    Mirrors AU3 SearchResultToVectMultiRes:
    returns [(x,y,SAD), ...] for resCount+1 entries (AU3 iterates 0..resCount).
    """
    if not raw:
        return []
    toks = [t for t in (x.strip() for x in raw.split("|")) if t != ""]
    if not toks:
        return []
    try:
        res_count = int(toks[0])
    except ValueError:
        return []
    out: List[Tuple[int,int,int]] = []
    # AU3 indexes tokens as 1-based; here we’re 0-based. For i in [0..res_count]:
    # x at idx = i*3 + 1, y at +2, sad at +3
    for i in range(res_count + 1):
        base = i * 3 + 1
        if base + 2 < len(toks):
            try:
                x = int(toks[base + 0])
                y = int(toks[base + 1])
                sad = int(toks[base + 2])
                out.append((x, y, sad))
            except ValueError:
                break
    return out

def _get_coords_from_image_filename(path: str) -> Tuple[int,int,int,int]:
    """
    AU3 version: split by '_' and read the last 4 tokens as: x, y, width, height
    (order seen in the AU3 file: ... _ x _ y _ width _ height).
    Returns (x,y,width,height) or (0,0,0,0) if not present.
    """
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
    def __init__(self, dll_path: str = "ImageSearchDLL.dll"):
        self._dll = _DLL(dll_path)
        self._mask_default = _DEFAULT_MASK

    def InitScreenshotDllIfRequired(self) -> None:
        # noop in Python; ctor already loads the DLL
        return

    def SetScreenshotMaxFPS(self, max_fps: int) -> None:
        if self._dll.SetScreehotFPSLimit is not None:
            self._dll.SetScreehotFPSLimit(int(max_fps))

    def TakeScreenshotRegionAndSaveit(self, start_x: int, start_y: int, end_x: int, end_y: int,
                                      color_mask: int = None) -> None:
        self._dll.TakeScreenshot(int(start_x), int(start_y), int(end_x), int(end_y))
        if color_mask is None:
            color_mask = self._mask_default
        if self._dll.ApplyColorBitmask and color_mask:
            self._dll.ApplyColorBitmask(ctypes.c_uint(color_mask))
        self._dll.SaveScreenshot()

    # --- Image search (region) ---
    def ImageIsAtRegion(self, img_name: str,
                        start_x: int = -1, start_y: int = -1,
                        end_x: int = -1, end_y: int = -1,
                        search_flags: int = 0,
                        apply_mask: bool = True) -> SingleResult:
        """
        Mirrors AU3 ImageIsAtRegion logic:
        - If coords are negative, parse them from file name and search a small radius
        - TakeScreenshot(-1,-1,-1,-1) to respect FPS limit throttling inside the DLL
        - Apply color bitmask (default 0x00C0C0C0) unless apply_mask=False
        - Call ImageSearch_SAD_Region and parse the '|' output
        """
        # optional filename-derived coordinates
        if start_x < 0 or start_y < 0 or end_x < 0 or end_y < 0:
            x, y, w, h = _get_coords_from_image_filename(img_name)
            if x or y or w or h:
                radius = 2
                start_x = x - radius
                start_y = y - radius
                end_x   = x + radius
                end_y   = y + radius

        # take a (possibly throttled) screenshot of “full screen” (-1,-1,-1,-1)
        self._dll.TakeScreenshot(-1, -1, -1, -1)

        # optional mask to smooth color blending
        if self._dll.ApplyColorBitmask and apply_mask:
            self._dll.ApplyColorBitmask(ctypes.c_uint(self._mask_default))

        # call the search; DLL expects ANSI/MBCS path (change to utf-8 if your DLL does)
        path_b = img_name.encode("mbcs", errors="replace")
        raw_ptr = self._dll.ImageSearch_SAD_Region(
            path_b, int(start_x), int(start_y), int(end_x), int(end_y),
            ctypes.c_uint(search_flags)
        )
        raw = (ctypes.cast(raw_ptr, ctypes.c_char_p).value or b"").decode("mbcs", errors="replace")
        return _parse_single_result(raw)

