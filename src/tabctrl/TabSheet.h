#if !defined(AFX_TABSHEET_H__7C9EC680_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_TABSHEET_H__7C9EC680_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTabSheet

class CTabSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CTabSheet)

private:

	DWORD m_dwAppFlag;

// Construction
public:

	// CTabSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	// CTabSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	CTabSheet (CWnd* = NULL);

// Attributes

// Operations

// Implementation
public:
	COutPg m_pOutPg;
	CExpPag m_pExpPg;
	CCompPg m_pCompPg;
	CTabSheet * m_pMainWnd;
	virtual ~CTabSheet();
	CButton & BtnApply () { return *(CButton*) GetDlgItem (ID_APPLY_NOW); }

	// Generated message map functions
protected:

	//{{AFX_MSG(CTabSheet)
	virtual BOOL OnInitDialog();
	virtual void OnCancel ();
	afx_msg void OnOk ();
	afx_msg void OnApply ();
	afx_msg void OnMenuShow ();
	afx_msg void OnDestroy();
   virtual void PostNcDestroy ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABSHEET_H__7C9EC680_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
