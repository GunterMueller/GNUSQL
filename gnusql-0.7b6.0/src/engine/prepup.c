/*
 *  prepup.c -  BD initialization
 *              Kernel of GNU SQL-server
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko
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

/* $Id: prepup.c,v 1.245 1997/03/31 03:46:38 kml Exp $ */       

#include "setup_os.h"
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "rnmtp.h"
#include "adfstr.h"
#include "pupsi.h"
#include "totdecl.h"
#include "admdef.h"


#define  adfsize sizeof (struct ADF)

int
main ()
{
  i4_t fdaf;
  struct ADF adf;  
  u2_t BD_seg_scale_size;
  
  setbuf (stdout, NULL);
  
  
  ini_adm_file (ADMFILE);
  
  ini_mj (MJFILE);
  
  ini_lj (LJFILE);

  if ( (fdaf = open (ADMFILE, O_RDWR)) < 0)
    {
      perror ("ADMFILE: open error");
      exit (1);
    }
  if (read (fdaf, (char *)&adf,adfsize) != adfsize)
    {
      perror ("ADMFILE: write error");
      exit (1);
    }   
  BD_seg_scale_size = adf.sizesc;
  close (fdaf);  
  
  ini_BD_seg (SEG1, BD_seg_scale_size);
  
  return 0;
}
