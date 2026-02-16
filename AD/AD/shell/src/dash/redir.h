#define REDIR_PUSH 01
#ifdef notyet
#define REDIR_BACKQ 02
#endif
#define REDIR_SAVEFD2 03

struct redirtab;
union node;
void redirect(union node *, int);
void popredir(int);
int savefd(int, int);
int redirectsafe(union node *, int);
void unwindredir(struct redirtab *stop);
struct redirtab *pushredir(union node *redir);
int sh_open(const char *pathname, int flags, int mayfail);
