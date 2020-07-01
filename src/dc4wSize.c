

// dc4wSize.c
// this is public domain software - praise me, if ok, just don't blame me!
#include "dc4w.h"

extern   HWND  hwndStatus;    /* status bar across top */
extern   int   status_height; // and its HEIGHT
extern   HWND  hwndBar;       /* graphic of sections as vertical bars */
extern   BOOL  gbDnInit; // into message loop
extern   BOOL  g_bIn5050;
extern   TCHAR g_szHotSel[];  // = "Hot Select";  // sprintf(pbuf, "%s = [%s]", g_szHotSel, fname );
extern   BOOL  g_bInExit;
extern   TCHAR sszDef[];   // = "Ready";
extern   DWORD g_dwActSecs;

#ifdef ADD_LIST_VIEW
extern   VOID  Do_IDM_LISTVIEWOFF(VOID);
extern   BOOL  g_bLVHasFocus;   // show (globally) that the keyboard is here/not here!
#endif // #ifdef ADD_LIST_VIEW

extern   VOID  Do_WM_SETFOCUS( HWND hWnd );

#define  SB_DEF_HEIGHT     20    // this seems 'fixed'
#define  MY_MIN_ROWS       5     // need at least 5 visible lines, before enabling LISTVIEW

#define  RECTWIDTH(a)   ( a->right - a->left )
#define  RECTHEIGHT(a)  ( a->bottom - a->top )

/***************************************************************************
 * Function: DoResize - WM_SIZE to Frame window
 *
 * Purpose:
 *
 * Position child windows on a resize of the main window
 */
RECT  g_rcClient; // Client rectangle
RECT  g_rcStatus; // TOP STATUS LINE
// display available to TABLE/BAR and LISTVIEW
RECT  g_rcDisplay;   // max. display available
RECT  g_rcTablesz;  // TABLE size - NOTE: This is in x,y - cx,cy form - not true rc!
RECT  g_Tablerc;  // logical RECTABLE of TABLE
RECT  g_rcVBar;   // vertical BAR
RECT  g_rcListView;  // and the LISTVIEW
RECT  g_rcSizeBar;   // size bar between TABLE and LISTVIEW - parents area

#define  IN_NOTHING  0
#define  IN_TOP_STAT 1
#define  IN_TABLE    2
#define  IN_SIZEBAR  3
#define  IN_LISTVIEW 4
#define  IN_BAR      5

BOOL  g_bInSizing = FALSE; // in the SIZING - splitter bar - between Table and LISTVIEW
DWORD dwLastPos = -1;
POINT g_ptLast;
TCHAR g_szMsAt[] = "Mouse at ";

DWORD g_dwPercent = 50;
BOOL  g_bChgPCnt  = FALSE;
static TCHAR _s_szmousepos[264];
VOID  SetMousePos( DWORD pos, DWORD xPos, DWORD yPos, LPTSTR ppos )
{
   LPTSTR   lps = _s_szmousepos;
//TCHAR g_szHotSel[] = "Hot Select";  // sprintf(pbuf, "%s = [%s]", g_szHotSel, fname );
//      if( InStr( gszM1, g_szHotSel ) != 1 )
   if(( strcmp(gszM1, sszDef) == 0 ) ||
      ( InStr(gszM1, g_szMsAt) == 1 ) )
   {
      if(ppos)
      {
         sprintf(lps, "%s %d,%d - in %s",
            g_szMsAt,
            xPos, yPos,
            ppos );
      }
      else
      {
         sprintf(lps, "%s %d,%d - in %s",
            g_szMsAt,
            xPos, yPos,
            ((pos == IN_TOP_STAT) ? "Top Status" :
            (pos == IN_TABLE)    ? "Table" :
            (pos == IN_SIZEBAR)  ? "Size Bar" :
            (pos == IN_LISTVIEW) ? "ListView" :
            (pos == IN_BAR)      ? "V.Bar" : "unlisted!") );
      }

      SBSetTimedTxt( lps, g_dwActSecs, FALSE );
      
   }
}

#define  TRACK_ROW   6
INT   trackmode = 0;
INT   trackline1;
extern   void  gtab_setcursor( HCURSOR hCur );
extern   HANDLE hNormCurs, hVertCurs; // = IDC_CURSOR1
extern   HANDLE hHorzCurs;  // = (IDC_CURSOR2));
extern   HANDLE hWorkCurs;  // = (IDC_CURSOR3));
extern   void gtab_drawhorzline(HWND hwnd, INT y);

