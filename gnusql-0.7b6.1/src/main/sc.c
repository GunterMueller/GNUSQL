/*
 *  sc.c  -  Generation C ANSI-style routines with interpretator calls
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
 *  Contact: gss@ispras.ru
 *
 */

/* $Id: sc.c,v 1.250 1998/09/29 21:26:21 kimelman Exp $ */

#include "setup_os.h"
#include "global.h"
#include "type_lib.h"
#include <assert.h>
#include "tassert.h"

static FILE *ofile = NULL;

static char *
get_c_type (i4_t t)
{
  static char *c_type_names[] = {
#define DEF_SQLTYPE(CODE,SQLTN,CTN,DYN_ID) CTN,
#include "sql_type.def"
    NULL
  };
  assert(t > 0 && t < sizeof(c_type_names)/sizeof(char*));
  return c_type_names[t];
}


static void
sc_prolog (char *modname,char *ext,char *host)
{
  fprintf (ofile,
           "/*\n"
           " * %s.%s - SQL statement client stubs\n"
	   " *\n"
           " *       Generated by GNU SQL compiler \n"
           " *       DON'T EDIT THIS FILE       \n"
           " */\n"
           "\n"
           "#include <sql.h>\n"
           "\n"
	   "static struct module mdl = \n"
           "\t{\"%s\",0,0,\"%s\"};\n"
           "\n"
           "\n",
	   sql_prg,ext,
           host,modname);
}/*-----------------------------------------------------------------------*/

static char *
subst_name(i4_t object,i4_t method)
{
  static char buf[1024];
  switch(object) {
  case -2:     sprintf(buf,"_SQL_commit"); break;
  case -1:     sprintf(buf,"_SQL_rollback"); break;
  default:
    sprintf(buf,"SQL__local_o%d_m%d",object,method);
  }
  return buf;
}

static char *
parm_name(i4_t proto,char *par_name,i4_t id)
{
  static char buf[1024];
  
  assert (proto || *par_name);
  
  if (!proto)
    return par_name;
  sprintf(buf,"%s_%d",(*par_name?par_name:"SQL_par"),id);
  return buf;
}

/*
 * creating textual parameters list for given parameter disriptors.
 *  Examples are given for the following case:
 *    SMALLINT a; input parameter   
 *    char b(...) INDICATOR c;  output parameter  
 *    REAL d;     input parameter  
 * list_style: 
 *  MOD_PROTO(0)  -  ANSI prototype for module procedure 
 *    (i2_t a, char *b, i4_t *c, float d)
 *  EMB_PROTO(1)  -  extended prototype with sizeof's and indicators 
 *                   for replacement procedure.
 *    (i2_t a, char *b, i4_t sizeof_b, i4_t *c, float d)
 *  EMB_CALL(2)   -  substitution actual params to call routine defined in
 *                   EMB_PROTO style. if we omitted indicator of parameter
 *                   any in call it replaced by 0(in) or NULL(out).
 *    (a,0,b,sizeof(b),&c,d,0)
 *
 */

typedef enum
{
  MOD_PROTO,
  EMB_PROTO,
  EMB_CALL
} parm_style_t;

