/*
**  Return the name of this host.  Something for everyone.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Id: lhost.c,v 3.0.3.3 1993/12/21 08:49:29 ram Exp $";
#endif	/* RCSID */

/*
 * $Log: lhost.c,v $
 * Revision 3.0.3.3  1993/12/21  08:49:29  ram
 * patch15: prioritized definition of the Host function
 *
 * Revision 3.0.3.2  1993/08/25  17:03:24  ram
 * patch12: forgot this one too...
 *
 * Revision 3.0.3.1  91/01/21  11:33:05  ram
 * 3.0 baseline (ram).
 * 
 */

#ifdef	HAS_GETHOSTNAME
char *
Host()
{
    static char		buff[64];

    (void)gethostname(buff, sizeof buff);
    return buff;
}
#else
#ifdef	HAS_UNAME
#include <sys/utsname.h>
char *
Host()
{
    static struct utsname	U;

    return uname(&U) < 0 ? DEF_HOST : U.nodename;
}
#else
#ifdef	UUNAME
char *
Host()
{
    static char		buff[50];
    extern FILE		*popen();
    FILE		*F;
    char		*p;

    if (F = popen("exec uuname -l", "r")) {
	if (fgets(buff, sizeof buff, F) == buff && (p = IDX(buff, '\n'))) {
	    (void)pclose(F);
	    *p = '\0';
	    return buff;
	}
	(void)pclose(F);
    }
    return DEF_HOST;
}
#else
#ifdef	I_WHOAMI
char *
Host()
{
    static char		name[64];
    REGISTER FILE	*F;
    REGISTER char	*p;
    char		buff[100];

    /* Try /etc/whoami; look for a single well-formed line. */
    if (F = fopen("/etc/whoami", "r")) {
	if (fgets(name, sizeof name, F) && (p = IDX(name, '\n'))) {
	    (void)fclose(F);
	    *p = '\0';
	    return name;
	}
	(void)fclose(F);
    }

    /* Try /usr/include/whoami.h; look for #define sysname "foo" somewhere. */
    if (F = fopen("/usr/include/whoami.h", "r")) {
	while (fgets(buff, sizeof buff, F))
	    /* I don't like sscanf, nor do I trust it.  Sigh. */
	    if (sscanf(buff, "#define sysname \"%[^\"]\"", name) == 1) {
		(void)fclose(F);
		return name;
	    }
	(void)fclose(F);
    }

    return DEF_HOST;
}
#else
#ifdef	HOST_STRING
char *
Host()
{
    return DEF_HOST;
}
#endif	/* HOST_STRING */
#endif	/* I_WHOAMI */
#endif	/* UUNAME */
#endif	/* HAS_UNAME */
#endif	/* HAS_GETHOSTNAME */

