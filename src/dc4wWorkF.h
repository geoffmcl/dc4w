
// dc4wFWork.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef  _dc4wFWork_HH
#define  _dc4wFWork_HH

//#define  MALLOC(a)      new BYTE[a]
#define  MALLOC(a)      LocalAlloc( LPTR, a );
#define  MFREE(a)       LocalFree(a)

#define  MXXLSTS     16
#define  MXCMPLST    32

// #define IDM_LNRS                        40021
// #define IDM_RNRS                        40022
// #define IDM_NONRS                       40023

/* outline_include is an OR of the following */
// controls the types of 'file-compares' that will be shown
#define  INCLUDE_SAME            0x00000001
#define  INCLUDE_DIFFER          0x00000002
#define  INCLUDE_LEFTONLY        0x00000004
#define  INCLUDE_RIGHTONLY       0x00000008
// If master DIFFER on, then separate NEWER / OLDER
#define  INCLUDE_NEWER           0x00000010
#define  INCLUDE_OLDER           0x00000020

#ifdef   ADDUPDATE2
// special - used only in update
#define  DELETE_ONLY             INCLUDE_RIGHTONLY
#endif   // ADDUPDATE2

// and expanded has moved line, so OR of the following
#define  INCLUDE_MOVELEFT        0x00000040
#define  INCLUDE_MOVERIGHT       0x00000080

// and a line number option
#define  INCLUDE_LINENUMS        0x00000100
#define  INCLUDE_TAGS            0x00000200
#define  APPEND_FILE             0x00000400
#define  INCLUDE_HEADER          0x00000800
#define  WRAP_LINES              0x00001000
#define  DEF_WRAP_WIDTH          75
#define  FULL_NAMES              0x00002000  // use FULL name to SAVE LIST (else REL name)
#define  FLEFT_NAME              0x00004000  // use LEFT full name
#define  FRIGHT_NAME             0x00008000  // use RIGHT full name
#define  COMBINED_NAME           0x00010000  // use combination of left and right
#define  ADD_COMMENTS            0x00020000  // add tailing comments
#define  ADD_REL_PATH            0x00040000  // put "foo.c", not ".\foo.c" (note reversal)
#define  ADD_X_HDR               0x00080000  // multi-lines
#define  ADD_FIL_INFO            0x00100000  // add file date/time/size(s)

//#define  COPY_FROMLEFT           0x00100000  /* copy files from left tree */
//#define  COPY_FROMRIGHT          0x00200000  /* copy files from right tree */
// dec 28, 2001 - All 'update' actions are LEFT to RIGHT only.
// There is a MENU item to FLIP the left/right trees
/* This used to be
 * Copy file options are either COPY_FROMLEFT or COPY_FROMRIGHT 
 * (indicating which  * tree is to be the source of the files), plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT (INCLUDE_LEFT
 * and INCLUDE_RIGHT are treated the same here since the COPY_FROM* option
 * indicates which side to copy from). *** NOW DISCONTINUED *** */

// and some common combinations
#define  INC_ALLONLY    (INCLUDE_LEFTONLY | INCLUDE_RIGHTONLY)
#define  INC_ALLMOVE    (INCLUDE_MOVELEFT | INCLUDE_MOVERIGHT)
#define  INC_OTHERS     (INCLUDE_LINENUMS | INCLUDE_TAGS | APPEND_FILE | \
                         INCLUDE_HEADER   | WRAP_LINES   | FULL_NAMES  | \
                         FLEFT_NAME       | ADD_COMMENTS )
#define  INC_OUTLINE    (INCLUDE_SAME|INCLUDE_DIFFER|INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY)
#define  INC_OUTLINE2   (INC_OUTLINE|INCLUDE_NEWER|INCLUDE_OLDER)

// EXPANDED MODE
// in expanded mode, INCLUDE_DIFFER is redundant since each line
// is either same, left, right or moved! Toggling the bit
// sets both the line display and the difference file output
#define  INC_ALLXSAME   (INC_ALLONLY|INC_ALLMOVE|INC_OTHERS)
// presently a line section only has ONE of 5 states
// and remember 'same' can be after ignoring blanks, and line endings
#define  EXP_ALL  (INCLUDE_SAME|INC_ALLONLY|INC_ALLMOVE)


