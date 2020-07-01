
// TCIni.cpp = dc4wIni.c 
// #include	"dc4w.h"
// #include	"dc4wIni.h"
#include "stdafx.h"
//#include "TabCtrl.h"
//#include "TabCtrlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
	DWORD	   i_dwCnt;
} INILST, * PINILST;

typedef struct tagINILST2 { /* ii */
   PINILST  pIniLst;
   HWND     hwndCtl;
} INILST2, * PINILST2;

// single declaration of work data structure
// *****************************************
FIXEDWORK	sFW;
// *****************************************

static	TCHAR	g_szDefIni[] = "DC4W.INI";
LPTSTR   g_pTmpIni = "D:\\Gtools\\Tools\\dc4w\\TabCtrl\\TEMPI2.INI";

LPTSTR   g_pTCIni = "D:\\Gtools\\Tools\\dc4w\\TabCtrl\\TC.INI";

TCHAR	   g_szIni[264];

TCHAR szBlk[] = "\0";

BOOL  g_bAddOneEx = TRUE;  // add an 'exclude' list

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

// items in sections
// [Version]
TCHAR szDt[] = "Date";
TCHAR szCVer[] = VER_DATE;    // control re-setting of INI file
BOOL  bChgAll = FALSE;        // if SET, whole file is cleaned and redone

// [Options]
// int line_numbers;
TCHAR szLNum[] = "LineNumbers";
BOOL  bChgLnN = FALSE;


TCHAR szEdit[] = "Editor";
// extern   TCHAR editor_cmdline[]; // [264] = { "notepad %p" };  /* editor cmdline */
BOOL  bChgEd = FALSE;
// extern int outline_include
TCHAR szIncl[] = "FileInclude";
BOOL  bChgInc = FALSE;
TCHAR szBnks[] = "Blanks";
//BOOL  bChgBks = FALSE;

TCHAR szCase[] = "IgnoreCase";
// gbIgnCase 
// bChgIgC

///extern   BOOL picture_mode;
TCHAR szPic[] = "Picture";
BOOL  bChgPic = FALSE;

TCHAR szLft[] = "NameLeft";
TCHAR szRit[] = "NameRight";
TCHAR g_szLeft[264] = { "\0" };
TCHAR g_szRite[264] = { "\0" };
BOOL  bChgLf = FALSE;
BOOL  bChgRt = FALSE;

TCHAR szRec[] = "Recursive";

// gdwDiffOpts / bChgDO
TCHAR szDO[]  = "DiffInclude";
// gdwWrapWid / bChgWW
TCHAR szWW[]  = "WrapWidth";

// gdwFileOpts - write file list options / bChgFO
TCHAR szFO[]  = "FileListInclude";

// IDM_WARNING
TCHAR szWn[] = "Warning";

// IDM_OPTEXACT
TCHAR szExa[] = "Exact";
//BOOL  gbExact = TRUE;
//BOOL  bChgExa = FALSE;

// IDM_OPTEXCLUDE
TCHAR szBXcl[] = "Exclude";
TCHAR szXSel[] = "XSelection";

TCHAR szVF[]  = "VerifyFlag";
TCHAR szCOp[] = "FileCopyOptions";

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

//#define  gbNoTT      sFW.fw_NoTT    // diable tool tip
//#define  bChgTT      sFW.fw_bChgTT
TCHAR szNTT[] = "DisableToolTip";


//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
TCHAR szSkipC[] = "Skip-C/C++-Comments";
//   BOOL     fw_bSkipCPP;   // gbSkipCPP
//   BOOL     fw_bChgSCPP;   // bChgSCPP
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
TCHAR szSkipT[] = "Skip-Quoted-Text";
//   BOOL     fw_bSkipTxt;   // gbSkipTxt
//   BOOL     fw_bChgSTxt;   // bChgSTxt
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
TCHAR szIgnEL[] = "Ignore-Line-Termination";
//   BOOL     fw_bIgnEOL; // gbIgnEOL
//   BOOL     fw_bChgIEOL;   // bChgIEOL
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11

