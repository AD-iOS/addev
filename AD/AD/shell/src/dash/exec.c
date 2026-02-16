#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#include "shell.h"
#include "main.h"
#include "nodes.h"
#include "parser.h"
#include "redir.h"
#include "eval.h"
#include "exec.h"
#include "builtins.h"
#include "var.h"
#include "options.h"
#include "output.h"
#include "syntax.h"
#include "memalloc.h"
#include "error.h"
#include "init.h"
#include "mystring.h"
#include "show.h"
#include "jobs.h"
#include "alias.h"
#include "system.h"


#define CMDTABLESIZE 31
#define ARB 1

struct tblentry {
	struct tblentry *next;
	union param param;
	short cmdtype;
	char rehash;
	char cmdname[ARB];
};

STATIC struct tblentry *cmdtable[CMDTABLESIZE];
STATIC int builtinloc = -1;


STATIC void tryexec(char *, char **, char **);
STATIC void printentry(struct tblentry *);
STATIC void clearcmdentry(void);
STATIC struct tblentry *cmdlookup(const char *, int);
STATIC void delete_cmd_entry(void);
STATIC void addcmdentry(char *, struct cmdentry *);
STATIC int describe_command(struct output *, char *, const char *, int);

void shellexec(char **argv, const char *path, int idx) {
	char *cmdname;
	int e;
	char **envp;
	int exerrno;
	envp = environment();
	if (strchr(argv[0], '/') != NULL) {
		tryexec(argv[0], argv, envp);
		e = errno;
	} else {
		e = ENOENT;
		while (padvance(&path, argv[0]) >= 0) {
			cmdname = stackblock();
			if (--idx < 0 && pathopt == NULL) {
				tryexec(cmdname, argv, envp);
				if (errno != ENOENT && errno != ENOTDIR)
					e = errno;
			}
		}
	}
	switch (e) {
	default:
		exerrno = 126;
		break;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		exerrno = 127;
		break;
	}
	exitstatus = exerrno;
	TRACE(("shellexec failed for %s, errno %d, suppressint %d\n",
		argv[0], e, suppressint ));
	exerror(EXEND, "%s: %s", argv[0], errmsg(e, E_EXEC));
}


STATIC void tryexec(char *cmd, char **argv, char **envp) {
	char *const path_bshell = _PATH_BSHELL;
repeat:
#ifdef SYSV
	do {
		execve(cmd, argv, envp);
	} while (errno == EINTR);
#else
	execve(cmd, argv, envp);
#endif
	if (cmd != path_bshell && errno == ENOEXEC) {
		*argv-- = cmd;
		*argv = cmd = path_bshell;
		goto repeat;
	}
}

static const char *legal_pathopt(const char *opt, const char *term, int magic) {
	switch (magic) {
	case 0:
		opt = NULL;
		break;
	case 1:
		opt = prefix(opt, "builtin") ?: prefix(opt, "func");
		break;
	default:
		opt += strcspn(opt, term);
		break;
	}
	if (opt && *opt == '%')
		opt++;
	return opt;
}

const char *pathopt;

int padvance_magic(const char **path, const char *name, int magic) {
	const char *term = "%:";
	const char *lpathopt;
	const char *p;
	char *q;
	const char *start;
	size_t qlen;
	size_t len;
	if (*path == NULL)
		return -1;
	lpathopt = NULL;
	start = *path;
	if (*start == '%' && (p = legal_pathopt(start + 1, term, magic))) {
		lpathopt = start + 1;
		start = p;
		term = ":";
	}
	len = strcspn(start, term);
	p = start + len;
	if (*p == '%') {
		size_t extra = strchrnul(p, ':') - p;
		if (legal_pathopt(p + 1, term, magic))
			lpathopt = p + 1;
		else
			len += extra;
		p += extra;
	}
	pathopt = lpathopt;
	*path = *p == ':' ? p + 1 : NULL;
	qlen = len + strlen(name) + 2;
	q = growstackto(qlen);
	if (likely(len)) {
		q = mempcpy(q, start, len);
		*q++ = '/';
	}
	strcpy(q, name);
	return qlen;
}

