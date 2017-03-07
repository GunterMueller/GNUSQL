/*
 *  destrn.h  -  Internal Transaction structures 
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

#ifndef __destrn_h__
#define __destrn_h__

/* $Id: destrn.h,v 1.248 1998/09/29 21:26:05 kimelman Exp $ */

#include <sql.h>
#include "rnmtp.h"
#include "pupsi.h"
#include "tptrn.h"
#include "fieldtp.h"
#include <stdio.h>

#include "deftr.h"

typedef i4_t COST;
typedef i2_t CPNM;

struct d_r_bd
{
  i4_t relnum;
  u2_t fieldnum;
  u2_t fdfnum;
  u2_t indnum;
};

struct fun_desc_fields   /* descriptor of fields for functions */
{
  struct des_field *df_pnt;
  u2_t f_fn;
  u2_t f_fdf;
};

struct d_r_t
{
  u2_t segnr;
  u2_t pn_r;
  u2_t ind_r;
  struct fun_desc_fields f_df_bt;
  struct d_r_t *drlist;
  u2_t oscnum;
  CPNM cpndr;
  struct ldesind *pid;
  struct d_r_bd desrbd;
};

struct des_index
{
  i4_t unindex;
  u2_t rootpn;
  char kifn;
};

struct ldesind
{
  struct des_field *pdf;
  CPNM cpndi;
  u2_t i_segn;
  struct d_r_t *dri;
  struct ldesind *listind;
  u2_t oscni;
  struct des_index ldi;
};

struct p_head
{
  i4_t csum;
  i4_t idmod;
};

struct page_head
{
  struct p_head ph_ph;
  u2_t lastin;
};

struct des_nseg
{
  u2_t lexnum;
  u2_t mexnum;
  struct des_exns *dextab;
  u2_t mtobnum;
  char **tobtab;
};

struct des_exns
{
  u2_t efpn;
  u2_t funpn;
  u2_t ldfpn;
  u2_t freecntr;
};

struct prtob
{
  unsigned prob:1;		/* relation or filter */
  unsigned prsort:1;
  unsigned prdbl:1;
  unsigned prdrctn:1;
};

struct des_tob
{
  struct prtob prdt;
  u2_t firstpn;
  u2_t lastpn;
  u2_t osctob;
  u2_t free_sz;
};

struct des_trel
{
  struct des_tob tobtr;
  i4_t  row_number;
  struct fun_desc_fields f_df_tt;
  u2_t keysntr;
};

struct des_fltr
{
  struct des_tob tobfl;
  struct d_r_t *pdrtf;
  u2_t selszfl;
  u2_t keysnfl;
};

struct des_tid
{
  u2_t tpn;
  u2_t tindex;
};

struct full_des_tuple   /* full descriptor of a tuple */
{
  u2_t sn_fdt;
  i4_t rn_fdt;
  struct des_tid tid_fdt;
};

struct ldesscan
{
  struct ldesind *pdi;
  u2_t curlpn;
  u2_t offp;
  u2_t offa;
  i4_t sidmod;
  struct des_tid ctidi;
  struct des_tid mtidi;
  char *dpnsc;
  char *dpnsval;
  char *cur_key;
};

struct d_mesc
{
  unsigned obsc:3;		/* scan object */
  unsigned modesc:3;
  unsigned empty:1;
  unsigned prcrt:1;
  unsigned ancrt:1;
  char *pobsc;
  CPNM cpnsc;
  char *pslc;
  u2_t ndc;
  u2_t fnsc;
  u2_t fmnsc;
};

struct d_sc_r
{				/* relation scan descriptor */
  struct d_mesc mescr;
  struct des_tid curtid;
  struct des_tid memtid;
};

struct d_sc_i
{				/* index scan descriptor */
  struct d_mesc mesci;
  struct ldesscan dessc;
};

struct d_sc_f
{				/* filter scan descriptor */
  struct d_mesc mescf;
  u2_t pnf;
  u2_t offf;
  u2_t mpnf;
  u2_t mofff;
};

struct listtob
{
  u2_t prevpn;
  u2_t nextpn;
};

struct p_h_tr
{
  struct listtob listtr;
  u2_t linptr;
};

struct p_h_f
{
  struct listtob listfl;
  u2_t freeoff;
};

struct dmbl_sz
{
  unsigned TAG:1;
  unsigned bls:15;
};

struct dmbl_head
{
  struct dmbl_sz dmblsz;
  struct dmbl_head *nextbl;
  struct dmbl_head *prevbl;
};

struct A
{
  u2_t p_sn;
  u2_t p_pn;
  char *p_shm;
};

struct ind_page
{
  struct p_head ind_ph;
  u2_t ind_nextpn;
  u2_t ind_off;
  u2_t ind_wpage;
};

#endif
