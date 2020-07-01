#if !defined(AFX_OUTPG_H__7C9EC683_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_OUTPG_H__7C9EC683_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COutPg dialog

class COutPg : public CPropertyPage
{
	DECLARE_DYNCREATE(COutPg)

// Construction
public:
	COutPg();
	~COutPg();

// Dialog Data
	//{{AFX_DATA(COutPg)
	enum { IDD = IDD_OUTLINE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COutPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COutPg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTPG_H__7C9EC683_E847_11D5_BA60_0050BAA7E0E7__INCLUDED_)
