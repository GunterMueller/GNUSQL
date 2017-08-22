/*
 *  insrtn.c  - Insertion operation
 *              Kernel of GNU SQL-server 
 *
 * $Id: insrtn.c,v 1.252 1998/01/20 05:14:55 kml Exp $
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

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"
#include "typeif.h"

extern struct des_nseg desnseg;
extern struct ADBL adlj;
extern char *pbuflj;
extern i4_t ljmsize;
extern i4_t ljrsize;

static u2_t
tuple_frm (struct fun_desc_fields *desf, u2_t * lenval,
               Colval colval, char *tuple)
{
  i4_t kt = 0;
  char *t;
  struct des_field *df;
  u2_t fn, size, type, fnum, fdf, corsize;

  t = tuple;
  fdf = desf->f_fdf;
  fnum = desf->f_fn;
  for (kt = 0, fn = fdf, *t++ = CORT, *t = 0; fn < fnum; kt++, fn++)
    {
      if (kt == 7)
	{
	  kt = 0;
	  t++;
	  *t = 0;
	}
      if (colval[fn] != NULL)
	*t |= BITVL(kt);
    }
  *t |= EOSC;
  t++;
  df = desf->df_pnt;
  for (fn = 0; fn < fnum; df++, fn++)
    {
      if (colval[fn] != NULL)
	{
	  size = lenval[fn];
	  if (size > df->field_size)
	    size = df->field_size;
	  if ((type = df->field_type) == TCH || type == TFL)
	    {
	      t2bpack (size, t);
	      t += size2b;
	    }
          bcopy (colval[fn], t, size);
          t += size;
	}
      else if (fn < fdf)
	return (0);
    }
  corsize = t - tuple;
  if (corsize > BD_PAGESIZE - phsize)
    return (0);
  return (corsize);
}

int
insrtn (struct id_rel *pidrel, u2_t *lenval, Colval colval)
{
  char *tuple;
  struct fun_desc_fields *desf;
  u2_t sn, corsize;
  /*  u2_t scsize = 1;*/

  /*
  for (tuple = cort; (*tuple & EOSC) == 0; tuple++)
    {
      scsize += 1;
        if (scsize > BD_PAGESIZE - phsize)
	  return (-ER_NCF);
          }
          */
  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      struct des_trel *destrel;
      destrel = (struct des_trel *) * (desnseg.tobtab + pidrel->urn.obnum);
      if (destrel->tobtr.prdt.prob != TREL)
	return (-ER_NIOB);
      desf = &destrel->f_df_tt;
      if ((corsize = tuple_frm (desf, lenval, colval, pbuflj)) == 0)
	return (-ER_NCF);
      if (corsize < MIN_TUPLE_LENGTH)
        corsize = MIN_TUPLE_LENGTH;
      return (instr ((struct des_tob *) destrel, pbuflj, corsize));
    }
  else
    {
      struct ADBL last_adlj;
      struct d_r_t *desrel;
      i4_t n, ni;
      struct des_tid tid;
      CPNM cpn;
      
      if ((cpn = cont_fir (pidrel, &desrel)) != OK)
	return (cpn);
      desf = &desrel->f_df_bt;
      tuple = pbuflj + ljmsize;
      if ((corsize = tuple_frm (desf, lenval, colval, tuple)) == 0)
	return (-ER_NCF);
      if ((cpn = synlock (desrel, tuple)) != 0)
	return (cpn);
      modmes ();
      last_adlj = adlj;
      if (corsize < MIN_TUPLE_LENGTH)
        corsize = MIN_TUPLE_LENGTH;
      tid = ordins (pidrel, tuple, corsize, 'w');
      n = desrel->desrbd.indnum;
      ni = proind (ordindi, desrel, n, tuple, &tid);
      if (ni < n)
	{
          struct full_des_tuple dtuple;
	  wmlj (RLBLJ, ljrsize, &last_adlj, pidrel, &tid, 0);
	  proind (ordindd, desrel, ni, tuple, &tid);
          dtuple.sn_fdt = sn;
          dtuple.rn_fdt = pidrel->urn.obnum;
          dtuple.tid_fdt = tid;
	  orddel (&dtuple);
          BUF_endop ();
	  return (-ER_NU);
	}
      BUF_endop ();
      return (OK);
    }
}
/*
  -----------------------------------------------------
  for 'ins_data' variant 1

  u2_t lenval[nv];
  void *colval[nv];
  TpSet *Place;

  INITBUF;
  Place = pointbuf;
  for (i = 0; i < nv; i++)
    if (ins_from[i].dat.nl_fl == REGULAR_VALUE)
      {
        if (dt_from->type.code == T_STR)
          {
            lenval[i] = dt_from->type.len;
            colval[i] = STR_PTR (&(dt_from->dat));
          }
        else
        {
          if (to_type)
            {
            Place = pointbuf;
            lenval[i] = sizeof_sqltype (*to_type);
            err = put_dat (&(dt_from->dat), 0, dt_from->type.code, REGULAR_VALUE,
                           Place, 0, to_type->code, NULL);
            colval[i] = pointbuf;
            pointbuf += sizeof (TpSet);
            if (err < 0)
            return err;
            }
            else
              colval[i] = &(dt_from->dat);
          }
      }
    else
      colval[i] = NULL;
  
  stat = insrtn (&pidrel, lenval, colval);
  -----------------------------------------------------

   -----------------------------------------------------
  for 'ins_data' variant 2

  u2_t lenval[nv];
  void *colval[nv];

  for (i = 0; i < nv; i++)
    if (ins_from[i].dat.nl_fl == REGULAR_VALUE)
      {
        if ((err = DU_to_colval (ins_from + i, lenval + i, colval[i], dt_types + i)) < 0)
          return err;
      }
    else
      colval[i] = NULL;
  
  stat = data_insrtn (&pidrel, ins_from);
        -----------------------------------------------------
        
static i4_t
DU_to_colval (data_unit_t *dt_from, u2_t *lenval, void *colval, sql_type_t *to_type)*/
     /* returns 0 if O'K and < 0 if error */
     /* if to_type == NULL => type from dt_from won't be changed */
/*{	
  i4_t err;
  
  if (dt_from->type.code == T_STR)
    {
      *lenval = dt_from->type.len;
      colval = STR_PTR (&(dt_from->dat));
    }
  else
    {
      *lenval = (to_type) ? sizeof_sqltype (*to_type) : sizeof_sqltype (dt_from->type);
      err = put_dat (&(dt_from->dat), 0, dt_from->type.code, REGULAR_VALUE,
                     colval, 0, (to_type) ? to_type->code : dt_from->type.code, NULL);
      if (err < 0)
	return err;
    }
  return 0;
}
  -----------------------------------------------------

  for 'insrow'
  
  stat = insrtn (&pidrel, lenval, colval);
  */
