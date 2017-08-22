#include <stdio.h>
#include <stdlib.h>

char  info[20];
int   k;

$ whenever sqlerror goto errexit;

int
show_data(int lo,int hi)
{
  $ declare show0 cursor for
   ( select k,info 
       from ins
       where :lo <= k and k <= :hi  
   )
  ;
  $ open show0;
  fputs("|-------+--------------------|\n",stdout);
  while(1)
    {
      $ fetch show0 into :k,:info;
      if(SQLCODE==100)break;
      printf("|%7d|%20s|\n",k,info);
    }
  fputs("|-------+--------------------|\n",stdout);
  $ close show0;
  return 0;
errexit:
  return SQLCODE;
}

int
check_data(int expect)
{
  int i=0, e = expect;
  $ declare show1 cursor for
   ( select k,info
       from ins )
    order by 1
  ;
  $ open show1;
  while(1)
    {
      $ fetch show1 into :k,:info;
      if(SQLCODE==100)break;
      if(k!=i)
        {
          printf("mismath: %d -> %d(%s)\n",i,k,info);
          i = k;
        }
      fprintf(stderr,"\b\b\b\b\b%4d.",i);
      i++;
      e--;
    }
  $ close show1;
  if(e!=-1)
    printf("unexpected number of rows: %d(%d)\n",i,expect+1);
  return 0;
errexit:
  return SQLCODE;
}

int
main(int argc,char **argv)
{
  int pass = 0;
  int i = 0;

  fprintf(stderr,"deleting from ins...\n");
  $ delete from ins ;
  $ commit work;
  fprintf(stderr,"deleting commited;\n");
  for(i=0;i<=501;i++)
    {
      k = i;
      sprintf(info," %04d",i);
      fprintf(stderr,"\r%s.",info);
      $ insert into ins (k,info)
	values (:k,:info );
      if(i%5000==0)
        show_data(-1,i + 1);
      else if (i%400==1)
        {
          fprintf(stderr,"\nChecking ...     ");
          check_data(i);
          fprintf(stderr,"  Checked\n");
        }
    }
  check_data(i-1);
  
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
