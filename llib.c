/*
**  Some systems will need these routines because they're missing from
**  their C library.  This is also a catch-all for miscellaneous routines.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Id: llib.c,v 3.0.3.3 1993/08/25 17:04:40 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: llib.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:40  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:50:47  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:33:18  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Return the printable representation of a character.  Assumes ASCII,
**  but that's ok, it's all we use for portability.
*/
char *
Seechar(c)
    register int	c;
{
    static char		buff1[20];
    static char		buff2[20];
    register char	*p;
    int			meta;

    meta = c & 0200;
    c &= 0177;
    if (c >= 001 && c <= 032) {
	Sprintf(buff1, "CTRL-%c", c - 1 + 'A');
	p = buff1;
    }
    else if (c >= 040 && c <= 0176) {
	buff1[0] = c;
	buff1[1] = '\0';
	p = buff1;
    }
    else
	switch (c) {
	default:	p = "<BAD>";	break;
	case 000:	p = "NUL";	break;
	case 033:	p = "ESCAPE";	break;
	case 034:	p = "CTRL-\\";	break;
	case 035:	p = "CTRL-]";	break;
	case 036:	p = "CTRL-^";	break;
	case 037:	p = "CTRL-_";	break;
	case 0177:	p = "DELETE";	break;
	}
    if (!meta)
	return p;
    Sprintf(buff2, "META-%s", p);
    return buff2;
}


/*
**  Return the text string that corresponds to errno.
*/
#ifdef	NEED_STRERROR
char *
strerror(e)
    int			e;
{
    static char		buff[30];

#ifdef	USE_SYSERRLIST
    if (e > 0 && e < sys_nerr)
	return sys_errlist[e];
#endif	/* USE_SYSERRLIST */
    Sprintf(buff, "Error code %d", e);
    return buff;
}
#endif	/* NEED_STRERROR */


#ifdef	NEED_CTERMID
/*
**  Return a pointer to a device that can be fopen'd to query the user,
**  regardless of whether or not stdin is redirected.
*/
char *
ctermid(p)
    char	*p;
{
    static char	buff[L_ctermid];

    return strcpy(p ? p : buff, THE_TTY);
}
#endif	/* NEED_CTERMID */


#ifdef	NEED_MKDIR
/*
**  Quick and dirty mkdir routine for systems without one.
*/
int
mkdir(name, mode)
    char	*name;
    int		mode;
{
    char	*av[3];
    int		i;
    int		U;

    av[0] = "mkdir";
    av[1] = name;
    av[2] = NULL;
    U = umask(~mode);
    i = Execute(av);
    (void)umask(U);
    return i ? -1 : 0;
}
#endif	/* NEED_MKDIR */


#ifdef	UNOS_MAKEDIR
/*
**  UNOS has makedir(), but not mkdir...
*/
#include <sys/stat.h>
#include <errno.h>


int
mkdir(path, mode)
    char	*path;
    int		mode;
{
    struct stat	Sb;

    /* See if it exists, or fails for other than non-existance. */
    if (stat(path, &Sb) == 0) {
	errno = EEXIST;
	return -1;
    }
    if (errno != ENOENT)
	return -1; 

    /* UNOS makedir doesn't set the mode, so we have to do it. */
    return makedir(path) == -1 ? -1 : chmod(path, mode);
}
#endif	/* UNOS_MAKEDIR */


#ifdef	NEED_QSORT
/*
**  Bubble sort an array of arbitrarily-sized elements.  This routine
**  can be used as an (inefficient) replacement for the Unix qsort
**  routine.
*/
qsort(Table, Number, Width, Compare)
    REGISTER char	*Table;
    REGISTER int	Number;
    REGISTER int	Width;
    REGISTER int	(*Compare)();
{
    REGISTER char	*i;
    REGISTER char	*j;

    for (i = &Table[Number * Width]; (i -= Width) >= &Table[Width]; )
	for (j = i; (j -= Width) >= &Table[0]; )
	    if ((*Compare)(i, j) < 0) {
		REGISTER char	*p;
		REGISTER char	*q;
		REGISTER int	 t;
		REGISTER int	 w;

		/* Swap elements pointed to by i and j. */
		for (w = Width, p = i, q = j; --w >= 0; *p++ = *q, *q++ = t)
		    t = *p;
	    }
}
#endif	/* NEED_QSORT */


#ifdef	NEED_GETOPT

#define TYPE	int

#define ERR(s, c)					\
    if (opterr) {					\
	char buff[2];					\
	buff[0] = c; buff[1] = '\n';			\
	(void)write(2, av[0], (TYPE)strlen(av[0]));	\
	(void)write(2, s, (TYPE)strlen(s));		\
	(void)write(2, buff, 2);			\
    }

int	 opterr = 1;
int	 optind = 1;
int	 optopt;
char	*optarg;

/*
**  Return options and their values from the command line.
**  This comes from the AT&T public-domain getopt published in mod.sources
**  (i.e., comp.sources.unix before the great Usenet renaming).
*/
int
getopt(ac, av, opts)
    int			ac;
    char		*av[];
    char		*opts;
{
    static int		i = 1;
    REGISTER char	*p;

    /* Move to next value from argv? */
    if (i == 1) {
	if (optind >= ac || av[optind][0] != '-' || av[optind][1] == '\0')
	    return EOF;
	if (strcmp(av[optind], "--") == 0) {
	    optind++;
	    return EOF;
	}
    }

    /* Get next option character. */
    if ((optopt = av[optind][i]) == ':' || (p = IDX(opts,  optopt)) == NULL) {
	ERR(": illegal option -- ", optopt);
	if (av[optind][++i] == '\0') {
	    optind++;
	    i = 1;
	}
	return '?';
    }

    /* Snarf argument? */
    if (*++p == ':') {
	if (av[optind][i + 1] != '\0')
	    optarg = &av[optind++][i + 1];
	else {
	    if (++optind >= ac) {
		ERR(": option requires an argument -- ", optopt);
		i = 1;
		return '?';
	    }
	    optarg = av[optind++];
	}
	i = 1;
    }
    else {
	if (av[optind][++i] == '\0') {
	    i = 1;
	    optind++;
	}
	optarg = NULL;
    }

    return optopt;
}
#endif	/* NEED_GETOPT */
