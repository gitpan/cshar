/*
**  Header file for shar and friends.
**
**  If you have to edit this file, then I messed something up -- please
**  let me know what.
**
**  $Id: shar.h,v 3.0.3.3 1993/08/25 17:04:57 ram Exp $
*/
/*
 * $Log: shar.h,v $
 * Revision 3.0.3.3  1993/08/25  17:04:57  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:51:39  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:37:31  ram
 * 3.0 baseline (ram).
 * 
 */
/* SUPPRESS 223 *//* Nested comment */
#include "config.h"
#include "ramconf.h"

#ifdef	ANSI_HDRS
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <time.h>
#endif	/* ANSI_HDRS */

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

#ifdef	VMS
#include <types.h>
#else
#include <sys/types.h>
#endif	/* VMS */

/*
**  Let's play "find the header file."
*/
#ifdef	IN_SYS_DIR
#include <sys/dir.h>
#endif	/* IN_SYS_DIR */
#ifdef	IN_DIR
#include <dir.h>
#endif	/* IN_DIR */
#ifdef	IN_DIRECT
#include <direct.h>
#endif	/* IN_DIRECT */
#ifdef	IN_SYS_NDIR
#include <sys/ndir.h>
#endif	/* IN_SYS_NDIR */
#ifdef	IN_NDIR
#include "ndir.h"
#endif	/* IN_NDIR */
#ifdef	IN_DIRENT
#include <dirent.h>
#endif	/* IN_DIRENT */


/*
**  Handy shorthands.
*/
#define TRUE		1		/* What I tell you three times	*/
#define FALSE		0		/* ... is false			*/
#define OOB_FALSE	3		/* "Emergency" exit from unshar	*/
#define WIDTH		72		/* Contents lines in shar.c	*/
#define F_DIR		36		/* Something is a directory	*/
#define F_FILE		65		/* Something is a regular file	*/
#define SAFE_WIDTH	78		/* Warn if longer than this	*/
#define READ_CHUNK	150		/* Size of binary-test chunk	*/
#define UULINE_SIZE	45		/* Size of uuencode chunk	*/
#define UUTABLE_SIZE	127		/* Character mapping table	*/
#define UUCHAR_DATA	64		/* Characters used in map	*/


/*
** These are used by the archive parser or findsrc.
*/
#define LINE_SIZE	200		/* Length of physical input line*/
#define MAX_VARS	 20		/* Number of shell vars allowed	*/
#define MAX_WORDS	 30		/* Make words in command lnes	*/
#define VAR_NAME_SIZE	 30		/* Length of a variable's name	*/
#define VAR_VALUE_SIZE	128		/* Length of a variable's value	*/
#define MAX_PATTERNS	 60		/* Filename patterns in table	*/


#ifndef	L_ctermid
#define L_ctermid	50
#endif	/* L_ctermid */

/*
**  Lint placation.
*/
#ifdef	lint
#undef RCSID
#undef putc
#undef putchar
#endif	/* lint */
#define Printf		(void)printf
#define Fprintf		(void)fprintf
#define Sprintf		(void)sprintf


/*
**  Saber placation.
*/
#ifdef	SABER
#undef RCSID
#endif	/* SABER */

/*
**  Memory hacking.
*/
#define NEW(T, count)	((T *)shar_getmem(sizeof (T), (unsigned int)(count)))
#define COPY(s)		strcpy(NEW(char, strlen((s)) + 1), (s))


/*
**  Macros.
*/
#define BADCHAR(c)	(!isascii((c)) || (iscntrl((c)) && !isspace((c))))
#define HDRCHAR(c)	((c) == '-' || (c) == '_' || (c) == '.')
#define EQ(a, b)	(strcmp((a), (b)) == 0)
#define EQn(a, b, n)	(strncmp((a), (b), (n)) == 0)
#define PREFIX(a, b)	(EQn((a), (b), sizeof b - 1))
#define WHITE(c)	((c) == ' ' || (c) == '\t')


/*
**  Linked in later.
*/
extern int	 optind;
extern char	*optarg;

#ifndef	ANSI_HDRS
extern int	 errno;

/* From your C run-time library. */
extern FILE	*popen();
extern time_t	time();
extern long	atol();
extern char	*IDX();
extern char	*RDX();
extern char	*ctime();
extern char	*gets();
extern char	*mktemp();
extern char	*strcat();
extern char	*strcpy();
extern char	*strncpy();
extern char   	*getenv();
#ifdef	PTR_SPRINTF
extern char	*sprintf();
#endif	/* PTR_SPRINTF */
#ifdef	USE_SYSERRLIST
extern int	sys_nerr;
extern char	*sys_errlist[];
#endif	/* USE_SYSERRLIST */

#endif	/* ANSI_HDRS */


/* From our local library. */

#ifdef	DECLARE_PROTOTYPES
#define PARMS(x)	(x)
#else
#define PARMS(x)	()
#endif	/* DECLARE_PROTOTYPES */

extern char	*strerror PARMS( (int) );
extern char	*GetDir PARMS( (char *, int) );
extern char	*Host PARMS( (void) );
extern char	*Seechar PARMS( (int) );
extern char	*User PARMS( (void) );
extern align_t	shar_getmem PARMS( (int, unsigned int) );
extern off_t	Fsize PARMS( (char *) );
extern int	Argify PARMS( (char **) );
extern int	Exec PARMS( (char **) );
extern int	Execute PARMS( (char **) );
extern int	Fexecute PARMS( (char *) );
extern int	Fexists PARMS( (char *) );
extern int	Ftype PARMS( (char *) );
extern int	GetLine PARMS( (int) );
extern int	GetStat PARMS( (char *) );
extern int	IsProbablySource PARMS( (char *) );
extern void	MakeTempName PARMS( (char *, char *) );
extern int	Pid PARMS( (void) );
extern void	SafeRename PARMS( (char *) );
extern void	SetSigs PARMS( (sigret_t (*)()) );
extern int	getopt PARMS( (int, char **, char *) );
extern void	SetVar PARMS( (char *, char *) );
extern void	SynErr PARMS( (char *) );
extern void	Version PARMS( (char *) );
extern void	uuencode PARMS( (char *, char *) );
