# imagesearch_au3.py
# Windows-only. Python 3.8+.
from __future__ import annotations
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
from typing import List, Tuple, Optional, Union
from enum import IntFlag
import time
import pyautogui
import psutil
import win32gui
import win32process
from pynput.keyboard import Key, Controller
import os
import sys
import platform

log_fnc=print

# Enable ANSI escape sequences on Windows
if os.name == 'nt':
    os.system('color')
    
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
    SSRF_ST_Similar = 1 << 25 # dummy temp flag, remove me if not removed
    
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
        self.ImageSearchSimilar = ctypes.WINFUNCTYPE(ctypes.c_char_p, ctypes.c_char_p)(("SearchSimilarOnScreenshot", self.h))  
        self.BlurrImage = ctypes.WINFUNCTYPE(None, ctypes.c_uint)(("BlurrImage", self.h))

_DEFAULT_MASK = 0x00C0C0C0  # same default as AU3 script
_CHECK_FILE_EXISTS = False   # AU3 popped a MsgBox; python version just skips it

@dataclass
class SingleResult:
    found_result: int = 0
    x: int = -1
    y: int = -1
    sad: int = -1       # sum of absolute differences
    mad: int = -1       # mean value obtained from SAD. SAD operates on 8x8 blocks, so SAD pixel count is not the same as image size
    avg_color_diff: int = -1    # compare each pixel that changed and see what is the avg change value
    color_diff_count: int = -1  # number of pixels changed 
    color_diff_pct: int = -1    
    hash_diff_pct: float = -1   # using image hashing, how different were the 2 images
    satd: int = -1  # Sum of absolute Hadamard transformed differences
    satd_per_pixel: int = -1 # SATD might be working on 8x8 or 16x16 pixel blocks
    sad_brightness_corrected: int = -1 # if there is a brightness difference between 2 images, try to remove it
    raw: str = ""   # the result string returned by the imgsearch dll
    img_name: str = "" # input parameter to the search function
    found_it: bool = False # if the image we found is not similar enough to the one we were searching for, we makr it to be not found
    img_width: int = -1 # only populated if values are fetched from file name
    img_height: int = -1 # only populated if values are fetched from file name

