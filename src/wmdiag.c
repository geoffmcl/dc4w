
// ====================================================================
// Copyright (c) 1997-2001  Geoff R. McLane
//
// Module Name:
//		WMDiag.c
//
// Abstract:
//		This is for DIAGNOSTIC windows message information
//		ONLY and should be EXCLUDED from a RELEASE.
//    Just to convert WINDOW MESSAGES hopefully into a meaningful string
//    This file is EQUIVALENT to D:\GTools\Samples\NetPSec\NPSG\wmdiag.cpp
//    This set was redone from July 2000 MS SDK Windows.h header
//    IT seems to continue to GROW ...
//
// Environment:
//		Application mode in Windows 9* ++
//		Application mode in Windows NT 4.0++
//
// History:
//		September, 1997 - Created.
//		September, 1998 - Full interface enabled my DIAGWM flag
//		November,  1999 - Added NAME registration to replace
//			HEX (H=0xcee5) with a USER name, like Edit, Graf, ...
//    Sep 30,    2000 - ADDED the "CB_..." set of messages
//    February,  2001 - Made eqivalence between various version,
//       and add some addtional H_Handlers.
//    Feb 10,    2001 - ADDED the Edit Control Messages - 0x00B0 to 0x00D9 from
//                         Jan 2001 MSDN
//    July 10,   2001 - Merged with EWM project, which has the additional
//                      List Control Notify (GetLVNStg())
//    AND               Added Combo Box WM_COMMAND notification messages CBN_???
//
// Author:
//		Geoff R. McLane (Email: geoffmclane *AT* hotmail *DOT* Com
//
// ====================================================================
// this is public domain software - praise me, if ok, just don't blame me!
#include	<windows.h>
#include <stdio.h>
#include <richedit.h>
// #pragma warning( disable : 4201 )  // Disable UNION warning message in commctrl.h
#include <commctrl.h>      // toolbar messages

#include "wmdiag.h"
#undef  NOWMDIAG

BOOL  gbNoDOut = FALSE;    // be able to INHIBIT the output
BOOL  gbShwAll = FALSE;    // but can be over-ridden with this!

//#ifndef   NDEBUG
#if   !(defined (NDEBUG) || defined (NOWMDIAG))
// ==========================================================
// ==========================================================
#define  lEndBuf(a)     ( a + strlen(a) )

// *********************************************************************
// HEAVY Window diagnostic stuff
// =============================
typedef void (*MGHND) (UINT, LPTSTR, WPARAM, LPARAM);

typedef struct {
	UINT		w_uiMsg;
	LPTSTR	w_lpStg;
   MGHND    w_pMgHnd;
}WMSTR, * PWMSTR;

// return a string corresponding to the UINT
// If NO match, return 'UNKNOWN 0x?????' ...
TCHAR szUnk[] = TEXT("UNKNOWN %#x");
TCHAR szUsr[] = TEXT("WM_USER + %d");

LPTSTR   GetEDPtr( WPARAM wParam );
LPTSTR   GetRENPtr( UINT uMsg );
LPTSTR   GetSCPtr( WPARAM uCmd );

C2S   C2s = 0;

VOID  SetC2s( C2S pc )
{
   C2s = pc;
}

//SendMessage( 
//  (HWND) hWnd,              // handle to destination window 
//  CB_FINDSTRING,            // message to send
//  (WPARAM) wParam,          // index of item preceding start
//  (LPARAM) lParam );        // search string (LPCSTR)
//void   HCB_FINDSTRING( LPTSTR lps, WPARAM wParam, LPARAM lParam )
void   H_ADDSTRING( LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lpf = (LPTSTR)lParam;
   DWORD    dwi;
   if( lpf )
   {
      dwi = strlen(lpf);
      if(dwi)
      {
         strcat(lps,"[");
         if( dwi < 256 )
         {
            strcat(lps,lpf);
         }
         else
         {
            // what to do
            sprintf(lEndBuf(lps),"%d bytes",dwi);
         }
         strcat(lps,"]");
      }
      else
      {
         strcat(lps,"[<nul>]");
      }
   }
   else
   {
      strcat(lps,"[<nul>]");
   }

   UNREFERENCED_PARAMETER(wParam);

}

// The low-order word of lParam specifies the new width of the client area. 
// The high-order word of lParam specifies the new height of the client area. 
void   H_ADDSIZE( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   sprintf(lEndBuf(lps), "[w=%d h=%d]",
      (LOWORD(lParam) & 0xffff),
      (HIWORD(lParam) & 0xffff) );
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(ut);
}
void   H_ADDMOVE( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   sprintf(lEndBuf(lps), "[x=%d y=%d]",
      (LOWORD(lParam) & 0xffff),
      (HIWORD(lParam) & 0xffff) );
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(ut);
}
void   H_ADDCHAR( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   sprintf(lEndBuf(lps), "[0x%llx]", wParam );
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(ut);
}

//wParam 
//The low-order word of wParam specifies the event for which the
//parent is being notified. This parameter can be one of the
//following values. Value Meaning 
//WM_CREATE The child window is being created. 
//WM_DESTROY The child window is being destroyed. 
//WM_LBUTTONDOWN The user has placed the cursor over the child
//window and has clicked the left mouse button. 
//WM_MBUTTONDOWN The user has placed the cursor over the child
//window and has clicked the middle mouse button. 
//WM_RBUTTONDOWN The user has placed the cursor over the child
//window and has clicked the right mouse button. 
//WM_XBUTTONDOWN Windows 2000: The user has placed the cursor over
//the child window and has clicked the first or second X button. 
//The meaning of the high-order word of wParam depends on the
//value of the low-order word of wParam, as shown in the following
//table. LOWORD(wParam) Meaning of HIWORD(wParam) 
//WM_CREATE Identifier of the child window. 
//WM_DESTROY Identifier of the child window. 
//WM_LBUTTONDOWN Undefined. 
//WM_MBUTTONDOWN Undefined. 
//WM_RBUTTONDOWN Undefined. 
//WM_XBUTTONDOWN Windows 2000: Indicates which button was pressed.
//This parameter can be one of the following values: 
//XBUTTON1
//XBUTTON2
//lParam 
//The meaning of lParam depends on the value of the low-order word
//of wParam, as shown in the following table. LOWORD(wParam)
//Meaning of lParam 
//WM_CREATE Handle of the child window. 
//WM_DESTROY Handle of the child window. 
//WM_LBUTTONDOWN The x-coordinate of the cursor is the low-order
//word, and the y-coordinate of the cursor is the high-order word.
//WM_MBUTTONDOWN The x-coordinate of the cursor is the low-order
//word, and the y-coordinate of the cursor is the high-order word.
//WM_RBUTTONDOWN The x-coordinate of the cursor is the low-order
//word, and the y-coordinate of the cursor is the high-order word.
//WM_XBUTTONDOWN The x-coordinate of the cursor is the low-order
//word, and the y-coordinate of the cursor is the high-order word.
void   H_ADDNOTE( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   WORD  lwcmd = LOWORD(wParam);
   switch(lwcmd)
   {
   case WM_CREATE:   // The child window is being created.
      strcat(lps,"[CREATED]");
      break;
   case WM_DESTROY:  // The child window is being destroyed. 
      strcat(lps,"[DESTROY]");
      break;
   case WM_LBUTTONDOWN: // The user has placed the cursor over the child
//window and has clicked the left mouse button. 
      strcat(lps,"[LBDWN]");
      break;
   case WM_MBUTTONDOWN: // The user has placed the cursor over the child
//window and has clicked the middle mouse button. 
      strcat(lps,"[MBDWN]");
      break;

   case WM_RBUTTONDOWN: // The user has placed the cursor over the child
//window and has clicked the right mouse button. 
      strcat(lps,"[RBDWN]");
      break;

#if(WINVER >= 0x0500)
   //case WM_XBUTTONDOWN: // Windows 2000: The user has placed the cursor over
   case 0x020B:
//the child window and has clicked the first or second X button. 
      strcat(lps,"[XBDWN]");
      break;
#endif   // if(WINVER >= 0x0500)

   default:
      sprintf(lEndBuf(lps),"[UNK%#x]", lwcmd);
      break;
   }
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(ut);
}

void  H_NOTIFY( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   NMHDR * pnh = (NMHDR *)lParam;
   LPTSTR   lpp = 0;
   if( pnh )
   {
#ifdef   _RICHEDIT_
      lpp = GetRENPtr( pnh->code );
      if( lpp )
      {
         sprintf(lEndBuf(lps), "[h=%p id=%lld c=%s(%d)]",
            pnh->hwndFrom,
            pnh->idFrom,
            lpp,
            pnh->code );
      }
      else
#endif   // _RICHEDIT_
      {
         lpp = GetLVNStg( 0, pnh->code, wParam, lParam );
         sprintf(lEndBuf(lps), "[h=%p id=%lld c=%d",
            pnh->hwndFrom,
            pnh->idFrom,
            pnh->code );
         if(*lpp == '<')
            strcat(lps,"]");
         else
            sprintf(lEndBuf(lps), "(%s)]", lpp);

      }
   }
   else
   {
      strcat(lps,"<nul!>");
   }
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(ut);
}

// Parameters
//wParam 
// The high-order word specifies the notification code if the message is from a control.
// If the message is from an accelerator, this value is 1.
// If the message is from a menu, this value is zero. 
// The low-order word specifies the identifier of the menu item, control, or accelerator. 
// lParam 
// Handle to the control sending the message if the message is from a control.
// Otherwise, this parameter is NULL. 
void  H_COMMAND( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   DWORD  dwCode, dwID;
   LPTSTR   lpp;
   dwCode = HIWORD(wParam);
   dwID   = LOWORD(wParam);
   lpp = GetEDPtr(wParam);
   if(lpp)
   {
      sprintf(lEndBuf(lps), "[h=%#llx id=%d c=%s(%d)]",
         lParam,
         dwID,
         lpp,
         dwCode );
   }
   else
   {
      LPTSTR   lpc = 0;
      if( C2s )
         lpc = C2s( dwID );
      if( lpc )
      {
         sprintf(lEndBuf(lps), "[h=%#llx id=%s(%d) c=%d]",
            lParam,
            lpc,
            dwID,
            dwCode );
      }
      else
      {
         sprintf(lEndBuf(lps), "[h=%#llx id=%d c=%d]",
            lParam,
            dwID,
            dwCode );
      }
   }
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(ut);
}

//wParam 
//Specifies the type of system command requested.
// This parameter can be one of the following values. Value Meaning
WMSTR    sSysCmd[] = {
   { SC_SIZE , "SC_SIZE", 0 },  // 0xF000 = Sizes the window. 
   { SC_MOVE , "SC_MOVE", 0 },  // 0xF010 = Moves the window. 
   { SC_MINIMIZE, "SC_MINIMIZE", 0 },  // 0xF020 = Minimizes the window.
   { SC_MAXIMIZE, "SC_MAXIMIZE", 0 },  // 0xF030 = Maximizes the window.
   { SC_NEXTWINDOW , "SC_NEXTWINDOW", 0 }, // 0xF040 = Moves to the next window. 
   { SC_PREVWINDOW , "SC_PREVWINDOW", 0 }, // 0xF050 = Moves to the previous window. 
   { SC_CLOSE, "SC_CLOSE", 0 },  // 0xF060 = Closes the window. 
   { SC_VSCROLL , "SC_VSCROLL", 0 },  // 0xF070 = Scrolls vertically. 
   { SC_HSCROLL, "SC_HSCROLL", 0 },   // 0xF080 = Scrolls horizontally. 
   { SC_MOUSEMENU , "SC_MOUSEMENU", 0 }, // 0xF090 = Retrieves the window menu as a result of a mouse click. 
   { SC_KEYMENU, "SC_KEYMENU", 0 }, // 0xF100 = Retrieves the window menu as a result of a keystroke. For more information, see the Remarks section. 
   { SC_ARRANGE, "SC_ARRANGE", 0 },       // 0xF110
   { SC_RESTORE , "SC_RESTORE", 0 },  // 0xF120 = Restores the window to its normal position and size. 
   { SC_TASKLIST , "SC_TASKLIST", 0 },   // 0xF130 = Activates the Start menu. 
   { SC_SCREENSAVE , "SC_SCREENSAVE", 0 },  // 0xF140 = Executes the screen saver application specified in the [boot] section of the System.ini file. 
   { SC_HOTKEY, "SC_HOTKEY", 0 },   // 0xF150 = Activates the window associated with the application-specified hot key. The lParam parameter identifies the window to activate. 
// #if(WINVER >= 0x0400)
   { SC_DEFAULT, "SC_DEFAULT", 0 }, // 0xF160 = Selects the default item; the user double-clicked the window menu. 
   { SC_MONITORPOWER, "SC_MONITORPOWER", 0 },   // 0xF170 = Sets the state of the display. This command supports devices that have power-saving features, such as a battery-powered personal computer. 
   // The lParam parameter can have the following values:
   // 1 - the display is going to low power
   // 2 - the display is being shut off
   { SC_CONTEXTHELP, "SC_CONTEXTHELP", 0 },  // 0xF180 = Changes the cursor to a question mark with a pointer. If the user then clicks a control in the dialog box, the control receives a WM_HELP message. 
   { SC_SEPARATOR, "SC_SEPARATOR", 0 },   // 0xF00F
// #endif /* WINVER >= 0x0400 */
   { 0x0F095, "SC_NOTLISTED?", 0  },
   { 0,           0,             0 }	// end of SYSCOMMAND list
};



