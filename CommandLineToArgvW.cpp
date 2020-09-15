#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cwchar>

// Because the implementation of Microsoft's CommandLineToArgvW() DESTROYs everything that includes quotes
// NOTE: This function does NOT conform to MS' standard
wchar_t** xCommandLineToArgvW(const wchar_t* pCmdline, int* pArgc)
{
    // 1. Calculate argc
    size_t argc = 0;
    const wchar_t* pToken = pCmdline;
    bool isQuoted = false;
    while ((pToken = wcspbrk(pToken, isQuoted ? L"\"\\" : L"\" ")))
    {
        if (*pToken == L'"')
        {
            isQuoted = !isQuoted;
        }
        else if (*pToken == L'\\')
        {
            if (*(pToken + 1))
                pToken++;
        }
        else
        {
            argc++;
        }
        pToken++;
    }
    if (pCmdline[0] != L'\0')
        argc++;

    if (pArgc)
        *pArgc = (int)argc;

    // 2. Copy string into an unified buffer
    wchar_t** ret = (wchar_t**)malloc((argc + 1) * sizeof(wchar_t*) + (wcslen(pCmdline) + 1) * sizeof(wchar_t));
    wchar_t* pStr = (wchar_t*)(ret + (argc + 1));
    wcscpy_s(pStr, wcslen(pCmdline) + 1, pCmdline);

    // 3. Determine argv's poisition
    wchar_t** pArgv = ret;
    wchar_t* pTokenStr = pStr;
    isQuoted = false;
    while ((pTokenStr = wcspbrk(pTokenStr, isQuoted ? L"\"\\" : L"\" ")))
    {
        if (*pTokenStr == L'"')
        {
            isQuoted = !isQuoted;
        }
        else if (*pTokenStr == L'\\')
        {
            if (*(pTokenStr + 1))
                pTokenStr++;
        }
        else
        {
            *pTokenStr = L'\0';
            *pArgv++ = pStr;
            pStr = pTokenStr + 1;
        }
        pTokenStr++;
    }
    if (pCmdline[0] != L'\0')
        *pArgv++ = pStr;
    *pArgv++ = NULL;

    return ret;
}

int main()
{
    const wchar_t* cmdline = GetCommandLineW();
    printf("cmdline = %S\n", cmdline);

    int argc;
    wchar_t** argv = xCommandLineToArgvW(cmdline, &argc);

    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %S\n", i, argv[i]);
    }

    wchar_t** pArgv = argv;
    while (*pArgv)
    {
        _putws(*pArgv);
        pArgv++;
    }

    free(argv);
    return 0;
}

