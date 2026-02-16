#include <stddef.h>
#include <stdlib.h>

struct stackmark {
	struct stack_block *stackp;
	char *stacknxt;
	size_t stacknleft;
};

extern char *stacknxt;
extern size_t stacknleft;
extern char *sstrend;

pointer ckmalloc(size_t);
pointer ckrealloc(pointer, size_t);
char *savestr(const char *);
pointer stalloc(size_t);
void stunalloc(pointer);
void pushstackmark(struct stackmark *mark, size_t len);
void setstackmark(struct stackmark *);
void popstackmark(struct stackmark *);
void *growstackstr(void);
char *growstackto(size_t len);
char *makestrspace(size_t, char *);
char *stnputs(const char *, size_t, char *);
char *stputs(const char *, char *);

static inline void grabstackblock(size_t len) {
	stalloc(len);
}

static inline char *_STPUTC(int c, char *p) {
	if (p == sstrend)
		p = growstackstr();
	*p++ = c;
	return p;
}

#define stackblock() ((void *)stacknxt)
#define stackblocksize() stacknleft
#define STARTSTACKSTR(p) ((p) = stackblock())
#define STPUTC(c, p) ((p) = _STPUTC((c), (p)))
#define CHECKSTRSPACE(n, p) \
	({ \
		char *q = (p); \
		size_t l = (n); \
		size_t m = sstrend - q; \
		if (l > m) \
			(p) = makestrspace(l, q); \
		0; \
	})
#define USTPUTC(c, p)	(*p++ = (c))
#define STACKSTRNUL(p)	((p) == sstrend? (p = growstackstr(), *p = '\0') : (*p = '\0'))
#define STUNPUTC(p)	(--p)
#define STTOPC(p)	p[-1]
#define STADJUST(amount, p)	(p += (amount))

#define grabstackstr(p)	stalloc((char *)(p) - (char *)stackblock())
#define ungrabstackstr(s, p) stunalloc((s))
#define stackstrend() ((void *)sstrend)

#define ckfree(p)	free((pointer)(p))