//   case WM_LBUTTONDOWN,    case WM_LBUTTONUP:

VOID  Do_WM_LBUTTONDOWN( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   INT   x = LOWORD(lParam);
   INT   y = HIWORD(lParam);

   /* ensure we see the mouse-up */
   SetCapture(hWnd);

   trackmode = TRACK_ROW;
   trackline1 = y;

   gtab_drawhorzline(hWnd, y);

}
extern   BOOL  Set_LV_ON(VOID);
extern   BOOL  g_bLVOff;
extern   BOOL  Set_LV_OFF(VOID);
extern   VOID  Toggle5050(VOID);
extern   VOID  DoResize(HWND);

VOID  Do_WM_LBUTTONUP( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   INT   x = LOWORD(lParam);
   INT   y = HIWORD(lParam);
   BOOL  inrng = FALSE;

   ReleaseCapture();

   trackmode = 0;
   gtab_drawhorzline(hWnd, trackline1);
   // position TOP STATUS window - self handled status bar
   //   g_rcStatus.bottom = g_rcStatus.top + status_height;
   if( trackline1 > g_rcStatus.bottom )
      inrng = TRUE;
   if( inrng && g_sSB.sb_hStatus && !g_sSB.sb_bHidden )
   {
      if( trackline1 > g_rcListView.bottom )
         inrng = FALSE;
   }
   if(inrng)
   {
      if(( g_rcListView.bottom - g_rcStatus.bottom ) > 4 )
      {
         INT   relline = trackline1 - g_rcStatus.bottom;
         INT   dispht  = (g_rcDisplay.bottom - g_rcDisplay.top);
#ifdef ADD_LIST_VIEW
         if(( trackline1 - 1 ) == g_rcStatus.bottom )
         {
             // zero the TABLE completely
            if( !g_hListView ) // could be just HIDDEN
               Set_LV_ON();   // so set to ON

            if(g_hListView)
            {
               if( g_bIn5050 )
                  Toggle5050();

               DoResize(hwndClient);
            }

            return;  // DONE closing up TABLE window, and expanded LISTVIEW to 100%
            // ***TBD*** Now must perhaps use the LAST LINE of the top status
            // as the SIZING bar ???
         }

         //if( g_sSB.sb_hStatus && !g_sSB.sb_bHidden ) {
         if( trackline1 == g_rcListView.bottom )
         {
            // ok, HIDE the LISTVIEW, and let the TABLE have it ALL
            if(g_hListView)
               Set_LV_OFF();

            DoResize(hwndClient);

            return;  // DONE hiding the LISTVIEW, and expanded TABLE to 100%
            // ***TBD*** Now must perhaps use the LAST pixel row of the TABLE
            // as the SIZING bar;

         }
#endif // #ifdef ADD_LIST_VIEW

         // partial change in Table/ListView relative SIZE
         if(( relline > 0 ) &&
            ( dispht  > 0 ) )
         {
            DWORD pct = (relline * 100) / dispht;
            if( pct != g_dwPercent )
            {
               LONG div = 2; // normal 'divisor'
               LONG  tbl;
               PRECT prt  = &g_Tablerc;
               PRECT prlv = &g_rcListView;

               // if give it SOME SPACE
               //if( g_dwPercent && (g_dwPercent != 50) ) {
               // a potential new DIVISOR, to divide up the visible CLIENT screen
               if(pct == 0)
                  div = 1;
               else if(pct < 100)
                  div = 100 / pct;
               else
                  div = 1;
               tbl = dispht / div;
#ifdef   ADDDIAGRECT2
               sprtf("Percent = %d. tab=%s lv=%s on y=%d.(%d:%d)"MEOR,
                  g_dwPercent,
                  Rect2Stg(prt),
                  Rect2Stg(prlv),
                  dispht,
                  RECTHEIGHT(prt),
                  RECTHEIGHT(prlv) );
#endif   // #ifdef   ADDDIAGRECT2

               g_dwPercent = pct;
               g_bChgPCnt  = TRUE;

#ifdef ADD_LIST_VIEW

               if( !g_hListView ) // could be just HIDDEN
                  Set_LV_ON();   // so set to ON

               if(g_hListView)
               {
                  if( !g_bIn5050 )
                     Toggle5050();

                  DoResize(hwndClient);
               }
#endif // #ifdef ADD_LIST_VIEW

#ifdef   ADDDIAGRECT2
               sprtf("Percent = %d. tab=%s lv=%s on y=%d.(%d:%d)"MEOR,
                  pct,
                  Rect2Stg(prt),
                  Rect2Stg(prlv),
                  dispht,
                  RECTHEIGHT(prt),
                  RECTHEIGHT(prlv) );
#endif   // #ifdef   ADDDIAGRECT2
            }
         }
      }
   }
}

