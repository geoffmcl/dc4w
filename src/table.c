
/****************************** Module Header *******************************
* Module Name: TABLE.C
*
* Standard table class and main interface functions.
*
* Functions:
*
* gtab_wndproc()  - system main for table class
* gtab_init()
* gtab_deltools()
* gtab_sendtq()
* gtab_freelinedata()
* gtab_createtools()
* gtab_deltable()
* gtab_buildtable()
* gtab_setsize()
* gtab_newsize()
* gtab_calcwidths()
* gtab_alloclinedata()
* gtab_invallines()
* gtab_append()
*
* Comments:
*
* The table class communicates with its 'owner' window to
* get the layout info and the data to display. The owner window handle
* can be sent as the lParam in CreateWindow - if not, the parent window will
* be used.
*
* After creating the window, send it a TM_NEWID message, with a 'data id'
* as the lParam. This is any non-zero 32-bit value. The table will then call
* back to its owner window to find out how many rows/columns, then to fetch
* the name/properties of each column, and finally to get the data to display.
*
* Send TM_NEWID of 0 to close (or destroy the window) - wait for TQ_CLOSE
* (in either case) before discarding data. Send
* TM_REFRESH if data or row-count changes; send TM_NEWLAYOUT if column
* properties or nr cols change etc - this is the same as sending TM_NEWID
* except that no TQ_CLOSE happens on TM_NEWLAYOUT.
*
* TQ_SELECT is sent whenever the current selection changes. TQ_ENTER is sent
* when enter or double-click occurs.
*
* *** TBD *** Add hover select, and single click expand
*
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"
#include "tpriv.h"

//#define   ADDTBLMSG   // output a TABLE diagnostic message
#undef   ADDTBLMSG   // output a TABLE diagnostic message

extern   BOOL  IsViewExpFile( VOID );
extern   void  gtab_updateline(HWND hwnd, lpTable ptab, int line);
extern   BOOL  CopyOneIsOK( PCFDLGSTR pcfds );
extern   BOOL  DeleteOneIsOK( PCFDLGSTR pcfds );
extern   void  gtab_setcursor( HCURSOR hCur );

/* global tools etc */
extern   HANDLE   hLibInst;
extern   BOOL     gfBusy;
extern   INT      giSelection;    // = -1 if NONE, or is selected row in table

HANDLE hNormCurs;
HANDLE hVertCurs; // = IDC_CURSOR1
HCURSOR  hHorzCurs;  // = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR2));
HCURSOR  hWorkCurs;  // = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR3));
HPEN hpenDotted = 0;    // no pen yet
UINT gtab_msgcode;      // now a simple WM_USER++ type
// GLOBAL copy of table allocation
lpTable  g_ptab;  // = (lpTable)GetWindowLongPtr(hwnd, WL_TABLE);
SIZE  g_TblChar;
/* function prototypes */
LRESULT CALLBACK gtab_wndproc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
//long FAR PASCAL gtab_wndproc(HWND, UINT, UINT, long);

BOOL  gtab_createtools(void);
void  gtab_deltools(void);
void gtab_deltable(HWND hwnd, lpTable ptab);
lpTable gtab_buildtable(HWND hwnd, LPARAM id);
void gtab_setsize(HWND hwnd, lpTable ptab);
void gtab_newsize(HWND hwnd, lpTable ptab);
void gtab_calcwidths(HWND hwnd, lpTable ptab);
BOOL gtab_alloclinedata(HWND hwnd, HANDLE heap, lpTable ptab);
void gtab_invallines(HWND hwnd, lpTable ptab, int start, int count);
void gtab_append(HWND hwnd, lpTable ptab, int rows, LPARAM id);

// NEW
#ifdef   ADDOLDDIFF
// but later abandoned for direct write (thread)
BOOL  gtab_writediff(HWND hwnd, lpTable ptab, HANDLE heap, LPARAM lParam );
#endif   // #ifdef   ADDOLDDIFF

//BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, lpTable ptab, WPARAM wParam, LPARAM lParam );
BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, WPARAM wParam, LPARAM lParam );
VOID  gtab_setWL_TABLE( HWND hwnd, LONG_PTR lPar );

/***************************************************************************
 * Function: gtab_init
 *
 * Purpose:
 *
 * Initialise window class - called from DLL main init
 */
BOOL  gtab_init(void)
{
   WNDCLASS wc;

   if( !gtab_createtools() )
      return FALSE;

   //gtab_msgcode = RegisterWindowMessage(TableMessage);
   gtab_msgcode = MWM_TABLE;

   wc.style         = CS_GLOBALCLASS | CS_DBLCLKS;
   wc.lpfnWndProc   = gtab_wndproc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = WLTOTAL;
   wc.hInstance     = hLibInst;
   wc.hIcon         = NULL;
   wc.hCursor       = NULL;
   wc.hbrBackground = GetStockObject(WHITE_BRUSH);
   wc.lpszClassName = TableClassName;
   wc.lpszMenuName  = NULL;

   if(  RegisterClass(&wc) )
      return TRUE;

   return FALSE;
}

/***************************************************************************
 * Function: gtab_createtools
 *
 * Purpose:
 *
 * Load cursors and pens.
 */
BOOL  gtab_createtools(void)
{
    hNormCurs = LoadCursor(NULL, IDC_ARROW);
    hVertCurs = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR1));

    hHorzCurs = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR2));

    hWorkCurs = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR3));

    hpenDotted = CreatePen(PS_DOT, 1, RGB(0, 0, 0));

    if( hVertCurs && hNormCurs && hpenDotted && hHorzCurs && hWorkCurs )
    {
       return TRUE;
    }
    else
    {
       gtab_deltools();
       return FALSE;
    }
}

VOID  gtab_sethorzcurs( VOID )
{
   gtab_setcursor(hHorzCurs);
}

VOID  gtab_setnormcurs( VOID )
{
   gtab_setcursor(hNormCurs);
}

/***************************************************************************
 * Function: gtab_deltools
 *
 * Purpose:
 *
 * Delete pen
 */
 void
gtab_deltools(void)
{
    if( hpenDotted )
       DeleteObject(hpenDotted);
    hpenDotted = 0;
//    hHorzCurs = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR2));
//    hWorkCurs = LoadCursor(hLibInst, MAKEINTRESOURCE(IDC_CURSOR3));
}

