

// dc4wOpts.c
// this is public domain software - praise me, if ok, just don't blame me!
#include "dc4w.h"

extern   BOOL  SetChgAll( BOOL bChg );

// Global variables 
 
//HINSTANCE g_hinst;    // handle to application instance 
#define  g_hinst     g_hInst  // handle to application instance 
char g_achTemp[256];  // temporary buffer for strings 
//HWND g_hwndMain;      // main application window 
HWND g_hwndTabCtrl = 0;  // tab control 
HWND g_hwndDisplay = 0;  // handle to static control in 
                         //   tab control's display area 

// some sample code
// DoCreateDisplayWindow - creates a child window (a static 
//     control) to occupy the tab control's display area. 
// Returns the handle to the static control. 
// hwndParent - parent window (the application's main window). 
 
//HWND WINAPI DoCreateDisplayWindow(HWND hwndParent) 
HWND dc4wCreateDisplayWindow_NOT_USED(HWND hwndParent) 
{ 
    HWND hwndStatic = CreateWindow("STATIC", "", 
        WS_CHILD | WS_VISIBLE | WS_BORDER, 
        0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 
        hwndParent, NULL, g_hinst, NULL); 
 
    sprtf( "Created static child %x for %X"MEOR, hwndStatic, hwndParent );

    return hwndStatic; 
}  // not used

// MainWindowProc - processes the message for the main window class. 
// The return value depends on the message. 
// hwnd - handle to the window. 
// uMsg - identifier for the message. 
// wParam - message-specific parameter. 
// lParam - message-specific parameter. 
 
LRESULT CALLBACK PREFDIALOGPROC_NOT_USED( 
        HWND hwnd, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam 
        ) 
{ 
    switch (uMsg) { 
        case WM_SIZE: { 
                HDWP hdwp; 
                RECT rc; 
 
                // Calculate the display rectangle, assuming the 
                // tab control is the size of the client area. 
                SetRect(&rc, 0, 0, 
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); 
                TabCtrl_AdjustRect(g_hwndTabCtrl, FALSE, &rc); 
 
                // Size the tab control to fit the client area. 
                hdwp = BeginDeferWindowPos(2); 
                DeferWindowPos(hdwp, g_hwndTabCtrl, NULL, 0, 0, 
                    GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 
                    SWP_NOMOVE | SWP_NOZORDER 
                    ); 
 
                // Position and size the static control to fit the 
                // tab control's display area, and make sure the 
                // static control is in front of the tab control. 
                DeferWindowPos(hdwp, 
                    g_hwndDisplay, HWND_TOP, rc.left, rc.top, 
                    rc.right - rc.left, rc.bottom - rc.top, 0 
                    ); 
                EndDeferWindowPos(hdwp); 
            } 
            break; 
 
        case WM_NOTIFY: 
//            sprtf( "WM_NOTIFY: To PREFDIALOGPROC %#x %#x"MEOR, wParam, lParam );
            switch (HIWORD(wParam))
            { 
                case 0: 
                    //. 
                    //.    // menu command processing 
                    //. 
                   break;
 
                case TCN_SELCHANGE: { 
                        int iPage = TabCtrl_GetCurSel(g_hwndTabCtrl); 
//                        LoadString(g_hinst, IDS_FIRSTDAY + iPage, 
//                            g_achTemp, sizeof(g_achTemp)); 
                        SendMessage(g_hwndDisplay, WM_SETTEXT, 0,
                            (LPARAM) g_achTemp); 
                    } 
                    break; 
            } 
            break; 
 
            //. 
            //.       // additional message processing 
            //. 
 
        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 
    return 0; 
}  // NOT USED - original sample code only 


// DoCreateTabControl - creates a tab control, sized to fit the 
//     specified parent window's client area, and adds some tabs. 
// Returns the handle to the tab control. 
// hwndParent - parent window (the application's main window). 
 
//HWND WINAPI DoCreateTabControl(HWND hwndParent) 
HWND dc4wCreateTabControl(HWND hwndParent, PRECT lpr, LPTSTR lpt, INT iTabs)
{ 
    RECT rcClient; 
    HWND hwndTab; 
    TCITEM tie;
    LPTSTR  lps = lpt;     // get pointer to TAB LABELS
    int i = strlen(lps); 
 
    if(!i)
       return NULL;

    // Get the dimensions of the parent window's client area, and 
    // create a tab control child window of that size. 
    //GetClientRect(hwndParent, &rcClient); 
    rcClient = *lpr;

    InitCommonControls(); 

    hwndTab = CreateWindow( 
        WC_TABCONTROL, "", 
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, NULL, g_hinst, NULL 
        ); 

    if (hwndTab == NULL) 
        return NULL; 
 
    // Add tabs for each day of the week. 
    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 
    tie.pszText = g_achTemp; 
 
    for( i = 0; i < iTabs; i++ )
    { 
        //LoadString(g_hinst, IDS_FIRSTDAY + i, 
        //        g_achTemp, sizeof(g_achTemp)); 
        strcpy(g_achTemp, lps); 
        if( TabCtrl_InsertItem( hwndTab, i, &tie ) == -1 )
        { 
            DestroyWindow(hwndTab); 
            return NULL; 
        }
        lps += strlen(lps) + 1;
        if( *lps == 0 )
           break;
    } 
    return hwndTab; 
} 
 

#define C_PAGES 3 

typedef struct tagTabPage {
   LPTSTR   tp_Header;
   UINT     tp_ResId;
} TABPAGE, * PTABPAGE;


//LPTSTR ppPages[C_PAGES] = {
//   { "Compare" },
//   { "Outline" },
//   { "Expanded" }
//};

TABPAGE  sTabPage[C_PAGES] = {
   { "Compare",  IDD_COMPARE  },
   { "Outline",  IDD_OUTLINE  },
   { "Expanded", IDD_EXPANDED }
};

typedef struct tag_dlghdr { 
    HWND hwndTab;       // tab control 
    HWND hwndDisplay;   // current child dialog box 
    RECT rcDisplay;     // display rectangle for the tab control 
    DLGTEMPLATE * apRes[C_PAGES];
}DLGHDR, * PDLGHDR; 

LPTSTR   GetTabTxt( INT i )
{
   if(( i >= 0 ) &&
      ( i < C_PAGES ) )
   {
      // ok
      //return( ppPages[i] );
      return( sTabPage[i].tp_Header );
   }
   else
   {
      return( "Unknown" );
   }
}

// DoLockDlgRes - loads and locks a dialog box template resource. 
// Returns the address of the locked resource. 
// lpszResName - name of the resource 
 
DLGTEMPLATE * dc4wLockDlgRes(LPCSTR lpszResName) 
{ 
    HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG); 
    HGLOBAL hglb = LoadResource(g_hinst, hrsrc); 
    return (DLGTEMPLATE *) LockResource(hglb); 
} 

