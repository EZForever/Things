#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <vector>

static HWND g_hDlg;

class DialogTemplate
{
private:
    std::vector<BYTE> m_buf;

public:
    DialogTemplate() = default;

    inline operator LPCDLGTEMPLATEW()
    {
        return (LPCDLGTEMPLATEW)m_buf.data();
    }

    inline void Reset()
    {
        m_buf.clear();
    }

    inline void Align()
    {
        while (m_buf.size() % sizeof(WORD) != 0)
        {
            m_buf.push_back(0);
        }
    }

    template<class T>
    inline void Write(const T& value)
    {
        auto ptr = (const BYTE*)&value;
        m_buf.insert(m_buf.end(), ptr, ptr + sizeof(T));
    }
};

static INT_PTR DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        _InterlockedExchangePointer((PVOID*)&g_hDlg, hDlg);

        // TODO

        return TRUE;
    }
    case WM_DESTROY:
    {
        // TODO

        _InterlockedExchangePointer((PVOID*)&g_hDlg, NULL);
        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hDlg, 0);
            PostQuitMessage(0); // TODO: For testing only; remove in production
            return TRUE;
        default:
            return FALSE;
        }
    }
    case WM_PAINT:
    {
        // TODO

        return TRUE;
    }
    default:
        return FALSE;
    }
}

int main()
{
    DWORD dwUnits = GetDialogBaseUnits();
    WORD wUnitX = LOWORD(dwUnits);
    WORD wUnitY = HIWORD(dwUnits);
    printf("%hu %hu\n", wUnitX, wUnitY);

    // Pixel to dialog unit
    // Reversed formula from https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-mapdialogrect#remarks
    short cx = MulDiv(300, 4, wUnitX);
    short cy = MulDiv(400, 8, wUnitY);

    DialogTemplate dlg;

    // DLGTEMPLATEEX
    dlg.Write<WORD>(1); // dlgVer
    dlg.Write<WORD>(0xFFFF); // signature
    dlg.Write<DWORD>(0); // helpID
    dlg.Write<DWORD>(0); // exStyle
    dlg.Write<DWORD>(WS_VISIBLE | WS_CAPTION | WS_SYSMENU | DS_CENTER); // style
    dlg.Write<WORD>(0); // cDlgItems
    dlg.Write<short>(0); // x
    dlg.Write<short>(0); // y
    dlg.Write<short>(cx); // cx
    dlg.Write<short>(cy); // cy
    dlg.Write<WORD>(0); // menu (null)
    dlg.Write<WORD>(0); // windowClass (null)
    dlg.Write(L"Hello"); // title
#if 0 // DLGTEMPLATEEX, only when DS_SETFONT is set
    dlg.Write<WORD>(0); // pointsize
    dlg.Write<WORD>(0); // weight
    dlg.Write<BYTE>(FALSE); // italic
    dlg.Write<BYTE>(DEFAULT_CHARSET); // charset
    dlg.Write(L""); // typeface
#endif

    if (!_InterlockedCompareExchangePointer((PVOID*)&g_hDlg, (PVOID)(UINT_PTR)-1, NULL))
    {
        auto hwnd = CreateDialogIndirectW(NULL, dlg, GetConsoleWindow(), DlgProc);
        printf("%p %u\n", hwnd, GetLastError());

        if (hwnd)
        {
            MSG msg;
            while (GetMessageW(&msg, NULL, WM_NULL, WM_NULL))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        else
        {
            _InterlockedExchangePointer((PVOID*)&g_hDlg, NULL);
        }
    }

    return 0;
}

