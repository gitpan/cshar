/*
**  UNSHAR
**
**  Unpack shell archives that might have gone through mail, notes, news, etc.
**  This is Michael Mauldin's code which I have usurped and heavily modified.
*/
#include "shar.h"
static char RCS[] =
	"$Id: unshar.c,v 3.0.3.3 1993/08/25 17:05:01 ram Exp $";

/*
 * $Log: unshar.c,v $
 * Revision 3.0.3.3  1993/08/25  17:05:01  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:58  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:38:36  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Print error message and die.
*/
static void
Quit(text)
    char	*text;
{
    Fprintf(stderr, "unshar:  %s, %s.\n", text, strerror(errno));
    exit(1);
    /* NOTREACHED */
}


/*
**  Does this look like a mail header line?
*/
static int
IsHeader(p)
    REGISTER char	*p;
{
    REGISTER int	i;

    if (*p == '\0' || *p == '\n')
	return FALSE;
    if (WHITE(*p))
	return TRUE;
    for (i = 0; (CTYPE(*p) && isalnum(*p)) || HDRCHAR(*p); i++)
	p++;
    return i && *p == ':';
}


/*
**  Is this a /bin/sh comment line?  We check that because some shars
**  output comments before the CUT line.
*/
static int
IsSHcomment(p)
    REGISTER char	*p;
{
    while (CTYPE(*p) &&
      (isalpha(*p) || WHITE(*p) || *p == '\n' || *p == ',' || *p == '.'))
	p++;
    return *p == '\0';
}


/*
**  Return TRUE if p has wd1 and wd2 as words (i.e., no preceeding or
**  following letters).  Clever code, Michael.
*/
static int
Has(p, wd1, wd2)
    REGISTER char	*p;
    REGISTER char	*wd1;
    REGISTER char	*wd2;
{
    REGISTER char	*wd;
    REGISTER int	first;

    wd = wd1;
    first = TRUE;
again: 
    while (*p) {
	if (!CTYPE(*p) || !isalpha(*p)) {
	    p++;
	    continue;
	}
	while (*p++ == *wd++) {
	    if (*wd == '\0') {
		if (!CTYPE(*p) || !isalpha(*p)) {
		    if (!first)
			return TRUE;
		    first = FALSE;
		    wd = wd2;
		    goto again;
		}
		break;
	    }
	}
	while (CTYPE(*p) && isalpha(*p))
	    p++;
	wd = first ? wd1 : wd2;
    }
    return FALSE;
}


