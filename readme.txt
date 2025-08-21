Autoit is an amazing automatization tool.
There are many image processing libs out there, i wanted to make one myself. Nothing special about my version ( maybe slower or maybe faster )

How to use:
	- use the 'TakeScreenshot' to capture a portion of the screen
	- apply 'ApplyColorBitmask' to remove small lightning effects ( many games will have color bleed effects )
	- use 'SaveScreenshot' to save the recently captured image to a BMP file
	- use 'ImageSearch_SAD_Region' to search for a recently captured BMP file on the last screen capture
	- Fastest is using flag 'SSRF_ST_ALLOW_MULTI_STAGE_GSAD2', but not as acurate as simple SAD
	- if you have many many searched images on large area, use HASH based image searching that can precompute searched image data. Other option is Similar search
	
Good to know :
	- screen capture is frames per second limited to avoid 100% cpu usage. You can disable FPS limit
	- SAD is never precise, make sure to accept a certain limit of error
	- transparency is only supported by 'ImageSearchOnScreenshotMasked'. Mask is not a gradient, it is a 100% match or no match
	- ImageSearch_SAD_Region will not search a second time on the same image
	- blurring an image will make the edges unusable ( equal to kernel size )
	
Tips :
	- use images where size is multiple of 16. Image compare uses 16 pixels at once and will neglect anything not fully 16 ! 
		Good Ex : 16x16, 32x32, 64x64
		Not perfect ex : 17x25 would only use 16x25 image, 31x31 would only use 16x31 portion of the image
	- if searched image is "large", try to use ImageSearch_Similar. The better the match you are looking for, the faster the search. A perfect match image has diff=0. The worse the match, the larger the difference
	- try to limit screenshot rate. Use "SetScreehotFPSLimit" and set it to 1 FPS ?
	- re-using the same screenshot for multiple image searches is faster than taking a screenshot as soon as possible. Though obvious, if you spam screenshots to be able to catch changes as fast as possible, it might take so much time you detect changes slower
	- internal image format is A8R8G8B8. You don't care about this, but in case you want to take a look at the code, it helps
	- make sure to use raw BMP 24bpp format for searched images. Best is to make a screenshot using the DLL and cut out images using Paint ( yes that idiot tool ) because it will not alter the copied image portion with fancy filters.
	- you can use OCR by creating your own alphabet library
 
Some orientative benchmark results ( just for relative comparison between different modes ):
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