////////////////////////////////////////////////////////
// FUNCTION   :  GetSCPtr
// Description: 
// Return type: LPTSTR  
// Argument   : WPARAM wParam
////////////////////////////////////////////////////////
LPTSTR   GetSCPtr( WPARAM wParam )
{
   LPTSTR   lps;
   PWMSTR   lpm = &sSysCmd[0];
   UINT     ui  = (UINT)wParam;   // get command message
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == ui )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
   return lps;
}

////////////////////////////////////////////////////////
// Function name	: H_SYSCOMMAND
// Description	    : 
// Return type		: void  
// Argument         :  UINT ut
// Argument         : LPTSTR lps
// Argument         : WPARAM wParam
// Argument         : LPARAM lParam
////////////////////////////////////////////////////////
void  H_SYSCOMMAND( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   DWORD  dwCode, dwID;
   LPTSTR   lpp;
   dwCode = HIWORD(wParam);
   dwID   = LOWORD(wParam);
   lpp = GetSCPtr( wParam );
   if(lpp)
      sprintf( lEndBuf(lps), "[%s]", lpp );
   else
      sprintf( lEndBuf(lps), "[%llX?]", wParam );
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(ut);
}


// wParam 
// The low-order word specifies a scroll bar value that indicates the user's scrolling request.
// This word can be one of the following values.
// Value Meaning 
WMSTR sSBMsg[] = {
   { SB_ENDSCROLL, "SB_ENDSCROLL", 0 }, //Ends scroll. 
   { SB_LEFT,  "SB_LEFT ", 0 },  //Scrolls to the upper left. 
   { SB_RIGHT, "SB_RIGHT ", 0 },   //Scrolls to the lower right. 
   { SB_LINELEFT, "SB_LINELEFT", 0 },   //Scrolls left by one unit. 
   { SB_LINERIGHT, "SB_LINERIGHT", 0 }, //Scrolls right by one unit. 
   { SB_PAGELEFT, "SB_PAGELEFT", 0 },  // Scrolls left by the width of the window. 
   { SB_PAGERIGHT, "SB_PAGERIGHT", 0 },   // Scrolls right by the width of the window. 
   { SB_THUMBPOSITION, "SB_THUMBPOSITION", 0 }, // The user has dragged the scroll box (thumb) and released the mouse button. The high-order word indicates the position of the scroll box at the end of the drag operation. 
   { SB_THUMBTRACK, "SB_THUMBTRACK", 0 }, // The user is dragging the scroll box. This message is sent repeatedly until the user releases the mouse button. The high-order word indicates the position that the scroll box has been dragged to. 
   { 0,             0,               0 }
};



////////////////////////////////////////////////////////
// FUNCTION   :  GetScrollPtr
// Description: Return a pointer to string description,
//              (if available)
// Return type: LPTSTR  
// Argument   : WPARAM wParam
////////////////////////////////////////////////////////
LPTSTR   GetScrollPtr( WPARAM wParam )
{
   LPTSTR   lps;
   PWMSTR   lpm = &sSBMsg[0];
   UINT     ui  = LOWORD(wParam);   // get request message
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == ui )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
   return lps;
}


////////////////////////////////////////////////////////
// FUNCTION   : H_SCROLL
// Description: Return more information about messages
//              WM_HSCROLL and WM_VSCROLL
// Return type: void  
// Arguments  :  UINT ut
//            : LPTSTR lps
//            : WPARAM wParam
//            : LPARAM lParam
////////////////////////////////////////////////////////
void  H_SCROLL( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   DWORD  dwCode, dwID;
   LPTSTR   lpp;
   dwCode = HIWORD(wParam);
   dwID   = LOWORD(wParam);
   lpp    = GetScrollPtr( wParam );
   if( lpp )
      sprintf( lEndBuf(lps), "[%s]", lpp );
   else
      sprintf( lEndBuf(lps), "[%#X?]", dwID );
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(ut);
}


WMSTR sFlgs[] = {
   { MF_BITMAP, "MF_BITMAP", 0 },   // Item displays a bitmap.
   { MF_CHECKED, "MF_CHECKED", 0 }, // Item is checked. 
   { MF_DISABLED, "MF_DISABLED", 0 },  // Item is disabled.
   { MF_GRAYED, "MF_GRAYED", 0 },   // Item is grayed.
   { MF_HILITE, "MF_HILITE", 0 },   // Item is highlighted.
   { MF_MOUSESELECT, "MF_MOUSESELECT", 0 },  // Item is selected with the mouse.
   { MF_OWNERDRAW, "MF_OWNERDRAW", 0 },   // Item is an owner-drawn item.
   { MF_POPUP, "MF_POPUP", 0 },  // Item opens a drop-down menu or submenu.
   { MF_SYSMENU, "MF_SYSMENU", 0 }, // Item is contained in the window menu. The lParam parameter contains a handle to the menu associated with the message.
   { 0,          0,            0 }
};

void  AddMFFlag(LPTSTR lpb, DWORD dwFlag)
{
   PWMSTR   lpm = &sFlgs[0];
   LPTSTR   lps;
   DWORD    dwf;
   DWORD    dwc;
   dwf = dwFlag;
   if(dwf)
   {
      dwc = 0;
      lps = lpm->w_lpStg;
      while( lps )
      {
         if( lpm->w_uiMsg & dwf )
         {
            if(dwc)
               strcat(lpb,"|");
            strcat(lpb, lps);
            dwf &= ~( lpm->w_uiMsg );
            if( dwf == 0 )
               break;
            dwc++;
         }
         lpm++;
         lps = lpm->w_lpStg;
      }
   }
}

void  H_MENUSEL( UINT ut, LPTSTR lps, WPARAM wParam, LPARAM lParam )
{
   DWORD dwItem, dwFlag;
   dwItem = LOWORD(wParam);
   dwFlag = HIWORD(wParam);
   sprintf(lEndBuf(lps), "[h=%llx id=%u f=%u(",
         lParam,
         dwItem,
         dwFlag );
   AddMFFlag(lps,dwFlag);
   strcat(lps, ")]" );
   UNREFERENCED_PARAMETER(ut);
}

#ifndef WM_UAHDESTROYWINDOW
#define WM_UAHDESTROYWINDOW 0x0090
#define WM_UAHDRAWMENU 0x0091
#define WM_UAHDRAWMENUITEM 0x0092
#define WM_UAHINITMENU 0x0093
#define WM_UAHMEASUREMENUITEM 0x0094
#define WM_UAHNCPAINTMENUPOPUP 0x0095
#endif
#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION   0x00AE
#endif 


