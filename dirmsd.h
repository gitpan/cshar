/*
**  Readdir package for MS-DOS.
**  
**  [  I have not tried this at all except to reformat it.  --r$  ]
**
**  Public domain implementation Michael Rendell <garfield!michael>
**  August 1897.
**
**  $Id: dirmsd.h,v 3.0.3.3 1993/08/25 17:04:31 ram Exp $
*/
/*
 * $Log: dirmsd.h,v $
 * Revision 3.0.3.3  1993/08/25  17:04:31  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:50:06  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:30:40  ram
 * 3.0 baseline (ram).
 * 
 */
#include <sys/types.h>

#define MAXNAMLEN	12

typedef struct _dircontents {
	char			*_d_entry;
	struct _dircontents	*_d_next;
} _DIRCONTENTS;

struct direct {
	ino_t	d_ino;			/* a bit of a farce */
	int	d_reclen;		/* more farce */
	int	d_namlen;		/* length of d_name */
	char	d_name[MAXNAMLEN + 1];	/* gaurantee null termination */
};

typedef struct _dir {
	int		 dd_id;		/* identify each open directory */
	long		 dd_loc;	/* where we are in directory entry */
	_DIRCONTENTS	*dd_contents;	/* pointer to contents of dir */
	_DIRCONTENTS	*dd_cp;		/* pointer to current position */
} DIR;

#define rewinddir(dirp)		seekdir(dirp, 0L)
extern DIR		*opendir();
extern struct direct	*readdir();
extern void		 seekdir();
extern long		 telldir();
extern void		 closedir();
