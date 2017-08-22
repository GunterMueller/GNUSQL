/*
 * $Id: testM1.ec,v 1.6 1998/09/11 19:53:26 kimelman Exp $
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
 *
 * $Log: testM1.ec,v $
 * Revision 1.6  1998/09/11 19:53:26  kimelman
 * add log mark
 *
 */

#include <stdio.h>
#include <stdlib.h>

int    N_OF_K1 = 200;
int    N_OF_K2 = 200;
int    N_OF_T0 = 200;

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

int simple_insertation(void)
{
  int i;
  fprintf(stderr,"\n");
  
  $ whenever sqlerror continue;
  for(i=0;i<N_OF_K1;i++)
    {
      int v;
      char s[50];
      v=i;                                      /* append to the end of index */
      sprintf(s,"     %010d     ",(N_OF_K1-i)); /* insert at the top of index */
      fprintf(stderr,
              "\rRun simple_insertation ... c1(%d - %4d,%10s)       ",i,v,s);
      $ insert into tbl1 (k1,t1)
	values (:v,:s );
      if (SQLCODE<0)
        return SQLCODE;
    }
  fprintf(stderr,"\n");
  for(i=0;i<N_OF_K2;i++)
    {
      int v;
      short sv;
      /* middle sequence */
        
      sv = (i%2?N_OF_K2 - 1 - i/2:i/2);         /* insert in the middle   */
      v  = N_OF_K1 + i;
      fprintf(stderr,
              "\rRun simple_insertation ... c2(%d - %5d,%10d)       ",i,sv,v);
      $ insert into tbl2 (k2,t2)
	values (:sv,:v );
      if (SQLCODE<0)
        return SQLCODE;
    }
  return 0;
}

$ whenever sqlerror goto fatal_exit;

int
show_22(void)
{
  int t2;
  
  $ declare show22 cursor for
    ( select k2 from tbl2 group by k2,t2 )
    order by 1 ;
  $ open show22;
  fputs("---c2----\n",stdout);
  while(1) 
    {
      $ fetch show22 into :k2;
      if(SQLCODE==100) break;
      printf("\n|%7d| ",k2);
    }
  fputs("\n----c2---\n",stdout);
  $ close show22;
  return 0;
 fatal_exit:
  return SQLCODE;
}  
  
int
complex_insertation(void)
{
  int   i = 0;
  int   fails=0;
  int   cnt =0;
  float ta;
  char  tb[20];
  char  c[] = "-\\|/";
  
  $ declare c1 cursor for
    ( select k1 from tbl1 )
    order by k1  ;
  $ declare c2 cursor for
    ( select k2 from tbl2 group by k2,t2 )
    order by 1 ;

  $ select count(distinct k1) into :i from tbl1 ;
  fprintf(stderr,"count(tbl1) = %d \n",i);
  N_OF_K1=i;
  $ select count(distinct k2) into :i from tbl2 ;
  fprintf(stderr,"count(tbl2) = %d \n",i);
  N_OF_K2=i;
  if (N_OF_T0 > N_OF_K1*N_OF_K2/2)
    N_OF_T0 = N_OF_K1*N_OF_K2/2;
  SQL__cache(0);

  i = cnt = 0;
  k2=-1;
  $ open c2;
  while(i<N_OF_T0)
    {
      {
        int old_k2 = k2;
        $ fetch c2 into :k2;
        if (SQLCODE==100)
          {
            $ close c2;
            $ open c2;  /* start whole loop from the beginning  */
            fprintf(stderr,"\n restart loop \n ");
            k2=-1;
            continue;
          }
        if (k2 <= old_k2)
          {
            fprintf(stderr,"\n incorrect c2 scan (k2=%d)\n ",k2);
            continue;
          }
        
      }
      
      $ open c1;
      while(i<N_OF_T0)
        {
          $ fetch c1 into :k1;
          if (SQLCODE==100)
            {
              $ close c1;
              break;
            }

          fprintf(stderr,"\b\b\b\b\b\b%4d%%%c",(int)i,c[i%4]);
#if 0
          $ select  k1 into :cnt from tbl0 where k1 = :k1 and k2 = :k2;
          if (SQLCODE!=100)
            continue;
#endif
          
          $ whenever sqlerror continue;

          switch(cnt++%7)
            {
            case 3:
              i++;
              if (k2 == 0)
                ta = k1;
              else
                ta=k1/k2;
              sprintf(tb,"userhole(%g)",ta);
              $ insert into tbl0 ( k2, k1 ,tailb, taila )
                values ( :k2, :k1, :tb, :ta ) ;
              break;
            case 0:
              i++;
              sprintf(tb,"case 1(%d)",i);
              $ insert into tbl0 ( k2, k1 ,tailb, taila)
                values ( :k2, :k1, :tb , 0.5) ;
              break;
            case 2:
              ta = i++;
              $ insert into tbl0 ( k1, k2 ,taila)
                values ( :k1, :k2, :ta ) ; 
              break;
            default:
              i++;
              sprintf(tb,"default (%d)",i);
              $ insert into tbl0 ( k2, k1 ,tailb, taila)
                values ( :k2, :k1, :tb , 100 ) ;
              break;
            }
          if (SQLCODE == -1) /* if key not unique */
            {
              fprintf(stderr,
                      "\ncomplex_insertation ... %4d(%3d:%2d) %4d %4d\n",
                      i--,cnt,fails,k1,k2);
              fails++;
            }
          else if (SQLCODE<0)
            {
              fprintf(stderr,"\ncomplex_insertation ... %4d %4d %4d\n",
                      i-1,k2,k1);
              fprintf(stderr,"insertation problem:'%s' \n",gsqlca.errmsg);
              return SQLCODE;
            }
          if (SQLCODE == 0 && fails > 0)
            fails=0;
          if (fails < 0)
            fails=-fails;
        }
    }
  return 0;
 fatal_exit:
  return SQLCODE;
}

