
// dc4wDlg.c
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"
//#include "dc4wDlg.h"
//#include "Browse.h"

extern INT Get_Excl_List( LPTSTR lpb2, PLE pH ); // ini service - list back to ONE LINE ; sepped
extern INT Add_List_2_List( PLE pH, LPTSTR lps ); // add "abc;def;temp*... string to LIST

// resolution
extern   DWORD gcxScreen, gcyScreen;

extern   BOOL IsBusy(); // check if a THREAD is running

extern   BOOL dlg_recursive;
/* data for Directory and SaveList */
//extern   TCHAR dialog_leftname[];
//extern   TCHAR dialog_rightname[];
extern   TCHAR dlg_leftname[];
extern   TCHAR dlg_rightname[];
extern   DWORD dlg_flg;

extern   VOID  compitem_retrelname( LPTSTR lpb, COMPITEM ci );
extern   COMPITEM view_getcompitem(PVIEW view);
extern   TCHAR dlg_root[];  // temporary buffer for COPY TO destination

extern   BOOL  dir_isvaliddir(LPSTR path);
extern   LPTSTR   GetMinTxt( LPTSTR pb1, LPTSTR pb2, LPTSTR pSep );
extern   BOOL     g_bOneShot;  // posted IDM_DIR to bring up dialog box
// =====================
extern   TCHAR szFilCnt[]; // = "(%d files)";
//extern   BOOL dlg_identical, dlg_differ, dlg_left, dlg_right;
extern   TCHAR dlg_head[];                /* BEFORE filename */
extern   TCHAR dlg_tail[];                /* AFTER filename */
extern   BOOL  g_bFileCopy;   // set the do-it-quick flag
extern   VOID  AppendCopyOptLetters( LPTSTR lpb, DWORD dwo );

extern   INT   InitExcludeList( HWND hwnd );
#ifdef ADD_ZIP_SUPPORT
extern   INT   WriteZIPList( PTHREADARGS pta, DWORD dwt ); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT

extern   FD    g_sFD;   // global WIN32_FIND_DATA pads

// global TCHAR gszCopyTo[];   // destination of COPY files
// and BOOL  bChgCT;
DWORD g_dwchkcnt = 0;
BOOL  bDnDlgInit = FALSE;
TCHAR szLstTot[] = "Total = %d";
BOOL  g_bHadPost = FALSE;
TCHAR prev_dlg_left[264];  // old dlg_leftname );
INT   dlg_chgleft = 0;
TCHAR prev_dlg_right[264];
INT   dlg_chgright = 0;
BOOL  b_doZIP = FALSE; // set to FALSE

// list of OPTIONS matched with COUNT (if OPTION on)
typedef struct tagOPTCNT {
   UINT  uiCheck;    // check box (on dialog)
   UINT  uiLabel;    // label showing count of item
   DWORD dwCount;    // actual COUNT of this 'type'
   DWORD dwBits;     // output bit - what to include
}OPTCNT, * POPTCNT;

// to hide this a bit -
   // set DIALOG labels
   //SDL( szFilCnt, pcfds->dwTLeft, IDC_LABLEFT );
   //SDL( szFilCnt, pcfds->dwTRite, IDC_LABRITE );

// COPY FILES INIT
//   SDL( szFilCnt, pcfds->dwSame, IDC_LABIDENT );
//   sprintf(lpb, szFilCnt, pcfds->dwDiff );
//   SetDlgItemText( hDlg, IDC_LABDIFF, lpb );
//   SDL( szFilCnt, pcfds->dwNewer, IDC_LABDIFF );
//   SDL( szFilCnt, (pcfds->dwDiff - pcfds->dwNewer), IDC_LABDIFF2 );
// dlg - only ONE at a time - on a thread usually - modal, but can
// be quietly aborted, and the old view will be rebuilt.
// per IDD_COPYFILES (and IDD_SAVELIST)
// so there should be a common init.
// { <no check box>, IDC_LISTTOTAL,0 },
// *** WRITE FILE LIST ***
//    LTEXT           "ident",IDC_LABIDENT,7,56,40,8
//    CONTROL         "&Identical files",IDD_IDENTICAL,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,51,56,60,10
//    LTEXT           "newer",IDC_LABDIFF,7,68,40,8
//    CONTROL         "&Newer files",IDD_DIFFER,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,51,68,64,10
//    LTEXT           "older",IDC_LABDIFF2,7,80,40,8
//    CONTROL         "&Older files",IDD_DIFFER2,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,51,80,64,10
//    LTEXT           "left",IDC_LABLEFT,7,92,40,8
//    CONTROL         "Files only in &left tree",IDD_LEFT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,51,92,198,10
//    LTEXT           "right",IDC_LABRITE,7,104,40,8
//    CONTROL         "Files only in &right tree",IDD_RIGHT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,51,104,198,10
OPTCNT   sOptCnt[] = {
   { IDD_IDENTICAL, IDC_LABIDENT, 0, INCLUDE_SAME      },
   { IDD_DIFFER,    IDC_LABDIFF,  0, INCLUDE_NEWER     }, // newer
   { IDD_DIFFER2,   IDC_LABDIFF2, 0, INCLUDE_OLDER     }, // older
   { IDD_LEFT,      IDC_LABLEFT,  0, INCLUDE_LEFTONLY  },
   { IDD_RIGHT,     IDC_LABRITE,  0, INCLUDE_RIGHTONLY },
   // last record
   { 0, 0, 0 }
};

VOID  ClearDlgCnts( VOID )
{
   POPTCNT  poc = &sOptCnt[0];
   while( poc->uiCheck )
   {
      poc->dwCount = 0;
      poc++;
   }
}

VOID  SetDlgCnts( UINT ui, DWORD dwCnt )
{
   POPTCNT  poc = &sOptCnt[0];
   while( poc->uiCheck )
   {
      if( poc->uiLabel == ui )
      {
         poc->dwCount = dwCnt;
         break;
      }
      poc++;
   }
}

DWORD GetDlgCnts( UINT ui )
{
   POPTCNT  poc = &sOptCnt[0];
   while( poc->uiCheck )
   {
      if( poc->uiLabel == ui )
      {
         return(poc->dwCount);
         break;
      }
      poc++;
   }
   return 0;
}

BOOL  ToggleDlgBit( HWND hDlg, UINT ui, PDWORD pdwFlag, DWORD dwBit );

// format text with value, and set label
#define  SDL( a, b, c ) {             \
      sprintf(lpb, a, b );            \
      SetDlgItemText( hDlg, c, lpb ); \
      SetDlgCnts( c, b );\
   }


#define  ICDB_TB(a,b,c)\
{\
   DWORD _dwflg = b;\
   if( ToggleDlgBit( hDlg, a, &_dwflg, c ) )\
      b = _dwflg;\
}

#define  ICDB3(a,b,c)\
   if( IsDlgButtonChecked( hDlg, a ) == BST_CHECKED )\
      if( !(b & c) ) {b |= c;g_dwchkcnt++;}\
   else\
      if(   b & c  ) {b &= ~(c); g_dwchkcnt++;}


#define  CDBB(a,b) CheckDlgButton( hDlg, a, ( b  ? BST_CHECKED : BST_UNCHECKED ) )
#define  ICDBB(a)     ( IsDlgButtonChecked( hDlg, a ) == BST_CHECKED )

// forward references
VOID  Set_Action_Line( HWND hDlg, DWORD dwCaller );

CC g_sCC;

VOID  CheckCommonSet( HWND hDlg, DWORD dwi )
{
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME      );
   CDB( IDD_DIFFER,    dwi, INCLUDE_NEWER     );
   CDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER     );
   CDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );
}

VOID  MyCommonDlgInit2( HWND hDlg )
{
   TCHAR    buf[128];
   LPTSTR   lpb = &buf[0];
   DWORD    dwTot;

   ClearDlgCnts();

   //SDL( szLstTot, view_gettotcount( current_view ), IDC_LISTTOTAL );
   dwTot = view_gettotcount( current_view );
   SDL( szLstTot, dwTot, IDC_LISTTOTAL );

   // set DIALOG labels
   //SDL( szFilCnt, pcfds->dwTLeft, IDC_LABLEFT );
   //SDL( szFilCnt, pcfds->dwTRite, IDC_LABRITE );
   SDL( szFilCnt, g_dwLeft, IDC_LABLEFT );    // Left tree only
   SDL( szFilCnt, g_dwRite, IDC_LABRITE );    // right tree only

   // COPY FILES INIT
//   sprintf(lpb, szFilCnt, pcfds->dwDiff );
//   SetDlgItemText( hDlg, IDC_LABDIFF, lpb );
   SDL( szFilCnt, g_dwNewer, IDC_LABDIFF );   // newer
   SDL( szFilCnt, (g_dwDiff - g_dwNewer), IDC_LABDIFF2 );  // older
   SDL( szFilCnt, g_dwSame, IDC_LABIDENT );

   *lpb = 0;
   if( g_dwUnk )
      sprintf(lpb, szFilCnt, g_dwUnk );
   SetDlgItemText( hDlg, IDC_LABUNK, lpb );

   if(dwTot != g_ListTotal) { // - total in 'combined' file list
      sprtf( "WARNING: Total different! %d vs %d!"MEOR, dwTot, g_ListTotal );
   }
   if(g_dwSame != g_SameCnt) {  // add count of files that are the SAME
      sprtf( "WARNING: Same count different! %d vs %d."MEOR, g_dwSame, g_SameCnt);
   }
   if(g_dwOlder != g_OlderCnt) {
      sprtf( "WARNING: OLd count different! %d vs %d."MEOR, g_dwOlder, g_OlderCnt );
   }
   if(g_dwNewer != g_NewerCnt) { //  - if date newer - ie ready for update
      sprtf( "WARNING: Newer count different! %d vs %d!"MEOR, g_dwNewer, g_NewerCnt);
   }
   if(g_dwLeft != g_LeftOnly) { //  - files ONLY in left tree
      sprtf( "WARNING: Left count different! %d vs %d."MEOR, g_dwLeft, g_LeftOnly);
   }
   if(g_dwRite != g_RightOnly) {
      sprtf( "WARNING: Right count different! %d vs %d!"MEOR, g_dwRite, g_RightOnly);
   }

   //if(bchg)
   //if( !g_bHadPost )
   {
      Set_Action_Line( hDlg, 0 );
      //g_bHadPost = TRUE;
   }

   sprtf( "Done WM_INITDIALOG. S=%d O=%d N=%d L=%d R=%d"MEOR,
      g_SameCnt, g_OlderCnt, g_NewerCnt, g_LeftOnly, g_RightOnly );

}

DWORD g_dwActLstOpts = 0;

VOID  Set_Action_Line( HWND hDlg, DWORD dwCaller )
{
   LPTSTR   lpb = GetNxtBuf();  // &gszTmpBuf[0];
   PCC      pcc = &g_sCC;
   DWORD    dwo = 0;

   ZeroMemory( pcc, sizeof(CC) );
      // refresh the 'on OK' action count display
      // IDC_OK_STATUS - If we got an OK now, how many
      // items would be processed - actually probaly only interested in NONE
//typedef struct tagCC {
//   DWORD cnt1; // total in LIST
//   DWORD cnt2; // pass the compare vs state test
//   DWORD cnt3; // passed actual copy test criteria
//   BOOL  bUseOpt; // passing options to use for test
//   DWORD dwopts;
//} CC, * PCC;
   // get any UPDATES from the check boxes
   ICDB( IDD_IDENTICAL, dwo, INCLUDE_SAME      );
   ICDB( IDD_DIFFER,    dwo, INCLUDE_NEWER     );
   ICDB( IDD_DIFFER2,   dwo, INCLUDE_OLDER     );
   ICDB( IDD_LEFT,      dwo, INCLUDE_LEFTONLY  );
   ICDB( IDD_RIGHT,     dwo, INCLUDE_RIGHTONLY );
   FixDiff(dwo);

//   if( ( dwCaller == 0 ) || ( dwCaller == 1 ) )
   {
      // just want to write out a list of names
      // options = gdwFileOpts
      pcc->bUseOpt = TRUE;
      pcc->dwopts  = dwo;
      complist_countlistable( pcc );
      if( pcc->cnt2 == 0 )
      {
         strcpy( lpb, "No action. Options ");
         AppendCopyOptLetters( lpb, dwo );
         strcat(lpb,"!");
         SDL( lpb, 0, IDC_OKSTATUS );
      }
      else
      {
         SDL( "Will action %d on OK", pcc->cnt2, IDC_OKSTATUS );
      }

      sprtf( "DLGACTION: [%s]."MEOR, lpb );
   }

}

// Set the RESULT's label, using the current options
// sort of a common dialog command processor
// if it is one of interest, trigger the event
// to change the TEXT of the resultes.
VOID  Post_WM_COMMAND( HWND hDlg, UINT wCmd, LPARAM lParam, PCFDLGSTR pcfds,
                      DWORD dwCaller )
{
//   static CC _s_cc;
   POPTCNT  poc = &sOptCnt[0];
   DWORD    dwo = pcfds->dwCpyOpts; // extract COPY options
   BOOL     bchg = FALSE;
//   PCC      pcc;  // = &_s_cc;
   while( poc->uiCheck )
   {
      if( poc->uiCheck == wCmd ) // the CHECK is being TOGGLED
      {
         bchg = TRUE;
         break;
      }
      poc++;
   }

   if(bchg)
   {
      g_bHadPost = TRUE;
      Set_Action_Line( hDlg, dwCaller );
   }
}


BOOL  ToggleDlgBit( HWND hDlg, UINT ui, PDWORD pdwFlag, DWORD dwBit )
{
   BOOL  bRet = FALSE;
   DWORD dwFlag = *pdwFlag;
   if( IsDlgButtonChecked( hDlg, ui ) == BST_CHECKED )
   {
      if( !(dwFlag & dwBit) )
      {
         dwFlag |= dwBit;
         g_dwchkcnt++;
         bRet++;
      }
   }
   else
   {
      if( dwFlag & dwBit )
      {
         dwFlag &= ~(dwBit);
         g_dwchkcnt++;
         bRet++;
      }
   }
   if(bRet)
      *pdwFlag = dwFlag;   // return update

   return bRet;
}

INT  ToggleOption( PBOOL bOpt, PBOOL bChg, BOOL bSet )
{
   INT  iRet = 0;
   if( bSet )
   {
      if( !*bOpt )
      {
         *bOpt = TRUE;
         *bChg = TRUE;
         iRet = 1;
      }
   }
   else
   {
      if( *bOpt )
      {
         *bOpt = FALSE;
         *bChg = TRUE;
         iRet = 1;
      }
   }
   return iRet;
}

VOID  co_WM_INITDIALOG( HWND hDlg )
{
   // CONTROL         "Ignore CaSe",IDC_NOCASE,"Button",BS_AUTOCHECKBOX |
   // CONTROL         "Ignore Spaces (blanks)",IDC_IGNORESP,"Button",
   // CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
   // CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
   // CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
   // FIX20081125
   // CONTROL         "Ignore File Time",IDC_IGNORETIME,"Button",
   // CONTROL         "Ignore File Time Completely",IDC_IGNORETIME2,"Button",
   // // IDM_OPTIGNOREDT
   // gbIgnDT, gbChgIDT, gbIgnDT2, gbChgIDT2
   CDBB( IDC_IGNORETIME, gbIgnDT );
   CDBB( IDC_IGNORETIME2, gbIgnDT2 );

//TCHAR szCase[] = "IgnoreCase";
//   { szOpt, szCase, it_Bool, (LPTSTR)&gbIgnCase,        &bChgIgC, (PVOID)
   CDBB( IDC_NOCASE, gbIgnCase );

   //     MENUITEM "Ignore &Blanks",              IDM_IGNBLANKS
   //{ szOpt, szBnks, it_Bool, (LPTSTR)&ignore_blanks,    &bChgBks, (PVOID)
   CDBB( IDC_IGNORESP, ignore_blanks );

//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
//   { szOpt, szSkipC, // = "Skip_C/C++_Comments"; BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//   it_Bool, (LPTSTR)&gbSkipCPP, &bChgSCPP, (PVOID)IDC_SKIPCCOMM, 0 },
   CDBB( IDC_SKIPCCOMM, gbSkipCPP );

//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
//   { szOpt, szSkipT, // "Skip-Quoted-Text"; BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//   it_Bool, (LPTSTR)&gbSkipTxt, &bChgSTxt, (PVOID)IDC_SKIPQTXT, 0 },
   CDBB( IDC_SKIPQTXT, gbSkipTxt );

//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
//   { szOpt, szIgnEL, // "Ignore-Line-Termination"; BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11
//   it_Bool, (LPTSTR)&gbIgnEOL, &bChgIEOL, (PVOID)IDC_IGNORETERM, 0 },
   CDBB( IDC_IGNORETERM, gbIgnEOL );

   //PUSHBUTTON   "Compare Options",ID_BTN_COMPARE,7,7,79,14
   //PUSHBUTTON   "Expanded Options",ID_BTN_EXPANDED,7,54,79,14
   //PUSHBUTTON   "Outline Options",ID_BTN_OUTLINE,7,31,79,14
}


INT  co_IDOK( HWND hDlg )
{
   INT   i = 0;

   // FIX20081125
   // IDC_IGNORETIME, IDC_IGNORETIME2
   // gbIgnDT, gbChgIDT, gbIgnDT2, gbChgIDT2
   i += ToggleOption( &gbIgnDT, &gbChgIDT,
      ( ICDBB(IDC_IGNORETIME) ? TRUE : FALSE ) );
   i += ToggleOption( &gbIgnDT2, &gbChgIDT2,
      ( ICDBB(IDC_IGNORETIME2) ? TRUE : FALSE ) );
   if( gbIgnDT2 && !gbIgnDT ) // if COMPLETELY is ON, then the partial MUST be on also
      i += ToggleOption( &gbIgnDT, &gbChgIDT, TRUE );

//TCHAR szCase[] = "IgnoreCase";
//   { szOpt, szCase, it_Bool, (LPTSTR)&
   i += ToggleOption( &gbIgnCase, &bChgIgC,
      ( ICDBB(IDC_NOCASE) ? TRUE : FALSE ) );

   //     MENUITEM "Ignore &Blanks",              IDM_IGNBLANKS
   //{ szOpt, szBnks, it_Bool, (LPTSTR)&ignore_blanks,    &bChgBks, (PVOID)
   i += ToggleOption( &ignore_blanks, &bChgBks,
      ( ICDBB(IDC_IGNORESP) ? TRUE : FALSE ) );

//TCHAR szSkipC[] = "Skip-C/C++-Comments";
   i += ToggleOption( &gbSkipCPP, &bChgSCPP,
      ( ICDBB(IDC_SKIPCCOMM) ? TRUE : FALSE ) );

//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
//TCHAR szSkipT[] = "Skip-Quoted-Text";
   i += ToggleOption( &gbSkipTxt, &bChgSTxt,
      ( ICDBB(IDC_SKIPQTXT) ? TRUE : FALSE ) );

//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
//TCHAR szIgnEL[] = "Ignore-Line-Termination";
   i += ToggleOption( &gbIgnEOL, &bChgIEOL,
      ( ICDBB( IDC_IGNORETERM ) ? TRUE : FALSE ) );

   return i;   // erturn CHANGE count
}

VOID  SetRadio123( HWND hDlg, DWORD dwi )
{
   UINT  ui;
//#define  FLEFT_NAME              0x00004000  // use LEFT full name
//#define  FRIGHT_NAME             0x00008000  // use RIGHT full name
//#define  COMBINED_NAME           0x00010000  // use combination of left and right
   if( dwi & FLEFT_NAME )
      ui = IDC_RADIO1;
   else if( dwi & FRIGHT_NAME )
      ui = IDC_RADIO2;
   else
      ui = IDC_RADIO3;
   CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO3, ui );
}

VOID  GetRadio123( HWND hDlg, PDWORD pdwi )
{
   DWORD dwi = *pdwi;
//#define  FLEFT_NAME              0x00004000  // use LEFT full name
//#define  FRIGHT_NAME             0x00008000  // use RIGHT full name
//#define  COMBINED_NAME           0x00010000  // use combination of left and right
   if( IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED )
   {
      dwi |= FLEFT_NAME;
      dwi &= ~(FRIGHT_NAME | COMBINED_NAME);
   }
   else if( IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED )
   {
      dwi |= FRIGHT_NAME;
      dwi &= ~(FLEFT_NAME | COMBINED_NAME);
   }
   else //if( IsDlgButtonChecked(hDlg, IDC_RADIO3) == BST_CHECKED )
   {
      dwi |= COMBINED_NAME;
      dwi &= ~(FLEFT_NAME | FRIGHT_NAME);
   }
}


