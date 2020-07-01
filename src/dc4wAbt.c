

// dc4wAbt.c
// this is public domain software - praise me, if ok, just don't blame me!

#include	"dc4w.h"
#include "dc4wcomp.h"

// VER_DATE    "28 March, 2007"  // and added the following 'static'
static const char sszDate[] = __DATE__;
static const char sszTime[] = __TIME__;
// *********************************

extern VOID  show_help( VOID ); // FIX20060917 - add Alt+? brief help dialog - show_help()

extern	TCHAR	g_szIni[];	// INI file
// static const char sszDate[];  // = __DATE__;
// static const char sszTime[];  // = __TIME__;
extern   TCHAR gszTmpOut[];

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : abt_WM_INITDIALOG
// Return type: VOID 
// Argument   : HWND hDlg
// Description: Initialise IDD_DLGABOUT resource
//              
///////////////////////////////////////////////////////////////////////////////
VOID  abt_WM_INITDIALOG( HWND hDlg )
{
   static TCHAR szComp[] = MY_COMP_STG;
   LPTSTR   lpb = GetNxtBuf();  //&gszTmpBuf[0];
   PLE   ph;

   sprintf( lpb, "Version: %d.%d.%d.%d ", _MajVersion, _MinVersion, _SubVersion, _MakeNumber );
   strcat(lpb, "Compiled on ");
   strcat(lpb, sszDate);
   strcat(lpb, " at ");
   strcat(lpb, sszTime);
   ConditionText(lpb);
   SetDlgItemText(hDlg, IDD_VERSION, lpb);

   strcpy(lpb, "CWD: ");
   _getcwd( EndBuf(lpb), 256 );
   SetDlgItemText(hDlg, IDC_LAB_CWD, lpb);

   strcpy(lpb, "RT: ");
   GetModuleFileName( NULL, EndBuf(lpb), 256 );
   SetDlgItemText(hDlg, IDC_LAB_RT, lpb);

   sprintf(lpb, "INI: %s", &g_szIni[0] );
   SetDlgItemText(hDlg, IDC_INIFILE, lpb);

   sprintf(lpb, "LOG: %s", &gszTmpOut[0] );
   SetDlgItemText(hDlg, IDC_LOGFILE, lpb);

   sprintf(lpb, "COMPILER: %s", szComp );
   SetDlgItemText(hDlg, IDC_COMPILER, lpb);

   sprintf(lpb, "CMD:[%s]", g_szCmd);
   ph = &gsXDirsList;
   if( !IsListEmpty(ph) ) {
      strcat(lpb,MEOR"Excl.DIR[");
      Add_Excl_List(lpb, ph, " ");
      strcat(lpb,"]");
   }
   ph = &gsXFileList;
   if( !IsListEmpty(ph) ) {
      strcat(lpb,MEOR"Excl.FILES[");
      Add_Excl_List(lpb, ph, " ");
      strcat(lpb,"]");
   }
   SetDlgItemText(hDlg, IDC_EDIT1, lpb);

   CenterDialog( hDlg, hwndClient );
}

INT_PTR  abt_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD    wCmd = GET_WM_COMMAND_ID(wParam, lParam);
   switch( wCmd )
   {
   case WM_CLOSE:
   case IDOK:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;
   case IDC_BUTTON1:
      show_help(); // put up a BREIF HELP dialog
      break;
   }

   return iRet;
}

/***************************************************************************
 * Function: AboutBox
 *
 * Purpose: From menu IDM_ABOUT using IDD_DLGABOUT resource
 *
 * Standard processing for About box. Typically, the dialog box
 * procedure should return TRUE if it processed the message, 
 * and FALSE if it did not. If the dialog box procedure returns FALSE,
 * the dialog manager performs the default dialog operation in
 * response to the message.
 *
 */
INT_PTR CALLBACK AboutBox(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;   
   switch( uMsg )
   {

   case WM_INITDIALOG:
      abt_WM_INITDIALOG( hDlg );
      iRet = TRUE;
      break;

   case WM_COMMAND:
      iRet = abt_WM_COMMAND( hDlg, wParam, lParam );
      break;
   }
   return iRet;
}


VOID  Do_IDM_ABOUT( HWND hWnd, PVIEW view )
{
   DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_DLGABOUT),   // "About",
      hWnd,
      AboutBox,
      (LPARAM) view );
}

// eof - dc4wAbt.c

