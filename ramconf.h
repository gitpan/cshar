/*

 #####     ##    #    #   ####    ####   #    #  ######          #    #
 #    #   #  #   ##  ##  #    #  #    #  ##   #  #               #    #
 #    #  #    #  # ## #  #       #    #  # #  #  #####           ######
 #####   ######  #    #  #       #    #  #  # #  #        ###    #    #
 #   #   #    #  #    #  #    #  #    #  #   ##  #        ###    #    #
 #    #  #    #  #    #   ####    ####   #    #  #        ###    #    #

	This is to translate Configure defines into cshar ones.
	Rich Salz, the author of cshar, does not want to use
	Configure scripts, because they run only under UNIX. But here,
	the Configure is only an help, not a require.

	So, if you are not running UNIX, you can still compile this
	package by using the prototypical Makefile named Makefile.salz
*/

/*
 * $Id: ramconf.h,v 3.0.3.2 1993/11/12 19:11:04 ram Exp $
 *
 * $Log: ramconf.h,v $
 * Revision 3.0.3.2  1993/11/12  19:11:04  ram
 * patch14: moved index definition after inclusion of confmagic.h
 * patch14: commented the CONFIGURE token in #endif line
 *
 * Revision 3.0.3.1  1993/08/25  16:41:41  ram
 * patch10: updated for new metaconfig
 *
 * Revision 3.0  91/01/21  11:36:54  ram
 * 3.0 baseline (ram).
 * 
 */

#ifdef CONFIGURE	/* Ignore this file if not using Configure */

#define UNIX

#ifndef HAS_CTERMID
# define NEED_CTERMID
#endif

#ifndef HAS_MKDIR
# define NEED_MKDIR
#endif

#ifndef HAS_GETOPT
# define NEED_GETOPT
#endif

#ifdef strerror
# undef strerror
# define NEED_STRERROR
#endif

#ifdef HAS_SYSTEM
# define HAVE_SYSTEM
#endif

#ifdef HAS_POPEN
# define CAN_POPEN
#endif

#ifdef VOIDSIG
typedef void sigret_t;
#else
typedef int sigret_t;
#endif

typedef int *align_t;

#if Off_t != off_t
typedef Off_t off_t;
#endif

#if Time_t != time_t
typedef Time_t time_t;
#endif

#ifdef I_SYS_WAIT
# ifdef HAS_VFORK
#  define SYS_WAIT
# endif
#endif

#ifdef HAS_SYS_ERRLIST
# define USE_SYSERRLIST
#endif

#ifdef HAS_GETPWENT
# define USE_GETPWUID
#endif

#define UNIX_FILES
#define NAME_FORMAT	"%s%2.2d"
#define NUMB_FORMAT "%02d"
#define MANI_FORMAT "%-25s %2d\t%s\n"
#define THE_TTY		"/dev/tty"
#define RCSID
#define USERNAME	"Somebody"

#ifdef CHARSPRINTF
# define PTR_SPRINTF
#endif

#define REGISTER	register	/* may lead to problems */
#define PEDAGOGY

#define CTYPE(c)	(isascii((c)))

#ifdef HAS_RENAME
# define BU_VIA_RENAME
#else
# define BU_VIA_LINK
#endif

#ifndef FLEXFILENAMES
# define BU_NAME_LEN	14
#endif

#define TEMP_NAME1	"/tmp/csharXXXXXX"
#define TEMP_NAME2	"/tmp/csharXXXXXX"

#define TEMPSIZE	40

#ifndef HAS_GETHOSTNAME
# ifdef DOUNAME	/* if no gethostname but uname */
#  define HAS_UNAME
#  undef UUNAME
#  undef I_WHOAMI
#  undef HOST_STRING
# else	/* if no gethostname and no uname */
#  undef UUNAME
#  undef I_WHOAMI
#  define HOST_STRING
# endif
#else	/* if gethostname */
# undef HAS_UNAME
# undef UUNAME
# undef I_WHOAMI
# undef HOST_STRING
#endif

#define DEF_HOST	MYHOSTNAME

#ifndef HAS_GETWD
# define GETCWD
#endif

#define NOTES1	"$$"
#define NOTES2	"$$"

#ifdef I_DIRENT
# define IN_DIRENT
#else
# ifdef I_SYS_DIR
#  define IN_SYS_DIR
# else
#  ifdef I_SYS_NDIR
#   define IN_SYS_NDIR
#  else
#   define IN_DIRECT
#  endif
# endif
#endif

#ifdef IN_DIRENT
# define DIRENTRY	struct dirent
#else
# define DIRENTRY	struct direct
#endif

#include "confmagic.h"

#ifdef index
# define IDX strchr
#else
# define IDX index
#endif

#ifdef rindex
# define RDX strrchr
#else
# define RDX rindex
#endif

#endif /* CONFIGURE */
