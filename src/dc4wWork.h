
// dc4wWork.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wWork_H
#define	_dc4wWork_H

#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

// There are TWO work blocks. One FIXED and one allocated
#define  MXXLSTS     16
#define  MXCMPLST    32    // keep most recent ?? compares
#define  MXTMPBUF    2048  // was 1024

// user actions - g_dwUseFlag
#define  uf_DnListOut   0x00000001  // written out to a list file
#define  uf_DnDiffOut   0x00000002  // written a difference file
#define  uf_DnUpdate    0x00000004  // items copied l->r, or deleted from right

typedef struct tagDDIFF {

   COMPLIST di_sCL;           // = list of file (in OUTLINE)
   COMPITEM di_sCI;           // file from a list (outline) now expanded
   TCHAR    di_szFile[264];   // output file
   BOOL     di_bWrap;         // if WRAP output lines
   DWORD    di_dwWidth;       // width of output
   DWORD    di_dwOpts;        // various BIT options
   PVOID    di_pTargs;        // back pointer to main structure
   PLE      di_pDFList;       // list of DIFF SAVE FILE list
   LPTSTR   di_pszMsg;        // pointer to message if any

}DDIFF, * PDDIFF;

// memory allocated and passed as thread arguments
#define	taCID_DifDlg	3
#define	taCID_DirDlg	2
#define	taCID_Parse		1
#define	taCID_None		0

/*
 * structure containing args passed to worker thread in initial
 * case (executing command line instructions). 
 */
//typedef  BOOL  (*ISEXP)  ( VOID );
//typedef  void  (*SETSEL) ( long );  // void  SetSelection( long rownr );
//typedef  BOOL  (*TOEXP)  ( PVOID );  // BOOL  ToExpand( HWND hwnd, BOOL bMax );
//typedef  void  (*TOOUT)  ( PVOID );  // void  ToOutline( HWND hwnd, BOOL bRest );
// stored in DIRLIST dl->dl_dwFlag = dwFlg;   // keep left and right flags
#define  tf_IsZip       0x00000001  // DIRITEM is ZIP file
#define  tf_IsLeft      0x00000002
#define  tf_IsRight     0x00000004

#define  tf_LeftisZip   0x40000000     // left argument is a ZIP file
#define  tf_RightisZip  0x80000000     // right argument is a ZIP file

#define  Left_Zip    (tf_IsZip | tf_IsLeft)
#define  Right_Zip   (tf_IsZip | tf_IsRight)
/*
 * structure containing args passed to worker thread in initial
 * case (executing command line instructions). 
 */
typedef struct tagTHREADARGS {   /* ta */

        PVIEW  view;

        INT    iSel;    // selection (if any)

        DWORD  taFlag;  // per ta_dwFlag;  // various FLAGS

        LPTSTR pFirst;     // pointer to FIRST argument
        LPTSTR pSecond;    // pointer to SECOND argument

        LPTSTR pSaveList;  // pointer to SAVE output file
        UINT   saveopts;

        LPTSTR pDiffList;  // difference file name pointer
        UINT   diffopts;   // difference options

#ifndef USE_GLOBAL_RECURSIVE
        BOOL   fDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
        BOOL   fShall;
        BOOL   fReverse;

        TCHAR  szFirst[264];
        TCHAR  szSecond[264];

        TCHAR  szDiffList[264];
        TCHAR  szSaveList[264];

        TCHAR  ta_szTmpBuf[1024];   // pta work buffer

        PVOID  pTargs;     // poiner BACK to bigger structure (w/ ptr to cfds)

}THREADARGS, * PTHREADARGS;

#define  COMBARGS    // try to COMBINE argument blocks

