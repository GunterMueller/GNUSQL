/*
 *  cs_link.c  -  library used by both client and server of GNU SQL compiler
 *
 *  NOTE:(!!)  This file does NOT compiled by itself. It's only included
 *             by main routine of either client or server part of compiler.
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman.
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: cs_link.c,v 1.246 1998/07/30 03:23:37 kimelman Exp $ */

#ifndef __CS_LINK_H__
#define __CS_LINK_H__

#if   !defined(__CLIENT__) && !defined(__SERVER__)
#error File cs_link.c can`t be included here!!!!
#endif

#include "setup_os.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef STDC_HEADERS 
#include <string.h>
#else
#include <strings.h>
#endif

#include "gsqltrn.h"           /* c/s interface structures */

#ifdef __SERVER__
static i4_t last_comp_pass = sizeof (compiler_passes) / sizeof (*compiler_passes) - 2;
#endif

#ifdef __SERVER__
#define STD_FLS_PROC  { OP(STDERR,"err"); OP(STDOUT,"out"); }
#else
#define STD_FLS_PROC
#endif

#define PROCESS_FILES  {			\
  i4_t i;					\
  STD_FLS_PROC OP(sc_file,"Sc");             \
  for(i=0;compiler_passes[i].sname;i++)      \
    if(compiler_passes[i].dump)              \
      { OP(compiler_passes[i].dmp_file,compiler_passes[i].dumpext);} \
}

static i4_t is_open = 0;

int
open_file (FILE ** f, char *ext)
{
  FILE *F = NULL;
  char b[120];

#ifdef __SERVER__
#define DBG under_dbg
#else
#define DBG 1
#endif

  if      ((bcmp(ext,"err",3)==0) && DBG)
    F=stderr;
  else if ((bcmp(ext,"out",3)==0) && DBG)
    F=stdout;
  else
    {
      strcpy (b, sql_prg);
      strcat (b, ".");
      strcat (b, ext);
      remove (b);
#ifdef __CLIENT__
      F = fopen (b, "w");
#else
      F = fopen (b, "w+");
      unlink(b);
#endif
    }
  if (F == NULL)
    {
      fprintf (STDERR, "Can not create file '%s'\n", b);
      return 0;
    }
  if ( f  && (*f != F))
    *f = F;
  return 1;
#undef DBG
}

static void
close_file (FILE ** f, char *ext)
{
  i4_t l = 1;
  if (f)
    {
      if ( (*f) == stderr || (*f) == stdout)
	return;
      if (*f)
	{
          l = ftell(*f);
	  fclose (*f);
	  if (strcmp (ext, "err") == 0)
	    *f = stderr;
          else if (strcmp (ext, "out") == 0)
	    *f = stdout;
	  else
            *f = NULL;
	}
    }
#ifdef __CLIENT__ 
  if ((errors && (strcmp(ext,"c")==0 || strcmp(ext,"Sc")==0 )) || /* if errors occured or    */
      (l==0 &&   strcmp(ext,"c")     && strcmp(ext,"Sc") ) /*there is no output to some file */
      )
#endif
  { /* remove it */
    char b[120];
    strcpy (b, sql_prg);
    strcat (b, ".");
    strcat (b, ext);
    remove (b);
  }
}

static i4_t 
close_out_files (void)
{
  if(!is_open)
    return 1;
#define OP(f,ext)  close_file(&(f),ext);
  PROCESS_FILES;
  is_open = 0;
  return 1;
#undef OP
}

static void 
close_out(void)
{
  close_out_files();
}

static i4_t 
open_out_files (void)
{
  static i4_t done = 0;
  if (done==0)
    {
      ATEXIT(close_out);
      done = 1;
    }
  close_out_files();
#define OP(f,ext)  if(!open_file(&(f),ext)) return 0;
  PROCESS_FILES;
#undef OP
  is_open = 1;
  return 1;
}

#ifdef __SERVER__
static file_buf_t *
get_file_buf (file_buf_t *ptr,FILE * f, char *ext)
{
  register file_buf_t *p;
  register i4_t        i;
  if (f == NULL)
    return ptr;
  i = ftell (f);
  if (i <= 0)
    return ptr;
  p =  (file_buf_t*)xmalloc(sizeof(file_buf_t));
  p->text = (char*) xmalloc (i + 1);
  fseek (f, SEEK_SET, 0);
  fread (p->text, i, 1, f);
  fseek (f, SEEK_SET, 0); /* and set write position to the top again */
  p->text[i] = 0;
  if (!ext) ext = "";
  p->ext = (char*) xmalloc (strlen(ext) + 1);
  strcpy(p->ext,ext);
  p->next = ptr;
  return p;
}

static file_buf_t *
get_files_bufs (void)
{
  register file_buf_t *ptr = NULL;
  
#define OP(f,ext)   ptr = get_file_buf(ptr,f,ext)
  PROCESS_FILES;
#undef OP
  return ptr;
}
#endif

#ifdef __CLIENT__
static i4_t 
put_file_buf (file_buf_t *ptr,FILE * f, char *ext)
{
  if (!f)
    return 0;
  while(ptr)
    if(bcmp(ptr->ext,ext,strlen(ext))!=0)
      ptr=ptr->next;
    else
      {
        fwrite (ptr->text, strlen (ptr->text), 1, f); /* or just fputs(bf,f);*/
        break;
      }
  return 0;
}

static i4_t 
put_files_bufs (file_buf_t * ptr)
{
  if (ptr == NULL)
    return 0;
#define OP(f,ext)   put_file_buf(ptr,f,ext)
  PROCESS_FILES;
#undef OP
  while (ptr)
    {
      if(bcmp(ptr->ext,"err",3)==0)
        fwrite (ptr->text, strlen (ptr->text), 1, stderr);
      if(bcmp(ptr->ext,"out",3)==0)
        fwrite (ptr->text, strlen (ptr->text), 1, stdout);
      ptr=ptr->next;
    }
  return 0;
}

#endif

#endif
