#include <sys/param.h>

#ifndef JOBS
#define JOBS 1
#endif
#ifndef BSD
#define BSD 1
#endif

#ifndef DO_SHAREDVFORK
#if __NetBSD_Version__ >= 104000000
#define DO_SHAREDVFORK
#endif
#endif

typedef void *pointer;
#ifndef NULL
#define NULL (void *)0
#endif
#define STATIC static
#define MKINIT	/* empty */

extern char nullstr[1];


#ifdef DEBUG
#define TRACE(param)	trace param
#define TRACEV(param)	tracev param
#else
#define TRACE(param)
#define TRACEV(param)
#endif

#if defined(__GNUC__) && __GNUC__ < 3
#define va_copy __va_copy
#endif

#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)	__builtin_expect(!!(x),1)
#define unlikely(x)	__builtin_expect(!!(x),0)

static inline int max_int_length(int bytes) {
	return (bytes * 8 - 1) * 0.30102999566398119521 + 14;
}