INILST  sIniLst[] = {
   { szVer, szDt,   it_Version,          szCVer,        &bChgAll, 0, 0 },  // CHANGE ALL!
   { szOpt, szLNum, it_Int,     (LPTSTR) &line_numbers, &bChgLnN, 0, 0 },
   { szOpt, szEdit, it_String,      &editor_cmdline[0], &bChgEd,  0, 0 },
// extern int outline_include TCHAR szIncl[] = "FileInclude"; BOOL  bChgInc = FALSE;
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
   { szOpt, szLft,  it_String, g_szLeft,                &bChgLf,  0, 0 },
   { szOpt, szRit,  it_String, g_szRite,                &bChgRt,  0, 0 },
   { szOpt, szRec,  it_Bool, (LPTSTR) &gbRecur,         &bChgRec, 0, 0 },
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
   // gdwFileOpts - write file list options / bChgFO
   { szOpt, szFO,   it_DWord,(LPTSTR)&gdwFileOpts,      &bChgFO,  0, 0 },
   // gszDif2Fil / bChgDF
   { szOpt, szDF,   it_String, &gszDif2Fil[0],          &bChgDF,  0, 0 },
   // gszListFil / bChgLF
   { szOpt, szLF,   it_String, &gszListFil[0],          &bChgLF,  0, 0 },

   { szOpt, szNTT,  it_Bool, (LPTSTR)&gbNoTT,           &bChgTT,  0, 0 },

// IDM_DIR - put up directories DIALOG
   { szOpt, szAuSel,it_Bool, (LPTSTR)&gbAutoSel,        &bChgASel,0, 0 },

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

   // ===================================================================
   { 0,     0,      it_None,    0,     0,        0, 0 }  // END OF TABLE
   // ===================================================================
};

#define  IniSize  (sizeof(sIniLst) / sizeof(INILST))

INILST  sIniCopy[IniSize] = { 0 };
INILST2 sIniLst2[IniSize];

TCHAR _s_szTmpBuf[264];
TCHAR _s_szTmpBuf2[264];
TCHAR gszBody1[264];
TCHAR gszExt1[264];
TCHAR gszBody2[264];
TCHAR gszExt2[264];

// extract from dw4wUtil.c
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : InStr
// Return type: INT 
// Arguments  : LPTSTR lpb
//            : LPTSTR lps
// Description: Return the position of the FIRST instance of the string in lps
//              Emulates the Visual Basic function.
///////////////////////////////////////////////////////////////////////////////
INT   InStr( LPTSTR lpb, LPTSTR lps )  // extracted from FixFObj.c
{
   INT   iRet = 0;
   INT   i, j, k, l, m;
   TCHAR    c;
   i = strlen(lpb);
   j = strlen(lps);
   if( i && j && ( i >= j ) )
   {
      c = *lps;   // get the first we are looking for
      l = i - ( j - 1 );   // get the maximum length to search
      for( k = 0; k < l; k++ )
      {
         if( lpb[k] == c )
         {
            // found the FIRST char so check until end of compare string
            for( m = 1; m < j; m++ )
            {
               if( lpb[k+m] != lps[m] )   // on first NOT equal
                  break;   // out of here
            }
            if( m == j )   // if we reached the end of the search string
            {
               iRet = k + 1;  // return NUMERIC position (that is LOGICAL + 1)
               break;   // and out of the outer search loop
            }
         }
      }  // for the search length
   }
   return iRet;
}

// *********************************************************
// EXTRACTED FROM utils\grmLib.c - May 2001 - AND MODIFIED

BOOL	GotWild( LPTSTR lps )
{
	BOOL	   flg = FALSE;
	INT      i, j;
	INT      c;
	i = strlen( lps ); 
	if(i)
	{
		for( j = 0; j < i; j++ )
		{
			c = lps[j];
			if( ( c == '*' ) || ( c == '?' ) )
			{
				flg = TRUE;
				break;
			}
		}
	}
	return flg;
}