// set prepared from July 2000 MS SDK Windows.h header
// IT seems to continue to GROW ...
// 2000 Sep 30 - ADDED the "CB_..." set of messages - grm
// Feb 10, 2001 - ADDED the Edit Control Messages - 0x00B0 to 0x00D9 fromJan 2001 MSDN
// ================================================
WMSTR	sWm[] = {
	{ WM_NULL                         , "WM_NULL", 0 },
	{ WM_CREATE                       , "WM_CREATE", 0 },
	{ WM_DESTROY                      , "WM_DESTROY", 0 },
	{ WM_MOVE                         , "WM_MOVE", H_ADDMOVE },
	{ WM_SIZE                         , "WM_SIZE", H_ADDSIZE },
	{ WM_ACTIVATE                     , "WM_ACTIVATE", 0 },
	{ WM_SETFOCUS                     , "WM_SETFOCUS", 0 },
	{ WM_KILLFOCUS                    , "WM_KILLFOCUS", 0 },
	{ WM_ENABLE                       , "WM_ENABLE", 0 },
	{ WM_SETREDRAW                    , "WM_SETREDRAW", 0 },
	{ WM_SETTEXT                      , "WM_SETTEXT", 0 },
	{ WM_GETTEXT                      , "WM_GETTEXT", 0 },
	{ WM_GETTEXTLENGTH                , "WM_GETTEXTLENGTH", 0 },
	{ WM_PAINT                        , "WM_PAINT", 0 },
	{ WM_CLOSE                        , "WM_CLOSE", 0 },
	{ WM_QUERYENDSESSION              , "WM_QUERYENDSESSION", 0 },
	{ WM_QUERYOPEN                    , "WM_QUERYOPEN", 0 },
	{ WM_ENDSESSION                   , "WM_ENDSESSION", 0 },
	{ WM_QUIT                         , "WM_QUIT", 0 },
	{ WM_ERASEBKGND                   , "WM_ERASEBKGND", 0 },
	{ WM_SYSCOLORCHANGE               , "WM_SYSCOLORCHANGE", 0 },
	{ WM_SHOWWINDOW                   , "WM_SHOWWINDOW", 0 },
	{ WM_WININICHANGE                 , "WM_WININICHANGE", 0 },
	{ WM_DEVMODECHANGE                , "WM_DEVMODECHANGE", 0 },
	{ WM_ACTIVATEAPP                  , "WM_ACTIVATEAPP", 0 },
	{ WM_FONTCHANGE                   , "WM_FONTCHANGE", 0 },
	{ WM_TIMECHANGE                   , "WM_TIMECHANGE", 0 },
	{ WM_CANCELMODE                   , "WM_CANCELMODE", 0 },
	{ WM_SETCURSOR                    , "WM_SETCURSOR", 0 },
	{ WM_MOUSEACTIVATE                , "WM_MOUSEACTIVATE", 0 },
	{ WM_CHILDACTIVATE                , "WM_CHILDACTIVATE", 0 },
	{ WM_QUEUESYNC                    , "WM_QUEUESYNC", 0 },
	{ WM_GETMINMAXINFO                , "WM_GETMINMAXINFO", 0 },
	{ WM_PAINTICON                    , "WM_PAINTICON", 0 },
	{ WM_ICONERASEBKGND               , "WM_ICONERASEBKGND", 0 },
	{ WM_NEXTDLGCTL                   , "WM_NEXTDLGCTL", 0 },
	{ WM_SPOOLERSTATUS                , "WM_SPOOLERSTATUS", 0 },
	{ WM_DRAWITEM                     , "WM_DRAWITEM", 0 },
	{ WM_MEASUREITEM                  , "WM_MEASUREITEM", 0 },
	{ WM_DELETEITEM                   , "WM_DELETEITEM", 0 },
	{ WM_VKEYTOITEM                   , "WM_VKEYTOITEM", 0 },
	{ WM_CHARTOITEM                   , "WM_CHARTOITEM", 0 },
	{ WM_SETFONT                      , "WM_SETFONT", 0 },
	{ WM_GETFONT                      , "WM_GETFONT", 0 },
	{ WM_SETHOTKEY                    , "WM_SETHOTKEY", 0 },
	{ WM_GETHOTKEY                    , "WM_GETHOTKEY", 0 },
	{ WM_QUERYDRAGICON                , "WM_QUERYDRAGICON", 0 },
	{ WM_COMPAREITEM                  , "WM_COMPAREITEM", 0 },
#if(WINVER >= 0x0500)
	{ WM_GETOBJECT                    , "WM_GETOBJECT", 0 },
#endif   // WINVER >= 0x0500
	{ WM_COMPACTING                   , "WM_COMPACTING", 0 },
	{ WM_COMMNOTIFY                   , "WM_COMMNOTIFY", 0 },
	{ WM_WINDOWPOSCHANGING            , "WM_WINDOWPOSCHANGING", 0 },
	{ WM_WINDOWPOSCHANGED             , "WM_WINDOWPOSCHANGED", 0 },
	{ WM_POWER                        , "WM_POWER", 0 },
	{ WM_COPYDATA                     , "WM_COPYDATA", 0 },
	{ WM_CANCELJOURNAL                , "WM_CANCELJOURNAL", 0 },
	{ WM_NOTIFY                       , "WM_NOTIFY", H_NOTIFY },
	{ WM_INPUTLANGCHANGEREQUEST       , "WM_INPUTLANGCHANGEREQUEST", 0 },
	{ WM_INPUTLANGCHANGE              , "WM_INPUTLANGCHANGE", 0 },
	{ WM_TCARD                        , "WM_TCARD", 0 },
	{ WM_HELP                         , "WM_HELP", 0 },
	{ WM_USERCHANGED                  , "WM_USERCHANGED", 0 },
	{ WM_NOTIFYFORMAT                 , "WM_NOTIFYFORMAT", 0 },
	{ WM_CONTEXTMENU                  , "WM_CONTEXTMENU", 0 },
	{ WM_STYLECHANGING                , "WM_STYLECHANGING", 0 },
	{ WM_STYLECHANGED                 , "WM_STYLECHANGED", 0 },
	{ WM_DISPLAYCHANGE                , "WM_DISPLAYCHANGE", 0 },
	{ WM_GETICON                      , "WM_GETICON", 0 },
	{ WM_SETICON                      , "WM_SETICON", 0 },
	{ WM_NCCREATE                     , "WM_NCCREATE", 0 },
	{ WM_NCDESTROY                    , "WM_NCDESTROY", 0 },
	{ WM_NCCALCSIZE                   , "WM_NCCALCSIZE", 0 },
	{ WM_NCHITTEST                    , "WM_NCHITTEST", 0 },
	{ WM_NCPAINT                      , "WM_NCPAINT", 0 },
	{ WM_NCACTIVATE                   , "WM_NCACTIVATE", 0 },
	{ WM_GETDLGCODE                   , "WM_GETDLGCODE", 0 },   // 0x0087
	{ WM_SYNCPAINT                    , "WM_SYNCPAINT", 0 },    // 0x0088
	/* FIX20200621 add some 'hidden' messages */
	{ WM_UAHDESTROYWINDOW             , "WM_UAHDESTROYWINDOW", 0 }, // 0x0090
	{ WM_UAHDRAWMENU                  , "WM_UAHDRAWMENU", 0 }, // 0x0091
	{ WM_UAHDRAWMENUITEM              , "WM_UAHDRAWMENUITEM", 0 }, // 0x0092
	{ WM_UAHINITMENU                  , "WM_UAHINITMENU", 0 }, // 0x0093
	{ WM_UAHMEASUREMENUITEM           , "WM_UAHMEASUREMENUITEM", 0 }, // 0x0094
	{ WM_UAHNCPAINTMENUPOPUP          , "WM_UAHNCPAINTMENUPOPUP", 0 }, // 0x0095
	/////////////////////////////////////////////////////////////

	{ WM_NCMOUSEMOVE                  , "WM_NCMOUSEMOVE", 0 },     // 0x00A0
	{ WM_NCLBUTTONDOWN                , "WM_NCLBUTTONDOWN", 0 },   // 0x00A1
	{ WM_NCLBUTTONUP                  , "WM_NCLBUTTONUP", 0 },
	{ WM_NCLBUTTONDBLCLK              , "WM_NCLBUTTONDBLCLK", 0 },
	{ WM_NCRBUTTONDOWN                , "WM_NCRBUTTONDOWN", 0 },
	{ WM_NCRBUTTONUP                  , "WM_NCRBUTTONUP", 0 },
	{ WM_NCRBUTTONDBLCLK              , "WM_NCRBUTTONDBLCLK", 0 },
	{ WM_NCMBUTTONDOWN                , "WM_NCMBUTTONDOWN", 0 },
	{ WM_NCMBUTTONUP                  , "WM_NCMBUTTONUP", 0 },
	{ WM_NCMBUTTONDBLCLK              , "WM_NCMBUTTONDBLCLK", 0 }, // 0x00A9

//#if(_WIN32_WINNT >= 0x0500)
	{ WM_NCXBUTTONDOWN                , "WM_NCXBUTTONDOWN", 0 },   // 0x00AB
	{ WM_NCXBUTTONUP                  , "WM_NCXBUTTONUP", 0 },
	{ WM_NCXBUTTONDBLCLK              , "WM_NCXBUTTONDBLCLK", 0 }, // 0x00AD
	{ WM_NCUAHDRAWCAPTION             , "WM_NCUAHDRAWCAPTION", 0 }, // 0x00AE
///#endif   // _WIN32_WINNT >= 0x0500

   /* * Edit Control Messages * */
   { EM_GETSEL, "EM_GETSEL", 0 }, // 0x00B0
   { EM_SETSEL, "EM_SETSEL", 0 }, // 0x00B1
   { EM_GETRECT, "EM_GETRECT", 0 }, // 0x00B2
   { EM_SETRECT, "EM_SETRECT", 0 }, // 0x00B3
   { EM_SETRECTNP, "EM_SETRECTNP", 0 }, // 0x00B4
   { EM_SCROLL, "EM_SCROLL", 0 }, // 0x00B5
   { EM_LINESCROLL, "EM_LINESCROLL", 0 }, // 0x00B6
   { EM_SCROLLCARET, "EM_SCROLLCARET", 0 }, // 0x00B7
   { EM_GETMODIFY, "EM_GETMODIFY", 0 }, // 0x00B8
   { EM_SETMODIFY, "EM_SETMODIFY", 0 }, // 0x00B9
   { EM_GETLINECOUNT, "EM_GETLINECOUNT", 0 }, // 0x00BA
   { EM_LINEINDEX, "EM_LINEINDEX", 0 }, // 0x00BB
   { EM_SETHANDLE, "EM_SETHANDLE", 0 }, // 0x00BC
   { EM_GETHANDLE, "EM_GETHANDLE", 0 }, // 0x00BD
   { EM_GETTHUMB, "EM_GETTHUMB", 0 }, // 0x00BE
   { EM_LINELENGTH, "EM_LINELENGTH", 0 }, // 0x00C1
   { EM_REPLACESEL, "EM_REPLACESEL", 0 }, // 0x00C2
   { EM_GETLINE, "EM_GETLINE", 0 }, // 0x00C4
   { EM_LIMITTEXT, "EM_LIMITTEXT", 0 }, // 0x00C5
   { EM_CANUNDO, "EM_CANUNDO", 0 }, // 0x00C6
   { EM_UNDO, "EM_UNDO", 0 }, // 0x00C7
   { EM_FMTLINES, "EM_FMTLINES", 0 }, // 0x00C8
   { EM_LINEFROMCHAR, "EM_LINEFROMCHAR", 0 }, // 0x00C9
   { EM_SETTABSTOPS, "EM_SETTABSTOPS", 0 }, // 0x00CB
   { EM_SETPASSWORDCHAR, "EM_SETPASSWORDCHAR", 0 }, // 0x00CC
   { EM_EMPTYUNDOBUFFER, "EM_EMPTYUNDOBUFFER", 0 }, // 0x00CD
   { EM_GETFIRSTVISIBLELINE, "EM_GETFIRSTVISIBLELINE", 0 }, // 0x00CE
   { EM_SETREADONLY, "EM_SETREADONLY", 0 }, // 0x00CF
   { EM_SETWORDBREAKPROC, "EM_SETWORDBREAKPROC", 0 }, // 0x00D0
   { EM_GETWORDBREAKPROC, "EM_GETWORDBREAKPROC", 0 }, // 0x00D1
   { EM_GETPASSWORDCHAR, "EM_GETPASSWORDCHAR", 0 }, // 0x00D2

#if(WINVER >= 0x0400)
   { EM_SETMARGINS, "EM_SETMARGINS", 0 }, // 0x00D3
   { EM_GETMARGINS, "EM_GETMARGINS", 0 }, // 0x00D4
   { EM_SETLIMITTEXT, "EM_SETLIMITTEXT", 0 }, // =EM_LIMITTEXT -win40 Name change
   { EM_GETLIMITTEXT, "EM_GETLIMITTEXT", 0 }, // 0x00D5
   { EM_POSFROMCHAR, "EM_POSFROMCHAR", 0 }, // 0x00D6
   { EM_CHARFROMPOS, "EM_CHARFROMPOS", 0 }, // 0x00D7
#endif /* WINVER >= 0x0400 */

#if(WINVER >= 0x0500)
   { EM_SETIMESTATUS, "EM_SETIMESTATUS", 0 }, // 0x00D8
   { EM_GETIMESTATUS, "EM_GETIMESTATUS", 0 }, // 0x00D9
#endif /* WINVER >= 0x0500 */

//	{ WM_KEYFIRST                     , "WM_KEYFIRST", 0 },        // 0x0100
	{ WM_KEYDOWN                      , "WM_KEYDOWN", 0 },         // 0x0100
	{ WM_KEYUP                        , "WM_KEYUP", 0 },
	{ WM_CHAR                         , "WM_CHAR", H_ADDCHAR },
	{ WM_DEADCHAR                     , "WM_DEADCHAR", 0 },
	{ WM_SYSKEYDOWN                   , "WM_SYSKEYDOWN", 0 },
	{ WM_SYSKEYUP                     , "WM_SYSKEYUP", 0 },
	{ WM_SYSCHAR                      , "WM_SYSCHAR", 0 },
	{ WM_SYSDEADCHAR                  , "WM_SYSDEADCHAR", 0 },     // 0x0107
//	{ WM_KEYLAST                      , "WM_KEYLAST", 0 },         // 0x0108

	{ WM_IME_STARTCOMPOSITION         , "WM_IME_STARTCOMPOSITION", 0 },  // 0x010D
	{ WM_IME_ENDCOMPOSITION           , "WM_IME_ENDCOMPOSITION", 0 },
	{ WM_IME_COMPOSITION              , "WM_IME_COMPOSITION", 0 },       // 0x010F
//	{ WM_IME_KEYLAST                  , "WM_IME_KEYLAST", 0 },           // 0x010F

	{ WM_INITDIALOG                   , "WM_INITDIALOG", 0 },            // 0x0110
	{ WM_COMMAND                      , "WM_COMMAND", H_COMMAND },       // 0x0111
	{ WM_SYSCOMMAND                   , "WM_SYSCOMMAND", H_SYSCOMMAND }, // 0x0112
	{ WM_TIMER                        , "WM_TIMER", 0 },
	{ WM_HSCROLL                      , "WM_HSCROLL", H_SCROLL },
	{ WM_VSCROLL                      , "WM_VSCROLL", H_SCROLL },
	{ WM_INITMENU                     , "WM_INITMENU", 0 },
	{ WM_INITMENUPOPUP                , "WM_INITMENUPOPUP", 0 },
	{ WM_MENUSELECT                   , "WM_MENUSELECT", H_MENUSEL },    // 0x011F
	{ WM_MENUCHAR                     , "WM_MENUCHAR", 0 },              // 0x0120
	{ WM_ENTERIDLE                    , "WM_ENTERIDLE", 0 },             // 0x0121
//#if(WINVER >= 0x0500)
//	{ WM_MENURBUTTONUP                , "WM_MENURBUTTONUP", 0 },         // 0x0122
//	{ WM_MENUDRAG                     , "WM_MENUDRAG", 0 },
//	{ WM_MENUGETOBJECT                , "WM_MENUGETOBJECT", 0 },
//	{ WM_UNINITMENUPOPUP              , "WM_UNINITMENUPOPUP", 0 },
//	{ WM_MENUCOMMAND                  , "WM_MENUCOMMAND", 0 },
//	{ WM_CHANGEUISTATE                , "WM_CHANGEUISTATE", 0 },
//	{ WM_UPDATEUISTATE                , "WM_UPDATEUISTATE", 0 },
//	{ WM_QUERYUISTATE                 , "WM_QUERYUISTATE", 0 },          // 0x0129
//#else
	{ 0x0122                , "WM_MENURBUTTONUP(5)", 0 },
	{ 0x0123                , "WM_MENUDRAG(5)", 0 },
	{ 0x0124                , "WM_MENUGETOBJECT(5)", 0 },
	{ 0x0125                , "WM_UNINITMENUPOPUP(5)", 0 },
	{ 0x0126                , "WM_MENUCOMMAND(5)", 0 },
	{ 0x0127                , "WM_CHANGEUISTATE(5)", 0 },
	{ 0x0128                , "WM_UPDATEUISTATE(5)", 0 },
	{ 0x0129                , "WM_QUERYUISTATE(5)", 0 },
//#endif   // if(WINVER >= 0x0500)

	{ WM_CTLCOLORMSGBOX               , "WM_CTLCOLORMSGBOX", 0 },     // 0x0132
	{ WM_CTLCOLOREDIT                 , "WM_CTLCOLOREDIT", 0 },
	{ WM_CTLCOLORLISTBOX              , "WM_CTLCOLORLISTBOX", 0 },
	{ WM_CTLCOLORBTN                  , "WM_CTLCOLORBTN", 0 },
	{ WM_CTLCOLORDLG                  , "WM_CTLCOLORDLG", 0 },
	{ WM_CTLCOLORSCROLLBAR            , "WM_CTLCOLORSCROLLBAR", 0 },
	{ WM_CTLCOLORSTATIC               , "WM_CTLCOLORSTATIC", 0 },     // 0x0138
// ========================

	{ CB_GETEDITSEL               ,"CB_GETEDITSEL", 0 }, // 0x0140
	{ CB_LIMITTEXT                ,"CB_LIMITTEXT", 0 }, // 0x0141
	{ CB_SETEDITSEL               ,"CB_SETEDITSEL", 0 }, // 0x0142
	{ CB_ADDSTRING                ,"CB_ADDSTRING", 0 }, // 0x0143
	{ CB_DELETESTRING             ,"CB_DELETESTRING", 0 }, // 0x0144
	{ CB_DIR                      ,"CB_DIR", 0 }, // 0x0145
	{ CB_GETCOUNT                 ,"CB_GETCOUNT", 0 }, // 0x0146
	{ CB_GETCURSEL                ,"CB_GETCURSEL", 0 }, // 0x0147
	{ CB_GETLBTEXT                ,"CB_GETLBTEXT", 0 }, // 0x0148
	{ CB_GETLBTEXTLEN             ,"CB_GETLBTEXTLEN", 0 }, // 0x0149
	{ CB_INSERTSTRING             ,"CB_INSERTSTRING", 0 }, // 0x014A
	{ CB_RESETCONTENT             ,"CB_RESETCONTENT", 0 }, // 0x014B
   { CB_FINDSTRING               ,"CB_FINDSTRING", 0 }, //H_ADDSTRING }, // 0x014C
	{ CB_SELECTSTRING             ,"CB_SELECTSTRING", 0 }, // 0x014D
	{ CB_SETCURSEL                ,"CB_SETCURSEL", 0 }, // 0x014E
	{ CB_SHOWDROPDOWN             ,"CB_SHOWDROPDOWN", 0 }, // 0x014F
	{ CB_GETITEMDATA              ,"CB_GETITEMDATA", 0 }, // 0x0150
	{ CB_SETITEMDATA              ,"CB_SETITEMDATA", 0 }, // 0x0151
	{ CB_GETDROPPEDCONTROLRECT    ,"CB_GETDROPPEDCONTROLRECT", 0 }, // 0x0152
	{ CB_SETITEMHEIGHT            ,"CB_SETITEMHEIGHT", 0 }, // 0x0153
	{ CB_GETITEMHEIGHT            ,"CB_GETITEMHEIGHT", 0 }, // 0x0154
	{ CB_SETEXTENDEDUI            ,"CB_SETEXTENDEDUI", 0 }, // 0x0155
	{ CB_GETEXTENDEDUI            ,"CB_GETEXTENDEDUI", 0 }, // 0x0156
	{ CB_GETDROPPEDSTATE          ,"CB_GETDROPPEDSTATE", 0 }, // 0x0157
   { CB_FINDSTRINGEXACT          ,"CB_FINDSTRINGEXACT", 0 }, //H_ADDSTRING }, // 0x0158
	{ CB_SETLOCALE                ,"CB_SETLOCALE", 0 }, // 0x0159
	{ CB_GETLOCALE                ,"CB_GETLOCALE", 0 }, // 0x015A
#if(WINVER >= 0x0400)
	{ CB_GETTOPINDEX              ,"CB_GETTOPINDEX", 0 }, // 0x015b
	{ CB_SETTOPINDEX              ,"CB_SETTOPINDEX", 0 }, // 0x015c
	{ CB_GETHORIZONTALEXTENT      ,"CB_GETHORIZONTALEXTENT", 0 }, // 0x015d
	{ CB_SETHORIZONTALEXTENT      ,"CB_SETHORIZONTALEXTENT", 0 }, // 0x015e
	{ CB_GETDROPPEDWIDTH          ,"CB_GETDROPPEDWIDTH", 0 }, // 0x015f
	{ CB_SETDROPPEDWIDTH          ,"CB_SETDROPPEDWIDTH", 0 }, // 0x0160
	{ CB_INITSTORAGE              ,"CB_INITSTORAGE", 0 }, // 0x0161
#if(_WIN32_WCE >= 0x0400)
	{ CB_MULTIPLEADDSTRING        ,"CB_MULTIPLEADDSTRING", 0 }, // 0x0163
#endif
#endif /* WINVER >= 0x0400 */
#if(_WIN32_WCE >= 0x0400)
	{ CB_MSGMAX                   ,"CB_MSGMAX", 0 }, // 0x0163
#elif(WINVER >= 0x0400)
	{ CB_MSGMAX                   ,"CB_MSGMAX", 0 }, // 0x0162
#else
	{ CB_MSGMAX                   ,"CB_MSGMAX", 0 }, // 0x015B
#endif
// ========================
// *****************************
   { STM_SETICON, "STM_SETICON", 0 },                           // 0x0170
   { STM_GETICON, "STM_GETICON", 0 },                           // 0x0171
   { LB_ADDSTRING, "LB_ADDSTRING", 0 },                         // 0x0180
   { LB_INSERTSTRING, "LB_INSERTSTRING", 0 },                   // 0x0181
   { LB_DELETESTRING, "LB_DELETESTRING", 0 },                   // 0x0182
   { LB_SELITEMRANGEEX, "LB_SELITEMRANGEEX", 0 },               // 0x0183
   { LB_RESETCONTENT, "LB_RESETCONTENT", 0 },                   // 0x0184
   { LB_SETSEL, "LB_SETSEL", 0 },                               // 0x0185
   { LB_SETCURSEL, "LB_SETCURSEL", 0 },                         // 0x0186
   { LB_GETSEL, "LB_GETSEL", 0 },                               // 0x0187
   { LB_GETCURSEL, "LB_GETCURSEL", 0 },                         // 0x0188
   { LB_GETTEXT, "LB_GETTEXT", 0 },                             // 0x0189
   { LB_GETTEXTLEN, "LB_GETTEXTLEN", 0 },                       // 0x018A
   { LB_GETCOUNT, "LB_GETCOUNT", 0 },                           // 0x018B
   { LB_SELECTSTRING, "LB_SELECTSTRING", 0 },                   // 0x018C
   { LB_DIR, "LB_DIR", 0 },                                     // 0x018D
   { LB_GETTOPINDEX, "LB_GETTOPINDEX", 0 },                     // 0x018E
   { LB_FINDSTRING, "LB_FINDSTRING", 0 },                       // 0x018F
   { LB_GETSELCOUNT, "LB_GETSELCOUNT", 0 },                     // 0x0190
   { LB_GETSELITEMS, "LB_GETSELITEMS", 0 },                     // 0x0191
   { LB_SETTABSTOPS, "LB_SETTABSTOPS", 0 },                     // 0x0192
   { LB_GETHORIZONTALEXTENT, "LB_GETHORIZONTALEXTENT", 0 },     // 0x0193
   { LB_SETHORIZONTALEXTENT, "LB_SETHORIZONTALEXTENT", 0 },     // 0x0194
   { LB_SETCOLUMNWIDTH, "LB_SETCOLUMNWIDTH", 0 },               // 0x0195
   { LB_ADDFILE, "LB_ADDFILE", 0 },                             // 0x0196
   { LB_SETTOPINDEX, "LB_SETTOPINDEX", 0 },                     // 0x0197
   { LB_GETITEMRECT, "LB_GETITEMRECT", 0 },                     // 0x0198
   { LB_GETITEMDATA, "LB_GETITEMDATA", 0 },                     // 0x0199
   { LB_SETITEMDATA, "LB_SETITEMDATA", 0 },                     // 0x019A
   { LB_SELITEMRANGE, "LB_SELITEMRANGE", 0 },                   // 0x019B
   { LB_SETANCHORINDEX, "LB_SETANCHORINDEX", 0 },               // 0x019C
   { LB_GETANCHORINDEX, "LB_GETANCHORINDEX", 0 },               // 0x019D
   { LB_SETCARETINDEX, "LB_SETCARETINDEX", 0 },                 // 0x019E
   { LB_GETCARETINDEX, "LB_GETCARETINDEX", 0 },                 // 0x019F
   { LB_SETITEMHEIGHT, "LB_SETITEMHEIGHT", 0 },                 // 0x01A0
   { LB_GETITEMHEIGHT, "LB_GETITEMHEIGHT", 0 },                 // 0x01A1
   { LB_FINDSTRINGEXACT, "LB_FINDSTRINGEXACT", 0 },             // 0x01A2
   { LB_SETLOCALE, "LB_SETLOCALE", 0 },                         // 0x01A5
   { LB_GETLOCALE, "LB_GETLOCALE", 0 },                         // 0x01A6
   { LB_SETCOUNT, "LB_SETCOUNT", 0 },                           // 0x01A7

// *****************************

	// { WM_MOUSEFIRST                   , "WM_MOUSEFIRST", 0 },      // 0x0200
	{ WM_MOUSEMOVE                    , "WM_MOUSEMOVE", 0 },		// 0x0200
	{ WM_LBUTTONDOWN                  , "WM_LBUTTONDOWN", 0 },		// 0x0201
	{ WM_LBUTTONUP                    , "WM_LBUTTONUP", 0 },
	{ WM_LBUTTONDBLCLK                , "WM_LBUTTONDBLCLK", 0 },
	{ WM_RBUTTONDOWN                  , "WM_RBUTTONDOWN", 0 },
	{ WM_RBUTTONUP                    , "WM_RBUTTONUP", 0 },
	{ WM_RBUTTONDBLCLK                , "WM_RBUTTONDBLCLK", 0 },
	{ WM_MBUTTONDOWN                  , "WM_MBUTTONDOWN", 0 },
	{ WM_MBUTTONUP                    , "WM_MBUTTONUP", 0 },
	{ WM_MBUTTONDBLCLK                , "WM_MBUTTONDBLCLK", 0 },
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	{ WM_MOUSEWHEEL                   , "WM_MOUSEWHEEL", 0 },
#endif   // (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
#if (_WIN32_WINNT >= 0x0500)
	{ WM_XBUTTONDOWN                  , "WM_XBUTTONDOWN", 0 },
	{ WM_XBUTTONUP                    , "WM_XBUTTONUP", 0 },
	{ WM_XBUTTONDBLCLK                , "WM_XBUTTONDBLCLK", 0 },
#endif   // (_WIN32_WINNT >= 0x0500)
   { WM_MOUSELAST                    , "WM_MOUSELAST", 0 },	// 0x020E
	{ WM_PARENTNOTIFY                 , "WM_PARENTNOTIFY", H_ADDNOTE },
	{ WM_ENTERMENULOOP                , "WM_ENTERMENULOOP", 0 },
	{ WM_EXITMENULOOP                 , "WM_EXITMENULOOP", 0 },
	{ WM_NEXTMENU                     , "WM_NEXTMENU", 0 },
	{ WM_SIZING                       , "WM_SIZING", 0 },
	{ WM_CAPTURECHANGED               , "WM_CAPTURECHANGED", 0 },
	{ WM_MOVING                       , "WM_MOVING", 0 },
	{ WM_POWERBROADCAST               , "WM_POWERBROADCAST", 0 },
	{ WM_DEVICECHANGE                 , "WM_DEVICECHANGE", 0 },
	{ WM_MDICREATE                    , "WM_MDICREATE", 0 },
	{ WM_MDIDESTROY                   , "WM_MDIDESTROY", 0 },
	{ WM_MDIACTIVATE                  , "WM_MDIACTIVATE", 0 },
	{ WM_MDIRESTORE                   , "WM_MDIRESTORE", 0 },
	{ WM_MDINEXT                      , "WM_MDINEXT", 0 },
	{ WM_MDIMAXIMIZE                  , "WM_MDIMAXIMIZE", 0 },
	{ WM_MDITILE                      , "WM_MDITILE", 0 },
	{ WM_MDICASCADE                   , "WM_MDICASCADE", 0 },
	{ WM_MDIICONARRANGE               , "WM_MDIICONARRANGE", 0 },
	{ WM_MDIGETACTIVE                 , "WM_MDIGETACTIVE", 0 },
	{ WM_MDISETMENU                   , "WM_MDISETMENU", 0 },
	{ WM_ENTERSIZEMOVE                , "WM_ENTERSIZEMOVE", 0 },
	{ WM_EXITSIZEMOVE                 , "WM_EXITSIZEMOVE", 0 },
	{ WM_DROPFILES                    , "WM_DROPFILES", 0 },
	{ WM_MDIREFRESHMENU               , "WM_MDIREFRESHMENU", 0 },
	{ WM_IME_SETCONTEXT               , "WM_IME_SETCONTEXT", 0 },
	{ WM_IME_NOTIFY                   , "WM_IME_NOTIFY", 0 },
	{ WM_IME_CONTROL                  , "WM_IME_CONTROL", 0 },
	{ WM_IME_COMPOSITIONFULL          , "WM_IME_COMPOSITIONFULL", 0 },
	{ WM_IME_SELECT                   , "WM_IME_SELECT", 0 },
	{ WM_IME_CHAR                     , "WM_IME_CHAR", 0 },
#if(WINVER >= 0x0500)
	{ WM_IME_REQUEST                  , "WM_IME_REQUEST", 0 },
#endif   // (WINVER >= 0x0500)
	{ WM_IME_KEYDOWN                  , "WM_IME_KEYDOWN", 0 },
	{ WM_IME_KEYUP                    , "WM_IME_KEYUP", 0 },
#if((_WIN32_WINNT >= 0x0400) || (WINVER >= 0x0500))
	{ WM_MOUSEHOVER                   , "WM_MOUSEHOVER", 0 },
	{ WM_MOUSELEAVE                   , "WM_MOUSELEAVE", 0 },
#endif   // ((_WIN32_WINNT >= 0x0400) || (WINVER >= 0x0500))
#if(WINVER >= 0x0500)
	{ WM_NCMOUSEHOVER                 , "WM_NCMOUSEHOVER", 0 },
	{ WM_NCMOUSELEAVE                 , "WM_NCMOUSELEAVE", 0 },
#endif   // (WINVER >= 0x0500)
	{ WM_CUT                          , "WM_CUT", 0 }, // 0x0300
	{ WM_COPY                         , "WM_COPY", 0 },
	{ WM_PASTE                        , "WM_PASTE", 0 },
	{ WM_CLEAR                        , "WM_CLEAR", 0 },
	{ WM_UNDO                         , "WM_UNDO", 0 },
	{ WM_RENDERFORMAT                 , "WM_RENDERFORMAT", 0 },
	{ WM_RENDERALLFORMATS             , "WM_RENDERALLFORMATS", 0 },
	{ WM_DESTROYCLIPBOARD             , "WM_DESTROYCLIPBOARD", 0 },
	{ WM_DRAWCLIPBOARD                , "WM_DRAWCLIPBOARD", 0 },
	{ WM_PAINTCLIPBOARD               , "WM_PAINTCLIPBOARD", 0 },
	{ WM_VSCROLLCLIPBOARD             , "WM_VSCROLLCLIPBOARD", 0 },
	{ WM_SIZECLIPBOARD                , "WM_SIZECLIPBOARD", 0 },
	{ WM_ASKCBFORMATNAME              , "WM_ASKCBFORMATNAME", 0 },
	{ WM_CHANGECBCHAIN                , "WM_CHANGECBCHAIN", 0 },
	{ WM_HSCROLLCLIPBOARD             , "WM_HSCROLLCLIPBOARD", 0 },
	{ WM_QUERYNEWPALETTE              , "WM_QUERYNEWPALETTE", 0 },   // 0x030F
	{ WM_PALETTEISCHANGING            , "WM_PALETTEISCHANGING", 0 }, // 0x0310
	{ WM_PALETTECHANGED               , "WM_PALETTECHANGED", 0 },
	{ WM_HOTKEY                       , "WM_HOTKEY", 0 },
	{ WM_PRINT                        , "WM_PRINT", 0 },
	{ WM_PRINTCLIENT                  , "WM_PRINTCLIENT", 0 },
#if(_WIN32_WINNT >= 0x0500)
	{ WM_APPCOMMAND                   , "WM_APPCOMMAND", 0 }, // 0x0319
#endif   // (_WIN32_WINNT >= 0x0500)

#if(_WIN32_WINNT >= 0x0600) // added 20200621
	{ WM_DWMCOMPOSITIONCHANGED         , "WM_DWMCOMPOSITIONCHANGED", 0 }, // 0x031E
	{ WM_DWMNCRENDERINGCHANGED         , "WM_DWMNCRENDERINGCHANGED", 0 }, // 0x031F
	{ WM_DWMCOLORIZATIONCOLORCHANGED   , "WM_DWMCOLORIZATIONCOLORCHANGED", 0 }, // 0x0320
	{ WM_DWMWINDOWMAXIMIZEDCHANGE      , "WM_DWMWINDOWMAXIMIZEDCHANGE", 0 }, // 0x0321
#endif /* _WIN32_WINNT >= 0x0600 */


    { WM_HANDHELDFIRST                , "WM_HANDHELDFIRST", 0 }, // 0x0358
	{ WM_HANDHELDLAST                 , "WM_HANDHELDLAST", 0 }, // 0x035F
	{ WM_AFXFIRST                     , "WM_AFXFIRST", 0 },	// 0x0360
	{ WM_AFXLAST                      , "WM_AFXLAST", 0 },  // 0x037F
	{ WM_PENWINFIRST                  , "WM_PENWINFIRST", 0 }, // 0x0380
	{ WM_PENWINLAST                   , "WM_PENWINLAST", 0 },  // 0x038F
//	{ WM_USER                         , "WM_USER", 0 },
   { 0                               , 0, 0 }
};

