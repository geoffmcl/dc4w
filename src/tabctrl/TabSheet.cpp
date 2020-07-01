// TabSheet.cpp : implementation file
//

#include "stdafx.h"
//#include "TabCtrl.h"
//#include "TabSheet.h"
extern   CTabCtrlApp theApp;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTabSheet

IMPLEMENT_DYNAMIC(CTabSheet, CPropertySheet)

//	ON_WM_CLOSE ()			// afx_msg void OnClose( );
//	ON_WM_CREATE()
/* *** begin disabled ***************
	ON_COMMAND (IDM_DISABLE, OnMenuDisable)
	ON_COMMAND (IDM_EXIT, OnMenuExit)
	ON_COMMAND (IDM_HISTORY, OnMenuHistory)
	ON_WM_ENDSESSION ()		// afx_msg void OnEndSession( BOOL );
	ON_WM_DESTROY ()		// afx_msg void OnDestroy( );
	ON_MESSAGE (MYMSG_UNINSTALL, OnMsgUninstall)
   ON_COMMAND_RANGE (IDM_MENUCOPY, IDM_MENUCOPYMAX, OnMenuCopy)
   end disabled ************** */

// MFC macro that creates the message dispatch table for the dialog
// BEGIN_MESSAGE_MAP (CClipAidSheet, CPropertySheet)
BEGIN_MESSAGE_MAP (CTabSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CTabSheet)
	ON_BN_CLICKED (IDOK, OnOk)
	ON_BN_CLICKED (ID_APPLY_NOW, OnApply)
	ON_COMMAND (IDM_SHOW, OnMenuShow)
	ON_WM_DESTROY()
	ON_BN_CLICKED (IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP ()

// ClipAidSheet
// Constructor
BOOL  g_bFirstRun = TRUE;
BOOL  g_bShowUp = FALSE;
#define  MMXSHEETS   3
//CPropertySheet   grpropsheet[MMXSHEETS];
//CPropertySheet   someSheet;   // no need to call Construct for this one

UINT rgID[MMXSHEETS] = { IDD_COMPARE, IDD_OUTLINE, IDD_EXPANDED };

//----------------------------------------------------------------------------
//CClipAidSheet::CClipAidSheet (CWnd* pParentWnd) : CPropertySheet ( APPNAME2, pParentWnd)
CTabSheet::CTabSheet( CWnd* pParentWnd )
 : CPropertySheet( APPNAME2, pParentWnd)
{
//   int   i;
//   for (i = 0; i < MMXSHEETS; i++ )
//      grpropsheet[i].Construct(rgID[i]);

   // AFTER setting the defaults for the pages
	// ADD the TABBED pages to the CProperty window
   // ********************************************
   // This method adds a CPropertyPage object to the property sheet.
   // Add pages to the property sheet in the left-to-right order you
   // want them to appear.
   // NOTE WELL: The INDEX_???PAGE must be adjusted to REFLECT this ORDER
   // =========


   AddPage( &m_pCompPg );
   AddPage( &m_pOutPg );
   AddPage( &m_pExpPg );

	// Actually create the dialog box.
	// Override the default styles
   // like to remove the WS_VISIBLE flag
   // or remove the CONTEXT help button
	{

		//DWORD dwStyle = ( DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP |
		DWORD dwStyle = ( DS_MODALFRAME | DS_3DLOOK |
		    DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU );
	//Create( NULL, DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP |
	//	    DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU );

//		if( m_ConfigPage.m_fFirstRun )
		if( g_bFirstRun )
		{
//			dwStyle |= WS_VISIBLE;
//         dwStyle = (DWORD)-1;
		}

      // ************************
		Create( NULL, dwStyle );   // creation of dialog - not visible
      // ************************

	}

	//if( m_ConfigPage.m_fFirstRun )
	if( g_bFirstRun || g_bShowUp )
	{

		PostMessage( WM_COMMAND, IDM_SHOW, 0 );
		//SendMessage( WM_COMMAND, IDM_SHOW, 0 );
		//if( GetActiveIndex() != INDEX_CONFIG )
		//	SetActivePage( INDEX_CONFIG );
		//ShowWindow( SW_SHOW );
		//::SetForegroundWindow( m_hWnd );
		// Add VIDEO bit
		//m_ConfigPage.m_dwConfFlag |= CF_VIDON;
	}

	//m_ConfigPage.m_dwConfFlag |= CF_DNINIT;

	//m_fDirty = fUpdt;	// Possible initial update also

}


CTabSheet::~CTabSheet()
{
}


/////////////////////////////////////////////////////////////////////////////
// CTabSheet message handlers
HWND   g_hTabShtDlg = 0;   // m_hWnd;

BOOL CTabSheet::OnInitDialog() 
{
	// BOOL bResult = CPropertySheet::OnInitDialog();
	// TODO: Add your specialized code here
	// Create my main window dialog class with new op
	// to make it static.
   // as per BOOL CClipAidSheet::OnInitDialog () {
	BOOL	rc;
	HWND	hWnd;
	//NOTIFYICONDATA nid;

	//
	// First, call base class function.  Before calling, set modless
	// flag to false, to fool it into leaving the window size large 
	// enough for the buttons below the property pages.  These are
	// normally, removed in modeless dialogs.
	//
	m_bModeless = FALSE;
	rc = CPropertySheet::OnInitDialog ();
	m_bModeless = TRUE;

	//m_fFlags = 0;
	//m_dwSheetFlag = 0;	// Begin SHEET Flag
	//m_dwBeginSize = 0;	// and the SAVE FILE Size
	//m_sCurrentSave = "";

	// Copy the CClipAidSheet handle to a CConfigPage variable
	// for easier access
	//m_ConfigPage.m_hSheet = m_hWnd;
   g_hTabShtDlg = m_hWnd;

	hWnd = 0;

//#ifndef  NDEBUG
	if( hWnd = ::GetDlgItem( m_hWnd, IDOK ) )
	{
		// ::ShowWindow( hWnd, SW_HIDE );
		// ::EnableWindow( hWnd, FALSE );
	}

	// Hide the cancel button
	if( hWnd = ::GetDlgItem( m_hWnd, IDCANCEL ) )
	{
		ASSERT( hWnd != NULL );
		// ::ShowWindow( hWnd, SW_HIDE );
	}

//#endif   // !NDEBUG

	// Show the 'Apply' button, but change its text to Close.
	// BUT this does not eaxctly describe what happens!
	// 13Mar1998 - Changed to "Hide",
	// which is exactly what happens
	if( hWnd = ::GetDlgItem( m_hWnd, ID_APPLY_NOW ) )
	{
		ASSERT(hWnd != NULL);
		//::SetWindowText( hWnd, "Close" );
		//::SetWindowText( hWnd, "&Hide" );
		::ShowWindow( hWnd, SW_SHOW );
		::EnableWindow( hWnd, TRUE );
	}

   // July 2000 - Indicate UPDATE of Autorun string needed
   // if( m_ConfigPage.m_dwConfFlag & CF_CHGMODULE )	// Change of MODULES indicator

	// Hook from the clipboard viewer chain.
	// m_hwndNextClip = SetClipboardViewer ();

	// Insert taskbar icon.
	// nid.cbSize = sizeof (nid);
	// nid.hWnd = m_hWnd;
	// nid.uID = TBICON;
	// nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	// nid.uCallbackMessage = MYMSG_TBNOTIFY;
	// Put us in the System Tray
	// =========================
	// ::Shell_NotifyIcon( NIM_ADD, &nid );
   // a posted message may have made the window visible
   // +++++++++++++++++++++++++++++++++++++++++++++++++

	return rc;
//}
//	return bResult;
}


//BOOL CTabSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
//{
//	// TODO: Add your specialized code here and/or call the base class
//	
//	return CPropertySheet::OnCommand(wParam, lParam);
//}

#define  IX_COMPARE     0
#define  IX_OUTLINE     1
#define  IX_EXPANDED    2

//void CClipAidSheet::OnMenuShow ()
void CTabSheet::OnMenuShow ()
{
	// Being SHOWN from Right Mouse POPUP context menu
	// so DEFAULT to the CONFIG Page
//	if( GetActiveIndex() != IX_CONFIG )
	//if( GetActiveIndex() != IX_COMPARE )
   {
      // set it to this
		SetActivePage( IX_COMPARE );
   }

	ShowWindow( SW_SHOW );
	// Add VIDEO bit
//	m_ConfigPage.m_dwConfFlag |= CF_VIDON;
//	m_ConfigPage.m_dwConfFlag |= ( CF_VIDON | CF_DNVIDON) ;

//	return;
}

//----------------------------------------------------------------------------
// OnCancel
// Called when esc key is pressed,
// or the [Cancel] button which now shows [Hide]
// Must be overridden for Modeless dialogs
//----------------------------------------------------------------------------
//void CClipAidSheet::OnCancel ()
void CTabSheet::OnCancel ()
{
	// HideSheet();
   //exit(1);
   sprtf( "Exit app. in process"MEOR );

	::DestroyWindow( m_hWnd );
}

void CTabSheet::OnOk ()
{
   sprtf( "OnOk event"MEOR );
   // write results - if any changes - to INI
   // and advise NEW count

   OnCancel();

}
void CTabSheet::OnApply ()
{
   sprtf( "OnApply event"MEOR );
   BtnApply().EnableWindow(FALSE);

   //BtnApply().ShowWindow(SW_HIDE);

}

void CTabSheet::OnDestroy() 
{
	CPropertySheet::OnDestroy();
	
	// TODO: Add your message handler code here
   sprtf( "Done Sheet dest."MEOR );

   // see PostNcDestroy below for memory release of dialog box
}

//----------------------------------------------------------------------------
// PostNCDestroy - Called after window destroyed
//            Must be overridden for Modeless dialogs
//----------------------------------------------------------------------------
//void CClipAidSheet::PostNcDestroy () 
void CTabSheet::PostNcDestroy () 
{
	int i;

	i = 0;
	// Maybe the MAIN Window is NOT VALID at this TIME!!!
	// ==================================================
	if( ( m_hWnd ) &&
		( IsWindow( m_hWnd ) ) )
   {
      // yup, it appears valid
      i++;

   }

	// Since the dialog box is modeless and
	// the dialog object was created 
	// with a new operator, we need to delete ourselves.
	delete this;
   sprtf( "Done Sheet PostNcDestroy."MEOR );

}