BOOL	SplitExt2( LPTSTR lpb, LPTSTR lpe, LPTSTR lpf )
{
	BOOL	   flg = FALSE;
   DWORD    i;
   LPTSTR   p;
   //LPTSTR   lpr = _sGetSStg();
   //LPTSTR   lpr = new char[264];
   LPTSTR   lpr = _s_szTmpBuf;
   // if(!lpr) { return FALSE; }
   i = strlen(lpf);
   *lpb = 0;
   *lpe = 0;
	if(i)
	{
      strcpy(lpr,lpf);
      p = strrchr(lpr, '.');
		if(p)
		{
         *p = 0;
         p++;
         strcpy(lpb,lpr);
         strcpy(lpe,p);
         flg = TRUE;
      }
      else
      {
         strcpy(lpb,lpr);
      }
	}

   // // // delete lpr;

	return	flg;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : WildComp2
// Return type: BOOL 
// Arguments  : LPSTR lp1
//            : LPSTR lp2
// Description: Compare two components of a file name
//              FIX20010503 - Should NOT return TRUE on "c" with "cab"
//
///////////////////////////////////////////////////////////////////////////////
BOOL	WildComp2( LPTSTR lp1, LPTSTR lp2 )
{
	BOOL	   flg = FALSE;
	DWORD    i1, i2, j1, j2, ilen;
	INT      c, d;

	i1 = strlen(lp1);
	i2 = strlen(lp2);
	if( i1 && i2 )    // if BOTH have lengths
	{
      ilen = i1;
      if( i2 > i1 )
         ilen = i2;
		j2 = 0;
		for( j1 = 0; j1 < ilen; j1++ )
		{
			c = toupper(lp1[j1]);   // extract next char from each
			d = toupper(lp2[j2]);
         if( c == d )
         {
            j2++;
            continue;
         }
			// they are NOT equal
         if( c == 0 )
            break;
         if( d == 0 )
            break;

			{
				if( c == '*' )
				{
               j1++;
               if( lp1[j1] == 0 )   // get NEXT
               {
                  // ended with this asteric, so
                  flg = TRUE; // this matches all the rest of 2
					   break;   // out of here with a MATCH
               }
               // else we have somehting like *abc, which mean the asteric
               // matched what ever was in 2, up until this letter encountered
               c = toupper(lp1[j1]);
               j2++;    // asteric matched at least this one
      			if( lp2[j2] == 0 )   // 2 ended, but 1 has more
                  break;   // so no MATCH
               for( ; j2 < ilen; j2++ )
               {
         			d = toupper(lp2[j2]);
                  if( c == d )
                     break;
                  if( d == 0 )
                     break;
               }
               if( c == d )
               {
                  // found next of 1 in 2
                  j2++;
                  continue;
               }
               // else the char in 1 not present in two;
               break;   // no MATCH
				}

				if( d == '*' )
				{
               j2++;
               if( lp2[j2] == 0 )
               {
                  // 2 ends with asteric, so matches all rest in 1
                  flg = TRUE;
                  break;
               }
               d = toupper(lp2[j2]);
               j1++;    // asteric matched at least this one
      			if( lp1[j1] == 0 )   // 1 ended, but 2 has more
                  break;   // so no MATCH
               for( ; j1 < ilen; j1++ )   // find the 2 in 1
               {
         			c = toupper(lp1[j1]);
                  if( c == d )   // found it?
                     break;
                  if( c == 0 )   // or ran out of chars
                     break;
               }
               if( c == d )
               {
                  // found next of 2 in 1
                  j2++;
                  continue;
               }
               // else the char in 2 is not present in 1
					break;
				}

            if( ( c == '?' ) || ( d == '?' ) )
				{
					// One match char ok.
				}
				else
				{
					if( toupper( c ) != toupper( d ) )
						break;
				}
			}
			j2++;
		}
		if( !flg && ( j1 == ilen ) )
			flg = TRUE;
	}
   else
   {
      // FIX20010509 - Fix temp*.* should match tempf
      // Here the extension of tempf is nul, and thus should match with "*"
      if( ( i1 == 0 ) && ( i2 == 0 ) )
      {
         // two blanks is a PERFECT match
         flg = TRUE;
      }
      else if( i1 == 0 )
      {
         // the first is BLANK. This would be a MATCH if an "*" or "?" in 2, no?
         if( ( i2 == 1 ) &&
             ( ( *lp2 == '*' ) || ( *lp2 == '?' ) ) )
             flg = TRUE;
      }
      else  // if( i2 == 0 )
      {
         if( ( i1 == 1 ) &&
             ( ( *lp1 == '*' ) || ( *lp1 == '?' ) ) )
             flg = TRUE;
      }
   }
	return flg;
}

BOOL	MatchFiles( LPTSTR lp1, LPTSTR lp2 )
{
	BOOL	flg = FALSE;
	if( lp1 && lp2 &&
		*lp1 && *lp2 )
	{
		if( !GotWild( lp1 ) &&
			 !GotWild( lp2 ) )
		{
         // neither have wild cards 
         // so just do a COMPARE
			if( strcmpi( lp1, lp2 ) == 0 )
				flg = TRUE;
		}
		else
		{
			// One of the other HAS WILD CHAR(S)
         //LPTSTR   lpb = new char[ (264 * 4) ];
         //LPTSTR   lpb1 = &lpb[ (264 * 0) ]; // gszBody1;
         //   chkme( "Mem FAILED!"MEOR );
         LPTSTR   lpb1 = gszBody1;
         LPTSTR   lpe1 = gszExt1;
      	LPTSTR   lpb2 = gszBody2;
         LPTSTR   lpe2 = gszExt2;
			SplitExt2( lpb1, lpe1, lp1 );
			SplitExt2( lpb2, lpe2, lp2 );
			if( ( WildComp2( lpb1, lpb2 ) ) &&
				 ( WildComp2( lpe1, lpe2 ) ) )
			{
				flg = TRUE;
			}
         // // // // // delete lpb;
		}
	}
	return flg;
}

// EXTRACTED FROM utils\grmLib.c - May 2001 - and MODIFIED
// *********************************************************
// end extract from dc4wUtil.c

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
   GetModulePath( lpini );    // does   GetModuleFileName( NULL, lpini, 256 );
   strcat(lpini, g_szDefIni);
}