#ifdef _RICHEDIT_
#pragma message( "RichEdit also defined ..." )
WMSTR	sRe[] = {
   { EM_GETLIMITTEXT         ,"EM_GETLIMITTEXT", 0 }, // (WM_USER + 37)
   { EM_POSFROMCHAR          ,"EM_POSFROMCHAR", 0 }, // (WM_USER + 38)
   { EM_CHARFROMPOS          ,"EM_CHARFROMPOS", 0 }, // (WM_USER + 39)
   { EM_SCROLLCARET          ,"EM_SCROLLCARET", 0 }, // (WM_USER + 49)
   { EM_CANPASTE             ,"EM_CANPASTE", 0 }, // (WM_USER + 50)
   { EM_DISPLAYBAND          ,"EM_DISPLAYBAND", 0 }, // (WM_USER + 51)
   { EM_EXGETSEL             ,"EM_EXGETSEL", 0 }, // (WM_USER + 52)
   { EM_EXLIMITTEXT          ,"EM_EXLIMITTEXT", 0 }, // (WM_USER + 53)
   { EM_EXLINEFROMCHAR       ,"EM_EXLINEFROMCHAR", 0 }, // (WM_USER + 54)
   { EM_EXSETSEL             ,"EM_EXSETSEL", 0 }, // (WM_USER + 55)
   { EM_FINDTEXT             ,"EM_FINDTEXT", 0 }, // (WM_USER + 56)
   { EM_FORMATRANGE          ,"EM_FORMATRANGE", 0 }, // (WM_USER + 57)
   { EM_GETCHARFORMAT        ,"EM_GETCHARFORMAT", 0 }, // (WM_USER + 58)
   { EM_GETEVENTMASK         ,"EM_GETEVENTMASK", 0 }, // (WM_USER + 59)
   { EM_GETOLEINTERFACE      ,"EM_GETOLEINTERFACE", 0 }, // (WM_USER + 60)
   { EM_GETPARAFORMAT        ,"EM_GETPARAFORMAT", 0 }, // (WM_USER + 61)
   { EM_GETSELTEXT           ,"EM_GETSELTEXT", 0 }, // (WM_USER + 62)
   { EM_HIDESELECTION        ,"EM_HIDESELECTION", 0 }, // (WM_USER + 63)
   { EM_PASTESPECIAL         ,"EM_PASTESPECIAL", 0 }, // (WM_USER + 64)
   { EM_REQUESTRESIZE        ,"EM_REQUESTRESIZE", 0 }, // (WM_USER + 65)
   { EM_SELECTIONTYPE        ,"EM_SELECTIONTYPE", 0 }, // (WM_USER + 66)
   { EM_SETBKGNDCOLOR        ,"EM_SETBKGNDCOLOR", 0 }, // (WM_USER + 67)
   { EM_SETCHARFORMAT        ,"EM_SETCHARFORMAT", 0 }, // (WM_USER + 68)
   { EM_SETEVENTMASK         ,"EM_SETEVENTMASK", 0 }, // (WM_USER + 69)
   { EM_SETOLECALLBACK       ,"EM_SETOLECALLBACK", 0 }, // (WM_USER + 70)
   { EM_SETPARAFORMAT        ,"EM_SETPARAFORMAT", 0 }, // (WM_USER + 71)
   { EM_SETTARGETDEVICE      ,"EM_SETTARGETDEVICE", 0 }, // (WM_USER + 72)
   { EM_STREAMIN             ,"EM_STREAMIN", 0 }, // (WM_USER + 73)
   { EM_STREAMOUT            ,"EM_STREAMOUT", 0 }, // (WM_USER + 74)
   { EM_GETTEXTRANGE         ,"EM_GETTEXTRANGE", 0 }, // (WM_USER + 75)
   { EM_FINDWORDBREAK        ,"EM_FINDWORDBREAK", 0 }, // (WM_USER + 76)
   { EM_SETOPTIONS           ,"EM_SETOPTIONS", 0 }, // (WM_USER + 77)
   { EM_GETOPTIONS           ,"EM_GETOPTIONS", 0 }, // (WM_USER + 78)
   { EM_FINDTEXTEX           ,"EM_FINDTEXTEX", 0 }, // (WM_USER + 79)
   { EM_GETWORDBREAKPROCEX   ,"EM_GETWORDBREAKPROCEX", 0 }, // (WM_USER + 80)
   { EM_SETWORDBREAKPROCEX   ,"EM_SETWORDBREAKPROCEX", 0 }, // (WM_USER + 81)
   { EM_SETUNDOLIMIT         ,"EM_SETUNDOLIMIT", 0 }, // (WM_USER + 82)
   { EM_REDO                 ,"EM_REDO", 0 }, // (WM_USER + 84)
   { EM_CANREDO              ,"EM_CANREDO", 0 }, // (WM_USER + 85)
   { EM_GETUNDONAME          ,"EM_GETUNDONAME", 0 }, // (WM_USER + 86)
   { EM_GETREDONAME          ,"EM_GETREDONAME", 0 }, // (WM_USER + 87)
   { EM_STOPGROUPTYPING      ,"EM_STOPGROUPTYPING", 0 }, // (WM_USER + 88)
   { EM_SETTEXTMODE          ,"EM_SETTEXTMODE", 0 }, // (WM_USER + 89)
   { EM_GETTEXTMODE          ,"EM_GETTEXTMODE", 0 }, // (WM_USER + 90)
   { EM_AUTOURLDETECT        ,"EM_AUTOURLDETECT", 0 }, // (WM_USER + 91)
   { EM_GETAUTOURLDETECT     ,"EM_GETAUTOURLDETECT", 0 }, // (WM_USER + 92)
   { EM_SETPALETTE           ,"EM_SETPALETTE", 0 }, // (WM_USER + 93)
   { EM_GETTEXTEX            ,"EM_GETTEXTEX", 0 }, // (WM_USER + 94)
   { EM_GETTEXTLENGTHEX      ,"EM_GETTEXTLENGTHEX", 0 }, // (WM_USER + 95)
   { EM_SHOWSCROLLBAR        ,"EM_SHOWSCROLLBAR", 0 }, // (WM_USER + 96)
   { EM_SETTEXTEX            ,"EM_SETTEXTEX", 0 }, // (WM_USER + 97)
   { EM_SETPUNCTUATION       ,"EM_SETPUNCTUATION", 0 }, // (WM_USER + 100)
   { EM_GETPUNCTUATION       ,"EM_GETPUNCTUATION", 0 }, // (WM_USER + 101)
   { EM_SETWORDWRAPMODE      ,"EM_SETWORDWRAPMODE", 0 }, // (WM_USER + 102)
   { EM_GETWORDWRAPMODE      ,"EM_GETWORDWRAPMODE", 0 }, // (WM_USER + 103)
   { EM_SETIMECOLOR          ,"EM_SETIMECOLOR", 0 }, // (WM_USER + 104)
   { EM_GETIMECOLOR          ,"EM_GETIMECOLOR", 0 }, // (WM_USER + 105)
   { EM_SETIMEOPTIONS        ,"EM_SETIMEOPTIONS", 0 }, // (WM_USER + 106)
   { EM_GETIMEOPTIONS        ,"EM_GETIMEOPTIONS", 0 }, // (WM_USER + 107)
   { EM_CONVPOSITION         ,"EM_CONVPOSITION", 0 }, // (WM_USER + 108)
   { EM_SETLANGOPTIONS       ,"EM_SETLANGOPTIONS", 0 }, // (WM_USER + 120)
   { EM_GETLANGOPTIONS       ,"EM_GETLANGOPTIONS", 0 }, // (WM_USER + 121)
   { EM_GETIMECOMPMODE       ,"EM_GETIMECOMPMODE", 0 }, // (WM_USER + 122)
   { EM_FINDTEXTW            ,"EM_FINDTEXTW", 0 }, // (WM_USER + 123)
   { EM_FINDTEXTEXW          ,"EM_FINDTEXTEXW", 0 }, // (WM_USER + 124)
   { EM_RECONVERSION         ,"EM_RECONVERSION", 0 }, // (WM_USER + 125)
   { EM_SETIMEMODEBIAS       ,"EM_SETIMEMODEBIAS", 0 }, // (WM_USER + 126) 
   { EM_GETIMEMODEBIAS       ,"EM_GETIMEMODEBIAS", 0 }, // (WM_USER + 127)
   { EM_SETBIDIOPTIONS       ,"EM_SETBIDIOPTIONS", 0 }, // (WM_USER + 200)
   { EM_GETBIDIOPTIONS       ,"EM_GETBIDIOPTIONS", 0 }, // (WM_USER + 201)
   { EM_SETTYPOGRAPHYOPTIONS ,"EM_SETTYPOGRAPHYOPTIONS", 0 }, // (WM_USER + 202)
   { EM_GETTYPOGRAPHYOPTIONS ,"EM_GETTYPOGRAPHYOPTIONS", 0 }, // (WM_USER + 203)
   { EM_SETEDITSTYLE         ,"EM_SETEDITSTYLE", 0 }, // (WM_USER + 204)
   { EM_GETEDITSTYLE         ,"EM_GETEDITSTYLE", 0 }, // (WM_USER + 205)
   { EM_OUTLINE              ,"EM_OUTLINE", 0 }, // (WM_USER + 220)
   { EM_GETSCROLLPOS         ,"EM_GETSCROLLPOS", 0 }, // (WM_USER + 221)
   { EM_SETSCROLLPOS         ,"EM_SETSCROLLPOS", 0 }, // (WM_USER + 222)
   { EM_SETFONTSIZE          ,"EM_SETFONTSIZE", 0 }, // (WM_USER + 223)
   { EM_GETZOOM              ,"EM_GETZOOM", 0 }, // (WM_USER + 224)
   { EM_SETZOOM              ,"EM_SETZOOM", 0 }, // (WM_USER + 225)
   { 0,                        0,           0 }
};

