
// module: dc4wDlgcf.c
// Copy Files Dialog
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"

extern BOOL dir_isvaliddir(LPSTR path); // from scandir.c
extern VOID  MyCommonDlgInit2( HWND hDlg ); // from dc4wDlg.c
extern BOOL  g_bFileCopy;  // set the do-it-quick flag - from dc4w.c
#ifdef ADD_ZIP_SUPPORT
extern INT  WriteZIPList(PTHREADARGS pta, DWORD dwt_NOT_USED ); // from dc4wZip.c // #ifdef ADD_ZIP_SUPPORT

#endif // #ifdef ADD_ZIP_SUPPORT

extern VOID  Post_WM_COMMAND( HWND hDlg, UINT wCmd, LPARAM lParam, PCFDLGSTR pcfds,
                      DWORD dwCaller ); // from dc4wDlg.c


// ======================
// BEGIN COPYFILES DIALOG

VOID  cf_ToggleSet2( HWND hDlg, BOOL flg )
{
   // when DELETE is ENABLED, all OTHERS are DISABLED
   EnableWindow( GetDlgItem(hDlg, IDD_IDENTICAL), flg );
   EnableWindow( GetDlgItem(hDlg, IDD_DIFFER),    flg );
   EnableWindow( GetDlgItem(hDlg, IDD_LEFT),      flg );

   EnableWindow( GetDlgItem(hDlg, IDC_CH_SEL),    flg );
}

VOID  cf_ToggleSet( HWND hDlg, BOOL flg )
{
   // when SINGLE copy ENABLED, all OTHER are DISABLED
   EnableWindow( GetDlgItem(hDlg, IDD_IDENTICAL), flg );
   EnableWindow( GetDlgItem(hDlg, IDD_DIFFER),    flg );
   EnableWindow( GetDlgItem(hDlg, IDD_LEFT),      flg );

   EnableWindow( GetDlgItem(hDlg, IDD_RIGHT),     flg ); // INCLUDE_RIGHTONLY (***DELETE***)
   EnableWindow( GetDlgItem(hDlg, IDC_CHECK1),    flg ); // REVIEW_LIST
}

VOID  cf_Enable( HWND hDlg, PCFDLGSTR pcfds )
{
   cf_ToggleSet( hDlg, TRUE );
}
VOID  cf_Disable( HWND hDlg, PCFDLGSTR pcfds )
{
   cf_ToggleSet( hDlg, FALSE );
}

VOID  cf_Enable2( HWND hDlg, PCFDLGSTR pcfds )
{
   cf_ToggleSet2( hDlg, TRUE );
}
VOID  cf_Disable2( HWND hDlg, PCFDLGSTR pcfds )
{
   cf_ToggleSet2( hDlg, FALSE );
}

//extern   BOOL  bDlgSingle;

BOOL  cf_WARN( HWND hDlg, LPTSTR lpf )
{
   BOOL  bret = FALSE;
   LPTSTR   lpb = &g_szBuf1[0];
   if( dir_isvalidfile( lpf ) ) {  // if this is a VALID file
      strcpy( lpb, "*** IS FILE! ***" );
   } else if( dir_isvaliddir( lpf ) ) {
      strcpy( lpb, "== Is Valid ==" );
      bret = TRUE;
   } else {
      strcpy( lpb, "*** NOT VALID! ***");
   }

   if( IsDlgButtonChecked(hDlg, IDD_RIGHT) == BST_CHECKED )
      strcat( lpb, " ** DELETE IS ON **" );  // INCLUDE_RIGHTONLY (***DELETE***)

   SetDlgItemText( hDlg, IDC_LABWARN, lpb );

   return bret;
}

VOID  cf_SetDeleteStr( HWND hDlg, PCFDLGSTR pcfds, LPTSTR lpft )
{
   LPTSTR   lpb2 = &g_szBuf2[0];
   PDWORD   pdwr = &pcfds->dwRite;     // total in RIGHT tree
   // string for IDD_RIGHT // INCLUDE_RIGHTONLY (***DELETE***)
   sprintf(lpb2, "DELETE Files only in [%s] (%d)",
         lpft,
         *pdwr );
   SetDlgItemText(hDlg, IDD_RIGHT, lpb2); // set // INCLUDE_RIGHTONLY (***DELETE***)
}

