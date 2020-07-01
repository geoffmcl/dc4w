
// TCWork.h
#ifndef  _TCWork_HH
#define  _TCWork_HH

//#define  MALLOC(a)      new BYTE[a]
#define  MALLOC(a)      LocalAlloc( LPTR, a );
#define  MFREE(a)       LocalFree(a)

#define  MXXLSTS     16
#define  MXCMPLST    32
#define IDM_LNRS                        40021
#define IDM_RNRS                        40022
#define IDM_NONRS                       40023

/* Outline_include is an OR of the following */
#define  INCLUDE_SAME            0x00000001
#define  INCLUDE_DIFFER          0x00000002
#define  INCLUDE_LEFTONLY        0x00000004
#define  INCLUDE_RIGHTONLY       0x00000008
// special - used only in update
#define  DELETE_ONLY             INCLUDE_RIGHTONLY

// and expanded has moved line, so OR of the following
#define  INCLUDE_MOVELEFT        0x00000010
#define  INCLUDE_MOVERIGHT       0x00000020
// If master DIFFER on, then separate NEWER / OLDER
#define  INCLUDE_NEWER           0x00000040
#define  INCLUDE_OLDER           0x00000080

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

/* Copy file options are either COPY_FROMLEFT or COPY_FROMRIGHT 
 * (indicating which  * tree is to be the source of the files), plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT (INCLUDE_LEFT
 * and INCLUDE_RIGHT are treated the same here since the COPY_FROM* option
 * indicates which side to copy from). */
#define  COPY_FROMLEFT           0x00100000  /* copy files from left tree */
#define  COPY_FROMRIGHT          0x00200000  /* copy files from right tree */

// and some common combinations
#define  INC_ALLONLY    (INCLUDE_LEFTONLY | INCLUDE_RIGHTONLY)
#define  INC_ALLMOVE    (INCLUDE_MOVELEFT | INCLUDE_MOVERIGHT)
#define  INC_OTHERS     (INCLUDE_LINENUMS | INCLUDE_TAGS | APPEND_FILE | \
                         INCLUDE_HEADER   | WRAP_LINES   | FULL_NAMES  | \
                         FLEFT_NAME       | ADD_COMMENTS )
#define  INC_OUTLINE    (INCLUDE_SAME|INCLUDE_DIFFER|INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY)
#define  INC_OUTLINE2   (INC_OUTLINE|INCLUDE_NEWER|INCLUDE_OLDER)

// in expanded mode, INCLUDE_DIFFER is redundant since each line
// is either same, left, right or moved!
#define  INC_ALLXSAME   (INC_ALLONLY|INC_ALLMOVE|INC_OTHERS)
// in outline mode, files are either same, differ, left or right
#define  INC_ALLXSM     (INCLUDE_DIFFER|INC_ALLONLY|INC_OTHERS)
// default copy options
//#define  DEF_COPY_SET   (COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER)
#define  DEF_COPY_SET   (COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER|INCLUDE_NEWER)
// int   expand_mode = IDM_BOTHFILES;
#define  INCLUDE_DIFFER2   (INCLUDE_DIFFER|INCLUDE_NEWER|INCLUDE_OLDER)

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

// FIXED WORK BLOCK - Ensure initialised to default value, before INI read
typedef	struct tagFIXEDWORK {

	BOOL  fw_bRecur;	// gbRecur
	BOOL  fw_bChgRec;	// bChgRec

	BOOL  fw_bExclude;	// gbExclude
	BOOL  fw_bChgExcl;	// bChgExcl

   DWORD fw_dwDiffOpts;    // gdwDiffOpts
   BOOL  fw_bChgDO;        // bChgDO

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

   LIST_ENTRY     fw_sCopyList;  // gsCopyList
   LIST_ENTRY     fw_sCompList;  // gsCompList - compare list for MXCMPLST
   LIST_ENTRY     fw_sDiffList;  // gsDiffList - List of files where differences written
   BOOL           fw_bChgDLst;   // bChgDLst
   LIST_ENTRY     fw_sFileList;  // gsFileList - List of files where file list written
   BOOL           fw_bChgFLst;   // bChgFLst

   LRESULT        fw_iCurXSel;   // giCurXSel
   BOOL           fw_bChgXSel;   // bChgXSel

   LIST_ENTRY     fw_sExclLists[MXXLSTS];  // gsExclLists
   BOOL           fw_bChgXLst;

   LIST_ENTRY     fw_sXDirsList;  // gsXDirsList = EXCLUDE these DIRECTORIES -xd:Scenery
   LIST_ENTRY     fw_sXFileList;  // gsXFileList = EXCLUDE these FILES       -xf:temp*.*;*.old
   BOOL           fw_bChgXDL, fw_bChgXFL;

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
   DWORD    fw_line_numbers;  // line_numbers
   DWORD    fw_outline_include;  // outline_include
   DWORD    fw_expand_mode;      // expand_mode

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

}FIXEDWORK, * PFIXEDWORK;

extern	FIXEDWORK	sFW;

#define	gbRecur		sFW.fw_bRecur
#define	bChgRec		sFW.fw_bChgRec
#define	gbExclude   sFW.fw_bExclude
#define	bChgExcl		sFW.fw_bChgExcl

#define  gsCopyList  sFW.fw_sCopyList
#define  gsCompList  sFW.fw_sCompList  // compare list for MXCMPLST
#define  gsDiffList  sFW.fw_sDiffList  // List of files where differences written
#define  bChgDLst    sFW.fw_bChgDLst
#define  gsFileList  sFW.fw_sFileList  // List of files where file list written
#define  bChgFLst    sFW.fw_bChgFLst

//#define  gsExclList     sFW.fw_sExclList
#define  gsExclLists     sFW.fw_sExclLists
#define  bChgXLst    sFW.fw_bChgXLst
#define  giCurXSel   sFW.fw_iCurXSel
#define  bChgXSel    sFW.fw_bChgXSel

#define  gbWarn      sFW.fw_bWarn
#define  bChgWn      sFW.fw_bChgWn

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
#define  gsXDirsList sFW.fw_sXDirsList // = EXCLUDE these DIRECTORIES -xd:Scenery
#define  gsXFileList sFW.fw_sXFileList // = EXCLUDE these FILES       -xf:temp*.*;*.old

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


// ***** ABOVE ARE GLOBAL DEFINITIONS OF THE FIXED WORK STRUCTURE *****
#endif   // #ifndef  _TCWork_HH
// eof - TCWork.h

