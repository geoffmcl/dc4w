
// dc4wList.c - LISTVIEW control
// this is public domain software - praise me, if ok, just don't blame me!

#include	"dc4w.h"
//#include <shlwapi.h>
// #include "dc4wLV.h"

#ifdef ADD_LIST_VIEW
////////////////////////////////////////////////////////////////////
// attempt at a generic implementation, to be added easily
//#define IDC_LISTVIEW	1000
//extern   HWND  g_hListView;
//extern   HFONT g_hFixedFont8, g_hfCN8, g_hFF8bold;    // LOGFONT creations
//#define IDC_LISTVIEW 1000
//#define  NO_IMAGE_LISTS
//typedef struct tagLISTHEADERS {
//   LPTSTR   lh_pTitle;  // title at top of column
//   DWORD    lh_dwWidth;   // suggested WIDTH of the column
//   DWORD    lh_dwFlag;  // flag of bits - used by list
//}LISTHEADERS, * PLISTHEADERS;

#define  VLV(a)      ( a && IsWindow(a) )

extern   BOOL  g_bLVOff, g_bChgLVOff;

//TCHAR    g_szSzFm[] = "%10I64u ";
TCHAR    g_szSzFm[] = "%8I64u";


LISTHEADERS sOutlineList[LV_DEF_COLS] = {
   { "File",   24*8, 0 },
   { "Path",   32*8, 0 },
   { "Info",   16*8, 0 },
   { "L.Date", 14*8, 0 },
   { "R.Date", 14*8, 0 },
   { "L.Size", 9*8,  0 },
   { "R.Size", 9*8,  0 }
};

#define  nOutlineCols   (sizeof(sOutlineList) / sizeof(LISTHEADERS)) // = 7

#ifndef  _dc4wLV_HH

typedef  struct LVOPTIONS {
   int   lvo_iType;    // LVS_REPORT, LIST or ICONS = DEF_LV_TYPE;
   int	lvo_iChgType;
   int	lvo_iAddTrack; // case IDM_HOVERSELECT:
   int	lvo_iChgTrack;
   int	lvo_iAddCheck;	// LVS_EX_CHECKBOXES case IDM_CHECKBOXES:
   int	lvo_iChgCheck;
   int	lvo_iAddGrid;	// LVS_EX_GRIDLINES  case IDM_GRIDLINES:
   int	lvo_iChgGrid;
   int	lvo_iAddFull;	// LVS_EX_FULLROWSELECT case IDM_FULLROWSELECT:
   int	lvo_iChgFull;
   int	lvo_iAddHDrag;	// LVS_EX_HEADERDRAGDROP case IDM_HEADERDRAGDROP:
   int	lvo_iChgHDrag;
   int	lvo_iAddISub;	// LVS_EX_SUBITEMIMAGES case IDM_SUBITEMIMAGES:
   int	lvo_iChgISub;
}LVOPTIONS, * PLVOPTIONS;

typedef struct LVINST { // an instance
   PLISTHEADERS   lvi_pHeaders;  // pointer to HEADER structure
   DWORD          lvi_nColumns;  // count of active columns
   PLVOPTIONS     lvi_pOptions;  // point to active OPTIONS
}LVINST, * PLVINST;

#endif   // #ifndef  _dc4wLV_HH

LVOPTIONS   sLVOptions = {
LVS_REPORT, 0,
	0, 0,    //   case IDM_HOVERSELECT:
   0, 0, 	//   case IDM_CHECKBOXES: // LVS_EX_CHECKBOXES
	0, 0,    //   case IDM_GRIDLINES:	// LVS_EX_GRIDLINES
   1, 0,    //case IDM_FULLROWSELECT:	// LVS_EX_FULLROWSELECT
   0, 0,    //case IDM_HEADERDRAGDROP: // LVS_EX_HEADERDRAGDROP
   0, 0,    //case IDM_SUBITEMIMAGES: // LVS_EX_SUBITEMIMAGES
};

LVOPTIONS   sLVOptions_ORG = {
LVS_REPORT, 0,
	1, 0,    //   case IDM_HOVERSELECT:
   1, 0, 	//   case IDM_CHECKBOXES: // LVS_EX_CHECKBOXES
	1, 0,    //   case IDM_GRIDLINES:	// LVS_EX_GRIDLINES
   1, 0,    //case IDM_FULLROWSELECT:	// LVS_EX_FULLROWSELECT
   0, 0,    //case IDM_HEADERDRAGDROP: // LVS_EX_HEADERDRAGDROP
   0, 0,    //case IDM_SUBITEMIMAGES: // LVS_EX_SUBITEMIMAGES
};