VOID  cf_SetRight( HWND hDlg, PCFDLGSTR pcfds )
{
   LPTSTR      lpft = &pcfds->cf_szRight[0]; // to here (or delete here if right only)
   cf_SetDeleteStr( hDlg, pcfds, lpft );
}

VOID  cf_SetLeftRight( HWND hDlg, PCFDLGSTR pcfds )
{
   LPTSTR      lpb  = &g_szBuf1[0];
//   LPTSTR      lpb2 = &g_szBuf2[0];
   DWORD       dwi  = pcfds->dwCpyOpts;
   // establish DEFAULT wording           COPY
   LPTSTR      lpff = &pcfds->cf_szLeft[0];  // from here
   LPTSTR      lpft = &pcfds->cf_szRight[0]; // to here (or delete here if right only)
   PDWORD      pdwl = &pcfds->dwLeft;     // total in LEFT tree
//   PDWORD      pdwr = &pcfds->dwRite;     // total in RIGHT tree

#ifdef   USEOLDCOPY2
   if( dwi & COPY_FROMRIGHT )
   {
      // switch pointers         COPY
      lpft = &pcfds->cf_szLeft[0];  // to here
      lpff = &pcfds->cf_szRight[0]; // from here
      pdwr = &pcfds->dwLeft;     // LEFT total
      pdwl = &pcfds->dwRite;     // RIGHT total
   }
#endif   // #ifdef   USEOLDCOPY2
   // DEFAULT = COPY FROM LEFT TO RIGHT

   // string for IDD_LEFT
   sprintf(lpb, LoadRcString(IDS_FILES_ONLY), (LPSTR) lpff);
   sprintf(EndBuf(lpb), " (%d)", *pdwl );
   SetDlgItemText(hDlg, IDD_LEFT,  lpb );

   // string for IDD_RIGHT // INCLUDE_RIGHTONLY (***DELETE***)
   //sprintf(lpb2, "DELETE Files only in [%s] (%d)",
   //      lpft,
   //      *pdwr );
   //SetDlgItemText(hDlg, IDD_RIGHT, lpb2); // set // INCLUDE_RIGHTONLY (***DELETE***)
   cf_SetRight( hDlg, pcfds );

   if( ( pcfds->bHadInit ) &&
       ( !pcfds->bHadUpd ) )
   {
      // ********************************************************
      SetDlgItemText( hDlg, IDD_DIR1, lpft );   // SET DESTINATION
      // ********************************************************
      cf_WARN( hDlg, lpft );   // and set WARNING indication
      pcfds->bHadUpd = FALSE;
   }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : cf_WM_INITDIALOG
// Return type: BOOL 
// Arguments  : HWND hDlg
//            : LPARAM lParam
// Description: Initialise the COPY FILES dialog box.
//              
/* ===============================
Extract from RC file
IDD_COPYFILES2 DIALOGEX 0, 0, 321, 202
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "UPDATE FOLDERS"
FONT 8, "MS Sans Serif", 0, 0, 0x1
BEGIN
    CTEXT           "DESTINATION:",IDD_LAB1,7,18,58,12,SS_CENTERIMAGE | 
                    WS_BORDER
    EDITTEXT        IDD_DIR1,69,18,245,12,ES_AUTOHSCROLL | NOT WS_TABSTOP
    LTEXT           "Include:",IDD_LAB2,7,89,32,8
    CONTROL         "&Identical files",IDD_IDENTICAL,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,42,102,69,10
    CONTROL         "&Newer files",IDD_DIFFER,"Button",BS_AUTOCHECKBOX | 
                    WS_TABSTOP,42,114,70,10
    CONTROL         "&Older files",IDD_DIFFER2,"Button",BS_AUTOCHECKBOX | 
                    WS_TABSTOP,42,125,70,10
    CONTROL         "Files only in &left tree",IDD_LEFT,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,42,137,272,10
    PUSHBUTTON      "Cancel",IDCANCEL,195,181,40,14
    DEFPUSHBUTTON   "OK",IDOK,274,181,40,14
    LTEXT           "Left",IDC_LABLEFT,35,53,47,8
    LTEXT           "Right",IDC_LABRITE,87,53,47,8
    LTEXT           "Ident",IDC_LABIDENT,133,103,50,8
    LTEXT           "Newer",IDC_LABDIFF,133,115,50,8
    LTEXT           "Older",IDC_LABDIFF2,133,126,171,8
    LTEXT           "",IDC_LABUNK,11,164,95,8
    LTEXT           "Aim is to aid in synchronisation of two directories.",
                    IDC_STATIC,7,7,154,8
    LTEXT           "",IDC_LABCWD,7,162,307,8
    CONTROL         "Review and modify list.",IDC_CHECK1,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,19,72,90,10
    CONTROL         "Selected File ONLY",IDC_CH_SEL,"Button",BS_AUTOCHECKBOX | 
                    WS_TABSTOP,136,90,178,10
    LTEXT           "",IDC_LABWARN,169,7,73,8
    CONTROL         "DELETE files only in right tree",IDD_RIGHT,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,42,148,272,10
    CONTROL         "Backup to Recycle Bin",IDC_BACKUP,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,112,72,90,10
    CONTROL         "Each",IDC_RADIO1,"Button",BS_AUTORADIOBUTTON,195,111,33,
                    10
    CONTROL         "Older",IDC_RADIO2,"Button",BS_AUTORADIOBUTTON,230,111,
                    33,10
    CONTROL         "None",IDC_RADIO3,"Button",BS_AUTORADIOBUTTON,267,111,33,
                    10
    GROUPBOX        "Verify",IDC_STATIC,190,103,117,21
    CONTROL         "Warn on DELETE!",IDC_CHK_WARN,"Button",BS_AUTOCHECKBOX | 
                    WS_TABSTOP,205,72,90,10
    LTEXT           "Total %d",IDC_LISTTOTAL,245,7,69,10
    LTEXT           "Will Action %d on OK",IDC_OKSTATUS,41,90,89,8
    CTEXT           "Source:",IDC_LAB2,7,34,58,12,SS_CENTERIMAGE | NOT 
                    WS_GROUP,WS_EX_DLGMODALFRAME
    EDITTEXT        IDC_EDSOURCE,69,34,245,12,ES_AUTOHSCROLL | ES_READONLY | 
                    NOT WS_BORDER | NOT WS_TABSTOP,WS_EX_DLGMODALFRAME
    LTEXT           "Stats:",IDC_STATIC,7,53,22,8
    PUSHBUTTON      "Zip Em Up!",IDC_BUTZIPIT,7,182,48,13
END
   =============================== */
BOOL  cf_WM_INITDIALOG( HWND hDlg, LPARAM lParam )
{
   LPTSTR      lpb   = &g_szBuf1[0];
   LPTSTR      lpb2  = &g_szBuf2[0];
   PCFDLGSTR   pcfds = (PCFDLGSTR)lParam;
   DWORD       dwi;  // extract the COPY options
   COMPITEM    ci;
   INT         state;
   PVIEW       view;
   LPTSTR      lpf, lpfl, lpfr;
   PCPYTST     pct;

   if( !VALIDPCFDS( pcfds ) )
   {
      EndDialog(hDlg, -1);
      return FALSE;
   }

   view = pcfds->cf_pView;
   if( !view )
   {
      EndDialog(hDlg, -2);
      return FALSE;
   }

   lpfl = &pcfds->cf_szLeft[0];
   lpfr = &pcfds->cf_szRight[0];
   // NOTE: Already handles if expanded or not
   ci = view_getitem(view, pcfds->cf_iSelected);
   //ci = view_getcompitem(view);   // if in outline, and this file
   // is ONLY in left or right, then there will be NO COMPITEM
   //if( !ci )
   //{
      // no COMPITEM so it is a LEFT or RIGHT only file
      // if !view->bExpand, ie in OUTLINE list of files mode
      // then if( (row >= 0) && (row < view->rows) )
      // ci = view->pItems[row];, else NULL
      // else if in EXPANDED, then ci = view->ciSelect;
   //   ci = view_getitem(view, pcfds->cf_iSelected);
   //}

   pcfds->bSingle  = FALSE;

   //gszSingle[0] = 0;

   // already tested if( VALIDPCFDS( pcfds ) ) so save it away
   SET_PROP( hDlg, COPY_ATOM, pcfds );

   // *** at end *************************************
   //CenterDialog( hDlg, pcfds->cf_hParent );  // put it in a good position
   //CenterDialog( hDlg, hwndClient );   // centre it on the client
   //MyCommonDlgInit( hDlg, pcfds, lpb, 1 );   // from COPY FILES

   // ***************************************************
   dwi   = pcfds->dwCpyOpts;  // extract the COPY options
   // ***************************************************
   //state = view_getstate(pv, pcfds->iSelected);
   state = compitem_getstate(ci);   // we always want the FILE compare state
   //( ( dwi & COPY_FROMLEFT ) && ( state == STATE_FILELEFTONLY ) ) )
   //( ( dwi & COPY_FROMRIGHT ) && ( state == STATE_FILERIGHTONLY ) )
   pct = &pcfds->cf_sCopyTest;
   lpf = &pct->ct_szSTitle[0];
   //   !( dwi & COPY_FROMRIGHT ) &&
   if(( ci ) &&
      (pct->ct_dwFlag & flg_Copy1OK) &&
      (dir_isvalidfile(lpf)        ) )
   {
      sprintf(lpb, "Copy [%s] to DESTINATION", lpf);
      SetDlgItemText(hDlg, IDC_CH_SEL, lpb);
      //CheckDlgButton( hDlg, IDC_CH_SEL, BST_UNCHECKED );
   }
   else
   {
      HWND hc = GetDlgItem(hDlg, IDC_CH_SEL);
      EnableWindow( hc, FALSE );
      ShowWindow( hc, SW_HIDE );
   }

   /*
    * set checkboxes and directory field to defaults
    */
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME   );

   // change to NEW /OLD listing
//   CDB( IDD_DIFFER,    dwi, INCLUDE_DIFFER );
   if( dwi & INCLUDE_DIFFER )
   {
      CDB( IDD_DIFFER,    dwi, INCLUDE_NEWER );
      CDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER );
   }

   //CDB( IDD_LEFT,    dwi, (INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY) );
   CDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );

   // *** SET DESTINATION OF COPY ***
   //if( dlg_root[0] ) // use it
   //   strcpy( lpb, dlg_root );