// OUTLINE MODE
// in outline mode, files are either same, differ, left or right
#define  INC_ALLXSM     (INCLUDE_DIFFER|INC_ALLONLY|INC_OTHERS)
// default copy options
//#define  DEF_COPY_SET   (COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER)
//#define  DEF_COPY_SET   (COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_NEWER)
#define  DEF_COPY_SET   (INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_NEWER)
// int   expand_mode = IDM_BOTHFILES;

// NOTE: 3 bit are used for the 'differ' option so can split older/newer
#define  INCLUDE_DIFFAGE   (INCLUDE_NEWER|INCLUDE_OLDER)
#define  INCLUDE_ALLDIFF   (INCLUDE_DIFFER|INCLUDE_DIFFAGE)

// this is the OUTLINE set of switches
#define  INCLUDE_BASE   (INCLUDE_SAME|INCLUDE_DIFFER|INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY)
#define  INCLUDE_MINO   INCLUDE_DIFFAGE

//#define  FixDiff(a)
#define  FIXINCDIFF(a) {      \
   if( a & INCLUDE_DIFFAGE )  \
      a |= INCLUDE_DIFFER;    \
   else                       \
      a &= ~(INCLUDE_DIFFER); \
   }

#define  FixDiff(a)  FIXINCDIFF(a)
#define  ShowingAll(a) ((a & INCLUDE_DIFFAGE) == INCLUDE_DIFFAGE)

#define  ISNUM(a)    ( ( a >= '0' ) && ( a <= '9' ) )
#define  ISUPPER(a)  ( ( a >= 'A' ) && ( a <= 'Z' ) )
#define  ISLOWER(a)  ( ( a >= 'a' ) && ( a <= 'z' ) )
#define  ISALPHANUM(a)  ( ISNUM(a) || ISUPPER(a) || ISLOWER(a) )

#define  VERIFY_EACH    0x00000001
#define  VERIFY_OLDER   0x00000002
#define  MAKE_BACKUP    0x00000004
#define  REVIEW_LIST    0x00000008
#define  CHECK_WARN     0x00000010

#define  DEF_VER_FLAG   (VERIFY_OLDER | MAKE_BACKUP | REVIEW_LIST | CHECK_WARN)

#define  DEF_CR_WT   RGB(255,255,255)
#define  DEF_CR_WB   RGB(200,30,30)
#define  DEF_CR_HT   RGB(0,0,0)
#define  DEF_CR_HB   RGB(255,255,255)

#define  DEF_TAB_SIZE   8

#define  ZP_COMMAND  0
#define  ZP_SWITCHES 1
#define  ZP_ZIPFILE  2
#define  ZP_ZIPINP   3
#define  ZP_ZIPENV   4
#define  ZP_DIRCMP   5

#define  ZP_MAXCMD   8

// ZIPUP feature - command line structure for say a 'complex' zip command
typedef struct tagCMDLN {
   TCHAR    szCmd[264]; // runtime EXE (or batch) to do the ZIP job
   TCHAR    szSws[264]; // command switches to pass to the runtime
   TCHAR    szZip[264]; // name of the OUTPUT zip file name (and path)
   TCHAR    szInp[264]; // input LIST of files - written as part of process
   TCHAR    szEnv[264]; // *** not used ***
   TCHAR    szCmp[264*2];  // compare directories - actually List1
}CMDLN, * PCMDLN;

/* Structure containing all the arguments we'd like to give to do_editfile
   Need a structure because CreateThread only allows for one argument. */
typedef struct {
   PVIEW    view;
   int      option;
   int      selection;
   int      onviewcnt;
   BOOL     bDoMB;   // put up a message box if error
   DWORD    dwError; // error indication
   DWORD    dwWait;  // return from WaitForSingleObject(pi.hProcess, INFINITE);
   BOOL     bZipOk;  // TRUE if return is WAIT_OBJECT_0
   LPTSTR   pCmd;
   COMPLIST cl;   // = view_getcomplist(current_view);
   CMDLN    sCmdLn;  // various components of the command line
   TCHAR    cCmdLine[1024];
   TCHAR    cErrMsg[264];
   PVOID    pThreadArgs;   // filled in if available
} EDITARGS, * PEDITARGS;