VOID  SetRadio456( HWND hDlg, DWORD dwi )
{
   UINT  ui;
   // This was just
   // CDB( IDC_CHECK1,    dwi, FULL_NAMES        );
   // but now it is a radio set 4=Full, 5=Relative, 6=title only
   // controlled by two bits in gdwFileOpts - FULL_NAMES and ADD_REL_PATH
   if( dwi & FULL_NAMES )
      ui = IDC_RADIO4;  // check Full
   else if( dwi & ADD_REL_PATH ) // NOTE REV - better would be USE_TITLE_ONLY
      ui = IDC_RADIO6;  // check Title, else
   else
      ui = IDC_RADIO5;  // show relative path - minus the ".\" root item
  CheckRadioButton( hDlg, IDC_RADIO4, IDC_RADIO6, ui );

}

VOID  GetRadio456( HWND hDlg, PDWORD pdwi )
{
   DWORD dwi = *pdwi;
   //ICDB( IDC_CHECK1,    dwi, FULL_NAMES        );
   if( IsDlgButtonChecked(hDlg, IDC_RADIO4) == BST_CHECKED )
   {
      dwi |= FULL_NAMES;
   }
   else if( IsDlgButtonChecked(hDlg, IDC_RADIO6) == BST_CHECKED )
   {
      dwi |= ADD_REL_PATH; // this is FILE TITLE ONLY SHOW - can be confusing
      dwi &= ~(FULL_NAMES);
   }
   else //if( IsDlgButtonChecked(hDlg, IDC_RADIO5) == BST_CHECKED )
   {
      // this it the default - the relative name (minus the root ".\")
      dwi &= ~(FULL_NAMES | ADD_REL_PATH);
   }
}

VOID  ou_CommonInit( HWND hDlg, DWORD dwi )
{
//   LPTSTR      lpb   = &g_szBuf1[0];
//   PCFDLGSTR   pcfds = &gsPCFDLGSTR;
   //dwi = gdwFileOpts; // use to when writing OUTLINE list of files
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME      );
   CDB( IDD_DIFFER,    dwi, INCLUDE_NEWER     );
   CDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER     );
   CDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );

   // Options for output control - how to give MORE here
   // One idea would be to execute a requested program pointing
   // to the file written - for further processing of file list info
   // ***
   //CDB( IDC_CHECK1,    dwi, FULL_NAMES        );
   SetRadio456( hDlg, dwi );
   SetRadio123( hDlg, dwi );

   CDB( IDC_CHECK2,    dwi, INCLUDE_HEADER    );
   CDB( IDC_CHECK3,    dwi, APPEND_FILE       );
   CDB( IDC_CHECK4,    dwi, ADD_COMMENTS      );   // add trailing comments
   CDB( IDC_CHECK7,    dwi, ADD_FIL_INFO      );   // add file date/time/size(s)
   CDB( IDC_CHECK8,    dwi, ADD_X_HDR         );   // multi-lined

   //MyCommonDlgInit( hDlg, pcfds, lpb, 0 );   // from SAVE LIST
   MyCommonDlgInit2( hDlg );   // from SAVE LIST

}

VOID  ou_WM_INITDIALOG( HWND hDlg )
{
   //DWORD dwo = gdwFileOpts;
   DWORD dwo = outline_include;
   DWORD dwv = gdwVerFlag;

   ou_CommonInit( hDlg, dwo );
//        MENUITEM "Show &Identical Files\tCtrl+I", IDM_INCSAME
   // outline  mode - file list output default options
   //gdwFileOpts = INC_ALLXSM;
   //gdwWrapWid  = DEF_WRAP_WIDTH;
// case 'S': ta->diffopts |= INCLUDE_SAME;
//   CDB( IDD_IDENTICAL, dwo, INCLUDE_SAME );
// case 'L': ta->diffopts |= INCLUDE_LEFTONLY;
//   CDB( IDD_LEFT,      dwo, INCLUDE_LEFTONLY );
// case 'R': ta->diffopts |= INCLUDE_RIGHTONLY;
   // copy / update files default options

   CDB( IDC_BACKUP, dwv, MAKE_BACKUP );
   CDB( IDC_CHECK1, dwv, REVIEW_LIST );
   CDB( IDC_CHK_WARN, dwv, CHECK_WARN);

}

BOOL  ou_IDOK( HWND hDlg )
{
   BOOL  bRet = FALSE;
   DWORD dwi = gdwFileOpts;
   DWORD dwo = outline_include;
   DWORD dwv = gdwVerFlag;

// case 'S': ta->diffopts |= INCLUDE_SAME;
   ICDB( IDD_IDENTICAL, dwo, INCLUDE_SAME );
// case 'L': ta->diffopts |= INCLUDE_LEFTONLY;
   ICDB( IDD_LEFT,      dwo, INCLUDE_LEFTONLY );
// case 'R': ta->diffopts |= INCLUDE_RIGHTONLY;
   ICDB( IDD_RIGHT,     dwo, INCLUDE_RIGHTONLY );

// case 'D': ta->diffopts |= INCLUDE_DIFFER;
   ICDB( IDD_DIFFER,    dwo, INCLUDE_NEWER );
   ICDB( IDD_DIFFER2,   dwo, INCLUDE_OLDER );
   if( dwo & INCLUDE_DIFFER )
   {
      if( !(dwo & (INCLUDE_NEWER|INCLUDE_OLDER)) )
         dwo &= ~(INCLUDE_DIFFER);  // remove the FLAG
   }
   else
   {
      if( (dwo & (INCLUDE_NEWER|INCLUDE_OLDER) ) )
         dwo |= INCLUDE_DIFFER;  // add single FLAG
   }

   // note: idc_check1 removed from dialog - see radio456 below
   ICDB( IDC_CHECK2,    dwi, INCLUDE_HEADER    );
   ICDB( IDC_CHECK3,    dwi, APPEND_FILE       );
   ICDB( IDC_CHECK4,    dwi, ADD_COMMENTS      );   // add trailing comments
   ICDB( IDC_CHECK7,    dwi, ADD_FIL_INFO      );   // add date/time/size(s)
   ICDB( IDC_CHECK8,    dwi, ADD_X_HDR         );   // multi-lined
   GetRadio456( hDlg, &dwi );
   GetRadio123( hDlg, &dwi );

   ICDB( IDC_BACKUP,   dwv, MAKE_BACKUP );
   ICDB( IDC_CHECK1,   dwv, REVIEW_LIST );
   ICDB( IDC_CHK_WARN, dwv, CHECK_WARN);

   if( (gdwFileOpts & ~(INC_OUTLINE2)) != (dwi & ~(INC_OUTLINE2) ) )
   {
      gdwFileOpts = dwi | (gdwFileOpts & INC_OUTLINE2);
      bChgFO = TRUE;
   }

   if( dwo != outline_include )
   {
      //gdwFileOpts = dwo;
      sprtf( "Changed outline_include from %d to %d"MEOR,
         outline_include,
         dwo );

      outline_include = dwo;
      // { szOpt, szIncl, it_Int, (LPTSTR) &outline_include,  &
      bChgInc = TRUE;

      bRet++;
   }
   if( (gdwFileOpts & INC_OUTLINE2) != (outline_include & INC_OUTLINE2) )
   {
      gdwFileOpts = (gdwFileOpts & ~(INC_OUTLINE2)) | (outline_include & INC_OUTLINE2);
      bChgFO = TRUE;
   }

   if( dwv != gdwVerFlag  )
   {
      gdwVerFlag  = dwv;

      bChgVF     = TRUE;

      bRet++;

   }

   return bRet;

}

//    DEFPUSHBUTTON   "OK",IDOK,122,165,50,14
//    PUSHBUTTON      "Cancel",IDCANCEL,182,165,50,14
//    PUSHBUTTON      "Apply",ID_BTN_APPLY,243,165,50,14
//    PUSHBUTTON      "Help",ID_BTN_HELP,56,165,50,14,WS_DISABLED | NOT 
//                    WS_TABSTOP
//    PUSHBUTTON      "Compare Options",ID_BTN_COMPARE,7,7,79,14
//    PUSHBUTTON      "Expanded Options",ID_BTN_EXPANDED,163,7,79,14,BS_FLAT
//    PUSHBUTTON      "Outline Options",ID_BTN_OUTLINE,85,7,79,14
/* ===============

VOID  ex_WM_INITDIALOG2( HWND hDlg )
{
   // expanded mode - line output default options
   // DWORD dwo = gdwDiffOpts;   // = INC_ALLXSAME;   // everything excluding SAME lines
   DWORD dwo = outline_include;
   UINT  dwi = 0;

//IDD_EXPANDED DIALOG DISCARDABLE  0, 0, 300, 186
//STYLE WS_POPUP | WS_CAPTION | WS_SYSMENU
//CAPTION "PREFERENCES"
//FONT 8, "MS Sans Serif"
//BEGIN
   // not yet implemented - so can only Show ALL Lines
//    CONTROL         "Show ALL Lines",IDC_RADIO1,"Button",BS_AUTORADIOBUTTON,
//                    23,42,110,14
//    CONTROL         "Show Major Differences",IDC_RADIO2,"Button",
//                    BS_AUTORADIOBUTTON,155,42,110,14
   CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1 );

//    GROUPBOX        "Display Options ",IDC_STATIC,17,29,262,49
//    GROUPBOX        "Write Options",IDC_STATIC,15,83,263,61
//    CONTROL         "&Line Numbers",IDC_CHECK1,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK1,     dwo, INCLUDE_LINENUMS  );

//    GROUPBOX        "Choose Style",IDC_STATIC,21,110,153,28
//    CONTROL         "Differences",IDC_RADIO5,"Button",BS_AUTORADIOBUTTON,29,
//    CONTROL         "All Lines",IDC_RADIO6,"Button",BS_AUTORADIOBUTTON,107,
   if( dwo & INCLUDE_DIFFER )
      dwi = IDC_RADIO6;
   else
      dwi = IDC_RADIO5;
   CheckRadioButton( hDlg, IDC_RADIO5, IDC_RADIO6, dwi );

//    CONTROL         "Add &Tags.",IDC_CHECK2,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK2,     dwo, INCLUDE_TAGS      );
//    CONTROL         "Append",IDC_CHECK3,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK3,    dwi, APPEND_FILE       );
//    CONTROL         "Extra Header",IDC_CHECK4,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK4,     dwo, INCLUDE_HEADER );
//    CONTROL         "Moved Lines",IDC_CHECK5,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK5,     dwo, INC_ALLMOVE );
//    CONTROL         "Wrap at ",IDC_CHECK6,"Button",BS_AUTOCHECKBOX | 
   CDB( IDC_CHECK6,     dwo, WRAP_LINES );

//    EDITTEXT        IDC_EDIT1,235,96,31,12,ES_AUTOHSCROLL
//END

}
 ======== */


VOID  ex_WM_INITDIALOG( HWND hDlg )
{
   DWORD dwi = gdwDiffOpts;   // = INC_ALLXSAME;   // everything excluding SAME lines

   sprtf( "ex_init: begin with gdwDiffOpts of %x"MEOR, dwi );

   // identical lines
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME );

   // differ line will be either left, right or moved
   //CDB( IDD_DIFFER,    ( dwi & INCLUDE_DIFFER   ) );
   CDB( IDD_LEFT,       dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,      dwi, INCLUDE_RIGHTONLY );
   CDB( IDC_MOVEDLEFT,  dwi, INCLUDE_MOVELEFT  );
   CDB( IDC_MOVEDRIGHT, dwi, INCLUDE_MOVERIGHT );

   // other otions
   CDB( IDC_CHECK1,     dwi, INCLUDE_LINENUMS  );
   CDB( IDC_CHECK2,     dwi, INCLUDE_TAGS      );
   CDB( IDC_CHECK4,     dwi, INCLUDE_HEADER    );
   CDB( IDC_CHECK6,     dwi, WRAP_LINES        );

   if( ( dwi & EXP_ALL ) == EXP_ALL )
   {
      CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1 );
   }
   else
   {
      CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2 );
   }

   //GetComboSel(hCtl, lpb, NULL);
   //if( dir_isvalidfile( lpb ) )  // if this is a VALID file
   //   CDB( IDC_CHECK3, TRUE, TRUE );   // set the APPEND regardless of the option
   //else
   CDB( IDC_CHECK3, dwi, APPEND_FILE ); // else set check per option

}


INT  ex_IDOK( HWND hDlg )
{
   INT   iRet = 0;
   DWORD dwi = gdwDiffOpts;   // = INC_ALLXSAME;   // everything excluding SAME lines
   //DWORD dwo = outline_include;

   // include SAME lines
   ICDB( IDD_IDENTICAL, dwi, INCLUDE_SAME     );

   // include DIFFER line. That is either left, right or moved
   //ICDB( IDD_DIFFER,    ( INCLUDE_DIFFER   ) );
   ICDB( IDD_LEFT,       dwi, INCLUDE_LEFTONLY );
   ICDB( IDD_RIGHT,      dwi, INCLUDE_RIGHTONLY );
   ICDB( IDC_MOVEDLEFT,  dwi, INCLUDE_MOVELEFT  );
   ICDB( IDC_MOVEDRIGHT, dwi, INCLUDE_MOVERIGHT );

   // other options
   ICDB( IDC_CHECK1,     dwi, INCLUDE_LINENUMS  );
   ICDB( IDC_CHECK2,     dwi, INCLUDE_TAGS      );
   ICDB( IDC_CHECK3,     dwi, APPEND_FILE       );
   ICDB( IDC_CHECK4,     dwi, INCLUDE_HEADER    );
   ICDB( IDC_CHECK6,     dwi, WRAP_LINES        );

   if( dwi != gdwDiffOpts )   // = INC_ALLXSAME;   // everything excluding SAME lines
   {

      sprtf( "Updated INI gdwDiffOPts from %x to %#x."MEOR,
         gdwDiffOpts,
         dwi );

      bChgDO = TRUE;
      gdwDiffOpts = dwi;   // = INC_ALLXSAME;   // everything excluding SAME lines
      // outline_include = dwo;
      iRet++;
   }

   return iRet;
}


