
/****************************** Module Header *******************************
* Module Name: STATUS.C
*
* Status line handler.
*
* Functions:
*
* StatusInit()
* StatusCreate()
* StatusHeight()
* StatusAlloc()
* StatusAddItem()
* StatusCreateTools()
* StatusDeleteTools()
* StatusWndProc()
* StatusResize()
* StatusCalcHeight()
* StatusCalcWidth()
* StatusGetItem()
* LowerRect()
* RaiseRect()
* StatusPaint()
*  eg if (ip->type == SF_STATIC)
*        if (ip->flags & SF_RAISE)
*            RaiseRect(hDC, &ip->rc);
*        else if (ip->flags & SF_LOWER)
*            LowerRect(hDC, &ip->rc);
*        ...    
*        DrawText(hDC, ip->st_text, lstrlen(ip->st_text), &rc,
*
* BottomRight()
* TopLeft()
* StatusButtonDown()
* StatusButtonUp()
* InitDC()
*
* Comments:
* To set some text on a label in the status bar, use
*     case SM_SETTEXT:
*        strncpy(ip->st_text, (LPSTR) lParam, SF_MAXLABEL);
*
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"


/* --- data structures ------------------------------------------------- */
//#define SF_MAXLABEL     80   /* no more than 80 in an item within the bar */
                             /* Is this adequate for long pathnames on a
                                hi-res screen?
                             */

typedef struct statel {
        int type;                       /* SF_BUTTON or SF_STATIC */
        int flags;                      /* SF_VAR => variable width
                                           SF_LEFT=> left aligned (else right)
                                           SF_RAISE=> paint as 'raised' 3d rect
                                           SF_LOWER=> paint as lowered 3D rect
                                           SF_SZMIN=>together with SF_VAR
                                                     allows minimum size for
                                                     var sized item
                                           SF_SZMAX=>see SZMIN and use nouse
                                        */
        int id;                         /* control id */
        int width;                      /* width of control in chars */
        char st_text[SF_MAXLABEL+1];       /* null-term string for label */

        RECT rc;                        /* used by status.c */
} STATEL, FAR * PSTATEL;

typedef struct itemlist {
        int nitems;
        PSTATEL statels;

        int selitem;                    /* used by status.c */
        BOOL isselected;                /* used by status.c */
} ILIST, FAR * PILIST;

/* prototypes of routines in this module */

BOOL StatusCreateTools(void);
void StatusDeleteTools(void);
LONG_PTR APIENTRY StatusWndProc(HWND, UINT, WPARAM, LPARAM);
void StatusResize(HWND hWnd, PILIST pilist);
int StatusCalcHeight(HWND hWnd, PSTATEL ip);
int StatusCalcWidth(HWND hWnd, PSTATEL ip);
PSTATEL StatusGetItem(PILIST plist, WPARAM id);
void LowerRect(HDC hDC, LPRECT rcp);
void RaiseRect(HDC hDC, LPRECT rcp);
void StatusPaint(HWND hWnd, PILIST iplistp);
void BottomRight(HDC hDC, LPRECT rcp, HPEN hpen, BOOL bCorners);
void TopLeft(HDC hDC, LPRECT rcp, HPEN hpen, BOOL bCorners);
void StatusButtonDown(HDC hDC, PSTATEL ip);
void StatusButtonUp(HDC hDC, PSTATEL ip);
void InitDC(HDC hdc);


/*--global data---------------------------------------------------------*/

HPEN     hpenHilight   = 0;
HPEN     hpenLowlight  = 0;
HPEN     hpenBlack     = 0;
HPEN     hpenNeutral   = 0;
HBRUSH   hbrBackground = 0; /* pieces and board */
HFONT    hFont         = 0;

int status_charheight, status_charwidth;
POINT    g_ptStatus;

/* default pt size for font (tenths of a pt) */
#define         DEF_PTSIZE      80

/***************************************************************************
 * Function: StatusInit
 *
 * Purpose:
 *
 * Create window class
 */
BOOL
StatusInit(HANDLE hInstance)
{
   WNDCLASS    wc;
   BOOL        resp = FALSE;
   TEXTMETRIC  tm;
   HDC         hDC;


   if( !StatusCreateTools() )
      return FALSE;

   wc.style = CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
   wc.lpfnWndProc = (WNDPROC) StatusWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = sizeof(HANDLE);
   wc.hInstance = hInstance;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = hbrBackground;
   wc.lpszClassName = (LPSTR) "gdstatusclass";
   wc.lpszMenuName = NULL;

   resp = RegisterClass(&wc);
   if( resp )
   {
      hDC = GetDC(NULL);
      if(hDC)
      {
         InitDC(hDC);
         GetTextMetrics(hDC, &tm);
         status_charheight = (int)(tm.tmHeight + tm.tmExternalLeading);
         status_charwidth = (int)tm.tmAveCharWidth;
         ReleaseDC(NULL, hDC);
      }
      else
         resp = FALSE;
   }

   return resp;
}

/*
/***************************************************************************
 * Function: StatusCreate
 *
 * Purpose:
 *
 * Create and show the window
 */
HWND APIENTRY
StatusCreate(HANDLE hInst, HWND hParent, HMENU id, LPRECT rcp, HANDLE hmem)
{

        HWND hWnd;

        /* create a child window of status class */


        hWnd = CreateWindow("gdstatusclass",
                        NULL,
                        WS_CHILD | WS_VISIBLE,
                        rcp->left,
                        rcp->top,
                        (rcp->right - rcp->left),
                        (rcp->bottom - rcp->top),
                        hParent,
                        id,
                        hInst,
                        (LPVOID) hmem);

        return(hWnd);
}

/* **************************************************************************
 * Function: StatusHeight
 *
 * Purpose:
 *
 * Return default height of this window 
 *
 * The window has a number of items which are arranged horizontally,
 *  so the window height is the maximum of the individual heights
 */

int APIENTRY
StatusHeight(HANDLE hmem)
{
        PILIST plist;
        int i;
        int sz;
        int maxsize = 0;

        plist = (PILIST) GlobalLock(hmem);
        if (plist != NULL) {
                for (i = 0; i<plist->nitems; i++) {
                        sz = StatusCalcHeight(NULL, &plist->statels[i]);
                        maxsize = max(sz, maxsize);
                }
        }
        GlobalUnlock(hmem);
        if (maxsize > 0) {
                return(maxsize + 4);
        } else {
                return(status_charheight + 4);
        }
}

/***************************************************************************
 * Function: StatusAlloc
 *
 * Purpose:
 *
 * Allocate the plist struct and return handle to caller 
 *
 */
HANDLE FAR PASCAL
StatusAlloc(int nitems)
{
        HANDLE hmem;
        PILIST pilist;
        LPSTR chp;

        hmem = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                sizeof(ILIST) + (sizeof(STATEL) * nitems));
        chp = GlobalLock(hmem);
        if (chp == NULL) {
                return(NULL);
        }

        pilist = (PILIST) chp;
        pilist->nitems = nitems;
        pilist->statels = (PSTATEL) &chp[sizeof(ILIST)];
        GlobalUnlock(hmem);

        return(hmem);
}


/***************************************************************************
 * Function: StatusAddItem
 *
 * Purpose:
 *
 * Insert an item into the plist 
 */
BOOL FAR PASCAL
StatusAddItem(HANDLE hmem, int itemnr, int type, int flags, int id,
        int width, LPSTR text)
{
        PILIST pilist;
        PSTATEL pel;

        pilist = (PILIST) GlobalLock(hmem);
        if ((pilist == NULL) || (itemnr >= pilist->nitems)) {
                GlobalUnlock(hmem);
                return(FALSE);
        }
        pel = &pilist->statels[itemnr];
        pel->type = type;
        pel->flags = flags;
        pel->id = id;
        pel->width = width;
        if (text == NULL) {
                pel->st_text[0] = '\0';
        } else {
                lstrcpy(pel->st_text, text);
        }


        GlobalUnlock(hmem);
        return(TRUE);
}

/***************************************************************************
 * Function: InitDC
 *
 * Purpose:
 *
 * Initialize colors and font
 */ 
void
InitDC(HDC hdc)
{
        SetBkColor(hdc, RGB(192,192,192));
        SelectObject(hdc, hbrBackground);
        SelectObject(hdc, hFont);
}


/***************************************************************************
 * Function: StatusCreateTools
 *
 * Purpose:
 *
 * Create Pens and brushes
 */ 

LOGFONT     g_slfDefPts;

