/*
**  Return current working directory.  Something for everyone.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Id: lcwd.c,v 3.0.3.2 1993/08/25 17:04:38 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: lcwd.c,v $
 * Revision 3.0.3.2  1993/08/25  17:04:38  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:32:22  ram
 * 3.0 baseline (ram).
 * 
 */

#ifdef	PWDGETENV
/* ARGSUSED */
char *
GetDir(p, i)
    char	*p;
    int		i;
{
    char	*q;

    return (q = getenv(PWDGETENV)) ? strcpy(p, q) : NULL;
}
#endif	/* PWDGETENV */


#ifdef	HAS_GETWD
/* ARGSUSED1 */
char *
GetDir(p, size)
    char	*p;
    int		size;
{
    extern char	*getwd();

    return getwd(p) ? p : NULL;
}
#endif	/* HAS_GETWD */


#ifdef	GETCWD
char *
GetDir(p, size)
    char	*p;
    int		size;
{
    extern char	*getcwd();

    return getcwd(p, size) ? p : NULL;
}
#endif	/* GETCWD */


#ifdef	PWDPOPEN
char *
GetDir(p, size)
    char	*p;
    int		size;
{
    extern FILE	*popen();
    FILE	*F;
    int		i;

    /* This string could be "exec /bin/pwd" if you want... */
    if (F = popen("pwd", "r")) {
	if (fgets(p, size, F) && p[i = strlen(p) - 1] == '\n') {
	    p[i] = '\0';
	    (void)fclose(F);
	    return p;
	}
	(void)fclose(F);
    }
    return NULL;
}
#endif	/* PWDPOPEN */
