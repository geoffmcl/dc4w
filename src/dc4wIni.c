
// dc4wIni.c
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"
// #include	"dc4wIni.h"
// forward reference
INT  Get_Excl_List( LPTSTR lpb2, PLE pH );

#define  it_None        0     // end of list
#define  it_Version     1
#define  it_String      2
#define  it_Int         3
#define  it_Bool        4
#define  it_WinSize     5     // special WINDOWPLACEMENT
#define  it_Rect        6
#define  it_Color       7
#define  it_SList       8     // simple list
#define  it_DWord       it_Int   // virtually the same in WIN32

typedef struct	tagINILST {	/* i */
	LPTSTR	i_Sect;
	LPTSTR	i_Item;
	DWORD	   i_Type;
	LPTSTR	i_Deft;
	PBOOL	   i_Chg;
	PVOID	   i_Void;
	DWORD	   i_Res1;
} INILST, * PINILST;

static	TCHAR	g_szDefIni[] = "DC4W.INI";
TCHAR	g_szIni[264];

TCHAR szBlk[] = "\0";

// [Sections]
TCHAR szVer[] = "Version";
TCHAR szOpt[] = "Options";
TCHAR szOut[] = "OutLine";
TCHAR szExp[] = "Expanded";
TCHAR szWin[] = "Window";
TCHAR szXcl[] = "Exclude";
TCHAR szClr[] = "Colours";
TCHAR szCmp[] = "Compares";
TCHAR szDif[] = "Differences";
TCHAR szFil[] = "FileLists";
TCHAR szZipL[] = "ZipSaveList";   // whne the COPY of the auto-zip is complete
TCHAR szLvw[] = "ListView";   // some LISTVIEW options

//TCHAR szLastZp[] = "LastZip";

// items in sections
// TCHAR szLvw[] = "[ListView]";   // some LISTVIEW options
TCHAR szLV50[] = "Show5050";
extern   BOOL  g_bIn5050;
extern   BOOL  g_bChg5050;
TCHAR szHLV[] = "HideListView";
BOOL  g_bLVOff    = FALSE;
BOOL  g_bUsrLVOff = FALSE;
BOOL  g_bChgLVOff = TRUE;
DWORD g_dwActSecs = 10;    // standard delay of timed text message
TCHAR szLVSEL[] = "TrackSelection";
TCHAR szLVGRD[] = "AddGridLines";
//#define  glv_iAddTrack  sLVOptions.lvo_iAddTrack   // case IDM_HOVERSEL
//#define  glv_iChgTrack  sLVOptions.lvo_iChgTrack
//#define  glv_iAddGrid   sLVOptions.lvo_iAddGrid	// LVS_EX_GRIDLINES  case IDM_LVGRID
//#define  glv_iChgGrid   sLVOptions.lvo_iChgGrid
TCHAR szPCcl[] = "Percent_of_Client";
extern   DWORD g_dwPercent;   // default = 50;
extern   BOOL  g_bChgPCnt; // begins = FALSE;

// [Version]
TCHAR szDt[] = "Date";
TCHAR szCVer[] = VER_DATE;    // control re-setting of INI file
BOOL  bChgAll = FALSE;        // if SET, whole file is cleaned and redone

// [Options]
// int line_numbers;
TCHAR szLNum[] = "LineNumbers";
BOOL  bChgLnN = FALSE;
TCHAR szEdit[] = "Editor";
// global   TCHAR editor_cmdline[]; // [264] = { "notepad %p" };  /* editor cmdline */
BOOL  bChgEd = FALSE;

#define  DEF_ZIP_CMD "c:\\mdos\\Zip8.bat -a -P -o TEMPZ001.zip @tempz001.lst"
// #define  DEF_ZIP_CMD2 "c:\\mdos\\Zip8.bat -a -P -o -x@d:\\temp\\TEMPEXCL.TXT TEMPZ001.zip @tempz001.lst"
#define  DEF_ZIP_CMD2 "c:\\mdos\\Zip8.bat -a -P -o -x@c:\\windows\\temp\\TEMPEXCL.TXT TEMPZ001.zip @tempz001.lst"

// This is ONLY to be used on final OUTPUT - It is historic information
TCHAR szZpCmd[] = "ZipCommand";
TCHAR szZipCmd[264] = { DEF_ZIP_CMD };
BOOL  bChgZp = FALSE;
TCHAR szZpCmd2[] = "ZipCommand2";
TCHAR szZipCmd2[264] = { DEF_ZIP_CMD2 };
BOOL  bChgZp2 = FALSE;
// *** THIS IS THE FULL ZIPUP INFORMATION ***
// sub-section [LastZip] - uses glob. fixed work structure for buffers
TCHAR szLastZp[] = "LastZip";
TCHAR szCmd[] = "cmd";
TCHAR szSws[] = "sw";
TCHAR szZip[] = "zip";
TCHAR szInp[] = "inp";
TCHAR szEnv[] = "env";
TCHAR szCmD[] = "cmp";

TCHAR szAutoZip[] = "AutoZip";
BOOL  bAutoZip = TRUE;  // can be overridden with -N command
BOOL  bChgAZp = FALSE;

// extern int outline_include - like INC_OUTLINE[2] - see dc4wWorkF.h - bit flags - ugh
// #define  INC_OUTLINE    (INCLUDE_SAME|INCLUDE_DIFFER|INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY)
// #define  INC_OUTLINE2   (INC_OUTLINE|INCLUDE_NEWER|INCLUDE_OLDER)
TCHAR szIncl[] = "FileInclude";
BOOL  bChgInc = FALSE;


TCHAR szBnks[] = "Blanks";
// BOOL  bChgBks = FALSE;
// BOOL picture_mode;
TCHAR szPic[] = "Picture";
BOOL  bChgPic = FALSE;
// IDM_OPTIGNOREDT
TCHAR szIgnDT[] = "Ignore_Date_Time";
//#define gbIgnDT	sFW.fw_bIgnDT // g BOOL
//#define gbChgIDT	sFW.fw_bChgIDT
TCHAR szIgnDT2[] = "Ignore_File_Time_COMPETELY";

// IDM_BOTHFILES, IDM_LONLY, IDM_RONLY
TCHAR szEM[] = "Expand_Mode";
//#define  expand_mode       sFW.fw_expand_mode
//#define gbChgEM	sFW.fw_bChgEM // g DWORD

TCHAR szLft[] = "NameLeft";
TCHAR szRit[] = "NameRight";
TCHAR gszLefti[264] = { "\0" };
TCHAR gszRitei[264] = { "\0" };
BOOL  bChgLf = FALSE;
BOOL  bChgRt = FALSE;

TCHAR szRec[] = "Recursive";

// gdwWrapWid / bChgWW
TCHAR szWW[]  = "WrapWidth";

TCHAR szVF[]  = "VerifyFlag";

//   { szOpt, szCOp,  it_DWord,(LPTSTR)&gdwCpyOpts, &bChgCO, 0, 0 },
TCHAR szCOp[] = "FileCopyOptions";
// gdwFileOpts - write file list options / bChgFO
TCHAR szFO[]  = "FileListInclude";

// gdwDiffOpts / bChgDO = expanded mode lines included in display
TCHAR szDO[]  = "DiffInclude";

// IDM_WARNING
TCHAR szWn[] = "Warning";

// IDM_OPTEXACT
TCHAR szExa[] = "Exact";
//BOOL  gbExact = TRUE;
//BOOL  bChgExa = FALSE;

// IDM_OPTEXCLUDE
TCHAR szBXcl[] = "Exclude";      // gbExclude, bChgExcl
TCHAR szXSel[] = "XSelection";   // giCurXSel, bChgXSel

//extern   TCHAR gszCopyTo[];   // destination of COPY files
//extern   BOOL  bChgCT;
TCHAR szCT[] = "CopyDestination";

//BOOL  gbVerify = TRUE;
//BOOL  bChgVer = FALSE;
//TCHAR szVri[] = "VerifyCopy";

// IDM_ADDSTATUS
TCHAR szAddS[] = "Status_Line";
//#define  gbAddS      sFW.fw_gbAddS
//#define  gbChgAddS   sFW.fw_bChgAddS

// IDM_DIR - put up directories DIALOG
TCHAR szAuSel[] = "AutoSelect1";
//#define  gbAutoSel   sFW.fw_bAutoSel
//#define  bChgASel   sFW.fw_bChgASel