#ifdef   USEOLDCOPY2
//   if( dwi & COPY_FROMRIGHT )  // was COPY_FROMLEFT
//      strcpy( lpb, lpfl);  // dialog_leftname );   // from right to left
//   else
#endif   // #ifdef   USEOLDCOPY2
      strcpy( lpb, lpfr);  // dialog_rightname );   // ELSE from left to right
   // ********************************************************
   SetDlgItemText( hDlg, IDD_DIR1, lpb );   // SET EDIT DESTINATION
   SetDlgItemText( hDlg, IDC_DESTINATION, lpb );   // SET STATIC DESTINATION
   // ********************************************************

#ifdef   USEOLDCOPY2
   /*
    * set 'copy from' buttons to have the full pathname
    */
   SetDlgItemText(hDlg, IDD_FROMLEFT,  lpfl);   //dialog_leftname  );
   SetDlgItemText(hDlg, IDD_FROMRIGHT, lpfr);   // dialog_rightname );

   /*
    * set default radio button for copy from, and set
    * the text on the 'files only in...' checkbox to
    * indicate which path is being selected
    */
   if( dwi & COPY_FROMRIGHT )
      state = IDD_FROMRIGHT;
   else  // DEFAULT = COPY FROM LEFT TO RIGHT
      state = IDD_FROMLEFT;

   CheckRadioButton(hDlg, IDD_FROMLEFT, IDD_FROMRIGHT, state);

