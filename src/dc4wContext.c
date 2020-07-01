
// dc4wContext.c
// RIGHT MOUSE BUTTON CONTEXT MENU

#include "dc4w.h"
extern   BOOL  DeleteOneIsOK( PCFDLGSTR pcfds );

#define DONEUPDCHK2 // a FAST one file update (L->R)
/* =================================
typedef struct tagLSTSTATS {
   DWORD dwid;    // id of stats
   DWORD dwleftcnt;  // total in left
   DWORD dwrightcnt; // total in right
   DWORD dwtot;   // total in compare list
   DWORD dwsame;  // the same
   DWORD dwsamex; // diff date, but same on compare
   DWORD dwnewer; // newer
   DWORD dwolder; // older
   DWORD dwleft;  // only left
   DWORD dwright; // only right
   DWORD dwinview;   // count in view = displayable
   DWORD dwdeleted;  // excluded from list
   DWORD dwunk;   // does not belong here
}LSTSTATS, * PLSTSTATS;
  ====================================== */

// if BUSY (on thread working or in dialog asking)
extern   BOOL     gfBusy;  // only add ABORT to context menu
extern   INT      giSelection;    // = -1 if NONE, or is selected row in table
extern   BOOL  CopyOneIsOK( PCFDLGSTR  pcfds );
extern BOOL Got_Sel_File( PTSTR lpb );

// forward references
VOID  SetUpdateLabel(LPTSTR lpb, PCFDLGSTR pcfds);
DWORD GetListableCount( VOID );
DWORD GetOutlineCount( VOID );    // get counts for current view - fill in g_sOutCnt
DWORD GetCopiableCount( VOID );
VOID  SetDeleteLabel(LPTSTR lpb, PCFDLGSTR pcfds);
VOID  AppendStateFlag(LPTSTR lpb, INT iState, DWORD dwFlag );
VOID  AppendSState(LPTSTR lpb, INT iState );


// FIX20080121 - Remove DUPLICATES in POPUP context menu
#define  MAX_MENU_ITEMS 32
UINT  uMenuCount = 0;
UINT  uMenuItems[MAX_MENU_ITEMS];

//#define  AM(a,b)  AppendMenu(hMenu,uFlags,a,b)
#define  AM(a,b)  Append_2_Menu(hMenu,uFlags,a,b)

// data
CC    g_sListCnt;
CC    g_sOutCnt;

// FIX20080121 - Remove DUPLICATES in POPUP context menu
BOOL Not_Yet_Added( UINT uCmd )
{
   UINT  ui;
   for( ui = 0; ui < uMenuCount; ui++ ) {
      if( uMenuItems[ui] == uCmd )
         return FALSE;
   }
   // add it now ...
   if( uMenuCount < MAX_MENU_ITEMS )
      uMenuItems[uMenuCount++] = uCmd;

   return TRUE;
}

BOOL Append_2_Menu( HMENU hMenu, UINT uFlags, UINT uCmd, PTSTR lpb )
{
   BOOL  bRet = FALSE;
   if( Not_Yet_Added( uCmd ) )
      bRet = AppendMenu( hMenu, uFlags, uCmd, lpb );
   return bRet;
}

#ifdef   ADD_EXCLUDE_FILE
void Add_Exclude_File( HMENU hMenu, COMPITEM ci, INT state, PTSTR lpb,
                      UINT uFlags, PINT piCnt)
{
   INT iCnt = *piCnt;
   if( giSelection != -1 ) {
      // have a SELECTION
      if(ci) {
         DIRITEM diritem;  // = file_getdiritem(compitem_getleftfile(ci));
         if( state == STATE_FILERIGHTONLY )
            diritem = file_getdiritem(compitem_getrightfile(ci));
         else
            diritem = file_getdiritem(compitem_getleftfile(ci));

         if(diritem) {
            // we have our FILE names
            //char * relname  = dir_getrelname(diritem);
            //char * cp = relname;
            //if(*cp=='.')   cp++;
            //if(*cp=='\\')  cp++;
            char * cp = dir_getnameptr(diritem);

            sprintf(lpb, "Exclude [%s]", cp);

            if( AM( IDM_EXCLUDE, lpb ) ) {
               iCnt++;
               *piCnt = iCnt;
            }
            //dir_freerelname( diritem, relname );
         }
      }  // got the COMPITEM of the SELECTION
   }
}
#endif   // ADD_EXCLUDE_FILE