// FIXED WORK BLOCK - Ensure initialised to default value, before INI read
typedef	struct tagFIXEDWORK {

   DWORD    fw_line_numbers;  // line_numbers - was left,right or none

   DWORD    fw_outline_include;  // outline_include - controls OUTLINE displayable data

   // control display of which lines in expanded mode
   DWORD fw_expand_mode;   // expand_mode
   DWORD fw_bChgEM;        // gbChgEM

   DWORD fw_dwDiffOpts;    // gdwDiffOpts
   BOOL  fw_bChgDO;        // bChgDO

#ifndef USE_GLOBAL_RECURSIVE
	BOOL  fw_bRecur;	// gbRecur
#endif // #ifndef USE_GLOBAL_RECURSIVE
	BOOL  fw_bChgRec;	// bChgRec

	BOOL  fw_bExclude;	// gbExclude
	BOOL  fw_bChgExcl;	// bChgExcl

   DWORD fw_dwWrapWid;    // gdwWrapWid
   BOOL  fw_bChgWW;        // bChgWW

   DWORD fw_dwFileOpts;    // gdwFileOpts
   BOOL  fw_bChgFO;        // bChgFO

   DWORD fw_dwCpyOpts;    // gdwCpyOpts
   BOOL  fw_bChgCO;        // bChgCO

   DWORD fw_dwVerFlag;    // gdwVerFlag
   BOOL  fw_bChgVF;        // bChgVF

   TCHAR fw_szDif2Fil[264]; // gszDif2Fil
   BOOL  fw_bChgDF;        // bChgDF
   TCHAR fw_szListFil[264]; // gszListFil
   BOOL  fw_bChgLF;        // bChgLF

   // FIX20091125 - add tags to list output
   TCHAR fw_gszTagHead[264]; // gszTagHead
   BOOL  fw_gbChgTH;         // gbChgTH
   TCHAR fw_gszTagTail[264]; // gszTagTail
   BOOL  fw_gbChgTT;         // gbChgTT

   LIST_ENTRY     fw_sCopyList;  // gsCopyList
   LIST_ENTRY     fw_sCompList;  // gsCompList - compare list for MXCMPLST
   LIST_ENTRY     fw_sDiffList;  // gsDiffList - List of files where differences written
   BOOL           fw_bChgDLst;   // bChgDLst
   LIST_ENTRY     fw_sFileList;  // gsFileList - List of files where file list written
   BOOL           fw_bChgFLst;   // bChgFLst

   LIST_ENTRY     fw_sZipList;   // gsZipList - on successful copy of save ZIP
   BOOL           fw_bChgZLst;   // bChgZLst

   LRESULT        fw_iCurXSel;   // giCurXSel
   BOOL           fw_bChgXSel;   // bChgXSel

   LIST_ENTRY     fw_sExclLists[MXXLSTS];  // gsExclLists
   BOOL           fw_bChgXLst;

   LIST_ENTRY     fw_sXDirsList;  // gsXDirsList = EXCLUDE these DIRECTORIES -xd:Scenery
   BOOL           fw_bXAllRepos;
   LIST_ENTRY     fw_sXFileList;  // gsXFileList = EXCLUDE these FILES       -xf:temp*.*;*.old
   BOOL           fw_bChgXDL, fw_bChgXFL;

   // gsXDirsIni = EXCLUDE these DIRECTORIES -xd:Scenery
   LIST_ENTRY     fw_sXDirsIni, fw_sXFileIni, fw_sXMissIni;
   BOOL           fw_bChgXDI, fw_bChgXFI, fw_bChgXMI;

   LIST_ENTRY     fw_sXMissList;  // gsXMissList = EXCLUDE these FILES or DIRECTORIES
   LIST_ENTRY     fw_sZipFiles;   // g_sZipFiles

   BOOL           fw_bWarn;   // gbWarn
   BOOL           fw_bChgWn;  // bChgWn

   COLORREF    fw_warn_text;  // gwarn_text = RGB(255,255,255);
   COLORREF    fw_warn_back;  // gwarn_back = RGB(200,30,30);
   BOOL     fw_bChgWT, fw_bChgWB;

//   ghelp_text = DEF_CR_HT;    // = RGB(0,0,0)
//   ghelp_back = DEF_CR_HB;    // = RGB(255,255,255)
//TCHAR szCHT[] = "Help_Text";
//TCHAR szCHB[] = "Help_Back";
//   { szClr, szCHT,  it_Color, (LPTSTR)&ghelp_text,       &bChgHT,  0, 0 },
//   { szClr, szCHB,  it_Color, (LPTSTR)&ghelp_back,       &bChgHB,  0, 0 },
   COLORREF    fw_help_text;  // ghelp_text = RGB(0,0,0);
   COLORREF    fw_help_back;  // ghelp_back = RGB(255,255,255);
   BOOL     fw_bChgHT, fw_bChgHB;

   BOOL     fw_NoTT;    // gbNoTT
   BOOL     fw_bChgTT;  // bChgTT

   TCHAR    fw_szSingle[MXFNB];     // gszSingle - its NAME
   BOOL     fw_bChgSng;              // gbChgSng

   UINT     fw_iTabSize;   // giTabSize
   BOOL     fw_bChgTab;    // gbChgTab

   BOOL     fw_bMaxExp;    // gbMaxExp;
   BOOL     fw_bChgME;     // gbChgME

   BOOL     fw_bSetOut;    // gbSetOut;
   BOOL     fw_bChgSO;     // gbChgSO

//   BOOL     fw_bVerify;          // gbVerify
//   BOOL     fw_bChgVer;          // bChgVer
// IDM_ADDSTATUS
//TCHAR szAddS[] = "Status_Line";
   BOOL     fw_bAddS;      // gbAddS
   BOOL     fw_bChgAddS;   // gbChgAddS

// IDM_DIR - put up directories DIALOG
// TCHAR szAuSel[] = "AutoSelect1";
   BOOL     fw_bAutoSel;   // gbAutoSel
   BOOL     fw_bChgASel;   // bChgASel

   //   { szClr, szcHi,  it_Color, (LPTSTR)&
   COLORREF fw_rgb_outlinehi; // ,    &bChgcHi, 0, 0 },
   //   { szOpt, szAddS, it_Bool, (LPTSTR)&gbAddS,           &gbChgAddS, 0, 0 },
   COLORREF fw_rgb_outlineNew;   // rgb_outlineNew //   &bChgcNw, 0, 0 },

   COLORREF fw_rgb_outviewBack;  // rgb_outviewBack   // gray

   /* expand view */
   //{ szClr, szcELF, it_Color, (LPTSTR)&
   COLORREF fw_rgb_leftfore;  //    &bChgcELF,0, 0 },
   //{ szClr, szcELB, it_Color, (LPTSTR)&
   COLORREF fw_rgb_leftback;  //     &bChgcELB,0, 0 },
   //{ szClr, szcERF, it_Color, (LPTSTR)&
   COLORREF fw_rgb_rightfore; //   &bChgcERF,0, 0 },
   //{ szClr, szcERB, it_Color, (LPTSTR)&
   COLORREF fw_rgb_rightback; //   &bChgcERB,0, 0 },
   /* moved lines */
   //{ szClr, szcMLF, it_Color, (LPTSTR)&
   COLORREF fw_rgb_mleftfore; //  &bChgcMLF,0, 0 },
   //{ szClr, szcMLB, it_Color, (LPTSTR)&
   COLORREF fw_rgb_mleftback; //  &bChgcMLB,0, 0 },
   //{ szClr, szcMRF, it_Color, (LPTSTR)&
   COLORREF fw_rgb_mrightfore;   //  &bChgcMRF,0, 0 },
   //{ szClr, szcMRB, it_Color, (LPTSTR)&
   COLORREF fw_rgb_mrightback;   // &bChgcMRB,0, 0 },
   /* bar window */
   //{ szClr, szcBLf, it_Color, (LPTSTR)&
   COLORREF fw_rgb_barleft;   // &bChgcBLf,0, 0 },
   //{ szClr, szcBRt, it_Color, (LPTSTR)&
   COLORREF fw_rgb_barright;  //  &bChgcBRt,0, 0 },
   //{ szClr, szcBCr, it_Color, (LPTSTR)&
   COLORREF fw_rgb_barcurrent;   //  &bChgcBCr,0, 0 },

   RECT     fw_rcSize;  // grcSize
   BOOL     fw_bSzValid;   // bSzValid
   BOOL     fw_bChgSz, fw_IsZoom, fw_bChgZm, fw_IsIcon, fw_bChgIc;
   TCHAR    fw_szCopyTo[264]; // gszCopyTo   // destination of COPY files
   BOOL     fw_bChgCT;        // bChgCT;

   TCHAR    fw_editor_cmdline[264]; // editor_cmdline - to run EDIT a file

//   { szOpt, szBnks, it_Bool, (LPTSTR)&ignore_blanks,    &bChgBks, (PVOID)IDC_IGNORESP, 0 },
   BOOL     fw_ignore_blanks;    // ignore_blanks
   BOOL     fw_bChgBks;          // bChgBks

   DWORD    fw_picture_mode;     // picture_mode

   //TCHAR szCase[] = "IgnoreCase";
   //{ szOpt, szCase, it_Bool, (LPTSTR)&
   BOOL     fw_bIgnCase;   // gbIgnCase
   BOOL     fw_bChgIgC;    // bChgIgC, (PVOID)IDC_NOCASE, 0 },

//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
   BOOL     fw_bSkipCPP;   // gbSkipCPP
   BOOL     fw_bChgSCPP;   // bChgSCPP
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
   BOOL     fw_bSkipTxt;   // gbSkipTxt
   BOOL     fw_bChgSTxt;   // bChgSTxt
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
   BOOL     fw_bIgnEOL; // gbIgnEOL
   BOOL     fw_bChgIEOL;   // bChgIEOL
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11

// IDM_OPTEXACT
//TCHAR szExa[] = "Exact";
   BOOL     fw_bExact;  // gbExact  // load file and compare in full
   BOOL     fw_bChgExa; // bChgExa

   BOOL  fw_bUseCSV; // gbUseCSV // use comma delimited output
   BOOL  fw_bChgCSV; // gbChgCSV

//   case IDM_NONRS: // this is a display toggle
#ifndef   NEWCOLS2 // add or subtract columns with ease
//TCHAR szShwNums[] = "ShowLineNumbers";
   BOOL  fw_bShowNums;  // gbShowNums
   BOOL  fw_bChgSN;     // gbChgSN
#endif   // #ifndef   NEWCOLS2 // add or subtract columns with ease
//   case IDM_LNRS: case IDM_RNRS:
//TCHAR szUseRt[] = "UseRightNumbers"; // just the source of the number - if abv ON
   BOOL  fw_bUseRight;  // g_bUseRight
   BOOL  fw_bChgURt;    // g_bChgURt

// IDM_OPTIGNOREDT
   BOOL  fw_bIgnDT;  // g_bIgnDT
   BOOL  fw_bChgIDT;
   BOOL  fw_bIgnDT2;  // g_bIgnDT2 (FIX20081125)
   BOOL  fw_bChgIDT2;

// IDM_OPTADDROWNUM
   BOOL  fw_bAddRow;  // g_bAddRow
   BOOL  fw_bChgRow;

   EDITARGS fw_sZipArgs;   //   g_sZipArgs - Arguments for ZIP of a LIST
//   CMDLN fw_sZipCmd; // g_sZipCmd
   BOOL  fw_bChgZp2[ZP_MAXCMD]; // gbChgZp2

   BOOL fw_bDelOn; // gbDelOn
   BOOL fw_bChgDel; // gbChgDel

   BOOL  fw_bSimple; // gbSimple
   BOOL  fw_bChgSim; // gbChgSim

   // ID_VIEW_NOEXCLUDES
   BOOL fw_bNoExcludes; // bNoExcludes
   BOOL fw_bChgNoExcl; // bChgNoExcl
   BOOL fw_bSaveNoExcl; // FIX20091125 - if -xdall, etc, then save an OFF, bNoExcludes

}FIXEDWORK, * PFIXEDWORK;

