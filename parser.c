/*
**  Parser for /bin/sh language commonly used in writing shars.
**
**  Note that (void)Exec(...) is all over the place, even though every
**  command returns the right exist status; I changed my mind about
**  implementing $? "properly."
**
**  This code is big and ugly, and should probably be rewritten from the
**  ground up.  It wasn't designed; it started with Argify and SynTable
**  as a cute ten-minute hack and it just grew.
*/
#define PARSER_DATA
#include "parser.h"
#ifdef	RCSID
static char RCS[] =
	"$Id: parser.c,v 3.0.3.3 1993/08/25 17:04:53 ram Exp $";
#endif	/* RCSID */
/*
 * $Log: parser.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:53  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:25  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:35:22  ram
 * 3.0 baseline (ram).
 * 
 */


/*
**  Global variables.
*/
#ifdef	MAX_MKDIRS
static int	DirCount;		/* Number of directories made	*/
#endif	/* MAX_MKDIRS */
#ifdef	MAX_FOPENS
static int	CreatedCount;		/* Number of files created	*/
#endif	/* MAX_FOPENS */
static VAR	VarList[MAX_VARS];	/* Our list of variables	*/
static char	Text[BUFSIZ];		/* Current text line		*/
static int	LineNum = 1;		/* Current line number		*/
static int	Running = TRUE;		/* Working, or skipping?	*/
static short	SynTable[256] = {	/* Syntax table			*/
    /*	\0	001	002	003	004	005	006	007	*/
	C_TERM,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,
    /*	\h	\t	\n	013	\f	\r	016	017	*/
	C_WHIT,	C_WHIT,	C_TERM,	C_WHIT,	C_TERM,	C_TERM,	C_WHIT,	C_WHIT,
    /*	020	021	022	023	024	025	026	027	*/
	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,
    /*	can	em	sub	esc	fs	gs	rs	us	*/
	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,	C_WHIT,

    /*	sp	!	"	#	$	%	&	'	*/
	C_WHIT,	C_LETR,	C_QUOT,	C_TERM,	C_LETR,	C_LETR,	C_DUBL,	C_QUOT,
    /*	(	)	*	+	,	-	.	/	*/
	C_WORD,	C_WORD,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	0	1	2	3	4	5	6	7	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	8	9	:	;	<	=	>	?	*/
	C_LETR,	C_LETR,	C_LETR,	C_DUBL,	C_DUBL,	C_LETR,	C_DUBL,	C_LETR,

    /*	@	A	B	C	D	E	F	G	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	H	I	J	K	L	M	N	O	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	P	Q	R	S	T	U	V	W	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	X	Y	Z	[	\	]	^	_	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_META,	C_LETR,	C_LETR,	C_LETR,

    /*	`	a	b	c	d	e	f	g	*/
	C_WORD,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	h	i	j	k	l	m	n	o	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	p	q	r	s	t	u	v	w	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_LETR,
    /*	x	y	z	{	|	}	~	del	*/
	C_LETR,	C_LETR,	C_LETR,	C_LETR,	C_DUBL,	C_LETR,	C_LETR,	C_WHIT,
};

/**
***		E R R O R   R O U T I N E S
**/


/*
**  Print message with current line and line number.
*/
static void
Note(text, arg)
    char	*text;
    char	*arg;
{
    Fprintf(stderr, "\nIn line %d of %s:\n\t", LineNum, File);
    Fprintf(stderr, text, arg);
    Fprintf(stderr, "Current line:\n\t%s\n", Text);
    (void)fflush(stderr);
}


/*
**  Print syntax message and die.
*/
void
SynErr(text)
    char	*text;
{
    Note("Fatal syntax error in %s statement.\n", text);
    exit(1);
    /* NOTREACHED */
}

/**
***		I N P U T   R O U T I N E S
**/


/*
**  Get a yes/no/quit reply.
*/
static int
YNQreply(prompt)
    char		*prompt;
{
    REGISTER FILE	*DEVTTY;
    register int	value;
    char		buff[LINE_SIZE];

    DEVTTY = fopen(ctermid((char *)NULL), "r");
    for (value = '\0'; !value; ) {
	Fprintf(stderr, "%s:  ", prompt);
	(void)fflush(stderr);
	clearerr(DEVTTY);
	if (fgets(buff, sizeof buff, DEVTTY) == NULL)
	    value = 'Q';
	else
	    switch (buff[0]) {
	    case 'q': case 'Q': case '\n':
		value = 'Q';
		break;
	    case 't': case 'T':
	    case 'y': case 'Y':
		value = 'T';
		break;
	    case 'f': case 'F':
	    case 'n': case 'N':
		value = 'F';
		break;
	    }
    }
    (void)fclose(DEVTTY);
    return value;
}