VOID  tb_WM_CREATE( HWND hwnd, LPARAM lParam )
{
   lpTable           ptab;
   CREATESTRUCT FAR * csp;
   HWND              hOwner;
   HANDLE            hHeap;

   /* create window. set the wnd extra bytes to
    * contain the owner window, a heap and a null table.
    * Owner window is either in lParam or the parent.
    * Then wait for TM_NEWID.
    */
   csp = (CREATESTRUCT FAR *) lParam;
   if (csp->lpCreateParams == NULL)
   {
           hOwner = GetParent(hwnd);
   }
   else
   {
           hOwner = (HWND) csp->lpCreateParams;
   }

   ptab = NULL;
   hHeap = gmem_init( "tb_WM_CREATE" );

   //SetWindowLongPtr(hwnd, WL_TABLE, (LONG_PTR) ptab);
   gtab_setWL_TABLE(hwnd, (LONG_PTR) ptab);   // in CREATE, set ZERO
   SetWindowLongPtr(hwnd, WW_OWNER, (LONG_PTR) hOwner);
   SetWindowLongPtr(hwnd, WW_HEAP, (LONG_PTR) hHeap);

   SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
   SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
}

VOID  tb_WM_PAINT( lpTable ptab, HWND hwnd )
{
   PAINTSTRUCT ps;
   HDC         hDC;

   hDC = BeginPaint(hwnd, &ps);
   if( ptab != NULL )
   {
      int y, y2, i;
           /* separator lines between fixed rows/columns
            * (ie headers) and the rest - if enabled
            */
           /* paint here first for good impression,
            * and again after to clean up!!
            */
           if (ptab->hdr.vseparator) {
                   gtab_vsep(hwnd, ptab, hDC);
           }
           if (ptab->hdr.hseparator) {
                   gtab_hsep(hwnd, ptab, hDC);
           }

           /* paint only the rows that need painting */
           for(i = 0; i < ptab->nlines; i++)
           {
                   y = ptab->pdata[i].linepos.start;
                   y2 = y + ptab->pdata[i].linepos.px_size;
                   if( (y  <= ps.rcPaint.bottom) &&
                       (y2 >= ps.rcPaint.top   ) )
                   {
                           gtab_paint(hwnd, hDC, ptab, i);
                   }
           }

           if (ptab->hdr.vseparator) {
                   gtab_vsep(hwnd, ptab, hDC);
           }
           if (ptab->hdr.hseparator) {
                   gtab_hsep(hwnd, ptab, hDC);
           }

           if (ptab->selvisible) {
                   gtab_invertsel(hwnd, ptab, hDC);
           }
   }

   if( disp_number == 0 )
   {
      if( current_view )
      {
         disp_number = view_getrowcount(current_view);
      }
      else
      {
         disp_number = (INT)-1;
      }

      if( disp_number == 0 )
      {
         Paint_Warning( hDC, 0 );
      }
   }

   bHadPaint = TRUE;

   EndPaint(hwnd, &ps);
}


#ifndef  NDEBUG
extern   LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern   BOOL  NotInX( UINT a );

typedef  struct   tagUSRTM {
   UINT     id;
   LPTSTR   str;
}USRTM, * PUSRTM;

USRTM sUsrTm[] = {
   { TM_REFRESH, "TM_REFRESH - data/rows changed" },
   { TM_NEWLAYOUT, "TM_NEWLAYOUT - cols/props/layout changed" },
   { TM_NEWID,   "TM_NEWID - Close old and display new" },
   { TM_SELECT, "TM_SELECT - Select and show" },
   { TM_PRINT,  "TM_PRINT - Print table" },
   { TM_TOPROW, "TM_TOPROW - Reurn top row" },
   { TM_ENDROW, "TM_ENDROW - Return end row visible" },
   { TM_APPEND, "TM_APPEND - Append new row" },
#ifdef   ADDOLDDIFF
   { TM_WRITEDIFF, "TM_WRITEDIFF - Write diff file" },
#endif   // #ifdef   ADDOLDDIFF
   { TM_ROWCOUNT, "TM_ROWCOUNT - Return row count" },
   { TM_ENTER,    "TM_ENTER - Select and Expand" },
   { 0,           0 }
};

LPTSTR   GetTM( UINT msg )
{
   LPTSTR   lps = 0;
   PUSRTM   pu   = &sUsrTm[0];
   while( pu->str )
   {
      if( pu->id == msg )
      {
         lps = pu->str;
         break;
      }
      pu++; // to next
   }
   return lps;
}

#endif   // #ifndef NDEBUG

// case WM_NOTIFY
// typedef struct tagNMHDR { 
//    HWND hwndFrom; 
//    UINT idFrom; 
//    UINT code; 
//} NMHDR; 
DWORD    g_NOTIFIES[16];
DWORD    g_dwNotifies = 0;
BOOL  _newcode( DWORD code )
{
   DWORD i;
   for(i = 0; i < g_dwNotifies; i++)
   {
      if(g_NOTIFIES[i] == code)
         return FALSE;
   }
   if( g_dwNotifies < 16 )
      g_NOTIFIES[g_dwNotifies++] = code;

   return TRUE;
}

LRESULT  tb_WM_NOTIFY( lpTable ptab, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
   INT      idCtrl = (INT) wParam;
   LPNMHDR  pnmh   = (LPNMHDR) lParam;
   DWORD    code   = pnmh->code;
   
   //sprtf( "tb_WM_NOTIFY with code = %d"MEOR, code );
   if( pnmh->hwndFrom == g_hwndTT )
   {
      if( code == TTN_GETDISPINFO )
      {
         LPNMTTDISPINFO lpttd = (LPNMTTDISPINFO)pnmh;
         lpttd->lpszText = &g_strTT[0];
         SendMessage(pnmh->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 300);
         //sprtf( "TTN_GETDISPINFO: Set tool tip to 300 pixels"MEOR );
         return 0;
      }

      // examples thru here are -
// WM_NOTIFY: From 0x93c(TT), id = 0, code = -12
// WM_NOTIFY: From 0x93c(TT), id = 0, code = -521
//      if( _newcode(code) )
//      {
//         sprtf( "WM_NOTIFY: From %#x(TT), id = %d, code = %d"MEOR,
  //          pnmh->hwndFrom,
    //        pnmh->idFrom,
      //      code );
//      }
   }
   else
   {
      sprtf( "WM_NOTIFY: From %#x, id = %d, code = %d"MEOR,
         pnmh->hwndFrom,
         pnmh->idFrom,
         code );
   }
   return (DefWindowProc(hwnd, msg, wParam, lParam));
}

POINT g_ptMouTab, g_ptMTScn, g_ptMTClient;
extern   VOID  Do_WM_MOUSEMOVE( HWND hWnd, WPARAM wParam, DWORD xPos, DWORD yPos,
                               DWORD src );
VOID  ptab_updateclient( HWND hwnd, WPARAM wParam )
{
   g_ptMTScn = g_ptMouTab;
   if( ClientToScreen( hwnd, &g_ptMTScn ) )
   {
      g_ptMTClient = g_ptMTScn;
      if( ScreenToClient( hwndClient, &g_ptMTClient ) )
         Do_WM_MOUSEMOVE( hwndClient, wParam, g_ptMTClient.x, g_ptMTClient.y, SRC_UPDCLIENT );
   }
}

