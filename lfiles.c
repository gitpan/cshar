/*
**  File-related routines.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifndef	VMS
#include <sys/stat.h>
#else
#include <stat.h>
#endif	/* VMS */
#ifdef	RCSID
static char RCS[] =
	"$Id: lfiles.c,v 3.0.3.2 1993/08/25 17:04:39 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: lfiles.c,v $
 * Revision 3.0.3.2  1993/08/25  17:04:39  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:32:51  ram
 * 3.0 baseline (ram).
 * 
 */

#ifndef	F_OK
#define F_OK	0
#endif	/* F_OK */

/* Mask of executable bits. */
#define	EXE_MASK	(S_IEXEC | (S_IEXEC >> 3) | (S_IEXEC >> 6))

/* Stat buffer for last file. */
static struct stat	Sb;


/*
**  Stat the file.  We used to save the old name and only do the stat(2)
**  if the name changed, but that's wrong -- we could open and write to
**  the file between stat() calls.
*/
int
GetStat(p)
    char	*p;
{
    return stat(p, &Sb) < 0 ? FALSE : TRUE;
}


/*
**  See if the file exists.  Dumb test, not suitable for use everywhere.
*/
int
Fexists(p)
    char	*p;
{
    return access(p, F_OK) >= 0 || GetStat(p);
}


/*
**  Return the file type -- directory or regular file.
*/
int
Ftype(p)
    char	*p;
{
    return GetStat(p) && ((Sb.st_mode & S_IFMT) == S_IFDIR) ? F_DIR : F_FILE;
}


/*
**  Return the file size.
*/
off_t
Fsize(p)
    char	*p;
{
#ifdef	UNIX_FILES
    return GetStat(p) ? Sb.st_size : 0;
#else
    REGISTER FILE	*F;
    REGISTER off_t	 i;

    if ((F = fopen(p, "r")) == NULL)
	return 0;
    for (i = 0; getc(F) != EOF; i++)
	;
    (void)fclose(F);
    return i;
#endif	/* UNIX_FILES */
}


/*
**  Is a file executable?
*/
int
Fexecute(p)
    char	*p;
{
    return GetStat(p) && (Sb.st_mode & EXE_MASK) ? TRUE : FALSE;
}


/*
**  Does it look like a source file (ends with *.[cFflpy])?
*/
int
IsProbablySource(p)
    register char	*p;
{
    if ((p = RDX(p, '.')) && p[1] != '\0' && p[2] == '\0')
	switch (p[1]) {
	case 'c':
	case 'F':
	case 'f':
	case 'l':
	case 'p':
	case 'y':
	    return TRUE;
	}

    return FALSE;
}


/*
**  Rename a file so that we don't overwrite it.
*/
#ifdef	BU_NONE
void
SafeRename(Name)
    char	*Name;
{
    Name = Name;
}

#else

void
SafeRename(Name)
    char		*Name;
{
#ifdef	BU_VIA_COPY
    REGISTER FILE	*In;
    REGISTER FILE	*Out;
#endif	/* BU_VIA_COPY */
#ifdef	BU_NAME_LEN
    REGISTER char	*p;
#endif	/* BU_NAME_LEN */
    char		buff[BUFSIZ];

#ifdef	BU_PREFIX
    /* Turn /foo/bar/baz into /foo/bar/B-baz. */
    if (p = RDX(buff, '/')) {
	*p = '\0';
	Sprintf(buff, "%s/%s%s", buff, BU_PREFIX, p + 1);
	*p = '/';
    }
    else
	Sprintf(buff, "%s%s", BU_PREFIX, Name);
#endif	/* BU_PREFIX */

#ifdef	BU_SUFFIX
    /* Turn /foo/bar/baz into /foo/bar/baz.BAK */
    Sprintf(buff, "%s%s", Name, BU_SUFFIX);
#ifdef	BU_NAME_LEN
    /* If on a shortname system, turn longfilename.BAK into longfilena.BAK */
    p = (p = RDX(buff, '/')) ? p + 1 : buff;
    if (strlen(p) > BU_NAME_LEN)
	(void)strcpy(&p[BU_NAME_LEN - strlen(BU_SUFFIX)], BU_SUFFIX);
#endif	/* BU_NAME_LEN */
#endif	/* BU_SUFFIX */

#ifdef	BU_VIA_RENAME
    (void)rename(Name, buff);
#endif	/* BU_VIA_RENAME */

#ifdef	BU_VIA_LINK
    (void)unlink(buff);
    (void)link(Name, buff);
    unlink(Name);
#endif	/* BU_VIA_LINK */

#ifdef	BU_VIA_COPY
    /* Open the old and new files. */
    if ((In = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for reading to copy, %s.\n",
		Name, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }
    if ((Out = fopen(buff, "w")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for writing to copy, %s.\n",
		Name, strerror(errno));
	(void)fclose(In);
	exit(1);
	/* NOTREACHED */
    }

    /* Slurp and spew. */
    while (fgets(buff, sizeof buff, In))
	(void)fputs(buff, Out);

    /* Close. */
    (void)fclose(In);
    (void)fclose(Out);
#endif	/* BU_VIA_COPY */
}
#endif	/* BU_NONE */


/*
**  Make a complete pathname to a temporary file.
*/
void
MakeTempName(buff, Name)
    char		*buff;
    char		*Name;
{
#ifdef	TEMPVAR
    register char	*p;
    register int	i;

    /* find the temporary directory */
    if ((p = getenv(TEMPVAR)) == NULL)
	(void)strcpy(buff, Name);
    else {
	/* Guarantee a directory separator. */
	i = strlen(p);
	if (p[i - 1] != '/' && p[i - 1] != '\\' && *Name != '/' && *Name != '\\')
	    Sprintf(buff, "%s/%s", p, Name);
	else
	    Sprintf(buff, "%s%s", p, Name);
    }
    (void)mktemp(buff);
#else
    (void)mktemp(strcpy(buff, Name));
#endif	/* TEMPVAR */
}
