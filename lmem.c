/*
**  Get some memory or die trying.
*/
/* LINTLIBRARY */
#include "shar.h"
#include "patchlog.h"

/* ram:
 * I keep track of my own patchlevel.
 */
#undef PATCHLEVEL
#include "patchlevel.h"

#ifdef	RCSID
static char RCS[] =
	"$Id: lmem.c,v 3.0.3.4 1993/08/25 12:40:08 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: lmem.c,v $
 * Revision 3.0.3.4  1993/08/25  12:40:08  ram
 * patch9: cleanup
 *
 * Revision 3.0.3.3  91/04/19  10:14:30  ram
 * patch5: the '-v' option will now give correct information
 * 
 * Revision 3.0.3.2  91/04/07  18:50:52  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:33:32  ram
 * 3.0 baseline (ram).
 * 
 */

align_t
shar_getmem(i, j)
    int			i;
    unsigned int	j;
{
#ifndef	ANSI_HDRS
    extern char		*malloc();
#endif	/* ANSI_HDRS */
    align_t		p;

    /* NOTSTRICT: "possible pointer alignment problem." */
    if ((p = (align_t)malloc(i * j)) == NULL) {
	/* Print the unsigned values as int's so ridiculous values show up. */
	Fprintf(stderr, "Can't getmem(%d,%d), %s.\n", i, j, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }
    return p;
}


/*
**  Print out the version string and exit.  Also print out the RCS string
**  for the heck of it.
*/
void
Version(p)
    char	*p;
{
    Printf(
      "This is Version %s, patchlevel %d of the public-domain cshar package\n",
      VERSION, PATCHLEVEL);
	Printf("(version modified by Raphael Manfredi)\n");
    Printf("%s\n", p);
    exit(0);
    /* NOTREACHED */
}
