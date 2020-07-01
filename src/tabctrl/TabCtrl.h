// TabCtrl.h : main header file for the TABCTRL application
//

#if !defined(AFX_TABCTRL_H__78B9F966_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_TABCTRL_H__78B9F966_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlApp:
// See TabCtrl.cpp for the implementation of this class
//

class CTabCtrlApp : public CWinApp
{
public:
	VOID FinaliseApp( VOID );
	CTabCtrlApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabCtrlApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTabCtrlApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABCTRL_H__78B9F966_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