extern	FIXEDWORK	sFW;

#ifndef USE_GLOBAL_RECURSIVE
#define	gbRecur		sFW.fw_bRecur
#endif // #ifndef USE_GLOBAL_RECURSIVE

#define	bChgRec		sFW.fw_bChgRec
#define	gbExclude   sFW.fw_bExclude
#define	bChgExcl		sFW.fw_bChgExcl

#define  gsCopyList  sFW.fw_sCopyList
#define  gsCompList  sFW.fw_sCompList  // compare list for MXCMPLST
#define  gsDiffList  sFW.fw_sDiffList  // List of files where differences written
#define  bChgDLst    sFW.fw_bChgDLst
#define  gsFileList  sFW.fw_sFileList  // List of files where file list written
#define  bChgFLst    sFW.fw_bChgFLst
#define  gsZipList  sFW.fw_sZipList  // List of files where file list written
#define  bChgZLst    sFW.fw_bChgZLst

//#define  gsExclList     sFW.fw_sExclList
#define  gsExclLists     sFW.fw_sExclLists
#define  bChgXLst    sFW.fw_bChgXLst
#define  giCurXSel   sFW.fw_iCurXSel
#define  bChgXSel    sFW.fw_bChgXSel

#define  gbWarn      sFW.fw_bWarn
#define  bChgWn      sFW.fw_bChgWn

