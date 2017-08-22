/*
 * $Id: tassert.h,v 1.245 1997/03/31 03:46:38 kml Exp $ 
 */


#undef TASSERT
#undef TASSERt

#ifdef NDEBUG

  #define TASSERT(__ignore,n)	 

#else	/* #ifdef NDEBUG	*/

  #include "trl.h"

  #ifdef __STDC__

    #define TASSERt(_EX,n) if(!(_EX))\
      trl_err("Assertation (" #_EX ") failed in",__FILE__,__LINE__,n)
  #else

    #define TASSERt(_EX,n) if(!(_EX))\
     (trl_err("Assertation ( _EX ) failed in",__FILE__,__LINE__,n)

  #endif /*  #ifdef __STDC__  */
  
  #define TASSERT(_EX,n)  TASSERt(_EX,Ptree(n))

#endif  /*  #ifdef NDEBUG  */