VOID  ptab_WM_MOUSEMOVE( HWND hwnd, WPARAM wParam, LPARAM lParam, lpTable ptab )
{
   g_ptMouTab.x = LOWORD(lParam);
   g_ptMouTab.y = HIWORD(lParam);

   ptab_updateclient(hwnd, wParam);

   if( ptab )
   {
      gtab_move(hwnd, ptab, LOWORD(lParam), HIWORD(lParam));
   }
   else
   {
      //SetCursor(hNormCurs);
      gtab_setcursor(hNormCurs);
      
   }
}

/***************************************************************************
 * Function: gtab_wndproc
 *
 * Purpose:
 *
 * Window procedure for table
 */
//long FAR PASCAL
//gtab_wndproc(HWND hwnd, UINT msg, UINT wParam, long lParam)
LRESULT CALLBACK gtab_wndproc(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
   LRESULT  lRet = 0;
   lpTable  ptab = (lpTable)GetWindowLongPtr(hwnd, WL_TABLE);

   HWND hOwner;
   HANDLE hHeap;
   lpTableSelection pselect;
   long oldtop;
   long change;

#if   (defined( ADDTBLMSG) && !defined(NDEBUG) )
   if( NotInX(uMsg) )
   {
      LPTSTR   lps = GetTM(uMsg);
      if(lps)
      {
         static UINT prev_tm = 0;
         if( prev_tm != uMsg )
         {
            sprtf( "TBL: %s wP=%x lP=%x. (ptab=%#x)"MEOR, lps, wParam, lParam, ptab );
            prev_tm = uMsg;
         }
      }
      else
         sprtf( "TBL: %s wP=%x lP=%x. (ptab=%#x)"MEOR, GetWMStg(0, uMsg, wParam, lParam ), wParam, lParam, ptab );
   }
#endif   // #if ADDTBLMSG and !NDEBUG
   switch(uMsg)
   {

   case WM_CREATE:
          tb_WM_CREATE( hwnd, lParam );
           // If an application processes this message,
           // it should return zero to continue creation of the window. 
           break;

   case TM_NEWID: // only sent by function view_close(view)
           /* complete change of table.
            * close old table, discard memory and
            * build new table
            */
           if( ptab != NULL )
           {
                   gtab_sendtq(hwnd, TQ_CLOSE, ptab->hdr.id);
                   gtab_deltable(hwnd, ptab);
                   //SetCursor(hNormCurs);
                   gtab_setcursor(hNormCurs);
                   //SetWindowLongPtr(hwnd, WL_TABLE, (LONG_PTR)0);
                   gtab_setWL_TABLE(hwnd, 0);   // in TM_NEWID, deleted old, set ZERO
           }
           //if ( (ptab = gtab_buildtable(hwnd, lParam)) != NULL)
           ptab = gtab_buildtable(hwnd, lParam);   // allocate and build a NEW table
           if ( ptab )
           {
                   //SetWindowLongPtr(hwnd, WL_TABLE, (LONG_PTR) (LPSTR) ptab);
                   gtab_setWL_TABLE(hwnd, (LONG_PTR) ptab); // set allocated, new table
                   gtab_setsize(hwnd, ptab);
           }
           else
           {
                   SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
                   SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
           }
           bHadPaint = FALSE;
           InvalidateRect(hwnd, NULL, TRUE);
           break;

   case TM_NEWLAYOUT:
           /* change of layout but for same id. no TQ_CLOSE,
            * but otherwise same as TM_NEWID
            */
           if( ptab != NULL )
           {
              // delete any previous table
                   gtab_deltable(hwnd, ptab);
                   //SetCursor(hNormCurs);
                   gtab_setcursor(hNormCurs);
                   //SetWindowLongPtr(hwnd, WL_TABLE, (LONG_PTR)0);
                   gtab_setWL_TABLE(hwnd, 0);   // TM_NEWLAYOUT - deleted old - set ZERO
           }
           //if ( (ptab = gtab_buildtable(hwnd, lParam)) != NULL) {
           ptab = gtab_buildtable(hwnd, lParam);   // allocate / build new table
           if( ptab != NULL )
           {
                   //SetWindowLongPtr(hwnd, WL_TABLE, (LONG_PTR) (LPSTR) ptab);
                   gtab_setWL_TABLE(hwnd, (LONG_PTR) ptab);  // store the NEW table
                   gtab_setsize(hwnd, ptab); 
           }
           else
           {
                   SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
                   SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
           }
           bHadPaint = FALSE;
           InvalidateRect(hwnd, NULL, TRUE);    // cause a PAINT of the table changes
           break;

   case TM_REFRESH:
           /* data in table has changed. nrows may have
            * changed. ncols and col types have not changed
            */
           if (ptab != NULL) {
                   gtab_newsize(hwnd, ptab);
           }
           bHadPaint = FALSE;
           InvalidateRect(hwnd, NULL, TRUE);
           break;

   case TM_SELECT:
           if (ptab != NULL)
           {
                   pselect = (lpTableSelection) lParam;

                   /* we only support TM_SINGLE - so force the
                    * selection to a single row or cell.
                    */
                   gtab_select(hwnd, ptab, pselect->startrow,
                           pselect->startcell,
                           1,
                           (ptab->hdr.selectmode & TM_ROW) ?
                                   ptab->hdr.ncols : 1,
                           TRUE);
                   gtab_showsel_middle(hwnd, ptab);
           }
           break;

   case TM_PRINT:
           hHeap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);
           if (ptab != NULL)
           {
                   gtab_print(hwnd, ptab, hHeap, (lpPrintContext) lParam);
                   return(TRUE);
           }

   case TM_TOPROW:

           /* return top row. if wParam is TRUE, set lParam
            * as the new toprow
            */
           if (ptab == NULL)
           {
                   return(0);
           }
           oldtop = ptab->toprow;
           if ((wParam) && (lParam < ptab->hdr.nrows))
           {
                   change = lParam - ptab->toprow;
                   change -= ptab->hdr.fixedrows;
                   gtab_dovscroll(hwnd, ptab, change);
           }
           return(oldtop);

   case TM_ENDROW:
           /* return the last visible row in the window */
           if (ptab == NULL)
           {
                   return(0);
           }
           return(ptab->nlines + ptab->toprow - 1);


   case TM_APPEND:
           /* new rows have been added to the end of the
            * table, but the rest of the table has no
            * been change. Update without forcing redraw of
            * everything.
            * lParam contains the new total nr of rows
            */
           if (ptab != NULL)
           {
                   gtab_append(hwnd, ptab, wParam, lParam);
                   return(TRUE);
           }
           break;

#ifdef   ADDOLDDIFF
   case TM_WRITEDIFF: // from case IDM_WRITEDIFF:
           hHeap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);
           if( (ptab != NULL) && (hHeap != 0) )
           {
                   if( gtab_writediff(hwnd, ptab, hHeap, lParam) )
                      lRet = TRUE;
           }
           break;
