/*
 * $Id: testM4.ec,v 1.1 1998/01/19 06:14:46 kml Exp $
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

int    N_OF_K1 = 2000;
int    N_OF_K2 = 2000;
int    N_OF_T0 = 2000;

char   tailb[20];
char  *sh;
int    t2m,k1;
short  k2;
int    expectations = -1;
int    switcher     =  0;

static void
shift(void)
{
  k2 -- ;
  k1 ++ ;
}

$ whenever sqlerror goto fatal_exit;

int
simple_selection(void)
{
  int i,j,k;
  $ declare curs cursor for
    ( select tailb,k1,k2
      from tbl0
      where
--            k1 = SOME ( select k1 from tbl1 where t1 like :sh escape '#'   ) 
--        and
--            exists    ( select *  from tbl2 where k2 = tbl0.k2 and t2<:t2m )
--        and
            taila > 0
    )    
  ;
  $ open curs;
  fputs("\n---------------------\n",stdout);
  for(j=i=0;;i++)
    {
      $ fetch curs into :tailb,:k1,:k2;
      if(SQLCODE==100) break;
      fputs(tailb,stdout);
      if(i%switcher==0)
	switch(switcher)
	  {
	  case 1:
	    $ delete from tbl0 where current of curs ;
	    j++;
	    break;
	  case 3:
	    $ update tbl0 set taila=-taila where current of curs ;
	    j++;
	    break;
	  default:
	    break;
	  }
    }
  $ close curs;
  fputs("\n---------------------\n",stdout);
  $ select count(*)
    into :k
    from tbl0
    where 
          k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and not ( 0 =   
          ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
      and taila>0
  ;

  fprintf(stderr,"\n----------%3d--------\n",k);
  if(k!=i)
    return 1;
  if(expectations>=0)
    if(i!=expectations)
      return 2;
  expectations=i-j;
  return 0;
 fatal_exit:
  return SQLCODE;
}

int double_test(int swi)
{
  int rc;
  switcher=swi;
  rc=simple_selection();
  if(rc)
    return rc;
  switcher=0;
  return simple_selection();
}

int positioned_delete(void)
{
  return double_test(1);
}

int positioned_update(void)
{
  return double_test(3);
}

int searched_delete(void)
{
  $ delete from tbl0 where k2=:k2 and k1=:k1 ;
  shift(); 
  expectations--;
  return simple_selection();
fatal_exit:
  return SQLCODE;
}

int searched_update(void)
{
  $ update tbl0 set taila = -2 - taila where k2 = :k2 and k1 = :k1 ;
  shift(); 
  expectations--;
  return simple_selection();
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
  t2m= N_OF_K1 + N_OF_K2/5;  /* filter for n_of_k1 + numer of the record */
  sh = "%34%";               /* filter for "   %010d   " strings         */
  k1 = N_OF_K1/2;
  k2 = N_OF_K2 - 5 ;
  assert(k2 > 10);
  assert(k1 > 0);
}

int main(int argc,char **argv)
{
  int pass = 0;
  int loop = 3;

  for(loop = 1; loop; loop--, RUN(rollback_work))
    {
      printf("pass %d...\n",pass++);
      refresh_settings();
      /*
        RUN(leveled_selections);
        RUN(simple_selection);
        */
      RUN(positioned_delete);
      RUN(positioned_update);

      RUN(searched_delete);
      RUN(searched_update);

    }
      
  RUN(commit_work);

  return 0;

}
