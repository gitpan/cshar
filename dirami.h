/*
**  Readdir package for the Amiga.
**
**  [  I have not tried this at all except to reformat it.  --r$  ]
**  
**  To: Richard Salz <rsalz@pineapple.bbn.com>
**  Subject: Re: Amiga version of the dir library... 
**  Date: Mon, 13 Jul 87 21:54:25 PDT
**  From: Mike (My watch has windows) Meyer <mwm@violet.Berkeley.EDU>
**  
**  Here's what I did. This is built to reflect the 4BSD manual pages, not
**  the SysV/dpANS manual pages.
**  
**  I now know how to get the telldir/seekdir pair to work correctly with
**  multiple tell()s, but don't have much motivation to do so. If someone
**  out there does it, or is interested in doing it, I'd be interested in
**  the results or willing to help.
**  
**  Final note: as with many micros, there's more than one C compiler.
**  This was written for the Lattice compiler, and uses features known
**  not to exist in other Amiga compilers. Fixing it should be trivial.
**  
**  	<mike
**
**  $Id: dirami.h,v 3.0.3.2 1993/08/25 17:04:30 ram Exp $
*/
/*
 * $Log: dirami.h,v $
 * Revision 3.0.3.2  1993/08/25  17:04:30  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:30:16  ram
 * 3.0 baseline (ram).
 * 
 */
#ifndef	DIR_H
#define DIR_H

#ifndef	EXEC_TYPES_H
#include "exec/types.h"
#endif	/* EXEC_TYPES_H */

#ifndef	LIBRARIES_DOS_H
#include "libraries/dos.h"
#endif /* LIBRARIES_DOS_H */

#ifndef	LIBRARIES_DOSEXTENS_H
#include "libraries/dosextens.h"
#endif /* LIBRARIES_DOSEXTENS_H */


/*
**  MAXNAMELEN is the maximum length a file name can be. The direct structure
**  is lifted from 4BSD, and has not been changed so that code which uses
**  it will be compatable with 4BSD code. d_ino and d_reclen are unused,
**  and will probably be set to some non-zero value.
*/
#define	MAXNAMLEN	31		/* AmigaDOS file max length */

struct direct {
	ULONG	d_ino;			/* unused - there for compatability */
	USHORT	d_reclen;		/* ditto */
	USHORT	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
};


/*
**  The DIRSIZ macro gives the minimum record length which will hold
**  the directory entry.  This requires the amount of space in struct direct
**  without the d_name field, plus enough space for the name with a terminating
**  null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
*/
#undef DIRSIZ
#define DIRSIZ(dp)	\
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen + 1 + 3) & ~3))


/*
**  The DIR structure holds the things that AmigaDOS needs to know about
**  a file to keep track of where it is and what it's doing.
*/
typedef struct {
	struct FileInfoBlock	d_info;	/* Default info block */
	struct FileInfoBlock	d_seek;	/* Info block for seeks */
	struct FileLock		*d_lock; /* Lock on directory */
} DIR;
	
extern DIR		*opendir(char *);
extern struct direct	*readdir(DIR *);
extern long		 telldir(DIR *);
extern void		 seekdir(DIR *, long);
extern void		 rewinddir(DIR *);
extern void		 closedir(DIR *);

#endif	/* DIR_H */
