/*
**  Readdir package for the Amiga.
**
**  [  I have not tried this at all, except to reformat it.  --r$  ]
**  
**  To: Richard Salz <rsalz@pineapple.bbn.com>
**  Subject: Re: Amiga version of the dir library...
**  Date: Mon, 13 Jul 87 21:54:25 PDT
**  From: Mike (My watch has windows) Meyer <mwm@violet.Berkeley.EDU>
**  
**  Here is what I did. This is built to reflect the 4BSD manual pages, not
**  the SysV/dpANS manual pages.
**  
**  I now know how to get the telldir/seekdir pair to work correctly with
**  multiple tell()s, but do not have much motivation to do so. If someone
**  out there does it, or is interested in doing it, I wouldd be interested in
**  the results or willing to help.
**  
**  Final note: as with many micros, there is more than one C compiler.
**  This was written for the Lattice compiler, and uses features known
**  not to exist in other Amiga compilers. Fixing it should be trivial.
**  
**  	<mike
**
**  $Id: dirami.c,v 3.0.3.3 1993/08/25 17:04:28 ram Exp $
*/
/*
 * $Log: dirami.c,v $
 * Revision 3.0.3.3  1993/08/25  17:04:28  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:49:53  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:30:02  ram
 * 3.0 baseline (ram).
 * 
 */
#include <dir.h>


DIR		*AllocMem(int, int);
struct FileLock	*Lock(char *, int);
struct FileLock	*CurrentDir(struct FileLock *);


DIR *
opendir(dirname)
    char		*dirname;
{
    REGISTER DIR	*my_dir;

    if ((my_dir = AllocMem(sizeof *my_dir, 0)) == NULL)
	return NULL;

    /* If we can't examine it, or it's not a directory, that's not good. */
    if ((my_dir->d_lock = Lock(dirname, ACCESS_READ)) == NULL
     || !Examine(my_dir->d_lock, &(my_dir->d_info))
     || (my_dir->d_info.fib_DirEntryType < 0)) {
	FreeMem(my_dir, sizeof *my_dir);
	return NULL;
    }
    return my_dir;
}


struct direct *
readdir(my_dir)
    DIR				*my_dir;
{
    static struct direct	result;

    if (!ExNext(my_dir->d_lock, &(my_dir->d_info)))
	return NULL;

    result.d_reclen = result.d_ino = 1;	/* Not NULL! */
    (void)strcpy(result.d_name, my_dir->d_info.fib_FileName);
    result.d_namlen = strlen(result.d_name);
    return &result;
}


void
closedir(my_dir)
    DIR		*my_dir;
{
    UnLock(my_dir->d_lock);
    FreeMem(my_dir, sizeof *my_dir);
}


/*
**  telldir and seekdir don't work quite right.  The problem is that you have
**  to save more than a long's worth of stuff to indicate position, and it's
**  socially unacceptable to alloc stuff that you don't free later under
**  AmigaDOS.  So we fake it -- you get one level of seek, and that's all.
**  As of now, these things are untested.
*/
#define DIR_SEEK_RETURN		((long)1)	/* Not 0! */


long
telldir(my_dir)
    DIR		*my_dir;
{
    my_dir->d_seek = my_dir->d_info;
    return DIR_SEEK_RETURN;
}


void
seekdir(my_dir, where)
    DIR		*my_dir;
    long	 where;
{
    if (where == DIR_SEEK_RETURN)
	my_dir->d_info = my_dir->d_seek;
    else
	/* Makes the next readdir fail */
	setmem((char *)my_dir, sizeof *my_dir, 0);
}


void
rewinddir(my_dir)
    DIR		*my_dir;
{
    if (!Examine(my_dir->d_lock, &(my_dir->d_info)))
	setmem((char *)my_dir, sizeof *my_dir, 0);
}


#ifdef	TEST
/*
 * Simple code to list the files in the argument directory,
 *	lifted straight from the man page.
 */
#include <stdio.h>
int
main(ac, av)
    int				ac;
    char			*av[];
{
    register DIR		*dirp;
    register struct direct	*dp;
    register char		*name;

    name = ac < 2 ? "" : av[1];
    if ((dirp = opendir(name)) == NULL) {
	(void)fprintf(stderr, "Bogus! Can't opendir %s\n", name);
	exit(1);
	/* NOTREACHED */
    }

    while (dp = readdir(dirp))
	(void)printf("%s ", dp->d_name);
    closedir(dirp);
    putchar('\n');
    exit(0);
    /* NOTREACHED */
}
#endif	/* TEST */
