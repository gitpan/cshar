/*
**  MAKEKIT
**
**  Split up source files into reasonably-sized shar lists.
*/
#include "shar.h"
static char RCS[] =
	"$Id: makekit.c,v 3.0.3.3 1993/08/25 17:04:43 ram Exp $";

/*
 * $Log: makekit.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:43  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:00  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  12:05:11  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**	How much overhead a file takes up.
*/
#define OVERHEAD(Name)		(strlen(Name) * 8 + 325)

/*
**  Our block of information about the files we're doing.
*/
typedef struct _block {
    char	*Name;			/* Filename			*/
    char	*Text;			/* What it is			*/
    int		Where;			/* Where it is			*/
    int		Type;			/* Directory or file?		*/
    long	Bsize;			/* Size in bytes		*/
} BLOCK;


/*
**  Our block of information about the archives we're making.
*/
typedef struct _archive {
    int		Count;			/* Number of files		*/
    long	Asize;			/* Bytes used by archive	*/
} ARCHIVE;


/*
**  Global variables.
*/
static char	*InName;		/* File with list to pack	*/
static char	*OutName;		/* Where our output goes	*/
static char	*SharName = "Part";	/* Prefix for name of each shar	*/
static char	*Trailer;		/* Text for shar to pack in	*/
static char	TEMP[TEMPSIZE];		/* Temporary manifest file	*/
#ifdef	USE_TEMP_MANIFEST
static char	FLST[TEMPSIZE];		/* File with list for shar	*/
#endif	/* USE_TEMP_MANIFEST */
static int	ArkCount = 20;		/* Max number of archives	*/
static int	ExcludeIt;		/* Leave out the output file?	*/
static int	Header;			/* Lines of prolog in input	*/
static int	Preserve;		/* Preserve order for Manifest?	*/
static int	Working = TRUE;		/* Call shar when done?		*/
static long	Size = 55000;		/* Largest legal archive size	*/


/*
**  Sorting predicate to put README first, then MANIFEST, then directories,
**  then larger files, then smaller files, which is how we want to pack
**  stuff in archives.
*/
static int
SizeP(p1, p2)
    char	*p1;
    char	*p2;
{
    BLOCK	*t1;
    BLOCK	*t2;
    long	l;

    t1 = (BLOCK *)p1;
    t2 = (BLOCK *)p2;

    if (t1->Type == F_DIR)
	return t2->Type == F_DIR ? 0 : -1;
    if (t2->Type == F_DIR)
	return 1;
    if (EQ(t1->Name, "PACKNOTES"))
	return -1;
    if (EQ(t2->Name, "PACKNOTES"))
	return 1;
    if (EQn(t1->Name, "README", 6))
	return -1;
    if (EQn(t2->Name, "README", 6))
	return 1;
    if (OutName && EQ(t1->Name, OutName))
	return -1;
    if (OutName && EQ(t2->Name, OutName))
	return 1;
    l = t1->Bsize - t2->Bsize;
    /* NOSTRICT: "long assignment may lose accuracy." */
    if (l != 0L)
	return l < 0 ? 1 : -1;
    return 0;
}


/*
**  Sorting predicate to get things in alphabetical order, with the
**  README and MANIFEST files as first and second, respectively.
*/
static int
ReadmeP(p1, p2)
    char	*p1;
    char	*p2;
{
    BLOCK	*t1;
    BLOCK	*t2;
    int		i;

    t1 = (BLOCK *)p1;
    t2 = (BLOCK *)p2;
    if (EQ(t1->Name, "PACKNOTES"))
	return -1;
    if (EQ(t2->Name, "PACKNOTES"))
	return 1;
    if (EQ(t1->Name, "README"))
	return -1;
    if (EQ(t2->Name, "README"))
	return 1;
    if (EQ(t1->Name, "MANIFEST"))
	return -1;
    if (EQ(t2->Name, "MANIFEST"))
	return 1;
    i = *t1->Name - *t2->Name;
    return i ? i : strcmp(t1->Name, t2->Name);
}


/*
**  Skip whitespace.
*/
static char *
Skip(p)
    REGISTER char	*p;
{
    while (*p && WHITE(*p))
	p++;
    return p;
}