TCHAR g_szActLeft[264]; // ,pleft);
TCHAR g_szActRight[264];   // ,pright);
//            dwi = AppdMsgTxt( pleft  );
//            dwi = dwi << 16;
//            dwi |= AppdMsgTxt( pright );
DWORD g_dwActComp;
BOOL  g_bOneShot2 = FALSE;  // posted IDM_DIR to bring up dialog box
// and now post an IDOK message to close it - JUST ONE TIME (AT START UP)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : FillComboBox
// Return type: BOOL 
// Arguments  : HWND hCtl
//            : PLE ph
//            : LPTSTR pdef
// Description: Fill the COMBO BOX control from the LLIST (PLE ph) supplied,
//              ensuring the default (LPTSTR pdef) is SELECTED if already
// in the linked list, or adding it at the TOP if it is NOT
///////////////////////////////////////////////////////////////////////////////
BOOL  FillComboBox( HWND hCtl, PLE ph, LPTSTR pdef )
{
   PLE      pn;
   LPTSTR   lps;
   LRESULT  lRes, lRes2;
   INT      icnt = 0;

   lRes2 = CB_ERR;

   SendMessage( hCtl, CB_RESETCONTENT, 0, 0 );

   Traverse_List( ph, pn )
   {
      lps = (LPTSTR)((PLE)pn + 1);
      lRes = SendMessage( hCtl, CB_ADDSTRING, 0, (LPARAM)lps );
      if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         return FALSE;
      icnt++;
      if(pdef)
      {
         if( strcmpi( lps, pdef ) == 0 )
            lRes2 = lRes;  // select this one
      }
   }
   if( lRes2 == CB_ERR )
   {
      // default string NOT found
      if( pdef && *pdef )
      {
         //lRes = SendMessage( hCtl, CB_ADDSTRING, 0, (LPARAM)pdef );
         // maybe INSERT this at the TOP
         lRes = SendMessage( hCtl, CB_INSERTSTRING, 0, (LPARAM)pdef );
         if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
            return FALSE;
         icnt++;
      }
   }

   if( icnt == 0 )
   {
      // we MUST add at least ONE string
      lps = &g_szBuf1[0];
      *lps = 0;
      _getcwd( lps, 256 );
      if( *lps )
         strcat(lps, "\\");
      strcat(lps, "TEMPD001.TXT");
      GetNxtDif(lps);
      lRes = SendMessage( hCtl, CB_ADDSTRING, 0, (LPARAM)lps );
      if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         return FALSE;
      icnt++;
      lRes2 = lRes;
   }

   if( lRes2 == CB_ERR )
      lRes2 = 0;

   // SELECT A STRING - That is copy it to the EDIT control that is part of the COMBO
   SendMessage( hCtl, CB_SETCURSEL, lRes2, 0 );

   return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : GetComboSel
// Return type: BOOL 
// Arguments  : HWND hCtl
//            : LPTSTR lpb
//            : LRESULT * pres
// Description: Return the current SELECTION, either from the COMBO list, 
//              or from the COMBO EDIT control in the supplied buffer.
// Also return the INDEX of the item (if LRESULT * pres supplied)
///////////////////////////////////////////////////////////////////////////////
BOOL  GetComboSel( HWND hCtl, LPTSTR lpb, LRESULT * pres )
{
   LRESULT  lRes = SendMessage( hCtl, CB_GETCURSEL, 0, 0 );
   *lpb = 0;
   if( lRes == CB_ERR )
   {
      if(pres)
         *pres = lRes;  // return index of selection in ONE
      // there is NO SELECTION - the user has typed a NEW entry in the edit, so
      // For a combo box, the text is the content of the edit control
      // (or static-text) portion of the combo box.
      lRes = SendMessage( hCtl, WM_GETTEXT, (WPARAM) 256, (LPARAM) lpb ); // text buffer
   }
   else
   {
      if(pres)
         *pres = lRes;  // return index of selection in ONE
      // we have a SELECTION, so get the text of the SELECTION
      lRes = SendMessage( hCtl, CB_GETLBTEXT, lRes, (LPARAM) lpb );  // receives string
   }
   if( ( lRes == CB_ERR ) ||
       ( *lpb == 0      ) )
   {
      return FALSE;
   }

   return TRUE;

}

// IDC_COMBO_SAVE and IDC_EXISTS
static TCHAR    _s_szVerFile[264] = { "\0" };
static BOOL     _s_bChkAppend = FALSE;

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : AddAppPrompt
// Return type: VOID 
// Arguments  : HWND hDlg
//            : UINT uiCombo
//            : UINT uiLab
// Description: Get the current COMBO selection, and check if this file
//              EXIST, and set an EDIT/LABEL with the results.
///////////////////////////////////////////////////////////////////////////////
VOID     AddAppPrompt( HWND hDlg, UINT uiCombo, UINT uiLab )
{
   HWND     hCtl = GetDlgItem( hDlg, uiCombo );
   LPTSTR   lpb  = GetNxtBuf(); //&gszTmpBuf[0];
   LPTSTR   lpm  = &g_szMsg[0];
   HANDLE   h;    // = CreateFile( lpb, // file name
   // SET_PROP( hDlg, DIFF_ATOM, pta );
   PTARGS   pta  = GET_PROP( hDlg, DIFF_ATOM );
   PDDIFF   pdd  = &pta->ta_sDI;
   DWORD    dwo = 0;    //pdd->di_dwOpts;
   // CDB( IDC_CHECK3, dwo, APPEND_FILE  );

   ICDB( IDC_CHECK3,     dwo, APPEND_FILE  );

   *lpb = 0;
   *lpm = 0;
   if( GetComboSel( hCtl, lpb, 0 ) )
   {
        if( strcmpi( _s_szVerFile, lpb ) ) {
            strcpy( _s_szVerFile, lpb );   // update file name
            sprtf("AddAppPrompt: Got GetComboSel: [%s]"MEOR, lpb);
        }
        // FIX20091117 - Always check the FILE exists
        h = CreateFile( lpb, // file name
            GENERIC_READ,  // access mode
            FILE_SHARE_READ,  // share mode
            0, // SD
            OPEN_EXISTING, // how to create
            FILE_ATTRIBUTE_NORMAL,  // file attributes
            0 ); // handle to template file
        if(VFH(h)) {
            //strcpy(lpm, "CHECK Append!");
            CloseHandle(h);
            _s_bChkAppend = TRUE;
        } else {
            //strcpy(lpm, "**New File**");
            _s_bChkAppend = FALSE;
        }

        // FIX20091117 - Change messages to 3 - Append, Overwrite, or New
        if( _s_bChkAppend ) {
            if (dwo & APPEND_FILE)
                strcpy(lpm, "Append CHECKED!");
            else
                strcpy(lpm, "**Overwrite**");
        } else
            strcpy(lpm, "**New File**");

   } else {
       sprtf("AddAppPrompt: FAILED in GetComboSel:"MEOR);
   }
   SetDlgItemText(hDlg, uiLab, lpm);
}

// DIFF - IDM_WRITEDIFF - DIALOG BOX
// =================================
//#define SET_PROP( x, y, z )  SetProp( x, MAKEINTATOM(y), z )
//#define GET_PROP( x, y )     GetProp( x, MAKEINTATOM(y) )
//#define REMOVE_PROP( x, y )  RemoveProp( x, MAKEINTATOM(y) )
//#define  DIFF_ATOM      0x1010

// IDD_DLG_DIFF
INT_PTR  diff_INIT(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 1;
   PTARGS   pta  = (PTARGS)lParam;
   PDDIFF   pdd;
	//LPTSTR	lpd;
   LPTSTR   lpf;
   LPTSTR   lpb = GetNxtBuf();  // &gszTmpBuf[0];
   PVIEW    view;
   DWORD    dwo, dwi;

   if( !pta )
   {
      EndDialog( hDlg, FALSE );
      return 0;
   }

   SET_PROP( hDlg, DIFF_ATOM, pta );

   pdd = &pta->ta_sDI;
   dwo = pdd->di_dwOpts;

   //lpd = &pdd->di_szCWD[0];
   //if( !lpd[0] )
   _getcwd( lpb, 256 );
	SetDlgItemText( hDlg, IDC_ED_CWD, lpb );

   lpf = &pdd->di_szFile[0];
   if( *lpf == 0 )
   {
      Setg_szNxtDif();
      strcpy(lpf, g_szNxtDif);   // ensure we SUGGEST a file name
   }
	//SetDlgItemText( hDlg, IDC_ED_SAVE, lpf );
   if( !FillComboBox( GetDlgItem(hDlg, IDC_COMBO_SAVE), &gsDiffList, lpf ) )
   {
      EndDialog( hDlg, FALSE );
      return 0;
   }

   //CheckDlgButton( hDlg, IDC_CHECK1,
   //   ( pdd->di_bIncLn ? BST_CHECKED : BST_UNCHECKED ) );
   CDB( IDC_CHECK1,     dwo, INCLUDE_LINENUMS  );
   CDB( IDC_CHECK3,     dwo, APPEND_FILE  );
   CDB( IDC_CHECK2,     dwo, INCLUDE_TAGS      );
   CDB( IDC_CHECK4,     dwo, INCLUDE_HEADER );
   CDB( IDC_CHECK5,     dwo, INC_ALLMOVE );
   CDB( IDC_CHECK6,     dwo, WRAP_LINES );

   if( dwo & INCLUDE_DIFFER )
      dwi = IDC_RADIO1;
   else
      dwi = IDC_RADIO2;
   CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO2, dwi );
   //   ( pdd->di_bDiff ? IDC_RADIO2 : IDC_RADIO1 ) );

   sprintf(lpb, "%d", pdd->di_dwWidth );
   SetDlgItemText(hDlg, IDC_EDIT1, lpb);

   *lpb = 0;
#ifdef   COMBARGS
   view = pta->ta_sTA.view;
#else // !#ifdef   COMBARGS
   view = pta->ta_pView;
#endif   // #ifdef   COMBARGS
   if( view )
   {
      LPTSTR   pstr;
      INT      idiff = view_haschange( view, TRUE );
      LPTSTR   lp1, lp2;
      if( view_isexpanded(view) )
      {
         /* set names of left and right files
          * NOTE in EXPANDED view_getitem(row is ignored)
          */
         COMPITEM ci = view_getitem(view, 0);
#ifdef   COMBARGS
         lp1 = &pta->ta_sTA.szFirst[0];
         lp2 = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
         lp1 = &pta->ta_szFirst[0];
         lp2 = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
         if( ( *lp1 ) &&
             ( *lp2 ) )
         {
            DWORD          i, j;
            LARGE_INTEGER  li;
            SYSTEMTIME     st;
            i = strlen(lp2);
            if( strlen(lp1) > i )
               i = strlen(lp1);     // get length of longest;
            i++;
            // set first / left file name
            strcpy(lpb, lp1);
            while( strlen(lpb) < i )
               strcat(lpb, " ");
            strcat(lpb, " ");
            // TIME TROUBLE - This is NOT sufficient
            //FileTimeToSystemTime( &pta->ta_sFDL.ftLastWriteTime, &st );
            FT2LST( &pta->ta_sFDL.ftLastWriteTime, &st );
            AppendDateTime( lpb, &st );
            strcat(lpb, " ");
            li.LowPart  = pta->ta_sFDL.nFileSizeLow;
            li.HighPart = pta->ta_sFDL.nFileSizeHigh;
            pstr = GetI64Stg( &li );
            j = strlen(pstr);
            while( j < 10 )
            {
               strcat(lpb, " ");
               j++;
            }
            strcat(lpb, pstr);
            SetDlgItemText( hDlg, IDC_CMPLEFT, lpb );

            // set second / right file name
            strcpy(lpb, lp2);
            while( strlen(lpb) < i )
               strcat(lpb, " ");
            strcat(lpb, " ");
            // TIME TROUBLE - This is NOT sufficient
            //FileTimeToSystemTime( &pta->ta_sFDR.ftLastWriteTime, &st );
            FT2LST( &pta->ta_sFDR.ftLastWriteTime, &st );
            AppendDateTime( lpb, &st );
            strcat(lpb, " ");
            li.LowPart  = pta->ta_sFDR.nFileSizeLow;
            li.HighPart = pta->ta_sFDR.nFileSizeHigh;
            pstr = GetI64Stg( &li );
            j = strlen(pstr);
            while( j < 10 )
            {
               strcat(lpb, " ");
               j++;
            }
            strcat(lpb, pstr);
            SetDlgItemText( hDlg, IDC_CMPRITE, lpb );
         }
         else if(ci)
         {
            pstr = compitem_getfilename(ci, CI_LEFT);
            if(pstr)
               SetDlgItemText( hDlg, IDC_CMPLEFT, pstr );
            compitem_freefilename(ci, CI_LEFT, pstr);
            pstr = compitem_getfilename(ci, CI_RIGHT);
            if(pstr)
               SetDlgItemText( hDlg, IDC_CMPRITE, pstr );
            compitem_freefilename(ci, CI_RIGHT, pstr);
         }

         if(idiff)
         {
            if(idiff > 1)
               sprintf(lpb, "Compare, with %d differences:", idiff );
            else
               strcpy(lpb, "Compare, with 1 difference:" );
         }
         else
            strcpy(lpb, "Compare has NO differences:");

         SetDlgItemText( hDlg, IDC_CMPOF, lpb );
         strcpy(lpb, "EXPANDED");
      }
      else
      {
         COMPLIST cl = view_getcomplist(view);
         if( cl )
         {
            /* set root names of left and right trees */
            pstr = complist_getroot_left(cl);
            if(pstr)
               SetDlgItemText( hDlg, IDC_CMPLEFT, pstr );
            complist_freeroot_left(cl, pstr);
            pstr = complist_getroot_right(cl);
            if(pstr)
               SetDlgItemText( hDlg, IDC_CMPRITE, pstr );
            complist_freeroot_right(cl, pstr);
         }
         if(idiff)
         {
            if(idiff > 1)
               sprintf(lpb, "Compare, with %d differences:", idiff );
            else
               strcpy(lpb, "Compare, with 1 difference:" );
         }
         else
            strcpy(lpb, "Compare has NO differences:");
         SetDlgItemText( hDlg, IDC_CMPOF, lpb );
         strcpy(lpb, "OUTLINE");
      }
   }

   SetDlgItemText( hDlg, IDC_CURMODE, lpb );


   AddAppPrompt( hDlg, IDC_COMBO_SAVE, IDC_EXISTS );

   SetFocus( GetDlgItem(hDlg, IDC_COMBO_SAVE) );

   iRet = 0;

   return iRet;

}

INT_PTR  diff_IDOK( HWND hDlg, PDDIFF pdd )
{
   if( pdd )
   {
      LPTSTR   lpb = &gszTmpBuf[0];
      LPTSTR   lpf = &pdd->di_szFile[0];
      DWORD    dwo = pdd->di_dwOpts;

      ICDB( IDC_CHECK1,     dwo, INCLUDE_LINENUMS  );
      ICDB( IDC_CHECK3,     dwo, APPEND_FILE  );
      ICDB( IDC_CHECK2,     dwo, INCLUDE_TAGS      );
      ICDB( IDC_CHECK4,     dwo, INCLUDE_HEADER );
      ICDB( IDC_CHECK5,     dwo, INC_ALLMOVE );
      ICDB( IDC_CHECK6,     dwo, WRAP_LINES );

      /// *** TBC *** checkme - this code does NOT look right
      if( IsDlgButtonChecked( hDlg, IDC_RADIO1 ) == BST_CHECKED )
      {
         dwo |= INCLUDE_DIFFER;
         dwo &= ~(INCLUDE_SAME);
      }
      else
      {
         dwo |= INCLUDE_DIFFER;
         dwo |= INCLUDE_SAME;
      }

      pdd->di_dwOpts = dwo;

      GetDlgItemText( hDlg, IDC_EDIT1, lpb, 256 );
      pdd->di_dwWidth = atoi(lpb);

      //GetDlgItemText( hDlg, IDC_ED_SAVE, lpf, 256 );
      if( GetComboSel( GetDlgItem(hDlg, IDC_COMBO_SAVE), lpf, 0 ) )
         EndDialog( hDlg, IDOK );   // return IDOK

   }
//   else
//   {
//      EndDialog( hDlg, FALSE );
//   }
   return TRUE;
}

INT_PTR  diff_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 0;
   PTARGS   pta  = GET_PROP( hDlg, DIFF_ATOM );
   PDDIFF   pdd  = &pta->ta_sDI;
   DWORD    cmd  = LOWORD(wParam);
   switch(cmd)
   {
   case IDOK:
      iRet = diff_IDOK( hDlg, pdd );
      break;

   case IDCANCEL:
      EndDialog( hDlg, FALSE );
      break;

   case IDC_BUT_SRCH:
      // ***TBD***
      break;

   case IDC_COMBO_SAVE:
      AddAppPrompt( hDlg, IDC_COMBO_SAVE, IDC_EXISTS );
      break;

   }
   return iRet;
}
INT_PTR  diff_CLOSE(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 0;
   EndDialog( hDlg, FALSE );
   return iRet;
}

INT_PTR  diff_DESTROY(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 0;
   REMOVE_PROP( hDlg, DIFF_ATOM );
   return iRet;
}

INT_PTR CALLBACK DIFFDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = 0;
   switch(uMsg)
   {
   case WM_INITDIALOG:
      iRet = diff_INIT(hDlg,wParam,lParam);
      break;
   case WM_COMMAND:
      iRet = diff_COMMAND(hDlg,wParam,lParam);
      break;
   case WM_CLOSE:
      iRet = diff_CLOSE(hDlg,wParam,lParam);
      break;
   case WM_DESTROY:
      iRet = diff_DESTROY(hDlg,wParam,lParam);
      break;
   }

   return iRet;
}

INT_PTR  Do_DIFF_Dlg( PVOID pta ) // actually is PTARGS pta
{
   INT_PTR   iOK;
   dc4w_UI(TRUE);
   iOK = DialogBoxParam(
         g_hInst,      // handle to module
         MAKEINTRESOURCE(IDD_DLG_DIFF),   // dialog box template
         hwndClient,    // handle to owner window
         DIFFDLGPROC,   // dialog box procedure
         (LPARAM)pta ); // initialization value
   dc4w_UI(FALSE);
   return iOK;
}
// END DIFF DIALOG
// ===================================

// Copy from RC file
// BEGIN IDD_SAVEDIFF DIALOG
//IDD_SAVEDIFF DIALOG DISCARDABLE  0, 0, 256, 169
//STYLE DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
//CAPTION "SAVE DIFFERENCES TO FILE"
//FONT 8, "MS Sans Serif"
//BEGIN
//    LTEXT           "Aim is to write a list of differences to a file for 
//further processing.",
//                    IDC_STATIC,7,6,199,8
//    LTEXT           "Save to &file:",IDM_INCSAME,7,18,39,8
//    LTEXT           "Include:",IDM_INCDIFFER,7,33,27,8
//    CONTROL         "&Identical lines",IDD_IDENTICAL,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,46,89,10
//    CONTROL         "Lines only in &left file",IDD_LEFT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,70,89,10
//    CONTROL         "Lines only in &right file",IDD_RIGHT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,82,89,10
//    LTEXT           "",IDC_LABCWD,37,33,210,8
//    EDITTEXT        IDC_EDIT1,7,117,242,28,ES_MULTILINE | ES_AUTOVSCROLL | 
//                    ES_READONLY | NOT WS_TABSTOP
//    PUSHBUTTON      "Cancel",IDCANCEL,135,148,40,14
//    DEFPUSHBUTTON   "OK",IDOK,203,148,40,14
//    CONTROL         "Lines moved in l&eft file",IDC_MOVEDLEFT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,94,89,10
//    CONTROL         "Lines moved in r&ight file",IDC_MOVEDRIGHT,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,106,89,10
//    CONTROL         "Add &Line numbers.",IDC_CHECK1,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,121,46,75,10
//    CONTROL         "Add &Tag strings.",IDC_CHECK2,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,121,58,67,10
//    CONTROL         "Append to File",IDC_CHECK3,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,121,95,61,10
//    LTEXT           "",IDC_LABWARN,121,107,101,8
//    CONTROL         "Add &Detail Header",IDC_CHECK4,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,121,70,74,10
//    CONTROL         "Wrap at ",IDC_CHECK6,"Button",BS_AUTOCHECKBOX | 
//                    WS_TABSTOP,121,82,43,10
//    EDITTEXT        IDC_EDIT2,168,81,40,12,ES_AUTOHSCROLL
//    COMBOBOX        IDC_COMBO_SAVE,52,18,197,132,CBS_DROPDOWN | WS_VSCROLL | 
//                    WS_TABSTOP
//END

//VOID  sd_WARN( HWND hDlg, LPTSTR lpf )
//{
//   if( dir_isvalidfile( lpf ) )  // if this is a VALID file
//      SetDlgItemText( hDlg, IDC_LABWARN, "*** File Exists ***" );
//   else
//      SetDlgItemText( hDlg, IDC_LABWARN, "=== No such file ===" );
//}

INT_PTR  sd_INIT(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR     iRet = 1;
   PCFDLGSTR   pcfds = (PCFDLGSTR)lParam;
   LPTSTR      lpb2 = &gszTmpBuf2[0];
   LPTSTR      lpb;   // message buffer
   HWND        hCtl;
   DWORD       dwi;  //  = gdwDiffOpts;
   PLE         ph;

   if( !VALIDPCFDS(pcfds) )
   {
      EndDialog(hDlg, -1);
      return -1;
   }

   //hCtl  = GetDlgItem( hDlg, IDC_COMBO_FILE );
   hCtl  = GetDlgItem( hDlg, IDC_COMBO_SAVE );
   ph = pcfds->cf_sTARGS.ta_sDI.di_pDFList;     // extract the LIST
   if( !hCtl || !ph ||
      (!FillComboBox(hCtl,ph, &pcfds->cf_sTARGS.ta_sDI.di_szFile[0])) )
   {
      EndDialog(hDlg, -2);
      return -1;
   }

   SET_PROP( hDlg, DIFF_ATOM, pcfds );

   // this should be last, or near last
   //CenterDialog( hDlg, hwndClient );   // centre it on the client

   lpb  = &pcfds->cf_szMsg[0];   // message buffer
   if( lpb && *lpb )
      SetDlgItemText(hDlg, IDC_EDIT1, lpb);
   else
      ShowWindow( GetDlgItem(hDlg, IDC_EDIT1), SW_HIDE );

   lpb = &gszTmpBuf[0];
   _getcwd(lpb, 256);
   sprintf(lpb2, "CWD=[%s]", lpb);
   SetDlgItemText(hDlg, IDC_LABCWD, lpb2);

   //sd_WARN(hDlg, lpb);
   dwi = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;     // extract the OPTIONS

   // identical lines
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME );

   // differ line will be either left, right or moved
   //CDB( IDD_DIFFER,    ( dwi & INCLUDE_DIFFER   ) );
   CDB( IDD_LEFT,       dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,      dwi, INCLUDE_RIGHTONLY );
   CDB( IDC_MOVEDLEFT,  dwi, INCLUDE_MOVELEFT  );
   CDB( IDC_MOVEDRIGHT, dwi, INCLUDE_MOVERIGHT );

   // other otions
   CDB( IDC_CHECK1,     dwi, INCLUDE_LINENUMS  );
   CDB( IDC_CHECK2,     dwi, INCLUDE_TAGS      );
   CDB( IDC_CHECK4,     dwi, INCLUDE_HEADER    );
   CDB( IDC_CHECK6,     dwi, WRAP_LINES        );


   GetComboSel(hCtl, lpb, NULL);

   if( dir_isvalidfile( lpb ) )  // if this is a VALID file
      CDB( IDC_CHECK3, TRUE, TRUE );   // set the APPEND regardless of the option
   else
      CDB( IDC_CHECK3, dwi, APPEND_FILE ); // else set check per option

   sprintf(lpb, "%d", pcfds->cf_sTARGS.ta_sDI.di_dwWidth );
   SetDlgItemText(hDlg, IDC_EDIT2, lpb);

   bDnDlgInit = TRUE;

   AddAppPrompt( hDlg, IDC_COMBO_SAVE, IDC_LABWARN );

   // this should be last, or near last
   CenterDialog( hDlg, hwndClient );   // centre it on the client

   //SetFocus( GetDlgItem(hDlg, IDC_COMBO_SAVE) );
   SetFocus(hCtl);
   iRet = 0;

   return iRet;
}


BOOL  sd_IDOK( HWND hDlg )
{
   DWORD       dwi;  // = gdwDiffOpts;
   PCFDLGSTR   pcfds = GET_PROP( hDlg, DIFF_ATOM );
   LPTSTR      lpb = &gszTmpBuf[0];
   LPTSTR      lpf;

   if( !VALIDPCFDS(pcfds) )
      return FALSE;

   lpf = &pcfds->cf_sTARGS.ta_sDI.di_szFile[0];
   *lpf = 0;
   GetComboSel( GetDlgItem(hDlg, IDC_COMBO_SAVE), lpf, NULL );
   if( *lpf == 0 )
      return FALSE;

   dwi = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;  // extract the OPTIONS

   // include SAME lines
   ICDB( IDD_IDENTICAL, dwi, INCLUDE_SAME     );

   // include DIFFER line. That is either left, right or moved
   //ICDB( IDD_DIFFER,    ( INCLUDE_DIFFER   ) );
   ICDB( IDD_LEFT,       dwi, INCLUDE_LEFTONLY );
   ICDB( IDD_RIGHT,      dwi, INCLUDE_RIGHTONLY );
   ICDB( IDC_MOVEDLEFT,  dwi, INCLUDE_MOVELEFT  );
   ICDB( IDC_MOVEDRIGHT, dwi, INCLUDE_MOVERIGHT );

   // other options
   ICDB( IDC_CHECK1,     dwi, INCLUDE_LINENUMS  );
   ICDB( IDC_CHECK2,     dwi, INCLUDE_TAGS      );
   ICDB( IDC_CHECK3,     dwi, APPEND_FILE       );
   ICDB( IDC_CHECK4,     dwi, INCLUDE_HEADER    );
   ICDB( IDC_CHECK6,     dwi, WRAP_LINES        );

   pcfds->cf_sTARGS.ta_sDI.di_dwOpts = dwi;  // set any changed OPTIONS

   GetDlgItemText( hDlg, IDC_EDIT2, lpb, 256 );
   pcfds->cf_sTARGS.ta_sDI.di_dwWidth = atoi(lpb);

   return TRUE;

}

// EDIT CONTROL NOTIFICATION MESSAGES
// EN_CHANGE, EN_ERRSPACE, EN_HSCROLL, EN_KILLFOCUS
// EN_MAXTEXT, EN_SETFOCUS, EN_UPDATE, EN_VSCROLL
//VOID  sd_IDD_FILE( HWND hDlg, WPARAM wParam, LPARAM lParam )
//{
   // EDIT control update /change notification
   // The wParam parameter specifies the identifier of the edit control,
   // in the low-order word and the notification code in the high-order word.
   // and the lParam parameter specifies the handle to the edit control
//   DWORD msg = HIWORD(wParam);   // get the notification
//   if( ( msg == EN_CHANGE ) ||
//       ( msg == EN_SETFOCUS ) )
//   {
//      LPTSTR lpb = &gszTmpBuf[0];
      //GetDlgItemText( hDlg, IDD_FILE, lpb, 256 );
      // OR
