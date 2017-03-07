/*
 *  xmem.h - interface of extended functions for memory processing of
 *	     GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  This code extracted from GNU CC and partially modified by Michael Kimelman
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
 *  Contact:  gss@ispras.ru
 *
 */

#ifndef __XMEM_H__
#define __XMEM_H__

/* $Id: xmem.h,v 1.248 1998/09/29 21:26:01 kimelman Exp $ */

#include "setup_os.h"
#include <setjmp.h>
#if STDC_HEADERS
# include <string.h>
#else
# include <strings.h>
#endif

#define ALIGN(ptr,align_size)  \
   { if (ptr%align_size) ptr+=align_size-ptr%align_size; }

#define TYP_ALLOC(num, type) ((type *) xmalloc (sizeof (type) * (num)))
#define TYP_REALLOC(var, num, type) { var = (type *) xrealloc (var, sizeof (type) * (num)); }

#define CHECK_ARR_SIZE(arr, old_cnt, new_cnt, type)        \
        CHECK_ARR_ELEM(arr, old_cnt, (new_cnt) - 1, 1, type)

/* in the next definition : delta - number of elements are *
 * being added to arr if its old size was <= el_num        *
 * delta must be >= 1 if there is needed that after this   *
 * operation arr contains element with number el_num       */

#define CHECK_ARR_ELEM(arr, old_cnt, el_num, delta, type)       \
        if (old_cnt <= el_num)                                  \
          {                                                     \
            if (old_cnt) {                                      \
              TYP_REALLOC (arr, el_num + delta, type);          \
              bzero (((char*)arr) + sizeof (type) * (old_cnt),  \
                     (el_num + delta - old_cnt)*sizeof (type)); \
	    }                                                   \
            else                                                \
              arr = TYP_ALLOC (el_num + delta, type);           \
            old_cnt = el_num + delta;                           \
          }

#if defined(HAVE_MEMCPY)
# if 0
#  define bzero(a,c)    Bzero((char*)(a),c)
#  define bcopy(a,b,c)  Bcopy((char*)(a),(char*)(b),c)
#  define bcmp(a,b,c)   Bcmp((char*)(a),(char*)(b),c)
# else
#  define bzero(a,c)    memset((char*)(a),0,c)
#  define bcopy(s,d,n)  memcpy((char*)(d),(char*)(s),(n))
#  define bcmp(a,b,n)   memcmp((char*)(a),(char*)(b),(n))
# endif /* #if 0 */
#endif /* defined(HAVE_MEMCPY) */

#define TYP_COPY(a, b, num, type) bcopy (a, b, (num) * sizeof (type))

#if 0
void Bzero __P((register char *b,register i4_t length));
void Bcopy __P((register char *src,register char *dest,register i4_t length));
i4_t  Bcmp  __P((register char *b1,register char *b2,register i4_t length));
#endif

#define yyerror(s) perror_with_name(s)
#define yyfatal(s) pfatal_with_name(s)

extern char *current_user_login_name;

char * get_user_name  __P((void));
void   fatal  __P((char *str,char *arg));
void   fancy_abort  __P((char *f, int l));
void   lperror  __P((char *fmt,...));
void   perror_with_name  __P((char *name));
void   pfatal_with_name  __P((char *name));
void   memory_full  __P((void));
void * xmalloc  __P((i4_t size));
void * xxmalloc  __P((i4_t size,char *f,int l));
void * xrealloc  __P((void *old,i4_t size));
void * xcalloc  __P((i4_t number,i4_t size));
void   xfree  __P((void *ptr));
char * savestring  __P((char *input));

/*--------------------------------------------------------------------*/
/*****************   Buffers processing   *****************************/
/*--------------------------------------------------------------------*/
typedef struct 
{
  i4_t   buf_len,clear_buf;
  char *buffer;
} str_buf;

char  * buffer_string  __P((str_buf *bf,char *sptr,i4_t l));
#define BUFFER_STRING(b,s)  buffer_string(&(b),s,(s?strlen(s):0))

#define no_static

#define ARR_DECL(name,typ,modifier)	                	\
   modifier struct { typ *arr,*d,*u; i4_t count,limit; } name;	

#define ARR_PROC_PROTO(name,typ)        \
void name##_ini_check __P((void));		\
void name##_ini __P((void));  		\
void name##_put  __P((typ pn));		

#define ARR_PROC_DECL(name,typ,modifier)	\
modifier void name##_ini_check(void)		\
{						\
  assert ( name.d   == name.arr);		\
  assert ( name.u   == name.arr);		\
  assert ( name.count == 0);			\
  assert ( name.limit == 0);			\
}						\
						\
modifier void name##_ini(void) 			\
{						\
  if (name.arr)					\
    {						\
      if (name.limit > 256)			\
        {					\
          xfree (name.arr);			\
          name.arr = NULL;			\
        }					\
      name.d = name.u = name.arr;       	\
      name.count = name.limit = 0;		\
    }						\
  else						\
    name##_ini_check();				\
}						\
						\
modifier void name##_put (typ pn)		\
{						\
  if (name.limit == name.count)			\
    {						\
      typ *old = name.arr;			\
      name.limit += 64;				\
      if (name.arr)				\
        name.arr = (typ *) xrealloc ((void *) name.arr, (unsigned) name.limit * sizeof(typ)); \
      else					\
        name.arr = (u2_t *) xmalloc ((unsigned) name.limit * sizeof(typ)); \
      name.d = name.arr + (i4_t)(name.d - old);	\
      name.u = name.arr + (i4_t)(name.u - old);	\
    }						\
  name.arr[name.count++] = pn;			\
  name.d++;	/* point to the next hole */	\
  assert(name.d == &(name.arr[name.count]));	\
} 

/*--------------------------------------------------------------------*/
/************* EXCEPTION MACROPROCESSING ******************************/
/*--------------------------------------------------------------------*/

typedef struct catch
{
  char         *ident;
  jmp_buf       env;
  struct catch *next;
} catch_t;

extern catch_t *env_stack_ptr;

#define EXCEPTION(code) {			\
  if (env_stack_ptr)				\
    longjmp (env_stack_ptr->env, code );	\
  else						\
    exit(code);					\
}


#define TRY					\
{						\
  catch_t catch_buffer;				\
  i4_t     catch_buffer_rc;			\
  catch_buffer.next = env_stack_ptr;		\
  env_stack_ptr = & catch_buffer;		\
  catch_buffer_rc = setjmp(catch_buffer.env);	\
  if (catch_buffer_rc==0)			\
    {

#define CATCH					\
      env_stack_ptr = catch_buffer.next;	\
    }						\
  else						\
    {						\
      env_stack_ptr = catch_buffer.next;	\
      switch (catch_buffer_rc)			\
        {
      
#define END_TRY					\
          break;				\
        END_TRY0

#define END_TRY0				\
          break;				\
        }					\
    }						\
}


#define TRY_CATCH(execute,catch_cases) TRY execute; CATCH catch_cases; END_TRY

#endif
