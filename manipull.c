/*
**  MANIPULL
**
**  Extract entries out of an existing manifest.
**  This is Phil Budne's code which I have usurped and heavily modified.
*/
#include "shar.h"
static char RCS[] =
	"$Id: manipull.c,v 3.0.3.3 1993/08/25 17:04:45 ram Exp $";

/*
 * $Log: manipull.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:45  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:10  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:34:23  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Our block of information about the files we're doing.
*/
typedef struct _entry {
    char	*Name;			/* Filename			*/
    char	*Text;			/* What it is			*/
    int		Wanted;			/* Does the user want this one?	*/
} ENTRY;


#define DEF_FILECOUNT	500		/* Seems like plenty		*/


int
main(ac, av)
    int			ac;
    register char	*av[];
{
    REGISTER ENTRY	*E;
    REGISTER char	*p;
    REGISTER int	i;
    REGISTER int	Count;
    REGISTER int	Silent;
    REGISTER int	Header;
    ENTRY		*Entries;
    int			FileCount;
    int			Oops;
    char		line[BUFSIZ];
    char		*Infile;
    char		*Outfile;

    /* Parse JCL. */
    Header = 0;
    Silent = FALSE;
    Infile = NULL;
    Outfile = NULL;
    FileCount = DEF_FILECOUNT;
    for (Oops = FALSE; (i = getopt(ac, av, "vfh:i:mo:s")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	case 'f':		/* Maximum number of files to pull	*/
	    FileCount = atoi(optarg);
	    break;
	case 'h':		/* Lines of header to skip		*/
	    Header = atoi(optarg);
	    break;
	case 'i':		/* Name of input manifest		*/
	    Infile = optarg;
	    break;
	case 'm':		/* Convenient option shorthand		*/
	    Header = 2;
	    Infile = "MANIFEST";
	    Outfile = "MANIFEST.NEW";
	    break;
	case 'o':		/* Name of output manifest		*/
	    Outfile = optarg;
	    break;
	case 's':		/* Don't list what we're doing		*/
	    Silent = TRUE;
	    break;
	}
    ac -= optind;
    av += optind;

    if (ac == 0 || Oops) {
	Fprintf(stderr, "Usage:\n  manipull %s files...\n", 
		"[-m | -h2 -i MANIFEST -o MANIFEST.NEW] -f# -s");
	exit(1);
	/* NOTREACHED */
    }

    /* Open input and output streams as necessary. */
    if (Infile && freopen(Infile, "r", stdin) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for input, %s.\n",
		Infile, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }
    if (Outfile && freopen(Outfile, "r", stdout) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for output, %s.\n",
		Outfile, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }

    /* Read and store the manifest. */
    Entries = NEW(ENTRY, FileCount);
    for (E = Entries, Count = 0; fgets(line, sizeof line, stdin); ) {
	if (Header > 0) {
	    Header--;
	    continue;
	}
	if ((p = IDX(line, '\n')) == NULL)
	    Fprintf(stderr, "Line truncated!\n");
	else
	    *p = '\0';

	/* Skip leading whitespace; check for totally blank line. */
	for (p = line; *p && CTYPE(*p) && isspace(*p); )
	    p++;
	if (*p == '\0')
	    continue;

	/* Got enough room? */
	if (++Count == FileCount - 1) {
	    Fprintf(stderr, "Need more than %d files; use -f flag.\n",
		    FileCount);
	    exit(1);
	    /* NOTREACHED */
	}

	/* Copy the name, skip whitespace after it. */
	E->Name = COPY(line);
	for (p = E->Name; *p && CTYPE(*p) && !isspace(*p); )
	    p++;
	for (*p++ = '\0'; *p && CTYPE(*p) && isspace(*p); )
	    p++;

	/* Skip past the archive number. */
	while (*p && CTYPE(*p) && isdigit(*p))
	    p++;

	/* Skip whitespace. */
	while (*p && CTYPE(*p) && isspace(*p))
	    p++;

	/* Save description. */
	E->Text = p;
	E->Wanted = FALSE;
	E++;
    }

    /* Rest of command line is files to pull out of the manifest. */
    for (; *av; av++) {
	/* Awful linear search. */
	for (E = Entries, i = 0; i < Count; E++, i++)
	    if (EQ(E->Name, *av)) {
		E->Wanted = TRUE;
		break;
	    }
	if (i == Count) {
	    /* Not in MANIFEST; add it by hand. */
	    if (!Silent)
		Fprintf(stderr, "ADDING %s\n", *av);

	    /* Got enough room? */
	    if (++Count == FileCount - 1) {
		Fprintf(stderr, "Need more than %d files; use -f flag.\n",
			FileCount);
		exit(1);
		/* NOTREACHED */
	    }
	    E->Name = COPY(*av);
	    E->Text = "";
	    E->Wanted = TRUE;
	}
    }

    /* Write header, then the entries. */
    Printf("FEED THIS BACK INTO MAKEKIT FOR REAL PACKING...\n");
    Printf("   File Name		Archive #	Description\n");
    for (E = Entries, i = 0; i < Count; E++, i++)
	if (E->Wanted) {
		/* ram:
		 * Avoid useless final tabs that "maniscan" will
		 * complain about...
		 */
		if (strlen(E->Text) > 0)
			Printf(" %s\t%d\t%s\n", E->Name, 0, E->Text);
		else
			Printf(" %s\t%d\n", E->Name, 0);
	} else if (!Silent)
	    Fprintf(stderr, "SKIPPING %s\n", E->Name);

    /* That's all she wrote. */
    exit(0);
    /* NOTREACHED */
}