//      SendMessage( (HWND)lParam, WM_GETTEXT, (WPARAM)256, (LPARAM)lpb );
//      sd_WARN( hDlg, lpb );
//   }
//}

INT_PTR  sd_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 1;
   DWORD    cmd = LOWORD(wParam);
   switch(cmd)
   {
   case IDOK:
      bDnDlgInit = FALSE;
      if( !sd_IDOK( hDlg ) )
         break;
   case IDCANCEL:
      bDnDlgInit = FALSE;
      EndDialog(hDlg, cmd);
      break;
   //case IDD_FILE:
   //   sd_IDD_FILE( hDlg, wParam, lParam );
   //   break;
   case IDC_COMBO_SAVE:
      if( bDnDlgInit )
         AddAppPrompt(hDlg, IDC_COMBO_SAVE, IDC_LABWARN );
      break;

   }
   return iRet;
}
INT_PTR  sd_CLOSE(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 1;
   EndDialog(hDlg, FALSE);
   return iRet;
}
INT_PTR  sd_DESTROY(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR  iRet = 1;
   REMOVE_PROP( hDlg, DIFF_ATOM );
   return iRet;
}

INT_PTR CALLBACK SAVEDIFFPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = 0;
   switch(uMsg)
   {
   case WM_INITDIALOG:
      iRet = sd_INIT(hDlg,wParam,lParam);
      break;
   case WM_COMMAND:
      iRet = sd_COMMAND(hDlg,wParam,lParam);
      break;
   case WM_CLOSE:
      iRet = sd_CLOSE(hDlg,wParam,lParam);
      break;
   case WM_DESTROY:
      iRet = sd_DESTROY(hDlg,wParam,lParam);
      break;
   }

   return iRet;
}

INT_PTR  Do_IDD_SAVEDIFF( PCFDLGSTR pcfds )
{
   INT_PTR   iOK;
   dc4w_UI(TRUE);
   {
      bDnDlgInit = FALSE;
      iOK = DialogBoxParam(
         g_hInst,      // handle to module
         MAKEINTRESOURCE(IDD_SAVEDIFF),   // dialog box template
         hwndClient,    // handle to owner window
         SAVEDIFFPROC,   // dialog box procedure
         (LPARAM)pcfds ); // initialization value
   }
   dc4w_UI(FALSE);
   return iOK;
}
// END   IDD_SAVEDIFF DIALOG

// BEGIN EXLUDED DIALOG
// ======================================
extern   LPTSTR   GetxStg( PLE pN );
extern   PVOID    Add2Exclude( PLE pH, LPTSTR lpb );
extern   BOOL     IsXDeleted( PLE pN );
//extern   BOOL     DeleteXStg( LPTSTR lpb );
//extern   BOOL     CopyXList( PLE pCopy, INT inum );
//extern   BOOL     CompareXList( PLE pCopy );
//extern   BOOL     CopyXList2( PLE pCopy, PLE pH ); // from pH to pCopy
static   BOOL     bDlg_Excl;
static   LRESULT  iDlg_XSel;
static   BOOL     bDlg_XChg;

INT   InitListBox( HWND hwnd )
{
   INT      icnt = 0;
   LPTSTR   lpb = &gszTmpBuf[0];
   PLE      pH, pN;
   LPTSTR   lps;
   LRESULT  lr;
   INT      i;

   SendMessage( hwnd, LB_RESETCONTENT, 0, 0 );
   
   for( i = 0; i < MXXLSTS; i++ )
   {
      pH = &gsExclLists[i];
      *lpb = 0;
      Traverse_List( pH, pN )
      {
         if( !IsXDeleted(pN) )
         {
            lps = GetxStg(pN);
            if( *lpb )
               strcat(lpb, ";");
            strcat(lpb,lps);
         }
      }

      if( *lpb == 0 )
         strcpy(lpb, "<blank>");

      {
         lr = SendMessage( hwnd,   // handle to destination window 
               LB_INSERTSTRING,          // message to send
               (WPARAM)-1,          // item index
               (LPARAM) lpb );      // string (LPCTSTR)
         if( ( lr == LB_ERR ) ||
             ( lr == LB_ERRSPACE ) )
         {
            return (INT)-1;
         }
         icnt++;
      }
   }
   pH = &gsXFileList;   // the EXCLUDE FILE LIST
   *lpb = 0;
   Get_Excl_List( lpb, pH );
   if(*lpb == 0)
      strcpy(lpb,"<blank>");
   {
         lr = SendMessage( hwnd,   // handle to destination window 
               LB_INSERTSTRING,          // message to send
               (WPARAM)-1,          // item index
               (LPARAM) lpb );      // string (LPCTSTR)
         if( ( lr == LB_ERR ) ||
             ( lr == LB_ERRSPACE ) )
         {
            return (INT)-1;
         }
         icnt++;
   }

   pH = &gsXDirsList; // = EXCLUDE these DIRECTORIES -xd:Scenery
   *lpb = 0;
   Get_Excl_List( lpb, pH );
   if(*lpb == 0)
      strcpy(lpb,"<blank>");
   {
         lr = SendMessage( hwnd,   // handle to destination window 
               LB_INSERTSTRING,          // message to send
               (WPARAM)-1,          // item index
               (LPARAM) lpb );      // string (LPCTSTR)
         if( ( lr == LB_ERR ) ||
             ( lr == LB_ERRSPACE ) )
         {
            return (INT)-1;
         }
         icnt++;
   }


   if( ( iDlg_XSel ) &&  // do we have a SELECTION index
       ( iDlg_XSel <= MXXLSTS ) )
   {
      lr = iDlg_XSel - 1;  // get the zero based index
      // IF multiple selection LIST BOX, use
      //SendMessage( hwnd, LB_SETSEL, (WPARAM) TRUE, (LPARAM) lr ); // item index, else
      SendMessage( hwnd, LB_SETCURSEL, (WPARAM)lr, 0 );  // in SINGLE selection LIST BOX

      sprtf( "LB_SETSEL: Selection set to index %d."MEOR, lr );
   }

   return icnt;
}

// IDM_EDITEXCLUDE - MAKEINTRESOURCE(IDD_DLG_EXCLUDE),   // dialog box template
INT_PTR  excl_INIT( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT     iRet = 0;
   HWND     hwnd;
   LPTSTR   lpb = &gszTmpBuf[0];


   // do this near the END
   //CenterDialog( hDlg, hwndClient );

   hwnd = GetDlgItem( hDlg, IDC_LIST1 );
   if( !hwnd )
   {
      EndDialog(hDlg, -1);
      return (INT_PTR)-1;
   }

   sprtf( "Initialising LIST BOX %#x id=%d..."MEOR, hwnd, IDC_LIST1 );
   if( InitListBox( hwnd ) == (INT)-1 )
   {
      EndDialog(hDlg, -2);
      return (INT_PTR)-1;
   }

   hwnd = GetDlgItem( hDlg, IDC_LIST3 );
   if( !hwnd )
   {
      EndDialog(hDlg, -3);
      return (INT_PTR)-1;
   }

   sprtf( "Initialising LIST BOX %#x id=%d..."MEOR, hwnd, IDC_LIST3 );
   iRet = InitExcludeList( hwnd );
   if( iRet == (INT)-1 )
   {
      EndDialog(hDlg, -4);
      return (INT_PTR)-1;
   }
   else if( iRet )
   {
      *lpb = 0;
      GetDlgItemText(hDlg, IDC_CUR_SEL, lpb, 256);

      sprintf(EndBuf(lpb), MEOR"Current excluded items %d"MEOR
         "shown in second list below!"MEOR,
         iRet );

      SetDlgItemText(hDlg, IDC_CUR_SEL, lpb );
   }


   CheckDlgButton(hDlg, IDC_CHECK1,
      ( bDlg_Excl ? BST_CHECKED : BST_UNCHECKED ) );

   sprtf( "Done dialog %#x Initialisation ..."MEOR, hDlg );

   // do this near the END
   CenterDialog( hDlg, hwndClient );

   return TRUE;   // to 'select' the default OK button
}

LRESULT  GetCurSel( HWND hDlg, HWND hwnd )
{
   LRESULT  lr, lr2;
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb2 = &gszTmpBuf2[0];
   if( !hwnd )
      hwnd = GetDlgItem(hDlg, IDC_LIST1);
   lr = SendMessage( hwnd, // handle to destination window 
      LB_GETCURSEL,             // message to send
      0, 0 );  // not used; must be zero
   if( ( lr != LB_ERR ) &&
       ( lr < MXXLSTS ) )
   {
      *lpb2 = 0;
      lr2 = SendMessage( hwnd, LB_GETTEXT,
         (WPARAM)lr,   // item index
         (LPARAM)lpb2 );  // buffer for items (LPTSTR)

      if( lr2 && ( lr2 != LB_ERR ) )
         iDlg_XSel = (lr + 1);   // set the SELECTION (if it has text)

      sprintf( lpb, "Selection=%lld (%s)%lld", (lr+1), lpb2, lr2 );
   }
   else if(( lr == MXXLSTS ) ||
      ( lr == (MXXLSTS + 1)) )
   {
      *lpb2 = 0;
      lr2 = SendMessage( hwnd, LB_GETTEXT,
         (WPARAM)lr,   // item index
         (LPARAM)lpb2 );  // buffer for items (LPTSTR)

      if( lr2 && ( lr2 != LB_ERR ) )
         iDlg_XSel = (lr + 1);   // set the SELECTION (if it has text)

      sprintf( lpb, "Selection=%lld (%s)%lld", (lr+1), lpb2, lr2 );

   }
   else
   {
      strcpy( lpb, "ERROR: Appears NO SELECTION!" );
   }

   SetDlgItemText(hDlg, IDC_CUR_SEL, lpb );

   sprtf( "GetCurSel(): %s"MEOR, lpb );

   return lr;
}

LPTSTR   g_pInTitle = "ADD/MODIFY FILE/FOLDER NAME(S)";
PTSTR g_pExclInfo = "Enter or Modify List, each separated by ';'"MEOR
"(Use <blank>, to clear a list)";

INT_PTR  excl_IDC_BUTADD( HWND hDlg )
{
   INT_PTR  iRet = 0;
   LPTSTR   lpb  = &gszTmpBuf[0];
   LPTSTR   lpb2 = &gszTmpBuf2[0];
   PTSTR lpb3 = GetStgBuf(); // get a string buffer, 1 of many
   INT      iPos, iCnt;
   LRESULT  lr;
   PLE      pH, pN;
   LPTSTR   lps;
   HWND     hwnd = GetDlgItem(hDlg, IDC_LIST1);

   *lpb = 0;
   lr = GetCurSel( hDlg, hwnd );
   if( ( lr != LB_ERR ) &&
       ( lr < MXXLSTS ) )
   {
      pH = &gsExclLists[lr];
      *lpb = 0;
      Traverse_List( pH, pN )
      {
         if( !IsXDeleted(pN) )
         {
            lps = GetxStg(pN);
            if( *lpb )
               strcat(lpb, ";");
            strcat(lpb,lps);
         }
      }

      *lpb2 = 0;     // no return string yet
      sprintf(lpb3, "gsExclLists[%lld], item %lld"MEOR
          "%s", lr, (lr + 1), g_pExclInfo );
      iRet = StringInput( lpb2, 256, lpb3, g_pInTitle,
         lpb );

   }
   else if( lr == MXXLSTS )
   {
      pH = &gsXFileList;   // the EXCLUDE FILE LIST
      *lpb = 0;
      Get_Excl_List( lpb, pH );
      *lpb2 = 0;     // no return string yet
      sprintf(lpb3, "gsXFileList, item %lld"MEOR
          "%s", (lr + 1), g_pExclInfo );
      iRet = StringInput( lpb2, 256, lpb3, g_pInTitle,   // "ADD/MODIFY FILE/FOLDER NAME(S)",
         lpb );
      if( iRet && *lpb2 && strcmpi(lpb,lpb2) )
      {
         KillLList( pH );
         bDlg_XChg = TRUE;    // FIX20070525 - changing one of the LISTS
         if( strcmp( lpb2, "<blank>" ) ) {
             iCnt = Add_List_2_List( pH, lpb2 );
         } else {
             iCnt = 0;
         }
         InitListBox( hwnd );
      }
      return iRet;
   }
   else if( lr == (MXXLSTS+1) )
   {
      pH = &gsXDirsList; // = EXCLUDE these DIRECTORIES -xd:Scenery
      *lpb = 0;
      Get_Excl_List( lpb, pH );
      *lpb2 = 0;     // no return string yet
      sprintf(lpb3, "gsXDirsList, item %lld"MEOR
          "%s", (lr + 1), g_pExclInfo );
      iRet = StringInput( lpb2, 256, lpb3, g_pInTitle,
         lpb );

      if( iRet && *lpb2 && strcmpi(lpb,lpb2) )
      {
         KillLList( pH );
         if( strcmp( lpb2, "<blank>" ) ) {
             iCnt = Add_List_2_List( pH, lpb2 );
         } else {
             iCnt = 0;
         }
         InitListBox( hwnd );
      }
      return iRet;
   }
   else
   {
      return iRet;
   }

   iCnt = 0;
   pH = &gsExclLists[lr];
   *lpb = 0;
   Traverse_List( pH, pN )
   {
      if( !IsXDeleted(pN) )
      {
         lps = GetxStg(pN);
         if( *lpb )
            strcat(lpb, ";");
         strcat(lpb,lps);
      }
   }

   if( iRet && *lpb2 && strcmpi(lpb,lpb2) )
   {
      KillLList( pH );
      bDlg_XChg = TRUE;    // changing one of the LISTS
      iPos = InStr(lpb2, ";");
      while(iPos)
      {
         lpb2[(iPos - 1)] = 0; // remove char
         if( *lpb2 )
         {
            if( Add2Exclude( pH, lpb2 ) )
                  iCnt++;
         }
         lpb2 = &lpb2[iPos];
         iPos = InStr(lpb2, ";");
      }
      // and the last one, if any ...
      if( *lpb2 )
      {
         if( Add2Exclude( pH, lpb2 ) )
            iCnt++;
      }

      if(iCnt)
         InitListBox( hwnd );
   }
   return iRet;
}

INT_PTR  excl_IDOK( HWND hDlg )
{
   if( IsDlgButtonChecked(hDlg, IDC_CHECK1) == BST_CHECKED )
      bDlg_Excl = TRUE;
   else
      bDlg_Excl = FALSE;

   // set iDlg_XSel
   GetCurSel( hDlg, 0 );

   EndDialog(hDlg, IDOK);

   return TRUE;
}

INT_PTR  excl_IDCANCEL( HWND hDlg )
{

   EndDialog(hDlg, IDCANCEL);

   sprtf( "Done EndDialog with IDCANCEL ..."MEOR );

   return TRUE;
}

INT_PTR  excl_IDC_LIST1( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   // The low-order word is the list box identifier
   DWORD dwn = HIWORD(wParam);   // The high-order word is the notification message
   switch(dwn) // switch pre notification
   {
   case LBN_SELCHANGE:
      GetCurSel( hDlg, (HWND)lParam );
      break;
   case LBN_DBLCLK:
      excl_IDC_BUTADD( hDlg );
      break;
   }
   return TRUE;
}

INT_PTR  excl_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = TRUE;
   DWORD    dwc = LOWORD(wParam);
   switch(dwc)
   {
   case IDOK:
      iRet = excl_IDOK( hDlg );
      break;
   case IDCANCEL:
      iRet = excl_IDCANCEL(hDlg);
      break;
   case IDC_BUTADD:
      iRet = excl_IDC_BUTADD(hDlg);
      break;
   case IDC_LIST1:
      iRet = excl_IDC_LIST1( hDlg, wParam, lParam );
      break;

   }
   return iRet;
}

#ifndef  NDEBUG
extern   LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern   BOOL  NotInX( UINT a );
#endif   // #ifndef NDEBUG

// IDM_EDITEXCLUDE - MAKEINTRESOURCE(IDD_DLG_EXCLUDE),   // dialog box template
INT_PTR CALLBACK EXCLUDEDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = 0;
#ifndef  NDEBUG
   if( NotInX(uMsg) )
      sprtf( "EXDLG: %s wP=%x lP=%x."MEOR, GetWMStg(0, uMsg, wParam, lParam ), wParam, lParam );
#endif   // #ifndef NDEBUG
   switch(uMsg)
   {
   case WM_INITDIALOG:
      iRet = excl_INIT(hDlg,wParam,lParam);
      break;
   case WM_COMMAND:
      iRet = excl_COMMAND(hDlg,wParam,lParam);
      break;
   }
   return iRet;
}

INT_PTR  Do_IDM_EDITEXCLUDE( VOID ) // actually is PTARGS pta
{
   INT_PTR     iOK;
   DWORD       dwDlgBox  = IDD_DLG_EXCLUDE;

      bDlg_Excl = gbExclude;  // get the current EXCLUDE
      iDlg_XSel = giCurXSel;  // and current/last selection
      bDlg_XChg = FALSE;      // NO change in the LIST(s) yet

      if( gcxScreen >= 1024 )
         dwDlgBox  = IDD_DLG_EXCLUDE2;


      dc4w_UI(TRUE);
      iOK = DialogBoxParam(
         g_hInst,      // handle to module
         MAKEINTRESOURCE(dwDlgBox),   // dialog box template MAKEINTRESOURCE(IDD_DLG_EXCLUDE)
         hwndClient,    // handle to owner window
         EXCLUDEDLGPROC,   // dialog box procedure
         (LPARAM)0 ); // initialization value
      dc4w_UI(FALSE);

      if( iOK == IDOK )
      {
         iOK = 0;
         if( bDlg_XChg )   // any LIST changed
         {
            iOK++;
            bChgXLst = TRUE;     // there have been changes
         }
         if( bDlg_Excl != gbExclude )  // change in EXCLUDE state flag
         {
            gbExclude = bDlg_Excl;  // set NEw flag
            bChgExcl = TRUE;        // signal INI change
            iOK++;                  // and view refresh
         }

         if( iDlg_XSel != giCurXSel )  // did the EXCLUDE LIST SELECTION change
         {
            giCurXSel = iDlg_XSel;  // update to NEW EXCLUDE list
            bChgXSel = TRUE;        // set INI change
            iOK++;                  // and view refresh
         }

         if(iOK)
            Do_FULL_REFRESH();   // refresh the DISPLAY
      }

   return iOK;
}

// ======================================
// END EXLUDED DIALOG

// ================================================
// BEGIN DIRECTORY DIALOG
//
// From the RC file
//IDD_DIRECTORY DIALOG DISCARDABLE  0, 0, 268, 157
//STYLE WS_POPUP | WS_CAPTION
//CAPTION "SELECT DIRECTORIES"
//FONT 8, "MS Sans Serif"
//BEGIN
//    LTEXT           "Enter complete pathname of directories:",IDD_LABEL,7,6,
//                    130,10,NOT WS_GROUP
//    CONTROL         "&Include subdirectories",IDD_RECURSIVE,"Button",
//                    BS_AUTOCHECKBOX | WS_TABSTOP,7,135,90,10
//    LTEXT           "Dir &1",IDD_LAB1,7,18,23,10,NOT WS_GROUP
//    LTEXT           "Dir &2",IDD_LAB2,7,40,24,10,NOT WS_GROUP
//    PUSHBUTTON      "&Cancel",IDCANCEL,171,136,40,15
//    DEFPUSHBUTTON   "&OK",IDOK,221,136,40,15
//    COMBOBOX        IDC_DIR1,32,18,183,79,CBS_DROPDOWN | WS_VSCROLL | 
//                    WS_TABSTOP
//    COMBOBOX        IDC_DIR2,32,39,183,57,CBS_DROPDOWN | WS_VSCROLL | 
//                    WS_TABSTOP
//    LTEXT           "lab1",IDC_LAB1,7,57,254,8,NOT WS_GROUP
//    LTEXT           "lab2",IDC_LAB2,7,70,254,8,NOT WS_GROUP
//    GROUPBOX        "Compare List / Project / Session Name(s)",IDC_STATIC,7,
//                    84,254,39
//    COMBOBOX        IDC_COMBO1,14,96,117,94,CBS_DROPDOWN | CBS_SORT | 
//                    WS_VSCROLL | WS_TABSTOP
//    DEFPUSHBUTTON   "&Browse...",ID_BROWSE1,221,17,40,15
//    DEFPUSHBUTTON   "&Browse...",ID_BROWSE2,221,38,40,15
//END

TCHAR g_szBuf3[264];
HWND  g_hCtl3;

#define  AddString(a,b)\
{\
         lRes = SendMessage( a, CB_ADDSTRING, 0, (LPARAM)b );\
         if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )\
            return FALSE;\
}

