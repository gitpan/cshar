/*
**  Header file for VMS readdir() routines.
**  Written by Rich $alz, <rsalz@bbn.com> in August, 1990.
**  This code has no copyright.
**
**  You must #include <descrip.h> before this file.
*/

/*
 * $Log: dirvms.h,v $
 * Revision 3.0.3.3  1993/08/25  17:04:33  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.2  91/04/07  18:50:16  ram
 * patch1: merged official cshar 3.0 into beta version
 * 
 * Revision 3.0.3.1  91/01/21  11:31:08  ram
 * 3.0 baseline (ram).
 * 
 */
    /* Data structure returned by readdir(). */
struct dirent {
    char	d_name[100];		/* File name		*/
    int		vms_verscount;		/* Number of versions	*/
    int		vms_versions[20];	/* Version numbers	*/
};

    /* Handle returned by opendir(), used by the other routines.  You
     * are not supposed to care what's inside this structure. */
typedef struct _dirdesc {
    long			context;
    long			count;
    int				vms_wantversions;
    char			*pattern;
    struct dirent		entry;
    struct dsc$descriptor_s	pat;
} DIR;


#define rewinddir(dirp)		seekdir((dirp), 0L)


extern DIR		*opendir();
extern struct dirent	*readdir();
extern long		telldir();
extern void		seekdir();
extern void		closedir();
extern void		vmsreaddirversions();
