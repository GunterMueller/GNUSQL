/*
 * $Id: testM1a.ec,v 1.1 1998/01/19 06:14:46 kml Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, Free Software Foundation, Inc
 * Developed at Institute of System Programming of Russian Academy of Science
 * This file is written by Michael Kimelman
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 */

#include <stdio.h>
#include <stdlib.h>

char *sh;
int  t2m;

int
simple_selection(void)
{
  int k;
  $ select count(*)
    into :k
    from tbl0
    where 
          k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and not ( 0 =   
          ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
      and taila>0
    ;
  return 0;
 fatal_exit:
  return SQLCODE;
}

int
commit_work(void)
{
  $ commit work;
  return 0;
fatal_exit:
  return SQLCODE;
}

int
rollback_work(void)
{
  $ rollback work;
  return 0;
fatal_exit:
  return SQLCODE;
}

void
run(char *tn,int (*f)(void))
{
  fprintf(stderr,"Run %s ...",tn);
  if(f())
    {
      fprintf(stderr,": failed(%d): '%s'\n",SQLCODE,gsqlca.errmsg);
      exit(1);
    }
  else
    fprintf(stderr,": OK\n");
}

#define RUN(tn) run(#tn,tn)

static void
refresh_settings(void)
{
  t2m= 100;
  sh = "%34%";               /* filter for "   %010d   " strings         */
}

int main(int argc,char **argv)
{
  refresh_settings();
  RUN(simple_selection);
  RUN(commit_work);
  return 0;
}
