

// dcw4TT.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wTT_H
#define	_dc4wTT_H

// a COMPITEM flag can be modified by
//DWORD compitem_addflag( COMPITEM ci, DWORD dwf )
//DWORD compitem_subflag( COMPITEM ci, DWORD dwf )
// each of which return the resultant flag of bits
// and DWORD compitem_getflag(COMPITEM ci) get the FLAG

#define  TT_LEFT     0x00000001
#define  TT_RIGHT    0x00000002
#define  TT_BOTH     ( TT_LEFT | TT_RIGHT )

#define  TT_SMALLER  0x00000080
#define  TT_LARGER   0x00000040
#define  TT_YOUNGER  0x00000020
#define  TT_OLDER    0x00000010

// === other flags of a COMPITEM = pair of files ===
#define  TT_VIEWED   0x00000100  // add if COMITEM viewed recently
#define  TT_HADDIFFS 0x00000200  // and if 'differences' found
#define  TT_DATEDIFF 0x00000400  // date diff = may skip on line x line compare
// === FIX20081125 - NEW completely IGNORE file time - gbIgnDT2 ===
#define  TT_DATEOLD  0x00001000
#define  TT_DATENEW  0x00002000

#define  TT_GOTZERO  0x20000000  // in exact compare, found ZERO in file data
#ifdef ADD_LIST_VIEW
#define  TT_INLISTVW 0x40000000  // have added COMPITEM to LISTVIEW
#endif // #ifdef ADD_LIST_VIEW

// not used yet!!!
#define  TT_MARKED   0x80000000  // marked for exclusion

#define  TT_FLAGS (TT_LEFT|TT_RIGHT|TT_SMALLER|TT_LARGER|TT_YOUNGER|TT_OLDER)

extern	BOOL  IsComCtl32400( VOID );

extern   BOOL  CreateMyTooltip( HWND hwnd );

extern   DWORD AddToolText( LPTSTR lptstr, COMPITEM ci );

#endif	// _dc4wTT_H
// eof - dwc4TT.h
