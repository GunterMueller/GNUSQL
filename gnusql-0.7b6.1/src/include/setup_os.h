/*  $Id: setup_os.h,v 1.220 1998/09/29 21:25:56 kimelman Exp $
 *
 *  setup_os.h  - setup global directives to current operating system 
 *                and program environment 
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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
 *  $Log: setup_os.h,v $
 *  Revision 1.220  1998/09/29 21:25:56  kimelman
 *  copyright years fixed
 *
 *  Revision 1.219  1998/08/21 00:29:13  kimelman
 *  fix:SVC_UNREG for direct mode
 *
 *  Revision 1.218  1998/07/30 03:23:37  kimelman
 *  DIRECT_MODE
 *
 */

#ifndef __SETUP_OS_H__
#define __SETUP_OS_H__

#include <gnusql/config.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#include <errno.h>

#define FATAL_EXIT_CODE    1
#define SUCCESS_EXIT_CODE  0

typedef unsigned int       word_t  ;   /* 16 or 32 bits    */

#if SIZEOF_CHAR == 1
  typedef unsigned char     byte_t ; /* 8 bit unsigned   */
  typedef signed   char       i1_t ;
#else
# error "sizeof(char) != 1 -- fatal error "
#endif

#if SIZEOF_SHORT == 2
  typedef          short      i2_t ; /* 16 bit signed    */
  typedef unsigned short      u2_t ; /* 16 bit unsigned  */
#else
#  error "sizeof(short) != 2 -- fatal error "
#endif

#if SIZEOF_INT == 4
  typedef          int       i4_t  ; /* 32 bits signed   */
  typedef unsigned int       u4_t  ; /* 32 bits unsigned */
#elif SIZEOF_LONG == 4
  typedef          long      i4_t  ; /* 32 bits signed   */
  typedef unsigned long      u4_t  ; /* 32 bits unsigned */
#else
#  error "cant quess 4 byte integer"
#endif

#if SIZEOF_LONG == 8
  typedef signed   long       i8_t ; /* 64 bits signed   */
  typedef unsigned long       u8_t ; /* 64 bits unsigned */
#elif SIZEOF_LONG_LONG == 8
  typedef signed   long long  i8_t ; /* 64 bits signed   */
  typedef unsigned long long  u8_t ; /* 64 bits unsigned */
#else
# define NO_INT8
#endif

/* replacements for ancient typedefs */
typedef byte_t    byte ; 
typedef word_t    word ; /* 16 or 32 bits */

/* mnemonic for bit mask operations */
#define SETVL  |=
#define CLRVL  &=~
#define TSTVL  &
#define BITVL(bit)  (01L<<(bit))

#ifndef gmin
#define gmin(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef gmax
#define gmax(a,b) ((a)<(b)?(b):(a))
#endif

#ifdef RPCSTUB_SVC_STYLE
#define RPC_SVC_PROTO(out_t,in_t,proc,vers)  \
    out_t *proc##_##vers##_svc (in_t *in, struct svc_req *rqstp)
#else
#define RPC_SVC_PROTO(out_t,in_t,proc,vers)  \
    out_t *proc##_##vers       (in_t *in, struct svc_req *rqstp)
#endif


#define SVC_UNREG(prog,vers) gss_svc_unregister(prog, vers)


#if defined(HAVE_STRFTIME)
#  define CFTIME(to,max,fmt,tp) strftime(to, max, fmt, localtime(tp))
#elif defined(HAVE_CFTIME)
#  define CFTIME(to,max,fmt,tp) cftime(to, fmt, tp)
#else
#  error "can`t find 'cftime' or 'strftime' to format time output"
#endif

/*
 * if we have not any particular idea about 'struct msg_buf' equivalent in
 * <sys/msg.h> prototypes -- cast out structures to 'void *' to avoid extra
 * warnings
 */

#ifndef MSGBUFP 
#  define MSGBUFP  void*
#endif

/*
 * The following 2 macros help us to use debugger with adm's subprocesses. Using
 * these macros prevents subproceses from 'panic' reaction on debugger signals.
 */

#ifdef HEAVY_IPC_DEBUG

#define __MSGRCV( mid, bufp, sz, typ, fl, err)  \
 { int __msg_rc;\
   PRINTF(("\n%s:%d: [%d/%d] >>>>>> ______\n",__FILE__,__LINE__,mid,typ));\
   do __msg_rc = msgrcv (mid,(MSGBUFP)(bufp),(sz),(typ), (fl));\
   while ( (__msg_rc < 0) && (errno == EINTR )) ;\
   PRINTF(("\n%s:%d: [%d] >>>>>> %d\n",__FILE__,__LINE__,mid,*(int*)(bufp)));\
   if (__msg_rc < 0) {\
       perror (err);\
       exit (1);\
} }

#define __MSGSND( mid, bufp, sz, fl,err)  \
{ int __msg_rc;\
  PRINTF(("\n%s:%d: '%d' >>>>> [%d] \n",__FILE__,__LINE__,*(int*)(bufp),mid));\
  do __msg_rc = msgsnd (mid,(MSGBUFP)(bufp),(sz), fl);\
  while ( (__msg_rc < 0) && (errno == EINTR )) ;\
  PRINTF(("\n%s:%d: =================\n",__FILE__,__LINE__));\
  if (__msg_rc < 0) {\
      perror (err);\
      exit (1);\
} }

#else

#define __MSGRCV( mid, bufp, sz, typ, fl, err)  \
{ int __msg_rc;\
  do __msg_rc = msgrcv (mid,(MSGBUFP)(bufp),(sz),(typ), (fl));\
  while ( (__msg_rc < 0) && (errno == EINTR )) ;\
  if (__msg_rc < 0) { perror (err); exit (1); } \
}

#define __MSGSND( mid, bufp, sz, fl,err)  \
{ int __msg_rc;\
  do __msg_rc = msgsnd (mid,(MSGBUFP)(bufp),(sz), fl);\
  while ( (__msg_rc < 0) && (errno == EINTR )) ;\
  if (__msg_rc < 0) { perror (err); exit (1); } \
}

#endif


#ifdef NOT_DEBUG

#define  MEET_DEBUGGER	

#else

int wait_debugger(void);

#define  MEET_DEBUGGER				 \
{ char **arg_p;					 \
  for (arg_p=argv; *arg_p; arg_p++)		 \
    if (strcmp(*arg_p,"DebugIt")==0)		 \
      {						\
        while(wait_debugger());                	\
        break;					\
      }						\
}   
#endif

#define abort()   fancy_abort(__FILE__,__LINE__)

#ifdef  NOT_DEBUG
#  define PRINTF(x)   
#else
#  define PRINTF(x)   printf x
#endif

/* define macro wrapper if it has not defined yet */
#ifndef __P
# if defined(__STDC__) || defined(__cplusplus)
#   define __P(x) x
# else
#   define __P(x) ()
# endif
#endif

#if HAVE_ATEXIT
#  define  ATEXIT atexit
#elif HAVE_ON_EXIT
#  define  ATEXIT on_exit
#else
#  define  ATEXIT dummy_atexit
#endif

#endif