BOOL  StatusCreateTools()
{
   BOOL     bRet = FALSE;
   LOGFONT * plf = &g_slfDefPts;
   HDC      hdc;
   int      scale;

   hbrBackground = CreateSolidBrush(RGB(192,192,192));
   hpenHilight   = CreatePen(0, 1, RGB(255, 255, 255));
   hpenLowlight  = CreatePen(0, 1, RGB(128, 128, 128));
   hpenNeutral   = CreatePen(0, 1, RGB(192, 192, 192));
   hpenBlack     = CreatePen(0, 1, RGB(0, 0, 0));

   hdc = GetDC(NULL);
   if(hdc)
   {
      scale = GetDeviceCaps(hdc, LOGPIXELSY);
      ReleaseDC(NULL, hdc);

      plf->lfHeight = -MulDiv(DEF_PTSIZE, scale, 720);
      plf->lfWidth = 0;
      plf->lfEscapement = 0;
      plf->lfOrientation = 0;
      plf->lfWeight = FW_REGULAR;
      plf->lfItalic = 0;
      plf->lfUnderline = 0;
      plf->lfStrikeOut = 0;
      plf->lfCharSet = ANSI_CHARSET;
      plf->lfOutPrecision = OUT_DEFAULT_PRECIS;
      plf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
      plf->lfQuality = PROOF_QUALITY;
      plf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
      plf->lfFaceName[0] = '\0';
#ifdef COMPLEX
      hFont = CreateFontIndirect(plf);
#else
      hFont = GetStockObject(SYSTEM_FONT);
#endif
      if( hbrBackground && hpenHilight && hpenLowlight &&
          hpenNeutral && hpenBlack && hFont )
      {
         bRet = TRUE;
      }
   }

   if( !bRet )
      StatusDeleteTools();

   return bRet;

}

/***************************************************************************
 * Function: StatusDeleteTools
 *
 * Purpose:
 *
 * Delete brushes and pens
 */
void
StatusDeleteTools()
{
   if(hbrBackground)
      DeleteObject(hbrBackground);
   if(hpenHilight)
      DeleteObject(hpenHilight);
   if(hpenLowlight)
      DeleteObject(hpenLowlight);
   if(hpenBlack)
      DeleteObject(hpenBlack);
   if(hpenNeutral)
      DeleteObject(hpenNeutral);
   hpenHilight   = 0;
   hpenLowlight  = 0;
   hpenBlack     = 0;
   hpenNeutral   = 0;
   hbrBackground = 0; /* pieces and board */

#ifdef COMPLEX
   if(hFont)
      DeleteObject(hFont);
   hFont = 0;
#endif

}

VOID  status_WM_CREATE( HWND hWnd, LPARAM lParam )
{
   CREATESTRUCT * cp;
   HANDLE hitems; // = (HANDLE) GetWindowLongPtr(hWnd, 0);
   PILIST plist;

   cp = (CREATESTRUCT FAR *) lParam;
   hitems = (HANDLE)cp->lpCreateParams;
   plist = (PILIST) GlobalLock(hitems);
   if(plist != NULL)
   {
      SetWindowLongPtr(hWnd, 0,  (LONG_PTR)hitems);
      plist->selitem = -1;
      GlobalUnlock(hitems);
   }
}

VOID  status_WM_SIZE( HWND hWnd, LPARAM lParam, HANDLE hitems )
{
   if( hitems )
   {
      PILIST plist = (PILIST) GlobalLock(hitems);
      if( plist != NULL )
      {
         StatusResize(hWnd, plist);
         GlobalUnlock(hitems);
      }
   }
}

VOID  status_WM_PAINT( HWND hWnd, LPARAM lParam, HANDLE hitems )
{
   if(hitems)
   {
      PILIST plist = (PILIST) GlobalLock(hitems);
      if(plist)
      {
         StatusPaint(hWnd, plist);
         GlobalUnlock(hitems);
      }
   }
}

extern   VOID  Do_WM_MOUSEMOVE( HWND hWnd, WPARAM wParam, DWORD dx, DWORD dy,
                               DWORD src );  // mouse on parent