//   case IDM_NONRS: // this is a display toggle
//TCHAR szShwNums[] = "ShowLineNumbers";
//      gbShowNums = !gbShowNums;
//      gbChgSN = TRUE;
//   case IDM_LNRS: case IDM_RNRS:
TCHAR szUseRt[] = "UseRightNumbers"; // just the source of the number - if abv ON
//      gbUseRight = !gbUseRight;
//      gbChgURt = TRUE;

TCHAR szDelOn[] = "Enable_Delete";
// gbDelOn, gbChgDel

//#define gbSimple	sFW.fw_bSimple // g BOOL
//#define gbChgSim	sFW.fw_bChgSim // g BOOL
TCHAR szSimp[] = "Simple_Display";
// ******************** end [options] ********

// section [OutLine]
TCHAR szGOut[] = "OutSaved";
BOOL  gbGotOut = FALSE;
WINDOWPLACEMENT   g_sWPO;
BOOL  bChgOut = FALSE;

// section [Expanded]
TCHAR szGExp[] = "ExpandedSaved";
BOOL  gbGotExp = FALSE;
WINDOWPLACEMENT   g_sWPE;
BOOL  bChgExp = FALSE;

TCHAR szShow[] = "ShowCmd";
TCHAR szMaxX[] = "MaxX";
TCHAR szMaxY[] = "MaxY";
TCHAR szMinX[] = "MinX";
TCHAR szMinY[] = "MinY";
TCHAR szLeft[] = "NormLeft";
TCHAR szTop[]  = "NormTop";
TCHAR szRite[] = "NormRight";
TCHAR szBot[]  = "NormBottom";

// section [Window]
TCHAR szZm[] = "Zoomed";
TCHAR szIc[] = "Iconic";
TCHAR szSz[] = "Size";

// simple x,y pos, and cx,cy size
//		if( GetWindowPlacement( &wp ) ) {
//		myApp.WriteProfileInt( SECT_WIND, PRO_XPOS, wp.rcNormalPosition.left );
//		myApp.WriteProfileInt( SECT_WIND, PRO_YPOS, wp.rcNormalPosition.top );
//		myApp.WriteProfileInt( SECT_WIND, PRO_XSIZE,
//			(wp.rcNormalPosition.right - wp.rcNormalPosition.left) );
//		myApp.WriteProfileInt( SECT_WIND, PRO_YSIZE,
//			(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) );

// section [Exclude]
//TCHAR szXcl[] = "Exclude";
//TCHAR szFn[] = "File%d";
TCHAR szLst[] = "List%d";
// 20080415 - Must have exclude list
TCHAR szMustHave[] = "*.obj;*.ilk;*.suo;*.sln;*.vcproj;*.ncb;*.user;*.lib;"
 "*.dep;buildlog.htm;*.manifest;*.exp;*.idb;*.map;*.pdb;*.res;*.exe;*.dll";

// ID_VIEW_NOEXCLUDES - section szXcl
TCHAR szNoEx[] = "NoExcludes"; // bNoExcludes, bChgNoExcl

// section [Colours]
//DWORD rgb_outlinehi = RGB(255, 0, 0);   /* hilighted files in outline mode  */
TCHAR szcHi[] = "Highlight";
BOOL  bChgcHi = FALSE;
/* expand view */
//DWORD rgb_leftfore =   RGB(  0,   0,   0);         /* foregrnd for left lines */
TCHAR szcELF[] = "ExpLeftFore";
BOOL  bChgcELF = FALSE;
//DWORD rgb_leftback  =  RGB(200,  80,   0);         /* backgrnd for left lines */
TCHAR szcELB[] = "ExpLeftBack";
BOOL  bChgcELB = FALSE;
//DWORD rgb_rightfore =  RGB(  0,   0,   0);         /* foregrnd for right lines*/
TCHAR szcERF[] = "ExpRightFore";
BOOL  bChgcERF = FALSE;
//DWORD rgb_rightback =  RGB(255, 255,   0);         /* backgrnd for right lines*/
TCHAR szcERB[] = "ExpRightBack";
BOOL  bChgcERB = FALSE; 
/* moved lines */
//DWORD rgb_mleftfore =  RGB(  0, 255, 128);         /* foregrnd for moved-left */
TCHAR szcMLF[] = "MovLeftFore";
BOOL  bChgcMLF = FALSE;
//DWORD rgb_mleftback =  RGB(128, 128, 128);         /* backgrnd for moved-left */
TCHAR szcMLB[] = "MovLeftBack";
BOOL  bChgcMLB = FALSE;
//DWORD rgb_mrightfore = RGB(  0, 255, 255);         /* foregrnd for moved-right*/
TCHAR szcMRF[] = "MovRightFore";
BOOL  bChgcMRF = FALSE;
//DWORD rgb_mrightback = RGB(128, 128, 128);         /* backgrnd for moved-right*/
TCHAR szcMRB[] = "MovRightBack";
BOOL  bChgcMRB = FALSE;
/* bar window */
//DWORD rgb_barleft =    RGB(255,   0,   0);         /* bar sections in left only  */
TCHAR szcBLf[] = "BarLeft";
BOOL  bChgcBLf = FALSE;
//DWORD rgb_barright =   RGB(255, 255,   0);         /* bar sections in right only */
TCHAR szcBRt[] = "BarRight";
BOOL  bChgcBRt = FALSE;
//DWORD rgb_barcurrent = RGB(  0,   0, 255);         /* current pos markers in bar */
TCHAR szcBCr[] = "BarCurrent";
BOOL  bChgcBCr = FALSE;

// gwarn_text  sFW.fw_warn_text  // = RGB(255,255,255);
//  gwarn_back  sFW.fw_warn_back  // = RGB(200,30,30);
//  bChgWT      sFW.fw_bChgWT
//  bChgWB      sFW.fw_bChgWB
TCHAR szCWT[] = "Warn_Text";
TCHAR szCWB[] = "Warn_Back";

//   ghelp_text = DEF_CR_HT;    // = RGB(0,0,0)
//   ghelp_back = DEF_CR_HB;    // = RGB(255,255,255)
TCHAR szCHT[] = "Help_Text";
TCHAR szCHB[] = "Help_Back";

//TCHAR szCmp[] = "Compares";
//TCHAR szLst[] = "List%d";
BOOL  bChgCLst = FALSE;

// gszDif2Fil / bChgDF
TCHAR szDF[] = "DiffFile";
// gszListFil / bChgLF
TCHAR szLF[] = "ListFile";

// FIX20091125 - adding tags to
TCHAR szTH[] = "Head_Tag";
TCHAR szTT[] = "Tail_Tag";
// gszTagHead / gbChgTH
// gszTagTail / gbChgTT

//#define  gbNoTT      sFW.fw_NoTT    // disable tool tip
//#define  bChgTT      sFW.fw_bChgTT
TCHAR szNTT[] = "DisableToolTip";

TCHAR szCase[] = "IgnoreCase";
//   { szOpt, szCase, it_Bool, (LPTSTR)&gbIgnCase,        &bChgIgC, (PVOID)IDC_NOCASE, 0 },
TCHAR szSkipC[] = "Skip-C/C++-Comments";
//   it_Bool, (LPTSTR)&gbSkipCPP, &bChgSCPP, (PVOID)IDC_SKIPCCOMM, 0 },
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
TCHAR szSkipT[] = "Skip-Quoted-Text";
//   it_Bool, (LPTSTR)&gbSkipTxt, &bChgSTxt, (PVOID)IDC_SKIPQTXT, 0 },
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
TCHAR szIgnEL[] = "Ignore-Line-Termination";
//   it_Bool, (LPTSTR)&gbIgnEOL, &bChgIEOL, (PVOID)IDC_IGNORETERM, 0 },

// IDM_OPTADDROWNUM
TCHAR szARN[] = "Add_Row_Nums";
//#define gbAddRow	sFW.fw_bAddRow // g_ BOOL
//#define gbChgRow	sFW.fw_bChgRow