#endif   // #ifdef   ADDOLDDIFF

   case TM_ROWCOUNT:
           /* return the total rows */
            lRet = 0;
            if( ptab )
               lRet = ptab->nlines;
            break;

   case TM_ENTER:
           if (ptab != NULL)
           {
              pselect = (lpTableSelection) lParam;
              gtab_select(hwnd, ptab,
                 pselect->startrow,
                 pselect->startcell,
                 1,
                 ((ptab->hdr.selectmode & TM_ROW) ? ptab->hdr.ncols : 1),
                 TRUE );
              gtab_showsel_middle(hwnd, ptab);
           }
           break;

   case WM_SIZE:
           if (ptab != NULL)
           {
                   gtab_setsize(hwnd, ptab);
           }
           // If an application processes this message, it should return zero.
           break;

   case WM_DESTROY:
           if (ptab != NULL)
           {
                   gtab_sendtq(hwnd, TQ_CLOSE, ptab->hdr.id);
                   gtab_deltable(hwnd, ptab);
           }
           hHeap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);
           gmem_freeall(hHeap);
           // If an application processes this message, it should return zero.
           break;

   case WM_PAINT:
           tb_WM_PAINT( ptab, hwnd );
           // An application returns zero if it processes this message.
           break;

   case WM_HSCROLL:
           if (ptab != NULL)
           {
                   gtab_msg_hscroll(hwnd, ptab,
                     GET_SCROLL_OPCODE(wParam, lParam),
                     GET_SCROLL_POS(wParam, lParam));
           }
           break;

   case WM_VSCROLL:
           if (ptab != NULL)
           {
                   gtab_msg_vscroll(hwnd, ptab,
                     GET_SCROLL_OPCODE(wParam, lParam),
                     GET_SCROLL_POS(wParam, lParam));
           }
           break;

   case WM_MOUSEMOVE:
      ptab_WM_MOUSEMOVE( hwnd, wParam, lParam, ptab );
      break;

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
   case WM_MOUSEWHEEL:
      if (ptab != NULL) {
         gtab_msg_vscroll(hwnd, ptab,
            ((HIWORD(wParam) & 0x8000) ? SB_LINEDOWN : SB_LINEUP),
             0 ); // not used
      }
      break;
#endif   // #if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)

   case WM_LBUTTONDOWN:
           if (ptab != NULL) {
                   gtab_press(hwnd, ptab, LOWORD(lParam), HIWORD(lParam));
           }
           break;

   case WM_LBUTTONUP:
           if (ptab != NULL)
           {
                   gtab_release(hwnd, ptab,
                           LOWORD(lParam), HIWORD(lParam));
           }
           break;

   case WM_LBUTTONDBLCLK:
           if (ptab != NULL)
           {
                   gtab_dblclick(hwnd, ptab,
                           LOWORD(lParam), HIWORD(lParam));
           }
           break;

   case WM_NOTIFY:
//      sprtf( "WM_NOTIFY: To gtab_wndproc %#x %#x"MEOR, wParam, lParam );
      lRet = tb_WM_NOTIFY( ptab, hwnd, uMsg, wParam, lParam );
      break;

   case WM_KEYDOWN:
           /* handle key presses for cursor movement about
            * the table, and return/space for selection.
            * Any key we don't handle is passed to the owner window
            * for him to handle.
            * The table window should have the focus
            */
           if (ptab != NULL)
           {
                   if (gtab_key(hwnd, ptab, wParam) != 0)
                   {
                           /* pass key to owner since
                            * we don't know what to do with it
                            */
                           hOwner = (HANDLE) GetWindowLongPtr(hwnd, WW_OWNER);
                           return(SendMessage(hOwner, WM_KEYDOWN,
                                   wParam, lParam));
                   }
                   else
                   {
                           return(0);      
                   }
           }
           break;

#ifdef   NO_PARENT_CONTEXT
   case WM_RBUTTONDOWN:
      tb_WM_RBUTTONDOWN( hwnd, wParam, lParam );
#endif   // #ifdef   NO_PARENT_CONTEXT
   default:
           //return(DefWindowProc(hwnd, msg, wParam, lParam));
           lRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
           break;

   }
   // return(TRUE); WHY THIS???
   return lRet;
}

/***************************************************************************
 * Function: gtab_sendtq
 *
 * Purpose:
 *
 * Send a table-query message to the owner window. Returns message
 * value.
 */
LRESULT gtab_sendtq(HWND hwnd, UINT cmd, LPARAM lParam)
{
        HWND hOwner = (HWND) GetWindowLongPtr(hwnd, WW_OWNER);
        return( SendMessage(hOwner, gtab_msgcode, cmd, lParam) );
}

/***************************************************************************
 * Function: gtab_freelinedata
 *
 * Purpose:
 *
 * Free the memory allocated for the array of lines (each containing
 * an array of Cells, each containing an array of chars for the actual
 * data). Called on any occasion that would change the number of visible lines
 */
void
gtab_freelinedata(HANDLE hHeap, lpTable ptab)
{
        int i, j, ncols;
        lpCellData cd;


        ncols = ptab->hdr.ncols;

        /* for each line */
        for(i = 0; i < ptab->nlines; i++) {
                /* for each cell */
                for (j = 0; j < ncols; j++) {
                        /* free up the actual text space */
                    cd = &ptab->pdata[i].pdata[j];
                    gmem_free(hHeap, (LPSTR) cd->ptext, cd->nchars, "gtab_alloclinedata" );
                }
                /* dealloc array of CellData */
            gmem_free(hHeap, (LPSTR) ptab->pdata[i].pdata, (sizeof(CellData) * ncols), "gtab_alloclinedata1");
        }
        /* de-alloc array of linedatas */
        gmem_free(hHeap, (LPSTR) ptab->pdata, (sizeof(LineData) * ptab->nlines), "gtab_alloclinedata0" );
        ptab->pdata = NULL;
}

/***************************************************************************
 * Function: gtab_alloclinedata
 *
 * Purpose:
 *
 * Allocate and init array of linedatas (include cell array
 * and text for each cell)
 */
