Autoit is an amazing automatization tool.
There are many image processing libs out there, i wanted to make one myself. Nothing special about my version ( maybe slower or maybe faster )

How to use:
	- use the 'TakeScreenshot' to capture a portion of the screen
	- apply 'ApplyColorBitmask' to remove small lightning effects ( many games will have color bleed effects )
	- use 'SaveScreenshot' to save the recently captured image to a BMP file
	- use 'ImageSearch_SAD_Region' to search for a recently captured BMP file on the last screen capture
	- if you have many many searched images on large area, use HASH based image searching that can precompute searched image data
	- if you have many many searched images done on the "same" location, use some optimized search ('ImageSearch_Similar' ?) that caches SAD for both BMP files and screen capture. 
	
Good to know :
	- screen capture is frames per second limited to avoid 100% cpu usage. You can disable FPS limit
	- SAD is never precise, make sure to accept a certain limit of error
	- transparency is only supported by 'ImageSearchOnScreenshotMasked'. Mask is not a gradient, it is a 100% match or no match
	- ImageSearch_SAD_Region will not search a second time on the same image
	- blurring an image will make the edges unusable ( equal to kernel size )
	
Tips :
	- use images where size is multiple of 16. Image compare uses 16 pixels at once and will neglect anything not fully 16 ! 
		Good Ex : 16x16, 32x32, 64x64
		Not perfect ex : 17x25 would only use 16x25 image, 31x31 would only use 16x16 portion of the image
	- if searched image is "large", try to use ImageSearch_Similar. The better the match you are looking for, the faster the search. A perfect match image has diff=0. The worse the match, the larger the difference
	- try to limit screenshot rate. Use "SetScreehotFPSLimit" and set it to 1 FPS ?
	- re-using the same screenshot for multiple image searches is faster than taking a screenshot as soon as possible. Though obvious, if you spam screenshots to be able to catch changes as fast as possible, it might take so much time you detect changes slower
	- internal image format is A8R8G8B8. You don't care about this, but in case you want to take a look at the code, it helps
	- make sure to use raw BMP 24bpp format for searched images. Best is to make a screenshot using the DLL and cut out images using Paint ( yes that idiot tool ) because it will not alter the copied image portion with fancy filters.
	- you can use OCR by creating your own alphabet library
 