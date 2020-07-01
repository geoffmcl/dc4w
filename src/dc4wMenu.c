

// dc4wMenu.c
// this is public domain software - praise me, if ok, just don't blame me!
#include "dc4w.h"

#define   ADD_EXCLUDE_FILE // if selected, offer to exclude it from LIST

#undef   ADDVIEWUPDATE2    // this is an UNFINISHED quick update without review

extern   CC    g_sOutCnt;
extern   CC    g_sListCnt;
// if BUSY (on thread working or in dialog asking)
//extern   BOOL     gfBusy;  // only add ABORT to context menu
//verify that the SELECTED item (ie in expanded mode or outline with one selected)
extern   INT      giSelection;    // = -1 if NONE, or is selected row in table
extern   BOOL  CopyOneIsOK( PCFDLGSTR  pcfds );
extern   DWORD       g_dwviewedcnt;
// helper services, like c = GetList( &gsXFileList, cp );
// extern INT GetList( PLE pHead, PTSTR cp );
//        POPUP "&List View Control"
//        BEGIN
//            MENUITEM "&Off",                        IDM_LISTVIEWOFF
//         CMI( IDM_LISTVIEWOFF, g_hListView );
//         if(g_hListView)
//         {
//            MENUITEM "&50%",                        IDM_LISTVIEW50
//            MENUITEM "&100%",                       IDM_LISTVIEW100
BOOL  g_bIn5050 = TRUE;

// forward references


// utility functions
VOID  AppendSStateShort(LPTSTR lpb, INT iState )
{
   if( iState == STATE_FILELEFTONLY )
      strcat(lpb, " LO");
   else if( iState == STATE_FILERIGHTONLY )
      strcat(lpb, " RO");
   else if( iState == STATE_DIFFER )
      strcat(lpb, " DF");
   else if( iState == STATE_SAME )
      strcat(lpb, " SM");
   else
      strcat(lpb, " ??");
}

#define  MENU_FILE      0
#define  MENU_EDIT      1
#define  MENU_VIEW      2
#define  MENU_EXPAND    3
#define  MENU_OPTIONS   4
#define  MENU_ABOUT     5

