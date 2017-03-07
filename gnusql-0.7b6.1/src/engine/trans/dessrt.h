/* dessrt.h - Sorter descriptors structures
 *            Kernel of GNU SQL-server. Sorter   
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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

/* $Id: dessrt.h,v 1.247 1998/09/29 21:25:27 kimelman Exp $ */

#include "destrn.h"

#define PINIT 16
#define STEKSZ 1024
#define MINIT  9
#define YES    1
#define NO     0

#define pntsize  sizeof(char *)

struct el_tree {
  char *pket;             /* key pointer */
  struct el_tree *lsr;    /* louser pointer */
  struct el_tree *first;
  struct el_tree *fe;
  struct el_tree *fi;
};

struct des_sort {
  struct des_field *s_df;
  u2_t  s_kn;
  u2_t *s_mfn;
  char  *s_drctn;
  char   s_prdbl;
};