LRESULT  AddString2( HWND hCtl, LPTSTR lpb )
{
   LRESULT  lRes = CB_ERR;
   //AddString( hCtl, lpb );
   lRes = SendMessage( hCtl, CB_ADDSTRING, 0, (LPARAM)lpb );

   if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
      return CB_ERR;

   return lRes;
}

VOID  dd_IDC_DIR1( HWND hDlg, BOOL flg );

BOOL  GetListMember( LPTSTR lpb, DWORD dwNum )
{
   DWORD iCnt;
   PLE   ph = &gsCompList; // global 'compare' list [ %s : %s ] form
   PLE   pn;

   //ListCount2( ph, &iCnt );
   //if( ( iCnt == 0 ) || ( dwNum >= iCnt ) )
   //   return FALSE;
   iCnt = 0;
   Traverse_List( ph, pn )
   {
      // get the INI string
      if( iCnt == dwNum )
      {
         strcpy( lpb, (LPTSTR)((PLE)pn + 1) );
         return TRUE;
      }
      iCnt++;
   }

   return FALSE;
}

DWORD g_dwLC = 0;
DWORD g_dwLN = 0;
VOID  Do_IDM_NEXTINLIST( HWND hWnd )
{
   DWORD    iCnt;
   PLE      ph = &gsCompList;
   //PLE      pn;
   LPTSTR   lpb;  // = GetStgBuf();

   if( g_dwLC == 0 )
   {
      ListCount2( ph, &g_dwLC );
   //if( ( iCnt == 0 ) || ( dwNum >= iCnt ) )
   //   return FALSE;
   }
   else
   {
      ListCount2( ph, &iCnt );
      if( iCnt > g_dwLC )
         g_dwLC = iCnt;
   }

   if( !g_dwLC )
      return;

   g_dwLN++;
   if( g_dwLN == g_dwLC )
   {
      g_dwLN = 0;
   }

   lpb = GetStgBuf();
   if( GetListMember( lpb, g_dwLN ) )
   {
      // we have the NEXT string to flip to
      sprtf( "Should change compare to [%s]"MEOR, lpb );
   }
}


DWORD  IsListMember( LPTSTR lpFile )
{
   DWORD iCnt;
   LPTSTR   lpb;
   PLE   ph = &gsCompList;
   PLE   pn;

   //ListCount2( ph, &iCnt );
   //if( ( iCnt == 0 ) || ( dwNum >= iCnt ) )
   //   return FALSE;
   iCnt = 0;
   Traverse_List( ph, pn )
   {
      iCnt++;
      // get the INI string
      lpb = (LPTSTR)((PLE)pn + 1);
      if( InStr( lpb, lpFile ) )
         return iCnt;
   }

   return 0;   // FAILED to find snippet
}

// forward reference
INT  dd_IDOK( HWND hDlg );

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : dd_WM_INITDIALOG
// Return type: VOID 
// Argument   : HWND hDlg
// Description: Initialise the IDD_DIRECTORY compare dialog
//              FIX20010523 - Changed IDD_DIR1/IDD_DIR2 edit boxes to
// IDC_DIR1/IDC_DIR2 combo boxes.
///////////////////////////////////////////////////////////////////////////////
BOOL  dd_WM_INITDIALOG( HWND hDlg )
{
   LPTSTR   lpb  = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
   LPTSTR   lpb3 = &g_szBuf3[0];
   HWND     hCtl1 = GetDlgItem(hDlg, IDC_DIR1);
   HWND     hCtl2 = GetDlgItem(hDlg, IDC_DIR2);
   HWND     hCtl3 = GetDlgItem(hDlg, IDC_COMBO1);
   PLE      ph, pn;
   INT      iPos, iCnt;
   LRESULT  lRes, lRes2;
   LPTSTR   lpb4;

   if( !hCtl1 || !hCtl2 || !hCtl3 )
   {
      return FALSE;
   }

   g_hCtl3 = hCtl3;  // keep the LIST of project names handy

   // this should be done MUCH LATER in the INIT
   // CenterDialog( hDlg, hwndClient );   // centre it on the client

   /* set recursive option to most recent value */
   CheckDlgButton(hDlg, IDD_RECURSIVE, dlg_recursive);

// IDM_DIR - put up directories DIALOG
//TCHAR szAuSel[] = "AutoSelect1";
   CheckDlgButton(hDlg, IDC_AUTOSEL, gbAutoSel);   //   sFW.fw_bAutoSel
//#define  bChgASel   sFW.fw_bChgASel

   // FILL THE COMBO BOXES WITH THE LIST OF COMPARES
   // gsCompList
   SendMessage( hCtl1, CB_RESETCONTENT, 0, 0 );
   SendMessage( hCtl2, CB_RESETCONTENT, 0, 0 );
   SendMessage( hCtl3, CB_RESETCONTENT, 0, 0 );
   lRes2 = CB_ERR;

   ph = &gsCompList;
   ListCount2( ph, &iCnt );
   if( iCnt == 0 )
      goto Dn_List;

   iCnt = 0;
   Traverse_List( ph, pn )
   {
      // get the INI string
      strcpy( lpb, (LPTSTR)((PLE)pn + 1) );
      sprintf( lpb3, "List %3d: ", (iCnt + 1) );

      iPos = InStr( lpb, " : " );
      if(iPos)
      {
         strcpy( lpb2, &lpb[ (iPos + 2) ] );  // get the second string
         lpb[(iPos-1)] = 0;   // and CHOP first at the SEPARATOR " : "

         // add LEFT = first to combo list
         lRes = SendMessage( hCtl1, CB_ADDSTRING, 0, (LPARAM)lpb );
         if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
            return FALSE;

         //if( strcmpi( lpb, &gszLeft[0] ) == 0 )
         //if( strcmpi( lpb, dialog_leftname ) == 0 ) // or dialog_rightname
         if( strcmpi( lpb, dlg_leftname ) == 0 ) // or dialog_rightname
            lRes2 = lRes;

         // add RIGHT (target) second to combo list
         lRes = SendMessage( hCtl2, CB_ADDSTRING, 0, (LPARAM)lpb2 );
         if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
            return FALSE;

         lpb4 = GetMinTxt( lpb, lpb2, " : " );   // try to resolve a 'name'

         // add in generated name
         strcat(lpb3, lpb4);

         AddString( hCtl3, lpb3 );

         iCnt++;
      }
   }

Dn_List:

   if( iCnt == 0 )
   {
      _getcwd( lpb2, 256 );
      lRes = SendMessage( hCtl1, CB_ADDSTRING, 0, (LPARAM)lpb2 );
      if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         return FALSE;
      lRes2 = lRes;
      lRes = SendMessage( hCtl1, CB_ADDSTRING, 0, (LPARAM)lpb2 );
      if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         return FALSE;
   }

   if( lRes2 == CB_ERR )
      lRes2 = 0;

   SendMessage( hCtl1, CB_SETCURSEL, lRes2, 0 );
   SendMessage( hCtl2, CB_SETCURSEL, lRes2, 0 );
   SendMessage( hCtl3, CB_SETCURSEL, lRes2, 0 );

   dd_IDC_DIR1( hDlg, FALSE );

// int   dd_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
// INT  dd_IDOK( HWND hDlg )
// IDM_DIR - put up directories DIALOG
//TCHAR szAuSel[] = "AutoSelect1";
//   CheckDlgButton(hDlg, IDC_AUTOSEL, gbAutoSel);   //   sFW.fw_bAutoSel
//#define  bChgASel   sFW.fw_bChgASel
   if( iCnt && gbAutoSel && g_bOneShot &&
      !g_bOneShot2 )  //= FALSE;  // posted IDM_DIR to bring up dialog box
   {
      sprtf("Shutting DIRDIALOG - from posted IDM_DIR"MEOR);
      dd_IDOK( hDlg );
      g_bOneShot2 = TRUE;
   }
   else
   {
      // this should be done MUCH LATER in the INIT
      CenterDialog( hDlg, hwndClient );   // centre it on the client
   }

   return TRUE;

}

BOOL  dd_GetComboSel( LPTSTR lpText, HWND hCtl, LRESULT * pres )
{
   LRESULT  lRes;
   LPTSTR   lpb  = &g_szBuf1[0];
   if( !hCtl )
      return FALSE;  // need thise thing

   *lpb = 0;
   lRes = SendMessage( hCtl, CB_GETCURSEL, 0, 0 ); // get index (0 based) of CURRENT SELECTION
   if( lRes == CB_ERR )
   {
      // there is NO SELECTION - If the user has typed a NEW entry in the edit, so
      // For a combo box, the text is the content of the edit control
      // (or static-text) portion of the combo box.
      lRes = SendMessage( hCtl, WM_GETTEXT, (WPARAM) 256, (LPARAM) lpb ); // text buffer
   }
   else
   {
      // we have a SELECTION
      if(pres)
         *pres = lRes;  // return index of selection in ONE

      // if asked, also get the text
      lRes = SendMessage( hCtl, CB_GETLBTEXT, lRes, (LPARAM) lpb );  // receives string (LPCSTR)
   }
   if(( lRes == CB_ERR ) ||
      ( *lpb == 0      ) )
   {
      return FALSE;
   }

   if( lpText )
      strcpy(lpText, lpb); // pass back TEXT

   return TRUE;
}


BOOL  dd_GetSel2( HWND hCtl1, HWND hCtl2, LPTSTR lpb, LPTSTR lpb2, LRESULT * pres )
{
   LRESULT  lRes;
   if( lpb )
   {
      *lpb = 0;
      if( hCtl1 )
      {
         lRes = SendMessage( hCtl1, CB_GETCURSEL, 0, 0 );
         if( lRes == CB_ERR )
         {
            // there is NO SELECTION - the user has typed a NEW entry in the edit, so
            // For a combo box, the text is the content of the edit control
            // (or static-text) portion of the combo box.
            lRes = SendMessage( hCtl1, WM_GETTEXT, (WPARAM) 256, (LPARAM) lpb ); // text buffer
         }
         else
         {
            if(pres)
               *pres = lRes;  // return index of selection in ONE
      
            lRes = SendMessage( hCtl1, CB_GETLBTEXT, lRes, (LPARAM) lpb );  // receives string (LPCSTR)
         }
         if( ( lRes == CB_ERR ) ||
             ( *lpb == 0      ) )
         {
            return FALSE;
         }
      }
   }

   if( lpb2 )
   {
      *lpb2 = 0;
      if( hCtl2 )
      {
         lRes = SendMessage( hCtl2, CB_GETCURSEL, 0, 0 );
         if( lRes == CB_ERR )
         {
            // there is NO SELECTION - the user has typed a NEW entry in the edit, so
            // For a combo box, the text is the content of the edit control
            // (or static-text) portion of the combo box.
            lRes = SendMessage( hCtl2, WM_GETTEXT, (WPARAM) 256, (LPARAM) lpb2 ); // text buffer
         }
         else
         {
            lRes = SendMessage( hCtl2, CB_GETLBTEXT, lRes, (LPARAM) lpb2 );  // receives string (LPCSTR)
         }
         if( ( lRes  == CB_ERR ) ||
             ( *lpb2 == 0      ) )
         {
            return FALSE;
         }
      }
   }

   return TRUE;

}

BOOL  dd_GetSel( HWND hDlg, LPTSTR lpb, LPTSTR lpb2, LRESULT * pres )
{
   HWND     hCtl1 = GetDlgItem(hDlg, IDC_DIR1);
   HWND     hCtl2 = GetDlgItem(hDlg, IDC_DIR2);
//   HWND     hCtl3 = GetDlgItem(hDlg, IDC_COMBO1);
   if( !hCtl1 || !hCtl2 )  // || !hCtl3 )
      return FALSE;
   return( dd_GetSel2( hCtl1, hCtl2, lpb, lpb2, pres ) );
}

// A dc4w (left or right) compare item
#define  IS_FILE_ITEM      0x00000001
#define  IS_DIR_ITEM       0x00000002
#define  IS_ZIP_FILE       0x00000004

#define  L_IS_FILE_ITEM    (IS_FILE_ITEM << 16)
#define  L_IS_DIR_ITEM       0x0002
#define  L_IS_ZIP_FILE       0x0004

#define  GOOD_COMP1(a)   ( ( a & (L_IS_FILE_ITEM | IS_FILE_ITEM) ) == (L_IS_FILE_ITEM | IS_FILE_ITEM))
#define  GOOD_COMP2(a)   ( ( a & (L_IS_DIR_ITEM | IS_DIR_ITEM) ) == (L_IS_DIR_ITEM | IS_DIR_ITEM))
#define  GOOD_COMP(a)    ( GOOD_COMP1(a) || GOOD_COMP2(a) )

BOOL  IsFileComp( VOID )
{
   if( GOOD_COMP1( g_dwActComp ) )
      return TRUE;
   return FALSE;
}

DWORD AppdMsgTxt( LPTSTR lpb )
{
   DWORD    dwi = 0;
   if( dir_isvalidfile(lpb) )
   {
      dwi |= IS_FILE_ITEM;
#ifdef ADD_ZIP_SUPPORT
      if( IsValidZip(lpb) ) // #ifdef ADD_ZIP_SUPPORT
      {
         strcat( lpb, " (Zip)");
         dwi |= IS_ZIP_FILE;
      }
      else
#endif // #ifdef ADD_ZIP_SUPPORT
      {
         strcat( lpb, " (FOk)");
      }
   }
   else if( dir_isvaliddir(lpb) )
   {
      strcat( lpb, " <DIR>");
      dwi |= IS_DIR_ITEM;
   }
   else
   {
      strcat( lpb, " (*Invalid*)");
   }
   return dwi;
}

// g_dwActComp FLAG
DWORD GetCompFlag( LPTSTR pDir ) 
{
   DWORD    dwi = 0;
   if( dir_isvalidfile(pDir) )
   {
      dwi |= IS_FILE_ITEM;
#ifdef ADD_ZIP_SUPPORT
      if( IsValidZip(pDir) ) // #ifdef ADD_ZIP_SUPPORT
      {
         //strcat( lpb, " (Zip)");
         dwi |= IS_ZIP_FILE;
      }
      else
#endif // #ifdef ADD_ZIP_SUPPORT
      {
         //strcat( lpb, " (FOk)");
      }
   }
   else if( dir_isvaliddir(pDir) )
   {
      //strcat( lpb, " <DIR>");
      dwi |= IS_DIR_ITEM;
   }
   else
   {
      //strcat( lpb, " (*Invalid*)");
   }
   return dwi;
}

//LRESULT CALLBACK WindowProc(
//  HWND hwnd,       // handle to window
//  UINT uMsg,       // WM_COMMAND
//  WPARAM wParam,   // combo box identifier, CBN_EDITCHANGE
//  LPARAM lParam    // handle to combo box (HWND)
//wParam 
//The low-order word specifies the control identifier of the combo box. 
//The high-order word specifies the notification message. 
//TCHAR g_szListnm[] = "List %d";
TCHAR g_szListnm[] = "List %d";
BOOL  g_bInSET = 0;

VOID  dd_IDC_DIR1( HWND hDlg, BOOL flg )
{
   LPTSTR   lpb  = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
   LPTSTR   lpb3 = &g_szBuf3[0];
   LRESULT  lRes = CB_ERR;
   *lpb  = 0;
   *lpb2 = 0;
   *lpb3 = 0;
   if( flg && !g_bInSET )
   {
      if( dd_GetSel( hDlg, lpb, 0, &lRes ) )
      {
         if( lRes != CB_ERR )
         {
            // line up the second with the first
            SendMessage( GetDlgItem(hDlg, IDC_DIR2  ), CB_SETCURSEL, lRes, 0 );
            SendMessage( GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, lRes, 0 );
            sprintf( lpb3, g_szListnm, (lRes + 1) );
         }
         if( dd_GetSel( hDlg, lpb, lpb2, &lRes ) )
         {
            //SetDlgItemText( hDlg, IDC_LAB1, lpb  );
            //SetDlgItemText( hDlg, IDC_LAB2, lpb2 );
         }
      }
   }
   else if( dd_GetSel( hDlg, lpb, lpb2, &lRes ) )
   {
      //SetDlgItemText( hDlg, IDC_LAB1, lpb  );
      //SetDlgItemText( hDlg, IDC_LAB2, lpb2 );
   }

   AppdMsgTxt(lpb);
   AppdMsgTxt(lpb2);      
   SetDlgItemText( hDlg, IDC_LAB1, lpb  );
   SetDlgItemText( hDlg, IDC_LAB2, lpb2 );

   if( !g_bInSET )
   {
      // show the LIST name selection
      SetDlgItemText( hDlg, IDC_LISTLABEL, lpb3 );
   }
}

VOID  SetActComp( LPTSTR pleft, LPTSTR pright )
{
   DWORD    dwi;
   strcpy(g_szActLeft ,pleft );
   strcpy(g_szActRight,pright);
   dwi = GetCompFlag( pleft );
   //dwi = AppdMsgTxt( pleft  );
   dwi = dwi << 16;
   //dwi |= AppdMsgTxt( pright );
   dwi |= GetCompFlag( pright );
   g_dwActComp = dwi;
}

INT  dd_IDOK( HWND hDlg )
{
   INT      iRet = IDOK;
   LPTSTR   lpb1 = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
   LRESULT  lRes;
   BOOL     bAuto;

   // get left and right from combo boxes
   if( !dd_GetSel( hDlg, lpb1, lpb2, &lRes ) )
   {
      EndDialog(hDlg, -2);
      return FALSE;
   }

   // fetch the text from the dialog, and remember it in win.ini
   //GetDlgItemText(hDlg, IDD_DIR1, dialog_leftname, 256 );
   //GetDlgItemText(hDlg, IDD_DIR2, dialog_rightname, 256 );
   /* fetch recursive option */
   dlg_recursive = SendDlgItemMessage(hDlg, IDD_RECURSIVE,
                        BM_GETCHECK, 0, 0);
// IDM_DIR - put up directories DIALOG
//TCHAR szAuSel[] = "AutoSelect1";
   bAuto = SendDlgItemMessage(hDlg, IDC_AUTOSEL,
                              BM_GETCHECK, 0, 0);  // gbAutoSel);   //   sFW.fw_bAutoSel

   if( gbAutoSel != bAuto )
   {
      bChgASel = TRUE;  //   sFW.fw_bChgASel
      gbAutoSel = !gbAutoSel; // toggle mode
   }

   // copy the selected or entered strings
//   strcpy( dialog_leftname,  lpb1 );
//   strcpy( dialog_rightname, lpb2 );
   if( strcmp( dlg_leftname, lpb1 ) )
   {
      strcpy( prev_dlg_left, dlg_leftname );
      strcpy( dlg_leftname,  lpb1 );
      dlg_chgleft++;
   }
   if( strcmp( dlg_rightname, lpb2 ) )
   {
      strcpy( prev_dlg_right, dlg_rightname );
      strcpy( dlg_rightname,  lpb2 );
      dlg_chgright++;
   }

#if   0  // this will be DONE in complist_args() if dialog_left/right pan out ok

   if( strcmpi( gszLeft, lpb1 ) )  // changed
   {
      strcpy( gszLeft, lpb1 ); // changed
      bChgLf = TRUE;
   }

   if( strcmpi( gszRite, lpb2 ) )
   {
      strcpy( gszRite, lpb2 );  // update RIGHT
      bChgRt = TRUE;
   }

#endif   // #if   0  // DONE in complist_args()

   //extern   TCHAR gszCopyTo[];   // destination of COPY files
//extern   BOOL  bChgCT;
//TCHAR szCT[] = "CopyDestination";
   if( strcmpi( gszCopyTo, lpb2 ) )
   {
      strcpy( gszCopyTo, lpb2 );
      bChgCT = TRUE;
   }

   dir_setzipflag( &dlg_flg, lpb1, lpb2 );

   SetActComp( lpb1, lpb2 );

   // note the IDOK passed back
   EndDialog(hDlg, iRet);

   return iRet;

}

VOID  dd_IDC_DIR2( HWND hDlg, DWORD dwNote, HWND hCtl )
{
   LPTSTR   lpb2 = &g_szBuf2[0];
   if( dwNote == CBN_EDITCHANGE )
   {
      if( dd_GetSel2( 0, hCtl, 0, lpb2, 0 ) )
         SetDlgItemText( hDlg, IDC_LAB2, lpb2 );
   }
   else if( dwNote == CBN_SELCHANGE )
   {
      if( dd_GetSel2( 0, hCtl, 0, lpb2, 0 ) )
         SetDlgItemText( hDlg, IDC_LAB2, lpb2 );
   }
}