#else // !#ifdef   USEOLDCOPY2
   SetDlgItemText( hDlg, IDC_EDSOURCE, lpfl );   // SET SOURCE
#endif   // #ifdef   USEOLDCOPY2 y/n

   cf_WARN( hDlg, lpb );   // and set WARNING indication
   // this "warning" indication will be UPDATED if the 'destination' text
   // is modified ...
   cf_SetLeftRight( hDlg, pcfds );

   *lpb = 0;
   _getcwd( lpb, 256 );
   if( *lpb )
   {
      strcat(lpb, " is the CWD");
      SetDlgItemText( hDlg, IDC_LABCWD, lpb );
   }

   dwi = pcfds->dwVerFlag;
   if( dwi & VERIFY_EACH )
      state = IDC_RADIO1;
   else if( dwi & VERIFY_OLDER )
      state = IDC_RADIO2;
   else
      state = IDC_RADIO3;
   CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, state);

   CDB( IDC_BACKUP, dwi, MAKE_BACKUP );
   CDB( IDC_CHECK1, dwi, REVIEW_LIST );
   CDB( IDC_CHK_WARN, dwi, CHECK_WARN);

   // ****************************************
   if( IsWindow( pcfds->cf_hParent ) )
      CenterDialog( hDlg, pcfds->cf_hParent );  // put it in a good position
   else
      CenterDialog( hDlg, hwndClient );   // centre it on the client
   // ****************************************

   pcfds->bHadInit = TRUE;

   //MyCommonDlgInit( hDlg, pcfds, lpb, 1 );   // from COPY FILES
   MyCommonDlgInit2( hDlg );   // from COPY FILES

   //case IDM_FILECOPY:   // these start off the SAME
   if( g_bFileCopy )      // set the do-it-quick flag
   {
      PostMessage( hDlg, WM_COMMAND, (WPARAM)IDOK, 0 );
      g_bFileCopy = FALSE;
   }

   return TRUE;

}