// OnChildDialogInit - Positions the child dialog box to fall 
//     within the display area of the tab control. 
//#ifndef GWL_USERDATA
//#define GWL_USERDATA -21
//#endif 
 
VOID dc4wOnChildDialogInit(HWND hwndDlg) 
{ 
    HWND hwndParent = GetParent(hwndDlg); 
    DLGHDR *pHdr = (DLGHDR *) GetWindowLongPtr( hwndParent, GWLP_USERDATA ); 
    SetWindowPos(hwndDlg, HWND_TOP, 
        pHdr->rcDisplay.left, pHdr->rcDisplay.top, 
        0, 0, SWP_NOSIZE);
    sprtf( "SetWindowPos for %#X at (%d,%d)."MEOR,
       hwndDlg,
       pHdr->rcDisplay.left,
       pHdr->rcDisplay.top );
} 

INT_PTR CALLBACK PREFCHILDDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      dc4wOnChildDialogInit( hDlg );
      iRet = TRUE;
      break;
   }
   return iRet;
}

// OnSelChanged - processes the TCN_SELCHANGE notification. 
// hwndDlg - handle to the parent dialog box. 
 
//VOID WINAPI OnSelChanged(HWND hwndDlg) 
VOID dc4wOnSelChanged(HWND hwndDlg) 
{ 
    DLGHDR *pHdr = (DLGHDR *) GetWindowLongPtr( hwndDlg, GWLP_USERDATA); 
    int iSel = TabCtrl_GetCurSel(pHdr->hwndTab); 
 
    // Destroy the current child dialog box, if any. 
    if (pHdr->hwndDisplay != NULL) 
        DestroyWindow(pHdr->hwndDisplay); 
 
    // Create the new child dialog box. 
    pHdr->hwndDisplay = CreateDialogIndirect(g_hinst, 
        pHdr->apRes[iSel], hwndDlg,
        PREFCHILDDLGPROC );   // ChildDialogProc); 
} 

HWND  g_hwndTab;  // established in INITDIALOG message

