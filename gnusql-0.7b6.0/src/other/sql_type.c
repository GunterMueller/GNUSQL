/*
 *  sql_type.c  - SQL types support library for SQL precompiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of SysTem Prpgramming
 *  This file is written by Michael Kimelman & Konstantin Dyshlevoi
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

/* $Id: sql_type.c,v 1.246 1997/03/31 11:03:16 kml Exp $ */

#include "setup_os.h"
#include "sql.h"
#include "global.h"
#include "type_lib.h"
#include <assert.h>

/*******==================================================*******\
*******            Type conversion functions               *******
\*******==================================================*******/
static struct
{
  char                *SQLtypename;
  dyn_sql_type_code_t  dyn_id;
} typesname[]= {

#define DEF_SQLTYPE(Code,SQLstr,Cstr,Dyn_Id)    {SQLstr,Dyn_Id},
#include "sql_type.def"
  { NULL, 0 }
};

/*******==================================================*******\
*******            Type conversion functions               *******
\*******==================================================*******/

void 
conv_type (char *s, sql_type_t * l, i4_t direct)  /* direct=s->l?1:0 */
{
  sql_type_t tt;
  byte ll = 0;
  char c, c1;
  i4_t len, prec;
  char buffer[20];
  assert (s != NULL);
  assert (l != NULL);
  if (direct)
    {
      sscanf (s, "%[^[]%*c%u%c%u%c", buffer, &len, &c, &prec, &c1);
      tt.len = (i2_t) len;
      tt.prec = (byte) (prec & 0xff);
      if (c != ',' || c1 != ']')
	{
	  fprintf (stderr, "Internal error at %s:%d: "
		   "incorrect type format in string '%s'\n",
		   __FILE__, __LINE__, s);
	  return;
	}
      ll = (byte) SQLType_LAST;
      while (ll--)
	if (0 == strcmp (buffer, typesname[ll].SQLtypename))
	  {
	    tt.code = ll;
	    break;
	  }
      *l = tt;
      conv_type (buffer, l, 0);	/* restore string */
      if (strncmp (buffer, s,strlen(buffer)))
	yyfatal ("Trl.conv_type: Internal error in algorithm");
    }
  else
    {
      tt = *l;
      sprintf (s, "%s[%u,%u]", typesname[tt.code].SQLtypename,
	       (int) tt.len, (int) tt.prec);
    }
}

dyn_sql_type_code_t
get_dyn_sqltype_code(SQLType t)
{
  if (t<=SQLType_0 || t>=SQLType_LAST)
    return 0;
  return typesname[t].dyn_id;
}

SQLType
get_sqltype_code(dyn_sql_type_code_t t)
{
  SQLType i;
  for(i=0;i<SQLType_LAST;i++)
    if(typesname[i].dyn_id ==t)
      return i;
  return SQLType_0;
}

sql_type_t 
pack_type (SQLType tpname, i2_t lenght, i2_t prec)
{
#define LEN1(tl)   (i4_t)(tl*2.4+0.99)
#define LEN(t)     LEN1((sizeof(t)))
#define FATAL(str)   fprintf(STDERR,"%s\n",str)
  sql_type_t tt;
  
  assert (tpname < SQLType_LAST);
  if (prec > 0xff)
    {
      char s[100];
      sprintf(s,"trl.pack_type: precision (%d) more than 256",prec);
      FATAL (s);
      prec = 0xff;
    }
  assert ( (prec & 0xff) == prec );
  
  tt.prec = prec;
  tt.code = tpname;
  tt.len = lenght;
  
  if ( prec > lenght )
    {
      char s[100];
      sprintf(s,"trl.pack_type: precision more than lenght in '%s'",
	      type2str(tt));
      FATAL (s);
      prec = lenght - 1;
    }
  switch (tt.code)
    {
    case SQLType_0:
      tt.len  = 0;
      tt.prec = 0;
      break;
    case SQLType_Char:
    case SQLType_CharVar:
      tt.prec = 0;
      tt.code = SQLType_Char;
      break;
    case SQLType_Short:
      tt.len = LEN (i2_t);
      tt.prec = 0;
      break;
    case SQLType_Int:
      tt.len = LEN (i4_t);
      tt.prec = 0;
      break;
    case SQLType_Long:
      tt.len = LEN (i4_t);
      tt.prec = 0;
      break;
    case SQLType_Real:
      tt.len = LEN1 (3);
      tt.prec = 0;
      break;
    case SQLType_Double:
      tt.len = LEN1 (6);
      tt.prec = 0;
      break;
    case SQLType_Num:
      if ( tt.prec == 0 )
	{
	  if (tt.len <= LEN (i2_t) )
	    {
	      tt.code = SQLType_Short;
	      tt.len  = LEN (i2_t);
	      break;
	    }
	  else if (tt.len <= LEN (i4_t) )
	    {
	      tt.code = SQLType_Long;
	      tt.len  = LEN (i4_t);
	      break;
	    }
	}
    case SQLType_Float:
      tt.prec = 0;
      if (tt.len <= LEN1(3) )
	{
	  tt.code = SQLType_Real;
	  tt.len  = LEN1(3);
	  break;
	}
      else if (tt.len <= LEN1(6) )
	{
	  tt.code = SQLType_Double;
	  tt.len  = LEN1(6);
	  break;
	}
    default:
      /* error should be detected here */
      FATAL("sql_type.pack_type: unacceptable type given ");
      fprintf(STDERR,"-%s ",type2str(tt));
      tt.code = SQLType_Double;
      tt.len  = LEN1(6);
      fprintf(STDERR,"--> %s \n",type2str(tt));
    }
  return tt;
#undef FATAL
#undef LEN
#undef LEN1
}

