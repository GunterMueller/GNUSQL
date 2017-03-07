/*
 *  fdcltrn.h  -  Transaction Functions declarations 
 *                Kernel of GNU SQL-server  
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

#ifndef __fdcltrn_h__
#define __fdcltrn_h__

/* $Id: fdcltrn.h,v 1.258 1998/09/29 21:26:06 kimelman Exp $ */

#include "inpop.h"
#include "f1f2decl.h"
#include "strml.h"
#include "typeif.h"
#include "type_lib.h"
#include <assert.h>

/* addflds.c */
i4_t addflds __P ((struct id_rel * pidrel, i4_t fn, struct des_field * afn));

/* aggravg.c */
struct ans_avg
avgitab __P ((struct id_ind * pidind, u2_t slsz,
              char *sc, u2_t diasz, char *diasc));
struct ans_avg
avgstab __P ((struct id_rel * pidrel));
i4_t fndslc __P ((struct d_r_t *desrel, char *tuple,
                  char *selcon, u2_t slsz, char *cort));
u2_t tstcsel __P ((struct fun_desc_fields * df, u2_t slsz,
		  char *selc, char *c, char **arrpnt, u2_t * arrsz));

/* aggrifn.c */
void minitab __P ((struct ans_next * ans, struct id_ind * pidind,
                   u2_t slsz, char *sc, u2_t diasz, char *diasc));
void maxitab __P ((struct ans_next *ans, struct id_ind *pidind,
                   u2_t slsz, char *sc, u2_t diasz, char *diasc));

i4_t agrfind __P ((data_unit_t **colval, struct id_ind *pidind, u2_t nf, u2_t *mnf,
                   u2_t slsz, char *sc, u2_t diasz, char *diasc, char *flaglist));
void write_aggr_val __P ((data_unit_t **colval, char **agrl, struct des_field *df,
                         u2_t nf, u2_t * mnf, char *flaglist));
float retval __P ((char *val, u2_t type));
void agrcount __P ((char **agrl, char *tuple, struct fun_desc_fields * df,
                    u2_t nf, u2_t * mnf, char *flaglist));
void agr_frm __P ((char *agrl, i4_t flag, char *val, u2_t type));
i4_t write_val __P ((char *mas, char **agrl, struct des_field * df,
                     u2_t nf, u2_t * mnf, char *flaglist));
char *write_average __P ((u2_t type, char *pnt_from, char *pnt_to));

/* aggrsfn.c */
u2_t minstab __P ((struct id_rel * pidrel, char *val));
u2_t maxstab __P ((struct id_rel * pidrel, char *val));
void agrfrel __P ((struct ans_next *ans, struct id_rel *pidrel, u2_t nf,
                   u2_t *mnf, u2_t slsz, char *sc, char *flaglist));

/* bdunion.c */
struct ans_ctob bdunion __P ((struct id_ob * pit1, struct id_ob * pit2));
struct ans_ctob intersctn __P ((struct id_ob * pit1, struct id_ob * pit2));
struct ans_ctob differnc __P ((struct id_ob * pit1, struct id_ob * pit2));
void minsfltr __P ((char * asp, struct des_tob * dt, struct des_tid * tid));
char *getcort __P ((char *asp, u2_t ** ai));
i4_t cmpfv __P ((u2_t type, i4_t d, char *val1, char *val2));

/* blfltr.c */
struct ans_ctob blflrl __P ((struct id_rel * pidrel, u2_t slsz,
                             char *sc, u2_t kn, u2_t * fsrt));
struct ans_ctob blflin __P ((struct id_ind * pidind, u2_t slsz, char *sc,
                             u2_t diasz, char *diasc, u2_t kn, u2_t *fsrt));
struct ans_ctob blflfl __P ((i4_t idfl, u2_t slsz, char *sc, u2_t kn, u2_t * fsrt));

/* cmpkeys.c */
i4_t cmpkeys __P ((u2_t kn, u2_t *afn, struct des_field * df, char *pk1, char *pk2));
i4_t cmp2keys __P ((u2_t type, char *pk1, char *pk2));

/* cntrid.c */
CPNM contir __P ((struct id_rel * idrel, struct d_r_t ** desrel));
CPNM tabcl __P ((struct id_rel * pidrel, u2_t fln, u2_t * fmn));
CPNM cont_id __P ((struct id_ind * pidind, struct d_r_t ** desrel,
                   struct ldesind ** di));