BOOL
gtab_alloclinedata(HWND hwnd, HANDLE heap, lpTable ptab)
{
        lpLineData pline;
        lpCellData cd;
        int i, j;

        ptab->pdata =
           (lpLineData) gmem_get(heap, (sizeof(LineData) * ptab->nlines), "gtab_alloclinedata0" );
        if (ptab->pdata == NULL) {
                return(FALSE);
        }
        for (i = 0; i < ptab->nlines; i++) {
                pline = &ptab->pdata[i];
                pline->linepos.px_size = ptab->rowheight;   // pixel height (LOGFONT)
                pline->pdata =
                   (lpCellData) gmem_get(heap, (sizeof(CellData) * ptab->hdr.ncols), "gtab_alloclinedata1" );
                if (pline->pdata == NULL) {
                        return(FALSE);
                }
                for (j = 0; j < ptab->hdr.ncols; j++) {
                        cd = &pline->pdata[j];
                        cd->props.valid = 0;
                        cd->flags = 0;
                        cd->nchars = ptab->pcolhdr[j].nchars;
                        if (cd->nchars > 0) {
                                cd->ptext =
                                   gmem_get(heap, cd->nchars, "gtab_alloclinedata" );
                                if (cd->ptext == NULL) {
                                        return(FALSE);
                                }
                        }
                }
        }
        return(TRUE);
}

/***************************************************************************
 * Function: gtab_deltable
 *
 * Purpose:
 *
 * Free up all table data structures. Called for new layout or new data.
 */
void
gtab_deltable(HWND hwnd, lpTable ptab)
{
   HANDLE   hHeap;
   int      ncols;

   if( ptab == NULL )
   {
           return;
   }

   hHeap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);

   ncols = ptab->hdr.ncols;

   if (ptab->pcolhdr != NULL)
   {
      gmem_free(hHeap, (LPSTR) ptab->pcolhdr, (sizeof(ColProps) * ncols), "gtab_buildtable1" );
   }
   if (ptab->pcellpos != NULL)
   {
      gmem_free(hHeap, (LPSTR) ptab->pcellpos, (sizeof(CellPos) * ncols), "gtab_buildtable" );
   }
   if (ptab->pdata != NULL)
   {
           gtab_freelinedata(hHeap, ptab);
   }

   // finally delete the table structure itself
   gmem_free(hHeap, (LPSTR) ptab, sizeof(Table), "gtab_buildtable0" );
}


/***************************************************************************
 * Function: gtab_buildtable
 *
 * Purpose:
 *
 * Build up a Table struct (excluding data allocation and
 * anything to do with font or window size).
 * Return ptr to this or NULL if error
 */
lpTable gtab_buildtable(HWND hwnd, LPARAM id)
{
        lpTable      ptab;
        HANDLE       hHeap;
        int          ncols, i;
        ColPropsList cplist;

        hHeap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);
        ptab = (lpTable) gmem_get(hHeap, sizeof(Table), "gtab_buildtable0" );

        if( ptab == NULL )
        {
           chkme( "MEMORY ERROR: Failed to get TABLE memory!"MEOR );
           return NULL;
        }

        /* get the row/column count from owner window */
        ptab->hdr.id = id;    // this is the lParam, and can be ZERO

        ptab->hdr.props.valid = 0;
        ptab->hdr.sendscroll = FALSE;
        if( gtab_sendtq(hwnd, TQ_GETSIZE, (LPARAM)&ptab->hdr) == FALSE)
        {
           //lpTableHdr phdr = &ptab->hdr;
           // NOTE: If id (PVIEW) is NULL, the get size returns FALSE, so
           if( id == 0 )
              goto Bld_Err2;  // this is NORMAL in a CLOSE situation, so no
           // chkme message!

           goto Bld_Err;
        }

        ncols = ptab->hdr.ncols;
        ptab->pcolhdr = (lpColProps) gmem_get(hHeap, (sizeof(ColProps) * ncols), "gtab_buildtable1" );
        if( ptab->pcolhdr == NULL )
        {
           /* should prob send TQ_CLOSE at this point */
           goto Bld_Err;
        }

        /* init col properties to default */
        for( i=0; i < ncols; i++ )
        {
                ptab->pcolhdr[i].props.valid = 0;
                ptab->pcolhdr[i].nchars = 0;
        }

        /* get the column props from owner */
        cplist.plist = ptab->pcolhdr;
        cplist.id = id;
        cplist.startcol = 0;
        cplist.ncols = ncols;
        gtab_sendtq(hwnd, TQ_GETCOLPROPS, (LPARAM)&cplist);

        /* init remaining fields */
        ptab->pcellpos = (lpCellPos) gmem_get(hHeap, (sizeof(CellPos) * ncols), "gtab_buildtable" );
        if( ptab->pcellpos == NULL )
        {
           goto Bld_Err;
        }

        ptab->scrollscale = 1;
        ptab->scroll_dx = 0;
        ptab->toprow = 0;
        ptab->pdata = NULL;
        ptab->nlines = 0;
        ptab->trackmode = TRACK_NONE;

        /* we have to notify owner of the current selection
         * whenever it is changed
         */
        ptab->select.id = id;
        gtab_select(hwnd, ptab, 0, 0, 0, 0, TRUE);

        /* calc ave height/width, cell widths and min height.
         * these change only when cell properties / col count changes -
         * ie only on rebuild-header events
         */
        gtab_calcwidths(hwnd, ptab);

        return ptab;

Bld_Err:

        chkme( "ERROR: Something FAILED in gtab_buildtable()!!!"MEOR );

Bld_Err2:

        gtab_deltable(hwnd, ptab);

        return NULL;
}

/***************************************************************************
 * Function: gtab_setsize - ie WM_SIZE to table procedure
 *
 * Purpose:
 *
 * Set sizes that are based on window size and scroll pos
 * set:
 *      winwidth
 *      nlines
 *      cellpos start, clip start/end
 * Allocate linedata and init
 */

// note: handle is that of the table window, so the GetClientRect() only gets
// the size set when handling a WM_SIZE to the FRAME window.
void
gtab_setsize(HWND hwnd, lpTable ptab)
{
   RECT rc;
   int nlines;
   HANDLE heap;
   long range, change;

   GetClientRect(hwnd, &rc);

   ptab->winwidth = rc.right - rc.left;
   nlines = (rc.bottom - rc.top) / ptab->rowheight;

   /* nlines is the number of whole lines - add one extra
    * for the partial line at the bottom
    */
   nlines += 1;

   /* alloc space for nlines of data - if nlines has changed */
   if (nlines != ptab->nlines)
   {
                heap = (HANDLE) GetWindowLongPtr(hwnd, WW_HEAP);
                gtab_freelinedata(heap, ptab);
                ptab->nlines = nlines;
                if (!gtab_alloclinedata(hwnd, heap, ptab))
                {
                        ptab->nlines = 0;
                        return;
                }
   }

   /* set scroll vertical range */
   range = ptab->hdr.nrows - (ptab->nlines - 1);
   if (range < 0)
   {
                range = 0;
                change =  -(ptab->toprow);
   }
   else if (ptab->toprow > range)
   {
                change = range - ptab->toprow;
   }
   else
   {
                change = 0;
   }

   /* the scroll range must be 16-bits for Win3
    * scale until this is true
    */
   ptab->scrollscale = 1;
   while (range > 32766)
   {
                ptab->scrollscale *= 16;
                range /= 16;
   }

   SetScrollRange(hwnd, SB_VERT, 0, (int) range, TRUE);

   gtab_dovscroll(hwnd, ptab, change);

   /* set horz scroll range */
   range = ptab->rowwidth - ptab->winwidth;
   if (range < 0)
   {
                range = 0;
                change = -(ptab->scroll_dx);
   }
   else if (ptab->scroll_dx > range)
   {
              change = range - ptab->scroll_dx;
   }
   else
   {
             change = 0;
   }

   /* horz scroll range will always be < 16 bits */
   SetScrollRange(hwnd, SB_HORZ, 0, (int) range, TRUE);

   gtab_dohscroll(hwnd, ptab, change);

}