// notify messages 
WMSTR sRENotify[] = {
   // A rich edit control sends the following notification messages
   // only if they have been enabled by using the EM_SETEVENTMASK message.
   { EN_CORRECTTEXT,     "EN_CORRECTTEXT", 0 },
   { EN_DRAGDROPDONE,    "EN_DRAGDROPDONE", 0 },
   { EN_DROPFILES,       "EN_DROPFILES", 0 },
   { EN_IMECHANGE,       "EN_IMECHANGE", 0 },
   { EN_LINK,            "EN_LINK", 0 },
   { EN_MSGFILTER,       "EN_MSGFILTER", 0 },
   { EN_OBJECTPOSITIONS, "EN_OBJECTPOSITIONS", 0 },
   { EN_PROTECTED,       "EN_PROTECTED", 0 },
   { EN_REQUESTRESIZE,   "EN_REQUESTRESIZE", 0 },
   { EN_SELCHANGE,       "EN_SELCHANGE", 0 },
   // The following notification messages are always enabled
   // and do not depend on the EM_SETEVENTMASK message.
   { EN_ALIGNLTR,        "EN_ALIGNLTR", 0 },
   { EN_ALIGNRTL,        "EN_ALIGNRTL", 0 },
   { EN_OLEOPFAILED,     "EN_OLEOPFAILED", 0 },
   { EN_SAVECLIPBOARD,   "EN_SAVECLIPBOARD", 0 },
   { EN_STOPNOUNDO,      "EN_STOPNOUNDO", 0 },
   { 0,                  0,              0 }
};

