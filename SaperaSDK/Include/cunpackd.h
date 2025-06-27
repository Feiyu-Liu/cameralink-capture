//
// Unpacking directive function
//

#include <corenv.h>

#if COR_I960
	#pragma align
#endif

#if COR_WIN32 || COR_NTKERNEL
	#ifdef _MSC_VER
		#pragma pack()
	#endif

	#ifdef __BORLANDC__
      #ifdef __clang__
   		#pragma pack()
      #else
   		#pragma option -a.
      #endif
	#endif
#endif

#if defined(COR_POWERPC) && COR_POWERPC
 		#pragma pack() 		
#endif

#if COR_ATMEL_AT91
	#pragma pack()
#endif

#if COR_CYGWIN
	#pragma pack()
#endif