// There are some interesting posibilities concerning the current file list on view
// - Could be ONLY left
// - or ONLY right
// - or ONLY left and ONLY right
// else mixed display
//
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : tb_WM_RBUTTONDOWN
// Return type: BOOL 
// Arguments  : HWND hwnd
//            : WPARAM wParam
//            : LPARAM lParam
// Description: Put up a context menu
//              
// NOTE: This appears to also send a WM_INITMENUPOPUP, perhaps when
// the 'command' is also in the main MENU (IDR_MENU1)!!!
//
// some list statisics
//   DWORD    dwTLeft;    // total in left
//   DWORD    dwTRite;    // total in right
//   DWORD    dwSame;     // total the same
//   DWORD    dwDiff;     // total different
//   DWORD    dwNewer;    // diff AND NEWER == POSSIBLE UPDATE!!!
//   DWORD    dwLeft;     // total left only
//   DWORD    dwRite;     // total right only
//   DWORD    dwUnk;      // this should be ZERO
// BUT NOTE the above are being ABANDONED in favour of
// g_ListTotal - total in 'combined' file list
// g_LeftCnt   - total in left tree
// g_RightCnt  - Total in right tree
// g_SameCnt   - if folders are equal these 4 totals would be the SAME

// g_NewerCnt  - if date newer - ie ready for update
// g_OlderCnt  - Maybe destination ALSO modified
// g_LeftOnly  - files ONLY in left tree
// g_RightOnly - files ONLY in right tree
//
//       if( !gbDelOn )
///////////////////////////////////////////////////////////////////////////////
//BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, lpTable ptab, WPARAM wParam, LPARAM lParam )
//BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, PVOID ptab_NU, WPARAM wParam, LPARAM lParam )
BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
   POINT pnt;
   HMENU hMenu;
   HWND  hWnd = hwndClient;   // get PARENT handle

   pnt.x = LOWORD(lParam);
   pnt.y = HIWORD(lParam);
   //ClientToScreen(hWnd, &pnt);
   //ClientToScreen(hwnd, &pnt);
   hMenu = CreatePopupMenu(); // create a POPUP Menu
   uMenuCount = 0;   // clear ADDED commands

   if(hMenu)  
   {
      INT         iCnt  = 0;
      UINT        uFlags = MF_STRING | MF_BYCOMMAND;
      UINT        uOpts;
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;
      DWORD       dwosv = pcfds->dwCpyOpts;
      LPTSTR      lpb   = GetNxtBuf();  //&gszTmpBuf[0];
      BOOL        bZip;
      INT         itot = view_gettotcount(current_view);
      INT         icnt = view_getrowcount(current_view);
      INT         icnt2;   // actionable items per present options
      INT         state;   // = compitem_getstate(ci);
      DWORD       dwFlag;
      COMPITEM    ci = view_getitem( current_view, giSelection );
      DIRITEM     diritem = 0;
      PCPYTST     pct = &pcfds->cf_sDeleTest;
      //LRESULT  lr;
      //RECT     rc;
      sprtf( "CONTEXT MENU: Right button down at %d,%d"MEOR,
         GET_X_LPARAM(lParam),
         GET_Y_LPARAM(lParam) );
      uOpts = TPM_LEFTALIGN | TPM_TOPALIGN;
      // uOpts = TPM_CENTERALIGN | TPM_VCENTERALIGN; // options
      // we COULD fiddle with these position options, but it seems the
      // windows UI already does some of this so forget it for now ...
      // GetClientRect(hwnd, &rc);
     	// Add the cut, copy, paste verbs enabled or disable accordingly
      // uFlags = MF_STRING | MF_BYCOMMAND;

      if( gfBusy )
      {
         if( AM( IDM_ABORT, "ABORT!" ) )
            iCnt++;
         goto Add_Sep;
      }

      // if a ZIP then some things are NOT possible
      // like update / copy *** TBD *** extract the file data, and do the COPY
      // if it is FROM the ZIP to the RIGHT (real) folder
      bZip = view_haszip(current_view);
      state = compitem_getstate(ci);
      pcfds->cf_sCopyTest.ct_iState = state; // get the ci state
      pcfds->cf_sDeleTest.ct_iState = state;
      dwFlag = compitem_getflag(ci);   // and the file compare flags

      //pcfds->cf_sCPYTST.ct_iState = view_getstate(current_view, giSelection);
      if( view_isexpanded(current_view) )
      {
         // NOTE: Since the selection is EXPANDED,
         // then giSelection will/could be -1 !!!
         DWORD    dwc = view_haschange( current_view, TRUE );
         //state = compitem_getstate(ci);
         //pcfds->cf_sCopyTest.ct_iState = state; // get the ci state
         //pcfds->cf_sDeleTest.ct_iState = state;
         //dwFlag = compitem_getflag(ci);   // and the file compare flags
         if(dwc)
         {
            //sprintf(lpb, "Write Changes...(%d)", dwc );
            //sprintf(lpb, "File Changes...(%d)", dwc );
            sprintf(lpb, "Write Differences (%d)...", dwc );
            if( AM( IDM_WRITEDIFF, lpb ) )
               iCnt++;
         }

         if( !bZip ) // if we are NOT a ZIP compare
         {
            // and we are viewing the 'difference' between the two files, and
            // the next 'likely' action is to immediately UPDATE this 'pair'
            // if it is left only, then offer to "Copy [filename] Left Only"
            // if it is right only, then offer to "Delete [filename] Right Only"
            // if it is newer, offer "Update [filename] with Newer"
            // if it is OLDER, offer "OVERWRITE [filename] with Older"
            // if it is the SAME, then offer nothing here
            pcfds->cf_sCopyTest.ct_szSTitle[0] = 0;
//            pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_SAME;
//            pcfds->dwCpyOpts = INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_SAME;
            pcfds->dwCpyOpts = outline_include;
            if(( state != STATE_SAME  ) &&
               ( CopyOneIsOK( pcfds ) ) ) // in expanded the compitem is already set - selected
            {
               // offer to COPY or DELETE
               SetUpdateLabel(lpb, pcfds);
               if( AM(IDM_UPDFILE, lpb) )
                  iCnt++;
            }
   
            //pcfds->cf_sDeleTest.ct_szSTitle[0] = 0;
            //if( DeleteOneIsOK( pcfds ) )
            //{
            //   SetDeleteLabel(lpb, pcfds);
            //   if( AppendMenu(hMenu, uFlags, IDM_DELETEFILE, lpb) )
            //      iCnt++;
            //}
            pcfds->dwCpyOpts = dwosv;

            if( view_getrowcount(current_view) > 1 )
            {
               if( AM( IDM_VIEW_NEXT, "Next Compare" ) )
                  iCnt++;
            }

            if( AM( IDM_OUTLINE, "To Outline" ) )
               iCnt++;

            if( AM( IDM_COPYFILES, "Copy Files..." ) )
               iCnt++;
         }
         else  // IT IS ZIP
         {
            if( view_getrowcount(current_view) > 1 )
            {
               if( AM( IDM_VIEW_NEXT, "Next Compare" ) )
                  iCnt++;
            }
            if( AM( IDM_OUTLINE, "To Outline" ) )
               iCnt++;
         }

         //if( AM( IDM_OUTLINE, "To Outline" ) )
         //   iCnt++;
#ifdef   ADD_EXCLUDE_FILE
         Add_Exclude_File(hMenu, ci, state, lpb, uFlags, &iCnt);
#else
         //        MENUITEM "Exclude &File",               IDM_EXCLUDE
         if( Got_Sel_File( lpb ) ) {
             PTSTR lpb2 = GetNxtBuf();  // &gszTmpBuf2[0];
             strcpy(lpb2,"Exclude File ");
             sprintf(EndBuf(lpb2),"[%s]",lpb);
             if( AM( IDM_EXCLUDE, lpb2 ) )
                 iCnt++;
         }
#endif   // ADD_EXCLUDE_FILE
      }
      else if( current_view ) // NOT Expanded = Outline file list view
      {
         // *** OUTLINE = List of files is on VIEW ***
         // ******************************************
         //COMPITEM ci = view_getitem( current_view, giSelection );
         //state = compitem_getstate(ci);
         //dwFlag = compitem_getflag(ci);   // and the file compare flags

//         INT itot = view_gettotcount(current_view);
         if(itot == 0)
            goto Add_Sep;  // nothing to do here

         //if( AppendMenu(hMenu, uFlags, IDM_SAVELIST, "Save List..." ) )
         icnt2 = GetListableCount();
         if( g_bListSave == 0 ) {
            lpb = GetStgBuf();
            if( icnt2 )
               sprintf( lpb, "Write List (%d of %d) ...", icnt2, itot );
            else
               sprintf( lpb, "Write List File (%d)...", itot );
            if( AM(IDM_SAVELIST, lpb) )
               iCnt++;
         }

         //        MENUITEM "Exclude &File",               IDM_EXCLUDE
         if( Got_Sel_File( lpb ) ) {
             PTSTR lpb2 = GetNxtBuf();  // &gszTmpBuf2[0];
             strcpy(lpb2,"Exclude File ");
             sprintf(EndBuf(lpb2),"[%s]",lpb);
             if( AM( IDM_EXCLUDE, lpb2 ) )
                 iCnt++;
         }
         // *** TBD *** do NOT add this if all that remains in the compare
         // lists (left and/or right) only left or right
         if( AM( IDM_WRITEDIFF, "Write Difference File..." ) )
            iCnt++;

         if( !bZip ) { // ZIP functionality severely limited at present, so
             // === NOT ZIP ===========================================
            GetOutlineCount();   // get counts for current view - fill in g_sOutCnt
            icnt2 = GetCopiableCount();
//            if( AppendMenu(hMenu, uFlags, IDM_COPYFILES, "Copy Files..." ) )
            lpb = GetStgBuf();
            //sprtf( "Presently %d of %d on view (%d only)."MEOR,
            //   g_sOutCnt.cnt2,
            //   g_sOutCnt.cnt1,
            //   g_sOutCnt.cnt3 );
            icnt2 = g_sOutCnt.cnt2; // get the SET on view = outline_include
            if( itot && icnt2 ) {
               *lpb = 0;
               //sprintf(lpb, "Update L->R (%d of %d)", icnt2, itot );
               if(( g_sOutCnt.cnt2 == g_sOutCnt.cnt3   ) &&  // ONLY left/right only on display
                  ( ( ( g_sOutCnt.cnt2 == g_dwLeft ) ||
                      ( g_sOutCnt.cnt2 == g_dwRite ) ) ) )
               {
                  sprintf(lpb, "Update %d per View!", icnt2 );
                  if( g_sOutCnt.cnt2 == g_dwLeft )
                  {
                     //sprintf(lpb, "Update %d per View!", icnt2 );
                  } //else if( ( gbDelOn ) && (g_sOutCnt.cnt2 == g_dwRite) )
                  else if( g_sOutCnt.cnt2 == g_dwRite )
                  {
                      if( gbDelOn ) {
                        sprintf(lpb, "Delete %d per View!", icnt2 );
                        if( g_sZipArgs.bZipOk ) // TRUE if return is WAIT_OBJECT_0
                           strcat(lpb, " (ZipOk)");

                        if( AM(IDM_DELETEFILE, lpb) )
                           iCnt++;
                     } else {
                        strcpy(lpb, "Enable DELETE function!");
                        if( AM(IDM_ENABLEDELETE, lpb) )
                           iCnt++;
                     }

                     *lpb = 0;   // addition already done, so clear buffer
                  }
               }
               else
               {
#ifdef   ADDVIEWUPDATE2
                  sprintf(lpb, "Update per View! (%d of %d)", icnt2, itot );
#endif   // #ifdef   ADDVIEWUPDATE2
               }

               if( *lpb ) {
                  if( AM(IDM_FILECOPY, lpb ) )
                     iCnt++;
               }
            }

            if( giSelection != pcfds->cf_iSelected )
               chkme( "WARNING: A global pair is out of sync!"MEOR );

#ifdef   DONEUPDCHK2
            // this was supposed to be a FAST one file update (L->R)
            // or perhaps later *** TBD *** a quick DELETE of an orphaned files
            // especially if it belongs to one of the exclude sets.
            // Also *** TBD *** add EXCLUDE directories which does work, I think
            // from the command line but no dialog yet
            if( pcfds->cf_iSelected != -1 )
            {
               pcfds->cf_sCopyTest.ct_szSTitle[0] = 0;
//               pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_SAME;
               pcfds->dwCpyOpts = INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_SAME;
               if( CopyOneIsOK( pcfds ) )
               {
                  SetUpdateLabel(lpb, pcfds);
                  if( AM(IDM_UPDFILE, lpb) )
                     iCnt++;
               }
      
               pcfds->cf_sDeleTest.ct_szSTitle[0] = 0;
               if( DeleteOneIsOK( pcfds ) )
               {
                  SetDeleteLabel(lpb, pcfds);
                  //if( AppendMenu(hMenu, uFlags, IDM_DELETEFILE, lpb) )
                  if( AM(IDM_DELETEONE, lpb) )
                     iCnt++;
               }
               pcfds->dwCpyOpts = dwosv;
            }

#endif   // #ifdef   DONEUPDCHK2

            // if a SELECTION, and is RIGHT ONLY
            // =========================================
            if( ci && ( state == STATE_FILERIGHTONLY ) )
            {
               // candidate for a SINGLE delete
               diritem = file_getdiritem(compitem_getrightfile(ci));
               if(( diritem ) &&
                  ( dir_deletetest( pct, diritem ) ) )
               {
                  // part of test fills in -
                  // SplitFN( NULL, &pct->ct_szSTitle[0], pfn ); // and get JUST the file
                  if( gbDelOn ) {
                     SetDeleteLabel(lpb, pcfds);
                     if( AM( IDM_DELETEONE, lpb ) )
                        iCnt++;
                  } else {
                     strcpy(lpb, "Enable DELETE function!");
                     if( AM( IDM_ENABLEDELETE, lpb) )
                           iCnt++;
                  }
               }
            }     // end if a SELECTION, and is RIGHT ONLY
            // =========================================

            // This is the standard copy files dialog
            // **************************************
            // FIX20100108 - Count be just Copy Files, if there
            // are NO Deletes in the list, else should be
            // Copy/Delete Files (%d of %d)... or even better
            // show copy count, and delete count spearately!
            sprintf(lpb, "Copy Files (%d of %d)...", icnt, itot );
            if( AM(IDM_COPYFILES, lpb ) )
               iCnt++;
            // **************************************
            // if a SELECTION, and is LEFT ONLY
            // =========================================
            if( ci && ( state == STATE_FILELEFTONLY ) )
            {
               diritem = file_getdiritem(compitem_getleftfile(ci));
               if(diritem) {
                  // we have our FILE names
                  char * cp = dir_getnameptr(diritem);
                  sprintf(lpb, "Delete [%s] - Left Only", cp);
                  if( AM( IDM_DELETELEFTFILE, lpb ) ) {
                     iCnt++;
                  }
               }
            }
            // =========================================
            // end if a SELECTION, and is LEFT ONLY

         }  // if NOT bZip

         // this is also 'supposed' to work with ZIP files
         // but failed on last try ...
         if( ( current_view != NULL ) && ( giSelection >= 0 ) )
         {
            sprintf(lpb, "Expand Selection %d", (giSelection + 1) );
            if( AM( IDM_EXPAND, lpb ) )
               iCnt++;
         }
         // **********************************************

#ifdef   ADD_EXCLUDE_FILE
         Add_Exclude_File(hMenu, ci, state, lpb, uFlags, &iCnt);
#endif   // ADD_EXCLUDE_FILE

      }

Add_Sep:

      if(iCnt)
         AppendMenu(hMenu, MF_SEPARATOR, 0, 0 );

//      AM( IDM_DIR,   "Compare Directories ...");
      AM( IDM_DIR,   "Change Directories ...");
      AM( IDM_FILE,  "Compare TWO Files ..."       );

//      AppendMenu(hMenu, MF_SEPARATOR, 0, 0    );
//      AM( IDM_PRINT, "Print..."               );

      AppendMenu(hMenu, MF_SEPARATOR, 0, 0    );
      AM( IDM_EXIT,  "Exit!"                  );

      SBSetTimedTxt( "Context Menu: Choose item from the CONTEXT MENU!", 15, FALSE );

      // TrackPopupMenu()
      TrackPopupMenu( hMenu,         // handle to shortcut menu
         uOpts,   // TPM_CENTERALIGN | TPM_VCENTERALIGN, // options
         pnt.x,   // horizontal position
         pnt.y,   // vertical position
         0,       // reserved, must be zero
         hWnd,    // handle to owner window
         0 );     // ignored

      SBSetTimedTxt( "Destroying the CONTEXT MENU!", 2, FALSE );

      DestroyMenu(hMenu);

      // sprtf( "CONTEXT MENU: Destroyed."MEOR );

   }
   else
   {
      chkme( "WARNING: CreatePopupMenu() FAILED!!!"MEOR );
   }

   return TRUE;
}