typedef struct tagTARGS {

	UINT	   ta_uSize;

	int	   ta_iCallID;

   WIN32_FIND_DATA   ta_sFDL;
   WIN32_FIND_DATA   ta_sFDR;

   PVOID    ta_psCFDLGSTR; // pcfds

   DDIFF    ta_sDI;     // and incorporate the DDIFF DLG structure

#ifdef   COMBARGS

   THREADARGS  ta_sTA;  // thread arguments
   THREADARGS  ta_swd_initTA;  // thread arguments from initial worker thread

#else // !#ifdef   COMBARGS

   PVIEW    ta_pView;	// active VIEW - actually a pointer to 

   INT      ta_iSel;    // selection (if any)

   DWORD    ta_dwFlag;  // various FLAGS (per above)

   LPTSTR   ta_pFirst;
   LPTSTR   ta_pSecond;

   LPTSTR   ta_pSaveList;  // pointer to SAVE output file
   UINT     ta_saveopts;

   LPTSTR   ta_pDiffList;  // difference file name pointer
   UINT     ta_diffopts;   // difference options

   BOOL     ta_fDeep;	// if recursive is CHECKED or -t on command line
   BOOL     ta_fShall;
   BOOL     ta_fReverse;

   // agument buffers
   TCHAR    ta_szFirst[ MAX_PATH+8];  // left  - first PATH
   TCHAR    ta_szSecond[MAX_PATH+8];  // right - second PATH

   TCHAR    ta_szDiffList[264];
   TCHAR    ta_szSaveList[264];

#endif   // #ifdef   COMBARGS y/n

}TARGS, FAR * PTARGS;

typedef  struct tagCLASTR {   // complist argument structure
   LPTSTR   pCaller;
   PVIEW    pView;   // pointer to the view
   LPTSTR   pLeft;   // left folder/file
   LPTSTR   pRight;  // right folder/file
#ifndef USE_GLOBAL_RECURSIVE
   BOOL     bDeep;   // the recursive flag
#endif // #ifndef USE_GLOBAL_RECURSIVE
   BOOL     bIsExp;  // was expanded before
   INT      iSel;    // previous selection
   DWORD    dwFlg;   // flag (of bits)

}CLASTR, * PCLASTR;

//#define  VERIFY_EACH    0x00000001
//#define  VERIFY_OLDER   0x00000002
//#define  MAKE_BACKUP    0x00000004
//#define  REVIEW_LIST    0x00000008
//#define  CHECK_WARN     0x00000010

//#define  DEF_VER_FLAG   (VERIFY_OLDER | MAKE_BACKUP | REVIEW_LIST | CHECK_WARN)

typedef  struct tagCFDLGSTR {

   DWORD    cf_dwSize;     // size of structure
   HWND     cf_hParent;    // parent window = hwndClient

   // state of display
   PVIEW    cf_pView;      // active VIEW pointer
   BOOL     cf_bExpanded;  // if current VIEW is expanded
   INT      cf_iSelected;  // if current SELECTION is VALID (ie not -1)
   PLE      cf_pList;      // list of files to write to

   DWORD    dwCpyOpts;  // copy option bits

   DWORD    dwVerFlag;  // verify flag

   // some list statisics
   DWORD    dwTLeft;    // total in left
   DWORD    dwTRite;    // total in right
   DWORD    dwSame;     // total the same
   DWORD    dwDiff;     // total different
   DWORD    dwNewer;    // diff AND NEWER == POSSIBLE UPDATE!!!
   DWORD    dwLeft;     // total left only
   DWORD    dwRite;     // total right only
   // FIX20081125 - NEW completely IGNORE file time, but still put in FLAG
   DWORD    dwNFTNewer; // see gbIgnDT2
   DWORD    dwNFTOlder;
   DWORD    dwNFTSame;
   DWORD    dwUnk;      // this should be ZERO

   BOOL     bSingle;       // single copy

   INT      nFiles, nFails, nSkips; // results of COPY/DELETE process

   //BOOL     bVerify;    // View and Modify LIST - see REVIEW_LIST bit above
   BOOL     bHadInit, bHadUpd;   // init and update flags
   BOOL     bDnWarn;       // first time WARNING on *** DELETE *** selected

   CPYTST   cf_sCopyTest;     // structure to TEST the COPY should function
   CPYTST   cf_sDeleTest;     // structure to TEST the DELETE should function

   LARGE_INTEGER  cf_liCopied, cf_liDeleted, cf_liToBin; // totals

   TCHAR    cf_szLeft[264];   // g_szLeft -  left directory
   TCHAR    cf_szRight[264];  // g_szRight - right directory

   TCHAR    szSingle[264]; // single selected file
   TCHAR    cf_szDest[264];   // all important DESTINATION (root)
   // NOTE: At present, this DESTINATION folder MUST exist!
   // It is typically either the LEFT or RIGHT (root) tree,
   // but can be another existing FOLDER.

   TARGS    cf_sTARGS;        // structure TARGS
   TCHAR    cf_szMsg[1024];   // general message buffer [1024]

}CFDLGSTR, * PCFDLGSTR;

