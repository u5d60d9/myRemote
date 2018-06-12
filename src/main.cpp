#include <windows.h>  
#include <stdlib.h>  
#include <tchar.h> 
#include <string>
#include <regex>   
#include <strsafe.h>

#include <shlobj.h>
#include "WinInet.h"
#include "Shlwapi.h"

#pragma comment(lib,"WinInet.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"Urlmon.lib")


//#define debug

// Global variables  
#define IDC_TIMER1 1000
#define WM_COMPLETE WM_APP+1
#define MAPWIDTH   300
#define MAPHEIGHT   80
#define FONT_SIZE	40
#define TIME_OUT		30

bool enable_debug = false;

//#define ScreenX GetSystemMetrics(SM_CXSCREEN);
//#define ScreenY GetSystemMetrics(SM_CYSCREEN);
const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - MAPWIDTH) / 2;
const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - MAPHEIGHT) / 2;

BOOL b_executed = FALSE;

//enum all controls
struct MyControl
{
	LPWSTR ClassName;
	
	RECT Rect;
	LPWSTR Caption;
};


// The main window class name.  
static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.  
static TCHAR szTitle[] = _T("远程辅助工具");
//版本号为4位单字符,最大版本号1.9.9.9。
static TCHAR szVersion[] = _T("1.0.0.2");

HWND  tvHandle;
TCHAR tvInfo[MAX_PATH];
TCHAR params[MAX_PATH]; //store application arguments;
TCHAR teamViewerEXE[MAX_PATH];
LPCWSTR teamViewerTemp = TEXT("%temp%");
UINT counter = 0;
HINSTANCE hInst;
HWND hMyWin;
std::vector<MyControl> winlist;

void hexchar(unsigned char c, unsigned char &hex1, unsigned char &hex2)
{
	hex1 = c / 16;
	hex2 = c % 16;
	hex1 += hex1 <= 9 ? '0' : 'a' - 10;
	hex2 += hex2 <= 9 ? '0' : 'a' - 10;
}