static char *
get_param_list (descr_t *in, descr_t *out, parm_style_t list_style)
{
  register i4_t    cont,i;
  static   char   bf[1024];
  i4_t      count = in->descr_t_len + (out?out->descr_t_len:0); 
  
  assert(out || list_style==MOD_PROTO);
  strcpy (bf, "(");
  
#define SP  sprintf
#define BF  bf + strlen(bf)
  for ( cont = i = 0 ;  i < count; i++)
    {
      prep_elem_t *ce;
      i4_t          out_parm = 1;
      assert ( i < in->descr_t_len || out); 
      if ( i < in->descr_t_len )
        {
          ce = & (in->descr_t_val[i]);
          if (!(list_style==MOD_PROTO && ce->nullable==-1))
            out_parm = 0;
        }
      else /* if (out) */
        ce = & (out->descr_t_val[i - in->descr_t_len]);
        
      if (cont)  strcat (bf, ",");
      else       cont = 1;
      if (list_style == MOD_PROTO || list_style == EMB_PROTO )
        { /* if we generate prototype --> product type */
          SP(BF,"%s ",get_c_type (ce->type));
          if (out_parm && ce->type != SQLType_Char)
            strcat (bf, "*");
        }
      else if (out_parm && ce->type != SQLType_Char)
        strcat (bf, "&"); /* if we generate real call --> special processing */
      /*                     for output parameters                           */
      strcat (bf, parm_name (list_style!=EMB_CALL, ce->name,i));
      if (list_style == EMB_PROTO)
        {
          if (out_parm && ce->type == SQLType_Char)
            SP(BF,",i4_t sizeof_%s",
               parm_name (list_style!=EMB_CALL, ce->name,i));
          SP(BF,", i4_t %sind_of_%s",(out_parm?"*":""),
             parm_name (list_style!=EMB_CALL, ce->name,i));
        }
      else if (list_style == EMB_CALL)
        {
          if (out_parm && ce->type == SQLType_Char)
            SP(BF,",sizeof(%s)", parm_name (list_style!=EMB_CALL, ce->name,i));
          strcat(bf,",");
          if(!*ce->ind_name)
            strcat(bf,(out_parm?"NULL":"0"));
          else
            {
              if (out_parm)
                strcat(bf,"&");
              strcat(bf,ce->ind_name);
            }
        }
    }
  if (!cont && (list_style == MOD_PROTO || list_style == EMB_PROTO ))
    strcat (bf, "void");
  strcat (bf, ")");
#undef SP
#undef BF
  return bf;
}

/* 
 * replace original call by call of stub correctly prototyped
 * ANSI C function
 * out_name -> &name and 
 * out "string_name" -> "string_name, sizeof(string_name)"
 */

void
subst_call(call_subst_t *subst, i4_t is_macro)
{
#define Nl      (is_macro?"\\\n\t":"\n\t")
#define NL(f,a,b) fprintf(ofile,"%s" f,Nl,a,b)
  
  NL("%s %s ;",
     subst_name(subst->object,subst->method),
     get_param_list(&(subst->in_sql_parm),&(subst->out_sql_parm),EMB_CALL));
  {
    i4_t i;
    for (i = 0; i < subst->out_sql_parm.descr_t_len; i++ )
      if (strstr (subst->out_sql_parm.descr_t_val[i].name, "SQLCODE") )
        {
          NL ("*%s = SQLCODE; /*%s*/",
              subst->out_sql_parm.descr_t_val[i].name,
              " ---- error codde -----> ");
          break;
        }
  }
  if (*subst->jmp_on_error)
    NL("if (SQLCODE <  0  ) goto %s ;%s",subst->jmp_on_error,"");
  if (*subst->jmp_on_eofscan)
    NL("if (SQLCODE == 100) goto %s ;%s",subst->jmp_on_eofscan,"");
  fprintf(ofile,"\n\n");
  
#undef Nl
#undef NL
}

/*
 * Generate stub to call particular object of module with explicit
 * typed parameter list to give C compiler a chance to detect
 * SQL call parameters type mismatch problems.
 */


static void 
print_routine (descr_t in, descr_t out, i4_t object, i4_t method,i4_t options)
{
  int          pcount;
  int          i;
  
#define Nl        fputs ("\n\t",ofile)
#define L1(f,a)   fprintf (ofile,f,a)
#define L2(f,a,b) fprintf (ofile,f,a,b)
#define NL(s)     { Nl; fputs (s,ofile); }
#define N1(f,a)   { Nl; L1(f,a); }
#define N2(f,a,b) { Nl; L2(f,a,b); }
#define A_       (out_parm || ce->type==SQLType_Char ?' ':'&')
#define A_i      (out_parm                           ?' ':'&')
  
  L2("\n"
     "static void\n"
     "%s %s\n"
     "{",
     subst_name(object,method),
     get_param_list (&in,&out, EMB_PROTO));

  pcount = in.descr_t_len + out.descr_t_len; 

  NL("gsql_parms ldata;");
  if (pcount)
    {
      N1("gsql_parm  p[%d], *e;",pcount);
      Nl;
      NL("ldata.prm     = p;");
      N1("ldata.count   = %d;",pcount);
      N1("ldata.options = %d;",options);
    }
  else
    {
      NL("ldata.count   = 0;");
      NL("ldata.options = 0;");
      NL("ldata.prm     = NULL;");
    }
  N1("ldata.sectnum = %d;",object);
  N1("ldata.command = %d;",method);
  /* filling parameters fields */
  for(i=0;i<pcount;i++)
    {
      int          out_parm = 0;
      prep_elem_t *ce;
      
      if ( i < in.descr_t_len) 
        ce = &(in.descr_t_val[i]);
      else /* if current parameter is output */
        {
          out_parm = 1;
          ce = &(out.descr_t_val[i - in.descr_t_len]);
        }
      
      Nl;
      N1("e = &p[%d];",i);
      N1("e->type   = %d;",ce->type);
      if (ce->type==SQLType_Char && out_parm)
        N1("e->length = sizeof_%s;",parm_name (1, ce->name,i))
      else
        N1("e->length = %d;",ce->length);
      
      N1("e->flags  =  %d;",out_parm);
      N2("e->valptr = (void*) %c %s;"       ,A_i,parm_name (1, ce->name,i));
      N2("e->indptr =         %c ind_of_%s;",A_i,parm_name (1, ce->name,i));
    }
  /* make a finall call to runtime library */
  Nl;
  N1("SQL__runtime (&mdl, &ldata);\n"
     "}/*--- %s ----*/\n"
     "\n",
     subst_name(object,method));
#undef Nl
#undef N1
#undef N2
#undef L1
#undef L2
#undef A_
#undef A_i
}

