/* cmpftn.h */
#ifndef _CMPDECL_H_
#define _CMPDECL_H_

#include "fieldtp.h"
#include "rnmtp.h"
#include "totdecl.h"

i4_t f1b __P((char *a1, char *b1, u2_t n1, u2_t n2));
i4_t f2b __P((char *a1, char *b1, u2_t n1, u2_t n2));
i4_t f4b __P((char *a1, char *b1, u2_t n1, u2_t n2));
char *ftint __P((i4_t at, char *a, char **a1, char **a2, i4_t n));
char *ftch __P((i4_t at, char *a, char **a1, char **a2, u2_t *n1, u2_t *n2));
i4_t chcmp __P((char *a, char *b, u2_t n1, u2_t n2));
i4_t ch_cmp __P((char *a, char *b));
i4_t ffloat __P((char *a, char *b, u2_t n1, u2_t n2));
i4_t f_float __P((char *a, char *b));
i4_t digcmp __P((char *a, char *b, u2_t n1, u2_t n2));
i4_t flcmp __P((char *a1, char *b1, u2_t n1, u2_t n2));
i4_t fl_cmp __P((char *a1, char *b1));
i4_t cmpval __P ((char *val1, char *val2, u2_t type));

i4_t ss1  __P((char **scal, i4_t stsc));
#define ss1(scalpp,stsc)  (( (stsc)%2 ? **(scalpp) : *((*(scalpp))--) >> 4) & 0xf)
/*
  #define ss2(scalpp,stscp) \
(((*(stscp))%2 ? **(scalpp) : (*stscp += 2,*(--(*(scalpp))) >> 4)) & 0xf)
*/
u2_t get_length __P((char *val, u2_t type));
char *proval __P((char *aval, u2_t type));

#define get_length(p,t) \
((t)==T1B?sizeof(i1_t):				\
  ((t)==T2B?sizeof(i2_t):				\
    ((t)==TFLOAT || (t)==T4B ? sizeof(i4_t):		\
      ((t)==TFL  || (t)==TCH ? sizeof(i2_t) + t2bunpack (p): \
      (printf("%s:%d: unexpected type code encountered '%d'\n",__FILE__,__LINE__,(t)),\
      exit(1),0)))))

#define proval(p,t) ( (char*)(p) + get_length(p,t)) 

#endif
