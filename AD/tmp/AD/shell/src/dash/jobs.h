#include <inttypes.h>
#include <sys/types.h>

#define FORK_FG 0
#define FORK_BG 1
#define FORK_NOJOB 2

#define	SHOW_PGID	0x01
#define	SHOW_PID	0x04
#define	SHOW_CHANGED	0x08

struct procstat {
	pid_t	pid;
 	int	status;
 	char	*cmd;
};

struct job {
	struct procstat ps0;
	struct procstat *ps;
#if JOBS
	int stopstatus;
#endif
	uint32_t
		nprocs: 16,
		state: 8,
#define	JOBRUNNING	0
#define	JOBSTOPPED	1
#define	JOBDONE		2
#if JOBS
		sigint: 1,
		jobctl: 1,
#endif
		waited: 1,
		used: 1,
		changed: 1;
	struct job *prev_job;
};

union node;
extern pid_t backgndpid;
extern int job_warning;
#if JOBS
extern int jobctl;
#else
#define jobctl 0
#endif
extern int vforked;

void setjobctl(int);
int killcmd(int, char **);
int fgcmd(int, char **);
int bgcmd(int, char **);
int jobscmd(int, char **);
struct output;
void showjobs(struct output *, int);
int waitcmd(int, char **);
struct job *makejob(union node *, int);
int forkshell(struct job *, union node *, int);
struct job *vforkexec(union node *n, char **argv, const char *path, int idx);
int waitforjob(struct job *);
int stoppedjobs(void);

#if ! JOBS
#define setjobctl(on) ((void)(on))	
#endif
