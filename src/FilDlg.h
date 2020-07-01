#if !defined(AFX_FILDLG_H__F35BF380_E976_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_FILDLG_H__F35BF380_E976_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilDlg dialog

class CFilDlg : public CFileDialog
{
	DECLARE_DYNAMIC(CFilDlg)

public:
	CFilDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

protected:
	//{{AFX_MSG(CFilDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILDLG_H__F35BF380_E976_11D5_BA60_0050BAA7E0E7__INCLUDED_)