BOOL dc4wOnTabbedDialogInit(HWND hwndDlg) 
{ 
    //DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR)); 
    DWORD dwDlgBase = GetDialogBaseUnits(); 
    int cxMargin = LOWORD(dwDlgBase) / 4; 
    int cyMargin = HIWORD(dwDlgBase) / 8; 
//    TCITEM tie; 
    RECT rcTab; 
//    HWND hwndButton; 
//    RECT rcButton; 
    int i; 
    PDLGHDR pHdr = (PDLGHDR)MALLOC(sizeof(DLGHDR));

    if( !pHdr )
       return FALSE;

    // Save a pointer to the DLGHDR structure. 
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pHdr); 
 
    // Create the tab control. 
    //InitCommonControls(); 
    //pHdr->hwndTab = CreateWindow( 
    //    WC_TABCONTROL, "", 
    //    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 
    //    0, 0, 100, 100, 
    //    hwndDlg, NULL, g_hinst, NULL 
    //    ); 
    pHdr->hwndTab = g_hwndTab;

    if(pHdr->hwndTab == NULL) {
        // handle error
       return FALSE;
    }
 
    // Add a tab for each of the three child dialog boxes. 
    //tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    //tie.iImage = -1; 
    //tie.pszText = "First"; 
    //TabCtrl_InsertItem(pHdr->hwndTab, 0, &tie); 
    //tie.pszText = "Second"; 
    //TabCtrl_InsertItem(pHdr->hwndTab, 1, &tie); 
    //tie.pszText = "Third"; 
    //TabCtrl_InsertItem(pHdr->hwndTab, 2, &tie); 
 
    // Lock the resources for the three child dialog boxes. 
    //pHdr->apRes[0] = DoLockDlgRes(MAKEINTRESOURCE(DLG_FIRST)); 
    //pHdr->apRes[1] = DoLockDlgRes(MAKEINTRESOURCE(DLG_SECOND)); 
    //pHdr->apRes[2] = DoLockDlgRes(MAKEINTRESOURCE(DLG_THIRD)); 
    pHdr->apRes[0] = dc4wLockDlgRes(MAKEINTRESOURCE(IDD_COMPARE)); 
    pHdr->apRes[1] = dc4wLockDlgRes(MAKEINTRESOURCE(IDD_OUTLINE)); 
    pHdr->apRes[2] = dc4wLockDlgRes(MAKEINTRESOURCE(IDD_EXPANDED)); 
 
    // Determine the bounding rectangle for all child dialog boxes. 
    SetRectEmpty(&rcTab);

    for (i = 0; i < C_PAGES; i++)
    { 
        if (pHdr->apRes[i]->cx > rcTab.right) 
            rcTab.right = pHdr->apRes[i]->cx; 
        if (pHdr->apRes[i]->cy > rcTab.bottom) 
            rcTab.bottom = pHdr->apRes[i]->cy; 
    }
    //sprtf( "TAB display rect in dlg units %s."MEOR, Rect2Stg(&rcTab) );

    rcTab.right  = rcTab.right * LOWORD(dwDlgBase) / 4; 
    rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8; 
    //sprtf( "Set the TAB display rect to %s."MEOR, Rect2Stg(&rcTab) );

    // Calculate how large to make the tab control, so 
    // the display area can accommodate all the child dialog boxes. 
    TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);
    //sprtf( "After adjustment rect = %s."MEOR, Rect2Stg(&rcTab) );

    OffsetRect(&rcTab, cxMargin - rcTab.left, 
            cyMargin - rcTab.top); 
    //sprtf( "After offset rect = %s."MEOR, Rect2Stg(&rcTab) );
 
    // Calculate the display rectangle. 
    CopyRect(&pHdr->rcDisplay, &rcTab);

    TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay); 
 
    sprtf( "Final display rect = %s."MEOR, Rect2Stg(&pHdr->rcDisplay) );
    // Set the size and position of the tab control, buttons, 
    // and dialog box. 
    //SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, 
    //        rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, 
    //        SWP_NOZORDER); 
 
    // Move the first button below the tab control. 
    //hwndButton = GetDlgItem(hwndDlg, BTN_CLOSE); 
    //SetWindowPos(hwndButton, NULL, 
    //        rcTab.left, rcTab.bottom + cyMargin, 0, 0, 
    //        SWP_NOSIZE | SWP_NOZORDER); 
 
    // Determine the size of the button. 
    //GetWindowRect(hwndButton, &rcButton); 
    //rcButton.right -= rcButton.left; 
    //rcButton.bottom -= rcButton.top; 
 
    // Move the second button to the right of the first. 
    //hwndButton = GetDlgItem(hwndDlg, BTN_TEST); 
    //SetWindowPos(hwndButton, NULL, 
    //    rcTab.left + rcButton.right + cxMargin, 
    //    rcTab.bottom + cyMargin, 0, 0, 
    //    SWP_NOSIZE | SWP_NOZORDER); 
 
    // Size the dialog box. 
    //SetWindowPos(hwndDlg, NULL, 0, 0, 
    //    rcTab.right + cyMargin + 
    //    2 * GetSystemMetrics(SM_CXDLGFRAME), 
    //    rcTab.bottom + rcButton.bottom + 2 * cyMargin + 
    //    2 * GetSystemMetrics(SM_CYDLGFRAME) + 
    //    GetSystemMetrics(SM_CYCAPTION), 
    //    SWP_NOMOVE | SWP_NOZORDER); 
 
    // Simulate selection of the first item. 
    dc4wOnSelChanged(hwndDlg);

    return TRUE;

} 
 
 
BOOL  pd_WM_INITDIALOG( HWND hDlg )
{
   TCITEM   tie;
   INT      i;
   HWND     hwndTab;


   g_hwndTab = GetDlgItem( hDlg, IDC_TAB1 );
   if( !g_hwndTab )
   {
      EndDialog(hDlg, -1);
      return FALSE;
   }

   // this should be near the LAST thing done
   //CenterDialog( hDlg, hwndClient );   // centre it on the client
   hwndTab = g_hwndTab;

   // Add tabs for each page of items. 
   tie.mask = TCIF_TEXT | TCIF_IMAGE; 
   tie.iImage = -1; 
   tie.pszText = g_achTemp;

   i = 0;
   //strcpy(g_achTemp, "Compare");
   strcpy(g_achTemp, GetTabTxt(i) );   // "Compare"
   if( TabCtrl_InsertItem( hwndTab, i, &tie ) == -1 )
   {
       //DestroyWindow(hwndTab);
       return FALSE;
   }

   i++;
   //strcpy(g_achTemp, "Outline");
   strcpy(g_achTemp, GetTabTxt(i) );
   if( TabCtrl_InsertItem( hwndTab, i, &tie ) == -1 )
      return FALSE;

   i++;
   //strcpy(g_achTemp, "Expanded");
   strcpy(g_achTemp, GetTabTxt(i) );
   if( TabCtrl_InsertItem( hwndTab, i, &tie ) == -1 )
      return FALSE;

   if( !dc4wOnTabbedDialogInit( hDlg ) )
      return FALSE;

   return TRUE;
}