/*
**  Miniscule regular-expression matcher; only groks the . meta-character.
*/
static int
Matches(p, text)
    REGISTER char	*p;
    REGISTER char	*text;
{
    for (; *p && *text; text++, p++)
	if (*p != *text && *p != '.')
	    return FALSE;
    return TRUE;
}



/*
**  Read input, possibly handling escaped returns.  Returns a value so
**  we can do things like "while (GetLine(TRUE))", which is a hack.  This
**  should also be split into two separate routines, and punt the Flag
**  argument, but so it goes.
*/
int
GetLine(Flag)
    REGISTER int	Flag;
{
    REGISTER char	*p;
    REGISTER char	*q;
    REGISTER int	i;
    char		buf[LINE_SIZE];

    if (Interactive) {
	Fprintf(stderr, "Line %d%s>  ", LineNum, Running ? "" : "(SKIP)");
	(void)fflush(stderr);
    }
    Text[0] = '\0';
    for (q=Text; fgets(buf, sizeof buf, Input); q += strlen(strcpy(q, buf))) {
	LineNum++;
	i = strlen(buf);
	p = &buf[i - 1];
	if (*p != '\n') {
	    /* If we hit EOF without a terminating \n, that's okay. */
	    if (feof(Input))
		p++;
	    else {
		Note("Input line too long.\n", (char *)NULL);
		exit(1);
		/* NOTREACHED */
	    }
	}

	/* Make sure line isn't too long. */
	if (q + i >= &Text[sizeof Text - 1]) {
	    Note("Input line too big.\n", (char *)NULL);
	    exit(1);
	    /* NOTREACHED */
	}

	if (!Flag || p == buf || p[-1] != '\\') {
	    (void)strcpy(q, buf);
	    return 1;
	}
	p[-1] = '\0';
	if (Interactive) {
	    Fprintf(stderr, "PS2>  ");
	    (void)fflush(stderr);
	}
    }
    Note("RAN OUT OF INPUT.\n", (char *)NULL);
    exit(1);
    /* NOTREACHED */
}


/*
**  Copy a sub-string of characters into dynamic space.
*/
static char *
CopyRange(Start, End)
    char	*Start;
    char	*End;
{
    char	*p;
    int		i;

    i = End - Start + 1;
    p = strncpy(NEW(char, i + 1), Start, i);
    p[i] = '\0';
    return p;
}


/*
**  Split a line up into shell-style "words."
*/
int
Argify(ArgV)
    char		**ArgV;
{
    REGISTER char	**av;
    REGISTER char	*p;
    REGISTER char	*q;
    
    for (av = ArgV, p = Text; *p; p++) {
	/* Skip whitespace, but treat "\ " as a letter. */
	for (; ISwhit(*p); p++)
	    if (ISmeta(*p))
		p++;
	if (ISterm(*p))
	    break;
	switch (SynTable[*p]) {
	default:
	    /* Gross type-casting for lint, oh well, shouldn't happen. */
	    Note("Bad case %x in Argify.\n", (char *)SynTable[*p]);
	    /* FALLTHROUGH */
	case C_META:
	    p++;
	    /* FALLTHROUGH */
	case C_WHIT:
	case C_LETR:
	    for (q = p; ISletr(*++q) || ISmeta(q[-1]); )
		continue;
	    *av++ = CopyRange(p, --q);
	    p = q;
	    break;
	case C_DUBL:
	    if (*p == p[1]) {
		*av++ = CopyRange(p, p + 1);
		p++;
		break;
	    }
	    /* FALLTHROUGH */
	case C_WORD:
	    *av++ = CopyRange(p, p);
	    break;
	case C_QUOT:
	    for (q = p; *++q; )
		if (*q == *p && !ISmeta(q[-1]))
		    break;
	    *av++ = CopyRange(p + 1, q - 1);
	    p = q;
	    break;
	}
    }
    *av = NULL;
    if (av > &ArgV[MAX_WORDS - 1])
	SynErr("TOO MANY WORDS IN LINE");
    return av - ArgV;
}

/**
***		V A R I A B L E   R O U T I N E S
**/