// #################################################
// HELPER FUNCTIONS
VOID  SetUpdateLabel(LPTSTR lpb, PCFDLGSTR pcfds)
{
   LPTSTR   lpf = &pcfds->cf_sCopyTest.ct_szSTitle[0];
   if( *lpf )
   {
//            pct->ct_ci      = ci;
//            pct->ct_iState  = compitem_getstate(ci);    // state of the entry
//            pct->ct_dwFlag  = compitem_getflag(ci);
      INT state = pcfds->cf_sCopyTest.ct_iState;
      DWORD dwFlag = pcfds->cf_sCopyTest.ct_dwFlag;   // get the FLAG
      //sprintf(lpb, "Update [%s]", lpf );
      *lpb = 0;
      switch(state)
      {
      case STATE_SAME:
         strcat(lpb, "Change ");
         break;
      case STATE_DIFFER:
         if( dwFlag & TT_OLDER )
            strcat(lpb, "OVERWRITE ");
         else
            strcat(lpb, "Update ");
         break;
      case STATE_FILELEFTONLY:
         strcat(lpb, "Copy ");
         break;
      case STATE_FILERIGHTONLY:
         strcat(lpb, "DELETE ");
         break;
      }

      sprintf(EndBuf(lpb), "[%s]", lpf );

      //AppendSState(lpb, state );
      AppendStateFlag(lpb, state, dwFlag );

   }
   else
   {
      strcpy(lpb, "Update File?");
   }
}