VOID  pd_WM_NOTIFY( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
//typedef struct tagNMHDR { 
//    HWND hwndFrom; 
//    UINT idFrom; 
//    UINT code; 
   NMHDR * pnh = (NMHDR *)lParam; 
   if( wParam == IDC_TAB1 )
   {
      switch( pnh->code )
      {
      case TCN_SELCHANGE:
         {
            int iPage = TabCtrl_GetCurSel(g_hwndTab);
            sprtf( "IDC_TAB1 changed to %d (%s)"MEOR,
               iPage,
               GetTabTxt(iPage) );
            dc4wOnSelChanged(hDlg);
         } 
         break; 
      }
   }
}

#define  MTABLEFT    5
#define  MTABWID     300
#define  MTABTOP     45
#define  MTABHEIGHT  160

VOID  dd_WM_SIZE( HWND hDlg, LPARAM lParam )
{
   HDWP hdwp; 
   RECT rc, rc2; 
 
   // Calculate the display rectangle, assuming the 
   // tab control is the size of the client area. 
   SetRect(&rc, 0, 0, 
           GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

   rc2 = rc;
   rc2.left  = MTABLEFT;
   rc2.right = rc2.left + MTABWID;
   rc2.top   = MTABTOP;
   rc2.bottom = rc2.top + MTABHEIGHT;


   if( g_hwndTabCtrl && g_hwndDisplay )
   {
      //TabCtrl_AdjustRect(g_hwndTab, FALSE, &rc); 
      TabCtrl_AdjustRect(g_hwndTabCtrl, FALSE, &rc2); 
 
      // Size the tab control to fit the client area. 
      hdwp = BeginDeferWindowPos(2); 

      //DeferWindowPos(hdwp, g_hwndTab, NULL, 0, 0, 
      //    GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 
      //    SWP_NOMOVE | SWP_NOZORDER 
      //    ); 
      DeferWindowPos(hdwp, g_hwndTabCtrl, NULL,
         MTABLEFT,      // x
         MTABTOP,       // y
         MTABWID,       // cx
         MTABHEIGHT,    // cy
         SWP_NOMOVE | SWP_NOZORDER );
 
      // Position and size the static control to fit the 
      // tab control's display area, and make sure the 
      // static control is in front of the tab control. 
      DeferWindowPos(hdwp, 
         g_hwndDisplay, HWND_TOP,
         rc.left,     // x
         rc.top,      // y
         rc.right - rc.left,   // cx
         rc.bottom - rc.top,   // cy
         0 ); 

      EndDeferWindowPos(hdwp);

      sprtf( "Set dialog to %s, and tab control to %s"MEOR,
         Rect2Stg( &rc ),
         Rect2Stg( &rc2 ) );

   }
   else
   {
      sprtf( "Dialog %x size %s, and tab control to %s"MEOR,
         hDlg,
         Rect2Stg( &rc ),
         Rect2Stg( &rc2 ) );
   }
} 

// Try to use modal dialogs in a tab control way
HWND     g_hParDlg   = 0;  // hidden parent - The HELP, OK, Cancel are handled back
// in the base dialog, and it puts up different dialogs to match the TAB setting
// ID_BTN_COMPARE, ID_BTN_OUTLINE ID_BTN_EXPANDED
DWORD    g_dwLastTab = 0;  // remember which page we were on
DWORD    g_dwChgCnt  = 0;  // click OK or Apply
DWORD    g_dwBtnChg  = 0;  // clicked on a button changed [X] to [ ] and vv

UINT  GetLastTab( VOID )
{
   UINT  ui = ID_BTN_COMPARE; // set default return
   switch(g_dwLastTab)
   {
   case 0:
      break;
   case 1:
      ui = ID_BTN_OUTLINE;
      break;
   case 2:
      ui = ID_BTN_EXPANDED;
      break;
   }
   return ui;
}


BOOL SetDialog( HWND hwndChild, HWND hwndParent )
{
	BOOL	bret = FALSE;
    RECT    rcChild, rcParent;
    int     cxChild, cyChild, cxParent, cyParent;
    int     cxScreen, cyScreen, xNew, yNew;
    HDC     hdc;

	if( hwndChild && hwndParent )
	{
		// Get the Height and Width of the child window
      // and its x,y screen location
		if( GetWindowRect( hwndChild, &rcChild ) )
		{
         // calculate child width
			cxChild = rcChild.right  - rcChild.left;
			cyChild = rcChild.bottom - rcChild.top;

			// Get the Height and Width of the parent window
			if( GetWindowRect( hwndParent, &rcParent ) );
			{
				cxParent = rcParent.right  - rcParent.left;
				cyParent = rcParent.bottom - rcParent.top;

				// Get the display limits
				if( hdc = GetDC(hwndChild) )
				{
					cxScreen = GetDeviceCaps(hdc, HORZRES);
					cyScreen = GetDeviceCaps(hdc, VERTRES);
					ReleaseDC( hwndChild, hdc );

					// Calculate new X position,
					// then adjust for screen
					xNew = rcParent.left +
						( (cxParent - cxChild) / 2 );
					if( xNew < 0 )
					{
						xNew = 0;
					}
					else if( (xNew + cxChild) > cxScreen )
					{
						xNew = cxScreen - cxChild;
					}

					// Calculate new Y position,
					// then adjust for screen
					//yNew = rcParent.top  +
					//	( (cyParent - cyChild) / 2 );
               // here we allign with the parents BOTTOM
					yNew = rcParent.bottom - cyChild;
					if( yNew < 0 )
					{
						yNew = 0;
					}
					else if( (yNew + cyChild) > cyScreen )
					{
						yNew = cyScreen - cyChild;
					}

					// Set it, and return
					bret = SetWindowPos( hwndChild,
                        NULL,
                        xNew, yNew,
                        0, 0,
                        SWP_NOSIZE | SWP_NOZORDER );
				}
			}
		}
	}
	return bret;
}


BOOL     g_bApplyOn = FALSE;
BOOL  SetApplyOnOff( HWND hDlg )
{
   BOOL  bRet = FALSE;
   HWND  hwnd = GetDlgItem( hDlg, ID_BTN_APPLY );
   if(hwnd)
   {
      EnableWindow( hwnd, g_bApplyOn );
      bRet = TRUE;
   }
   return bRet;
}

VOID  SetApplyOn( HWND hDlg )
{
   if( !g_bApplyOn )
   {
      g_bApplyOn = TRUE;
      SetApplyOnOff( hDlg );
   }
}

VOID  SetApplyOff( HWND hDlg )
{
   if( g_bApplyOn )
   {
      g_bApplyOn = FALSE;
      SetApplyOnOff( hDlg );
   }
}

VOID     gen_WM_INITDIALOG( HWND hDlg )
{
   SetApplyOnOff( hDlg );           // set the state of the APPLY button
   SetDialog( hDlg, g_hParDlg );   // centre it on parent
}

INT_PTR  gen_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
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

      // clicked on one of the TAB controls
   case ID_BTN_COMPARE:
   case ID_BTN_OUTLINE:
   case ID_BTN_EXPANDED:
      // ignore it if they clicked on US - we are already UP on show
      if( GetLastTab() != wCmd )
      {
         EndDialog(hDlg, wCmd);
         iRet = TRUE;
      }
      break;
//    GROUPBOX        "Not Exact Compare",IDC_STATIC,7,50,156,108
//    CONTROL         "Ignore CaSe",
   case IDC_NOCASE:  //,"Button",BS_AUTOCHECKBOX |
//    CONTROL         "Ignore Spaces (blanks)",
   case IDC_IGNORESP:   //"Button",
//    CONTROL         "Skip C/C++ Comments",
   case IDC_SKIPCCOMM:  //,"Button",
//    CONTROL         "Skip Text within Quotes",
   case IDC_SKIPQTXT:   //"Button",
//    CONTROL         "Ignore Line Termination",
   case IDC_IGNORETERM: //"Button",
//      g_dwBtnChg++;
//      break;
// case 'S': ta->diffopts |= INCLUDE_SAME;
   case IDD_IDENTICAL:  // INCLUDE_SAME );
// case 'L': ta->diffopts |= INCLUDE_LEFTONLY;
   case IDD_LEFT: // INCLUDE_LEFTONLY );
// case 'R': ta->diffopts |= INCLUDE_RIGHTONLY;
   case IDD_RIGHT:   // INCLUDE_RIGHTONLY );
// case 'D': ta->diffopts |= INCLUDE_DIFFER;
   case IDD_DIFFER:  // INCLUDE_NEWER );
   case IDD_DIFFER2: // INCLUDE_OLDER );
   case IDC_BACKUP:  // dwv, MAKE_BACKUP );
   case IDC_CHECK1:  // dwv, REVIEW_LIST );
   case IDC_CHK_WARN:   //, dwv, CHECK_WARN);
//   ICDB( IDD_IDENTICAL, dwi, INCLUDE_SAME     );
   // include DIFFER line. That is either left, right or moved
   //ICDB( IDD_DIFFER,    ( INCLUDE_DIFFER   ) );
//   ICDB( IDD_LEFT,       dwi, INCLUDE_LEFTONLY );
//   ICDB( IDD_RIGHT,      dwi, INCLUDE_RIGHTONLY );
   case IDC_MOVEDLEFT:  //  dwi, INCLUDE_MOVELEFT  );
   case IDC_MOVEDRIGHT: // dwi, INCLUDE_MOVERIGHT );

   // other options
   // case IDC_CHECK1:  //,     dwi, INCLUDE_LINENUMS  );
   case IDC_CHECK2:  //,     dwi, INCLUDE_TAGS      );
   case IDC_CHECK3:  //,     dwi, APPEND_FILE       );
   case IDC_CHECK4:  //,     dwi, INCLUDE_HEADER    );
   case IDC_CHECK6:  //,     dwi, WRAP_LINES        );

      g_dwBtnChg++;
      SetApplyOn( hDlg );
      break;

   case ID_BTN_APPLY:
      if( g_bApplyOn )
      {
         // g_bApplyOn = FALSE;
         SetApplyOff( hDlg );           // set the state of the APPLY button
      }
      break;
   }
   return iRet;
}

