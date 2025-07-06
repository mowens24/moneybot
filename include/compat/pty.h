// Minimal pty.h for macOS compatibility (local workaround)
// This header is provided for GUI terminal support on systems missing /usr/include/pty.h
// See: https://opensource.apple.com/source/Libc/Libc-825.40.1/include/pty.h.auto.html

#ifndef _COMPAT_PTY_H_
#define _COMPAT_PTY_H_

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int openpty(int *amaster, int *aslave, char *name,
            struct termios *termp, struct winsize *winp);
int forkpty(int *amaster, char *name, struct termios *termp,
            struct winsize *winp);

#ifdef __cplusplus
}
#endif

#endif // _COMPAT_PTY_H_