CPNM cont_fir __P ((struct id_rel * pidrel, struct d_r_t ** desrel));

/* cnttab.c */
void cntttab __P ((struct ans_cnt * ans, struct id_rel * pidrel,
                   u2_t condsz, char *cond));
void cntitab __P ((struct ans_cnt * ans, struct id_ind * pidind,
                   u2_t condsz, char *cond, u2_t diasz, char *diasc));
i4_t cntftab __P ((i4_t idfl, u2_t condsz, char *cond));
void sumtmpt __P ((struct ans_next * ans, struct id_rel * pidrel));

/* crind.c */
struct ans_cind crind __P ((struct id_rel * pidrel, i4_t prun, i4_t type,
                            i4_t afsize, u2_t * arfn));
void dipack __P ((struct des_index * di, i4_t disize, char *pnt));

/* crrel.c */
struct ans_cr crrel __P ((i4_t sn, i4_t fn, i4_t fdf, struct des_field * df));
void drbdpack __P ((struct d_r_bd * drbd, char *pnt));
void dfpack __P ((struct des_field * df, u2_t fn, char *pnt));
struct ans_ctob crview __P ((u2_t sn, u2_t fn, struct des_field * df));

/* crtfrm.c */
u2_t cortform __P ((struct fun_desc_fields * df, Colval colval, u2_t *lenval,
                    char *cort, char *buf, u2_t mod_count, u2_t * mfn));

/* delcon.c */
CPNM delcrl __P ((struct id_rel * pidrl, u2_t slsz, char *sc));
i4_t delcin __P ((struct id_ind * pidind, u2_t slsz, char *sc,
                  u2_t diasz, char *diasc));
CPNM delcfl __P ((i4_t idfl, u2_t slsz, char *sc));

/* delind.c */
CPNM delind __P ((struct id_ind * pidind));

/* delrel.c */
CPNM delrel __P ((struct id_rel * pidrel));
struct d_r_t *getrd __P ((struct id_rel *pidrel, struct des_tid *ref_tid,
                          char *a, u2_t * corsize));
u2_t getrc __P ((struct d_r_bd * drbd, char *cort));

/* dltn.c */
i4_t dltn __P ((i4_t scnum));

/* empty_page.c */
void emptypg __P ((u2_t sn, u2_t pn, char type));
u2_t getempt __P ((u2_t sn));

/* fltrel.c */
struct ans_ctob rflrel __P ((struct id_rel * pidrel, u2_t fln,
                             u2_t * fl, u2_t slsz, char *sc));
struct ans_ctob rflind __P ((struct id_ind * pidind, u2_t fln, u2_t * fl,
                             u2_t slsz, char *sc, u2_t diasz, char *diasc));
struct ans_ctob rflflt __P ((u2_t idfl, u2_t fln, u2_t * fl, u2_t slsz, char *sc));

/* ind_ins.c */
i4_t icp_insrtn __P ((struct ldesind * desind, char *key,
                      char *key2, char *inf, i4_t infsz));
i4_t icp_spusk __P ((struct A * pg, i4_t elsz, char *key, char *key2));
i4_t remrep __P ((char *asp, char *key, char *key2, i4_t elsz,
                  char **rbeg, char **rloc, i4_t *offbef));
void icp_remrec __P ((char *beg, char *loc, i4_t sz, char *lastb,
                      i4_t elsz, struct A *pg));
i4_t mlreddi __P ((char *lkey, char *lkey2, char *lnkey, char *lnkey2, u2_t pn));
i4_t modlast __P ((char *key, char *key2, char *nkey, char *nkey2, u2_t pn));
void all_unlock __P ((void));
void upunlock __P ((void));
void downunlock __P ((void));
i4_t lenforce __P ((void));

/* ind_rem.c */
int  icp_rem __P ((struct ldesind * desind, char *key, char *key2, i4_t infsz));
int  kszcal __P ((char *key, u2_t * mfn, struct des_field * ad_f));
int  killind __P ((struct ldesind * desind));

/* ind_scan.c */
i4_t fscan_ind __P ((struct ldesscan * desscn, char *key2,
                     char *inf, i4_t infsz, char modescan));