def _parse_single_result(raw: str) -> SingleResult:
    if not raw:
        return SingleResult(raw="")

    # just making values not have huge size. No actual functionality is being done here
    raw = raw.replace("18446744073709551615","-1")

    toks = [t for t in (x.strip() for x in raw.split("|")) if t != ""]
    try:    # be flexible about short/partial strings
        ret = SingleResult(
            found_result    = int(toks[0]) if len(toks) > 0 else 0,
            x               = int(toks[1]) if len(toks) > 1 else -1,
            y               = int(toks[2]) if len(toks) > 2 else -1,
            sad             = int(toks[3]) if len(toks) > 3 else -1,
            mad             = int(toks[4]) if len(toks) > 4 else -1,
            avg_color_diff  = int(toks[5]) if len(toks) > 5 else -1,
            color_diff_count= int(toks[6]) if len(toks) > 6 else -1,
            color_diff_pct  = int(toks[7]) if len(toks) > 7 else -1,
            hash_diff_pct   = float(toks[8]) if len(toks) > 8 else -1,
            satd            = int(toks[9]) if len(toks) > 9 else -1,
            satd_per_pixel   = int(toks[10]) if len(toks) > 10 else -1,
            sad_brightness_corrected   = int(toks[11]) if len(toks) > 11 else -1,
            raw=raw
        )
        return ret
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
    
    def SearchImageInRegion(self, img_names,
                        start_x: int = -1, start_y: int = -1,
                        end_x: int = -1, end_y: int = -1,
                        search_flags: int = int(SADSearchRegionFlags.SSRF_ST_ALLOW_MULTI_STAGE_GSAD2) | int(SADSearchRegionFlags.SSRF_ST_INLCUDE_HASH_INFO),
                        apply_mask: bool = True,
                        search_radius_for_auto_taget = 10,
                        no_new_screenshot = False,
                        wait_appear_timeout = -1,
                        wait_dissapear_timeout = -1,
                        accepted_diff_pct = -1,
                        click_image = False,
                        click_offset_x = -10000,
                        click_offset_y = -10000,
                        click_sleep = 0.2,
                        extend_search_left = -10000,
                        extend_search_up = -10000,
                        extend_search_right = -10000,
                        extend_search_down = -10000,
                        move_mouse_to00 = True,
                        auto_adjust_to_resolution_change = True,
                        return_first_match = True
                        ) -> SingleResult:
        """
        Search for multiple images and return the first one found (or best match).
        """
        
        if isinstance(img_names, str):
            img_names = [img_names]
        elif not isinstance(img_names, list):
            raise TypeError("img_names must be either a string or a list of strings")
        
        if not img_names:
            result = SingleResult()
            result.found_result = 0
            result.found_it = False
            return result
        
        if move_mouse_to00:
            MouseClick(0, 0, skip_click=True)
        
        start_time = time.time()
        prev_search_res = ""
        debug_output_printed = {}
        
        while True:
            current_best_result = None
            current_best_similarity = -1
            
            # Take screenshot once per iteration
            if no_new_screenshot == False:
                self._dll.TakeScreenshot(-1, -1, -1, -1)
                if apply_mask:
                    self._dll.ApplyColorBitmask(ctypes.c_uint(self._mask_default))
            
            # Search each image
            for img_name in img_names:
                # Calculate search coordinates for this image
                img_start_x, img_start_y, img_end_x, img_end_y = start_x, start_y, end_x, end_y
                w, h = -1, -1
                
                # Get coordinates from filename if not provided
                if img_start_x < 0 or img_start_y < 0 or img_end_x < 0 or img_end_y < 0:
                    x, y, w, h = _get_coords_from_image_filename(img_name)
                    if x and y and w and h:
                        img_start_x = x - search_radius_for_auto_taget
                        img_start_y = y - search_radius_for_auto_taget
                        img_end_x = x + search_radius_for_auto_taget
                        img_end_y = y + search_radius_for_auto_taget
                
                # Apply extensions
                if extend_search_left != -10000:
                    img_start_x -= extend_search_left
                if extend_search_up != -10000:
                    img_start_y -= extend_search_up
                if extend_search_right != -10000:
                    img_end_x += extend_search_right
                if extend_search_down != -10000:
                    img_end_y += extend_search_down
                
                # Apply resolution scaling
                if auto_adjust_to_resolution_change:
                    if img_start_x >= 0:
                        img_start_x = img_start_x * self.img_search_scale_x
                    if img_end_x >= 0:
                        img_end_x = img_end_x * self.img_search_scale_x
                    if img_start_y > 0:
                        img_start_y = img_start_y * self.img_search_scale_y
                    if img_end_y > 0:
                        img_end_y = img_end_y * self.img_search_scale_y
                
                if img_name not in debug_output_printed:
                    debug_output_printed[img_name] = 1
                    log_fnc(f"Searching {img_name} in [{img_start_x},{img_start_y}],[{img_end_x},{img_end_y}]")
                
                # Perform search for this image
                path_b = img_name.encode("mbcs", errors="replace")
                raw_ptr = self._dll.ImageSearchRegion(path_b, int(img_start_x), int(img_start_y), int(img_end_x), int(img_end_y), int(search_flags))
                raw = (ctypes.cast(raw_ptr, ctypes.c_char_p).value or b"").decode("mbcs", errors="replace")
                result = _parse_single_result(raw)
                result.img_name = img_name
                result.img_width = w
                result.img_height = h
                
                # Check similarity
                similarity_ok = False
                if accepted_diff_pct >= 0 and result.found_result != 0:
                    similarity_ok = result.hash_diff_pct <= accepted_diff_pct
                    
                    # Log progress
                    diff_time = int((time.time() - start_time) * 1000)
                    log_msg = f" t{diff_time:>03} {result.hash_diff_pct}% <?> {accepted_diff_pct}% [{img_name}] {result.raw}"
                    if prev_search_res != result.raw:
                        log_fnc(log_msg)
                        prev_search_res = result.raw
                    else:
                        log_fnc(f"\033[A\r\033[K{log_msg}")
                
                result.found_it = similarity_ok
                
                # If return_first_match is True and we found a good match, return immediately
                if return_first_match and similarity_ok == True and wait_dissapear_timeout < 0:
                    current_best_result = result
                    break
                if return_first_match and similarity_ok == False and wait_dissapear_timeout > 0:
                    current_best_result = result
                    break
                
                # Track best result if not returning first match
                if return_first_match == False:
                    similarity_score = result.hash_diff_pct
                    if wait_dissapear_timeout < 0:
                        if similarity_ok == True and (current_best_result is None or similarity_score < current_best_similarity):
                            current_best_result = result
                            current_best_similarity = similarity_score
                    else:
                        if similarity_ok == False and (current_best_result is None or similarity_score < current_best_similarity):
                            current_best_result = result
                            current_best_similarity = similarity_score
            
            # Handle waiting logic
            found_any = current_best_result is not None and current_best_result.found_it
            
            # Handle wait_appear_timeout
            if wait_appear_timeout >= 0:
                if found_any == True:
                    log_fnc("Waiting for img to appear : We see it")
                    break
                elif time.time() - start_time > wait_appear_timeout:
                    log_fnc("Waiting for img to appear. Timeout")
                    break
                else:
                    continue
            
            # Handle wait_disappear_timeout
            if wait_dissapear_timeout >= 0:
                if (current_best_result is not None) and current_best_result.found_it == False and current_best_result.found_result != 0:
                    log_fnc("Waiting for img to vanish. We no longer see it")
                    break
                elif time.time() - start_time > wait_dissapear_timeout:
                    log_fnc("Waiting for img to vanish. Timeout")
                    break
                else:
                    continue
            
            # If no wait timeouts, exit after one iteration
            if wait_appear_timeout < 0 and wait_dissapear_timeout < 0:
                log_fnc("Returning one time search result")
                break
        
        # Return best result or create empty result
        final_result = current_best_result if current_best_result else SingleResult()
        if not current_best_result:
            final_result.found_result = 0
            final_result.found_it = False
        
        # Handle clicking
        if click_image and final_result.found_it:
            click_x = click_offset_x if click_offset_x != -10000 else final_result.img_width/2
            click_y = click_offset_y if click_offset_y != -10000 else final_result.img_height/2
            MouseClick(final_result.x + click_x, final_result.y + click_y, click_sleep)
        
        return final_result


    def BlurrImage(self, half_kernel_size: int) -> None:
        self._dll.BlurrImage(int(half_kernel_size))
        
    # only exists for benchmarking    
    def SearchSimilar(self, img_name: str) -> SingleResult:
        path_b = img_name.encode("mbcs", errors="replace")
        raw_ptr = self._dll.ImageSearchSimilar(path_b)
        raw = (ctypes.cast(raw_ptr, ctypes.c_char_p).value or b"").decode("mbcs", errors="replace")
        return _parse_single_result(raw)     

    img_search_scale_x = 1
    img_search_scale_y = 1
    def auto_scale_images_for_resolution(self, width, height, zoom, scale):
        resolution_info = get_windows_display_essentials()
        print(resolution_info)
        if zoom != resolution_info['zoom_percent']:
            log_fnc(f"{zoom} != {resolution_info['zoom_percent']}, can't auto adjust image positions. Automation will probably fail")
            return
        if scale != resolution_info['scale_factor']:
            log_fnc(f"{scale} != resolution_info['scale_factor'], can't auto adjust image positions. Automation will probably fail")
            return
        self.img_search_scale_x = width / resolution_info['logical_resolution']['width']
        self.img_search_scale_y = height / resolution_info['logical_resolution']['height']
        log_fnc(f"Will auto scale image positions by x={self.img_search_scale_x} y={self.img_search_scale_y}")