void
gen_substitution (i4_t subst_to_c,char *host, compiled_t *prg)
{
  call_t            *call;
  compiled_object_t *obj;
  i4_t    st_uid;

  errors = prg->errors;
  /* open output file ----------------------------------------------------*/
  open_file(&ofile,(subst_to_c?"c":"Sc"));
  /* output prolog -------------------------------------------------------*/
  sc_prolog (prg->stored.comp_data_t_u.module,(subst_to_c?"c":"Sc"),host);
  /* output object interface routines ------------------------------------*/
  for (obj = prg->objects; obj; obj = obj->next)
    if (*obj->cursor_name) /* if compiled object is cursor */
      {
        descr_t zero = { 0 , NULL } ;
        /*---- open ----------*/
        print_routine (obj->descr_in, zero          , obj->object, 0,0);
        /*---- fetch ---------*/
        print_routine (zero         , obj->descr_out, obj->object, 1,
                       /* if RO query - cached scan */
                       (*obj->table_name? 0 : 50 )); 
        /*---- close ---------*/
        print_routine (zero         , zero          , obj->object, 2,0);
        /*---- delete --------*/
        if (*obj->table_name)
          print_routine (zero       , zero          , obj->object, 3,0);
      }
    else
      print_routine (obj->descr_in  , obj->descr_out, obj->object, 0,0);
  /* output call substitutions -------------------------------------------*/
  for (st_uid=0,call = prg->calls; call; call = call->next, st_uid++)
    {
      i4_t has_sqlcode_parm = 0;
      /*----- call prolog ----------*/
      if(!subst_to_c) /* if not module */
        fprintf (ofile, "\n\n#define SQL__subst_%d ", st_uid);
      if (!call->subst) /* if there is nothing to replace */
        continue;
      if (*call->subst->proc_name) /* if we compiled module */
        { /* output the procedure title */
          char params[4000];
          assert(subst_to_c);
          strcpy(params,
                 get_param_list(&(call->subst->interface),NULL,MOD_PROTO));
          assert(strlen(params)<sizeof(params));
          has_sqlcode_parm = (NULL!=strstr(params,"SQLCODE"));
          fprintf (ofile,
                   "\n"
                   "void\n"
                   "%s %s {\n",call->subst->proc_name,params);
#ifdef MOD_PROTO_OUTPUT
          fprintf (hfile,"\nvoid %s %s ;\n",
                   call->subst->proc_name,params);
#endif
        }
      /*------- actual call substitution -------------*/
      subst_call(call->subst,!subst_to_c);
      /*------- call epilogue ------------------------*/
      if (*call->subst->proc_name) /* if we compiled module */
        { /* output the procedure end */
          if(has_sqlcode_parm) 
            fprintf (ofile,"\t*SQLCODE_parm = SQLCODE;\n");
          fprintf (ofile,
                   "\treturn;\n"
                   "}\n"
                   "\n");
        }
    }
  /*---------------------  E  N  D  --------------------------------------*/
  fprintf (ofile, "\n/*--------------- End of file ------------------*/\n");
  fclose(ofile);
  return;
}/*-----------------------------------------------------------------------*/