int
fast_complex_insertation(void)
{
  int   i   = 0;
  int   cnt = 0;
  float ta;
  char  tb[20];
  char  c[] = "-\\|/";
  
  fprintf(stderr,"\n");

  if (N_OF_T0 > N_OF_K1*N_OF_K2/2)
    N_OF_T0 = N_OF_K1*N_OF_K2/2;
  SQL__cache(N_OF_T0/10);
  /* SQL__cache(0); */
  for(k1 = N_OF_K1-1; k1 >=0;k1--)
    for(k2 = N_OF_K2-1; k2 >=0; k2--)   /* easiest way of include */
      {
        if ( N_OF_T0 <= i)
          break;
#if 1
        fprintf(stderr,"%5d %c\b\b\b\b\b\b\b",i,c[i%4]);
#else
        if (i%50 == 1) 
          fprintf(stderr,"%c\b",c[(i/50)%4]);
#endif
        $ whenever sqlerror continue;

        switch(i%7)
          {
          case 3:
            i++;
            if (k2 == 0)
              ta = k1;
            else
              ta=k1/k2;
            sprintf(tb,"userhole(%g)",ta);
            $ insert into tbl0 ( k2, k1 ,tailb, taila )
              values ( :k2, :k1, :tb, :ta ) ;
            break;
          case 1:
            i++;
            sprintf(tb,"case 1(%d)",i);
            $ insert into tbl0 ( k2, k1 ,tailb, taila)
              values ( :k2, :k1, :tb , 0.5) ;
            break;
          case 2:
            ta = i++;
            $ insert into tbl0 ( k1, k2 ,taila)
              values ( :k1, :k2, :ta ) ;
            break;
          default:
            i++;
            sprintf(tb,"default (%d)",i);
            $ insert into tbl0 ( k2, k1 ,tailb, taila)
              values ( :k2, :k1, :tb , 100 ) ;
          }
        if (SQLCODE <0)
          {
            fprintf(stderr,"\ncomplex_insertation ... %4d %4d %4d    ",i,k2,k1);
            fprintf(stderr,"insertation problem:'%s' \n",gsqlca.errmsg);
            i--;
            return SQLCODE;
          }
      }
  fprintf(stderr,"\nfast complex_insertation: inserted ?? rows... (checking)");
  $ select count (*) into :i from tbl0 ;
  fprintf(stderr,"\rfast complex_insertation: inserted %d rows              \n",i);
  
fatal_exit:
  SQL__cache(0);
  return SQLCODE;
}

$ whenever sqlerror goto fatal_exit;

int
show_0(void)
{
  float ta;
  char tb[20];
  
  $ declare show0 cursor for
   ( select k1,k2,taila,tailb 
     from tbl0 )
  ;
  $ open show0;
  fputs("---------------------c0-----------------------\n",stdout);
  fputs("|  k1   |  k2   | taila |   tailb            |\n",stdout);
  fputs("----------------------------------------------\n",stdout);
  while(1)
    {
      $ fetch show0 into :k1,:k2, :ta, :tb;
  
      if(SQLCODE==100) break;
      printf("|%7d|%7d|%7.3g|%20s|\n",k1,k2,ta,tb);
    }
  fputs("---------------------c0-----------------------\n",stdout);
  $ close show0;
  return 0;
 fatal_exit:
  return SQLCODE;
}

