/*
 * $Id: tests.h,v 1.1 1998/01/19 06:14:49 kml Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, Free Software Foundation, Inc
 * Developed at Institute of System Programming of Russian Academy of Science
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 */
/* common defenitions for tests */

#define PRINT_S(str, str1) if (str[0])                                \
                             fprintf(stderr,str1 "='%s' ",str);       \
                           else                                       \
	 		     fprintf(stderr,str1 "=NULL ")
     
#define PRINT_IS(str, str1) { fprintf(stderr,str1 " = ");             \
                              if (str##_ind < 0)                      \
				fprintf(stderr, "NULL_VALUE ");       \
			      else				      \
                                fprintf(stderr, "'%s' ",str); }

#define PRINT_D(num, num_str) if (num != -153)                        \
                                fprintf(stderr,num_str "='%d' ",num); \
                              else 		     		      \
                                fprintf(stderr,num_str "=NULL ")

#define PRINT_ID(num, num_str) { fprintf(stderr,num_str " = ");       \
                                 if (num##_ind < 0)                   \
			           fprintf(stderr, "NULL_VALUE ");    \
			         else				      \
                                   fprintf(stderr, "'%d' ",num); }

#define PRINT_IDI(num, numid, num_str) { fprintf(stderr,num_str " = ");    \
                                         if (numid < 0)                    \
			                   fprintf(stderr, "NULL_VALUE "); \
			                 else				   \
                                           fprintf(stderr, "'%d' ",num); }

#define PRINT_END          fprintf(stderr,"\n\n")