int hashcmd(int argc, char **argv) {
	struct tblentry **pp;
	struct tblentry *cmdp;
	int c;
	struct cmdentry entry;
	char *name;
	while ((c = nextopt("r")) != '\0') {
		clearcmdentry();
		return 0;
	}
	if (*argptr == NULL) {
		for (pp = cmdtable ; pp < &cmdtable[CMDTABLESIZE] ; pp++) {
			for (cmdp = *pp ; cmdp ; cmdp = cmdp->next) {
				if (cmdp->cmdtype == CMDNORMAL)
					printentry(cmdp);
			}
		}
		return 0;
	}
	c = 0;
	while ((name = *argptr) != NULL) {
		if ((cmdp = cmdlookup(name, 0)) &&
		    (cmdp->cmdtype == CMDNORMAL ||
		     (cmdp->cmdtype == CMDBUILTIN &&
		      !(cmdp->param.cmd->flags & BUILTIN_REGULAR) &&
		      builtinloc > 0)))
			delete_cmd_entry();
		find_command(name, &entry, DO_ERR, pathval());
		if (entry.cmdtype == CMDUNKNOWN)
			c = 1;
		argptr++;
	}
	return c;
}

STATIC void printentry(struct tblentry *cmdp) {
	int idx;
	const char *path;
	char *name;
	idx = cmdp->param.index;
	path = pathval();
	do {
		padvance(&path, cmdp->cmdname);
	} while (--idx >= 0);
	name = stackblock();
	out1str(name);
	out1fmt(snlfmt, cmdp->rehash ? "*" : nullstr);
}

void find_command(char *name, struct cmdentry *entry, int act, const char *path) {
	struct tblentry *cmdp;
	int idx;
	int prev;
	char *fullname;
	struct stat64 statb;
	int e;
	int updatetbl;
	struct builtincmd *bcmd;
	int len;
	if (strchr(name, '/') != NULL) {
		entry->u.index = -1;
		if (act & DO_ABS) {
			while (stat64(name, &statb) < 0) {
#ifdef SYSV
				if (errno == EINTR)
					continue;
#endif
				entry->cmdtype = CMDUNKNOWN;
				return;
			}
		}
		entry->cmdtype = CMDNORMAL;
		return;
	}
	updatetbl = (path == pathval());
	if (!updatetbl)
		act |= DO_ALTPATH;
	if ((cmdp = cmdlookup(name, 0)) != NULL) {
		int bit;

		switch (cmdp->cmdtype) {
		default:
#if DEBUG
			abort();
#endif
		case CMDNORMAL:
			bit = DO_ALTPATH | DO_REGBLTIN;
			break;
		case CMDFUNCTION:
			bit = DO_NOFUNC;
			break;
		case CMDBUILTIN:
			bit = cmdp->param.cmd->flags & BUILTIN_REGULAR ?
			      0 : DO_REGBLTIN;
			break;
		}
		if (act & bit) {
			if (act & bit & DO_REGBLTIN)
				goto fail;

			updatetbl = 0;
			cmdp = NULL;
		} else if (cmdp->rehash == 0)
			goto success;
	}
	bcmd = find_builtin(name);
	if (bcmd && ((bcmd->flags & BUILTIN_REGULAR) | (act & DO_ALTPATH) |
		     (builtinloc <= 0)))
		goto builtin_success;

	if (act & DO_REGBLTIN)
		goto fail;
	prev = -1;
	if (cmdp && cmdp->rehash) {
		if (cmdp->cmdtype == CMDBUILTIN)
			prev = builtinloc;
		else
			prev = cmdp->param.index;
	}
	e = ENOENT;
	idx = -1;
loop:
	while ((len = padvance(&path, name)) >= 0) {
		const char *lpathopt = pathopt;
		fullname = stackblock();
		idx++;
		if (lpathopt) {
			if (*lpathopt == 'b') {
				if (bcmd)
					goto builtin_success;
				continue;
			} else if (!(act & DO_NOFUNC)) {
			} else {
				continue;
			}
		}
		if (fullname[0] == '/' && idx <= prev) {
			if (idx < prev)
				continue;
			TRACE(("searchexec \"%s\": no change\n", name));
			goto success;
		}
		while (stat64(fullname, &statb) < 0) {
#ifdef SYSV
			if (errno == EINTR)
				continue;
#endif
			if (errno != ENOENT && errno != ENOTDIR)
				e = errno;
			goto loop;
		}
		e = EACCES;
		if (!S_ISREG(statb.st_mode))
			continue;
		if (lpathopt) {
			stalloc(len);
			readcmdfile(fullname);
			if ((cmdp = cmdlookup(name, 0)) == NULL ||
			    cmdp->cmdtype != CMDFUNCTION)
				sh_error("%s not defined in %s", name,
					 fullname);
			stunalloc(fullname);
			goto success;
		}
#ifdef notdef
		if (statb.st_uid == geteuid()) {
			if ((statb.st_mode & 0100) == 0)
				goto loop;
		} else if (statb.st_gid == getegid()) {
			if ((statb.st_mode & 010) == 0)
				goto loop;
		} else {
			if ((statb.st_mode & 01) == 0)
				goto loop;
		}
#endif
		TRACE(("searchexec \"%s\" returns \"%s\"\n", name, fullname));
		if (!updatetbl) {
			entry->cmdtype = CMDNORMAL;
			entry->u.index = idx;
			return;
		}
		INTOFF;
		cmdp = cmdlookup(name, 1);
		cmdp->cmdtype = CMDNORMAL;
		cmdp->param.index = idx;
		INTON;
		goto success;
	}
	if (cmdp && updatetbl)
		delete_cmd_entry();
	if (act & DO_ERR)
		sh_warnx("%s: %s", name, errmsg(e, E_EXEC));
fail:
	entry->cmdtype = CMDUNKNOWN;
	return;
builtin_success:
	if (!updatetbl) {
		entry->cmdtype = CMDBUILTIN;
		entry->u.cmd = bcmd;
		return;
	}
	INTOFF;
	cmdp = cmdlookup(name, 1);
	cmdp->cmdtype = CMDBUILTIN;
	cmdp->param.cmd = bcmd;
	INTON;
success:
	cmdp->rehash = 0;
	entry->cmdtype = cmdp->cmdtype;
	entry->u = cmdp->param;
}