int
show_1(void)
{
  char t1[40];
  
  $ declare show1 cursor for
    ( select k1,t1 
      from tbl1 )
    ;
   $ open show1;
  fputs("---------------------c1---------------------------\n",stdout);
  fputs("|  k1   |                    t1                  |\n",stdout);
  fputs("---------------------c1---------------------------\n",stdout);
  while(1) 
    {
      $ fetch show1 into :k1, :t1;
      if(SQLCODE==100) break;
      printf("|%7d|%40s|\n",k1,t1);
    }
  fputs("---------------------c1---------------------------\n",stdout);
  $ close show1;
  return 0;
 fatal_exit:
  return SQLCODE;
}

int
show_2(void)
{
  int t2;
  
  $ declare show2 cursor for
    ( select k2,t2 
        from tbl2 )
  ; 
  $ open show2;
  fputs("---------c2---------\n",stdout);
  fputs("|  k2   |   t2     |\n",stdout);
  fputs("--------------------\n",stdout);
  while(1) 
    {
      $ fetch show2 into :k2, :t2;
      if(SQLCODE==100) break;
      printf("|%7d|%10d|\n",k2,t2);
    }
  fputs("---------c2---------\n",stdout);
  $ close show2;
  return 0;
 fatal_exit:
  return SQLCODE;
}  
  
int
show_data(void)
{
  if ( show_0()!=0 ) return SQLCODE;
  if ( show_1()!=0 ) return SQLCODE;
  if ( show_2()!=0 ) return SQLCODE;
  return 0;
}