/*
**  Return the value of a variable, or an empty string.
*/
static char *
GetVar(Name)
    REGISTER char	*Name;
{
    REGISTER VAR	*Vptr;

    for (Vptr = VarList; Vptr < &VarList[MAX_VARS]; Vptr++)
	if (Vptr->Name && EQ(Vptr->Name, Name))
	    return Vptr->Value;

    /* Try the environment. */
    Name = getenv(Name);
    return Name ? Name : "";
}


/*
**  Insert a variable/value pair into the list of variables.
*/
void
SetVar(Name, Value)
    REGISTER char	*Name;
    REGISTER char	*Value;
{
    REGISTER VAR	*Vptr;
    REGISTER VAR	*FreeVar;

    /* Skip leading whitespace in variable names, sorry... */
    while (ISwhit(*Name))
	Name++;

    /* Try to find the variable in the table. */
    for (Vptr = VarList, FreeVar = NULL; Vptr < &VarList[MAX_VARS]; Vptr++)
	if (Vptr->Name) {
	    if (EQ(Vptr->Name, Name)) {
		free(Vptr->Value);
		Vptr->Value = COPY(Value);
		return;
	    }
	}
	else if (FreeVar == NULL)
	    FreeVar = Vptr;

    if (FreeVar == NULL) {
	Fprintf(stderr, "Overflow, can't do '%s=%s'\n", Name, Value);
	SynErr("ASSIGNMENT");
    }
    FreeVar->Name = COPY(Name);
    FreeVar->Value = COPY(Value);
}


/*
**  Expand variable references inside a word that are of the form:
**	foo${var}bar
**	foo$$bar
**  Returns a pointer to a static area which is overwritten every
**  other time it is called, so that we can do EQ(Expand(a), Expand(b)).
*/
static char *
Expand(p)
    REGISTER char	*p;
{
    static char		buff[2][VAR_VALUE_SIZE];
    static int		Flag;
    REGISTER char	*q;
    REGISTER char	*n;
    REGISTER char	Closer;
    char		name[VAR_NAME_SIZE];

    /* This is a hack, but it makes things easier in DoTEST, q.v. */
    if (p == NULL)
	return p;

    /* Pick the "other" buffer then loop over the string to be expanded. */
    for (Flag = 1 - Flag, q = buff[Flag]; *p; )
	if (*p == '$')
	    if (*++p == '$') {
		Sprintf(name, "%d", Pid());
		q += strlen(strcpy(q, name));
		p++;
	    }
	    else if (*p == '?') {
		/* Fake it -- all commands always succeed, here. */
		*q++ = '0';
		*q = '\0';
		p++;
	    }
	    else {
		if (*p == '{') {
		    Closer = '}';
		    p++;
		}
		else
		    Closer = '\0';
		for (n = name; *p && *p != Closer; )
		    *n++ = *p++;
		if (*p)
		    p++;
		*n = '\0';
		q += strlen(strcpy(q, GetVar(name)));
	    }
	else
	    *q++ = *p++;
    *q = '\0';
    return buff[Flag];
}


/*
**  Do a variable assignment of the form:
**	var=value
**	var="quoted value"
**	var="...${var}..."
**	etc.
*/
static void
DoASSIGN(Name)
    REGISTER char	*Name;
{
    REGISTER char	*Value;
    REGISTER char	*q;
    REGISTER char	Quote;

    /* Split out into name=value strings, and deal with quoted values. */
    Value = IDX(Name, '=');
    *Value = '\0';
    if (ISquot(*++Value))
	for (Quote = *Value++, q = Value; *q && *q != Quote; q++)
	    continue;
    else
	for (q = Value; ISletr(*q); q++)
	    continue;
    *q = '\0';

    SetVar(Name, Expand(Value));
}

/**
***		" O U T P U T "   C O M M A N D S
**/

#ifndef	OPEN_OVERHEAD

#define CanOpenForWriting(name, mode)	fopen(name, mode)

#else

/*
**  Translate bad characters in a name into something more palatable.
**  This could be much more involved, but in practice it isn't very
**  necessary to do too much work, here.
*/
static void
Translate(Name)
    char		*Name;
{
    REGISTER char	*p;
    char		*Period;

    /* If we have an extension, remember it. */
    Period = RDX(Name, '.');

    /* Loop over the file, transliterating. */
    for (p = Period; *p; p++)
	switch (*p) {
	case '.': case ',':
	case '#': case '@':
	    *p = '_';
	    break;
	}

    /* Restore the extension. */
    if (Period)
	*Period = '.';
}