// *** TBD *** These do NOT look correct CHECK
DWORD GetListableCount( VOID )
{
//   CC       scc;
//   PCC      pcc = &scc;
   PCC      pcc = &g_sListCnt;
   ZeroMemory( pcc, sizeof(CC) );
   pcc->bUseOpt = TRUE;
   //pcc->dwopts  = gdwFileOpts;
   //pcc->dwopts = GetFileOpts();
   pcc->dwopts  = outline_include;
   complist_countlistable( pcc );
   return( pcc->cnt2 );
}

DWORD GetOutlineCount( VOID )    // get counts for current view - fill in g_sOutCnt
{                               // per global 'outline_include' options
   PCC      pcc = &g_sOutCnt;
   ZeroMemory( pcc, sizeof(CC) );
   pcc->bUseOpt = TRUE;
   pcc->dwopts  = outline_include;
   complist_countlistable( pcc ); // does complist_setstats... and sets g_sLstStats
   pcc->pliststats = &g_sLstStats;
   return( pcc->cnt2 );
}

DWORD GetCopiableCount( VOID )
{
   CC       scc;
   PCC      pcc = &scc;
   DWORD    dwo = gdwFileOpts;
   ZeroMemory( pcc, sizeof(CC) );
   pcc->bUseOpt = TRUE;
   //pcc->dwopts  = gdwFileOpts;
   //pcc->dwopts = GetFileOpts();
   pcc->dwopts  = outline_include;
   complist_countcopiable( pcc );
   return( pcc->cnt2 );
}