INILST  sIniLst[] = {
   { szVer, szDt,   it_Version,          szCVer,        &bChgAll, 0, 0 },  // CHANGE ALL!
   { szOpt, szLNum, it_Int,     (LPTSTR) &line_numbers, &bChgLnN, 0, 0 },
   { szOpt, szEdit, it_String,      &editor_cmdline[0], &bChgEd,  0, 0 },
   { szOpt, szZpCmd,it_String,      &szZipCmd[0],       &bChgZp,  0, 0 },
   { szOpt, szZpCmd2,it_String,     &szZipCmd2[0],      &bChgZp2, 0, 0 },
   { szOpt, szAutoZip, it_Bool, (LPTSTR) &bAutoZip,     &bChgAZp, 0, 0 },
   { szOpt, szIncl, it_Int, (LPTSTR) &outline_include,  &bChgInc, 0, 0 },
   { szOpt, szBnks, it_Bool, (LPTSTR)&ignore_blanks,    &bChgBks, (PVOID)IDC_IGNORESP, 0 },
//TCHAR szCase[] = "IgnoreCase";
   { szOpt, szCase, it_Bool, (LPTSTR)&gbIgnCase,        &bChgIgC, (PVOID)IDC_NOCASE, 0 },
//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
   { szOpt, szSkipC, // = "Skip_C/C++_Comments"; BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
   it_Bool, (LPTSTR)&gbSkipCPP, &bChgSCPP, (PVOID)IDC_SKIPCCOMM, 0 },
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
   { szOpt, szSkipT, // "Skip-Quoted-Text"; BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
   it_Bool, (LPTSTR)&gbSkipTxt, &bChgSTxt, (PVOID)IDC_SKIPQTXT, 0 },
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
   { szOpt, szIgnEL, // "Ignore-Line-Termination"; BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11
   it_Bool, (LPTSTR)&gbIgnEOL, &bChgIEOL, (PVOID)IDC_IGNORETERM, 0 },

   { szOpt, szExa,   // = "Exact";  // IDM_OPTEXACT
   it_Bool, (LPTSTR) &gbExact, &bChgExa, 0, 0 },

   { szOpt, szPic,  it_Bool, (LPTSTR)&picture_mode,     &bChgPic, 0, 0 },
   { szOpt, szLft,  it_String, gszLefti,                &bChgLf,  0, 0 },
   { szOpt, szRit,  it_String, gszRitei,                &bChgRt,  0, 0 },
#ifndef USE_GLOBAL_RECURSIVE
   { szOpt, szRec,  it_Bool, (LPTSTR) &gbRecur,         &bChgRec, 0, 0 },
#endif // #ifndef USE_GLOBAL_RECURSIVE
   { szOut, szGOut, it_WinSize, (LPTSTR)&g_sWPO,        &bChgOut, (PVOID)&gbGotOut, 0 },
   { szExp, szGExp, it_WinSize, (LPTSTR)&g_sWPE,        &bChgExp, (PVOID)&gbGotExp, 0 },
   { szWin, szZm,   it_Bool,   (LPTSTR)&IsZoom,         &bChgZm,  0, 0 },
   { szWin, szIc,   it_Bool,   (LPTSTR)&IsIcon,         &bChgIc,  0, 0 },
   { szWin, szSz,   it_Rect,   (LPTSTR)&grcSize,        &bChgSz,  (PVOID)&bSzValid, 0 },
   { szOpt, szCT,   it_String, &gszCopyTo[0],           &bChgCT,  0, 0 },
//   { szOpt, szVri,  it_Bool, (LPTSTR)&gbVerify,         &bChgVer, 0, 0 },
   // IDM_OPTEXACT
   { szOpt, szExa,  it_Bool, (LPTSTR)&gbExact,          &bChgExa, 0, 0 },
   { szOpt, szBXcl, it_Bool, (LPTSTR)&gbExclude,        &bChgExcl,0, 0 },
   { szOpt, szXSel, it_Int, (LPTSTR)&giCurXSel,         &bChgXSel,0, 0 },
   // IDM_WARNING
   { szOpt, szWn,   it_Bool, (LPTSTR)&gbWarn,           &bChgWn,  0, 0 },
   // gdwCpyOpts / bChgCO = "FileCopyOptions"
   { szOpt, szCOp,  it_DWord,(LPTSTR)&gdwCpyOpts,       &bChgCO,  0, 0 },
   { szOpt, szVF,   it_DWord,(LPTSTR)&gdwVerFlag,       &bChgVF,  0, 0 },
   // gdwDiffOpts / bChgDO
   { szOpt, szDO,   it_DWord,(LPTSTR)&gdwDiffOpts,      &bChgDO,  0, 0 },
   { szOpt, szWW,   it_DWord,(LPTSTR)&gdwWrapWid,       &bChgWW,  0, 0 },
   // gdwFileOpts - write file list options / bChgFO = "FileListInclude";
   { szOpt, szFO,   it_DWord,(LPTSTR)&gdwFileOpts,      &bChgFO,  0, 0 },
   // gszDif2Fil / bChgDF
   { szOpt, szDF,   it_String, &gszDif2Fil[0],          &bChgDF,  0, 0 },
   // gszListFil / bChgLF
   { szOpt, szLF,   it_String, &gszListFil[0],          &bChgLF,  0, 0 },
   // FIX20091125 - adding tags to
   { szOpt, szTH,   it_String, &gszTagHead[0],          &gbChgTH, 0, 0 },
   { szOpt, szTT,   it_String, &gszTagTail[0],          &gbChgTT, 0, 0 },
   
   { szOpt, szNTT,  it_Bool, (LPTSTR)&gbNoTT,           &bChgTT,  0, 0 },
// IDM_DIR - put up directories DIALOG
   { szOpt, szAuSel,it_Bool, (LPTSTR)&gbAutoSel,        &bChgASel,0, 0 },
   //   case IDM_NONRS: // this is a display toggle
//   { szOpt, szShwNums,it_Bool,(LPTSTR)&gbShowNums,      &gbChgSN, 0, 0 },
   //   case IDM_LNRS: case IDM_RNRS:
   { szOpt, szUseRt,  it_Bool,(LPTSTR)&gbUseRight,      &gbChgURt, 0, 0 },
// IDM_OPTIGNOREDT
   { szOpt, szIgnDT,  it_Bool,(LPTSTR)&gbIgnDT,         &gbChgIDT, 0, 0 },
   { szOpt, szIgnDT2, it_Bool,(LPTSTR)&gbIgnDT2,        &gbChgIDT2, 0, 0 },
// IDM_OPTADDROW
//   { szOpt, szARN,    it_Bool,(LPTSTR)&gbAddRow,        &gbChgRow, 0, 0 },

// IDM_BOTHFILES, IDM_LONLY, IDM_RONLY
   { szOpt, szEM,     it_DWord,(LPTSTR)&expand_mode,    &gbChgEM,  0, 0 },

   { szOpt, szDelOn, it_Bool, (LPTSTR)&gbDelOn,         &gbChgDel, 0, 0 },
//#define gbSimple	sFW.fw_bSimple // g BOOL
//#define gbChgSim	sFW.fw_bChgSim // g BOOL
//TCHAR szSimp[] = "Simple_Display";
   { szOpt, szSimp, it_Bool, (LPTSTR)&gbSimple,         &gbChgSim, 0, 0 },

   // section [Colours]
   { szClr, szcHi,  it_Color, (LPTSTR)&rgb_outlinehi,    &bChgcHi, 0, 0 },
   { szOpt, szAddS, it_Bool, (LPTSTR)&gbAddS,           &gbChgAddS, 0, 0 },

   /* expand view */
   { szClr, szcELF, it_Color, (LPTSTR)&rgb_leftfore,     &bChgcELF,0, 0 },
   { szClr, szcELB, it_Color, (LPTSTR)&rgb_leftback,     &bChgcELB,0, 0 },
   { szClr, szcERF, it_Color, (LPTSTR)&rgb_rightfore,    &bChgcERF,0, 0 },
   { szClr, szcERB, it_Color, (LPTSTR)&rgb_rightback,    &bChgcERB,0, 0 },
   /* moved lines */
   { szClr, szcMLF, it_Color, (LPTSTR)&rgb_mleftfore,    &bChgcMLF,0, 0 },
   { szClr, szcMLB, it_Color, (LPTSTR)&rgb_mleftback,    &bChgcMLB,0, 0 },
   { szClr, szcMRF, it_Color, (LPTSTR)&rgb_mrightfore,   &bChgcMRF,0, 0 },
   { szClr, szcMRB, it_Color, (LPTSTR)&rgb_mrightback,   &bChgcMRB,0, 0 },
   /* bar window */
   { szClr, szcBLf, it_Color, (LPTSTR)&rgb_barleft,      &bChgcBLf,0, 0 },
   { szClr, szcBRt, it_Color, (LPTSTR)&rgb_barright,     &bChgcBRt,0, 0 },
   { szClr, szcBCr, it_Color, (LPTSTR)&rgb_barcurrent,   &bChgcBCr,0, 0 },
   { szClr, szCWT,  it_Color, (LPTSTR)&gwarn_text,       &bChgWT,  0, 0 },
   { szClr, szCWB,  it_Color, (LPTSTR)&gwarn_back,       &bChgWB,  0, 0 },
   { szClr, szCHT,  it_Color, (LPTSTR)&ghelp_text,       &bChgHT,  0, 0 },
   { szClr, szCHB,  it_Color, (LPTSTR)&ghelp_back,       &bChgHB,  0, 0 },

   // simple text list
   { szCmp, szLst,  it_SList, (LPTSTR)&gsCompList,       &bChgCLst,0, MXCMPLST },
   { szDif, szLst,  it_SList, (LPTSTR)&gsDiffList,       &bChgDLst,0, MXCMPLST },
   { szFil, szLst,  it_SList, (LPTSTR)&gsFileList,       &bChgFLst,0, MXCMPLST },
   { szZipL,szLst,  it_SList, (LPTSTR)&gsZipList,        &bChgZLst,0, MXCMPLST },

   // backup zipup automatically on start up
   { szLastZp, szCmd, it_String, &g_sZipCmd.szCmd[0],  &gbChgZp2[0],0,0 },
   { szLastZp, szSws, it_String, &g_sZipCmd.szSws[0],  &gbChgZp2[1],0,0 },
   { szLastZp, szZip, it_String, &g_sZipCmd.szZip[0],  &gbChgZp2[2],0,0 },
   { szLastZp, szInp, it_String, &g_sZipCmd.szInp[0],  &gbChgZp2[3],0,0 },
   { szLastZp, szEnv, it_String, &g_sZipCmd.szEnv[0],  &gbChgZp2[4],0,0 },
   { szLastZp, szCmD, it_String, &g_sZipCmd.szCmp[0],  &gbChgZp2[ZP_DIRCMP],0,0 },
   // ***************************************

#ifdef ADD_LIST_VIEW
// TCHAR szLvw[] = "[ListView]";   // some LISTVIEW options
   { szLvw, szLV50, it_Bool, (LPTSTR)&g_bIn5050,      &g_bChg5050,   0, 0 },
   { szLvw, szHLV,  it_Bool, (LPTSTR)&g_bUsrLVOff,       &g_bChgLVOff,  0, 0 },
   { szLvw, szLVSEL,it_Bool, (LPTSTR)&glv_iAddTrack,  &glv_iChgTrack, 0, 0 },
   { szLvw, szLVGRD,it_Bool, (LPTSTR)&glv_iAddGrid,   &glv_iChgGrid, 0, 0 },
   { szLvw, szPCcl, it_DWord,(LPTSTR)&g_dwPercent,    &g_bChgPCnt,   0, 0 },
#endif // #ifdef ADD_LIST_VIEW

    // ID_VIEW_NOEXCLUDES - section [Exclude]
   { szXcl, szNoEx, it_Bool, (PTSTR)&bNoExcludes,     &bChgNoExcl,   0, 0 },

   // ===================================================================
   { 0,     0,      it_None,    0,     0,        0, 0 }  // END OF TABLE
   // ===================================================================
};