VOID  cf_ShowErr( HWND hDlg )
{
   LPTSTR      lpb = &g_szBuf1[0];
   sprintf(lpb, "ERROR:"MEOR
         "GET_PROP() FAILED"MEOR
         "to get a valid"MEOR
         "COPY FILE DIALOG STRUCTURE!"MEOR
         "On OK can ONLY exit dialog!" );
   MB( hDlg, lpb, "DIALOG ERROR", MB_ICONSTOP|MB_OK);
}

VOID  cf_ShowErr2( HWND hDlg, LPSTR lpdir )
{
   LPTSTR      lpb = &g_szBuf1[0];
   sprintf(lpb, "ERROR:"MEOR
      "Destination folder:"MEOR
      "%s"MEOR
      "IS NOT VALID!"MEOR
      "No update can be done to an INVALID folder!", lpdir );
   MB( hDlg, lpb, "DIALOG ERROR", MB_ICONSTOP|MB_OK);
}

INT_PTR  cf_IDOK( HWND hDlg )
{
   //PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP(hDlg, COPY_ATOM);
   LPTSTR      lpb = &g_szBuf1[0];
   DWORD       dwi;
   LPTSTR      pdst;

   if( !VALIDPCFDS( pcfds ) )
   {
      cf_ShowErr( hDlg );
      EndDialog(hDlg, -1);
      return FALSE;
   }

   pdst = &pcfds->cf_szDest[0];
   // GET DESTINATION
   // **************************************************************
   //GetDlgItemText(hDlg, IDD_DIR1, dlg_root, 256); // GET DESTINATION
   GetDlgItemText(hDlg, IDD_DIR1, pdst, 256); // GET DESTINATION
   // **************************************************************
   //pdroot = &pcfds->cf_szDest[0];   // ROOT OF DESTINATION MUST EXIST
   if( !dir_isvaliddir(pdst) ) {
      cf_ShowErr2( hDlg, pdst );
      return FALSE;
   }

   // update copy options
   // =====================================================================
   dwi = pcfds->dwCpyOpts;

#ifdef   USEOLDCOPY2
   // GET SOURCE
   // **********
   if( IsDlgButtonChecked(hDlg, IDD_FROMRIGHT) == BST_CHECKED )
   {
      dwi &= ~(COPY_FROMLEFT);    // remove LEFT
      dwi |= COPY_FROMRIGHT;      // and ADD RIGHT

   }
   else // if( IsDlgButtonChecked(hDlg, IDD_FROMLEFT) == BST_CHECKED )
   {
      dwi &= ~(COPY_FROMRIGHT);   // remove RIGHT, and
      dwi |= COPY_FROMLEFT;       // add    LEFT
   }

#endif   // #ifdef   USEOLDCOPY2

   ICDB( IDD_IDENTICAL, dwi, INCLUDE_SAME      );

//   ICDB( IDD_DIFFER,    dwi, INCLUDE_DIFFER    );
   // remove different items
   dwi &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
   ICDB( IDD_DIFFER,    dwi, INCLUDE_NEWER     );
   ICDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER     );
   if( dwi & (INCLUDE_NEWER|INCLUDE_OLDER) ) // if either or boht present
      dwi |= INCLUDE_DIFFER;  // add in DIFFER
   else
      dwi &= ~(INCLUDE_DIFFER);  // else remove DIFFER

   ICDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   ICDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );

   if( IsDlgButtonChecked(hDlg, IDC_CH_SEL) == BST_CHECKED )
      pcfds->bSingle = TRUE;  // single (selected or expanded) file ONLY
   else
      pcfds->bSingle = FALSE;  // single (selected or expanded) file ONLY

   pcfds->dwCpyOpts = dwi; // update the copy options
   // =====================================================================

   // =====================================================================
   // update the VERIFY options
   dwi = pcfds->dwVerFlag;
   if( IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED )
   {
      dwi &= ~(VERIFY_OLDER);
      dwi |= VERIFY_EACH;
   }
   else if( IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED )
   {
      dwi &= ~(VERIFY_EACH);
      dwi |= VERIFY_OLDER;
   }
   else //if( IsDlgButtonChecked(hDlg, IDC_RADIO3) == BST_CHECKED )
   {
      dwi &= ~( VERIFY_OLDER | VERIFY_EACH );
   }

   ICDB( IDC_CHECK1, dwi, REVIEW_LIST );  // review (and modify) list
   ICDB( IDC_BACKUP, dwi, MAKE_BACKUP );
   ICDB( IDC_CHK_WARN, dwi, CHECK_WARN);

   pcfds->dwVerFlag = dwi;    // update the flag in the structure
   // =====================================================================

   EndDialog(hDlg, IDOK);

   return TRUE;
}