// forward refs
void LVSwitchView(HWND hwndListView, LONG_PTR dwView);
void LVAddExStyle(HWND hwndListView, LONG_PTR dwNewStyle);
void LVRemoveExStyle(HWND hwndListView, LONG_PTR dwNewStyle);
int	LVToggleHover( HWND hWnd );
BOOL LVInitColumns(HWND hwndListView, PLISTHEADERS plhs, INT cnt );
BOOL LVInitColumns2(HWND hwndListView, PLISTHEADERS plhs, INT cnt, PLVOPTIONS plvo );

HWND  g_hListView = 0;  // handle, of the LISTVIEW window
DWORD g_dwLVCount = 0;

INT   g_iLVColCnt = 0;

DWORD LVcolumncount( VOID )
{
   return( nOutlineCols );
}

BOOL  LVInit( VOID )
{
	UseComCtrls(ICC_LISTVIEW_CLASSES);
   return TRUE;
}

BOOL  setcolumns( HWND hLV )
{
   PLVOPTIONS plvo = &sLVOptions;
   return( LVInitColumns2( hLV, &sOutlineList[0], LVcolumncount(), plvo ) );
   // &sLVOptions ) );  // = nOutlineCols
}

DWORD lastcolhead = -1;

//typedef struct _LVCOLUMN { 
//    UINT mask; 
//    int fmt; 
//    int cx; 
//    LPTSTR pszText; 
//    int cchTextMax; 
//    int iSubItem; 
//#if (_WIN32_IE >= 0x0300)
//    int iImage;
//    int iOrder;
//#endif
//} LVCOLUMN, FAR *LPLVCOLUMN; 

static LVCOLUMN   _s_lvcols;
VOID  setcolumnheader( DWORD col, DWORD flag )
{
   LPTSTR      lps = gszTmpBuf;
//   DWORD       flag = getcolumnflag(col);
   BOOL        brev = ( flag & dwflg_Rev );
   LPLVCOLUMN  plvc = &_s_lvcols;
   LPTSTR      pform;

   ZeroMemory( plvc, sizeof(LVCOLUMN) );
   plvc->mask = LVCF_TEXT;
//   if((lastcolhead == -1 ) ||
//      (lastcolhead != col) )
//   {
      if((lastcolhead != -1 ) &&
         (lastcolhead != col) )
      {
         // undo this column header
         if(lastcolhead < LVcolumncount())   // nOutlineCols)
         {
            plvc->pszText = sOutlineList[lastcolhead].lh_pTitle;
            ListView_SetColumn( g_hListView, // LISTVIEW handle
               lastcolhead,   // column to fix
               plvc );
         }
      }

      // set this column header
      if(col < LVcolumncount() ) // = nOutlineCols
      {
         if( col == 0 )
         {
            // if the FIRST column
            //if( sOutlineList[0].lh_dwFlag & dwflg_HadClk )
            if( flag & dwflg_HadClk )
               pform = "[ ] %s - %d - %s";
            else
               pform = "[ ] %s - %d";

            sprintf(lps, pform,
                  sOutlineList[col].lh_pTitle,
                  g_dwLVCount,
                  (brev ? "Down" : "Up") );
         }
         else
         {
            sprintf(lps, "%s - %s",
               sOutlineList[col].lh_pTitle,
               (brev ? "Down" : "Up") );
         }

         plvc->pszText = lps;

         ListView_SetColumn( g_hListView, // LISTVIEW handle
               col,   // column to fix
               plvc );

      }

      lastcolhead = col;

//   }
}

DWORD getcolumnflag( DWORD col )
{
   if(col < LVcolumncount() ) // = nOutlineCols
   {
      DWORD flag = sOutlineList[col].lh_dwFlag;
// common flag - only stored in 0, but returned in all requests
// like #define  dwflg_HadClk   0x80000000  // user click - only in col=0 - had sort
      if(col)
         flag |= ( sOutlineList[0].lh_dwFlag & dwflg_Common );

      return flag;
   }
   return 0;
}

