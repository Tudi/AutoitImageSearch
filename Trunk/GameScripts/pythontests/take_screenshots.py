# hot_region_screenshot.py
# Windows-only (uses Win32 APIs). Requires: pip install keyboard
from __future__ import annotations
import ctypes
from ctypes import wintypes
import sys
import keyboard
import time

# Import your wrapper around ImageSearchDLL.dll
# (This file comes from your upload and exposes TakeScreenshotRegionAndSaveit)
from imagesearchdll import ImageSearchDLL  # :contentReference[oaicite:0]{index=0}

# --- Win32 mouse position ---
def get_mouse_pos() -> tuple[int, int]:
    pt = wintypes.POINT()
    ctypes.windll.user32.GetCursorPos(ctypes.byref(pt))
    return pt.x, pt.y

def normalize_rect(p1: tuple[int, int], p2: tuple[int, int]) -> tuple[int, int, int, int]:
    x1, y1 = p1
    x2, y2 = p2
    left   = x1 if x1 < x2 else x2
    right  = x2 if x2 > x1 else x1
    top    = y1 if y1 < y2 else y2
    bottom = y2 if y2 > y1 else y1
    return left, top, right, bottom

running = True

def exit_program():
    global running
    print("ESC pressed, exiting...")
    running = False
    
def main():
    print("Hotkeys:")
    print("  q  -> set TOP-LEFT to current mouse position")
    print("  s  -> set BOTTOM-RIGHT to current mouse position")
    print("  r  -> TakeScreenshotRegionAndSaveit(left, top, right, bottom)")
    print("  esc-> quit")
    print()

    # Load the DLL wrapper (expects ImageSearchDLL.dll in the same folder or in PATH)
    try:
        dll = ImageSearchDLL()  # uses default path "ImageSearchDLL.dll"  
    except Exception as e:
        print("Failed to load ImageSearchDLL.dll:", e)
        print("Make sure ImageSearchDLL.dll is next to this script or in your PATH.")
        sys.exit(1)

    top_left: tuple[int, int] | None = None
    bottom_right: tuple[int, int] | None = None

    def set_top_left():
        nonlocal top_left
        top_left = get_mouse_pos()
        print(f"[q] top-left set to {top_left}")

    def set_bottom_right():
        nonlocal bottom_right
        bottom_right = get_mouse_pos()
        print(f"[s] bottom-right set to {bottom_right}")

    def run_screenshot():
        if top_left is None or bottom_right is None:
            print("[r] Please set both corners first (q then s).")
            return
        l, t, r, b = normalize_rect(top_left, bottom_right)
        try:
            # Calls into your DLL via the wrapper; it also applies the default mask & saves the screenshot.
            dll.TakeScreenshotRegionAndSaveit(l, t, r, b)  # :contentReference[oaicite:2]{index=2}
            print(f"[r] Screenshot saved for region (L{l}, T{t}, R{r}, B{b}).")
        except Exception as e:
            print("[r] Error calling TakeScreenshotRegionAndSaveit:", e)

    keyboard.add_hotkey("q", set_top_left, suppress=False)
    keyboard.add_hotkey("s", set_bottom_right, suppress=False)
    keyboard.add_hotkey("r", run_screenshot, suppress=False)
    keyboard.add_hotkey("esc", exit_program, suppress=False)

    print("Ready. Set your corners with q/s, then press r.")
    try:
        while running:
            time.sleep(0.1)  # tiny sleep, avoids busy wait
    except KeyboardInterrupt:
        sys.exit(0)

if __name__ == "__main__":
    main()
