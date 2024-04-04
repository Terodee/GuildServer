#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <mmsystem.h>
#include <time.h>				   
#include "winmain.h"
#include "UserMessages.h"
#include "resource.h"
#include "Game.h"
#include "Version.h"
#include "XSocket.h"

#pragma comment(lib,"winmm.lib")

// #define API_HOOK

#ifdef API_HOOK

#include "madCHook.h"
#pragma comment(lib, "madCHook.lib")



BOOL Inject(BOOL inject)
{

	BOOL    result;
	if (inject)
		result =   InjectLibrary(CURRENT_SESSION | SYSTEM_PROCESSES, "WorldProtectDll.dll");
	else result = UninjectLibrary(CURRENT_SESSION | SYSTEM_PROCESSES, "WorldProtectDll.dll");
	if (!result) {
		MessageBox(NULL, "Error protecting WLServer, please contack lv!", "error", MB_OK);
	}
	return result;
}
#endif


void	PutAdminLogFileList( char * cStr );

// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500

char			szAppClass[32];
HWND			G_hWnd = NULL;
BOOL			G_cMsgUpdated =	FALSE;
char			G_cTxt[1024];
char			G_cData50000[50000];
MMRESULT		G_mmTimer = NULL;

XSocket	* G_pListenSock = NULL;
XSocket	* G_pLogSock    = NULL;
CGame		*G_pGame       = NULL;

int				G_iQuitProgramCount = 0;

FILE			* pLogFile;
//2004-09-28 added by jack
DWORD			g_dwCurrentTime;
char			*G_cMsgList[50];
//char			G_cCrashTxt[50000];
// --------------------------------------------------------------
LRESULT CALLBACK	WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{ 
	switch( message )
	{//2004-10-02 modified by jack
		case WM_SYSCOMMAND:
			if( wParam == SC_SCREENSAVE )
			{
				return( 0 );
			}
		case WM_NCPAINT:		//	Äd§T WINDOWS  ­«µeµøµ¡°©¬[ªº°Ê§@
		case WM_NCCALCSIZE:		//	ªý¤î user §ïÅÜ window style
		case WM_ERASEBKGND:
		case WM_MOUSEMOVE:
			return( DefWindowProc( hWnd, message, wParam, lParam ) );

		case WM_CREATE:
			break;

		case WM_KEYDOWN:
	//		G_pGame->OnKeyDown( wParam, lParam );
			return( DefWindowProc( hWnd, message, wParam, lParam ) );
			break;

		case WM_KEYUP:
		//	G_pGame->OnKeyUp( wParam, lParam );
			return( DefWindowProc( hWnd, message, wParam, lParam ) );
			break;
		
		case WM_USER_INTERNAL_ACCEPT:
			G_pGame->bAccept();
			break;

		case WM_USER_STARTGAMESIGNAL:
		//	G_pGame->OnStartGameSignal();
			break;

		case WM_USER_TIMERSIGNAL:
			G_pGame->OnTimer();
			UpdateScreen();
			break;

//		case WM_USER_ACCEPT:
//			OnAccept();
//			break;

		//case WM_KEYUP:
		//	OnKeyUp( wParam, lParam );
		//	break;

		case WM_PAINT:
			OnPaint();
			break;

		case WM_DESTROY:
			OnDestroy();
			break;

		case WM_CLOSE:
		if( G_pGame->bOnClose() == TRUE ) return( DefWindowProc( hWnd, message, wParam, lParam ) );
			//G_iQuitProgramCount++;
			//if (G_iQuitProgramCount >= 2) {
			//	return (DefWindowProc(hWnd, message, wParam, lParam));
			//}
			break;

		//case WM_ONGATESOCKETEVENT:
		//	G_pGame->OnGateSocketEvent( message, wParam, lParam );
		//	break;

		//case WM_ONLOGSOCKETEVENT:
		//	G_pGame->OnMainLogSocketEvent( message, wParam, lParam );
		//	break;

		default:
			if( (message >= WM_ONCLIENTSOCKETEVENT) && (message < WM_ONCLIENTSOCKETEVENT + DEF_MAXCLIENTS) )
				G_pGame->OnClientSocketEvent( message, wParam, lParam );
			else
				return( DefWindowProc( hWnd, message, wParam, lParam ) );
	}

	return( NULL );
}