POINT g_ptS2S, g_ptS2C;
BOOL  g_ptIsOk = FALSE;
VOID  status_WM_MOUSEMOVE( HWND hWnd, WPARAM wParam, LPARAM lParam, HANDLE hitems )
{
   g_ptStatus.x = LOWORD(lParam);
   g_ptStatus.y = HIWORD(lParam);
   g_ptS2S = g_ptStatus;
   g_ptIsOk = ClientToScreen( hWnd,       // handle to window
      &g_ptS2S ); // LPPOINT lpPoint  // screen coordinates
   if( g_ptIsOk )
   {
      g_ptS2C = g_ptS2S;
      if( ScreenToClient( hwndClient, &g_ptS2C ) )
         Do_WM_MOUSEMOVE( hwndClient, wParam, g_ptS2C.x, g_ptS2C.y, SRC_STATUS );

   }
   if(hitems)
   {
      HDC      hDC;
      PSTATEL  ip;
      PILIST   plist = (PILIST) GlobalLock(hitems);

      if (plist)
      {
//         g_ptStatus.x = LOWORD(lParam);
//         g_ptStatus.y = HIWORD(lParam);
         if (plist->selitem != -1)
         {
                ip = &plist->statels[plist->selitem];
                if (PtInRect(&ip->rc, g_ptStatus))
                {
                        if (!plist->isselected)
                        {
                                hDC = GetDC(hWnd);
                                InitDC(hDC);
                                StatusButtonDown(hDC, ip);
                                ReleaseDC(hWnd, hDC);
                                plist->isselected = TRUE;
                        }
                } else {
                        if(plist->isselected) {
                                hDC = GetDC(hWnd);
                                InitDC(hDC);
                                StatusButtonUp(hDC, ip);
                                ReleaseDC(hWnd, hDC);
                                plist->isselected = FALSE;
                        }
                }
         }

         GlobalUnlock(hitems);
 
      }
   }
}

/***************************************************************************
 * Function: StatusWndProc
 *
 * Purpose:
 *
 * Main winproc for status windows
 *
 * handles create/destroy and paint requests
 */

LONG_PTR FAR PASCAL StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HANDLE hitems = (HANDLE) GetWindowLongPtr(hWnd, 0);
    PSTATEL ip;
    PILIST plist;
//    CREATESTRUCT FAR * cp;
    int i;
    HDC hDC;
    RECT rc;
//    POINT pt;

    switch(message)
    {

    case WM_CREATE:
       status_WM_CREATE( hWnd, lParam );
       break;

    case WM_SIZE:
       status_WM_SIZE( hWnd, lParam, hitems );
       break;

    case WM_PAINT:
       status_WM_PAINT( hWnd, lParam, hitems );
       break;

    case WM_LBUTTONUP:
       if( hitems )
       {
          plist = (PILIST) GlobalLock(hitems);
          g_ptStatus.x = LOWORD(lParam);
          g_ptStatus.y = HIWORD(lParam);
          if (plist == NULL)
          {
                break;
          }
          if( plist->selitem != -1 )
          {
                ip = &plist->statels[plist->selitem];
                if (plist->isselected)
                {
                        hDC = GetDC(hWnd);
                        InitDC(hDC);
                        StatusButtonUp(hDC, ip);
                        ReleaseDC(hWnd, hDC);
                }
                plist->selitem = -1;
                ReleaseCapture();
                if (PtInRect(&ip->rc, g_ptStatus)) {
                    SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(ip->id, WM_LBUTTONUP), (LPARAM)hWnd);
                }
          }
          GlobalUnlock(hitems);
       }
       break;

    case WM_LBUTTONDOWN:
       if(hitems)
       {
          plist = (PILIST) GlobalLock(hitems);
          if (plist == NULL)
             break;
          g_ptStatus.x = LOWORD(lParam);
          g_ptStatus.y = HIWORD(lParam);
          if( plist->selitem == -1 )
          {
                for (i = 0; i< plist->nitems; i++)
                {
                        ip = &plist->statels[i];
                        if (PtInRect(&ip->rc, g_ptStatus))
                        {
                                if (ip->type != SF_BUTTON)
                                {
                                        break;
                                }
                                plist->selitem = i;
                                SetCapture(hWnd);

                                plist->isselected = TRUE;
                                hDC = GetDC(hWnd);
                                InitDC(hDC);
                                StatusButtonDown(hDC, ip);
                                ReleaseDC(hWnd, hDC);
                                break;
                        }
                }
          }
          GlobalUnlock(hitems);
       }
       break;

    case WM_MOUSEMOVE:
       status_WM_MOUSEMOVE( hWnd, wParam, lParam, hitems );
       break;


    case WM_DESTROY:
       if(hitems)
       {
          GlobalUnlock(hitems);
          GlobalFree(hitems);
       }
       SetWindowLongPtr(hWnd, 0, (LONG_PTR)0);
       break;

    case SM_NEW:
       if(hitems)
       {
          GlobalUnlock(hitems);
          GlobalFree(hitems);
       }
       hitems = (HANDLE) wParam;
       if( hitems == NULL )
       {
                SetWindowLongPtr(hWnd, 0, 0L);
                InvalidateRect(hWnd, NULL, TRUE);
                break;
       }
       plist = (PILIST) GlobalLock(hitems);
       if(plist == NULL)
       {
          SetWindowLongPtr(hWnd, 0, 0L);
          InvalidateRect(hWnd, NULL, TRUE);
          break;
       }

       plist->selitem = -1;
       StatusResize(hWnd, plist);
       GlobalUnlock(hitems);
       SetWindowLongPtr(hWnd, 0, (LONG_PTR)hitems);
       InvalidateRect(hWnd, NULL, TRUE);
       break;

    case SM_SETTEXT:
       if(hitems)
       {
          plist = (PILIST) GlobalLock(hitems);
          if(plist)
          {
             ip = StatusGetItem(plist, wParam);
             if(ip != NULL)
             {
                if (lParam == 0)
                {
                   ip->st_text[0] = '\0';
                }
                else
                {
                   strncpy(ip->st_text, (LPSTR) lParam, SF_MAXLABEL);
                   ip->st_text[SF_MAXLABEL] = '\0';
                }

                /* if this is a variable width field, we need to redo
                 * all size calcs in case the field width has changed.
                 * in that case, we need to repaint the entire window
                 * and not just this field - so set rc to indicate the
                 * area to be redrawn.
                 */
                if (ip->flags & SF_VAR)
                {
                        StatusResize(hWnd, plist);
                        GetClientRect(hWnd, &rc);
                        RedrawWindow(hWnd, &rc, NULL,
                                RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW);
                }
                else
                {
                        /* instead of just invalidating the window, we can
                         * force the window to be repainted now. This is
                         * essential for status updates during a busy
                         * loop when no messages are being processed,
                         * but we should still update the user on what's
                         * happening.
                         */
                        RedrawWindow(hWnd, &ip->rc, NULL,
                                RDW_INVALIDATE|RDW_NOERASE|RDW_UPDATENOW);
                }

        
             }
             GlobalUnlock(hitems);
          }
       }
       break;

    default:
       return(DefWindowProc(hWnd, message, wParam, lParam));

    }
    return 0;
}