#define	GetStg( a, b )	\
   ::GetPrivateProfileString( a, b, &szBlk[0], lpb, 256, lpini )

#define	GetStg2( a, b )	\
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
   }
   return bRet;
}

#define  VALIDLN(a)  ( ( a == IDM_LNRS ) || ( a == IDM_RNRS ) || ( a == IDM_NONRS ) )

#define     MAXXLC      16

#define  xf_Deleted     0x00000001
#define  xf_Found       0x80000000

typedef  struct   tagXLST {
   LIST_ENTRY  x_List;  // first LIST entry
   DWORD       x_dwFlag;   // flag of item
   TCHAR       x_szStg[1]; // string
}XLST, * PXLST;


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

PVOID Add2StgList( PLE pH, LPTSTR lpb )
{
   LPTSTR   lps;
   INT      i;
   PXLST    pxl;
   i = (strlen(lpb) + sizeof(XLST));
   pxl = (PXLST)MALLOC(i);
   if(pxl)
   {
      ZeroMemory( pxl, i );
      lps = &pxl->x_szStg[0];
      strcpy(lps, lpb);
      InsertTailList(pH,(PLE)pxl);
   }
   return pxl;
}

PVOID  Add2Exclude( PLE pH, LPTSTR lpb )
{
   INT   i = strlen(lpb);
   PXLST pxl = 0;
   if(i)
   {
      PLE         pN;
      LPTSTR      lps;
      Traverse_List( pH, pN )
      {
         pxl = (PXLST)pN;
         lps = &pxl->x_szStg[0];
         if( strcmpi( lps, lpb ) == 0 )
         {
            pxl->x_dwFlag &= ~(xf_Deleted);
            return NULL;
         }
      }
      pxl = (PXLST) Add2StgList( pH, lpb );
   }
   return pxl;
}

