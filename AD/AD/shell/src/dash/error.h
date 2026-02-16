#include <setjmp.h>
#include <signal.h>

#define E_OPEN  01
#define E_CREAT 02
#define E_EXEC  04

struct jmploc {
	jmp_buf loc;
};

extern struct jmploc *handler;
extern int exception;

#define EXINT   0
#define EXERROR 1
#define EXEND   3
#define EXEXIT  4

extern int suppressint;
extern volatile sig_atomic_t intpending;

#define barrier() ({ __asm__ __volatile__ ("": : :"memory"); })
#define INTOFF \
	({ \
		suppressint++; \
		barrier(); \
		0; \
	})
#ifdef REALLY_SMALL
void __inton(void);
#define INTON __inton()
#else
#define INTON \
	({ \
		barrier(); \
		if (--suppressint == 0 && intpending) onint(); \
		0; \
	})
#endif
#define FORCEINTON \
	({ \
		barrier(); \
		suppressint = 0; \
		if (intpending) onint(); \
		0; \
	})
#define SAVEINT(v) ((v) = suppressint)
#define RESTOREINT(v) \
	({ \
		barrier(); \
		if ((suppressint = (v)) == 0 && intpending) onint(); \
		0; \
	})
#define CLEAR_PENDING_INT intpending = 0
#define int_pending() intpending

void exraise(int) __attribute__((__noreturn__));
void onint(void) __attribute__((__noreturn__));
extern int errlinno;
void sh_error(const char *, ...) __attribute__((__noreturn__));
void exerror(int, const char *, ...) __attribute__((__noreturn__));
const char *errmsg(int, int);
void sh_warnx(const char *, ...);