VOID  Do_WM_MOUSEMOVE( HWND hWnd, WPARAM wParam, DWORD dx, DWORD dy, DWORD src )
{
   RECT  rc, dest;
   DWORD pos = IN_NOTHING;
   LONG  xPos = dx;
   LONG  yPos = dy;
   rc.left   = xPos;
   rc.top    = yPos;
   rc.right  = rc.left + 1;
   rc.bottom = rc.top  + 1;

   if( trackmode == TRACK_ROW )
   {
      BOOL  rpnt = FALSE;
      pos = IN_SIZEBAR;
      if( trackline1 != yPos )
      {
         if( yPos > g_rcStatus.bottom )   // stop at the TOP STATUS BAR
            rpnt = TRUE;

         if( rpnt && g_sSB.sb_hStatus && !g_sSB.sb_bHidden )
         {
            if( yPos > g_rcListView.bottom )
               rpnt = FALSE;
         }
         if(rpnt)
         {
            gtab_drawhorzline(hWnd, trackline1);
            trackline1 = yPos;
            gtab_drawhorzline(hWnd, trackline1);
         }
      }
      SetMousePos( pos, xPos, yPos, NULL );
      return;
   }

#ifdef ADD_LIST_VIEW
   if( src != SRC_HOTTRACK )
   {
      if( g_bLVHasFocus )
         Do_WM_SETFOCUS( hWnd );
   }
#endif // #ifdef ADD_LIST_VIEW

   if( IntersectRect( &dest, &rc, &g_rcStatus ) )
      pos = IN_TOP_STAT;
   else if( IntersectRect( &dest, &rc, &g_Tablerc ) )
      pos = IN_TABLE;
#ifdef ADD_LIST_VIEW
   else if( g_hListView && IntersectRect( &dest, &rc, &g_rcSizeBar ) )
      pos = IN_SIZEBAR;
   else if( g_hListView && IntersectRect( &dest, &rc, &g_rcListView ) )
      pos = IN_LISTVIEW;
#endif // #ifdef ADD_LIST_VIEW
   else if(( DisplayMode == MODE_EXPAND ) && ( picture_mode ) && IntersectRect( &dest, &rc, &g_rcVBar ) )
      pos = IN_BAR;
   else
      pos = IN_NOTHING;

   if(( dwLastPos != pos   ) ||
      ( g_ptLast.x != xPos ) ||
      ( g_ptLast.y != yPos ) )
   {
      dwLastPos  = pos;
      g_ptLast.x = xPos;
      g_ptLast.y = yPos;
      if( g_bInSizing )
      {
         if( pos != IN_SIZEBAR )
         {
            gtab_setcursor(hNormCurs);
            g_bInSizing = FALSE;
         }
      
      }
      else if( pos == IN_SIZEBAR )
      {
         if(!g_bInSizing)
         {
            gtab_setcursor(hHorzCurs);
         }
         g_bInSizing = TRUE;
      }

      SetMousePos( pos, xPos, yPos, NULL );
   }
}

VOID  Do_WM_MOUSEMOVE2( HWND hWnd, WPARAM wParam, DWORD dx, DWORD dy,
                       LPTSTR ppos )
{
   RECT  rc, dest;
   DWORD pos = IN_NOTHING;
   LONG  xPos = dx;
   LONG  yPos = dy;
   rc.left   = xPos;
   rc.top    = yPos;
   rc.right  = rc.left + 1;
   rc.bottom = rc.top  + 1;

   if( IntersectRect( &dest, &rc, &g_rcStatus ) )
      pos = IN_TOP_STAT;
   else if( IntersectRect( &dest, &rc, &g_Tablerc ) )
      pos = IN_TABLE;
#ifdef ADD_LIST_VIEW
   else if( g_hListView && IntersectRect( &dest, &rc, &g_rcSizeBar ) )
      pos = IN_SIZEBAR;
   else if( g_hListView && IntersectRect( &dest, &rc, &g_rcListView ) )
      pos = IN_LISTVIEW;
#endif // #ifdef ADD_LIST_VIEW
   else if(( DisplayMode == MODE_EXPAND ) && ( picture_mode ) && IntersectRect( &dest, &rc, &g_rcVBar ) )
      pos = IN_BAR;
   else
      pos = IN_NOTHING;

   //if(( dwLastPos != pos   ) ||
   if(( g_ptLast.x != xPos ) ||
      ( g_ptLast.y != yPos ) )
   {
      dwLastPos = pos;
      g_ptLast.x = xPos;
      g_ptLast.y = yPos;
      SetMousePos( pos, xPos, yPos, ppos );
   }
}

