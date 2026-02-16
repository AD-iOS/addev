#include <histedit.h>

extern History *hist;
extern EditLine *el;
extern int displayhist;

void histedit(void);
void sethistsize(const char *);
void setterm(const char *);
int histcmd(int, char **);
int not_fcnumber(char *);
int str_to_event(const char *, int);