def get_windows_display_essentials():
    """Get essential display info using only Windows API"""
    if platform.system() != 'Windows':
        return {'error': 'Windows only'}
    
    user32 = ctypes.windll.user32
    gdi32 = ctypes.windll.gdi32
    
    hdc = user32.GetDC(0)
    try:
        # Essential measurements
        logical_width = gdi32.GetDeviceCaps(hdc, 8)   # HORZRES
        logical_height = gdi32.GetDeviceCaps(hdc, 10)  # VERTRES
        physical_width = gdi32.GetDeviceCaps(hdc, 118) # DESKTOPHORZRES  
        physical_height = gdi32.GetDeviceCaps(hdc, 117) # DESKTOPVERTRES
        dpi_x = gdi32.GetDeviceCaps(hdc, 88)  # LOGPIXELSX
        dpi_y = gdi32.GetDeviceCaps(hdc, 90)  # LOGPIXELSY
        
        zoom_percent = round((dpi_x / 96) * 100)
        scale_factor = physical_width / logical_width if logical_width > 0 else 1.0
        
        return {
            'physical_resolution': {'width': physical_width, 'height': physical_height},
            'logical_resolution': {'width': logical_width, 'height': logical_height},
            'dpi': {'x': dpi_x, 'y': dpi_y},
            'zoom_percent': zoom_percent,
            'scale_factor': round(scale_factor, 3),
        }
    finally:
        user32.ReleaseDC(0, hdc)
        
#######################################################################################################
    
