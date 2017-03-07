# Generated automatically from Makefile.in by configure.
##########################################################################
##                    GNU SQL Server (PUPSI Page Buffer)                ##
##########################################################################
##
## $Id: Makefile.in,v 1.249 1998/09/29 21:24:59 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/engine/buf
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/engine/buf
include ../../Makefile_h
##########################################################################
#                make program                                            #
##########################################################################
all :	install
install : $(SERVBIN)/buf
$(SERVBIN)/buf : buf.stamp
buf.stamp : buf
	- [ -d $(SERVBIN) ] || mkdir -p $(SERVBIN)
	$(CP)  buf $(SERVBIN)
	- $(RM) $@
	echo .>$@

bin :   buf

SRC=    buf.c bufipc.c buflock.c get_put.c page.c queue.c 

SRCOBS=         ${SRC:.c=.o}

buf :  $(SRCOBS) $(LIBSR_DEP)
	echo --  generating $@
	- $(RM) $@ 
	$(LD) $(SRCOBS) $(SRLIBS) -o $@ 

clean::
	- $(RM) buf


HDRS=$(IINC)/rnmtp.h bufdefs.h $(IINC)/rnmtp.h $(IINC)/inpop.h \
     $(IINC)/pupsi.h $(INC)/sql.h fdeclbuf.h
fdeclbuf.h : $(IINC)/totdecl.h 
	touch $@
bufipc.o   : bufipc.c  $(HDRS) $(IINC)/strml.h 
buf.o      : buf.c     $(HDRS)
buflock.o  : buflock.c $(HDRS)  
get_put.o  : get_put.c $(HDRS)  
page.o     : page.c    $(HDRS)
queue.o    : queue.c   $(HDRS)