VOID  cf_ADDWARN( HWND hDlg, PCFDLGSTR pcfds )
{
   LPTSTR lpb = &gszTmpBuf[0];
   LPTSTR lprt = &pcfds->cf_szRight[0]; // to here (or delete here if right only)
   GetDlgItemText( hDlg, IDD_DIR1, lpb, 256 );
   // OR
   //SendMessage( (HWND)lParam, WM_GETTEXT, (WPARAM)256, (LPARAM)lpb );
   if( cf_WARN( hDlg, lpb ) && strcmpi( lpb, lprt ) ) {
      cf_SetDeleteStr( hDlg, pcfds, lpb );
   }
}

VOID  cf_IDD_DIR1( HWND hDlg, WPARAM wParam, LPARAM lParam, PCFDLGSTR pcfds )
{
   DWORD msg = HIWORD(wParam);   // get the notification
   if( ( pcfds->bHadInit ) &&
       ( ( msg == EN_CHANGE ) ||
         ( msg == EN_SETFOCUS ) ) )
   {
      cf_ADDWARN( hDlg, pcfds );
      if( VALIDPCFDS( pcfds ) )
      {
         //if( pcfds->bHadInit )
            pcfds->bHadUpd = TRUE;
      }
   }
}

// INCLUDE_RIGHTONLY (***DELETE***)
VOID  cf_IDD_RIGHT( HWND hDlg, WPARAM wParam, LPARAM lParam, PCFDLGSTR pcfds )
{
   if( VALIDPCFDS( pcfds ) )
   {
      if( IsDlgButtonChecked(hDlg, IDD_RIGHT) == BST_CHECKED )
      {
         if( pcfds->bDnWarn )
         {
            if( IsDlgButtonChecked(hDlg, IDC_CHK_WARN) == BST_CHECKED )
               pcfds->bDnWarn = FALSE;
         }
         if( !pcfds->bDnWarn )
         {
            DWORD dwi = (MAKE_BACKUP|REVIEW_LIST);   // check the BACKUP
            CDB( IDC_BACKUP, dwi, MAKE_BACKUP );
            CDB( IDC_CHECK1, dwi, REVIEW_LIST );
            CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
            pcfds->bDnWarn = TRUE;
         }
         cf_Disable2(hDlg, pcfds);  // disable some, but ENSURE REVIEW LIST is ENABLED
         EnableWindow( GetDlgItem(hDlg, IDC_CHECK1),  TRUE ); // REVIEW_LIST
         SetWindowText( hDlg, "DELETE FILES" );
      }
      else
      {
         cf_Enable2(hDlg, pcfds);
         SetWindowText( hDlg, "COPY FILES" );
      }
      cf_ADDWARN( hDlg, pcfds );
   }
}