// *** THIS IS GETTING OVERLY COMPLEX ***
// General Window Layout
// 0,0                                                  cxClient
// [ Status Window - with messages and button      [ Expand ]  ]
//
// Reduced Client Space is SHARED between
// The TABLE Window - hwndRCD, with its graphic BAR window, if
// in expanded mode.
// * AND/OR * my newly developing LISTVIEW Control, with lots
// of built in features.
//
// BUT, the LISTVIEW does NOT presently 'assist' in showing the
// so called [ Expanded ] view of the file 'sections' - read lines,
// and thus if ON, must be forced to occupy only say 50% or less,
// of the Client.
//
// This would need a little re-work to be able to use 'dynamic'
// mouse sizeing of the children, like a MDI interface does, but i
// do NOT want to go the MDI way, which introduces yet ANOTHER
// 'hidden' window, to handle the children!!!
//
// Optional
// [ Status Bar - messages, and                 time: 12:34  //]
// 0, cyClient                                          cxClient,cyClient
//
// should perhaps one day use -
// HDWP BeginDeferWindowPos( int nNumWindows );   // number of windows
// but, time, ...
VOID  status_resize(VOID)
{
   RECT rc;
   rc = g_rcClient;
   g_rcStatus   = rc;
   // position TOP STATUS window - self handled status bar
   g_rcStatus.bottom = g_rcStatus.top + status_height;
   //MoveWindow(hwndStatus, 0, 0, rc.right - rc.left, status_height, TRUE);

   MoveWindow( hwndStatus,
      g_rcStatus.left,     // x
      g_rcStatus.top,      // y
      g_rcStatus.right - g_rcStatus.left,  // cx
      g_rcStatus.bottom - g_rcStatus.top,  // cy, adjusted to STATUS designed height
      TRUE );
}