// expanded display options
#define  gdwDiffOpts sFW.fw_dwDiffOpts // write difference options
#define  bChgDO      sFW.fw_bChgDO

#define  gdwWrapWid  sFW.fw_dwWrapWid  // wrap width for difference line output
#define  bChgWW      sFW.fw_bChgWW

#define  gdwFileOpts sFW.fw_dwFileOpts // write file list options
#define  bChgFO      sFW.fw_bChgFO

#define  gdwCpyOpts  sFW.fw_dwCpyOpts // = DEF_COPY_SET
#define  bChgCO      sFW.fw_bChgCO

#define  gdwVerFlag  sFW.fw_dwVerFlag // = DEF_VER_FLAG (Now includes REVIEW_LIST
#define  bChgVF      sFW.fw_bChgVF     // replacing gbVerify)

#define  gszDif2Fil  sFW.fw_szDif2Fil
#define  bChgDF      sFW.fw_bChgDF
#define  gszListFil  sFW.fw_szListFil
#define  bChgLF      sFW.fw_bChgLF

// FIX20091125 - add tags to list output
#define  gszTagHead  sFW.fw_gszTagHead
#define  gbChgTH     sFW.fw_gbChgTH
#define  gszTagTail  sFW.fw_gszTagTail
#define  gbChgTT     sFW.fw_gbChgTT