i4_t scan_ind __P ((struct ldesscan * desscn, char *key2,
                    char *inf, i4_t infsz, char modescan));
char *icp_lookup __P ((struct A * pg, struct ldesind * desind, char *key,
                       char *key2, i4_t infsz, char **agr, char **loc));

/* inscon.c */
CPNM inscrl __P ((struct id_rel *pidrl_in, struct id_rel *pidrl_out,
                  u2_t fln, u2_t *fl, u2_t slsz, char *sc));
i4_t inscin __P ((struct id_ind *pidind, struct id_rel *pidrl_out, u2_t fln,
                  u2_t *fl, u2_t slsz, char *sc, u2_t diasz, char *diasc));
i4_t inscfl __P ((i4_t idfl, struct id_rel *pidrl_out, u2_t fln,
                  u2_t *fl, u2_t slsz, char *sc));

/* insfltr.c */
i4_t insfltr __P ((i4_t scnum, i4_t idfl));

/* insrtn.c */
/*i4_t insrtn __P ((struct id_rel *pidrel, char *cort));*/
i4_t insrtn __P ((struct id_rel *pidrel, u2_t *lenval, Colval colval));

/* join.c */
struct ans_ctob join __P ((struct id_rel *pir1, i4_t mfn1sz, u2_t *mfn1,
                           struct id_rel *pir2, i4_t mfn2sz, u2_t *mfn2));

/* keyfrm.c */
void keyform __P ((struct ldesind *desind, char *mas, char *cort));
char *remval __P ((char *aval, char **a, u2_t type));

/* makegr.c */
struct ans_ctob makegroup __P ((struct id_rel * pidrel, u2_t ng,
                                u2_t * glist, u2_t nf, u2_t * mnf,
                                char *flaglist, char *order));
int tuple_break __P ((char *tuple, char **arrpnt, u2_t * arrsz,
                      struct fun_desc_fields * df));
void agrl_frm __P ((char **agrl, struct des_field * df, u2_t nf,
                    u2_t * mnf, char *flaglist));
void distagr_frm __P ((char **agrl, u2_t nf, char *flaglist));

/* mdfctn.c */
i4_t check_cmod __P ((struct fun_desc_fields *desf, u2_t fmnum,
                      u2_t *mfn, Colval colval, u2_t *lenval));
i4_t mdfctn __P ((i4_t scnum, Colval colval, u2_t *lenval));
i4_t testcmod __P ((struct fun_desc_fields * df, u2_t fmn,
                    u2_t * mfn, char *fml, char **fval));
i4_t mod_spec_flds __P ((i4_t scnum, u2_t fmnum, u2_t *fmn,
                         Colval colval, u2_t *lenval));

/* modcon.c */
i4_t modcrl __P ((struct id_rel * pidrl, u2_t slsz, char *sc,
                  u2_t flsz, u2_t *fl, Colval colval, u2_t *lenval));
CPNM modcin __P ((struct id_ind * pidind, u2_t slsz, char *sc,
                  u2_t diasz, char *diasc, u2_t flsz,
		  u2_t *fl, Colval colval, u2_t *lenval));
i4_t modcfl __P ((i4_t idfl, u2_t slsz, char *sc, u2_t flsz,
                  u2_t *fl, Colval colval, u2_t *lenval));

/* next.c */
int next __P ((i4_t scnum, data_unit_t **colval));
int readrow __P ((i4_t scnum, i4_t fln, u2_t * fl, data_unit_t **colval));

/* obrind.c */
u2_t getrec __P ((struct id_ob *fullrn, u2_t pn, struct A * pg, u2_t * offloc));
i4_t fgetnext __P ((struct ldesscan * desscn, u2_t * pn, u2_t * size, i4_t modescan));
i4_t getnext __P ((struct ldesscan * desscn, u2_t * pn, u2_t * size, i4_t modescan));
void modcur __P ((struct ldesscan * desscn, u2_t size));
void modrec __P ((struct id_ob *fullrn, u2_t pn, i2_t delta));
i4_t insrec __P ((struct ldesind * desind, i4_t rn, u2_t pn, u2_t size));
i4_t delrec __P ((struct ldesind * desind, i4_t rn, u2_t pn));
void crindci __P ((struct ldesind * desind));
i4_t ordindi __P ((struct ldesind * desind, char *key, struct des_tid * tid));
i4_t ordindd __P ((struct ldesind * desind, char *key, struct des_tid * tid));
i4_t ind_tid __P ((struct ldesscan * desscn, struct des_tid * tid, i4_t modescan));
i4_t ind_ftid __P ((struct ldesscan * desscn, struct des_tid * tid, i4_t modescan));

