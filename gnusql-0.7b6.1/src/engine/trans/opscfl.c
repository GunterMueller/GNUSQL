/*
 *  opscfl.c  -  Open scanning by a filter
 *               Kernel of GNU SQL-server 
 *
 *  This file is a part of GNU SQL Server
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

/* $Id: opscfl.c,v 1.250 1998/09/29 21:25:39 kimelman Exp $ */

#include "fdcltrn.h"
#include "destrn.h"
#include "strml.h"

extern i2_t maxscan;
extern char **scptab;
extern struct des_nseg desnseg;

u2_t
opscfl (i4_t idfl, i4_t mode, u2_t fn, u2_t * fl,
        u2_t slsz, char *sc, u2_t fmn, u2_t * fml)
{
  struct des_fltr *desfl;
  struct d_r_t *desrel;
  struct d_sc_f *scfl;
  i2_t n;

  if ((u2_t) idfl > desnseg.mtobnum)
    return (-ER_NIOB);
  desfl = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfl == NULL)
    return (-ER_NIOB);    
  if (((struct prtob *) desfl)->prob != FLTR)
    return (-ER_NIOB);
  desrel = desfl->pdrtf;
  if (testcond (&desrel->f_df_bt, fn, fl, &slsz, sc, fmn, fml) != OK)
    return (-ER_NCF);
  scfl = (struct d_sc_f *) lusc (&n, scfsize, (char *) desfl, SCF,
                                 mode, fn, fl, sc, slsz, fmn, fml, 0);
  scfl->pnf = desfl->tobfl.firstpn;
  scfl->offf = phfsize;
  scfl->mpnf = (u2_t) ~ 0;
  scfl->mofff = 0;
  return (n);
}