BOOL  SetChgAll( BOOL bChg )
{
   BOOL  bRet = bChgAll;
   bChgAll = bChg;
   return bRet;
}
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SetINIFile
// Return type: VOID 
// Argument   : LPTSTR lpini
// Description: Establish the INI file name. In current work directory
//              when DEBUG, else placed in the RUNTIME directory.
// FIX20010522 - ALWAYS place it in the RUNTIME folder!!!
///////////////////////////////////////////////////////////////////////////////
VOID  SetINIFile( LPTSTR lpini )
{
   *lpini = 0;
   // GetModulePath( lpini );    // does   GetModuleFileName( NULL, lpini, 256 );
   GetAppData(lpini);
   strcat(lpini, g_szDefIni);
}

#define	GetStg( a, b )	\
	GetPrivateProfileString( a, b, &szBlk[0], lpb, 256, lpini )


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : IsYes
// Return type: BOOL 
// Argument   : LPTSTR lpb
// Description: Return TRUE if the string given is "Yes" OR "On"
///////////////////////////////////////////////////////////////////////////////
BOOL  IsYes( LPTSTR lpb )
{
   BOOL  bRet = FALSE;
   if( ( strcmpi(lpb, "YES") == 0 ) ||
       ( strcmpi(lpb, "ON" ) == 0 ) )
       bRet = TRUE;
   return bRet;
}
BOOL  IsNo( LPTSTR lpb )
{
   BOOL  bRet = FALSE;
   if( ( strcmpi(lpb, "NO" ) == 0 ) ||
       ( strcmpi(lpb, "OFF") == 0 ) )
       bRet = TRUE;
   return bRet;
}

