/*
**  UUDECODE
**
**  Decode printable representation of binary data.  This is based on the
**  public-domain implementation that Mark Horton released to mod.sources
**  with the translation table written by Jean-Pierre H. Dumas.
*/
#include "shar.h"
static char RCS[] =
	"$Id: uudecode.c,v 3.0.3.3 1993/08/25 17:05:03 ram Exp $";

/*
 * $Log: uudecode.c,v $
 * Revision 3.0.3.3  1993/08/25  17:05:03  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:52:05  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:39:05  ram
 * 3.0 baseline (ram).
 * 
 */

/* single character decode */
#define DEC(c)		(((c) - ' ') & 077)

#undef DEC
#define DEC(c)		(Table[(c)])

static char		Table[UUTABLE_SIZE];


/*
**  Read in the translation table.
*/
static void
GetTable(F)
    REGISTER FILE	*F;
{
    REGISTER char	*p;
    REGISTER int	 c;
    REGISTER int	 n;
    char		 buff[BUFSIZ];

    /* Clear it out; kinda messy, but one less config parameter... */
    for (p = Table, n = sizeof Table; --n >= 0; p++)
	*p = '\0';

    /* Read lines until we hit the all the data elements. */
    for (n = 0; ; ) {

	/* Get a line. */
	if (fgets(buff, sizeof buff, F) == NULL) {
	    Fprintf(stderr, "EOF in translation table, %s\n", strerror(errno));
	    exit(1);
	}
	if (EQn(buff, "begin", 5)) {
	    Fprintf(stderr, "Incomplete translation table.\n");
	    exit(1);
	}

	/* Strip trailing spaces. */
	for (p = &buff[strlen(buff)]; p > buff && (*--p == '\n' || WHITE(*p)); )
	    *p = '\0';
	/* Rest of the line is part of our table. */
	for (p = buff; (c = *p) != '\0'; p++) {
	    if (Table[c]) {
		Fprintf(stderr,
		    "Duplicate character 0%0 (%s) in translation table.\n",
		    Table[c], Seechar(Table[c]));
		exit(1);
	    }
	    Table[c] = n;
	    if (++n >= UUCHAR_DATA)
		return;
	}
    }
}


int
main(ac, av)
    int			ac;
    char		*av[];
{
    REGISTER FILE	*Out;
    REGISTER FILE	*F;
    REGISTER char	*p;
    REGISTER int	n;
    REGISTER int	c;
    char		Name[128];
    char		buff[80];
    int			mode;
    int			Oops;

    /* Parse JCL. */
    for (Oops = FALSE; (n = getopt(ac, av, "v")) != EOF; )
	switch (n) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	}
    ac -= optind;
    av += optind;

    if (!Oops)
	switch (ac) {
	default:
	    Oops = TRUE;
	    break;
	case 0:
	    F = stdin;
	    break;
	case 1:
	    if ((F = fopen(*av, "r")) == NULL) {
		Fprintf(stderr, "Can't open \"%s\" for input, %s.\n",
			*av, strerror(errno));
		exit(1);
		/* NOTREACHED */
	    }
	    break;
	}

    if (Oops) {
	Fprintf(stderr, "Usage:\n  uudecode %s\n", "[infile]");
	exit(1);
	/* NOTREACHED */
    }

    /* Set up default translation table. */
    for (n = 0, p = &Table[n]; n < ' '; n++)
	*p++ = '\0';
    for (c = 0; n < ' ' + UUCHAR_DATA; n++)
	*p++ = c++;
    for (; n < UUTABLE_SIZE; n++)
	*p++ = '\0';
    /* Space and backquote are same; 4.3 started using backquote to avoid
     * BITNET lossage of removing trailing spaces.  Uparrow and tilde are
     * another common BITNET loss, which also luckily map to the same thing. */
    Table['`'] = Table[' '];
    Table['~'] = Table['^'];

    /* Get header line. */
    do {
	if (fgets(buff, sizeof buff, F) == NULL) {
	    Fprintf(stderr, "Can't find \"begin\" line, %s.\n",
		    strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	}
	if (EQn(buff, "table", 5))
	    GetTable(F);
    } while (!EQn(buff, "begin ", 6));
    if (sscanf(buff, "begin %o %s", &mode, Name) != 2) {
	Fprintf(stderr, "Malformed \"begin\" line.\n");
	exit(1);
	/* NOTREACHED */
    }

    /* Create output file. */
    if ((Out = fopen(Name, "w")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for output, %s.\n",
		Name, strerror(errno));
	perror(Name);
	exit(1);
	/* NOTREACHED */
    }

    if (chmod(Name, mode) < 0)
	Fprintf(stderr, "Warning, can't chmod(\"%s\", 0%o), %s.\n",
		Name, mode, strerror(errno));

    /* Read and munch. */
    for ( ; ; ) {
	/* Read a line; zero datacount means end of data. */
	if (fgets(buff, sizeof buff, F) == NULL) {
	    Fprintf(stderr, "Short file, %s.\n", strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	}
	if ((n = DEC(buff[0])) <= 0)
	    break;

	/* Decode it. */
	for (p = &buff[1]; n > 0; p += 4, n -= 3) {
	    c = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
	    (void)putc(c, Out);
	    if (n >= 2) {
		c = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
		(void)putc(c, Out);
		if (n >= 3) {
		    c = DEC(p[2]) << 6 | DEC(p[3]);
		    (void)putc(c, Out);
		}
	    }
	}
    }

    /* Check for good ending. */
    if (fgets(buff, sizeof buff, F) == NULL || !EQ(buff, "end\n")) {
	Fprintf(stderr, "No end line\n");
	exit(1);
	/* NOTREACHED */
    }

    /* That's all she wrote. */
    (void)fclose(Out);
    (void)fclose(F);
    exit(0);
    /* NOTREACHED */
}