extern   VOID  co_WM_INITDIALOG( HWND hDlg );
extern   INT   co_IDOK( HWND hDlg );
extern   VOID  ou_WM_INITDIALOG( HWND hDlg );
extern   BOOL  ou_IDOK( HWND hDlg );
extern   INT  ex_IDOK( HWND hDlg );
extern   VOID  ex_WM_INITDIALOG( HWND hDlg );

INT_PTR  co_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD wCmd = LOWORD(wParam);

   if( ( wCmd == IDOK ) || ( wCmd == ID_BTN_APPLY ) )
   {
      g_dwChgCnt += co_IDOK( hDlg );  // update on OK or APPLY
   }

   iRet = gen_WM_COMMAND( hDlg, wParam, lParam );

   return iRet;
}

INT_PTR CALLBACK COMPDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      //CenterDialog( hDlg, hwndClient );   // centre it on the client
      co_WM_INITDIALOG( hDlg );
      gen_WM_INITDIALOG( hDlg );       // some common inits
//      SetDialog( hDlg, g_hParDlg );   // centre it on parent
      return TRUE;
      break;

   case WM_COMMAND:
      iRet = co_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   }

   return iRet;
}

INT_PTR  ou_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD wCmd = LOWORD(wParam);

   if( ( wCmd == IDOK ) || ( wCmd == ID_BTN_APPLY ) )
   {
      g_dwChgCnt += ou_IDOK( hDlg );  // update on OK or APPLY
   }

   iRet = gen_WM_COMMAND( hDlg, wParam, lParam );

   return iRet;
}