std::wstring urlEncodeCPP(std::wstring s)
{
	const wchar_t *str = s.c_str();
	std::vector<wchar_t> v(s.size());
	v.clear();
	for (size_t i = 0, l = s.size(); i < l; i++)
	{
		wchar_t c = str[i];
		if ((c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
			c == '*' || c == '\'' || c == '(' || c == ')')
		{
			v.push_back(c);
		}
		/*else if (c == ' ')
		{
			v.push_back('+');
		}*/
		else
		{
			v.push_back('%');
			unsigned char d1, d2;
			hexchar(c, d1, d2);
			v.push_back(d1);
			v.push_back(d2);
		}
	}

	return std::wstring(v.cbegin(), v.cend());
}


BOOL CALLBACK  child_window_enum(HWND hWnd, LPARAM lparam){
 
	RECT rect;
	GetWindowRect(hWnd, &rect);
	 
	LPWSTR windowText = new wchar_t[MAX_PATH]; 
	LPWSTR className = new wchar_t[MAX_PATH];

 
	//GetWindowTextW(hWnd,  windowText, MAX_PATH);
	GetClassNameW(hWnd, className, MAX_PATH);
	// MessageBoxW(std::ptr::null_mut(), as_ptr(&mut wide), as_ptr(&mut wide), 0);
 
	if (
		//lstrcmpi(className, _T("Static")) == 0 ||
		lstrcmpi(className, _T("Edit")) == 0)
	{
		
		MyControl ctl;
		
		//if (lstrlen(windowText) == 0){
			//
			UINT len = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
			SendMessage(hWnd, WM_GETTEXT, len + 1, (LPARAM)windowText);
		//}
		ctl.Caption = windowText;
		ctl.ClassName = className;
		ctl.Rect = rect;
		winlist.push_back(ctl);
	}
	 
	return TRUE;
}

//---------------------------------------------------------------------------------------------------
void GetTvInfo(){
	TCHAR temp[40], temp1[40];
	//HWND hwndEdit1, hwndEdit2;

	TCHAR tempPath[MAX_PATH];
	TCHAR adblock[MAX_PATH];

	/* ----------------begin 去广告------------------*/
	ExpandEnvironmentStrings(teamViewerTemp, adblock, MAX_PATH);
	StringCchCat(adblock, MAX_PATH, L"\\TeamViewer\\7.hta");

	if (PathFileExists(adblock)){
		DeleteFile(adblock);
	}
	SHCreateDirectoryEx(NULL, adblock, NULL);
	/* ----------------end 去广告------------------*/

	tvHandle = FindWindow(L"#32770", L"TeamViewer");
	if (tvHandle != NULL){
		winlist.clear();
		EnumChildWindows(tvHandle,  child_window_enum, 0);
		std::wregex rgx(L"^\\d{1,3}(\\s\\d{3}){2,}$");
		std::wsmatch matches;
		bool found = false;
		int right = 0;
		for (std::vector<MyControl>::iterator it = winlist.begin(); it != winlist.end(); ++it)
		{
			if (!found){
				std::wstring s(it->Caption);
				
				if (std::regex_search(s, matches, rgx)) {
					StringCchCopy(temp, MAX_PATH, it->Caption);
					right = it->Rect.right;
					found = true;
					
				}
			}else{
				//almost right align ...
				if (abs((it->Rect.right - right) < 2)){
					StringCchCopy(temp1, MAX_PATH, it->Caption);
					break;
				}
			}
		}

		//wait for next timer!
		if( !found) {
			return;
		}

	 
		KillTimer(hMyWin, IDC_TIMER1);
		std::wstring s;
		s.append(L"&tv_id=");
		s.append(urlEncodeCPP(temp));
		s.append(L"&tv_pw=");
		s.append(urlEncodeCPP(temp1));
		s.append(L"&appver=");
		s.append(szVersion);
		StringCchCat(params, MAX_PATH, s.c_str());  	 

		SendMessage(hMyWin, WM_COMPLETE, 0, 0);
		return;
	}
	else
	{
		if (b_executed)
			return;

		tempPath[0] = '\0';

		ExpandEnvironmentStrings(teamViewerTemp, tempPath, MAX_PATH);
		StringCchCat(tempPath, MAX_PATH, L"\\TeamViewer\\teamviewer.exe");

		//第一次运行
		if (!PathFileExists(tempPath))
		{
			//Open QS包中的teamviewer.exe
			ShellExecute(NULL, L"open", teamViewerEXE, NULL, NULL, SW_SHOWNORMAL);
			b_executed = true;
			return;

		}
		else
		{
			//Open 释放出来的teamviewer.exe
			ShellExecute(NULL, L"open", tempPath, NULL, NULL, SW_SHOWNORMAL);
			b_executed = true;
			return;
		}
	}



}
//---------------------------------------------------------------------------------------------------

BOOL  InetSendData()
{
	if (enable_debug) {
		MessageBox(NULL, params, szVersion, MB_OK);
		return TRUE;
	}

	//https://testtv-request/?uid=xxx
	BOOL result = false;
	BOOL useSSL = false;
	LPCWSTR lpszAccept = _T("*/*");
	LPCWSTR szHeader = _T("Content-Type: application/x-www-form-urlencoded");
	LPCWSTR verb = _T("GET");
	std::wstring host;
	std::wstring httpPath;
	int port=80;

	std::wregex rgx(L"\"?((https?)://)([^:/]*)(:(\\d*))?(/.*)");
	std::wsmatch matches;
	std::wstring s(params);

	if (std::regex_search(s, matches, rgx)) {

		if (matches[2].str() == L"https"){
			useSSL = true;
		}
		host = matches[3].str();
		if (matches[5].str().length() > 0){
			port = _tstoi(matches[5].str().c_str());
		}
		else
		{
			if (useSSL){
				port = INTERNET_DEFAULT_HTTPS_PORT;
			}
			else
				port = INTERNET_DEFAULT_HTTP_PORT;
		}
		httpPath = matches[6].str();
		/*int qoutIndex = httpPath.find_last_of(L'"');
		if (qoutIndex > 0){
			httpPath = httpPath.substr(0, qoutIndex) + httpPath.substr(qoutIndex+1, httpPath.length() - 1);
		}*/
	}


	DWORD flags = INTERNET_FLAG_DONT_CACHE;
	if (useSSL)
		flags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;





	HINTERNET hInet = InternetOpen(_T("myRemote/1.1"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, INTERNET_INVALID_PORT_NUMBER, 0);
	//if (hInet == NULL) MessageBox(NULL, teamViewerTemp, teamViewerTemp, MB_OK);
	// 第二个参数是主机的地址  
	HINTERNET hConn = InternetConnect(hInet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
	//if (hConn == NULL) MessageBox(NULL, teamViewerTemp, teamViewerTemp, MB_OK);
	HINTERNET hPOSTs = HttpOpenRequest(hConn, verb, httpPath.c_str(), _T("HTTP/1.1"), _T("https://myRemoteTool/client"), NULL,
		flags
		, 1);
	//if (hPOSTs == NULL) MessageBox(NULL, teamViewerTemp, teamViewerTemp, MB_OK);
	UINT t = GetLastError();
	BOOL bRequest = HttpSendRequest(hPOSTs, szHeader, lstrlen(szHeader), NULL, NULL);

	/*
	UINT len = lstrlen(params);
	byte* pszData = new byte[len];
	char *pConten = new char[len];
	memset(pszData, 0, len);
	memset(pConten, 0, len);
	WideCharToMultiByte(CP_ACP, 0, params,
	-1, pConten, len, NULL, NULL);
	memcpy(pszData, pConten, len);

	BOOL bRequest = HttpSendRequest(hPOSTs, szHeader, lstrlen(szHeader), pszData, len);

	TCHAR szBuffer[1024];
	DWORD dwByteRead = 0;
	// 防止乱码的方法就是建立完变量立即清空
	ZeroMemory(szBuffer, sizeof(szBuffer));
	// 循环读取缓冲区内容直到结束
	while (InternetReadFile(hPOSTs, szBuffer, sizeof(szBuffer), &dwByteRead) && dwByteRead > 0){
	// 加入结束标记
	szBuffer[dwByteRead] = '\0';
	// 应该用变长字符串的 比如 AnsiString
	if (lstrcmp(szBuffer, _T("OK")) == 0){
	result = true;
	break;
	}
	// 清空缓冲区以备下一次读取
	ZeroMemory(szBuffer, sizeof(szBuffer));
	}
	*/
	// 清理现场  
	InternetCloseHandle(hPOSTs);
	InternetCloseHandle(hConn);
	InternetCloseHandle(hInet);
	return result;
}


// Forward declarations of functions included in this code module:  
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK _tWinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPWSTR     lpCmdLine,
	_In_ int       nCmdShow
	)
{
	LPWSTR c_enable_debug = new wchar_t[2];
	GetEnvironmentVariable(L"RMT_DEBUG", c_enable_debug, sizeof(c_enable_debug));
	if (GetLastError() == ERROR_ENVVAR_NOT_FOUND){
		enable_debug = false;
	}
	else
	{
		enable_debug = (c_enable_debug[0] == '1');
	}

	wchar_t szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	//目录
	PathRemoveFileSpec(szPath);
	//QS路径
	StringCchCat(szPath, MAX_PATH, _T("\\TeamViewerQS11.exe"));
	StringCchCopy(teamViewerEXE, MAX_PATH, szPath);
	//MessageBox(NULL, teamViewerEXE, teamViewerEXE, MB_OK);

	//缓存命令行
	StringCchCopy(params, MAX_PATH, (LPCWSTR)lpCmdLine);
	if (params[lstrlen(params)-1] == L'"'){
		params[lstrlen(params) - 1] = '\0';
	}

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // COLOR_WINDOW
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("ERROR"),
			NULL);

		return 1;
	}

	hInst = hInstance; // Store instance handle in our global variable  


	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_POPUP | WS_DLGFRAME | WS_VISIBLE | WS_EX_TOOLWINDOW,
		ScreenX,ScreenY,MAPWIDTH,MAPHEIGHT,
		/*CW_USEDEFAULT, CW_USEDEFAULT,
		500, 100,*/
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}
	hMyWin = hWnd;
	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);
	SetTimer(hWnd, IDC_TIMER1, 1000, NULL);


	// Main message loop:  
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