WMSTR sREEventMask[] = {
   { ENM_CHANGE, "ENM_CHANGE", 0 }, // Sends EN_CHANGE notifications.
   { ENM_CORRECTTEXT, "ENM_CORRECTTEXT", 0 },   // Sends EN_CORRECTTEXT notifications.
   { ENM_DRAGDROPDONE, "ENM_DRAGDROPDONE", 0 }, // Sends EN_DRAGDROPDONE notifications.
   { ENM_DROPFILES, "ENM_DROPFILES", 0 }, // Sends EN_DROPFILES notifications.
   { ENM_IMECHANGE, "ENM_IMECHANGE", 0 }, // Rich Edit 1.0 only: Sends EN_IMECHANGE
//notifications when the IME conversion status has changed. Only
//for Asian-language versions of the operating system.
   { ENM_KEYEVENTS, "ENM_KEYEVENTS", 0 }, // Sends EN_MSGFILTER notifications for keyboard
//events. 
   { ENM_LINK, "ENM_LINK", 0 }, // Rich Edit 2.0 and later: Sends EN_LINK notifications
//when the mouse pointer is over text that has the CFE_LINK and
//one of several mouse actions is performed.
   { ENM_MOUSEEVENTS, "ENM_MOUSEEVENTS", 0 }, // Sends EN_MSGFILTER notifications for mouse
//events.
   { ENM_OBJECTPOSITIONS, "ENM_OBJECTPOSITIONS", 0 }, // Sends EN_OBJECTPOSITIONS notifications.
   { ENM_PROTECTED, "ENM_PROTECTED", 0 }, // Sends EN_PROTECTED notifications.
   { ENM_REQUESTRESIZE, "ENM_REQUESTRESIZE", 0 }, // Sends EN_REQUESTRESIZE notifications.
   { ENM_SCROLL, "ENM_SCROLL", 0 }, // Sends EN_HSCROLL and EN_VSCROLL notifications.
   { ENM_SCROLLEVENTS, "ENM_SCROLLEVENTS", 0 }, // Sends EN_MSGFILTER notifications for mouse
//wheel events. 
   { ENM_SELCHANGE, "ENM_SELCHANGE", 0 }, // Sends EN_SELCHANGE notifications.
   { ENM_UPDATE, "ENM_UPDATE", 0 }, // Sends EN_UPDATE notifications. 
//Rich Edit 2.0 and later: this flag is ignored and the EN_UPDATE
//notifications are always sent. However, if Rich Edit 3.0
//emulates Rich Edit 1.0, you must use this flag to send EN_UPDATE
//notifications..
   { 0,           0,           0 }
};


////////////////////////////////////////////////////////
// FUNCTION   :  GetREEvMask
// Description:
// Return type: LPTSTR
// Argument   : DWORD dwEvMask
////////////////////////////////////////////////////////
LPTSTR   GetREEvMask( DWORD dwEvMask )
{
   static TCHAR _s_evmsk[264];
   LPTSTR   lpr = &_s_evmsk[0];
   PWMSTR   lpm = &sREEventMask[0];
   LPTSTR   lps;
   DWORD    dwe;

   *lpr = 0;   // start with NOTHING
   dwe = dwEvMask;
   if(dwe)
   {
      lps = lpm->w_lpStg;
      while( lps )
      {
         if( dwe & lpm->w_uiMsg )
         {
            if( *lpr )
               strcat(lpr, "|");
            strcat(lpr,lps);
            dwe &= ~( lpm->w_uiMsg );
            if(dwe == 0)
               break;
         }
         lpm++;   // bump to NEXT
         lps = lpm->w_lpStg;
      }
   }
   return lpr; // return pointer to accumulated string
}

#endif   // _RICHEDIT_

///*
// * Edit Control Notification Codes
// */
//#define EN_SETFOCUS         0x0100   (256)
//#define EN_KILLFOCUS        0x0200   (512)
//#define EN_CHANGE           0x0300   (768)
//#define EN_UPDATE           0x0400   (1024)
//#define EN_ERRSPACE         0x0500   (1280)
//#define EN_MAXTEXT          0x0501   (1281)
//#define EN_HSCROLL          0x0601   (1537)
//#define EN_VSCROLL          0x0602   (1538)
// AND FROM richedit.h
///* New notifications */
//#define EN_MSGFILTER                    0x0700 (1792)
//#define EN_REQUESTRESIZE                0x0701 (1793)
//#define EN_SELCHANGE                    0x0702 (1794)
//#define EN_DROPFILES                    0x0703 (1794)
//#define EN_PROTECTED                    0x0704 (1795)
//#define EN_CORRECTTEXT                  0x0705 (1796)
//#define EN_STOPNOUNDO                   0x0706 (1797)
//#define EN_IMECHANGE                    0x0707 (1798)
//#define EN_SAVECLIPBOARD                0x0708 (1799)
//#define EN_OLEOPFAILED                  0x0709 (1800)
//#define EN_OBJECTPOSITIONS              0x070a (1801)
//#define EN_LINK                         0x070b (1802)
//#define EN_DRAGDROPDONE                 0x070c (1803)
//#define EN_PARAGRAPHEXPANDED            0x070d (1804)
///* BiDi specific notifications */
//#define EN_ALIGNLTR                     0x0710 (1808)
//#define EN_ALIGNRTL                     0x0711 (1809)
//
// Edit Control Notification Messages
// The user makes editing requests by using the keyboard and mouse.
// The system sends each request to the edit control's parent window
// in the form of a WM_COMMAND message. The message includes the
// edit control identifier in the low-order word of the wParam parameter,
// the handle of the edit control in the lParam parameter,
// and an edit control notification message corresponding to
// the user's action in the high-order word of the wParam parameter. 
// The following table lists each edit control notification message
WMSTR sEdNote[] = {
   { EN_CHANGE,    "EN_CHANGE", 0 }, // The user has modified text in an edit control.
      // The system updates the display before sending this message (unlike EN_UPDATE).
   { EN_ERRSPACE,  "EN_ERRSPACE", 0 },  // The edit control cannot allocate enough
      // memory to meet a specific request.
   { EN_HSCROLL,   "EN_HSCROLL", 0 }, // The user has clicked the edit control's
      // horizontal scroll bar. The system sends this message before updating the screen.
   { EN_KILLFOCUS, "EN_KILLFOCUS", 0 }, // The user has selected another control.
   { EN_MAXTEXT,   "EN_MAXTEXT", 0 }, // While inserting text,
      // the user has exceeded the specified number of characters for the edit control.
      // Insertion has been truncated. This message is also sent either
      // when an edit control does not have the ES_AUTOHSCROLL style and
      // the number of characters to be inserted exceeds the width of the
      // edit control or when an edit control does not have the ES_AUTOVSCROLL style
      // and the total number of lines to be inserted exceeds the height of the edit control.
   { EN_SETFOCUS,  "EN_SETFOCUS", 0 },  // The user has selected this edit control.
   { EN_UPDATE,    "EN_UPDATE", 0 },   // The user has altered the text in the edit control
      // and the system is about to display the new text. The system sends this message
      // after formatting the text, but before displaying it, so that
      // the application can resize the edit control window.
   { EN_VSCROLL,   "EN_VSCROLL", 0 }, // The user has clicked the edit control's
      // vertical scroll bar or has scrolled the mouse wheel over the edit control.
      // The system sends this message before updating the screen.
   { 0,           0,           0 }
};


