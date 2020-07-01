
/*
 * COMPITEM.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _COMPITEM_H
#define  _COMPITEM_H

/* handle to a CompItem */
typedef struct compitem FAR* COMPITEM;

COMPITEM compitem_new(DIRITEM left, DIRITEM right, LIST list, BOOL fExact);
void compitem_delete(COMPITEM item);
LIST compitem_getcomposite(COMPITEM item);
void compitem_discardsections(COMPITEM item);
LIST compitem_getleftsections(COMPITEM item);
LIST compitem_getrightsections(COMPITEM item);
FILEDATA compitem_getleftfile(COMPITEM item);
FILEDATA compitem_getrightfile(COMPITEM item);
int compitem_getstate(COMPITEM item);
LPSTR compitem_gettext_tag(COMPITEM item);
LPSTR compitem_gettext_result(COMPITEM item);
LPSTR compitem_getfilename(COMPITEM item, int option);
void compitem_freefilename(COMPITEM item, int option, LPSTR filename);

extern   DWORD compitem_getflag( COMPITEM ci ); // get compare FLAG(s)
extern   LPTSTR compitem_gettext_tag2(COMPITEM item); // remove root .\ from file name
extern   LPTSTR compitem_gettext_tag3(COMPITEM item); // remove relative path from files
extern   LPTSTR compitem_gettext_tags(COMPITEM item); // pre gdwFileOpts flag

/*
 * options for compitem_getfilename, indicating which name is desired
 */
#define CI_LEFT         1       /* name of left file */
#define CI_RIGHT        2       /* name of right file */
#define CI_COMP         3       /* name of composite file */

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

// NEW
extern   VOID   compitem_savediff(COMPITEM ci, LPTSTR savename, UINT options);
extern   LPTSTR compitem_getdiffname( COMPITEM item, DWORD dwOpts, PVOID pv );
extern   VOID   compitem_freediffname(LPTSTR filename);
extern   DWORD  compitem_writediff( COMPITEM ci, HANDLE * pfh, DWORD dwOpts, PVOID pv );
extern   BOOL   compitem_settargs( COMPITEM ci, PVOID pv );

extern   DWORD    compitem_addflag( COMPITEM ci, DWORD dwf );
extern   DWORD    compitem_subflag( COMPITEM ci, DWORD dwf );

extern   DWORD compitem_addflag2( COMPITEM ci, DWORD dwf, LRESULT index );   // LISTVIEW index
extern   DWORD compitem_getindex( COMPITEM ci );   // and to GET the index BACK

#ifdef ADD_LIST_VIEW
extern   LRESULT  compitem_addLV( COMPITEM ci );
#define  ADDLISTVIEW(a) compitem_addLV(a) // returns INDEX of item, or -1 if ERROR
#endif // #ifdef ADD_LIST_VIEW

extern   LPTSTR   compitem_getcoltext( COMPITEM ci, DWORD ccol );
extern   LPTSTR   compitem_getcol0text( COMPITEM ci );   // get the FILE NAME (no path)
// note: if they are the SAME, then only a pointer to the LEFT item is returned,
// else a composite name of "%s : %s" formed, and returned.

#endif   // _COMPITEM_H
// eof - compitem.h