DWORD orcolumnflag( DWORD col, DWORD flg )
{
   if(col < LVcolumncount() ) // = nOutlineCols
   {
      sOutlineList[col].lh_dwFlag |= flg;
      return(sOutlineList[col].lh_dwFlag);
   }
   return -1;
}
DWORD andcolumnflag( DWORD col, DWORD flg )
{
   if(col < LVcolumncount() ) // = nOutlineCols
   {
      sOutlineList[col].lh_dwFlag &= flg;
      return(sOutlineList[col].lh_dwFlag);
   }
   return -1;
}
DWORD xorcolumnflag( DWORD col, DWORD flg )
{
   if(col < LVcolumncount() ) // = nOutlineCols
   {
      sOutlineList[col].lh_dwFlag ^= flg; // add or remove flag
      return(sOutlineList[col].lh_dwFlag);
   }
   return -1;
}

int ResizeLV( HWND hLV, LPRECT lpr )
{
	int	i = 0;
	if(( hLV           ) &&
		( IsWindow(hLV) ) &&
		( lpr           ) )
	{
		i = MoveWindow( hLV,
			lpr->left,              // x
			lpr->top,               // y
			(lpr->right  - lpr->left), // cx
			(lpr->bottom - lpr->top),  // cy
			TRUE );
	}
	return i;
}

/******************************************************************************

   LVResize

******************************************************************************/

void LVResize(HWND hwndListView, HWND hwndParent)
{
	RECT  rc;

	GetClientRect(hwndParent, &rc);

   rc.top = rc.bottom / 2;
   
	ResizeLV( hwndListView, &rc );
//	MoveWindow( hwndListView,
//		rc.left, rc.top,
//		rc.right - rc.left,
//		rc.bottom - rc.top,
//		TRUE );

}


/******************************************************************************

   LVCreateWindow

******************************************************************************/

HWND LVCreateWindow(HINSTANCE hInstance, HWND hwndParent)
{

	DWORD       dwStyle;
	HWND        hwndListView;
#ifndef  NO_IMAGE_LISTS
	HIMAGELIST  himlSmall;
	HIMAGELIST  himlLarge;
	SHFILEINFO  sfi;
#endif   // #ifndef  NO_IMAGE_LISTS
	BOOL        bSuccess = TRUE;

#ifdef  NO_IMAGE_LISTS
	dwStyle =   WS_TABSTOP | 
            WS_CHILD | 
            WS_BORDER | 
            LVS_AUTOARRANGE |
            LVS_REPORT | 
            WS_VISIBLE;
#else    // #ifndef  NO_IMAGE_LISTS
	dwStyle =   WS_TABSTOP | 
            WS_CHILD | 
            WS_BORDER | 
            LVS_AUTOARRANGE |
            LVS_REPORT | 
            LVS_EDITLABELS |
            LVS_SHAREIMAGELISTS |
            WS_VISIBLE;
#endif   // #ifndef  NO_IMAGE_LISTS

   LVInit();   // ensure common control ocx is loaded

	hwndListView = CreateWindowEx( WS_EX_CLIENTEDGE,	// ex style
		WC_LISTVIEW,			// class name - defined in commctrl.h
		NULL,					// window text
		dwStyle,				// style
		0,						// x position
		0,						// y position
		0,						// width
		0,						// height
		hwndParent,				// parent
		(HMENU)IDC_LISTVIEW,	// ID
		g_hInst,				// instance
		NULL );					// no extra data
   sprtf( "CREATED: Listview window (class=WC_LISTVIEW) hwnd=%#x."MEOR,
           hwndListView );

	if(!hwndListView)
		return NULL;

	LVResize(hwndListView, hwndParent);

#ifndef  NO_IMAGE_LISTS
	// ====================================================
	// set the large and small icon image lists
	himlSmall = (HIMAGELIST)SHGetFileInfo( TEXT("C:\\"),
		0,
		&sfi,
		sizeof(SHFILEINFO),
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

	himlLarge = (HIMAGELIST)SHGetFileInfo( TEXT("C:\\"),
		0,
		&sfi,
		sizeof(SHFILEINFO),
		SHGFI_SYSICONINDEX | SHGFI_LARGEICON);

	if(( himlSmall ) &&
		( himlLarge ) )
	{
      // we have the LARGE and SMALL image lists
		SendMessage(hwndListView, LVM_SETIMAGELIST, (WPARAM)LVSIL_SMALL, (LPARAM)himlSmall );
		SendMessage(hwndListView, LVM_SETIMAGELIST, (WPARAM)LVSIL_NORMAL, (LPARAM)himlLarge);
	}
	// ====================================================
#endif   // #ifndef  NO_IMAGE_LISTS

   {
      HWND  hwnd = hwndListView;
      HFONT hfont = 0;

//      if( g_hFF8bold )
//         hfont = g_hFF8bold;

      if( g_hfCN8 && !hfont )
         hfont = g_hfCN8;

      if( g_hFixedFont8 && !hfont )
         hfont = g_hFixedFont8;

      if(hfont)
         SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, TRUE);

   }

	return hwndListView;
}



