/*
**  Header file for the shell/shar parser.
**
**  $Id: parser.h,v 3.0.3.2 1993/08/25 17:04:54 ram Exp $
*/
#include "shar.h"

/*
 * $Log: parser.h,v $
 * Revision 3.0.3.2  1993/08/25  17:04:54  ram
 * patch12: cleanup checkin for RCS 5.6
 *
 * Revision 3.0.3.1  91/01/21  11:35:41  ram
 * 3.0 baseline (ram).
 * 
 */

/*
**  Manifest constants, handy shorthands.
*/

/* Character classes used in the syntax table. */
#define C_LETR		1		/* A letter within a word	*/
#define C_WHIT		2		/* Whitespace to separate words	*/
#define C_WORD		3		/* A single-character word	*/
#define C_DUBL		4		/* Something like <<, e.g.	*/
#define C_QUOT		5		/* Quotes to group a word	*/
#define C_META		6		/* Heavy magic character	*/
#define C_TERM		7		/* Line terminator		*/

/* Macros used to query character class. */
#define ISletr(c)	(SynTable[(c)] == C_LETR)
#define ISwhit(c)	(SynTable[(c)] == C_WHIT)
#define ISword(c)	(SynTable[(c)] == C_WORD)
#define ISdubl(c)	(SynTable[(c)] == C_DUBL)
#define ISquot(c)	(SynTable[(c)] == C_QUOT)
#define ISmeta(c)	(SynTable[(c)] == C_META)
#define ISterm(c)	(SynTable[(c)] == C_TERM)


/* Safety-checking. */
#ifdef	MAX_FOPENS
#ifndef	OPEN_OVERHEAD
#define OPEN_OVERHEAD
#endif	/* OPEN_OVERHEAD */
#endif	/* MAX_FOPENS */

#ifdef	PATH_CHECK
#ifndef	OPEN_OVERHEAD
#define OPEN_OVERHEAD
#endif	/* OPEN_OVERHEAD */
#endif	/* PATH_CHECK */


#ifdef	NAME_CHECK
#ifndef	OPEN_OVERHEAD
#define OPEN_OVERHEAD
#endif	/* OPEN_OVERHEAD */
#endif	/* NAME_CHECK */

/*
**  Data types
*/

/* Command dispatch table. */
typedef struct _comtab {
    char	  Name[10];		/* Text of command name		*/
    int		(*Func)();		/* Function that implements it	*/
} COMTAB;

/* A shell variable.  We only have a few of these. */
typedef struct _var {
    char	 *Name;
    char	 *Value;
} VAR;


/*
**  Parser variables and declarations.
*/
#ifdef	PARSER_DATA
#define PEXTERN		/* NULL */
#else
#define PEXTERN		extern
#endif	/* PARSER_DATA */

#ifdef	USE_JMPBUF
PEXTERN jmp_buf	 jEnv;			/* Pop out of main loop		*/
#endif	/* USE_JMPBUF */
PEXTERN FILE	*Input;			/* Current input stream		*/
PEXTERN char	 File[TEMPSIZE];	/* Input filename		*/
PEXTERN int	 Interactive;		/* isatty(fileno(stdin))?	*/

extern void	 SetVar();		/* Shell variable assignment	*/
extern void	 SynErr();		/* Fatal syntax error		*/