/*
**  Open a file for writing, "safely."
*/
static FILE *
CanOpenForWriting(Name, mode)
    char		*Name;
    char		*mode;
{
    register char	 *p;

#ifdef	NAME_CHECK
    Translate(Name);
#endif	/* NAME_CHECK */

    Fprintf(stderr, "Want to open \"%s\" for %s\n",
	    Name, *mode == 'w' ? "writing" : "appending");
#ifdef	MAX_FOPENS
    if (++CreatedCount >= MAX_FOPENS) {
	Fprintf(stderr, " -- would exceed limit of %d.\n", MAX_FOPENS);
	SynErr("FILE CREATION");
    }
#endif	/* MAX_FOPENS */

#ifdef	PATH_CHECK
    /* Absolute pathname? */
    if (Name[0] == '/'
     && YNQreply("!!!Absolute pathname, confirm [ynq] (q)") != 'T')
	SynErr("FILE CREATION");

    /* Check for going up... */
    for (bad = FALSE, p = Name; *p; p++)
	if (p[0] == '.' && p[1] == '.') {
	    bad++;
	    break;
	}
    if (bad && YNQreply("!!!Going upwards, confirm [ynq] (q)") != 'T')
	SynErr("FILE CREATION");

    /* See if it exists. */
    if (Fexists(Name) && YNQreply("!!!File exists, confirm [ynq] (q)") != 'T')
	SynErr("FILE CREATION");
#endif	/* PATH_CHECK */
}

#endif	/* OPEN_OVERHEAD */


/*
**  Do a cat command.  Understands the following:
**	cat >arg1 <<arg2
**	cat >>arg1 <<arg2
**	cat >>arg1 /dev/null
**  Except that arg2 is assumed to be quoted (no meta-char expansion
**  inside the "here" document).  The IO redirection can be in any order.
*/
/* ARGSUSED */
static int
DoCAT(ac, av)
    int			ac;
    REGISTER char	*av[];
{
    REGISTER FILE	*Out;
    REGISTER char	*Ending;
    REGISTER char	*Source;
    REGISTER int	V;
    REGISTER int	l;
#ifdef	MAX_LINES
    REGISTER long	c;
#endif	/* MAX_LINES */

    /* Parse the I/O redirecions. */
    for (V = TRUE, Source = NULL, Out = NULL, Ending = NULL; *++av; )
	if (EQ(*av, ">") && av[1]) {
	    av++;
	    /* This is a hack, but maybe MS-DOS doesn't have /dev/null? */
	    Out = Running ? CanOpenForWriting(Expand(*av), "w") : stderr;
	}
	else if (EQ(*av, ">>") && av[1]) {
	    av++;
	    /* And besides, things are actually faster this way. */
	    Out = Running ? CanOpenForWriting(Expand(*av), "a") : stderr;
	}
	else if (EQ(*av, "<<") && av[1]) {
	    for (Ending = *++av; *Ending == '\\'; Ending++)
		continue;
	    l = strlen(Ending);
	}
	else if (!EQ(Source = *av, "/dev/null"))
	    SynErr("CAT (bad input filename)");

    if (Out == NULL || (Ending == NULL && Source == NULL)) {
	Note("Missing parameter in CAT command.\n", (char *)NULL);
	V = FALSE;
    }

    /* Read the input, spit it out. */
    if (V && Running && Out != stderr) {
#ifdef	MAX_LINES
	c = 0;
#endif	/* MAX_LINES */
	if (Source == NULL)
	    while (GetLine(FALSE) && !EQn(Text, Ending, l)) {
#ifdef	MAX_LINES
		if (++c == MAX_LINES) {
		    Fprintf(stderr, "Line limit of %ld exceeded.\n",
			    (long)MAX_LINES);
		    SynErr("CAT");
		}
#endif	/* MAX_LINES */
		(void)fputs(Text, Out);
	    }
	(void)fclose(Out);
    }
    else
	while (GetLine(FALSE) && !EQn(Text, Ending, l))
	    continue;

    return V;
}


