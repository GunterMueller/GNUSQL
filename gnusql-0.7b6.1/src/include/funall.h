/*
 *  funall.h  - contains descriptions of functions from engine library.
 *  The following parts describe:
 *      - functions for connection compiler with DB    (catfun.c)
 *      - engine interface library                     (libfunc1.c)
 *      - DB engine interface                          (lib1.c)
 *      - engine interaction support                   (copy.c, initbas.c)
 *      - getting DB statistics for query optimizer    (estlib.c)
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
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
 */

/* $Id: funall.h,v 1.248 1998/09/29 21:25:53 kimelman Exp $ */

/**********************************************************/
/*             engine interface library                   */
/*  functions for connection compiler with DB             */
/*  (catfun)                                              */
/**********************************************************/

#ifndef __FUNALL_H__
#define __FUNALL_H__

#include "setup_os.h"
#include "typeif.h"
#include "type.h"
#include "sql_type.h"

i4_t   ind_read  __P((Indid * indid, i4_t ind_clm_cnt, data_unit_t **ind_cond_du,
                i4_t read_cnt, i4_t *read_col_nums, data_unit_t **read_du,
                Scanid *ext_scanid, char mode));
i4_t   tab_read  __P((Tabid * tabid, i4_t tab_clm_cnt, i4_t max_clm_num,
                i4_t *tab_col_nums, data_unit_t **tab_cond_du,
                i4_t read_cnt, i4_t *read_col_nums, data_unit_t **read_du));
void  db_func_init  __P((void));
i4_t   existsc  __P((char  *autnm));
i4_t   existtb  __P((char *owner,char *tabnm,Tabid *tabid,char *type));
i4_t   existcl  __P((Tabid *tabid,char *colnm,sql_type_t *coltype,i4_t *colnumb ,i4_t *a));
i4_t   tab_cl  __P((Tabid * tabid, i2_t clnm, sql_type_t * coltype, 
              char **cl_name, char **defval, char **defnull,i4_t *a));
i4_t   get_col_stat  __P((i4_t untabid, i2_t clnm, col_stat_info *col_info));
i4_t   put_col_stat  __P((i4_t untabid, i2_t clnm, col_change_info *change_info));
i4_t   tabclnm  __P((Tabid * tabid, i4_t *nnulcolnum));
i4_t   get_nrows  __P((i4_t untabid));
i4_t   put_nrows  __P((i4_t untabid, i4_t nrows));
i4_t   tabpvlg  __P((i4_t untabid, char *user, char *acttype,
               i4_t un, i4_t *ulist,i4_t rn, i4_t *rlist));
Constr_Info *get_constraints  __P((i4_t untabid, i4_t tab_clm_num));
char *get_chconstr  __P((i4_t chconid, i4_t consize));
i4_t   getview  __P((i4_t untabid, char **res_segm));
i4_t   drop_table  __P((i4_t untabid));

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*             engine interface library (part 1)                   */
/*                         (libfunc1)                              */
/*=================================================================*/

i4_t  opentscan  __P(( Tabid *tabid,i4_t *spn,char mode,i4_t nr,i4_t *rlist,
		i4_t nc,cond_buf_t cond,i4_t nm,i4_t *mlist));
i4_t  openiscan  __P((Indid *indid,i4_t *spn, char mode,i4_t nr,i4_t *rlist, i4_t nc,
                      cond_buf_t cond,i4_t nra,cond_buf_t range,i4_t nm,i4_t *mlist));
i4_t  openfscan  __P((Filid *filid,char *mode,i4_t nr,i4_t *rlist,
                      i4_t nc,cond_buf_t cond,i4_t nm,i4_t *mlist));
i4_t findrow  __P((Scanid scanid, data_unit_t **colval));
i4_t read_row  __P((Scanid scanid, i4_t nr, i4_t *rlist, data_unit_t **colval));
i4_t  saposit  __P((i4_t scanid));
i4_t  reposit  __P((i4_t scanid));
i4_t  delrow  __P((i4_t scanid));
i4_t  mod_data  __P((Scanid scanid, i4_t nm, data_unit_t *mod_from,
                     sql_type_t *dt_types, i4_t *mlist));
i4_t  ins_data  __P((Tabid *tabid, i4_t nv, data_unit_t *ins_from,
                     sql_type_t *dt_types));
i4_t  insrow  __P((Tabid *tabid, u2_t *lenval, Colval colval));
i4_t  instid  __P((Scanid scanid, Filid filid));
i4_t  closescan  __P((Scanid scanid));
i4_t  crepview  __P((Tabid *tabid, Segid segid, i4_t colnum,
                     sql_type_t *coltype));
i4_t  creptab  __P((Tabid *tabid, Segid segid, i4_t colnum,
                    i4_t nnulnum, sql_type_t *coltype));
i4_t  crettab  __P((Tabid *tabid, i4_t colnum, i4_t nnulnum, sql_type_t *coltype));
i4_t  crefil  __P((Filid *filid, Tabid *tabid));
i4_t  creind  __P((Indid *indid, Tabid *tabid, char unique,
                   char clust,i4_t ncn,i4_t *cnlist));
i4_t  dropptab  __P((Tabid *tabid));
i4_t  dropttab  __P((Tabid *tabid));
i4_t  dropind  __P((Indid *indid));
i4_t  addcol  __P((Tabid *tabid, i4_t ncn, sql_type_t *coltype));
i4_t  savepoint  __P((void));
i4_t  rollback __P((i4_t spno));
void commit __P((void));
i4_t  sorttab  __P((Tabid * itabid, Tabid * otabid, i4_t ns,
                    i4_t *slist, char order, char fl));
