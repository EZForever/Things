#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>

#include <stdio.h>

#pragma comment(lib, "winsta.lib")

extern "C"
{
	BOOL WinStationPreCreateGlassReplacementSession(LPCWSTR lpwszInitialCommand, DWORD dwConnectTimeout, DWORD* lpdwSessionId); // dwReconnectTargetId == -1
	BOOL WinStationPreCreateGlassReplacementSessionEx(LPCWSTR lpwszInitialCommand, DWORD dwReconnectTargetId, DWORD dwConnectTimeout, DWORD* lpdwSessionId);
	BOOL WinStationTerminateGlassReplacementSession(DWORD dwSessionId);
}

int main()
{
	DWORD dwSessionId = 0;
	if (!WinStationPreCreateGlassReplacementSession(L"%SYSTEMROOT%\\System32\\WinLogon.exe -SpecialSession", 0, &dwSessionId))
	{
		fwprintf(stderr, L"WinStationPreCreateGlassReplacementSession(), GetLastError() = %u\n", GetLastError());
		return 1;
	}

	wprintf(L"dwSessionId = %u\n", dwSessionId);
	system("pause");

	if (!WinStationTerminateGlassReplacementSession(dwSessionId))
	{
		fwprintf(stderr, L"WinStationTerminateGlassReplacementSession(), GetLastError() = %u\n", GetLastError());
		return 1;
	}

	return 0;
}