BOOL  cf_IsCommand( DWORD wCmd )
{
   BOOL  bRet = FALSE;
   switch( wCmd )
   {
   case IDD_FROMLEFT:
   case IDD_FROMRIGHT:
   case IDOK:
   case IDCANCEL:
   case IDC_CH_SEL:
   case IDD_DIR1:
      bRet = TRUE;
      break;
   }
   return bRet;
}

VOID  cf_IDC_CH_SEL( HWND hDlg, PCFDLGSTR pcfds )
{
   LPTSTR   lpf = &pcfds->szSingle[0];

   if( ( VALIDPCFDS( pcfds ) ) &&
       ( pcfds->bHadInit     ) )
   {
      if( ( IsDlgButtonChecked(hDlg, IDC_CH_SEL) == BST_CHECKED ) &&
          ( dir_isvalidfile(lpf) ) )
      {
         cf_Disable(hDlg, pcfds);
      }
      else
      {
         cf_Enable(hDlg, pcfds);
         if( ( IsDlgButtonChecked(hDlg, IDC_CH_SEL) == BST_CHECKED ) &&
             ( !dir_isvalidfile(lpf) ) )
         {
            CheckDlgButton( hDlg, IDC_CH_SEL, FALSE );
            pcfds->bSingle = FALSE;
         }
      }
   }
}