/***************************************************************************
 * Function: StatusResize
 *
 * Purpose:
 *
 * Position the labels and buttons within the status window 
 */
void
StatusResize(HWND hWnd, PILIST iplistp)
{
        RECT rc;
        int curpos_right, curpos_left;
        int height, width;
        int i;
        PSTATEL ip;


        if (iplistp == NULL) {
                return;
        }
        GetClientRect(hWnd, &rc);
        curpos_left = rc.left + status_charwidth / 2;
        curpos_right = rc.right - (status_charwidth / 2);

        /* loop through all items setting their position rects.
         * items are flagged as being left or right. We place them
         * in order starting at the left and the right, with a single
         * char's width between each item
         */
        for (i = 0; i < iplistp->nitems; i++)
        {
                ip = &iplistp->statels[i];

                width = StatusCalcWidth(hWnd, ip);
                height = StatusCalcHeight(hWnd, ip);
                ip->rc.top = (rc.bottom - height) / 2;
                ip->rc.bottom = ip->rc.top + height;

                /* see if  this item fits. Items that partially fit
                 * are placed reduced in size.
                 */
                if (ip->flags & SF_LEFT)
                {

                        if (curpos_left+width >= curpos_right)
                        {
                                /* doesn't completely fit-does it partly? */
                                if ((curpos_left + 1) >= curpos_right)
                                {

                                        /* no - this item does not fit */
                                        ip->rc.left = 0;
                                        ip->rc.right = 0;
                                } else {
                                        /* partial fit */
                                        ip->rc.left = curpos_left;
                                        ip->rc.right = curpos_right - 1;
                                        curpos_left = curpos_right;
                                }
                        } else {
                                /* complete fit */
                                ip->rc.left = curpos_left;
                                ip->rc.right = curpos_left + width;
                                curpos_left += width + 1;
                        }
                } else {

                        /* same size check for right-aligned items */
                        if (curpos_right-width <= curpos_left) {

                                /* partial fit ? */
                                if (curpos_right <= curpos_left+1) {
                                        ip->rc.left = 0;
                                        ip->rc.right = 0;
                                } else {
                                        /* yes - partial fit */
                                        ip->rc.left = curpos_left + 1;
                                        ip->rc.right = curpos_right;
                                        curpos_right = curpos_left;
                                }
                        } else {
                                /* complete fit */
                                ip->rc.right = curpos_right;
                                ip->rc.left = curpos_right - width;
                                curpos_right -= (width + 1);
                        }
                }
        }
}


/***************************************************************************
 * Function: StatusPaint
 *
 * Purpose:
 *
 * Paint the status window
 */
void
StatusPaint(HWND hWnd, PILIST iplistp)
{
        RECT rc;
        HDC hDC;
        PAINTSTRUCT ps;
        int i;
        PSTATEL ip;
        HPEN hpenOld;

        GetClientRect(hWnd, &rc);
        hDC = BeginPaint(hWnd, &ps);
        InitDC(hDC);

        RaiseRect(hDC, &rc);
        if (iplistp == NULL)
        {
                EndPaint(hWnd, &ps);
                return;
        }
        for (i =0; i < iplistp->nitems; i++)
        {
                ip = &iplistp->statels[i];

                if (ip->rc.left == ip->rc.right)
                {
                        continue;
                }
                if (ip->type == SF_STATIC)
                {
                        if (ip->flags & SF_RAISE)
                        {
                                RaiseRect(hDC, &ip->rc);
                        }
                        else if (ip->flags & SF_LOWER)
                        {
                                LowerRect(hDC, &ip->rc);
                        }
                        rc = ip->rc;
                        rc.left += (status_charwidth / 2);
                        rc.right--;
                        rc.top++;
                        rc.bottom--;
                        hpenOld = SelectObject(hDC, hpenNeutral);
                        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
                        SelectObject(hDC, hpenOld);
                        DrawText(hDC, ip->st_text, lstrlen(ip->st_text), &rc,
                                        DT_LEFT | DT_VCENTER);
                }
                else
                {
                        StatusButtonUp(hDC, ip);
                }
        }

        EndPaint(hWnd, &ps);
}

/***************************************************************************
 * Function: RaiseRect
 *
 * Purpose:
 *
 */
void
RaiseRect(HDC hDC, LPRECT rcp)
{
        TopLeft(hDC, rcp, hpenHilight, FALSE);
        BottomRight(hDC, rcp, hpenLowlight, FALSE);
}

/***************************************************************************
 * Function: LowerRect
 *
 * Purpose:
 *
 */ 
void
LowerRect(HDC hDC, LPRECT rcp)
{
        TopLeft(hDC, rcp, hpenLowlight, FALSE);
        BottomRight(hDC, rcp, hpenHilight, FALSE);
}

/***************************************************************************
 * Function: StatusButtonUp
 *
 * Purpose:
 *
 */
void
StatusButtonUp(HDC hDC, PSTATEL ip)
{
        RECT rc;
        HPEN hpenOld;

        rc = ip->rc;
        TopLeft(hDC, &rc, hpenBlack, TRUE);
        BottomRight(hDC, &rc, hpenBlack, FALSE);

        rc.top++;
        rc.bottom--;
        rc.left++;
        rc.right--;
        TopLeft(hDC, &rc, hpenHilight, FALSE);
        BottomRight(hDC, &rc, hpenLowlight, TRUE);

        rc.top++;
        rc.bottom--;
        rc.left++;
        rc.right--;
        BottomRight(hDC, &rc, hpenLowlight, TRUE);
        rc.bottom--;
        rc.right--;
        hpenOld = SelectObject(hDC, hpenNeutral);
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hDC, hpenOld);
        DrawText(hDC, ip->st_text, lstrlen(ip->st_text), &rc, DT_CENTER | DT_VCENTER);
}

/***************************************************************************
 * Function: StatusButtonDown
 *
 * Purpose:
 *
 */