#define  gwarn_text  sFW.fw_warn_text  // = RGB(255,255,255);
#define  gwarn_back  sFW.fw_warn_back  // = RGB(200,30,30);
#define  bChgWT      sFW.fw_bChgWT
#define  bChgWB      sFW.fw_bChgWB

#define  ghelp_text  sFW.fw_help_text  // = RGB(0,0,0);
#define  ghelp_back  sFW.fw_help_back  // = white
#define  bChgHT      sFW.fw_bChgHT
#define  bChgHB      sFW.fw_bChgHB

#define  gbNoTT      sFW.fw_NoTT    // diable tool tip
#define  bChgTT      sFW.fw_bChgTT

#define  gszSingle   sFW.fw_szSingle   // - its NAME
#define  bChgSng     sFW.fw_bChgSng

//#define  gbVerify    sFW.fw_bVerify
//#define  bChgVer     sFW.fw_bChgVer
#define  gbXAllRepos sFW.fw_bXAllRepos  // FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS
#define  gsXDirsList sFW.fw_sXDirsList // = EXCLUDE these DIRECTORIES -xd:Scenery
#define  gsXFileList sFW.fw_sXFileList // = EXCLUDE these FILES       -xf:temp*.*;*.old
#define  gsXMissList sFW.fw_sXMissList // = EXCLUDE these FILES or DIRECTORIES
// read from INI
#define  gsXDirsIni sFW.fw_sXDirsIni // = EXCLUDE these DIRECTORIES -xd:Scenery
#define  gsXFileIni sFW.fw_sXFileIni // = EXCLUDE these FILES       -xf:temp*.*;*.old
#define  gsXMissIni sFW.fw_sXMissIni // = EXCLUDE these FILES or DIRECTORIES

#define giTabSize   sFW.fw_iTabSize
#define gbChgTab     sFW.fw_bChgTab

#define  gbMaxExp    sFW.fw_bMaxExp
#define  gbChgME     sFW.fw_gbChgME

#define  gbSetOut    sFW.fw_bSetOut
#define  gbChgSO     sFW.fw_gbChgSO