#define  VALIDPCFDS(a)  ( ( a ) && ( (*a).cf_dwSize == sizeof(CFDLGSTR) ) )

//   OUTSTATS ws_sOutStats;  // g_sOutStats = outline "found files" information
typedef struct tagOUTSTATS {
   DWORD    o_dwListTot;   // total AVAILABLE for display
   DWORD    o_dwDisplayed; // total that 'meet' compare criteria
   DWORD    o_dwDExcluded; // directories in EXCLUDE THIS DIRECTORY list
   DWORD    o_dwFExcluded; // found these, but BARRED from inclusion by bExclude RULE
   DWORD    o_dwLnsperScn; // number of line 'displayable' in the 'given' client area.
}OUTSTATS, * POUTSTATS;

#define  CLROSTATS(a)   ZeroMemory(a, sizeof(OUTSTATS))

// ********************************************
// ALLOCATED WORK STRUCTURE
// ========================

typedef struct tagWORKSTR {
   DWORD          ws_dwSize;     // gdwSize

   HINSTANCE      ws_hInst;   // g_hInst -  set instance handle

   SYSTEMTIME     ws_sST1, ws_sST2; // g_sST1, g_sST2

   TIME_ZONE_INFORMATION   ws_sTZ;  // g_sTZ    // TIME_ZONE_INFORMATION
   DWORD                   ws_dwTZID;  // g_dwTZID

   //LPTSTR   ws_pDifName;      // g_pDifName
   BOOL     ws_bNoUpdate;     // g_bNoUpdate

   BOOL     ws_bInExpand;  // g_bInExpand
   INT      ws_bListSave;  // g_bListSave

   DWORD    ws_dwUseFlag;  // g_dwUseFlag
   BOOL     ws_bNoExp;     // g_bNoExp    // -E switch - do NOT expand if 1

   BOOL     ws_bComCtl4;      // g_bComCtl4
   BOOL     ws_bGotTT;        // g_bGotTT
   HWND     ws_hwndTT;        // g_hwndTT
   TOOLINFO ws_sTI;           // g_sTI

   DWORD    ws_dwScDirs;   // g_dwScDirs
   DWORD    ws_dwScFiles;  // g_dwScFiles
   DWORD    ws_dwLeftDirs;   // g_dwLeftDirs
   DWORD    ws_dwLeftFiles;  // g_dwLeftFiles
   DWORD    ws_dwRightDirs;   // g_dwRightDirs
   DWORD    ws_dwRightFiles;  // g_dwRightFiles

    BOOL ws_bNOTRecursive;  // g_bNOTRecursive - take over from the 'confusing' bDeep used

   TCHAR    ws_strTT[1024];    // g_strTT    // current string
   TCHAR    ws_cTTOut[1024];   // g_cTTOut   // for outline selection
   TCHAR    ws_cTTExp[1024];   // g_cTTExp   // when single expansion

   WIN32_FIND_DATA   ws_sFDL; // g_sFDL   // global WIN32_FIND_DATA pads
   WIN32_FIND_DATA   ws_sFDR; // g_sFDR

   UINT_PTR  ws_uiTimerID;     // g_uiTimerID = SetTimer()

   // FIX20051204 -I<file|filemask> as INPUT file & Active FILE MASK FIX20051203
   LIST_ENTRY  ws_sInFiles;   // g_sInFiles - as XLST - from -I<mask> or data\root* input
   // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
   LIST_ENTRY  ws_sInDirs;   // g_sInDirs - as XLST - from -I<mask> or data\root* input

   TCHAR    ws_szActZip[264]; // g_szActZip - Currently ACTIVE zip
   TCHAR    ws_szCurZip[264]; // g_szCurZip

   // used by RENAME ZIP DIALOG
   TCHAR    ws_szRenZip[264]; // g_szRenZip - New NAME for ACTIVE zip
   TCHAR    ws_szDlgZip[264]; // g_szDlgZip - rename dialog start name
   TCHAR    ws_szZipRoot[264]; // g_szZipRoot - New DRIVE
   TCHAR    ws_szZipRel[264]; // g_szZipRel - full relative NAME
   TCHAR    ws_szZipTit[264]; // g_szZipTit - File NAME only

   //LIST_ENTRY  ws_sZipFiles;     // g_sZipFiles - List of FILES in ACTIVE ZIP

   CLASTR   ws_sCLA;          // g_sCLA - complist_args structure

   CFDLGSTR ws_sCFDLGSTR;     // g_sCFDLGSTR - As part of ALLOCATED work structure

   BOOL  ws_bCmdError;  // g_bCmdError
   BOOL  ws_bAskCont;   // g_bAskCont  // set for timer to find
   LPTSTR ws_lpAskMsg;  // g_lpAskMsg
   DWORD ws_dwZipMax;  // g_dwZipMax
   PTSTR ws_pZipInfo;  // g_pZipInfo

   TCHAR ws_szNxtDif[MXFNB];  // g_szNxtDif

	TCHAR	ws_szLeftName[MXFNB];	// = gszLeftName  // [MXFNB];
	TCHAR	ws_szRightName[MXFNB];	// = gszRightName

   TCHAR ws_szNewPath[MXFNB]; // g_szNewPath[264] build the DESTINATION FULL file name
   //TCHAR ws_szDifFil[MXFNB];  // gszDifFil

	TCHAR ws_Body1[MXFNB];
	TCHAR ws_Ext1[MXFNB];
	TCHAR ws_Body2[MXFNB];
	TCHAR ws_Ext2[MXFNB];

   TCHAR ws_szBuf1[MXFNB];    // g_szBuf1
   TCHAR ws_szBuf2[MXFNB];    // g_szBuf2

   TCHAR ws_szTmpBuf[MXTMPBUF];   // gszTmpBuf  - enough for TWO full PATH names
   TCHAR ws_szTmpBuf2[MXTMPBUF];   // gszTmpBuf2

   TCHAR ws_szMsg[MXTMPBUF];      // g_szMsg

   THREADARGS ws_sThreadArgs; // g_sThreadArgs (PTHREADARGS)
   EDITARGS ws_sZAwd_init;   // g_sZAwd_init
   EDITARGS ws_sZipUp;   //   g_sZipUp - Temporary arguments for ZIP
   CMDLN ws_sCmdLn[8];  // g_sCmdLn
   // project/session FindFirstFile/FindNextFile totals
   OUTSTATS ws_sOutStats;  // g_sOutStats = outline "found files" information

#ifdef   ADDSTATUS
   SB       ws_sSB;     // g_sSB - status bar
#endif   // #ifdef   ADDSTATUS
   TCHAR ws_szCmd[MXTMPBUF];      // g_szCmd

   TCHAR ws_szNxtBuf[ MXFNB * MXTMPBUFS ];
   int   ws_iNxtBuf;

}WORKSTR, * PWORKSTR;

