/*
 *  xmem.c - extended library for memory processing of GNU SQL compiler
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

/* $Id: xmem.c,v 1.255 1998/09/29 22:23:45 kimelman Exp $ */

#include "xmem.h"
#include <sql.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <setjmp.h>
#include "sql_decl.h"
#include <assert.h>

catch_t *env_stack_ptr = NULL;

void 
fatal (char *str, char *arg)
{
  if (progname)
    fprintf (STDERR, "\n%s: ", progname);
  fprintf (STDERR, str, arg);
  fprintf (STDERR, "\n");
  EXCEPTION(-ER_FATAL);
}

/* More 'friendly' abort that prints the line and file.
config.h can #define abort fancy_abort if you like that sort of thing.  */

void 
fancy_abort (char *f,int l)
{
  fprintf (STDERR, "%s:%d: Internal gsqltrn abort.",f,l);
  EXCEPTION(-ER_FATAL);
}

void
lperror(char *fmt, ...)
{
  extern int errno;
  static char *comp_errlist[]= {
#define  DEF_ERR(e,msg)  msg,
#include   "errors.h"
#undef   DEF_ERR
    NULL};
  
  static i4_t comp_nerr = sizeof(comp_errlist)/sizeof(char*) - 1;
  i4_t    rc = errno;
  va_list va;
  va_start(va,fmt);

  if (progname && *progname)
    fprintf (STDERR, "%s:", progname);
  if (file_pos)
    fprintf (STDERR, "%d:", (int)file_pos);
#if defined(HAVE_VPRINTF)
  vfprintf(STDERR,fmt,va);
#elif defined(HAVE_DOPRNT)
#  error put here correct '_doprnt' call
  _doprnt(STDERR,fmt,va); <<-- check correctness 
#else
#  error Nether 'vprintf' nor 'doprnt' can be found -- fail
#endif
  if (-comp_nerr < rc && rc < 0)
    fprintf (STDERR, "%s", comp_errlist[-errno]);
  fprintf (STDERR, "\n");

  errors++;
  
  errno    = 0;
  file_pos = 0;
}

void 
perror_with_name (char *name)
{
  extern int sys_nerr;
  errno = sys_nerr;
  lperror(name);
}

void 
pfatal_with_name (char *name)
{
  perror_with_name (name);
  EXCEPTION(-ER_FATAL);
}

void 
memory_full (void)
{
  EXCEPTION(-ER_NOMEM);
}

static i4_t xxmalloc_flag = 0;

void *
xxmalloc (i4_t size, char *f, int l)
{
  xxmalloc_flag = 1;
  if (size == 0)
    {
      fprintf (STDERR, "\n Allocating memory (0) at %s.%d\n", f, l);
      return NULL;
    }
  return xmalloc (size);
}

#ifndef USE_XVM

static int before_guard = 0;
static int after_guard = 0;/*100 * sizeof(double)*/

#define M_BORD_B before_guard
#define M_BORD_A after_guard

void *
xmalloc (i4_t size)
{
  register char *ptr = NULL;
  if (size == 0)
    {
      if (!xxmalloc_flag)
	fprintf (STDERR, "\n allocating memory (0) \n");
    }
  else
    {
      errno=EAGAIN;
      while (errno==EAGAIN)
        {
          ptr = malloc (size + M_BORD_B + M_BORD_A);
          if (ptr)
            break;
          sleep(1);
        }
      assert(ptr || errno==ENOMEM);
      if (ptr == 0)
        memory_full ();
#if 0
      bzero (ptr, size + M_BORD_B + M_BORD_A);
      ptr = (char*)ptr + M_BORD_B;
#else
      ptr = (char*)ptr + M_BORD_B;
      bzero (ptr, size);
#endif
    }
  xxmalloc_flag = 0;
  return ptr;
}

