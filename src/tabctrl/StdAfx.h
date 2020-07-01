// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__78B9F96A_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_STDAFX_H__78B9F96A_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TCVers.h"  // establish VERSION

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "TabCtrl.h"
#include "TabCtrlDlg.h"
#include "TCWork.h"  // shared global work data structure
// Pages
#include "CompPg.h"
#include "OutPg.h"
#include "ExpPag.h"
// on SHEET
#include "TabSheet.h"
#include "TCLog.h"

// include double linked list support
#include "..\dc4wList.h"
#include "TCIni.h"   // get the INI read/write

// remember changes here will NOT trigger a RE-BUILD
// appears as a 'missing' idetifier ...
#include "resource.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__78B9F96A_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
