
/* ***************************** Module Header *******************************
* // this is public domain software - praise me, if ok, just don't blame me!
* Module Name: dc4w.c
*
* File and directory comparisions.
*
* Functions:
*
* WinMain()
* BOOL  InitWork( VOID )
* VOID  KillWork( VOID )
* ParseArgs(argc, argv) ->  ProcessArgs( ta, argc, argv ) -> ProcessInput( ta, lpf )
* MainWndProc() main/frame hwndClient procedure
*
* dc4w_UI()
* dc4w_usage()
* Poll()
* DoResize()
* AboutBox()
* DoPrint()
* FindNextChange()
* FindPrevChange()
* ToOutline()
* ToMoved()
* do_editfile()
* do_editthread()
* SetStatus()
* SetNames()
* IsBusy()
* BusyError()
* StateToColour()
* SetSelection()
* do_gethdr()
* do_getprops()
* do_getdata()
* SvrClose()
* TableServer()
* wd_dirdialog()
* wd_copy()
* InitApplication()
* InitInstance()
* CreateTools()
* DeleteTools()
* SetBusy()
* SetNotBusy()
* SetButtonText()
* ToExpand()
* wd_initial()
*
* Comments:
*
* Compare two directories (including all files and subdirs). Look for names
* that are present in both (report all that are not). For files that
* are present in both, produce a line-by-line comparison of the differences
* between the two files (if any).
*
* Overview of internals - the whole program.
*
* dc4w is built from several modules (a "module" has a .h file
* which describes its interface and a .c file which implements it).
* Apart from THIS comment which tries to give an overview of the whole
* scheme of things, each module is as self-contained as possible.
* This is enforced by the use of opaque data types.  Modules cannot
* see each others' internal data structures.  Modules are abstract
* data types.  The term "Module" (from Modula2) and "Class" (from C++)
* are used synonymously.
*
*    Dc4w     - main program - parse arguments, put up main window,
*               handle input, calling other modules as needed
*               invoke table class to create the main display and
*               service callbacks from the table class.
*               Contains global flags for options (e.g. ignore_blanks)
*    list     - (in gutils) a generalised LIST of anything data type
*               has full set of operations for insert, delete, join etc.
*    line     - a LINE is a numbered line of text.  Information is kept to
*               allow fast comparisons of LINEs.  A LINE can hold a
*               link to another LINE.  The links are used to connect
*               lines in one file to matching lines in the other file.
*    file     - a FILEDATA represents a file as a file name in the form
*               of a DIRITEM and a LIST of LINEs
*    scandir  - a DIRITEM represents information about a file.  (for
*               instance its name, whether it has a local copy).
*    compitem - a COMPITEM is a pair of files together with information
*               on how they compare in the form of a breakdown of the
*               files into a LIST of matching or non-matching sections.
*               Either file can be absent.  This module contains the
*               file "contrast" algorithm used for the actual comparison
*    tree       (in gutils) A binary tree.  Important because it is what
*               gives the file comparison its speed as it makes it
*               an "N log N" algorithm rather than "N squared"
*    complist - a COMPLIST is the master data structure.  It has a DIRLIST
*               of the left hand files, a DIRLIST of the right hand files
*               and a LIST of COMPITEMs. The left and right hand DIRLISTs
*               are working data used to produce the COMPLIST.  The LIST
*               is displayed as the outline table.  Any given COMPITEM can
*               be displayed as an expanded item.
*    section  - a SECTION is a section of a file (first line, last line)
*               and information as to what it matches in the other file.
*    bar.c    - the picture down the left of the screen
*               has a WNDPROC.  
*    view     - Although the COMPLIST is the master state, it doesn't do
*               all the work itself.  The data is actually displayed by
*               the table class which is highly generalised.  View
*               owns a COMPLIST (and therefore calls upon the functions
*               in complist to fill it and interrogate it) and calls
*               upon (and is called back by) the functions in table to
*               actually display it.  Read about table in gutils.h
*    table.c    (in gutils) a highly generalised system for displaying
*               data in rows and columns.  The interface is in gutils.h.
*    status.c   (in gutils) the status line at the top. See gutils.h
*************************************************************************
*
* Overview of this file:
*
*   We create a table window (gutils.dll) to show the files and the
*   results of their comparisons. We create a COMPLIST object representing
*   a list of files and their differences, and a VIEW object to map between
*   the rows of the table window and the COMPLIST.
*
*   This module is responsible for creating and managing the main window,
*   placing the child windows (table, status window etc) within it, and
*   handling all menu items. We maintain global option flags set by
*   menu commands.
*
*   Creating a COMPLIST creates a list of unmatched files, and of matching
*   files that are compared with each other (these are COMPITEMS).
*   The VIEW provides a mapping between rows on the screen, and items in
*   the COMPLIST.
*
*   This version tries to maintain a responsive user interface by
*   creating worker threads to do long jobs.  This potentially creates
*   conflicts between the threads as they will both want to update common
*   variables (for instance the UI thread may be changing the options to
*   exclude identical files while the worker thread is adding in the
*   results of new comparisons).  Critical sections are used to manage
*   the conflicts.
*
*   The Edit options invoke an editor on a separate thread.  This allows
*   us to repaint our window and thereby allow the user to refer back to
*   what he saw before invoking the editor.  When he's finished editing,
*   we would of course like to refresh things and if this is still on the
*   separate thread it might clash. We avoid this clash by POSTing ourselves
*   a (WM_COMMAND, IDM_UPDATE) message.
*
*************************************************************************** */

#include "dc4w.h"
#include "wmdiag.h"
#include "dc4wHelp.h"   // command line HELP module

#undef   USEMUTEXWAIT   // no mux wait for thread

//#define  TBLSRVDBG
#undef   TBLSRVDBG
#define  MWPDBG      // output messages from MainWndProc
//#undef   MWPDBG
#undef   ONLYONEINST    // *** TBD *** support MULTIPLE instances (I hope)
#ifndef  LI
#define  LI    LARGE_INTEGER
#endif   // LI short form of 64-bit entity

extern VOID  show_help( VOID ); // FIX20060917 - add Alt+? brief help dialog - show_help()
extern   PVOID    Add2Exclude( PLE pH, LPTSTR lpb );
extern   BOOL APIENTRY DummyDllMain(HANDLE hInstance, DWORD dwReason, LPVOID reserved);
extern   COMPITEM view_getcompitem(PVIEW view);
extern   INT_PTR  Do_IDD_SAVEDIFF( PCFDLGSTR pcfds );  // was LPTSTR lpb
extern   VOID  Do_IDM_SHOWTOOLTIP( HWND hWnd );
extern   DWORD WriteDiffOut( PVIEW view, LPTSTR lpf, DWORD dwo );
//extern   PVOID Add2StgList( PLE pH, LPTSTR lpb );
extern   DWORD setToolText( int row );
extern   VOID  Do_IDM_ABOUT( HWND hWnd, PVIEW view );
#ifdef ADD_ZIP_SUPPORT
extern   VOID  ChkMissedList( VOID ); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT
extern   VOID  complist_getstats( PVOID pcfds, COMPLIST cl );  // initialize FILE LIST STATS

extern   LPTSTR   view_getfilcntstg( PVIEW view );
extern   INT      Do_IDM_PREFERENCES( HWND hWnd );
extern   BOOL     g_bNeedPaint;  // we have PAINTED a WARNING colour - once only
extern   VOID     Do_IDM_NEXTINLIST( HWND hWnd );  // go to NEXT in LIST

extern   DWORD    g_iXChar, g_iYChar;  // interger pixel width and height
extern   BOOL     g_bColourCycle;   // advise timer to put up colour display, and cycle thru colours
extern   BOOL     g_bHadPost;    // FALSE done during UI advice TRUE, before init dialog

extern   void  do_zipthread(PVIEW view, int option);
extern   DWORD       g_dwviewedcnt;
extern   long  Do_WM_INITMENUPOPUP( HWND hWnd, WPARAM wParam, LPARAM lParam );
extern   BOOL  bAutoZip;   // def = TRUE; to write LIST text
// extern   BOOL  g_bWriteAZip;  // def = FALSE; to launch command, and WAIT...
#ifdef ADD_ZIP_SUPPORT
extern   INT  WriteZIPList( PTHREADARGS pta, DWORD dwt ); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT

extern   DWORD GetOutlineCount( VOID );
extern   CC    g_sOutCnt;
extern   BOOL  g_bNoAZip;  // = FALSE; -n option = NO ZIP
extern   BOOL  g_bZipAll;  // = FALSE; -na option = ZIP ALL
extern   VOID  Do_WM_SIZE( HWND hWnd, WPARAM wParam, LPARAM lParam ); // see dc4wSize.c
extern   VOID  Do_WM_MOUSEMOVE( HWND hWnd, WPARAM wParam, DWORD xPos, DWORD yPos, DWORD src );
extern   RECT  g_rcSizeBar;
extern   BOOL  Do_WM_NCHITTEST( HWND hWnd, WPARAM wParam, INT x, INT y, LRESULT lRes );
extern   BOOL  tb_WM_RBUTTONDOWN( HWND hwnd, WPARAM wParam, LPARAM lParam );
//   case WM_LBUTTONDOWN,    case WM_LBUTTONUP:
extern   VOID  Do_WM_LBUTTONDOWN( HWND hWnd, WPARAM wParam, LPARAM lParam );
extern   VOID  Do_WM_LBUTTONUP( HWND hWnd, WPARAM wParam, LPARAM lParam );

#ifdef ADD_LIST_VIEW
//        POPUP "&List View Control"
//            MENUITEM "&Off",                        IDM_LISTVIEWOFF
//   case IDM_LISTVIEWOFF:
extern   VOID  Do_IDM_LISTVIEWOFF(VOID);
//            MENUITEM "&50%",                        IDM_LISTVIEW50
//   case IDM_LISTVIEW50:
extern   VOID  Do_IDM_LISTVIEW50(VOID);
//            MENUITEM "&100%",                       IDM_LISTVIEW100
//   case IDM_LISTVIEW100:
extern   VOID  Do_IDM_LISTVIEW100(VOID);
extern   VOID  Do_IDM_HOVERSEL( HWND hWnd );
extern   VOID  Do_IDM_LVADDGRID( HWND hWnd );
#endif // #ifdef ADD_LIST_VIEW

extern INT Do_IDM_EXCLUDEREPOS(VOID);

extern   DWORD g_dwActSecs;   // = 10;    // standard delay of timed text message

/*--constants and data types--------------------------------------------*/

/* When we print the current table, we pass this id as the table id
 * When we are queried for the properties of this table, we know they
 * want the printing properties for the current view. We use this to
 * select different fonts and colours for the printer.
 */
#define TABID_PRINTER   1

#define  BT_FILEDLG     1
#define  BT_DIRDLG      2
#define  BT_REFRESH     3
#define  BT_COPYFILES   4
#define  BT_INITIAL     5
#define  BT_WRITEDIFF   6
#define  BT_DELETEFILES 7

//typedef struct tagCMDLN {
//   TCHAR    szCmd[264];
//   TCHAR    szSws[264];
//   TCHAR    szZip[264];
//   TCHAR    szInp[264];
//   TCHAR    szEnv[264];
//   TCHAR    szCmp[...];
//}CMDLN, * PCMDLN;

/* Structure containing all the arguments we'd like to give to do_editfile
   Need a structure because CreateThread only allows for one argument. */
//typedef struct {
//   PVIEW    view;
//   int      option;
//   int      selection;
//   int      onviewcnt;
//   BOOL     bDoMB;   // put up a message box if error
//   DWORD    dwError; // error indication
//   LPTSTR   pCmd;
//   COMPLIST cl;   // = view_getcomplist(current_view);
//   CMDLN    sCmdLn;  // various components of the command line
//   TCHAR    cCmdLine[1024];
//   TCHAR    cErrMsg[264];
//} EDITARGS, * PEDITARGS;

LONG do_editfile(PEDITARGS pe);

char gszRegRun[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

HWND     g_hWnd;  // main window - also see g_hClient - WM_CREATE
DWORD    threadid;
INT      disp_number = (INT)-1;   // count of LINES displayed
BOOL     bHadPaint = FALSE;
BOOL     gbInInitial = FALSE;
INT      g_iThrdRun = 0;   // incremented when thread to process argument has begun

TCHAR    g_szBarClass[] = "DC4WBARCLASS";

BOOL  _afxForceEXCLUDE = FALSE;

BOOL  g_bFileCopy = FALSE;  // set the do-it-quick flag

// text output to main window
INT   g_iYChar8 = 8; // = (int)(ptm->tmHeight + ptm->tmExternalLeading);
INT   g_iXChar8 = 8; // = (int)ptm->tmAveCharWidth;
// if( g_hFixedFont ) { ptm = &g_sFF12; GetTextMetrics(hdc, ptm);
INT   g_iYChar12 = 12;  // (int)(ptm->tmHeight + ptm->tmExternalLeading);
INT   g_iXChar12 = 12;  // (int)ptm->tmAveCharWidth;
TEXTMETRIC g_sFF8, g_sFF12;   // with FONT hdc width, height, etc ...

DWORD g_iYChar = 12; // = (int)(ptm->tmHeight + ptm->tmExternalLeading);
DWORD g_iXChar = 12; // = (int)ptm->tmAveCharWidth;
// resolution
DWORD gcxScreen = 0;
DWORD gcyScreen = 0;

int g_done_crit_section = 0;

#ifndef ADD_LIST_VIEW
//TCHAR    g_szSzFm[] = "%10I64u ";
TCHAR    g_szSzFm[] = "%8I64u";
#endif // #ifndef ADD_LIST_VIEW

/*---- colour scheme------------------------------- */
/* outline - all global */
//    rgb_outlinehi = RGB(255, 0, 0);        == old and new and size = different files ==

/* expand view */
//    rgb_leftfore =   RGB(  0,   0,   0);   == foregrnd for left lines ==
//      rgb_leftback  =  RGB(255,   0,   0); == backgrnd for left lines ==
//    rgb_leftback  =  RGB(200,  80,   0);   == backgrnd for left lines ==
//    rgb_rightfore =  RGB(  0,   0,   0);   == foregrnd for right lines==
//    rgb_rightback =  RGB(255, 255,   0);   == backgrnd for right lines==
/* moved lines */
//      rgb_mleftfore =  RGB(  0,   0, 128); == foregrnd for moved-left ==
//      rgb_mleftback =  RGB(255,   0,   0); == backgrnd for moved-left ==
//      rgb_mrightfore = RGB(  0,   0, 255); == foregrnd for moved-right==
//      rgb_mrightback = RGB(255, 255,   0); == backgrnd for moved-right==
// new moved colors
//    rgb_mleftfore =  RGB(  0, 255, 128);   == foregrnd for moved-left ==
//    rgb_mleftback =  RGB(128, 128, 128);   == backgrnd for moved-left ==

//    rgb_mrightfore = RGB(  0, 255, 255);   == foregrnd for moved-right==
//    rgb_mrightback = RGB(128, 128, 128);   == backgrnd for moved-right==
/* bar window */
//    rgb_barleft =    RGB(255,   0,   0);   == bar sections in left only  ==
//    rgb_barright =   RGB(255, 255,   0);   == bar sections in right only ==
//    rgb_barcurrent = RGB(  0,   0, 255);   == current pos markers in bar ==

CRITICAL_SECTION CSDc4w;

/* module static data -------------------------------------------------*/

/* current value of status window title - usually "left : right" folders */
char AppTitle[264];

HWND hwndClient = 0;        /* main window */
HWND hwndRCD    = 0;           /* table window */
HWND hwndStatus;        /* status bar across top */
HWND hwndBar;           /* graphic of sections as vertical bars */
BOOL  gbDnInit  = FALSE; // into message loop
//RECT  grcSize;
//BOOL  bSzValid  = FALSE;
//BOOL  bChgSz    = FALSE;
//BOOL  IsZoom    = FALSE;
//BOOL  bChgZm    = FALSE;
//BOOL  IsIcon    = FALSE;
//BOOL  bChgIc    = FALSE;
HACCEL haccel;

/*
 * The status bar told us it should be this high. Rest of client area
 * goes to the hwndBar and hwndRCD.
 */
int      status_height;
HMENU    hMenu;    /* handle to menu for hwndClient */
int      nMinMax = SW_SHOWNORMAL;         /* default state of window normal */

/* The message sent to us as a callback by the table window needs to be
 * registered - table_msgcode is the result of the RegisterMessage call
 * BUT now that the DLL and the Table are one and the same application
 * this is now a WM_USER++ type.
 */
UINT     table_msgcode;
/* True if we are currently doing some scan or comparison.
 * Must get critical section before checking/changing this (call
 * SetBusy.
 */
BOOL     gfBusy = FALSE;
INT      giSelection = -1;     /* selected row in table*/

/* Options for DisplayMode field indicating what is currently shown.
 * We use this to know whether or not to show the graphic bar window.
 */
//#define MODE_NULL       0       /* nothing displayed */
//#define MODE_OUTLINE    1       /* a list of files displayed */
//#define MODE_EXPAND     2       /* view is expanded view of one file */

int   DisplayMode = MODE_NULL;    /* indicates whether we are in expand mode */

PVIEW  current_view = NULL;

/* command line parameters */
extern int __argc;
extern char ** __argv;

BOOL  bAbort = FALSE;    /* set to request abort of current operation */

// global TCHAR editor_cmdline[264] = { "notepad %p" };  /* editor cmdline */
  /* slick version is "s %p -#%l" */

/* app-wide global data --------------------------------------------- */

/* Handle returned from gmem_init - we use this for all memory allocations */
HANDLE   hHeap = 0;

/* Current state of menu options */
// int   line_numbers = IDM_LNRS;
//  #define  INCLUDE_ALLDIFF   (INCLUDE_DIFFER|INCLUDE_NEWER|INCLUDE_OLDER)

// int   outline_include = INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY|INCLUDE_SAME|INCLUDE_ALLDIFF;
// BOOL  picture_mode = TRUE;

BOOL     bCopyDeep;
LOGFONT  g_sLogFont, g_sLF8bold;
HFONT    g_hFF8bold    = 0;   // fixed font 8 bold FONT
HFONT    g_hFixedFont  = 0;
HFONT    g_hFixedFont8 = 0;   // LOGFONT (was 8 points)
HFONT    g_hActFont = 0;
HFONT    g_hfCN8 = 0;

BOOL     gbDiffAllSw = FALSE; // how the ALL menu toggles
BOOL  g_bNoInvert = TRUE;  // to invert LISTVIEW colouring, but NOT too good??!!
BOOL  g_bNoResize = FALSE; // no re-sizing done in ToOutline()

//sprintf(pbuf, "Hot Select = [%s]", fname );
TCHAR g_szHotSel[] = "Hot Select";  // sprintf(pbuf, "%s = [%s]", g_szHotSel, fname );

BOOL  g_bInExit = FALSE;
BOOL  g_bLVHasFocus = FALSE;   // show (globally) that the keyboard is here!

FIXEDWORK	sFW;  // fixed work structure

PWORKSTR	   pW;   // allocated work structure

#define COLOR_SCALE_RED     1
#define COLOR_SCALE_GREEN   2
#define COLOR_SCALE_BLUE    3
#define COLOR_SCALE_GRAY    4

int   giCurColor = COLOR_SCALE_RED;

/* function prototypes --- forward references */
BOOL  InitApplication(HINSTANCE hInstance);
BOOL  InitInstance(HINSTANCE hInstance, int nCmdShow);
void  CreateTools(void);
void  DeleteTools(void);
VOID  DeleteFonts( VOID );
LRESULT APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL  SetBusy( UINT uType );
void  SetNotBusy(void);
void  SetSelection(long rownr);
void  SetButtonText(LPSTR cmd);
BOOL  ToExpand(PVIEW view);
int  ParseArgs(int argc, char ** argv);
DWORD wd_initial(LPVOID arg);
void  DoPrint(void);
VOID  Paint_Warning( HDC hDC, DWORD dwType );
void  DoResize(HWND hWnd);
#ifdef ADD_DBCS_FUNCS2 // FIX20050129 - using MSVC7 .NET 2003
#ifndef  _GUTILS_H2
unsigned char * _CRTAPI1 My_mbschr( unsigned char *psz, unsigned short uiSep );
#endif   // !_GUTILS_H2
unsigned char * _CRTAPI1 My_mbsrchr( unsigned char *psz, unsigned short uiSep );
#endif // #ifdef ADD_DBCS_FUNCS2 // FIX20050129 - using MSVC7 .NET 2003

HANDLE   ghMux = 0;  // mutex object for thread to wait for
static HANDLE ghThread = NULL;
static DWORD gdwMainThreadId;     /* threadid of main (user interface) thread
                                     initialised in winmain(), thereafter constant.
                                     See dc4w_UI()
                                  */
// newer - July 2003
VOID  Do_IDM_EXCLUDE( HWND hWnd );  // currently SELECTED file ADDED to gsXFileList

// NEW
BOOL  IsViewExpFile( VOID );
INT   OptionsToString( LPTSTR lpb, BOOL bAll );
VOID  SetAppTitle( VOID );
VOID  Do_IDM_RECURSIVE( VOID );
VOID  Do_IDM_OPTEXACT( VOID );
VOID  Do_IDM_OPTALTDISPLAY( VOID );
VOID  Do_IDM_OPTADDROW( VOID );
VOID  Do_IDM_OPTIGNOREDT( VOID );
VOID  Do_IDM_OPTEXCLUDE( VOID );
extern   INT_PTR  Do_IDM_EDITEXCLUDE( VOID );
extern   INT_PTR  Do_IDM_EDITZIPUP( VOID );
extern   INT_PTR  Do_IDM_FILE_RENZIP( VOID );
#ifdef ADD_ZIP_SUPPORT
extern   INT_PTR  Do_ID_FILE_CREATEZIPFILE( VOID ); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT
VOID  Do_IDM_VIEW_NEXT( VOID );
int  ProcessArgs( PTHREADARGS ta, int argc, char * * argv );
BOOL  SendArgCopy( HINSTANCE hInstance, HWND hWnd );
BOOL  Select_Nearest( INT iSel );
LPTSTR   Cmd2Stg( UINT uid );
VOID  Do_IDM_REVERSE( VOID );
VOID  Do_IDM_REFRESH( VOID );
VOID  Do_IDM_UPDFILE( HWND hWnd );
VOID  Do_IDM_DELETEFILE( HWND hWnd );
VOID  Do_IDM_DELETEONE( HWND hWnd );
VOID  Post_Copy_Items( PCFDLGSTR pcfds, BOOL bRet, INT iWasSel );
LONG  GetCurrRow( VOID );
VOID  UpdCopyDelStrs( PCFDLGSTR pcfds );
VOID  Do_WM_COMMAND( HWND hWnd, WPARAM wParam, LPARAM lParam );

//typedef struct tagPISTR {
//   HANDLE   hFile;
//   INT      iCnt;
//   LPTSTR   pBuf;
//   LPTSTR * pArgs;
//}PISTR, * PPISTR;
//BOOL  Getpis( PPISTR ppis, LPTSTR lpf );

VOID  InitC2S( VOID )
{
#ifndef  NDEBUG
   SetC2s( &Cmd2Stg );
#endif   // !NDEBUG
}

INT  g_iAttached = 0;


void SetWindowTitle( PTSTR lpb)
{
    static TCHAR _s_szcurrentapptit[264] = { 0 };
   if( strcmp(_s_szcurrentapptit, lpb) ) {
      strcpy( _s_szcurrentapptit, lpb );
      SetWindowText(hwndClient, lpb);
   }
}

/***************************************************************************
 * Function: dc4w_UI
 *
 * Purpose:
 *
 * If you are about to put up a dialog box or in fact process input in any way
 * on any thread other than the main thread - or if you MIGHT be on a thread other
 * than the main thread, then you must call this function with TRUE before doing
 * it and with FALSE immediately afterwards.  Otherwise you will get one of a
 * number of flavours of not-very-responsiveness
 *
 *
 */
void dc4w_UI(BOOL bAttach)
{
   static BOOL g_bCycAtt = FALSE;
   static BOOL g_bSaveColCyc = FALSE;
   DWORD dwThreadId = GetCurrentThreadId();

   if(bAttach)
      g_iAttached++; //  = bAttach;  // save global
   else
   {
      if(g_iAttached)
         g_iAttached--;
   }

   if( dwThreadId == gdwMainThreadId )
   {
      // nothing to do here = return;
   }
   else
   {
      if(bAttach)
         GetDesktopWindow();

      AttachThreadInput(dwThreadId, gdwMainThreadId, bAttach);
   }

   if( bAttach )
   {
      if( !g_bCycAtt )
      {
         g_bSaveColCyc = g_bColourCycle;
         g_bColourCycle = FALSE;
         g_bCycAtt = TRUE;
      }
      g_bHadPost = FALSE;
   }
   else
   {
      if( g_bCycAtt )
      {
         g_bColourCycle = g_bSaveColCyc;
         g_bCycAtt = FALSE;
      }
   }
} /* dc4w_UI */

BOOL  g_bAddOneEx = FALSE;

BOOL  InitFixedWork( VOID )
{
   INT   i;
   //pW = LocalAlloc( LPTR, sizeof(WORKSTR) );
   //if( !pW )
   //   return FALSE;

   //ZeroMemory( pW, sizeof(WORKSTR) );
   ZeroMemory( &sFW, sizeof(FIXEDWORK) );

   //g_sCFDLGSTR.cf_dwSize = sizeof(CFDLGSTR); // set just once

   // Initialise default values (other than 0/FALSE)
#ifndef USE_GLOBAL_RECURSIVE
   gbRecur = TRUE;
#endif // #ifndef USE_GLOBAL_RECURSIVE

   InitLList( &gsCopyList );
   for( i = 0; i < MXXLSTS; i++ )
   {
      InitLList( &gsExclLists[i] );
   }
#ifdef   NOINIREAD1
   if( g_bAddOneEx )
   {
      PLE   pH = &gsExclLists[0];
      Add2Exclude( pH, "*.bak");
      Add2Exclude( pH, "*.old");
      Add2Exclude( pH, "temp*.*");
      Add2Exclude( pH, "*.obj");
      Add2Exclude( pH, "*.ncb");
      Add2Exclude( pH, "*.opt");
      bChgXLst = TRUE;
   }
#endif   // #ifdef   NOINIREAD1

   InitLList( &gsCompList );  // list of COMPARE items

   InitLList( &gsDiffList );
   InitLList( &gsFileList );
   InitLList( &gsZipList  );

   InitLList( &gsXDirsList ); // = EXCLUDE these DIRECTORIES -xd:Scenery
   InitLList( &gsXFileList ); // = EXCLUDE these FILES       -xf:temp*.*;*.old
   InitLList( &gsXMissList ); // = EXCLUDE these FILES or DIRECTORIES
   // read in from INI file, if any
   InitLList( &gsXDirsIni ); // = EXCLUDE these DIRECTORIES -xd:Scenery
   InitLList( &gsXFileIni ); // = EXCLUDE these FILES       -xf:temp*.*;*.old
   InitLList( &gsXMissIni ); // = EXCLUDE these FILES or DIRECTORIES

   InitLList( &g_sZipFiles ); // list in active ZIP

   InitLList( &g_sInFiles ); // -I<mask> or dtat\root* input file list
   InitLList( &g_sInDirs ); // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>

   // Initialise default values (other than 0/FALSE)
#ifndef USE_GLOBAL_RECURSIVE
   gbRecur = TRUE;
#endif // #ifndef USE_GLOBAL_RECURSIVE

   InitLList( &g_sZipFiles ); // list in active ZIP

   giTabSize  = 3;   //DEF_TAB_SIZE;    // was 8

   // outline  mode - file list output default options
   gdwFileOpts = INC_ALLXSM;
   gdwFileOpts &= ~(FULL_NAMES); // remove the full name

   gdwWrapWid  = DEF_WRAP_WIDTH;

   gbUseRight   = FALSE;
   // copy / update files default options
   gdwCpyOpts    = DEF_COPY_SET;   // = (INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_NEWER)
   gdwVerFlag    = DEF_VER_FLAG;   // bChgVF
   // outline_include = INC_ALLXSAME;
   //#define  INCLUDE_DIFFER2   (INCLUDE_DIFFER|INCLUDE_NEWER|INCLUDE_OLDER)
   //outline_include = INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY|INCLUDE_SAME|INCLUDE_ALLDIFF;
   // now Jan 2002 prefer default of
   outline_include = INCLUDE_LEFTONLY|INCLUDE_ALLDIFF;
   // march 2002 - add Right = Deleteable items
   outline_include |= INCLUDE_RIGHTONLY;
   // and perhaps surprisingly
   //ignore_blanks = TRUE;
   ignore_blanks = FALSE;

   // expanded mode - line display and output default options
   gdwDiffOpts = INC_ALLXSAME;   // everything excluding SAME lines

   picture_mode  = TRUE;

   //if( !VALIDLN( line_numbers ) )  {
      //   { szOpt, szLNum, it_Int,     (LPTSTR) &line_numbers, &bChgLnN, 0, 0 },
   line_numbers = IDM_LNRS;   // set default bChgLnN = FALSE;

   expand_mode  = IDM_BOTHFILES; // that is show left and right lines of EACH
   // file, extending the line display downwards

//   if( gdwDiffOpts & INCLUDE_LINENUMS )
//      gbShowNums   = TRUE;


   gwarn_text = DEF_CR_WT;    // = RGB(255,255,255)
   gwarn_back = DEF_CR_WB;    // = RGB(200,30,30)

   ghelp_text = DEF_CR_HT;    // = RGB(0,0,0)
   ghelp_back = DEF_CR_HB;    // = RGB(255,255,255)

   /* outline */
   rgb_outlinehi  = RGB(255, 0, 0);   /* different files in outline mode  */
   rgb_outlineNew = RGB(0,128, 0);   // NEWER in 'green'
   rgb_outviewBack = RGB(180,180,180); // should be system background toned down

   /* expand view */
   rgb_leftfore =   RGB(  0,   0,   0);         /* foregrnd for left lines */
   // rgb_leftback  =  RGB(255,   0,   0);         /* backgrnd for left lines */
   rgb_leftback  =  RGB(200,  80,   0);         /* backgrnd for left lines */

   rgb_rightfore =  RGB(  0,   0,   0);         /* foregrnd for right lines*/
   rgb_rightback =  RGB(255, 255,   0);         /* backgrnd for right lines*/

/* moved lines */
//DWORD rgb_mleftfore =  RGB(  0,   0, 128);         /* foregrnd for moved-left */
//DWORD rgb_mleftback =  RGB(255,   0,   0);         /* backgrnd for moved-left */
//DWORD rgb_mrightfore = RGB(  0,   0, 255);         /* foregrnd for moved-right*/
//DWORD rgb_mrightback = RGB(255, 255,   0);         /* backgrnd for moved-right*/
// new moved colors
   rgb_mleftfore =  RGB(  0, 255, 128);         /* foregrnd for moved-left */
   rgb_mleftback =  RGB(128, 128, 128);         /* backgrnd for moved-left */

   rgb_mrightfore = RGB(  0, 255, 255);         /* foregrnd for moved-right*/
   rgb_mrightback = RGB(128, 128, 128);         /* backgrnd for moved-right*/

   /* bar window */
   rgb_barleft =    RGB(255,   0,   0);         /* bar sections in left only  */
   rgb_barright =   RGB(255, 255,   0);         /* bar sections in right only */
   rgb_barcurrent = RGB(  0,   0, 255);         /* current pos markers in bar */

// IDM_DIR - put up directories DIALOG
// TCHAR szAuSel[] = "AutoSelect1";
   gbAutoSel = TRUE;
//   BOOL     fw_bChgASel;   // bChgASel

   //TCHAR szCase[] = "IgnoreCase";
   //{ szOpt, szCase, it_Bool, (LPTSTR)&
   gbIgnCase = TRUE;    // &bChgIgC, (PVOID)IDC_NOCASE, 0 },
//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
   // { szOpt, szSkipC, // = "Skip-C/C++-Comments"; it_Bool, (LPTSTR)&
   gbSkipCPP = FALSE;   // &ChgSCPP, (PVOID)IDC_SKIPCOMM, 0 },
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
   //{ szOpt, szSkipT, // "Skip-Quoted-Text";   it_Bool, (LPTSTR)&
   gbSkipTxt = FALSE;   // &bChgSTxt, (PVOID)IDC_SKIPQTXT, 0 },
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
   // { szOpt, szIgnEL, // "Ignore-Line-Termination"; it_Bool, (LPTSTR)&
   gbIgnEOL = TRUE;  // &bChgIEOL, (PVOID)IDC_IGNORETERM, 0 },
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11

   gbUseCSV = FALSE;  // output as comma delimited data
   // note: *** TBD *** add option to INI, and MENU, but must fix display first

   //strcpy( g_strTT, "This is your tooltip string." );
   //InitC2S();
//   ReadTmpINI();  // load the INI written by dc4w
   return TRUE;
}

BOOL  InitWork( VOID )
{
   INT   i;
   pW = LocalAlloc( LPTR, sizeof(WORKSTR) );
   if( !pW )
      return FALSE;

   ZeroMemory( pW, sizeof(WORKSTR) );
   //ZeroMemory( &sFW, sizeof(FIXEDWORK) );

   g_sCFDLGSTR.cf_dwSize = sizeof(CFDLGSTR); // set just once


   i = InitFixedWork();

   //strcpy( g_strTT, "This is your tooltip string." );
   InitC2S();

   // FIX20060216 - remove automatic expansion when only ONE difference!
   g_bNoExp = TRUE; // set default to NO EXPANSION use -E+ to override

   /* create any pens/brushes etc -
   ALSO Initialize Critical Section */
   CreateTools();

   return TRUE;
}

VOID  KillWork( VOID )
{
   INT   i;

   KillLList( &gsCopyList );

   for( i = 0; i < MXXLSTS; i++ )
   {
      KillLList( &gsExclLists[i] );
   }

   KillLList( &gsCompList );  // delete compared item list
   KillLList( &gsDiffList );
   KillLList( &gsFileList );
   KillLList( &gsZipList );

   KillLList( &gsXDirsList ); // = EXCLUDE these DIRECTORIES -xd:Scenery
   KillLList( &gsXFileList ); // = EXCLUDE these FILES       -xf:temp*.*;*.old
   KillLList( &gsXMissList ); // = EXCLUDE these FILES or DIRECTORIES
   // remove any read from INI
   KillLList( &gsXDirsIni ); // = EXCLUDE these DIRECTORIES -xd:Scenery
   KillLList( &gsXFileIni ); // = EXCLUDE these FILES       -xf:temp*.*;*.old
   KillLList( &gsXMissIni ); // = EXCLUDE these FILES or DIRECTORIES

   KillLList( &g_sZipFiles ); // list in active ZIP

   KillLList( &g_sInFiles ); // -I<mask> or dtat\root* input file list
   KillLList( &g_sInDirs ); // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>

//   if( g_pDifName )
//      compitem_freediffname( g_pDifName );
//   BOOL  ws_bAskCont;   // g_bAskCont  // set for timer to find
//   LPTSTR ws_lpAskMsg;  // g_lpAskMsg
   g_bAskCont = FALSE;  
   if( g_lpAskMsg )
      MFREE(g_lpAskMsg);

   if( pW )
      LocalFree(pW);

   pW = 0;
}

VOID  SetCopyOptions( PDWORD pdw )  // like say &g_sCFDLGSTR.dwCpyOpts );
{
   DWORD dwo = gdwCpyOpts & ~(INC_OUTLINE2);   // set copy options
   dwo |= outline_include &   INC_OUTLINE2;
   *pdw = dwo;
}

/***************************************************************************
 * Function: WinMain
 *
 * Purpose:
 *
 * Main entry point. Register window classes, create windows,
 * parse command line arguments and then perform a message loop
 */
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   MSG msg;

   if( !InitWork() )
   {
       msg.wParam = (WPARAM)-3;
       goto Win_Exit;
   }

   g_hInst = hInstance; // set instance handle

   gdwMainThreadId = GetCurrentThreadId();

   OpenDiagFile();

   sprtf( "dc4w began at %s"MEOR, GetDTStg() );

   InitOutList();

#ifdef   ONLYONEINST
// *****************
   hwndClient = FindWindow( APPCLASS, NULL );
   if( hwndClient )
   {
      // we want to communicate any action that our 'twin'
      // should now perform based on how the user tried to
      // execute us.
      //SendArgCopy( hInstance, hwndClient );
      if( IsIconic( hwndClient ) )
      {
         ShowWindow( hwndClient, SW_RESTORE );
      }
#ifndef  WIN32
      else
      {
         ShowWindow( hwnd, SW_SHOW );
      }
#else // WIN32
      SetForegroundWindow(hwndClient);
#endif   // WIN32 y/n
      msg.wParam = (WPARAM)-4;
      goto Win_Exit;
   }

// *****************
#endif   // #ifdef   ONLYONEINST

   InitTimers();

   ReadDc4wINI();

   SetCopyOptions( &g_sCFDLGSTR.dwCpyOpts );

   g_bComCtl4 = IsComCtl32400(); // now seeks min 4.70 for LISTVIEW

   /* create any pens/brushes etc */
   // CreateTools();
   //g_done_crit_section = 1;

   if( !DummyDllMain( hInstance, DLL_PROCESS_ATTACH, 0 ) )   // init the previous DLL code
   {
      msg.wParam = (WPARAM)-5;
      goto Win_Exit;
   }

   /* init window class unless other instances running */
   if( !hPrevInstance )
   {
       if( !InitApplication( hInstance ) )   // establish APPCLASS
       {
          msg.wParam = (WPARAM)-1;
          goto Win_Exit;
       }
   }

   /* init this instance - create all the windows */
   if( !InitInstance(hInstance, nCmdShow) )
   {
      // failed CreateWindow( APPCLASS, APPNAME, ... ) or some other FAILURE
       msg.wParam = (WPARAM)-2;
       goto Win_Exit;
   }

   GetTZI();

   if (ParseArgs(__argc, __argv)) {
       msg.wParam = (WPARAM)-3;
       goto Win_Exit;
   }

   g_done_crit_section = 1;
   gbDnInit = TRUE; // into message loop
   /* message loop */
   while( GetMessage(&msg, NULL, 0, 0) )
   {
      if( !TranslateAccelerator(hwndClient, haccel, &msg) )
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   WriteDc4wINI();

Win_Exit:

#ifdef   ADDTIMER
   if( g_uiTimerID )
      KillTimer( hwndClient, g_uiTimerID );
#endif   // ADDTIMER

   //if( hwndRCD )
   //   DestroyWindow( hwndRCD );
   KillOutList();

   gmem_freeall( hHeap );

   DummyDllMain( hInstance, DLL_PROCESS_DETACH, 0 );   // kill the previous DLL code

   sprtf( "dc4w ended at %s"MEOR, GetDTStg() );
   
   CloseDiagFile();

   return (int)(msg.wParam);
}

/***************************************************************************
 * Function: InitApplication
 *
 * Purpose:
 *
 * Register window class for the main window and the bar window.
 */
BOOL
InitApplication(HINSTANCE hInstance)
{
        WNDCLASS    wc;
        BOOL resp;

        /* register the bar window class */
        InitBarClass(hInstance);

        wc.style         = 0;
        wc.lpfnWndProc   = MainWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszClassName =  APPCLASS;
        wc.lpszMenuName  = NULL;

        resp = RegisterClass(&wc);

        return(resp);
}

/***************************************************************************
 * Function: InitInstance
 *
 * Purpose:
 *
 * Create and show the windows
 */
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   RECT      rect;
   HANDLE    hstatus;
   int       bar_width;
   RECT      childrc;

   /* initialise a heap. we use this one heap throughout
    * the app. for all memory requirements
    */
   hHeap = gmem_init( "InitInstance" );

   /* initialise the list package */
   List_Init();

   //hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
#ifdef ADD_LIST_VIEW
   hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU));
