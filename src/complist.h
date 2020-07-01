
/*
 * COMPLIST.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _COMPLIST_H
#define  _COMPLIST_H

/* The file view.h includes the term COMPLIST: we need to use the term VIEW.
 * Due to MIPS limitations,the module which declares it first does
 * the real declares and the second one gets no-ops.
 */
#ifndef INC_VIEW_COMPLIST
#define INC_VIEW_COMPLIST

//typedef struct view FAR * VIEW;                 /* handle to a VIEW     */
typedef struct viewstr * PVIEW;                 /* handle to a VIEW     */
typedef struct complist FAR * COMPLIST;         /* handle to a complist */
#endif // INC_VIEW_COMPLIST

COMPLIST complist_filedialog(PVIEW view);
COMPLIST complist_dirdialog(PVIEW view);
COMPLIST complist_args( PVOID pcla );
void complist_delete(COMPLIST cl);
LIST complist_getitems(COMPLIST cl);
DWORD complist_savelist(COMPLIST cl, LPSTR savename, UINT saveopts);
BOOL  complist_copyfiles( PVOID pcfds );
DWORD complist_querytime(void);
BOOL  APIENTRY complist_open(LPSTR prompt, LPSTR ext, LPSTR spec,
        OFSTRUCT FAR *osp, LPSTR fn);
LPSTR complist_getroot_left(COMPLIST cl);
LPSTR complist_getroot_right(COMPLIST cl);
void complist_freeroot_left(COMPLIST cl, LPSTR path);
void complist_freeroot_right(COMPLIST cl, LPSTR path);

/******************************************************************************\
\******************************************************************************/
// new
extern   BOOL  complist_isfiles( COMPLIST cl );
extern   VOID  complist_setdlgstgs( PVOID pcfds, COMPLIST cl );
extern   VOID  complist_setpcfds( PVOID pcfds, INT state, DWORD dwFlag );

typedef struct tagLSTSTATS {
   LONG_PTR dwid;    // id of stats
   DWORD dwleftcnt;  // total in left
   DWORD dwrightcnt; // total in right
   DWORD dwtot;   // total in compare list
   DWORD dwsame;  // the same
   DWORD dwsamex; // diff date, but same on compare
   DWORD dwnewer; // newer
   DWORD dwolder; // older
   DWORD dwleft;  // only left
   DWORD dwright; // only right
   DWORD dwinview;   // count in view = displayable
   DWORD dwdeleted;  // excluded from list
   DWORD dwunk;   // does not belong here
}LSTSTATS, * PLSTSTATS;
extern   LSTSTATS g_sLstStats;
#define  gls   g_sLstStats

#define  g_ListTotal    gls.dwtot
#define  g_LeftCnt      gls.dwleftcnt
#define  g_RightCnt     gls.dwrightcnt
#define  g_SameCnt      gls.dwsame
#define  g_SameExa      gls.dwsamex
#define  g_NewerCnt     gls.dwnewer
#define  g_OlderCnt     gls.dwolder
#define  g_LeftOnly     gls.dwleft
#define  g_RightOnly    gls.dwright
#define  g_InView       gls.dwinview

typedef struct tagCC {
   DWORD cnt1; // total in LIST
   DWORD cnt2; // pass the compare vs state test
   DWORD cnt3; // passed actual copy test criteria
   BOOL  bUseOpt; // passing options to use for test
   DWORD dwopts;
   BOOL  bTestCpy;   // do the actual 'copy test'
   PLSTSTATS pliststats;    // to g_sLstStats, if not NULL
} CC, * PCC;

extern   VOID complist_countlistable( PCC pcc );
extern   VOID complist_countcopiable( PCC pcc );

#define  g_DirsEqual    ( gls.dwid && ( g_LeftCnt == g_RightCnt ) && \
( g_LeftCnt == g_SameCnt ) && \
( g_NewerCnt == 0 ) && (g_OlderCnt == 0) && (g_LeftOnly == 0) &&\
( g_RightOnly == 0 ) && (g_SameExa == 0) )

#endif   // _COMPLIST_H
// eof - complist.h
/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