INT_PTR CALLBACK OUTLINEDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      ou_WM_INITDIALOG( hDlg );
      gen_WM_INITDIALOG( hDlg );       // some common inits
      //SetDialog( hDlg, g_hParDlg );   // centre it on parent
      //CenterDialog( hDlg, hwndClient );   // centre it on the client
      return TRUE;
      break;

   case WM_COMMAND:
      iRet = ou_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   }

   return iRet;
}

INT_PTR  ex_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD wCmd = LOWORD(wParam);

   if( ( wCmd == IDOK ) || ( wCmd == ID_BTN_APPLY ) )
   {
      g_dwChgCnt += ex_IDOK( hDlg );  // update on OK or APPLY
   }

   iRet = gen_WM_COMMAND( hDlg, wParam, lParam );

   return iRet;
}

INT_PTR CALLBACK EXPANDDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      //CenterDialog( hDlg, hwndClient );   // centre it on the client
      iRet = TRUE;
      ex_WM_INITDIALOG( hDlg );       // some common inits
      //SetDialog( hDlg, g_hParDlg );   // centre it on parent
      //return TRUE;
      gen_WM_INITDIALOG( hDlg );       // some common inits
      break;

   case WM_COMMAND:
      iRet = ex_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   }

   return iRet;
}

//INT_PTR DialogBoxParam(
//  HINSTANCE hInstance,     // handle to module
//  LPCTSTR lpTemplateName,  // dialog box template
//  HWND hWndParent,         // handle to owner window
//  DLGPROC lpDialogFunc,    // dialog box procedure
//  LPARAM dwInitParam );     // initialization value

INT_PTR  Run_Opt_Dlg( HWND hDlg, UINT ui, DLGPROC pDP )
{
   INT_PTR i = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(ui), //IDD_PREFERENCE2 // IDD_PREFERENCES),  // Copy files template
      hDlg,    // or hwndClient,
      pDP,     // dialog procedure
      (LPARAM) 0 );
   return i;
}