#else
   hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU2));
#endif

   if( !hMenu )
      return FALSE;

   haccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
   if( !haccel )
      return FALSE;

   if( bSzValid )
      rect = grcSize;
   else
      SetRect( &rect, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT );

   /* create the main window - see WM_CREATE in main proc 
    * also called g_hWnd = g_hParent in copy DIALOGS structure
    */
   // ****************************************************************************
   hwndClient = CreateWindow( APPCLASS,
                       APPNAME,
                       WS_OVERLAPPEDWINDOW,
                       rect.left, rect.top,       // x,y position
                       rect.right, rect.bottom,   // cx,cy size
                       NULL,
                       hMenu, // use menu from our resource
                       hInstance,
                       NULL );
   if( !hwndClient )
      return FALSE;
   // ****************************************************************************

   g_sCFDLGSTR.cf_hParent = hwndClient;   // set this in Copy Files Dialog structure

#ifdef   ADDTIMER
   g_uiTimerID = SetTimer( hwndClient, TIMER_ID, TIMER_TIC, NULL );
   if( !g_uiTimerID )
   {
      DestroyWindow( hwndClient );
      return FALSE;
   }
#endif   // ADDTIMER

   /*
    * create 3 child windows, one status, one table and one bar
    * Initially, the bar window is hidden and covered by the table.
    * later added 4th child, a LISTVIEW control
    */

   /* create a status bar window as
    * a child of the main window.
    */

   /* build a TOP status struct for two labels and an abort button */
#ifdef   ADDSTATS2
   hstatus = StatusAlloc(4);
#else // ADDSTATS2
   hstatus = StatusAlloc(3);
#endif   // #ifdef   ADDSTATS2
   if( !hstatus )
      return FALSE;

   StatusAddItem(hstatus, 0, SF_STATIC,   // type
      SF_LEFT|SF_VAR|SF_SZMIN,   // character
      IDL_STATLAB,   // id
      14,            // text width (when NULL)
      NULL );

   StatusAddItem(hstatus, 1, SF_BUTTON, SF_RIGHT|SF_RAISE, IDM_ABORT, 8,
           LoadRcString(IDS_EXIT));
   StatusAddItem(hstatus, 2, SF_STATIC, SF_LOWER|SF_LEFT|SF_VAR,
                   IDL_NAMES, 60, NULL);
#ifdef   ADDSTATS2
   StatusAddItem(hstatus, 3, SF_STATIC,   // type
      SF_LEFT|SF_VAR|SF_SZMIN,   // character
      IDL_LISTSTATS,   // id
      14,            // text width (when NULL)
      NULL );
#endif   // #ifdef   ADDSTATS2
   /* ask the status bar how high it should be for the controls
    * we have chosen, and save this value for re-sizing.
    */
   status_height = StatusHeight(hstatus);

   /*
    * create a window of this height full width of parent -
    * but this is 'corrected' during a WM_SIZE 
    */
   GetClientRect(hwndClient, &rect);
   childrc        = rect;
   childrc.bottom = status_height;
   hwndStatus = StatusCreate(g_hInst, hwndClient, (HMENU)IDC_STATUS, &childrc, hstatus);
   if( !hwndStatus )
   {
      DestroyWindow( hwndClient );
      return FALSE;
   }

   /* layout constants are stated as percentages of the window width */
   bar_width = (rect.right - rect.left) * BAR_WIN_WIDTH / 100;

   /* create the table class covering all the remaining part of
    * the main window
    */
   hwndRCD = CreateWindow(TableClassName,
                   NULL,
                   WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                   0,
                   status_height,
                   (int)(rect.right - rect.left),
                   (int)(rect.bottom - status_height),
                   hwndClient,
                   (HANDLE) IDC_RCDISP1,
                   g_hInst,
                   NULL);
   if( !hwndRCD )
   {
      DestroyWindow( hwndClient );
      return FALSE;
   }
   /* create a bar window as a child of the main window.
    * this window remains hidden until we switch into MODE_EXPAND
    */
   hwndBar = CreateWindow( g_szBarClass,  // "BarClass",
                   NULL,
                   WS_CHILD | WS_VISIBLE,
                   0,
                   status_height,
                   bar_width,
                   (int)(rect.bottom - status_height),
                   hwndClient,
                   (HANDLE) IDC_BAR,
                   g_hInst,
                   NULL);
   if( !hwndBar )
   {
      DestroyWindow( hwndClient );
      return FALSE;
   }

   /* nMinMax indicates whether we are to be minimised on startup,
    * on command line parameters
    */
   ShowWindow(hwndBar, SW_HIDE);

#ifdef ADD_LIST_VIEW
//#ifdef   ADDLISTVIEW2
   if( g_bComCtl4 )  // we have comctl32.dll 4.70, or above
   {
      g_hListView = LVCreateWindow(g_hInst, hwndClient);
      if( !g_hListView )
      {
         DestroyWindow( hwndClient );
         return FALSE;
      }

      setcolumns( g_hListView );

      Set_LV_OFF();
//      if( g_bLVOff )
      // regardless of the g_bLVOff user INI variable, start with the LISTVIEW off
//      {
//         BOOL  uval = g_bLVOff;
//         BOOL  chg = g_bChgLVOff;
//         Do_IDM_LISTVIEWOFF();
//         g_bChgLVOff = chg;
//         g_bLVOff = uval;  // restore USER's desire of the final state Off or On
//      }

   }
//#endif   // #ifdef   ADDLISTVIEW2
#endif // #ifdef ADD_LIST_VIEW

   //if( gbGotOut )
   //   SetWindowPlacement(hwndClient,&g_sWPO);
   //else
   //    ShowWindow(hwndClient, nMinMax);
   ShowWindow(hwndClient, nCmdShow);

   UpdateWindow(hwndClient);

   /* initialise busy flag and status line to show we are idle
    * (ie not comparing or scanning) */
   SetNotBusy();

   if( g_bComCtl4 )
      g_bGotTT = CreateMyTooltip( hwndRCD );

   return(TRUE);

} /* InitInstance */

int  ProcessArgs( PTHREADARGS ta, int argc, char * * argv );

/***************************************************************************
 * Function: ParseArgs
 *
 * Purpose:
 *
 * Parse command line arguments
 *
 * The user can give one or two paths. If only one, we assume the second
 * is '.' for the current directory. If one of the two paths is a directory
 * and the other a file, we compare a file of the same name in the two dirs.
 *
 * The command -s filename causes the outline list to be written to a file
 * and then the program exits. -s{slrd} filename allows selection of which
 * files are written out; by default, we assume -sld for files left and different.
 *
 * -T means tree.  Go deep.
 * -R means recursive (into subdirectories)
 *
 * The default is Deep, -L overrides and implies shallow, -T overrides -L
 */
int ParseArgs(int argc, char ** argv)
{
    int iret = 0;
   PTHREADARGS  ta;
   /* thread args can't be on the stack since the stack will change
    * before the thread completes execution
    */
   if( argc > 1 )
   {
        // assume valid arguments
#ifdef   COMBARGS
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL pointer
      PTARGS      pta   = &pcfds->cf_sTARGS;
      pta->ta_psCFDLGSTR = pcfds;
      ta = &pta->ta_sTA;   // get THREAD argument pointer
      ta->pTargs = pta;
#endif   // #ifdef   COMBARGS

      // **** ALLOCATION of thread arguments ****
      ta = (PTHREADARGS) gmem_get(hHeap, sizeof(THREADARGS), "ParseArgs" );
      if(!ta)
         return 1;
      // **** copied and FREED in wd_initial ****
      // zero everything
      ZeroMemory(ta, sizeof(THREADARGS));

      // like 
      //ta->pFirst    = NULL;
      //ta->pSecond   = NULL;
      //ta->pSaveList = NULL;
      //ta->saveopts  = 0;
      //ta->fDeep     = FALSE;  /* No -T option seen yet */
      //ta->fShall    = FALSE;   // no -L yet to force shallow

      // -N =
// FIX20051222 - Reverse logic on ZIPPING - presently defaults ON, and -N to stop
// So, keep -n to say NO ZIP, but -n+ or -na (all) will cause a ZIP to be written
//      g_bNoAZip = FALSE;
      g_bNoAZip = TRUE;
      if (ProcessArgs( ta, argc, argv ))
          return 1;

      /* set the correct depth */
#ifndef USE_GLOBAL_RECURSIVE
      if( !ta->fDeep && !ta->fShall )  /* if -L, leave it alone */
         ta->fDeep = TRUE;          /* else global default */
#endif // #ifndef USE_GLOBAL_RECURSIVE

      /* any paths to scan ? */
      if (ta->pFirst == NULL)
         return 1;

      if (ta->pSecond == NULL)
      {
         ta->pSecond = &ta->szSecond[0];  // setup local pointer
         _getcwd(ta->pSecond, 256);    // fill in the CURRENT WORK DIRECTORY
         if( ta->szSecond[0] == 0 )
            strcpy(ta->pSecond, ".");  // and use current (cwd) as second
      }

      if( ta->fReverse )
      {
         // already check first and second
         LPTSTR ptmp = &gszTmpBuf[0];
         strcpy(ptmp, ta->pSecond);
         strcpy(ta->pSecond, ta->pFirst);
         strcpy(ta->pFirst, ptmp);
      }

      SetBusy( BT_INITIAL );  // set gfBusy flag

      /* minimise the window if -s or -d flags given */
      if(( ta->pSaveList != NULL ) ||
         ( ta->pDiffList != NULL ) )
      {
         ShowWindow(hwndClient, SW_MINIMIZE);
         if( ta->pDiffList )
         {
            if( ta->diffopts == 0 )
               ta->diffopts = (INC_ALLXSM | INC_ALLMOVE);
         }
         if( ta->pSaveList )
         {
            if( ta->saveopts == 0 ) /* default to left and differ */
               ta->saveopts = (INCLUDE_LEFTONLY) | (INCLUDE_ALLDIFF);
         }
      }

      /* make an empty view */
      current_view = view_new(hwndRCD);

      DisplayMode = MODE_OUTLINE;

      ta->view = current_view;

#ifdef   USEMUTEXWAIT
      /* attempt to create a worker thread */
      ghMux = CreateMutex( NULL, // LPSECURITY_ATTRIBUTES lpMutexAttributes,  // SD
         TRUE, // BOOL bInitialOwner,      // initial owner
         NULL ); // LPCTSTR lpName          // object name

#endif   // #ifdef   USEMUTEXWAIT
#ifdef   USE_THREAD_INITIAL
      ghThread = CreateThread(NULL,
           0,
           (LPTHREAD_START_ROUTINE)wd_initial,
           (LPVOID) ta,
           0,
           &threadid);
#else // !USE_THREAD_INITIAL
      ghThread = NULL;
#endif // USE_THREAD_INITIAL

      if( ghThread == NULL )
      {
         if(ghMux)
            CloseHandle(ghMux);
         ghMux = 0;

           wd_initial( (LPVOID) ta);
      }
      else
      {
         if(ghMux == 0)
         {
            Sleep( 100 );  // give the thread / child a chance to
            // get things done quickly
         }
      }

   }
   return iret;
} /* ParseArgs */


/***************************************************************************
 * Function: CreateTools
 *
 * Purpose:
 *
 * Create any pens/brushes, and read defaults
 * from the profile file for menu settings etc.
 *
 * NOTE: From MSDN - January 2001
 *    If two different applications register the same message string, the
 * applications return the same message value. The message remains 
 * registered until the session ends. 
 *    Only use RegisterWindowMessage when more than one application must
 * process the same message. For sending private messages within a window
 * class, an application can use any integer in the range WM_USER through 0x7FFF. 
 *
 */
void
CreateTools(void)
{
   /* standard message that table class sends us for
    * notifications and queries.
    */
   //table_msgcode = RegisterWindowMessage(TableMessage);
   table_msgcode = MWM_TABLE;

   InitializeCriticalSection(&CSDc4w);

}

/***************************************************************************
 * Function: DeleteTools
 *
 * Purpose:
 *
 * Delete any fonts, pens or brushes that were created in CreateTools 
 */
void
DeleteTools(void)
{
   DeleteCriticalSection(&CSDc4w);
   // and delete fonts
   DeleteFonts();

}


/***************************************************************************
 * Function:
 *
 * Purpose:
 *
 * Check whether we have had an abort request (IDM_ABORT), and
 * return TRUE if abort requested, otherwise FALSE
 */
BOOL
Poll(void)
{
    return(bAbort);
}

/* -- menu commands ---------------------------------------------------*/

/***************************************************************************
 * Function: DoPrint
 *
 * Purpose:
 *
 * Print the current view 
 */
void
DoPrint(void)
{
        Title head, foot;
        PrintContext context;
        TCHAR szPage[50];

        /* print context contains the header and footer. Use the
         * default margins and printer selection
         */

        /* we set the table id to be TABID_PRINTER. When the table calls
         * back to get text and properties, we use this to indicate
         * that the table refered to is the 'current_view', but in print
         * mode, and thus we will use different colours/fonts.
         */
        context.head = &head;
        context.foot = &foot;
        context.margin = NULL;
        context.pd = NULL;
        context.id = TABID_PRINTER;

        /* header is filenames or just dc4w if no names known*/
        if( strlen(AppTitle) > 0 )
        {
                head.ptext = AppTitle;
        }
        else
        {
                head.ptext = APPNAME;
        }

        /* header is centred, footer is right-aligned and
         * consists of the page number
         */
        head.props.valid = P_ALIGN;
        head.props.alignment = P_CENTRE;
        lstrcpy(szPage,LoadRcString(IDS_PAGE));
        foot.ptext = (LPSTR)szPage;
        foot.props.valid = P_ALIGN;
        foot.props.alignment = P_RIGHT;

        SendMessage(hwndRCD, TM_PRINT, 0, (LPARAM) (LPSTR) &context);
}

/***************************************************************************
 * Function: FindNextChange
 *
 * Purpose: Handle menu IDM_FCHANGE
 *
 * Find the next line in the current view that is
 * not STATE_SAME. Start from the current selection, if valid, or
 * from the top of the window if no selection.
 *
 */
BOOL FindNextChange( BOOL bWarn )
{
   long crow, row;

   /* start from the selection or top of the window if no selection */
   if( giSelection >= 0 )
   {
      crow = giSelection;
   }
   else
   {
      crow = (int) SendMessage(hwndRCD, TM_TOPROW, FALSE, 0);
   }

   /* find the next 'interesting' line */
   row = view_findchange(current_view, crow, FIND_DOWN );
   if (row >= 0)
   {
      SetSelection(row);
      return(TRUE);
   }
   else
   {
           if( bWarn )
           {
              LPTSTR lpb = GetNxtBuf(); // &gszTmpBuf[0];
              INT  idiff = view_haschange( current_view, TRUE );
              strcpy( lpb, LoadRcString(IDS_NO_MORE_CHANGES) );
              sprintf( EndBuf(lpb), MEOR"There are %d 'changes'!", idiff );
              MB(hwndClient, lpb, APPNAME,
                        MB_ICONINFORMATION|MB_OK);
           }
           else
           {
              // wrap to top
              row = view_findchange(current_view, 0, FIND_DOWN );
              if( ( row >= 0 ) && ( crow != row ) )
              {
                  SetSelection(row);
                  return(TRUE);
              }
           }
   
   }

   return(FALSE);
}

/***************************************************************************
 * Function: FindPrevChange
 *
 * Purpose:
 *
 * Find the previous line in the current view that is not STATE_SAME
 */
BOOL
FindPrevChange( BOOL bWarn )
{
   long crow, row;

   /* start from the selection or top of window if no selection */
   if( giSelection >= 0 )
   {
      crow = giSelection;
        
   }
   else
   {
      crow = (int) SendMessage(hwndRCD, TM_TOPROW, FALSE, 0);
   }

   /* find the previous 'interesting' line */
   row = view_findchange(current_view, crow, FIND_UP);
   if( row >= 0 )
   {
                SetSelection(row);
                return(TRUE);
   }
   else
   {
           if( bWarn )
           {
              LPTSTR lpb = &gszTmpBuf[0];
              INT  idiff = view_haschange( current_view, TRUE );
              strcpy( lpb, LoadRcString(IDS_NO_PREV_CHANGES) );
              sprintf( EndBuf(lpb), MEOR"There are %d 'changes'!", idiff );
              MB(hwndClient, lpb, APPNAME,
                 MB_ICONINFORMATION|MB_OK);
           }
           else
           {
              // wrap to bottom, and start up
              row = view_findchange( current_view,
                 (view_getrowcount(current_view)-1), FIND_UP );
              if( ( row >= 0 ) && ( crow != row ) )
              {
                   SetSelection(row);
                   return(TRUE);
              }
           }
   }
   return(FALSE);
}


/***************************************************************************
 * Function: ToExpand
 *
 * Purpose:
 *
 * Switch to expand view of the selected line 
 * From menu IDM_EXPAND
 *
 */
BOOL ToExpand(PVIEW view)
{
   INT   row = giSelection;   // extract GLOBAL 'single' LIST SELECTION

   if( row < 0 )
      return FALSE;
   if( !view )
      return FALSE;

   g_bInExpand++; // signal we are processing a FILE expansion

   if( ( !view_isexpanded(view) ) &&
       ( !g_bNoUpdate           ) )
   {
      /* save the current outline size and position */
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);
      if( GetWindowPlacement(hwndClient,&wp) )
      {
         if( ChangedWP( &wp, &g_sWPO ) )
         {
            memcpy( &g_sWPO, &wp, sizeof(WINDOWPLACEMENT) );
            g_sWPO.length = sizeof(WINDOWPLACEMENT);
            bChgOut = TRUE;
         }
      }

      // if the user desires such SIZE / position changes *****
      // ******************************************************
      // 1 Dec 2001 - Turn this OFF and ON
      if( gbMaxExp )
      {
         /* restore the previous expanded size and position, if any */
         if( gbGotExp && ( g_sWPE.length == sizeof(WINDOWPLACEMENT) ) )
            SetWindowPlacement(hwndClient,&g_sWPE);
         else
            ShowWindow(hwndClient, SW_SHOWMAXIMIZED);
      }

   }

   /* change the view mapping to expand mode */
   if( view_expand(view, row) )
   {
           /* ok - we now have an expanded view - change status
            * to show this
            */
           DisplayMode = MODE_EXPAND;

           if( !g_bNoUpdate )
           {
              /* resize to show the graphic bar picture */
               DoResize(hwndClient);

               /* change button,status text-if we are not still busy*/
               if( !gfBusy )
               {
                  LPTSTR  lpb = GetNxtBuf();    // &gszTmpBuf[0];

                  /* the status field when we are expanded shows the
                   * tag field (normally the file name) for the
                   * item we are expanding
                   */

                  //SetStatus(view_getcurrenttag(current_view) );
                  sprintf(lpb, "%d %s ", (row + 1), view_getcurrenttag(view) );
                  SetStatus(lpb);

                  strcpy(lpb,LoadRcString(IDS_OUTLINE));
                  SetButtonText(lpb);

                  //FindNextChange(FALSE);   // goto to CHANGE (with no WARN message)
                  row = view_findchange(view, 0, FIND_DOWN);
                  if( row >= 0 )
                     SetSelection(row);

                  SetAppTitle();

               }
           }


//           {
//              LPTSTR   lpb   = &gszTmpBuf[0];
//              LPTSTR   lpb2  = &gszTmpBuf2[0];
//              LPTSTR lptstr  = &g_strTT[0];
//              LPTOOLINFO pti = &g_sTI;
//              sprintf( lpb, "Expanded view of [%s]"MEOR
//                  "with %d 'changes'."MEOR,
//                  GetRelNameStg( view_getcompitem(view) ),
//                  view_haschange( view, TRUE ) );
//              strcpy(lpb2, lptstr);
//              strcpy(lptstr, lpb ); // add new header text
//              strcat(lptstr, lpb2); // but keep all the information
//              SendMessage( g_hwndTT, TTM_UPDATETIPTEXT, 0, (LPARAM) pti );
              // for diagnostics only
//              ConditionText(lpb);   // remove 'spacey' stuff
//              sprtf("%s"MEOR,lpb);  // out as SINGLE line
//           }           

           return(TRUE);
   }
   else
   {
      chkme( MEOR"INFORMATION: Failed to EXPAND an item!!!"MEOR );
      Dec_g_bInExpand;

   }

   return(FALSE);

} /* ToExpand */

// ======= fun painting a splashy window ====== GLAZING I call it
//#define COLOR_SCALE_RED     1
//#define COLOR_SCALE_GREEN   2
//#define COLOR_SCALE_BLUE    3
//#define COLOR_SCALE_GRAY    4

COLORREF _GetNxtColr( int index, int iMax, int nColor )
{
   COLORREF cr = 0;
   switch(nColor)
   {
   case COLOR_SCALE_RED:
      cr = RGB(index,0,0);
      break;
   case COLOR_SCALE_GREEN:
      cr = RGB(0,index,0);
      break;
   case COLOR_SCALE_BLUE:
      cr = RGB(0,0,index);
      break;
   case COLOR_SCALE_GRAY:
      cr = RGB(index,index,index);
      break;
   }
   return cr;
}

//MM_ANISOTROPIC   Logical units are converted to arbitrary units
//with arbitrarily scaled axes. Setting the mapping mode to
//MM_ANISOTROPIC does not change the current window or viewport
//settings. To change the units, orientation, and scaling, call
//the SetWindowExt and SetViewportExt member functions.

//VOID PntRect1( HDC hDC, LPRECT lpRect, int idx, int nColor, int i )
VOID PntRect1( HDC hDC, LPRECT lpRect, int nColor )
{
	RECT     rc, rect;
	HBRUSH   hBrush;
   int      idx; 
   int      nMapMode;
   SIZE	   szw,szv;
   POINT	   px;

	rc = *lpRect;
//	rc.right  -= rc.left;
//	rc.bottom -= rc.top;
   nMapMode =	SetMapMode(	hDC,	MM_ANISOTROPIC );
   // sets the horizontal and vertical extents of the window 
   SetWindowExtEx(	hDC,	512,			512,			         &szw );
   // function sets the horizontal and vertical extents of the viewport 
   SetViewportExtEx(	hDC,	rc.right,	-rc.bottom + rc.top,	&szv );
   SetViewportOrgEx(	hDC,	rc.left,     rc.bottom,	         &px  );

	for( idx = 0; idx < 256; idx++ )
	{
  		SetRect( &rect,
            idx, idx,
            512 - idx, 512 - idx );
		if( hBrush = CreateSolidBrush( _GetNxtColr( idx, 256, nColor ) ) ) //RGB(0,0,idx));
		{
			FillRect( hDC, &rect, hBrush );
			DeleteObject(hBrush);
		}
	}

   SetViewportOrgEx(	hDC,	px.x,				 px.y,	NULL );
   SetViewportExtEx(	hDC,	szv.cx,	      szv.cy,	NULL );
   SetWindowExtEx(	hDC,	szw.cx,			szw.cy,	NULL );
   SetMapMode( hDC, nMapMode );

}

