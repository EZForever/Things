#include <windows.h>
#include <stdio.h>

/*
    The command `prompt` of Windows Command Prompt (hmmm) offers some "escape characters" to represent variables, like current drive or path, as well as some special chars.
    One of them is $E, which literally transfers into the "escape char", or ^[ as key-stroke, or \e , \x9b, \u001b as in C.
    For example, `prompt $E[31;40m$P$G$E[0m` gives an red prompt on black background.
    But this trick don't always work when put into a custom program; `puts("\e[1;31mRED\e[0m")` sometimes gives out mess without any colorings on it.

    After serveral tests, it turns out that only programs started on a CMD window can display colors.
    And when I try this:
    puts("\e[1;31mRED\e[0m"); system("echo \e[1;31mRED\e[0m"); puts("\e[1;31mRED\e[0m");
    We got the latter 2 lines working. In fact even only `system("")` itself is enough to "activite" the coloring function.
    That's funny and useful. But why?

    Disassembling into MSVCRT shows that what `system()` really does is just run `cmd.exe /c ...`, and `cmd /c` will just quit immediately.
    So it must in the init process of CMD. And disassembling CMD's `main()` function shows a function called `ResetConsoleMode()`.
    That's an inner function, so let's do it the other way.

    A list of console modes on Windows can be seen here:
    https://docs.microsoft.com/en-us/windows/console/setconsolemode
    With another program comparing console mode before & after launching CMD, these are what had happened:
        1. Set ENABLE_MOUSE_INPUT on stdin (not useful)
        2. Set ENABLE_VIRTUAL_TERMINAL_PROCESSING on stdout and stderr (useful)
    ENABLE_VIRTUAL_TERMINAL_PROCESSING turns on VT100 terminal emulating, which allows ANSI Escape Sequences (actual name of "\e[...") to be interpreted.

    Below shows what to do exactly to turn on that. Difficult than a single `system("")`.
    Run it from Explorer to see full effect.
*/

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING //My MinGW32-w64 don't have this
#   define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

//nStdHandle can be STD_OUTPUT_HANDLE or STD_ERROR_HANDLE
void EnableEscSeq(DWORD nStdHandle) {
    DWORD Flags;
    HANDLE hBuf = GetStdHandle(nStdHandle);
    GetConsoleMode(hBuf, &Flags);
    Flags |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hBuf, Flags);
}

int main(void) {
    //For more Escape Sequences see here:
    //https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
    puts("Before: Hello \e[1;31mRED\e[0m world!");
    getchar();
    EnableEscSeq(STD_OUTPUT_HANDLE); //system("");
    puts("After : Hello \e[1;31mRED\e[0m world!");
    getchar();
    return 0;
}