BOOL  Match2XList( PLE pH, LPTSTR lpf )
{
   BOOL     bRet = FALSE;  // assume NOT in the LIST
   PLE      pN;
   LPTSTR   lps;
   PXLST    pxl;
   Traverse_List( pH, pN )
   {
      pxl = (PXLST)pN;
      lps = &pxl->x_szStg[0];
      if( MatchFiles( lpf, lps ) )
      {
         bRet = TRUE;   // *** IT IS IN THE LIST ***
         break;
      }
   }
   return bRet;
}

BOOL  MatchesExclude( LPTSTR lpf )
{
   BOOL     bRet = FALSE;
   PLE      pH;
   INT      i = giCurXSel;    // get current SELECTION
   if( i && ( i <= MXXLSTS ) )
      i--;
   else
      i = 0;
   pH = &gsExclLists[i];   // select the correct user list of EXCLUDES
   return( Match2XList( pH, lpf ) );
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
      pxln = (PXLST) Add2Exclude( pCopy, lps );
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

VOID  ReadINI( LPTSTR lpini );
VOID  ReadDc4wINI( VOID )
{
   LPTSTR   lpini = &g_szIni[0];
   SetINIFile( lpini );
   ReadINI( lpini );
}

VOID  ReadTmpINI( VOID )
{
   LPTSTR   lpini = g_pTmpIni;
   // SetINIFile( lpini ); // more like FIND INI file!!!
   ReadINI( lpini );
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : ReadINI
// Return type: VOID 
// Argument   : LPTSTR lpini - name of file to use
// Description: Read the "preferences" from an INI file
///////////////////////////////////////////////////////////////////////////////
VOID  ReadINI( LPTSTR lpini )
{
   PINILST  plst = &sIniLst[0];
   //LPTSTR   lpb = &gszTmpBuf[0];
   //LPTSTR   lpb = new TCHAR[ (1024 * 2) ];  // temp buffer for reading
   LPTSTR   lpb = _s_szTmpBuf;  // temp buffer for reading
   DWORD    dwt;
   LPTSTR   pSect, pItem, pDef;
   PBOOL    pb, pbd;
   PINT     pi;
   INT      i;
   PRECT    pr, pr1;
   //LPTSTR   lpb2 = &lpb[1024];
   LPTSTR   lpb2 = _s_szTmpBuf2;

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
         //for( i = 0; i < (INT)plst->i_Res1; i++ )
         for( i = 0; i < (INT)plst->i_dwCnt; i++ )
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
         *pb = TRUE;    // can only SET change

      plst++;
   }

   //{
      //LPTSTR   lpb2 = &gszTmpBuf2[0];
      //LPTSTR   lpb2 = new TCHAR[1024];
   if( lpb2 )
   {
      PLE      pH;
      INT      iPos, iCnt;
      //LPTSTR   lpsave = lpb;  // keep being of buffer

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
      iPos = 0;
      for( i = 0; i < MXXLSTS; i++ )
      {
         iCnt = 0;
         pH = &gsExclLists[i];
         ListCount2(pH,&iCnt);
         iPos += iCnt;
      }
      if( !iPos )
      {
         pH = &gsExclLists[0];
         Add2Exclude( pH, "*.bak");
         Add2Exclude( pH, "*.old");
         Add2Exclude( pH, "temp*.*");
         Add2Exclude( pH, "*.obj");
         Add2Exclude( pH, "*.ncb");
         Add2Exclude( pH, "*.opt");
         bChgXLst = TRUE;
      }

      //lpb = &gszTmpBuf[0]; // return lpb to its correct value
      //lpb = lpsave; // return lpb to its correct value
      lpb = _s_szTmpBuf;   // reset to original
   }


   // any VALIDATIONS required - you never know WHAT you can read from an INI file
   // ************************
   if( !VALIDLN( line_numbers ) )
   {
      //   { szOpt, szLNum, it_Int,     (LPTSTR) &line_numbers, &bChgLnN, 0, 0 },
      line_numbers = IDM_LNRS;   // set default
      bChgLnN = TRUE;
   }

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

   //if( INCLUDE_DIFFER then MUST have one or both of Newer or Older
   if( outline_include & INCLUDE_DIFFER )
   {
      if( ( outline_include & (INCLUDE_NEWER|INCLUDE_OLDER) ) == 0 )
      {
         if( !(outline_include & INCLUDE_NEWER) )
         {
            outline_include |= INCLUDE_NEWER;
//         outline_include |= (INCLUDE_NEWER|INCLUDE_OLDER);
//TCHAR szIncl[] = "FileInclude";
            bChgInc = TRUE;
         }
      }
   }
   else
   {
      if( outline_include & (INCLUDE_NEWER|INCLUDE_OLDER) )
      {
         outline_include &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
         bChgInc = TRUE;
      }
   }

   if( gdwCpyOpts & INCLUDE_DIFFER )
   {
      if( ( gdwCpyOpts & (INCLUDE_NEWER|INCLUDE_OLDER) ) == 0 )
      {
         if( !(gdwCpyOpts & INCLUDE_NEWER) )
         {
            gdwCpyOpts |= INCLUDE_NEWER;
//         outline_include |= (INCLUDE_NEWER|INCLUDE_OLDER);
         // gdwCpyOpts / bChgCO = "FileCopyOptions"
         // { szOpt, szCOp,  it_DWord,(LPTSTR)&gdwCpyOpts,       &
            bChgCO = TRUE;
         }
      }
   }
   else
   {
      if( gdwCpyOpts & (INCLUDE_NEWER|INCLUDE_OLDER) )
      {
         gdwCpyOpts &= ~(INCLUDE_NEWER|INCLUDE_OLDER);
         bChgCO = TRUE;
      }
   }

   // ************************

   // // // // delete lpb;

}

#define  WI( a, b )\
   {  sprintf(lpb, "%d", b );\
      WritePrivateProfileString(pSect, a, lpb, lpini ); }


VOID  WriteExcludeList( LPTSTR lpini )
{
   PLE         pH, pN;
   LPTSTR      lps;
   //LPTSTR      lpb = &gszTmpBuf[0];
   //LPTSTR      lpb2 = &gszTmpBuf2[0];
   //LPTSTR      lpb = new TCHAR[ (1024 * 1) ];
   //LPTSTR      lpb2 = &lpb[1024];
   LPTSTR      lpb = &_s_szTmpBuf[0];
   LPTSTR      lpb2 = &_s_szTmpBuf2[0];
   INT         i = 0;
   PXLST       pxl;

   // if( !lpb ) { chkme( "C:MEM"MEOR ); return; }

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

   // // // delete lpb;
}

VOID  WriteINI( LPTSTR lpini, BOOL bNoReset );
VOID  WriteDc4wINI( VOID )
{
   LPTSTR   lpini = &g_szIni[0];
   SetINIFile( lpini );
   WriteINI( lpini, FALSE );  // ensure FULL reset of 'changed' flags
}

//LPTSTR   g_pTCIni = "D:\\Gtools\\Tools\\dc4w\\TabCtrl\\TC.INI";
VOID  WriteTCINI( VOID )
{
   LPTSTR   lpini = g_pTCIni;
   //SetINIFile( lpini );
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
   //LPTSTR   lpb   = &gszTmpBuf[0];
   //LPTSTR   lpb   = new TCHAR[1024];
   LPTSTR   lpb   = _s_szTmpBuf;
   PINILST  plst  = &sIniLst[0];
   DWORD    dwt;
   LPTSTR   pSect, pItem, pDef;
   BOOL     ball = *(plst->i_Chg);
   BOOL     bchg;
   PINT     pint;
   PBOOL    pb;
   WINDOWPLACEMENT * pwp;
   PRECT    pr;

   //SetINIFile( lpini );
   //if( !lpb ) return;
   //if( gbAddS != !( g_sSB.sb_bHidden );
//   if( gbAddS == g_sSB.sb_bHidden )
//   {
//      gbAddS = !( gbAddS );
//      gbChgAddS = TRUE;
//   }

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
                  if( dwt >= plst->i_dwCnt )  // do NOT exceed maximum
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
      WriteExcludeList( lpini );

   if( !bNoReset )
     bChgXLst = FALSE;

   // // // // delete lpb;

}


// eof - dc4wIni.c

// dc4w.c
BOOL  InitFixedWork( VOID )
{
   INT   i;
   PLE   pH;
   //pW = LocalAlloc( LPTR, sizeof(WORKSTR) );
   //if( !pW )
   //   return FALSE;

   //ZeroMemory( pW, sizeof(WORKSTR) );
   ZeroMemory( &sFW, sizeof(FIXEDWORK) );

   //g_sCFDLGSTR.cf_dwSize = sizeof(CFDLGSTR); // set just once

   // Initialise default values (other than 0/FALSE)
   gbRecur = TRUE;

   InitLList( &gsCopyList );
   for( i = 0; i < MXXLSTS; i++ )
   {
      InitLList( &gsExclLists[i] );
   }
   if( g_bAddOneEx )
   {
      pH = &gsExclLists[0];
      Add2Exclude( pH, "*.bak");
      Add2Exclude( pH, "*.old");
      Add2Exclude( pH, "temp*.*");
      Add2Exclude( pH, "*.obj");
      Add2Exclude( pH, "*.ncb");
      Add2Exclude( pH, "*.opt");
      bChgXLst = TRUE;
   }

   InitLList( &gsCompList );  // list of COMPARE items

   InitLList( &gsDiffList );
   InitLList( &gsFileList );

   InitLList( &gsXDirsList ); // = EXCLUDE these DIRECTORIES -xd:Scenery
   InitLList( &gsXFileList ); // = EXCLUDE these FILES       -xf:temp*.*;*.old

   InitLList( &g_sZipFiles ); // list in active ZIP

   // expanded mode - line output default options
   gdwDiffOpts = INC_ALLXSAME;   // everything excluding SAME lines
   // outline  mode - file list output default options
   gdwFileOpts = INC_ALLXSM;
   gdwWrapWid  = DEF_WRAP_WIDTH;

   // copy / update files default options
   //gdwCopyOpts  = DEF_COPY_SET;
   gdwCpyOpts  = DEF_COPY_SET;

   gdwVerFlag  = DEF_VER_FLAG;   // bChgVF

   gwarn_text = DEF_CR_WT;    // = RGB(255,255,255)
   gwarn_back = DEF_CR_WB;    // = RGB(200,30,30)

   ghelp_text = DEF_CR_HT;    // = RGB(0,0,0)
   ghelp_back = DEF_CR_HB;    // = RGB(255,255,255)

   /* outline */
   rgb_outlinehi = RGB(255, 0, 0);   /* hilighted files in outline mode  */

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

   giTabSize  = 3;   //DEF_TAB_SIZE;    // was 8

   //{ szOpt, szBnks, it_Bool, (LPTSTR)&
   ignore_blanks = TRUE;   // &bChgBks, (PVOID)IDC_IGNORESP, 0 },
   //if( !VALIDLN( line_numbers ) )  {
      //   { szOpt, szLNum, it_Int,     (LPTSTR) &line_numbers, &bChgLnN, 0, 0 },
   line_numbers = IDM_LNRS;   // set default bChgLnN = FALSE;
//   expand_mode = IDM_BOTHFILES;
   //#define  INCLUDE_DIFFER2   (INCLUDE_DIFFER|INCLUDE_NEWER|INCLUDE_OLDER)
   outline_include = INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY|INCLUDE_SAME|INCLUDE_DIFFER2;

   ignore_blanks = TRUE;
   picture_mode = TRUE;

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

   //strcpy( g_strTT, "This is your tooltip string." );
   //InitC2S();
   ReadTmpINI();  // load the INI written by dc4w

   return TRUE;
}

// eof - dc4w.c
// eof - TCIni.cpp
