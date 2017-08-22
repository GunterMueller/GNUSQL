/*
 *  bufdefs.c  - This file contains structures describing objects of the
 *               pool of buffers
 *               Kernel of GNU SQL-server. Buffer
 *
 *  This file is a part of GNU SQL Server
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

#ifndef __bufdefs_h__
#define __bufdefs_h__

/* $Id: bufdefs.h,v 1.246 1997/08/24 16:28:05 kml Exp $ */


/*****************************************************************************
                                  TYPES

                              RELATED STRUCTURES

***************************** page descriptor *******************************/

struct PAGE
{
  struct PAGE *p_next;
  struct PAGE *p_prev;		/* ring of pages with the same page address */
  u2_t p_seg;
  u2_t p_page;			/* full page number */
  u2_t p_status;		/* status of page: 0 or number of weak locks */
  u2_t p_ltype;		/* lock type */
  struct WAIT *p_queue;		/* last in queue */
  struct BUFF *p_buf;
};

/****************************** waiting lock ********************************/

struct WAIT
{
  struct WAIT *w_next;		/* next in queue */
  i4_t w_conn;			/* connection to user */
  u2_t w_type;			/* type of lock */
  u2_t w_tact;			/* tact when lock came in */
  u2_t w_prget;		/* get page or not */
};

/*************************** buffer descriptor ******************************/

struct des_seg
{				/* segment identifier */
  i4_t keyseg;
  i4_t idseg;
};
struct BUFF
{
  struct BUFF *b_next;
  struct BUFF *b_prev;		/* priority ring */
  i4_t b_status;			/* status and number of users */
  struct PAGE *b_page;		/* page descriptor */
  struct des_seg *b_seg;	/* segment identifier */
  i4_t b_micro;			/* reference to microlog */
  char b_prmod;
};

/*****************************************************************************

                                 CONSTANTS
*/
#define NO 0
#define YES 1			/* logical */

#define BUF_MOD  1		/* modifyed buffer */
#define BUF_NMOD 0		/* nonmodifyed buffer */

#define NO_SEGS 0		/* absense of segments */

#define NULL_CONN 0		/* absense of connection */
#define NULL_MICRO 0		/* absense of reference to microlog */

#if 0  /* has already declared in inpop.h */
 #define BACKUP -1		/* backup indicator */
#endif


#define HASHSIZE 256


#define PRIORITIES 3		/* there are 3 priorities of buffers */

#define EMP 0
#define FREE 1
#define LOCKED 2
#define USED 3			/* priorities or types of buffers */

/*****************************************************************************

                                   MACROS
*/
#define WORD_BD 1
#define wd_bd(x) ((x+wodr_bd-1)&~(word_bd-1))

/***************************** the end **************************************/

#endif
