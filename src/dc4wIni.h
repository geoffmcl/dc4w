// dc4wIni.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wIni_H
#define	_dc4wIni_H

typedef  struct   tagXLST {
   LIST_ENTRY  x_List;  // first LIST entry
   DWORD       x_dwFlag;   // flag of item
   TCHAR       x_szBody[264]; //	SplitExt2( lpb1, lpe1, lp1 );
   TCHAR       x_szExt[264];  // SplitExt2( lpb2, lpe2, lp2 );
   TCHAR       x_szStg[1]; // string
}XLST, * PXLST; // FIX20051204 - this ALSO used as -I input LIST

extern	VOID  ReadDc4wINI( VOID );
extern	VOID  WriteDc4wINI( VOID );

extern	VOID  WriteINI( LPTSTR lpini, BOOL bNoReset );
extern	VOID  ReadINI( LPTSTR lpini );

extern   BOOL  bChgLnN;
extern   BOOL  bChgEd;
extern   BOOL  bChgInc;
extern   BOOL  bChgBks;
extern   BOOL  bChgPic;
extern   TCHAR gszLeft[]; // [264] = { "\0" };
extern   TCHAR gszRite[]; // [264] = { "\0" };
extern   BOOL  bChgLf, bChgRt;

extern   BOOL  gbGotOut;
extern   WINDOWPLACEMENT   g_sWPO;
extern   BOOL  bChgOut;

extern   BOOL  gbGotExp;
extern   WINDOWPLACEMENT   g_sWPE;
extern   BOOL  bChgExp;

#define  IS_FILE        1
#define  IS_DIR         2

extern   BOOL  g_bLVOff;   // current STATE of the LISTVIEW control
extern   BOOL  g_bUsrLVOff; // user's INI choice, toggled by menu, and
extern   BOOL  g_bChgLVOff; // the change flag for the above

extern PVOID Add2StgList( PLE pH, LPTSTR lpb );

extern INT Add_Excl_List( PTSTR lpb2, PLE pH, PTSTR sep); // ini service - list back to ONE LINE ; sepped

#endif	// _dc4wIni_H
// eof - dc4wIni.h
