/*
**  MANISCAN
**
**  Read a manifest, looking for large files and those with binary data.
*/
#include "shar.h"
#include <signal.h>		/* for signal catching */
static char RCS[] =
	"$Id: maniscan.c,v 3.0.3.4 1993/08/25 17:04:50 ram Exp $";
/*
 * $Log: maniscan.c,v $
 * Revision 3.0.3.4  1993/08/25  17:04:50  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.3  91/04/19  10:18:59  ram
 * patch5: now call clean_on_sig() to exit, thus cleaning tmp files
 * patch5: missed a isspace() call to determine uu encoding
 * 
 * Revision 3.0.3.2  91/04/07  18:51:15  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:34:51  ram
 * 3.0 baseline (ram).
 * 
 */

#ifdef	HAVE_SYSTEM
#define ARGS	"vc:ef:h:i:l:mno:s:t:w:"
#else
#define ARGS	"vef:h:i:l:mno:s:t:w:"
#endif	/* HAVE_SYSTEM */

/*
**  Global variables.
*/
static long	Size;			/* Bigger than this is bad	*/
#ifdef	HAVE_SYSTEM
static char	*Command;		/* Command to create file	*/
#endif	/* HAVE_SYSTEM */
static int	Width = SAFE_WIDTH + 1; /* Longest safe linelength	*/
static int	TrailWhite = TRUE;	/* Check for trailing spaces?	*/
static FILE	*Logfile;		/* Where to record problems	*/
static int	NameLength;		/* Limit length of filenames?	*/
static char	TempName[TEMPSIZE];	/* Temporary file we use */


/*
**	Do some cleaning (temporary files)
*/
void
tmp_clean()
{
	(void) unlink(TempName);	/* remove temporary file */
}

/*
**	Signal handler
**	We get here when a signal is caught. If TempName holds
**	a valid name, remove it.
**	Note that TempName was made global because of this routine.
*/
static sigret_t
clean_on_sig()
{
	if (strlen(TempName) > 0)
		tmp_clean();	/* clean temporary file if needed */
	
	exit(1);		/* Signals: die under abnormal conditions */
	/* NOTREACHED */
}

/*
**  Safely add an extension to a filename.
*/
static void
AddExtension(Name, Suffix, Buffer)
    char	*Name;
    char	*Suffix;
    char	*Buffer;
{
    if (NameLength == 0 || strlen(Name) + 1 + strlen(Suffix) < NameLength)
	Sprintf(Buffer, "%s.%s", Name, Suffix);
    else
	Sprintf(Buffer, "%.*s%s", NameLength - strlen(Suffix), Name, Suffix);
}


/*
**  Note that we have to run uudecode.
*/
static int
UUnote(Smudged, Name, Newname)
    int			Smudged;
    char		*Name;
    char		*Newname;
{
    if (Logfile) {
	if (!Smudged) {
	    Smudged = TRUE;
	    Fprintf(Logfile, "\n");
	}
	Fprintf(Logfile, "# Run uudecode to create \"%s\":\n", Name);
	Fprintf(Logfile, "\tuudecode %s\n", Newname);
    }
    return Smudged;
}