VOID  dd_IDC_COMBO1( HWND hDlg, DWORD dwNote, HWND hCtl )
{
   if( dwNote == CBN_SELCHANGE )
   {
      LPTSTR   lpb = &gszTmpBuf[0];
      LPTSTR   lpb1 = &g_szBuf1[0];
      LPTSTR   lpb2 = &g_szBuf2[0];
      LPTSTR   lpb3 = &g_szBuf3[0];
      LRESULT  lRes = CB_ERR;
      DWORD    iPos;
      DWORD    dwl, dwr;

      // if( dd_GetSel2( 0, hCtl, 0, lpb2, 0 ) )
      if(( dd_GetComboSel( lpb3, g_hCtl3, &lRes ) ) &&
         ( lRes != CB_ERR ) )
      {
         HWND     hCtl1, hCtl2;
         LPTSTR   pleft, pright;
         //DWORD    dwlen;
         LPTSTR   lpb4;
         DWORD    dwi;

         hCtl1 = GetDlgItem(hDlg, IDC_DIR1);
         hCtl2 = GetDlgItem(hDlg, IDC_DIR2);
         // we should ensure the other two LINE UP EXACTLY
         g_bInSET++;
         // *****************************************
         SendMessage( hCtl1, CB_SETCURSEL, lRes, 0 );
         SendMessage( hCtl2, CB_SETCURSEL, lRes, 0 );
         // *****************************************
         g_bInSET--;

         GetListMember( lpb, lRes );
         iPos = InStr(lpb, " : ");     // get SEPARATOR
         if(iPos)
         {
            pleft = Left(lpb, (iPos - 1));
            pright = Right( lpb, (strlen(lpb) - (iPos + 2)) );
            lpb4 = GetMinTxt( pleft, pright, MEOR );   // try to resolve a 'name'
            dwl = strlen(pleft);
            dwr = strlen(pright);
            strcpy(g_szActLeft,pleft);
            strcpy(g_szActRight,pright);

            dwi = AppdMsgTxt( pleft  );
            dwi = dwi << 16;
            dwi |= AppdMsgTxt( pright );
            g_dwActComp = dwi;

            SetDlgItemText( hDlg, IDC_LAB1, pleft  );
            SetDlgItemText( hDlg, IDC_LAB2, pright );

            // set the TEXT to
            sprintf(lpb, "List %lld:"MEOR, (lRes + 1) );  // begin the header information
            strcat(lpb, lpb4);
            strcat(lpb, MEOR);
            if(dwi)
            {
               if( GOOD_COMP(dwi) )
               {
                  if( dwi & IS_FILE_ITEM )
                     strcat(lpb, "Compare of two files. ");
                  else
                     strcat(lpb, "Compare of two folders. ");
               }
               else
               {
                  if( dwi & L_IS_FILE_ITEM )
                     strcat(lpb, "Left is File. ");
                  else if( dwi & L_IS_DIR_ITEM )
                     strcat(lpb, "Left is Folder. ");

                  if( dwi & IS_FILE_ITEM )
                     strcat(lpb, "Right is File. ");
                  else if( dwi & IS_DIR_ITEM )
                     strcat(lpb, "Right is Folder. ");
               }

               if( dwi & (L_IS_ZIP_FILE | IS_ZIP_FILE) )
                  strcat( lpb, "(Zip restrictions) " );
            }
            else
            {
               strcat(lpb, "Compare does NOT appear valid! ");
            }
         }


         SetDlgItemText( hDlg, IDC_LISTLABEL, lpb );

      }  // if a SELECTION change

   }
   else if( dwNote == CBN_EDITCHANGE )
   {
      // new edit string
   }
}

#define  MXBROWSEBUF    1024

BOOL do_file_item( HWND hDlg, LPTSTR lpb1, LPTSTR lpb, LPTSTR lpCurr )
{
   BOOL  bNew = FALSE;
            if( FgmGetFil2( hDlg, lpb1, lpb ) )
            {
               // got a NEW "file name"
               if( strcmpi( lpCurr, lpb ) == 0 )
               {
                  sprtf("Not a new file - [%s]"MEOR, lpb );
               }
               else
               {
                  sprtf("Got a NEW file name -"MEOR
                     "[%s]"MEOR, lpb );
                  bNew = TRUE;
               }
            }
   return bNew;
}


LPTSTR   GetNewStg( DWORD dwNum, DWORD iCnt, LPTSTR lpNew, DWORD dwLR )
{
   LPTSTR   lpb = GetStgBuf();
   LPTSTR   lpb1 = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
   LPTSTR   lpb3 = &g_szBuf3[0];
   LPTSTR   lpb4 = GetStgBuf();  // &g_szBuf4[0];
   DWORD    iPos;

   if( !GetListMember( lpb, dwNum ) )
   {
      strcpy(lpb, " <*FAILED*>");
      return lpb;
   }

   sprintf( lpb3, "List %3d: ", iCnt );

   iPos = InStr( lpb, " : " );
   if(iPos)
   {
      if( lpNew && *lpNew )
      {
         if( dwLR )
         {
            // NEW right target
            //strcpy( lpb2, &lpb[ (iPos + 2) ] );  // get the second string
            strcpy(lpb2, lpNew);    // insert NEW into mix

            // previous LEFT source
            strcpy( lpb1, lpb );
            //lpb[(iPos-1)] = 0;   // and CHOP first at the SEPARATOR " : "
            lpb1[(iPos-1)] = 0;   // and CHOP first at the SEPARATOR " : "

         }
         else
         {
            // NEW left source
            strcpy( lpb1, lpNew );  // left to right, make left the new
            strcpy( lpb2, &lpb[ (iPos + 2) ] );  // get the second (right) string
         }
      }
      else
      {

         strcpy( lpb2, &lpb[ (iPos + 2) ] );  // get the second string
         strcpy( lpb1, lpb );

         //lpb[(iPos-1)] = 0;   // and CHOP first at the SEPARATOR " : "
         lpb1[(iPos-1)] = 0;   // and CHOP first at the SEPARATOR " : "
      }

         // add LEFT = first to combo list
         //lRes = SendMessage( hCtl1, CB_ADDSTRING, 0, (LPARAM)lpb );
         //if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         //   return FALSE;

         //if( strcmpi( lpb, &gszLeft[0] ) == 0 )
         //   lRes2 = lRes;

         // add RIGHT (target) second to combo list
         //lRes = SendMessage( hCtl2, CB_ADDSTRING, 0, (LPARAM)lpb2 );
         //if( ( lRes == CB_ERR ) || ( lRes == CB_ERRSPACE ) )
         //   return FALSE;
         lpb4 = GetMinTxt( lpb1, lpb2, " : " );   // try to resolve a 'name'

         // add in generated name
         strcat(lpb3, lpb4);

         //AddString( hCtl3, lpb3 );

         //iCnt++;
      
   }
   return lpb3;
}

TCHAR g_szGotNew[264] = "\0";

BOOL  do_dir_item( HWND hDlg, LPTSTR lpb1, LPTSTR lpb, LPTSTR lpCurr )
{
   BOOL  bNew = FALSE;
            if( FgmGetDir2( hDlg, lpb1, lpb ) )
            {
               // got a NEW folder
               //lpn = lpb1; // set NEW location
               if( strcmpi( lpCurr, lpb1 ) == 0 )
               {
                  sprtf("Not a new folder - [%s]"MEOR,
                     lpb1 );
               }
               else
               {
                  sprtf("Got a NEW folder -"MEOR
                     "[%s]"MEOR,
                     lpb1 );
                  strcpy(g_szGotNew, lpb1);
                  bNew = TRUE;
               }
            }
   return bNew;
}


//extern   BOOL FgmGetFil2( HWND hWnd, LPTSTR lpFilNam, LPTSTR lpRes );
//extern   BOOL FgmGetDir2( HWND hWnd, LPTSTR lpDir, LPTSTR lpPath );
VOID  dd_ID_BROWSE( HWND hDlg, DWORD wCmd )
{
   HWND     hCtl = 0;    // which COMBO
   //LPTSTR   lpb = &gszTmpBuf[0];
   //LPTSTR   lpb1 = &g_szBuf1[0];
   LPTSTR   lpb;  // = (LPTSTR) MALLOC( ( 264 * 2 ) );  // &gszTmpBuf[0];
   LPTSTR   lpb1, lpCurr;
   LRESULT  lRes = CB_ERR;
   DWORD    dwi;
   UINT     ui = (MXBROWSEBUF * 3);
   LPTSTR   lpn;
   BOOL     bNew = FALSE;
   DWORD    dwOrg = 0;  // IsListMember( lpb1 );  // Check if in the LIST

   lpb = (LPTSTR) MALLOC(ui);  // &gszTmpBuf[0];
   if( !lpb )
   {
      sprtf( MEOR"ERROR: Memory get %d bytes FAILED!"MEOR, ui );
      return;
   }

   lpb1   = &lpb[MXBROWSEBUF];
   lpCurr = &lpb[MXBROWSEBUF * 2];
   lpn    = lpb;  // NEW name (file or folder) gors here

   ZeroMemory(lpb,ui);

   // first get WHICH 'browse button was pressed
   ui = 0;
   switch(wCmd)
   {
   case ID_BROWSE1:
      //hCtl = GetDlgItem( IDC_DIR1, hDlg );
      ui = IDC_DIR1;
      break;
   case ID_BROWSE2:
      //hCtl = GetDlgItem( IDC_DIR2, hDlg );
      ui = IDC_DIR2;
      break;
   }

   //*lpb = 0;
   //*lpb1 = 0;
   //if( hCtl )
   if( ui )
   {
      //HWND     hCtl1 = GetDlgItem(hDlg, IDC_DIR1);
      //HWND     hCtl2 = GetDlgItem(hDlg, IDC_DIR2);
      hCtl = GetDlgItem(hDlg, ui);  // get the HANDLE
//BOOL  GetComboSel( HWND hCtl, LPTSTR lpb, LRESULT * pres )
      *lpb1 = 0;
      if(( dd_GetComboSel( lpb1, hCtl, &lRes ) ) &&
         ( lRes != CB_ERR                      ) &&
         ( *lpb1                               ) )
      {
         dwOrg = IsListMember( lpb1 );  // Check if in the LIST

         strcpy(lpCurr, lpb1);   // keep current selected name safe

         strcpy(lpb, lpb1);
         dwi = AppdMsgTxt(lpb);
         strcat(lpb, " ");
         lpn = lpb;
         if( dwi & IS_FILE_ITEM )
         {
            strcat(lpb, "Right is File."MEOR);
            sprtf(lpb);
            *lpb = 0;
            bNew = do_file_item( hDlg, lpb1, lpb, lpCurr );
         }
         else if( dwi & IS_DIR_ITEM )
         {
            strcat(lpb, "Right is Folder."MEOR);
            sprtf(lpb);
            *lpb = 0;
            lpn = lpb1; // set NEW location
            bNew = do_dir_item( hDlg, lpb1, lpb, lpCurr );
         }
         else
         {
            strcat(lpb," <*Not Valid*>");
            sprtf(lpb); // show what we start with
            *lpb = 0;
            bNew = do_file_item( hDlg, lpb1, lpb, lpCurr );
         }

      }
      else // if NOTHING from the COMBO box
      {
            *lpb = 0;
            lpn = lpb1; // set NEW location
            if( *lpn == 0 )   // if NO FOLDER, then try
               strcpy(lpn, g_szGotNew);   // fill in with LAST got new

            bNew = do_dir_item( hDlg, lpb1, lpb, lpCurr );
      }

         if( bNew )
         {
            DWORD dwNum = IsListMember( lpn );  // Check if in the LIST
            if( dwNum )
            {
               sprtf( "Switch to a new selected in LIST"MEOR );
            }
            else
            {
               //ui = IDC_DIR1;
               //ui = IDC_DIR2;
               switch(ui)
               {
                  //case ID_BROWSE1:
                  //case ID_BROWSE2:
                  //hCtl = GetDlgItem( IDC_DIR1, hDlg );
                  //hCtl = GetDlgItem( IDC_DIR2, hDlg );
               case IDC_DIR1:
               case IDC_DIR2:
                  // Add string to combo CONTROL list,
                  lRes = AddString2( hCtl, lpn ); // add this to the LIST box
                  if( lRes != CB_ERR )
                  {
                     LPTSTR   lpb3;
                     // and if successful, select it
                     SendMessage( hCtl, CB_SETCURSEL, lRes, 0 );
                     lpb3 = GetNewStg( dwOrg,   // index of original left/right pair
                        0,       // List number %3d
                        lpn,     // use this NEW string as part of combined
                        ( ( ui == IDC_DIR1 ) ? 0 : 1 ) );

                     lRes = AddString2( g_hCtl3, lpb3 );
                     if( lRes != CB_ERR ) // if no error, select it
                        SendMessage( g_hCtl3, CB_SETCURSEL, lRes, 0 );

                  }
                  break;
               }
               sprtf( "To be ADDED to the LIST on OK"MEOR );
            }
         }

//      }

   }

   MFREE(lpb);
}

int   dd_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   DWORD wCmd = LOWORD(wParam);

   switch( wCmd )
   {
   case IDCANCEL:
      EndDialog(hDlg, FALSE);
      return TRUE;

   case IDOK:
      dd_IDOK( hDlg );
      return TRUE;

   case IDC_DIR1:
      dd_IDC_DIR1( hDlg, TRUE );
      return TRUE;

   case IDC_DIR2:
      dd_IDC_DIR2( hDlg, HIWORD(wParam), (HWND)lParam);
      return TRUE;

   case IDC_COMBO1:
      dd_IDC_COMBO1( hDlg, HIWORD(wParam), (HWND)lParam);
      return TRUE;

   case ID_BROWSE1:
   case ID_BROWSE2:
      dd_ID_BROWSE( hDlg, wCmd );
      return TRUE;

   }

   return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : DIRDLGPROC
// Return type: INT_PTR CALLBACK 
// Arguments  : HWND hDlg =  handle to dialog box
//            : UINT uMsg =  message
//            : WPARAM wParam
//            : LPARAM lParam // second message parameter
// Description: Standard DIALOG BOX handler
//              Returns IDOK if there is a selection when user
// clicks OK.
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DIRDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = 0;
   switch( uMsg )
   {
   case WM_INITDIALOG:
      if( !dd_WM_INITDIALOG( hDlg ) )
      {
         EndDialog(hDlg, -1); // some FAILURE - do not continue
      }
      else
      {
         iRet = TRUE;
      }
      break;

   case WM_COMMAND:
      iRet = dd_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      break;

   }

   return iRet;
}

INT_PTR  Do_DIR_DLG( VOID )
{
   INT_PTR  ip;

   dc4w_UI(TRUE);
   sprtf( "Do_DIR_DLG:[%s : %s]"MEOR, dlg_leftname, dlg_rightname );
   ip = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_DIRECTORY),   // "directory"
      hwndClient,
      DIRDLGPROC,
      (LPARAM) 0);
   dc4w_UI(FALSE);

   return ip;
}

// END   DIRECTORY DIALOG
// ================================================

// BEGIN SAVELIST DIALOG = IDD_SAVELIST
// Extract from RC file

// FIX20010525 - Changed from EDIT to COMBOBOX (dropdown)
//    EDITTEXT        IDD_FILE,49,16,200,12,ES_AUTOHSCROLL

// Do_SAVELISTDLG IDM_SAVELIST
// FIX20091125 v.2 IDD_SAVELIST2 "SaveList" SAVE FILE LIST2
// ADDED CONTROL "Add Tags - enable to add following tags",IDC_CHECK1,
//               "Button",BS_AUTOCHECKBOX | WS_TABSTOP,15,135,152,9
INT_PTR  sl_WM_INITDIALOG( HWND hDlg, LPARAM lParam ) // IDD_SAVELIST
{
   LPTSTR      lpb   = &g_szBuf1[0];
   PCFDLGSTR   pcfds = (PCFDLGSTR)lParam;
   HWND        hCtl  = GetDlgItem( hDlg, IDC_COMBO_FILE );
   DWORD       dwi;
   PLE         ph;
   LPTSTR      lpf;
   UINT        ui;

   SET_PROP( hDlg, SAVE_ATOM, pcfds ); // ATOM set

   if(( !hCtl ) ||
      ( !VALIDPCFDS(pcfds) ) )
   {
      EndDialog(hDlg, -1);
      return 0;
   }

   ph = pcfds->cf_sTARGS.ta_sDI.di_pDFList; // get the INI list of LIST FILES
   lpf = &pcfds->cf_sTARGS.ta_sDI.di_szFile[0]; // and the LAST used
// pcfds->cf_sTARGS.ta_sDI.di_dwOpts  = options; // gdwFileOpts; GLOBAL list file options
   dwi = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;
//   if( dwi != gdwFileOpts )
   if( (( dwi & ~(INC_OUTLINE2) ) != ( gdwFileOpts & ~(INC_OUTLINE2) ) ) )
   {
      chkme( "Can NOT have two options going at one time %x vs %x"MEOR,
         dwi, gdwFileOpts );
      exit(-1);
   }

   if(( !ph ) || // fill in the LIST of out files, into the COMBO
      ( !FillComboBox( hCtl, ph, lpf ) ) ) // this is to be the SELECTED
   {
      EndDialog(hDlg, -2);
      return 0;
   }

   // this should be near the end
   //CenterDialog( hDlg, hwndClient );   // centre it on the client
   //MyCommonDlgInit( hDlg, pcfds, lpb );

   //dwi = gdwFileOpts; // use to when writing OUTLINE list of files
   CDB( IDD_IDENTICAL, dwi, INCLUDE_SAME      );
   CDB( IDD_DIFFER,    dwi, INCLUDE_NEWER     );
   CDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER     );
   CDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   CDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );

   // Options for output control - how to give MORE here
   // One idea would be to execute a requested program pointing
   // to the file written - for further processing of file list info
   // ***
   // This was just
   // CDB( IDC_CHECK1,    dwi, FULL_NAMES        );
   // but now it is a radio set 4=Full, 5=Relative, 6=title only
   // controlled by two bits in gdwFileOpts - FULL_NAMES and ADD_REL_PATH
   if( dwi & FULL_NAMES )
      ui = IDC_RADIO4;  // check Full
   else if( dwi & ADD_REL_PATH ) // NOTE REV - better would be USE_TITLE_ONLY
      ui = IDC_RADIO6;  // check Title, else
   else
      ui = IDC_RADIO5;  // default show relative path - minus the ".\" root item
   CheckRadioButton( hDlg, IDC_RADIO4, IDC_RADIO6, ui );

   CDB( IDC_CHECK1,    dwi, INCLUDE_TAGS      ); // FIX20091125 - Include tags in output
   CDB( IDC_CHECK2,    dwi, INCLUDE_HEADER    );
   CDB( IDC_CHECK3,    dwi, APPEND_FILE       );
   CDB( IDC_CHECK4,    dwi, ADD_COMMENTS      );   // add trailing comments
   CDB( IDC_CHECK7,    dwi, ADD_FIL_INFO      );   // add file date and times
   CDB( IDC_CHECK8,    dwi, ADD_X_HDR         );   // multi-lined

//#define  FLEFT_NAME              0x00004000  // use LEFT full name
//#define  FRIGHT_NAME             0x00008000  // use RIGHT full name
//#define  COMBINED_NAME           0x00010000  // use combination of left and right
   if( dwi & FLEFT_NAME )
      ui = IDC_RADIO1;
   else if( dwi & FRIGHT_NAME )
      ui = IDC_RADIO2;
   else
      ui = IDC_RADIO3;
   CheckRadioButton( hDlg, IDC_RADIO1, IDC_RADIO3, ui );

   /* convert 'left tree' into the right name */
   //sprintf(lpb, LoadRcString(IDS_FILES_ONLY), (LPSTR) dialog_leftname);
   sprintf(lpb, LoadRcString(IDS_FILES_ONLY), (LPSTR) dlg_leftname);
   SendDlgItemMessage(hDlg, IDD_LEFT, WM_SETTEXT, 0, (LPARAM)lpb);

   /* convert 'right tree' msg into correct path */
   //sprintf(lpb, LoadRcString(IDS_FILES_ONLY), (LPSTR) dialog_rightname);
   sprintf(lpb, LoadRcString(IDS_FILES_ONLY), (LPSTR) dlg_rightname);
   SendDlgItemMessage(hDlg, IDD_RIGHT, WM_SETTEXT, 0, (LPARAM)lpb);

   *lpb = 0;
   _getcwd( lpb, 256 );
   if( *lpb ) {
      strcat(lpb, " is cwd");
      SetDlgItemText( hDlg, IDC_LABCWD, lpb );
   }

   // FIX20091125 - add head/tail tags - get from INI
   strcpy(dlg_head,gszTagHead);
   strcpy(dlg_tail,gszTagTail);
   // show previous string
   SetDlgItemText( hDlg, IDC_EDIT1, &dlg_head[0] );
   SetDlgItemText( hDlg, IDC_EDIT2, &dlg_tail[0] );

   bDnDlgInit = TRUE;

   AddAppPrompt( hDlg, IDC_COMBO_FILE, IDC_LABWARN );

   // this should be near the end
   CenterDialog( hDlg, hwndClient );   // centre it on the client

   //MyCommonDlgInit( hDlg, pcfds, lpb, 0 );   // from SAVE LIST
   MyCommonDlgInit2( hDlg );   // from SAVE LIST

   SetFocus( hCtl ); // = GetDlgItem(hDlg, IDC_COMBO_SAVE) );

   //return TRUE;
   return FALSE;

}