// IDM_ADDSTATUS
//TCHAR szAddS[] = "Status_Line";
#define  gbAddS      sFW.fw_bAddS
#define  gbChgAddS   sFW.fw_bChgAddS

// IDM_DIR - put up directories DIALOG
//TCHAR szAuSel[] = "AutoSelect1";
#define  gbAutoSel   sFW.fw_bAutoSel
#define  bChgASel   sFW.fw_bChgASel

// COLOURS
   //   { szClr, szcHi,  it_Color, (LPTSTR)&
#define  rgb_outlinehi   sFW.fw_rgb_outlinehi //   &bChgcHi, 0, 0 },
   //   { szOpt, szAddS, it_Bool, (LPTSTR)&gbAddS,           &gbChgAddS, 0, 0 },
#define  rgb_outlineNew  sFW.fw_rgb_outlineNew //   &bChgcNw, 0, 0 },
#define  rgb_outviewBack   sFW.fw_rgb_outviewBack   // gray

   /* expand view */
   //{ szClr, szcELF, it_Color, (LPTSTR)&
#define  rgb_leftfore   sFW.fw_rgb_leftfore  //  &bChgcELF,0, 0 },
   //{ szClr, szcELB, it_Color, (LPTSTR)&
#define  rgb_leftback   sFW.fw_rgb_leftback  //  &bChgcELB,0, 0 },
   //{ szClr, szcERF, it_Color, (LPTSTR)&
#define  rgb_rightfore  sFW.fw_rgb_rightfore //   &bChgcERF,0, 0 },
   //{ szClr, szcERB, it_Color, (LPTSTR)&
#define  rgb_rightback  sFW.fw_rgb_rightback //   &bChgcERB,0, 0 },
   /* moved lines */
   //{ szClr, szcMLF, it_Color, (LPTSTR)&
#define  rgb_mleftfore  sFW.fw_rgb_mleftfore //  &bChgcMLF,0, 0 },
   //{ szClr, szcMLB, it_Color, (LPTSTR)&
#define  rgb_mleftback  sFW.fw_rgb_mleftback //  &bChgcMLB,0, 0 },
   //{ szClr, szcMRF, it_Color, (LPTSTR)&
#define  rgb_mrightfore sFW.fw_rgb_mrightfore   //  &bChgcMRF,0, 0 },
   //{ szClr, szcMRB, it_Color, (LPTSTR)&
#define  rgb_mrightback sFW.fw_rgb_mrightback   // &bChgcMRB,0, 0 },
   /* bar window */
   //{ szClr, szcBLf, it_Color, (LPTSTR)&
#define  rgb_barleft    sFW.fw_rgb_barleft   // &bChgcBLf,0, 0 },
   //{ szClr, szcBRt, it_Color, (LPTSTR)&
#define  rgb_barright   sFW.fw_rgb_barright  //  &bChgcBRt,0, 0 },
   //{ szClr, szcBCr, it_Color, (LPTSTR)&
#define  rgb_barcurrent sFW.fw_rgb_barcurrent   //  &bChgcBCr,0, 0 },

#define  grcSize  sFW.fw_rcSize  // grcSize
#define  bSzValid sFW.fw_bSzValid   // bSzValid
#define  bChgSz   sFW.fw_bChgSz
#define  IsZoom   sFW.fw_IsZoom
#define  bChgZm   sFW.fw_bChgZm
#define  IsIcon   sFW.fw_IsIcon
#define  bChgIc   sFW.fw_bChgIc
#define  gszCopyTo   sFW.fw_szCopyTo // destination of COPY files
#define  bChgCT      sFW.fw_bChgCT     // and change

#define  editor_cmdline sFW.fw_editor_cmdline   // [264] // editor_cmdline - to run EDIT a file
#define  line_numbers   sFW.fw_line_numbers
#define  outline_include   sFW.fw_outline_include
#define  expand_mode       sFW.fw_expand_mode
#define gbChgEM	sFW.fw_bChgEM // g DWORD

#define  ignore_blanks  sFW.fw_ignore_blanks
//   { szOpt, szBnks, it_Bool, (LPTSTR)&ignore_blanks,    &bChgBks, (PVOID)IDC_IGNORESP, 0 },
//   BOOL     fw_ignore_blanks;    // ignore_blanks
#define  bChgBks  sFW.fw_bChgBks

#define  picture_mode   sFW.fw_picture_mode

