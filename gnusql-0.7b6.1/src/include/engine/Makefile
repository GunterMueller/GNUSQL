# Generated automatically from Makefile.in by configure.
##########################################################################
##                 GNU SQL Engine  (include dependencies)               ##
##########################################################################
##
## $Id: Makefile.in,v 1.249 1998/09/29 21:26:02 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/include/engine
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/include/engine
include ../../Makefile_h
##########################################################################
#                make program                                            #
##########################################################################
all:   headers
headers: adfstr.h agrflg.h cmpdecl.h deftr.h destrn.h dispatch_h expop.h \
	f1f2decl.h fdcltrn.h fieldtp.h inpop.h pupsi.h sctp.h \
	strml.h totdecl.h tptrn.h rnmtp.h
dispatch_h: $(srcdir)/../setup_os.h
	( cd ../../engine; $(MAKERT) headers ; )
cmpdecl.h: totdecl.h fieldtp.h rnmtp.h
	- touch $@
f1f2decl.h: totdecl.h cmpdecl.h destrn.h
	- touch $@
destrn.h : $(srcdir)/../sql.h rnmtp.h pupsi.h tptrn.h fieldtp.h deftr.h
	- touch $@
fdcltrn.h: inpop.h f1f2decl.h strml.h $(srcdir)/../typeif.h $(srcdir)/../type_lib.h
	- touch $@
inpop.h: pupsi.h
	- touch $@
rnmtp.h: $(srcdir)/../setup_os.h
	- touch $@
strml.h: $(srcdir)/../setup_os.h
	- touch $@
totdecl.h: $(srcdir)/../setup_os.h $(srcdir)/../xmem.h
	- touch $@
tptrn.h: rnmtp.h
	- touch $@

clean::
	- $(RM) dispatch.h