/*
**  Do a SED command.  Understands the following:
**	sed sX^yyyyXX >arg1 <<arg2
**	sed -e sX^yyyyXX >arg1 <<arg2
**  Where the yyyy is a miniscule regular expression; see Matches(), above.
**  The "X" can be any single character and the ^ is optional (sigh).  No
**  shell expansion is done inside the "here" document.  The IO redirection
**  can be in any order.
*/
/* ARGSUSED */
static int
DoSED(ac, av)
    int			ac;
    REGISTER char	*av[];
{
    REGISTER FILE	*Out;
    REGISTER char	*Pattern;
    REGISTER char	*Ending;
    REGISTER char	*p;
    REGISTER int	V;
    REGISTER int	l;
    REGISTER int	i;
#ifdef	MAX_LINES
    REGISTER long	c;
#endif	/* MAX_LINES */

    /* Parse IO redirection stuff. */
    for (V = TRUE, Out = NULL, Pattern = NULL, Ending = NULL; *++av; )
	if (EQ(*av, ">") && av[1]) {
	    av++;
	    Out = Running ? CanOpenForWriting(Expand(*av), "w") : stderr;
	}
	else if (EQ(*av, ">>") && av[1]) {
	    av++;
	    Out = Running ? CanOpenForWriting(Expand(*av), "a") : stderr;
	}
	else if (EQ(*av, "<<") && av[1]) {
	    for (Ending = *++av; *Ending == '\\'; Ending++)
		continue;
	    l = strlen(Ending);
	}
	else
	    Pattern = EQ(*av, "-e") && av[1] ? *++av : *av;

    /* All there? */
    if (Out == NULL || Ending == NULL || Pattern == NULL) {
	Note("Missing parameter in SED command.\n", (char *)NULL);
	V = FALSE;
    }

    /* Parse the substitute command and its pattern. */
    if (*Pattern != 's') {
	Note("Bad SED command -- not a substitute.\n", (char *)NULL);
	V = FALSE;
    }
    else {
	Pattern++;
	p = Pattern + strlen(Pattern) - 1;
	if (*p != *Pattern || *--p != *Pattern) {
	    Note("Bad substitute pattern in SED command.\n", (char *)NULL);
	    V = FALSE;
	}
	else {
	    /* Now check the pattern. */
	    if (*++Pattern == '^')
		Pattern++;
	    for (*p = '\0', i = strlen(Pattern), p = Pattern; *p; p++)
		if (*p == '[' || *p == '*' || *p == '$') {
		    Note("Bad meta-character in SED pattern.\n", (char *)NULL);
		    V = FALSE;
		}
	}
    }

    /* Spit out the input. */
    if (V && Running && Out != stderr) {
#ifdef	MAX_LINES
	c = 0;
#endif	/* MAX_LINES */
	while (GetLine(FALSE) && !EQn(Text, Ending, l)) {
#ifdef	MAX_LINES
	    if (++c == MAX_LINES) {
		Fprintf(stderr, "Line limit of %ld exceeded.\n",
			(long)MAX_LINES);
		SynErr("SED");
	    }
#endif	/* MAX_LINES */
	    (void)fputs(Matches(Pattern, Text) ? &Text[i] : Text, Out);
	}
	(void)fclose(Out);
    }
    else
	while (GetLine(FALSE) && !EQn(Text, Ending, l))
	    continue;

    return V;
}

/**
***		" S I M P L E "   C O M M A N D S
**/


/*
**  Parse a cp command of the form:
**	cp /dev/null arg
**  We should check if "arg" is a safe file to clobber, but...
*/
static int
DoCP(ac, av)
    int		ac;
    char	*av[];
{
    FILE	*F;

    if (Running) {
	if (ac != 3 || !EQ(av[1], "/dev/null"))
	    SynErr("CP");
	if ((F = CanOpenForWriting(Expand(av[2]), "w")) != NULL) {
	    (void)fclose(F);
	    return TRUE;
	}
	Note("Can't create %s.\n", av[2]);
    }
    return FALSE;
}


/*
**  Do a mkdir command of the form:
**	mkdir args... [>name] [>>name] [2>name]
*/
/* ARGSUSED */
static int
DoMKDIR(ac, av)
    int		ac;
    char	*av[];
{
    if (!Running)
	return FALSE;

    while (*++av) {
#ifdef	MAX_MKDIRS
	if (++DirCount >= MAX_MKDIRS) {
	    Fprintf(stderr, "Directory limit of %ld exceeded for \"%s\".\n",
		    MAX_MKDIRS, Expand(av[1]));
	    SynErr("mkdir");
	}
#endif	/* MAX_MKDIRS */
 	if (EQ(*av, ">") || EQ(*av, ">>"))
 	    av++;
 	else if (EQ(*av, "2") && EQ(av[1], ">"))
 	    av += 2;
	else if (mkdir(Expand(av[1]), 0777) < 0)
	    Note("Can't make directory %s.\n", av[1]);
    }
    return TRUE;
}


/*
**  Do a cd command of the form:
**	cd arg
**	chdir arg
*/
static int
DoCD(ac, av)
    int		ac;
    char	*av[];
{
    if (Running) {
	if (ac != 2)
	    SynErr("CD");
	if (chdir(Expand(av[1])) >= 0)
	    return TRUE;
	Note("Can't cd to %s.\n", av[1]);
    }
    return FALSE;
}


/*
**  Do the echo command.  Understands the "-n" hack.
*/
/* ARGSUSED */
static int
DoECHO(ac, av)
    int		ac;
    char	*av[];
{
    int		Flag;

    if (Running) {
	Flag = av[1] != NULL;
	if (Flag && EQ(av[1], "-n"))
	    av++;
	while (*++av)
	    Fprintf(stderr, "%s ", Expand(*av));
	if (!Flag)
	    Fprintf(stderr, "\n");
	(void)fflush(stderr);
    }
    return TRUE;
}


/*
**  Generic "handler" for commands we can't do.
*/
static int
DoIT(ac, av)
    int		ac;
    char	*av[];
{
    if (Running)
	Fprintf(stderr, "You'll have to do this yourself:\n\t%s ", *av);
    return DoECHO(ac, av);
}


/*
**  Handler for the comment command, :
*/
static int
DoCOMMENT(ac, av)
    int		ac;
    char	*av[];
{
    if (Running)
	Fprintf(stderr, "Shell comment:\n");
    return DoECHO(ac, av);
}


/*
**  Do an EXIT command.
*/
static int
DoEXIT(ac, av)
    int		ac;
    char	*av[];
{
    ac = *++av ? atoi(Expand(*av)) : 0;
    Fprintf(stderr, "Exiting, with status %d\n", ac);
#ifdef	USE_LONGJMP
    longjmp(jEnv, 1);
#else
    return ac;
#endif	/* USE_LONGJMP */
}


/*
**  Do an EXPORT command.  Often used to make sure the archive is being
**  unpacked with the Bourne (or Korn?) shell.  We look for:
**	export PATH blah blah blah
*/
static int
DoEXPORT(ac, av)
    int		ac;
    char	*av[];
{
    if (ac < 2 || !EQ(av[1], "PATH"))
	SynErr("EXPORT");
    return TRUE;
}

/**
***		F L O W - O F - C O N T R O L   C O M M A N D S
**/