void	LVSetViewType( HWND hwndListView, int iType );
#define	lvStyle(a,b)	\
	if( *a )\
	{\
		if( !( dwf & b ) )\
			LVAddExStyle(hwndListView,b);\
	}else{\
		if( dwf & b )\
			LVRemoveExStyle(hwndListView,b);\
	}

/* ==================

#define	DEF_LV_TYPE		LVS_REPORT
int		g_iType     = DEF_LV_TYPE;
int		g_iChgType  = 0;
	//   case IDM_HOVERSELECT:
int		g_iAddTrack = 1;
	//   case IDM_CHECKBOXES:
int		g_iAddCheck = 1;	// LVS_EX_CHECKBOXES
	//   case IDM_GRIDLINES:
int		g_iAddGrid  = 1;	// LVS_EX_GRIDLINES
   //case IDM_FULLROWSELECT:
int		g_iAddFull  = 1;	// LVS_EX_FULLROWSELECT
   //case IDM_HEADERDRAGDROP:
int		g_iAddHDrag = 0;	// LVS_EX_HEADERDRAGDROP
   //case IDM_SUBITEMIMAGES:
int		g_iAddISub  = 0;	// LVS_EX_SUBITEMIMAGES

int		g_iChgTrack = 0;	// IDM_HOVERSELECT
	//   case IDM_CHECKBOXES:
int		g_iChgCheck = 1;	// LVS_EX_CHECKBOXES
	//   case IDM_GRIDLINES:
int		g_iChgGrid  = 1;	// LVS_EX_GRIDLINES
   //case IDM_FULLROWSELECT:
int		g_iChgFull  = 1;	// LVS_EX_FULLROWSELECT
   //case IDM_HEADERDRAGDROP:
int		g_iChgHDrag = 0;	// LVS_EX_HEADERDRAGDROP
   //case IDM_SUBITEMIMAGES:
int		g_iChgISub  = 0;	// LVS_EX_SUBITEMIMAGES


void	LVSetDefault(HWND hwndListView)
{
	DWORD	dwf = 0;

	LVSetViewType( hwndListView, g_iType );

	dwf = (DWORD)SendMessage( hwndListView,
		LVM_GETEXTENDEDLISTVIEWSTYLE,
		0, 
		0 );

	//   case IDM_HOVERSELECT:
	lvStyle( &g_iAddTrack, LVS_EX_TRACKSELECT );
	//   case IDM_CHECKBOXES:
	lvStyle( &g_iAddCheck, LVS_EX_CHECKBOXES );
	//   case IDM_GRIDLINES:
	lvStyle( &g_iAddGrid,  LVS_EX_GRIDLINES  );
	//	 case IDM_FULLROWSELECT:
	lvStyle( &g_iAddFull,  LVS_EX_FULLROWSELECT );
	//	 case IDM_HEADERDRAGDROP:
	lvStyle( &g_iAddHDrag, LVS_EX_HEADERDRAGDROP );
	//	 case IDM_SUBITEMIMAGES:
	lvStyle( &g_iAddISub,  LVS_EX_SUBITEMIMAGES );

//	InvalidateRect( hwndListView, NULL, TRUE );
//	UpdateWindow( hwndListView );

}

BOOL LVInitColumns(HWND hwndListView, PLISTHEADERS plhs, INT cnt )
{
	LV_COLUMN   lvColumn;
	INT         i, j;
	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	//lvColumn.cx = 100;
	for(i = 0; i < cnt; i++)
	{
		//make the secondary columns smaller
      //if(i) lvColumn.cx = 50;
      lvColumn.cx       = plhs[i].lh_dwWidth;
		lvColumn.pszText  = plhs[i].lh_pTitle;  // szString[i];
      lvColumn.iSubItem = i;

		//SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)i, (LPARAM)&lvColumn);
      j = ListView_InsertColumn(hwndListView, i, &lvColumn);
      if( j == -1 )
         return FALSE;
	}

//	InsertListViewItems( hwndListView );

	// set listview default view
	LVSetDefault( hwndListView );

   g_iLVColCnt = cnt;

	return TRUE;

}



   =============== */