extern	PWORKSTR	pW;

#define		W	(*pW)

#define  g_hInst        W.ws_hInst  // set instance handle

// left and right ROOT compare names (in full)
#define	gszLeftName 	W.ws_szLeftName		// [MXFNB];
#define	gszRightName	W.ws_szRightName

#define  g_sST1         W.ws_sST1
#define  g_sST2         W.ws_sST2
#define  gszTmpBuf      W.ws_szTmpBuf  // [1024] // enough for TWO full PATH names
#define  gszTmpBuf2     W.ws_szTmpBuf2 // [1024]

//#define  gszDifFil      W.ws_szDifFil  // [MXFNB]

#define  gdwSize        W.ws_dwSize

#define  gszBody1       W.ws_Body1     // [MXFNB];
#define  gszExt1        W.ws_Ext1      // [MXFNB];
#define  gszBody2       W.ws_Body2     // [MXFNB];
#define  gszExt2        W.ws_Ext2      // [MXFNB];

//#define  g_pDifName     W.ws_pDifName  // 'temporary DIFFERENCE name (allocated)
#define  g_szNxtDif     W.ws_szNxtDif  // 'next' difference file

#define  g_sCFDLGSTR    W.ws_sCFDLGSTR    // structure of information on current view
#define  g_pcfds        &g_sCFDLGSTR      // which can be addressed through global pointer
// and the LIST statistics - kept at a global level of address
#define  g_dwTLeft   g_sCFDLGSTR.dwTLeft  // total in left
#define  g_dwTRite   g_sCFDLGSTR.dwTRite  // total in right
#define  g_dwSame    g_sCFDLGSTR.dwSame   // total the same
#define  g_dwNewer   g_sCFDLGSTR.dwNewer  // diff AND NEWER == POSSIBLE UPDATE!!!
#define  g_dwOlder   g_dwDiff - g_dwNewer
#define  g_dwDiff    g_sCFDLGSTR.dwDiff   // total different
#define  g_dwLeft    g_sCFDLGSTR.dwLeft   // total left only
#define  g_dwRite    g_sCFDLGSTR.dwRite    // total right only
#define  g_dwUnk     g_sCFDLGSTR.dwUnk    // this should be ZERO

