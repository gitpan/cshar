/*
**  SHELL
**
**  Stand-alone driver for shell/shar interpreter.
*/
#include "parser.h"
static char RCS[] =
	"$Id: shell.c,v 3.0.3.3 1993/08/25 17:04:59 ram Exp $";
/*
 * $Log: shell.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:59  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:49  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:38:04  ram
 * 3.0 baseline (ram).
 * 
 */


int
main(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    char		*vec[MAX_WORDS];
    char		buff[VAR_VALUE_SIZE];
    int			Oops;
    int			i;

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

    if (Oops) {
	Fprintf(stderr, "Usage:\n  shell [file [parameters] ]\n");
	exit(1);
	/* NOTREACHED */
    }

    Interactive = ac == 0;
    if (Interactive) {
	Fprintf(stderr, "Testing shell interpreter...\n");
	Input = stdin;
	(void)strcpy(File, "stdin");
    }
    else {
	(void)strcpy(File, av[0]);
	if ((Input = fopen(av[0], "r")) == NULL)
	    SynErr("UNREADABLE INPUT");
	/* Build the positional parameters. */
	for (ac = 0; av[ac]; ac++) {
	    Sprintf(buff, "%d", ac);
	    SetVar(buff, av[ac]);
	}
    }

    /* Read, parse, and execute. */
    while (GetLine(TRUE))
	if (Argify(vec) && Exec(vec) == OOB_FALSE)
	    break;

    /* That's it. */
    (void)fclose(Input);
    exit(0);
    /* NOTREACHED */
}