INT_PTR  pd_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = FALSE;
   DWORD wCmd = LOWORD(wParam);
   DWORD tabcmd = 0;
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
   //PUSHBUTTON   "Compare Options",
   case ID_BTN_COMPARE:
      tabcmd = Run_Opt_Dlg( hDlg, IDD_COMPARE, COMPDLGPROC );
      iRet = TRUE;
      break;
   //PUSHBUTTON   "Expanded Options",
   case ID_BTN_EXPANDED:
      tabcmd = Run_Opt_Dlg( hDlg, IDD_EXPANDED, EXPANDDLGPROC );
      iRet = TRUE;
      break;
   //PUSHBUTTON   "Outline Options",
   case ID_BTN_OUTLINE:
      tabcmd = Run_Opt_Dlg( hDlg, IDD_OUTLINE, OUTLINEDLGPROC );
      iRet = TRUE;
      break;
      
   }
   if( tabcmd )
   {
      iRet = FALSE;
      switch(tabcmd)
      {
      case IDOK:
      case IDCANCEL:
         EndDialog(hDlg, tabcmd);
         break;

      case ID_BTN_COMPARE:
         g_dwLastTab = 0;  // select first default page
         iRet = TRUE;
         break;
      case ID_BTN_OUTLINE:
         g_dwLastTab = 1;  // next, 2nd page
         iRet = TRUE;
         break;
      case ID_BTN_EXPANDED:
         g_dwLastTab = 2;  // currently LAST page of parameters
         iRet = TRUE;
         break;
      }
      if( iRet )
      {
         // put up the NEXT TAB
         PostMessage( hDlg, WM_COMMAND, (WPARAM)tabcmd, 0 );
      }
      iRet = TRUE;
   }
   return iRet;
}

INT_PTR CALLBACK PREFDLGPROC(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      {
         UINT  ui  = GetLastTab();
         g_hParDlg = hDlg; // set the DIALOG
         CenterDialog( hDlg, hwndClient );   // centre it on the client
         PostMessage( hDlg, WM_COMMAND, (WPARAM)ui, 0 );
         g_dwBtnChg = 0;
         // g_dwChgCnt = 0;
      }
      return TRUE;
      break;

   case WM_COMMAND:
      iRet = pd_WM_COMMAND( hDlg, wParam, lParam );
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   }

   return iRet;
}


INT_PTR CALLBACK PREFDLGPROC_NOT_USED(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR  iRet = FALSE;
   switch(uMsg)
   {

   case WM_INITDIALOG:
      if( pd_WM_INITDIALOG( hDlg ) )
      {
//The dialog box procedure should return TRUE to direct the system to set the 
//keyboard focus to the control specified by wParam. Otherwise, it should 
//return FALSE to prevent the system from setting the default keyboard focus. 
         // iRet = TRUE;
         EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );;
         SetFocus( GetDlgItem( hDlg, IDCANCEL ) );
      }
      break;

   case WM_NOTIFY:
      pd_WM_NOTIFY( hDlg, wParam, lParam );
      iRet = TRUE;
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
         }
      }
      break;

   case WM_CLOSE:
      EndDialog(hDlg, 0);
      iRet = TRUE;
      break;

   case WM_SIZE:
      dd_WM_SIZE( hDlg, lParam );
      break; 

   //case WM_DESTROY:
   //   REMOVE_PROP( hDlg, COPY_ATOM );
   //   break;

   }

   return iRet;

}

TCHAR g_szExeFile[264] = {"TabCtrl\\DEBUG\\TABCTRL.EXE"};
TCHAR g_szCmdLine[264];
TCHAR g_szTmpIni2[] = "TEMPINI.INI";   // this would be in c:\windows\tempini.ini!!!
TCHAR g_szTmpIni[264]  = {"D:\\GTools\\Tools\\dc4w\\TEMPINI.INI"};

STARTUPINFO          sSI;
PROCESS_INFORMATION  sPI;

VOID  Set_SI( STARTUPINFO * p, LPTSTR pcmdline )
{
   /* Launch the process and waits for it to complete */
   STARTUPINFO si;
   ZeroMemory( &si, sizeof(STARTUPINFO) );
   si.cb = sizeof(STARTUPINFO);
   //si.lpReserved = NULL;
   //si.lpReserved2 = NULL;
   //si.cbReserved2 = 0;
   si.lpTitle = pcmdline; 
   //si.lpDesktop = (LPTSTR)NULL;
   si.dwFlags = STARTF_FORCEONFEEDBACK;
   memcpy( p, &si, sizeof(STARTUPINFO) );
}

BOOL  g_bUseEDSI = TRUE;
DWORD optsthreadid;