VOID  GlazeWindow1( HDC hdc, LPRECT lpr )
{
   if( hdc )
   {
       PntRect1( hdc, lpr, giCurColor );
       giCurColor++;    // bump to next colour
       if( giCurColor > COLOR_SCALE_GRAY )
           giCurColor = COLOR_SCALE_RED;  // wrap back to RED
   }
}

#define  MMIN_HLEN      40    // statement placed on glaze
// minimum window size
#define  MTOP_BRDR      4
#define  MMIN_HHGT      4     // greater than FOUR lines
#define  MBOT_BRDR      4

// no list chosen - paint some HELP
VOID  Paint_HELP( HDC hdc, PRECT prc )
{
   //LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb;  // = GetStgBuf();
   COLORREF fgcol, bkcol;
   HFONT    hof = 0;
   RECT     rc;
   INT      ilen;
   DWORD    dwo = ( DT_CENTER | DT_SINGLELINE | DT_VCENTER ); // text-drawing options

   GlazeWindow1( hdc, prc );  // paint with interesting glaze effect

   lpb = GetStgBuf();
   // prepare TEXT STRING
   //            1234567890123456789012345678901234567890
   strcpy( lpb, " ADVICE: Right click and go Compare ..." );

   ilen = strlen(lpb);

   rc = *prc;  // get the CLIENT size
   if( g_iXChar && g_iYChar )
   {
      INT      ix, iy;
      INT      idx, idy;

      ix = ( rc.right  - rc.left) / g_iXChar;
      iy = ( rc.bottom - rc.top ) / g_iYChar;
      if(( ix > MMIN_HLEN ) && ( ix > ilen ) &&
         ( iy > (MTOP_BRDR + MMIN_HHGT + MBOT_BRDR) ) )
      {
         static INT _s_iLstY = 0;
         static INT _s_iLstX = 0;
         _s_iLstY++;

         idy = MTOP_BRDR * g_iYChar;   // get the TOP
         rc.top = idy;  // start row for text

         //if( ( _s_iLstY + (MTOP_BRDR + MBOT_BRDR) ) >= iy )
         if( ( _s_iLstY + MBOT_BRDR ) >= iy )
            _s_iLstY = 1;

         rc.bottom = rc.top + ( _s_iLstY * g_iYChar );

         _s_iLstX++;

         // ix is characters that will fit in the line
         // and ilen is the number of characters in the message
         idx  = ix - ilen;    // remove message length
         idx  = idx / 2;      // half left / right sides

         //idx  = ( rc.right  - rc.left);
         //idx -= ( ilen * g_iXChar );      // remove LINE length
         //idx  = idx / 2;
         if( _s_iLstX > idx )
            _s_iLstX = 1;

         rc.left  = ( _s_iLstX * g_iXChar );
         rc.right = rc.left + (ilen * g_iXChar);

      }
   }

   // textout
   fgcol = SetTextColor(hdc, ghelp_text);
   bkcol = SetBkColor(  hdc, ghelp_back);

   if( g_hActFont )
      hof = SelectObject( hdc, g_hActFont );
   
   DrawText( hdc,          // handle to DC
            lpb, // text to draw
            ilen,   // text length
            &rc,  // prc,  // formatting dimensions
            dwo );   // DT_CENTER | DT_SINGLELINE | DT_VCENTER ); // text-drawing options

   SetTextColor(hdc, fgcol);
   SetBkColor(  hdc, bkcol);

   if(hof)
      SelectObject(hdc, hof);

}

VOID  Add_Using( PTSTR lpb )
{
   if( !IsListEmpty( &g_sInFiles ) ) {
      PLE ph = &g_sInFiles;
      PLE pn;
      strcat( lpb, " using"MEOR );
      strcat( lpb, "[");
      Traverse_List(ph,pn) {
         PXLST pil = (PXLST)pn;
         strcat(lpb, pil->x_szStg);
         strcat(lpb, " ");
      }
      strcat(lpb, "]");
   }
}

void Get_List_String( PTSTR lpb )
{
   // LONG  rows = view_getrowcount(current_view);
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   DWORD       tot = pcfds->dwLeft + pcfds->dwRite + pcfds->dwSame + pcfds->dwDiff;   
   sprintf( EndBuf(lpb),
         "There are %u items in total - ",
         tot );
   sprintf( EndBuf(lpb),
         "left=%u, right=%u, "
         "same=%u, and diff=%u (newer=%u)",
         pcfds->dwLeft,
         pcfds->dwRite,
         pcfds->dwSame,
         pcfds->dwDiff,
         pcfds->dwNewer );
   if( pcfds->dwUnk ) {
      sprintf( EndBuf(lpb), " And others=%d???",
            pcfds->dwUnk );
   }
}

// FIX20081125 - NEW completely IGNORE file time, but still put in FLAG
// and show TIME COMPARE statistics
void Get_List_String2( PTSTR lpb )
{
   PCFDLGSTR pcfds = &g_sCFDLGSTR;
   DWORD     tot = pcfds->dwNFTNewer + pcfds->dwNFTOlder + pcfds->dwNFTSame;   
   sprintf( EndBuf(lpb),
         "Of the %u items, File Time ",
         tot );
   sprintf( EndBuf(lpb),
         "Newer=%u, Older=%u, Same=%u",
         pcfds->dwNFTNewer,
         pcfds->dwNFTOlder,
         pcfds->dwNFTSame);
}


//#define  g_dwDiff    g_sCFDLGSTR.dwDiff   // total different
//#define  g_dwLeft    g_sCFDLGSTR.dwLeft   // total left only
//#define  g_dwRite    g_sCFDLGSTR.dwRite    // total right only
void Add_Stat_Info2( PTSTR lpb )
{
   sprintf(EndBuf(lpb),
      "Scanned %u dirs, found %u files. Left d=%u f=%u. Right d=%u f=%u",
      g_dwLeftDirs + g_dwRightDirs, g_dwLeftFiles + g_dwRightFiles,
      g_dwLeftDirs, g_dwLeftFiles, g_dwRightDirs, g_dwRightFiles);
}

