// TabCtrl.cpp : Defines the class behaviors for the application.
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
// CTabCtrlApp

BEGIN_MESSAGE_MAP(CTabCtrlApp, CWinApp)
	//{{AFX_MSG_MAP(CTabCtrlApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG

// 	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlApp construction

CTabCtrlApp::CTabCtrlApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTabCtrlApp object

CTabCtrlApp theApp;

HINSTANCE   ghInst = 0;
BOOL        g_bUseModeless = TRUE;

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlApp initialization

BOOL CTabCtrlApp::InitInstance()
{
   HWND  hWnd;
   int   nResponse = -1;
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

   // my initialization for this TAB control
   ghInst = m_hInstance;
	// See if we already are running.
	// ==============================
	if( hWnd = FindWindow( NULL, APPNAME2 ) )
	{
		// ::SendMessage( hWnd, WM_COMMAND, IDM_SHOW, 0 );
		::ShowWindow( hWnd, SW_SHOW );
		::SetForegroundWindow( hWnd );
		// Play Sound
		//PlaySound( (LPCSTR)IDR_WAVE1,
		//	AfxGetResourceHandle(),
		//	(SND_RESOURCE | SND_ASYNC | SND_NOWAIT) );
		AfxMessageBox( "presently only ONE instance allowed!",
			(MB_OK | MB_ICONINFORMATION) );
		return FALSE;
	}

	if( !AfxOleInit() )
	{
		// Play Sound
		//PlaySound( (LPCSTR)IDR_WAVE1,
		//	AfxGetResourceHandle(),
		//	(SND_RESOURCE | SND_ASYNC | SND_NOWAIT) );
		AfxMessageBox( "failed to initialize OLE libraries!",
			(MB_OK | MB_ICONINFORMATION) );
		return FALSE;
	}

   sprtf( "Doing app init."MEOR );

   InitFixedWork();     // put in sensible values before INI read

   if( g_bUseModeless )
   {
	   // =====================================================
	   m_pMainWnd = new CTabSheet;  // generate a NEW sheet - we MUST also delete it
      // =====================================================
      if( !m_pMainWnd )
      {
		   AfxMessageBox( "failed to create MAIN window!",
			   (MB_OK | MB_ICONINFORMATION) );
		   return FALSE;
      }

      // some more to do AFTER window creation
      FinaliseApp(); // like reset previous size / position
   }
   else
   {
      CTabCtrlDlg dlg;
      m_pMainWnd = &dlg;
      nResponse = dlg.DoModal();
      if (nResponse == IDOK)
	   {
		   // TODO: Place code here to handle when the dialog is
		   //  dismissed with OK
	   }
	   else if (nResponse == IDCANCEL)
	   {
		   // TODO: Place code here to handle when the dialog is
		   //  dismissed with Cancel
	   }

   	// Since the dialog has been closed, return FALSE so that we exit the
	   //  application, rather than start the application's message pump.
	   return FALSE;

   }

   return TRUE;   // to start message PUMP

}

//Example
//This is the example from the CWnd::CalcWindowRect method
// Uses CalcWindowRect to determine size for new CFrameWnd
// based on the size of the current view. The end result is a
// top level frame window of the same size as CMyView's frame.
/* =============================
void CMyView::OnMyCreateframe() 
{
  CFrameWnd* pFrameWnd = new CFrameWnd;
  CRect myRect;
  GetClientRect(myRect);
  pFrameWnd->Create(NULL, "My Frame");
  pFrameWnd->CalcWindowRect(&myRect, CWnd::adjustBorder);
  pFrameWnd->MoveWindow(0, 0, myRect.Width(), myRect.Height());
  pFrameWnd->ShowWindow(SW_SHOW);
}
   ============= */
// example end


int CTabCtrlApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
   // LPTSTR   g_lpini = "D:\\Gtools\\Tools\\dc4w\\TabCtrl\\TEMPI2.INI";
   INT      i;

   sprtf( "Doing exit instance."MEOR );

   i = SetChgAll( TRUE );
   //WriteINI( g_lpini, FALSE );
   WriteTCINI();
   i = SetChgAll( i );
	
	return CWinApp::ExitInstance();
}

BOOL  g_bAddIcon = FALSE;

VOID CTabCtrlApp::FinaliseApp(VOID)
{
   RECT  rc;
   INT  scnsx,scnsy;
   INT  sx,sy;
   INT   x,y;

	// Set main Window ICON.
	// =====================
   if( g_bAddIcon )
   {
#ifdef	NDEBUG
      // load the blue 'C' icon
	   m_pMainWnd->SetIcon( theApp.LoadIcon( IDI_ICON1 ), TRUE );
#else	// !NDEBUG = DEBUG
      // load a blue 'D' icon
	   m_pMainWnd->SetIcon( theApp.LoadIcon( IDI_ICON2 ), TRUE );
#endif	// NDEBUG y/n

   }
	// Establish a DEFAULT screen position in bottom right
	// ===================================================
	// What is the DIFFERENCE between these two???
	GetWindowRect( m_pMainWnd->m_hWnd, &rc );
	m_pMainWnd->GetWindowRect( &rc );

   // get screen WIDTH and HEIGHT
	scnsx = GetSystemMetrics( SM_CXSCREEN );
	scnsy = GetSystemMetrics( SM_CYSCREEN );
   sx = scnsx;
   sy = scnsy;

   // default to TOP, LEFT if we are too big for the screen
   x = y = 0;
	if( sx > (( rc.right - rc.left ) + 20) )
		x = sx - (( rc.right - rc.left ) + 20);

	if( sy > (( rc.bottom - rc.top ) + 20) )
		y = sy - (( rc.bottom - rc.top ) + 20);

	sx = x;
	sy = y;

	// Now get the previous position (if any) for the dialog
	//x = GetProfileInt( SECT_WIND, PRO_XPOS, sx );
	//y = GetProfileInt( SECT_WIND, PRO_YPOS, sy );

	x = min( x, sx );
	y = min( y, sy );

   //{ szWin, szSz,   it_Rect,   (LPTSTR)&grcSize,        &bChgSz,  (PVOID)&bSzValid, 0 },
   if( bSzValid )
   {
      //RECT  rc2 = *grcSize;
      RECT  rc2 = grcSize;
      INT   dx  = 0;

      if(( rc2.right > rc2.left ) &&
         ( ( rc2.right - rc2.left ) > rc.right ) )
      {
         dx = ( ( rc2.right - rc2.left ) - rc.right );
      }

      if( ( rc2.left + rc.right ) < scnsx )
         x = rc2.left + dx;   // center us if possible - else parents left

   }

	m_pMainWnd->SetWindowPos( NULL,
		x, y,
		0, 0,
		SWP_NOSIZE | SWP_NOZORDER );
   sprtf( "Set x,y pos to %d,%d"MEOR, x, y );

	// Set POSITION,
	// and HIDE, since we are a task bar ICON!
	// =======================================
	//m_pMainWnd->SetWindowPos( NULL,
	//	x, y,
	//	0, 0,
	//	SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOZORDER );
   //sprtf( "Set x,y pos to %d,%d"MEOR, x, y );

	// force paint msg
	// ==========================
//   m_pMainWnd->Invalidate(TRUE);
   // i do this through an IDM_SHOW sent when the window was created
   // and thus the first page display gets correctly built.
	// m_pMainWnd->UpdateWindow ();
	// ==========================

	// Any actions post create
	// =======================
	//if(	m_pDlgWnd->m_ConfigPage.m_fFirstRun ) 	{
//			m_pDlgWnd->m_ConfigPage.WriteAutoRun( TRUE, FALSE );
	// Set DONE InitInstance
//	m_pDlgWnd->m_ConfigPage.m_dwConfFlag |= CF_DNFIRST;
	// ==========================

}