// --------------------------------------------------------------
int		APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// Install SEH
	// SetUnhandledExceptionFilter( (LPTOP_LEVEL_EXCEPTION_FILTER)lpTopLevelExceptionFilter );
	sprintf( szAppClass, "GameServer%d", (int)hInstance );
	if( !InitApplication( hInstance) )			return( FALSE );
    if( !InitInstance( hInstance, nCmdShow ) )
	{
		InvalidateRect( G_hWnd, NULL, TRUE );
		OnPaint();
		OnDestroy();
		Sleep( 100000 );
		return( FALSE );
	}

	if( !Initialize() )
	{
		InvalidateRect( G_hWnd, NULL, TRUE );
		OnPaint();
		OnDestroy();
		Sleep( 100000 );
		return( FALSE );
	}

	MSG		msg;

#ifdef API_HOOK
	//Injust
	Inject(true);
#endif

	while( 1 )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			if( !GetMessage( &msg, NULL, 0, 0 ) )
			{
				return((int) msg.wParam );
			}
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		else 
			WaitMessage();
		//G_pGame->MsgProcess();
//		else( WaitMessage() );
	}
	
#ifdef API_HOOK
	Inject(false);
#endif

    return 0;
}
// --------------------------------------------------------------
BOOL	InitApplication( HINSTANCE hInstance )
{     
	WNDCLASS  wc;

	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = sizeof (int);
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOWTEXT;//(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szAppClass;

	return( RegisterClass( &wc ) );
}
// --------------------------------------------------------------
BOOL	InitInstance( HINSTANCE hInstance, int nCmdShow )
{
	char	cTitle[100];
//	HANDLE	hFile;
	SYSTEMTIME	SysTime;


	GetLocalTime( &SysTime );

	wsprintf( cTitle, "¹íÖ®¶¼Õ½³¡Guild Server V%s.%s %d (Æô¶¯ÈÕÆÚ: %d %d %d)", DEF_UPPERVERSION, DEF_LOWERVERSION, DEF_BUILDDATE, SysTime.wMonth, SysTime.wDay, SysTime.wHour );

	G_hWnd = CreateWindowEx( 0,  // WS_EX_TOPMOST,
								szAppClass,
								cTitle,
								WS_VISIBLE | // so we don't have to call ShowWindow
								//WS_POPUP |   // non-app window
								//WS_CAPTION | // so our menu doesn't look ultra-goofy
								WS_SYSMENU |  // so we get an icon in the tray
								WS_MINIMIZEBOX,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								800, //GetSystemMetrics(SM_CXSCREEN),
								600, //GetSystemMetrics(SM_CYSCREEN),
								NULL,
								NULL,
								hInstance,
								NULL );

    if( !G_hWnd ) return( FALSE );

	for( int i=0; i < 50; i++ )
	{
		G_cMsgList[i]=new char[120];
		if( !G_cMsgList[i] ) return( FALSE );
		G_cMsgList[i][0]=0;
	}

	ShowWindow( G_hWnd, nCmdShow );
	UpdateWindow( G_hWnd );

	return( TRUE );
}
// --------------------------------------------------------------
BOOL	Initialize()
{
	
	if( _InitWinsock() == FALSE )
	{
		MessageBox( G_hWnd, "Socket 2.2 not found! Cannot execute program.","ERROR", MB_ICONEXCLAMATION | MB_OK );
		PostQuitMessage( 0 );
		return( FALSE );
	}

	G_pGame = new class CGame();
	if( G_pGame->bInit(G_hWnd) == FALSE )
	{
		PutLogList( "(!!!) STOPPED!" );
		return( FALSE );
	}

	// °ÔÀÓ ÁøÇà¿ë Å¸ÀÌ¸Ó 
	G_mmTimer = _StartTimer( 500 );

//	G_pListenSock = new class XSocket( G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT );
//	if( G_pListenSock )
//	{
//		if( G_pGame->m_iGameServerMode == 1 )
//		{
//			G_pListenSock->bListen( G_pGame->m_cGameServerAddrInternal, G_pGame->m_iGameServerPort, WM_USER_ACCEPT );
//		}
//		else if( G_pGame->m_iGameServerMode == 2 )
//		{
//			G_pListenSock->bListen( G_pGame->m_cGameServerAddr, G_pGame->m_iGameServerPort, WM_USER_ACCEPT );
//		}
//	}
//	else
//	{
//		PutLogList( "(!!!) Can't create XSocket. STOPPED!" );
//		return( FALSE );
//	}

	pLogFile = NULL;
	pLogFile = fopen( "test.log","wt+" );
	
	return( TRUE );
}
// --------------------------------------------------------------
void	OnDestroy()
{
	for( int i=0; i < 50; i++ )
	{
		if( G_cMsgList[i] )
		{
			delete [] G_cMsgList[i];	G_cMsgList[i]=NULL;
		}
	}

	if( G_pListenSock != NULL ) delete G_pListenSock;
	if( G_pLogSock != NULL ) delete G_pLogSock;

	if( G_pGame != NULL )
	{
//		G_pGame->Quit();
		delete G_pGame; G_pGame=NULL;
	}

	if( G_mmTimer != NULL ) _StopTimer( G_mmTimer ); G_mmTimer=NULL;
	WSACleanup();

	if( pLogFile != NULL ) fclose( pLogFile ); pLogFile=NULL;

	PostQuitMessage( 0 );
}
// --------------------------------------------------------------
void	PutLogList( char * cMsg )
{
	char	*pTemp=G_cMsgList[49];

	G_cMsgUpdated = TRUE;
	for( int i=48; i > -1; i-- )
		G_cMsgList[i+1]=G_cMsgList[i];
	strcpy( pTemp, cMsg );
	G_cMsgList[0]=pTemp;
	PutAdminLogFileList( cMsg );
}
// --------------------------------------------------------------
void	PutXSocketLogList( char * cMsg )
{
//	char	*pTemp=G_cMsgList[49];

	//G_cMsgUpdated = TRUE;
	//for( int i=48; i > -1; i-- )
	//	G_cMsgList[i+1]=G_cMsgList[i];
	//strcpy( pTemp, cMsg );
	//G_cMsgList[0]=pTemp;
	PutXSocketLogFileList( cMsg );
}
// --------------------------------------------------------------
void	UpdateScreen()
{
	if( G_cMsgUpdated == TRUE )
	{
		InvalidateRect( G_hWnd, NULL, TRUE );
		G_cMsgUpdated = FALSE;
	}
}
// --------------------------------------------------------------
void	OnPaint()
{
	register short	i;
	PAINTSTRUCT		ps;
	HDC		hdc;
	int		y;

	hdc = BeginPaint( G_hWnd, &ps );
//	FillRect( hdc, &rctSrvList, (HBRUSH)(15) );
	SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );
	SetTextColor( hdc, RGB( 255, 255, 255 ) );
	SetBkMode( hdc, TRANSPARENT );

	for( i = 0, y = 557; i < 37; i++, y-=15 )
	{
		TextOut( hdc, 5,  y, G_cMsgList[i], (int)strlen( G_cMsgList[i] ) );
	}