/* ==================
Copied form RC file 20110201
IDR_MAINMENU MENU 
BEGIN
    POPUP "&File"
    BEGIN
        MENUITEM "Compare &Directories...",     IDM_DIR
        MENUITEM "&Next in List",               IDM_NEXTINLIST
        MENUITEM "Select TWO &Files ...",       IDM_FILE
        MENUITEM SEPARATOR
        MENUITEM "&Abort",                      IDM_ABORT
        MENUITEM SEPARATOR
        MENUITEM "&Save File List...",          IDM_SAVELIST
        MENUITEM "&Write Difference...",        IDM_WRITEDIFF
        MENUITEM "C&opy Files...",              IDM_COPYFILES
        MENUITEM "&Create ZIP File ...",        ID_FILE_CREATEZIPFILE
        MENUITEM "Copy Saved &Zip ...",         IDM_FILE_RENZIP
        MENUITEM "Dele&te File...",             IDM_DELETELEFTFILE
        MENUITEM SEPARATOR
        MENUITEM "&Print",                      IDM_PRINT
        MENUITEM SEPARATOR
        MENUITEM "E&xit",                       IDM_EXIT
    END
    POPUP "&Edit"
    BEGIN
        MENUITEM "&Copy\tCtrl+C",               IDM_EDITCOPY, GRAYED
        MENUITEM SEPARATOR
        MENUITEM "Edit &Left File",             IDM_EDITLEFT
        MENUITEM "Edit &Right File",            IDM_EDITRIGHT
        MENUITEM "Edit &Composite File",        IDM_EDITCOMP
        MENUITEM SEPARATOR
        MENUITEM "&Set Editor...",              IDM_SETEDIT
        MENUITEM "&Edit Exclude List ...",      IDM_EDITEXCLUDE
        MENUITEM "Edit Zip Controls ...",       IDM_EDITZIPUP
    END
    POPUP "&View"
    BEGIN
        MENUITEM "&Outline",                    IDM_OUTLINE
        MENUITEM "&Expand",                     IDM_EXPAND
        MENUITEM SEPARATOR
        MENUITEM "Show &Picture Bar\tAlt+P",    IDM_PICTURE
        MENUITEM "Bottom Status Bar",           IDM_ADDSTATUS
        MENUITEM SEPARATOR
        MENUITEM "&Previous Change\tF3",        IDM_FPCHANGE
        MENUITEM "&Next Change\tF4",            IDM_FCHANGE
        MENUITEM SEPARATOR
        MENUITEM "Expand Next &File\tF5",       IDM_VIEW_NEXT
        MENUITEM "&Reverse L/R",                IDM_REVERSE
        MENUITEM "R&efresh Display",            IDM_REFRESH
        MENUITEM "No Excludes",                 ID_VIEW_NOEXCLUDES
        MENUITEM "&Use Exclude List",           IDM_USEEXCLUDE
        MENUITEM "&Alternate Outline",          IDM_OPTALTDISPLAY
        MENUITEM SEPARATOR
        POPUP "&List View Control"
        BEGIN
            MENUITEM "&Off",                        IDM_LISTVIEWOFF
            MENUITEM "&50%",                        IDM_LISTVIEW50
            MENUITEM "&100%",                       IDM_LISTVIEW100
            MENUITEM SEPARATOR
            MENUITEM "&Hover Select",               IDM_HOVERSEL
            MENUITEM "&Add Grid",                   IDM_LVADDGRID
        END
    END
    POPUP "Ex&panded"
    BEGIN
        MENUITEM "Le&ft File Only\tAlt+L",      IDM_LONLY
        MENUITEM "Rig&ht File Only\tAlt+R",     IDM_RONLY
        MENUITEM "B&oth Files\tAlt+B",          IDM_BOTHFILES
        MENUITEM SEPARATOR
        MENUITEM "&Left Line Numbers",          IDM_LNRS
        MENUITEM "&Right Line Numbers",         IDM_RNRS
        MENUITEM "Add Line &Numbers",           IDM_NONRS
        MENUITEM SEPARATOR
        MENUITEM "Show E&qual Lines",           IDM_SHOWSAME
        MENUITEM "Show &Moved Lines",           IDM_SHOWMOVE
        MENUITEM "Add &Tag",                    IDM_SHOWTAG
        MENUITEM SEPARATOR
        MENUITEM "Ignore &Blanks",              IDM_IGNBLANKS
        MENUITEM "Ignore EOL",                  IDM_IGNOREEOL
        MENUITEM "Ignore &Case",                IDM_IGNCASE
    END
    POPUP "&Outline"
    BEGIN
        POPUP "File List &Options"
        BEGIN
            MENUITEM "Show &Identical\tCtrl+I",     IDM_INCSAME
            MENUITEM "Show &Left-Only Files\tCtrl+L", IDM_INCLEFT
            MENUITEM "Show &Right-Only Files\tCtrl+R", IDM_INCRIGHT
            MENUITEM "Show All &Different",         IDM_DIFFALL
            MENUITEM "Show &Newer Files\tCtrl+N",   IDM_NEWER
            MENUITEM "Show &Older Files\tCtrl+O",   IDM_OLDER
            MENUITEM SEPARATOR
            MENUITEM "Full Path Name",              ID_FILELISTOPTIONS_FULLPATHNAME

            MENUITEM "Relative Name",               ID_FILELISTOPTIONS_RELATIVENAME

            MENUITEM "File Title Only",             ID_FILELISTOPTIONS_FILETITLEONLY

            MENUITEM SEPARATOR
            MENUITEM "Clear Viewed Flag",           IDM_CLEARVIEWED, GRAYED
        END
        MENUITEM SEPARATOR
        MENUITEM "&Full File Compare",          IDM_OPTEXACT
        MENUITEM "Ignore &Blanks",              IDM_IGNBLANKS
        MENUITEM "&Ignore File Times",          IDM_OPTIGNOREDT
        MENUITEM SEPARATOR
        MENUITEM "Re&cursive",                  IDM_RECURSIVE
        MENUITEM "E&xclude per List",           IDM_OPTEXCLUDE
        MENUITEM "Exclude &File",               IDM_EXCLUDE
        MENUITEM SEPARATOR
        MENUITEM "Sh&ow Row Numbers",           IDM_OPTADDROW, CHECKED
        MENUITEM "&Warn on Next Find\tCtrl+W",  IDM_WARNING
        MENUITEM "Show &Tool Tip",              IDM_SHOWTOOLTIP
        MENUITEM "&Show Copy Dialog",           IDM_SHOWCOPY
        MENUITEM "&Preferences...",             IDM_PREFERENCES
        MENUITEM "&Enable *** DELETE ***",      IDM_ENABLEDELETE
    END
    POPUP "&Help"
    BEGIN
        MENUITEM "&About...\tCtrl+H",           IDM_ABOUT
        MENUITEM "Brief Help...\tAlt+?",        ID_HELP_BRIEFHELP
    END
END
  ============================== */

