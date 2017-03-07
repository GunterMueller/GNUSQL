# Generated automatically from Makefile.in by configure.
###########################################################
##       GNU SQL Compiler  (optimizer pass)              ##
###########################################################
##
## $Id: Makefile.in,v 1.248 1998/09/29 21:26:22 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/opt
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/opt
include ../Makefile_h
##=======================================================##
##          other setups                                 ##
##=======================================================##

H=$(INC)/kitty.h
TPH=$(INC)/type_lib.h
DEP=$(H) opt.h $(KITTY)
K_SRC=cnf.k groupby.k insert.k optimize.k optutil.k update.k sorter.k get_up.k
###########################################################
#                make program                             #
###########################################################
all:  $(LIBSER) 
libgss_ser : $(K_SRC:.k=.o)
	echo $? >$@

cnf.o     : cnf.c      $(DEP)
cnf.h     : cnf.c
cnf.c     : cnf.k      $(KITTY)
groupby.o : groupby.c  $(DEP) cnf.h 
groupby.c : groupby.k  $(KITTY)
insert.o  : insert.c   $(DEP)
insert.c  : insert.k   $(KITTY)
optimize.o: optimize.c $(DEP) cnf.h
optimize.c: optimize.k $(KITTY)
optutil.o : optutil.c  $(DEP)
optutil.c : optutil.k  $(KITTY)
get_up.o  : get_up.c   $(DEP) cnf.h
get_up.c  : get_up.k   $(KITTY)
sorter.o  : sorter.c   $(DEP)
sorter.c  : sorter.k   $(KITTY)
update.o  : update.c   $(DEP)
update.c  : update.k   $(KITTY)
