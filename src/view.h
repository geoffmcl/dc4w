/*
 * VIEW.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _VIEW_HH
#define  _VIEW_HH
/* view.h includes the term COMPLIST: complist.h uses the term VIEW.
 */
#ifndef INC_VIEW_COMPLIST
#define INC_VIEW_COMPLIST
//typedef struct view FAR * VIEW;                 /* handle to a VIEW     */
typedef struct viewstr * PVIEW;                 /* handle to a VIEW     */
typedef struct complist FAR * COMPLIST;         /* handle to a complist */
#endif // INC_VIEW_COMPLIST

#define  FIND_DOWN      TRUE
#define  FIND_UP        FALSE

PVIEW    view_new(HWND hwndTable);
BOOL     view_setcomplist(PVIEW view, COMPLIST cl, BOOL fDeep);
COMPLIST view_getcomplist(PVIEW view);
void     view_close(PVIEW view);
void     view_delete(PVIEW view);
LPSTR    view_gettext(PVIEW view, long row, int col);
int      view_getlinenr_left(PVIEW view, long row);
int      view_getlinenr_right(PVIEW view, long row);
int      view_getwidth(PVIEW view, int col);
long     view_getrowcount(PVIEW view);
int      view_getstate(PVIEW view, long row);
BOOL     view_expand(PVIEW view, long row);
COMPITEM view_getitem(PVIEW view, long row);
BOOL     view_isexpanded(PVIEW view);        
LPSTR    view_getcurrenttag(PVIEW view);
BOOL     view_newitem(PVIEW view);
void     view_changeviewoptions(PVIEW view);
void     view_changediffoptions(PVIEW view);
long     view_findchange(PVIEW view, long startrow, BOOL bForward);

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// CHANGED
extern   INT   view_outline( PVIEW view );

// NEW
extern   INT   view_haschange( PVIEW view, BOOL bCnt );
extern   BOOL  view_getrefresh( PVIEW view );
extern   BOOL  view_setrefresh( PVIEW view, BOOL flg );
extern   BOOL  view_getrecursive( PVIEW view );
extern   INT   view_getiselect( PVIEW view );
extern   BOOL  view_haszip( PVIEW view ); // TRUE if left or right is a ZIP
extern   DWORD view_getflag(PVIEW view, long row); // only in OUTLINE
extern   long  view_gettotcount(PVIEW view); // total FILES (COMITEMS) in VIEW

#endif   // _VIEW_HH
// eof - view.h