VOID  SetDeleteLabel(LPTSTR lpb, PCFDLGSTR pcfds)
{
   LPTSTR   lpf = &pcfds->cf_sDeleTest.ct_szSTitle[0];
   if( *lpf )
   {
      INT state = pcfds->cf_sDeleTest.ct_iState;
      sprintf(lpb, "Delete [%s]", lpf );
      AppendSState(lpb, state );
   }
   else
   {
      strcpy(lpb, "Delete File?");
   }
}

VOID  AppendStateFlag(LPTSTR lpb, INT iState, DWORD dwFlag )
{
   if( iState == STATE_FILELEFTONLY )
      strcat(lpb, " Left Only");
   else if( iState == STATE_FILERIGHTONLY )
      strcat(lpb, " Right Only");
   else if( iState == STATE_DIFFER )
   {
      //strcat(lpb, " Different");
      if( dwFlag & TT_OLDER )
         strcat(lpb, " Older!");
      else
         strcat(lpb, " Newer.");
   }
   else if( iState == STATE_SAME )
      strcat(lpb, " Same");
   else
      strcat(lpb, " ??");
}

VOID  AppendSState(LPTSTR lpb, INT iState )
{
   if( iState == STATE_FILELEFTONLY )
      strcat(lpb, " Left Only");
   else if( iState == STATE_FILERIGHTONLY )
      strcat(lpb, " Right Only");
   else if( iState == STATE_DIFFER )
      strcat(lpb, " Different");
   else if( iState == STATE_SAME )
      strcat(lpb, " Same");
   else
      strcat(lpb, " ??");
}

// eof - dw4wContext.c