WMSTR sTb[] = {
	{ TB_ENABLEBUTTON         ,"TB_ENABLEBUTTON", 0 }, // (WM_USER + 1)
	{ TB_CHECKBUTTON          ,"TB_CHECKBUTTON", 0 }, // (WM_USER + 2)
	{ TB_PRESSBUTTON          ,"TB_PRESSBUTTON", 0 }, // (WM_USER + 3)
	{ TB_HIDEBUTTON           ,"TB_HIDEBUTTON", 0 }, // (WM_USER + 4)
	{ TB_INDETERMINATE        ,"TB_INDETERMINATE", 0 }, // (WM_USER + 5)
#if (_WIN32_IE >= 0x0400)
	{ TB_MARKBUTTON           ,"TB_MARKBUTTON", 0 }, // (WM_USER + 6)
#endif                                                      
	{ TB_ISBUTTONENABLED      ,"TB_ISBUTTONENABLED", 0 }, // (WM_USER + 9)
	{ TB_ISBUTTONCHECKED      ,"TB_ISBUTTONCHECKED", 0 }, // (WM_USER + 10)
	{ TB_ISBUTTONPRESSED      ,"TB_ISBUTTONPRESSED", 0 }, // (WM_USER + 11)
	{ TB_ISBUTTONHIDDEN       ,"TB_ISBUTTONHIDDEN", 0 }, // (WM_USER + 12)
	{ TB_ISBUTTONINDETERMINATE ,"TB_ISBUTTONINDETERMINATE", 0 }, // (WM_USER + 13)
#if (_WIN32_IE >= 0x0400)
	{ TB_ISBUTTONHIGHLIGHTED  ,"TB_ISBUTTONHIGHLIGHTED", 0 }, // (WM_USER + 14)
#endif                                                      
	{ TB_SETSTATE             ,"TB_SETSTATE", 0 }, // (WM_USER + 17)
	{ TB_GETSTATE             ,"TB_GETSTATE", 0 }, // (WM_USER + 18)
	{ TB_ADDBITMAP            ,"TB_ADDBITMAP", 0 }, // (WM_USER + 19)
#if (_WIN32_IE >= 0x0400)
	{ TB_ADDBUTTONSA          ,"TB_ADDBUTTONSA", 0 }, // (WM_USER + 20)
	{ TB_INSERTBUTTONA        ,"TB_INSERTBUTTONA", 0 }, // (WM_USER + 21)
#else                                                       
	{ TB_ADDBUTTONS           ,"TB_ADDBUTTONS", 0 }, // (WM_USER + 20)
	{ TB_INSERTBUTTON         ,"TB_INSERTBUTTON", 0 }, // (WM_USER + 21)
#endif                                                      
	{ TB_DELETEBUTTON         ,"TB_DELETEBUTTON", 0 }, // (WM_USER + 22)
	{ TB_GETBUTTON            ,"TB_GETBUTTON", 0 }, // (WM_USER + 23)
	{ TB_BUTTONCOUNT          ,"TB_BUTTONCOUNT", 0 }, // (WM_USER + 24)
	{ TB_COMMANDTOINDEX       ,"TB_COMMANDTOINDEX", 0 }, // (WM_USER + 25)
	{ TB_SAVERESTOREA         ,"TB_SAVERESTOREA", 0 }, // (WM_USER + 26)
	{ TB_SAVERESTOREW         ,"TB_SAVERESTOREW", 0 }, // (WM_USER + 76)
	{ TB_CUSTOMIZE            ,"TB_CUSTOMIZE", 0 }, // (WM_USER + 27)
	{ TB_ADDSTRINGA           ,"TB_ADDSTRINGA", 0 }, // (WM_USER + 28)
	{ TB_ADDSTRINGW           ,"TB_ADDSTRINGW", 0 }, // (WM_USER + 77)
	{ TB_GETITEMRECT          ,"TB_GETITEMRECT", 0 }, // (WM_USER + 29)
	{ TB_BUTTONSTRUCTSIZE     ,"TB_BUTTONSTRUCTSIZE", 0 }, // (WM_USER + 30)
	{ TB_SETBUTTONSIZE        ,"TB_SETBUTTONSIZE", 0 }, // (WM_USER + 31)
	{ TB_SETBITMAPSIZE        ,"TB_SETBITMAPSIZE", 0 }, // (WM_USER + 32)
	{ TB_AUTOSIZE             ,"TB_AUTOSIZE", 0 }, // (WM_USER + 33)
	{ TB_GETTOOLTIPS          ,"TB_GETTOOLTIPS", 0 }, // (WM_USER + 35)
	{ TB_SETTOOLTIPS          ,"TB_SETTOOLTIPS", 0 }, // (WM_USER + 36)
	{ TB_SETPARENT            ,"TB_SETPARENT", 0 }, // (WM_USER + 37)
	{ TB_SETROWS              ,"TB_SETROWS", 0 }, // (WM_USER + 39)
	{ TB_GETROWS              ,"TB_GETROWS", 0 }, // (WM_USER + 40)
	{ TB_SETCMDID             ,"TB_SETCMDID", 0 }, // (WM_USER + 42)
	{ TB_CHANGEBITMAP         ,"TB_CHANGEBITMAP", 0 }, // (WM_USER + 43)
	{ TB_GETBITMAP            ,"TB_GETBITMAP", 0 }, // (WM_USER + 44)
	{ TB_GETBUTTONTEXTA       ,"TB_GETBUTTONTEXTA", 0 }, // (WM_USER + 45)
	{ TB_GETBUTTONTEXTW       ,"TB_GETBUTTONTEXTW", 0 }, // (WM_USER + 75)
	{ TB_REPLACEBITMAP        ,"TB_REPLACEBITMAP", 0 }, // (WM_USER + 46)
#if (_WIN32_IE >= 0x0300)
	{ TB_SETINDENT            ,"TB_SETINDENT", 0 }, // (WM_USER + 47)
	{ TB_SETIMAGELIST         ,"TB_SETIMAGELIST", 0 }, // (WM_USER + 48)
	{ TB_GETIMAGELIST         ,"TB_GETIMAGELIST", 0 }, // (WM_USER + 49)
	{ TB_LOADIMAGES           ,"TB_LOADIMAGES", 0 }, // (WM_USER + 50)
	{ TB_GETRECT              ,"TB_GETRECT", 0 }, // (WM_USER + 51) // wParam is the Cmd instead of index
	{ TB_SETHOTIMAGELIST      ,"TB_SETHOTIMAGELIST", 0 }, // (WM_USER + 52)
	{ TB_GETHOTIMAGELIST      ,"TB_GETHOTIMAGELIST", 0 }, // (WM_USER + 53)
	{ TB_SETDISABLEDIMAGELIST ,"TB_SETDISABLEDIMAGELIST", 0 }, // (WM_USER + 54)
	{ TB_GETDISABLEDIMAGELIST ,"TB_GETDISABLEDIMAGELIST", 0 }, // (WM_USER + 55)
	{ TB_SETSTYLE             ,"TB_SETSTYLE", 0 }, // (WM_USER + 56)
	{ TB_GETSTYLE             ,"TB_GETSTYLE", 0 }, // (WM_USER + 57)
	{ TB_GETBUTTONSIZE        ,"TB_GETBUTTONSIZE", 0 }, // (WM_USER + 58)
	{ TB_SETBUTTONWIDTH       ,"TB_SETBUTTONWIDTH", 0 }, // (WM_USER + 59)
	{ TB_SETMAXTEXTROWS       ,"TB_SETMAXTEXTROWS", 0 }, // (WM_USER + 60)
	{ TB_GETTEXTROWS          ,"TB_GETTEXTROWS", 0 }, // (WM_USER + 61)
#endif      // _WIN32_IE >= 0x0300
#if (_WIN32_IE >= 0x0400)
	{ TB_GETOBJECT            ,"TB_GETOBJECT", 0 }, // (WM_USER + 62)  // wParam == IID, lParam void **ppv
	{ TB_GETHOTITEM           ,"TB_GETHOTITEM", 0 }, // (WM_USER + 71)
	{ TB_SETHOTITEM           ,"TB_SETHOTITEM", 0 }, // (WM_USER + 72)  // wParam == iHotItem
	{ TB_SETANCHORHIGHLIGHT   ,"TB_SETANCHORHIGHLIGHT", 0 }, // (WM_USER + 73)  // wParam == TRUE/FALSE
	{ TB_GETANCHORHIGHLIGHT   ,"TB_GETANCHORHIGHLIGHT", 0 }, // (WM_USER + 74)
	{ TB_MAPACCELERATORA      ,"TB_MAPACCELERATORA", 0 }, // (WM_USER + 78)  // wParam == ch, lParam int * pidBtn
	{ TB_GETINSERTMARK        ,"TB_GETINSERTMARK", 0 }, // (WM_USER + 79)  // lParam == LPTBINSERTMARK
	{ TB_SETINSERTMARK        ,"TB_SETINSERTMARK", 0 }, // (WM_USER + 80)  // lParam == LPTBINSERTMARK
	{ TB_INSERTMARKHITTEST    ,"TB_INSERTMARKHITTEST", 0 }, // (WM_USER + 81)  // wParam == LPPOINT lParam == LPTBINSERTMARK
	{ TB_MOVEBUTTON           ,"TB_MOVEBUTTON", 0 }, // (WM_USER + 82)
	{ TB_GETMAXSIZE           ,"TB_GETMAXSIZE", 0 }, // (WM_USER + 83)  // lParam == LPSIZE
	{ TB_SETEXTENDEDSTYLE     ,"TB_SETEXTENDEDSTYLE", 0 }, // (WM_USER + 84)  // For TBSTYLE_EX_*
	{ TB_GETEXTENDEDSTYLE     ,"TB_GETEXTENDEDSTYLE", 0 }, // (WM_USER + 85)  // For TBSTYLE_EX_*
	{ TB_GETPADDING           ,"TB_GETPADDING", 0 }, // (WM_USER + 86)
	{ TB_SETPADDING           ,"TB_SETPADDING", 0 }, // (WM_USER + 87)
	{ TB_SETINSERTMARKCOLOR   ,"TB_SETINSERTMARKCOLOR", 0 }, // (WM_USER + 88)
	{ TB_GETINSERTMARKCOLOR   ,"TB_GETINSERTMARKCOLOR", 0 }, // (WM_USER + 89)
	{ TB_MAPACCELERATORW      ,"TB_MAPACCELERATORW", 0 }, // (WM_USER + 90)  // wParam == ch, lParam int * pidBtn
#endif  // _WIN32_IE >= 0x0400
	{ TB_GETBITMAPFLAGS       ,"TB_GETBITMAPFLAGS", 0 }, // (WM_USER + 41)
#if (_WIN32_IE >= 0x0400)
// BUTTONINFO APIs do NOT support the string pool.
	{ TB_GETBUTTONINFOW       ,"TB_GETBUTTONINFOW", 0 }, // (WM_USER + 63)
	{ TB_SETBUTTONINFOW       ,"TB_SETBUTTONINFOW", 0 }, // (WM_USER + 64)
	{ TB_GETBUTTONINFOA       ,"TB_GETBUTTONINFOA", 0 }, // (WM_USER + 65)
	{ TB_SETBUTTONINFOA       ,"TB_SETBUTTONINFOA", 0 }, // (WM_USER + 66)
	{ TB_INSERTBUTTONW        ,"TB_INSERTBUTTONW", 0 }, // (WM_USER + 67)
	{ TB_ADDBUTTONSW          ,"TB_ADDBUTTONSW", 0 }, // (WM_USER + 68)
	{ TB_HITTEST              ,"TB_HITTEST", 0 }, // (WM_USER + 69)
	{ TB_SETDRAWTEXTFLAGS     ,"TB_SETDRAWTEXTFLAGS", 0 }, // (WM_USER + 70)  // wParam == mask lParam == bit values
#endif  // _WIN32_IE >= 0x0400
#if (_WIN32_IE >= 0x0500)
	{ TB_GETSTRINGW           ,"TB_GETSTRINGW", 0 }, // (WM_USER + 91)
	{ TB_GETSTRINGA           ,"TB_GETSTRINGA", 0 }, // (WM_USER + 92)
#endif  // _WIN32_IE >= 0x0500
   { 0,                        0,              0 }
};


LPTSTR   GetRENPtr( UINT uMsg )
{
   LPTSTR   lps = 0;
#ifdef   _RICHEDIT_
   PWMSTR   lpm = &sRENotify[0];
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == uMsg )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
#endif   // _RICHEDIT_
   return lps;
}


LPTSTR   GetRENStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lps;
#ifdef   _RICHEDIT_
   static TCHAR _s_buf[64];
   lps = GetRENPtr(uMsg);
   if( !lps )
   {
      lps = &_s_buf[0];
      sprintf(lps, "REUNK %#x", uMsg );
   }
#else // !_RICHEDIT_
   lps = "<not enabled!>";
   UNREFERENCED_PARAMETER(uMsg);
#endif   // _RICHEDIT_
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(uType);
   return lps;
}


// and an edit control notification message corresponding to
// the user's action in the high-order word of the wParam parameter.

// only get a pointer IF known
LPTSTR   GetEDPtr( WPARAM wParam )
{
   LPTSTR   lps;
   PWMSTR   lpm = &sEdNote[0];
   UINT     ui  = HIWORD(wParam);   // get notification message
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == ui )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
   return lps;
}

static TCHAR _s_buf[64];
LPTSTR   DBGGetEdNote( UINT ui )
{
   LPTSTR   lps;
   PWMSTR   lpm = &sEdNote[0];
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == ui )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
   if( !lps )
   {
      lps = &_s_buf[0];
      sprintf(lps, "EDUNK %#x",
         ui );
   }
   return lps;
}

// always get a string pointer, even if UNKNOWN
LPTSTR   GetEDStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lps;
   lps = GetEDPtr(wParam);
   if( !lps )
   {
      lps = &_s_buf[0];
      sprintf(lps, "EDUNK %#x",
         HIWORD(wParam) );
   }
   UNREFERENCED_PARAMETER(lParam);
   UNREFERENCED_PARAMETER(uMsg);
   UNREFERENCED_PARAMETER(uType);
   return lps;
}

LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   static TCHAR _s_buf[264];
   LPTSTR   lps, lpb;
   PWMSTR   lpm = &sWm[0];
   MGHND    mh;
   lps = lpm->w_lpStg;
   while( lps )
   {
      if( lpm->w_uiMsg == uMsg )
         break;
      lpm++;
      lps = lpm->w_lpStg;
   }
   if( lps )
   {
      mh = lpm->w_pMgHnd;
      if( mh )
      {
         lpb = &_s_buf[0];
         strcpy(lpb,lps);
         mh (uType, lpb, wParam, lParam);
         lps = lpb;
      }
   }
   else  // if( !lps )
   {
      if( uType == TB_TYPE )
      {
         lpm = &sTb[0];
         lps = lpm->w_lpStg;
         while( lps )
         {
            if( lpm->w_uiMsg == uMsg )
               break;
            lpm++;
            lps = lpm->w_lpStg;
         }
         if(lps)
            goto Ret_lps;
      }
#ifdef _RICHEDIT_
      lpm = &sRe[0];
      lps = lpm->w_lpStg;
      while( lps )
      {
         if( lpm->w_uiMsg == uMsg )
            break;
         lpm++;
         lps = lpm->w_lpStg;
      }
      if( lps )
      {
         mh = lpm->w_pMgHnd;
         if( mh )
         {
            lpb = &_s_buf[0];
            strcpy(lpb,lps);
            mh (uType, lpb, wParam, lParam);
            lps = lpb;
         }
      }
      else
#endif   // _RICHEDIT_
      {
         lps = &_s_buf[0];
         if( uMsg >= WM_USER )
         {
            sprintf(lps,
               szUsr,   // "WM_USER + %d",
               ( uMsg - WM_USER ) );
         }
         else
         {
            sprintf(lps,
               szUnk,   // "UNKNOWN %#x",
               uMsg );
         }
      }
Ret_lps:
      mh = 0;
   }
   return lps;
}

BOOL  bSplMsg( UINT a )
{
   BOOL  bRet = FALSE;
   if( ( a == WM_TIMER      ) ||
       ( a == WM_DELETEITEM ) ||
       ( a == WM_ENTERIDLE  ) )
   {
      bRet = TRUE;
   }
   return bRet;
}

