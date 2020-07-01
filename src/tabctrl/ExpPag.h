#if !defined(AFX_EXPPAG_H__7C9EC681_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_EXPPAG_H__7C9EC681_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExpPag.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExpPag dialog

class CExpPag : public CPropertyPage
{
	DECLARE_DYNCREATE(CExpPag)

// Construction
public:
	CExpPag();
	~CExpPag();
	CButton & BtnColours () { return *(CButton*) GetDlgItem (IDC_COLOURS); }

// Dialog Data
	//{{AFX_DATA(CExpPag)
	enum { IDD = IDD_EXPANDED };
	int		m_iDisplayType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CExpPag)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CExpPag)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPPAG_H__7C9EC681_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