def MouseClick(x:int, y:int, click_sleep = 0.1, skip_click = False):
    if skip_click == False:
        log_fnc(f"Will click mouse coord : [{x},{y}]")
    else:
        log_fnc(f"Will move mouse to coord : [{x},{y}]")
        
    # Windows API constants
    MOUSEEVENTF_MOVE = 0x0001
    MOUSEEVENTF_ABSOLUTE = 0x8000
    MOUSEEVENTF_LEFTDOWN = 0x0002
    MOUSEEVENTF_LEFTUP = 0x0004

    # get screen size
    screen_width = ctypes.windll.user32.GetSystemMetrics(0)
    screen_height = ctypes.windll.user32.GetSystemMetrics(1)

    # convert to absolute coordinates (0..65535)
    abs_x = int(x * 65535 / screen_width)
    abs_y = int(y * 65535 / screen_height)

    # move mouse
    ctypes.windll.user32.mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, abs_x, abs_y, 0, 0)
    time.sleep(click_sleep)  # short delay to ensure move registers

    # click
    if skip_click == False:
        ctypes.windll.user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        ctypes.windll.user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)    

#######################################################################################################

class SendKeys:
    def __init__(self):
        self.keyboard = Controller()
    
    def send_key(self, key, delay=0.03):
        """Send a single key press"""
        if isinstance(key, str):
            if len(key) == 1:
                # Single character
                self.keyboard.press(key)
                self.keyboard.release(key)
            else:
                # Special key name (convert to pynput Key)
                special_key = self._get_special_key(key)
                if special_key:
                    self.keyboard.press(special_key)
                    self.keyboard.release(special_key)
        else:
            # Direct Key object
            self.keyboard.press(key)
            self.keyboard.release(key)
        
        time.sleep(delay)
    
    def send_text(self, text, delay=0.03):
        """Send a string of text"""
        for char in text:
            self.keyboard.press(char)
            self.keyboard.release(char)
            time.sleep(delay)
    
    def send_keys_combination(self, *keys):
        """Send key combination (like Ctrl+C)"""
        # Press all keys
        for key in keys:
            if isinstance(key, str):
                key = self._get_special_key(key) or key
            self.keyboard.press(key)
        
        # Release all keys in reverse order
        for key in reversed(keys):
            if isinstance(key, str):
                key = self._get_special_key(key) or key
            self.keyboard.release(key)
    
    def _get_special_key(self, key_name):
        """Convert string key names to pynput Key objects"""
        key_mapping = {
            'enter': Key.enter,
            'return': Key.enter,
            'tab': Key.tab,
            'space': Key.space,
            'backspace': Key.backspace,
            'delete': Key.delete,
            'esc': Key.esc,
            'escape': Key.esc,
            'shift': Key.shift,
            'ctrl': Key.ctrl,
            'alt': Key.alt,
            'cmd': Key.cmd,
            'win': Key.cmd,  # Add this line - Windows key
            'windows': Key.cmd,  # Add this line - Alternative name
            'up': Key.up,
            'down': Key.down,
            'left': Key.left,
            'right': Key.right,
            'home': Key.home,
            'end': Key.end,
            'page_up': Key.page_up,
            'page_down': Key.page_down,
            'f1': Key.f1, 'f2': Key.f2, 'f3': Key.f3, 'f4': Key.f4,
            'f5': Key.f5, 'f6': Key.f6, 'f7': Key.f7, 'f8': Key.f8,
            'f9': Key.f9, 'f10': Key.f10, 'f11': Key.f11, 'f12': Key.f12,
        }
        return key_mapping.get(key_name.lower())
        
#######################################################################################################
        
def winactivate(process_name, start_process_path = "", wait_time_startup = 3):
    found = False
    
    def callback(hwnd, _):
        nonlocal found
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd):
            try:
                _, pid = win32process.GetWindowThreadProcessId(hwnd)
                if psutil.Process(pid).name() == process_name:
                    win32gui.SetForegroundWindow(hwnd)
                    found = True
                    time.sleep(0.1)
            except:
                pass
        return True
    
    win32gui.EnumWindows(callback, None)
    
    if found == False and len(start_process_path) != 0:
        # Start the process with admin rights
        try:
            # Method 1: Using ShellExecute with runas
            import win32api
            win32api.ShellExecute(None, "runas", start_process_path, None, None, 1)
        except ImportError:
            # Method 2: Using subprocess with PowerShell
            try:
                subprocess.run([
                    "powershell", 
                    "-Command", 
                    f"Start-Process '{start_process_path}' -Verb RunAs"
                ], check=True)
            except subprocess.CalledProcessError:
                # Method 3: Fallback to regular process start
                subprocess.Popen([start_process_path])
        except:
            return False
        
        # Wait a bit
        time.sleep(wait_time_startup)
        
    return found
    
class ScriptExitException(Exception):
    """Custom exception to gracefully exit script execution"""
    def __init__(self, message="Script execution terminated", return_value=False):
        self.message = message
        self.return_value = return_value
        super().__init__(self.message)    
        