BOOL  ValidShowCmd( UINT ui )
{
   BOOL  bRet = FALSE;
   if( ( ui == SW_HIDE ) ||   //Hides the window and activates another window. 
       ( ui == SW_MAXIMIZE ) ||  //Maximizes the specified window. 
       ( ui == SW_MINIMIZE ) ||  //Minimizes the specified window and activates the next top-level window in the Z order. 
       ( ui == SW_RESTORE ) ||   //Activates and displays the window. If the window is minimized or maximized, the system restores it to its original size and position. An application should specify this flag when restoring a minimized window. 
       ( ui == SW_SHOW ) || //Activates the window and displays it in its current size and position.  
       ( ui == SW_SHOWMAXIMIZED ) || //Activates the window and displays it as a maximized window. 
       ( ui == SW_SHOWMINIMIZED ) || //Activates the window and displays it as a minimized window. 
       ( ui == SW_SHOWMINNOACTIVE ) ||  //Displays the window as a minimized window. 
       ( ui == SW_SHOWNA ) || //Displays the window in its current size and position. 
       ( ui == SW_SHOWNOACTIVATE ) ||  //Displays a window in its most recent size and position. 
       ( ui == SW_SHOWNORMAL ) )
       bRet = TRUE;
   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : GotWP
// Return type: BOOL 
// Arguments  : LPTSTR pSect
//            : LPTSTR pDef
//            : LPTSTR lpb
//            : LPTSTR lpini
// Description: Read in a special BLOCK of window palcement item from
//              the INI file. This is in its own [section].
///////////////////////////////////////////////////////////////////////////////
BOOL  GotWP( LPTSTR pSect, LPTSTR pDef, LPTSTR lpb, LPTSTR lpini )
{
   BOOL  bRet = FALSE;
   WINDOWPLACEMENT   wp;
   WINDOWPLACEMENT * pwp = (WINDOWPLACEMENT *)pDef;
   if( !pwp )
      return FALSE;

   *lpb = 0;
   GetStg( pSect, szShow ); // = "ShowCmd";
   if( *lpb == 0 )
      return FALSE;
   wp.showCmd = atoi(lpb);
   if( !ValidShowCmd( wp.showCmd ) )
      return FALSE;

   *lpb = 0;
   GetStg( pSect, szMaxX );
   if( *lpb == 0 )
      return FALSE;
   wp.ptMaxPosition.x = atoi(lpb);
   *lpb = 0;
   GetStg( pSect, szMaxY );
   if( *lpb == 0 )
      return FALSE;
   wp.ptMaxPosition.y = atoi(lpb);

   *lpb = 0;
   GetStg( pSect, szMinX );
   if( *lpb == 0 )
      return FALSE;
   wp.ptMinPosition.x = atoi(lpb);
   *lpb = 0;
   GetStg( pSect, szMinY );
   if( *lpb == 0 )
      return FALSE;
   wp.ptMinPosition.y = atoi(lpb);

   *lpb = 0;
   GetStg( pSect, szLeft );   // = "NormLeft";
   if( *lpb == 0 )
      return FALSE;
   wp.rcNormalPosition.left = atoi(lpb);
   *lpb = 0;
   GetStg( pSect, szTop ); // = "NormTop";
   if( *lpb == 0 )
      return FALSE;
   wp.rcNormalPosition.top = atoi(lpb);
   *lpb = 0;
   GetStg( pSect, szRite );   // = "NormRight";
   if( *lpb == 0 )
      return FALSE;
   wp.rcNormalPosition.right = atoi(lpb);
   *lpb = 0;
   GetStg( pSect, szBot ); //  = "NormBottom";
   if( *lpb == 0 )
      return FALSE;
   wp.rcNormalPosition.bottom = atoi(lpb);

   wp.flags = 0;
   wp.length = sizeof(WINDOWPLACEMENT);

   memcpy( pwp, &wp, sizeof(WINDOWPLACEMENT) );
   bRet = TRUE;
   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : IsRectOk
// Return type: BOOL 
// Argument   : PRECT pr
// Description: A rough verification that a RECTANGLE 'looks' OK
//              NOTE: Thre must be SOME value, and they must all
// be positive (or zero, but not all).
///////////////////////////////////////////////////////////////////////////////
BOOL  IsRectOk( PRECT pr )
{
   BOOL  bRet = FALSE;
   if( pr->left || pr->top || pr->right || pr->bottom )
   {
      // good start - some value is NOT zero
      // here I am ONLY accepting POSITIVE values
      if( ( pr->left >= 0 ) &&
          ( pr->top >= 0 ) &&
          ( pr->right >= 0 ) &&
          ( pr->bottom >= 0 ) )
      {
         bRet = TRUE;
      }
      else if(( pr->right  > 0 ) &&
              ( pr->bottom > 0 ) )
      {
         // this is a POSSIBLE candidate,
         // but limit neg x,y to say 5 pixels
         if(( pr->left > -5 ) &&
            ( pr->top  > -5 ) )
         {
            bRet = TRUE;
         }
      }
   }
   return bRet;
}

#define  VALIDLN(a)  ( ( a == IDM_LNRS ) || ( a == IDM_RNRS ) || ( a == IDM_NONRS ) )

#define     MAXXLC      16

#define  xf_Deleted     0x00000001
#define  xf_GotWild     0x00000002

#define  xf_Found       0x80000000

//typedef  struct   tagXLST {
//   LIST_ENTRY  x_List;  // first LIST entry
//   DWORD       x_dwFlag;   // flag of item
//   TCHAR       x_szBody[264]; //	SplitExt2( lpb1, lpe1, lp1 );
//   TCHAR       x_szExt[264];  // SplitExt2( lpb2, lpe2, lp2 );
//   TCHAR       x_szStg[1]; // string
//}XLST, * PXLST; // FIX20051204 - this ALSO used as -I input LIST

LPTSTR   GetxStg( PLE pN )
{
   PXLST pxl = (PXLST)pN;
   LPTSTR  lps = &pxl->x_szStg[0];
   return lps;
}

BOOL  IsXDeleted( PLE pN )
{
   BOOL  bRet = FALSE;
   PXLST pxl = (PXLST)pN;
   if( ( pxl ) &&
       ( pxl->x_dwFlag & xf_Deleted ) )
       bRet = TRUE;
   return bRet;
}

// ============================================
// PVOID Add2StgList( PLE pH, LPTSTR lpb )
// Unconditionally ADD this string to this LIST
// ============================================
PVOID Add2StgList( PLE pH, LPTSTR lpb )
{
   LPTSTR   lps;
   INT      i;
   PXLST    pxl;
   i = (strlen(lpb) + sizeof(XLST));
   pxl = (PXLST)MALLOC(i);
   if(pxl)
   {
   	LPTSTR   lpb2 = pxl->x_szBody;  // user input masks, like  zlib*;*.obj;... etc
      LPTSTR   lpe2 = pxl->x_szExt;

      ZeroMemory( pxl, i );

      lps = &pxl->x_szStg[0];
      strcpy(lps, lpb);

      // FIX20021007 - Do this BEFORE we get to scan_dir, where time is scarce
      if( GotWild(lps) )
         pxl->x_dwFlag |= xf_GotWild;  // signal it contains * or ? or both

      if( strchr(lps,'.' ) )  // if we have a FULL STOP in the input
         SplitExt2( lpb2, lpe2, lps ); // then split it into two
      else
         strcpy( lpb2, lps ); // if NO full stop, then NO EXTENT, just 'body'

      // FIX20021007 - refinement of the wild compare
      if(( lpe2[0] ==  0                 ) &&
         ( strchr( lps, '.' ) == 0       ) &&   // no extent given
         ( lps[(strlen(lps) - 1)] == '*' ) )    // and last is an asterix
      {
         strcpy(lpe2,"*"); // add to extent
      }

      InsertTailList(pH,(PLE)pxl);
   }
   return pxl;
}

// ================================================
// PVOID  Add2Exclude( PLE pH, LPTSTR lpb )
// Add entry to an EXCLUDE list
// PLE pH    = Head of LIST
// LPTST lpb = New string to add
// Do NOT add it if this string is already there
// ================================================
PVOID  Add2Exclude( PLE pH, LPTSTR lpb )
{
   INT   i = strlen(lpb);
   PXLST pxl = 0;
   PLE         pN;
   LPTSTR      lps;
   if(!i)
      return NULL;

   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      lps = &pxl->x_szStg[0];
      if( strcmpi( lps, lpb ) == 0 )
      {
         pxl->x_dwFlag &= ~(xf_Deleted);
         return NULL;   // already in LIST
      }
   }

   pxl = Add2StgList( pH, lpb );

   return pxl;
}

#define  TOUP(a)     ( ISLOWER(a) ? ( a & 0x5f ) : a )

BOOL  Match2XList( PLE pH, LPTSTR lpf, DWORD ftyp )
{
   BOOL     bRet = FALSE;  // assume NOT in the LIST
   PLE      pN;
   LPTSTR   lps;
   PXLST    pxl;

   if( bNoExcludes ) // ID_VIEW_NOEXCLUDES
       return bRet; // then NOT IN LIST!

   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      lps = &pxl->x_szStg[0];
#ifdef   REV_TO_OLD
         if( MatchFiles( lpf, lps ) )
         {
            bRet = TRUE;   // *** IT IS IN THE LIST ***
            break;
         }
#else    // !REV_TO_OLD
      if( pxl->x_dwFlag & xf_GotWild )  // signal it contains * or ? or both
      {
//         if( MatchWild( lpf, lps ) )
         if( MatchWild2( lpf, pxl->x_szBody, pxl->x_szExt ) )
         {
            bRet = TRUE;   // *** IT IS IN THE LIST ***
            break;
         }
      }
      else
      {
			if( strcmpi( lpf, lps ) == 0 )
         {
				bRet = TRUE;   // *** IT IS IN THE LIST ***
            break;
         }
      }
#endif   // #ifdef   REV_TO_OLD
   }
   return bRet;
}

BOOL  MatchesExclude( LPTSTR lpf, DWORD ftyp )
{
   BOOL     bRet = FALSE;
   PLE      pH;
   INT      i = giCurXSel;    // get current SELECTION
   if( i && ( i <= MXXLSTS ) )
      i--;
   else
      i = 0;
   pH = &gsExclLists[i];   // select the correct user list of EXCLUDES
   return( Match2XList( pH, lpf, ftyp ) );
}

BOOL  CopyXList2( PLE pCopy, PLE pH )
{
   BOOL     bRet = FALSE;
   PLE      pN;
   LPTSTR   lps;
   PXLST    pxl, pxln;
   INT      icnt = 0;
   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      lps = &pxl->x_szStg[0];
      pxln = Add2Exclude( pCopy, lps );
      if( pxln )
      {
         pxln->x_dwFlag = pxl->x_dwFlag;
         bRet++;
      }
      icnt++;
   }
   if(icnt == 0)
      bRet = TRUE;   // this is also OK
   return bRet;
}


BOOL  CopyXList_NOT_USED( PLE pCopy )
{
   PLE      pH = &gsExclLists[0];
   BOOL     bRet = CopyXList2( pCopy, pH );
   return bRet;
}

BOOL  CompareXList_NOT_USED( PLE pCopy )
{
   BOOL     bRet = FALSE;
   PLE      pH = &gsExclLists[0];
   PLE      pN, pNC;
   LPTSTR   lps1, lps2;
   PXLST    pxl, pxln;
   INT      icnt = 0;

   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      lps1 = &pxl->x_szStg[0];
      Traverse_List( pCopy, pNC )
      {
         pxln = (PXLST)pNC;
         if( !(pxln->x_dwFlag & xf_Found) )
         {
            lps2 = &pxln->x_szStg[0];
            if( strcmpi(lps1,lps2) == 0 )
            {
               pxl->x_dwFlag |= xf_Found;
               pxln->x_dwFlag |= xf_Found;
               break;
            }
         }
      }
      if( pxl->x_dwFlag & xf_Found )
      {
         if( ( pxl->x_dwFlag & xf_Deleted ) != ( pxln->x_dwFlag & xf_Deleted ) )
         {
            icnt++;
         }
      }
   }

   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      if( pxl->x_dwFlag & xf_Found )
         pxl->x_dwFlag &= !(xf_Found);
      else
         icnt++;  // not found in list
   }

   Traverse_List( pCopy, pNC )
   {
      pxln = (PXLST)pNC;
      if( pxln->x_dwFlag & xf_Found )
         pxln->x_dwFlag &= !(xf_Found);
      else
         icnt++;
   }
   bRet = icnt;
   return bRet;
}

