/*
**  FINDSRC
**
**  Walk directories, trying to find source files.
*/
#include "shar.h"
static char RCS[] =
	"$Id: findsrc.c,v 3.0.3.3 1993/08/25 17:04:35 ram Exp $";
/*
 * $Log: findsrc.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:35  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:50:22  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:31:40  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Wildcard patterns, and whether or not to include these things.
*/
typedef struct _table {
    char	*Pattern;
    int		*Value;
    int		FullPath;
} TABLE;


/*
**  Global variables.
*/
static int	DoRCS;			/* Do RCS and SCCS files?	*/
static int	False;			/* A FALSE value		*/
static int	True = 1;		/* A TRUE value			*/
static int	Default;		/* Default answer from user	*/
static int	Verbose;		/* List rejected files, too?	*/
static char	Dname[TEMPSIZE];	/* Filename of directory list	*/
static char	Fname[TEMPSIZE];	/* Filename of file list	*/
static FILE	*Dfile;			/* List of directories found	*/
static FILE	*Ffile;			/* List of files found		*/
static FILE	*DEVTTY;		/* The tty, if in filter mode	*/
static TABLE	*EndTable;		/* End of pattern table		*/


TABLE	Table[MAX_PATTERNS] = {
	/* Common cruft we never want. */
    { "TAGS",	&False, FALSE },	{ "core",	&False, FALSE },
    { "tags",	&False, FALSE },	{ "lint",	&False, FALSE },
    { "*.out",	&False, FALSE },	{ "*.orig",	&False, FALSE },
    { "*.rej",	&False, FALSE },	{ "*.BAK",	&False, FALSE },
    { "*.CKP",	&False, FALSE },	{ "*.old",	&False, FALSE },
    { "*.o",	&False, FALSE },	{ "*.EXE",	&False, FALSE },
    { "*.OBJ",	&False, FALSE },	{ "*~",		&False, FALSE },
    { "*.ln",	&False, FALSE },	{ "*.dvi",	&False, FALSE },
    { "Part[0-9][0-9]", &False, FALSE },

	/* RCS or SCCS file or directory. */
    { "RCS",	&DoRCS, FALSE },	{ "*,v",	&DoRCS, FALSE },
    { "SCCS",	&DoRCS, FALSE },	{ "s.*",	&DoRCS, FALSE },
    { "CVS.adm", &False, FALSE },

	/* GNU Emacs elisp. */
    { "*.elc",	&False, FALSE },	{ "*.el*",	&True, FALSE },

	/* C, lex, yacc, C++, and shell source. */
    { "*.[Cchyl]", &True, FALSE },	{ "*.cc",	&True, FALSE },
    { "*.sh",	&True, FALSE },		{ "*.csh",	&True, FALSE },

	/* Documentation. */
    { "*.[1-9]", &True, FALSE },	{ "*.man",	&True, FALSE },
    { "*.[1-9][lmcx]", &True, FALSE },	{ "*.tex",	&True, FALSE },
    { "*.sty",	&True, FALSE },
    { "*.texinfo", &True, FALSE },	{ "*.texi", &True, FALSE },

    /* README, READ.ME, read_me, etc. */
    { "[rR][eE][aA][dD]*", &True, FALSE },
    { ":[rR][eE][aA][dD]*", &True, FALSE },

    /* Files this package uses/creates. */
    { "MANIFEST", &True, FALSE },	{ "PACKNOTES",	&True, FALSE },

	/* Make control file. */
    { "[Mm]akefile", &True, FALSE },	{ "descrip.mms", &True, FALSE },
    { "GNUmakefile", &True, FALSE },

	/* Larry Wall-style configuration stuff. */
    { "Configure", &True, FALSE },	{ "*.SH",	&True, FALSE },

	/* Mlisp sources, m4. */
    { "*.mo",	&False, FALSE },	{ "*.m[4a-z]",	&True, FALSE },

    { NULL }
};


/*
**  Signal handler.  Clean up and die.
*/
static sigret_t
FCatch(s)
    int		s;
{
    int		e;

    e = errno;
    if (Dname[0])
	(void)unlink(Dname);
    if (Fname[0])
	(void)unlink(Fname);
    Fprintf(stderr, "Got signal %d, %s.\n", s, strerror(e));
    exit(1);
    /* NOTREACHED */
}


