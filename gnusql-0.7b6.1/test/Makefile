# Generated automatically from Makefile.in by configure.
##
## $Id: Makefile.in,v 1.255 1998/09/29 21:26:55 kimelman Exp $
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

srcdir=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/test
VPATH=/home/gunter/GM_DATABASES/DBMS/GnuSQL/gnusql-0.7b6.1/test
SRC=$(srcdir)/../src
ETC=../etc

RM=rm -f 
LIB=../sqlser/client
LIBFILE=-lgss  
CFLAGS=  -Wall -g3 -O4 -I../src/include -I$(SRC)/include -I$(SRC)/trl $(CFLAGS1) -DHAVE_CONFIG_H
LDFLAGS= -Wall -g3 -O4   -L$(LIB)
CC=gcc
GSQLF=# -Dl  
# disable BASE debug printing (redirecting to /dev/null)
SHELL=/bin/sh
ESQL=$(LIB)/gsqlc
SQLCPP= $(ESQL) $(LIB)/../bin/gsqlt-dyn $(LIB)/../gsqls $(LIB)/../db/base.dat
##############################################
.c.exe:
	echo "-- compiling $< ==>> $*.exe " 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $*.exe $< $(LIBFILE)
.ec.c:
	echo "-- compiling $< ==>> $*.c"
	$(ESQL) $(GSQLF) -rC  $<
.sql.c:
	echo "-- compiling $<"
	$(ESQL) $(GSQLF)  $<
.EC.c:
	echo "-- compiling $<"
	cp $< $*.c
.ec.Sc:
	echo "-- compiling $< ==>> $*.Sc"
	$(ESQL) $(GSQLF) -rC  $<
.exe.stamp:
	echo "-- run $<"
	./$<
.SQL.stamp:
	echo "-- processing $<"
	$(LIB)/gsqlmon $<
###########################################
all:  do_all

cleanall: clean
	$(ETC)/Tdeps

clean :
	$(ETC)/make-util clean "$(MAKE) clean"
	- $(RM) dynlog.* 
distclean : clean
	$(ETC)/make-util clean "$(MAKE) distclean"
	- mkdir -p tmptest
	- cp -p *.ec *.sql *.txt *.h *.EC *.SQL testdeps.mk tmptest 2>/dev/null
	- $(RM) test* 
	mv tmptest/* .
	- $(RM) -r tmptest

include testdeps.mk

.SUFFIXES: .stamp .exe .c .Sc .SQL .EC .ec .sql
.SILENT:
