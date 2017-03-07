# Generated automatically from Makefile.in by configure.
##########################################################################
##                 GNU SQL Compiler  (main routines)                    ##
##########################################################################
##
## $Id: Makefile.in,v 1.250 1998/09/29 21:26:17 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/main
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/src/main
include ../Makefile_h
##########################################################################
#                make program                                            #
##########################################################################
LD.main=$(LD) $(CFLAGS) $(CPPFLAGS) 
LD.tail=$(srcdir)/gsqltrn_lib.c $(SRLIBS) -o
H = cs_link.c options.c $(LINC)/gsqltrn.h $(INC)/sql_decl.h

COMP_CLI = $(CLNT)/gsqlc
TRNS=boot dyn# types of transactions

all: client server
install: client server

client: $(LIBCL_DEP) gsqlc
	- $(RM) $(COMP_CLI) $@
	$(CP)  gsqlc $(COMP_CLI) 	# compiler client 
	echo .>$@

server: $(LIBSR_DEP) gss-adm $(TRNS)
# copy transactons to server lib catalog
	- $(RM)  $(SERVBIN)/gsqlt-* $@
	echo "refreshing transactions ..." ;
	for trn in $(TRNS) ; do \
	  echo " : $$trn" ;  \
	  $(CP)  $$trn $(SERVBIN)/gsqlt-$$trn ; done
	echo .>$@

# library dependencies
libgss_ser : servlib.o gsqltrn_svc.o gsqltrn_xdr.o 
	echo $? >$@
libgss_cli : dyn_client.o dyn_funcs.o sc.o gsqltrn_clnt.o gsqltrn_xdr.o 
	echo $? >$@

gss-adm: # it must be already done -- see engine/adm
	echo .>$@

gsqlc    : gsql_cli.o $(LIBCL_DEP)
	echo --  generating $@
	- $(RM) $@ 
	$(LD) gsql_cli.o $(CLLIBS) -o $@ 

$(TRNS) : gsqltrn_lib.c $(LIBSR_DEP)
	echo -- generating transaction servers
	- $(RM) $(TRNS)
	echo '--> boot'
	$(LD.main) -DSERVE_BOOT                    $(LD.tail) boot
	echo '--> dyn '
	$(LD.main) -DSERVE_COMPILE -DSERVE_EXECUTE $(LD.tail) dyn
gsql_cli.o  : gsql_cli.c $(H) $(INC)/cl_lib.h
dyn_client.o: dyn_client.c $(H) $(INC)/cl_lib.h 
servlib.o   : servlib.c $(H) $(IINC)/pupsi.h $(INC)/global.h 
dyn_funcs.o : dyn_funcs.c $(H) $(INC)/dyn_funcs.h $(LINC)/gsqltrn.h $(INC)/sql.h
sc.o        : sc.c $(H) $(INC)/type_lib.h

gsqltrn_clnt.o: gsqltrn_clnt.c $(LINC)/gsqltrn.h
gsqltrn_svc.o: gsqltrn_svc.c $(LINC)/gsqltrn.h
gsqltrn_xdr.o: gsqltrn_xdr.c $(LINC)/gsqltrn.h
gsqltrn_clnt.c $(LINC)/gsqltrn.h gsqltrn_svc.c gsqltrn_xdr.c: gsqltrn.x $(srcdir)/../../etc/rpcgen-sh.in
	$(RPCGEN) $(LINC) $(srcdir)/gsqltrn.x
headers : $(LINC)/gsqltrn.h 

clean::
	- $(RM) gsqlc $(TRNS) gss-adm 
	- $(RM) gsqltrn_clnt.c gsqltrn_svc.c gsqltrn_xdr.c
	- $(RM) client server
force:
	:
#
#
