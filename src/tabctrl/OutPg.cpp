// OutPg.cpp : implementation file
//

#include "stdafx.h"
//#include "TabCtrl.h"
//#include "OutPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutPg property page

IMPLEMENT_DYNCREATE(COutPg, CPropertyPage)

COutPg::COutPg() : CPropertyPage(COutPg::IDD)
{
	//{{AFX_DATA_INIT(COutPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   sprtf( "Done OutPg init"MEOR );

}

COutPg::~COutPg()
{
   sprtf( "Done OutPg dest."MEOR );
}

void COutPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COutPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COutPg, CPropertyPage)
	//{{AFX_MSG_MAP(COutPg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutPg message handlers