/*
**  Do the grunge work of checking a file.  Note that we scan stuff more
**  than once because, e.g., after it's been uuencoded and edited it
**  might be too long.
*/
static int
CheckUp(Mfile, Name, Commentary)
    register FILE	*Mfile;
    char		*Name;
    char		*Commentary;
{
    REGISTER FILE	*F;
    REGISTER FILE	*Sfile;
    REGISTER char	*p;
    REGISTER int	i;
    REGISTER int	n;
    REGISTER long	s;
    char		buff[READ_CHUNK];
    char		Newname[LINE_SIZE];
    char		Oldname[LINE_SIZE];
    char		Digits[10];
    int			Encoded;
    int			Smudged;

    /* Does file exist? */
    n = !Fexists(Name);
#ifdef	HAVE_SYSTEM
    if (n && Command) {
	/* No -- try to make it. */
	(void)sprintf(buff, Command, Name);
	(void)system(buff);
	n = !Fexists(Name);
    }
#endif	/* HAVE_SYSTEM */
    if (n) {
	Fprintf(stderr, "File \"%s\" doesn't exist.\n",
		Name);
	return FALSE;
    }

    /* For now, we do nothing about directories. */
    if (Ftype(Name) == F_DIR) {
	Fprintf(Mfile, "%s%s\n", Name, Commentary);
	return FALSE;
    }

    /* Open file, read a chunk. */
    if ((F = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" to check it, %s.\n",
		Name, strerror(errno));
	return FALSE;
    }
    if ((n = fread(buff, sizeof buff[0], READ_CHUNK, F)) <= 0) {
	Fprintf(stderr, "Can't read \"%s\" to check it, %s.\n",
		Name, strerror(errno));
	(void)fclose(F);
	return FALSE;
    }

    /* Is it binary?  Rough hueristic -- one third of the chunk isn't
     * printable. */
    for (Smudged = FALSE, p = buff, i = 0; p < &buff[n]; p++)
	if (!CTYPE(*p) || !(isprint(*p) || isspace(*p)))
	    i++;
    Encoded = i > n / 3;
    if (Encoded) {
	AddExtension(Name, "UU", Newname);
	uuencode(Name, Newname);
	(void)strcpy(Oldname, Name);
	(void)strcpy(Name, Newname);
	(void)fclose(F);
	if ((F = fopen(Name, "r")) == NULL) {
	    Fprintf(stderr, "Can't open \"%s\" to check it, %s.\n",
		    Name, strerror(errno));
	    return UUnote(Smudged, Oldname, Newname);
	}
    }

    /* See if the input has any long lines, or bad characters. */
    if (!Encoded) {
	rewind(F);
	for (i = 1; fgets(buff, sizeof buff, F); i++) {
	    if ((p = IDX(buff, '\n')) != NULL)
		*p = '\0';
	    if (Width && (p == NULL || p >= &buff[Width]))
		if (Logfile) {
		    if (!Smudged) {
			Smudged = TRUE;
			Fprintf(Logfile, "\n");
		    }
		    Fprintf(Logfile, "# \"%s\", line %d: line too long\n",
			    Name, i);
		}

	    for (p = buff; *p; p++)
		if (!CTYPE(*p) || !(isprint(*p) || isspace(*p)))
		    if (Logfile) {
			if (!Smudged) {
			    Smudged = TRUE;
			    Fprintf(Logfile, "\n");
			}
			Fprintf(Logfile,
			"# \"%s\", line %d: non-printable character 0%o (%s)\n",
				Name, i, *p, Seechar(*p));
		    }

	    /* Is last character whitespace?  BITNET will choke. */
	    if (TrailWhite && p > buff) {
		p--;
		if (!CTYPE(*p) || isspace(*p))
		    if (Logfile) {
			if (!Smudged) {
			    Smudged = TRUE;
			    Fprintf(Logfile, "\n");
			}
			Fprintf(Logfile,
				"# \"%s\", line %d: ends with whitespace 0%o\n",
				Name, i, *p);
		    }
	    }
	}
    }

    (void)fclose(F);

    /* Small enough to fit? */
    if (Fsize(Name) <= Size) {
	Fprintf(Mfile, "%s%s\n", Name, Commentary);
	return Encoded ? UUnote(Smudged, Oldname, Newname) : Smudged;
    }

    /* Open input. */
    if ((F = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" to split it, %s.\n",
		Name, strerror(errno));
	return Encoded ? UUnote(Smudged, Oldname, Newname) : Smudged;
    }

    /* Open first output, write commentary. */
    i = 0;
    Sprintf(Digits, NUMB_FORMAT, ++i);
    AddExtension(Name, Digits, Newname);
    if ((Sfile = fopen(Newname, "w")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for writing, %s.\n",
		Newname, strerror(errno));
	(void)fclose(F);
	return Encoded ? UUnote(Smudged, Oldname, Newname) : Smudged;
    }
    Fprintf(Mfile, "%s%s (part %d)\n", Newname, Commentary, i);

    /* Read input, when current output gets too big, start a new file. */
    for (s = 0; fgets(buff, sizeof buff, F); ) {
	s += strlen(buff);
	if (s > Size) {
	    /* Start a new file. */
	    (void)fclose(Sfile);
	    Sprintf(Digits, NUMB_FORMAT, ++i);
	    AddExtension(Name, Digits, Newname);
	    if ((Sfile = fopen(Newname, "w")) == NULL) {
		Fprintf(stderr, "Can't open \"%s\" for writing, %s.\n",
			Newname, strerror(errno));
		(void)fclose(F);
		return Encoded ? UUnote(Smudged, Oldname, Newname) : Smudged;
	    }
	    Fprintf(Mfile, "%s%s (part %d)\n", Newname, Commentary, i);
	    s = strlen(buff);
	}
	(void)fputs(buff, Sfile);
    }

    /* Close input and current output, remove temporary if we made one. */
    (void)fclose(F);
    (void)fclose(Sfile);
    if (Encoded)
	(void)unlink(Name);

    /* Write summary message. */
    if (Logfile) {
	if (!Smudged) {
	    Smudged = TRUE;
	    Fprintf(Logfile, "\n");
	}
	Fprintf(Logfile,
	    "# \"%s\" was split into %d parts; to create it, do\n",
	    Name, i);
	if (i < 10)
	    Fprintf(Logfile, "\tcat %s.0[1-9] >%s\n", Name, Name);
	else if (i < 100)
	    Fprintf(Logfile, "\tcat %s.0[1-9] %s.[1-9][0-9] >%s\n",
		    Name, Name, Name);
	else
	    Fprintf(Logfile, "# Whatever is appropriate\n");
	/* ram:
	 * If file is executable, add a chmod in the log file.
	 */
	if (Fexecute(Name))
	    Fprintf(Logfile, "\tchmod +x %s\n", Name);
    }
    return Encoded ? UUnote(Smudged, Oldname, Newname) : Smudged;
}


