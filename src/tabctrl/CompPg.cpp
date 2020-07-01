// CompPg.cpp : implementation file
//

#include "stdafx.h"
//#include "TabCtrl.h"
//#include "CompPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCompPg property page

IMPLEMENT_DYNCREATE(CCompPg, CPropertyPage)

CCompPg::CCompPg() : CPropertyPage(CCompPg::IDD)
{
	//{{AFX_DATA_INIT(CCompPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   // 
   sprtf( "Done CompPg init"MEOR );
   //m_btnIgnoreSp.CheckDlgButton( ignore_blanks );  // IDC_IGNORESP
   //{ szOpt, szBnks, it_Bool, (LPTSTR)&
   //ignore_blanks = !(ignore_blanks);
   //bChgBks = TRUE;   //, (PVOID)IDC_IGNORESP, 0 },
   //m_btnIgnoreSp.SetCheck( ignore_blanks );
}

CCompPg::~CCompPg()
{
   sprtf( "CompPg dest."MEOR );
}

void CCompPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCompPg)
	DDX_Control(pDX, IDC_SKIPQTXT, m_btnSkipTxt);
	DDX_Control(pDX, IDC_SKIPCCOMM, m_btnSkipCPP);
	DDX_Control(pDX, IDC_IGNORETERM, m_btnIgnoreTerm);
	DDX_Control(pDX, IDC_NOCASE, m_btnNoCaSe);
	DDX_Control(pDX, IDC_IGNORESP, m_btnIgnoreSp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCompPg, CPropertyPage)
	//{{AFX_MSG_MAP(CCompPg)
	ON_BN_CLICKED(IDC_NOCASE, OnNocase)
	ON_BN_CLICKED(IDC_IGNORESP, OnIgnoresp)
	ON_BN_CLICKED(IDC_SKIPCCOMM, OnSkipccomm)
	ON_BN_CLICKED(IDC_SKIPQTXT, OnSkipqtxt)
	ON_BN_CLICKED(IDC_IGNORETERM, OnIgnoreterm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompPg message handlers
BOOL CCompPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here - for each CHECK button there is a BOOL
   m_btnIgnoreSp.SetCheck( ignore_blanks );
	m_btnSkipTxt.SetCheck( gbSkipTxt );
	m_btnSkipCPP.SetCheck( gbSkipCPP );
	m_btnIgnoreTerm.SetCheck( gbIgnEOL );
	m_btnNoCaSe.SetCheck( gbIgnCase );
	//m_btnIgnoreSp;
   //TCHAR szCase[] = "IgnoreCase";
   //{ szOpt, szCase, it_Bool, (LPTSTR)&
//#define  gbIgnCase      sFW.fw_bIgnCase
//#define  bChgIgC        sFW.fw_bChgIgC    // (PVOID)IDC_NOCASE, 0 },

//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
//#define  gbSkipCPP      sFW.fw_bSkipCPP
//#define  bChgSCPP       sFW.fw_bChgSCPP
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
//#define  gbSkipTxt      sFW.fw_bSkipTxt
//#define  bChgSTxt       sFW.fw_bChgSTxt
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
//#define  gbIgnEOL       sFW.fw_bIgnEOL
//#define  bChgIEOL       sFW.fw_bChgIEOL
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// TODO: Add control notification handler code here to toggle, set, etc

void CCompPg::OnNocase() 
{
//TCHAR szCase[] = "IgnoreCase";
   //{ szOpt, szCase, it_Bool, (LPTSTR)&
   gbIgnCase = !(gbIgnCase);
   bChgIgC = TRUE;   // (PVOID)IDC_NOCASE, 0 },
}

void CCompPg::OnIgnoresp() 
{
   //{ szOpt, szBnks, it_Bool, (LPTSTR)&
   ignore_blanks = !(ignore_blanks);
   bChgBks = TRUE;   //, (PVOID)IDC_IGNORESP, 0 },
}


void CCompPg::OnSkipccomm() 
{
//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
//   { szOpt, szSkipC, // = "Skip_C/C++_Comments"; BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//   it_Bool, (LPTSTR)&
   gbSkipCPP = !(gbSkipCPP);
   bChgSCPP = TRUE;  // (PVOID)IDC_SKIPCCOMM, 0 },
}

void CCompPg::OnSkipqtxt() 
{
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
//   { szOpt, szSkipT, // "Skip-Quoted-Text"; BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//   it_Bool, (LPTSTR)&
   gbSkipTxt = !(gbSkipTxt);
   bChgSTxt  = TRUE; // (PVOID)IDC_SKIPQTXT, 0 },
}

void CCompPg::OnIgnoreterm() 
{
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
//   { szOpt, szIgnEL, // "Ignore-Line-Termination"; BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11
//   it_Bool, (LPTSTR)&
   gbIgnEOL = !(gbIgnEOL);
   bChgIEOL = TRUE;  // (PVOID)IDC_IGNORETERM, 0 },
}

