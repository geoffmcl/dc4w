
// wmdiag.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef  _wmdiag_H
#define  _wmdiag_H

// exclude some frequent messages
extern   BOOL  NotInX( UINT a );

// exposed items in wmdiag.c
#define  NO_TYPE        0
#define  TB_TYPE        1
#define  CB_TYPE        2
#define  LB_TYPE        3
#define  ED_TYPE        4

// functions in wmDiag.c
extern	LPTSTR   GetWMStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );
// if _RICHEDIT_ defined, then
#ifdef   _RICHEDIT_
extern   LPTSTR   GetRENStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern   LPTSTR   GetREEvMask( DWORD dwEvMask );
#endif   // _RICHEDIT_

// WM_COMMAND notification of an EDIT control
extern   LPTSTR   GetEDStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );

// WM_NOTIFY from List View CONTROL
extern   LPTSTR   GetLVNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );

// WM_COMMAND notification of a COMBO BOX
extern   LPTSTR   GetCBNStg( UINT uType, UINT uMsg, WPARAM wParam, LPARAM lParam );

extern   BOOL  gbNoDOut;   // inhibit ALL, (but gbShwAll overrides)
extern   BOOL  gbShwAll;   // be able to SHOW EVERYTHING

//LPTSTR   Cmd2Stg( UINT uid )
typedef LPTSTR (*C2S) (UINT);
extern   VOID  SetC2s( C2S pc );

#endif   // _wmdiag_H
// eof - wmdiag.h

