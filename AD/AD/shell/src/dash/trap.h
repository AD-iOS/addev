#include <signal.h>

extern int trapcnt;
extern char sigmode[];
extern volatile sig_atomic_t pending_sig;
extern volatile sig_atomic_t gotsigchld;

int trapcmd(int, char **);
void setsignal(int);
void ignoresig(int);
void onsig(int);
void dotrap(void);
void setinteractive(int);
void exitshell(void) __attribute__((__noreturn__));
int decode_signal(const char *, int);
void sigblockall(sigset_t *oldmask);

static inline int have_traps(void) {
	return trapcnt;
}
