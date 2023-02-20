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
;		_WinWaitActivate("BloonsTD6","")
		Send("1")
		MouseClick("left",1310,858,1)
		MouseClick("left",1423,854,1)
		Sleep(21000)
		Send("1")
		MouseClick("left",1423,854,1)
		MouseClick("left",1310,858,1)
		Sleep(21000)
	wend
endfunc