void *
xrealloc (void *old, i4_t size)
{
  register char *ptr;
  assert(size>=0);
  if (old == NULL)
    {
      /* fprintf (STDERR, "\n reallocating NULL memory "); */
      return xmalloc(size);
    }
  if (size == 0)
    {
      fprintf (STDERR, "\n reallocating memory (0)");
      xfree (old);
      return NULL;
    }
  ptr = NULL;
  while (ptr == NULL)
    {
      ptr = realloc ((char*)old - M_BORD_B, (size_t) (size + M_BORD_B + M_BORD_A)) ;
      if (ptr == 0)
        {
          /*
           * check for memory avalability.
           * xmalloc suspends execution till required amount of memory
           * become available or fails 
           */
          xfree(xmalloc(size));
        }
    }
  return (char*)ptr + M_BORD_B;
}

void *
xcalloc (i4_t number, i4_t size)
{
  register i4_t total = number * size;
  return xmalloc (total);
}

void 
xfree (void *ptr)
{
  if (ptr)
    free ((char*)ptr - M_BORD_B);
}

#endif

char *
savestring (char *input)
{
  i4_t size = strlen (input);
  char *output = xmalloc (size + 1);
  bcopy(input,output,size + 1);
  return output;
}

void 
Bzero (register char *b, register i4_t length)
{
  while (length-- > 0)
    *b++ = 0;
}

void 
Bcopy (register char *src, register char *dest, register i4_t length)
{
  while (length-- > 0)
    *dest++ = *src++;
}

int
Bcmp (register char *b1, register char *b2, register i4_t length)
{
  while (length-- > 0)
    if (*b1++ != *b2++)
      return 1;
  return 0;
}

char *
buffer_string (str_buf * bf, char *sptr, i4_t l)
{
  i4_t bl;
  if (bf->clear_buf)
    {
      if (bf->buf_len)
	bf->buffer[0] = 0;
      bf->clear_buf = 0;
    }
  if (sptr == NULL)
    {
      if (bf->buffer && bf->buffer[0] == 0)
	{
	  bf->buf_len = 0;
	  xfree (bf->buffer);
	  bf->buffer = NULL;
	}
      bf->clear_buf = 1;
      return bf->buffer;
    }
  if (bf->buffer == NULL)
    {
      bf->buffer = xmalloc (bf->buf_len = l + 1);
      bf->buffer[0] = 0;
    }
  if (bf->buf_len < (bl = strlen (bf->buffer) + l + 1))
    bf->buffer = xrealloc (bf->buffer, bf->buf_len = bl);
  bl = strlen (bf->buffer);
  bcopy (sptr, bf->buffer + bl, l + 1);
  return bf->buffer;
}


extern char *getlogin(void);
extern char *cuserid(char *buf);

char *current_user_login_name=NULL;

char *
get_user_name (void)
{
  char *s=NULL;
  s = current_user_login_name;
#ifdef HAVE_GETLOGIN
  if ((s == NULL) || (*s == 0))
    s= getlogin();
#endif
#ifdef HAVE_CUSERID
  if ((s == NULL) || (*s == 0))
    s= cuserid(NULL);
#endif
  if ((s == NULL) || (*s == 0))
    s = getenv ("user");
  if ((s == NULL) || (*s == 0))
    s = getenv ("USER");
  if ((s == NULL) || (*s == 0))
    s = getenv ("LOGNAME");
  if ((s == NULL) || (*s == 0))
    s = "usr";
  return s;
}

#include "totdecl.h"

#ifdef PROC_PACK

i4_t  t4bunpack(char *pnt)       { i4_t  n; TUPACK(pnt,n); return n; }
void t4bpack(i4_t  n,char *pnt)  { TPACK(n,pnt); }
u2_t t2bunpack(char *pnt)       { u2_t n; TUPACK(pnt,n); return n; }
void t2bpack(u2_t n,char *pnt)  { TPACK(n,pnt); }

#endif

#ifndef NOT_DEBUG

int
wait_debugger(void)
{
  static volatile int status = 20000;
  if (status >0)
    {
      status--;
      sleep(1);
    }
  return status;
}

#endif
