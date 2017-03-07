# Generated automatically from Makefile.in by configure.
#############################################################################
##                    GNU SQL Server (etc directory)                       ##
#############################################################################
## $Id: Makefile.in,v 1.6 1998/09/29 21:24:41 kimelman Exp $
##
## This file is a part of GNU SQL Server
##
## Copyright (c) 1996-1998, Free Software Foundation, Inc
## Developed at Institute of System Programming of Russian Academy of Science
## This file is written by Michael Kimelman
##
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free
## Software Foundation; either version 2 of the License, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
## more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc.,
## 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##
## Contacts: gss@ispras.ru
##

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/etc
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/etc
builddir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1

SERVDIR=$(builddir)/sqlser
SERVBIN=$(SERVDIR)/bin

RM=rm -f
CP=cp -p
DOC=$(SERVDIR)/doc
#############################################################################
all            :  gss
	$(CP) dq boot $(SERVBIN)
install        : infos mans
infos          : $(DOC)/gss
mans           : $(DOC)/gsqlc.1
$(DOC)/gss: $(DOC) gss
	cp $(srcdir)/gss $(srcdir)/gss-* $(DOC)
$(DOC)/gsqlc.1 : $(DOC) gsqlc.1 gsqls.1
	cp $(srcdir)/*.1 $(DOC)
gss : gss.texi gnu.texi ask.texi
	makeinfo -I$(srcdir) $(srcdir)/gss.texi
$(DOC)         :
	- [ -d $(DOC) ] || mkdir -p $(DOC)
clean          :
	./make-util clean "make clean"
distclean      :
	./make-util clean "make distclean"
.SILENT:
