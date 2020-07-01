
// dc4w.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4w_h
#define	_dc4w_h

#include "dc4wVers.h"      // establish some things BEFORE all the rest

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <commdlg.h>
#include <commctrl.h>
#include <string.h>
#include <dos.h>
#include <direct.h>
#include <memory.h>

#define  LE    LIST_ENTRY
#define  PLE   PLIST_ENTRY
extern HANDLE   ghMux;  // 0 = inactive, else is // mutex object for thread to wait for

#include "gutils.h"
#include "table.h"
#include "list.h"
#include "scandir.h"            /* needed for file.h     */
#include "file.h"               /* needed for compitem.h */
#include "compitem.h"           /* needed for view.h     */
#include "complist.h"
#include "view.h"
#include "state.h"
#include "line.h"
#include "tree.h"
#include "section.h"
#include "dc4wUtil.h"   // various utility functions
#include "dc4wDlg.h"    // just a dialog service
#include "dc4wIni.h"    // the INI file
#include "dc4wList.h"   // common double linked list
#include "dc4wTT.h"     // comstl32 4.00 TOOLTIP code
#include "dc4wTime.h"   // handle timer tick message
#include "dc4wSB.h"     // status bar window
//#include "TabCtrl\TCWork.h"   // get the COMMON fixed WORK structure
#include "dc4wWorkF.h"  // get the COMMON fixed WORK structure
#include "dc4wWork.h"   // and the allocated work structure
#include "dc4wUser.h"   // User messages (WM_USER++)
#include "dc4wZip.h"    // interface to Info-Zip modules
#include "dc4wLog.h"    // various LOGGING functions
#include "Browse.h"     // browse for file or folder
//#include "browse2.h" // browse for file or folder
#include "dc4wLV.h"     // switch to LISTVIEW, from 'table' stuff
#include "dc4wStgs.h"   // partial attempt at 'internationalization' (Internationalisation)
#include "dc4wDel1.h"   // delete LEFT ONLY selected file ...
#include "resource.h"

/* A gmem_init() heap shared by the app. call gmem get to alloc. */
extern HANDLE hHeap;

/* The instance handle for this app. Needed by anyone who uses resources
 * such as dialogs
 */

extern HWND hwndClient;
extern HWND hwndRCD;

/* global option flags-------------------------------------------  */

/* Which files do we show in outline mode ? all, changed... */
// global outline_include;

/* Do we ignore blanks during the line-by-line diff ? */
// global BOOL ignore_blanks;

/* Which line numbers do we show - left original, right original or none ?*/
// global int line_numbers;  // in dc4w.c = IDM_LNRS;

/* What lines do we show in expand mode - all, left only, right only ? */
// global int expand_mode;

/* -- display layout constants---------------------------------------*/

/* Percentage of width of window taken by bar display (when visible) */
#define BAR_WIN_WIDTH   10

/* Following are horizontal positions within the bar window, expressed
 * in percent of the width of the bar window
 */
#define L_POS_START     10      /* start of left position marker */
#define L_POS_WIDTH     5       /* width of left position marker */
#define R_POS_START     80      /* start of right position marker */
#define R_POS_WIDTH     5       /* width of right position marker */

#define L_UNMATCH_START 30      /* start of left bar for unmatched section */
#define L_UNMATCH_WIDTH 10      /* width of above */
#define R_UNMATCH_START 60      /* start of right bar for unmatch section */
#define R_UNMATCH_WIDTH 10      /* width of right unmatched section marker */
#define L_MATCH_START   30      /* start of left bar for matched section */
#define L_MATCH_WIDTH   10      /* width of left bar for matched section */
#define R_MATCH_START   60      /* start of right bar for matched section */
#define R_MATCH_WIDTH   10      /* width of right bar for matched section */

/* dc4w.c functions */
extern   void dc4w_UI(BOOL bAttach);
extern   BOOL Poll(void);                /* true if abort pending */
extern   void SetNames(LPSTR names);
extern   void SetStatus(LPSTR state);
#ifdef   ADDSTATS2   // activate another item on the status bar
extern   void SetListStats( LPTSTR lps );
#endif   // #ifdef   ADDSTATS2   // activate another item on the status bar

/* in bar.c */
extern   BOOL InitBarClass(HINSTANCE hInstance);
extern   void BarDrawPosition(HWND hwndBar, HDC hdcIn, BOOL bErase);

//#define IDM_UPDATE      39900 - NOW in resource.h - see IDR_MENU2

/* --- synchronisation ----------------------------------------- */
/*
 * In WIN32 we spawn worker threads to do time-consuming actions.
 * This causes a possible conflict with the UI thread when accessing the
 * BUSY flag.
 *
 * To protect against this we have a critical section. The UI thread
 * will get this before checking/changing the Busy flag,
 * The worker thread will get this before Busy flag* changes.
 *
 */
extern   CRITICAL_SECTION CSDc4w;
extern   int g_done_crit_section;
#define WDEnter()       if(g_done_crit_section)EnterCriticalSection(&CSDc4w);
#define WDLeave()       if(g_done_crit_section)LeaveCriticalSection(&CSDc4w);
#define WDCheck()       if(g_done_crit_section)TryEnterCriticalSection(&CSDc4w);

// some internal id's given to objects
#define IDC_RCDISP1     501
#define IDC_BAR         502   // graphic bar showing expanded view of pair of files
#define IDC_STATUS      503
#define IDL_STATLAB     504
#define IDL_NAMES       505   // [ Left tree : Right tree ]

// #define  ADDSTATS2   // activate another item on the status bar
#define IDL_LISTSTATS   506   // folder compare list information

#define  EndBuf(a)   ( a + strlen(a) )

// extracted from fc4w project
#define SET_PROP( x, y, z )  SetProp( x, MAKEINTATOM(y), z )
#define GET_PROP( x, y )     GetProp( x, MAKEINTATOM(y) )
#define REMOVE_PROP( x, y )  RemoveProp( x, MAKEINTATOM(y) )

#define  DIFF_ATOM      0x1010
#define  COPY_ATOM      0x1011
#define  SAVE_ATOM      0x1012
#define  SHOW_ATOM      0x1013

extern   BOOL  IsViewExpFile( VOID );
extern   PVIEW current_view;

// #define  MALLOC(a)   LocalAlloc( LPTR, a )
// #define  MFREE(a)    LocalFree(a)

extern   VOID  StateToString( LPTSTR lpb, int state, DWORD dwFlag );

// put 'warning' when nothing displayed, but HAVE a list of file
// or HELP if no list active - get a new list - Compare ..
extern   VOID  Paint_Warning( HDC hDC, DWORD dwType );

extern   INT   disp_number;   // count of LINES displayed
extern   BOOL  bHadPaint;     // had our PAINT message
extern   VOID  Do_FULL_REFRESH( VOID );   // redo the entire VIEW (on a thread)

/* Options for DisplayMode field indicating what is currently shown.
 * We use this to know whether or not to show the graphic bar window.
 */
#define MODE_NULL       0       /* nothing displayed */
#define MODE_OUTLINE    1       /* a list of files displayed */
#define MODE_EXPAND     2       /* view is expanded view of one file */

extern   int   DisplayMode;   // = MODE_NULL; /* indicates whether we are in expand mode */

// Do_WM_MOUSEMOVE(hwndClient, 0, pt.x, pt.y, SRC_HOTTRACK );
#define  SRC_FRAME      1  // mouse is reporting FRAME/Client WINDOW
#define  SRC_HOTTRACK   2
#define  SRC_STATUS     3
#define  SRC_UPDCLIENT  4

#endif	// _dc4w_h
// eof - dc4w.h