i4_t  sortfil  __P((Filid *ifilid, Filid *ofilid));
i4_t  make_group  __P((Tabid *itabid, Tabid *otabid,i4_t ng,i4_t *glist,
                       char order, i4_t nf,i4_t *flist,char *fl));

/*---------- E N D   P A R T 1 ----------------------*/

/*****************************************************/
/*****************************************************/
/*  ((lib1)  -  DB engine interface (part2)
 */                
/*********************************************************/

i4_t uidnsttab  __P((Tabid * i1tabid, Tabid * i2tabid, Tabid * otabid, i4_t code));
i4_t unsttab  __P((Tabid *i1tabid, Tabid *i2tabid, Tabid *otabid,
                   i4_t ns, i4_t *rlist));
i4_t intsttab  __P((Tabid *i1tabid, Tabid *i2tabid, Tabid *otabid,
                    i4_t ns, i4_t *rlist));
i4_t difsttab  __P((Tabid *i1tabid, Tabid *i2tabid, Tabid *otabid,
                    i4_t ns, i4_t *rlist));
i4_t bldttfil  __P((Tabid *tabid, Filid *filid,i4_t nc, cond_buf_t cond, i4_t *spn));
i4_t bldtifil  __P((Indid *indid, Filid *filid,i4_t nc, cond_buf_t cond,i4_t nra,
                    cond_buf_t range, i4_t *spn));
i4_t bldtffil  __P((Filid *ifilid, Filid *ofilid,i4_t nc,cond_buf_t cond));
i4_t bldtttab  __P((Tabid *itabid, Tabid *otabid,i4_t nr, i4_t *rlist,i4_t nc ,
                    cond_buf_t cond, i4_t *spn));
i4_t bldtitab  __P((Indid *indid, Tabid *otabid,i4_t nr, i4_t *rlist,i4_t nc,
                    cond_buf_t cond,i4_t nra,cond_buf_t range, i4_t *spn));
i4_t bldtftab  __P((Filid *filid, Tabid *otabid,i4_t nr, i4_t *rlist,i4_t nc ,
                    cond_buf_t cond));
i4_t delttab  __P((Tabid *tabid,i4_t nc , cond_buf_t cond, i4_t *spn));
i4_t delitab  __P((Indid *indid, i4_t nc , cond_buf_t cond, i4_t nra,
                   cond_buf_t range, i4_t *spn));
i4_t delftab  __P((Filid *filid, i4_t nc ,cond_buf_t cond, i4_t *spn));
i4_t modttab  __P((Tabid *tabid, i4_t nc,cond_buf_t cond,i4_t nm,i4_t *mlist,
                   u2_t *lenval , Colval colval));
i4_t moditab  __P((Indid *indid,i4_t nc,cond_buf_t cond,i4_t nr,cond_buf_t range,
                   i4_t nm,i4_t *mlist, u2_t * lenval, Colval colval));
i4_t modftab  __P((Filid *filid, i4_t nc,cond_buf_t cond,i4_t nm, i4_t *mlist,
                   u2_t * lenval, Colval colval));
i4_t instttab  __P((Tabid *itabid, Tabid *otabid, i4_t nr, i4_t *rlist,
                    i4_t nc,cond_buf_t cond, i4_t *spn));
i4_t institab  __P((Indid *indid, Tabid *otabid, i4_t nr, i4_t *rlist,i4_t nc,
                    cond_buf_t cond,i4_t nra, cond_buf_t range, i4_t *spn));
i4_t instftab  __P((Filid *filid, Tabid *otabid, i4_t nr, i4_t *rlist,i4_t nc,
                    cond_buf_t cond, i4_t *spn));
i4_t jnsttab  __P((Tabid *i1tabid, Tabid *i2tabid, Tabid *otabid, i4_t ns,
                   i4_t *r1list,i4_t *r2list));
i4_t cnt_ttab  __P((Tabid *tabid,i4_t nc, cond_buf_t cond));
i4_t cnt_itab  __P((Indid *indid,i4_t nc, cond_buf_t cond,
                    i4_t nr,cond_buf_t range));
i4_t cnt_ftab  __P((Filid *filid,i4_t nc, cond_buf_t cond));
i4_t maxminavgitab  __P((Indid * indid, i4_t maxminavg, i4_t nc, cond_buf_t cond,
                         i4_t nr, cond_buf_t range, i4_t code));
i4_t minmaxavgstab  __P((Tabid * tabid, i4_t minmaxavg, i4_t code));
i4_t funci  __P((Indid * indid, i4_t *spn, i4_t nc, cond_buf_t cond,
                 i4_t nr, cond_buf_t range, data_unit_t **colval, i4_t nf, 
                 i4_t *flist, char *fl));
i4_t func  __P((Tabid * tabid, i4_t *spn, i4_t nc, cond_buf_t cond,
                data_unit_t **colval, i4_t nf, i4_t *flist, char *fl));

/**************************************************************
 *
 *  (copy, initbas, newnewcr)  -  engine interaction support
 *              common function
 *
 *---------------------------------------------------------*/
i4_t Copy  __P((void *v_to,void *v_from,i4_t len));
#define t2bto2char(source,dest)  t2bpack(source,dest)
i4_t  create_base __P((void));
i4_t  initbas  __P((void));
TXTREF existind  __P((Tabid *tabid));
#endif