/***************************************************************************
 * Function: gtab_calcwidths
 *
 * Purpose:
 *
 * Set column widths/height and totals (based on column props)
 * - no assumption of window size (see gtab_setsize)
 * sets avewidth,rowheight,cellpos.size,rowwidth (total of cellpos.size)
 */
void
gtab_calcwidths(HWND hwnd, lpTable ptab)
{
   int i, cxtotal, cx, ave;
   TEXTMETRIC tm, tmcol;
   HDC hdc;
   lpProps hdrprops, cellprops;
   HFONT hfont;

   hdrprops = &ptab->hdr.props;
   hdc = GetDC(hwnd);
   if (hdrprops->valid & P_FONT)
   {
           hfont = SelectObject(hdc, hdrprops->hFont);
   }
   GetTextMetrics(hdc, &tm);

   if (hdrprops->valid & P_FONT)
   {
           SelectObject(hdc, hfont);
   }
   ReleaseDC(hwnd, hdc);

   /* get width and height of average character */
   ptab->avewidth = tm.tmAveCharWidth;
   ptab->rowheight = tm.tmHeight + tm.tmExternalLeading;
   if (hdrprops->valid & P_HEIGHT)
   {
           ptab->rowheight = hdrprops->height;
   }

   if(( g_TblChar.cx != ptab->avewidth ) ||
      ( g_TblChar.cy != ptab->rowheight) )
   {
      // set first/new character sizes
      g_TblChar.cx = ptab->avewidth;
      g_TblChar.cy = ptab->rowheight;

      sprtf( "gtab_calcwidths: for %#x = (%dx%d)."MEOR, ptab,
         g_TblChar.cx,
         g_TblChar.cy );
   }

   /* set pixel width of each cell (and add up for row total)
    * based on ave width * nr chars, unless P_WIDTH set
    */
   cxtotal = 0;
   for (i = 0; i < ptab->hdr.ncols; i++)
   {
           cellprops = &ptab->pcolhdr[i].props;

           if (cellprops->valid & P_WIDTH)
           {
                   cx = cellprops->width;
           }
           else if (hdrprops->valid & P_WIDTH)
           {
                   cx = hdrprops->width;
           }
           else
           {

                   if (cellprops->valid & P_FONT)
                   {
                           hdc = GetDC(hwnd);
                           hfont = SelectObject(hdc, cellprops->hFont);
                           GetTextMetrics(hdc, &tmcol);
                           SelectObject(hdc, hfont);
                           ReleaseDC(hwnd, hdc);
                           ave = tmcol.tmAveCharWidth;
                   }
                   else
                   {
                           ave = ptab->avewidth;
                   }
                   /* ave width * nchars */
                   cx =  ptab->pcolhdr[i].nchars + 1;
                   cx *= ave;
           }

           /* add 2 pixels for box lines */
           cx += 2;
           ptab->pcellpos[i].px_size = cx; // pixel width (LOGFONT)
           cxtotal += cx;
   }

   ptab->rowwidth = cxtotal;
}

/***************************************************************************
 * Function: gtab_newsize
 *
 * Purpose:
 *
 * Called when row data + possible nrows changes.
 * other changes are ignored
 */
void
gtab_newsize(HWND hwnd, lpTable ptab)
{
        TableHdr hdr;

        /* get new row count */
        hdr = ptab->hdr;
        gtab_sendtq(hwnd, TQ_GETSIZE, (LPARAM) &hdr);
        if (hdr.nrows != ptab->hdr.nrows) {
                ptab->hdr.nrows = hdr.nrows;
                gtab_setsize(hwnd, ptab);
        }

        gtab_invallines(hwnd, ptab, 0, ptab->nlines);

        bHadPaint = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
}

void
gtab_invallines(HWND hwnd, lpTable ptab, int start, int count)
{
        int i, j;

        for (i = start; i < start + count; i++) {
                for (j = 0; j < ptab->hdr.ncols; j++) {
                        ptab->pdata[i].pdata[j].flags = 0;
                }
        }
}

/***************************************************************************
 * Function: gtab_append
 *
 * Purpose:
 *
 * New rows have been added to the table. Adjust the scroll range and
 * position, and redraw the rows if the end of the table is currently
 * visible.
 * rows = the new total row count.
 */
void gtab_append(HWND hwnd, lpTable ptab, int rows, LPARAM id)
{
        long range;
        long oldrows;
        int line, nupdates;
        RECT rc;


        /* change to the new id */
        ptab->hdr.id = id;
        ptab->select.id = id;

        /* update the header, but remember the old nr of rows
         * so we know where to start updating
         */
        oldrows = ptab->hdr.nrows;

        /* check that the new nr of rows is not smaller. this is
         * illegal at this point and should be ignored
         */
        if (oldrows >= rows) {
                return; 
        }

        ptab->hdr.nrows = rows;

        /* set the vertical scroll range */
        range = rows - (ptab->nlines - 1);

        if (range < 0) {
                range = 0;      
        }

        /* force the scroll range into 16-bits for win 3.1 */
        ptab->scrollscale = 1;
        while (range > 32766) {
                ptab->scrollscale *= 16;
                range /= 16;
        }

        /* now set the scroll bar range and position */
        SetScrollRange(hwnd, SB_VERT, 0, (int) range, TRUE);
        if (range > 0) {
                SetScrollPos(hwnd, SB_VERT,
                        (int) (ptab->toprow / ptab->scrollscale), TRUE);
        }

        /* calculate which screen lines need to be updated - find what
         * screen line the start of the new section is at
         */
        line = gtab_rowtoline(hwnd, ptab, oldrows);
        if (line == -1) {
                /* not visible -> no more to do */
                return;
        }

        /* how many lines to update - rest of screen or nr of
         * new lines if less than rest of screen
         */
        nupdates = min((ptab->nlines - line), (int)(rows - oldrows));

        /* invalidate the screen line buffers to indicate data
         * needs to be refetch from parent window
         */
        gtab_invallines(hwnd, ptab, line, nupdates);

        /* calculate the region of the screen to be repainted -
         * left and right are same as window. top and bottom
         * need to be calculated from screen line height
         */
        
        GetClientRect(hwnd, &rc);
        rc.top += line * ptab->rowheight;
        rc.bottom = rc.top + (nupdates * ptab->rowheight);

        /* force a repaint of the updated region */
        bHadPaint = FALSE;
        InvalidateRect(hwnd, &rc, TRUE);
}
        

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

