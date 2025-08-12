import os
from imagesearchdll import ImageSearchDLL

isr = ImageSearchDLL(r"./ImageSearchDLL.dll")
isr.SetScreenshotMaxFPS(2)

# else it will save with new name
file_path = r"Screenshot_0000_0080_0045_0160_0080.bmp"
if os.path.exists(file_path):
    os.remove(file_path)
    
# quick region capture -> save (DLL decides where/how to save)
isr.TakeScreenshotRegionAndSaveit(80, 45, 80+160, 45+80)

res1 = isr.SearchImageInRegion(file_path, search_flags=0)
res2 = isr.SearchImageInRegion(file_path, search_flags=((1<<0)|(1<<1)|(1<<2)))
print("found at x,y:", res1.x, res1.y, "SAD:", res1.sad, "raw:", res1.raw)
print("found at x,y:", res2.x, res2.y, "SAD:", res2.sad, "raw:", res2.raw)
