
/*
 * SCANDIR.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _SCANDIR_HH
#define  _SCANDIR_HH

// just do not like these LONG names
#define  FD       WIN32_FIND_DATA
#define  PFD      FD *
// but for ZIP support, must also include FULL NAME buffer, so
typedef struct tagFDX {
   FD       sFD;  // set 'like' WIN32 file data is ok
   TCHAR    szPath[264];   // but need PATH as well from zip.
}FDX, * PFDX;

/* Handle to the list of files scanned */
typedef struct dirlist * DIRLIST;

/* Handle to one item within the list of files */
typedef struct diritem * DIRITEM;

#ifndef  _COMPITEM_H
/* handle to a CompItem */
typedef struct compitem * COMPITEM;
#endif   // COMPITEM

DIRLIST dir_buildlist(LPSTR pathname, BOOL bOnDemand, DWORD dwFlg);
void dir_delete(DIRLIST list);
BOOL dir_isfile(DIRLIST list);
DIRITEM dir_firstitem(DIRLIST list);
DIRITEM dir_nextitem(DIRLIST list, DIRITEM previtem, BOOL fDeep);

/* Filenames
 *
 * From a DIRITEM, you can query either the relative or the full name.
 *
 * The relative name does not include the tree root that was originally
 * passed to dir_buildlist. The full name does include this. Note however
 * that if you passed a relative name to dir_buildlist, the full
 * name you get back will not be an *absolute* pathname.
 *
 * Thus, if you call dir_buildlist with "c:\",
 * we will return:
 *      relative name:  ".\config.sys"
 *      full name:      "c:\config.sys"
 *
 * If you call dir_buildlist with ".\geraintd",
 * we will return:
 *      relative name:  ".\source\scandir.h"
 *      full name:      ".\geraintd\source\scandir.h"
 *
 * In both cases, we return a pointer to a filename string: you must
 * call dir_freefullname or dir_freerelname to free this memory when you
 * have finished with it. Depending on the implementation, one or other
 * (or possibly both) of these names will have been built specially
 * when you called the query function.
 *
 * You can also return a pointer to the tree root name. (In the above
 * examples this would be c:\ and .\geraintd). Depending on the implementation,
 * this may have been forced to an absolute path.
 *
 */

LPSTR dir_getfullname(DIRITEM item);
LPTSTR dir_getrelname(DIRITEM item);
LPSTR dir_getroot_item(DIRITEM item);
LPSTR dir_getroot_list(DIRLIST dl);
void dir_freefullname(DIRITEM item, LPSTR fullname);
void dir_freerelname(DIRITEM item, LPSTR relname);
void dir_freeroot_item(DIRITEM item, LPSTR rootname);
void dir_freeroot_list(DIRLIST dl, LPSTR rootname);
LPSTR dir_getopenname(DIRITEM item);
void dir_freeopenname(DIRITEM item, LPSTR openname);
int dir_openfile(DIRITEM item);
void dir_closefile(DIRITEM item, int fh);
long dir_getfilesize(DIRITEM item);
//BOOL dir_copy(DIRITEM item, LPTSTR pdroot, PVOID pct, PVOID pcfds);
BOOL dir_startcopy(DIRLIST dl);
int dir_endcopy(DIRLIST dl);
// return pointer to 'name' in DIRITEM
extern   LPTSTR dir_getnameptr(DIRITEM cur); // return *JUST* the file name/title ptr, or 0

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

// NEW
// NOTE: BITS in ct_dwFlag
#define  flg_Delete  0x80000000     // if the item is DELETE from the COPY list
#define  flg_User    0x40000000     // USER aborted this COPY
#define  flg_Abort   0x20000000     // USER aborted ALL COPIES
#define  flg_NotinOp 0x10000000     // do not show the item - the include option is OFF

#define  flg_DnAction   0x01000000  // have copied or deleted the file item

#define  flg_Dele1ON    0x00000008  // DELETE of ONE file chosen by USER
#define  flg_Dele1OK    0x00000004  // DELETE of ONE file is OK
#define  flg_Copy1ON    0x00000002  // USER has selected COPY SINGLE ITEM IN DIALOG
#define  flg_Copy1OK    0x00000001  // copy of ONE file is OK

typedef struct tagCPYTST {
   LIST_ENTRY  ct_List;       // linked list - MUST BE FIRST

   PVOID    ct_pcfds;         // pointer back to primary structure

   INT      ct_iState;        // state of the ITEM
   COMPITEM ct_ci;            // appropriate COMPITEM
   DIRITEM  ct_diritem;       // the appropriate directory item (LEFT or RIGHT depends!)
   DWORD    ct_dwFlag;        // various flags per above

   // filled in during COPY-TEST
   // source
   TCHAR    ct_szCopyFull[264];  // FULL name of COPY item
   TCHAR    ct_szCopyRel[264];   // RELATIVE name of COPY item
   TCHAR    ct_szSTitle[264];    // and the file title (after CopyTest())
   BOOL     ct_bFound;           // TRUE if source file FOUND
   WIN32_FIND_DATA ct_sFDSrc;    // SOURCE find file information

   // destination
   TCHAR    ct_szCopyTo[264];    // FULL DESTINATION name of COPY item
   TCHAR    ct_NewDir[264];      // NEW DIRECTORY to CREATE (if any)
   BOOL     ct_bDestValid;       // TRUE if the destination find file info is VALID
   WIN32_FIND_DATA ct_sFDDst;    // DESTINATION find directory information

   BOOL     ct_bDestFound; // there is an exosting destination file
   BOOL     ct_bDestRO;    // destination found to be READ ONLY
   WIN32_FIND_DATA ct_sFDCpy;    // DESTINATION file information (used by COPY function)

   UINT     ct_uiIndex;          // index in LIST view

}CPYTST, * PCPYTST;

// previous function made PUBLIC
extern   BOOL  dir_isvalidfile(LPTSTR path);

// function added April, 2001
extern   PWIN32_FIND_DATA  dir_getpfd(DIRITEM item);
extern   BOOL  dir_isvalidfile2(LPTSTR path, PWIN32_FIND_DATA pfd);

extern   BOOL  dir_copy(PVOID pct, PVOID pcfds);
extern   BOOL  dir_copytest(PVOID pcfds, PCPYTST pct);
extern   BOOL  dir_deletetest( PCPYTST pct, DIRITEM diritem );

extern   BOOL     dir_iszip( DIRITEM di );   // return state of IsZip FLAG for item
extern   INT      dir_openzip( PVOID pozf ); // extract file data from a ZIP
extern   BOOL     dir_iszipfile( LPTSTR lpf );
extern   BOOL     dir_setzipflag( PDWORD pdw, LPTSTR lplf, LPTSTR lprf );


#endif   // #ifndef  _SCANDIR_HH
// eof - scandir.h
