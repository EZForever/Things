#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#ifndef _AMD64_
#error "Change all AppvIsvSubsystems64.dll references to AppvIsvSubsystems32.dll"
#endif

// AppvIsvSubsystems64 communicates with C2R server (OfficeClickToRun.exe) through RPC endpoints:
// "ncalrpc:[C2RClientAPI_Server_System16]"
// "ncalrpc:[AppV-ISV-<UUID>APPV-VIRTMAN-NOTIFICATIONS]"
// Where <UUID> can be found under "HKLM\SOFTWARE\Microsoft\AppVISV" or "HKCU\SOFTWARE\Microsoft\AppVISV"
// Key is package root in lower case, without trailing "\root\..."
// Where package root is the full path to AppvIsvSubsystems64.dll or main EXE

// AppvIsvSubsystems64.dll is symlinked into Office root in various places

typedef int (WINAPI * APIExportForDetours_t)(); // Ordinal #1, Do nothing but return 1
typedef BOOLEAN (WINAPI * RequestUnhookedFunctionList_t)(FARPROC pfnUnkCallback1, FARPROC pfnUnkCallback2); // TODO: Type and usage
typedef void (WINAPI * VirtualizeCurrentThread_t)(BOOL bEnable);
typedef BOOLEAN (WINAPI * CurrentThreadIsVirtualized_t)();
typedef void (WINAPI * VirtualizeCurrentProcess_t)(BOOL bEnable); // bEnable must be TRUE
typedef BOOLEAN (WINAPI * IsProcessHooked_t)();

int main()
{
	// Get Office base path: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Office\ClickToRun#PackageFolder:REG_SZ
	// AppvIsvSubsystems64.dll loads C2R64.dll
	HMODULE hmod = LoadLibraryExW(L"C:\\Program Files\\Microsoft Office\\root\\Client\\AppvIsvSubsystems64.dll", NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (!hmod)
	{
		fprintf(stderr, "LoadLibraryExW(AppvIsvSubsystems64.dll) failed, GetLastError() = %u\n", GetLastError());
		return 1;
	}

#define API(name) \
	auto name = (name##_t)GetProcAddress(hmod, #name);

	API(APIExportForDetours);
	API(RequestUnhookedFunctionList);
	API(VirtualizeCurrentThread);
	API(CurrentThreadIsVirtualized);
	API(VirtualizeCurrentProcess);
	API(IsProcessHooked);

	printf("IsProcessHooked() = %hhu\n", IsProcessHooked());
	printf("CurrentThreadIsVirtualized() = %hhu\n", CurrentThreadIsVirtualized());

	VirtualizeCurrentProcess(TRUE);

	printf("IsProcessHooked() = %hhu\n", IsProcessHooked());
	printf("CurrentThreadIsVirtualized() = %hhu\n", CurrentThreadIsVirtualized());

	// This DLL is from Office VBA and is in App-V VFS
	HMODULE fm20 = LoadLibraryW(L"FM20.DLL");
	if (!fm20)
	{
		fprintf(stderr, "LoadLibraryW(FM20.DLL) failed, GetLastError() = %u\n", GetLastError());
		return 1;
	}

	char buf[MAX_PATH] = { 0 };
	GetModuleFileNameA(fm20, buf, MAX_PATH);
	puts(buf); // "C:\Windows\SYSTEM32\FM20.DLL"
	FreeLibrary(fm20);

	FreeLibrary(hmod);
	return 0;
}