/*
**  Here's where the work gets done.  Skip headers and try to intuit
**  if the file is, e.g., C code, etc.
*/
static int
Found(Name, buff, Forced, Stream, Header)
    REGISTER char	*Name;
    REGISTER char	*buff;
    REGISTER int	Forced;
    REGISTER FILE	*Stream;
    REGISTER FILE	*Header;
{
    REGISTER char	*p;
    REGISTER int	InHeader;
    char		lower[BUFSIZ];

    if (Header)
	InHeader = TRUE;

    for ( ; ; ) {
	/* Read next line, fail if no more */
	if (fgets(buff, BUFSIZ, Stream) == NULL) {
	    Fprintf(stderr, "unshar:  No shell commands in %s.\n", Name);
	    return FALSE;
	}

	/* See if it looks like another language. */
	if (!Forced) {
	    if (PREFIX(buff, "#include") || PREFIX(buff, "# include")
	     || PREFIX(buff, "#define") || PREFIX(buff, "# define")
	     || PREFIX(buff, "#ifdef") || PREFIX(buff, "# ifdef")
	     || PREFIX(buff, "#ifndef") || PREFIX(buff, "# ifndef")
	     || (PREFIX(buff, "/*")
	      && !PREFIX(buff, NOTES1) && !PREFIX(buff, NOTES2)))
		p = "C code";
	    else if (PREFIX(buff, "(*"))		/* For vi :-) */
		p = "PASCAL code";
	    else if (buff[0] == '.'
		  && CTYPE(buff[1]) && isalpha(buff[1])
		  && CTYPE(buff[2]) && isalpha(buff[2])
		  && CTYPE(buff[3]) && !isalpha(buff[3]))
		p = "TROFF source";
	    else
		p = NULL;
	    if (p) {
		Fprintf(stderr,
			"unshar:  %s is apparently %s, not a shell archive.\n",
			Name, p);
		return FALSE;
	    }
	}

	/* Does this line start with a shell command or comment? */
	if ((buff[0] == '#' && !IsSHcomment(buff + 1))
	 || buff[0] == ':' || PREFIX(buff, "echo ")
	 || PREFIX(buff, "sed ") || PREFIX(buff, "cat ")) {
	    return TRUE;
	}

	/* Does this line say "Cut here"? */
	for (p = strcpy(lower, buff); *p; p++)
	    if (CTYPE(*p) && islower(*p))
		*p = toupper(*p);
	if (PREFIX(buff, "-----") || Has(lower, "cut", "here")
	 || Has(lower, "cut", "cut") || Has(lower, "tear", "here")) {
	    /* Get next non-blank line. */
	    do {
		if (fgets(buff, BUFSIZ, Stream) == NULL) {
		    Fprintf(stderr, "unshar:  cut line is last line of %s\n",
			    Name);
		    return FALSE;
		}
	    } while (*buff == '\n');

	    /* If it starts with a comment or lower-case letter we win. */
	    if (*buff == '#' || *buff == ':'
	     || (CTYPE(*buff) && islower(*buff)))
		return TRUE;

	    /* The cut message lied. */
	    Fprintf(stderr, "unshar: %s is not a shell archive,\n", Name);
	    Fprintf(stderr, "        the 'cut' line was followed by: %s", buff);
	    return FALSE;
	}

	if (Header) {
	    (void)fputs(buff, Header);
	    if (InHeader && !IsHeader(buff))
		InHeader = FALSE;
	}
    }
}


/*
**  Create file for the header, find true start of the archive,
**  and send it off to the shell.
*/
static void
Unshar(Name, HdrFile, Stream, Saveit, Forced)
    char		*Name;
    char		*HdrFile;
    REGISTER FILE 	*Stream;
    int			Saveit;
    int			Forced;
{
    REGISTER FILE	*Header;
#ifndef	USE_MY_SHELL
    REGISTER FILE	*Pipe;
#endif	/* USE_MY_SHELL */
    char		*p;
    char		buff[BUFSIZ];

    if (Saveit) {
	/* Create a name for the saved header. */
	if (HdrFile)
	    (void)strcpy(buff, HdrFile);
	else if (Name) {
	    p = RDX(Name, '/');
#ifdef	BU_NAME_LEN
	    (void)strncpy(buff, p ? p + 1 : Name, BU_NAME_LEN);
	    buff[BU_NAME_LEN - 4] = '\0';
#else
	    (void)strcpy(buff, p ? p + 1 : Name);
#endif	/* BU_NAME_LEN */
	    (void)strcat(buff, ".hdr");
	}
	else
	    (void)strcpy(buff, "UNSHAR.HDR");

	/* Tell user, and open the file. */
	Fprintf(stderr, "unshar:  Sending header to %s.\n", buff);
	if ((Header = fopen(buff, "a")) == NULL)
	    Quit("Can't open file for header");
    }
    else
	Header = NULL;

    /* If name is NULL, we're being piped into... */
    p = Name ? Name : "the standard input";
    Printf("unshar:  Doing %s:\n", p);

    if (Found(p, buff, Forced, Stream, Header)) {
#ifdef	USE_MY_SHELL
	BinSh(Name, Stream, buff);
#else
	if ((Pipe = popen("/bin/sh", "w")) == NULL)
	    Quit("Can't open pipe to /bin/sh process");

	(void)fputs(buff, Pipe);
	while (fgets(buff, sizeof buff, Stream))
	    (void)fputs(buff, Pipe);

	(void)pclose(Pipe);
#endif	/* USE_MY_SHELL */
    }

    /* Close the headers. */
    if (Saveit)
	(void)fclose(Header);
}


