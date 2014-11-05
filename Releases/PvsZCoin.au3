Opt('MustDeclareVars', 1)

global $MB_SYSTEMMODAL = 4096

HotKeySet("]", "ToggleSearch" )
HotKeySet("o", "ExitScript" )

global $dllhandle = DllOpen ( "debug/ImageSearchDLL.dll" )

global $ScriptIsRunning = 1
global $ScriptIsLooping = 0

global $Startx = 0
global $Starty = 120
global $Endx = 805
global $Endy = 525

while $ScriptIsRunning == 1
	Sleep( 100 )
wend	
	
DllClose ( $dllhandle )

func ExitScript()
	$ScriptIsRunning = 0
	$ScriptIsLooping = 0 
endfunc

func ToggleSearch()
	$ScriptIsLooping = 1 - $ScriptIsLooping
	LopSearch()
endfunc 

func LopSearch()
	local $SkipSearchOnColor = 0x00FEFEFE
	local $colorTolerance = 30
	local $ColorToleranceFaultsAccepted = 400 / 30;
	local $ExitAfterNMatchesFound=1
	while( $ScriptIsLooping == 1 )
		DllCall( $dllhandle,"NONE","TakeScreenshot","int",$Startx,"int",$Starty,"int",$Endx,"int",$Endy)
		local $result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","PvsZSilverCoin2.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
		HandleResult( $result )
		local $result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","PvsZGoldCoin.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
		HandleResult( $result )
	wend
endfunc

func HandleResult( $result )
	local $array = StringSplit($result[0],"|")
	local $resCount = Number( $array[0] )
;	MsgBox( $MB_SYSTEMMODAL, "", "res count " & $resCount )
	if( $resCount > 1 ) then
		local $x = Int(Number($array[2]))
		local $y = Int(Number($array[3]))
		MouseClick( "left", $x + 10, $y + 10 );
;		MsgBox( $MB_SYSTEMMODAL, "", "found at " & $x & " " & $y )
	endif
endfunc

Func OnAutoItExit()
    MsgBox(0, "Exit Code", "The exit code is: " & @ExitCode)
EndFunc