INT_PTR  cf_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR     iRet = 0;
   DWORD       wCmd = GET_WM_COMMAND_ID(wParam, lParam);
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP(hDlg, COPY_ATOM);
   LPTSTR      lpb = &g_szBuf1[0];
   DWORD       dwo;

   //if( !cf_IsCommand(wCmd) )
   //   return FALSE;

   if( !VALIDPCFDS( pcfds ) )
   {
      //cf_ShowErr( hDlg );
      //EndDialog(hDlg, -1);
      return FALSE;
   }

   dwo = pcfds->dwCpyOpts;

   switch( wCmd )
   {
#ifdef   USEOLDCOPY2
   case IDD_FROMLEFT:
           dwo &= ~(COPY_FROMRIGHT);
           dwo |= COPY_FROMLEFT;
           pcfds->dwCpyOpts = dwo;
           cf_SetLeftRight( hDlg, pcfds );
           iRet = TRUE;
           break;

   case IDD_FROMRIGHT:
           dwo &= ~(COPY_FROMLEFT);
           dwo |= COPY_FROMRIGHT;
           pcfds->dwCpyOpts = dwo;
           cf_SetLeftRight( hDlg, pcfds );
           iRet = TRUE;
           break;
#endif   // #ifdef   USEOLDCOPY2

   case IDOK:
      cf_IDOK( hDlg );
      iRet = TRUE;
      break;

   case IDCANCEL:
           EndDialog(hDlg, FALSE);
           iRet = TRUE;
           break;

   case IDC_CH_SEL:
      cf_IDC_CH_SEL( hDlg, pcfds );
      break;

   case IDD_DIR1:
      cf_IDD_DIR1( hDlg, wParam, lParam, pcfds );
      break;

#ifdef ADD_ZIP_SUPPORT
   case IDC_BUTZIPIT:
      WriteZIPList( &g_sThreadArgs, 1 ); // #ifdef ADD_ZIP_SUPPORT
      break;
#endif // #ifdef ADD_ZIP_SUPPORT

   }

   Post_WM_COMMAND( hDlg, wCmd, lParam, pcfds, 1 );   // from COPY FILES

   return iRet;
}

/***************************************************************************
 * Function: COPYFILESDLGPROC
 *
 * Purpose:
 *
 * dialog to get directory name and inclusion options. Init dlg fields from
 * the dlg_* variables, and save state to the dlg_* variables on dialog
 * close. return TRUE for OK, or FALSE for cancel (from the dialogbox()
 * using EndDialog).
 * 
 * From MENU item IDM_COPYFILES using IDD_COPYFILES2 dialog template
 * Is actually passed PCFDLGSTR pcfds = &g_sCFDLGSTR; // get GLOBAL structure
 *
 **************************************************************************/
//complist_dodlg_copyfiles(HWND hDlg, UINT message, UINT wParam, long lParam)
INT_PTR CALLBACK COPYFILESDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   int iRet = 0;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      iRet = cf_WM_INITDIALOG( hDlg, lParam );
      break;

   case WM_COMMAND:
      iRet = cf_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, FALSE);
      break;

   case WM_DESTROY:
      REMOVE_PROP( hDlg, COPY_ATOM );
      break;

   }

   return iRet;

} /* COPYFILESDLGPROC (was complist_dodlg_copyfiles) */



///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Do_COPYFILES_DLG
// Return type: INT_PTR 
// Argument   : PVOID pv
// Description: Put up a DIALOG to get the USER COPY Options
//              
// NOTE: UpdCopyDelStrs(pcfds) has ALREADY been called
// Is actually passed PCFDLGSTR pcfds = &g_sCFDLGSTR; // get GLOBAL structure
// From MENU IDM_COPYFILES on thread wd_copy()
///////////////////////////////////////////////////////////////////////////////
INT_PTR  Do_COPYFILES_DLG( PVOID pv )
{
   INT_PTR  ip = 0;
   PCFDLGSTR   pcfds = (PCFDLGSTR)pv;
   DWORD       dwi; // extract COPY options

//   dwi = (pcfds->dwCpyOpts & INC_OUTLINE); // extract COPY options
   dwi = (pcfds->dwCpyOpts & INC_OUTLINE2); // extract COPY options - w/ new/old sep.
   if( dwi == 0 )
      pcfds->dwCpyOpts |= INCLUDE_DIFFER; // always have DIFFERENT at least

   pcfds->bHadInit = FALSE;
   pcfds->bHadUpd  = FALSE;
   pcfds->bDnWarn  = FALSE;   // this is ALWAYS reset for ONE WARNING

   dc4w_UI(TRUE);
   ip = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_COPYFILES2), // IDD_COPYFILES),  // Copy files template
      hwndClient,
      COPYFILESDLGPROC,     // dialog procedure
      (LPARAM) pcfds );
   dc4w_UI(FALSE);

   return ip;
}

// END   COPYFILES DIALOG
// ======================
// eof - dc4wDlgcf.c