// NEW
//#define  MFH         HANDLE
//#define  MEOR        "\r\n"
//#define  ISNUM(a)    ( ( a >= '0' ) && ( a <= '9' ) )

VOID  gtab_setWL_TABLE( HWND hwnd, LONG_PTR lPar )
{
   g_ptab = (lpTable)lPar;   // GetWindowLongPtr(hwnd, WL_TABLE);

   SetWindowLongPtr(hwnd, WL_TABLE, lPar);
#ifdef   ADDTBLMSG   // output a TABLE diagnostic message
   sprtf( "WL_TABLE(%d): Set to %#x."MEOR, WL_TABLE, lPar );
#endif   // ADDTBLMSG

}


#define  MXLN     256
#define  fg_Left  0x00000001
#define  fg_Right 0x00000002

typedef struct tagLNS {
   DWORD l_iSame;
   DWORD l_dwLen;
   TCHAR l_szLn[ MXLN + 8 ];
}LNS, * PLNS;

static   TCHAR szFHdr[] = "----------%s"MEOR;

void  OutDiff( HWND hwnd, lpTable ptab, HANDLE heap, PTARGS pta, LPTSTR lpb, MFH fh,
         int icnt, PLNS plns )
{
   int   icol, ic2, ic3;
   int   ilen;
   int   idif, ibdif;
   int   isam;
   LPTSTR lps, lpf;
   PLNS  pln1;

   if( ic2 = icnt )
   {
      ic2--;
      while(ic2)
      {
         pln1 = &plns[ic2];
         if( pln1->l_dwLen )
            break;
         ic2--;
      }
   }
   if(ic2)
      icnt = ic2 + 1;
   else
      icnt = 0;
   idif = isam = 0;
   for( icol = 0; icol < icnt; icol++ )
   {
      pln1 = &plns[icol];
      if( idif )
      {
         if( pln1->l_iSame )
         {
            // not the SAME
            idif++;
            isam = 0;   // restart SAME
         }
         else
         {
            // back to same after diff
            if( pln1->l_dwLen )
            {
               isam++;
               if( isam == 1 )
               {
                  ic3 = icol;    // output up to here - count should be configurable
               }
               else if( isam == 2 )
               {
                  // had TWO significant lines as the SAME
                  // without any intervening DIFFERENCES
                  // TIME TO DO AN OUT
                  // but only include the FIRST significan SAME
#ifdef   COMBARGS
                  lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
                  lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
                  wsprintf(lpb, szFHdr, lpf );
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
                  for( ic2 = ibdif; ic2 < ic3; ic2++ )
                  {
                     pln1 = &plns[ic2];
                     if( ( pln1->l_iSame & fg_Left ) ||  // in LEFT only
                         ( pln1->l_iSame == 0      ) ||  // same in BOTH
                         ( pln1->l_dwLen == 0      ) )   // or is ZERO length
                     {
                        lps  = &pln1->l_szLn[0];
                        ilen = OutStg(fh, lps);
                        if( ilen == (int)-1 )
                           break;
                     }
                  }
                  if( ilen == (int)-1 )
                     break;
                  pln1 = &plns[ic2];
                  lps  = &pln1->l_szLn[0];
                  strcpy(lpb, lps);
                  strcat(lpb, MEOR);
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
#ifdef   COMBARGS
                  lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
                  lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
                  wsprintf(lpb, szFHdr, lpf );
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
                  for( ic2 = ibdif; ic2 < ic3; ic2++ )
                  {
                     pln1 = &plns[ic2];
                     if( ( pln1->l_iSame & fg_Right) ||  // in RIGHT only
                         ( pln1->l_iSame == 0      ) ||  // same in BOTH
                         ( pln1->l_dwLen == 0      ) )   // or is ZERO length
                     {
                        lps  = &pln1->l_szLn[0];
                        ilen = OutStg(fh, lps);
                        if( ilen == (int)-1 )
                           break;
                     }
                  }
                  if( ilen == (int)-1 )
                     break;
                  pln1 = &plns[ic2];
                  lps  = &pln1->l_szLn[0];
                  strcpy(lpb, lps);
                  strcat(lpb, MEOR);
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
                  idif = 0;
               }
            }
         }
      }
      else
      {
         if( pln1->l_iSame )
         {
            // different
            idif++;        // at least ONE difference
            ibdif = icol;  // keep START of difference(s)
            isam  = 0;     // restart same counter
         }
      }
   }  // for each record
   while( ( ilen != (int)-1 ) &&
          ( idif            ) )
      {
//         if( pln1->l_iSame )
//         {
//            idif++;
//            isam = 0;   // restart SAME
//         }
//         else
         {
            // back to same after diff
//            if( pln1->l_dwLen )
            {
//               isam++;
               //if( isam == 2 )
               {
                  // time to do an OUT
#ifdef   COMBARGS
                  lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
                  lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
                  wsprintf(lpb, szFHdr, lpf );
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
                  for( ic2 = ibdif; ic2 < icol; ic2++ )
                  {
                     pln1 = &plns[ic2];
                     if( ( pln1->l_iSame & fg_Left ) ||  // in LEFT only
                         ( pln1->l_iSame == 0      ) ||  // same in BOTH
                         ( pln1->l_dwLen == 0      ) )   // or is ZERO length
                     {
                        lps  = &pln1->l_szLn[0];
                        ilen = OutStg(fh, lps);
                        if( ilen == (int)-1 )
                           break;
                     }
                  }
                  if( ilen == (int)-1 )
                     break;
                  //pln1 = &plns[ic2];
                  //lps  = &pln1->l_szLn[0];
                  ilen = OutStg(fh, MEOR);
                  if( ilen == (int)-1 )
                     break;
#ifdef   COMBARGS
                  lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
                  lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
                  wsprintf(lpb, szFHdr, lpf );
                  ilen = OutStg(fh, lpb);
                  if( ilen == (int)-1 )
                     break;
                  for( ic2 = ibdif; ic2 < icol; ic2++ )
                  {
                     pln1 = &plns[ic2];
                     if( ( pln1->l_iSame & fg_Right) ||  // in RIGHT only
                         ( pln1->l_iSame == 0      ) ||  // same in BOTH
                         ( pln1->l_dwLen == 0      ) )   // or is ZERO length
                     {
                        lps  = &pln1->l_szLn[0];
                        ilen = OutStg(fh, lps);
                        if( ilen == (int)-1 )
                           break;
                     }
                  }
                  if( ilen == (int)-1 )
                     break;
                  //pln1 = &plns[ic2];
                  //lps  = &pln1->l_szLn[0];
                  ilen = OutStg(fh, MEOR);
                  if( ilen == (int)-1 )
                     break;
               }
               idif = 0;
            }
         }
      }
}


