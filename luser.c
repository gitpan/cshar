/*
**  Get user name.  Something for everyone.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	USE_GETPWUID
#include <pwd.h>
#endif	/* USE_GETPWUID */
#ifdef	RCSID
static char RCS[] =
	"$Id: luser.c,v 3.0.3.3 1993/08/25 17:04:42 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: luser.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:42  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:50:56  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:33:45  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Get user name.  Not secure, but who sends nastygrams as shell archives?
*/
char *
User()
{
#ifdef	USE_GETPWUID
    extern struct passwd	*getpwuid();
    struct passwd		*p;
#endif	/* USE_GETPWUID */
    char			*g;

    if ((g = getenv("USER")) != NULL)
	return g;
    if ((g = getenv("LOGNAME")) != NULL)
	return g;
    if ((g = getenv("NAME")) != NULL)
	return g;
#ifdef	USE_GETPWUID
    if ((p = getpwuid(getuid())) != NULL)
	return p->pw_name;
#endif	/* USE_GETPWUID */
    return USERNAME;
}
