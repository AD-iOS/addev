#define ALIASINUSE	1
#define ALIASDEAD	2

struct alias {
	struct alias *next;
	char *name;
	char *val;
	int flag;
};

struct alias *lookupalias(const char *, int);
int aliascmd(int, char **);
int unaliascmd(int, char **);
void rmaliases(void);
int unalias(const char *);
void printalias(const struct alias *);