#define  mw2         g_sCFDLGSTR
#define  g_szLeft    mw2.cf_szLeft   // global - left directory
#define  g_szRight   mw2.cf_szRight  // global - right directory

#define  g_bNoUpdate    W.ws_bNoUpdate    // in a cycle - limit actions

// TOOLTIP things
// =======================================
#define  g_bGotTT       W.ws_bGotTT
#define  g_bComCtl4     W.ws_bComCtl4
#define  g_hwndTT       W.ws_hwndTT
#define  g_sTI          W.ws_sTI
#define  g_strTT        W.ws_strTT
#define  g_cTTOut       W.ws_cTTOut   // for outline selection
#define  g_cTTExp       W.ws_cTTExp   // when single expansion
// =======================================

#define  g_uiTimerID    W.ws_uiTimerID // = SetTimer()

#define  g_sFDL         W.ws_sFDL   // global WIN32_FIND_DATA pads
#define  g_sFDR         W.ws_sFDR

#define  g_sCLA         W.ws_sCLA   // complist_args structure

#define  g_szBuf1       W.ws_szBuf1
#define  g_szBuf2       W.ws_szBuf2

#define  g_szMsg        W.ws_szMsg  // [1024] bytes

#define  g_szActZip     W.ws_szActZip  // [264] - Currently ACTIVE zip

#define  g_szCurZip     W.ws_szCurZip  // [264] g_

#define  g_szDlgZip     W.ws_szDlgZip  // [264] g_
#define  g_szRenZip     W.ws_szRenZip // g_ - New NAME for ACTIVE zip [264]
#define  g_szZipRoot W.ws_szZipRoot // [264]; // g_szZipRoot - New DRIVE
#define g_szZipRel	W.ws_szZipRel // g_- full relative NAME TCHAR [264]
#define g_szZipTit	W.ws_szZipTit // g_- File NAME only TCHAR [264]

// moved to FIXED structrue, since is only a double link pointer
// #define  g_sZipFiles    W.ws_sZipFiles // List of FILES in ACTIVE ZIP