struct builtincmd * find_builtin(const char *name) {
	struct builtincmd *bp;
	bp = bsearch(
		&name, builtincmd, NUMBUILTINS, sizeof(struct builtincmd),
		pstrcmp
	);
	return bp;
}

void hashcd(void) {
	struct tblentry **pp;
	struct tblentry *cmdp;

	for (pp = cmdtable ; pp < &cmdtable[CMDTABLESIZE] ; pp++) {
		for (cmdp = *pp ; cmdp ; cmdp = cmdp->next) {
			if (cmdp->cmdtype == CMDNORMAL || (
				cmdp->cmdtype == CMDBUILTIN &&
				!(cmdp->param.cmd->flags & BUILTIN_REGULAR) &&
				builtinloc > 0
			))
				cmdp->rehash = 1;
		}
	}
}

void changepath(const char *newval) {
	const char *new;
	int idx;
	int bltin;
	new = newval;
	idx = 0;
	bltin = -1;
	for (;;) {
		if (*new == '%' && prefix(new + 1, "builtin")) {
			bltin = idx;
			break;
		}
		new = strchr(new, ':');
		if (!new)
			break;
		idx++;
		new++;
	}
	builtinloc = bltin;
	clearcmdentry();
}

STATIC void clearcmdentry(void) {
	struct tblentry **tblp;
	struct tblentry **pp;
	struct tblentry *cmdp;
	INTOFF;
	for (tblp = cmdtable ; tblp < &cmdtable[CMDTABLESIZE] ; tblp++) {
		pp = tblp;
		while ((cmdp = *pp) != NULL) {
			if (cmdp->cmdtype == CMDNORMAL ||
			    (cmdp->cmdtype == CMDBUILTIN &&
			     !(cmdp->param.cmd->flags & BUILTIN_REGULAR) &&
			     builtinloc > 0)) {
				*pp = cmdp->next;
				ckfree(cmdp);
			} else {
				pp = &cmdp->next;
			}
		}
	}
	INTON;
}

struct tblentry **lastcmdentry;


STATIC struct tblentry * cmdlookup(const char *name, int add) {
	unsigned int hashval;
	const char *p;
	struct tblentry *cmdp;
	struct tblentry **pp;
	p = name;
	hashval = (unsigned char)*p << 4;
	while (*p)
		hashval += (unsigned char)*p++;
	hashval &= 0x7FFF;
	pp = &cmdtable[hashval % CMDTABLESIZE];
	for (cmdp = *pp ; cmdp ; cmdp = cmdp->next) {
		if (equal(cmdp->cmdname, name))
			break;
		pp = &cmdp->next;
	}
	if (add && cmdp == NULL) {
		cmdp = *pp = ckmalloc(sizeof (struct tblentry) - ARB
					+ strlen(name) + 1);
		cmdp->next = NULL;
		cmdp->cmdtype = CMDUNKNOWN;
		strcpy(cmdp->cmdname, name);
	}
	lastcmdentry = pp;
	return cmdp;
}

