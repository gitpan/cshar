case $CONFIG in
'')
	if test -f config.sh; then TOP=.;
	elif test -f ../config.sh; then TOP=..;
	elif test -f ../../config.sh; then TOP=../..;
	elif test -f ../../../config.sh; then TOP=../../..;
	elif test -f ../../../../config.sh; then TOP=../../../..;
	else
		echo "Can't find config.sh."; exit 1
	fi
	. $TOP/config.sh
    ;;
esac
: This forces SH files to create target in same directory as SH file.
: This is so that make depend always knows where to find SH derivatives.
case "$0" in
*/*) cd `expr X$0 : 'X\(.*\)/'` ;;
esac
echo "Extracting Makefile (with variable substitutions)"
: This section of the file will have variable substitutions done on it.
: Move anything that needs config subs from !NO!SUBS! section to !GROK!THIS!.
: Protect any dollar signs and backticks that you do not want interpreted
: by putting a backslash in front.  You may delete these comments.
$spitshell >Makefile <<!GROK!THIS!
##
##  SOURCE-SHIPPING TOOLS MAKEFILE
##  $Id: Makefile.SH,v 3.0.3.2 1993/08/19 15:03:46 ram Exp $
##

# $Log: Makefile.SH,v $
# Revision 3.0.3.2  1993/08/19  15:03:46  ram
# patch7: random cleanup
#
# Revision 3.0.3.1  91/04/19  10:04:13  ram
# patch5: 'shell' is no longer built on UNIX systems
# patch5: installation of manual pages is now optional
# 
# Revision 3.0  91/01/21  11:26:58  ram
# 3.0 baseline (ram).
# 

##  Grrr...
SHELL	= /bin/sh

##  Put -D options here, if they're not in your config.h
DEFS	=

##  Flags for the Compiler, the linker, and lint
CC = $cc
CFLAGS = \$(DEFS) $optimize $ccflags
LINKF =
LINTF = \$(DEFS) -a -b -h

##  How to remove a file, create one, or make one read-only
REMOVE	= $rm -f
TOUCH	= -touch
READ_ONLY	= chmod 444

##  Prolog -- first file that should be linked in.
PROLOG	=

DIRLIB	= $libs

##  The shar package has a local library that it builds.  Define its name.
MYLIB	= lib.a

##  Specify how to make the library
!GROK!THIS!

if $orderlib; then
	$spitshell >>Makefile <<!GROK!THIS!
MKLIB	= ar cr \$(MYLIB) \`lorder \$(LIB_OBJ) | tsort\`
!GROK!THIS!
else
	$spitshell >>Makefile <<!GROK!THIS!
MKLIB	= ar cr \$(MYLIB) \$(LIB_OBJ)
!GROK!THIS!
fi

$spitshell >>Makefile <<!GROK!THIS!
##  Specify how to finish off the library.
RANLIB	= $ranlib \$(MYLIB)

##  Where executables are installed.
DESTDIR	= $bin

##  The programs.  You might not want to install everything.
##	ram:
##	On UNIX, do not make 'shell' -- we probably won't need it.
UUSTUFF	= uuencode uudecode
PROGRAMS= findsrc makekit maniscan manipull shar unshar \$(UUSTUFF)

##  The "foo" manpage will get put in \$(MANDIR)/foo.\$(MANEXT)
MANDIR	= $mansrc
MANEXT	= $manext

##  Documentation.  You might not want to install everything.
MANPAGES= findsrc.man makekit.man maniscan.man manipull.man \
	shar.man unshar.man shell.man uuencode.man uudecode.man
##  For systems without nroff manpages are nearly useless.  But see the
##  "doc" target before giving this package to one of those systems.
#MANPAGES=

##
##  END OF MAIN CONFIGURATION SECTION
##

!GROK!THIS!

: In the following dollars and backticks do not need the extra backslash.
$spitshell >>Makefile <<'!NO!SUBS!'
##  UNOS needs this default rule, can you believe it?
#.SUFFIXES	:	.o
#.c.o		:
#	$(CC) $(CFLAGS) -c $<

##  Header files.
HDRS	= shar.h config.h

##  If you want to keep lpt-formatted manpages around, use this.
DOCPAGES= findsrc.doc makekit.doc maniscan.doc manipull.doc \
	shar.doc unshar.doc shell.doc uuencode.doc uudecode.doc
.SUFFIXES	:	.doc .man
.man.doc	:
	@$(REMOVE) $@
	sh dodoc.sh $*
	@$(READ_ONLY) $@

##  Our library of utility functions.
LIB_SRC	= glue.c parser.c lcwd.c lexec.c lfiles.c lhost.c llib.c \
	lmem.c luser.c uuencode.c wildmat.c $(DIRSRC)
LIB_OBJ	= glue.o parser.o lcwd.o lexec.o lfiles.o lhost.o llib.o \
	lmem.o luser.o uuencode.o wildmat.o $(DIROBJ)


all		:	$(PROGRAMS) $(MANPAGES)
	$(TOUCH) all

doc		:	dodoc.sh $(DOCPAGES)
	$(TOUCH) doc

##  You might want to change these actions.  It would be nice if everyone
##  had the same install program.
install		:	all install_bin install_man
	$(TOUCH) install

install_bin	: $(PROGRAMS)
	cd $(DESTDIR) ; $(REMOVE) $(PROGRAMS)
	cp $(PROGRAMS) $(DESTDIR)
	cd $(DESTDIR) ; strip $(PROGRAMS) ; chmod 755 $(PROGRAMS)
#	cd $(DESTDIR) ; /etc/chown root $(PROGRAMS)
	$(TOUCH) install_bin

install_man	: $(MANPAGES)
	if test "$(MANDIR)"; then \
		if test `pwd` != "$(MANDIR)" ; then \
			for I in $(MANPAGES) ; do \
				cp $$I $(MANDIR)/`basename $$I .man`.$(MANEXT); \
			done ; \
		fi; \
	fi
	$(TOUCH) install_man

clean		:
	$(REMOVE) *.[oa] *.doc *.BAK $(PROGRAMS) unshar2
	$(REMOVE) lint lib.ln tags core foo a.out
	$(REMOVE) Part?? all install install_bin install_man doc clean

realclean: clean
	$(REMOVE) Makefile

clobber: realclean
	$(REMOVE) config.h config.sh

##  Header hacking.
$(LIB_OBJ)	:	$(HDRS)

##  Programs.
findsrc		:	findsrc.o $(MYLIB)
	@$(REMOVE) findsrc
	$(CC) $(CFLAGS) -o findsrc \
		$(PROLOG) findsrc.o $(MYLIB) $(DIRLIB) $(LINKF)

makekit		:	makekit.o $(MYLIB)
	@$(REMOVE) makekit
	$(CC) $(CFLAGS) -o makekit \
		$(PROLOG) makekit.o $(MYLIB) $(DIRLIB) $(LINKF)

maniscan	:	maniscan.o $(MYLIB)
	@$(REMOVE) maniscan
	$(CC) $(CFLAGS) -o maniscan \
		$(PROLOG) maniscan.o $(MYLIB) $(DIRLIB) $(LINKF)

manipull	:	manipull.o $(MYLIB)
	@$(REMOVE) manipull
	$(CC) $(CFLAGS) -o manipull \
		$(PROLOG) manipull.o $(MYLIB) $(DIRLIB) $(LINKF)

shar		:	shar.o $(MYLIB)
	@$(REMOVE) shar
	$(CC) $(CFLAGS) -o shar \
		$(PROLOG) shar.o $(MYLIB) $(DIRLIB) $(LINKF)

shell		:	shell.o $(MYLIB)
	@$(REMOVE) shell
	$(CC) $(CFLAGS) -o shell \
		$(PROLOG) shell.o $(MYLIB) $(DIRLIB) $(LINKF)

uudecode	:	uudecode.o $(MYLIB)
	@$(REMOVE) uudecode
	$(CC) $(CFLAGS) -o uudecode \
		$(PROLOG) uudecode.o $(MYLIB) $(DIRLIB) $(LINKF)

uuencode	:	uuenmain.o $(MYLIB)
	@$(REMOVE) uuencode
	$(CC) $(CFLAGS) -o uuencode \
		$(PROLOG) uuenmain.o $(MYLIB) $(DIRLIB) $(LINKF)

unshar		:	unshar.o $(MYLIB)
	@$(REMOVE) unshar
	$(CC) $(CFLAGS) -o unshar \
		$(PROLOG) unshar.o $(MYLIB) $(DIRLIB) $(LINKF)

##  Special case, a version of unshar that uses the /bin/sh interpreter.
unshar2		:	unshar.c $(HDRS) $(MYLIB)
	@$(REMOVE) unshar2 unshar.o
	$(CC) $(CFLAGS) -DUSE_MY_SHELL -o unshar \
		$(PROLOG) unshar.c $(MYLIB) $(DIRLIB) $(LINKF)
	@$(REMOVE) unshar.o


##  Support your local library.
$(MYLIB)	:	$(LIB_OBJ)
	@$(REMOVE) $(MYLIB)
	$(MKLIB)
	$(RANLIB)

##  Lint; this is probably only good for BSD-derived lints.
##  A slight speed hack...
X	= exec
lint		:	$(PROGRAMS) lib.ln
	@$(REMOVE) lint
	$(X) lint $(LINTF) -u $(LIB_SRC)          >lint
	$(X) lint $(LINTF)    findsrc.c   lib.ln >>lint
	$(X) lint $(LINTF)    makekit.c   lib.ln >>lint
	$(X) lint $(LINTF)    maniscan.c  lib.ln >>lint
	$(X) lint $(LINTF)    manipull.c  lib.ln >>lint
	$(X) lint $(LINTF)    shar.c      lib.ln >>lint
	$(X) lint $(LINTF)    shell.c     lib.ln >>lint
	$(X) lint $(LINTF)    unshar.c    lib.ln >>lint
	$(X) lint $(LINTF) -DUSE_MY_SHELL unshar.c lib.ln >>lint
	-checknr -c.BI.IR.SS  *.man              >>lint

lib.ln		:	$(LIB_SRC)
	@$(REMOVE) lib.ln
	lint -CX $(LIB_SRC)
	mv llib-lX.ln lib.ln
!NO!SUBS!
chmod 755 Makefile
$eunicefix Makefile
