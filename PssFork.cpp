// This is just some wrapper stuff around phnt headers
// You don't need it to understand what this code is trying (and failing) to do
//#include "ntapi.h"

#include <stdio.h>

//DECLARE_NTAPI(RtlAdjustPrivilege)
DECLARE_NTAPI(ZwCreateProcessEx)
DECLARE_NTAPI(ZwCreateThreadEx)

int main()
{
    NTSTATUS status;

#if 0
    BOOLEAN wasDebugEnabled;
    status = GET_NTAPI(RtlAdjustPrivilege)(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &wasDebugEnabled);
    if (!NT_SUCCESS(status))
    {
        fwprintf(stderr, L"RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE) failed, status = %08x\n", status);
        //return 1;
    }
#endif

    constexpr ULONG fProcess = \
        PROCESS_CREATE_FLAGS_INHERIT_HANDLES;

    HANDLE hProcChild;
    status = GET_NTAPI(ZwCreateProcessEx)(&hProcChild, PROCESS_ALL_ACCESS | 0x2000000, NULL, GetCurrentProcess(), fProcess, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        fwprintf(stderr, L"ZwCreateProcessEx() failed, status = %08x\n", status);
        return 1;
    }
    wprintf(L"hProcChild = %p\n", hProcChild);

    DWORD dwProcId = GetProcessId(hProcChild);
    if(dwProcId == 0)
    {
        fwprintf(stderr, L"GetProcessId(hProcChild) failed, GetLastError() = %u\n", GetLastError());
    }
    else
    {
        wprintf(L"dwProcId = %u\n", dwProcId);
    }

    DWORD dwExitCode;
    if (!GetExitCodeProcess(hProcChild, &dwExitCode))
    {
        fwprintf(stderr, L"GetExitCodeProcess(hProcChild) failed, GetLastError() = %u\n", GetLastError());
    }
    else
    {
        wprintf(L"dwExitCode = %u\n", dwExitCode);
    }

    // TODO: Currently STATUS_PROCESS_IS_TERMINATING
    HANDLE hThreadChild;
    status = GET_NTAPI(ZwCreateThreadEx)(&hThreadChild, THREAD_ALL_ACCESS, NULL, hProcChild, WinExec, (PVOID)"calc.exe", 0, 0, 0, 0, NULL);
    if (!NT_SUCCESS(status))
    {
        fwprintf(stderr, L"ZwCreateThreadEx() failed, status = %08x\n", status);
        //return 1;
    }
    else
    {
        wprintf(L"hProcChild = %p\n", hProcChild);
        CloseHandle(hThreadChild);
    }
    int x = getwc(stdin);

    CloseHandle(hProcChild);
    return 0;
}