DWORD WINAPI do_getopts( PVOID lpParameter )  // thread data
{

   LPTSTR               lpcmd = &g_szCmdLine[0];
   LPTSTR               lpini = &g_szTmpIni[0];
   STARTUPINFO         * psSI = &sSI;
   PROCESS_INFORMATION * psPI = &sPI;

   dc4w_UI(TRUE);

   // CreateThread - NO CREATE PROCESS and wait for it ...
   ZeroMemory(psSI, sizeof(STARTUPINFO) );
   if( g_bUseEDSI )
      Set_SI( psSI, lpcmd );
   else
      psSI->cb = sizeof(STARTUPINFO);

   ZeroMemory(psPI, sizeof(PROCESS_INFORMATION));

   if( CreateProcess( g_szExeFile,   // name of executable module
      lpcmd,   // command line string
      NULL, // SD
      NULL, // SD
      FALSE,   // handle inheritance option
      CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, // creation flags
      NULL, // new environment block
      NULL, // current directory name
      psSI,      // startup information
      psPI  ) ) // process information
   {
      DWORD dwi;
      TCHAR    szhex[64];
      LPTSTR   lps = &szhex[0];
      sprtf( "Got [%s] running. id=%X"MEOR, g_szExeFile, psPI->dwProcessId );

      /* wait for completion. */
      dwi = WaitForSingleObject( psPI->hProcess, INFINITE );
      switch(dwi)
      {
      case WAIT_ABANDONED:
         lps = "Abandoned";
         break;
      case WAIT_OBJECT_0:
         lps = "ClosedApp";   // "Object0";
         break;
      case WAIT_TIMEOUT:
         lps = "TimeOut";
         break;
      case WAIT_FAILED:
         lps = "Failed";
         break;
      default:
         lps = &szhex[0];
         sprintf(lps, "%#X", dwi );
         break;
      }

      /* close process and thread handles */
      CloseHandle( psPI->hThread);
      CloseHandle( psPI->hProcess);
      sprtf( "Closed [%s]. id=%X ?=%s"MEOR, g_szExeFile, psPI->hThread, lps );

   }

   dc4w_UI(FALSE);

   return( (DWORD) 0xadded );
}


//void do_editthread(PVIEW view, int option)
void do_optsthread( void )
{
   HANDLE thread;
   thread = CreateThread( NULL, 0,
            do_getopts,
            0,
            0,
            &optsthreadid );
        if (thread == NULL)
        {
                /* The createthread failed, do without the extra thread - just
                 * call the function synchronously
                 */
                 do_getopts( 0 );
        }
        else
        {
           CloseHandle(thread);
        }

} /* do_editthread */

BOOL  g_bUseOld = FALSE;

INT_PTR   Do_IDM_PREFERENCES( HWND hWnd )
{
   INT_PTR  i;

   g_dwChgCnt = 0;   // FIX20081125 - moved to HERE
   dc4w_UI(TRUE);

   InitCommonControls(); 
   //i = -1;
   i = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_PREFERENCE3), //IDD_PREFERENCE2 // IDD_PREFERENCES),  // Copy files template
      hwndClient,
      PREFDLGPROC,     // dialog procedure
      (LPARAM) 0 );

#ifdef   USETHDDTABCTRL
   {
      LPTSTR               lpcmd = &g_szCmdLine[0];
      LPTSTR               lpini = &g_szTmpIni[0];
      STARTUPINFO         * psSI = &sSI;
      PROCESS_INFORMATION * psPI = &sPI;

   sprintf(lpcmd, "%s",
      lpini );
   i = SetChgAll( TRUE );
   WriteINI( lpini, TRUE );
   i = SetChgAll( i );

   do_optsthread();  // start a thread to get OPTIONS

   if( g_bUseOld )
   {

      // CreateThread - NO CREATE PROCESS and wait for it ...
      ZeroMemory(psSI, sizeof(STARTUPINFO) );
      if( g_bUseEDSI )
         Set_SI( psSI, lpcmd );
      else
         psSI->cb = sizeof(STARTUPINFO);
   
      ZeroMemory(psPI, sizeof(PROCESS_INFORMATION));
   
      if( CreateProcess( g_szExeFile,   // name of executable module
         lpcmd,   // command line string
         NULL, // SD
         NULL, // SD
         FALSE,   // handle inheritance option
         CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, // creation flags
         NULL, // new environment block
         NULL, // current directory name
         psSI,      // startup information
         psPI  ) ) // process information
      {
         sprtf( "Got [%s] running. id=%X"MEOR, g_szExeFile, psPI->dwProcessId );
         /* wait for completion. */
         WaitForSingleObject(psPI->hProcess, INFINITE);
   
         /* close process and thread handles */
         CloseHandle( psPI->hThread);
         CloseHandle( psPI->hProcess);
   
      }
   }

   }
#endif   // #ifdef   USETHDDTABCTRL
   
   dc4w_UI(FALSE);
   if ( g_dwChgCnt )   // FIX20081125 - moved to HERE
      PostMessage( hWnd, WM_COMMAND, IDM_REFRESH, 0 );

   return i;
}

// eof - dc4wOpts.c