//	if( G_pGame != NULL ) G_pGame->DisplayInfo( hdc );

	EndPaint( G_hWnd, &ps );
}

// --------------------------------------------------------------
void	OnKeyUp( WPARAM wParam, LPARAM lParam )
{
}
// --------------------------------------------------------------
void	OnAccept()
{
//	G_pGame->bAccept( G_pListenSock );
}
// --------------------------------------------------------------
void	CALLBACK _TimerFunc( UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2 )
{
	PostMessage( G_hWnd, WM_USER_TIMERSIGNAL, wID, NULL );
}
// --------------------------------------------------------------
MMRESULT	_StartTimer( DWORD dwTime )
{
	TIMECAPS	caps;
	MMRESULT	timerid;

	timeGetDevCaps( &caps, sizeof(caps) );
	timeBeginPeriod( caps.wPeriodMin );
	timerid = timeSetEvent( dwTime, 0, _TimerFunc, 0, (UINT)TIME_PERIODIC );

	return( timerid );
}
// --------------------------------------------------------------
void	_StopTimer( MMRESULT timerid )
{
	TIMECAPS	caps;

	if( timerid != 0 )
	{
		timeKillEvent( timerid );
		timerid = 0;
		timeGetDevCaps( &caps, sizeof(caps) );
		timeEndPeriod( caps.wPeriodMin );
	}
}
// --------------------------------------------------------------
void	PutLogFileList( char * cStr )
{
	FILE	* pFile;
	char	cBuffer[512];
	SYSTEMTIME	SysTime;

	pFile = fopen( "Events.log", "at" );
	if( pFile == NULL ) return;

	ZeroMemory( cBuffer, sizeof(cBuffer) );

	GetLocalTime( &SysTime );
	wsprintf( cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute );
	strcat( cBuffer, cStr );
	strcat( cBuffer, "\n" );

	fwrite( cBuffer, 1, strlen( cBuffer ), pFile );
	fclose( pFile );
}
// --------------------------------------------------------------
void	PutAdminLogFileList( char * cStr )
{
	FILE	* pFile;
	char	cBuffer[512];
	SYSTEMTIME	SysTime;

	pFile = fopen( "GuildServer.log", "at" );
	if( pFile == NULL ) return;

	ZeroMemory( cBuffer, sizeof(cBuffer) );

	GetLocalTime( &SysTime );
	wsprintf( cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute );
	strcat( cBuffer, cStr );
	strcat( cBuffer, "\n" );

	fwrite( cBuffer, 1, strlen( cBuffer ), pFile );
	fclose( pFile );
}
// --------------------------------------------------------------
void	PutXSocketLogFileList( char * cStr )
{
	FILE	* pFile;
	char	cBuffer[512];
	SYSTEMTIME	SysTime;

	pFile = fopen( "XSocket.log", "at" );
	if( pFile == NULL ) return;

	ZeroMemory( cBuffer, sizeof(cBuffer) );

	GetLocalTime( &SysTime );
	wsprintf( cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute );
	strcat( cBuffer, cStr );
	strcat( cBuffer, "\n" );

	fwrite( cBuffer, 1, strlen( cBuffer ), pFile );
	fclose( pFile );
}
// --------------------------------------------------------------
void	PutItemLogFileList( char * cStr )
{
	FILE	* pFile;
	char	cBuffer[512];
	SYSTEMTIME	SysTime;

	pFile = fopen( "ItemEvents.log", "at" );
	if( pFile == NULL ) return;

	ZeroMemory( cBuffer, sizeof(cBuffer) );

	GetLocalTime( &SysTime );
	wsprintf( cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute );
	strcat( cBuffer, cStr );
	strcat( cBuffer, "\n" );

	fwrite( cBuffer, 1, strlen( cBuffer ), pFile );
	fclose( pFile );
}
// --------------------------------------------------------------
void	PutLogEventFileList( char * cStr )
{
	FILE	* pFile;
	char	cBuffer[512];
	SYSTEMTIME	SysTime;

	pFile = fopen( "LogEvents.log", "at" );
	if( pFile == NULL ) return;

	ZeroMemory( cBuffer, sizeof(cBuffer) );

	GetLocalTime( &SysTime);
	wsprintf( cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute );
	strcat( cBuffer, cStr );
	strcat( cBuffer, "\n" );

	fwrite( cBuffer, 1, strlen( cBuffer ), pFile );
	fclose( pFile );
}
// --------------------------------------------------------------
