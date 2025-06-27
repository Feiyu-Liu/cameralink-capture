#ifndef _SAPCLASSGUI_H_
#define _SAPCLASSGUI_H_

//	SapClassGui.h : header file
//

#ifndef SAPCONTROLSINTEROP_EXPORT

/////////////////////////////////////////////////////////////////////////////
// Resource definitions

#include "SapClassGuiRes.h"


/////////////////////////////////////////////////////////////////////////////
// Basic classes

#include "SapClassBasic.h"


/////////////////////////////////////////////////////////////////////////////

// General definitions

#if defined SAPCLASSGUI
	#define SAPCLASSGUI_VAR		__declspec(dllexport)
	#define SAPCLASSGUI_CLASS	__declspec(dllexport)
	#define SAPCLASSGUI_FUNC	__declspec(dllexport)
#else
	#define SAPCLASSGUI_VAR		__declspec(dllimport)
	#define SAPCLASSGUI_CLASS	__declspec(dllimport)
	#define SAPCLASSGUI_FUNC	__declspec(dllimport)
#endif

#ifndef _AFXDLL
   #error The project option "General | Microsoft Foundation Classes" must be "Use MFC in a shared DLL"
#endif

#ifdef UNICODE
   #define _tCorSnprintf                     StringCbPrintfW
   #define _tCorStrncpy(dest, src, count)    StringCbCopyW((dest), (count), (src))
   #define _tCorStrncat(str1, str2, count)   StringCbCatW((str1), (count), (str2))
   #define _tCorSplitpath                    _tsplitpath_s
   #define _tCorStricmp                      _tcsicmp

#else
   #define _tCorSnprintf            CorSnprintf
   #define _tCorStrncpy             CorStrncpy
   #define _tCorStrncat             CorStrncat
   #define _tCorSplitpath           CorSplitpath
   #define _tCorStricmp             CorStricmp
#endif

/////////////////////////////////////////////////////////////////////////////
// General functions

void SAPCLASSGUI_FUNC SapClassGuiInitModule();


/////////////////////////////////////////////////////////////////////////////
// Windows and dialogs header files

#include "ImageWnd.h"
#include "ImageViewportWnd.h"
#include "ImageExWnd.h"
#include "AbortDlg.h"
#include "AcqDlg.h"
#include "AScanDlg.h"
#include "BufDlg.h"
#include "GioInputDlg.h"
#include "GioOutputDlg.h"
#include "GioBidirectionalDlg.h"
#include "AcqConfigDlg.h"
#include "CompDlg.h"
#include "ViewDlg.h"
#include "LoadSaveDlg.h"
#include "LScanDlg.h"
#include "FlatFieldDlg.h"
#include "BayerDlg.h" // obsolete
#include "ColorConversionDlg.h"
#include "ImageFilterEditorDlg.h"

#if !COR_WIN64
#include "CamVicDlg.h"
#include "PixProDlg.h"
#endif

#endif

#endif	// _SAPCLASSGUI_H_