int
main(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER FILE	*Stream;
    REGISTER int	i;
    char		*p;
    char		*Home;
    char		*HdrFile;
    char		cwd[BUFSIZ];
    char		dir[BUFSIZ];
    char		buff[BUFSIZ];
    int			Saveit;
    int			Forced;
    int			Oops;

    /* Parse JCL. */
    p = getenv("UNSHARDIR");
    Saveit = DEF_SAVEIT;
#ifdef	UNIX
    HdrFile = NULL;
#else
    HdrFile = "UNSHAR.HDR";
#endif	/* UNIX */
    Forced = FALSE;
    for (Oops = FALSE; (i = getopt(ac, av, "vc:d:fh:ns")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	case 'c': 		/* Change to directory first		*/
	case 'd': 		/* Change to directory first		*/
	    p = optarg;
	    break;
	case 'f':		/* "Force" input to be a shar		*/
	    Forced = TRUE;
	    break;
	case 'h':		/* Save shar header to named file	*/
	    HdrFile = optarg;
	    /* FALLTHROUGH */
	case 's': 		/* Save shar header			*/
	    Saveit = TRUE;
	    break;
	case 'n':		/* Don't save shar header		*/
	    Saveit = FALSE;
	    break;
	}
    ac -= optind;
    av += optind;

    if (Oops) {
	Fprintf(stderr, "Usage:\n  unshar %s [files...]\n",
		"[-f] [-s] [-n] [-c dir] [-d dir] [-h file]");
	exit(1);
	/* NOTREACHED */
    }

    /* Going somewhere? */
    if (p) {
	if (*p == '?') {
	    /* Ask for name. */
	    Stream = isatty(fileno(stdin))
		    ? stdin : fopen(ctermid((char *)NULL), "r");
	    if (Stream == NULL)
		Quit("Can't open tty to ask for directory");
	    Printf("unshar:  what directory?  ");
	    (void)fflush(stdout);
	    if (fgets(buff, sizeof buff, Stream) == NULL
	     || buff[0] == '\n'
	     || (p = IDX(buff, '\n')) == NULL)
		Quit("Okay, cancelled");
	    *p = '\0';
	    p = buff;
	    if (Stream != stdin)
		(void)fclose(Stream);
	}

	/* If name is ~/blah, he means $HOME/blah. */
	if (*p == '~') {
#ifdef	VMS
	    Sprintf(dir, "sys$login:[%s]", p + 1);
#else
	    Home = getenv("HOME");
	    if (Home == NULL) {
		Home = "/tmp";
		Fprintf(stderr, "Unshar warning, no $HOME; using \"%s\".\n",
			Home);
	    }
	    Sprintf(dir, "%s/%s", Home, p + 1);
#endif	/* VMS */
	    p = dir;
	}

	/* If we're gonna move, first remember where we were. */
	if (GetDir(cwd, sizeof cwd) == NULL) {
	    Fprintf(stderr, "Unshar warning, Can't get current directory.\n");
	    cwd[0] = '\0';
	}

	/* Got directory; try to go there.  Only make last component. */
	if (chdir(p) < 0 && (mkdir(p, 0777) < 0 || chdir(p) < 0))
	    Quit("Cannot chdir nor mkdir desired directory");
    }
    else
	cwd[0] = '\0';

    /* No buffering. */
    (void)setbuf(stdout, (char *)NULL);
    (void)setbuf(stderr, (char *)NULL);

    if (*av)
	/* Process filenames from command line. */
	for (; *av; av++) {
	    if (cwd[0] && av[0][0] != '/') {
		Sprintf(buff, "%s/%s", cwd, *av);
		*av = buff;
	    }
	    if ((Stream = fopen(*av, "r")) == NULL)
		Fprintf(stderr, "unshar:  Can't open file '%s'.\n", *av);
	    else {
		Unshar(*av, HdrFile, Stream, Saveit, Forced);
		(void)fclose(Stream);
	    }
	}
    else
	/* Do standard input. */
	Unshar((char *)NULL, HdrFile, stdin, Saveit, Forced);

    /* That's all she wrote. */
    exit(0);
    /* NOTREACHED */
}