#define  g_sZipFiles    sFW.fw_sZipFiles
   //TCHAR szCase[] = "IgnoreCase";
   //{ szOpt, szCase, it_Bool, (LPTSTR)&
#define  gbIgnCase      sFW.fw_bIgnCase
#define  bChgIgC        sFW.fw_bChgIgC    // (PVOID)IDC_NOCASE, 0 },

//    CONTROL         "Skip C/C++ Comments",IDC_SKIPCCOMM,"Button",
#define  gbSkipCPP      sFW.fw_bSkipCPP
#define  bChgSCPP       sFW.fw_bChgSCPP
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,54,109,10
//    CONTROL         "Skip Text within Quotes",IDC_SKIPQTXT,"Button",
#define  gbSkipTxt      sFW.fw_bSkipTxt
#define  bChgSTxt       sFW.fw_bChgSTxt
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,70,109,9
//    CONTROL         "Ignore Line Termination",IDC_IGNORETERM,"Button",
#define  gbIgnEOL       sFW.fw_bIgnEOL
#define  bChgIEOL       sFW.fw_bChgIEOL
//                    BS_AUTOCHECKBOX | WS_TABSTOP,20,84,109,11

// IDM_OPTEXACT
//TCHAR szExa[] = "Exact";
#define  gbExact        sFW.fw_bExact  // load file and compare in full
#define  bChgExa        sFW.fw_bChgExa //

// IDM_FILECOPY  Alt+U copy files per options without dialog query IDM_COPYONE does dlg.
//TCHAR  szTru[] = "SkipDialog"; // gbTrust is ON
//#define  gbTrust        sFW.fw_bTrust  // if ON, no Dialog query of COPY
//#define  gbChgTru       sFW.fw_bChgTru

#define  gbUseCSV       sFW.fw_bUseCSV // use comma delimited output
#define  gbChgCSV       sFW.fw_bChgCSV

//   case IDM_NONRS: // this is a display toggle
#ifndef   NEWCOLS2 // add or subtract columns with ease
//TCHAR szShwNums[] = "ShowLineNumbers";
#define  gbShowNums      sFW.fw_bShowNums
#define  gbChgSN        sFW.fw_bChgSN
#endif   // #ifndef   NEWCOLS2 // add or subtract columns with ease
//   case IDM_LNRS: case IDM_RNRS:
//TCHAR szUseRt[] = "UseRightNumbers"; // just the source of the number - if abv ON
#define  gbUseRight     sFW.fw_bUseRight
#define  gbChgURt       sFW.fw_bChgURt

// IDM_OPTIGNOREDT
#define gbIgnDT	sFW.fw_bIgnDT // g BOOL
#define gbChgIDT	sFW.fw_bChgIDT
// FIX20081125 - Even STRONGER
#define gbIgnDT2	sFW.fw_bIgnDT2 // g BOOL
#define gbChgIDT2	sFW.fw_bChgIDT2

// IDM_OPTADDROW
//#define gbAddRow	sFW.fw_bAddRow // g_ BOOL
//#define gbChgRow	sFW.fw_bChgRow

#define g_sZipArgs	sFW.fw_sZipArgs //   g_- Arguments for ZIP of a LIST
//   CMDLN    sCmdLn;  // various components of the command line
#define  g_sZipCmd g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
#define  gbChgZp2    sFW.fw_bChgZp2 // for [ZP_MAXCMD]
#define  g_szDirCmp  g_sZipCmd.szCmp   // current compare = ZP_DIRCMP  (5)

#define  gbDelOn    sFW.fw_bDelOn
#define  gbChgDel    sFW.fw_bChgDel

#define gbSimple	sFW.fw_bSimple // g BOOL
#define gbChgSim	sFW.fw_bChgSim // g BOOL

   // ID_VIEW_NOEXCLUDES
#define bNoExcludes sFW.fw_bNoExcludes // bNoExcludes
#define bChgNoExcl  sFW.fw_bChgNoExcl // bChgNoExcl
#define bSaveNoExcl sFW.fw_bSaveNoExcl // bSaveNoExcl

// ***** ABOVE ARE GLOBAL DEFINITIONS OF THE FIXED WORK STRUCTURE *****
#endif   // #ifndef  _TCWork_HH
// eof - TCWork.h

