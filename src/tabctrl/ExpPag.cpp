// ExpPag.cpp : implementation file
//

#include "stdafx.h"
//#include "TabCtrl.h"
//#include "ExpPag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpPag property page

IMPLEMENT_DYNCREATE(CExpPag, CPropertyPage)

CExpPag::CExpPag() : CPropertyPage(CExpPag::IDD)
{
	//{{AFX_DATA_INIT(CExpPag)
	m_iDisplayType = -1;
	//}}AFX_DATA_INIT

   sprtf( "ExpPg constructor"MEOR );
}

CExpPag::~CExpPag()
{
   sprtf( "ExpPg destructor"MEOR );
}

void CExpPag::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpPag)
	DDX_Radio(pDX, IDC_RADIO1, m_iDisplayType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpPag, CPropertyPage)
	//{{AFX_MSG_MAP(CExpPag)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpPag message handlers
DWORD    g_iDisplayType = 0;

BOOL CExpPag::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	CheckRadioButton( IDC_RADIO1, IDC_RADIO4, (g_iDisplayType + IDC_RADIO1) );

   sprtf( "Done OnInitDialog for Expanded page."MEOR );

	BtnColours().EnableWindow(FALSE);   // { return *(CButton*) GetDlgItem (IDC_COLOURS); }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