////////////////////////////////////////////////////////
// FUNCTION   : NotInX
// Return type: BOOL
// Argument   : UINT a
// Description: Define a set of messages to EXCLUDE
//              Well known or repeated messages are
//                likely candidates.
////////////////////////////////////////////////////////
BOOL  NotInX( UINT a )
{
   BOOL  bRet = TRUE;   // assume it is NOT in the exclude list
   if( gbShwAll && !bSplMsg(a) )
   {
      bRet = TRUE;
   }
   else if( ( gbNoDOut ) ||
      ( a == WM_TIMER )             || ( a == WM_MOUSEMOVE )        ||
      ( a == WM_NCMOUSEMOVE )       || ( a == WM_NCHITTEST )        ||
      ( a == WM_SETCURSOR )         || ( a == WM_DELETEITEM )       ||
      ( a == WM_CTLCOLORBTN )       || ( a == WM_CTLCOLORSTATIC )   ||
      ( a == WM_CTLCOLOREDIT )      || // ( a == WM_DRAWITEM )         ||
      ( a == WM_WINDOWPOSCHANGING ) || ( a == WM_WINDOWPOSCHANGED ) ||
      ( a == WM_CTLCOLORLISTBOX )   || ( a == WM_MEASUREITEM )      ||
      ( a == WM_CTLCOLORDLG )       || ( a == WM_ENTERIDLE )        ||
      ( a == CB_ADDSTRING )         || ( a == CB_SETITEMDATA )      ||
      ( a == WM_NCDESTROY )         || ( a == WM_MENUSELECT  )      )
   {
      bRet = FALSE;  // do not show this message
   }
   return bRet;
}

WMSTR sLvn[] = {
   { LVN_BEGINDRAG,     "LVN_BEGINDRAG", 0 },
   { LVN_BEGINLABELEDIT,"LVN_BEGINLABELEDIT", 0 },
   { LVN_BEGINRDRAG,    "LVN_BEGINRDRAG",      0 },
   { LVN_COLUMNCLICK,   "LVN_COLUMNCLICK",    0 },
   { LVN_DELETEALLITEMS,"LVN_DELETEALLITEMS", 0 },
   { LVN_DELETEITEM,    "LVN_DELETEITEM",     0 },
   { LVN_ENDLABELEDIT,  "LVN_ENDLABELEDIT",   0 },
   { LVN_GETDISPINFO,   "LVN_GETDISPINFO",    0 },
   { LVN_GETINFOTIP,    "LVN_GETINFOTIP",     0 },
   { LVN_INSERTITEM,    "LVN_INSERTITEM",     0 },
   { LVN_HOTTRACK,      "LVN_HOTTRACK",       0 },
   { LVN_ITEMACTIVATE,  "LVN_ITEMACTIVATE",   0 },
   { LVN_ITEMCHANGED,   "LVN_ITEMCHANGED",    0 },
   { LVN_ITEMCHANGING,  "LVN_ITEMCHANGING",   0 },
   { LVN_KEYDOWN,       "LVN_KEYDOWN",        0 },
   { LVN_MARQUEEBEGIN,  "LVN_MARQUEEBEGIN",   0 },
   { LVN_ODCACHEHINT,   "LVN_ODCACHEHINT",    0 },
   { LVN_ODFINDITEM,    "LVN_ODFINDITEM",     0 },
   { LVN_ODSTATECHANGED,"LVN_ODSTATECHANGED", 0 },
   { LVN_SETDISPINFO,   "LVN_SETDISPINFO",    0 },
   { NM_CLICK,          "NM_CLICK",           0 }, // (list view)
   { NM_CUSTOMDRAW,     "NM_CUSTOMDRAW",      0 }, // (list view)
   { NM_DBLCLK,         "NM_DBLCLK",          0 }, // (list view)
   { NM_HOVER,          "NM_HOVER",           0 }, // (list view)
   { NM_KILLFOCUS,      "NM_KILLFOCUS",       0 }, // (list view)
   { NM_RCLICK,         "NM_RCLICK",          0 }, // (list view)
   { NM_RDBLCLK,        "NM_RDBLCLK",         0 }, // (list view)
   { NM_RELEASEDCAPTURE,"NM_RELEASEDCAPTURE", 0 }, // (list view)
   { NM_RETURN,         "NM_RETURN",          0 }, // (list view)
   { NM_SETFOCUS,       "NM_SETFOCUS",        0 }, // (list view)
   { 0,                 0,                    0 }
};

/* =========
//#define HDN_ITEMCHANGINGA           (HDN_FIRST-0)
//#define HDN_ITEMCHANGINGW       (HDN_FIRST-20)
//#define HDN_ITEMCHANGEDA        (HDN_FIRST-1)
//#define HDN_ITEMCHANGEDW        (HDN_FIRST-21)
//#define HDN_ITEMCLICKA          (HDN_FIRST-2)
//#define HDN_ITEMCLICKW          (HDN_FIRST-22)
//#define HDN_ITEMDBLCLICKA       (HDN_FIRST-3)
//#define HDN_ITEMDBLCLICKW       (HDN_FIRST-23)
//#define HDN_DIVIDERDBLCLICKA    (HDN_FIRST-5)
//#define HDN_DIVIDERDBLCLICKW    (HDN_FIRST-25)
//#define HDN_BEGINTRACKA         (HDN_FIRST-6)
//#define HDN_BEGINTRACKW         (HDN_FIRST-26)
//#define HDN_ENDTRACKA           (HDN_FIRST-7)
//#define HDN_ENDTRACKW           (HDN_FIRST-27)
//#define HDN_TRACKA              (HDN_FIRST-8)
//#define HDN_TRACKW              (HDN_FIRST-28)
//#if (_WIN32_IE >= 0x0300)
//#define HDN_GETDISPINFOA        (HDN_FIRST-9)
//#define HDN_GETDISPINFOW        (HDN_FIRST-29)
   ========= */

WMSTR sLVHdr[] = {
{ HDN_BEGINDRAG,       "HDN_BEGINDRAG" }, //  (HDN_FIRST-10)
{ HDN_ENDDRAG,         "HDN_ENDDRAG"   }, //  (HDN_FIRST-11)
#if (_WIN32_IE >= 0x0500)
{ HDN_FILTERCHANGE,    "HDN_FILTERCHANGE" }, // (HDN_FIRST-12)
{ HDN_FILTERBTNCLICK,  "HDN_FILTERBTNCLICK" },  //   (HDN_FIRST-13)
#endif
{ HDN_ITEMCHANGING,    "HDN_ITEMCHANGING" },
{ HDN_ITEMCHANGED,     "HDN_ITEMCHANGED" },
{ HDN_ITEMCLICK,       "HDN_ITEMCLICK" },
{ HDN_ITEMDBLCLICK,    "HDN_ITEMDBLCLICK" },
{ HDN_DIVIDERDBLCLICK, "HDN_DIVIDERDBLCLICK" },
{ HDN_BEGINTRACK,      "HDN_BEGINTRACK" },
{ HDN_ENDTRACK,        "HDN_ENDTRACK" },
{ HDN_TRACK,           "HDN_TRACK" },
{ HDN_GETDISPINFO,     "HDN_GETDISPINFO" },
{ 0,                   0 }
};


LPTSTR   GetLVNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   PWMSTR   lpm = &sLvn[0];

   while( lpm->w_lpStg )
   {
      if( lpm->w_uiMsg == uMsg )
         return lpm->w_lpStg;
      lpm++;
   }

//#define LVN_FIRST               (0U-100U)       // listview
//#define LVN_LAST                (0U-199U)
   if(( uMsg >= LVN_LAST  ) &&
      ( uMsg <= LVN_FIRST ) )
   {
      static TCHAR _s_lvnnew[32];
      LPTSTR   plv = _s_lvnnew;
      sprintf( plv, "LVN_(%d)*N*", uMsg );
      return plv;
   }
   else if(( uMsg >= HDN_LAST ) &&
      ( uMsg <= HDN_FIRST     ) )
   {
      lpm = &sLVHdr[0];
      while( lpm->w_lpStg )
      {
         if( lpm->w_uiMsg == uMsg )
            return lpm->w_lpStg;
         lpm++;
      }

      // not yet in our list above
      {
         static TCHAR _s_lvnnew2[32];
         LPTSTR   plv = _s_lvnnew2;
         sprintf( plv, "HDN_(%d)*N*", uMsg );
         return plv;
      }
   }

   return( "<Not in list>" );
}

//Combo Box Notifications
//Messages from combo boxes are sent as notifications in the form of WM_COMMAND 
//messages. The notification message is stored in the high word of the wParam 
//parameter, and an application can process the following combo box 
//notification messages. 
//Notification message Description 
WMSTR sCBN[] = {
   { CBN_CLOSEUP,    "CBN_CLOSEUP", 0 },
   // Indicates the list in a drop-down combo box or drop-down list box 
   //is about to close. 
   { CBN_DBLCLK,     "CBN_DBLCLK",  0 },
   // Indicates the user has double-clicked a list item in a simple combo box. 
   { CBN_DROPDOWN,   "CBN_DROPDOWN",0 },
   // Indicates the list in a drop-down combo box or drop-down list 
   //box is about to open.
   { CBN_EDITCHANGE, "CBN_EDITCHANGE",0},
   // Indicates the user has changed the text in the edit control of 
   //a simple or drop-down combo box. This notification message is sent after the 
   //altered text is displayed. 
   //text in the edit control of a simple or drop-down combo box. This 
   //notification message is sent before the altered text is displayed. 
   { CBN_ERRSPACE,   "CBN_ERRSPACE", 0 },
   // Indicates the combo box cannot allocate enough memory to carry 
   //out a request, such as adding a list item. 
   { CBN_EDITUPDATE, "CBN_EDITUPDATE", 0 },
   // Indicates the user has changed the 
   { CBN_KILLFOCUS,  "CBN_KILLFOCUS",  0 },
   // Indicates the combo box is about to lose the input focus. 
   { CBN_SELCHANGE,  "CBN_SELCHANGE", 0 },
   // Indicates the current selection has changed. 
   { CBN_SELENDCANCEL,"CBN_SELENDCANCEL",0},
   // Indicates that the selection made in the drop down list, 
   //while it was dropped down, should be ignored. 
   { CBN_SELENDOK,   "CBN_SELENDOK",  0 },
   // Indicates that the selection made drop down list, while it was 
   //dropped down, should be accepted. 
   { CBN_SETFOCUS,   "CBN_SETFOCUS",  0 },
   // Indicates the combo box has received the input focus. 
   // END OF LIST
   { 0,              0,               0 }
};


LPTSTR   GetCBNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   PWMSTR   lpm = &sCBN[0];

   while( lpm->w_lpStg )
   {
      if( lpm->w_uiMsg == uMsg )
         return lpm->w_lpStg;
      lpm++;
   }

   return( "<Not in list>" );
}


// ==========================================================
#else    // NO DEBUG - Provide stubs
// ==========================================================
LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lps = "<off>";
   return lps;
}
BOOL  NotInX( UINT a )
{
   return FALSE;
}
LPTSTR   GetRENStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lps = "<disab>";
   return lps;
}

////////////////////////////////////////////////////////
// FUNCTION   :  GetEDStg
// Description: When NDEBUG this does NOTHING
// Return type: LPTSTR
// Arguments  : UINT uType
//            : UINT uMsg
//            : WPARAM wParam
//            : LPARAM lParam
////////////////////////////////////////////////////////
LPTSTR   GetEDStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lps = "<Off>";
   return lps;
}

LPTSTR   GetLVNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   return( "<Off>" );
}
LPTSTR   GetCBNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   return( "<Off>" );
}
// ==========================================================
#endif   // !NDEBUG

BOOL  bExclNone = TRUE;
BOOL IsNOTEExcl( DWORD code )
{
//   if(bExclNone)
//      return FALSE;

   switch(code)
   {
   case HDN_ITEMCLICK:
      return TRUE;
      break;
   case LVN_COLUMNCLICK:
      return TRUE;
      break;
   case NM_CUSTOMDRAW:
      return TRUE;
      break;
   case NM_HOVER:
      return TRUE;   // exclude these
      break;
   case LVN_HOTTRACK:
      return TRUE;
      break;
   case LVN_ITEMCHANGING:
      return TRUE;
      break;
   case LVN_ITEMCHANGED:
      return TRUE;
      break;
   case LVN_INSERTITEM:
      break;
   case NM_SETFOCUS:
      break;
   case NM_CLICK:
      break;
   case LVN_DELETEALLITEMS:
      return TRUE;
      break;
   case LVN_DELETEITEM:
      return TRUE;
      break;
   case LVN_GETDISPINFO:
      return TRUE;
      break;

   }

   return FALSE;
}

// WM_NOTIFY 
BOOL  excl_WM_NOTIFY( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   DWORD idCtrl = (DWORD) wParam; 
   LPNMHDR pnmh = (LPNMHDR) lParam; 
   DWORD code = pnmh->code;
   return( IsNOTEExcl(code) );
}

// eof - wmdiag.c
