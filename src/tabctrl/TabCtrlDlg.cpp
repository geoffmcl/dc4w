// TabCtrlDlg.cpp : implementation file
//

#include "stdafx.h"
//#include "TabCtrl.h"
//#include "TabCtrlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlDlg dialog

CTabCtrlDlg::CTabCtrlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTabCtrlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTabCtrlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTabCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTabCtrlDlg)
	DDX_Control(pDX, IDC_TAB1, m_tabCtrl);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTabCtrlDlg, CDialog)
	//{{AFX_MSG_MAP(CTabCtrlDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_BN_CLICKED(ID_CONTEXT_HELP, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlDlg message handlers

BOOL CTabCtrlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTabCtrlDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTabCtrlDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTabCtrlDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CTabCtrlDlg::OnContextHelp() 
{
	// TODO: Add your control notification handler code here
	int i;

	CWinApp* pApp = AfxGetApp();
	CWnd* pWnd = GetTopLevelParent();

	i = 0;

	ASSERT_VALID(pApp);
	ASSERT(pApp->m_pszHelpFilePath != NULL);
	sprtf("Help: pszHelpFile = [%s]."MEOR,
      pApp->m_pszHelpFilePath );

	// finally, run the Windows Help engine
//	if (!::WinHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData))
//		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);

//	CDialog::WinHelp(dwData, nCmd);

	i = 1;

	
}
