#! /bin/sh
##  Script to generate simple formatted documentation for distribution
##  to BBS's and other non-Unix systems.  It's basically a de-paginator
##  for not-very-long manual pages (five pages or so).
##
##  This runs on SunOS 3.4, and might need tweaking if you go to a system
##  where the -man macros have been munged differently.
##
##  $Id: dodoc.sh,v 3.0.3.2 1993/08/25 17:04:34 ram Exp $
##

# $Log: dodoc.sh,v $
# Revision 3.0.3.2  1993/08/25  17:04:34  ram
# patch12: cleanup checkin for RCS 5.6
#
# Revision 3.0.3.1  91/01/21  11:31:25  ram
# 3.0 baseline (ram).
# 

case $# in
0)
    echo "Usage: `basename $0` files..." >&2
    exit 1
    ;;
esac

for I in "$@" ; do
    echo ".pl 40i" \
	| nroff -man - ${I}.man \
	| col -b \
	| awk '
	  /SEE ALSO/ { F = 1 }
	  {
	    if (F == 1 && length == 0) { F = 2 }
	    if (F != 2) { print }
	  }' \
	| sed \
	  -e 's/^\([A-Z]*\)(1L)/\1    /' \
	  -e 's/\([A-Z]*\)(1L)$/    \1/' \
	  -e 's/^/     /' \
	  -e 's/UNKNOWN SECTION OF THE MANUAL/                             /' \
	>${I}.doc
done
