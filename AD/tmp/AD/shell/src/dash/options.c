#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#define DEFINE_OPTIONS
#include "options.h"
#undef DEFINE_OPTIONS
#include "nodes.h"
#include "eval.h"
#include "jobs.h"
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#ifndef SMALL
#include "myhistedit.h"
#endif
#include "show.h"

char *arg0;
struct shparam shellparam;
char **argptr;
char *optionarg;
char *optptr;

char *minusc;

static const char *const optnames[NOPTS] = {
	"errexit",
	"noglob",
	"ignoreeof",
	"interactive",
	"monitor",
	"noexec",
	"stdin",
	"xtrace",
	"verbose",
	"vi",
	"emacs",
	"noclobber",
	"allexport",
	"notify",
	"nounset",
	"nolog",
	"debug",
};

const char optletters[NOPTS] = {
	'e',
	'f',
	'I',
	'i',
	'm',
	'n',
	's',
	'x',
	'v',
	'V',
	'E',
	'C',
	'a',
	'b',
	'u',
	0,
	0,
};

char optlist[NOPTS];

static int options(int);
STATIC void minus_o(char *, int);
STATIC void setoption(int, int);
STATIC int getopts(char *, char *, char **);

int procargs(int argc, char **argv) {
	int i;
	const char *xminusc;
	char **xargv;
	int login;
	xargv = argv;
	login = xargv[0] && xargv[0][0] == '-';
	arg0 = xargv[0];
	if (argc > 0)
		xargv++;
	for (i = 0; i < NOPTS; i++)
		optlist[i] = 2;
	argptr = xargv;
	login |= options(1);
	xargv = argptr;
	xminusc = minusc;
	if (*xargv == NULL) {
		if (xminusc)
			sh_error("-c requires an argument");
		sflag = 1;
	}
	if (iflag == 2 && sflag == 1 && isatty(0) && isatty(1))
		iflag = 1;
	if (mflag == 2)
		mflag = iflag;
	for (i = 0; i < NOPTS; i++)
		if (optlist[i] == 2)
			optlist[i] = 0;
#if DEBUG == 2
	debug = 1;
#endif
	if (xminusc) {
		minusc = *xargv++;
		if (*xargv)
			goto setarg0;
	} else if (!sflag) {
		setinputfile(*xargv, 0);
setarg0:
		arg0 = *xargv++;
	}
	shellparam.p = xargv;
	shellparam.optind = 1;
	shellparam.optoff = -1;
	while (*xargv) {
		shellparam.nparam++;
		xargv++;
	}
	optschanged();
	return login;
}


void optschanged(void) {
#ifdef DEBUG
	opentrace();
#endif
	setinteractive(iflag);
#ifndef SMALL
	histedit();
#endif
	setjobctl(mflag);
}

STATIC int options(int cmdline) {
	char *p;
	int val;
	int c;
	int login = 0;
	if (cmdline)
		minusc = NULL;
	while ((p = *argptr) != NULL) {
		argptr++;
		if ((c = *p++) == '-') {
			val = 1;
                        if (p[0] == '\0' || (p[0] == '-' && p[1] == '\0')) {
                                if (!cmdline) {
                                        if (p[0] == '\0')
                                                xflag = vflag = 0;
                                        else if (*argptr == NULL)
						setparam(argptr);
                                }
				break;
			}
		} else if (c == '+') {
			val = 0;
		} else {
			argptr--;
			break;
		}
		while ((c = *p++) != '\0') {
			if (c == 'c' && cmdline) {
				minusc = p;
			} else if (c == 'l' && cmdline) {
				login = 1;
			} else if (c == 'o') {
				minus_o(*argptr, val);
				if (*argptr)
					argptr++;
			} else {
				setoption(c, val);
			}
		}
	}
	return login;
}

STATIC void minus_o(char *name, int val) {
	int i;
	if (name == NULL) {
		if (val) {
			out1str("Current option settings\n");
			for (i = 0; i < NOPTS; i++)
				out1fmt("%-16s%s\n", optnames[i],
					optlist[i] ? "on" : "off");
		} else {
			for (i = 0; i < NOPTS; i++)
				out1fmt("set %s %s\n",
					optlist[i] ? "-o" : "+o",
					optnames[i]);
		}
	} else {
		for (i = 0; i < NOPTS; i++)
			if (equal(name, optnames[i])) {
				optlist[i] = val;
				return;
			}
		sh_error("Illegal option -o %s", name);
	}
}

STATIC void setoption(int flag, int val) {
	int i;
	for (i = 0; i < NOPTS; i++)
		if (optletters[i] == flag) {
			optlist[i] = val;
			if (val) {
				if (flag == 'V')
					Eflag = 0;
				else if (flag == 'E')
					Vflag = 0;
			}
			return;
		}
	sh_error("Illegal option -%c", flag);
	/* NOTREACHED */
}

