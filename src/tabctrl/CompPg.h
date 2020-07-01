#if !defined(AFX_COMPPG_H__7C9EC682_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_COMPPG_H__7C9EC682_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CompPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCompPg dialog

class CCompPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CCompPg)

// Construction
public:
	CCompPg();
	~CCompPg();

// Dialog Data
	//{{AFX_DATA(CCompPg)
	enum { IDD = IDD_COMPARE };
	CButton	m_btnSkipTxt;
	CButton	m_btnSkipCPP;
	CButton	m_btnIgnoreTerm;
	CButton	m_btnNoCaSe;
	CButton	m_btnIgnoreSp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCompPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCompPg)
	afx_msg void OnNocase();
	afx_msg void OnIgnoresp();
	afx_msg void OnSkipccomm();
	afx_msg void OnSkipqtxt();
	afx_msg void OnIgnoreterm();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPPG_H__7C9EC682_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