void  AddNum2Buf( LPTSTR lpb, LPTSTR lpn )
{
   int   i, j, k, l;
   TCHAR c;
   if( ( lpn ) &&
       ( j = strlen(lpn) ) )
   {
      k = j;
      for( i = 0; i < j; i++ )
      {
         c = lpn[i];
         if( ISNUM( c ) )
         {
            l = i;   // begin of number
            i++;     // bump past it
            k = 1;   // start number counter
            break;
         }
      }
      for( ; i < j; i++ )
      {
         c = lpn[i]; // extract next
         if( !ISNUM(c) )   // end if NOT a number
            break;
         k++;  // count NUMERIC values
      }
      if( k )
      {
         if( k > 5 )
         {
            strcpy( lpb, &lpn[l] ); // copy the BIG number
         }
         else
         {
            // less than or equal to the length of 5
            j = 5 - k;
            while( j-- )
               strcat(lpb," ");  // fill in the length
            strcat( lpb, &lpn[l] );
         }
         if( ( i = strlen(lpb) ) &&
             ( i > 5           ) )
         {
            i--;
            while( !ISNUM(lpb[i]) )
            {
               lpb[i] = 0;
               if(i == 0)
                  break;
               i--;
            }
         }
      }
   }

   // if blank after all that
   if( *lpb == 0 )
      lstrcpy( lpb, "     " );
}

int   OutCompNms( PTARGS pta, LPTSTR lpb, MFH fh )
{
   int   ilen;
   LONG  lg;
   LARGE_INTEGER  li;
   LPTSTR      lpf;

   li.LowPart  = pta->ta_sFDL.nFileSizeLow;
   li.HighPart = pta->ta_sFDL.nFileSizeHigh;
#ifdef   COMBARGS
   lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
   lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
   wsprintf(lpb,
      MEOR"Comparing [%s] %s %s ",
      lpf,
      GetI64Stg( &li ),
      GetFDTStg( &pta->ta_sFDL.ftLastWriteTime ) );
   if( ( pta->ta_sFDL.nFileSizeHigh == pta->ta_sFDR.nFileSizeHigh ) &&
       ( pta->ta_sFDL.nFileSizeLow  == pta->ta_sFDR.nFileSizeLow  ) )
   {
      strcat( lpb, "=SIZE"MEOR );
   }
   else
   {
      if( pta->ta_sFDL.nFileSizeHigh == pta->ta_sFDR.nFileSizeHigh )
      {
         if( pta->ta_sFDL.nFileSizeLow  > pta->ta_sFDR.nFileSizeLow  )
            strcat( lpb, "BIGGER"MEOR );
         else
            strcat( lpb, "SMALLER"MEOR );
      }
      else
      {
         if( pta->ta_sFDL.nFileSizeHigh > pta->ta_sFDR.nFileSizeHigh )
            strcat( lpb, "BIGGER"MEOR );
         else
            strcat( lpb, "SMALLER"MEOR );
      }
   }

   li.LowPart  = pta->ta_sFDR.nFileSizeLow;
   li.HighPart = pta->ta_sFDR.nFileSizeHigh;
#ifdef   COMBARGS
   lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
   lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
   wsprintf(EndBuf(lpb),
      "With      [%s] %s %s ",
      lpf,
      GetI64Stg( &li ),
      GetFDTStg( &pta->ta_sFDR.ftLastWriteTime ) );
   lg = CompareFileTime( &pta->ta_sFDL.ftLastWriteTime, &pta->ta_sFDR.ftLastWriteTime );
   if( lg < 0 )
      strcat( lpb, "NEWER"MEOR );
   else if( lg > 0 )
      strcat( lpb, "OLDER"MEOR );
   else
      strcat( lpb, "SAME"MEOR );
   strcat(lpb,MEOR);
   ilen = OutStg(fh, lpb);
   return ilen;
}



LPTSTR   getcopycntstg( PVIEW view )
{
   PLSTSTATS   pls = &g_sLstStats;
   DWORD       dwonview = 0;
//   sprintf( g_szListStg, "T=%d (L=%d,R=%d) Sm=%d New=%d Old=%d LO=%d RO=%d",
//      pls->dwtot, // the total
//      pls->dwleftcnt, pls->dwrightcnt,
//      pls->dwsame,   // same
//      pls->dwnewer,  // newer
//      pls->dwolder,  // older
//      pls->dwleft,   // only in left = ready to copy
//      pls->dwright );   // only in right = probably all DELETE - What about ZIP backup
   LPTSTR   lpb = GetStgBuf();
   INT      icnt = view_getrowcount(view);
   INT      itot = view_gettotcount(view);
//   g_bColourCycle = FALSE;
   if( outline_include & INCLUDE_SAME )
      dwonview += pls->dwsame;
   if( outline_include & INCLUDE_NEWER )
      dwonview += pls->dwnewer;
   if( outline_include & INCLUDE_OLDER )
      dwonview += pls->dwolder;
   if( outline_include & INCLUDE_LEFTONLY )
      dwonview += pls->dwleft;
   if( outline_include & INCLUDE_RIGHTONLY )
      dwonview += pls->dwright;

   if((DWORD)itot != pls->dwtot )
      chkme( "WARNING: Table and Global variable differ %d vs %d", itot, pls->dwtot );
   if((DWORD)icnt != dwonview )
      chkme( "WARNING: Table and Global variable differ %d vs %d", icnt, dwonview );

   if( icnt == itot )
   {
      if(icnt)
      {
         sprintf(lpb, "Copy l->r %d ...", icnt);
      }
      else
      {
         strcpy(lpb, "No COMPARE list");  // active! Use File / Compare ... to start.");
         // g_bColourCycle = TRUE;
      }
   }
   else
   {
      if( icnt )  // if we have VIEWABLE
      {
         //sprintf(lpb, "List %d of %d", icnt, itot);
         sprintf(lpb, "Update l->r %d of %d ...", icnt, itot);
      }
      else  // we have NOTHING to display with the PRESENT option settings
      {
         sprintf(lpb, "NONE per options ...");
         //g_bNeedPaint = TRUE;
      }
   }
   return lpb;
}


// eof - table.c

        