BOOL  sl_IDOK( HWND hDlg, PCFDLGSTR pcfds )
{
   LPTSTR   lpb = &g_szBuf1[0];
   HWND     hCtl = GetDlgItem(hDlg, IDC_COMBO_FILE);
   DWORD    dwi;     // = gdwFileOpts;

   *lpb = 0;
   if(( !hCtl ) ||
      ( !VALIDPCFDS(pcfds) ) ||
      ( !GetComboSel( hCtl, lpb, 0 ) ) ||
      ( *lpb == 0 ) )
      return FALSE;

   bDnDlgInit = FALSE;

   strcpy( &pcfds->cf_sTARGS.ta_sDI.di_szFile[0], lpb );

   //dwi = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;  // extract the OPTIONS
   dwi = 0;  // extract the OPTIONS

   ICDB( IDD_IDENTICAL, dwi, INCLUDE_SAME      );
   ICDB( IDD_DIFFER,    dwi, INCLUDE_NEWER     );
   ICDB( IDD_DIFFER2,   dwi, INCLUDE_OLDER     );
   ICDB( IDD_LEFT,      dwi, INCLUDE_LEFTONLY  );
   ICDB( IDD_RIGHT,     dwi, INCLUDE_RIGHTONLY );
   FixDiff(dwi);  // if neither newer nor older, then NO INCLUDE_DIFFER flag

   ICDB( IDC_CHECK1,    dwi, INCLUDE_TAGS      ); // FIX20091125 - Include tags in output 
   ICDB( IDC_CHECK2,    dwi, INCLUDE_HEADER    );
   ICDB( IDC_CHECK3,    dwi, APPEND_FILE       );
   ICDB( IDC_CHECK4,    dwi, ADD_COMMENTS      );   // add trailing comments
   ICDB( IDC_CHECK7,    dwi, ADD_FIL_INFO      );   // add date/time/size(s)
   ICDB( IDC_CHECK8,    dwi, ADD_X_HDR         );   // multi-lined

   //ICDB( IDC_CHECK1,    dwi, FULL_NAMES        );
   if( IsDlgButtonChecked(hDlg, IDC_RADIO4) == BST_CHECKED )
   {
      dwi |= FULL_NAMES;
   }
   else if( IsDlgButtonChecked(hDlg, IDC_RADIO5) == BST_CHECKED )
   {
      // default relative names
      dwi &= ~(FULL_NAMES | ADD_REL_PATH);
   }
   else //if( IsDlgButtonChecked(hDlg, IDC_RADIO6) == BST_CHECKED )
   {
      dwi |= ADD_REL_PATH;
      dwi &= ~(FULL_NAMES);
   }

//#define  FLEFT_NAME              0x00004000  // use LEFT full name
//#define  FRIGHT_NAME             0x00008000  // use RIGHT full name
//#define  COMBINED_NAME           0x00010000  // use combination of left and right
   if( IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED )
   {
      dwi |= FLEFT_NAME;
      dwi &= ~(FRIGHT_NAME | COMBINED_NAME);
   }
   else if( IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED )
   {
      dwi |= FRIGHT_NAME;
      dwi &= ~(FLEFT_NAME | COMBINED_NAME);
   }
   else //if( IsDlgButtonChecked(hDlg, IDC_RADIO3) == BST_CHECKED )
   {
      dwi |= COMBINED_NAME;
      dwi &= ~(FLEFT_NAME | FRIGHT_NAME);
   }

   if( pcfds->cf_sTARGS.ta_sDI.di_dwOpts != dwi )  // return UPDATED OPTIONS
   {
      sprtf( "sl_IDOK: Setting new options (%x vs %x)"MEOR,
         pcfds->cf_sTARGS.ta_sDI.di_dwOpts,
         dwi );  // return UPDATED OPTIONS

      pcfds->cf_sTARGS.ta_sDI.di_dwOpts = dwi;  // return UPDATED OPTIONS
   }

   // get the head and tail text
   GetDlgItemText(hDlg, IDC_EDIT1, dlg_head, 256);
   GetDlgItemText(hDlg, IDC_EDIT2, dlg_tail, 256);
   if (strcmp(dlg_head, gszTagHead)) {
       strcpy(gszTagHead,dlg_head);
       gbChgTH = TRUE;
   }
   if (strcmp(dlg_tail, gszTagTail)) {
       strcpy(gszTagTail, dlg_tail);
       gbChgTT = TRUE;
   }

   return TRUE;

}


INT_PTR  sl_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = 0;
   DWORD wCmd = GET_WM_COMMAND_ID(wParam, lParam);
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP(hDlg, SAVE_ATOM);

   switch( wCmd )
   {
   case IDOK:
      if( sl_IDOK( hDlg, pcfds ) )
         EndDialog(hDlg, IDOK);
      iRet = TRUE;
      break;

   case IDCANCEL:
      bDnDlgInit = FALSE;
      EndDialog(hDlg, FALSE);
      iRet = TRUE;
      break;
   
   case IDC_COMBO_FILE:
      if( bDnDlgInit ) {
         AddAppPrompt(hDlg, IDC_COMBO_FILE, IDC_LABWARN );
         iRet = TRUE;
      }
      break;
   default:
       sprtf( "SAVE LIST: Command %u (%#x) not handled!"MEOR, wCmd, wCmd );
       break;

   }

   Post_WM_COMMAND( hDlg, wCmd, lParam, pcfds, 0 );   // from SAVE LIST

   return iRet;
}

// Do_SAVELISTDLG IDM_SAVELIST gdwFileOpts
// FIX20091125 v.2 IDD_SAVELIST2 "SaveList" SAVE FILE LIST2
INT_PTR CALLBACK SAVELISTDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = 0;
   switch(uMsg)
   {
   case WM_INITDIALOG:
      iRet = sl_WM_INITDIALOG( hDlg, lParam );
      break;

   case WM_COMMAND:
      iRet = sl_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      bDnDlgInit = FALSE;
      EndDialog(hDlg, FALSE);
      break;

   case WM_DESTROY:
      bDnDlgInit = FALSE;
      REMOVE_PROP( hDlg, SAVE_ATOM );
      break;

   }
   return iRet;
}

INT_PTR  Do_SAVELIST_DLG( PVOID pv )
{
   INT_PTR     ip;
   PCFDLGSTR   pcfds = (PCFDLGSTR)pv;

   dc4w_UI(TRUE);
   bDnDlgInit = FALSE;
   ip = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_SAVELIST2), // FIX20091125 v.2 IDD_SAVELIST2 "SaveList" SAVE FILE LIST2
      hwndClient,
      SAVELISTDLGPROC,     // dialog procedure
      (LPARAM) pcfds );
   dc4w_UI(FALSE);

   return ip;
}

// =====================
// END   SAVELIST DIALOG


// BEGIN VERIFY FILE COPY DIALOG
// =============================
typedef struct {
   UINT     uiType;
   LPTSTR   pText;
   LPTSTR   pCaption;

}MB2STR, * PMB2STR;

// a usable instance
static   MB2STR   _s_mb;

INT_PTR  v_WM_INITDIALOG( HWND hDlg, LPARAM lParam )
{
   INT_PTR iRet = TRUE;
   PMB2STR pmb = (PMB2STR)lParam;
   DWORD    dwt;
   LPTSTR   lpt;
   HWND     hwnd;

   SetDlgItemText( hDlg, IDC_EDIT1, pmb->pText );

   lpt = pmb->pCaption;
   if( lpt && *lpt )
      SetWindowText( hDlg, lpt );

   dwt = pmb->uiType & MB_TYPEMASK; // get the TYPE
   if( dwt == MB_YESNO )
   {
      hwnd = GetDlgItem(hDlg, IDC_COPYALL);
      if(hwnd)
      {
         EnableWindow( hwnd, FALSE );
         ShowWindow( hwnd, SW_HIDE );
      }
      hwnd = GetDlgItem(hDlg, IDC_ABORTALL);
      if(hwnd)
      {
         EnableWindow( hwnd, FALSE );
         ShowWindow( hwnd, SW_HIDE );
      }

      SetDlgItemText( hDlg, IDCANCEL, "NO!" );
      SetDlgItemText( hDlg, IDOK,     "Yes" );

   }


   CenterDialog( hDlg, hwndClient );   // centre it on the client

   return iRet;
}

INT_PTR CALLBACK VERIFYDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR iRet = 0;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      iRet = v_WM_INITDIALOG( hDlg, lParam );
      break;

   case WM_COMMAND:
      {
         DWORD wCmd = LOWORD(wParam);
         switch(wCmd)
         {
         case IDOK:
            EndDialog(hDlg, IDOK);
            iRet = TRUE;
            break;
         case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            iRet = TRUE;
            break;
         case IDC_COPYALL:
            EndDialog(hDlg, IDC_COPYALL);
            iRet = TRUE;
            break;
         case IDC_ABORTALL:
            EndDialog(hDlg, IDC_ABORTALL);
            iRet = TRUE;
            break;
         }
      }
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   //case WM_DESTROY:
   //   REMOVE_PROP( hDlg, COPY_ATOM );
   //   break;

   }

   return iRet;

} /* VERIFYDLGPROC */


//               (MB_ICONINFORMATION|MB_OKCANCEL) );
// similar to MessageBox()
INT_PTR MB2( HWND hWnd,          // handle to owner window
  LPTSTR lpText,     // text in message box
  LPTSTR lpCaption,  // message box title
  UINT uType         // message box style
)
{
   INT   i;
   PMB2STR   pmb = &_s_mb;

   ZeroMemory(pmb, sizeof(MB2STR) );

   pmb->uiType   = uType;
   pmb->pText    = lpText;
   pmb->pCaption = lpCaption;

   dc4w_UI(TRUE);
   i = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_VERIFY_COPY),   // was IDD_DIALOG1),  // Copy files template
      hwndClient,
      VERIFYDLGPROC,     // dialog procedure
      (LPARAM) pmb );
   dc4w_UI(FALSE);

   return i;
}
// END   VERIFY FILE COPY DIALOG
// =============================

// BEGIN EDIT ZIPUP

//IDD_ZIPUP DIALOG DISCARDABLE  0, 0, 186, 138
//STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
//CAPTION "ZIPUP OPTIONS"
//FONT 8, "MS Sans Serif"
//BEGIN
//    DEFPUSHBUTTON   "OK",IDOK,129,117,50,14
//    PUSHBUTTON      "Cancel",IDCANCEL,63,117,50,14
typedef struct tagZDSTR {
   UINT  uiEd;
   LPTSTR pMsg;
   UINT  uiID;
}ZDSTR, * PZDSTR;

TCHAR g_szRT[] = { "Runtime executable:" };
TCHAR g_szSW[] = { "Switches: (none = -a)" };
TCHAR g_szOUT[]= { "Output Path/File Name:"};
TCHAR g_szIN[] = { "Input File List:"  };
TCHAR g_szENV[]= { "Environment: Not used!" };
TCHAR g_szCMP[] = {"Compare of:"};

ZDSTR sZipupDlg[] = {
   { IDC_EDIT1, g_szRT,  IDC_ZIP1 },
   { IDC_EDIT3, g_szSW,  IDC_ZIP2 },
   { IDC_EDIT4, g_szOUT, IDC_ZIP3 },
   { IDC_EDIT5, g_szIN,  IDC_ZIP4 },
   { IDC_EDIT6, g_szENV, IDC_ZIP5 },
   { IDC_EDIT7, g_szCMP, IDC_ZIP6 },
   // *** termination ***
   { 0,         0,       0        }
};

LPTSTR   zu_get_cmd_ptr( PCMDLN pca, INT i )
{
   switch(i)
   {
   case 0:
      return( &pca->szCmd[0] );
   case 1:
      return( &pca->szSws[0] );
   case 2:
      return( &pca->szZip[0] );
   case 3:
      return( &pca->szInp[0] );
   case 4:
      return( &pca->szEnv[0] );
   case 5:
      return( &pca->szCmp[0] );  // directory with directory
   }
   return 0;
}

#define  ZU_GET_CMD(a)     &a->szCmd[0]
#define  ZU_GET_SW(a)     &a->szSws[0]
#define  ZU_GET_ZIP(a)     &a->szZip[0]
#define  ZU_GET_ENV(a)     &a->szEnv[0]
#define  ZU_GET_CMP(a)     &a->szCmp[0]

VOID  zu_set_edits( HWND hDlg, PCMDLN pca )
{
   PZDSTR   pzs = &sZipupDlg[0];
   LPTSTR   lps = pzs->pMsg;
   int      i = 0;
   LPTSTR   lpsrc, lpdst;
   PCMDLN   pczu = &g_sCmdLn[0]; // use the FIRST as DIALOG
   while(lps)
   {
      lpsrc = zu_get_cmd_ptr( pca, i );
      if( !lpsrc )
         break;

      lpdst = zu_get_cmd_ptr( pczu, i );

      SetDlgItemText( hDlg, pzs->uiEd, lpsrc );
//   CMDLN ws_sCmdLn[8];  // g_sCmdLn

      if(lpdst)
         strcpy(lpdst, lps);  // get COPY of BASE STRING above the EDIT line

      i++;     // bump count to next
      pzs++;   // to next
      lps = pzs->pMsg;
   }
}

INT_PTR  zu_WM_INITDIALOG( HWND hDlg )
{
   PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
// #define g_sZipArgs	sFW.fw_sZipArgs //   g_- Arguments for ZIP of a LIST
//   CMDLN    sCmdLn;  // various components of the command line
   PCMDLN   pcini = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
//#define  gbChgZp2    sFW.fw_bChgZp2
   memcpy( pca, pcini, sizeof(CMDLN) );   // get a COPY

   zu_set_edits( hDlg, pca ); // pass (allocated) argument structure

   return TRUE;
}

INT_PTR  zu_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD cmd = LOWORD(wParam);
   switch(cmd)
   {
   case IDC_EDIT1:
   case IDC_EDIT3:
   case IDC_EDIT4:
   case IDC_EDIT5:
   case IDC_EDIT6:
   case IDC_EDIT7:
      iRet = TRUE;
      break;
   }

   return iRet;
}

VOID  zu_get_edits( HWND hDlg, PCMDLN pca )
{
   PZDSTR   pzs = &sZipupDlg[0];
   LPTSTR   lps = pzs->pMsg;
   int      i = 0;
   LPTSTR   lpsrc, lpdst;
   PCMDLN   pczu = &g_sCmdLn[0]; // use the FIRST as DIALOG
   LPTSTR   lpb = &gszTmpBuf[0];
   while(lps)
   {
      lpsrc = zu_get_cmd_ptr( pca, i );
      if( !lpsrc )
         break;

      lpdst = zu_get_cmd_ptr( pczu, i );

//      SetDlgItemText( hDlg, pzs->uiEd, lpsrc );
//   CMDLN ws_sCmdLn[8];  // g_sCmdLn
//      if(lpdst)
//         strcpy(lpdst, lps);  // get COPY of BASE STRING above the EDIT line
      GetDlgItemText( hDlg, pzs->uiEd, lpb, 256 );
      if( strcmp( lpb, lpsrc ) ) {
         strcpy(lpsrc,lpb);
         gbChgZp2[i] = TRUE;
      }

      i++;     // bump count to next
      pzs++;   // to next
      lps = pzs->pMsg;
   }
}

INT_PTR  zu_OK( HWND hDlg )
{
   PCMDLN   pcini = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
   INT_PTR  iRet = TRUE;
   LPTSTR   lpb = &gszTmpBuf[0];

   //GetDlgItemText( hDlg, IDC_EDIT1, lpb, 256 );
   zu_get_edits( hDlg, pcini );    // so much to allow easy config of ZIPUP stuff

   return iRet;
}

INT_PTR CALLBACK ZIPUPDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR iRet = 0;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      iRet = zu_WM_INITDIALOG( hDlg );
      break;

   case WM_COMMAND:
      {
         DWORD wCmd = LOWORD(wParam);
         switch(wCmd)
         {
         case IDC_BUTTON1:
            b_doZIP = TRUE; // set to TRUE
         case IDOK:
            zu_OK( hDlg );
            EndDialog(hDlg, IDOK);
            iRet = TRUE;
            break;
         case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            iRet = TRUE;
            break;
            break;
         }
         if( !iRet )
            iRet = zu_WM_COMMAND( hDlg, wParam, lParam );

      }
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   //case WM_DESTROY:
   //   REMOVE_PROP( hDlg, COPY_ATOM );
   //   break;

   }

   return iRet;

} /* ZIPUPDLGPROC */

INT_PTR  Do_IDM_EDITZIPUP( VOID )
{
   INT_PTR     iOK = 0;
   //if( !gfBusy )
   b_doZIP = FALSE; // set to FALSE

   if( !IsBusy() )
   {
      dc4w_UI(TRUE);
      iOK = DialogBoxParam(
         g_hInst,      // handle to module
         MAKEINTRESOURCE(IDD_ZIPUP),   // dialog box template
         hwndClient,    // handle to owner window
         ZIPUPDLGPROC,   // dialog box procedure
         (LPARAM)0 ); // initialization value
      dc4w_UI(FALSE);
#ifdef ADD_ZIP_SUPPORT
      if( b_doZIP ) {
         WriteZIPList( &g_sThreadArgs, 2 ); // #ifdef ADD_ZIP_SUPPORT
      }
#endif 
   }
   return iOK;
}


// END EDIT ZIPUP OPTIONS


// BEGIN RENAME ZIP FILE


/* first check if the path is valid */
//if( dir_isvaliddir(path) )
//if( dir_isvaliddir2( path, &g_sFD ) ) { bFile = FALSE; }
//        else if( dir_isvalidfile2( path, &g_sFD ) ) { bFile = TRUE; }
static TCHAR _s_tmpbuf[264];
static HWND  _s_hCtl;
static DWORD _s_dwLCnt = 0;

INT_PTR  rz_OK( HWND hDlg )
{
//   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb = GetStgBuf();
   PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
   LPTSTR   lpsrc = zu_get_cmd_ptr( pca, 2 );   // get OUT ZIP FILE NAME

   *lpb = 0;
   if( _s_dwLCnt )
      GetComboSel( _s_hCtl, lpb, 0 );
   else
      GetDlgItemText( hDlg, IDC_EDIT1, lpb, 256 );

   if( strcmpi( lpb, lpsrc ) )
   {
      // we HAVE a CHANGE
      //gbChgZp2[ZP_ZIPFILE] = TRUE;
      //strcpy(lpsrc, lpb);
      strcpy( g_szRenZip, lpb ); // pass back the NEW ZIP name
      return TRUE;

   }
   else
   {
      MB( hDlg, "ERROR:"MEOR
         "Can NOT click OK without NEW zip name!"MEOR
         "[%s]",
         "NEED NEW FILE NAME",
         (MB_ICONINFORMATION|MB_OK) );

   }
   return FALSE;
}


INT_PTR  rz_WM_INITDIALOG( HWND hDlg )
{
   PLE   ph = &gsZipList;    // get the ZIP file list
   //LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb = _s_tmpbuf;
   LPTSTR   lpsrc;
   PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
// #define g_sZipArgs	sFW.fw_sZipArgs //   g_- Arguments for ZIP of a LIST
//   CMDLN    sCmdLn;  // various components of the command line
   PCMDLN   pcini = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
//#define  gbChgZp2    sFW.fw_bChgZp2
   HWND     hwnd; // = COMBO _s_hCtl or edit

   memcpy( pca, pcini, sizeof(CMDLN) );   // get a COPY

   // slower, get pointer, or
   //lpsrc = zu_get_cmd_ptr( pca, 2 );   // 0=runtime, 1=switches, and 2 is the ZIP FILE
   lpsrc = ZU_GET_ZIP(pca);   // get address of zip file name buffer

   ListCount2(ph, &_s_dwLCnt);   // get COUNT of previous

   _s_hCtl  = GetDlgItem( hDlg, IDC_COMBO1 );

   //zu_set_edits( hDlg, pca ); // pass (allocated) argument structure
   if(( lpsrc == 0 ) || ( *lpsrc == 0 ))  // *** MUST HAVE A SOURCE ZIP FILE ***
   {
      EndDialog(hDlg, IDABORT);
      return -1;
   }

   g_szDlgZip[0] = 0;
   // expand any 'relative' items, if possible
   /* convert the pathname to an absolute path */
   //_fullpath(dl->rootname, path, sizeof(dl->rootname));
   if( !_fullpath(lpb, lpsrc, 256) )
   {
      strcpy( lpb, lpsrc );
   }

   strcpy( g_szDlgZip, lpb );

   //SetDlgItemText( hDlg, IDC_EDIT1, lpsrc );
   //SetDlgItemText( hDlg, IDC_EDIT1, lpb );

   if( dir_isvalidfile2( lpb, &g_sFD ) )
   {
      strcat(lpb, " - ok");
   }
   else
   {
      strcat(lpb, " - *** Not valid ***");
   }

   SetDlgItemText( hDlg, IDC_CURR_ZIP, lpb );

   if( _s_dwLCnt )
   {
      // BUT if we have a LIST
      hwnd = GetDlgItem( hDlg, IDC_EDIT1 );
      EnableWindow( hwnd, FALSE );
      ShowWindow( hwnd, SW_HIDE );
      if( !FillComboBox( _s_hCtl, ph, g_szDlgZip ) )
      {
         EndDialog(hDlg, IDABORT);
         return -1;
      }
   }
   else
   {
      hwnd = _s_hCtl;
      EnableWindow( hwnd, FALSE );
      ShowWindow( hwnd, SW_HIDE );
      SetDlgItemText( hDlg, IDC_EDIT1, g_szDlgZip );
   }

   CenterDialog( hDlg, hwndClient );

   return TRUE;

}

