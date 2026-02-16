#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "shell.h"
#include "nodes.h"
#include "exec.h"
#include "var.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "mail.h"
#include "mystring.h"


#define MAXMBOXES 10

static time_t mailtime[MAXMBOXES];
static int changed;

void chkmail(void) {
	const char *mpath;
	char *p;
	char *q;
	time_t *mtp;
	struct stackmark smark;
	struct stat64 statb;
	setstackmark(&smark);
	mpath = mpathset() ? mpathval() : mailval();
	for (mtp = mailtime; mtp < mailtime + MAXMBOXES; mtp++) {
		int len;
		len = padvance_magic(&mpath, nullstr, 2);
		if (!len)
			break;
		p = stackblock();
		if (*p == '\0')
			continue;
		for (q = p ; *q ; q++);
#ifdef DEBUG
		if (q[-1] != '/')
			abort();
#endif
		q[-1] = '\0';
		if (stat64(p, &statb) < 0) {
			*mtp = 0;
			continue;
		}
		if (!changed && statb.st_mtime != *mtp) {
			outfmt(
				&errout, snlfmt,
				pathopt ? pathopt : "you have mail"
			);
		}
		*mtp = statb.st_mtime;
	}
	changed = 0;
	popstackmark(&smark);
}


void changemail(const char *val) {
	changed++;
}
