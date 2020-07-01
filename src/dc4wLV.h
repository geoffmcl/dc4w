
// dc4wLV.h
#ifndef  _dc4wLV_HH
#define  _dc4wLV_HH

#define  dwflg_Rev      0x00000001  // invert the sort

#define	UseComCtrls(a)	\
{\
	INITCOMMONCONTROLSEX _iccex;\
	_iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);\
	_iccex.dwICC = a;\
	InitCommonControlsEx(&_iccex);\
}


#ifdef ADD_LIST_VIEW

#define IDC_LISTVIEW	1000

extern   HWND  g_hListView;
extern   HFONT g_hFixedFont8, g_hfCN8, g_hFF8bold;    // LOGFONT creations

//#define IDC_LISTVIEW 1000

#define  NO_IMAGE_LISTS


// common flag - only stored in 0, but returned in all requests
#define  dwflg_Column1  0x00100000  // 8-bits - Column with FOCUS
#define  dwflg_HadEnt   0x40000000  // had enter - not implemented yet *TBD*
#define  dwflg_HadClk   0x80000000  // user click - only in col=0 - had sort

//#define  dwflg_Common   (dwflg_HadClk | dwflg_HadEnt)
#define  dwflg_Common   0xfff00000  // common bits

typedef struct tagLISTHEADERS {
   LPTSTR   lh_pTitle;  // title at top of column
   DWORD    lh_dwWidth; // suggested WIDTH of the column
   DWORD    lh_dwFlag;  // flag of bits - used by list
}LISTHEADERS, * PLISTHEADERS;

#define  LV_DEF_COLS    7     // presently 7 columns

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

// helpful macros
extern   LVOPTIONS   sLVOptions;
#define  glv_iAddTrack  sLVOptions.lvo_iAddTrack   // case IDM_HOVERSEL
#define  glv_iChgTrack  sLVOptions.lvo_iChgTrack
//   int	lvo_iAddCheck;	// LVS_EX_CHECKBOXES case IDM_CHECKBOXES:
//   int	lvo_iChgCheck;
#define  glv_iAddGrid   sLVOptions.lvo_iAddGrid	// LVS_EX_GRIDLINES  case IDM_LVGRID
#define  glv_iChgGrid   sLVOptions.lvo_iChgGrid
//   int	lvo_iAddFull;	// LVS_EX_FULLROWSELECT case IDM_FULLROWSELECT:
//   int	lvo_iChgFull;
//   int	lvo_iAddHDrag;	// LVS_EX_HEADERDRAGDROP case IDM_HEADERDRAGDROP:
//   int	lvo_iChgHDrag;
//   int	lvo_iAddISub;	// LVS_EX_SUBITEMIMAGES case IDM_SUBITEMIMAGES:
//   int	lvo_iChgISub;

typedef struct LVINST { // an instance
   PLISTHEADERS   lvi_pHeaders;  // pointer to HEADER structure
   DWORD          lvi_nColumns;  // count of active columns
   PLVOPTIONS     lvi_pOptions;  // point to active OPTIONS
}LVINST, * PLVINST;


extern   HWND LVCreateWindow(HINSTANCE hInstance, HWND hwndParent);  // switch to LISTVIEW
extern   BOOL  setcolumns( HWND hLV );
extern   VOID  LVEmpty( HWND hwndListView );
extern   LRESULT LVInsertItem( HWND hwndListView, LPTSTR * pstgs, INT cnt, LPARAM lp );
extern   LRESULT addci2lv( LPTSTR pfile, LPTSTR path, LPTSTR pinfo, PFD pfd1, PFD pfd2,
                        LPARAM lp );
extern   VOID  LVBeginUpdate( HWND hwndListView );
extern   VOID  LVEndUpdate( HWND hwndListView );

#define  BEGINLVUPDATE     LVBeginUpdate( g_hListView )
#define  ENDLVUPDATE       LVEndUpdate( g_hListView )

// restart the LISTVIEW, if any
#define  CLEARLISTVIEW  LVEmpty( g_hListView )

extern   DWORD getcolumnflag( DWORD col );
extern   DWORD orcolumnflag( DWORD col, DWORD flg );
extern   DWORD andcolumnflag( DWORD col, DWORD flg );
extern   DWORD xorcolumnflag( DWORD col, DWORD flg );
extern   VOID  setcolumnheader( DWORD col, DWORD flag );
extern   DWORD LVcolumncount( VOID );

extern   void LVSwitchView(HWND hwndListView, LONG_PTR dwView);
extern   void LVAddExStyle(HWND hwndListView, LONG_PTR dwNewStyle);
extern   void LVRemoveExStyle(HWND hwndListView, LONG_PTR dwNewStyle);
extern   int  LVToggleHover( HWND hWnd );
//extern   BOOL LVInitColumns(HWND hwndListView, PLISTHEADERS plhs, INT cnt );
extern   BOOL LVInitColumns2(HWND hwndListView, PLISTHEADERS plhs, INT cnt,
                             PLVOPTIONS plvo );

// block of data for listview strings
typedef struct tagMYLVITEM {
   TCHAR li_szFile[264];
   TCHAR li_szPath[264];
   TCHAR li_szInfo[264];
   TCHAR li_szLDate[32];
   TCHAR li_szLSize[32];
   TCHAR li_szRDate[32];
   TCHAR li_szRSize[32];
}MYLVITEM, * PMYLVITEM;

#endif // #ifdef ADD_LIST_VIEW

typedef struct tagSORTPARAM {
	int			ccol;
	BOOL        brev;
}SORTPARAM, * PSORTPARAM;


#endif   // _dc4wLV_HH
// dc4wLV.h
