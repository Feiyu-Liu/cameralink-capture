//
// Coreco log manager definitions. 
//

#ifndef _CORLOGID_H_
#define _CORLOGID_H_

#include <corenv.h>
#include "cpackd.h"


#if !COR_NTKERNEL
typedef struct COR_PACK _TIME_FIELDS
{
    short Year;
    short Month;
    short Day;
    short Hour;
    short Minute;
    short Second;
    short Milliseconds;
    short Weekday;
} TIME_FIELDS;
#endif

#if COR_LINUX

  #if defined(__KERNEL__)
    #include "linux/types.h"
  #else
    #include "stdint.h"
  #endif

#define CORLOG_MAXLOGSIZE	255
#define CORLOG_MAXFILESIZE 255

typedef struct COR_PACK 
{
	int32_t	tv_sec;	/* Seconds		 */
	uint32_t	tv_usec;	/* Microseconds */
} TD_TIMEVAL, *PTD_TIMEVAL;

typedef struct COR_PACK
{
	void        *owner;
	TD_TIMEVAL  time;
	uint32_t    unused[2]; /* legacy pad (for now) */
	uint32_t    logtype;
	char        log[ CORLOG_MAXLOGSIZE + 1];
	char        file[ CORLOG_MAXFILESIZE + 1];
	uint32_t    line;
} CORLOG, *PCORLOG;

#else

#define CORLOG_MAXLOGSIZE	256
#define CORLOG_MAXFILESIZE 256

typedef struct COR_PACK
{
	PVOID          owner;
	TIME_FIELDS   	time;
	unsigned long 	logtype;
	char 			log[ CORLOG_MAXLOGSIZE + 1];
	char 			file[ CORLOG_MAXFILESIZE + 1];
	unsigned long 	line;
} CORLOG, *PCORLOG;

typedef struct COR_PACK
{
	VOID *POINTER_32 owner;
	TIME_FIELDS   	time;
	unsigned long 	logtype;
	char 			log[ CORLOG_MAXLOGSIZE + 1];
	char 			file[ CORLOG_MAXFILESIZE + 1];
	unsigned long 	line;
} CORLOG_32, *PCORLOG_32;
#endif

#define CORLOG_TYPE_MAX			4

#define CORLOG_TYPEID_ERR	0	//	ERRORS			
#define CORLOG_TYPEID_FAT	1	//	FATAL ERRORS	
#define CORLOG_TYPEID_WRN	2	//	WARNINGS			
#define CORLOG_TYPEID_INF	3	//	INFORMATIONS	

#include "cunpackd.h"

#endif   // _CORLOGID_H_