VOID  ReadDc4wINI( VOID )
{
   LPTSTR   lpini = &g_szIni[0];
   SetINIFile( lpini );
   ReadINI( lpini );
}

INT Add_List_2_List( PLE pH, LPTSTR lps )
{
   INT   icnt  = 0;
   LPTSTR lpb  = lps;
   INT    iPos = InStr(lpb,";");
   while(iPos)
   {
      lpb[iPos-1] = 0;
      if( *lpb ) {
         if( Add2Exclude( pH, lpb ) )
            icnt++;
      }
      lpb = &lpb[iPos];
      iPos = InStr(lpb,";");
   }
   
   if( *lpb ) {
      if( Add2Exclude( pH, lpb ) )
         icnt++;
   }

   return icnt;

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : ReadINI
// Return type: VOID 
// Argument   : LPTSTR lpini - name of file to use
// Description: Read the "preferences" from an INI file
// Do any 'reality checks in -
VOID  Post_ReadINI( VOID );
///////////////////////////////////////////////////////////////////////////////
VOID  ReadINI( LPTSTR lpini )
{
   PINILST  plst = &sIniLst[0];
   LPTSTR   lpb = &gszTmpBuf[0];
   DWORD    dwt;
   LPTSTR   pSect, pItem, pDef;
   PBOOL    pb, pbd;
   PINT     pi;
   INT      i, icnt;
   PRECT    pr, pr1;
   DWORD    dwo = gdwFileOpts;   // get value BEFORE read

   icnt = 0;
   while( ( dwt = plst->i_Type ) != it_None )
   {
      pSect = plst->i_Sect;   // pointer to [section] name
      pItem = plst->i_Item;   // pointer to itme in section
      pDef  = plst->i_Deft;   // pointer to destination
      pb    = plst->i_Chg;    // pointer to CHANGE flag
      *lpb = 0;
      if( dwt == it_SList )
      {
         // must prepare multiple fetch items
         for( i = 0; i < (INT)plst->i_Res1; i++ )
         {
            sprintf(lpb, pItem, (i+1));
            GetStg( pSect, lpb );
            dwt = strlen(lpb);
            if(dwt)
            {
               PLE   ple = (PLE)MALLOC( sizeof(LIST_ENTRY) + dwt + 1 );
               if(ple)
               {
                  strcpy( (LPTSTR)((PLE)ple + 1), lpb );
                  InsertTailList( (PLE)pDef, ple);
               }
            }
         }
         // put a value in lpb, so change will NOT be set
         sprintf(lpb, pItem, (i+1));
      }
      else
      {
         GetStg( pSect, pItem );
      }
      if( *lpb )
      {
         switch(dwt)
         {
         case it_Version:
            if( strcmp( pDef, lpb ) )
               *pb = TRUE;
            break;
         case it_String:
            if( strcmp( pDef, lpb ) )
               strcpy( pDef, lpb );
            break;
         case it_Int:   // also doubles as a DWORD in WIN32
            pi = (PINT)pDef;
            i = atoi(lpb);
            *pi = i;
            break;
         case it_Bool:
            pbd = (PBOOL)pDef;
            if( IsYes(lpb) )
               *pbd = TRUE;
            else if( IsNo(lpb) )
               *pbd = FALSE;
            else
               *pb = TRUE;
            break;
         case it_WinSize:     // special WINDOWPLACEMENT
            if( ( IsYes(lpb) ) &&
                ( GotWP( pSect, pDef, lpb, lpini ) ) )
            {
               // only if SAVED is yes, AND then success
               pb = (PBOOL) plst->i_Void;
               *pb = TRUE; // set that we have a (valid!) WINDOWPLACEMENT
            }
            else
               *pb = TRUE;
            break;
         case it_Rect:
            pr = (PRECT) &lpb[ strlen(lpb) + 2 ];
            pr->left = 0;
            pr->top = 0;
            pr->right = 0;
            pr->bottom = 0;
            if( ( sscanf(lpb, "%d,%d,%d,%d", &pr->left, &pr->top, &pr->right, &pr->bottom ) == 4 ) &&
                ( IsRectOk( pr ) ) )
            {
               pr1 = (PRECT)pDef;
               pr1->left = pr->left;
               pr1->top  = pr->top;
               pr1->right = pr->right;
               pr1->bottom = pr->bottom;
               pb = (PBOOL) plst->i_Void;
               if(pb)
                  *pb = TRUE;
            }
            break;
         case it_Color:
            pr = (PRECT) &lpb[ strlen(lpb) + 2 ];
            pr->left = 0;
            pr->top = 0;
            pr->right = 0;
            pr->bottom = 0;
            if( sscanf(lpb, "%d,%d,%d", &pr->left, &pr->top, &pr->right ) == 3 )
            {
               COLORREF * pcr = (COLORREF *)pDef;
               *pcr = RGB( pr->left, pr->top, pr->right );
            }
            break;
         case it_SList:
            // it has all been done
            break;
         }
      }
      else
      {
         *pb = TRUE;    // can only SET change
         if( icnt == 0 )
            sprtf( "ADVICE: bChgAll has been set!"MEOR );

      }

      plst++;
      icnt++;
   }

   // READ EXCLUDE LISTS - MXXLSTS set of exclude files and masks
   // each of the form '*.bak;*.old;...'
   {
      LPTSTR   lpb2 = &gszTmpBuf2[0];
      PLE      pH;
      INT      iPos; //, iCnt;
      pSect = szXcl; // [Exclude] section
      for( i = 0; i < MXXLSTS; i++ )
      {
         sprintf( lpb2, szLst, (i + 1) );
         *lpb = 0;
         GetStg( pSect, lpb2 );
         if( *lpb )
         {
            pH   = &gsExclLists[i];    // select list
            iPos = InStr(lpb,";");
            while(iPos)
            {
               lpb[iPos-1] = 0;
               if( *lpb )
                  Add2Exclude( pH, lpb );
               lpb = &lpb[iPos];
               iPos = InStr(lpb,";");
            }
            if( *lpb )
               Add2Exclude( pH, lpb );
         }
      }

      pSect = szXcl; // [Exclude] section
      //pH = &gsXFileList;   // the EXCLUDE FILE LIST
      pH = &gsXFileIni;   // the EXCLUDE FILE LIST
      lpb = &gszTmpBuf[0]; // return lpb to its correct value
      *lpb = 0;
      GetStg( pSect, "FileList" );
      if(*lpb)
         Add_List_2_List( pH, lpb );
      //pH = &gsXDirsList; // = EXCLUDE these DIRECTORIES -xd:Scenery
      pH = &gsXDirsIni; // = EXCLUDE these DIRECTORIES -xd:Scenery
      *lpb = 0;
      GetStg( pSect, "DirList" );
      if(*lpb)
         Add_List_2_List( pH, lpb );

   }

   // Look for OTHER inits that should be done AFTER a READ
   // *****************************************************
   Post_ReadINI();
   if( dwo != gdwFileOpts )
   {
      sprtf( "gdwFileOpts changed from %d to %d."MEOR, dwo, gdwFileOpts );
   }
   // *****************************************************
}  // end ReadINI( lpini )


VOID  Post_ReadINI( VOID )
{
   PBOOL    pb;
   LPTSTR   lpb2 = &gszTmpBuf2[0];  // get a temp buffer
   // any VALIDATIONS required - you never know WHAT you can read from an INI file
   // ************************
   if( ( gdwDiffOpts & EXP_ALL ) == 0 )
   {
      gdwDiffOpts = INC_ALLXSAME;   // set the DEFAULT
      bChgDO = TRUE;
   }
   // and for moment, these are clamped together, but could be separted
   if( gdwDiffOpts & INC_ALLMOVE )
      gdwDiffOpts |= INC_ALLMOVE;   // ensure BOTH are ON

#ifndef   NEWCOLS2 // add or subtract columns with ease
   if( gdwDiffOpts & INCLUDE_LINENUMS )
      gbShowNums   = TRUE;
   else
      gbShowNums   = FALSE;
#endif   // #ifndef   NEWCOLS2 // add or subtract columns with ease

   if( !VALIDLN( line_numbers ) )
      line_numbers = IDM_LNRS;   // set default

   if(bSzValid)
   {
      // it appears the SIZE is valid
      UINT              ui;
      WINDOWPLACEMENT * pwp;
      pwp = &g_sWPO;
      pb  = &bChgOut;
      ui = pwp->showCmd;
      //if( ( ui == SW_RESTORE ) ||   //Activates and displays the window. If the window is minimized or maximized, the system restores it to its original size and position. An application should specify this flag when restoring a minimized window. 
      //    ( ui == SW_SHOW ) || //Activates the window and displays it in its current size and position.
      //    ( ui == SW_SHOWNA ) || //Displays the window in its current size and position. 
      //    ( ui == SW_SHOWNOACTIVATE ) ||  //Displays a window in its most recent size and position. 
      //    ( ui == SW_SHOWNORMAL ) )
      {
         if( pwp->rcNormalPosition.top != grcSize.top )
         {
            pwp->rcNormalPosition.top = grcSize.top;
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.left != grcSize.left )
         {
            pwp->rcNormalPosition.left = grcSize.left;
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.right != (grcSize.left + grcSize.right) )
         {
            pwp->rcNormalPosition.right = (grcSize.left + grcSize.right);
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.bottom != (grcSize.top + grcSize.bottom) )
         {
            pwp->rcNormalPosition.bottom = (grcSize.top + grcSize.bottom);
            *pb = TRUE;
         }
      }
      pwp = &g_sWPE;
      ui = pwp->showCmd;
      pb  = &bChgExp;
      //if( ( ui == SW_RESTORE ) ||   //Activates and displays the window. If the window is minimized or maximized, the system restores it to its original size and position. An application should specify this flag when restoring a minimized window. 
      //    ( ui == SW_SHOW ) || //Activates the window and displays it in its current size and position.
      //    ( ui == SW_SHOWNA ) || //Displays the window in its current size and position. 
      //    ( ui == SW_SHOWNOACTIVATE ) ||  //Displays a window in its most recent size and position. 
      //    ( ui == SW_SHOWNORMAL ) )
      {
         if( pwp->rcNormalPosition.top != grcSize.top )
         {
            pwp->rcNormalPosition.top = grcSize.top;
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.left != grcSize.left )
         {
            pwp->rcNormalPosition.left = grcSize.left;
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.right != (grcSize.left + grcSize.right) )
         {
            pwp->rcNormalPosition.right = (grcSize.left + grcSize.right);
            *pb = TRUE;
         }
         if( pwp->rcNormalPosition.bottom != (grcSize.top + grcSize.bottom) )
         {
            pwp->rcNormalPosition.bottom = (grcSize.top + grcSize.bottom);
            *pb = TRUE;
         }
      }
   }

   // MORE POST INI READ SANITY CHECKS
   // ================================
   if( ( outline_include & INCLUDE_BASE ) == 0 )
   {
      //if( INCLUDE_DIFFER then MUST have one or both of Newer or Older
      outline_include |= INCLUDE_MINO;
   }
   if( outline_include & INCLUDE_DIFFER )
   {
      if( ( outline_include & (INCLUDE_NEWER|INCLUDE_OLDER) ) == 0 )
         outline_include |= INCLUDE_NEWER;
//         outline_include |= (INCLUDE_NEWER|INCLUDE_OLDER);
   }
   else
   {
         outline_include &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
   }

   if( gdwCpyOpts == 0 )
   {
      gdwCpyOpts  = DEF_COPY_SET;
      bChgCO     = TRUE;       // set change for INI update
   }
   else if( gdwCpyOpts & INCLUDE_DIFFER )   // if master bit ON
   {
      // if none of the two bit newer / older are ON
      if( ( gdwCpyOpts & (INCLUDE_NEWER|INCLUDE_OLDER) ) == 0 )
      {
         gdwCpyOpts |= INCLUDE_NEWER;  // then at least ADD newer
         bChgCO     = TRUE;       // set change for INI update
//         outline_include |= (INCLUDE_NEWER|INCLUDE_OLDER);
      }
   }
   else if( gdwCpyOpts & (INCLUDE_NEWER|INCLUDE_OLDER) )
   {
         gdwCpyOpts &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
         bChgCO     = TRUE;       // set change for INI update
   }

// global   TCHAR editor_cmdline[]; // [264] = { "notepad %p" };  /* editor cmdline */
   if( editor_cmdline[0] == 0 ) // [264] =
   {
      strcpy(editor_cmdline, "notepad %p" ); /* editor cmdline */
      bChgEd = TRUE;
   }

   if( szZipCmd[0] == 0 )
   {
       strcpy(szZipCmd, DEF_ZIP_CMD );  // like say "Zip8 -a TEMPZ001.zip @tempz001.lst");
       bChgZp = TRUE;
   }
   if( szZipCmd2[0] == 0 )
   {
       strcpy(szZipCmd2, DEF_ZIP_CMD2 );  // like say "Zip8 -a -x@c:\windows\temp\tempexcl.txt TEMPZ001.zip @tempz001.lst");
       bChgZp2 = TRUE;
   }

   //ToggleBool( &gbAddRow, &gbChgRow, ((gdwFileOpts & INCLUDE_LINENUMS) ? TRUE : FALSE) );
   // or maybe better or worse - when two things say the same subject
   //ToggleBit( &gdwFileOpts, INCLUDE_LINENUMS, &bChgFO, gbAddRow );

   if((expand_mode != IDM_RONLY) &&
      (expand_mode != IDM_LONLY) &&
      (expand_mode != IDM_BOTHFILES) )
   {
      expand_mode = IDM_BOTHFILES;
      gbChgEM     = TRUE;
   }

   // ENSURE THERE IS SOMETHING IN THE EXCLUDE LISTS
   // ==============================================
   {
      int iPos = 0;
      int i, iCnt, iFnd, iBlank;
      PLE pH;

      iFnd = 0;
      iBlank = 0;
      for( i = 0; i < MXXLSTS; i++ )
      {
         iCnt = 0;
         pH = &gsExclLists[i];
         ListCount2(pH,&iCnt);
         iPos += iCnt;
         if(iCnt) {
            *lpb2 = 0;
            Get_Excl_List( lpb2, pH );
            if( stricmp( lpb2, szMustHave ) == 0 ) { // = "*.obj;*.ilk;*.suo;..."
               iFnd++;
            }
         } else if( !iBlank ) {
            iBlank = (i + 1); // keep (offset + 1) of first BLANK list
         }
      }
      if( !iPos ) {  // if NO EXTRIES
         pH = &gsExclLists[0];
         Add2Exclude( pH, "*.bak");
         Add2Exclude( pH, "*.old");
         Add2Exclude( pH, "temp*.*");
         Add2Exclude( pH, "*.obj");
         Add2Exclude( pH, "*.ncb");
         Add2Exclude( pH, "*.opt");
         bChgXLst = TRUE;
      }
      // 20080415 - Must have exclude list
      // TCHAR szMustHave[] = "*.obj;*.ilk;*.suo;*.sln;*.vcproj;*.ncb;*.user;*.lib;..."
      // If there is a BLANK LIST, and this exact entry is not there
      if( iBlank && !iFnd ) {
         // we have a BLANK, and our string was NOT found
         PTSTR lpb = szMustHave;
         pH = &gsExclLists[ iBlank - 1 ]; // get the BLANK list pointer
         iPos = InStr(lpb,";");
         while(iPos) {
            lpb[iPos-1] = 0;
            if( *lpb )
               Add2Exclude( pH, lpb );
            lpb = &lpb[iPos];
            iPos = InStr(lpb,";");
         }
         if( *lpb )
            Add2Exclude( pH, lpb );
         bChgXLst = TRUE;
      }
   }
   // ==============================================
   // FIX20110201 - Flip the FULL ignore as well
   // force these to FALSE at startup, regardless of INI value
   gbIgnDT = FALSE;
   gbIgnDT2 = FALSE;
   gbExact = FALSE;
   // Look for OTHER inits that should be done AFTER a READ
   // *****************************************************
}  // end Post_ReadINI( lpini )


#define  WI( a, b )\
   {  sprintf(lpb, "%d", b );\
      WritePrivateProfileString(pSect, a, lpb, lpini ); }

INT  Add_Excl_List( PTSTR lpb2, PLE pH, PTSTR sep )
{
   INT         iRet = 0;
   LPTSTR      lps;
   PLE         pN;
   PXLST       pxl;
   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      if( !( pxl->x_dwFlag & xf_Deleted ) ) {
         // get the string
         lps = &pxl->x_szStg[0]; // (LPTSTR)((PLE)pN + 1);
         if( *lpb2 ) {
            strcat(lpb2, sep);
            iRet += strlen(sep);
         }
         strcat(lpb2, lps);
         iRet += strlen(lps);
      }
   }
   return iRet;


}