// All translations between UTC time and local time are based on the following formula: 
// UTC = local time + bias 
// The bias is the difference, in minutes, between UTC time and local time. 
//typedef struct _TIME_ZONE_INFORMATION { 
//    LONG       Bias; 
//    WCHAR      StandardName[ 32 ]; 
//    SYSTEMTIME StandardDate; 
//    LONG       StandardBias; 
//    WCHAR      DaylightName[ 32 ]; 
//    SYSTEMTIME DaylightDate; 
//    LONG       DaylightBias; 
//} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION;
#define  g_sTZ       W.ws_sTZ    // TIME_ZONE_INFORMATION
#define  g_dwTZID    W.ws_dwTZID // DWORD  One of 
// TIME_ZONE_ID_UNKNOWN The system cannot determine the current time zone. This 
//  error is also returned if you call the SetTimeZoneInformation function and 
//  supply the bias values but no transition dates. 
//  Windows NT/2000: This value is returned if daylight saving time is not used 
//  in the current time zone, because there are no transition dates. 
// TIME_ZONE_ID_STANDARD The system is operating in the range covered by the 
//  StandardDate member of the TIME_ZONE_INFORMATION structure. 
//  Windows 95: This value is returned if daylight saving time is not used in the 
//  current time zone, because there are no transition dates. 
//TIME_ZONE_ID_DAYLIGHT The system is operating in the range covered by the 
//  DaylightDate member of the TIME_ZONE_INFORMATION structure

#define  g_bNoExp    W.ws_bNoExp    // -E switch - do NOT expand if 1

//   OUTSTATS ws_sOutStats;  // g_sOutStats = outline "found files" information
#define  g_sOutStats W.ws_sOutStats    // = outline "found files" information
#define  g_dwListTot g_sOutStats.o_dwListTot // total AVAILABLE for display

#ifdef   ADDSTATUS
#define  g_sSB       W.ws_sSB    // = status bar
#endif   // #ifdef   ADDSTATUS

#define  g_bAskCont  W.ws_bAskCont  // set for timer to find
#define  g_lpAskMsg  W.ws_lpAskMsg
#define  g_bCmdError W.ws_bCmdError

#define  g_szNewPath W.ws_szNewPath // [264];  // build the DESTINATION FULL file name

#define g_bInExpand	W.ws_bInExpand // g_ BOOL
#define  Dec_g_bInExpand   if(g_bInExpand){g_bInExpand--;}

#define g_bListSave	W.ws_bListSave // g_ INT

#define  g_dwUseFlag W.ws_dwUseFlag

#define  g_sZipUp W.ws_sZipUp // g_ - Temporary arguments
#define  g_sZCmdLn   g_sZipUp.sCmdLn   // ZIPUP (allocated) arguments
#define  g_sCmdLn W.ws_sCmdLn  // g_sCmdLn [8] array

#define g_sThreadArgs	W.ws_sThreadArgs // g_(PTHREADARGS)
#define g_sZAwd_init	W.ws_sZAwd_init // g_(PEDITARGS) - from wd_init

// FIX20051203 & FIX20051204 - allow data/root* and -Ifilename
#define g_sInFiles W.ws_sInFiles   // g_sInFiles - as XLST - from -I<mask> or data\root* input
// FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
#define g_sInDirs W.ws_sInDirs   // g_sInDirs - as XLST - from -I<mask> or data\root* input

// FIX20060219 - max length, and pointer to ZIP info
#define  g_dwZipMax  W.ws_dwZipMax  // g_dwZipMax
#define  g_pZipInfo  W.ws_pZipInfo  // g_pZipInfo

#define  g_dwScDirs  W.ws_dwScDirs
#define  g_dwScFiles W.ws_dwScFiles
#define  g_dwLeftDirs W.ws_dwLeftDirs
#define  g_dwLeftFiles W.ws_dwLeftFiles
#define  g_dwRightDirs W.ws_dwRightDirs
#define  g_dwRightFiles W.ws_dwRightFiles

#define  g_szCmd  W.ws_szCmd // [MXTMPBUF]; // g_szCmd

#define  g_szNxtBuf  W.ws_szNxtBuf  // [ MXFNB * MXTMPBUFS ]
#define  g_iNxtBuf   W.ws_iNxtBuf

// FIX20091224 - take over from bDeep
#define  g_bNOTRecursive  W.ws_bNOTRecursive // FIX20091224 // g_bNOTRecursive (also g_bArgsDeep and 

#ifdef  __cplusplus
// extern  "C"
}
#endif  // __cplusplus

#endif	// #ifndef	_dc4wWork_H
// eof - dc4wWork.h
