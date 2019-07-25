#include <windows.h>
#include <stdlib.h>

/*
    In Windows CMD, some "internal variables" exist to store sorts of internal data.
    Their names are begun with '=', so you cannot read/write them with `set` command.
    However, use `set ,` or something alike can display them.

    =C:, =D:, ...
        Store current directory on each drive letter.
        Only exist after current disk being switched onto.
        Does not overlap your working drive list; trying to switch to an invalid drive even with its variable set will fail.
        Variable of initial drive letter will be overwritten on CMD startup.

    =::
        (Maybe) the current directory of an invalid drive letter `::`, with initial value "::\".
        Notice that `::` is treated like `rem` in batch scripts, maybe by defining this CMD can get around with it easlier.
        Another fun fact: CMD's `subst` command can create drive letters with symbols, like ~: and ::.
        `::` still won't work even with :: defined by `subst :: C:\`, but `cd /d ::` works well.
        And, as expected, =:: gets modified after a `cd` command.
        Still need further experiment.

    =ExitCode
        Simillar to %ERRORLEVEL%, but only for external programs, and in a format of `%08x`.
        Only exist after a external program is called.

    Below is a PoC of one of their possible usages, which literally "disabled" D: to be switched onto.
    Trying `D:` will get you back to C:\.
    `cd /d D:\` breaks it, and by modifying =D:, makes `D:` work again.
*/

int main(void) {
    SetEnvironmentVariable("=D:", "C:\\");
    return system("cmd.exe");
}