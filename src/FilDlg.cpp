// FilDlg.cpp : implementation file
//

#include "dc4wVers.h"
#include "stdafx.h"
#include "dc4w.h"
#include "FilDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilDlg

IMPLEMENT_DYNAMIC(CFilDlg, CFileDialog)

CFilDlg::CFilDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
}


BEGIN_MESSAGE_MAP(CFilDlg, CFileDialog)
	//{{AFX_MSG_MAP(CFilDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