/*
**	Clean-up temporary files and exit
*/
Clean_quit()
{
    if (TEMP[0])
	(void)unlink(TEMP);
#ifdef	USE_TEMP_MANIFEST
    if (FLST[0])
	(void)unlink(FLST);
#endif	/* USE_TEMP_MANIFEST */
    exit(1);
    /* NOTREACHED */
}


/*
**  Signal handler.  Clean up and die.
*/
static sigret_t
Catch(s)
    int		s;
{
    Fprintf(stderr, "Got signal %d, %s.\n", s, strerror(errno));
	Clean_quit();
    /* NOTREACHED */
}


int
main(ac, av)
    REGISTER int	ac;
    char		*av[];
{
    REGISTER FILE	*F;
    REGISTER FILE	*In;
    REGISTER BLOCK	*t;
    REGISTER ARCHIVE	*k;
    REGISTER char	*p;
    REGISTER int	i;
    REGISTER int	lines;
    REGISTER int	FoundOutname;
    BLOCK		*Table;
    BLOCK		*TabEnd;
    ARCHIVE		*Ark;
    ARCHIVE		*ArkEnd;
    char		buff[BUFSIZ];
    long		lsize;
    int			LastOne;
    int			Start;
    int			Notkits;
    int			Believer;
    int			Oops;
    char		EndArkNum[20];
    char		CurArkNum[20];

    /* Collect input. */
    Believer = FALSE;
    Notkits = FALSE;
    for (Oops = FALSE; (i = getopt(ac, av, "v1beh:i:k:n:mo:ps:t:x")) != EOF; )
	switch (i) {
	default:
	    Oops = TRUE;
	    break;
	case 'v':		/* Print version			*/
	    Version(RCS);
	    /* NOTREACHED */
	case '1':		/* Pretend we are doing a single shar	*/
	    Notkits = TRUE;
	    break;
	case 'b':		/* Use manifest's assignments		*/
	    Believer = TRUE;
	    break;
	case 'e':		/* Don't include manifest in manifest	*/
	    ExcludeIt = TRUE;
	    break;
	case 'h':		/* Lines of header to skip		*/
	    Header = atoi(optarg);
	    break;
	case 'i':		/* Name of input manifest		*/
	    InName = optarg;
	    break;
	case 'k':		/* Maximum number of kits to make	*/
	    ArkCount = atoi(optarg);
	    break;
	case 'm':		/* Convenient option shorthand		*/
	    InName = OutName = "MANIFEST";
	    Header = 2;
	    break;
	case 'n':		/* Name for generated shar files	*/
	    SharName = optarg;
	    break;
	case 'o':		/* Name for generated manifest file	*/
	    OutName = optarg;
	    break;
	case 'p':		/* Preserve order in existing manifest	*/
	    Preserve = TRUE;
	    break;
	case 's':		/* Target size for generated shars	*/
	    Size = (long)atoi(optarg);
	    if (IDX(optarg, 'k') || IDX(optarg, 'K'))
		Size *= 1024;
	    break;
	case 't':		/* Text to print when all are unpacked	*/
	    Trailer = optarg;
	    break;
	case 'x':		/* Don't invoke shar to do the packing	*/
	    Working = FALSE;
	    break;
	}
    ac -= optind;
    av += optind;

    if (Oops == FALSE && InName && av[0]) {
	Fprintf(stderr,
		"Can't list files on command line and use -i option.\n");
	Oops = TRUE;
    }

    if (Oops) {
	Fprintf(stderr, "Usage:\n  makekit %s\n          %s [files...]\n",
		"[-1] [-b] [-e] [-x] [-k #] [-s #[k]] [-n Name] [-t Text]",
		"[-p] [-m | -i MANIFEST -o MANIFEST -h 2]");
	Clean_quit();
	/* NOTREACHED */
    }

    /* Write the file list to a temp file. */
    MakeTempName(TEMP, TEMP_NAME1);
    F = fopen(TEMP, "w");
    SetSigs(Catch);
    if (av[0])
	/* Got the arguments on the command line. */
	while (*av)
	    Fprintf(F, "%s\n", *av++);
    else {
	/* Got the name of the file from the command line. */
	if (InName == NULL)
	    In = stdin;
	else if ((In = fopen(InName, "r")) == NULL) {
	    Fprintf(stderr, "Can't read %s as manifest, %s.\n",
		    InName, strerror(errno));
	    Clean_quit();
	    /* NOTREACHED */
	}
	/* Skip any possible prolog, then output rest of file. */
	while (--Header >= 0 && fgets(buff, sizeof buff, In))
	    continue;
	if (feof(In)) {
	    Fprintf(stderr, "Nothing but header lines in list!?\n");
	    Clean_quit();
	    /* NOTREACHED */
	}
	while (fgets(buff, sizeof buff, In))
	    fputs(buff, F);
	if (In != stdin)
	    (void)fclose(In);
    }
    (void)fclose(F);

    /* Count number of files, allow for NULL and our output file. */
    F = fopen(TEMP, "r");
    for (lines = 2; fgets(buff, sizeof buff, F); lines++)
	continue;
    rewind(F);

    /* Read lines and parse lines, see if we found our OutFile. */
    FoundOutname = FALSE;
    Table = NEW(BLOCK, lines);		/* Initialized in loop, below. */
    for (t = Table, lines = 0; fgets(buff, sizeof buff, F); ) {
	/* Read line, skip first word, check for blank line. */
	if ((p = IDX(buff, '\n')) == NULL)
	    Fprintf(stderr, "Warning, line too long:\n\t%s\n", buff);
	else
	    *p = '\0';
	p = Skip(buff);
	if (*p == '\0')
	    continue;

	/* Copy the line, snip off the first word. */
	for (p = t->Name = COPY(p); *p && !WHITE(*p); p++)
	    continue;
	if (*p)
	    *p++ = '\0';

	/* Skip <spaces><digits><spaces>; remainder is the file description. */
	p = Skip(p);
	t->Where = atoi(p);
	while (*p && CTYPE(*p) && isdigit(*p))
	    p++;
	t->Text = Skip(p);

	/* Get file type. */
	if (!GetStat(t->Name)) {
	    Fprintf(stderr, "Can't stat %s (%s), skipping.\n",
		    t->Name, strerror(errno));
	    continue;
	}
	t->Type = Ftype(t->Name);

	/* Estimate how much space the file will take when archived. */
	if (t->Type == F_FILE) {
	    t->Bsize = OVERHEAD(t->Name);
	    lsize = Fsize(t->Name);
	    /* If we had a "wc" we could use it, but this is good enough. */
	    if (IsProbablySource(t->Name))
		/* Average chars/line in C, Pascal, Fortran.  Sort of. */
		t->Bsize += lsize + lsize / 25;
	    else
		t->Bsize += lsize + lsize / 60;
	}
	else
	    t->Bsize = strlen(t->Name) * 4 + 80;

	if (t->Bsize > Size) {
	    Fprintf(stderr,
		"At %ld bytes, %s seems too big for any archive!\n",
		t->Bsize, t->Name);
	    Clean_quit();
	    /* NOTREACHED */
	}

	/* Is our output file there? */
	if (!FoundOutname && OutName && EQ(OutName, t->Name))
	    FoundOutname = TRUE;

	/* All done -- advance to next entry. */
	t++;
	lines++;
    }
    (void)fclose(F);
    (void)unlink(TEMP);

    /* Add our output file? */
    if (!ExcludeIt && !FoundOutname && OutName) {
	t->Name = OutName;
	t->Text = "This shipping list";
	t->Type = F_FILE;
	t->Bsize = (lines + 3) * 60 + OVERHEAD(OutName);
	t->Where = 0;
	t++;
    }

    /* Sort by size. */
    lines = t - Table;
    TabEnd = &Table[lines];
    if (!Preserve)
	qsort((char *)Table, lines, sizeof Table[0], SizeP);

    /* Get archive space, allow for initial overhead. */
    Ark = NEW(ARCHIVE, ArkCount);
    ArkEnd = &Ark[ArkCount];
    for (k = Ark; k < ArkEnd; k++) {
	k->Count = 0;
	k->Asize = 1000;
    }

    /* The first archive gets extra overhead for introductory comments. */
    Ark[0].Asize += 2000;

    /* See if everyone has a place to be. */
    if (Believer)
	for (t = Table; t < TabEnd; t++) {
	    if (t->Where == 0) {
		Fprintf(stderr, "Can't believe the manifest assignments.\n");
		Believer = FALSE;
		break;
	    }
	    t->Where--;
	}

    /* Loop through the pieces, and put everyone into an archive. */
    if (!Believer) {
	for (t = Table; t < TabEnd; t++) {
	    for (k = Ark; k < ArkEnd; k++)
		if (t->Bsize + k->Asize <= Size) {
		    k->Asize += t->Bsize;
		    t->Where = k - Ark;
		    k->Count++;
		    break;
		}
	    if (k == ArkEnd) {
		Fprintf(stderr,
			"'%s' doesn't fit -- need more then %d archives.\n",
			t->Name, ArkCount);
		Clean_quit();
		/* NOTREACHED */
	    }

	    /* Since our shar doesn't build sub-directories... */
	    if (t->Type == F_DIR && k != Ark)
		Fprintf(stderr, "Warning, directory '%s' is in archive %d.\n",
			t->Name, k - Ark + 1);
	}
    }

    /* Open the output file. */
    if (OutName == NULL)
	F = stdout;
    else {
	if (Fexists(OutName))
	    SafeRename(OutName);
	if ((F = fopen(OutName, "w")) == NULL) {
	    Fprintf(stderr, "Can't open '%s' for output, %s.\n",
		    OutName, strerror(errno));
	    Clean_quit();
	    /* NOTREACHED */
	}
    }

    /* Sort the shipping list, then write it. */
    if (!Preserve)
	qsort((char *)Table, lines, sizeof Table[0], ReadmeP);
    Fprintf(F, "   File Name\t\tArchive #\tDescription\n");
    Fprintf(F, "----------------------------------------------------------\n");
    for (t = Table; t < TabEnd; t++)
	Fprintf(F, MANI_FORMAT, t->Name, t->Where + 1, t->Text);

    /* Close output.  Are we done? */
    if (F != stdout)
	(void)fclose(F);
    if (!Working)
	exit(0);
	/* NOTREACHED */

    /* Find last archive number. */
    for (i = 0, t = Table; t < TabEnd; t++)
	if (i < t->Where)
	    i = t->Where;
    LastOne = i + 1;

    /* Find archive with most files in it and build an argv vector. */
    for (i = 0, k = Ark; k < ArkEnd; k++)
	if (i < k->Count)
	    i = k->Count;
    av = NEW(char*, i + 10);

    /* Build the fixed part of the argument vector. */
    av[0] = "shar";
    i = 1;
    if (Trailer) {
	av[i++] = "-t";
	av[i++] = Trailer;
    }
    if (Notkits == FALSE) {
	Sprintf(EndArkNum, "%d", LastOne);
	av[i++] = "-e";
	av[i++] = EndArkNum;
	av[i++] = "-n";
	av[i++] = CurArkNum;
    }
    av[i++] = "-o";
    av[i++] = buff;			/* See sprintf call in loop below. */

#ifdef	USE_TEMP_MANIFEST
    av[i++] = "-i";
    MakeTempName(FLST, TEMP_NAME2);
    av[i++] = FLST;
    av[i] = NULL;
#endif	/* USE_TEMP_MANIFEST */

    /* Call shar to package up each archive. */
    for (Start = i, i = 0; i < LastOne; i++) {
	Sprintf(CurArkNum, "%d", i + 1);
	Sprintf(buff, NAME_FORMAT, SharName, i + 1);
#ifndef	USE_TEMP_MANIFEST
	for (lines = Start, t = Table; t < TabEnd; t++)
	    if (t->Where == i)
		av[lines++] = t->Name;
	av[lines] = NULL;
#else
	if ((F = fopen(FLST, "w")) == NULL) {
	    Fprintf(stderr, "Can't open list file '%s' for output, %s.\n",
		    FLST, strerror(errno));
	    Clean_quit();
	    /* NOTREACHED */
	}
	for (t = Table; t < TabEnd; t++)
	    if (t->Where == i)
		Fprintf(F, "%s\n", t->Name);
	(void)fclose(F);
#endif /* USE_TEMP_MANIFEST */
	Fprintf(stderr, "Packing kit %d...\n", i + 1);
	if ((lines = Execute(av)) != 0)
	    Fprintf(stderr, "Warning, shar returned status %d.\n", lines);
    }

#ifdef	USE_TEMP_MANIFEST
    (void)unlink(FLST);
#endif	/* USE_TEMP_MANIFEST */

    /* That's all she wrote. */
    exit(0);
    /* NOTREACHED */
}
