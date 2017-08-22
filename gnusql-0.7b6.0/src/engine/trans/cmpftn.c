/*
 *  cmpftn.c  -
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by  Vera Ponomarenko
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: cmpftn.c,v 1.249 1998/05/20 05:52:42 kml Exp $ */

#include <stdio.h>
#include "rnmtp.h"
#include "cmpdecl.h"
#include "sctp.h"
#include "totdecl.h"

#define size2b sizeof(i2_t)
#define size4b sizeof(i4_t)
#define	BETWEEN_CMP  (at == SS || at == SES || at == SSE || at == SESE)

#define _1 ((u2_t) ~0)

static  i4_t cmpm[3][3] =
{/* 0 other _1 <- n1    n2   */
  { 0,  1,  1   },   /*   0   */
  {-1,  2,  1   },   /* Other */
  {-1, -1,  0   }    /*  _1   */
};

#define CMP(n1,n2) {				\
  i4_t ret; ret = cmpm [(n1==0?0:(n1==_1?2:1))][(n2==0?0:(n2==_1?2:1))];  \
  if (ret != 2) return ret; }
                                                                           
i4_t
f1b (char *a1, char *b1, u2_t n1, u2_t n2)
{
  CMP(n1,n2);
  return *(i1_t *)a1 - *(i1_t *)b1;
}

i4_t
f2b (char *a1, char *b1, u2_t n1, u2_t n2)
{
  CMP(n1,n2);
  return t2bunpack(a1) - t2bunpack(b1);
}

i4_t
f4b (char *a1, char *b1, u2_t n1, u2_t n2)
{
  i4_t a, b;
  
  CMP(n1,n2);
  a = t4bunpack(a1);
  b = t4bunpack(b1);  
  if (a > b) return ( 1);
  if (a < b) return (-1);
  return (0);
}

i4_t
flcmp (char *a1, char *b1, u2_t n1, u2_t n2)
{
  float a, b;

  CMP(n1,n2);
  *((i4_t*)&a) = t4bunpack(a1);
  *((i4_t*)&b) = t4bunpack(b1);  
  if (a > b)  return ( 1);
  if (a < b)  return (-1);
  return (0);
}
int
fl_cmp (char *a1, char *b1)
{
  float a, b;

  *((i4_t*)&a) = t4bunpack(a1);
  *((i4_t*)&b) = t4bunpack(b1);  
  if (a > b)  return ( 1);
  if (a < b)  return (-1);
  return (0);
}

char *
ftint (i4_t at, char *a, char **a1, char **a2, i4_t n)
/* function defines intervals' ranges */
{
  *a1 = a;
  a += n;
  
  if (BETWEEN_CMP)
    {
      *a2 = a;
      a += n;
    }
  else
    *a2 = *a1;

  return (a);
}

char *
ftch (i4_t at, char *a, char **a1, char **a2, u2_t *n1, u2_t *n2)
/* function defines intervals' ranges */
/* function returns the same result to a1 and a2 for single predicate.  *
 * For double predicate (between) - to a1 and a2 - interval bounds.     */
{
  u2_t n;
  
  n = t2bunpack (a);
  a += size2b;
  
  *a1 = a;
  a += n;
  *n1 = n;
  
  if (BETWEEN_CMP)
    {
      n = t2bunpack (a);
      a += size2b;      
      *a2 = a;
      *n2 = n;
      a += n;
    }
  else
    {
      *a2 = *a1;
      *n2 = n;
    }
  return (a);
}

i4_t
chcmp (char *a, char *b, u2_t n1, u2_t n2)
{
  CMP(n1,n2);
  for (; *a == *b; a++, b++, n1--, n2--)
    {
      if (n1 == 1)
	return (n1 - n2);
      if (n2 == 1)
	return (1);
    }
  return (*a - *b);
}

int
ch_cmp (char *a, char *b)
{
  u2_t n1, n2;

  n1 = t2bunpack (a);
  a += size2b;
  n2 = t2bunpack (b);
  b += size2b;
  for (; *a == *b; a++, b++, n1--, n2--)
    {
      if (n1 == 1)
	return (n1 - n2);
      if (n2 == 1)
	return (1);
    }
  return (*a - *b);
}

i4_t
ffloat (char *a, char *b, u2_t n1, u2_t n2)
{
  i4_t msa, msb, rsa, rsb;
  i4_t i;
  char *ra, *rb;

  CMP(n1,n2);
  for (msa = 0, ra = a; *ra != 'e' && *ra != 'E'; ra++)
    msa++;
  rsa = n1 - msa - 1;
  ra++;
  for (msb = 0, rb = b; *rb != 'e' && *rb != 'E'; rb++)
    msb++;
  rsb = n2 - msb - 1;
  rb++;
  if ((i = digcmp (ra, rb, rsa, rsb)) != 0)
    return (i);			/* POWER COMPARISON */
  return (digcmp (a, b, msa, msb));	
}

int
f_float (char *a, char *b)
{
  i4_t msa, msb, rsa, rsb;
  i4_t i;
  u2_t n1, n2;
  char *ra, *rb;

  n1 = t2bunpack (a);
  a += size2b;
  n2 = t2bunpack (b);
  b += size2b;
  for (msa = 0, ra = a; *ra != 'e' && *ra != 'E'; ra++)
    msa++;
  rsa = n1 - msa - 1;
  ra++;
  for (msb = 0, rb = b; *rb != 'e' && *rb != 'E'; rb++)
    msb++;
  rsb = n2 - msb - 1;
  rb++;
  if ((i = digcmp (ra, rb, rsa, rsb)) != 0)
    return (i);			/* POWER COMPARISON */
  return (digcmp (a, b, msa, msb));	
}

i4_t
digcmp (char *a, char *b, u2_t n1, u2_t n2)
{
  if (n1 == 0)
    {
      if (n2 == 0)
	return (0);
      else
	return (-1);
    }
  if (*a == '-')
    {
      if (*b == '+' || *b != '-')
	return (-1);		/* unequal signs */
      return (-chcmp (a, b, n1, n2));
    }
  if (*b == '-')
    return (1);			/* unequal signs */
  return (chcmp (a, b, n1, n2));
}

int
cmpval (char *val1, char *val2, u2_t type)
{
  switch (type)
    {
    case T1B:
      return ((i1_t)*val1 - (i1_t)*val2);
    case T2B:
      return (t2bunpack (val1) - t2bunpack (val2));
    case T4B:
      return (t4bunpack (val1) - t4bunpack (val2));
    case TFLOAT:
      return (fl_cmp (val1, val2));
    case TFL:
      return (f_float (val1, val2));
    case TCH:
      return (ch_cmp (val1, val2));
    default:
      printf ("TRN.cmpval: This data type is incorrect");
      break;
    }
  return (0);
}
