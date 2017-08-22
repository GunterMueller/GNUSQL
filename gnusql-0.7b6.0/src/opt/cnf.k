/*
 * cnf.k  - converting condition to conjunctive normal form
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, 1997, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Andrew Yahin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 *
 */

/* $Id: cnf.k,v 1.245 1997/03/31 03:46:38 kml Exp $ */

i4_t assotiative_operation(TXTREF node);
i4_t the_same_code(TXTREF node);


#(Rule "assotiative_to_one"
  (Op:0 "assotiative_operation" { (Op:1) })
  ""
  { 
    (REMOVE (Op:0))
    (Op:1)
} )

#(Rule "nested_assotiative"
  (Op:0 "the_same_code" { 
    (Op:1:exist_op "the_same_code") 
  } )
  ""
  {
    (DELETE (Op:1) (Op:0))    ;; 
    (Op:0 {
	(DOWN:list (Op:0))
        (DOWN:list (Op:1))
    } )
    (REMOVE (Op:1))
    (Run "nested_assotiative" (Op:0))
  } 
)


#(Rule "CNF"                           ;; conjunction normal form
 (Or:1 { (And:2:exist_op )})           ;; ...t1 || (a1 && a2 ...) || t2...
  ""                                   ;; op1 -> ...t1 || (a1 && a2...) || t2...
 {                                     ;; op2 -> (a1 && a2...)
   ;; ------------------------------------------------------------------
   (DELETE (Op:2) (Op:1) )             ;; op1 -> ...t1 || t2...
   (COPY:3 (Op:1))                     ;; op3 -> copy of (...t1 || t2...)
   (DOWN:4 (Op:2))                     ;; op4 -> a1 
   (DELETE (Op:4) (Op:2) )             ;; op2 -> (a2 && ...)
   ;; let`s check the case op2 -> (a2) ==> op2 -> a2
   (Run:2 "assotiative_to_one" (Op:2)) ;;
   ;; construction of new tree:
   (Run "nested_assotiative"           ;;
      (And {                           ;;
         (Run "CNF"                    ;; (a2||.t1||t2.)&&(a3||...)&&...
            (Op:1 {                    ;; ( a2 && ...) || ...t1 || t2...  
               (Op:2)                  ;; ( a2 && ...)
               (DOWN:list (Op:1))      ;; (...t1 || t2...)
            } )                        ;;
         )                             ;;
         (Run "CNF"                    ;; (a2||.t1||t2.)&&(a3||...)&&...
            (Run "nested_assotiative"  ;;
               (Op:3 {                 ;; a1 || ...t1 || t2...
                  (Op:4)               ;; a1
                  (DOWN:list (Op:3))   ;; (...t1 || t2...)
               } )                     ;;
            )                          ;;
         )                             ;;
      } )                              ;;
   )                                   ;;
 }                                     ;;
)