STATIC void delete_cmd_entry(void) {
	struct tblentry *cmdp;
	INTOFF;
	cmdp = *lastcmdentry;
	*lastcmdentry = cmdp->next;
	if (cmdp->cmdtype == CMDFUNCTION)
		freefunc(cmdp->param.func);
	ckfree(cmdp);
	INTON;
}

#ifdef notdef
void getcmdentry(char *name, struct cmdentry *entry) {
	struct tblentry *cmdp = cmdlookup(name, 0);
	if (cmdp) {
		entry->u = cmdp->param;
		entry->cmdtype = cmdp->cmdtype;
	} else {
		entry->cmdtype = CMDUNKNOWN;
		entry->u.index = 0;
	}
}
#endif

STATIC void addcmdentry(char *name, struct cmdentry *entry) {
	struct tblentry *cmdp;
	cmdp = cmdlookup(name, 1);
	if (cmdp->cmdtype == CMDFUNCTION) {
		freefunc(cmdp->param.func);
	}
	cmdp->cmdtype = entry->cmdtype;
	cmdp->param = entry->u;
	cmdp->rehash = 0;
}

void defun(union node *func) {
	struct cmdentry entry;
	INTOFF;
	entry.cmdtype = CMDFUNCTION;
	entry.u.func = copyfunc(func);
	addcmdentry(func->ndefun.text, &entry);
	INTON;
}

void unsetfunc(const char *name) {
	struct tblentry *cmdp;

	if ((cmdp = cmdlookup(name, 0)) != NULL &&
	    cmdp->cmdtype == CMDFUNCTION)
		delete_cmd_entry();
}

int typecmd(int argc, char **argv) {
	int i;
	int err = 0;

	for (i = 1; i < argc; i++) {
		err |= describe_command(out1, argv[i], NULL, 1);
	}
	return err;
}

STATIC int describe_command(out, command, path, verbose)
	struct output *out;
	char *command;
	const char *path;
	int verbose;
{
	struct cmdentry entry;
	struct tblentry *cmdp;
	const struct alias *ap;
	if (verbose) {
		outstr(command, out);
	}
	if (findkwd(command)) {
		outstr(verbose ? " is a shell keyword" : command, out);
		goto out;
	}
	if ((ap = lookupalias(command, 0)) != NULL) {
		if (verbose) {
			outfmt(out, " is an alias for %s", ap->val);
		} else {
			outstr("alias ", out);
			printalias(ap);
			return 0;
		}
		goto out;
	}
	if (path == NULL) {
		path = pathval();
		cmdp = cmdlookup(command, 0);
	} else {
		cmdp = NULL;
	}
	if (cmdp != NULL) {
		entry.cmdtype = cmdp->cmdtype;
		entry.u = cmdp->param;
	} else {
		find_command(command, &entry, DO_ABS, path);
	}
	switch (entry.cmdtype) {
	case CMDNORMAL: {
		int j = entry.u.index;
		char *p;
		if (j == -1) {
			p = command;
		} else {
			do {
				padvance(&path, command);
			} while (--j >= 0);
			p = stackblock();
		}
		if (verbose) {
			outfmt(
				out, " is%s %s",
				cmdp ? " a tracked alias for" : nullstr, p
			);
		} else {
			outstr(p, out);
		}
		break;
	}
	case CMDFUNCTION:
		if (verbose) {
			outstr(" is a shell function", out);
		} else {
			outstr(command, out);
		}
		break;
	case CMDBUILTIN:
		if (verbose) {
			outfmt(
				out, " is a %sshell builtin",
				entry.u.cmd->flags & BUILTIN_SPECIAL ?
					"special " : nullstr
			);
		} else {
			outstr(command, out);
		}
		break;
	default:
		if (verbose) {
			outstr(": not found\n", out);
		}
		return 127;
	}
out:
	outc('\n', out);
	return 0;
}

int commandcmd(argc, argv)
	int argc;
	char **argv;
{
	char *cmd;
	int c;
	enum {
		VERIFY_BRIEF = 1,
		VERIFY_VERBOSE = 2,
	} verify = 0;
	const char *path = NULL;
	while ((c = nextopt("pvV")) != '\0')
		if (c == 'V')
			verify |= VERIFY_VERBOSE;
		else if (c == 'v')
			verify |= VERIFY_BRIEF;
#ifdef DEBUG
		else if (c != 'p')
			abort();
#endif
		else
			path = defpath;
	cmd = *argptr;
	if (verify && cmd)
		return describe_command(out1, cmd, path, verify - VERIFY_BRIEF);
	return 0;
}