void setparam(char **argv) {
	char **newparam;
	char **ap;
	int nparam;
	for (nparam = 0 ; argv[nparam] ; nparam++);
	ap = newparam = ckmalloc((nparam + 1) * sizeof *ap);
	while (*argv) {
		*ap++ = savestr(*argv++);
	}
	*ap = NULL;
	freeparam(&shellparam);
	shellparam.malloc = 1;
	shellparam.nparam = nparam;
	shellparam.p = newparam;
	shellparam.optind = 1;
	shellparam.optoff = -1;
}

void freeparam(volatile struct shparam *param) {
	char **ap;
	if (param->malloc) {
		for (ap = param->p ; *ap ; ap++)
			ckfree(*ap);
		ckfree(param->p);
	}
}

int shiftcmd(int argc, char **argv) {
	int n;
	char **ap1, **ap2;
	n = 1;
	if (argc > 1)
		n = number(argv[1]);
	if (n > shellparam.nparam)
		sh_error("can't shift that many");
	INTOFF;
	shellparam.nparam -= n;
	for (ap1 = shellparam.p ; --n >= 0 ; ap1++) {
		if (shellparam.malloc)
			ckfree(*ap1);
	}
	ap2 = shellparam.p;
	while ((*ap2++ = *ap1++) != NULL);
	shellparam.optind = 1;
	shellparam.optoff = -1;
	INTON;
	return 0;
}

int setcmd(int argc, char **argv) {
	if (argc == 1)
		return showvars(nullstr, 0, VUNSET);
	INTOFF;
	options(0);
	optschanged();
	if (*argptr != NULL) {
		setparam(argptr);
	}
	INTON;
	return 0;
}


void getoptsreset(value)
	const char *value;
{
	shellparam.optind = number(value) ?: 1;
	shellparam.optoff = -1;
}

int getoptscmd(int argc, char **argv) {
	char **optbase;
	if (argc < 3)
		sh_error("Usage: getopts optstring var [arg]");
	else if (argc == 3) {
		optbase = shellparam.p;
		if ((unsigned)shellparam.optind > shellparam.nparam + 1) {
			shellparam.optind = 1;
			shellparam.optoff = -1;
		}
	}
	else {
		optbase = &argv[3];
		if ((unsigned)shellparam.optind > argc - 2) {
			shellparam.optind = 1;
			shellparam.optoff = -1;
		}
	}
	return getopts(argv[1], argv[2], optbase);
}

STATIC int getopts(char *optstr, char *optvar, char **optfirst) {
	char *p, *q;
	char c = '?';
	int done = 0;
	char s[2];
	char **optnext;
	int ind = shellparam.optind;
	int off = shellparam.optoff;
	shellparam.optind = -1;
	optnext = optfirst + ind - 1;
	if (ind <= 1 || off < 0 || strlen(optnext[-1]) < off)
		p = NULL;
	else
		p = optnext[-1] + off;
	if (p == NULL || *p == '\0') {
		p = *optnext;
		if (p == NULL || *p != '-' || *++p == '\0') {
atend:
			p = NULL;
			done = 1;
			goto out;
		}
		optnext++;
		if (p[0] == '-' && p[1] == '\0')
			goto atend;
	}
	c = *p++;
	for (q = optstr; *q != c; ) {
		if (*q == '\0') {
			if (optstr[0] == ':') {
				s[0] = c;
				s[1] = '\0';
				setvar("OPTARG", s, 0);
			} else {
				outfmt(&errout, "Illegal option -%c\n", c);
				(void) unsetvar("OPTARG");
			}
			c = '?';
			goto out;
		}
		if (*++q == ':')
			q++;
	}
	if (*++q == ':') {
		if (*p == '\0' && (p = *optnext) == NULL) {
			if (optstr[0] == ':') {
				s[0] = c;
				s[1] = '\0';
				setvar("OPTARG", s, 0);
				c = ':';
			} else {
				outfmt(&errout, "No arg for -%c option\n", c);
				(void) unsetvar("OPTARG");
				c = '?';
			}
			goto out;
		}
		if (p == *optnext)
			optnext++;
		setvar("OPTARG", p, 0);
		p = NULL;
	} else
		setvar("OPTARG", nullstr, 0);
out:
	ind = optnext - optfirst + 1;
	setvarint("OPTIND", ind, VNOFUNC);
	s[0] = c;
	s[1] = '\0';
	setvar(optvar, s, 0);
	shellparam.optoff = p ? p - *(optnext - 1) : -1;
	shellparam.optind = ind;
	return done;
}

int nextopt(const char *optstring) {
	char *p;
	const char *q;
	char c;

	if ((p = optptr) == NULL || *p == '\0') {
		p = *argptr;
		if (p == NULL || *p != '-' || *++p == '\0')
			return '\0';
		argptr++;
		if (p[0] == '-' && p[1] == '\0')
			return '\0';
	}
	c = *p++;
	for (q = optstring ; *q != c ; ) {
		if (*q == '\0')
			sh_error("Illegal option -%c", c);
		if (*++q == ':')
			q++;
	}
	if (*++q == ':') {
		if (*p == '\0' && (p = *argptr++) == NULL)
			sh_error("No arg for -%c option", c);
		optionarg = p;
		p = NULL;
	}
	optptr = p;
	return c;
}
