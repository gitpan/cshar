/*
**  UUENCODE
**
**  Uuencode a file.  This is based on the public-domain implementation that
**  Mark Horton released to mod.sources with the translation table written
**  by Jean-Pierre H. Dumas.
*/
#include "shar.h"
#ifndef	VMS
#include <sys/stat.h>
#else
#include <stat.h>
#endif	/* VMS */
#ifdef	RCSID
static char RCS[] =
	"$Id: uuencode.c,v 3.0.3.2 1993/08/25 17:05:05 ram Exp $";
#endif	/* RCSID */
/*
 * $Log: uuencode.c,v $
 * Revision 3.0.3.2  1993/08/25  17:05:05  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:39:32  ram
 * 3.0 baseline (ram).
 * 
 */

/* Encode one character into printable. */
#define ENC(C)		((C) ? ((C) & 077) + ' ': '`')


/*
**
*/
void
uuencode(Name, Newname)
    char		*Name;
    char		*Newname;
{
    register char	*p;
    register FILE	*Out;
    register FILE	*In;
    register int	i;
    register int	c;
    register int	n;
    struct stat		Sb;
    char		Outbuffer[BUFSIZ];
    char		buff[UULINE_SIZE + 3];

    /* Open input. */
    if ((In = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for input, %s.\n",
		Name, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }

    /* Open the buffered output. */
    if ((Out = fopen(Newname, "w")) == NULL) {
	Fprintf(stderr, "Can't open \"%s\" for output, %s.\n",
		buff, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }
    setbuf(Out, Outbuffer);

    /* Put out a translation table. */
    Fprintf(Out, "table\n");
    for (i = ' '; i <= '_'; i++) {
	(void)putc(i, Out);
	if (i == ' ' + 31)
	    (void)putc('\n', Out);
    }
    (void)putc('\n', Out);

    /* Print the file's mode and name. */
    if (fstat(fileno(In), &Sb) < 0) {
	Fprintf(stderr, "Can't get modes of \"%s\", %s.\n",
		Name, strerror(errno));
	exit(1);
	/* NOTREACHED */
    }
    Fprintf(Out, "begin %o %s\n", Sb.st_mode & 0777, Name);

    do {
	/* Read one line, put out the length. */
	n = fread(buff, sizeof (char), UULINE_SIZE, In);
	(void)putc(ENC(n), Out);

	/* Put out the line. */
	for (p = buff, i = 0; i < n; i += 3, p += 3) {
	    c = (p[0] >> 2);
	    (void)putc(ENC(c), Out);
	    c = (p[0] << 4) & 060 | (p[1] >> 4) & 017;
	    (void)putc(ENC(c), Out);
	    c = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	    (void)putc(ENC(c), Out);
	    c = p[2] & 077;
	    (void)putc(ENC(c), Out);
	}
	(void)putc('\n', Out);
    } while (n > 0);

    Fprintf(Out, "end\n");

    (void)fclose(In);
    (void)fclose(Out);
}