BOOL  IsViewExpBoth( VOID )
{
   BOOL  bRet = FALSE;
   if( ( view_isexpanded( current_view ) ) &&
       ( expand_mode == IDM_BOTHFILES    ) )
   {
      bRet = TRUE;
   }
   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : IsViewExpFile
// Return type: BOOL 
// Argument   : VOID
// Description: 
//              
// NOTE: The function complist_isfiles( cl ) only advise IF the original entry
// were both files, rather than directories!
///////////////////////////////////////////////////////////////////////////////
BOOL  IsViewExpFile( VOID )
{
   BOOL  bRet = FALSE;
   COMPLIST cl = view_getcomplist( current_view );
   if( ( cl              ) &&
       ( IsViewExpBoth() ) &&
       ( view_haschange( current_view, FALSE ) ) )
   {
      bRet = TRUE;
   }
   return bRet;
}

typedef struct tagCMD2STG {
   LPTSTR   pstg;
   UINT     uid;
}CMD2STG, * PCMD2STG;

CMD2STG  sCmd2Stg[] = {
//IDR_MENU1 MENU DISCARDABLE 
//BEGIN
//    POPUP "&File"
//    BEGIN
   { "Compare &Directories...",     IDM_DIR },
   { "&Next in List",               IDM_NEXTINLIST },
   { "Select TWO &Files ...",       IDM_FILE },
//   { SEPARATOR
   { "&Abort",                      IDM_ABORT },
//   { SEPARATOR
   { "&Save File List...",          IDM_SAVELIST },
   { "&Write Difference...",        IDM_WRITEDIFF },
   { "C&opy Files...",              IDM_COPYFILES },
   { "&Update Now\tAlt+U",          IDM_FILECOPY },
//   { SEPARATOR
   { "&Print",                      IDM_PRINT },
//   { SEPARATOR
   { "E&xit",                       IDM_EXIT },
//    END
//    POPUP "&Edit"
//    BEGIN
   { "Edit &Left File",             IDM_EDITLEFT },
   { "Edit &Right File",            IDM_EDITRIGHT },
   { "Edit &Composite File",        IDM_EDITCOMP },
//   { SEPARATOR
   { "&Set Editor...",              IDM_SETEDIT },
   { "&Edit Exclude List ...",      IDM_EDITEXCLUDE },
//    END
//    POPUP "&View"
//    BEGIN
   { "&Outline",                    IDM_OUTLINE },
   { "&Expand",                     IDM_EXPAND },
//   { SEPARATOR
   { "Show &Picture Bar\tAlt+P",    IDM_PICTURE },
   { "Bottom Status Bar",           IDM_ADDSTATUS },
//   { SEPARATOR
   { "&Previous Change\tF3",        IDM_FPCHANGE },
   { "&Next Change\tF4",            IDM_FCHANGE },
//   { SEPARATOR
   { "Expand Next &File\tF5",       IDM_VIEW_NEXT },
   { "&Reverse L/R",                IDM_REVERSE },
   { "R&efresh Display",            IDM_REFRESH },
//        POPUP "&List View Control"
//        BEGIN
   { "&Off",                        IDM_LISTVIEWOFF },
   { "&50%",                        IDM_LISTVIEW50  },
   { "&100%",                       IDM_LISTVIEW100 },
//        END
//    END
//    POPUP "Ex&pand"
//    BEGIN
   { "Le&ft File Only\tAlt+L",      IDM_LONLY },
   { "Rig&ht File Only\tAlt+R",     IDM_RONLY },
   { "B&oth Files\tAlt+B",          IDM_BOTHFILES },
//   { SEPARATOR
   { "&Left Line Numbers",          IDM_LNRS },
   { "&Right Line Numbers",         IDM_RNRS },
   { "&No Line Numbers",            IDM_NONRS },

   { "Show E&qal Line",             IDM_SHOWSAME },
   { "Show &Moved Lines",           IDM_SHOWMOVE },
   { "Add &Tags",                   IDM_SHOWTAG  },

   { "Ignore &Blanks",              IDM_IGNBLANKS },
   { "Ignore EOL",                  IDM_IGNOREEOL },
   { "Ignore &Case",                IDM_IGNCASE   },
//    END
//    POPUP "&Options"
//    BEGIN
   { "Show &Identical Files\tCtrl+I", IDM_INCSAME },
   { "Show &Left-Only Files\tCtrl+L", IDM_INCLEFT },
   { "Show &Right-Only Files\tCtrl+R", IDM_INCRIGHT },
   { "Show All &Different",         IDM_DIFFALL },
   { "Show &Newer Files\tCtrl+N",   IDM_NEWER },
   { "Show &Older Files\tCtrl+O",   IDM_OLDER },
//   { SEPARATOR
   { "&Full File Compare",          IDM_OPTEXACT },
   { "Ignore &Blanks",              IDM_IGNBLANKS },
   { "&Ignore File Time",           IDM_OPTIGNOREDT },
//   { SEPARATOR
   { "Re&cursive",                  IDM_RECURSIVE },
   { "E&xclude per List",           IDM_OPTEXCLUDE },
//   { SEPARATOR
   { "Sh&ow Row Numbers",           IDM_OPTADDROW  },
   { "&Warn on Next Find\tCtrl+W",  IDM_WARNING  },
   { "Show &Tool Tip",              IDM_SHOWTOOLTIP  },
   { "&Show Copy Dialog",           IDM_SHOWCOPY  },
   { "&Preferences...",             IDM_PREFERENCES  },
   { "&Enable *** DELETE ***",      IDM_ENABLEDELETE },
//    END
//    POPUP "&Help"
//    BEGIN
   { "&About...Ctrl+H",             IDM_ABOUT },
//    END
//END
   // *** last record ***
   { 0, 0 }
};

LPTSTR   Cmd2Stg( UINT uid )
{
   PCMD2STG pc = &sCmd2Stg[0];
   while( pc->pstg )
   {
      if( pc->uid == uid )
         break;
      pc++;
   }
   return pc->pstg;
}

#undef  ADDMENUMOD

BOOL  g_bDbgPOP = FALSE;

#define  mf_Enab     (MF_BYCOMMAND | MF_ENABLED)
#define  mf_Disab    (MF_BYCOMMAND | MF_DISABLED | MF_GRAYED)
#define  EMI(a,b) EnableMenuItem(hMenu, a, ((b) ? mf_Enab : mf_Disab))
#define  CMI(a,b) CheckMenuItem(hMenu,  a, ((b) ? MF_CHECKED : MF_UNCHECKED) )
#define  IsExpanded     view_isexpanded( current_view )

BOOL  HaveGotUpdate( VOID )
{
   INT      icnt = view_getrowcount(current_view);
   INT      itot = view_gettotcount(current_view);
   if( icnt && itot )
      return TRUE;

   return FALSE;

}

BOOL  Got_Sel_File( LPTSTR lpb )
{
   extern BOOL  Get_Sel_File_Stg( LPTSTR lpb, COMPITEM ci );
   COMPITEM    ci = view_getitem( current_view, giSelection );
   if(ci) {
      // get CURRENT SELECTION
      if( Get_Sel_File_Stg( lpb, ci ) ) {
         return TRUE;
      }
   }
   return FALSE;
}


long  Do_WM_INITMENUPOPUP( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   long     lRet = 0;
   HMENU    hMenu = (HMENU)wParam;
   DWORD    dwPos = LOWORD(lParam);
   BOOL     bSys  = HIWORD(lParam);
   COMPLIST cl = 0;
   DWORD    dwo = outline_include;
   //UINT  ui1, ui2, ui3;
   LPTSTR   lps;
   LPTSTR   lpb  = &gszTmpBuf[0];
   BOOL     bEnab;
   LPTSTR   lpb2 = &gszTmpBuf2[0];

   if( bSys )
   {
      // is the windows (system) menu
      lRet = 1;   // use default processing
   }
   else
   {
      // it is MY menu
      if( g_bDbgPOP )
      {
         sprtf( "Do_WM_INITMENUPOPUP: For Pos=%d (%s)."MEOR,
            dwPos,
            ( (dwPos == MENU_FILE) ? "MENU_FILE" :
            (dwPos == MENU_EDIT) ? "MENU_EDIT" :
            (dwPos == MENU_VIEW) ? "MENU_VIEW" :
            (dwPos == MENU_EXPAND) ? "MENU_EXPAND" :
            (dwPos == MENU_OPTIONS) ? "MENU_OPTIONS" :
            (dwPos == MENU_ABOUT) ? "MENU_ABOUT" :
            "MENU_UNKNOWN" ) );
      }
      switch( dwPos )
      {
      case MENU_FILE:
         {
            BOOL  bZip = view_haszip(current_view);
            cl = view_getcomplist(current_view);
            if( current_view && cl )
               bSys = TRUE;
            else
               bSys = FALSE;
            EMI( IDM_SAVELIST, bSys );
            if( bZip )
            {
               // *** TBD *** Expand zip data to a file in an indicated directory
               // Compress to zip, and ADD update into zip *** TBD ***
               EMI( IDM_COPYFILES, FALSE );
               EMI( IDM_FILECOPY,  FALSE );  // immediate no dialog update
            }
            else  // NOT ZIP - we can do a LOTS
            {
               EMI( IDM_COPYFILES, bSys );
#ifdef   ADDMENUMOD
               if(bSys)
               {
                  lps = "C&opy Files...";
                  if( ( IsExpanded ) && ( gszLeftName[0] ) )
                  {
                     // LPTSTR   lpb  = &gszTmpBuf[0];
                     // LPTSTR   lpb2 = &gszTmpBuf2[0];
                     *lpb2 = 0;
                     compitem_retrelname( lpb2, ci );
                     if( ( lpb2[0] == '.' ) && ( lpb2[1] == '\\' ) )
                     {
                        strcpy( lpb, &gszLeftName[0] );
                        strcat( lpb, &lpb2[1]        );
                        if( dir_isvalidfile(lpb) )
                        {
                           sprintf(lpb, "C&opy %s", &lpb2[2]);
                           lps = lpb;
                        }
                     }
                  }
                  ModifyMenu( hMenu,           // handle to menu
                     IDM_COPYFILES,    // menu item to modify
                     (MF_BYCOMMAND | MF_STRING),     // options
                     IDM_COPYFILES,    // identifier, menu, or submenu
                     lps );   // menu item content
               }
#endif   //#ifdef   ADDMENUMOD
               EMI( IDM_FILECOPY, HaveGotUpdate() );  // if we have file count(s)
               // this depends on the left and right list of files,
               // but also on which 'display/view' options are on
            }
            EMI( IDM_WRITEDIFF, (bSys && view_haschange( current_view, FALSE )) );
            EMI( IDM_DELETELEFTFILE, Can_Delete_Left_File() );
         }
         break;

      case MENU_EDIT:
         break;

      case MENU_VIEW:
         if( !current_view )
         {
            // both are invalid
            EMI( IDM_OUTLINE, FALSE);
            EMI( IDM_EXPAND,  FALSE);
            EMI( IDM_FPCHANGE, FALSE);
            EMI( IDM_FCHANGE, FALSE);
            EMI( IDM_VIEW_NEXT, FALSE);
         }
         else if( view_isexpanded(current_view) )
         {
            // back to outline is ok
            EMI( IDM_OUTLINE, TRUE );
            EMI( IDM_EXPAND,  FALSE);
            EMI( IDM_FPCHANGE, TRUE);
            EMI( IDM_FCHANGE,  TRUE);
            EMI( IDM_VIEW_NEXT, TRUE);
         }
         else  // it is NOT expanded
         {
            EMI( IDM_OUTLINE, FALSE);  // so no to outline
            if( giSelection < 0 )        // and can only expand if a selection
               EMI( IDM_EXPAND,  FALSE );
            else
               EMI( IDM_EXPAND,  TRUE );
            EMI( IDM_FPCHANGE, FALSE);
            EMI( IDM_FCHANGE, FALSE);
            EMI( IDM_VIEW_NEXT, FALSE);
         }

         //CMI( IDM_ADDSTATUS, gbAddS );
         CMI( IDM_ADDSTATUS, ( !g_sSB.sb_bHidden ) );
//        MENUITEM "&Use Exclude List",
         CMI( IDM_USEEXCLUDE, gbExclude );   // Exclude per one of the lists

         CMI( IDM_OPTALTDISPLAY, gbSimple ); // check, if using alternate display info

#ifdef ADD_LIST_VIEW
// =======================================================
//        POPUP "&List View Control"
//        BEGIN
//            MENUITEM "&Off",                        IDM_LISTVIEWOFF
         if(g_hListView)
            lps = "&On. To Hide";   // menu item content
         else
            lps = "&Off. To Show";

         ModifyMenu( hMenu,           // handle to menu
                     IDM_LISTVIEWOFF,    // menu item to modify
                     (MF_BYCOMMAND | MF_STRING),     // options
                     IDM_LISTVIEWOFF,    // identifier, menu, or submenu
                     lps );   // menu item content
         CMI( IDM_LISTVIEWOFF, g_hListView );
         if(g_hListView)
         {
//            MENUITEM "&50%",                        IDM_LISTVIEW50
//            MENUITEM "&100%",                       IDM_LISTVIEW100
            if( g_bIn5050 )
            {
               EMI(IDM_LISTVIEW50, FALSE );
               EMI(IDM_LISTVIEW100, TRUE );
            }
            else
            {
               EMI(IDM_LISTVIEW50, TRUE  );
               EMI(IDM_LISTVIEW100, FALSE);
            }
//        END
         }
         else
         {
            EMI(IDM_LISTVIEW50, FALSE );
            EMI(IDM_LISTVIEW100, FALSE);
         }
#endif // #ifdef ADD_LIST_VIEW

         // ID_VIEW_NOEXCLUDES - section [Exclude]
         // { szXcl, szNoEx, it_Bool, &bNoExcludes, &bChgNoExcl,
         CMI( ID_VIEW_NOEXCLUDES, bNoExcludes );
         // CMI( IDM_EXCLUDEREPOS, gbXAllRepos ); // FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS
         break;

      case MENU_EXPAND:

         // could switch(expand_mode) { case IDM_LONLY:
         // but maybe -
         CMI( IDM_LONLY,     FALSE );
         CMI( IDM_RONLY,     FALSE );
         CMI( IDM_BOTHFILES, FALSE );
         CMI( expand_mode,   TRUE  );

//   { "&Left Line Numbers",          IDM_LNRS },
//   { "&Right Line Numbers",         IDM_RNRS },
//   { "&No Line Numbers",            IDM_NONRS },
//   case IDM_NONRS: // this is a display toggle
//TCHAR szShwNums[] = "ShowLineNumbers";
//         CMI( IDM_NONRS, gbShowNums );
         CMI( IDM_NONRS, ( gdwDiffOpts & INCLUDE_LINENUMS ) );

         CMI( IDM_LNRS,  ( !gbUseRight ) );
         CMI( IDM_RNRS,     gbUseRight   );
//TCHAR szUseRt[] = "UseRightNumbers"; // just the source of the number - if abv ON
// #define  gbUseRight     sFW.fw_bUseRight

         CMI( IDM_SHOWSAME, ( gdwDiffOpts & INCLUDE_SAME ) );  // include / exclude SAME
         CMI( IDM_SHOWMOVE, ( gdwDiffOpts & INC_ALLMOVE  ) );  // include / exclude MOVED
         CMI( IDM_SHOWTAG,  ( gdwDiffOpts & INCLUDE_TAGS ) );  // include 'tag' text
         // so can not have  if( !( gdwDiffOpts & INCLUDE_TAGS ) ) cell++; // no tags
         if( gdwDiffOpts & INCLUDE_LINENUMS )   // we have LINE NUMBERS
            EMI( IDM_SHOWTAG, FALSE);
         else
            EMI( IDM_SHOWTAG, TRUE);

         // NOTE: both these effect the hash code generated for a string
         // which is the fast primary compare of two lines from two folders
         // If these match, then the program STILL does a TCHAR by TCHAR
         // compare of the left file line, and right file line ...
         // *** TBD *** not here, but must gather line comare stats, and
         // present information to viewer of the compare. green on bits that match
         // =============== ******** ===========
         CMI( IDM_IGNBLANKS, ignore_blanks );
         CMI( IDM_IGNOREEOL, gbIgnEOL      );
         CMI( IDM_IGNCASE,   gbIgnCase     );

         break;

      case MENU_OPTIONS:
         // *** TBD ** This should depend on whether the display is OUTLINE
         // then these are correct ... but should be show line options
         // when the view is expanded. The missing stuff is
         // implemented very by Beyond Compare well in their 'expanded' mode.
         // *** TBD *** to be able to hide/display line(s) in expanded mode
//    POPUP "&Options"
//    BEGIN
//        MENUITEM "Show &Identical Files\tCtrl+I", IDM_INCSAME
         CMI( IDM_INCSAME,   (dwo & INCLUDE_SAME)      );
//        MENUITEM "Show &Left-Only Files\tCtrl+L", IDM_INCLEFT
         CMI( IDM_INCLEFT,   (dwo & INCLUDE_LEFTONLY)  );
//        MENUITEM "Show &Right-Only Files\tCtrl+R", IDM_INCRIGHT
         CMI( IDM_INCRIGHT,  (dwo & INCLUDE_RIGHTONLY) );
//        MENUITEM "Show All &Different",         IDM_DIFFALL
         // CMI( IDM_INCDIFFER, (dwo & INCLUDE_DIFFER)    );
//         CMI( IDM_DIFFALL,   ((dwo & (INCLUDE_NEWER|INCLUDE_OLDER)) == (INCLUDE_NEWER|INCLUDE_OLDER)) );
         CMI( IDM_DIFFALL,   ShowingAll(dwo) );
//             MENUITEM "Full Path Name",              ID_FILELISTOPTIONS_FULLPATHNAME
//             MENUITEM "Relative Name",               ID_FILELISTOPTIONS_RELATIVENAME
//             MENUITEM "File Title Only",             ID_FILELISTOPTIONS_FILETITLEONLY
         if( gdwFileOpts & FULL_NAMES ) {
            CMI(ID_FILELISTOPTIONS_FULLPATHNAME, TRUE);
            CMI(ID_FILELISTOPTIONS_FILETITLEONLY, FALSE);
            CMI(ID_FILELISTOPTIONS_RELATIVENAME, FALSE);
         } else if( gdwFileOpts & ADD_REL_PATH ) {
            CMI(ID_FILELISTOPTIONS_FULLPATHNAME, FALSE);
            CMI(ID_FILELISTOPTIONS_FILETITLEONLY, TRUE);
            CMI(ID_FILELISTOPTIONS_RELATIVENAME, FALSE);
         } else {
            CMI(ID_FILELISTOPTIONS_FULLPATHNAME, FALSE);
            CMI(ID_FILELISTOPTIONS_FILETITLEONLY, FALSE);
            CMI(ID_FILELISTOPTIONS_RELATIVENAME, TRUE);
         }

//        MENUITEM "Show &Newer Files\tCtrl+N",   IDM_NEWER
//        MENUITEM "Show &Older Files\tCtrl+O",   IDM_OLDER
         CMI( IDM_NEWER,     (dwo & INCLUDE_NEWER)     );
         CMI( IDM_OLDER,     (dwo & INCLUDE_OLDER)     );
//        MENUITEM SEPARATOR
//        MENUITEM "&Full File Compare",          IDM_OPTEXACT
         CMI( IDM_OPTEXACT,   gbExact  );

         //CMI( IDM_OPTADDROW,  gbAddRow );
         CMI( IDM_OPTADDROW,
            ((gdwFileOpts & INCLUDE_LINENUMS) ? TRUE : FALSE) );

//        MENUITEM "Ignore &Blanks",              IDM_IGNBLANKS
         CMI( IDM_IGNBLANKS,  ignore_blanks );
         CMI( IDM_OPTIGNOREDT,   gbIgnDT );
//        MENUITEM SEPARATOR
//        MENUITEM "Re&cursive",                  IDM_RECURSIVE
#ifndef USE_GLOBAL_RECURSIVE    // FIX20091125
         CMI( IDM_RECURSIVE,  gbRecur );
#else   // #ifndef USE_GLOBAL_RECURSIVE
         CMI( IDM_RECURSIVE,  (g_bNOTRecursive ? FALSE : TRUE) );   // FIX20091125
#endif // #ifndef USE_GLOBAL_RECURSIVE

//        MENUITEM "E&xclude per List",           IDM_OPTEXCLUDE
         CMI( IDM_OPTEXCLUDE, gbExclude );

//        MENUITEM "Exclude &File",               IDM_EXCLUDE
         *lpb = 0;
         bEnab = Got_Sel_File( lpb );
         if(bEnab) {
            //lps = "Exclude &File [%s]";   // menu item content
            strcpy(lpb2,"Exclude &File ");
            sprintf(EndBuf(lpb2),"[%s]",lpb);
            lps = lpb2;
         } else {
            lps = "Exclude &File Off";
         }

         ModifyMenu( hMenu,           // handle to menu
                     IDM_EXCLUDE,    // menu item to modify
                     (MF_BYCOMMAND | MF_STRING),     // options
                     IDM_EXCLUDE,    // identifier, menu, or submenu
                     lps );   // menu item content
         EMI( IDM_EXCLUDE, bEnab );  

//        MENUITEM SEPARATOR
//        MENUITEM "&Warn on Next Find\tCtrl+W",  IDM_WARNING
         CMI( IDM_WARNING,    gbWarn );
//        MENUITEM "Show &Tool Tip",              IDM_SHOWTOOLTIP
         CMI( IDM_EXCLUDEREPOS, gbXAllRepos ); // FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS

//    END
         if( g_hwndTT )
         {
            CMI( IDM_SHOWTOOLTIP, !gbNoTT );
         }
         else
         {
            EMI( IDM_SHOWTOOLTIP, FALSE );
         }

         if( g_dwviewedcnt )
            EMI( IDM_CLEARVIEWED, TRUE );
         else
            EMI( IDM_CLEARVIEWED, FALSE);


//        MENUITEM "&Show Copy Dialog",           IDM_SHOWCOPY
//        MENUITEM "&Preferences...",             IDM_PREFERENCES

//        MENUITEM "&Enable *** DELETE ***",      IDM_ENABLEDELETE
         if( gbDelOn )
            lps = "On. &Disable Delete";   // menu item content
         else
            lps = "&Enable *** DELETE ***";
         ModifyMenu( hMenu,           // handle to menu
                     IDM_ENABLEDELETE,    // menu item to modify
                     (MF_BYCOMMAND | MF_STRING),     // options
                     IDM_ENABLEDELETE,    // identifier, menu, or submenu
                     lps );   // menu item content
         CMI( IDM_ENABLEDELETE, gbDelOn );   // check

         break;

      case MENU_ABOUT:
         break;

      }
   }

   return lRet;
}


// eof - dc4wMenu.c