/* opinpg.c */
i4_t testfree __P ((char *asp, u2_t fs, u2_t corsize));
void rempbd __P ((struct A *pg));
void inscort __P ((struct A *pg, u2_t ind, char *cort, u2_t corsize));
void exspind __P ((struct A *pg, u2_t ind, u2_t oldsize,
                   u2_t newsize, char *nc));
void compress __P ((struct A *pg, u2_t ind, u2_t newsize));

/* opscfl.c */
u2_t opscfl __P ((i4_t idfl, i4_t mode, u2_t fn, u2_t * fl,
                  u2_t slsz, char *sc, u2_t fmn, u2_t * fml));

/* opscin.c */
struct ans_opsc opscin __P ((struct id_ind * pidind, i4_t mode, u2_t fln,
                             u2_t * fl, u2_t slsz, char *sc, u2_t diasz,
                             char *diasc, u2_t fmn, u2_t * fml));
i4_t testdsc __P ((struct d_r_t *desrel, u2_t * diasz, char *diasc,
                   u2_t * mfn, u2_t * dscsz));
i4_t dsccal __P ((u2_t fn, char *diasc, u2_t * dscsz));
char *pred_compress __P ((char *diaval, char *lastb, struct des_field * df,
                          unsigned char t));

/* opscrl.c */
struct ans_opsc opscrel __P ((struct id_rel * pidrel, i4_t mode,
                              u2_t fnum, u2_t * fl, u2_t slsz,
			      char *sc, u2_t fmnum, u2_t * fml));
i4_t testcond __P ((struct fun_desc_fields * df, u2_t fnum, u2_t * fl,
                    u2_t * slsz, char *selcon, u2_t fmnum, u2_t * fml));

/* orddel.c */
void orddel __P ((struct full_des_tuple *destuple));

/* ordins.c */
struct des_tid ordins __P ((struct id_rel *pidrel, char *cort,
                            u2_t corsize, char type));

/* ordmod.c */
void ordmod __P ((struct full_des_tuple *destuple, struct des_tid *ref_tid,
                  u2_t oldsize, char *nc, u2_t newsize));
i4_t nordins __P ((struct full_des_tuple *destuple, i4_t type,
                   u2_t oldsize, u2_t newsize, char *nc));
void doindir __P ((struct full_des_tuple *destuple,
                   u2_t oldsize, u2_t newsize, char *nc));

/* proind.c */
u2_t proind __P ((i4_t __P ((*f)) __P (()), struct d_r_t * desrel,
                  u2_t cinum, char *cort, struct des_tid * tid));
u2_t mproind __P ((struct d_r_t * desrel, u2_t cinum, char *cort,
                   char *newc, struct des_tid * tid));

/* rdcort.c */
u2_t readcort __P ((u2_t sn, struct des_tid *tid, char *cort,
                    struct des_tid *ref_tid));
u2_t calsc __P ((u2_t * afi, u2_t * ai));

/* reclj.c */
void wmlj __P ((i4_t type, u2_t size, struct ADBL * cadlj, struct id_rel *pidrel,
		struct des_tid * tid, CPNM cpn));

/* recmj.c */
void recmjform __P ((i4_t type, struct A *pg, u2_t off,
                     u2_t fs, char *af, i2_t shsize));
void begmop __P ((char *asp));
void beg_mop __P ((void));

/* rllbck.c */
i4_t rollback __P ((i4_t cpn));
i4_t roll_back __P ((i4_t cpn));
void rllbck __P ((CPNM cpn, struct ADBL cadlj));

/* rllbfn.c */
void redo_dltn __P ((struct full_des_tuple *destuple, struct d_r_t * desrel,
                     char *a));
void redo_insrtn __P ((struct full_des_tuple *destuple, struct d_r_t * desrel,
                       i2_t n, char *a));
void redo_dind __P ((struct d_r_t * desrel, char *a));
void redo_cind __P ((struct d_r_t * desrel, char *a));
u2_t get_placement __P ((u2_t sn, struct des_tid * tid,
                         struct des_tid * ref_tid));
