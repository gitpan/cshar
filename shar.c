/*
**  SHAR
**
**  Make a shell archive of a list of files.
*/
#include "shar.h"
static char RCS[] =
	"$Id: shar.c,v 3.0.3.3 1993/08/25 17:04:56 ram Exp $";
/*
 * $Log: shar.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:56  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:35  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:37:10  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Minimum allocation of file name pointers used in "-i" option processing.
*/
#define	MIN_FILES	50


/*
**  This prolog is output before the archive.
*/
static char	 *Prolog[] = {
  "! /bin/sh",
  " This is a shell archive.  Remove anything before this line, then feed it",
  " into a shell via \"sh file\" or similar.  To overwrite existing files,",
  " type \"sh file -c\".",
#ifdef	PEDAGOGY
  " The tool that generated this appeared in the comp.sources.unix newsgroup;",
  " send mail to comp-sources-unix@uunet.uu.net if you want that tool.",
#endif	/* PEDAGOGY */
  NULL
};


/*
**  Package up one file or directory.
*/
static void
shar(file, Basename)
    char		*file;
    int			Basename;
{
    REGISTER FILE	*F;
    REGISTER char	*s;
    REGISTER char	*Name;
    REGISTER int	Bads;
    REGISTER off_t	Size;
    REGISTER long	l;
    int			Fixit;
    int			FixedCount;
    char		buff[BUFSIZ];

    /* Just in case. */
    if (EQ(file, ".") || EQ(file, "..") || file[0] == '\0')
	return;

    Size = Fsize(file);
    Name = Basename && (Name = RDX(file, '/')) ? Name + 1 : file;

    /* Making a directory? */
    if (Ftype(file) == F_DIR) {
	s = &file[strlen(file) - 1];
	if (*s == '/')
	    *s = '\0';
	Printf("if test ! -d '%s' ; then\n", Name);
	Printf("    echo shar: Creating directory \\\"'%s'\\\"\n", Name);
	Printf("    mkdir '%s'\n", Name);
	Printf("fi\n");
    }
    else {
	if ((F = fopen(file, "r")) == NULL) {
	    Fprintf(stderr, "Can't open \"%s\" %s\n", file, strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	}

	/* Emit the per-file prolog. */
	Printf("if test -f '%s' -a \"${1}\" != \"-c\" ; then \n", Name);
	Printf("  echo shar: Will not clobber existing file \\\"'%s'\\\"\n",
	    Name);
	Printf("else\n");
	Printf("  echo shar: Extracting \\\"'%s'\\\" \\(%ld character%s\\)\n",
	       Name, (long)Size, Size == 1 ? "" : "s");
	Printf("  sed \"s/^X//\" >'%s' <<'END_OF_FILE'\n", Name);

	/* Output the file contents. */
	FixedCount = 0;
	for (l = 1, Bads = 0; fgets(buff, sizeof buff, F); l++) {
	    s = IDX(buff, '\n');
	    Fixit = s == NULL;
	    if (Fixit)
		Fprintf(stderr, "Warning, newline missing:\n\t%s\n", buff);

	    (void)putchar('X');
	    for (s = buff; *s; s++) {
		if (BADCHAR(*s)) {
		    Fprintf(stderr,
			"Non-printable character 0%o in line %ld of \"%s\".\n",
			*s, l, Name);
		    Bads++;
		}
		(void)putchar(*s);
	    }
	    if (Fixit) {
		(void)putchar('\n');
		FixedCount++;
	    }
	}
	(void)fclose(F);

	/* Check for missing \n at end of file. */
	Printf("END_OF_FILE\n",Name);
	if (FixedCount) {
	    Printf("  echo shar: appended %d NEWLINEs to \\\"'%s'\\\"\n",
		    FixedCount, Name);
	    Fprintf(stderr, "appended %d NEWLINEs appended to \"%s\"\n",
		    FixedCount, Name);
	    Size += FixedCount;
	 }

	/* Tell about any control characters. */
	if (Bads) {
	    Printf(
    "  echo shar: %d control character%s may be missing from \\\"'%s'\\\"\n",
		   Bads, Bads == 1 ? "" : "s", Name);
	    Fprintf(stderr, "Found %d control char%s in \"%s\"\n",
		    Bads, Bads == 1 ? "" : "s", Name);
	}

	/* Output size check. */
	Printf("  if test %ld -ne `wc -c <'%s'`; then\n", (long)Size, Name);
	Printf("    echo shar: \\\"'%s'\\\" unpacked with wrong size!\n", Name);
	Printf("  fi\n");

	/* Executable? */
	if (Fexecute(file))
	    Printf("  chmod +x '%s'\n", Name);

	Printf("  # end of '%s'\nfi\n", Name);
    }
}


/*
**  Read list of files from file.
*/
static char **
GetFiles(Name)
    char		*Name;
{
    REGISTER FILE	*F;
    REGISTER int	i;
    REGISTER int	count;
    REGISTER char	**files;
    REGISTER char	**temp;
    REGISTER int	j;
    char		buff[BUFSIZ];
    char		*p;

    /* Open the file. */
    if (EQ(Name, "-"))
	F = stdin;
    else if ((F = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for input.\n", Name);
	return NULL;
    }

    /* Get space. */
    count = MIN_FILES;
    files = NEW(char*, count);

    /* Read lines. */
    for (i = 0; fgets(buff, sizeof buff, F); ) {
	if ((p = IDX(buff, '\n')) != NULL)
	    *p = '\0';
	files[i] = COPY(buff);
	if (++i == count - 2) {
	    /* Get more space; some systems don't have realloc()... */
	    count += MIN_FILES;
	    for (temp = NEW(char*, count), j = 0; j < i; j++)
		temp[j] = files[j];
	    files = temp;
	}
    }

    /* Clean up, close up, return. */
    files[i] = NULL;
    (void)fclose(F);
    return files;
}


int
main(ac, av)
    int			ac;
    REGISTER char	*av[];
{
    REGISTER char	*Trailer;
    REGISTER char	*p;
    REGISTER char	*q;
    REGISTER int	i;
    REGISTER int	length;
    REGISTER int	Oops;
    REGISTER int	Knum;
    REGISTER int	Kmax;
    REGISTER int	Basename;
    REGISTER int	j;
    REGISTER FILE	*F;
    time_t		clock;
    char		**Flist;
    int			WithinSomething;

    /* Parse JCL. */
    Basename = 0;
    Knum = 0;
    Kmax = 0;
    Trailer = NULL;
    Flist = NULL;
    WithinSomething = FALSE;
    for (Oops = FALSE; (i = getopt(ac, av, "vbe:i:n:o:t:w")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	case 'b':		/* Just use basenames of files		*/
	    Basename = TRUE;
	    break;
	case 'e':		/* Ending kit number, the last kit	*/
	    Kmax = atoi(optarg);
	    break;
	case 'i':		/* File containing the list of files	*/
	    Flist = GetFiles(optarg);
	    break;
	case 'n':		/* Number of this shar in the kit	*/
	    Knum = atoi(optarg);
	    break;
	case 'o':		/* Output file name			*/
	    if (freopen(optarg, "w", stdout) == NULL) {
		Fprintf(stderr, "Can't open \"%s\" for output, %s.\n",
			optarg, strerror(errno));
		exit(1);
		/* NOTREACHED */
	    }
	    break;
	case 't':		/* Final note after unpacking		*/
	    Trailer = optarg;
	    break;
	}
    ac -= optind;
    av += optind;

    /* If user hasn't screwed up yet, make sure we exactly one of
     * the -i flag or files named on the command line. */
    if (!Oops
     && ((Flist == NULL && ac == 0) || (Flist != NULL && ac != 0))) {
	Fprintf(stderr,
		"What files to shar?  Use -i or command line, not both.\n");
	Oops = TRUE;
    }

    if (Oops) {
	Fprintf(stderr, "Usage:\n  shar %s [files...]\n",
		"[-v] [-b] [-o outfile] [-i infile] [-n# -e# -t'text']");
	exit(1);
	/* NOTREACHED */
    }

    /* If we didn't get the list from -i, rest of argv is the file list. */
    if (Flist == NULL)
	/* Rest of arguments are files. */
	Flist = av;

    /* Everything readable and reasonably-named? */
    for (Oops = FALSE, i = 0; (p = Flist[i]) != NULL; i++)
	if ((F = fopen(p, "r")) == NULL) {
	    Fprintf(stderr, "Can't read \"%s\", %s.\n", p, strerror(errno));
	    Oops = TRUE;
	}
	else {
	    (void)fclose(F);
	    for (; *p; p++)
		if (!CTYPE(*p)) {
		    Fprintf(stderr, "Bad character '%c' in \"%s\".\n",
			    *p, Flist[i]);
		    Oops = TRUE;
		}
	}
    if (Oops)
	exit(1);
	/* NOTREACHED */

    if (!WithinSomething) {
	/* Prolog. */
	for (i = 0; (p = Prolog[i]) != NULL; i++)
	    Printf("#%s\n", p);
	Printf("# Contents: ");
	for (length = 12, i = 0; (p = Flist[i++]) != NULL; length += j) {
	    if (Basename && (q = RDX(p, '/')))
		p = q + 1;
	    j = strlen(p) + 1;
	    if (length + j < WIDTH)
		Printf(" %s", p);
	    else {
		Printf("\n#   %s", p);
		length = 4;
	    }
	}
	Printf("\n");
	clock = time((time_t *)NULL);
	Printf("# Wrapped by %s@%s on %s", User(), Host(), ctime(&clock));
	Printf("PATH=/bin:/usr/bin:/usr/ucb ; export PATH\n");
	Printf("echo %s:\n",
	    "If this archive is complete, you will see the following message");
	if (Knum && Kmax)
	    Printf("echo '          \"shar: End of archive %d (of %d).\"'\n",
		Knum, Kmax);
	else
	    Printf("echo '          \"shar: End of archive.\"'\n");
    }

    /* Do it. */
    while (*Flist)
	shar(*Flist++, Basename);

    /* Epilog. */
    if (!WithinSomething) {
	if (Knum && Kmax) {
	    Printf("echo shar: End of archive %d \\(of %d\\).\n", Knum, Kmax);
	    Printf("cp /dev/null ark%disdone\n", Knum);
	    Printf("MISSING=\"\"\n");
	    Printf("for I in");
	    for (i = 0; i < Kmax; i++)
		Printf(" %d", i + 1);
	    Printf(" ; do\n");
	    Printf("    if test ! -f ark${I}isdone ; then\n");
	    Printf("\tMISSING=\"${MISSING} ${I}\"\n");
	    Printf("    fi\n");
	    Printf("done\n");
	    Printf("if test \"${MISSING}\" = \"\" ; then\n");
	    if (Kmax == 1)
		Printf("    echo You have the archive.\n");
	    else if (Kmax == 2)
		Printf("    echo You have unpacked both archives.\n");
	    else
		Printf("    echo You have unpacked all %d archives.\n", Kmax);
	    if (Trailer && *Trailer)
		Printf("    echo \"%s\"\n", Trailer);
	    Printf("    rm -f ark[1-9]isdone%s\n",
		   Kmax >= 9 ? " ark[1-9][0-9]isdone" : "");
	    Printf("else\n");
	    Printf("    echo You still must unpack the following archives:\n");
	    Printf("    echo \"        \" ${MISSING}\n");
	    Printf("fi\n");
	}
	else {
	    Printf("echo shar: End of archive.\n");
	    if (Trailer && *Trailer)
		Printf("echo \"%s\"\n", Trailer);
	}

	Printf("exit 0\n");
    }

    exit(0);
    /* NOTREACHED */
}
