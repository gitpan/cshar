/*
**  Subroutine to call the shell archive parser.  This is "glue" that holds
**  unshar and parser together.
*/
/*
 * $Log: glue.c,v $
 * Revision 3.0.3.2  1993/08/25  17:04:37  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:32:10  ram
 * 3.0 baseline (ram).
 * 
 */
#include "parser.h"
#ifdef	RCSID
static char RCS[] =
	"$Id: glue.c,v 3.0.3.2 1993/08/25 17:04:37 ram Exp $";
#endif	/* RCSID */


#ifdef	USE_MY_SHELL

#ifdef	MSDOS
/*
**  MS-DOS has a global filemode variable that we need to flip around a bit.
*/
static int	OldMode;
#endif	/* MSDOS */

/*
**  Cleanup routine after BinSh is done
*/
void
BSclean()
{
    (void)fclose(Input);
#ifdef	MSDOS
    _fmode = OldMode;
#endif	/* MSDOS */
    if (File[0])
	(void)unlink(File);
}


/*
**  Copy the input to a temporary file, then call the shell parser.
*/
BinSh(Name, Stream, Pushback)
    char		*Name;
    REGISTER FILE	*Stream;
    char		*Pushback;
{
    REGISTER FILE	*F;
    char		buff[BUFSIZ];
    char		*vec[MAX_WORDS];

    Interactive = Name == NULL;

#ifdef	MSDOS
    onexit(BSclean);
    old_fmode = _fmode;
    _fmode = O_BINARY;
#endif	/* MSDOS */

    MakeTempName(File, TEMP_NAME1);
    if ((F = fopen(File, "w")) == NULL) {
	Fprintf(stderr," Can't create \"%s\" as temp file, %s.\n",
		File, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }

    (void)fputs(Pushback, F);
    while (fgets(buff, sizeof buff, Stream))
	(void)fputs(buff, F);
    (void)fclose(Stream);
    (void)fclose(F);

    if ((Input = fopen(File, "r")) == NULL)
	Fprintf(stderr, "Can't open %s, %s!?\n", File, strerror(errno));
    else {
	while (GetLine(TRUE)) {
#ifdef	USE_JMPBUF
	    if (setjmp(jEnv))
		break;
#endif	/* USE_JMPBUF */
	    if (Argify(vec) && Exec(vec) == OOB_FALSE)
		    break;
	}
    }

    BSclean();
}
#endif	/* USE_MY_SHELL */
