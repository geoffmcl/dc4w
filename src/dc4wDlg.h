
// dc4wDlg.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wDlg_H
#define	_dc4wDlg_H

extern	INT_PTR  Do_DIFF_Dlg( PVOID pta );  // actually PTARGS
extern   INT_PTR  Do_DIR_DLG( VOID );
extern   INT_PTR  Do_SAVELIST_DLG( PVOID pv );
extern   INT_PTR  Do_COPYFILES_DLG( PVOID pv );
extern   INT_PTR  MB2( HWND hWnd,          // handle to owner window
  LPTSTR lpText,     // text in message box
  LPTSTR lpCaption,  // message box title
  UINT uType         // message box style
);

// VERIFY FILE DELETE DIALOG
extern
INT_PTR MB_DELETE( HWND hWnd,          // handle to owner window
  LPTSTR lpText,     // text in message box
  LPTSTR lpCaption,  // message box title
  UINT uType );         // message box style

//#define  CDB(a,b) CheckDlgButton( hDlg, a, ( b ? BST_CHECKED : BST_UNCHECKED ) )
#define  CDB(a,b,c)\
   CheckDlgButton( hDlg, a, ( (b & c) ? BST_CHECKED : BST_UNCHECKED ) )

#define  ICDB2(a,b,c)\
   if( IsDlgButtonChecked( hDlg, a ) == BST_CHECKED )\
      b |= c;\
   else\
      b &= ~(c)

#define  ICDB(a,b,c) ICDB2(a,b,c)

#endif	// #ifndef	_dc4wDlg_H
// eof - dc4wDlg.h