int
leveled_selections(void)
{
  int k = -1;
  if (show_data() !=0)
    return SQLCODE;
  printf("filter level 0...");  
  $ select count(*)
    into :k
    from tbl0
    ;
  printf("\nfilter level 0 - %d\n",k);  
  printf("filter level 1a...");  
  $ select count(*)
    into :k
    from tbl0 where taila>0
    ;
  printf("\nfilter level 1a- %d\n",k);  
  printf("filter level 1b --");  
  $ select count(*)
    into :k
    from tbl0
    where k1 in (select distinct k1 from tbl1 where t1 like 'k=%' escape '#' ) 
    ;
  printf("\nfilter level 1b- %d\n",k);  
  printf("filter level 1c0..");
  $ select count (distinct tbl2.k2) into :k from tbl2 where k2=10 and t2<:t2m ;
  printf("\nfilter level 1c0- %d\n",k);
  printf("filter level 1c --");
  $ select count(*)
    into :k
    from tbl0
    where not ( 0 = ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
    ;
  printf("\nfilter level 1c- %d\n",k);  
  printf("filter level 2a --");  
  $ select count(*)
    into :k
    from tbl0
    where
      not ( 0 = ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
      and taila>0
    ;
  printf("\nfilter level 2a- %d\n",k);  
  printf("filter level 2b --"); 
  $ select count(*)
    into :k
    from tbl0
    where k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and taila>0
    ;
  printf("\nfilter level 2b- %d\n",k);  
  printf("filter level 2c --");
  $ select count(*)
    into :k
    from tbl0
    where k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and not ( 0 = ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
    ;
  printf("\nfilter level 2c- %d\n",k);  
  printf("filter level 3 --");
  $ select count(*)
    into :k
    from tbl0
    where 
          k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and not ( 0 =   
          ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
      and taila>0
  ;
  printf("\nfilter level 3 - %d\n",k);
  return 0;
 fatal_exit:
  return SQLCODE;
}

int
simple_selection(void)
{
  int i,j,k;
  $ declare curs cursor for
    ( select tailb,k1,k2
      from tbl0
      where
            k1 = SOME ( select k1 from tbl1 where t1 like :sh escape '#'   ) 
        and exists    ( select *  from tbl2 where k2 = tbl0.k2 and t2<:t2m )
        and taila > 0
    )    
  ;
  $ open curs;
  fputs("\n---------------------\n",stdout);
  for(j=i=0;;i++)
    {
      $ fetch curs into :tailb,:k1,:k2;
      if(SQLCODE==100) break;
      printf("|%7d|%7d|%40s|",k1,k2,tailb);
      if(i%switcher==0)
	switch(switcher)
	  {
	  case 5:
            printf(" deleted \n");
	    $ delete from tbl0 where current of curs ;
	    j++;
	    break;
	  case 3:
            printf(" taila changed sign \n");
	    $ update tbl0 set taila=-taila where current of curs ;
	    j++;
	    break;
	  default:
            printf("\n");
	    break;
	  }
    }
  $ close curs;
  fputs("---------------------\n",stdout);
  $ select count(*)
    into :k
    from tbl0
    where 
          k1 in (select distinct k1 from tbl1 where t1 like :sh escape '#' ) 
      and not ( 0 =   
          ( select count (distinct k2) from tbl2 where k2=tbl0.k2 and t2<:t2m) )
      and taila>0
  ;
  if(k!=i-j)
    {
      printf(" DB counts mismatch local counts\n");
      return 1;
    }
  
  if(expectation >= 0 && expectation != i)
    {
      printf(" cursor counts mismatch expectation \n");
      return 2;
    }
  expectation -= j;
  return 0;
 fatal_exit:
  return SQLCODE;
}

int double_test(int swi)
{
  int rc;
  char buf[100];
  expectation = -1;
  t2m= N_OF_K1 + N_OF_K2/(swi+1);  /* filter for n_of_k1 + numer of the record */
  sprintf(buf,"%%%d%%",swi);
  sh = buf;                        /* filter for "   %010d   " strings         */
  switcher=swi;
  rc=simple_selection();
  if(rc)
    return rc;
  switcher=0;
  return simple_selection();
}

int positioned_delete(void)
{
  return double_test(5);
}

int positioned_update(void)
{
  return double_test(3);
}

int searched_delete(void)
{
  $ delete from tbl0 where k2=:k2 and k1=:k1 ;
  shift();
  return simple_selection();
fatal_exit:
  return SQLCODE;
}

int searched_update(void)
{
  $ update tbl0 set taila = -2 - taila where k2 = :k2 and k1 = :k1 ;
  shift(); 
  return simple_selection();
fatal_exit:
  return SQLCODE;
}

int total_delete(void)
{
  fprintf(stderr,"t0...");
  $ delete from tbl0 ;
  fprintf(stderr,"t1...");
  $ delete from tbl1 ;
  fprintf(stderr,"t2...");
  $ delete from tbl2 ;
  expectations = 0;
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
  int rc;
  fprintf(stderr,"Run %s ...",tn);
  rc = f();
  if(rc)
    {
      fprintf(stderr,": failed(%d): '%s':%d\n",rc,gsqlca.errmsg,SQLCODE);
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

  /* clear place at the beginning */
  if (argc > 1)
    goto skip;
  printf("pass %d...\n",pass++);
  RUN(total_delete);
  RUN(commit_work);
  
  /* checking of simple insert & indexes */
  printf("pass %d...\n",pass++);
  RUN(simple_insertation);
  RUN(commit_work);
  RUN(total_delete);
  RUN(commit_work);
  
  printf("pass %d...\n",pass++);
  N_OF_T0 = N_OF_K1 * N_OF_K2/ 20;
  loop = 2;
  /* protest: 1st look - just running in all direction a a bit */
  printf("pass %d...\n",pass++);
  while(loop--)
    {
      printf("pass %d(%d)...\n",pass,loop);
      if (loop%2)
        RUN(commit_work);
      RUN(show_data);
      RUN(simple_insertation);
      if (loop%2)
        RUN(commit_work);
      RUN(fast_complex_insertation);
      if (loop%2)
        RUN(commit_work);
      RUN(total_delete);
    }
  RUN(commit_work);

  
#if 0
  printf("pass %d...\n",pass++);
  
  RUN(leveled_selections);
  RUN(commit_work);
  
  
  printf("pass %d...\n",pass++);
  /*      RUN(show_data);*/
  RUN(total_delete);
  RUN(commit_work);
#endif

  /* 1st pass: loading */
  
skip:  
  N_OF_T0 = N_OF_K1 * N_OF_K2/ 20;
  
  if (argc > 2)
    goto skip1;
  
  RUN(total_delete);
  RUN(commit_work);
  printf("pass %d...\n",pass++);
  RUN(simple_insertation);
  RUN(commit_work);
  RUN(complex_insertation);
  RUN(commit_work);
  
skip1:
  
  /* 2nd pass: checking functionalities with deletes */
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