/*
**  Given a filename, apply heuristics to see if we want it.
*/
static int
Wanted(Name)
    REGISTER char	*Name;
{
    REGISTER FILE	*F;
    REGISTER TABLE	*Tp;
    REGISTER char	*p;
    char		*Tail;
    char		buff[BUFSIZ];
    int			Executable;
    int			Script;

    /* Get down to brass tacks. */
    if ((Tail = RDX(Name, '/')) == NULL)
	Tail = Name;
    else
	Tail++;
    if (*Tail == '\0')
	return FALSE;

    /* Only do directories if they're not the hidden ones. */
    switch (Ftype(Name)) {
    default:
	return FALSE;
    case F_DIR:
	if (EQ(Tail, ".") || EQ(Tail, ".."))
	    return FALSE;
	break;
    case F_FILE:
	break;
    }

    /* Scan the table. */
    for (Tp = Table; Tp < EndTable; Tp++)
	if (wildmat(Tp->FullPath ? Name : Tail, Tp->Pattern) == TRUE)
	    return *Tp->Value;

    /* If we have a default, give it back. */
    if (Default)
	return Default == 'y';

#ifdef	CAN_POPEN
    /* See what file(1) has to say; if it says executable, punt. */
    Sprintf(buff, "exec file '%s'", Name);
    if ((F = popen(buff, "r")) != NULL) {
	(void)fgets(buff, sizeof buff, F);
	(void)pclose(F);
	for (Executable = FALSE, p = buff; (p = IDX(p, 'e')) != NULL; p++)
	    if (PREFIX(p, "executable"))
		Executable = TRUE;
	for (Script = FALSE, p = buff; (p = IDX(p, 's')) != NULL; p++)
	    if (PREFIX(p, "script"))
		Script = TRUE;
	if (Executable && !Script)
	    return FALSE;
	(void)fputs(buff, stdout);
    }
    else
	Fprintf(stdout, "%s -- ", Name);
#else
    Fprintf(stdout, "%s -- ", Name);
#endif	/* CAN_POPEN */

    /* Add it? */
    for ( ; ; ) {
	if (DEVTTY == NULL)
	    DEVTTY = fopen(ctermid((char *)NULL), "r");
	Fprintf(stdout, "Add this one (y or n)[y]?  ");
	(void)fflush(stdout);
	if (fgets(buff, sizeof buff, DEVTTY) == NULL
	 || buff[0] == '\n' || buff[0] == 'y' || buff[0] == 'Y')
	    break;
	if (buff[0] == 'n' || buff[0] == 'N')
	    return FALSE;
	if (buff[0] == '!' ) {
	    (void)system(&buff[1]);
	    Fprintf(stdout, "--------------------\n");
	}
	Fprintf(stdout, "%s:  ", Name);
	clearerr(DEVTTY);
    }
    return TRUE;
}


/*
**  Quick and dirty recursive routine to walk down directory tree.
**  Could be made more general, but why bother?
*/
static void
Process(p, level)
    REGISTER char	*p;
    REGISTER int	level;
{
    REGISTER char	*q;
    DIR			*Dp;
    DIRENTRY		*E;
    char		buff[BUFSIZ];

#if	0
#ifdef	MSDOS
    /* Downcase the path.  I dunno why, somebody told me to. */
    for (q = p; *q; q++)
	if (CTYPE(*q) && isupper(*q))
	    *q = tolower(*q);
#endif	/* MSDOS */
#endif	/* 0 */

    if (!GetStat(p))
	Fprintf(stderr, "Can't walk down %s, %s.\n", p, strerror(errno));
    else {
	/* Skip leading ./ which find(1), e.g., likes to put out. */
	if (p[0] == '.' && p[1] == '/')
	    p += 2;

	if (Wanted(p))
	    Fprintf(Ftype(p) == F_FILE ? Ffile : Dfile, "%s\n", p);
	else if (Verbose)
	    Fprintf(Ftype(p) == F_FILE ? Ffile : Dfile, "REJECTED %s\n", p);

	if (Ftype(p) == F_DIR)
	    if (++level == MAX_LEVELS)
		Fprintf(stderr, "Won't walk down %s -- more than %d levels.\n",
			p, level);
	    else if ((Dp = opendir(p)) != NULL) {
		q = buff + strlen(strcpy(buff, p));
		for (*q++ = '/'; (E = readdir(Dp)) != NULL; )
		    if (!EQ(E->d_name, ".") && !EQ(E->d_name, "..")) {
			(void)strcpy(q, E->d_name);
			Process(buff, level);
		    }
		(void)closedir(Dp);
	    }
	    else
		Fprintf(stderr, "Can't open directory %s, %s.\n",
			p, strerror(errno));
    }
}


/*
**  Try to insert a pattern to the table; return FALSE if it failed.
*/
static int
AddPattern(Pattern, Value, FullPath)
    char		*Pattern;
    int			*Value;
    int			FullPath;
{
    register TABLE	*Tp;

    if (EndTable == &Table[sizeof Table / sizeof Table[0] - 1]) {
	Fprintf(stderr, "Not enough space in pattern table, sorry.\n");
	return FALSE;
    }

    /* Move everyone down, then insert the new pattern. */
    for (Tp = EndTable; Tp > Table; Tp--) {
	Tp[0].Pattern = Tp[-1].Pattern;
	Tp[0].Value = Tp[-1].Value;
	Tp[0].FullPath = Tp[-1].FullPath;
    }
    Table[0].Pattern = Pattern;
    Table[0].Value = Value;
    Table[0].FullPath = FullPath;
    EndTable++;
    return TRUE;
}


