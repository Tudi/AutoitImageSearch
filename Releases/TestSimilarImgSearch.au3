; !!!
; Similar search is only good when you want to search for a lot of small images(15x15?) on the screenshot
; Similar search will always return a position. It's up to you to decide if you want to accept it
; You can try to experiment to reduce screenshot size ( TakeScreenshot, BlurrImage, ResizeScreenshot ) and see if it works better for you

global $MB_SYSTEMMODAL = 4096

$dllhandle = DllOpen ( "Release/ImageSearchDLL.dll" )
;$dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

;taking a chunk out of the screen to test if img search is working
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",40,"int",40,"int",80,"int",80)	//takes 5 seconds / search
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",40,"int",40,"int",60,"int",60)
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",60,"int",60,"int",70,"int",70)
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",40,"int",40,"int",42,"int",42)
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")

;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",100,"int",100)
$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
;$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","tosearch.bmp")
;$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","tosearch10.bmp")
$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","tosearch20.bmp")
;$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","tosearch2.bmp")
;$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","PvsZGoldCoin.bmp")
HandleResult( $result )

DllClose ( $dllhandle )

func TestMultiSearch()
	$i=10
	while( $i>0 )
	$result = DllCall( $dllhandle,"str","SearchSimilarOnScreenshot","str","tosearch.bmp")
	$i = $i - 1;
	wend
endfunc

func HandleResult( $result )
	$array = StringSplit($result[0],"|")
	$resCount = Number( $array[1] )
	if( $resCount > 0 ) then
		$x=Int(Number($array[2]))
		$y=Int(Number($array[3]))
		MsgBox( $MB_SYSTEMMODAL, "", "found at " & $x & " " & $y & " result was " & $result[0])
		MouseMove( $x, $y );
	else
		MsgBox( $MB_SYSTEMMODAL, "", "res count " & $resCount & " from result str " & $result[0] )
	endif
endfunc
