Opt("MustDeclareVars", 1)
HotKeySet("o", "ExitBot")
HotKeySet("p", "TempFunc")

global $BotIsRunning = 1
global $FuncIsRunning = 0

while( $BotIsRunning == 1)
	Sleep(1000)
wend

func ExitBot()
	global $BotIsRunning = 0
endfunc

exit

func TempFunc()
	$FuncIsRunning = 1 - $FuncIsRunning
	while( $FuncIsRunning == 1)
;		MouseClick("left",1845,1013,2)
		Send(" ")
		Sleep(1000)		
		Send(" ")
		MouseClick("left",1715,1006,12)
		Sleep(12000)		
		MouseClick("left",938,790,1)
		Sleep(1000)		
	wend
endfunc		