//VOID     AddAppPrompt( HWND hDlg, UINT uiCombo, UINT uiLab )
VOID ShowComboSel( HWND hDlg, HWND hCtl, UINT uiLab )
{
   if( _s_dwLCnt )
   {
      PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
      LPTSTR   lpsrc = ZU_GET_ZIP(pca);   // get address of zip file name buffer
      LPTSTR   lpb  = GetStgBuf();
   
      *lpb = 0;
      if( GetComboSel( hCtl, lpb, 0 ) )
      {
         if(( strcmpi( lpb, lpsrc )  ) &&
            ( dir_isvalidfile( lpb ) ) )
         {
            strcpy(lpb, " *** Valid File *** - Need overwrite permission.");
         }
      }
   
      SetDlgItemText( hDlg, uiLab, lpb );
   }
}

static BOOL fEditFocus = FALSE;
INT_PTR  rz_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = 0;
   DWORD cmd = LOWORD(wParam);
   HWND  hwnd = (HWND)lParam;
   DWORD info = HIWORD(wParam);
   PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
   LPTSTR   lpsrc = ZU_GET_ZIP(pca);   // get address of zip file name buffer
   //case IDC_COMBO_SAVE:
   if( cmd == IDC_COMBO1 )
   {
      // AddAppPrompt( hDlg, IDC_COMBO_SAVE, IDC_EXISTS );
      ShowComboSel( hDlg, _s_hCtl, IDC_WARN_OVR );
      // * Combo Box Notification Codes *
      switch(info)
      {
//#define CBN_ERRSPACE        (-1)
//#define CBN_SELCHANGE       1
//#define CBN_DBLCLK          2
      case CBN_SETFOCUS:
         fEditFocus = TRUE;
         iRet = TRUE;
         break;
      case CBN_KILLFOCUS:  // 4
         fEditFocus = FALSE;
         iRet = TRUE;
         break;
      case CBN_EDITCHANGE: //  5
//         ShowComboSel( hDlg, _s_hCtl, IDC_WARN_OVR );
         // like - AddAppPrompt( hDlg, IDC_COMBO_SAVE, IDC_EXISTS );
         break;
//#define CBN_EDITUPDATE      6
//#define CBN_DROPDOWN        7
//#define CBN_CLOSEUP         8
//#define CBN_SELENDOK        9
//#define CBN_SELENDCANCEL    10
      }
   }
   else if( cmd == IDC_EDIT1 )
   {
      switch(info)
      {

      case EN_KILLFOCUS:
         fEditFocus = FALSE;
         iRet = TRUE;
         break;

      case EN_SETFOCUS:
         fEditFocus = TRUE;
         iRet = TRUE;
         break;
//Edit Control Messages
//The following messages are used with edit controls: 
//EM_CANUNDO
//EM_CHARFROMPOS
//EM_EMPTYUNDOBUFFER
//EM_FMTLINES
//EM_GETFIRSTVISIBLELINE
//EM_GETHANDLE
//EM_GETIMESTATUS 
//EM_GETLIMITTEXT
//EM_GETLINE
//EM_GETLINECOUNT
//EM_GETMARGINS
//EM_GETMODIFY
//EM_GETPASSWORDCHAR
//EM_GETRECT
//EM_GETSEL
//EM_GETTHUMB
//EM_GETWORDBREAKPROC
//EM_LIMITTEXT
//EM_LINEFROMCHAR
//EM_LINEINDEX
//EM_LINELENGTH
//EM_LINESCROLL
//EM_POSFROMCHAR
//EM_REPLACESEL
//EM_SCROLL
//EM_SCROLLCARET
//EM_SETHANDLE
//EM_SETIMESTATUS 
//EM_SETLIMITTEXT
//EM_SETMARGINS
//EM_SETMODIFY
//EM_SETPASSWORDCHAR
//EM_SETREADONLY
//EM_SETRECT
//EM_SETRECTNP
//EM_SETSEL
//EM_SETTABSTOPS
//EM_SETWORDBREAKPROC
//EM_UNDO
//EN_ALIGN_LTR_EC
//EN_ALIGN_RTL_EC
//EN_CHANGE
//EN_ERRSPACE
//EN_HSCROLL
//EN_KILLFOCUS
//EN_MAXTEXT
//EN_SETFOCUS
      case EN_UPDATE:
         {
//            PCMDLN   pca = &g_sZCmdLn; // =  g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
//            LPTSTR   lpsrc = ZU_GET_ZIP(pca);   // get address of zip file name buffer
            LPTSTR   lpb = GetStgBuf();
            *lpb = 0;
            GetDlgItemText( hDlg, IDC_EDIT1, lpb, 256 );
            if(( strcmpi( lpb, lpsrc )  ) &&
               ( dir_isvalidfile( lpb ) ) )
            {
               strcpy(lpb, " *** Valid Destination *** - Need overwrite permission.");
            }

            SetDlgItemText( hDlg, IDC_WARN_OVR, lpb );

         }
         break;
//EN_VSCROLL
//WM_COMMAND
//WM_COPY
//WM_CTLCOLOREDIT
//WM_CUT
//WM_PASTE
//WM_UNDO 

      }

   }
   return iRet;
}

INT_PTR CALLBACK RENZIPDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR iRet = 0;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      iRet = rz_WM_INITDIALOG( hDlg );
      break;

   case WM_COMMAND:
      {
         DWORD wCmd = LOWORD(wParam);
         switch(wCmd)
         {
         case IDOK:
            iRet = TRUE;
            if( rz_OK( hDlg ) )
            {
               EndDialog(hDlg, IDOK);
               break;
            }
            break;   // can NOT close with OK when names are the SAME
            // ie NO fall through 
         case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            iRet = TRUE;
            break;
         }
         if( !iRet )
            iRet = rz_WM_COMMAND( hDlg, wParam, lParam );

      }
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   //case WM_DESTROY:
   //   REMOVE_PROP( hDlg, COPY_ATOM );
   //   break;

   }

   return iRet;

} /* RENZIPDLGPROC */

extern   BOOL dir_copy2( LPTSTR psrcname, LPTSTR relname, LPTSTR pdroot, DWORD dwVerFlag );
// setup for the above COPY function
extern   BOOL  SetBusyCopy( LPTSTR pmsg );
extern   void  SetNotBusy(void);

BOOL  dlg_dir_copy2( LPTSTR psrcname, LPTSTR relname, LPTSTR pdroot, DWORD dwVerFlag )
{
   BOOL  iOK = FALSE;
   //if( SetBusy( BT_COPYFILES ) )
   if( SetBusyCopy( "Got IDM_RENZIP" ) )
   {
      // could create a THREAD, but ... *** TBD ***
      //ghThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wd_copy, (LPVOID) pcfds,   // = &g_sCFDLGSTR;   // get GLOBAL structure
      //   0, &threadid );
      //if( ghThread == NULL )
      //   wd_copy( (LPVOID) pcfds ); // &g_sCFDLGSTR;   // get GLOBAL structure
      iOK = dir_copy2( g_szDlgZip, g_szZipRel, g_szZipRoot, dwVerFlag );  // MAKE_BACKUP
      SetNotBusy();  // set NOT busy
   }
   //else { BusyError( "Got IDM_RENZIP" ); }

   return iOK;
}

VOID  SetBPS( LPTSTR pb, double dsz, double etm )
{
   double bps;
   if( etm > 0.0 )
   {
      bps = dsz / etm;
      if( bps < 1000.0 )
      {
         sprintf( pb, "%0.5f B/s", bps );
      }
      else if( bps < 1000000.0 )
      {
         sprintf( pb, "%0.5f KB/s",
            ( bps / 1000.0 ) );
      }
      else
      {
         sprintf( pb, "%0.5f MB/s",
            ( bps / 1000000.0 ) );
      }
   }
   else
   {
      strcpy(pb, "unknown>");
   }
}

INT_PTR  Do_Verify_NNZ( VOID )
{
   INT_PTR iOK = 0;
   LPTSTR   psrcname = g_szDlgZip;
   DWORD    dwver = 0;
   INT      i;

   //if( dir_isvalidfile2( g_szDlgZip, &g_sFDL ) )
   if( dir_isvalidfile2( psrcname, &g_sFDL ) )
   {
      // only if we have something VALID to RENAME
      LPTSTR   lpm = g_szMsg; // [1024] buffer
      LPTSTR   lpb = &gszTmpBuf[0];
      PFD      pfd1 = &g_sFDL;
      PFD      pfd2 = &g_sFDR;
      LPSYSTEMTIME pst1 = &g_sST1;
      LPSYSTEMTIME pst2 = &g_sST2;
      BOOL     b1, b2;
      LARGE_INTEGER  li1, li2, lit;

      b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
      *lpb = 0;
      AppendDateTime( lpb, pst1 );
      li1.LowPart  = pfd1->nFileSizeLow;
      li1.HighPart = pfd1->nFileSizeHigh;
      lit.QuadPart = li1.QuadPart;  // set total

      sprintf(lpm,"Copy save zip file from"MEOR
         "%s %s ",
         g_szDlgZip,
         lpb );

      sprintf(EndBuf(lpm), "%10I64u "MEOR, li1.QuadPart );

      if( dir_isvalidfile2( g_szRenZip, pfd2 ) ) // pass back the NEW ZIP name
      {
         // this file will have to be MOVED or DELETED to do what is required
         b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );
         li2.LowPart  = pfd2->nFileSizeLow;
         li2.HighPart = pfd2->nFileSizeHigh;
         *lpb = 0;
         AppendDateTime( lpb, pst2 );
         sprintf(EndBuf(lpm),"NOTE: *MUST* delete or move file -"MEOR
         "%s %s ",
         g_szRenZip,
         lpb );

         sprintf(EndBuf(lpm), "%10I64u "MEOR, li2.QuadPart );

         strcat(lpm, "Setting backup to garbage first."MEOR );

         dwver = MAKE_BACKUP; // set to COPY the about to be overwritten
         // file to the 'safety net' cache - rubbish bin

      }


      // DIVIDE UP the DESTINATION
      // into a (possible) Drive root, like D:,
      // full relative directory, like GTools\Tools\Dc4w\New-Name.ZIP
      // and finally a title - just New-Name.ZIP
      // the SOURCE (full destination name) is in g_szRenZip
      if( g_szRenZip[0] )
      {
         // destination does NOT exist. This may be a 'missing' directory,
         // and or a NEW name for the ZIP CreateFile()
         LPTSTR   pnp = &g_szNewPath[0];
         HANDLE   hFind;
         DWORD    ipos, dwl;
         LPTSTR   p, pel, pdest;

         *pnp = 0;
         strcpy(lpb, g_szRenZip);   // get the NEW name
         ipos = InStr( g_szRenZip, ":" );
         //p = strchr(lpb,':');
         if(ipos)
         {
            p = Left( g_szRenZip, ipos ); // add in the Drive
            strcpy( g_szZipRoot,  p    ); // This is the pdroot to pass to dir_copy2
            strcpy(pnp, p ); // add in the Drive
            strcat(pnp, "\\*.*");
            hFind = FindFirstFile( pnp, &g_sFD );  // try find the root of a drive
            if( VFH(hFind) )
            {
               // success
               FindClose(hFind); // close the FIND
               sprintf(EndBuf(lpm), "Drive %s appears valid."MEOR, p );
            }
            else
            {
               // WOW - FAILED to 'find' anything on this DRIVE. It could
               // be totally empty - nah won;t consider that ...
               sprintf(EndBuf(lpm), "ADVICE: Drive %s appears INVALID!"MEOR, p );

            }
            strcpy( pnp, p ); // add in the Drive
            p = &g_szRenZip[ipos];  // get to the RELATIVE name
         }
         else
         {
            p = g_szRenZip;   // no drive so start at beginning
         }

         if( *CharPrev(pnp, (pnp + strlen(pnp)) ) != '\\' )
            strcat(pnp, "\\");

         if( *p == '\\' )
            p++;  // skip this FIRST

         strcpy( g_szZipRel, p );   // keep the RELATIVE path of the DESTINATION

         //while( (pel = strchr(p, '\\')) != NULL )
         pel = strchr(p, '\\');
         while( pel )
         {
            /* found another element ending in slash. incr past the "\" */
            pel++;
            pdest = &pnp[strlen(pnp)];
            dwl   = pel - p;
            //strncpy(pdest, p, pel - p);
            //pdest[pel - p] = '\0';
            strncpy(pdest, p, dwl);
            pdest[dwl] = '\0';
            /* can create subdir if necessary */
            if( !dir_isvaliddir(pnp) )
            {
               break;
            }
            p = pel;
            pel = strchr(p, '\\');
         }
         if(pel)  // maybe can create directory
         {
            sprintf(EndBuf(lpm),"Note: Appears directory"MEOR
               "[%s] will need to be CREATED!"MEOR,
               pnp );
            while( pel )
            {
               /* found another element ending in slash. incr past the "\" */
               pel++;
               pdest = &pnp[strlen(pnp)];
               dwl   = pel - p;
               //strncpy(pdest, p, pel - p);
               //pdest[pel - p] = '\0';
               strncpy(pdest, p, dwl);
               pdest[dwl] = '\0';
               /* can create subdir if necessary */
               // if( !dir_isvaliddir(pnp) ) { break; }
               p = pel;
               pel = strchr(p, '\\');
            }
         }
         else
         {
            sprintf(EndBuf(lpm),"Note: Destination directory"MEOR
               "[%s] is OK."MEOR,
               pnp );
         }

         strcpy( g_szZipTit, p );
         sprintf(EndBuf(lpm),"Copy ZIP file name [%s]"MEOR, p );

         strcat(lpm, MEOR"   *** Click Yes to copy. ***, or"MEOR
                         "       Click NO to ABORT!"MEOR );
      }
      else  // *** MUST *** be given a DESTINATION NAME (different to source name)
      {
         return FALSE;
      }

//   iOK = DialogBoxParam(
//         g_hInst,      // handle to module
//         MAKEINTRESOURCE(IDD_RENZIP),   // dialog box template
//         hwndClient,    // handle to owner window
//         RENZIPDLGPROC,   // dialog box procedure
//         (LPARAM)0 ); // initialization value
//   if( iOK == IDOK )
//   {

//   }
      // MessageBox(
      iOK = MB2(NULL,lpm,
               "*** Verify File Copy ***",
               (MB_ICONINFORMATION|MB_YESNO));
      // SD_MB_DEF );   // (MB_ICONINFORMATION|MB_OKCANCEL));

      if( iOK == IDOK )
      {
         double   etm, dsz;
         LPTSTR   pb = GetStgBuf();

         iOK = TRUE;
         // the answer is YES
         i = SetBTime();
         iOK = dlg_dir_copy2( g_szDlgZip, g_szZipRel, g_szZipRoot, dwver );  // MAKE_BACKUP
         etm = GetETime(i);
         dsz = (double)lit.QuadPart;
         SetBPS( pb, dsz, etm );

         if( dwver & MAKE_BACKUP )
         {
            lit.QuadPart += li2.QuadPart;
            strcpy(lpm,"Copy(x2) of ");
         }
         else
         {
            strcpy(lpm,"Copy of ");
         }

         {
            //TCHAR bbps[64];
            TCHAR bsec[64];
            //LPTSTR   pb = bbps;
            LPTSTR   ps = bsec;
            //LPTSTR pbps = Dbl2Str(bps, 5);
            LPTSTR psec = Dbl2Str(etm, 5);
            //LPTSTR   pt = GetI64StgRLen2( &lit, 8 );

            //sprintf( pb, "%0.5f", bps );
            sprintf( ps, "%0.5f", etm );
            //sprintf(EndBuf(lpm), "%10I64u bytes disk transfer"MEOR

            sprintf(EndBuf(lpm), "%s of disk transfer"MEOR
               " time = took %s secs (xrate %s) (tm=%d)",
               GetI64StgRLen2( &lit, 8),
               psec, // = Dbl2Str(etm, 5),
               pb, // = Dbl2Str(bps, 5),
               i );
         }

         if( iOK )
         {
            BOOL  bchg = FALSE;
            // successful COPY of source to destination name
            // DECIDE if should delete the first - no for now
            strcat(lpm, " ok."MEOR);
            Add2SList( &gsZipList, &bchg, g_szRenZip ); // add this COPY
            if( bchg ) // add this COPY
            {
               bChgZLst = TRUE;
               strcat(lpm, "Added ZIP to INI output/input list"MEOR );

            // potentially - if NOT already there - and flag a correction
            // in INI on exit (with save) ...
            }

         }
         else
         {
            strcat(lpm, MEOR"Oops: My new dir_copy2 FAILED!"MEOR );
         }

         if( dwver & MAKE_BACKUP )
         {
            // GetI64StgRLen2
            //sprintf(EndBuf(lpm),"NOTE: %10I64u bytes recovered on EMPTY GARBAGE BIN!"MEOR,
            sprintf(EndBuf(lpm),"NOTE: %s recovered on * EMPTY GARBAGE BIN! *"MEOR,
               GetI64StgRLen2( &li2, 8 ) );
         }

         {
            PCMDLN   pcini = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
            // memcpy( pca, pcini, sizeof(CMDLN) );   // get a COPY
            LPTSTR lpsrc = ZU_GET_ZIP(pcini);   // get address of zip file name buffer

            sprintf(EndBuf(lpm),"Delete of %s will remove %s bytes."MEOR,
               lpsrc,
               GetI64StgRLen2( &li1, 8 ) );
         }

         sprtf(lpm); // add to LOG file
         // simplest, fastest confirmation form
         MB(hwndClient,
            lpm,
            APPNAME,
            MB_OK|MB_ICONINFORMATION);

      }
   }

   return iOK;
}

INT_PTR  Do_IDM_FILE_RENZIP( VOID )
{
   INT_PTR     iOK = 0;
   //if( !gfBusy )
   if( !IsBusy() )
   {
      dc4w_UI(TRUE);
      iOK = DialogBoxParam(
         g_hInst,      // handle to module
         MAKEINTRESOURCE(IDD_RENZIP),   // dialog box template
         hwndClient,    // handle to owner window
         RENZIPDLGPROC,   // dialog box procedure
         (LPARAM)0 ); // initialization value
      if( iOK == IDOK )
      {
         // a NEW name has been entered
         // strcpy( g_szRenZip, lpb ); // pass back the NEW ZIP name
         if( Do_Verify_NNZ() )
         {
            // success
         }
      }

      dc4w_UI(FALSE);
   }
   // _s_dwLCnt = 0;
   return iOK;
}

// END RENAME ZIP FILE

// BEGIN   VERIFY FILE DELETE DIALOG
INT_PTR MB_DELETE( HWND hWnd,          // handle to owner window
  LPTSTR lpText,     // text in message box
  LPTSTR lpCaption,  // message box title
  UINT uType         // message box style
)
{
   INT   i;
   PMB2STR   pmb = &_s_mb;

   ZeroMemory(pmb, sizeof(MB2STR) );

   pmb->uiType   = uType;
   pmb->pText    = lpText;
   pmb->pCaption = lpCaption;

   dc4w_UI(TRUE);
   i = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_VERIFY_DELETE),   // was IDD_DIALOG1),  // Copy files template
      hwndClient,
      VERIFYDLGPROC,     // dialog procedure
      (LPARAM) pmb );
   dc4w_UI(FALSE);

   return i;
}
// END   VERIFY FILE DELETE DIALOG
// ===============================

// eof - dc4wDlg.c