int
main(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER FILE	*F;
    REGISTER char	*p;
    REGISTER int	i;
    REGISTER int	Oops;
    register TABLE	*Tp;
    int			Header;
    int			Dumpit;
    char		*Outfile;
    char		buff[BUFSIZ];

    /* Find the end of the pattern table. */
    for (EndTable = Table; EndTable->Pattern; EndTable++)
	continue;

    /* Parse JCL. */
    Header = 0;
    Dumpit = FALSE;
    Outfile = NULL;
    for (Oops = FALSE; (i = getopt(ac, av, "d:h:lmN:n:o:rsvxY:y:")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'd':		/* Default answer			*/
	    switch (optarg[0]) {
	    default:
		Oops = TRUE;
		break;
	    case 'y': case 'Y':
		Default = 'y';
		break;
	    case 'n': case 'N':
		Default = 'n';
		break;
	    }
	    break;
	case 'h':		/* Lines of header to skip		*/
	    Header = atoi(optarg);
	    break;
	case 'l':		/* Log output				*/
	    Verbose = TRUE;
	    break;
	case 'm':		/* Convenient option shorthand		*/
	    Outfile = "MANIFEST";
	    Header = 2;
	    break;
	case 'N':		/* Add a reject pattern			*/
	case 'n':		/* Add a reject pattern			*/
	    if (AddPattern(optarg, &False, i == 'N') == FALSE)
		Oops = TRUE;
	    break;
	case 'o':		/* Output file name			*/
	    Outfile = optarg;
	    break;
	case 'r':		/* Take RCS and SCCS files		*/
	case 's':		/* Take RCS and SCCS files		*/
	    DoRCS = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	case 'x':		/* Dump the pattern database		*/
	    Dumpit = TRUE;
	    break;
	case 'Y':		/* Add an accept pattern		*/
	case 'y':		/* Add an accept pattern		*/
	    if (AddPattern(optarg, &True, i == 'Y') == FALSE)
		Oops = TRUE;
	    break;
	}
    ac -= optind;
    av += optind;

    if (Oops) {
	Fprintf(stdout, "Usage:\n  findsrc %s\n          %s files...\n",
		"[-d{yn}] [-{yYnN} pattern] [-r] [-s] [-v] [-x]",
		"[-m | -o MANIFEST -h 2]");
	exit(1);
	/* NOTREACHED */
    }

    /* Open the output file. */
    if (Outfile == NULL)
	F = stdout;
    else {
	if (Fexists(Outfile))
	    SafeRename(Outfile);
	if ((F = fopen(Outfile, "w")) == NULL) {
	    Fprintf(stderr, "Can't open %s for output, %s.\n",
		    optarg, strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	}
    }

    /* Dump the pattern table. */
    if (Dumpit) {
	Fprintf(F, "  Pattern               Included?\n");
	Fprintf(F, "  --------------------  ---------\n");
	for (i = 0, Tp = Table; Tp < EndTable; Tp++, i++)
	    Fprintf(F, "  %-20s  %s\n",
		    Tp->Pattern, *Tp->Value ? "yes" : "no");
	Printf("There are a total of %d patterns\n", i);
	(void)fclose(F);
	exit(0);
    }

    /* Open temp files, set signal catcher. */
    MakeTempName(Dname, TEMP_NAME1);
    Dfile = fopen(Dname, "w");
    MakeTempName(Fname, TEMP_NAME2);
    Ffile = fopen(Fname, "w");
    SetSigs(FCatch);

    /* Read list of files, determine their status. */
    if (*av)
	for (DEVTTY = stdin; *av; av++)
	    Process(*av, 0);
    else
	while (fgets(buff, sizeof buff, stdin)) {
	    if ((p = IDX(buff, '\n')) == NULL)
		Fprintf(stderr, "Warning, line too long:\n\t%s\n", buff);
	    else
		*p = '\0';
	    Process(buff, 0);
	}

    /* Print the header. */
    if (Header >= 2) {
	Fprintf(F, "Name\tDescription\n");
	Fprintf(F, "----\t-----------\n");
	Header -= 2;
    }
    while (--Header >= 0)
	Fprintf(F, "\n");

    /* First print directories. */
    if (freopen(Dname, "r", Dfile)) {
	while (fgets(buff, sizeof buff, Dfile))
	    if ((p = IDX(buff, '\n')) != NULL)
		*p = '\0';
	    Fprintf(F, "%s (Directory)\n");
	(void)fclose(Dfile);
    }

    /* Now print regular files. */
    if (freopen(Fname, "r", Ffile)) {
	while (fgets(buff, sizeof buff, Ffile))
	    (void)fputs(buff, F);
	(void)fclose(Ffile);
    }

    /* That's all she wrote. */
    (void)unlink(Dname);
    (void)unlink(Fname);
    exit(0);
    /* NOTREACHED */
}