INT  Get_Excl_List( LPTSTR lpb2, PLE pH )
{
   INT         iRet;
   *lpb2 = 0;
   iRet = Add_Excl_List( lpb2, pH, ";");
   return iRet;
}

VOID  WriteExcludeLists( LPTSTR lpini )
{
   PLE         pH;
   LPTSTR      lpb = &gszTmpBuf[0];
   LPTSTR      lpb2 = &gszTmpBuf2[0];
   INT         i = 0;

   for( i = 0; i < MXXLSTS; i++ )
   {
      pH = &gsExclLists[i];
      *lpb2 = 0;
      Get_Excl_List( lpb2, pH );
      sprintf(lpb, szLst, (i+1));   // set KEY
      WritePrivateProfileString(
						szXcl,		// Section
						lpb,		// Res.Word
						lpb2,		// String to write
						lpini );	// File Name
   }
}

VOID  Write_Excl_List( LPTSTR lpini, LPTSTR pSec, LPTSTR pKey, PLE pH, PLE pHI )
{
   LPTSTR lpb2 = GetStgBuf(); // &gszTmpBuf2[0];
   *lpb2 = 0;
   Get_Excl_List( lpb2, pH );

   if( pHI && ( *lpb2 == 0 ))
      Get_Excl_List( lpb2, pHI ); // get any previous INI items

   WritePrivateProfileString(
						pSec, // szXcl,		// Section
						pKey, // lpb,		// Res.Word
						lpb2,		// String to write
						lpini );	// File Name
}