void
StatusButtonDown(HDC hDC, PSTATEL ip)
{
        RECT rc;
        HPEN hpenOld;

        rc = ip->rc;
        TopLeft(hDC, &rc, hpenBlack, TRUE);
        BottomRight(hDC, &rc, hpenBlack, FALSE);

        rc.top++;
        rc.bottom--;
        rc.left++;
        rc.right--;
        TopLeft(hDC, &rc, hpenLowlight, TRUE);
        rc.top++;
        rc.left++;
        TopLeft(hDC, &rc, hpenNeutral, TRUE);
        rc.top++;
        rc.left++;
        TopLeft(hDC, &rc, hpenNeutral, TRUE);
        rc.top++;
        rc.left++;
        hpenOld = SelectObject(hDC, hpenNeutral);
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hDC, hpenOld);
        DrawText(hDC, ip->st_text, lstrlen(ip->st_text), &rc, DT_CENTER | DT_VCENTER);
}

/***************************************************************************
 * Function: TopLeft
 *
 * Purpose:
 *
 */
void
TopLeft(HDC hDC, LPRECT rcp, HPEN hpen, BOOL bCorners)
{
        HPEN hpenOld;
        int x, y;

        hpenOld = SelectObject(hDC, hpen);
        x = rcp->right - 1;
        y = rcp->bottom;
        if (!bCorners) {
                x--;
                y--;
        }
        MoveToEx(hDC, x, rcp->top, NULL);
        LineTo(hDC, rcp->left, rcp->top);
        LineTo(hDC, rcp->left, y);
        SelectObject(hDC, hpenOld);
}

/***************************************************************************
 * Function: BottomRight
 *
 * Purpose:
 *
 */
void
BottomRight(HDC hDC, LPRECT rcp, HPEN hpen, BOOL bCorners)
{
        HPEN hpenOld;
        int x, y;

        hpenOld = SelectObject(hDC, hpen);
        x = rcp->left - 1;
        y = rcp->top;
        if (!bCorners) {
                x++;
                y++;
        }
        MoveToEx(hDC, rcp->right-1, y, NULL);
        LineTo(hDC, rcp->right-1, rcp->bottom-1);
        LineTo(hDC, x, rcp->bottom-1);
        SelectObject(hDC, hpenOld);
}

/***************************************************************************
 * Function: StatusGetItem
 *
 * Purpose:
 *
 */
PSTATEL StatusGetItem(PILIST plist, WPARAM id)
{
        int i;

        if (plist == NULL)
        {
                return(NULL);
        }
        for (i = 0; i < plist->nitems; i++)
        {
                if (plist->statels[i].id == id)
                {
                        return(&plist->statels[i]);
                }
        }
        return(NULL);
}

/***************************************************************************
 * Function: StatusCalcWidth
 *
 * Purpose:
 *
 * Calculate the width of a given field. This is the width in characters
 * multiplied by the average character width, plus a few units for
 * borders.
 *
 * If SF_VAR is set, this field size varies depending on the text, so
 * we use GetTextExtent for the field size. If SF_VAR is selected, the caller
 * can specify that the size is not to exceed the (width * avecharwidth)
 * size (using SF_SZMAX) or that it is not be less than it (SF_SZMIN).
 */
int
StatusCalcWidth(HWND hWnd, PSTATEL ip)
{
        int ch_size, t_size;
        SIZE sz;
        HDC hDC;

        ch_size = ip->width * status_charwidth;
        if (ip->flags & SF_VAR)
        {
                hDC = GetDC(hWnd);
                InitDC(hDC);
                GetTextExtentPoint(hDC, ip->st_text, lstrlen(ip->st_text), &sz);
                ReleaseDC(hWnd, hDC);
                t_size = sz.cx;

                /*
                 * check this size against min/max size if
                 * requested
                 */

                if (ip->flags & SF_SZMIN)
                {
                        if (ch_size > t_size)
                        {
                                t_size = ch_size;
                        }
                }
                if (ip->flags & SF_SZMAX)
                {
                        if (ch_size < t_size)
                        {
                                t_size = ch_size;
                        }
                }
                ch_size = t_size;
        }

        if (ch_size != 0)
        {
                if (ip->type == SF_BUTTON)
                {
                        return(ch_size+6);
                }
                else
                {
                        return(ch_size+4);
                }
        }
        else
        {
                return(0);
        }
}

/***************************************************************************
 * Function: StatusCalcHeight
 *
 * Purpose:
 *
 * Calculate the height of a given field
 */
int
StatusCalcHeight(HWND hWnd, PSTATEL ip)
{
        int size;

        size = status_charheight;
        if (ip->type == SF_BUTTON)
        {
                return(size + 6);
        }
        else
        {
                return(size + 2);
        }
}

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - status.c