void delscd __P ((u2_t n, char *a));
void fill_ind __P ((struct d_r_t * desrel, struct ldesind * desind));

struct d_r_t *crtfrd __P ((struct id_rel *pidrel, char *a));
void crt_all_id __P ((struct d_r_t * desrel, char *a));
void dindunpack __P ((struct des_index * di, char *pnt));

/* snlock.c */
CPNM synrd __P ((u2_t sn, char *aval, u2_t size));
CPNM synlock __P ((struct d_r_t *desrel, char *cort));
CPNM synlsc __P ((i4_t type, struct id_rel * idr, char *selcon,
                  u2_t selsize, u2_t fn, u2_t * mfn));
CPNM synind __P ((struct id_ob *fullrn));
CPNM syndmod __P ((struct d_r_t *desrel, u2_t flsz,
                   u2_t *fl, Colval colval, u2_t *lenval));

/* tmpob.c */
struct ans_ctob crtrel __P ((i4_t fn, i4_t fdf, struct des_field * df));
struct ans_ctob crfltr __P ((struct id_rel * pidrel));
struct des_exns * getext (void);
void putext (u2_t *mfpn, i4_t exnum);
struct des_tob *gettob __P ((char *asp, u2_t size, i2_t * n, i4_t type));
i4_t deltob __P ((struct id_ob * pidtob));
i4_t instr __P ((struct des_tob * dt, char *cort, u2_t corsize));
void minstr __P ((char *asp, char *cort, u2_t corsize, struct des_tob * dt));
char *getloc __P ((char *asp, u2_t corsize, struct des_tob * dt));
void getptob __P ((char * asp, struct des_tob * destob));
void deltr __P ((char *asp, u2_t * ai,
                 struct des_tob * destob, u2_t pn));
i4_t frptr __P ((char *asp));
void comptr __P ((char *asp, u2_t * ai, u2_t size));
void frptob __P ((struct des_tob * destob, char *asp, u2_t pn));
struct ans_ctob trsort __P ((struct id_rel * pidrel, u2_t kn,
                             u2_t * mfn, char *drctn, char prdbl));
struct ans_ctob flsort __P ((struct id_ob * pidtob, u2_t kn,
                             u2_t * mfn, char *drctn, char prdbl));

/* trns.c */
void finit __P ((void));
i4_t svpnt __P ((void));
i4_t killtran __P ((void));
i4_t closesc __P ((i4_t scnum));
i4_t mempos __P ((i4_t scnum));
i4_t curpos __P ((i4_t scnum));
void modmes __P ((void));
i4_t uniqnm __P ((void));
CPNM sn_lock __P ((struct id_rel * pidrel, i4_t t, char *lc, i4_t sz));
void sn_unltsp __P ((CPNM cpn));
void srtr_trsort __P ((u2_t * fpn, struct fun_desc_fields * df, u2_t * fsrt,
		       u2_t kn, char prdbl, char *drctn, u2_t * lpn));
void srtr_flsort __P ((u2_t sn, u2_t * fpn, struct fun_desc_fields * df,
                       u2_t * mfn, u2_t kn, char prdbl, char *drctn, u2_t * lpn));
void srtr_tid __P ((struct des_tob *dt));
void LJ_put __P ((i4_t type));
void LJ_GETREC __P ((struct ADBL * pcadlj));
void MJ_PUTBL __P ((void));
void read_tmp_page __P ((u2_t pn, char *buff));
void write_tmp_page __P ((u2_t pn, char *buff));
char *getpg __P ((struct A * ppage, u2_t sn, u2_t pn, i4_t type));
char *getwl __P ((struct A * ppage, u2_t sn, u2_t pn));
char *getnew __P ((struct A * ppage, u2_t sn, u2_t pn));
void putpg __P ((struct A * ppage, i4_t type));
void putwul __P ((struct A * ppage, i4_t type));
void BUF_unlock __P ((u2_t sn, u2_t lnum, u2_t * mpn));
i4_t BUF_lockpage __P ((u2_t sn, u2_t pn, i4_t type));
i4_t BUF_enforce __P ((u2_t sn, u2_t pn));
void BUF_endop __P ((void));
void ans_adm __P ((void));
void error __P ((char *s));


#endif
