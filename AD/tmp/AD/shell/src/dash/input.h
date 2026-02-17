enum {
	INPUT_PUSH_FILE = 1,
	INPUT_NOFILE_OK = 2,
};

struct alias;

struct strpush {
	struct strpush *prev;
	char *prevstring;
	int prevnleft;
	struct alias *ap;
	struct strpush *spfree;
	int lastc[2];
	int unget;
};

struct parsefile {
	struct parsefile *prev;
	int linno;
	int fd;
	int nleft;
	int lleft;
	char *nextc;
	char *buf;
	struct strpush *strpush;
	struct strpush basestrpush;
	struct strpush *spfree;
	int lastc[2];
	int unget;
};

extern struct parsefile *parsefile;

#define plinno (parsefile->linno)

int pgetc(void);
int pgetc2(void);
void pungetc(void);
void pushstring(char *, void *);
int setinputfile(const char *, int);
void setinputstring(char *);
void popfile(void);
void unwindfiles(struct parsefile *);
void popallfiles(void);
