/*
**  Readdir package for MS-DOS.
**  
**  [  I have not tried this at all except to reformat it.  --r$  ]
**
**  Public domain implementation Michael Rendell <garfield!michael>
**  August 1897.
**
**  $Id: dirmsd.c,v 3.0.3.3 1993/08/25 17:04:30 ram Exp $
*/
/*
 * $Log: dirmsd.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:30  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:49:58  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:30:27  ram
 * 3.0 baseline (ram).
 * 
 */
#include "ndir.h"
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <dos.h>

#ifndef	NULL
#define NULL		0
#endif	/* NULL */

#ifndef	MAXPATHLEN
#define MAXPATHLEN	255
#endif	/* MAXPATHLEN */

/*
**  Attribute stuff.
*/
#define A_RONLY		0x01
#define A_HIDDEN	0x02
#define A_SYSTEM	0x04
#define A_LABEL		0x08
#define A_DIR		0x10
#define A_ARCHIVE	0x20

/*
**  DOS call values.
*/
#define DOSI_FINDF	0x4e
#define DOSI_FINDN	0x4f
#define DOSI_SDTA	0x1a

#define ATTRIBUTES	(A_DIR | A_HIDDEN | A_SYSTEM)

/*
**  What the "find first" and "find next" calls use.
*/
typedef struct _dtabuf {
    char		d_buf[21];
    char		d_attribute;
    unsigned short	d_time;
    unsigned short	d_date;
    long		d_size;
    char		d_name[13];
} DTABUF;


static DTABUF		dtabuf;
static DTABUF		*dtapnt = &dtabuf;
static union REGS	reg;
static union REGS	nreg;

#ifdef	M_I86LM
static struct SREGS	sreg;
#endif	/* M_I86LM */


static char *
getdirent(dir)
    char	*dir;
{
    if (dir) {
	/* get first entry */
	reg.h.ah = DOSI_FINDF;
	reg.h.cl = ATTRIBUTES;
#ifdef	M_I86LM
	reg.x.dx = FP_OFF(dir);
	sreg.ds = FP_SEG(dir);
#else
	reg.x.dx = (unsigned int)dir;
#endif	/* M_I86LM */
    }
    else {
	/* get next entry */
	reg.h.ah = DOSI_FINDN;
#ifdef	M_I86LM
	reg.x.dx = FP_OFF(dtapnt);
	sreg.ds = FP_SEG(dtapnt);
#else
	reg.x.dx = (unsigned int)dtapnt;
#endif	/* M_I86LM */
    }
#ifdef	M_I86LM
    intdosx(&reg, &nreg, &sreg);
#else
    intdos(&reg, &nreg);
#endif	/* M_I86LM */

    return nreg.x.cflag ? NULL : dtabuf.d_name;
}


static void
free_dircontents(dp)
    _DIRCONTENTS	*dp;
{
    _DIRCONTENTS	*new;

    for (; dp; dp = new) {
	if (dp->_d_entry)
	    free(dp->_d_entry);
	new = dp->_d_next;
	free((char *)dp);
    }
}

static void
setdta()
{
    reg.h.ah = DOSI_SDTA;
#ifdef	M_I86LM
    reg.x.dx = FP_OFF(dtapnt);
    sreg.ds = FP_SEG(dtapnt);
    intdosx(&reg, &nreg, &sreg);
#else
    reg.x.dx = (int)dtapnt;
    intdos(&reg, &nreg);
#endif	/* M_I86LM */
}


DIR *
opendir(name)
    char		*name;
{
    DIR			*dirp;
    _DIRCONTENTS	*dp;
    struct stat		statb;
    char		*s;
    char		c;
    char		nbuf[MAXPATHLEN + 1];

    if (stat(name, &statb) < 0 || (statb.st_mode & S_IFMT) != S_IFDIR)
	return NULL;
    if ((dirp = (DIR *)malloc(sizeof (DIR))) == NULL)
	return NULL;

    if (*name && (c = name[strlen(name) - 1]) != '\\' && c != '/')
	(void)strcat(strcpy(nbuf, name), "\\*.*");
    else
	(void)strcat(strcpy(nbuf, name), "*.*");

    dirp->dd_loc = 0;
    setdta();
    dirp->dd_contents = dirp->dd_cp = NULL;
    if ((s = getdirent(nbuf)) == NULL)
	return dirp;

    do {
	if ((dp = (_DIRCONTENTS *)malloc(sizeof (_DIRCONTENTS))) == NULL
	|| (dp->_d_entry = malloc((unsigned int)(strlen(s) + 1))) == NULL) {
	    if (dp)
		free((char *)dp);
	    free_dircontents(dirp->dd_contents);
	    return NULL;
	}
	if (dirp->dd_contents)
	    dirp->dd_cp = dirp->dd_cp->_d_next = dp;
	else
	    dirp->dd_cp = dirp->dd_contents = dp;
	(void) strcpy(dp->_d_entry, s);
	dp->_d_next = NULL;
    } while (s = getdirent((char *)NULL));
    dirp->dd_cp = dirp->dd_contents;

    return dirp;
}


void
closedir(dirp)
    DIR		*dirp;
{
    free_dircontents(dirp->dd_contents);
    free((char *)dirp);
}

struct direct *
readdir(dirp)
    DIR				*dirp;
{
    static struct direct	dp;

    if (dirp->dd_cp == NULL)
	return NULL;
    dp.d_namlen = dp.d_reclen =
	    strlen(strcpy(dp.d_name, dirp->dd_cp->_d_entry));
    dp.d_ino = 0;
    dirp->dd_cp = dirp->dd_cp->_d_next;
    dirp->dd_loc++;
    return &dp;
}


void
seekdir(dirp, off)
    DIR			*dirp;
    long		off;
{
    long		i;
    _DIRCONTENTS	*dp;

    if (off < 0)
	return;
    for (i = off, dp = dirp->dd_contents; --i >= 0 && dp; dp = dp->_d_next)
	;
    dirp->dd_loc = off - (i + 1);
    dirp->dd_cp = dp;
}


long
telldir(dirp)
    DIR		*dirp;
{
    return dirp->dd_loc;
}