sql_type_t 
read_type (char *s)
{
  sql_type_t t;
  conv_type (s, &t, 1);
  return t;
}

char *
type2str (sql_type_t t)
{
  static char b[40];
  sql_type_t t2 = t;
  conv_type (b, &t2, 0);
  return b;
}

i4_t
type2long (sql_type_t t)
{
  return *(i4_t*)&t;
}

i4_t
user_to_rpc (gsql_parm *parm, parm_t *rpc_arg)
{
  char *str;
  
  rpc_arg->value.type = parm->type;
  rpc_arg->indicator = (parm->indptr) ? *(parm->indptr) : 0;
  switch (parm->type)
    {
    case T_STR /* SQLType_Char */ :
      str = *(char **)(parm->valptr);
      rpc_arg->value.data_u.Str.Str_len = strlen (str);
      rpc_arg->value.data_u.Str.Str_val = str;
      break;
    case SQLType_Short:
      rpc_arg->value.data_u.Shrt = *((i2_t *)(parm->valptr));
      break;
    case SQLType_Int :
      rpc_arg->value.data_u.Int  = *((i4_t *)(parm->valptr));
      break;
    case SQLType_Long :
      rpc_arg->value.data_u.Lng  = *((i4_t *)(parm->valptr));
      break;
    case SQLType_Real :
      rpc_arg->value.data_u.Flt  = *((float *)(parm->valptr));
      break;
    case SQLType_Double :
      rpc_arg->value.data_u.Dbl  = *((double *)(parm->valptr));
      break;
    default    :
      return -ER_CLNT;
    }
  return 0;
} /* user_to_rpc */

i4_t
rpc_to_user (parm_t *rpc_res, gsql_parm *parm)
{
  i4_t typ, error = 0, size, from_size, to_size;
  
  if (rpc_res->indicator != -1) /* current result is REGULAR_VALUE */
    {
      if (parm->indptr)
	*(parm->indptr) = rpc_res->indicator;
	
      typ = parm->type;
      if (typ == T_STR)
	{
	  from_size = rpc_res->value.data_u.Str.Str_len;
	  to_size = parm->length - 1;
	  size = (to_size > from_size) ? from_size : to_size;
	  bcopy (rpc_res->value.data_u.Str.Str_val,
		 (char *) (parm->valptr), size);
	  ((char *) (parm->valptr))[size] = '\0';
	}
      else
	error = put_dat (&(rpc_res->value.data_u), 0, rpc_res->value.type,
			 REGULAR_VALUE, parm->valptr, 0, typ, NULL);
	
      if (error > 0) /* res_dt-string was shortened */
	if (parm->indptr)
	  *(parm->indptr) = error;
	  
      if (error < 0)
	return error;
    }
  else
    /* current result is NULL_VALUE */
    if (parm->indptr)
      *(parm->indptr) = -1;
    else
      return -ND_INDIC;
  
  return 0;
} /* rpc_to_user */

i4_t 
put_dat (void *from, i4_t from_size, byte from_mask, char from_nl_fl,
	 void *to,   i4_t to_size_,  byte to_mask_,  char *to_nl_fl)