DWORD g_dwWHLV;   // = ListView_ApproximateViewRect( g_hListView,
BOOL  g_bLVSzOff = FALSE;
extern   SIZE  g_TblChar;

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : DoResize
// Return type: VOID 
// Argument   : HWND hWnd
// Description: 
//              
///////////////////////////////////////////////////////////////////////////////
VOID DoResize(HWND hWnd)
{
   RECT  rc;
   INT   bar_width, cliht, statht;
   BOOL  addst = ( g_sSB.sb_hStatus && !g_sSB.sb_bHidden );

   if( g_bInExit )
      goto Set_Win_Size;   // forget it - we are dying ... no sizing done!

   //GetClientRect(hWnd, &rc);
   //g_rcClient = rc;
   rc = g_rcClient;
   cliht = RECTHEIGHT( (&rc) );  // get HEIGHT OF CLIENT
   g_rcStatus   = rc;
   g_rcDisplay  = rc;
   g_rcTablesz  = rc;
   g_rcVBar     = rc;
   g_rcListView = rc;
   statht = status_height;
   if(addst)
      statht += SB_DEF_HEIGHT;

//   statht += g_TblChar.cy + 1;   // at least ONE line of the TABLE
//   if( cliht < status_height)  // at LEAST THREE TABLE lines
   if( cliht < (statht + (g_TblChar.cy * MY_MIN_ROWS) + 1) )
   {
#ifdef ADD_LIST_VIEW
      // we should HIDE all the windows, perhaps except the top status???
      if( g_hListView ) // if LISTVIEW enabled AND visible
      {
         // no LISTVIEW visible while LESS THAN ?? LINES available for display
         Do_IDM_LISTVIEWOFF();   // hide the LIST VIEW
         g_bLVSzOff = TRUE;
      }
#endif // #ifdef ADD_LIST_VIEW

      if(cliht < statht)
      {
         ShowWindow(hwndRCD, SW_HIDE); // hide the TABLE completely
         ShowWindow(hwndBar, SW_HIDE); // and the BAR
      }
      else
      {
         // else we have LITTLE space, so use it for the TABLE or TABLE+BAR
         //ShowWindow(hwndRCD, SW_SHOW); // ensure TABLE visible
         goto Do_Table; // and position the TABLE in the tiny space

      }
#ifdef   ADDSTATUS
      if( cliht < status_height)
      {
         if( g_sSB.sb_hStatus && !g_sSB.sb_bOff )
            SBHide( &g_sSB );
      }
#endif   // #ifdef   ADDSTATUS
      return;
   }
#ifdef ADD_LIST_VIEW
   else
   {
      if( g_bLVSzOff ) // if LISTVIEW SHOULD BE enabled AND visible
      {
         Do_IDM_LISTVIEWOFF();   // show the LIST VIEW
         g_bLVSzOff = FALSE;
      }
   }
#endif // #ifdef ADD_LIST_VIEW

Do_Table:

   // position TOP STATUS window - self handled status bar
   g_rcStatus.bottom = g_rcStatus.top + status_height;
   g_rcDisplay.top   = g_rcStatus.bottom; // display STARTS after TOP status bar

   // position TABLE window - with or without the VERTICAL picture bar
   g_rcTablesz.top    += status_height;  // set the y vertical position
   g_rcTablesz.bottom -= status_height;  // and the cy height

   /* bar window is hidden unless in expand mode */
   bar_width = ((rc.right - rc.left) * BAR_WIN_WIDTH) / 100;
   // size the VERTICAL BAR window
   g_rcVBar.top    += status_height;      // start below TOP status line
   g_rcVBar.right   = rc.left + bar_width;
   g_rcVBar.bottom -= status_height;

#ifdef   ADDSTATUS
   if( g_sSB.sb_hStatus && !g_sSB.sb_bHidden )
   {
      if( g_sSB.sb_bOff )  // if logically OFF
         SBShow( &g_sSB );

      // remove the bottom ROW from ALL
      g_rcVBar.bottom     -= SB_DEF_HEIGHT;
      g_rcTablesz.bottom  -= SB_DEF_HEIGHT;
      g_rcListView.bottom -= SB_DEF_HEIGHT;
      g_rcDisplay.bottom  -= SB_DEF_HEIGHT;
   }
#endif   // ADDSTATUS

#ifdef ADD_LIST_VIEW
   if( g_hListView ) // if LISTVIEW enabled AND visible
   {
      if(( g_bIn5050                  ) ||
         ( DisplayMode == MODE_EXPAND ) )
      {
         double div = 2.0; // normal 'divisor'
         INT   dispht = RECTHEIGHT( (&g_rcDisplay) );
         INT   tht;
         // if give it SOME SPACE
         if( g_dwPercent ) // && (g_dwPercent != 50) )
         {
            div = (100.0 / (double)g_dwPercent);
         }

         tht = (INT)((double)dispht / div);

//         g_rcVBar.bottom    = g_rcVBar.bottom    / 2;
//         g_rcTablesz.bottom = g_rcTablesz.bottom / 2;
//         g_rcVBar.bottom    = g_rcVBar.bottom    / div;
//         g_rcTablesz.bottom = g_rcTablesz.bottom / div;
         g_rcVBar.bottom    = tht;
         g_rcTablesz.bottom = tht;

         g_rcListView.top    = g_rcTablesz.top + g_rcTablesz.bottom;   // y position = after table
         g_rcSizeBar         = g_rcListView; // keep this
         g_rcSizeBar.bottom  = g_rcSizeBar.top + 1;

         g_rcListView.top++;  // starts ONE pixel DOWN - for above SIZING bar

         ShowWindow(hwndRCD, SW_SHOW);

      }
      else
      {
         // LISTVIEW to 100% of client rectangle - top status, and minus bottom status, if on
         g_rcListView.top    = g_rcTablesz.top; // which is += status_height;  // set the y vertical position
         g_rcListView.bottom = g_rcListView.top + g_rcTablesz.bottom;  // and the cy height
         g_rcVBar.bottom     = 1;
         g_rcTablesz.bottom  = 1;

         ShowWindow(hwndRCD, SW_HIDE); // hide the TABLE completely

      }

      MoveWindow( g_hListView,
         g_rcListView.left,  // x pos
         g_rcListView.top,   // y pos
         (g_rcListView.right - g_rcListView.left),   // cx width
         (g_rcListView.bottom - g_rcListView.top),  // cy height
         TRUE );   // set new position, and size

      // ****************************************************
      g_dwWHLV = ListView_ApproximateViewRect( g_hListView,
         -1,   //  cx,
         -1,   //  cy,
         -1 ); // iCount
      sprtf( "LV Size = %d x %d pixels."MEOR,
         LOWORD(g_dwWHLV),
         HIWORD(g_dwWHLV) );
      // ****************************************************
   }
   else
#endif // #ifdef ADD_LIST_VIEW
   {
       ShowWindow(hwndRCD, SW_SHOW);
   }

   if( ( DisplayMode == MODE_EXPAND ) &&
       ( picture_mode               ) )
   {
           ShowWindow(hwndBar, SW_SHOW);
           //MoveWindow(hwndBar, 0, status_height,
           //        bar_width, rc.bottom - status_height, TRUE);
           MoveWindow( hwndBar,
              g_rcVBar.left,  // x pos
              g_rcVBar.top,   // y pos
              g_rcVBar.right, // cx width
              g_rcVBar.bottom,   // cy height
              TRUE );
           //MoveWindow(hwndRCD, bar_width, status_height,
           //        (rc.right - rc.left) - bar_width,
           //        rc.bottom - status_height, TRUE);
           g_rcTablesz.left  += bar_width;  // move to right, space for BAR
           g_rcTablesz.right -= bar_width;  // and reduce cx width
   }
   else
   {
           //MoveWindow(hwndRCD, 0, status_height, (rc.right - rc.left),
           //       rc.bottom - status_height, TRUE);
           //MoveWindow(hwndRCD,
           //   g_rcTable.left,
           //   g_rcTable.top,
           //   (g_rcTable.right  - g_rcTable.left),
           //   (g_rcTable.bottom - g_rcTable.top),
           //   TRUE);

           ShowWindow(hwndBar, SW_HIDE);
   }

   // set the start location, and size of the TABLE window
   MoveWindow(hwndRCD,
              g_rcTablesz.left,       // x
              g_rcTablesz.top,        // y
              g_rcTablesz.right,      // cx
              g_rcTablesz.bottom,     // cy
              TRUE);

   // establish a GLOBAL logical TABLE rectangle, in CLIENT co-ordinates
   g_Tablerc.top    = g_rcTablesz.top;
   g_Tablerc.bottom = g_Tablerc.top + g_rcTablesz.bottom;
   g_Tablerc.left   = g_rcTablesz.left;
   g_Tablerc.right  = g_Tablerc.left + g_rcTablesz.right;
   // *******************************************************************

Set_Win_Size:

   if( gbDnInit )
   {
      if( IsZoomed(hWnd) )
      {
         if( IsIcon )
         {
            IsIcon = FALSE;
            bChgIc = TRUE;
         }
         if( !IsZoom )
         {
            IsZoom = TRUE;
            bChgZm = TRUE;
         }
      }
      else if( IsIconic(hWnd) )
      {
         if( !IsIcon )
         {
            IsIcon = TRUE;
            bChgIc = TRUE;
         }
         if( IsZoom )
         {
            IsZoom = FALSE;
            bChgZm = TRUE;
         }
      }
      else  // NOT zoomed, nor iconic - set a SIZE
      {
         GetWindowRect(hWnd, &rc);
         if( IsIcon )
         {
            IsIcon = FALSE;
            bChgIc = TRUE;
         }
         if( IsZoom )
         {
            IsZoom = FALSE;
            bChgZm = TRUE;
         }

         if( ( grcSize.left   != rc.left              ) ||
             ( grcSize.top    != rc.top               ) ||
             ( grcSize.right  != (rc.right - rc.left) ) ||
             ( grcSize.bottom != (rc.bottom - rc.top) ) )
         {
            grcSize.left = rc.left;
            grcSize.top  = rc.top;
            grcSize.right = (rc.right - rc.left);
            grcSize.bottom = (rc.bottom - rc.top);
            bChgSz = TRUE;
         }
      }
   }
}



//VOID  Do_WM_SIZE( HWND hWnd, DWORD wid,   // specifies the new width of the client area. 
//         DWORD ht,   // The high-order word of lParam specifies the new height of the client area.
//         UINT  req )
VOID  Do_WM_SIZE( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   g_rcClient.right  = LOWORD(lParam);
   g_rcClient.bottom = HIWORD(lParam);
   g_rcClient.left   = g_rcClient.top = 0;

   DoResize(hWnd);   // resize, move and paint TABLE[+BAR] [+LISTVIEW]

#ifdef   ADDSTATUS
   // if a bottom status, and it is not hidden
   SBSize( &g_sSB, wParam, lParam );   // set bottom status bar size
#endif   // ADDSTATUS

   status_resize();  // move, and repaint the TOP STATUS BAR

}

// eof dc4wSize.c