void	LVSetDefault2(HWND hwndListView, PLVOPTIONS plvo)
{
	DWORD	dwf = 0;

	LVSetViewType( hwndListView, plvo->lvo_iType ); // g_iType );

	dwf = (DWORD)SendMessage( hwndListView,
		LVM_GETEXTENDEDLISTVIEWSTYLE,
		0, 
		0 );

	//   case IDM_HOVERSELECT:
	lvStyle( &plvo->lvo_iAddTrack, LVS_EX_TRACKSELECT );
	//   case IDM_CHECKBOXES:
	lvStyle( &plvo->lvo_iAddCheck, LVS_EX_CHECKBOXES );
	//   case IDM_GRIDLINES:
	lvStyle( &plvo->lvo_iAddGrid,  LVS_EX_GRIDLINES  );
	//	 case IDM_FULLROWSELECT:
	lvStyle( &plvo->lvo_iAddFull,  LVS_EX_FULLROWSELECT );
	//	 case IDM_HEADERDRAGDROP:
	lvStyle( &plvo->lvo_iAddHDrag, LVS_EX_HEADERDRAGDROP );
	//	 case IDM_SUBITEMIMAGES:
	lvStyle( &plvo->lvo_iAddISub,  LVS_EX_SUBITEMIMAGES );

//	InvalidateRect( hwndListView, NULL, TRUE );
//	UpdateWindow( hwndListView );

}

//	static TCHAR szString[5][20] = {  TEXT("Image Number"), 
 //                                TEXT("Left"), 
   //                              TEXT("Top"), 
     //                            TEXT("Right"), 
       //                          TEXT("Bottom")};

/******************************************************************************

   LVInitColumns

******************************************************************************/
//typedef struct tagLISTHEADERS {
//   LPTSTR   lh_pTitle;  // title at top of column
//   DWORD    lh_dwWidth;   // suggested WIDTH of the column
//   DWORD    lh_dwFlag;  // flag of bits - used by list
//}LISTHEADERS, * PLISTHEADERS;

BOOL LVInitColumns2(HWND hwndListView, PLISTHEADERS plhs, INT cnt, PLVOPTIONS plvo)
{
	LV_COLUMN   lvColumn;
	INT         i, j;
	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	//lvColumn.cx = 100;
	for(i = 0; i < cnt; i++)
	{
		//make the secondary columns smaller
      //if(i) lvColumn.cx = 50;
      lvColumn.cx       = plhs[i].lh_dwWidth;
		lvColumn.pszText  = plhs[i].lh_pTitle;  // szString[i];
      lvColumn.iSubItem = i;

		//SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)i, (LPARAM)&lvColumn);
      j = ListView_InsertColumn(hwndListView, i, &lvColumn);
      if( j == -1 )
         return FALSE;
	}

//	InsertListViewItems( hwndListView );

	// set listview default view
	LVSetDefault2( hwndListView, plvo );

   g_iLVColCnt = cnt;

	return TRUE;

}

VOID  LVEmpty( HWND hwndListView )
{
   if( hwndListView && IsWindow(hwndListView) )
   if( g_dwLVCount &&
      ( VLV(hwndListView) ) )
   {
	   //empty the list
	   SendMessage( hwndListView, LVM_DELETEALLITEMS, 0, 0 );
      g_dwLVCount = 0;
   }
}

VOID  LVBeginUpdate( HWND hwndListView )
{
   if( VLV(hwndListView) )
      SendMessage( hwndListView, WM_SETREDRAW, FALSE, 0 );
}

VOID  LVEndUpdate( HWND hwndListView )
{
   if( VLV(hwndListView) )
   {
	   SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
	   UpdateWindow(hwndListView);
   }
}

LRESULT  LVInsertItem( HWND hwndListView, LPTSTR * pstgs, INT cnt, LPARAM lp )
{
   LVITEM   lvItem;
   INT      i;
   LRESULT  nIndex = -1;

   ZeroMemory( &lvItem, sizeof(LVITEM) );

   lvItem.mask = LVIF_TEXT;   // add TEXT
   if(lp)
   {
      lvItem.mask |= LVIF_PARAM; // and user lParam
      lvItem.lParam = lp;
   }

	lvItem.iItem = (INT)SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
   for( i = 0; i < cnt; i++ )
   {
      lvItem.pszText = pstgs[i];
		lvItem.iSubItem = i;
      if( i == 0 )
      {
			//add the item - get the index in case the ListView is sorted
			nIndex = SendMessage( hwndListView,
				LVM_INSERTITEM,
				(WPARAM)0,
				(LPARAM)&lvItem);
         if(nIndex == -1)
            return nIndex;
      }
      else
      {
         lvItem.mask = LVIF_TEXT;   // just TEXT for sub-items
			if( !SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem) )
            return -1;
      }
   }

   g_dwLVCount++; // add an item
