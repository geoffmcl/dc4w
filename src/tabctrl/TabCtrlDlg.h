// TabCtrlDlg.h : header file
//

#if !defined(AFX_TABCTRLDLG_H__78B9F968_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
#define AFX_TABCTRLDLG_H__78B9F968_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlDlg dialog

class CTabCtrlDlg : public CDialog
{
// Construction
public:
	CTabCtrlDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTabCtrlDlg)
	enum { IDD = IDD_TABCTRL_DIALOG };
	CTabCtrl	m_tabCtrl;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabCtrlDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTabCtrlDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABCTRLDLG_H__78B9F968_E7D2_11D5_BA60_0050BAA7E0E7__INCLUDED_)