int
main(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER FILE	*F;
    REGISTER char	*p;
    REGISTER int	i;
    REGISTER int	Header;
    REGISTER int	LogDirty;
    char		*InName;
    char		*OutName;
    char		*LogName;
    char		*NameStart;
    char		*Trailer;
    char		buff[BUFSIZ];
    char		Rest[LINE_SIZE];
    char		Name[LINE_SIZE];
    int			ExcludeIt;
    int			Oops;

	/* ram:
	 * First initialize the temporary file name to be NULL,
	 * because when a signal SIGHUP, SIGINT SIGQUIT or
	 * SIGTERM occurs, we go to the signal handler that will
	 * try to remove any temporary file created. A NULL name
	 * (i.e. length is 0) will be ignored.
	 */
	TempName[0] = '\0';		/* NULL name */

	/* ram:
	 * Define the signal we want to trap. If the program
	 * dies due to one of these, we first check for remaining
	 * temporary files by calling clean_on_sig().
	 */
	signal(SIGHUP, clean_on_sig);
	signal(SIGINT, clean_on_sig);
	signal(SIGQUIT, clean_on_sig);
	signal(SIGTERM, clean_on_sig);
	signal(SIGBUS, clean_on_sig);	/* should never happen ! */
	signal(SIGSEGV, clean_on_sig);	/* but man never knows... */

    /* Parse JCL. */
    ExcludeIt = FALSE;
    InName = NULL;
    OutName = NULL;
    LogName = NULL;
    Header = 0;
    Size = 50000;
    Trailer = NULL;
    for (Oops = FALSE; (i = getopt(ac, av, ARGS)) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
#ifdef	HAVE_SYSTEM
	case 'c':		/* Command to run if file doesn't exist	*/
	    Command = optarg;
	    break;
#endif	/* HAVE_SYSTEM */
	case 'e':		/* Don't include manifest in manifest	*/
	    ExcludeIt = TRUE;
	    break;
	case 'f':		/* Limit length of generated filenames	*/
	    NameLength = atoi(optarg);
	    break;
	case 'h':		/* Lines of header to skip		*/
	    Header = atoi(optarg);
	    break;
	case 'i':		/* Name of input manifest		*/
	    InName = optarg;
	    break;
	case 'l':		/* Name of packnotes log file		*/
	    LogName = optarg;
	    break;
	case 'm':		/* Convenient option shorthand		*/
	    LogName = "PACKNOTES";
	    InName = OutName = "MANIFEST";
	    Header = 2;
	    break;
	case 'n':		/* Don't check for trailing whitespace	*/
	    TrailWhite = FALSE;
	    break;
	case 'o':		/* Name for generated manifest file	*/
	    OutName = optarg;
	    break;
	case 's':		/* Maximum file size			*/
	    Size = (long)atoi(optarg);
	    if (IDX(optarg, 'k') || IDX(optarg, 'K'))
		Size *= 1024;
	    break;
	case 't':		/* Final note after unpacking		*/
	    Trailer = optarg;
	    break;
	case 'w':		/* Maximum line width			*/
	    if ((Width = atoi(optarg)) > READ_CHUNK) {
		Fprintf(stderr,
		    "Can't check widths greater than %d, option ignored.\n",
		    READ_CHUNK);
		Width = 0;
	    }
	    break;
	}
    ac -= optind;
    av += optind;

    if (ac != 0 || Oops) {
	Fprintf(stderr, "Usage:\n  maniscan %s          %s\n",
#ifdef	HAVE_SYSTEM
		"[-e] [-n] [-s#[k]] [-ttext] [-w#]",
#else
		"[-e] [-n] [-s#[k]] [-ttext] [-w#] [-c command]",
#endif	/* HAVE_SYSTEM */
		"[-m | -iMANIFEST -oMANIFEST -h2 -lPACKNOTES]");
	clean_on_sig();
	/* NOTREACHED */
    }

    /* Open the input file. */
    if (InName && freopen(InName, "r", stdin) == NULL) {
	Fprintf(stderr, "Can't read %s as manifest, %s.\n",
		InName, strerror(errno));
	clean_on_sig();
	/* NOTREACHED */
    }

    /* Open the output file. */
    if (OutName == NULL)
	F = stdout;
    else {
	MakeTempName(TempName, TEMP_NAME1);
	if ((F = fopen(TempName, "w")) == NULL) {
	    Fprintf(stderr, "Can't open '%s' for output, %s.\n",
		    TempName, strerror(errno));
	    clean_on_sig();
	    /* NOTREACHED */
	}
    }

    /* Open the log file. */
    LogDirty = FALSE;
    if (LogName) {
	if (EQ(LogName, "-")) {
	    Logfile = stderr;
	    ExcludeIt = TRUE;
	}
	else if ((Logfile = fopen(LogName, "w")) == NULL) {
	    Fprintf(stderr, "Can't open \"%s\" for output, %s.\n",
		    LogName, strerror(errno));
	    clean_on_sig();
	    /* NOTREACHED */
	}
    }

    /* Reduce size by slop so last line doesn't make us too long. */
    Size -= 2 * Width;

    /* Skip any possible prolog, then output rest of file. */
    while (--Header >= 0 && fgets(buff, sizeof buff, stdin))
	(void)fputs(buff, F);
    if (feof(stdin)) {
	Fprintf(stderr, "Nothing but header lines in the manifest.\n");
	clean_on_sig();
	/* NOTREACHED */
    }

    /* Scan rest of file. */
    while (fgets(buff, sizeof buff, stdin)) {
	if ((p = IDX(buff, '\n')) == NULL)
	    Fprintf(stderr, "Warning, line too long:\n\t%s\n", buff);
	else
	    *p = '\0';

	/* Skip leading whitespace and ignore all-blank lines. */
	for (p = buff; *p && CTYPE(*p) && isspace(*p); )
	    p++;
	if (*p == '\0')
	    continue;
	/* Save name, find end of it, stuff rest of line away. */
	for (NameStart = p; *++p && CTYPE(*p) && !isspace(*p); )
	    continue;
	i = *p;
	*p = '\0';
	(void)strcpy(Name, NameStart);
	*p = i;
	(void)strcpy(Rest, p);

	/* ram:
	 * If LogName is PACKNOTES and PACKNOTES already is in the
	 * MANIFEST, then we must not try to read it !!
	 */
	if (!(EQ(LogName, "PACKNOTES") && EQ (Name, "PACKNOTES"))
	&& CheckUp(F, Name, Rest))
	    LogDirty = TRUE;
    }

    /* Close logfile, possibly add it to the manifest. */
    if (Logfile) {
	if (Trailer && *Trailer) {
	    Fprintf(F, "# Follow these instructions:\n");
	    Fprintf(F, "\techo \"%s\"\n", Trailer);
	}
	if (Logfile != stderr)
	    (void)fclose(Logfile);
	if (LogDirty && !ExcludeIt)
	    Fprintf(F, MANI_FORMAT,
		    LogName, 1, "Warnings about long lines, etc");
    }

    /* Move temp file to new file? */
    if (OutName) {
	/* Open new file. */
	(void)fclose(F);
	if ((F = fopen(TempName, "r")) == NULL) {
	    Fprintf(stderr, "Can't open \"%s\" for reading, %s.\n",
		    Name, strerror(errno));
	    clean_on_sig();
	    /* NOTREACHED */
	}

	/* Save old manifest. */
	if (Fexists(OutName))
	    SafeRename(OutName);
	if (freopen(OutName, "w", stdout) == NULL) {
	    Fprintf(stderr, "Can't open \"%s\" for writing, %s.\n",
		    OutName, strerror(errno));
	    clean_on_sig();
	    /* NOTREACHED */
	}

	/* Copy. */
	while (fgets(buff, sizeof buff, F))
	    (void)fputs(buff, stdout);
    }

	tmp_clean();	/* remove temporary file */

    /* That's all she wrote. */
    exit(0);
    /* NOTREACHED */
}
