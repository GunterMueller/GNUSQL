/*
 *  options.c  -  set of options processors of GNU SQL compiler
 *
 *  NOTE:(!!)  This file does NOT compiled by itself. It's only included
 *             by main routines of client and server parts of compiler.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman
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

/* $Id: options.c,v 1.246 1998/05/20 05:57:37 kml Exp $ */

#if   !defined(__CLIENT__) && !defined(__SERVER__)
#error File options.c can`t be included here!!!!
#endif

#include <string.h>

#ifdef __SERVER__
#define DEF_PASS(name,title,id,proc,st_id,skip,dmp,ext,dumper) \
    {name,title,id,proc,(skip?~0:0),(dmp?~0:0),ext,NULL,dumper,st_id},
#else /* if only __CLIENT__ is defined */
#define DEF_PASS(name,title,id,proc,st_id,skip,dmp,ext,dumper) \
    {name,title,id,NULL,(skip?~0:0),(dmp?~0:0),ext,NULL,NULL,0},
#endif

struct compiler_pass_descr compiler_passes[]=
{
#include "options.def"
  {NULL, NULL, 0, NULL, 0, 0, NULL, NULL}
};

#ifndef DYN_MODE

#define DEF_OPTION(o_nm,var_nm,is_settled)  {o_nm,&var_nm,NULL},
#define DEF_OPTION1(o_hd,o_proc)            {o_hd,&o_proc##_state,o_proc},

struct sql_options_info sql_options[]=
{
#include "options.def"
  {NULL, NULL, NULL}
};

void 
notdump (FILE * f)
{
}

void 
read_options (i4_t argc, char *argv[])
{
  register i4_t i;
  extern int errno;
  
  errno = -1;
  
  for (i = 1; i < argc; i++)
    {
      register struct sql_options_info *copt;
      if (argv[i][0] != '-')
        {
          if(!progname) /* if no module name is given --> store name of  */
            progname = savestring(argv[i]); /* first program file        */
          continue;
        }
      for (copt = sql_options; copt->name; copt++)
        {
          register char *opt=argv[i]+1;
          register i4_t k = strlen (copt->name),k1=strlen(opt);
      
          if ((0 == strcmp (opt, copt->name)) &&
              (copt->proc == NULL))
            *(copt->ptr_to_var) = -1;
          else if ((k1 == k + 1) &&
                   (0 == strncmp (opt, copt->name, k)) &&
                   (0 == strcmp (opt + k, "-")) &&
                   (copt->proc == NULL))
            *(copt->ptr_to_var) = 0;
          else if ((k1 == k + 1) &&
                   (0 == strncmp (opt, copt->name, k)) &&
                   (0 == strcmp (opt + k, "+")) &&
                   (copt->proc == NULL))
            *(copt->ptr_to_var) = -1;
          else if ((k1 > k) &&
                   (0 == strncmp (opt, copt->name, k)) &&
                   (copt->proc != NULL))
            copt->proc (copt->ptr_to_var, opt + k);
          else
            continue;
          break;
        }
      /*
       * if(copt->name==NULL)
       *   fprintf(STDERR,"Can't recognize option '%s' \n",argv[i]);
       */
    }
}

void 
scan_mode (i4_t *p, char *s)
{
#ifdef __CLIENT__
  extern void client_scan_mode  __P((i4_t *p, char *s));
  client_scan_mode (p, s);
#endif
}

void 
module_name (i4_t *p, char *s)
{
  if(progname)
    xfree(progname);
  progname = savestring(s);
}

void 
server_host(i4_t *p, char *s)
{
#ifdef __CLIENT__
  server_host_name = s;
#endif
}

void 
tree_memory_mode (i4_t *p, char *s)
{
#ifdef __SERVER__
/*  if (s)
    *p = set_virtual_mode (atoi (s)); 
  else
    *p = set_virtual_mode (-1); */
#endif
}

void 
vm_debug_mode (i4_t *ptr, char *s)
{
#ifdef __SERVER__
  extern i4_t debug_vmemory;
  if (s)
    *ptr = debug_vmemory = atoi (s);
  else
    debug_vmemory = *ptr;
#endif
}

static void 
pass_modes (i4_t *state, char c, char *s)
{
  register i4_t kk;
  for (kk = 0; compiler_passes[kk].sname; kk++)
    {
      if (s)
        {
          register i4_t k;
          for (k = strlen (s); k--;)
            if (compiler_passes[kk].atr == s[k])
              {
                if (c == 'd')
                  compiler_passes[kk].dump ^= ~0;
                else if (c == 's')
                  compiler_passes[kk].skip ^= ~0;
                else
                  yyfatal ("unexpected option type descriptor");
              }
          if (c == 'd')
            *state &= compiler_passes[kk].dump ? 1 << kk : 0;
          else if (c == 's')
            *state &= compiler_passes[kk].skip ? 1 << kk : 0;
        }
      else
        /* if s==NULL */
        {
          if (c == 'd')
            compiler_passes[kk].dump = (*state) & (1 << kk);
          else if (c == 's')
            compiler_passes[kk].skip = (*state) & (1 << kk);
        }
    }
}

void 
dump_modes (i4_t *p, char *s)
{
  pass_modes (p, 'd', s);
}

void 
skip_modes (i4_t *p, char *s)
{
  pass_modes (p, 's', s);
}

#endif



	
