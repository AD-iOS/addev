#include <errno.h>

extern int rootpid;
extern int shlvl;
#define rootshell (!shlvl)

#ifdef __GLIBC__
extern int *dash_errno;
#undef errno
#define errno (*dash_errno)
#endif

void readcmdfile(char *);
int dotcmd(int, char **);
int exitcmd(int, char **);
