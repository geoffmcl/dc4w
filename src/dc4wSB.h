

// dc4wSB.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef  _dc4wSB_H
#define  _dc4wSB_H

#ifdef   ADDSTATUS

#define		MXSBTXT		   264
#define		DEF_MX_PARTS	2     // Status + Time
#define     ID_STATUS	   2000

typedef struct	tagSB {
	HWND	sb_hStatus;
   BOOL  sb_bHidden; // TRUE when hidden
   BOOL  sb_bOff; // TRUE if done SW_HIDE
	int	sb_nPanes;
	HWND	sb_hFrame;	// OWNER window
   int   sb_iWidth;  // current owner width
   BOOL  sb_bDnLens;
   int	sb_idxTm, sb_idyTm;  // size of TIME message
   int	sb_idxLC, sb_idyLC;  // size of Ln:???? Col:???? message
	int	sb_iParts[DEF_MX_PARTS];
}SB, * PSB;

extern   int	SBCreate( HWND hwnd, PSB pSB, BOOL bHide );
extern   VOID  SBSetDefault( VOID );
extern   VOID  SBUpdateTT( LPTSTR lps );
extern   VOID  SBTimer( PSB pSB );
extern   BOOL	GetStringLens( PSB pSB );
extern   VOID  SBSetParts( PSB pSB, int iWid );
extern   void	SBUpdateTime( PSB pSB, int ih, int im );
extern   BOOL	SBSetText( PSB pSB, LPSTR lpText, int iPane );
extern   VOID  SBSetLnCol( PSB pSB, DWORD dwLn, DWORD dwCol, DWORD dwChr );
extern   void	SBSizePanes( PSB pSB, int iWidth );
extern   int	SBInit( HWND hPar, HWND hSB, PSB pSB );
extern   VOID  SBToggleHide( PSB pSB );
extern   VOID  SBHide( PSB pSB );
extern   VOID  SBShow( PSB pSB );
extern   DWORD SBSize( PSB pSB, WPARAM wParam, LPARAM lParam );

#ifdef   ADDTMDTXT
extern   VOID  SBSetTimedTxt( LPTSTR lps, DWORD dwSecs, BOOL bAddTic );
#endif   // ADDTMDTXT

extern   TCHAR gszM1[]; // [264] = { "\0" }; // current message

#endif   // ADDSTATUS
#endif   // _dc4wSB_H
// eof - dc4wSB.h