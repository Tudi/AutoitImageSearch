#ifndef _TOOLS_H_
#define _TOOLS_H_

int GetStrHash( const char *str );
int sqrt1( int N );
__int64 sqrt1( __int64 N );
void GetMaxDesktopResolution( int *Width, int *Height );
void StartCounter();
double GetTimeTick();
unsigned int GetTimeTickI();
void EnableDebugPriv();
DWORD GetProcessByExeName(char *ExeName);
HWND FindMainHWND(unsigned long process_id);
void MouseMove(int x, int y);
void MouseDrag(int x1, int y1, int x2, int y2);
void LeftClick(int x, int y);

#endif