/*
**  Parse a "test" statement.  Returns TRUE or FALSE.  Understands the
**  following tests:
**	test {!} -f arg		Is arg {not} a plain file?
**	test {!} -d arg		Is arg {not} a directory?
**	test {!} $var -eq $var	Is the variable {not} equal to the variable?
**	test {!} $var != $var	Is the variable {not} equal to the variable?
**	test {!} ddd -ne `wc -c {<} arg`
**				Is size of arg {not} equal to ddd in bytes?
**	test -f arg -a $var -eq val
**				Used by my shar, check for file clobbering
**  These last two tests are starting to really push the limits of what is
**  reasonable to hard-code, but they are common cliches in shell archive
**  "programming."  We also understand the [ .... ] way of writing test.
**  If we can't parse the test, we show the command and ask the luser.
*/
static int
DoTEST(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER char	**p;
    REGISTER char	*Name;
    REGISTER int	V;
    REGISTER int	i;

    /* Quick test. */
    if (!Running)
	return FALSE;

    /* See if we're called as "[ ..... ]" */
    if (EQ(*av, "[")) {
	for (i = 1; av[i] && !EQ(av[i], "]"); i++)
	    continue;
	if (av[i])
	    free(av[i]);
	av[i] = NULL;
	ac--;
    }

    /* Ignore the "test" argument. */
    av++;
    ac--;

    /* Inverted test? */
    if (EQ(*av, "!")) {
	V = FALSE;
	av++;
	ac--;
    }
    else
	V = TRUE;

    /* Testing for file-ness? */
    if (ac == 2
     && EQ(av[0], "-f")
     && (Name = Expand(av[1])))
	return GetStat(Name) && Ftype(Name) == F_FILE ? V : !V;

    /* Testing for directory-ness? */
    if (ac == 2
     && EQ(av[0], "-d")
     && (Name = Expand(av[1])))
	return GetStat(Name) && Ftype(Name) == F_DIR ? V : !V;

    /* Testing a variable's value? */
    if (ac == 3
     && (EQ(av[1], "-eq") || EQ(av[1], "=")))
	return EQ(Expand(av[0]), Expand(av[2])) ? V : !V;
    if (ac == 3
     && (EQ(av[1], "-ne") || EQ(av[1], "!=")))
	return !EQ(Expand(av[0]), Expand(av[2])) ? V : !V;

    /* Testing a file's size? */
    if (ac == (av[5] && EQ(av[5], "<") ? 8 : 7)
     && CTYPE(av[0][0]) && isdigit(av[0][0])
     && (EQ(av[1], "-ne") || EQ(av[1], "-eq"))
     && EQ(av[2], "`")
     && EQ(av[3], "wc")
     && EQ(av[4], "-c")
     && EQ(av[ac - 1], "`")) {
	if (GetStat(av[ac - 2])) {
	    if (EQ(av[1], "-ne"))
		return Fsize(av[ac - 2]) != atol(av[0]) ? V : !V;
	    return Fsize(av[ac - 2]) == atol(av[0]) ? V : !V;
	}
	Note("Can't get status of %s.\n", av[ac - 2]);
    }

    /* Testing for existing, but can clobber? */
    if (ac == 6
     && EQ(av[0], "-f")
     && EQ(av[2], "-a")
     && (EQ(av[4], "!=") || EQ(av[4], "-ne"))) {
	/* Gotta simplify this for some compilers.  People, too. :-) */
	if (GetStat(Name = Expand(av[1]))
	 && Ftype(Name) == F_FILE
	 && !EQ(Expand(av[3]), Expand(av[5])))
	    return V;
	return !V;
    }

    /* I give up -- print it out, and let's ask Mikey, he can do it... */
    Fprintf(stderr, "Can't parse this test:\n\t");
    for (i = FALSE, p = av; *p; p++) {
	Fprintf(stderr, "%s ", *p);
	if (p[0][0] == '$')
	    i = TRUE;
    }
    if (i) {
	Fprintf(stderr, "\n(Here it is with shell variables expanded...)\n\t");
	for (p = av; *p; p++)
	    Fprintf(stderr, "%s ", Expand(*p));
    }
    Fprintf(stderr, "\n");

    switch (YNQreply("Is value true/false/quit [tfq] (q)")) {
    case 'T':
	return TRUE;
    case 'F':
	return FALSE;
    }
    SynErr("TEST");
    /* NOTREACHED */
}


/*
**  Do until we reach a specific terminator.
*/
static void
DoUntil(Terminator, NewVal)
    char	*Terminator;
    int		NewVal;
{
    char	*av[MAX_WORDS];
    int		OldVal;

    for (OldVal = Running, Running = NewVal; GetLine(TRUE); )
	if (Argify(av)) {
	    if (EQ(av[0], Terminator))
		break;
	    (void)Exec(av);
	}

    Running = OldVal;
}


/*
**  Do an IF statement.
*/
static int
DoIF(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER char	**p;
    REGISTER int 	Flag;
    char		*vec[MAX_WORDS];
    char		**Pushed;

    /* Skip first argument. */
    if (!EQ(*++av, "[") && !EQ(*av, "test") && !EQ(*av, "/bin/test"))
	SynErr("IF");
    ac--;

    /* Look for " ; then " on this line, or "then" on next line. */
    for (Pushed = NULL, p = av; *p; p++) {
	Flag = EQ(*p, ";");
	if (Flag) {
	    if (p[1] == NULL || !EQ(p[1], "then"))
		SynErr("IF");
	    *p = NULL;
	    ac -= 2;
	    break;
	}
    }
    if (!Flag) {
	(void)GetLine(TRUE);
	if (Argify(vec) > 1)
	    Pushed = &vec[1];
	if (!EQ(vec[0], "then"))
	    SynErr("IF (missing THEN)");
    }

    if (DoTEST(ac, av)) {
	if (Pushed)
	    (void)Exec(Pushed);
	while (GetLine(TRUE)) {
	    if ((ac = Argify(vec)) == 1 && EQ(vec[0], "fi"))
		break;
	    if (EQ(vec[0], "else")) {
		DoUntil("fi", FALSE);
		break;
	    }
	    (void)Exec(vec);
	}
    }
    else
	while (GetLine(TRUE)) {
	    if ((ac = Argify(vec)) == 1 && EQ(vec[0], "fi"))
		break;
	    if (EQ(vec[0], "else")) {
		if (ac > 1)
		    (void)Exec(&vec[1]);
		DoUntil("fi", Running);
		break;
	    }
	}
    return TRUE;
}