//   setcolumheader(-1);
   return nIndex;
}

//typedef struct tagMYLVITEM {
//   TCHAR li_szFile[264];
//   TCHAR li_szPath[264];
//   TCHAR li_szInfo[264];
//   TCHAR li_szLDate[32];
//   TCHAR li_szLSize[32];
//   TCHAR li_szRDate[32];
//   TCHAR li_szRSize[32];
//}MYLVITEM, * PMYLVITEM;

MYLVITEM slvi;
// convert WIN32_FIND_DATA to strings for the LISTVIEW
//typedef struct _WIN32_FIND_DATA {
//  DWORD    dwFileAttributes; 
//  FILETIME ftCreationTime; 
//  FILETIME ftLastAccessTime; 
//  FILETIME ftLastWriteTime; 
//  DWORD    nFileSizeHigh; 
//  DWORD    nFileSizeLow; 
//  DWORD    dwReserved0; 
//  DWORD    dwReserved1; 
//  TCHAR    cFileName[ MAX_PATH ]; 
//  TCHAR    cAlternateFileName[ 14 ]; 
//} WIN32_FIND_DATA, *PWIN32_FIND_DATA; 

LRESULT  addci2lv( LPTSTR pfile, LPTSTR path, LPTSTR pinfo, PFD pfd1, PFD pfd2,
               LPARAM lp )
{
   LRESULT  lRes = -1;
   PMYLVITEM   pi = &slvi;
//#define  nOutlineCols   (sizeof(sOutlineList) / sizeof(LISTHEADERS))
   LPTSTR   lps[nOutlineCols];
   LONG              lg, iPos;
   LARGE_INTEGER     li1, li2;
   LPSYSTEMTIME      pst1, pst2;  // receives system time
   BOOL              b1, b2;
   LPTSTR            p;
   if( !pfd1 && !pfd2 )
      return -1;

   ZeroMemory( pi, sizeof(MYLVITEM) );
   pst1 = &g_sST1;
   pst2 = &g_sST2;

   lps[0] = pfile;  // set file name pointer
   p = strrchr(pfile,'\\');
   if(p)
   {
      p++;
      strcpy( pi->li_szFile, p );
      lps[0] = pi->li_szFile;
   }
   lps[1] = path;
   if(*path == '.')
   {
      strcpy( pi->li_szPath, &path[2] );
      lps[1] = pi->li_szPath;
      iPos = InStr( pi->li_szPath, lps[0] );
      if(iPos)
      {
         iPos--;
         if( iPos && ( pi->li_szPath[iPos] = '\\' ) )
            iPos--;

         pi->li_szPath[iPos] = 0;
      }
   }
   lps[2] = pinfo;
   p = strchr(pinfo,'>');
   if(p)
   {
      strcpy( pi->li_szInfo, pinfo );
      p = strchr(pi->li_szInfo,'>');
      if(p)
      {
         p++;
         *p = 0;
         lps[2] = pi->li_szInfo;
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

   lps[3] = pi->li_szLDate;
   lps[4] = pi->li_szRDate;
   lps[5] = pi->li_szLSize;
   lps[6] = pi->li_szRSize;

   // insert returns the INDEX, or -1 if ERROR inserting
   return( LVInsertItem( g_hListView, &lps[0], nOutlineCols, lp ) );
}


#ifdef  ADDSIMPLEICONLIST2

/******************************************************************************

   InsertListViewItems

******************************************************************************/

BOOL InsertListViewItems( HWND hwndListView )
{
	LV_ITEM     lvItem;
	int         i,
				nIndex,
				nImageCount;
	TCHAR       szString[MAX_PATH];
	HIMAGELIST  himl;
	IMAGEINFO   ii;

	SendMessage( hwndListView, WM_SETREDRAW, FALSE, 0 );

	//empty the list
	SendMessage( hwndListView, LVM_DELETEALLITEMS, 0, 0 );

	lvItem.lParam = 0;	// 32-bit value to associate with item
	//get the number of icons in the image list
	if( ( himl = (HIMAGELIST)SendMessage( hwndListView, LVM_GETIMAGELIST, (WPARAM)LVSIL_SMALL, 0 ) ) &&
		( nImageCount = ImageList_GetImageCount( himl ) ) )
	{
		for( i = 0; i < nImageCount; i++ )
		{
//			wsprintf(szString, TEXT("Image #%4d"), ( i + 1 ) );
			wsprintf(szString, TEXT("%4d Image #" ), ( i + 1 ) );
			//fill in the LV_ITEM structure for the first item
			lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
			lvItem.pszText = szString;
			lvItem.iImage = i;
			lvItem.iItem = SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
			lvItem.iSubItem = 0;

			//add the item - get the index in case the ListView is sorted
			nIndex = SendMessage( hwndListView,
				LVM_INSERTITEM,
				(WPARAM)0,
				(LPARAM)&lvItem);

			//set the text and images for the sub-items
			ImageList_GetImageInfo(himl, i, &ii);

			wsprintf(szString, TEXT("%d"), ii.rcImage.left);
			lvItem.iSubItem = 1;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

			wsprintf(szString, TEXT("%d"), ii.rcImage.top);
			lvItem.iSubItem = 2;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

			wsprintf(szString, TEXT("%d"), ii.rcImage.right);
			lvItem.iSubItem = 3;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

			wsprintf(szString, TEXT("%d"), ii.rcImage.bottom);
			lvItem.iSubItem = 4;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

		}
	}

	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
	UpdateWindow(hwndListView);

	return TRUE;
}

#endif   // #ifdef  ADDSIMPLEICONLIST2

/**************************************************************************

   LVSwitchView()

**************************************************************************/

void LVSwitchView(HWND hwndListView, LONG_PTR dwView)
{
	LONG_PTR dwStyle = GetWindowLongPtr(hwndListView, GWL_STYLE);
	SetWindowLongPtr(hwndListView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
	LVResize(hwndListView, GetParent(hwndListView));
}

/**************************************************************************

   LVAddExStyle()

**************************************************************************/

void LVAddExStyle(HWND hwndListView, LONG_PTR dwNewStyle)
{
	LONG_PTR dwStyle = SendMessage(hwndListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle |= dwNewStyle;
	SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

}

/**************************************************************************

   LVRemoveExStyle()

**************************************************************************/

void LVRemoveExStyle(HWND hwndListView, LONG_PTR dwNewStyle)
{
	LONG_PTR dwStyle = SendMessage(hwndListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle &= ~dwNewStyle;
	SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
}


int	LVToggleHover( HWND hwndListView )
{
   LONG_PTR dwf = 0;
   // get the current style
   if( VLV(hwndListView) )
   {
      dwf = SendMessage(hwndListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	   if( dwf & LVS_EX_TRACKSELECT )   // if ON
		   LVRemoveExStyle( hwndListView, LVS_EX_TRACKSELECT );  // remove
      else  // was not ON, so
		   LVAddExStyle( hwndListView, LVS_EX_TRACKSELECT );  // set style
      dwf ^= LVS_EX_TRACKSELECT;    // toggle the bit
   }
   return( dwf & LVS_EX_TRACKSELECT ); // return results
}

#define	tlvStyle(a,b)	\
	if( *a )\
	{\
		if( !( dwf & b ) )\
			LVAddExStyle(hLV,b);\
	}else{\
		if( dwf & b )\
			LVRemoveExStyle(hLV,b);\
	}

	//   case IDM_CHECKBOXES:
//	lvStyle( &plvo->lvo_iAddCheck, LVS_EX_CHECKBOXES );
	//	 case IDM_FULLROWSELECT:
//	lvStyle( &plvo->lvo_iAddFull,  LVS_EX_FULLROWSELECT );
	//	 case IDM_HEADERDRAGDROP:
//	lvStyle( &plvo->lvo_iAddHDrag, LVS_EX_HEADERDRAGDROP );
	//	 case IDM_SUBITEMIMAGES:
//	lvStyle( &plvo->lvo_iAddISub,  LVS_EX_SUBITEMIMAGES );
VOID  Do_Toggle( HWND hLV, PBOOL pb, PBOOL pc, LONG_PTR style )
{
   LONG_PTR dwf = SendMessage(hLV, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
   // LVToggleHover(g_hListView);
	//   case IDM_HOVERSEL:
   *pb = ! *pb;   // logically TOGGLE on/off
   tlvStyle( pb, style ); // physically alter LISTVIEW
   //lvStyle( &plvo->lvo_iAddTrack, LVS_EX_TRACKSELECT );
   *pc = TRUE;    // set CHANGED flag
}

VOID  Do_IDM_HOVERSEL( HWND hWnd )
{
   HWND  hLV = g_hListView;
   if( VLV(hLV) )
   {
      PLVOPTIONS plvo = &sLVOptions;
      PBOOL pb = &plvo->lvo_iAddTrack;
      PBOOL pc = &plvo->lvo_iChgTrack;
      Do_Toggle( hLV, pb, pc, LVS_EX_TRACKSELECT );
   }
}

//   case IDM_GRIDLINES:
VOID  Do_IDM_LVADDGRID( HWND hWnd )
{
   HWND  hLV = g_hListView;
   if( VLV(hLV) )
   {
      PLVOPTIONS plvo = &sLVOptions;
      PBOOL pb = &plvo->lvo_iAddGrid;
      PBOOL pc = &plvo->lvo_iChgGrid;
      Do_Toggle( hLV, pb, pc, LVS_EX_GRIDLINES );
   }
}

int	LVDlgToggleHover( HWND hDlg )
{
   HWND  hLV = GetDlgItem(hDlg, IDC_LISTVIEW);
   return(LVToggleHover(hLV));
}

// and an APPLICATION RC DEPENDANT menu items -
void	LVSetViewType( HWND hwndListView, int iType )
{
	LONG_PTR dwt;
	int		i = 0;
	switch(iType)
	{
	case IDM_LARGEICONS:
		dwt = LVS_ICON;
		i++;
		break;

	case IDM_SMALLICONS:
		dwt = LVS_SMALLICON;
		i++;
		break;
   
	case IDM_LIST:
		dwt = LVS_LIST;
		i++;
		break;

	case IDM_REPORT:
		dwt = LVS_REPORT;
		i++;
		break;

	}

	if( i )
		LVSwitchView( hwndListView, dwt );

}

//        POPUP "&List View Control"
//            MENUITEM "&Off",                        IDM_LISTVIEWOFF
//   case IDM_LISTVIEWOFF:
static HWND _s_hListView = 0;
extern   VOID DoResize(HWND);
extern   INT view_update_lv(PVIEW view);

BOOL  Set_LV_ON(VOID)
{
   if( _s_hListView )
   {
      g_bLVOff = FALSE;
//      g_bChgLVOff = TRUE;
      g_hListView = _s_hListView;
      view_update_lv( current_view );  // ensure the soon-to-be-visible lv is up-to-date
      ShowWindow(_s_hListView, SW_SHOW);  // let the PAINT go
      _s_hListView = 0; // kill store of handle
      return TRUE;
   }
   return FALSE;
}

BOOL  Set_LV_OFF(VOID)
{
   if(g_hListView)
   {
      g_bLVOff = TRUE;
//      g_bChgLVOff = TRUE;
      _s_hListView = g_hListView;
      g_hListView  = 0;
      ShowWindow(_s_hListView, SW_HIDE);
      return TRUE;
   }
   return FALSE;
}

VOID  Do_IDM_LISTVIEWOFF(VOID)
{
   if(g_hListView)
   {
      Set_LV_OFF();
      DoResize(hwndClient);
   }
   else if( _s_hListView )
   {
      Set_LV_ON();
      DoResize(hwndClient);
   }
   else
   {
      // could proceed to create the LISTVIEW
   }

   if( g_bLVOff != g_bUsrLVOff )
   {
      g_bUsrLVOff = g_bLVOff;
      g_bChgLVOff = TRUE;
   }
}

extern   BOOL  g_bIn5050;
BOOL  g_bChg5050 = FALSE;
VOID  Toggle5050(VOID)
{
   g_bIn5050  = !g_bIn5050;
   g_bChg5050 = TRUE;
}
//            MENUITEM "&50%",                        IDM_LISTVIEW50
//   case IDM_LISTVIEW50:
VOID  Do_IDM_LISTVIEW50(VOID)
{
   if( g_hListView && !g_bIn5050 )
   {
      Toggle5050();
      DoResize(hwndClient);
   }
}
//            MENUITEM "&100%",                       IDM_LISTVIEW100
//   case IDM_LISTVIEW100:
VOID  Do_IDM_LISTVIEW100(VOID)
{
   if( g_hListView && g_bIn5050 )
   {
      Toggle5050();
      DoResize(hwndClient);
   }
}

////////////////////////////////////////////////////////////////////
#endif // #ifdef ADD_LIST_VIEW


// eof - dc4wList.c
