/*
**  Process stuff, like fork exec and wait.  Also signals.
*/
/* LINTLIBRARY */
#include "shar.h"
#include <signal.h>
#ifdef	RCSID
static char RCS[] =
	"$Id: lexec.c,v 3.0.3.5 1993/12/21 08:49:10 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: lexec.c,v $
 * Revision 3.0.3.5  1993/12/21  08:49:10  ram
 * patch15: definition of W_VAL and WAITER now done in correct place
 *
 * Revision 3.0.3.4  1993/11/12  19:10:24  ram
 * patch14: duplicate FORK definition removed
 * patch14: a #else statement was missing
 *
 * Revision 3.0.3.3  1993/08/25  17:01:11  ram
 * patch11: forgot to patcil this one...
 *
 * Revision 3.0.3.2  91/04/07  18:50:40  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:32:37  ram
 * 3.0 baseline (ram).
 * 
 */

/* How to fork(), what to wait with. */
#ifdef	SYS_WAIT
#include <sys/wait.h>
#endif	/* SYS_WAIT */

#ifdef	HAS_VFORK
#define FORK()		 vfork()
#else
#define FORK()		 fork()
#endif	/* HAS_VFORK */

#ifdef UNION_WAIT
typedef union wait	 WAITER;
#define W_VAL(w)	 ((w).w_retcode)
#else
typedef int			 WAITER;
#define W_VAL(w)	 ((w) >> 8)
#endif



/*
**  Set up a signal handler.
*/
void
SetSigs(Func)
    sigret_t	(*Func)();
{
    if (Func == NULL)
	Func = SIG_DFL;
#ifdef	SIGINT
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	(void)signal(SIGINT, Func);
#endif	/* SIGINT */
#ifdef	SIGQUIT
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	(void)signal(SIGQUIT, Func);
#endif	/* SIGQUIT */
#ifdef	SIGTERM
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	(void)signal(SIGTERM, Func);
#endif	/* SIGTERM */
}


/*
**  Return the process ID.
*/
int
Pid()
{
#ifdef	UNIX
    static int	 X;

    if (X == 0)
	X = getpid();
    return X;
#endif	/* UNIX */
#ifdef	MSDOS
    return 1;
#endif	/* MSDOS */
#ifdef	VMS
    return 1;
#endif	/* VMS */
}


#ifndef	USE_SYSTEM
/*
**  Fork off a command.
*/
int
Execute(av)
    char		*av[];
{
    REGISTER int	i;
    REGISTER int	j;
    WAITER		W;

    if ((i = FORK()) == 0) {
	SetSigs((sigret_t (*)())NULL);
	(void)execvp(av[0], av);
	perror(av[0]);
	_exit(1);
	/* NOTREACHED */
    }

    /* We used to ignore signals around this wait, but that caused
     * surprising behaviour when someone tried to kill makekit, e.g. */
    while ((j = wait(&W)) < 0 && j != i)
	continue;
    return W_VAL(W);
}

#else

/*
**  Protect text from wildcard expansion, etc.
*/
static char *
Protect(p)
    REGISTER char	*p;
{
    char		*save;

    for (save = p; *p; p++)
	if (CTYPE(*p) && !isalpha(*p))
	    *p = '_';
    return save;
}


/*
**  Cons all the arguments together into a single command line and hand
**  it off to the shell to execute.  Gotta quote each argument after
**  the first one.
*/
int
Execute(av)
    REGISTER char	*av[];
{
    REGISTER char	**v;
    REGISTER char	*p;
    REGISTER char	*q;
    REGISTER int	i;

    /* Get length of command line. */
    for (i = 2, v = av; *v; v++)
	i += strlen(*v) + 3;

    /* Create command line and execute it. */
    p = NEW(char, i);
    q = p + strlen(strcpy(p, *v));
    for (v = &av[1]; *v; v++) {
	*q++ = ' ';
	q += strlen(strcpy(q, Protect(*v)));
    }
    *q = '\0';

    i = system(p);
    free(p);
    return i;
}
#endif	/* USE_SYSTEM */
