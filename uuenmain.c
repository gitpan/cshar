/*
**  UUENCODE
**
**  Uuencode a file.  This is based on the public-domain implementation that
**  Mark Horton released to mod.sources with the translation table written
**  by Jean-Pierre H. Dumas.
*/
#include "shar.h"
static char RCS[] =
	"$Id: uuenmain.c,v 3.0.3.2 1993/08/25 17:05:07 ram Exp $";
/*
 * $Log: uuenmain.c,v $
 * Revision 3.0.3.2  1993/08/25  17:05:07  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:40:07  ram
 * 3.0 baseline (ram).
 * 
 */


int
main(ac, av)
    int		ac;
    char	*av[];
{
    int		Oops;
    int		i;

    /* Parse JCL. */
    for (Oops = FALSE; (i = getopt(ac, av, "v")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	}
    ac -= optind;
    av += optind;

    if (Oops || ac != 2) {
	Fprintf(stderr, "Usage:\n  uuencode %s\n", "infile outfile");
	exit(1);
	/* NOTREACHED */
    }

    uuencode(av[0], av[1]);
    exit(0);
    /* NOTREACHED */
}
