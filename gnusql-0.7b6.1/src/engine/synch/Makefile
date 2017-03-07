# Generated automatically from Makefile.in by configure.
##########################################################################
##                 GNU SQL Server  ( Synchronizer )                     ##
##########################################################################
##
## $Id: Makefile.in,v 1.248 1998/09/29 21:25:12 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/engine/synch
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/engine/synch
include ../../Makefile_h
##########################################################################
#                make program                                            #
##########################################################################
all:	install
install: bin
	- [ -d $(SERVBIN) ] || mkdir -p $(SERVBIN)
	$(CP)  synpr $(SERVBIN)/syn

SRC=    dlock.c incrs.c lock.c shartest.c unlock.c \
        shtest1.c synpr.c snipc.c

SRCOBS= ${SRC:.c=.o}
SYN_H=  dessnch.h $(IINC)/destrn.h $(IINC)/rnmtp.h $(INC)/setup_os.h \
        $(IINC)/pupsi.h $(IINC)/tptrn.h $(INC)/sql.h \
        $(IINC)/fieldtp.h $(IINC)/deftr.h $(IINC)/sctp.h fdclsyn.h $(IINC)/totdecl.h

bin:    synpr
synpr :  $(SRCOBS) ../trans/cmpftn.o
	echo --  generating $@
	- $(RM) $@ 
	$(LD) $(SRCOBS) ../trans/cmpftn.o $(LIB)/xmem.o $(LIB)/sql_decl.o -o $@ 
# the following line should be used instead but it's impossible because of name conflicts
#	$(LD) $(SRCOBS) $(SRLIBS) -o $@ 

clean::
	- $(RM) synpr
headers    : sctpsyn.h
dlock.o    : dlock.c    $(SYN_H) $(IINC)/sctp.h sctpsyn.h
incrs.o    : incrs.c    $(SYN_H)
lock.o     : lock.c     $(SYN_H)
shartest.o : shartest.c $(SYN_H)
unlock.o   : unlock.c   $(SYN_H)
shtest1.o  : shtest1.c  $(SYN_H) $(IINC)/sctp.h sctpsyn.h
synpr.o    : synpr.c    $(SYN_H)
snipc.o    : snipc.c    $(SYN_H) $(IINC)/inpop.h $(IINC)/f1f2decl.h
