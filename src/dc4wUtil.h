
// dc4wUtils.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wUtils_hh
#define	_dc4wUtils_hh

#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus
// moved here Jan 2012
/*
 * a FILEBUFFER handle is a pointer to a struct filebuffer
 */
#define ff_found_zero       0x0001

#define MMX_LINE_BUF 512
struct filebuffer {
        int fh;         /* open file handle */
        PSTR start;     /* offset within buffer of next character */
        PSTR last;      /* offset within buffer of last valid char read in */
        int file_flag;       /* set 0x0001 if binary */
        char buffer[MMX_LINE_BUF];
};

// added April, 2001
#define  MFH         HANDLE
#define  MEOR        "\r\n"
#ifndef  VFH
#define  VFH(a)      ( a && ( a != INVALID_HANDLE_VALUE ) )
#endif   // !VFH

typedef struct tagFOUNDLIST {
   LIST_ENTRY  sList;   // at head
   TCHAR       szDir[264];
   TCHAR       szFile[264];
   TCHAR       szExt[264];
   WIN32_FIND_DATA   sFD;
}FOUNDLIST, * PFOUNDLIST;

extern   DWORD  dc4wProcessDir( LPTSTR lpdir, PLE pFileList, BOOL bReCur, DWORD level );
extern   BOOL  GetDirectoryOnly( LPTSTR lpb2, LPTSTR lpb );

extern	VOID     AppendDateTime( LPTSTR lpb, LPSYSTEMTIME pst );
extern   VOID     AppendDateTime2( LPTSTR lpb, LPSYSTEMTIME pst ); //add full YEAR number
extern   HANDLE   OpnFil( LPTSTR lpf, BOOL bAppend );
extern   LPTSTR	GetFDTStg( FILETIME * pft );
// as above but YEAR to 4 digits, and add seconds
extern   LPTSTR	GetFDTSStg( FILETIME * pft );
extern   int      OutStg( HANDLE fh, LPTSTR lps );
extern   DWORD    SigLen( LPTSTR lps );
extern   LPTSTR	CopyShortName( LPTSTR lps, LPTSTR lpd, DWORD siz );
extern   LPTSTR   GetDTStg( VOID );

// some data

// and some DIAGNOSTIC stuff
extern   VOID  OpenDiagFile( VOID );
extern   VOID  CloseDiagFile( VOID );
extern   int   _cdecl sprtf( LPTSTR lpf, ... );
extern   int   _cdecl chkme( LPTSTR lpf, ... );

// some timing stuff
extern   VOID  InitTimers( VOID );
//extern int  GetTimer( PGTM * pptm )
extern   int      SetBTime( void );
extern   double   GetETime( int i );
extern   double   GetRTime( int i );

extern   LPTSTR   Dbl2Str( double factor, int prec );

extern   INT      ConditionText( LPTSTR lpt );
extern   VOID     ToggleBool( PBOOL pBool, PBOOL pChg, BOOL flg );
extern   VOID     ToggleBit( PDWORD pdwFlag, DWORD dwBit, PBOOL pChg, BOOL flg );
extern   BOOL     ChangedWP( WINDOWPLACEMENT * pw1, WINDOWPLACEMENT * pw2 );
extern   LPTSTR	GetFDStg( FILETIME * pft );
extern   LPTSTR	GetFTStg( FILETIME * pft );
extern   INT      InStr( LPTSTR lpb, LPTSTR lps );  // extract FixFObj.c/fc4.c/...
extern   LPTSTR   Left( LPTSTR lpl, DWORD dwi );
extern   LPTSTR   Right( LPTSTR lpl, DWORD dwl );
extern   LPTSTR   Mid( LPTSTR lpl, DWORD dwb, DWORD dwl );
extern   LPTSTR   RetDiffStg( LPTSTR lps, LPTSTR lpd );

#ifdef   REV_TO_OLD
extern   BOOL	   MatchFiles( LPTSTR lp1, LPTSTR lp2 );
#endif   // #ifdef   REV_TO_OLD

extern   BOOL	   MatchWild( LPTSTR lp1, LPTSTR lp2 );
extern   BOOL	   MatchWild2( LPTSTR lp1, LPTSTR lpb2, LPTSTR lpe2 );
extern   BOOL	   SplitExt2( LPTSTR lpb, LPTSTR lpe, LPTSTR lpf );
extern   VOID     Setg_szNxtDif( VOID );
extern   BOOL	   WildComp2( LPTSTR lp1, LPTSTR lp2 );

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : TrimIB
// Return type: DWORD 
// Argument   : LPTSTR lps
// Description: Remove spacey stuf from BEGINNING and TAIL
//              NOTE WELL: Buffer is ALTERED, hence "IB"=IN BUFFER
///////////////////////////////////////////////////////////////////////////////
extern   DWORD    TrimIB( LPTSTR lps );
// above is actually made up of
extern   DWORD    LTrimIB( LPTSTR lps );  // and
extern   DWORD    RTrimIB( LPTSTR lps );

extern   LPTSTR   GetI64Stg( PLARGE_INTEGER pli ); // I64 sprintf
extern   LPTSTR   GetI64StgRLen( PLARGE_INTEGER pli, DWORD dwl ); // right justified
// DEC *=*=*=*=* GetI64StgRLen2 - FIX20010724 =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// As above, but BIG values are reduced to KB, or MB. NOTE: K=1024
extern   LPTSTR   GetI64StgRLen2( PLARGE_INTEGER pli, DWORD dwl );

extern   LPTSTR   GetRelNameStg( COMPITEM ci );
extern   BOOL     Chk4Debug( LPTSTR lpd );

extern   PLE      Add2SList( PLE ph, PBOOL pb, LPTSTR lps );

extern   int      MB( HWND hWnd, LPTSTR lpText, LPTSTR lpCaption, UINT uType );

extern   VOID     EnsureCrLf( LPTSTR lps );
extern   VOID     EnsureCrLf2( LPTSTR lps );

extern   void     SplitFN( LPTSTR pPath, LPTSTR pFile, LPTSTR pFullName );
extern   VOID     GetModulePath( LPTSTR lpb );
extern LPTSTR GetModulePathStg( VOID ); // return module path, in a buffer GetStgBuf()
extern LPTSTR GetCWDStg( VOID ); // return CWD in buffer
extern   VOID     GetNxtDif( LPTSTR lpf );

// convert UTC FILETIME to local FILETIME and then
//     local   FILETIME to local SYSTEMTIME
extern   BOOL     FT2LST( FILETIME * pft, SYSTEMTIME * pst );

extern   LPTSTR   SaveOpts2Stg( DWORD dwo, BOOL bLong );
extern   DWORD    GetSaveBits( INT chr );

extern   VOID     GetTZI( VOID );
extern   LPTSTR   GetStgBuf( void );
extern   LPTSTR   Rect2Stg( PRECT pr );

extern   BOOL	GotWild( LPTSTR lps );  // check for '*' or '?'

extern PTSTR GetNxtBuf( VOID );

extern void GetAppData(PTSTR lpini);


#ifdef  __cplusplus
// extern  "C"
}
#endif  // __cplusplus

#endif	// _dc4wUtils_hh
// eof - dw4wUtils.h