/* Copying of information accordingly "*_mask"          *
 * (if to_mask_==0 => type isn't being changed).        *
 * This function returns value:                         *
 * 0 - if it's all right,                               *
 * < 0 - if error,                                      *
 * > 0 - if result is string and this value = lenght of *
 *       this string. It is greater than res_size       *
 *       (so string was shortened).                     */
{
  i2_t size;
  byte to_mask = (to_mask_) ? to_mask_ : from_mask;
  i4_t to_size = (to_size_) ? to_size_ : from_size;
  
  if (to_nl_fl)
    *to_nl_fl = from_nl_fl;
  
  if (from_nl_fl != REGULAR_VALUE)
    return 0;
  
  if (from_mask == T_STR)
    {
      size = (to_size > from_size) ? from_size : to_size;
      if (to_mask != T_STR)
	return (-ER_1);
      bcopy ((char *) from, (char *) to, size);
      return from_size - size;
    }
  
  /* trere are handled here numbers only */
  if (from_mask == to_mask)
    switch (from_mask)
      {
      case T_SRT:
	*(i2_t *) to = *(i2_t *) from;
	break;
      case T_INT:
	*(i4_t *) to = *(i4_t *) from;
	break;
      case T_LNG:
	*(i4_t *) to = *(i4_t *) from;
	break;
      case T_FLT:
	*(float *) to = *(float *) from;
	break;
      case T_DBL:
	*(double *) to = *(double *) from;
	break;
      default:
	return (-ER_1);		/* ERROR */
      }				/* switch */
  else
    switch (to_mask)
      {

      case T_SRT:
	switch (from_mask)
	  {
	  case T_INT:
	    *(i2_t *) to = (i2_t) (*(i4_t *) from);
	    break;
	  case T_LNG:
	    *(i2_t *) to = (i2_t) (*(i4_t *) from);
	    break;
	  case T_FLT:
	    *(i2_t *) to = (i2_t) (*(float *) from);
	    break;
	  case T_DBL:
	    *(i2_t *) to = (i2_t) (*(double *) from);
	    break;
	  default:
	    return (-ER_1);	/* ERROR */
	  }			/* switch */
	break;

      case T_INT:
	switch (from_mask)
	  {
	  case T_SRT:
	    *(i4_t *) to = (i4_t) (*(i2_t *) from);
	    break;
	  case T_LNG:
	    *(i4_t *) to = (i4_t) (*(i4_t *) from);
	    break;
	  case T_FLT:
	    *(i4_t *) to = (i4_t) (*(float *) from);
	    break;
	  case T_DBL:
	    *(i4_t *) to = (i4_t) (*(double *) from);
	    break;
	  default:
	    return (-ER_1);	/* ERROR */
	  }			/* switch */
	break;

      case T_LNG:
	switch (from_mask)
	  {
	  case T_SRT:
	    *(i4_t *) to = (i4_t) (*(i2_t *) from);
	    break;
	  case T_INT:
	    *(i4_t *) to = (i4_t) (*(i4_t *) from);
	    break;
	  case T_FLT:
	    *(i4_t *) to = (i4_t) (*(float *) from);
	    break;
	  case T_DBL:
	    *(i4_t *) to = (i4_t) (*(double *) from);
	    break;
	  default:
	    return (-ER_1);	/* ERROR */
	  }			/* switch */
	break;

      case T_FLT:
	switch (from_mask)
	  {
	  case T_SRT:
	    *(float *) to = (float) (*(i2_t *) from);
	    break;
	  case T_INT:
	    *(float *) to = (float) (*(i4_t *) from);
	    break;
	  case T_LNG:
	    *(float *) to = (float) (*(i4_t *) from);
	    break;
	  case T_DBL:
	    *(float *) to = (float) (*(double *) from);
	    break;
	  default:
	    return (-ER_1);	/* ERROR */
	  }			/* switch */
	break;

      case T_DBL:
	switch (from_mask)
	  {
	  case T_SRT:
	    *(double *) to = (double) (*(i2_t *) from);
	    break;
	  case T_INT:
	    *(double *) to = (double) (*(i4_t *) from);
	    break;
	  case T_LNG:
	    *(double *) to = (double) (*(i4_t *) from);
	    break;
	  case T_FLT:
	    *(double *) to = (double) (*(float *) from);
	    break;
	  default:
	    return (-ER_1);	/* ERROR */
	  }			/* switch */
	break;
      default:
	return (-ER_1);		/* ERROR */
      }				/* switch,if,if */
  return (0);
}				/* "put" */

int
is_casted(sql_type_t pt1,sql_type_t pt2)
{
  sql_type_t t3, t1=pt1, t2=pt2;
  
  if ( *(i4_t*)&t1 == *(i4_t*)&t2 )
    return 1;
  
  if ( t1.code != t2.code )
    {
      static struct cast { 
	SQLType from,to; 
      } casting[] = { { SQLType_0      ,SQLType_Char   },
		      { SQLType_0      ,SQLType_Short  },
		      { SQLType_Char   ,SQLType_Cstring},
		      { SQLType_Short  ,SQLType_Int    },
		      { SQLType_Int    ,SQLType_Long   },
		      { SQLType_Long   ,SQLType_Real   },
		      { SQLType_Long   ,SQLType_Num    },
		      { SQLType_Num    ,SQLType_Float  },
		      { SQLType_Real   ,SQLType_Double },
		      { SQLType_Double ,SQLType_Float  },
		      { SQLType_Real   ,SQLType_Double }  };
      
      register int i;
      t3 = t2;
      i = sizeof(casting)/sizeof(struct cast);
      while (i--)
	{
	  if (t2.code != casting[i].from)
	    continue;
	  t3.code = casting[i].to;
	  if ( is_casted(t1,t3) )
	    return 1;
	}
      return 0;
    }
  else /* (t1.code == t2.code) */
    {
      switch(t1.code)
	{
	case SQLType_Char: 
	  if (t2.len > t1.len)
	    return 0;
	  break;
	case SQLType_Num:
	  if ( t2.len - t2.prec > t1.len - t1.prec || t2.prec > t1.prec )
	    return 0;
	  break;
	case SQLType_Float:
	  if ( t2.len > t1.len)
	    return 0;
	  break;
	default:
          break;
	}
    }
  return 1;
}