VOID  WriteExcludeLists_ORG( LPTSTR lpini )
{
   PLE         pH, pN;
   LPTSTR      lps;
   LPTSTR      lpb = &gszTmpBuf[0];
   LPTSTR      lpb2 = &gszTmpBuf2[0];
   INT         i = 0;
   PXLST       pxl;

   for( i = 0; i < MXXLSTS; i++ )
   {
      pH = &gsExclLists[i];
      *lpb2 = 0;
      Traverse_List( pH, pN )
      {
         pxl = (PXLST)pN;
         if( !( pxl->x_dwFlag & xf_Deleted ) )
         {
            lps = &pxl->x_szStg[0]; // (LPTSTR)((PLE)pN + 1);
            if( *lpb2 )
               strcat(lpb2, ";");
            strcat(lpb2, lps);
         }
      }
      sprintf(lpb, szLst, (i+1));
      WritePrivateProfileString(
						szXcl,		// Section
						lpb,		// Res.Word
						lpb2,		// String to write
						lpini );	// File Name
   }

}


VOID  WriteDc4wINI( VOID )
{
   LPTSTR   lpini = &g_szIni[0];
   SetINIFile( lpini );
   //ToggleBool( &gbAddRow, &gbChgRow, ((gdwFileOpts & INCLUDE_LINENUMS) ? TRUE : FALSE) );
   WriteINI( lpini, FALSE );  // ensure FULL reset of 'changed' flags
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : WriteINI
// Return type: VOID 
// Argument   : LPTSTR   lpini
//            : BOOL bNoReset
// Description: Save the "preferences" to an INI file
//              
///////////////////////////////////////////////////////////////////////////////
VOID  WriteINI( LPTSTR lpini, BOOL bNoReset )
{
   LPTSTR   lpb   = &gszTmpBuf[0];
   PINILST  plst  = &sIniLst[0];
   DWORD    dwt;
   LPTSTR   pSect, pItem, pDef;
   BOOL     ball = *(plst->i_Chg);
   BOOL     bchg;
   PINT     pint;
   PBOOL    pb;
   WINDOWPLACEMENT * pwp;
   PRECT    pr;
   PLE      pH;

   //SetINIFile( lpini );

   //if( gbAddS != !( g_sSB.sb_bHidden );
   if( gbAddS == g_sSB.sb_bHidden )
   {
      gbAddS = !( gbAddS );
      gbChgAddS = TRUE;
   }

   if( ball )
   {
      // clear down the current INI file
      while( ( dwt = plst->i_Type ) != it_None )
      {
         pSect = plst->i_Sect;
         WritePrivateProfileString( pSect,		// Section
            NULL,    // Res.Word
            NULL,    // String to write
            lpini );	// File Name
         plst++;
      }

      // and CLEAR exclude section
      pSect = szXcl;
      WritePrivateProfileString( pSect,		// Section
            NULL,    // Res.Word
            NULL,    // String to write
            lpini );	// File Name

   }

   plst = &sIniLst[0];  // start of LIST

   while( ( dwt = plst->i_Type ) != it_None )
   {
      pSect = plst->i_Sect;
      pItem = plst->i_Item;
      pDef  = plst->i_Deft;
      bchg  = *(plst->i_Chg);
      if( ball || bchg )
      {
         *lpb = 0;
         switch(dwt)
         {
         case it_Version:
            strcpy(lpb, pDef);
            break;
         case it_String:
            strcpy(lpb, pDef);
            break;
         case it_Int:
            pint = (PINT)pDef;
            sprintf(lpb, "%d", *pint );
            break;
         case it_Bool:
            pb = (PBOOL)pDef;
            if( *pb )
               strcpy(lpb, "Yes");
            else
               strcpy(lpb, "No");
            break;
         case it_WinSize:     // special WINDOWPLACEMENT
            pb = (PBOOL)plst->i_Void;
            pwp = (WINDOWPLACEMENT *)pDef;
            if( ( pwp->length == sizeof(WINDOWPLACEMENT) ) &&
                ( ValidShowCmd( pwp->showCmd ) ) )
            {
               WI( szShow, pwp->showCmd );
               WI( szMaxX, pwp->ptMaxPosition.x );
               WI( szMaxY, pwp->ptMaxPosition.y );
               WI( szMinX, pwp->ptMinPosition.x );
               WI( szMinY, pwp->ptMinPosition.y );
               WI( szLeft, pwp->rcNormalPosition.left );
               WI( szTop,  pwp->rcNormalPosition.top  );
               WI( szRite, pwp->rcNormalPosition.right);
               WI( szBot,  pwp->rcNormalPosition.bottom);
               strcpy(lpb, "Yes");
            }
            else
               strcpy(lpb, "No");
            break;
         case it_Rect:
            pr = (PRECT)pDef;
            sprintf(lpb, "%d,%d,%d,%d", pr->left, pr->top, pr->right, pr->bottom );
            break;
         case it_Color:
            {
               COLORREF * pcr = (COLORREF *)pDef;
               sprintf(lpb, "%d,%d,%d",
                  GetRValue(*pcr), GetGValue(*pcr), GetBValue(*pcr) );
            }
            break;
         case it_SList:
            {
               PLE      plh = (PLE)pDef;
               PLE      pln;
               dwt = 0;
               Traverse_List( plh, pln )
               {
                  sprintf(lpb, pItem, (dwt+1)); // build the reserved word
                  WritePrivateProfileString(
						   pSect,		// Section
						   lpb,		// Res.Word
                     (LPTSTR)((PLE)pln + 1),		// String to write
                     lpini );	// File Name
                  dwt++;      // bump to next LIST number
                  if( dwt >= plst->i_Res1 )  // do NOT exceed maximum
                     break;
               }
               *lpb = 0;
            }
            break;
         }

         if( !bNoReset )
            *(plst->i_Chg) = FALSE;

         if( *lpb )
         {
            WritePrivateProfileString(
						pSect,		// Section
						pItem,		// Res.Word
						lpb,		// String to write
						lpini );	// File Name
         }
      }
      plst++;
   }

   if( ball || bChgXLst )
      WriteExcludeLists( lpini );

   pH = &gsXFileList;   // the EXCLUDE FILE LIST
   Write_Excl_List( lpini, szXcl, "FileList", pH, &gsXFileIni );
   pH = &gsXDirsList; // = EXCLUDE these DIRECTORIES -xd:Scenery
   Write_Excl_List( lpini, szXcl, "DirList", pH, &gsXDirsIni );

   if( !bNoReset )
     bChgXLst = FALSE;

}


// eof - dc4wIni.c