long fnWndProc_OnPaint(HWND hWnd,LPCWSTR text)
{
	HFONT hFont, hTmp;
	PAINTSTRUCT ps;
	HBRUSH hBrush;
	RECT rc;
	HDC hDC;

	hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rc);
	hBrush = CreateSolidBrush(RGB(215, 215, 215));
	FillRect(hDC, &rc, hBrush);
	DeleteObject(hBrush);
	SetTextColor(hDC, RGB(50, 50, 170));
	hFont = CreateFont(FONT_SIZE, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"SYSTEM_FIXED_FONT");
	hTmp = (HFONT)SelectObject(hDC, hFont);
	SetBkMode(hDC, TRANSPARENT);
	DrawText(hDC, text, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	DeleteObject(SelectObject(hDC, hTmp));
	EndPaint(hWnd, &ps);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("正在启动,请稍候!");

	switch (message)
	{

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		counter++;
		if (counter >= TIME_OUT){
			KillTimer(hWnd, IDC_TIMER1);
			MessageBox(hWnd, L"操作超时,请重试!", L"警告", MB_OK);			
			PostQuitMessage(0);
		}
		GetTvInfo();
		break;
	case WM_PAINT:
		/*hdc = BeginPaint(hWnd, &ps);		
		TextOut(hdc,
			5, 5,
			greeting, _tcslen(greeting));
		EndPaint(hWnd, &ps);*/
		fnWndProc_OnPaint(hWnd, greeting);
		break;
	case WM_COMPLETE:
		InetSendData();
		//MessageBox(hWnd,params,params,MB_OK);
		PostQuitMessage(0);

		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