/*
**  Do a FOR statement.
*/
static int
DoFOR(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER char	*Var;
    REGISTER char	**Values;
    REGISTER int	Found;
    long		Here;
    char		*vec[MAX_WORDS];

    /* Check usage, get variable name and eat noise words. */
    if (ac < 4 || !EQ(av[2], "in"))
	SynErr("FOR");
    Var = av[1];
    ac -= 3;
    av += 3;

    /* Look for "; do" on this line, or just "do" on next line. */
    for (Values = av; *++av; ) {
	Found = EQ(*av, ";");
	if (Found) {
	    if (av[1] == NULL || !EQ(av[1], "do"))
		SynErr("FOR");
	    *av = NULL;
	    break;
	}
    }
    if (!Found) {
	(void)GetLine(TRUE);
	if (Argify(vec) != 1 || !EQ(vec[0], "do"))
	    SynErr("FOR (missing DO)");
    }

    for (Here = ftell(Input); *Values; ) {
	SetVar(Var, *Values);
	DoUntil("done", Running);
	    ;
	/* If we're not Running, only go through the loop once. */
	if (!Running)
	    break;
	if (*++Values && (fseek(Input, Here, 0) < 0 || ftell(Input) != Here))
	    SynErr("FOR (can't seek back)");
    }

    return TRUE;
}


/*
**  Do a CASE statement of the form:
**	case $var in
**	    text1)
**		...
**		;;
**	esac
**  Where text1 is a simple word or an asterisk.
**  In particular, these patterns aren't handled:
**	    text1|text2)
**	    text1) .... ;;
*/
static int
DoCASE(ac, av)
    REGISTER int	ac;
    REGISTER char	*av[];
{
    REGISTER int	FoundIt;
    char		*vec[MAX_WORDS];
    char		Value[VAR_VALUE_SIZE];

    if (ac != 3 || !EQ(av[2], "in"))
	SynErr("CASE");
    (void)strcpy(Value, Expand(av[1]));

    for (FoundIt = FALSE; GetLine(TRUE); ) {
	ac = Argify(vec);
	if (EQ(vec[0], "esac"))
	    break;
	/* This is for vi: (-; sigh. */
	if (!EQ(vec[1], ")"))
	    SynErr("CASE");
	if (!FoundIt && (EQ(vec[0], Value) || EQ(vec[0], "*"))) {
	    FoundIt = TRUE;
	    if (Running && ac > 2)
		(void)Exec(&vec[2]);
	    DoUntil(";;", Running);
	}
	else
	    DoUntil(";;", FALSE);
    }
    return TRUE;
}



/*
**  Dispatch table of known commands.
*/
static COMTAB	 Dispatch[] = {
    {	"cat",		DoCAT		},
    {	"case",		DoCASE		},
    {	"cd",		DoCD		},
    {	"chdir",	DoCD		},
    {	"chmod",	DoIT		},
    {	"cp",		DoCP		},
    {	"echo",		DoECHO		},
    {	"exit",		DoEXIT		},
    {	"export",	DoEXPORT	},
    {	"for",		DoFOR		},
    {	"if",		DoIF		},
    {	"mkdir",	DoMKDIR		},
    {	"rm",		DoIT		},
    {	"sed",		DoSED		},
    {	"test",		DoTEST		},
    {	"[",		DoTEST		},
    {	":",		DoCOMMENT	},
    {	"",		NULL		}
};


/*
**  Dispatch on a parsed line.
*/
int
Exec(av)
    REGISTER char	*av[];
{
    REGISTER int	i;
    REGISTER COMTAB	*p;

    /* We have to re-calculate this because our callers can't always
       pass the count down to us easily. */
    for (i = 0; av[i]; i++)
	continue;
    if (i) {
	/* Is this a command we know? */
	for (p = Dispatch; p->Func; p++)
	    if (EQ(av[0], p->Name)) {
		i = (*p->Func)(i, av);
		if (p->Func == DoEXIT)
		    /* Sigh; this is a hack. */
		    return OOB_FALSE;
		break;
	    }

	/* If not a command, try it as a variable assignment. */
	if (p->Func == NULL)
	    /* Yes, we look for "=" in the first word, but pass down
	       the whole line. */
	    if (IDX(av[0], '='))
		DoASSIGN(Text);
	    else
		Note("Command %s unknown.\n", av[0]);

	/* Free the line. */
	for (i = 0; av[i]; i++)
	    free(av[i]);
    }
    return TRUE;
}