// no display lines - grumble a little ;=))
VOID  Paint_Grumble( HDC hdc, PRECT prc )
{
   BOOL  bSame = FALSE;
   LPTSTR   lpb = GetNxtBuf();  // &gszTmpBuf[0];
   COLORREF fgcol, bkcol;
   int   hgt;
   RECT  rc = *prc;
   // UINT  uiForm = DT_CENTER | DT_SINGLELINE | DT_VCENTER; // text-drawing options
   UINT  uiForm = DT_CENTER | DT_VCENTER; // text-drawing options
   if(( g_dwLeft == 0 ) &&
      ( g_dwRite == 0 ) &&  // no orphanes
      ( g_dwDiff == 0 ) )
   {
      // they appear EXACTLY equal - can not GRUMBLE
      giCurColor = COLOR_SCALE_GREEN;    // set colour
      bSame = TRUE;
      //strcpy(lpb, " *** Left and Right appear the same");
      strcpy( lpb, " *** Nothing to display with options [");
      if (OptionsToString(lpb, TRUE))
          strcat(lpb, "]");
      else
          strcat(lpb, "<none>]");
      Add_Using( lpb );
      strcat( lpb, " *** " );
      // uiForm = DT_CENTER | DT_SINGLELINE; // text-drawing options
   }

   GlazeWindow1( hdc, prc );  // paint with interesting glaze effect

   if( !bSame )
   {
      // prepare TEXT STRING - was WARNING:
      strcpy( lpb, "ADVICE: With current options of [" );
      if( OptionsToString( lpb, FALSE ) )
         strcat(lpb, "]");
      else
         strcat(lpb, "<none>]");
      Add_Using( lpb );
      strcat(lpb, " there are NO display lines!" );
   }
   
   // textout
   fgcol = SetTextColor(hdc, gwarn_text);
   bkcol = SetBkColor(  hdc, gwarn_back);

   hgt = DrawText( hdc,          // handle to DC
      lpb, // text to draw
      strlen(lpb),   // text length
      prc,  // formatting dimensions
      uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options

   // could PAINT advice as to WHAT IS AVAILABLE
   if( hgt && !bSame ) {
      rc.top += hgt;
      if(( rc.top < rc.bottom ) && ((rc.bottom - rc.top) >= hgt)) {
         // we have have another line of text
         if( gbIgnDT2 )   // FIX20081125
         {
            strcpy(lpb,"*** NOTE: IGNORE File Time COMPLETELY is ON! ***");
            hgt = DrawText( hdc,          // handle to DC
               lpb, // text to draw
               strlen(lpb),   // text length
               &rc,  // formatting dimensions
               uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options
            if(hgt) {
               rc.top += hgt;
               strcpy( lpb, "*** ");
               Get_List_String2( lpb );
               strcat( lpb, " ***");
               hgt = DrawText( hdc,          // handle to DC
                  lpb, // text to draw
                  strlen(lpb),   // text length
                  &rc,  // formatting dimensions
                  uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options
               if(hgt) {
                  rc.top += hgt;
               }
            }
         }

         strcpy(lpb,"*** Change viewing options to see a list ***");
         hgt = DrawText( hdc,          // handle to DC
            lpb, // text to draw
            strlen(lpb),   // text length
            &rc,  // formatting dimensions
            uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options
         if(hgt) {
            rc.top += hgt;
            if(( rc.top < rc.bottom ) && ((rc.bottom - rc.top) >= hgt)) {
               // we have have another line of text
               strcpy( lpb, "*** ");
               Get_List_String( lpb );
               strcat( lpb, " ***");
               hgt = DrawText( hdc,          // handle to DC
                  lpb, // text to draw
                  strlen(lpb),   // text length
                  &rc,  // formatting dimensions
                  uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options
               if(hgt) {
                  rc.top += hgt;
                  if(( rc.top < rc.bottom ) && ((rc.bottom - rc.top) >= hgt)) {
                     // we have have another line of text
                     strcpy( lpb, "*** ");
                     Add_Stat_Info2( lpb );
                     strcat( lpb, " ***");
                     hgt = DrawText( hdc,          // handle to DC
                        lpb, // text to draw
                        strlen(lpb),   // text length
                        &rc,  // formatting dimensions
                        uiForm ); // DT_CENTER | DT_VCENTER ); // text-drawing options
                  }
               }
            }
         }
      }
   }

   SetTextColor(hdc, fgcol);
   SetBkColor(  hdc, bkcol);

}

// empty list - paint a warning,
// or paint a HELP message
VOID  Paint_Warning( HDC hDC, DWORD dwType )
{
   RECT     rc;
   HDC      hdc = hDC;
   HWND     hwnd = hwndRCD;   // get the TABLE window overlay
   if(( !gbInInitial               ) &&
      ( GetClientRect( hwnd, &rc ) ) ){
      if( hDC == 0 )
         hdc = GetDC(hwnd);
      if(hdc) {
         if( dwType == 1 ) {
            Paint_HELP( hdc, &rc );
         } else {
            Paint_Grumble( hdc, &rc );
         }

         if( hDC == 0 )
            ReleaseDC(hwnd, hdc);

         g_bNeedPaint = FALSE;      // we have PAINTED a WARNING colour - once only
      }
   }
}

/***************************************************************************
 * Function: ToOutline
 *
 * Purpose:
 *
 * Switch back to outline view - showing just the list of file names.
 * From button or menu IDM_OUTLINE
 *
 */
void
ToOutline(PVIEW view)
{
   if( !view )
      return;

   if( ( view_isexpanded(view) ) &&
       ( !g_bNoUpdate ) )
   {
      /* save the current expanded size and position */
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);
      if( GetWindowPlacement(hwndClient,&wp) )
      {
         if( ChangedWP( &wp, &g_sWPE ) )
         {
            memcpy( &g_sWPE, &wp, sizeof(WINDOWPLACEMENT) );
            g_sWPE.length = sizeof(WINDOWPLACEMENT);
            bChgExp = TRUE;
         }
      }

      /* restore the previous expanded size and position, if any */
      if( gbSetOut )
      {
         if( gbGotOut && ( g_sWPO.length == sizeof(WINDOWPLACEMENT) ) )
            SetWindowPlacement(hwndClient,&g_sWPO);
         else
            ShowWindow(hwndClient, SW_SHOWNORMAL);
      }

   }

   DisplayMode = MODE_OUTLINE;
   Dec_g_bInExpand;  // reduce in expand count
   /* switch mapping back to outline view */
   // we should perhaps paint a 'warning' that NOTHING got painted!
   disp_number = view_outline(view);   // now returns COUNT of lines (displayable?) available
   // ie list count in OUTLINE view
   // *****************************
   //if( !g_bNoUpdate )

   if( !g_bNoResize )
   {
      /* hide bar window and resize to cover */
      DoResize(hwndClient);

      /* change label on button */
      if( !gfBusy )
      {
         LPTSTR lpb = GetNxtBuf();  // &gszTmpBuf[0];

         strcpy(lpb,LoadRcString(IDS_EXPAND));
         SetButtonText(lpb);

         //SetStatus(NULL);
         //sprintf( lpb, "List of %d files. ", disp_number );
         //sprintf( lpb, "%d (of %d). ", disp_number, g_dwListTot );
         //SetStatus(lpb);
         SetStatus( view_getfilcntstg( view ) );

         // SetTitle
         SetAppTitle();
      }
   }

} /* ToOutline */

/***************************************************************************
 * Function: ToMoved
 *
 * Purpose:
 *
 * If the user clicks on a MOVED line in expand mode, we jump to the
 * other line. We return TRUE if this was possible,  or FALSE otherwise.
 */
BOOL
ToMoved(HWND hwnd)
{
        BOOL bIsLeft;
        int linenr, state;
        long i, total;
        int    iSel = giSelection;
        PVIEW  view = current_view;

        if(( !view ) ||
           ( DisplayMode != MODE_EXPAND ) ||
           ( iSel < 0 ) )
        {
                return(FALSE);
        }

        state = view_getstate(view, iSel);
        if( state == STATE_MOVEDLEFT )
        {
                bIsLeft = TRUE;
                /* get the linenr of the other copy */
                linenr = abs(view_getlinenr_right(view, iSel));
        }
        else if (state == STATE_MOVEDRIGHT)
        {
                bIsLeft = FALSE;
                /* get the linenr of the other copy */
                linenr = abs(view_getlinenr_left(view, iSel));
        }
        else
        {
                /* not a moved line - so we can't find another copy */
                return(FALSE);
        }

        /* search the view for this line nr */
        total = view_getrowcount(view);
        for (i = 0; i < total; i++)
        {
                if (bIsLeft)
                {
                        if (linenr == view_getlinenr_right(view, i))
                        {
                                /* found it */
                                SetSelection(i);
                                return(TRUE);
                        }
                }
                else
                {
                        if (linenr == view_getlinenr_left(view, i))
                        {
                                SetSelection(i);
                                return(TRUE);
                        }
                }
        }
        return(FALSE);
}


/***************************************************************************
 * Function: do_editfile
 *
 * Purpose:
 *
 * Launch an editor on the current file (the file we are expanding, or
 * in outline mode the selected row. Option allows selection of the
 * left file, the right file or the composite view of this item.
 * pe points to a packet of parameters that must be freed before returning.
 * The return value is meaningless (just to conform to CreateThread).
 *
 * Called from MENU items
 * case IDM_EDITCOMP: with CI_COMP;,
 * case IDM_EDITLEFT: with CI_LEFT;,
 * case IDM_EDITRIGHT: with CI_RIGHT;
 *
 */
LONG
do_editfile(PEDITARGS pe)
{
        PVIEW  view = pe->view;
        int    option = pe->option;
        int    selection = pe->selection;
        LPTSTR pcmd = &pe->cCmdLine[0];
        COMPITEM item;
        LPSTR fname;
//        static char _s_cmdline[256];
        int currentline;
//        char * pOut = _s_cmdline;
        char * pOut = pcmd;
//        char * pIn = editor_cmdline;
        char * pIn = pe->pCmd;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        // NOTE: Returns EXPANDED item if expanded, else
        // only if the 'selection' row is VALID
        item = view_getitem(view, selection);
        if( item == NULL )
        {
           return -1;
        }

        fname = compitem_getfilename(item, option);

        if( 0 == fname )
        {
            MB(hwndClient, LoadRcString(IDS_FILE_DOESNT_EXIST),
                       APPNAME, MB_ICONSTOP|MB_OK);
            goto error;
        }

       switch ( option )
        {
        case CI_LEFT:
            currentline = view_getlinenr_left( view,
                                               selection > 0 ? selection : 1);
            break;

        case CI_RIGHT:
            currentline = view_getlinenr_right( view,
                                                selection > 0 ? selection : 1);
            break;

        default:
            currentline = 1;
            break;
        }

        while( *pIn )
        {
            switch( *pIn )
            {
            case '%':
                pIn++;
                switch ( *pIn )
                {
                case 'p':
                    strcpy( (LPTSTR)pOut, fname );
                    while ( *pOut )
                        pOut++;
                    break;

                case 'l':
                    _ltoa( currentline, pOut, 10 );
                    while ( *pOut )
                        pOut++;
                    break;

                default:
                    if (IsDBCSLeadByte(*pIn) && *(pIn+1))
                    {
                        *pOut++ = *pIn++;
                    }
                    *pOut++ = *pIn;
                    break;
                }
                pIn++;
                break;

            default:
                if (IsDBCSLeadByte(*pIn) && *(pIn+1))
                {
                    *pOut++ = *pIn++;
                }
                *pOut++ = *pIn++;
                break;
            }
        }


        /* Launch the process and waits for it to complete */

        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = NULL;
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;
        //si.lpTitle = (LPSTR)_s_cmdline; 
        si.lpTitle = pcmd; 
        si.lpDesktop = (LPTSTR)NULL;
        si.dwFlags = STARTF_FORCEONFEEDBACK;

        if( !CreateProcess(NULL,
                        pcmd, // _s_cmdline,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        (LPTSTR)NULL,
                        &si,
                        &pi))
        {
                MB(hwndClient, LoadRcString(IDS_FAILED_TO_LAUNCH_EDT),
                        APPNAME, MB_ICONSTOP|MB_OK);
                goto error;
        }

        /* wait for completion. */
        WaitForSingleObject(pi.hProcess, INFINITE);

        /* close process and thread handles */
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        /* finished with the filename. deletes it if it was a temp. */
        compitem_freefilename(item, option, fname);

        /*
         * refresh cached view always .  A common trick is to edit the
         * composite file and then save it as a new left or right file.
         * Equally the user can edit the left and save as a new right.
         */

        /* We want to force both files to be re-read, but it's not a terribly
         * good idea to throw the lines away on this thread.  Someone might
         * be reading them on another thread!
         */
        /* file_discardlines(compitem_getleftfile(item)) */
        /* file_discardlines(compitem_getrightfile(item)) */

        /* force the compare to be re-done */
        PostMessage(hwndClient, WM_COMMAND, IDM_UPDATE, (LPARAM)item);

error:

        gmem_free(hHeap, (LPSTR) pe, sizeof(EDITARGS), "do_editthread" );

        return 0;

} /* do_editfile */


/***************************************************************************
 * Function: do_editthread
 *
 * Purpose:
 *
 * Launch an editor on a separate thread.  It will actually get a separate
 * process, but we want our own thread in this process.  This thread will
 * wait until it's finished and then order up a refresh of the UI.
 * Need to give it its parameters as a gmem allocated packet because
 * it IS on a separate thread.
 *
 * Called from MENU item    case IDM_EDITCOMP:
 *   with do_editthread(current_view, CI_COMP);
 * or case IDM_EDITLEFT: with CI_LEFT);,
 * or case IDM_EDITRIGHT: with CI_RIGHT);
 *
 */
void do_editthread(PVIEW view, int option)
{
        PEDITARGS pe;
        HANDLE thread;

        pe = (PEDITARGS) gmem_get(hHeap, sizeof(EDITARGS), "do_editthread" );
        ZeroMemory(pe, sizeof(EDITARGS));

        pe->view = view;
        pe->option = option;
        pe->selection = giSelection; // copy the GLOBAL selection row (-1 if NOT)
        pe->pCmd = editor_cmdline;
        pe->bDoMB = TRUE;  // show message BOX if error
        thread = CreateThread( NULL
                             , 0
                             , (LPTHREAD_START_ROUTINE)do_editfile
                             , (LPVOID) pe
                             , 0
                             , &threadid
                             );
        if (thread == NULL)
        {
                /* The createthread failed, do without the extra thread - just
                 * call the function synchronously
                 */
                 do_editfile(pe);
        }
        else CloseHandle(thread);
} /* do_editthread */


/* status bar and busy flags --------------------------------------------*/


/***************************************************************************
 * Function: SetButtonText
 *
 * Purpose:
 *
 * Set the Text on the statusbar button to reflect the current state 
 */
void
SetButtonText(LPSTR cmd)
{
        SendMessage(hwndStatus, SM_SETTEXT, IDM_ABORT, (LPARAM) cmd);
}

/***************************************************************************
 * Function: SetStatus
 *
 * Purpose:
 *
 * Set the status field (left-hand part) of the status bar. 
 */
void
SetStatus(LPSTR cmd)
{
        SendMessage(hwndStatus, SM_SETTEXT, IDL_STATLAB, (LPARAM) cmd);
}


/***************************************************************************
 * Function: SetNames
 *
 * Purpose:
 *
 * Set the names field - the central box in the status bar 
 */
void
SetNames(LPSTR names)
{
   if(names == NULL)
   {
      AppTitle[0] = '\0';
   }
   else
   {
      strncpy(AppTitle, names, 256);
      AppTitle[256] = 0;   // ensure a NULL added
   }

   SendMessage(hwndStatus, SM_SETTEXT, IDL_NAMES, (LPARAM)&AppTitle[0]);
}

#ifdef   ADDSTATS2   // activate another item on the status bar
void SetListStats( LPTSTR lps )
{
   SendMessage(hwndStatus, SM_SETTEXT,
      IDL_LISTSTATS, // folder compare list information
      (LPARAM) lps );
}
#endif   // #ifdef   ADDSTATS2   // activate another item on the status bar

LPTSTR   g_BTComp, g_BTScan;

VOID  SetStatusStg( UINT uType )
{
   //LPTSTR   lps = LoadRcString(IDS_COMPARING);   // load string into set of
   //LPTSTR   lpw = LoadRcString(IDS_SCANNING);    // 16 or so rotating buffers
   LPTSTR   lps = "Busy?";
   LPTSTR   lpw = "DC4W - Busy scanning..."; // default, if BT_INITIAL
   switch(uType)
   {
   case BT_FILEDLG:
      lpw = APPNAME" - Get compare files";
      lps = "Scanning ...";
      break;
   case BT_DIRDLG:
      lpw = APPNAME" - Get compare folders";
      lps = "Selecting ...";
      break;
   case BT_REFRESH:
      lpw = APPNAME" - Refresh of display";
      lps = "Refreshing ...";
      break;
   case BT_COPYFILES:
      lpw = APPNAME" - Copy files dialog";
      lps = "Copying ...";
      break;
   //case BT_INITIAL: // just use the LOADED string
   //   break;
   case BT_WRITEDIFF:
      lpw = APPNAME" - Writing differences";
      lps = "Differences...";
      break;

   case BT_DELETEFILES:
      lpw = APPNAME" - Delete files dialog";
      lps = "Deleting ...";
      break;
   }

   //SetStatus(LoadRcString(IDS_COMPARING));
   /* status also on window text, so that you can see even from
    * the icon when the scan has finished
    */
   //SetWindowText(hwndClient, LoadRcString(IDS_SCANNING));
   SetStatus(lps);
   SetWindowText(hwndClient, lpw);

   g_BTComp = lps;   // short description of BUSY event
   g_BTScan = lpw;

}

/***************************************************************************
 * Function: SetBusy
 *
 * Purpose:
 *
 * If we are not already busy, set the busy flag.
 *
 * Enter critical section first.
 */
BOOL
SetBusy( UINT uType )   // pass the TYPE of busy
{
        HMENU hmenu;

        WDEnter();
        if( gfBusy )
        {
           WDLeave();
           return(FALSE);
        }

        gfBusy = TRUE;     // SET THE BUSY FALG

        SetStatusStg( uType );

        /* disable appropriate parts of menu */
        hmenu = GetMenu(hwndClient);
        EnableMenuItem(hmenu, IDM_FILE,MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
        EnableMenuItem(hmenu, IDM_DIR,MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
        EnableMenuItem(hmenu, IDM_PRINT,MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);

        /* enable abort only when busy */
        EnableMenuItem(hmenu, IDM_ABORT,MF_ENABLED|MF_BYCOMMAND);

        SetButtonText(LoadRcString(IDS_ABORT));  /* leave DisplayMode unchanged */

        WDLeave();

        return(TRUE);

} /* SetBusy */

VOID  SetExpandStatus( PVIEW view )
{
   LPTSTR   lpb = &gszTmpBuf[0];
   sprintf(lpb, "%d %s ", (view_getiselect(view) + 1),
         view_getcurrenttag(view) );
   SetStatus(lpb);
}

//#define  g_pcfds     &g_sCFDLGSTR

VOID  SetOutlineStatus( PVIEW view )
{
   COMPLIST cl;

   //LPTSTR   lpb = &gszTmpBuf[0];
   //sprintf(lpb, "List of %d files. ", view_getrowcount(view) );
   //SetStatus(lpb);
   SetStatus( view_getfilcntstg( view ) );
   // complist_savelist(view_getcomplist(current_view), NULL, gdwFileOpts );
   cl = view_getcomplist(view);
   if(cl)
   {
      // get the LIST stats
      complist_getstats( g_pcfds, cl );
   }
}

/***************************************************************************
 * Function: SetNotBusy
 *
 * Purpose:
 *
 * This function can be called from the worker thread.
 * Thus we must not cause any SendMessage calls to windows
 * owned by the main thread while holding the CritSec or we
 * could cause deadlock.
 *
 * The critsec is only needed to protect the gfBusy flag - so
 * clear the busy flag last, and only get the crit sec as needed.
 */
void
SetNotBusy(void)
{
   static int _s_icntnb = 0;
   HMENU hmenu;
   LPTSTR   lpb = GetNxtBuf();  //&gszTmpBuf[0];
   PVIEW    view = current_view;
   BOOL     bIsExp = view_isexpanded(view);

   /* reset button and status bar (clearing out busy flags) */
   if( view == NULL )
   {
           SetButtonText(LoadRcString(IDS_EXIT));

           if( _s_icntnb == 0 )
              SetStatus("Initial View - Waiting for list ...");
           else
              SetStatus("View NULL");

           DisplayMode = MODE_NULL;
           // we have nothing to display - this is uninteresting as hell
           // just a white display with no help!!!
           // ******** put up a DIALOG and ask ******** politely which
           // but this check MUST wait until the 'scanning' thread has
           // run - maybe View will NOT be NULL
           //if( !g_bOneShot )
           //{
           //   g_bOneShot = TRUE;
           //   sprtf( "Posting an IDM_DIR to %x"MEOR, g_hWnd );
           //   PostMessage( g_hWnd, WM_COMMAND, (WPARAM) IDM_DIR, 0 );
           //   //sprtf( "Would post an IDM_DIR to %x"MEOR, g_hWnd );
           //}

   }
   else if( bIsExp )
   {
      // in EXPANDED mode
      DisplayMode = MODE_EXPAND;

      strcpy(lpb,LoadRcString(IDS_OUTLINE));
      SetButtonText(lpb);

      //SetStatus(view_getcurrenttag(current_view) );
      SetExpandStatus(view);
   }
   else
   {
      // in OUTLINE mode
      DisplayMode = MODE_OUTLINE;

      strcpy(lpb,LoadRcString(IDS_EXPAND));

      SetButtonText(lpb);

      //SetStatus(NULL);
      SetOutlineStatus(view);

   }


   /* re-enable appropriate parts of menu */

   hmenu = GetMenu(hwndClient);

   EnableMenuItem(hmenu, IDM_FILE,MF_ENABLED|MF_BYCOMMAND);
   EnableMenuItem(hmenu, IDM_DIR,MF_ENABLED|MF_BYCOMMAND);
   EnableMenuItem(hmenu, IDM_PRINT,MF_ENABLED|MF_BYCOMMAND);

   /* disable abort now no longer busy */
   EnableMenuItem(hmenu, IDM_ABORT,MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);


   /* clear the busy flag, protected by critical section */
   WDEnter();

   gfBusy = FALSE; // CLEAR THE BUSY FLAG
   bAbort = FALSE;

   if( ghThread != NULL )
   {
       CloseHandle(ghThread);
       ghThread = NULL;
   }

   WDLeave();

   SetAppTitle();

   setToolText( (int)GetCurrRow() );

   _s_icntnb++;

} /* SetNotBusy */

/***************************************************************************
 * Function: IsBusy
 *
 * Purpose:
 *
 * Checks whether or not crit sec is open
 */
BOOL
IsBusy()
{
        BOOL bOK;

        WDEnter();

        bOK = gfBusy;   // GET THE BUSY FLAG

        WDLeave();

        return(bOK);

} /* IsBusy */

/***************************************************************************
 * Function: BusyError
 *
 * Purpose:
 *
 * Puts up message box that system is busy.
 */
void
BusyError( LPTSTR lpmsg )
{
   LPTSTR lpb = GetStgBuf();
   sprintf( lpb, "BUSY ERROR"MEOR
      "Msg: %s"MEOR
      "%s"MEOR
      "Last: %s",
      lpmsg,
      LoadRcString(IDS_PLEASE_WAIT),
      g_BTComp ); // Please wait for current operation to finish
   sprtf("%s"MEOR, lpb);   // send to LOG

   //     MB(hwndClient,
   //             LoadRcString(IDS_PLEASE_WAIT), // Please wait for current operation to finish
   //             APPNAME, MB_OK|MB_ICONSTOP);
   MB(hwndClient, lpb, // Please wait for current operation to finish
      APPNAME, MB_OK|MB_ICONINFORMATION);

} /* BusyError */

/* --- colour scheme --------------------------------------------------- */

/***************************************************************************
 * Function: StateToColour
 *
 * Purpose:
 *
 * Map the state given into a foreground and a background colour
 * for states that are highlighted. Return P_FCOLOUR if the foreground
 * colour (put in *foreground) is to be used, return P_FCOLOUR|P_BCOLOUR if
 * both *foreground and *background are to be used, or 0 if the default
 * colours are to be used.
 *
 * ALSO add COLOUR difference for OUTLINE mode
 * namely STATE_FILELEFTONLY and STATE_FILERIGHTONLY
 * These can be the SAME as that applies to lines only of
 * STATE_LEFTONLY - line only in left file, and
 * STATE_RIGHTONLY - line only in right file
 *
 */
UINT
StateToColour(int state, DWORD dwFlag, int col,
              PDWORD foreground, PDWORD background )
{
   UINT  ui = 0;
   switch (state)
   {

   case STATE_DIFFER:
           /* files that differ are picked out in a foreground highlight,
            * with the default background
            */
      if( dwFlag & TT_YOUNGER )
         *foreground = rgb_outlineNew; // these are the NEW - to be copied
      else
         *foreground = rgb_outlinehi;

      if( dwFlag & TT_VIEWED )
      {
         *background = rgb_outviewBack;
         //  return(P_FCOLOUR|P_BCOLOUR);
         ui = (P_FCOLOUR|P_BCOLOUR);
      }
      else
      {
         //return(P_FCOLOUR);
         ui = P_FCOLOUR;
      }
      break;

   case STATE_LEFTONLY:
   case STATE_FILELEFTONLY:
           /* lines only in the left file */
           *foreground = rgb_leftfore;
           *background = rgb_leftback;
           //return(P_FCOLOUR|P_BCOLOUR);
           ui = (P_FCOLOUR|P_BCOLOUR);
         break;

   case STATE_RIGHTONLY:
   case STATE_FILERIGHTONLY:
           /* lines only in the right file */
           *foreground = rgb_rightfore;
           *background = rgb_rightback;
           //return(P_FCOLOUR|P_BCOLOUR);
           ui = (P_FCOLOUR|P_BCOLOUR);
         break;

   case STATE_MOVEDLEFT:
           /* displaced lines in both files - left file version */
           *foreground = rgb_mleftfore;
           *background = rgb_mleftback;
           //return(P_FCOLOUR|P_BCOLOUR);
           ui = (P_FCOLOUR|P_BCOLOUR);
         break;

   case STATE_MOVEDRIGHT:
           /* displaced lines in both files - right file version */
           *foreground = rgb_mrightfore;
           *background = rgb_mrightback;
           //return(P_FCOLOUR|P_BCOLOUR);
           ui = (P_FCOLOUR|P_BCOLOUR);
         break;

//   default:
//   no highlighting - default colours
//           return(0);
   }

   return ui;

}

VOID  StateToString( LPTSTR lpb, int state, DWORD dwFlag )
{
   switch(state)
   {
   case STATE_SAME:
//   case STATE_COMPARABLE:
//   case STATE_SIMILAR:
      strcat(lpb, "Same");
      break;
   case STATE_DIFFER:
      //strcat(lpb, "Differ");
      strcat(lpb, "Dif");

      if( dwFlag & TT_YOUNGER )
         strcat(lpb, "New");
      else
         strcat(lpb,"Old");

      break;

   case STATE_FILELEFTONLY:
   case STATE_LEFTONLY:
      strcat(lpb, "Left only");
      break;

   case STATE_FILERIGHTONLY:
   case STATE_RIGHTONLY:
      strcat(lpb, "Right only");
      break;

   case STATE_MOVEDLEFT:    /* this is the left file version */
      strcat(lpb, "Left");
      break;

   case STATE_MOVEDRIGHT:  /* this is the right file version*/
      strcat(lpb, "Right");
      break;

   default:
      sprintf(EndBuf(lpb), "?UNK?(%d)", state );
      break;
   }
}

INT   AppendOptStg( LPTSTR lpb, BOOL bAll, DWORD dwo )
{
   INT   iRet = 0;
   if( ( bAll                    ) &&  // show "All" instead of LIST
       ( dwo & INCLUDE_LEFTONLY  ) &&
       ( dwo & INCLUDE_RIGHTONLY ) &&
       ( dwo & INCLUDE_SAME      ) &&
       ( dwo & INCLUDE_DIFFER    ) )
   {
      strcat(lpb,"All");
      return 3;
   }

   if( dwo & INCLUDE_LEFTONLY )
   {
      iRet += 4;
      strcat(lpb, "Left");
   }

   if( dwo & INCLUDE_RIGHTONLY )
   {
      if(iRet)
      {
         iRet++;
         strcat(lpb,"|");
      }
      strcat(lpb, "Right");
      iRet += 5;
   }

   if( dwo & INCLUDE_SAME )
   {
      if(iRet)
      {
         iRet++;
         strcat(lpb,"|");
      }
      strcat(lpb, "Same");
      iRet += 4;
   }
   
   if( dwo & INCLUDE_DIFFER )
   {
      if(iRet)
      {
         iRet++;
         strcat(lpb,"|");
      }
      //strcat(lpb, "Differ");
      //iRet += 6;
      if( ( dwo & (INCLUDE_NEWER|INCLUDE_OLDER) ) == (INCLUDE_NEWER|INCLUDE_OLDER) )
      {
         strcat(lpb, "DifAll");
         iRet += 6;
      }
      else
      {
         strcat(lpb, "Dif");
         iRet += 3;
         if( dwo & INCLUDE_NEWER )
         {
            strcat(lpb, "New");
            iRet += 3;
         }
         else if( dwo & INCLUDE_OLDER )
         {
            strcat(lpb, "Old");
            iRet += 3;
         }
         else
         {
            strcat(lpb, "Err");
            iRet += 3;
         }
      }
   }
   return iRet;
}


INT   OptionsToString( LPTSTR lpb, BOOL bAll )
{
   INT   iRet = AppendOptStg( lpb, bAll, outline_include );
   return iRet;
}


/* table window communication routines ---------------------------------*/

/***************************************************************************
 * Function: SetSelection
 *
 * Purpose:
 *
 * Set a given row as the selected row in the table window 
 */
void SetSelection(long rownr)
{
   TableSelection select;
   select.startrow  = rownr;
   select.startcell = 0;
   select.nrows     = 1;
   select.ncells    = 1;
   SendMessage(hwndRCD, TM_SELECT, 0, (LPARAM) (LPSTR)&select);
}



/***************************************************************************
 * Function: do_gethdr
 *
 * Purpose:
 *
 * Handle table class call back to get nr of rows and columns,
 * and properties for the whole table.
 * The 'table id' is either TABID_PRINTER - meaning we are
 * printing the current_view, or it is the view to
 * use for row/column nr information
 */
long
do_gethdr(HWND hwnd, lpTableHdr phdr)
{
        PVIEW  view;
        BOOL   bIsPrinter = FALSE;
        DWORD  dwo;

        if( phdr->id == TABID_PRINTER )
        {
                view = current_view;
                bIsPrinter = TRUE;
        }
        else
        {
                view = (PVIEW) phdr->id;
        }

        if( view == NULL )
        {
            sprtf( "do_gethdr(TQ_GETSIZE): Returning FALSE since view is NULL!"MEOR );
            return (FALSE);
        }

        phdr->nrows = view_getrowcount(view);

        /*  three columns: line nr, tag and rest of line */
        // three (3) columns: Line nr, tag, and rest of line, or
        // two(2) if gdwDiffOpts & INCLUDE_LINENUMS = 0, and
        // one(1) if above and gdwDiffOpts & INCLUDE_TAG

        /*
         * if IDM_NONRS (no line numbers) is selected, suppress the
         * line-nr column entirely to save screen space
         */

#ifdef   NEWCOLS2 // add or subtract columns with ease
         phdr->ncols = 3;
         phdr->fixedcols = 1;
         //if( DisplayMode == MODE_EXPAND )
         if( g_bInExpand )
         {
            dwo = gdwDiffOpts;
            //if( !( gdwDiffOpts & INCLUDE_LINENUMS ) )
            if( !( dwo & INCLUDE_LINENUMS ) )
            {
               phdr->ncols--; // no line numbers
               phdr->fixedcols = 0;
               //if( !( gdwDiffOpts & INCLUDE_TAGS ) )
               if( !( dwo & INCLUDE_TAGS ) )
               {
                  phdr->ncols--; // no tags
                  phdr->fixedcols = 0;
               }
            }
         }
         else
         {
            dwo = gdwFileOpts;
            if( !( dwo & INCLUDE_LINENUMS ) )
            {
               phdr->ncols--; // no line numbers
               phdr->fixedcols = 0;
               // in outline tags are the FILE NAMES!!!!
               //if( !( dwo & INCLUDE_TAGS ) )
               //{
               //   phdr->ncols--; // no tags
               //   phdr->fixedcols = 0;
               //}
            }
         }

#else // !NEWCOLS2
        //if( line_numbers == IDM_NONRS )
        if( !gbShowNums )
        {
           // *** NO LINE NUMBERS ***
                phdr->ncols = 2;
                phdr->fixedcols = 0;
        }
        else
        {
           // add line numbers - from left or right
                phdr->ncols = 3;
                phdr->fixedcols = 1;
        }
#endif   // NEWCOLS2 y/n

        phdr->fixedrows = 0;
        phdr->fixedselectable = FALSE;
        phdr->hseparator = TRUE;
        phdr->vseparator = TRUE;

        phdr->selectmode = TM_ROW | TM_SINGLE;
        /*
         * find if we are in expand mode - ask for the item we are expanding.
         */
        if (view_isexpanded(view) == TRUE)
        {

                /* use focus rect as selection mode in expand mode
                 * so as not to interfere with background colours.
                 */
                phdr->selectmode |= TM_FOCUS;
        }
        else
        {
                /* use default solid inversion when possible as it is clearer.*/
                phdr->selectmode |= TM_SOLID;
        }

        /* please send TQ_SCROLL notifications when the table is scrolled */
        phdr->sendscroll = TRUE;
        phdr->props.valid = 0;

        return TRUE;
}

/***************************************************************************
 * Function: do_getprops
 *
 * Purpose:
 *
 * Respond to table callback asking for the size and properties
 * of each column. Table id is either TABID_PRINTER (meaning the
 * current_view, for printing) or it is the view to be used.
 */
long
do_getprops(HWND hwnd, lpColPropsList propslist)
{
        int    i, cell, mod;
        BOOL   bIsPrinter = FALSE;
        PVIEW  view;
        DWORD  dwo;

        if (propslist->id == TABID_PRINTER)
        {
                view = current_view;
                bIsPrinter = TRUE;
        }
        else
        {
                view = (PVIEW) propslist->id;
        }
        if (view == NULL)
        {
                return(FALSE);
        }

        // FIX20050226 - column error when entering expanded mode - compare of two files
        if(( DisplayMode == MODE_EXPAND ) || view_isexpanded(view)){
           // we are in EXPANDED view of a pair of files
           mod = 1; // in expanded mode -
           dwo = gdwDiffOpts;
        } else {
           mod = 0;
           dwo = gdwFileOpts;
        }

        /* The table inteface is slightly confused here. we are not
         * guaranteed which columns we are being asked about, so instead
         * of just setting each column cols[0], cols[1] etc, we need
         * to loop through, looking at each column in the table and
         * seeing which it is.
         */
        for (i = 0; i < propslist->ncols; i++)
        {
                cell = i + propslist->startcol;
                propslist->plist[i].props.valid = 0;
                // propslist->plist[i].nchars = 0;
#ifdef   NEWCOLS2 // add or subtract columns with ease
//         phdr->ncols = 3;
//         phdr->fixedcols = 1;
            //   if( !( gdwDiffOpts & INCLUDE_LINENUMS ) )
            // outline  mode - file list output default options
            // gdwFileOpts = INC_ALLXSM;
            //   if( !( gdwFileOpts & INCLUDE_LINENUMS ) )
               if( !( dwo & INCLUDE_LINENUMS ) )
               {
                  cell++; // no ROW numbers
                  //if( DisplayMode == MODE_EXPAND )
                  if(mod) // ( DisplayMode == MODE_EXPAND )
                  {
                     //if( !( gdwFileOpts & INCLUDE_TAGS ) )
                     if( !( dwo & INCLUDE_TAGS ) )
                        cell++; // no tags
                  }
               }

#else // !#ifdef   NEWCOLS2 // add or subtract columns with ease
                /* for all column widths, add on 1 for the NULL char. */

                /* skip the line nr column if IDM_NONRS */
                //if (line_numbers == IDM_NONRS)
                if( !gbShowNums )
                {
                        cell++;
                }
#endif   // #ifdef   NEWCOLS2 // add or subtract columns with ease y/n

                if( cell == 0 )
                {
                        /* properties for line nr column */
                        propslist->plist[i].nchars = view_getwidth(view, 0)+1;
                        propslist->plist[i].props.valid |= P_ALIGN;
                        propslist->plist[i].props.alignment = P_CENTRE;
                }
                else if( cell == 1 )
                {

                        /* properties for tag field */
                        propslist->plist[i].nchars = view_getwidth(view, 1)+1;
                        propslist->plist[i].props.valid |= P_ALIGN;
                        propslist->plist[i].props.alignment = P_LEFT;
                }
                else
                {
                        /* properties for main text column -
                         * use a fixed font unless printing (if
                         * printing, best to use the default font, because
                         * of resolution differences.
                         * add on 8 chars to the width to ensure that
                         * the width of lines beginning with tabs
                         * works out ok
                         */
                        propslist->plist[i].nchars = view_getwidth(view, 2)+1;
                        propslist->plist[i].props.valid |= P_ALIGN;
                        propslist->plist[i].props.alignment = P_LEFT;
                        if (!bIsPrinter)
                        {
                           // LOGFONT - change the font
                           propslist->plist[i].props.valid |= P_FONT;
                           if( g_hFixedFont8 )
                           {
                              // use our created FONT
                                propslist->plist[i].props.hFont = g_hFixedFont8;

                           }
                           else
                           {
                                propslist->plist[i].props.hFont =
                                        GetStockObject(SYSTEM_FIXED_FONT);
                           }
                        }
                }
        }
        return (TRUE);
}

/***************************************************************************
 * Function: do_getdata
 *
 * Purpose:
 *
 * Respond to a table callback asking for the contents of individual cells.
 * table id is either TABID_PRINTER, or it is a pointer to the view
 * to use for data. If going to the printer, don't set the
 * colours (stick to black and white).
 */
long
do_getdata(HWND hwnd, lpCellDataList cdlist)
{
        int start, endcell, col, i;
        lpCellData cd;
        PVIEW view;
        LPSTR textp;
        BOOL bIsPrinter = FALSE;
        DWORD     dwo;

        if (cdlist->id == TABID_PRINTER)
        {
                view = current_view;
                bIsPrinter = TRUE;
        }
        else
        {
                view = (PVIEW) cdlist->id;
        }

        start = cdlist->startcell;
        endcell = cdlist->ncells + start;
        if (cdlist->row >= view_getrowcount(view))
        {
                return(FALSE);
        }
        for (i = start; i < endcell; i++)
        {
                cd = &cdlist->plist[i - start];
#ifdef   NEWCOLS2 // add or subtract columns with ease
                col = i;
                if( DisplayMode == MODE_EXPAND )
                {
                   //if( !( gdwDiffOpts & INCLUDE_LINENUMS ) )
                   dwo = gdwDiffOpts;
                   if( !( dwo & INCLUDE_LINENUMS ) )
                   {
                      col++; // no line numbers
                      //if( !( gdwDiffOpts & INCLUDE_TAGS ) )
                      if( !( dwo & INCLUDE_TAGS ) )
                         col++;  // no tag
                   }
                }
                else
                {
                   dwo = gdwFileOpts;
                   if( !( dwo & INCLUDE_LINENUMS ) )
                   {
                      col++; // no row numbers
                   }
                }

#else // !#ifdef   NEWCOLS2 // add or subtract columns with ease
                /* skip the line number column if IDM_NONRS */
                //if (line_numbers == IDM_NONRS)
                if( !gbShowNums )
                {
                        col = i+1;
                }
                else
                {
                        col = i;
                }
#endif   // #ifdef   NEWCOLS2 // add or subtract columns with ease y/n

                /* set colour of text to mark out
                 * lines that are changed, if not printer - for the
                 * printer everything should stay in the default colours
                 */

                if (!bIsPrinter)
                {

                        /* convert the state of the requested row into a
                         * colour scheme. returns P_FCOLOUR and/or
                         * P_BCOLOUR if it sets either of the colours
                         */
                        cd->props.valid |=
                            StateToColour(view_getstate(view, cdlist->row),
                                 view_getflag(view, cdlist->row),
                                 col,
                                 &cd->props.forecolour,
                                 &cd->props.backcolour);
                }

                textp = view_gettext(view, cdlist->row, col);
                if (cd->nchars != 0)
                {
                        if (textp == NULL)
                        {
                                cd->ptext[0] = '\0';
                        }
                        else
                        {
                                strncpy(cd->ptext, textp, cd->nchars -1);
                                cd->ptext[cd->nchars - 1] = '\0';
                        }
                }

        }
        return(TRUE);
}

/***************************************************************************
 * Function: SvrClose
 *
 * Purpose:
 *
 * Table window has finished with this view. It can be deleted.
 *
 * Chain of events is like -
 * void view_close(PVIEW view) send a TM_NEWID message to the TABLE class,
 * who in turn send a TQ_CLOSE thru gtab_sendtq(hwnd, TQ_CLOSE, ptab->hdr.id);
 * like HWND hOwner = (HANDLE) GetWindowLongPtr(hwnd, WW_OWNER);
 *    return (SendMessage(hOwner, gtab_msgcode, cmd, lParam));
 *
 */
void SvrClose(void)
{
   view_delete(current_view);

   current_view = NULL;

   /* hide picture - only visible when we are in MODE_EXPAND */
   DisplayMode = MODE_NULL;

   DoResize(hwndClient);

   /* if we already busy when closing this view (ie
    * we are in the process of starting a new scan,
    * then leave the status bar alone, otherwise
    * we should clean up the state of the status bar
    */
   if (!gfBusy)
   {
      SetButtonText(LoadRcString(IDS_EXIT));
      SetNames(NULL);
      SetStatus("Closed");   // NULL);
   }

} /* SvrClose */


/***************************************************************************
 * Function: TableServer
 *
 * Purpose:
 *
 * Handle callbacks and notifications from the table class 
 */
long
TableServer(HWND hwnd, WPARAM cmd, LPARAM lParam)
{
        lpTableHdr phdr;
        lpColPropsList proplist;
        lpCellDataList cdlist;
        lpTableSelection pselect;

#if   (defined(TBLSRVDBG) && !defined(NDEBUG))

        {
           LPTSTR lpd = &g_szBuf2[0];

           *lpd = 0;
           switch(cmd)
           {
           case TQ_GETSIZE:
              /* get the nr of rows and cols in this table */
              sprintf(lpd, "TQ_GETSIZE: lpTableHdr = %#x"MEOR, lParam );
              break;
           case TQ_GETCOLPROPS:
              /* get the size and properties of each column */
              sprintf(lpd, "TQ_GETCOLPROPS: lpColPropsList = %#x"MEOR, lParam );
              break;
           case TQ_GETDATA:
              /* get the contents of individual cells */
              sprintf(lpd, "TQ_GETDATA: lpCellDataList = %#x"MEOR, lParam );
              break;
           case TQ_SELECT:
              /* selection has changed */
              sprintf(lpd, "TQ_SELECT: Selection changed. lpTableSelection = %#x"MEOR,
                 lParam );
              break;
           case TQ_ENTER:
              /* user has double-clicked or pressed enter */
              sprintf(lpd, "TQ_ENTER: lpTableSelection = %#x"MEOR, lParam );
              break;
              /* store location for use in later search (IDM_FCHANGE) */
           case TQ_CLOSE:
              /* close this table - table class no longer needs data*/
              sprintf(lpd, "TQ_CLOSE: Table class no longer need data."MEOR );
              break;
           case TQ_SCROLL:
              sprintf(lpd, "TQ_SCROLL: If picture_mode BarDraPosition()"MEOR );
              break;
           default:
              sprintf(lpd, "TQ DEFAULT: Who or what is this?"MEOR );
              break;
           }

           if(*lpd)
              sprtf(lpd);

        }
#endif   // TBLSRVDBG

        switch(cmd)
        {
        case TQ_GETSIZE:
                /* get the nr of rows and cols in this table */
                phdr = (lpTableHdr) lParam;
                return(do_gethdr(hwnd, phdr));

        case TQ_GETCOLPROPS:
                /* get the size and properties of each column */
                proplist = (lpColPropsList) lParam;
                return (do_getprops(hwnd, proplist));

        case TQ_GETDATA:
                /* get the contents of individual cells */
                cdlist = (lpCellDataList) lParam;
                return (do_getdata(hwnd, cdlist));


        case TQ_SELECT:    /* selection has changed */
        case TQ_ENTER:     /* user has double-clicked or pressed enter */
                pselect = (lpTableSelection) lParam;
                /* store location for use in later search (IDM_FCHANGE) */
                if (pselect->nrows < 1)
                {
                        giSelection = -1;
                }
                else
                {
                        giSelection = (int) pselect->startrow;
                        if( cmd == TQ_ENTER )
                        {
                                /* try to expand this row */
                                if( !ToExpand(current_view) )
                                {
                                        /* expand failed - maybe this
                                         * is a moved line- show the other
                                         * copy
                                         */
                                        ToMoved(hwnd);
                                }

                        }
                }
                g_sCFDLGSTR.cf_iSelected = giSelection;  // update a SECOND GLOBAL SELECTED
                break;

        case TQ_CLOSE:
                /* close this table - table class no longer needs data*/
                SvrClose();
                break;

        case TQ_SCROLL:
                /* notification that the rows visible in the window
                 * have changed -change the current position lines in
                 * the graphic bar view (the sections picture)
                 */
                if (picture_mode) {
                        BarDrawPosition(hwndBar, NULL, TRUE);
                }
                break;

        default:
                return(FALSE);
        }
        return(TRUE);
}

VOID  CopytoGlobal( PTHREADARGS pta )
{

#ifdef   COMBARGS
   // copy allocated thread arguments to a
   // global (in this case alloc.) memory.
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL pointer
   PTARGS      pgtgs = &pcfds->cf_sTARGS;
   PTHREADARGS pgt   = &pgtgs->ta_sTA;

   // copy the THREAD argument to GLOBAL
   memcpy(pgt, pta, sizeof(THREADARGS) );

   // fix some 'self' pointers
   pgt->pFirst    = &pgt->szFirst[0];
   pgt->pSecond   = &pgt->szSecond[0];
   pgt->pSaveList = &pgt->szSaveList[0];  // pointer to SAVE output file
   pgt->pDiffList = &pgt->szDiffList[0];  // difference file name pointer

   pgt->pTargs          = pgtgs;
   pgtgs->ta_psCFDLGSTR = pcfds;

#endif   // !COMBARGS

}

/***************************************************************************
 * Function: wd_initial
 *
 * Purpose:
 *
 * Called on worker thread (not UI thread) to handle the work
 * requested on the command line. 
 * arg is a pointer to a THREADARGS block allocated from gmem get(hHeap). This
 * needs to be freed before exiting.
 */
DWORD
wd_initial(LPVOID arg)
{
   PTHREADARGS pta = (PTHREADARGS) arg;
   COMPLIST    cl;
   int         i, iret;
   DWORD       dwRet = (DWORD)0xfeed;
   PCLASTR     pcla = &g_sCLA;
   LPTSTR      lpl,lpr;
   BOOL        bZip = FALSE;
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   DWORD       dwmux;
   double      dbtot, dbwait;

   gbInInitial = TRUE;
   g_iThrdRun++;  // bump the initial thread run counter

   sprtf( "Beginning wd_initial thread with PTHREADARGS!"MEOR );
   i = SetBTime();
   iret = 0;

   if(ghMux)
   {
      dwmux = WaitForSingleObject( ghMux, // HANDLE hHandle,        // handle to object
         INFINITE ); // DWORD dwMilliseconds   // time-out interval
      switch(dwmux)
      {
      case WAIT_ABANDONED:
         sprtf("Mux WAIT abandonned ..."MEOR);
         break;
      case WAIT_OBJECT_0:
         sprtf("Mux STATE signalled ..."MEOR);
         break;
      case WAIT_TIMEOUT:
         sprtf("Mux TIMEOUT ..."MEOR);
         break;
      default:
         sprtf("Mux UNKNOWN = %d (%#x) ..."MEOR, dwmux, dwmux );
         break;
      }
      
      dbwait = GetRTime(i);
   }
   else
   {
      dbwait = 0.0;
   }

   /* build a complist from these args,
    * and register with the view we have made
    */

   ZeroMemory(pcla, sizeof(CLASTR));
   pcla->pCaller= "wd_initial - g_sCLA";
   pcla->pLeft  = pta->pFirst;
   pcla->pRight = pta->pSecond;
   lpl          = pta->pFirst;
   lpr          = pta->pSecond;
   pcla->pView  = pta->view;
#ifndef USE_GLOBAL_RECURSIVE
   pcla->bDeep  = pta->fDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
   pcla->dwFlg  = pta->taFlag;   // contains left/right ZIP flags
   cl = complist_args( pcla );
   if( cl == NULL )
   {
      // FAILED TO GET AN INITIAL COMPARE LIST
      // *************************************
      view_close(pta->view);
      pta->view = 0;
      //dwRet = (DWORD)-1;
      dwRet = 0;  // kill the 0X0FEED return - post IDM_EXIT
      iret  = (int)-1;
      goto Thd_End;
   }

   CopytoGlobal(pta);
#ifdef ADD_ZIP_SUPPORT
   ChkMissedList(); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT

   /* if savelist was selected, write out the list and exit */
   if( ( pta->pSaveList ) ||
       ( pta->pDiffList ) )
   {
      iret = 0;

      // JUST WRITE LIST AND EXIT
      if( pta->pSaveList )
         iret += complist_savelist( cl, pta->pSaveList, pta->saveopts );

      // JUST WRITE DIFFERENCE AND EXIT
      if( pta->pDiffList )
         iret += WriteDiffOut( pta->view, pta->pDiffList, pta->diffopts );

      view_close(pta->view);
      pta->view = 0;
      dwRet = 0;

      goto Thd_End;
   }


   /* if there was only one file, expand it */
   disp_number = view_getrowcount(pta->view);
   //if( disp_number == 0 )
   //{
      // This must be done AFTER resetting gbInitial
      //InvalidateRect( hwndRCD, NULL, TRUE );
   //}
   //else
   if(( disp_number == 1 ) &&
      ( !g_bNoExp        ) )  // -E inhibits expansion of a one
   {
      SetSelection(0);        // select this TOP SINGLE entry
      ToExpand(pta->view);   // and EXPAND this file
   }

   //else
   //{
   //   //InvalidateRect( hwndRCD, NULL, TRUE );
   //}

#if   0  // this was DONE in complist_args()

   if( strcmpi( gszLeft, pta->pFirst ) )
   {
      strcpy( gszLeft, pta->pFirst );
      bChgLf = TRUE;
   }
   if( strcmpi( gszRite, pta->pSecond ) )
   {
      strcpy( gszRite, pta->pSecond );
      bChgRt = TRUE;
   }
#endif   // 0 not needed

#ifndef USE_GLOBAL_RECURSIVE
   ToggleBool( &gbRecur, &bChgRec, pta->fDeep );
#endif // #ifndef USE_GLOBAL_RECURSIVE

Thd_End:

   // exit thread:

   //gmem_free(hHeap, (LPSTR) pta, sizeof(THREADARGS), "ParseArgs" );

   //SetNotBusy();
   if( pta->taFlag & (tf_RightisZip | tf_LeftisZip ) )
      bZip = TRUE;

   //if( !( pta->dwFlag & ( tf_RightisZip | tf_LeftisZip ) ) &&
   //if( !( pta->dwFlag & tf_RightisZip ) ) &&
   if( bZip )
   {
      DWORD dwz = pta->taFlag & (tf_RightisZip|tf_LeftisZip);
      if( dwz == (tf_RightisZip|tf_LeftisZip) )
      {
         sprtf( "Both are zips - %s : %s"MEOR, lpl, lpr );
      }
      else if( dwz & tf_RightisZip )
      {
         sprtf( "Right is a ZIP [%s]"MEOR, lpr );
      }
      else
      {
         sprtf( "Left is a ZIP [%s]"MEOR, lpl );
      }
   }
   else if(( bAutoZip && dwRet ) && // if AUTO-ZIP is ON, and we are NOT exiting after a job
      ( !g_bNoAZip ) )  // -N = no zip option
   {
#ifdef ADD_ZIP_SUPPORT
      //PEDITARGS   pe2 = &g_sZipArgs;
      //LPTSTR   pzip = &pe2->sCmdLn.szZip[0];
      LPTSTR   pzip = &g_szCurZip[0];
      LPTSTR   lpb = &pta->ta_szTmpBuf[0];
      INT i = WriteZIPList( pta, 0 ); // #ifdef ADD_ZIP_SUPPORT
      if( i )
      {
         sprintf(lpb, "Written ZIP file [%s]", pzip );
      }
      else
      {
         sprintf(lpb, "Advice: NO ZIP file [%s]", pzip );
      }

      SBSetTimedTxt( lpb, 15, FALSE );

      sprtf("%s"MEOR, lpb);

#endif // #ifdef ADD_ZIP_SUPPORT

   }

   // saw later this was a DUPLICATE of CopytoGlobal() above!!!
   // ==============================
   CopytoGlobal(pta);
#ifdef   DOTWOCOPIESPLS
   {
      PTHREADARGS pgt = &pcfds->cf_sTARGS.ta_swd_initTA;
      //memcpy( &pcfds->cf_sTARGS.ta_swd_initTA, pta, sizeof(THREADARGS) );   // keep thread arguments
      memcpy( pgt, pta, sizeof(THREADARGS) );   // keep thread arguments
         // fix some 'self' pointers
      pgt->pFirst    = &pgt->szFirst[0];
      pgt->pSecond   = &pgt->szSecond[0];
      pgt->pSaveList = &pgt->szSaveList[0];  // pointer to SAVE output file
      pgt->pDiffList = &pgt->szDiffList[0];  // difference file name pointer
   }
#endif   // #ifdef   DOTWOCOPIESPLS

   gmem_free(hHeap, (LPSTR) pta, sizeof(THREADARGS), "ParseArgs" );

   SetNotBusy();

   dbtot = GetETime(i);
   //   Dbl2Str(GetETime(i), 5 ),
   sprtf( "Ending thread after %s secs - wait %s (tm=%d)"MEOR,
      Dbl2Str(dbtot, 5 ),
      Dbl2Str(dbwait, 5),
      i );

   gbInInitial = FALSE;

   if( dwRet == 0 )
   {
      //WriteDc4wINI();
      //exit(iret);
      // OR MAYBE JUST
      PostMessage( hwndClient, WM_COMMAND, IDM_EXIT, 0 );
   }
   else
   {

      if( disp_number == 0 )
         InvalidateRect( hwndRCD, NULL, TRUE );    // send another PAINT
      // to put up the EMPTY advice ...
   }

   if(ghMux) ReleaseMutex(ghMux);   // HANDLE hMutex   // handle to mutex


#ifdef ADD_LIST_VIEW
   if( !g_bUsrLVOff ) // regardless of the g_bLVOff user INI variable, start with the LISTVIEW off
   {
      if( g_bLVOff )
      {
         Set_LV_ON();
//         InvalidateRect( hwndRCD, NULL, TRUE );    // send another PAINT
         DoResize(hwndClient);
//         BOOL  chg = g_bChgLVOff;
//         Do_IDM_LISTVIEWOFF();
//         g_bChgLVOff = chg;
//         g_bLVOff = uval;  // restore USER's desire of the final state Off or On
      }
   }
#endif // #ifdef ADD_LIST_VIEW

   return dwRet;

} /* wd_initial */


/***************************************************************************
 * Function: wd_dirdialog
 *
 * Purpose:
 *
 * Called on worker thread (not UI thread) to handle a Dir request IDM_DIR
 *
 */
DWORD
wd_dirdialog(LPVOID arg)
{
   DWORD dwret = 0;
        PVIEW view = (PVIEW) arg;

        /* make a COMPLIST using the directory dialog,
         * and advise the view
         */
        if( complist_dirdialog(view) == NULL )
        {
           COMPLIST    cl = 0;
           LPTSTR    lplf = &gszLeftName[0];
           LPTSTR    lprf = &gszRightName[0];
           // user has CANCELLED the SELECT DIRECTORIES dialog
           // try to re-establish the previous "compare"
           if( *lplf && *lprf )
           {
              PCLASTR   pcla = &g_sCLA;
              ZeroMemory(pcla, sizeof(CLASTR));
              pcla->pCaller= "wd_dirdialog - g_sCLA";
              pcla->pLeft  = lplf;  // =&gszLeftName[0];
              pcla->pRight = lprf;  // =&gszRightName[0];
              pcla->pView  = view;
#ifndef USE_GLOBAL_RECURSIVE
              pcla->bDeep  = bCopyDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
              pcla->dwFlg  = g_sCFDLGSTR.cf_sTARGS.ta_sTA.taFlag;
              //pcla->dwFlg  = dlg_flg;
              // could add, but SEEMS redundant
              if( dir_setzipflag( &pcla->dwFlg, lplf, lprf ) )
              {
                  chkme( "WARNING: What was thought to be REDUNDANT is NEEDED!!!"MEOR );
              }
              dir_setzipflag( &pcla->dwFlg, lplf, lprf );
              cl = complist_args( pcla );
           }
           if(cl)
           {
               disp_number = view_getrowcount(view);
               if(disp_number == 0)
                  InvalidateRect( hwndRCD, NULL, TRUE );    // send another PAINT
               dwret = disp_number;
           }
           else
           {
              view_close(view);
           }
        }

        /* all done! */
        SetNotBusy();

        dwret |= 0xBEEF0000;

        return dwret;
}


VOID  Post_Copy_Items( PCFDLGSTR pcfds, BOOL bRet, INT iWasSel )
{
   BOOL        bDnNB = FALSE;  // flag NOT done
   LPTSTR      lpdl, lpdr;
   COMPLIST    cl;

   if((VALIDPCFDS(pcfds) ) &&
      ( bRet ) ) // if a SUCCESSFUL copy
   {
      lpdl = &pcfds->cf_szLeft[0];     // left folder
      lpdr = &pcfds->cf_szRight[0];    // right folder
      //if( gszLeftName[0] && gszRightName[0] )
      if( *lpdl && *lpdr )
      {
         PCLASTR   pcla = &g_sCLA;

         bCopyDeep = view_getrecursive( pcfds->cf_pView );

         view_close(current_view);

         current_view = view_new( hwndRCD );

         SetStatusStg( BT_INITIAL );   // put the comparing message ...

         ZeroMemory(pcla, sizeof(CLASTR));
         pcla->pCaller= "Post_Copy_Items - g_sCLA";
         pcla->pLeft  = lpdl;    //&gszLeftName[0];
         pcla->pRight = lpdr;    // &gszRightName[0];
         pcla->pView  = pcfds->cf_pView;
#ifndef USE_GLOBAL_RECURSIVE
         pcla->bDeep  = bCopyDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
         pcla->bIsExp = pcfds->cf_bExpanded; // was it EXPANDED before
         //pcla->iSel   = pcfds->cf_iSelected; // and the selection that was EXPANDED
         pcla->iSel   = iWasSel;
         pcla->dwFlg  = pcfds->cf_sTARGS.ta_sTA.taFlag;
         // could add, but SEEMS redundant
         if( dir_setzipflag( &pcla->dwFlg, lpdl, lpdr ) )
         {
            chkme( "WARNING: What was thought to be REDUNDANT is NEEDED!!!"MEOR );
         }
         cl = complist_args( pcla );
         if( !cl )
         {
            view_close(current_view);
            sprtf( "WARNING: Successful copy and NEW view FAILED!"MEOR );
         }
         else
         {
            SetNotBusy();  // set NOT busy NOW

            bDnNB = TRUE;  // and flag it DONE

            if( pcla->bIsExp )   // was EXPANDED before
            {
               if( iWasSel != (INT)-1 )
               {
                  // we should SELECT the same (or NEXT), and EXPAND it now
                  //Select_Nearest( pcla->iSel );    // select it or nearest
                  Select_Nearest( iWasSel );    // select it or nearest
                  ToExpand( current_view );
                  FindNextChange( FALSE );   // move the cursor to the FIRST change
                  sprtf( "ADVICE: Successful copy, view redone and expanded next"MEOR );
               }
               else
               {
                  sprtf( "ADVICE: Successful copy, view redone in outline."MEOR );
               }
            }
            else
            {
               disp_number = view_getrowcount(current_view);
               if(disp_number == 0)
               {
                  InvalidateRect( hwndRCD, NULL, TRUE );    // send another PAINT
                  sprtf( "ADVICE: Successful copy, view redone with NO members."MEOR );
               }
               else if( iWasSel != (INT)-1 )
               {
                  Select_Nearest( iWasSel );    // select it or nearest
                  sprtf( "ADVICE: Successful copy, view redone with next selection."MEOR );
               }
               else
               {
                  sprtf( "ADVICE: Successful copy, view redone with NO selection."MEOR );
               }
            }
         }
      }
      else
      {
         sprtf( "WARNING: Copy successful, but do not have LEFT and RIGHT names!"MEOR
            "\tL=[%s]"MEOR
            "\tR=[%s]"MEOR,
            &gszLeftName[0],
            &gszRightName[0] );
      }
   }
   else
   {
      sprtf( "ADVICE: complist-copyfiles() returned FALSE!"MEOR );
   }

   if( !bDnNB )
      SetNotBusy();  // set NOT busy

}

#define  USEPOST

/***************************************************************************
 * Function: wd_copy
 *
 * Purpose:
 *
 * Called on worker thread to do a copy-files operation from
 * MENU item - IDM_COPYFILES
 *
 * Is actually passed PCFDLGSTR pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
 *
 */
DWORD
wd_copy(LPVOID arg)
{
   BOOL        bRet;
   PCFDLGSTR   pcfds = (PCFDLGSTR)arg; // &g_sCFDLGSTR;   // get GLOBAL structure

   //PVIEW       view = (PVIEW) arg;
   PVIEW       view = pcfds->cf_pView;
   BOOL        bIsExp;
   INT         iSel = -1;
#ifndef  USEPOST
   COMPLIST    cl;
   BOOL        bDnNB = FALSE;
   LPTSTR      lpdl, lpdr;
#endif   // !USEPOST

   bIsExp = view_isexpanded(view);  // get whether we were in expanded state
   if( bIsExp )
      iSel = view_getiselect( view );
   else
      iSel = giSelection;

   //pcfds->bExpanded = bIsExp;
   //pcfds->iSelected = iSel;

   bRet = complist_copyfiles( pcfds );

#ifdef   USEPOST
   Post_Copy_Items( pcfds, bRet, iSel );
#else // !USEPOST
   if( bRet )  // if a SUCCESSFUL copy
   {
      lpdl = &pcfds->cf_szLeft[0];     // left folder
      lpdr = &pcfds->cf_szRight[0];    // right folder
      //if( gszLeftName[0] && gszRightName[0] )
      if( *lpdl && *lpdr )
      {
         PCLASTR   pcla = &g_sCLA;

         view_close(current_view);

         current_view = view_new( hwndRCD );

         SetStatusStg( BT_INITIAL );   // put the comparing message ...

         ZeroMemory(pcla, sizeof(CLASTR));
         pcla->pCaller= "wd_copy - g_sCLA";
         pcla->pLeft  = lpdl;    //&gszLeftName[0];
         pcla->pRight = lpdr;    // &gszRightName[0];
         pcla->pView  = current_view;
         pcla->bDeep  = bCopyDeep;
         pcla->bIsExp = bIsExp;     // was it EXPANDED before
         pcla->iSel   = iSel;       // and the selection that was EXPANDED
         pcla->dwFlg  = pcfds->cf_sTARGS.ta_sTA.dwFlag;  // orig ZIP flags (important)
         cl = complist_args( pcla );

         if( !cl )
         {
            view_close(current_view);
            sprtf( "WARNING: Successful copy and NEW view FAILED!"MEOR );
         }
         else
         {
            SetNotBusy();  // set NOT busy NOW
            bDnNB = TRUE;  // and flag it DONE

            sprtf( "ADVICE: Successful copy and view redone"MEOR );
            if( ( pcla->bIsExp ) &&    // was EXPANDED before
                ( pcla->iSel != (INT)-1 ) )
            {
               // we should SELECT the same (or NEXT), and EXPAND it now
               Select_Nearest( pcla->iSel );    // select it or nearest
               ToExpand( current_view );
               FindNextChange( FALSE );   // move the cursor to the FIRST change
            }
            else
            {
               disp_number = view_getrowcount(current_view);
               if(disp_number == 0)
                  InvalidateRect( hwndRCD, NULL, TRUE );    // send another PAINT
            }
         }
      }
      else
      {
         sprtf( "WARNING: Copy successful, but do not have LEFT and RIGHT names!"MEOR
            "\tL=[%s]"MEOR
            "\tR=[%s]"MEOR,
            &gszLeftName[0],
            &gszRightName[0] );
      }
   }
   else
   {
      sprtf( "ADVICE: complist-copyfiles() returned FALSE!"MEOR );
   }

   if( !bDnNB )
      SetNotBusy();  // set NOT busy
#endif   // USEPOST y/n

   return(0xcafe);
}

// TEXTMETRIC g_sFF8;
LOGFONT  g_slfCN = {
   -11, 0, 0, 0, 0,
      0, 0, 0,
      ANSI_CHARSET,
      0, 0,
      DEFAULT_QUALITY,
      FIXED_PITCH | FF_MODERN,
      "Courier New"
};

//HFONT g_hfCN8 = 0;

VOID  CreateFonts( HWND hWnd )
{
   HDC   hdc;
   PLOGFONT plf = &g_slfCN;   // get pointer to LOGFONT structure
//typedef struct tagLOGFONT { 
//  LONG lfHeight; 
//  LONG lfWidth; 
//  LONG lfEscapement; 
//  LONG lfOrientation; 
//  LONG lfWeight; 
//  BYTE lfItalic; 
//  BYTE lfUnderline; 
//  BYTE lfStrikeOut; 
//  BYTE lfCharSet; 
//  BYTE lfOutPrecision; 
//  BYTE lfClipPrecision; 
//  BYTE lfQuality; 
//  BYTE lfPitchAndFamily; 
//  TCHAR lfFaceName[LF_FACESIZE]; 
//} LOGFONT, *PLOGFONT;
   // lfHeight = -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
   // FontInitialize
   // or ANSI_FIXED_FONT Windows fixed-pitch (monospace) system font.
   // or OEM_FIXED_FONT Original equipment manufacturer (OEM) dependent fixed-pitch (monospace) font.
   GetObject (GetStockObject (SYSTEM_FIXED_FONT), sizeof (LOGFONT),
                (LPSTR) &g_sLogFont);
   g_hFixedFont = CreateFontIndirect(&g_sLogFont);
   g_hActFont   = g_hFixedFont; // start with FIXED font, about 12 ...
   memcpy( &g_sLF8bold, &g_sLogFont, sizeof(LOGFONT) );
   // SendMessage(hwnd, WM_SETFONT, (WPARAM) ghFixedFont, 0L);
   //hdc = GetDC(NULL);
   hdc = GetDC(hWnd);
   if(hdc)
   {
      INT   iy = GetDeviceCaps(hdc, LOGPIXELSY);
      INT   iht= g_sLogFont.lfHeight;
      // lfHt = (PointSize * iy) / 72
      // Pts = (lfHt * 72) / iy
// Created fixed font of point size 11.
// Name=[Fixedsys]
      sprtf( "Created fixed font of point size %d."MEOR
         "Name=[%s]"MEOR,
         MulDiv( g_sLogFont.lfHeight, 72, iy ),
         &g_sLogFont.lfFaceName[0] );

      g_sLogFont.lfHeight = -MulDiv(8, iy, 72);
      g_sLF8bold.lfHeight = -MulDiv(8, iy, 72);
      g_sLF8bold.lfWeight = FW_BOLD;
      g_hFF8bold    = CreateFontIndirect(&g_sLF8bold);

      g_hFixedFont8 = CreateFontIndirect(&g_sLogFont);
      if( g_sLogFont.lfHeight != plf->lfHeight )
         plf->lfHeight = -MulDiv(8, iy, 72);

      g_hfCN8 = CreateFontIndirect(plf);

      g_sLogFont.lfHeight = iht;
      if( g_hFixedFont8 )
      {
         TEXTMETRIC * ptm = &g_sFF8;
         HFONT hof = SelectObject(hdc, g_hFixedFont8);
         GetTextMetrics(hdc, ptm);
         g_iYChar8 = (int)(ptm->tmHeight + ptm->tmExternalLeading);
         g_iXChar8 = (int)ptm->tmAveCharWidth;
         //if( g_hFixedFont )
         if( g_hActFont )
         {
            ptm = &g_sFF12;
            GetTextMetrics(hdc, ptm);
            g_iYChar12 = (int)(ptm->tmHeight + ptm->tmExternalLeading);
            g_iXChar12 = (int)ptm->tmAveCharWidth;
            // SET the current AVERAGE font sizes for the
            // g_hActFont - to be changed when font changed!!!
            g_iXChar = g_iXChar12;  // remember if proportional
            g_iYChar = g_iYChar12;  // then these are 'only' a guide

         }

         // return to original FONT
         hof = SelectObject(hdc, hof);
      }

      // at END ensure HDC released
      ReleaseDC(hWnd,hdc); // release in WM_CREATE
   }
}

VOID  DeleteFonts( VOID )
{
   if( g_hFixedFont )   // = CreateFontIndirect(&g_sLogFont);
      DeleteObject( g_hFixedFont );
   if( g_hFixedFont8 )  // = CreateFontIndirect(&g_sLogFont);
      DeleteObject( g_hFixedFont8 );
   if( g_hfCN8 )  // = CreateFontIndirect(plf);
      DeleteObject( g_hfCN8 );
   if( g_hFF8bold )
      DeleteObject( g_hFF8bold );
   g_hFF8bold    = 0;
   g_hFixedFont  = 0; // = CreateFontIndirect(&g_sLogFont);
   g_hFixedFont8 = 0;  // = CreateFontIndirect(&g_sLogFont);
   g_hfCN8       = 0;   // = CreateFontIndirect(plf);
}

VOID  Do_WM_CREATE( HWND hWnd )
{
   g_hWnd = hWnd; // MAIN APPLICATION WINDOW g_hwndMain, g_hMain

   /* initialise menu options to default/saved
    * option settings
    */

//   CheckMenuItem(hMenu, line_numbers, MF_CHECKED);
//   CheckMenuItem(hMenu, expand_mode, MF_CHECKED);

   //CheckMenuItem(hMenu, IDM_IGNBLANKS,
   //        ignore_blanks ? MF_CHECKED : MF_UNCHECKED);
   CheckMenuItem(hMenu, IDM_PICTURE,
           picture_mode ? MF_CHECKED : MF_UNCHECKED);

   /* nothing currently displayed */
   DisplayMode = MODE_NULL;

   CreateFonts( hWnd );    // get/create FONTS to be used

#ifdef   ADDSTATUS
   SBCreate( hWnd, &g_sSB, !gbAddS  ); // = status bar
#endif   // #ifdef   ADDSTATUS
}


//DWORD    WriteDiffArgs( PVIEW view, LPTSTR lpf, DWORD dwo, INT icnt, BOOL bMB )
DWORD    WriteDiffArgs( PCFDLGSTR pcfds, INT icnt, BOOL bMB )
{
   PVIEW       view;
   LPTSTR      lpf;
   DWORD       dwo;
   DWORD       bRet  = 0;
   LONG        rows;
   LPTSTR      lpb   = GetNxtBuf(); // &gszTmpBuf[0];
   HANDLE      h     = 0;
   LONG        i;
   INT         state, dcnt;
   PTARGS      pta;

   if( !VALIDPCFDS(pcfds) )
      return 0;

   pta = &pcfds->cf_sTARGS;
   view = pcfds->cf_pView;
#ifdef   COMBARGS
   pta->ta_sTA.view = view;
#else // !#ifdef   COMBARGS
   pta->ta_pView   = view;
#endif   // #ifdef   COMBARGS
   rows  = view_getrowcount(view);
   pta->ta_iCallID = taCID_DifDlg;
   lpf = &pta->ta_sDI.di_szFile[0];
   dwo = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;
   h = OpnFil( lpf, ( ( dwo & APPEND_FILE ) ? TRUE : FALSE ) );
   if( VFH(h) )
   {
      sprtf( "Writing DIFFERENCES of %d file(s) to"MEOR
         "\t[%s]"MEOR,
         icnt,
         lpf );
      g_bNoUpdate = TRUE;  // hold off on the updates
      dcnt = 0;

      for( i = 0; i < rows; i++ )
      {
         state = view_getstate(view, i);
         if( state == STATE_DIFFER )
         {
            giSelection = i;
            // is this really needed? since the window has usually
            // been ICONISED ...
            dcnt++;
            sprintf( lpb, "Writing %d of %d ...", dcnt, icnt );
            SetStatus(lpb);
            // ===========================================
            // move the DIFFERENT files into EXPANDED mode
            if( ToExpand(view) )
            {
               COMPITEM ci = view_getitem(view, 0);

               if( compitem_settargs( ci, pta ) )
                  bRet += compitem_writediff( ci, &h, dwo, pta );
               else
                  chkme( "HEY! Failed to get COMPITEM for this expansion!!!"MEOR );

               // then back to OUTLINE
               ToOutline(view);
               // ====================
            }
            else
            {
               chkme( "OOPS! Failed to get EXPANDED mode!!!"MEOR );
            }
            // ===========================================
         }

         if( !VFH(h) )
            break;
      }

      if( VFH(h) )
         CloseHandle(h);

      g_bNoUpdate = FALSE; // now we can do full updates

   }
   else if( bMB )
   {
      sprintf(lpb, "ERROR: Unable to create file"MEOR
                  "[%s]",
                  lpf );
      MB(hwndClient,
                  lpb,
                  APPNAME,
                  MB_ICONINFORMATION|MB_OK);
   }

   // all done - clean up

   return bRet;

}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : WriteDiffOut
// Return type: DWORD 
// Arguments  : PVIEW view
//            : LPTSTR lpf
//            : DWORD dwo
// Description: Only called from wd_initial if a DIFFERENCE file was given
//              and thus we are going to wirte the difference AND EXIT!!!
///////////////////////////////////////////////////////////////////////////////
DWORD  WriteDiffOut( PVIEW view, LPTSTR lpf, DWORD dwo )
{
   DWORD       bRet  = 0;
   //PCFDLGSTR   pcfds = &g_cfds;
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   LONG        rows  = view_getrowcount(view);
   LPTSTR      lpb   = GetNxtBuf(); // &gszTmpBuf[0];
   HANDLE      h     = 0;
   LONG        i;
   INT         state, icnt;
   DWORD       dwFlag;

   if( !rows )
      return 0;

   if( view_isexpanded(view) )
      return 0;

   //pcfds->bExpanded = FALSE;
   //pcfds->iSelected = giSelection; // selected row in table (if not -1)
   //pcfds->pView     = view;
   icnt = 0;
   for( i = 0; i < rows; i++ )
   {
      state = view_getstate(view, i);
      dwFlag = view_getflag(view, i);
      complist_setpcfds( pcfds, state, dwFlag );
      if( state == STATE_DIFFER )
         icnt++;
   }

   if( !icnt )
      return 0;

   sprintf(lpb,
      "There are %d different files",
      icnt );

   //bRet =  WriteDiffArgs( view, lpf, dwo, icnt, FALSE );
   if (*lpf)
       strcpy(pcfds->cf_sTARGS.ta_sDI.di_szFile, lpf);  // copy the DIFF output filename
   if (pcfds->cf_sTARGS.ta_sDI.di_dwOpts == 0) {
       if (pcfds->cf_sTARGS.ta_sTA.diffopts == 0)
           pcfds->cf_sTARGS.ta_sDI.di_dwOpts = INCLUDE_ALLDIFF;
       else
           pcfds->cf_sTARGS.ta_sDI.di_dwOpts = pcfds->cf_sTARGS.ta_sTA.diffopts;
   }
   bRet =  WriteDiffArgs( pcfds, icnt, FALSE );

   //giSelection = pcfds->cf_iSelected; // selected row in table (if not -1)

   return bRet;

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Do_IDM_WRITEDIFF
// Return type: VOID 
// Argument   : PVIEW view
// Description: Depending on the current MODE (outline = list of files, or
//              expanded = list of 'difference' in one file) prompt for an
// output file name, and if given one, and OK clicked, then WRITE the
// differences. This can be a LONG process if in OUTLINE (file list) mode,
// since we have to 'select' each 'different' file pair, load and do the
// compare, and wrie a difference file. This process should be allocated
// to a thread so that we can still activate the UI for other actions.
//
///////////////////////////////////////////////////////////////////////////////
VOID  Do_IDM_WRITEDIFF( PVIEW view )
{
   BOOL  bSetBusy = FALSE;
   DWORD       dwFlag;

   if( !view )
      return;

   if( !SetBusy( BT_WRITEDIFF ) )
   {
      BusyError( "Got IDM_WRITEDIFF" );
      return;
   }

    bSetBusy = TRUE;
   //if( !view_haschange( view, FALSE ) )
   //   return;

   if( view_isexpanded(view) )
   {
      // NOTE: If view is EXPANDED then the row requested in view_getitem()
      // is IGNORED, since the COMPITEM is stored on EXPANSION
      compitem_savediff( view_getitem(view, 0), NULL, gdwDiffOpts ); // def = INC_ALLXSAME
   }
   else  // if( SetBusy( BT_WRITEDIFF ) )
   {
      // in OUTLINE mode - I want to select EACH file in the LIST,
      // and IF there are TWO (a left AND a right),
      // I want to EXPAND the entry, and WRITE the difference, to a
      // user chose FILE name

      // An earlier attempt was complist_savediff();
      // must process each file with a DIFFERENCE
      //LONG  winrows = SendMessage( hwndRCD, TM_ROWCOUNT, 0, 0 );
      LONG_PTR    rows = view_getrowcount(view);
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;
      LONG_PTR    i;
      INT         state, icnt;
      LPTSTR      lpb = &pcfds->cf_szMsg[0];
      LPTSTR      lpf = &pcfds->cf_sTARGS.ta_sDI.di_szFile[0];

      //Setg_szNxtDif();   // do this NOW since it uses gszTmpBuf!!!
      if( *lpf == 0 )
      {
         Setg_szNxtDif();
         strcpy(lpf, g_szNxtDif);   // ensure we SUGGEST a file name
      }

      //pcfds->bExpanded = view_isexpanded( view );
      //pcfds->iSelected = giSelection; // selected row in table (if not -1)
      //pcfds->pView     = view;
      icnt = 0;
      for( i = 0; i < rows; i++ )
      {
         // since in OUTLINE mode
         // it is a compitem state
         // state = compitem_getstate(view->pItems[row]);
         state = view_getstate(view, i);
         dwFlag = view_getflag(view, i);
         complist_setpcfds( pcfds, state, dwFlag );
         if( state == STATE_DIFFER )
            icnt++;
      }
      if( !icnt )
         goto Dn_WD;

      sprintf(lpb,
         "There are %d items in the list using [", (int)rows );
      OptionsToString( lpb, FALSE );
      sprintf( EndBuf(lpb), "]"MEOR
         "With left=%d, right=%d,"MEOR
         "same=%d, and diff=%d (n=%d)",
         pcfds->dwLeft,
         pcfds->dwRite,
         pcfds->dwSame,
         pcfds->dwDiff,
         pcfds->dwNewer );
      if( pcfds->dwUnk )
      {
         sprintf( EndBuf(lpb), " And others=%d???",
            pcfds->dwUnk );
      }

      pcfds->cf_sTARGS.ta_sDI.di_pDFList = &gsDiffList;  // difference file list
      pcfds->cf_sTARGS.ta_sDI.di_dwOpts  = gdwDiffOpts;  // fill in GLOBAL options
      if( pcfds->cf_sTARGS.ta_sDI.di_dwWidth == 0 )
      {
         if( gdwWrapWid == 0 )
            pcfds->cf_sTARGS.ta_sDI.di_dwWidth = DEF_WRAP_WIDTH;
         else
            pcfds->cf_sTARGS.ta_sDI.di_dwWidth = gdwWrapWid;
      }
      //sprintf( EndBuf(lpb), "Proceed to write file"MEOR
      //   "[%s]?",
      //   g_szNxtDif );
      i = Do_IDD_SAVEDIFF(pcfds); // pass information structure
      //i = MB(hwndClient,
      //   lpb,
      //   APPNAME,
      //   MB_ICONINFORMATION|MB_YESNO);
      //if( i == IDYES )
      if( i == IDOK )
      {
         DWORD    dwo = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;
         //strcpy( &g_szNxtDif[0], &gszDif2Fil[0] );
         //WriteDiffArgs( view, &g_szNxtDif[0], gdwDiffOpts, icnt, TRUE );
         //WriteDiffArgs( view, lpf, dwo, icnt, TRUE );
         WriteDiffArgs( pcfds, icnt, TRUE );
         //giSelection = pcfds->cf_iSelected; // selected row in table (if not -1)
         SetNotBusy();
         bSetBusy = FALSE;
         if( dwo != gdwDiffOpts )
         {
            sprtf( "Updated INI gdwDiffOPts to %#x."MEOR, dwo );
            gdwDiffOpts = dwo;
            bChgDO = TRUE;
         }
         Add2SList( &gsDiffList, &bChgDLst, lpf );
         // gszDif2Fil / bChgDF
         //{ szOpt, szDF,   it_String, &gszDif2Fil[0],          &bChgDF,  0, 0 },
         if( strcmp( lpf, gszDif2Fil ) )
         {
            strcpy( gszDif2Fil, lpf );
            bChgDF = TRUE;
         }
         if( gdwWrapWid != pcfds->cf_sTARGS.ta_sDI.di_dwWidth )
         {
// gdwWrapWid / bChgWW
//TCHAR szWW[]  = "WrapWidth";
            //gdwWrapWid = pdd->di_dwWidth;
            gdwWrapWid = pcfds->cf_sTARGS.ta_sDI.di_dwWidth;
            bChgWW = TRUE;
         }
      }
   }

Dn_WD:

   if( bSetBusy )
      SetNotBusy();
}

// BT_FILEDLG
VOID  Do_IDM_FILE( HWND hWnd )
{
   /* select two files and compare them */
   if( SetBusy( BT_FILEDLG ) )
   {
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;
      
      bCopyDeep = view_getrecursive(current_view);

      /* close the current view */
      view_close(current_view);

      /* make a new empty view */
      current_view = view_new(hwndRCD);

      /* make a COMPLIST using the files dialog,
       * and advise the view
       */
      if( complist_filedialog(current_view) == NULL )
      {
         LPTSTR   lplf = &gszLeftName[0];
         LPTSTR   lprf = &gszRightName[0];
           if( *lplf && *lprf )
           {
              COMPLIST  cl;
              PCLASTR   pcla = &g_sCLA;
              ZeroMemory(pcla, sizeof(CLASTR));
              pcla->pCaller= "Do_IDM_FILE - g_sCLA";
              pcla->pLeft  = lplf;  // = &gszLeftName[0];
              pcla->pRight = lprf;  // = &gszRightName[0];
              pcla->pView  = current_view;
#ifndef USE_GLOBAL_RECURSIVE
              pcla->bDeep  = bCopyDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
              // this FLAG should be correct
              pcla->dwFlg  = g_sCFDLGSTR.cf_sTARGS.ta_sTA.taFlag;
              cl = complist_args( pcla );
              if( !cl )
              {
                 view_close(current_view);
              }
           }
           else
           {
              view_close(current_view);
           }
      }

      /* all done! */
      SetNotBusy();
      
   }
   else
   {
      BusyError( "Got IDM_FILE" );
   }
}

VOID  Do_IDM_DIR( HWND hWnd )
{
   /* read two directory names, scan them and
    * compare all the files and subdirs.
    */
   sprtf( "IDM_DIR: Creating thread to run wd_dirdialog..."MEOR );

   if( SetBusy( BT_DIRDLG ) )
   {
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;

      bCopyDeep = view_getrecursive(current_view);

      /* close the current view */
      view_close(current_view);

      /* make a new empty view */
      current_view = view_new(hwndRCD);

      //sprtf( "Creating thread to run wd_dirdialog..."MEOR );

      ghThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wd_dirdialog,
                           (LPVOID) current_view, 0, &threadid);

      if( ghThread == NULL )
      {
         wd_dirdialog( (LPVOID) current_view);
      }

   }
   else
   {
      BusyError( "Got IDM_DIR" );
   }
}

DWORD WINAPI wd_resetview( LPVOID pv )   // thread data
{
   DWORD    dw = 0x0acceed;
   COMPLIST cl;
   PVIEW  view = (PVIEW)pv;

   INT   i = SetBTime();
   PCLASTR   pcla = &g_sCLA;
   LPTSTR   lplf = &gszLeftName[0];
   LPTSTR   lprf = &gszRightName[0]; 
   ZeroMemory(pcla, sizeof(CLASTR));

   pcla->pCaller= "wd_resetview - g_sCLA";
   pcla->pLeft  = lplf; // = &gszLeftName[0];
   pcla->pRight = lprf; // &gszRightName[0];
   pcla->pView  = view;
#ifndef USE_GLOBAL_RECURSIVE
   pcla->bDeep  = gbRecur;
#endif // #ifndef USE_GLOBAL_RECURSIVE
   pcla->dwFlg  = g_sCFDLGSTR.cf_sTARGS.ta_sTA.taFlag;
   cl = complist_args( pcla );
   if( !cl )
   {
      dw = 0x0dead;
      view_close(view);
   }

   /* all done! */
   SetNotBusy();  // NOTE: This closes and ZEROS ghThread

   SetAppTitle(); // and a potential change in the Title line

   sprtf( "Done new view in %s secs. (tm=%d)"MEOR,
      Dbl2Str( GetETime(i), 5 ),
      i );

   return dw;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Do_FULL_REFRESH
// Return type: VOID 
// Arguments  : VOID )
// Description: toss it ALL
//            : and re-read directories &| files
///////////////////////////////////////////////////////////////////////////////
VOID  Do_FULL_REFRESH( VOID ) // toss it ALL, and re-read directories &| files
{
   if( SetBusy( BT_REFRESH ) )
   {
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;

      if( gszLeftName[0] && gszRightName[0] )
      {
         /* close the current view */
         view_close(current_view);

         /* make a new empty view */
         current_view = view_new(hwndRCD);

         ghThread = CreateThread(NULL,
                              0,
                              wd_resetview,
                              (LPVOID) current_view,
                              0,
                              &threadid );

         if(ghThread == NULL)
         {
            wd_resetview( (LPVOID) current_view);
         }
      }
      else
      {
         SetNotBusy();
      }
   }
   else
   {
      BusyError( "Got Full Refresh" );
   }
}

VOID  Do_IDM_REFRESH( VOID )
{
   Do_FULL_REFRESH();
}

VOID  Do_IDM_REVERSE( VOID )
{
   LPTSTR   lpdl = &gszLeftName[0];
   LPTSTR   lpdr = &gszRightName[0];
   if( *lpdl && *lpdr )
   {
      LPTSTR   lpb = &g_szBuf1[0];
      // adjust pta->taFlag & (tf_RightisZip | tf_LeftisZip )
      DWORD flag = g_sCFDLGSTR.cf_sTARGS.ta_sTA.taFlag; // extract FLAG
      if( flag & tf_RightisZip ) {
         flag &= ~(tf_RightisZip);
         flag |= tf_LeftisZip;
      }
      if( flag & tf_LeftisZip ) {
         flag &= ~(tf_LeftisZip);
         flag |= tf_RightisZip;
      }
      g_sCFDLGSTR.cf_sTARGS.ta_sTA.taFlag = flag; // potentially ADJUSTED flag
      strcpy( lpb,  lpdl );
      strcpy( lpdl, lpdr );
      strcpy( lpdr, lpb  );
      Do_FULL_REFRESH();
   }
}

// Ignore Date/Time difference in file compare
VOID  Do_IDM_OPTIGNOREDT( VOID )
{
   ToggleBool( &gbIgnDT, &gbChgIDT, !gbIgnDT );
   // FIX20110201 - Flip the FULL ignore as well
   if (gbIgnDT) {
       if (!gbIgnDT2)
           ToggleBool( &gbIgnDT2, &gbChgIDT2, !gbIgnDT2 );
   } else {
       // maybe not really required, but keep them in SYNC
       if (gbIgnDT2)
           ToggleBool( &gbIgnDT2, &gbChgIDT2, !gbIgnDT2 );
   }
   Do_FULL_REFRESH();
}

VOID  Do_IDM_OPTADDROW( VOID )
{
   ToggleBit( &gdwFileOpts, INCLUDE_LINENUMS, &bChgFO,
      (( gdwFileOpts & INCLUDE_LINENUMS ) ? FALSE : TRUE ) );

//   ToggleBool( &gbAddRow, &gbChgRow,
//      (( gdwFileOpts & INCLUDE_LINENUMS ) ? TRUE : FALSE ) );

   Do_FULL_REFRESH();
}

VOID  Do_IDM_OPTEXACT( VOID )
{
   ToggleBool( &gbExact, &bChgExa, !gbExact );
   Do_FULL_REFRESH();
}
VOID  Do_IDM_OPTALTDISPLAY( VOID )
{
   ToggleBool( &gbSimple, &gbChgSim, !gbSimple );
   Do_FULL_REFRESH();
}

VOID  Do_IDM_RECURSIVE( VOID )
{
#ifndef USE_GLOBAL_RECURSIVE    // FIX20091125
   ToggleBool( &gbRecur, &bChgRec, !gbRecur );
#else // !#ifndef USE_GLOBAL_RECURSIVE
    if ( g_bNOTRecursive )
        g_bNOTRecursive = FALSE; // FIX20091125
    else
        g_bNOTRecursive = TRUE; // FIX20091125
#endif // #ifndef USE_GLOBAL_RECURSIVE y/n

   Do_FULL_REFRESH();
}

// toggle the EXCLUDE PER LIST
VOID  Do_IDM_OPTEXCLUDE( VOID )
{
   ToggleBool( &gbExclude, &bChgExcl, !gbExclude );
   Do_FULL_REFRESH();
}

VOID  Do_ID_VIEW_NOEXCLUDES( VOID )
{
   // ID_VIEW_NOEXCLUDES
   ToggleBool( &bNoExcludes, &bChgNoExcl, !bNoExcludes );
   Do_FULL_REFRESH();
}


BOOL  SetBusyCopy( LPTSTR pmsg )
{
   if( SetBusy( BT_COPYFILES ) )
      return TRUE;
   else
      BusyError( pmsg );

   return FALSE;
}

VOID  Do_IDM_COPYFILES( HWND hWnd )
{
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
   /*
    * copy files that are same/different to a new
    * root directory. dialog box allows user
    * to select new root and inclusion options
    */
   if( current_view == NULL )
   {
           MB(hWnd,
               LoadRcString(IDS_CREATE_DIFF_LIST),
               APPNAME, MB_OK|MB_ICONSTOP);
           return;
   }

   UpdCopyDelStrs( pcfds );   // update the (presently GLOBAL) structure

   if( SetBusy( BT_COPYFILES ) )
   {

      bCopyDeep = view_getrecursive(current_view);


      ghThread = CreateThread(NULL,
         0,
         (LPTHREAD_START_ROUTINE)wd_copy,
         (LPVOID) pcfds,   // = &g_sCFDLGSTR;   // get GLOBAL structure
         0,
         &threadid );

      if( ghThread == NULL )
      {
         wd_copy( (LPVOID) pcfds ); // &g_sCFDLGSTR;   // get GLOBAL structure
      }
   }
   else
   {
      BusyError( "Got IDM_COPYFILES" );
   }
}

VOID  Testpcfds( PCFDLGSTR pcfds )
{
   // just for DIAG
   // **************************************************************************
   if( pcfds->cf_pView != current_view )
   {
      chkme( "WARNING: CFDLGSTR has incorrect VIEW pointer!!! (%#x vs %#x)"MEOR,
         pcfds->cf_pView,
         current_view );
      pcfds->cf_pView = current_view;
   }
   //if( ( giSelection != -1 ) &&
//  g_sCFDLGSTR.cf_iSelected = giSelection;
   if( pcfds->cf_iSelected != giSelection )
   {
      chkme( "WARNING: CFDLGSTR has incorrect SELECTION!!! (%d vs %d)"MEOR,
         pcfds->cf_iSelected,
         giSelection );
      pcfds->cf_iSelected = giSelection;  // update it now
   }
   if( pcfds->cf_bExpanded != view_isexpanded( current_view ) )
   {
      chkme( "WARNING: CFDLGSTR has incorrect EXPANDED!!! (%s vs %s)"MEOR,
         ( pcfds->cf_bExpanded ? "TRUE" : "FALSE" ),
         ( view_isexpanded( current_view ) ? "TRUE" : "FALSE" ) );
   }
   // **************************************************************************
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : CopyOneIsOK
// Return type: BOOL 
// Argument   : PCFDLGSTR pcfds
// Description: NOTE: This ONLY verifies that a LEFT to RIGHT copy of the
//              currently EXPANDED file 'looks' ok to be COPIED
// or currently SELECTED file if in OUTLINE 'looks' ok to be COPIED
///////////////////////////////////////////////////////////////////////////////
BOOL  CopyOneIsOK( PCFDLGSTR  pcfds )
{
   BOOL  bRet = FALSE;
   //if(( VALIDPCFDS(pcfds)   ) &&
   //   ( pcfds->cf_bExpanded ) )
   if( VALIDPCFDS(pcfds) )
   {
      PVIEW view  = pcfds->cf_pView; // get the PVIEW
      PCPYTST pct = &pcfds->cf_sCopyTest;
      ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
      pct->ct_pcfds   = pcfds;    // have primary pointer ALWAYS available
      if(view)
      {
         //COMPLIST cl = view_getcomplist(view);
         // NOTE: view_getitem() already handles EXPANDED or NOT
         COMPITEM ci = view_getitem( pcfds->cf_pView, pcfds->cf_iSelected );
         //if( pcfds->cf_bExpanded )
         //   ci = view_getcompitem(view);
         //else
         //   ci = view_getitem( pcfds->cf_pView, pcfds->cf_iSelected );
         if(ci)
         {
            DIRITEM diritem = file_getdiritem(compitem_getleftfile(ci));
            pct->ct_ci      = ci;
            pct->ct_iState  = compitem_getstate(ci);    // state of the entry
            pct->ct_dwFlag  = compitem_getflag(ci);
            if(diritem)
            {
               pct->ct_diritem = diritem;
               strcpy( &pcfds->cf_szDest[0], &pcfds->cf_szRight[0] );
               if( dir_copytest( pcfds, pct ) )
               {
                  pct->ct_dwFlag |= flg_Copy1OK;   // Copy ONE is valid
                  bRet = TRUE;
               }
            }
         }
      }
   }

   // just for DIAG
   Testpcfds( pcfds );

   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : DeleteOneIsOK
// Return type: BOOL 
// Argument   : PCFDLGSTR pcfds
// Description: NOTE: This ONLY verifies that a LEFT or RIGHT ONLY file
//              can be DELETED
///////////////////////////////////////////////////////////////////////////////
BOOL  DeleteOneIsOK( PCFDLGSTR pcfds )
{
   BOOL  bRet = FALSE;
   //if(( VALIDPCFDS(pcfds)   ) &&
   //   ( pcfds->cf_bExpanded ) )
   if( VALIDPCFDS(pcfds) )
   {
      PCPYTST  pct = &pcfds->cf_sDeleTest;
      PVIEW view   = pcfds->cf_pView;
      ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
      pct->ct_pcfds   = pcfds;    // have primary pointer ALWAYS avaiable
      if(view)
      {
         //COMPLIST cl = view_getcomplist(view);
         // NOTE: view_getitem() already handles EXPANDED yes/no
         COMPITEM ci = view_getitem( view, pcfds->cf_iSelected );
         //if( pcfds->cf_bExpanded )
         //   ci = view_getcompitem(view);
         //else
         //   ci = view_getitem( pcfds->cf_pView, pcfds->cf_iSelected );
         if(ci)
         {
            INT      state = compitem_getstate(ci);
            DIRITEM  diritem = 0;
            LPTSTR   lpd = &pcfds->cf_szDest[0];

            pct->ct_ci      = ci;
            pct->ct_iState  = state;    // state of the entry
            pct->ct_dwFlag  = compitem_getflag(ci);
            //if( pcfds->cf_sCPYTST.ct_iState == STATE_LEFTONLY )
            if( state == STATE_FILELEFTONLY )
            {
               diritem = file_getdiritem(compitem_getleftfile(ci));
               strcpy( lpd, &pcfds->cf_szLeft[0] );
            }
            else if( state == STATE_FILERIGHTONLY )
            {
               diritem = file_getdiritem(compitem_getrightfile(ci));
               strcpy( lpd, &pcfds->cf_szRight[0] );
            }

            if(diritem)
            {
               DWORD    dwo;
               pct->ct_diritem = diritem;
               dwo = pcfds->dwCpyOpts;
               if( state == STATE_FILERIGHTONLY )
               {
                  if( dir_deletetest( pct, diritem ) )
                  {
                     pct->ct_dwFlag |= flg_Dele1OK;   // DELETE ONE is valid
                     bRet = TRUE;
                  }
               }
#ifdef   ADDUPDATE2
               pcfds->dwCpyOpts |= DELETE_ONLY;
               if( dir_copytest( pcfds, pct ) )
               {
                  pct->ct_dwFlag |= flg_Dele1OK;   // DELETE ONE is valid
                  bRet = TRUE;
               }
               pcfds->dwCpyOpts = dwo;
#endif   // #ifdef   ADDUPDATE2
            }
         }
      }
   }

   // just for DIAG
   Testpcfds( pcfds );

   return bRet;
}

VOID  UpdCopyDelStrs( PCFDLGSTR   pcfds )
{
   if( VALIDPCFDS(pcfds) )
   {
      DWORD dwo, dwv;
      dwo = pcfds->dwCpyOpts;
      dwv = pcfds->dwVerFlag;
      //pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_SAME;
      //pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwCpyOpts = INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwVerFlag |= MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER;
      CopyOneIsOK( pcfds );
      //pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      //pcfds->dwCpyOpts = COPY_FROMLEFT|INC_ALLONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwCpyOpts = INC_ALLONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwVerFlag |= MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER;
      DeleteOneIsOK( pcfds );
      pcfds->dwCpyOpts = dwo;
      pcfds->dwVerFlag = dwv;
   }
}


VOID  Do_IDM_UPDFILE( HWND hWnd )
{
   BOOL  bRet = FALSE;
   DWORD dwo, dwv;
   PCFDLGSTR   pcfds  = &g_sCFDLGSTR;
   INT   iSel = pcfds->cf_iSelected;   // extract current SELECTION

   pcfds->cf_liCopied.QuadPart = pcfds->cf_liDeleted.QuadPart =
      pcfds->cf_liToBin.QuadPart = 0;
   if( SetBusy( BT_COPYFILES ) )
   {
      dwo = pcfds->dwCpyOpts;
      dwv = pcfds->dwVerFlag;
      //pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwCpyOpts = INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwVerFlag |= MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER;
      if( CopyOneIsOK( pcfds ) )
      {
         PCPYTST pct = &pcfds->cf_sCopyTest;
         pct->ct_dwFlag &= ~(flg_User);   // remove flag as a USER skip
         if( dir_copy( pct, pcfds ) == FALSE )
         {
            // WARN: The file UPDATE failed!!!
            if( !( pct->ct_dwFlag & flg_User ) )   // flag as a USER skip
            {
               LPTSTR   lpm = GetNxtBuf();  // &gszTmpBuf[0];
               sprintf(lpm,
                  "WARNING: It appears the copy of"MEOR
                  "[%s] to"MEOR
                  "[%s] folder"MEOR
                  "FAILED!",
                  &pct->ct_szCopyFull[0], // copy the FULL name
                  &pcfds->cf_szDest[0] );
               MB( hWnd, lpm, "UPDATE FAILED", MB_ICONINFORMATION | MB_OK);
            }
         }
         else
         {
            bRet = TRUE;
         }
         Post_Copy_Items( pcfds, bRet, iSel );
      }
      pcfds->dwCpyOpts = dwo;
      pcfds->dwVerFlag = dwv;
   }
   else
   {
      BusyError( "Got IDM_UPDFILE" );
   }
}

extern   BOOL complist_deleteright( PVIEW view, PLE pHead );

VOID  Do_IDM_DELETEFILE( HWND hWnd )
{
   BOOL  bRet = FALSE;
   DWORD dwo, dwv, dwc;
   PCFDLGSTR   pcfds  = &g_sCFDLGSTR;
   INT   iSel = pcfds->cf_iSelected;   // extract current SELECTION
   LPTSTR   lpm = &gszTmpBuf[0];
   LIST_ENTRY  le;

   le.Blink = &le;
   le.Flink = &le;

   if( !gbDelOn )
   {
      dwc = GetOutlineCount();   // if these are all, say ONLY in the RIGHT
      sprintf(lpm,
                  "ADVICE: The DELETE of %d files"MEOR
                  "FAILED since the MASTER Delete switch is OFF!",
                  dwc );

      MB( hWnd, lpm, "DELETE INHIBITED", MB_ICONINFORMATION | MB_OK);
      SetNotBusy();  // set NOT busy
      return;
   }

   dwc = GetOutlineCount();   // if these are all, say ONLY in the RIGHT
   if(( dwc                       ) &&
      (g_sOutCnt.cnt2 == g_dwRite ) &&
      ( dwc == g_dwRite           ) &&
      ( SetBusy( BT_DELETEFILES ) ) )
   {
      dwo = pcfds->dwCpyOpts;
      dwv = pcfds->dwVerFlag;
      bRet = complist_deleteright( current_view, &le );
      if( bRet )
      {
         // we have DELETE right only file
      }

      KillLList( &le );

      pcfds->dwCpyOpts = dwo;
      pcfds->dwVerFlag = dwv;

      //SetNotBusy();  // set NOT busy
      Post_Copy_Items( pcfds, bRet, iSel );

   }
   else
   {
      BusyError( "Got IDM_DELETEFILE" );
   }
}

VOID  Do_IDM_DELETEONE( HWND hWnd )
{
   BOOL  bRet = FALSE;
   DWORD dwo, dwv, dwc;
   PCFDLGSTR   pcfds  = &g_sCFDLGSTR;
   INT   iSel = pcfds->cf_iSelected;   // extract current SELECTION
   LPTSTR   lpm = GetNxtBuf();  // &gszTmpBuf[0];

   dwc = GetOutlineCount();   // if these are all, say ONLY in the RIGHT

   pcfds->cf_liCopied.QuadPart = pcfds->cf_liDeleted.QuadPart =
      pcfds->cf_liToBin.QuadPart = 0;
   if( SetBusy( BT_DELETEFILES ) )
   {
      if( !gbDelOn )
      {
         sprintf(lpm,
                  "ADVICE: The DELETE of %d files"MEOR
                  "FAILED since the MASTER Delete ON is OFF!",
                  dwc );

         MB( hWnd, lpm, "DELETE INHIBITED", MB_ICONINFORMATION | MB_OK);
         SetNotBusy();  // set NOT busy
         return;

      }

      dwo = pcfds->dwCpyOpts;
      dwv = pcfds->dwVerFlag;
//      pcfds->dwCpyOpts = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
//      pcfds->dwCpyOpts = COPY_FROMLEFT|INC_ALLONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwCpyOpts = INC_ALLONLY|INCLUDE_ALLDIFF|INCLUDE_SAME;
      pcfds->dwVerFlag |= MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER;
      if( DeleteOneIsOK( pcfds ) )
      {
         PCPYTST pct = &pcfds->cf_sDeleTest;
         pct->ct_dwFlag &= ~(flg_User);   // remove flag as a USER skip
         if( dir_copy( pct, pcfds ) == FALSE )
         {
            // WARN: The file UPDATE failed!!!
            if( !( pct->ct_dwFlag & flg_User ) )   // flag as a USER skip
            {
               LPTSTR   lpm = &gszTmpBuf[0];
               sprintf(lpm,
                  "WARNING: It appears the DELETE of"MEOR
                  "[%s]"MEOR
                  "FAILED!",
                  &pct->ct_szCopyFull[0] );  // copy the FULL name
               MB( hWnd, lpm, "UPDATE FAILED", MB_ICONINFORMATION | MB_OK);
            }
         }
         else
         {
            bRet = TRUE;
         }
         Post_Copy_Items( pcfds, bRet, iSel );
      }
      pcfds->dwCpyOpts = dwo;
      pcfds->dwVerFlag = dwv;
   }
   else
   {
      BusyError( "Got IDM_DELETEONE" );
   }
}

VOID  Do_IDM_LINENUMS( HWND hWnd, DWORD wCmd )
{
           /* option selects whether the line nrs displayed
            * in expand mode are the line nrs in the left
            * file, the right file or none
            */
   // was like this
//      if( line_numbers != (int)wCmd )
//      {
//         HMENU hMenu = GetMenu(hWnd);
         // undo previous check
//         CheckMenuItem( hMenu, line_numbers, MF_UNCHECKED);
         //line_numbers = GET_WM_COMMAND_ID(wParam, lParam);
//         line_numbers = (int)wCmd;
//         bChgLnN = TRUE;
//         CheckMenuItem( hMenu, line_numbers, MF_CHECKED);

         /* change the display to show the line nr style
          * chosen
          */

//         view_changeviewoptions(current_view);
//      }
   // but I think this should be -
   switch( wCmd )
   {
   case IDM_NONRS:
      // this is a display toggle
      gdwDiffOpts ^= INCLUDE_LINENUMS;
      bChgDO = TRUE;
#ifndef   NEWCOLS2 // add or subtract columns with ease
      // and after the toggle, set a global BOOL
      if( gdwDiffOpts & INCLUDE_LINENUMS )
         gbShowNums = TRUE;
      else
         gbShowNums = FALSE;
      gbChgSN = TRUE;
#endif   // #ifndef   NEWCOLS2 // add or subtract columns with ease

      break;

   case IDM_LNRS:
   case IDM_RNRS:
      // these are just the source of the number - if it is ON
      gbUseRight = !gbUseRight;
      gbChgURt = TRUE;
      break;

   }
      if( line_numbers != (int)wCmd )
      {
//         HMENU hMenu = GetMenu(hWnd);
         // undo previous check
//         CheckMenuItem( hMenu, line_numbers, MF_UNCHECKED);
         //line_numbers = GET_WM_COMMAND_ID(wParam, lParam);
         line_numbers = (int)wCmd;
         bChgLnN = TRUE;
      }

   view_changeviewoptions(current_view);

}

VOID  Do_IDM_BOTHLR( HWND hWnd, DWORD wCmd )
{
   /* option selects whether the expanded file
    * show is the combined file, or just one
    * or other of the input files.
    *
    * if we are not in expand mode, this also
    * causes us to expand the selection */
//   HMENU hMenu = GetMenu(hWnd);
   // uncheck the current
//   CheckMenuItem(hMenu, expand_mode, MF_UNCHECKED);
   // expand_mode = GET_WM_COMMAND_ID(wParam, lParam);
   expand_mode = wCmd;  // set the new
//   CheckMenuItem(hMenu, expand_mode, MF_CHECKED);

   /* change the current view to show only the lines
    * of the selected type.
    */
   if( DisplayMode == MODE_OUTLINE )
   {
      ToExpand(current_view);
      
   }
   else
   {
      view_changeviewoptions(current_view);
   }
}

DWORD GetFileOpts( VOID )
{
   DWORD dwo = gdwFileOpts & ~(INC_OUTLINE2);
   dwo |= outline_include   &   INC_OUTLINE2;
   return dwo;
}

VOID  Do_IDM_EXIT( HWND hWnd )
{
   WINDOWPLACEMENT wp;
           if (ghThread!=NULL)
           {
                   extern CRITICAL_SECTION CSView;
                   /* Stop any other thread from allocating things that we
                      want to free!  See the threads DOGMA at the top
                      of this file.
                   */

                   /* Because the thread that we are about to kill might be in
                      a critical section, we first grab them both.  It is
                      essential that anyone else who ever does this, does
                      so in the same order!
                   */
                   WDEnter();
                   EnterCriticalSection(&CSView);
                   TerminateThread(ghThread, 31);
                   CloseHandle(ghThread);
                   ghThread = NULL;
                   LeaveCriticalSection(&CSView);
                   WDLeave();
                   sprtf( "IDM_EXIT: While thread active - now closed."MEOR );
           }
           else
           {
              sprtf( "IDM_EXIT: Doing a DestroyWindow()..."MEOR );
           }

           if( !view_isexpanded(current_view) )
           {
              /* save the current outline size and position */
              wp.length = sizeof(WINDOWPLACEMENT);
              if( GetWindowPlacement(hwndClient,&wp) )
              {
                 if( ChangedWP( &wp, &g_sWPO ) )
                 {
                    memcpy( &g_sWPO, &wp, sizeof(WINDOWPLACEMENT) );
                    g_sWPO.length = sizeof(WINDOWPLACEMENT);
                    bChgOut = TRUE;
                 }
              }
           }
           else
           {
              /* save the current expanded size and position */
              wp.length = sizeof(WINDOWPLACEMENT);
              if( GetWindowPlacement(hwndClient,&wp) )
              {
                 if( ChangedWP( &wp, &g_sWPE ) )
                 {
                    memcpy( &g_sWPE, &wp, sizeof(WINDOWPLACEMENT) );
                    g_sWPE.length = sizeof(WINDOWPLACEMENT);
                    bChgExp = TRUE;
                 }
              }
           }
   // what we came here to do
   DestroyWindow(hWnd);
   // ***********************
}

VOID  Do_IDM_ABORT( HWND hWnd )
{
           /* abort menu item, or status bar button.
            * the status bar button text gives the appropriate
            * action depending on our state - abort, outline
            * or expand. But the command sent is always
            * IDM_ABORT. Thus we need to check the state
            * to see what to do. If we are busy, set the abort
            * flag. If there is nothing to view,
            * exit, otherwise switch outline<->expand
            */
           if (IsBusy()) {
                   bAbort = TRUE;
                   SetStatus(LoadRcString(IDS_ABORT_PENDING));
           } else if (DisplayMode == MODE_NULL) {
                   DestroyWindow(hWnd);
           } else if (DisplayMode == MODE_EXPAND) {
                   ToOutline(current_view);
           } else {
                   ToExpand(current_view);
           }
}

//   case ID_FILELISTOPTIONS_FULLPATHNAME:
//   case ID_FILELISTOPTIONS_RELATIVENAME:
//   case ID_FILELISTOPTIONS_FILETITLEONLY:
//  if( dwo & FULL_NAMES ) { // this will be the full root path, plus drive
//      compitem_retfullname( lpb, ci, dwo );
//   } else if( dwo & ADD_REL_PATH ) { // but I like to remove the starting ".\" !!!
//      lpb = compitem_gettext_tag3(ci);
//   } else { file title only
VOID  Do_IDM_FLOPTS( HWND hWnd, DWORD wCmd )
{
   DWORD dwo = gdwFileOpts; // like &= ~(FULL_NAMES); // remove the full name
   //PTSTR ps = SaveOpts2Stg( dwo, TRUE );
   DWORD dwi = dwo;
   switch(wCmd)
   {
   case ID_FILELISTOPTIONS_FULLPATHNAME:
      dwi = FULL_NAMES | ( dwo & ~(FULL_NAMES|ADD_REL_PATH) );
      break;
   case ID_FILELISTOPTIONS_RELATIVENAME:
      dwi = ( dwo & ~(FULL_NAMES|ADD_REL_PATH) );
      break;
   case ID_FILELISTOPTIONS_FILETITLEONLY:
      dwi = ADD_REL_PATH | ( dwo & ~(FULL_NAMES|ADD_REL_PATH) );
      break;
   }
   if( dwo != dwi ) {
      gdwFileOpts = dwi;
      bChgFO = TRUE;
      view_changeviewoptions(current_view); // repaint
   }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Do_WM_COMMAND
// Return type: VOID 
// Arguments  : HWND hWnd
//            : WPARAM wParam
//            : LPARAM lParam
// Description: Command Handler - Where all MENU items come
//              
///////////////////////////////////////////////////////////////////////////////
VOID  Do_WM_COMMAND( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   LPTSTR   lpb = GetNxtBuf();  // &gszTmpBuf[0];
   DWORD    wCmd = GET_WM_COMMAND_ID(wParam, lParam);
//   WINDOWPLACEMENT wp;

   switch( wCmd )
   {
   case IDM_EXIT:
      g_bInExit = TRUE;
      Do_IDM_EXIT( hWnd );
      break;

   case IDM_ABORT:
      Do_IDM_ABORT( hWnd );
      break;

   case IDM_FILE:
      Do_IDM_FILE( hWnd );
      break;

   case IDM_DIR:
      Do_IDM_DIR( hWnd );
      break;

   case IDM_NEXTINLIST:
      Do_IDM_NEXTINLIST( hWnd );
      break;

   case IDM_CLOSE:
           /* close the output list -
            * discard all results so far
            */
           if( !IsBusy() )
           {
                   view_close(current_view);
           }
           SetAppTitle();
           break;

   case IDM_PRINT:
           /* print the current view -
            * either the outline list of filenames,
            * or the currently expanded file.
            */
           if (!IsBusy()) {
                   DoPrint();
           } else {
                   BusyError( "Got IDM_PRINT" );
           }
           break;

   case IDM_TIME:
           /* show time it took */
           {
              DWORD tim;
                   if (IsBusy()) {
                            BusyError( "Got IDM_TIME" );
                   }
                   else{
                           tim = complist_querytime();
                           sprintf(lpb, LoadRcString(IDS_SECONDS), tim/1000, tim%1000);
                           sprtf(lpb);
                   }
           }
           break;

   case IDM_SAVELIST:
           /* allow user to save list of same/different files
            * to a text file. dialog box to give filename
            * and select which types of file to include
            */
            // ADDZIPUP - test the service
           // do_zipthread(current_view, GetFileOpts());
           //complist_savelist(view_getcomplist(current_view), NULL, gdwFileOpts );
           complist_savelist(view_getcomplist(current_view), NULL, GetFileOpts() );
           break;

   case IDM_FILECOPY:   // these start off the SAME
      g_bFileCopy = TRUE;  // set the do-it-quick flag
   case IDM_COPYFILES:
      Do_IDM_COPYFILES( hWnd );
      break;

   case IDM_UPDFILE:
      Do_IDM_UPDFILE( hWnd );
      break;

   case IDM_DELETEFILE:
      Do_IDM_DELETEFILE( hWnd );
      break;

   case IDM_WRITEDIFF:
      Do_IDM_WRITEDIFF( current_view );
      break;

   case IDM_ABOUT:
      Do_IDM_ABOUT( hWnd, current_view );
      break;
   case ID_HELP_BRIEFHELP:
      show_help();
      break;

   /* launch an editor on the current item - left, right or
    * composite view
    */
   case IDM_EDITLEFT:
           do_editthread(current_view, CI_LEFT);
           break;

   case IDM_EDITRIGHT:
           do_editthread(current_view, CI_RIGHT);
           break;

   case IDM_EDITCOMP:
           do_editthread(current_view, CI_COMP);
           break;

   /* allow customisation of the editor command line */
   case IDM_SETEDIT:
      strcpy(lpb, editor_cmdline);
      if( StringInput(lpb, 256, LoadRcString(IDS_ENTER_EDT_CMD_LINE),
                           APPNAME, lpb) )
      {
         if( *lpb && strcmpi(lpb, editor_cmdline) )
         {
            strcpy(editor_cmdline, lpb);
            bChgEd = TRUE;
         }
      }
      break;


   case IDM_LNRS:
   case IDM_RNRS:
   case IDM_NONRS:
      Do_IDM_LINENUMS( hWnd, wCmd );
      break;

   /*
    * options selecting which files to include in the
    * outline listing, based on their state
    */
   case IDM_INCLEFT:
   case IDM_INCRIGHT:
   case IDM_INCSAME:
//   case IDM_INCDIFFER:
   case IDM_NEWER:
   case IDM_OLDER:
   case IDM_DIFFALL:
      {
         /* toggle flag in outline_include options */
         switch( wCmd )
         {
         case IDM_INCLEFT:
            outline_include ^= INCLUDE_LEFTONLY;
            break;
         case IDM_INCRIGHT:
            outline_include ^= INCLUDE_RIGHTONLY;
            break;
         case IDM_INCSAME:
            outline_include ^= INCLUDE_SAME;
            break;

// These are a set of 3 bit - INCLUDE_DIFFER is the MASTER switch
// **************************************************************
         case IDM_NEWER:
            outline_include ^= INCLUDE_NEWER;
            FixDiff(outline_include);
            break;
         case IDM_OLDER:
            outline_include ^= INCLUDE_OLDER;
            FixDiff(outline_include);
            break;
         case IDM_DIFFALL:
            // so we could do this
            if( gbDiffAllSw )
            {
               outline_include ^= INCLUDE_DIFFER;
               if( outline_include & INCLUDE_DIFFER )
                  outline_include |= (INCLUDE_NEWER|INCLUDE_OLDER);
               else
                  outline_include &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
            }
            else
            {
               // but I think I prefer something else, but not sure what yet ...
               // since the check is set by the MACRO
               // CMI( IDM_DIFFALL,   ShowingAll(dwo) );
               // I would like to CANCEL the 'newest' bit addition,
               // else the older bit, so
               if( ShowingAll(outline_include) )   // if we are showing BOTH new and old
               {
                  // toggle OLDER off - now NOT IDM_DIFFALL check
                  // outline_include ^= INCLUDE_OLDER;
                  // but maybe ????
                  outline_include &= ~(INCLUDE_DIFFAGE); // remove ALL differences
                  FixDiff(outline_include);
               }
               else
               {
                  // else ADD both in NOW
                  outline_include |= INCLUDE_DIFFAGE; // add in all differences
                  FixDiff(outline_include);
               }
            }
            break;
// **************************************************************

         }

         bChgInc = TRUE;

         view_changeviewoptions(current_view);

         SetAppTitle();
      }
      break;

   case IDM_UPDATE:
           /* update the display.  Options or files may have changed */
           /* discard lines  (thereby forcing re-read).
            */
           file_discardlines(compitem_getleftfile( (COMPITEM)lParam) );
           file_discardlines(compitem_getrightfile( (COMPITEM)lParam) );

           view_changediffoptions(current_view);

           /* force repaint of bar window */
           InvalidateRect(hwndBar, NULL, TRUE);
           break;

   case IDM_LONLY:
   case IDM_RONLY:
   case IDM_BOTHFILES:
      Do_IDM_BOTHLR( hWnd, wCmd );
      break;


   case IDM_IGNBLANKS:

           /* if selected, ignore all spaces and tabs on
            * comparison - expand view only: outline view
            * will still show that 'text files differ'
            */

           ignore_blanks = !ignore_blanks;
           //CheckMenuItem(hMenu, IDM_IGNBLANKS,
           //        ignore_blanks? MF_CHECKED:MF_UNCHECKED);

           bChgBks = TRUE;

           /* invalidate all diffs since we have
            * changed diff options, and re-do and display the
            * current diff if we are in expand mode.
            */
           view_changediffoptions(current_view);

           /* force repaint of bar window */
           InvalidateRect(hwndBar, NULL, TRUE);

           break;

   case IDM_PICTURE:
           /* do we show the bar picture in expand mode ? */
           picture_mode = !picture_mode;
           CheckMenuItem(hMenu, IDM_PICTURE,
                   picture_mode? MF_CHECKED:MF_UNCHECKED);
           bChgPic = TRUE;
           DoResize(hWnd);
           break;

#ifdef   ADDSTATUS

   case IDM_ADDSTATUS:
      SBToggleHide( &g_sSB );
      DoResize(hWnd);
      break;
#endif   // #ifdef   ADDSTATUS

   case IDM_EXPAND:

           /* show the expanded view of the
            * selected file
            */
           if( current_view != NULL )
           {
                   ToExpand(current_view);
                   //SetAppTitle();
           }

           break;

   case IDM_OUTLINE:
           /* return to the outline view (list of filenames) */
           ToOutline(current_view);
           //SetAppTitle();
           break;

   case IDM_FCHANGE:
           /* find the next line in the current view
            * that is not the same in both files -
            * in outline view, finds the next filename that
            * is not identical
            */
           FindNextChange( gbWarn );
           break;

   case IDM_FPCHANGE:
           /* same as IDM_FCHANGE, but going backwards from
            * current position
            */
           FindPrevChange( gbWarn );
           break;

   case IDM_OPTEXACT:   // szExa[] = "Exact";
      Do_IDM_OPTEXACT();
      break;

   case IDM_OPTADDROW:   // Add file row number - in outline mode
      Do_IDM_OPTADDROW();
      break;

   case IDM_RECURSIVE:   // szRec[] = "Recursive";
      Do_IDM_RECURSIVE();
      break;
   case IDM_OPTIGNOREDT:   // "Ignore_Date_Time";
      Do_IDM_OPTIGNOREDT();
      break;

   case IDM_OPTEXCLUDE:   // szBExl[] = "Exclude";
   case IDM_USEEXCLUDE:    // duplication of this item
      Do_IDM_OPTEXCLUDE();
      break;

   case ID_VIEW_NOEXCLUDES: // 
       Do_ID_VIEW_NOEXCLUDES();
       break;

   case IDM_EDITEXCLUDE:
      Do_IDM_EDITEXCLUDE();   // potention change in EXCLUDE list
      break;

   case IDM_VIEW_NEXT:
      Do_IDM_VIEW_NEXT();
      break;

   case IDM_WARNING:
      ToggleBool( &gbWarn, &bChgWn, !gbWarn );
      break;

   case IDM_SHOWTOOLTIP:
      Do_IDM_SHOWTOOLTIP( hWnd );
      break;

   case IDM_REVERSE:
      Do_IDM_REVERSE();
      break;

   case IDM_REFRESH:
      Do_IDM_REFRESH();
      break;

   case IDM_PREFERENCES:
      Do_IDM_PREFERENCES( hWnd );
      break;

   case IDM_SHOWSAME:
      gdwDiffOpts ^= INCLUDE_SAME;
      bChgDO = TRUE;
      view_changeviewoptions(current_view);
      break;

   case IDM_SHOWMOVE:
      if( gdwDiffOpts & INC_ALLMOVE )
         gdwDiffOpts &= ~(INC_ALLMOVE);
      else
         gdwDiffOpts |= INC_ALLMOVE;
      bChgDO = TRUE;
      view_changeviewoptions(current_view);
      break;

   case IDM_SHOWTAG:
      if( !( gdwDiffOpts & INCLUDE_LINENUMS ) )
      {
         gdwDiffOpts ^= INCLUDE_TAGS;
         bChgDO = TRUE;
         view_changeviewoptions(current_view);
      }
      break;

   case IDM_CLEARVIEWED:
      break;

   case IDM_ENABLEDELETE:
      ToggleBool( &gbDelOn, &gbChgDel, !gbDelOn );
      break;
   case IDM_DELETEONE:
      Do_IDM_DELETEONE( hWnd );
      break;

   case IDM_IGNOREEOL:
      gbIgnEOL = !gbIgnEOL;
      bChgIEOL = TRUE;
     /* invalidate all diffs since we have
      * changed diff options, and re-do and display the
      * current diff if we are in expand mode.
      */
      view_changediffoptions(current_view);

      /* force repaint of bar window */
      InvalidateRect(hwndBar, NULL, TRUE);

      break;
   case IDM_IGNCASE:

      /* if selected, ignore CASE on
       * comparison - expand view only: outline view
       * will still show that 'text files differ'
       */

      gbIgnCase = !gbIgnCase;
      bChgIgC   = TRUE;

      /* invalidate all diffs since we have
       * changed diff options, and re-do and display the
       * current diff if we are in expand mode.
       */
      view_changediffoptions(current_view);

      /* force repaint of bar window */
      InvalidateRect(hwndBar, NULL, TRUE);

      break;


   case IDM_EDITZIPUP:
      Do_IDM_EDITZIPUP();   // change the ZIP parameters
      break;
   case IDM_FILE_RENZIP:
      Do_IDM_FILE_RENZIP();
      break;
#ifdef ADD_ZIP_SUPPORT
   case ID_FILE_CREATEZIPFILE:
      Do_ID_FILE_CREATEZIPFILE(); // #ifdef ADD_ZIP_SUPPORT
      break;
#endif // #ifdef ADD_ZIP_SUPPORT
   case IDM_OPTALTDISPLAY:
      Do_IDM_OPTALTDISPLAY();
      break;

#ifdef ADD_LIST_VIEW
//        POPUP "&List View Control"
//            MENUITEM "&Off",                        IDM_LISTVIEWOFF
   case IDM_LISTVIEWOFF:
      Do_IDM_LISTVIEWOFF();
      break;
//            MENUITEM "&50%",                        IDM_LISTVIEW50
   case IDM_LISTVIEW50:
      Do_IDM_LISTVIEW50();
      break;
//            MENUITEM "&100%",                       IDM_LISTVIEW100
   case IDM_LISTVIEW100:
      Do_IDM_LISTVIEW100();
      break;

   case IDM_HOVERSEL:
      Do_IDM_HOVERSEL( hWnd );
      break;

   case IDM_LVADDGRID:
      Do_IDM_LVADDGRID( hWnd );
      break;
#endif // #ifdef ADD_LIST_VIEW

   case IDM_EXCLUDE:
      Do_IDM_EXCLUDE( hWnd );
      break;

   case ID_FILELISTOPTIONS_FULLPATHNAME:
   case ID_FILELISTOPTIONS_RELATIVENAME:
   case ID_FILELISTOPTIONS_FILETITLEONLY:
      Do_IDM_FLOPTS( hWnd, wCmd );
      break;

   case IDM_DELETELEFTFILE:
      Do_IDM_DELETELEFTFILE( hWnd );
      break;
   case IDM_EXCLUDEREPOS:
       if( Do_IDM_EXCLUDEREPOS() )
           PostMessage( hWnd, WM_COMMAND, IDM_REFRESH, 0 );
       break;
   }  // END of IDM_**** CASES
}  // end WM_COMMAND - MENU TUMBLE


long  Do_WM_COPYDATA( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   MSG msgT;
   PCOPYDATASTRUCT   pcs = (PCOPYDATASTRUCT)lParam;
   msgT.hwnd = (HWND)wParam;
   msgT.message = ((PCOPYDATASTRUCT)lParam)->dwData;
   msgT.wParam  = (WPARAM) pcs->cbData;   // length of data
   msgT.lParam  = (LPARAM) pcs->lpData;   // pointer to data block
   //msgT.wParam = ((PSPYMSGDATA)((PCOPYDATASTRUCT)lParam)->lpData)->wParam;
   //msgT.lParam = ((PSPYMSGDATA)((PCOPYDATASTRUCT)lParam)->lpData)->lParam;
//DbgPrintf("S Received Message hwnd:%8.8x msg:%d", msgT.hwnd, msgT.message);
   // PrintMsg(&msgT);
//DbgPrintf("S Printed Message hwnd:%8.8x msg:%d", msgT.hwnd, msgT.message);

   sprtf( "Got WM_COPYDATA ptr %#x with length %d."MEOR, pcs, pcs->cbData );
   if( pcs->lpData && pcs->cbData )
   {
      LPTSTR * lpv;
      INT   i, j, k;
      LPTSTR   lps = LocalAlloc( LPTR,
         ( pcs->cbData + 1 + (MXARGS * sizeof(LPVOID))) );
      sprtf( "In %#x [%s]"MEOR, pcs->lpData, pcs->lpData );
      strcpy(lps, pcs->lpData);
      j = strlen(lps);
      lpv = (LPTSTR *)((LPTSTR)lps + j);
      k = 0;
      lpv[k++] = "dummy";
      for( i = 0; i < j; i++ )
      {
         if(lps[i] > ' ' )
         {
            lpv[k++] = &lps[i];
            i++;
            for( ; i < j; i++ )
            {
               if( lps[i] <= ' ' )
               {
                  lps[i] = 0;
                  break;
               }
            }
         }
      }
      if( k > 1 )
      {
         sprtf( "Should process %d arguments."MEOR, (k - 1) );
      }

      LocalFree(lps);
   }

   return TRUE;
}

//typedef struct tagPAINTSTRUCT { 
//  HDC  hdc; 
//  BOOL fErase; 
//  RECT rcPaint; 
//  BOOL fRestore; 
//  BOOL fIncUpdate; 
//  BYTE rgbReserved[32]; 
//} PAINTSTRUCT, *PPAINTSTRUCT;
//
//DWORD gcxScreen = 0;
//DWORD gcyScreen = 0;

VOID  Do_WM_PAINT( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   PAINTSTRUCT ps;
   HDC         hDC;

   hDC = BeginPaint(hWnd, &ps);
				// Get the display limits
   if( gcxScreen == 0 ) //				if( hdc = GetDC(hwndChild) )
   {
      gcxScreen = GetDeviceCaps(hDC, HORZRES);
      gcyScreen = GetDeviceCaps(hDC, VERTRES);
      // ReleaseDC( hwndChild, hdc );
   }
   // all PAINTING done by children
   // =============================
   // except perhaps for
   if( g_bColourCycle ) // advise timer to put up colour display, and cycle thru colours
   {
      Paint_Warning( 0, 1 );  // cycle colours
   }
   else
   {
      //sprtf( "WM_PAINT: To main %x, hdc=%x, rect=%s"MEOR, hWnd,
      //   hDC,
      //   Rect2Stg( &ps.rcPaint ) );
   }

#ifdef ADD_LIST_VIEW
   if( g_hListView )
   {
      static INT  _s_lastbrush = DKGRAY_BRUSH;
      FillRect( hDC, &g_rcSizeBar, GetStockObject(_s_lastbrush) );   // BLACK_BRUSH) );
      if( _s_lastbrush == DKGRAY_BRUSH )
         _s_lastbrush = BLACK_BRUSH;
      else
         _s_lastbrush = DKGRAY_BRUSH;
   }
#endif // #ifdef ADD_LIST_VIEW

   EndPaint(hWnd, &ps);

}


//typedef struct tagNMCUSTOMDRAWINFO {
//    NMHDR     hdr;
//    DWORD     dwDrawStage;
//    HDC       hdc;
//    RECT      rc;
//    DWORD_PTR dwItemSpec;
//    UINT      uItemState;
//    LPARAM    lItemlParam;
//} NMCUSTOMDRAW, FAR * LPNMCUSTOMDRAW;
//typedef struct tagNMLVCUSTOMDRAW {
//    NMCUSTOMDRAW nmcd;
//    COLORREF clrText;
//    COLORREF clrTextBk;
//#if (_WIN32_IE >= 0x0400)
//    int iSubItem;
//#endif
//} NMLVCUSTOMDRAW, *LPNMLVCUSTOMDRAW;

/* ==============
case NM_CUSTOMDRAW:
    LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    switch(lplvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT :
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        SelectObject(lplvcd->nmcd.hdc,
                     GetFontForItem(lplvcd->nmcd.dwItemSpec,
                                    lplvcd->nmcd.lItemlParam) );
        lplvcd->clrText = GetColorForItem(lplvcd->nmcd.dwItemSpec,
                                          lplvcd->nmcd.lItemlParam);
        lplvcd->clrTextBk = GetBkColorForItem(lplvcd->nmcd.dwItemSpec,
/* At this point, you can change the background colors for the item
and any subitems and return CDRF_NEWFONT. If the list-view control
is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
to customize the item's subitems individually */
/* ===============
        return CDRF_NEWFONT;
	//  or return CDRF_NOTIFYSUBITEMREDRAW;
	
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
        SelectObject(lplvcd->nmcd.hdc,
                     GetFontForSubItem(lplvcd->nmcd.dwItemSpec,
                                       lplvcd->nmcd.lItemlParam,
                                       lplvcd->iSubItem));
        lplvcd->clrText = GetColorForSubItem(lplvcd->nmcd.dwItemSpec,
                                             lplvcd->nmcd.lItemlParam,
                                             lplvcd->iSubItem));
        lplvcd->clrTextBk = GetBkColorForSubItem(lplvcd->nmcd.dwItemSpec,
                                                 lplvcd->nmcd.lItemlParam,
                                                 lplvcd->iSubItem));

/* This notification is received only if you are in report mode and
returned CDRF_NOTIFYSUBITEMREDRAW in the previous step. At
this point, you can change the background colors for the
subitem and return CDRF_NEWFONT.*/
/* =====================
        ...
        return CDRF_NEWFONT;    
    }
...
}
    =============== */

//typedef struct tagNMLISTVIEW{
//    NMHDR   hdr;
//    int     iItem;
//    int     iSubItem;
//    UINT    uNewState;
//    UINT    uOldState;
//    UINT    uChanged;
//    POINT   ptAction;
//    LPARAM  lParam;
//} NMLISTVIEW, FAR *LPNMLISTVIEW;

//typedef struct tagSORTPARAM {
//	int			ccol;
//   BOOL        brev;
//}SORTPARAM, * PSORTPARAM;

#ifdef ADD_LIST_VIEW

POINT g_ptListView;
POINT g_ptLVClient;
//typedef struct tagMYLVITEM {
//   TCHAR li_szFile[264];
//   TCHAR li_szPath[264];
//   TCHAR li_szInfo[264];
//   TCHAR li_szLDate[32];
//   TCHAR li_szLSize[32];
//   TCHAR li_szRDate[32];
//   TCHAR li_szRSize[32];
//}MYLVITEM, * PMYLVITEM;
// a block of memory
static MYLVITEM _s_mlvi[2];
extern   TCHAR g_szSzFm[];

BOOL  addci2lv2( LPTSTR pfile, LPTSTR path, LPTSTR pinfo, PFD pfd1, PFD pfd2,
               LPARAM lp, PMYLVITEM pi )
{
//   PMYLVITEM   pi = &slvi;
//#define  nOutlineCols   (sizeof(sOutlineList) / sizeof(LISTHEADERS))
//   LPTSTR   lps[nOutlineCols];
   LONG              lg, iPos;
   LARGE_INTEGER     li1, li2;
   LPSYSTEMTIME      pst1, pst2;  // receives system time
   BOOL              b1, b2;
   LPTSTR            p;
   if( !pfd1 && !pfd2 )
      return FALSE;

   ZeroMemory( pi, sizeof(MYLVITEM) );
   pst1 = &g_sST1;
   pst2 = &g_sST2;

   strcpy( pi->li_szFile, pfile );
//   lps[0] = pi->li_szFile;    // set file name
   p = strrchr(pfile,'\\');
   if(p)
   {
      p++;
      strcpy( pi->li_szFile, p );
//      lps[0] = pi->li_szFile;
   }

   strcpy( pi->li_szPath, path );
//   lps[1] = pi->li_szPath;
   if(*path == '.')
   {
      strcpy( pi->li_szPath, &path[2] );
      iPos = InStr( pi->li_szPath, pi->li_szFile );
      if(iPos)
      {
         iPos--;
         if( iPos && ( pi->li_szPath[iPos] = '\\' ) )
            iPos--;

         pi->li_szPath[iPos] = 0;
      }
   }

   strcpy( pi->li_szInfo, pinfo );
//   lps[2] = pi->li_szInfo;
   p = strchr(pinfo,'>');
   if(p)
   {
      strcpy( pi->li_szInfo, pinfo );
      p = strchr(pi->li_szInfo,'>');
      if(p)
      {
         p++;
         *p = 0;
//         lps[2] = pi->li_szInfo;
      }
   }

   li1.QuadPart = 0;
   li2.QuadPart = 0;
   if(pfd1)
   {
      b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
      AppendDateTime( pi->li_szLDate, pst1 );
      li1.LowPart  = pfd1->nFileSizeLow;
      li1.HighPart = pfd1->nFileSizeHigh;
      sprintf( pi->li_szLSize,  g_szSzFm, li1 );
   }
   if(pfd2)
   {
      b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );
      AppendDateTime( pi->li_szRDate, pst2 );
      li2.LowPart  = pfd2->nFileSizeLow;
      li2.HighPart = pfd2->nFileSizeHigh;
      sprintf( pi->li_szRSize,  g_szSzFm, li2 );
   }

   lg = 0;
   if( pfd1 && pfd2 )
   {
      lg = CompareFileTime( &pfd1->ftLastWriteTime, &pfd2->ftLastWriteTime );
   }

//   lps[3] = pi->li_szLDate;
//   lps[4] = pi->li_szRDate;
//   lps[5] = pi->li_szLSize;
//   lps[6] = pi->li_szRSize;

   return TRUE;   // ( LVInsertItem( g_hListView, &lps[0], nOutlineCols, lp ) );
}

#endif // #ifdef ADD_LIST_VIEW

LPTSTR   compitem_getcoltext( COMPITEM ci, DWORD ccol )
{
   DIRITEM leftname, rightname;
   LPTSTR   pleft, pright;

   leftname  = file_getdiritem( compitem_getleftfile(ci)  );
   rightname = file_getdiritem( compitem_getrightfile(ci) );
   pleft     = dir_getnameptr( leftname  );
   pright    = dir_getnameptr( rightname );
   switch( ccol )
   {
   case 0:
      {
         if( pleft && pright )
         {
            // if got BOTH
            if( strcmpi(pleft, pright) )  // and DIFFERENT
            {
               LPTSTR   lps = GetStgBuf();
               sprintf( lps, "%s : %s", pleft, pright );
               return lps;
            }
            return pleft;  // then return the LEFT (of 2)
         }
         else if(pleft)
            return pleft;
         else if(pright)
            return pright;
         else
         {
            chkme( "C:ERROR: Internal function - BOTH POINTERS ZERO!!!"MEOR );
         }
      }
      break;

   case 1:
      {
         LPTSTR   lps = GetStgBuf();
         LPTSTR   pr;
         *lps = 0;
         if(leftname)
         {
            pr = dir_getrelname(leftname);
            if(pr)
            {
               if( ( pr[0] == '.' ) && ( pr[1] == '\\' ) )
                  strcpy(lps,&pr[2]);
               else
                  strcpy(lps,pr);
               dir_freerelname(leftname,pr);
            }
         }
         else if(rightname)
         {
            pr = dir_getrelname(rightname);
            if(pr)
            {
               if( ( pr[0] == '.' ) && ( pr[1] == '\\' ) )
                  strcpy(lps,&pr[2]);
               else
                  strcpy(lps,pr);
               dir_freerelname(rightname,pr);
            }
         }
         if(*lps)
         {
            pr = strrchr(lps, '\\');
            if(pr)
               *pr = 0;
            else  // = NO RELATIVE PATH = root L (or R)
               return ".";

            return lps;
         }
         else
         {
            chkme( "C:ERROR: Internal function - BOTH POINTERS ZERO!!!"MEOR );
         }
      }
      break;

      // information - Younger/Older/Left Only etc and size
   case 2:
      {
         LPTSTR   pr = compitem_gettext_result(ci);
         if(pr)
         {
            LPTSTR   p;
            LPTSTR lps = GetStgBuf();
            strcpy(lps,pr);
            p = strchr(lps,'>');
            if(p)
            {
               p++;
               *p = 0;
            }
            return lps;
         }
      }
      break;

      // then the L.date/time, R.date/time
   case 3:
      {
         PFD   pfd1 = dir_getpfd(leftname);
         LPSYSTEMTIME pst1 = &g_sST1;
         BOOL         b1;
         if(pfd1)
         {
            LPTSTR   lps = GetStgBuf();
            *lps = 0;
            b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
            AppendDateTime( lps, pst1 );
            return lps;
         }
      }
      break;
   case 4:
      // then the R.date/time
      {
         PFD   pfd2 = dir_getpfd(leftname);
         if(pfd2)
         {
            LPSYSTEMTIME pst2 = &g_sST2;
            LPTSTR       lps  = GetStgBuf();
            BOOL         b2   = FT2LST( &pfd2->ftLastWriteTime, pst2 );
            *lps = 0;
            AppendDateTime( lps, pst2 );
            return lps;
         }
      }
      break;

   case 5:
      // and R.size
      {
         PFD   pfd1 = dir_getpfd(leftname);
         if(pfd1)
         {
            LI    li1;
            LPTSTR   lps = GetStgBuf();
            li1.LowPart  = pfd1->nFileSizeLow;
            li1.HighPart = pfd1->nFileSizeHigh;
            sprintf( lps,  g_szSzFm, li1 );
            return lps;
         }
      }
      break;

   case 6:
      {
         PFD   pfd2 = dir_getpfd(rightname);
         if(pfd2)
         {
            LI    li2;
            LPTSTR   lps = GetStgBuf();
            li2.LowPart  = pfd2->nFileSizeLow;
            li2.HighPart = pfd2->nFileSizeHigh;
            sprintf( lps,  g_szSzFm, li2 );
            return lps;
         }
      }
      break;
   }

   // so we have nothing to return
   return "";  // this is a NULL, that is NOT ZERO
}

LPTSTR   compitem_getcol0text( COMPITEM ci )
{
   return( compitem_getcoltext( ci, 0 ) );
}


LPTSTR   compitem_getcol0text_DEVEL( COMPITEM ci )
{
   DIRITEM leftname, rightname;
   LPTSTR   pleft, pright;

   leftname  = file_getdiritem( compitem_getleftfile(ci)  );
   rightname = file_getdiritem( compitem_getrightfile(ci) );
   pleft     = dir_getnameptr( leftname  );
   pright    = dir_getnameptr( rightname );
   if( pleft && pright )
   {
      // if got BOTH
      if( strcmpi(pleft, pright) )  // and DIFFERENT
      {
         LPTSTR   lps = GetStgBuf();
         sprintf( lps, "%s : %s", pleft, pright );
         return lps;
      }
      return pleft;  // then return the LEFT (of 2)
   }
   else if(pleft)
      return pleft;
   else if(pright)
      return pright;
   else
   {
      chkme( "C:ERROR: Internal function - BOTH POINTERS ZERO!!!"MEOR );
   }

   // so we have nothing to return
   return "";  // this is a NULL, that is NOT ZERO
}

#ifdef ADD_LIST_VIEW
LPTSTR   compitem_getcolumntext( COMPITEM ci, INT ccol, PMYLVITEM pi )
{
   LPTSTR   pr;
   PWIN32_FIND_DATA  pfd1, pfd2;
   DIRITEM leftname;
   DIRITEM rightname;
   DIRITEM di;

   if( !ci )   //|| !ci->left || !ci->right )
      return "";

   pr = 0;
   di = leftname = rightname = 0;
   pfd1 = pfd2 = 0;
//   if( ci->left )
   {
//      leftname = file_getdiritem(ci->left);
      leftname = file_getdiritem(compitem_getleftfile(ci));
      if(leftname)
      {
         pr = dir_getrelname(leftname);
         if(pr)
            di = leftname;
      }
   }
//   if( ci->right )
   {
//      rightname = file_getdiritem(ci->right);
      rightname = file_getdiritem(compitem_getrightfile(ci));
      if(rightname)
      {
         if(!pr)
         {
            pr = dir_getrelname(rightname);
            if(pr)
               di = rightname;
         }
      }
   }

   // note zero returned is NO DIRITEM on left or right - THERE MUST BE ONE!!!
   pfd1 = dir_getpfd(leftname);
   pfd2 = dir_getpfd(rightname);

  // addci2lv2( ci->ci_pTag, pr, ci->ci_pResult, pfd1, pfd2,
  //    (LPARAM)ci, pi );
   addci2lv2( compitem_gettext_tag(ci), pr, compitem_gettext_result(ci),
      pfd1, pfd2, (LPARAM) ci, pi );

   if(pr && di)
      dir_freerelname(di, pr);

   switch( ccol )
   {
   case 0:  return( &pi->li_szFile[0] );
   case 1:  return( &pi->li_szPath[0] );
   case 2:  return( &pi->li_szInfo[0] );
   // but should the following be 'based' on text, or ...
   case 3:  return( &pi->li_szLDate[0] );
   case 4:  return( &pi->li_szRDate[0] );
   case 5:  return( &pi->li_szLSize[0] );
   case 6:  return( &pi->li_szRSize[0] );
   // but string compares 'work' also!!!
   }
   return "";
}

// The comparison function must return
// a negative value if the first item should precede the second,
// a positive value if the first item should follow the second,
// or zero if the two items are equivalent. 
int CALLBACK CICompareProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	int			iResult = 0;
   PSORTPARAM  ps = (PSORTPARAM)lParamSort;
   COMPITEM    ci1, ci2;
   LPTSTR      lps1, lps2;
   INT         ccol = ps->ccol;  // extract the column

   ci1 = (COMPITEM)lParam1;
   ci2 = (COMPITEM)lParam2;
   switch( ccol )
   {
   case 0:  // sort on FILE NAME
   case 1:  // sort on PATH
   case 2:  // sort on info
   case 3:  // sort on L.Date/Time
   case 4:  // sort on R.Date/Time
   case 5:  // sort on L.Size
   case 6:  // sort on R_Size
      {
         // get the string to compare
         lps1 = compitem_getcolumntext( ci1, ccol, &_s_mlvi[0] );
         lps2 = compitem_getcolumntext( ci2, ccol, &_s_mlvi[1] );
         iResult = strcmpi(lps1,lps2);
      }
      break;
   }

   if( ps->brev )
   {
      if(iResult < 0)
         iResult = +1;
      else if(iResult > 0)
         iResult = -1;
   }

   return iResult;
}

static LVITEM _s_lvihover;
COMPITEM Index2CI( HWND hLV, DWORD index )
{
   LPLVITEM plvi = &_s_lvihover;
   if(index != -1)
   {
      ZeroMemory( plvi, sizeof(LVITEM) );
      plvi->mask  = LVIF_PARAM;
      plvi->iItem = index;
      if( ListView_GetItem( hLV, plvi ) )
      {
         return((COMPITEM)plvi->lParam);  // extract COMPITEM
      }
   }
   return NULL;
}

BOOL  Do_NM_HOVER( HWND hLV )
{
//   static LVITEM _s_lvihover;
   static TCHAR _s_sznmhover[264];
   LPLVITEM plvi = &_s_lvihover;
   DWORD index = ListView_GetHotItem(hLV);
   COMPITEM ci = Index2CI(hLV,index);  // (COMPITEM)plvi->lParam;  // extract COMPITEM
   if(ci)
   {
      LPTSTR fname = compitem_getcol0text(ci);
      LPTSTR pbuf  = _s_sznmhover;  // gszTmpBuf;
      //sprintf(pbuf, "Hot Select = [%s]", fname );
      sprintf(pbuf, "%s = [%s]", g_szHotSel, fname );
      SBSetTimedTxt( pbuf, g_dwActSecs, FALSE ); // standard delay of timed text message
      return TRUE;
   }
   return FALSE;
}

#endif // #ifdef ADD_LIST_VIEW

#define  ViewIsOutline  ( DisplayMode == MODE_OUTLINE )

COMPITEM    g_pUpdatedCI = 0;
LONG        g_iLVUpdated;
LONG        g_iLVSelection = -1;

extern   LONG  view_findCI( PVIEW view, COMPITEM ci );

#ifdef ADD_LIST_VIEW

LRESULT  Do_LV_WM_NOTIFY( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   LPNMCUSTOMDRAW pcd = (LPNMCUSTOMDRAW)lParam;
   LPNMLVCUSTOMDRAW plvcd = (LPNMLVCUSTOMDRAW)lParam;
   LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
   DWORD code = pcd->hdr.code;
   HWND  hLV  = pcd->hdr.hwndFrom;
   COMPITEM ci; // extract COMPITEM

   switch(code)
   {
      // trace sequence is
//MWP: WM_PARENTNOTIFY[LBDWN] wP=201 lP=10b0137.
//MWP: WM_MOUSEACTIVATE wP=5e4 lP=2010001.
//MWP: WM_NOTIFY[h=0x1808 id=1000 c=-108(LVN_COLUMNCLICK)] wP=3e8 lP=76e4b0.
//MWP: WM_NOTIFY[h=0x180c id=0 c=-302] wP=0 lP=76fb30.

   case LVN_COLUMNCLICK:
		{
			INT ccol   = pnmlv->iSubItem;
         INT iItem  = pnmlv->iItem;
         DWORD flag = getcolumnflag(ccol);
         SORTPARAM   sp;

			// The user clicked a column header;
			// sort by this criterion.
         sp.ccol = ccol;
         sp.brev = (flag & dwflg_Rev);   // set up/down flag, on column by col basis

         //sOutlineList[0].lh_dwFlag |= dwflg_HadClk;   // store HAD click
         if( !(flag & dwflg_HadClk) )
            orcolumnflag(0, dwflg_HadClk);   // note: common flag to col 0 only
         // but subsequently returned in ALL getcolumnflag() calls

			ListView_SortItems( hLV,   // pnmlv->hdr.hwndFrom,	// ListView HANDLE
					CICompareProc,	// Compare PROC
					(LPARAM) &sp );

         setcolumnheader( ccol, flag );

         xorcolumnflag(ccol, dwflg_Rev);  // toggle the REVERSE for this COLUMN

		}

      break;

   case NM_CUSTOMDRAW:
      {
         DWORD ds = pcd->dwDrawStage;  // extract the stage

         switch(ds)
         {
         case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

         case CDDS_ITEMPREPAINT:
            {
               UINT  state;
               DWORD dwFlag;
               UINT  ui;

               ci = (COMPITEM)plvcd->nmcd.lItemlParam;  // extract COMPITEM
               state  = compitem_getstate(ci);
               dwFlag = compitem_getflag(ci);
               ui = 0;
            
               if(ci)
               {
                  if(g_bNoInvert)
                  {
                     ui = StateToColour( state, dwFlag, 0,
                        &plvcd->clrText,  // PDWORD foreground,
                        &plvcd->clrTextBk ); // PDWORD background )
                  }
                  else
                  {
                     ui = StateToColour( state, dwFlag, 0,
                        &plvcd->clrTextBk,  // PDWORD foreground,
                        &plvcd->clrText ); // PDWORD background )
                  }
               }

               if(ui)
                  return CDRF_NEWFONT;

            }
            break;
         }
      }
      break;

   case NM_HOVER:
      {
         Do_NM_HOVER( hLV );
      }
      break;

   case LVN_HOTTRACK:
      {
//         LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;   // following FAILS
//         COMPITEM ci = (COMPITEM)pnmlv->lParam;  // extract COMPITEM
//         if(ci) { UINT state = compitem_getstate(ci);   }
         POINT pt = pnmlv->ptAction;   // get the HOTTRACK point
         g_ptListView = pt;   // keep the POINT in the LISTVIEW client
         if( ClientToScreen( pnmlv->hdr.hwndFrom, &pt ) )
         {
            if( ScreenToClient(hwndClient, &pt) )
            {
               g_ptLVClient = pt;
               Do_WM_MOUSEMOVE(hwndClient, 0, pt.x, pt.y, SRC_HOTTRACK );
            }
         }
      }
      break;

   case LVN_ITEMCHANGING:
      break;

   case LVN_ITEMCHANGED:
      {
//typedef struct tagNMLISTVIEW{
//    NMHDR   hdr;
//    int     iItem;
//    int     iSubItem;
//    UINT    uNewState;
//    UINT    uOldState;
//    UINT    uChanged;
//    POINT   ptAction;
//    LPARAM  lParam;
//} NMLISTVIEW, FAR *LPNMLISTVIEW;
         static TCHAR _s_szlvnvhanged[264];
         LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
         LPTSTR   lpb = _s_szlvnvhanged;  // gszTmpBuf;

         ci = (COMPITEM)pnmv->lParam;  // extract COMPITEM

         if( g_pUpdatedCI != ci )
         {
            g_iLVUpdated = view_findCI( current_view, ci );
            sprintf(lpb, "LVN_ITEMCHANGED to [%s] in [%s] - Row %d",
               compitem_getcoltext(ci,0),
               compitem_getcoltext(ci,1),
               (g_iLVUpdated + 1) );

            SBSetTimedTxt( lpb, g_dwActSecs, FALSE );

            g_pUpdatedCI = ci;   // keep this UPDATE = inverted selection

         }
      }
      break;

   case LVN_INSERTITEM:
//      setcolumnheader(0,flag);
      break;

   case NM_SETFOCUS:
      //  hmmm ... what to do here???
      // ideally, if the mouse is over the LISTVIEW, which is visible and enabled
      // then the focus should stay here for now, ***BUT***, once the
      // mouse is over 'other' terrritory, then maybe give FOCUS to where
      // the mouse IS ...
      g_bLVHasFocus = TRUE;   // show (globally) that the keyboard is here!
      break;

   case NM_CLICK:
   case NM_DBLCLK:
      {
//   void CMyListCtrl::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
//   {
      // Get the current mouse location and convert it to client
      // coordinates.
         DWORD    pos;  // = GetMessagePos();
//      CPoint pt(LOWORD(pos), HIWORD(pos));
         POINTS   pts;  // = MAKEPOINTS(pos);
         POINT    pt;
         DWORD    index;   // = ListView_GetTopIndex(hLV);
         DWORD    endind;  // = index + ListView_GetCountPerPage(hLV);
         DWORD    icount;
         RECT     rc;

         if( code == NM_DBLCLK )
         {
            icount = 0;
            if( !ViewIsOutline )
            {
               g_bNoResize = TRUE; // no re-sizing done in ToOutline()
               ToOutline( current_view );
               SetSelection(g_iLVSelection);
               g_bNoResize = FALSE; // no re-sizing done in ToOutline()
               icount++;
            }

            ToExpand( current_view );
            if(icount)
               InvalidateRect(hwndBar, NULL, TRUE);   // make sure RE-PAINT of BAR

            break;
         }

         pos  = GetMessagePos();
         pts  = MAKEPOINTS(pos);
         pt.x = pts.x;
         pt.y = pts.y;
         ScreenToClient(hLV, &pt);
      // Get indexes of the first and last visible items in listview
      // control.
         index  = ListView_GetTopIndex(hLV);
         endind = index + ListView_GetCountPerPage(hLV);
//      int index = GetTopIndex();
//      int last_visible_index = index + GetCountPerPage();

//      if (last_visible_index > GetItemCount())
//          last_visible_index = GetItemCount();
         icount = ListView_GetItemCount(hLV);
         if(endind > icount)
            endind = icount;

      // Loop until number visible items has been reached.
//      while (index <= last_visible_index)
         while(index <= endind)
         {
            // Get the bounding rectangle of an item. If the mouse
            // location is within the bounding rectangle of the item,
            // you know you have found the item that was being clicked.
//          CRect r;
//          GetItemRect(index, &r, LVIR_BOUNDS);
            ListView_GetItemRect(hLV, index, &rc, LVIR_BOUNDS);
//          if (r.PtInRect(pt))
            if( PtInRect( &rc, pt ) )
            {
//               UINT flag = LVIS_SELECTED | LVIS_FOCUSED;
               //              SetItemState(index, flag, flag);
//               ListView_SetItemState(hLV, index, flag, flag);
               ci = Index2CI( hLV, index );
               if(ci)
               {
                  LPTSTR   lpb = GetStgBuf();
                  LONG     row = view_findCI( current_view, ci );
//LONG        g_iLVSelection = -1;
                  sprintf(lpb, "NM_CLICK on [%s] in [%s] inf.[%s] - row %d", compitem_getcoltext(ci,0),
                     compitem_getcoltext(ci,1),
                     compitem_getcoltext(ci,2),
                     (row + 1) );

                  // NOTE: if the above code is really working OK, then we already
                  // KNOW the 'selection', WITHOUT doing this dribble search for a
                  // point in a maze !!!!!!!!!!!

                  if( g_pUpdatedCI == ci )
                     sprintf(EndBuf(lpb), " %s %s", compitem_getcoltext(ci,3), compitem_getcoltext(ci,5) );
                  else
                     strcat(lpb, " ***NEW***");

                  SBSetTimedTxt( lpb, g_dwActSecs, FALSE );

                  g_iLVSelection = row;

                  if( ViewIsOutline )
                  {
                     SetSelection(g_iLVSelection);
                  }

               }
               break;
            }
            // Get the next item in listview control.
            index++;
         }
//      *pResult = 0;
//   }
      }
      break;

   case LVN_GETDISPINFO:
//typedef struct tagLVDISPINFO {
//    NMHDR hdr;
//    LVITEM item;
//} NMLVDISPINFO, FAR *LPNMLVDISPINFO;
//typedef struct _LVITEM { 
//    UINT mask; 
//    int iItem; 
//    int iSubItem; 
//    UINT state; 
//    UINT stateMask; 
//    LPTSTR pszText; 
//    int cchTextMax; 
//    int iImage; 
//    LPARAM lParam;
//#if (_WIN32_IE >= 0x0300)
//    int iIndent;
//#endif
//#if (_WIN32_IE >= 0x560)
//    int iGroupId;
//    UINT cColumns; // tile view columns
//    PUINT puColumns;
//#endif
//} LVITEM, FAR *LPLVITEM; 
      {
         NMLVDISPINFO * pdi = (NMLVDISPINFO *)lParam;
         LPLVITEM plvi = &pdi->item;
         DWORD mask = plvi->mask;
         
         if(mask & LVIF_TEXT)
         {
         //   sprtf( "Always asking for TEXT for sub-item %d, %d In %#x for %d?"MEOR,
         //      plvi->iItem,
         //      plvi->iSubItem,
         //      plvi->pszText,
         //      plvi->cchTextMax );
            plvi->mask |= LVIF_DI_SETITEM;   // this only stops a CASCADE of them!!!
         }
         if(mask & LVIF_STATE)
            sprtf( "Whay asking for STATE?"MEOR );
         if(mask & LVIF_IMAGE)
            sprtf( "Whay asking for IMAGE?"MEOR );

         //plvi->mask |= LVIF_DI_SETITEM;

      }
      break;


   case LVN_KEYDOWN:
      {
         LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) lParam;

      }

      break;


   }  // end of LV NOTIFY cases


   return 0;
}

#endif // #ifdef ADD_LIST_VIEW

LRESULT  Do_WM_NOTIFY( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
   INT   idCtrl = (INT)wParam; 
   LPNMHDR pnmh = (LPNMHDR)lParam;
   DWORD code   = pnmh->code;

#ifdef ADD_LIST_VIEW
   if( g_hListView && ( pnmh->hwndFrom == g_hListView ) )
   {
      return( Do_LV_WM_NOTIFY( hWnd, wParam, lParam ) );
   }
#endif // #ifdef ADD_LIST_VIEW

   return 0;
}

VOID  Do_WM_SETFOCUS( HWND hWnd )
{
      /* set the focus on the table class so it can process
       * page-up /pagedown keys etc.
       */
      SetFocus(hwndRCD);
#ifdef ADD_LIST_VIEW
      g_bLVHasFocus = FALSE;   // show (globally) that the keyboard is there!
#endif // #ifdef ADD_LIST_VIEW

}

#if   (defined(MWPDBG) && !defined(NDEBUG))      // output messages from MainWndProc
extern   LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern   BOOL  NotInX( UINT a );
extern   BOOL  excl_WM_NOTIFY( UINT uMsg, WPARAM wParam, LPARAM lParam );
static HWND last_hWnd;
static UINT last_message;
static WPARAM last_wParam;
static LPARAM last_lParam;
#endif   // if MWPDBG and !NDEBUG

/***************************************************************************
 * Function: MainWndProc
 *
 * Purpose:
 *
 * Window processing for main window
 */
LRESULT APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   LRESULT lRet = 0;
#if   (defined(MWPDBG) && !defined(NDEBUG))      // output messages from MainWndProc
   if ((last_hWnd != hWnd) || (last_message != message) || (last_wParam != wParam) || (last_lParam != lParam))
   {
       last_hWnd = hWnd;
       last_message = message;
       last_wParam = wParam;
       last_lParam = lParam;

       if ((message != table_msgcode) &&
       (NotInX(message)))
       {
           if (message == WM_NOTIFY)
           {
               if (!excl_WM_NOTIFY(message, wParam, lParam))
                   sprtf("MWP: %s wP=0x%x lP=0x%x."MEOR, GetWMStg(0, message, wParam, lParam), wParam, lParam);
           }
           else
           {
               sprtf("MWP: %s wP=0x%x lP=0x%x."MEOR, GetWMStg(0, message, wParam, lParam), wParam, lParam);
           }
       }
   }
#endif   // if MWPDBG and !NDEBUG
   switch( message )
   {
      // New message for Win32
      // allows the application to pass data to another application.
   case WM_COPYDATA:
      lRet = Do_WM_COPYDATA( hWnd, wParam, lParam );
      break;

   case WM_CREATE:
      Do_WM_CREATE( hWnd );
      break;

   case WM_PAINT:
      Do_WM_PAINT( hWnd, wParam, lParam );
      break;

   case WM_COMMAND:
      Do_WM_COMMAND( hWnd, wParam, lParam );
      break;

   case WM_NOTIFY:
      lRet = Do_WM_NOTIFY( hWnd, wParam, lParam );
      break;

	case WM_INITMENUPOPUP:  // menu popup in process
		if( Do_WM_INITMENUPOPUP( hWnd, wParam, lParam ) )
         goto Use_Def;
      break;   // return ZERO = processed

   case WM_SIZE:
      //Do_WM_SIZE( hWnd, LOWORD(lParam),   // specifies the new width of the client area. 
      //   HIWORD(lParam),   // The high-order word of lParam specifies the new height of the client area.
      //   (UINT)wParam );
      // DoResize(hWnd);
      Do_WM_SIZE( hWnd, wParam, lParam );
      break;

   case WM_SETFOCUS:
      Do_WM_SETFOCUS( hWnd );
      break;

   case WM_MOUSEMOVE:
      Do_WM_MOUSEMOVE( hWnd, wParam, LOWORD(lParam), HIWORD(lParam), SRC_FRAME );
      break;

   case WM_LBUTTONDOWN:
      Do_WM_LBUTTONDOWN( hWnd, wParam, lParam );
      break;
   case WM_LBUTTONUP:
      Do_WM_LBUTTONUP( hWnd, wParam, lParam );
      break;

      // NOTE: This is handle in the TABLE procedure!!!
   //case WM_RBUTTONDOWN: // The user clicked the RIGHT mouse button
   //   Do_WM_RBUTTONDOWN( hWnd, wParam, lParam );
   //   goto Use_Def;
   case WM_CONTEXTMENU:
      //goto Use_Def;
      tb_WM_RBUTTONDOWN( hWnd, wParam, lParam );
      break;

#ifdef   ADDTIMER
   case WM_TIMER:
      lRet = Do_WM_TIMER( hWnd );
      break;
#endif   // ADDTIMER

   case WM_KEYDOWN:
      /* although the table window has the focus, he passes
       * back to us any keys he doesn't understand
       * We handle escape here to mean 'return to outline view'
       */
       if (wParam == VK_ESCAPE) {
          ToOutline(current_view);
       } else if ((wParam == 'Q')||(wParam == 'q')) {
           // // FIX20130808 add quit request
           SendMessage( hWnd, WM_COMMAND, (WPARAM) IDM_EXIT, 0 );
       }
       break;

   case WM_CLOSE:
      /* don't allow close when busy - process this message in
       * order to ensure this
       */
      if(IsBusy()) {
         lRet = TRUE;
      } else {
         lRet = DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;

   case WM_DESTROY:
      g_bInExit = TRUE; // ensure 'in exit' set
      DeleteTools();
      PostQuitMessage(0);  // pop out of the windows LOOP
      break;

   case TM_CURRENTVIEW:
      /* allow other people such as the bar window to query the
       * current view
       */
      return((LRESULT) current_view);

   default:
Use_Def:
      /* handle registered table messages - actually NOW a WM_USER type */
      if( message == table_msgcode )
      {
         lRet = TableServer(hWnd, wParam, lParam);
      }
      else
      {
         lRet = DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
   }

   if( message == WM_NCHITTEST )
   {
      if( Do_WM_NCHITTEST( hWnd, wParam, LOWORD(lParam), HIWORD(lParam), lRet ) )
      {

      }

   }

   return lRet;
}


#ifdef ADD_DBCS_FUNCS2 // FIX20050129 - using MSVC7 .NET 2003

#ifndef  _GUTILS_H2
/***************************************************************************
 * Function: My_mbschr
 *
 * Purpose:
 *
 * DBCS version of strchr
 *
 */
unsigned char * _CRTAPI1 My_mbschr(
    unsigned char *psz, unsigned short uiSep)
{
    while (*psz != '\0' && *psz != uiSep) {
        psz = CharNext(psz);
    }
    return *psz == uiSep ? psz : NULL;
}
/***************************************************************************
 * Function: My_mbsncpy
 *
 * Purpose:
 *
 * DBCS version of strncpy
 *
 */
unsigned char * _CRTAPI1 My_mbsncpy(
	unsigned char *psz1, const unsigned char *psz2, size_t Length)
{
        int nLen = (int)Length;
	unsigned char *pszSv = psz1;

	while (0 < nLen) {
		if (*psz2 == '\0') {
			*psz1++ = '\0';
			nLen--;
		} else if (IsDBCSLeadByte(*psz2)) {
			if (nLen == 1) {
				*psz1 = '\0';
			} else {
				*psz1++ = *psz2++;
				*psz1++ = *psz2++;
			}
			nLen -= 2;
		} else {
			*psz1++ = *psz2++;
			nLen--;
		}
	}
	return pszSv;
}

#endif   // #ifndef  _GUTILS_H2

/***************************************************************************
 * Function: My_mbsrchr
 *
 * Purpose:
 *
 * DBCS version of strrchr
 *
 */
unsigned char * _CRTAPI1 My_mbsrchr(
    unsigned char *psz, unsigned short uiSep)
{
    unsigned char *pszHead;

    pszHead = psz;

    while (*psz != '\0') {
        psz++;
    }
    if (uiSep == '\0') {
        return psz;
    }

    while (psz > pszHead) {
        psz = CharPrev(pszHead, psz);
        if (*psz == uiSep) {
            break;
        }
    }
    return *psz == uiSep ? psz : NULL;
}

/***************************************************************************
 * Function: My_mbsncmp
 *
 * Purpose:
 *
 * DBCS version of strncmp
 * If 'nLen' splits a DBC, this function compares the DBC's 2nd byte also.
 *
 */
int _CRTAPI1 My_mbsncmp(
    const unsigned char *psz1, const unsigned char *psz2, size_t nLen)
{
    int Length = (int)nLen;

    while (0 < Length) {
        if ('\0' == *psz1 || '\0' == *psz2) {
            return *psz1 - *psz2;
        }
        if (IsDBCSLeadByte(*psz1) || IsDBCSLeadByte(*psz2)) {
            if (*psz1 != *psz2 || *(psz1+1) != *(psz2+1)) {
                return *psz1 - *psz2;
            }
            psz1 += 2;
            psz2 += 2;
            Length -= 2;
        } else {
            if (*psz1 != *psz2) {
                return *psz1 - *psz2;
            }
            psz1++;
            psz2++;
            Length--;
        }
    }
    return 0;
}

#endif // #ifdef ADD_DBCS_FUNCS2 // FIX20050129 - using MSVC7 .NET 2003

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// NEW

// IDM_IGNBLANKS  ignore_blanks  B
// IDM_OPTEXACT   gbExact        F
// IDM_RECURSIVE  gbRecur        C
// IDM_OPTEXCLUDE gbExclude      X
VOID  AddMoreOpts( LPTSTR lpb )
{
   if( ignore_blanks )
      strcat(lpb, "B");

   if( gbExact )
      strcat(lpb, "F");

   if( gbIgnCase )
      strcat(lpb, "C");

#ifndef USE_GLOBAL_RECURSIVE    // FIX20091125
   if( gbRecur )
      strcat(lpb, "R");
#else // !#ifndef USE_GLOBAL_RECURSIVE
   if ( !g_bNOTRecursive )
       strcat(lpb,"R"); // FIX20091125
#endif // #ifndef USE_GLOBAL_RECURSIVE y/n

   if( gbExclude )
      strcat(lpb, "X");

   if( gbIgnDT )
      strcat(lpb, "I");

}

// SetWindowTitle - SetTitle
VOID  SetAppTitle( VOID )
{
   static TCHAR _s_szstapptit1[264];
   static TCHAR _s_szstapptit2[264];
   LPTSTR lpb   = _s_szstapptit1;
   strcpy(lpb, APPNAME);   // start with application name
   if( !gfBusy )
   {
      BOOL  bAdded = FALSE;
      LPTSTR lpb2  = _s_szstapptit2;
      LPTSTR lpa   = 0;


      //strcat(lpb, " - Mode=");
      //  if(DisplayMode == MODE_NULL)
      //     strcat(lpb, "NULL");
      //  else if(DisplayMode == MODE_EXPAND)
      //     strcat(lpb, "Expand");
      //  else
      //     strcat(lpb, "Outline");
      if( DisplayMode == MODE_EXPAND )
         lpa = "Expanded";
      else if( DisplayMode == MODE_OUTLINE )
         lpa = "Outline";

      if(lpa)
      {
         if(!bAdded)
         {
            strcat(lpb, " - ");
            bAdded = TRUE;
         }
         strcat(lpb,lpa);
         lpa = 0;
      }

      *lpb2 = 0;
      OptionsToString(lpb2, TRUE);
      if( *lpb2 )
      {
         if(!bAdded)
         {
            strcat(lpb, " - ");  // hyphen after app name
            bAdded = TRUE;
         }
         else
            strcat(lpb, " ");    // space after mode

         strcat(lpb,"[");
         strcat(lpb,lpb2);
         strcat(lpb,"]");
      }

      *lpb2 = 0;
      AddMoreOpts( lpb2 );
      if( *lpb2 )
      {
         if(!bAdded)
         {
            strcat(lpb, " - ");  // hyphen after app name
            bAdded = TRUE;
         }
         else
            strcat(lpb, " ");    // space after mode

         strcat(lpb, lpb2);      // like
         // "B" for "Ignore Blanks"
         // "F" for "Full compare"
         // "C" for "Recursive"
         // "X" for "Exclude"
      }

      if( dir_isvalidfile( g_szRenZip ) )
      {
         if( g_szZipTit[0] )
         {
            strcat(lpb, " - ");
            strcat(lpb, g_szZipTit);
         }
      }

      //SetWindowText(hwndClient, APPNAME);

   } else {
      strcat(lpb, " - ");
      strcat(lpb, "Busy ...");
   }

   SetWindowTitle(lpb);
}


VOID  Do_IDM_VIEW_NEXT( VOID )
{
   if( view_isexpanded(current_view) )
   {
      long  lg;

      g_bNoUpdate = TRUE;

      ToOutline(current_view);

      lg = view_getrowcount(current_view);
      if( lg > 1 )
      {
         if( giSelection >= 0 )
         {
            // move to NEXT selection
            giSelection++;
            if( giSelection >= lg )
               giSelection = 0;         // restart at TOP

            g_bNoUpdate = FALSE;

            ToExpand(current_view);

            //FindNextChange( FALSE );   // move the cursor to the FIRST change
            lg = view_findchange(current_view, 0, FIND_DOWN);
            if( lg >= 0 )
               SetSelection(lg);

            // all done
         }
      }

      g_bNoUpdate = FALSE;

   }
}

INT   ProcessI2( LPTSTR lpf );
INT   ProcessI3( LPTSTR lps, LPTSTR lpf );

INT GetArgData( LPTSTR lps, int argc, char * * argv )
{
   INT      i, j, icnt;
   LPTSTR   pc;
   INT      c;

   icnt = 0;
   for( i = 1; i < argc; i++ )
   {
      pc = argv[i];  // extract the pointer
      if(pc)
      {
         c = *pc;
         if( c == '@' )
         {
            pc++;
            icnt += ProcessI3(lps, pc);
         }
         else
         {
            j = strlen(pc);
            if(j)
            {
               icnt += (j + 1);
               if(*lps)
                  strcat(lps, " ");
               strcat(lps, pc);
            }
         }
      }
   }
   return icnt;
}

INT GetArgLen( int argc, char * * argv )
{
   INT      i, j, icnt;
   LPTSTR   pc;
   INT      c;

   icnt = 0;
   for( i = 1; i < argc; i++ )
   {
      pc = argv[i];  // extract the pointer
      if(pc)
      {
         c = *pc;
         if( c == '@' )
         {
            pc++;
            icnt += ProcessI2(pc);
         }
         else
         {
            j = strlen(pc);
            if(j)
               icnt += (j + 1);
         }
      }
   }
   return icnt;
}

INT   ProcessI2( LPTSTR lpf )
{
   INT      iRet = 0;  // start zero
   PISTR    pis;
   PPISTR   ppis = &pis;

   ZeroMemory( ppis, sizeof(PISTR) );
   if( Getpis( ppis, lpf ) )
   {
      if( ( ppis->iCnt > 1 ) && ( ppis->pArgs ) )
         iRet = GetArgLen( ppis->iCnt, ppis->pArgs );
      // note we quietly forget BLANK input files!!!
   }

   // clean up what needs to be cleaned
   if( ppis->pBuf )
      LocalFree(ppis->pBuf);

   if( VFH( ppis->hFile ) )
      CloseHandle( ppis->hFile );

   return iRet;
}

INT   ProcessI3( LPTSTR lps, LPTSTR lpf )
{
   INT      iRet = 0;  // start zero
   PISTR    pis;
   PPISTR   ppis = &pis;

   ZeroMemory( ppis, sizeof(PISTR) );
   if( Getpis( ppis, lpf ) )
   {
      if( ( ppis->iCnt > 1 ) && ( ppis->pArgs ) )
         iRet = GetArgData( lps, ppis->iCnt, ppis->pArgs );
      // note we quietly forget BLANK input files!!!
   }

   // clean up what needs to be cleaned
   if( ppis->pBuf )
      LocalFree(ppis->pBuf);

   if( VFH( ppis->hFile ) )
      CloseHandle( ppis->hFile );

   return iRet;
}

//PCOPYDATASTRUCT   CopyArgs(__argc, __argv);
PCOPYDATASTRUCT  CopyArgs( HWND hWnd, int argc, char * * argv )
{
   PCOPYDATASTRUCT   pcs = 0;
   if( argc > 1 )
   {
      INT   icnt = GetArgLen( argc, argv );
      if( icnt )
      {
         pcs = (PCOPYDATASTRUCT) LocalAlloc( LPTR, (sizeof(COPYDATASTRUCT) +
            sizeof(HWND) + icnt) );
         if(pcs)
         {
            HWND * ph = (HWND *)((PCOPYDATASTRUCT)pcs + 1);
            LPTSTR lps = (LPTSTR)((HWND *)ph + 1);
            *lps = 0;
            if( ( GetArgData( lps, argc, argv ) ) &&
                ( *lps ) )
            {
               pcs->dwData = WM_COPYDATA;
               pcs->cbData = strlen(lps);
               pcs->lpData = lps;
               *ph = hWnd;
            }
            else
            {
               LocalFree(pcs);
               pcs = 0;
            }
         }
      }
   }
   return pcs;
}

static PCOPYDATASTRUCT  dlg_pcd = 0;

INT_PTR CALLBACK MSGDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  bRet = 0;
   switch(uMsg)
   {
   case WM_INITDIALOG:
      dlg_pcd = (PCOPYDATASTRUCT)lParam;

      return TRUE;

   case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
         case IDOK:
            if(dlg_pcd)
            {
               PCOPYDATASTRUCT   pcs = dlg_pcd;
               HWND * ph = (HWND *)((PCOPYDATASTRUCT)pcs + 1);
               sprtf( "Sending WM_COPYDATA to %#x in %#x, length %d"MEOR,
                  *ph, pcs, pcs->cbData );
               bRet = (BOOL)SendMessage( *ph, WM_COPYDATA, (WPARAM)hDlg,
                  (LPARAM)pcs );
            }
            // fall into cancel
         case IDCANCEL:
            EndDialog(hDlg, bRet);
            break;
         }
      }
   }

   return bRet;
}

BOOL  SendArgCopy( HINSTANCE hInstance, HWND hWnd )
{
   BOOL  bRet = FALSE;
   PCOPYDATASTRUCT   pcs = CopyArgs( hWnd, __argc, __argv );
   LPTSTR   ptmp = GetNxtBuf(); // &gszTmpBuf[0];

   if(pcs)
   {
      sprintf(ptmp, "Application is already running!"MEOR
         "A copy of the command arguments,"MEOR
         "Length %d will be passed to %p",
         pcs->cbData,
         hWnd );
      MB( NULL, ptmp,
         APPNAME,
         MB_ICONINFORMATION | MB_OK );
      if( IsWindow(hWnd ) )
      {
         sprtf( "Sending WM_COPYDATA to %#x in %#x, length %d"MEOR,
            hWnd, pcs, pcs->cbData );
         //DialogBoxParam( hInstance,
         //   MAKEINTRESOURCE(IDD_STRINGINPUT),  // "SaveList",
         //   NULL,
         //   MSGDLGPROC,
         //   (LPARAM)pcs );
               bRet = (BOOL)SendMessage( hWnd, WM_COPYDATA, (WPARAM)0,
                  (LPARAM)pcs );
      }
      LocalFree( pcs );
   }
   else
   {
      MB( NULL, "Application is already running!",
         APPNAME,
         MB_ICONINFORMATION | MB_OK );
   }
   return bRet;
}


// NEW functions

VOID  SetSelandExpand(LONG row)
{
   TableSelection select;

   select.startrow  = row;
   select.startcell = 0;
   select.nrows     = 1;
   select.ncells    = 1;
   SendMessage(hwndRCD, TM_ENTER, 0, (LPARAM) &select );
}

BOOL  Select_Nearest( INT iSel )
{
   LONG  lg;

   if( !current_view )
      return FALSE;

   if( view_isexpanded( current_view ) )
      return FALSE;

   lg = view_getrowcount( current_view );
   if( lg <= 0 )
      return FALSE;

   if( iSel >= lg )
      iSel = (INT)(lg - 1);

   SetSelandExpand( iSel );

   return TRUE;
}

LONG  GetCurrRow( VOID )
{
   LONG  crow;
   if( giSelection >= 0 )
   {
      crow = giSelection;
   }
   else
   {
      crow = (LONG) SendMessage(hwndRCD, TM_TOPROW, FALSE, 0);
   }
   return crow;
}

BOOL  Get_Sel_File_Stg( LPTSTR lpb, COMPITEM ci )
{
   if(ci) {
      INT   state = compitem_getstate(ci);
//            if( giSelection != -1 ) {
               // have a SELECTION
//               if(ci) {
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

                     //sprintf(lpb, "Exclude [%s]", cp);
                     //if( AM( IDM_EXCLUDE, lpb ) )
                     //   iCnt++;
                     strcpy(lpb,cp);

                     return TRUE;

                     //dir_freerelname( diritem, relname );
                  }
//               }  // got the COMPITEM of the SELECTION

//            }
   }
   return FALSE;
}

VOID  Do_IDM_EXCLUDE( HWND hWnd )  // currently SELECTED file ADDED to gsXFileList
{
   LPTSTR      lpb   = GetNxtBuf(); // &gszTmpBuf[0];
   COMPITEM    ci = view_getitem( current_view, giSelection );
   if( ci ) {  // FIX20070525 - added 'Exclude' to expanded context menu
      if( view_isexpanded(current_view) ) {
         // FIX20070525 - in EXPANDED VIEW of a SINGLE FILE, and IT IS TO BE EXCLUDED
         ToOutline(current_view);   // back to OUTLINE
      }
      if(ci) {
         // outline - file list view
         long lg = view_getrowcount(current_view);
         if( lg > 1 ) {
            if(( giSelection >= 0  ) &&
               ( giSelection  < lg ) )
            {
               // we have a CURRENT SELECTION
               if( Get_Sel_File_Stg( lpb, ci ) ) {
                  // add to EXCLUDE file LIST
                  PLE   pHead = &gsXFileList;

                  //Add2StgList( pHead, lpb );
                  Add2Exclude( pHead, lpb ); // if NOT already there

                  PostMessage( hwndClient, WM_COMMAND, IDM_REFRESH, 0 );

               }
            }
         }
      }
   }
}

// eof - dc4w.c
