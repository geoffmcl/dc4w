
/****************************** Module Header *******************************
* Module Name: SCANDIR.C
*
* Scan a directory tree and build a sorted list of filenames within that
* tree.
*
* Functions:
*
* dir_buildlist() - given a path or file build list from dl->dl_pdot(.) down
* dir_delete()
* dir_isfile()
* dir_firstitem()
* dir_nextitem()
* dir_findnextfile()
* dir_getrelname()
* LPTSTR dir_getnameptr(DIRITEM cur)
* dir_getfullname()
* dir_getroot_list()
* dir_getroot_item()
* dir_freerelname()
* dir_freefullname()
* dir_freeroot_list()
* dir_freerootitem()
* dir_getopenname()
* dir_freeopenname()
* dir_openfile()
* dir_closefile()
* dir_filesize()
* dir_startcopy()
* dir_endcopy()
* dir_copy()
* dir_finalelem()
* dir_cleardirect()
* dir_adddirect()
* dir_addfile()
* dir_scan()
* dir_isvaliddir()
* dir_isvalidfile()
* dir_fileinit()
* dir_dirinit()
* dir_getpathsize()
* dir_findnextfile()
* SOME NEW ITEMS
* VOID  dir_scan_zip(DIRECT dir, BOOL bRecurse); #ifdef ADD_ZIP_SUPPORT
* BOOL  dir_iszip( DIRITEM di ); // #ifdef ADD_ZIP_SUPPORT
* INT   dir_openzip( PVOID pv ); // #ifdef ADD_ZIP_SUPPORT
* BOOL  dir_iszipfile( LPTSTR lpf ); // #ifdef ADD_ZIP_SUPPORT
* BOOL  dir_setzipflag( PDWORD pdw, LPTSTR lplf, LPTSTR lprf );
*
* Comments:
*
* The call dir_buildlist takes a pathname and returns a handle. Subsequent
* calls to dir_firstitem and dir_nextitem return handles to
* items within the list, from which you can get the name of the
* file (relative to the original pathname, or complete), and filesize.
*
* The list can be either built entirely during the build call, or
* built one directory at a time as required by dir_nextitem calls. This
* option affects only relative performance, and is taken as a
* recommendation only (ie some of the time we will ignore the flag).
*
* The list is ordered alphabetically (case-insensitive using lstrcmpi).
* within any one directory, we list filenames before going on
* to subdirectory contents.
*
* All memory is allocated from a gmem_* heap hHeap declared
* and initialised elsewhere.
*
* The caller gets handles to two things: a DIRLIST, representing the
* entire list of filenames, and a DIRITEM: one item within the list.
*
* From the DIRITEM he can get the filename (including or excluding the
* tree root passed to dir_build*) - and also he can get to the next
* DIRITEM.
*
* We permit lazy building of the tree (usually so the caller can keep
* the user-interface up-to-date as we go along). In this case,
* we need to store information about how far we have scanned and
* what is next to do. We need to scan an entire directory at a time and then
* sort it so we can return files in the correct order.
*
* We scan an entire directory and store it in a DIRECT struct. This contains
* a list of DIRITEMs for the files in the current directory, and a list of
* DIRECTs for the subdirectories (possible un-scanned).
*
* dir_nextitem will use the list functions to get the next DIRITEM on the list.
* When the end of the list is reached, it will use the backpointer back to the
* DIRECT struct to find the next directory to scan.
*
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include "dc4w.h"
//#include  "scandir.h"
// just do not like these LONG names
//#define  FD       WIN32_FIND_DATA
//#define  PFD      FD *

// MB_ALLOKNOALL
//#define  SD_MB_DEF      (MB_ICONINFORMATION|MB_OKCANCEL)
#define  SD_MB_DEF      (MB_ICONINFORMATION|MB_ALLOKNOALL)

#undef   ADDOUTLST      // diag output of LIST of files

extern   BOOL  MatchesExclude( LPTSTR lpf, DWORD ftyp );
extern   VOID  AddOKCancel( LPTSTR lpb );
extern   BOOL  Match2XList( PLE pH, LPTSTR lpf, DWORD ftyp );
extern   HFONT g_hFixedFont8, g_hfCN8, g_hFF8bold;    // LOGFONT creation

/*
 * Hold name and information about a given file (one ITEM in a DIRectory)
 * caller's DIRITEM handle is a pointer to one of these structures
 */
struct diritem {

   LPTSTR   di_pname; /* ptr to filename (final element only) */

   DWORD    di_dwFlag;     // various flags
   long     size;          /* filesize - note full size in FD below */

   struct   direct FAR * di_pdirect; /* containing directory */

   LPTSTR   localname;        /* name of temp copy of file */
   BOOL     bLocalIsTemp;      /* true if localname is tempfile. */

   BOOL     di_bIsZip;        // TRUE if from a ZIP file

   FD       di_sfd;   /* this contain size, date and ... */

};


/* DIRECT:
 * Hold state about directory
 * and current position in list of filenames.
 */
typedef struct direct {

        LPTSTR    d_prelname;          /* name of dir relative to DIRLIST root */

        DIRLIST   head;           /* back ptr (to get fullname) */

        struct direct FAR * parent; /* parent directory (NULL if above tree root)*/

        BOOL   d_bScanned;   /* TRUE if scanned */

        BOOL   d_bIsZip;     // TRUE if a ZIP file

        // stats, during scan of directory
        DWORD  dwDirsTot, dwDirsExcl;
        DWORD  dwFilsTot, dwFilsExcl;

        FD     d_sfd;         // keep the information gathered

        LIST diritems;          /* list of DIRITEMs for files in cur. dir */

        LIST directs;           /* list of DIRECTs for child dirs */

        int pos;                /* where are we begin, files, dirs */

        struct direct FAR * curdir; /* subdir being scanned (ptr to list element)*/

} FAR * DIRECT;

/* Values for direct.pos */
#define DL_FILES        1       /* reading files from the diritems */
#define DL_DIRS         2       /* in the dirs: List_Next on curdir */

/*
 * The DIRLIST handle returned from a build function is in fact
 * a pointer to one of these
 */
struct dirlist {

   char     rootname[264]; /* name of root of tree */
   BOOL     dl_bFile;      /* TRUE if root of tree is file, not dir */
   BOOL     dl_bIsZip;        // is NOT really a FILE - it is a ZIP of FILES
   DWORD    dl_dwFlag;  // indicates tf_IsLeft or tf_IsRight
   INT      nCopies;    // copies made
   DIRECT   dl_pdot;    /* dir  for '.' - for tree root dir */

};

extern BOOL bAbort;             /* from dc4w.c (read only here). */


/* ------ memory allocation ---------------------------------------------*/

/* All memory is allocated from a heap created by the application */
extern HANDLE hHeap;

/*-- forward declaration of internal functions ---------------------------*/

LPSTR dir_finalelem(LPSTR path);
void dir_cleardirect(DIRECT dir);
void dir_adddirect(DIRECT dir, LPSTR path, PFD pfd, BOOL bZip);
void dir_scan(DIRECT dir, BOOL bRecurse, INT level);
BOOL dir_isvaliddir(LPSTR path);
//BOOL dir_isvalidfile(LPSTR path);    // now made PUBLIC
void dir_addfile(DIRECT dir, LPSTR path, long size);
void dir_fileinit(DIRITEM pfile, DIRECT dir, LPSTR path, long size);
//void dir_dirinit(DIRECT dir, DIRLIST head, DIRECT parent, LPSTR name);
long dir_getpathsize(LPSTR path);
DIRITEM dir_findnextfile(DIRLIST dl, DIRECT curdir);

// alternate set to use WIN32_FIND_DATA, and KEEP all the information
void dir_dirinit(DIRECT dir, DIRLIST dl_head, DIRECT parent, LPSTR name, PFD pfd);
// NEW
VOID dir_fileinit2(DIRITEM pfile, DIRECT dir, LPTSTR path, PFD pfd);
BOOL dir_isvaliddir2(LPTSTR path, PFD pfd);
BOOL dir_isvalidfile2(LPTSTR path, PFD pfd);
VOID dir_addfile2(DIRECT dir, LPTSTR path, PFD pfd);
// July 2001 - ZIP HANDLING

#ifdef ADD_ZIP_SUPPORT
VOID  dir_scan_zip(DIRECT dir, BOOL bRecurse); // #ifdef ADD_ZIP_SUPPORT
#endif // #ifdef ADD_ZIP_SUPPORT

// Jan 2002 - Keep list of MISSED files/directories (for review)
VOID  dir_add2missedfiles(DIRECT dir, LPTSTR path, PFD pfd ); // add file or directory to MISSED list

FD    g_sFD;   // global WIN32_FIND_DATA pads
int   nLocalCopies;  // cleared in startcopy, ++d in copy
int   nLocalFails;   // cleared in startcopy, inspected in endcopy

#define  build_list     0x00000001

DWORD    dbg_flag = build_list;

#define  DBG(a,b) if( dbg_flag & a ) { b }

TCHAR g_szActScan[264]; // = dl->rootname );
TCHAR g_szLeftScan[264]; // = dl->rootname );
TCHAR g_szRightScan[264]; // = dl->rootname );
DWORD g_dwActScan = 0;  // if left/right or zip ...
BOOL  g_bActScan;  // = bFile;
FD    g_sLeftScan, g_sRightScan;
BOOL  g_bSortExcl = FALSE;
BOOL  gbAddRelNm = FALSE;
/***************************************************************************
 * Function: dir_buildlist
 *
 * Purpose:
 *
 * Build a list of filenames
 *
 * Optionally build the list on demand, in which case we scan the
 * entire directory but don't recurse into subdirs until needed
 *
 * NOTE: In the present implementation, bOnDemand is TRUE, thus
 *       if given a DIRECTORY/FOLDER it will call dir_scan with !bOnDemand.
 *       That is with recursive FALSE at this initial stage of building a
 *       list.
 *
 */

DIRLIST
dir_buildlist(LPSTR path, BOOL bOnDemand, DWORD dwFlg )
{
        DIRLIST dl;
        BOOL bFile;
        
        /* first check if the path is valid */
        //if( dir_isvaliddir(path) )
        if( dir_isvaliddir2( path, &g_sFD ) )
        {
                bFile = FALSE;
        }
        else if( dir_isvalidfile2( path, &g_sFD ) )
        {
                bFile = TRUE;
        }
        else
        {
                /* not valid */
                return(NULL);
        }


        /* alloc and init the DIRLIST head */
        dl = (DIRLIST) gmem_get(hHeap, sizeof(struct dirlist), "dir_buildlist0" );

        memset(dl, 0, sizeof(struct dirlist));

        /* convert the pathname to an absolute path */
        //_fullpath(dl->rootname, path, sizeof(dl->rootname));
        _fullpath(dl->rootname, path, 256);

        strcpy( g_szActScan, dl->rootname );
        g_dwActScan = dwFlg;
        if( g_dwActScan & tf_IsLeft )
        {
           strcpy( g_szLeftScan, g_szActScan );
           memcpy( &g_sLeftScan, &g_sFD, sizeof(FD) );   // global WIN32_FIND_DATA pad
        }
        else if( g_dwActScan & tf_IsRight )
        {
           strcpy( g_szRightScan, g_szActScan );
           memcpy( &g_sRightScan, &g_sFD, sizeof(FD) );   // global WIN32_FIND_DATA pad
        }
        else
        {
           chkme( "WARNING: global active scan NEITHER LEFT nor RIGHT!!!"MEOR );
        }

        g_bActScan =  bFile;

        dl->dl_bFile  = bFile;
        dl->dl_bIsZip = ((dwFlg & tf_IsZip) ? TRUE : FALSE);
        dl->dl_dwFlag = dwFlg;   // keep left and right flags
        /* make a '.' directory for the current directory -
         * all files and subdirs will be listed from here
         */

        dl->dl_pdot = (DIRECT) gmem_get(hHeap, sizeof(struct direct), "dir_buildlist" );

        dir_dirinit(dl->dl_pdot, dl, NULL, ".", NULL); // no PFD for this dummy directory

        DBG( build_list, sprtf( "Building list for [%s] %s (%s)"MEOR, dl->rootname,
           (bFile ? ((dwFlg & tf_IsZip) ? "ZipFile" : "File") :
           ((dwFlg & tf_IsZip) ? "ErrorDir" : "Directory")),
              ( ( dwFlg & tf_IsLeft ) ? "Left" : "Right" ) ); );

        /* were we given a file or a directory ? */
        if( bFile )
        {
           //if( dwFlg )
#ifdef ADD_ZIP_SUPPORT
           if( dl->dl_bIsZip )
           {
              dir_scan_zip(dl->dl_pdot, (!bOnDemand) ); // #ifdef ADD_ZIP_SUPPORT
              return(dl);
           }
           else
#endif // #ifdef ADD_ZIP_SUPPORT
           {
                /* its a file. create a single file entry
                 * and set the state accordingly
                 */

                dl->dl_pdot->d_bScanned = TRUE;

                //dir_addfile(dl->dot, dir_finalelem(path), dir_getpathsize(path));
                dir_addfile2(dl->dl_pdot, dir_finalelem(path), &g_sFD);

                return(dl);
           }
        }

        /* scan the root directory and return. if we are asked
         * to scan the whole thing, this will cause a recursive
         * scan all the way down the tree
         */
        dir_scan(dl->dl_pdot, (!bOnDemand), 0 );

        return(dl);

} /* dir_buildlist */

/***************************************************************************
 * Function: dir_delete
 *
 * Purpose:
 *
 * Free up the DIRLIST and all associated memory 
 */
void
dir_delete(DIRLIST dl)
{
        if( dl == NULL )
        {
                return;
        }

        dir_cleardirect(dl->dl_pdot);

        gmem_free(hHeap, (LPSTR) dl->dl_pdot, sizeof(struct direct), "dir_buildlist" );

        dl->dl_pdot = NULL;

        gmem_free(hHeap, (LPSTR) dl, sizeof(struct dirlist), "dir_buildlist0" );
}



/***************************************************************************
 * Function: dir_isfile
 *
 * Purpose:
 *
 * Was the original build request a file or a directory ? 
 */
BOOL
dir_isfile(DIRLIST dl)
{
   if(dl == NULL)
   {
      return(FALSE);
   }
   if( dl->dl_bFile )
   {
      if( dl->dl_bIsZip )
         return FALSE;
      else
         return TRUE;
   }
   else
      return FALSE;

   //     return(dl->dl_bFile);
}

/***************************************************************************
 * Function: dir_firstitem
 *
 * Purpose:
 *
 * Return the first file in the list, or NULL if no files found.
 * Returns a DIRITEM. This can be used to get filename, size and chcksum.
 * If there are no files in the root, we recurse down until we find a file.
 */
DIRITEM
dir_firstitem(DIRLIST dl)
{
        if(( dl == NULL ) ||
           ( dl->dl_pdot == NULL ) )
        {
           if(dl)
              chkme( "WHOA: The dl_pdot is NULL!!!"MEOR );

           return(NULL);
        }

        /*
         * reset the state to indicate that no files have been read yet
         */
        dl->dl_pdot->pos = DL_FILES;
        dl->dl_pdot->curdir = NULL;

        /* now get the next filename */
        return( dir_findnextfile(dl, dl->dl_pdot) );
} /* dir_firstitem */


/***************************************************************************
 * Function:dir_nextitem
 *
 * Purpose:
 *
 * Get the next filename after the one given.
 *
 * The List_Next function can give us the next element on the list of files.
 * If this is null, we need to go back to the DIRECT and find the
 * next list of files to traverse (in the next subdir).
 *
 * After scanning all the subdirs, return to the parent to scan further
 * dirs that are peers of this, if there are any. If we have reached the end of
 * the tree (no more dirs in dl->dot to scan), return NULL.
 *
 * RECURSION: Don't recurse to lower levels unless fDeep is TRUE
 *
 */
DIRITEM
dir_nextitem(DIRLIST dl, DIRITEM cur, BOOL fDeep)
{
   DIRITEM next;

   if( (dl == NULL) || (cur == NULL) )
   {
      return(NULL);
   }
   if(bAbort)
      return NULL;  /* user requested abort */

   if( (next = List_Next(cur)) != NULL)
   {
      /* there was another file on this list */
      return(next);
   }
   if( !fDeep )
      return NULL;

   /* get the head of the next list of filenames from the directory */
   cur->di_pdirect->pos = DL_DIRS;
   cur->di_pdirect->curdir = NULL;

   return( dir_findnextfile( dl, cur->di_pdirect ) );

} /* dir_nextitem */

/***************************************************************************
 * Function: dir_findnextfile
 *
 * Purpose:
 *
 * Gets the next file in the directory
 */
DIRITEM
dir_findnextfile(DIRLIST dl, DIRECT curdir)
{
        DIRITEM curfile;

        if ((dl == NULL) || (curdir == NULL)) {
                return(NULL);
        }

        /* scan the subdir if necessary */
        if( !curdir->d_bScanned )
        {
            g_dwScDirs = 0;
            g_dwScFiles = 0;
#ifdef ADD_ZIP_SUPPORT
            if( curdir->d_bIsZip ) {
              dir_scan_zip( curdir, FALSE ); // #ifdef ADD_ZIP_SUPPORT
            } else 
#endif // #ifdef ADD_ZIP_SUPPORT
            {
              dir_scan( curdir, FALSE, 0 );
            }
           if( dl->dl_dwFlag & tf_IsRight ) {
              g_dwLeftDirs += g_dwScDirs;
              g_dwLeftFiles += g_dwScFiles;
           } else if( dl->dl_dwFlag & tf_IsLeft ) {
              g_dwRightDirs += g_dwScDirs;
              g_dwRightFiles += g_dwScFiles;
           } else {
              // UNKNOWN - BAD CALL!!!
               g_dwScDirs = 0;
           }
            g_dwScDirs = 0;
            g_dwScFiles = 0;
        }

        /* have we already read the files in this directory ? */
        if( curdir->pos == DL_FILES )
        {
                /* no - return head of file list */
                curfile = (DIRITEM) List_First(curdir->diritems);
                if (curfile != NULL) {
                        return(curfile);
                }

                /* no more files - try the subdirs */
                curdir->pos = DL_DIRS;
        }

        /* try the next subdir on the list, if any */
        /* is this the first or the next */
        if (curdir->curdir == NULL) {
                curdir->curdir = (DIRECT) List_First(curdir->directs);
        } else {
                curdir->curdir = (DIRECT) List_Next(curdir->curdir);
        }

        /* did we find a subdir ? */
        if (curdir->curdir == NULL) {

                /* no more dirs - go back to parent if there is one */
                if (curdir->parent == NULL) {
                        /* no parent - we have exhausted the tree */
                        return(NULL);
                }

                /* reset parent state to indicate this is the current
                 * directory - so that next gets the next after this.
                 * this ensures that multiple callers of dir_nextitem()
                 * to the same tree work.
                 */
                curdir->parent->pos = DL_DIRS;
                curdir->parent->curdir = curdir;

                return(dir_findnextfile(dl, curdir->parent));
        }

        /* there is a next directory - set it to the
         * beginning and get the first file from it
         */
        curdir->curdir->pos = DL_FILES;
        curdir->curdir->curdir = NULL;
        return(dir_findnextfile(dl, curdir->curdir));

} /* dir_findnextfile */


/*-- pathnames ----
 *
 * This module supports two types of pathnames, called relative and full.
 * Relative names are relative to the root passed in the initial call
 * to dir_build*, and full names include the tree root.
 *
 * Note that this is a different distinction to relative vs absolute
 * pathnames, since the tree root may still be either relative or absolute.
 *
 * Examples:
 *
 *  - if you called dir_buildlist("c:\")
 *              getrelname gives:               ".\config.sys"
 *              getfullname gives:              "c:\config.sys"
 *
 * - if you called dir_buildlist(".\geraintd")
 *              getrelname gives:               ".\source\scandir.h"
 *              getfullname gives either
 *                      ".\geraintd\source\scandir.h"
 *                    or "c:\geraintd\source\scandir.h"
 *                   (depending on the implementation).
 *
 * To support this, we maintain the tree root name in the DIRLIST head, and
 * in each directory, the name of that directory relative to tree root.
 * Files just have the filename, so we need to prepend the directory name,
 * and (for getfullname) the tree root name as well
 *
 * We store the directory name with a trailing
 * slash to make concatenation easier
 *
 * -----
 */

/***************************************************************************
 * Function: dir_getrelname
 *
 * Purpose:
 *
 * Return the name of the current file relative to tree root
 */
LPTSTR dir_getrelname(DIRITEM cur)
{
   LPTSTR pname;
   int size;

   /* check this is a valid item */
   if(( cur == NULL             ) ||
      ( cur->di_pname == NULL   ) ||
      ( cur->di_pdirect == NULL ) )
   {
        if( cur )
        {
           if( cur->di_pname == NULL   )
           {
              chkme( "HEY: Call to dir_getrelname() FAILED!"MEOR
                "because cur->di_pname is NULL!!!"MEOR );
           }
           else
           {
              chkme( "HEY: Call to dir_getrelname() FAILED!"MEOR
                "because cur->di_pdirect is NULL!!!"MEOR );
           }
        }
        // and RETURN a NULL
        return(NULL);
   }

   //if( (DWORD)cur->di_pdirect->d_prelname == 0x45434338 )
   //if( (DWORD)cur->di_pdirect == 0x45434338 )
   //   chkme( "YEEK! What is this VALUE? (0x45434338)"MEOR );

   /* remember to include the NULL when sizing */
   size = strlen(cur->di_pdirect->d_prelname) + strlen(cur->di_pname) + 1;
   pname = gmem_get(hHeap, size, "dir_getrelname" );
   if(!pname)
   {
           chkme( "C:ERROR: Memory allocation FAILED!"MEOR );
           return NULL;
   }

   strcpy(pname, cur->di_pdirect->d_prelname);
   strcat(pname, cur->di_pname);

   return(pname); // return (allocated memory pointer to) name
} /* dir_getrelname */

// WIN32_FILE_DATA
//typedef struct _WIN32_FIND_DATA {
//  DWORD    dwFileAttributes; 
//  FILETIME ftCreationTime; 
//  FILETIME ftLastAccessTime; 
//  FILETIME ftLastWriteTime; 
//  DWORD    nFileSizeHigh; 
//  DWORD    nFileSizeLow; 
//  DWORD    dwReserved0; 
//  DWORD    dwReserved1; 
//  TCHAR    cFileName[ MAX_PATH ]; 
//  TCHAR    cAlternateFileName[ 14 ]; 
//} WIN32_FIND_DATA, *PWIN32_FIND_DATA;
LPTSTR dir_getnameptr(DIRITEM cur)
{
   /* check this is a valid item */
   PFD pfd = dir_getpfd(cur);
   if(pfd)
      return( &pfd->cFileName[0] ); // if a PFD(FindFirstFile()) then return it

   if(( cur == NULL             ) ||
      ( cur->di_pname == NULL   ) )
   {
        if( cur )
        {
           chkme( "HEY: Call to dir_getrelname() FAILED!"MEOR
                "because cur->di_pname is NULL!!!"MEOR );
        }
        // and RETURN a NULL
        return(NULL);
   }
   return( cur->di_pname );
}

/***************************************************************************
 * Function: dir_getfullname
 *
 * Purpose:
 *
 * Return the fullname of the file (including the tree root passed in) 
 */
LPSTR
dir_getfullname(DIRITEM cur)
{
        LPSTR name;
        int size;
        LPSTR head;

        /* check this is a valid item */
        if (cur == NULL)  {
                return(NULL);
        }

        if (cur->di_pdirect->head->dl_bFile)
        {
                return(cur->di_pdirect->head->rootname);
        }

        /* remember to include the NULL when sizing */
        size = strlen(cur->di_pname) + 1;

        size += strlen(cur->di_pdirect->d_prelname);

        /* add on root name */
        head = cur->di_pdirect->head->rootname;
        size += strlen(head);

        /* root names may not end in a slash. we need to
         * insert one in this case. Also, relnames always begin .\, so
         * we skip the . always, and the .\ if we don't need to
         * append a slash
         *
         */
        size--;         /* omit the '.' */
        if (*CharPrev(head, head+strlen(head)) == '\\') {
                size--;                         /* omit the .\ */
        }

        name = gmem_get(hHeap, size, "get_fullname" );

        strcpy(name, cur->di_pdirect->head->rootname);

        /* add relname and then name, omiting the .\ */

        /* skip . or .\ before relname */
        if (*CharPrev(head, head+strlen(head)) == '\\') {
                strcat(name, &cur->di_pdirect->d_prelname[2]);
        } else {
                strcat(name, &cur->di_pdirect->d_prelname[1]);
        }

        strcat(name, cur->di_pname);

        return(name);

} /* dir_getfullname */


/***************************************************************************
 * Function: dir_getroot_list
 *
 * Purpose:
 *
 * Return the name of the tree root given a handle to the DIRLIST.
 */
LPSTR
dir_getroot_list(DIRLIST dl)
{
        if (dl == NULL) 
                return(NULL);
        return(dl->rootname);
} /* dir_getroot_list */

/***************************************************************************
 * Function: dir_getroot_item
 *
 * Purpose:
 *
 * Return the root name of this tree given a handle to a DIRITEM in the
 * list.
 */
LPSTR dir_getroot_item(DIRITEM item)
{
        if (item == NULL) 
                return(NULL);

        return(dir_getroot_list(item->di_pdirect->head));
}


/***************************************************************************
 * Function: dir_freerelname
 *
 * Purpose:
 *
 * Free up a relname that we allocated. This interface allows us
 * some flexibility in how we store relative and complete names
 *
 */
void
dir_freerelname(DIRITEM cur, LPSTR name)
{
        if((cur != NULL) && (name != NULL))
        {
            gmem_free(hHeap, name, (strlen(name) + 1), "dir_getrelname" );
        }
} /* dir_freerelname */

/***************************************************************************
 * Function: dir_freefullname
 *
 * Purpose:
 *
 */
void
dir_freefullname(DIRITEM cur, LPSTR name)
{
        if( !cur )   // if no DIRITEM
           return;   // then can do nothing

        if( cur->di_pdirect->head->dl_bFile ) // if it is a FILE
           return;   // then a pointer was returned

        if( name != NULL )    // if we have an allocated pointer
        {
           // then FREE that pointer
           gmem_free(hHeap, name, (strlen(name) + 1), "get_fullname" );
        }
} /* dir_freefullname            */

/***************************************************************************
 * Function: dir_freeroot_list
 *
 * Purpose:
 *
 * Free up rootname allocated by dir_getroot_list.
 * We just gave a pointer to the rootname, so do nothing.
 *
 */
void
dir_freeroot_list(DIRLIST dl, LPSTR name)
{
        if ((dl == NULL) || (name == NULL)) {
                return;
        }
        return;
} /* dir_freeroot_list */

/***************************************************************************
 * Function: dir_freeroot_item
 *
 * Purpose:
 *
 * Free up memory alloc-ed by a call to dir_getroot_item. 
 * We just gave a pointer to the rootname, so do nothing.
 *
 */
void
dir_freeroot_item(DIRITEM item, LPSTR name)
{
        if ((item == NULL) || (name == NULL)) 
                return;
        dir_freeroot_list(item->di_pdirect->head, name);
}

/***************************************************************************
 * Function: dir_getopenname
 *
 * Purpose:
 *
 * Get an open-able name for the file. This will be the same as the fullname.
 */
LPSTR
dir_getopenname(DIRITEM item)
{
        LPSTR fname;

        if (item == NULL) 
                return(NULL);

        fname = dir_getfullname(item);

        return(fname);
} /* dir_getopenname */


/***************************************************************************
 * Function: dir_freeopenname
 *
 * Purpose:
 *
 * Free up memory created by a call to dir_getopenname(). This *may*
 * cause the file to be deleted if it was a temporary copy.
 */
void
dir_freeopenname(DIRITEM item, LPSTR openname)
{
        if ((item == NULL) || (openname == NULL)) 
                return;

        dir_freefullname(item, openname);
} /* dir_freeopenname */

/***************************************************************************
 * Function: dir_openfile
 *
 * Purpose:
 *
 * Return an open file handle to the file. 
 */
int
dir_openfile(DIRITEM item)
{
        LPSTR fname;
        int fh;
        OFSTRUCT os;
        fname = dir_getfullname(item);
        fh = OpenFile(fname, &os, OF_READ|OF_SHARE_DENY_NONE);
        dir_freefullname(item, fname);
        return(fh);
} /* dir_openfile */

/***************************************************************************
 * Function: dir_closefile
 *
 * Purpose:
 *
 * Close a file opened with dir_openfile.
 */
void
dir_closefile(DIRITEM item, int fh)
{
   if( fh > 0 )
        _lclose(fh);

} /* dir_closefile */


/***************************************************************************
 * Function: dir_getfilesize
 *
 * Purpose:
 *
 * Return the file size (set during scanning) 
 */
long
dir_getfilesize(DIRITEM cur)
{
        /* check this is a valid item */
        if (cur == NULL)
                return(0);

        return(cur->size);
} /* dir_getfilesize */



/* ss_endcopy returns a number indicating the number of files copied,
   but we may have some local copies too.  We need to count these
   ourselves and add them in
*/

/***************************************************************************
 * Function: dir_startcopy
 *
 * Purpose:
 *
 * Start a bulk copy 
 */
BOOL dir_startcopy(DIRLIST dl)
{
   nLocalCopies = 0;
   nLocalFails  = 0;
   if( !dl )
      return FALSE;

   dl->nCopies = 0;  // clear copies count

   return   TRUE;

} /* dir_startcopy */
/***************************************************************************
 * Function: dir_endcopy
 *
 */
 
int dir_endcopy(DIRLIST dl)
{
   //return(nLocalCopies);
   return nLocalFails;
} /* dir_endcopy */

// When used to delete a file, SHFileOperation(&fo) will attempt
// to place the deleted file in the Recycle Bin.
// If you wish to delete a file and guarantee that it will
// not be placed in the Recycle Bin, use DeleteFile.
// DeleteFile(pnp);
//typedef struct _SHFILEOPSTRUCT{ 
//    HWND hwnd; 
//    UINT wFunc; 
//    LPCTSTR pFrom; 
//    LPCTSTR pTo; 
//    FILEOP_FLAGS fFlags; 
//    BOOL fAnyOperationsAborted; 
//    LPVOID hNameMappings; 
//    LPCSTR lpszProgressTitle; 
//} SHFILEOPSTRUCT, *LPSHFILEOPSTRUCT;
BOOL  dir_filedelete( LPTSTR lpf )
{
   BOOL              bRet = FALSE;
   DWORD             dwi  = strlen(lpf);
   SHFILEOPSTRUCT    shfo;
   LPSHFILEOPSTRUCT  pfo = &shfo;

   if( !dwi )
      return FALSE;

   dwi++;
   lpf[dwi] = 0;  // An additional NULL character must be appended to the
   // end of the final name to indicate the end of pFrom.

   ZeroMemory( pfo, sizeof(SHFILEOPSTRUCT) );
   pfo->hwnd = hwndClient;
   pfo->wFunc = FO_DELETE; // delete file in pFrom
   pfo->pFrom = lpf;       // pointer to file
   pfo->fFlags = FOF_ALLOWUNDO | FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION;

   if( SHFileOperation( pfo ) == 0 )
      bRet = TRUE;

   return bRet;
}


BOOL dir_copy_ORG(DIRITEM item, LPSTR newroot)
{
   static char newpath[256];
   LPSTR relname, fullname;
   LPSTR pstart, pdest, pel;
   BOOL bOK = FALSE;

   BY_HANDLE_FILE_INFORMATION bhfi;
   HANDLE hfile;

   /*
    * check that the newroot directory itself exists
    */
   if( (item == NULL) || !dir_isvaliddir(newroot) )
   {
           return(FALSE);
   }

   /*
    * name of file relative to the tree root
    */
   relname = dir_getrelname(item);

   /*
    * build the new pathname by concatenating the new root and
    * the old relative name. add one path element at a time and
    * ensure that the directory exists, creating it if necessary.
    */
   strcpy(newpath, newroot);

   /* add separating slash if not already there */
   if( *CharPrev(newpath, (newpath + strlen(newpath)) ) != '\\' )
   {
      strcat(newpath, "\\");
   }

   pstart = relname;
   while( (pel = strchr(pstart, '\\')) != NULL )
   {
      /* found another element ending in slash. incr past the \\ */
      pel++;

      /*
       * ignore the simple root name of ".\"!
       */
      if( strncmp(pstart, ".\\", 2) != 0 )
      {
         pdest = &newpath[strlen(newpath)];
         strncpy(pdest, pstart, pel - pstart);
         pdest[pel - pstart] = '\0';

         /* create subdir if necessary */
         if( !dir_isvaliddir(newpath) )
         {
            if( _mkdir(newpath) != 0 )
            {
               // oops, can not create this new folder!
               return(FALSE);
            }
         }
      }
      pstart = pel;
   }

   /*
    * there are no more slashes, so pstart points at the final
    * element
    */
   strcat(newpath, pstart);

   fullname = dir_getfullname(item);

   bOK = CopyFile(fullname, newpath, FALSE); // COPY with OVERWRITE

   /* having copied the file, now copy the times, attributes */
   hfile = CreateFile(fullname, GENERIC_READ, 0, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   bhfi.dwFileAttributes = GetFileAttributes(fullname);
   GetFileTime(hfile, &bhfi.ftCreationTime,
                   &bhfi.ftLastAccessTime, &bhfi.ftLastWriteTime);
   CloseHandle(hfile);

   hfile = CreateFile(newpath, GENERIC_WRITE, 0, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   SetFileTime(hfile, &bhfi.ftCreationTime,
                      &bhfi.ftLastAccessTime,
                      &bhfi.ftLastWriteTime);
   CloseHandle(hfile);
   SetFileAttributes(newpath, bhfi.dwFileAttributes);


   if (bOK) ++nLocalCopies;

   dir_freerelname(item, relname);
   dir_freefullname(item, fullname);

   return(bOK);

} /* dir_copy_ORG */

/***************************************************************************
 * Function: dir_dirinit
 *
 * Purpose:
 *
 * Fill out a new DIRECT for a subdirectory (pre-allocated).
 * Init files and dirs lists to empty (List_Create). Set the relname
 * of the directory by pre-pending the parent relname if there
 * is a parent, and appending a trailing slash (if there isn't one).
 */
void
dir_dirinit(DIRECT dir, DIRLIST dl_head, DIRECT parent, LPSTR name, PFD pfd)
{
        int size;

        dir->head   = dl_head;
        dir->parent = parent;

        /* add on one for the null and one for the trailing slash */
        size = strlen(name) + 2;
        if( parent != NULL )
        {
                size += strlen(parent->d_prelname);
        }

        /* build the relname from the parent and the current name
         * with a terminating slash
         */
        dir->d_prelname = gmem_get(hHeap, size, "dir_dirinit" );

        if( parent != NULL )
        {
                strcpy(dir->d_prelname, parent->d_prelname);
        }
        else
        {
                dir->d_prelname[0] = '\0';
        }

        strcat(dir->d_prelname, name);

        if( *CharPrev(dir->d_prelname,
                        ( dir->d_prelname + strlen(dir->d_prelname) ) ) != '\\' )
        {
                strcat(dir->d_prelname, "\\");
        }

        /* force name to lowercase - NO MORE!! */
        //AnsiLowerBuff(dir->relname, lstrlen(dir->relname));

        dir->diritems   = List_Create();
        dir->directs    = List_Create();
        dir->d_bScanned = FALSE;
        dir->d_bIsZip   = dl_head->dl_bIsZip;   // ensure ZIP flag is perpetuated
        dir->pos        = DL_FILES;

        if( pfd ) // if passed the FIND info,
           memcpy( &dir->d_sfd, pfd, sizeof(WIN32_FIND_DATA) );   // keep the INFO

} /* dir_dirinit */


/***************************************************************************
 * Function: dir_fileinit
 *
 * Purpose:
 *
 * Initialise the contents of an (allocated) DIRITEM struct. 
 */
//void
//dir_fileinit_NOT_USED(DIRITEM pfile, DIRECT dir, LPSTR path, long size)
//{
//        pfile->name = gmem_get(hHeap, (strlen(path) + 1), "dir_fileinit" );
//        lstrcpy(pfile->name, path);
        /* force name to lower case */
        //AnsiLowerBuff(pfile->name, lstrlen(path));
//        pfile->direct = dir;
//        pfile->size = size;
//        pfile->localname = NULL;
//} /* dir_fileinit */

/***************************************************************************
 * Function: dir_fileinit2
 *
 * Purpose:
 *
 * Initialise the contents of an (allocated) DIRITEM struct. 
 *
 */
VOID  dir_fileinit2(DIRITEM pfile, DIRECT dir, LPTSTR path, PFD pfd)
{
   pfile->di_pname = gmem_get(hHeap, (strlen(path) + 1), "dir_fileinit" );

   strcpy(pfile->di_pname, path);

   /* force name to lower case - NO WAY - keep case exact */
   //AnsiLowerBuff(pfile->name, lstrlen(path));

   //if( (DWORD)dir == 0x45434338 )
   //   chkme( "YEEK2! What is this VALUE? (%#x)"MEOR, dir );

   pfile->di_pdirect = dir;

   //pfile->size = size;
   memcpy( &pfile->di_sfd, pfd, sizeof(WIN32_FIND_DATA) );

   pfile->size = pfd->nFileSizeLow;

   pfile->localname = NULL;

   pfile->di_bIsZip = dir->d_bIsZip;   // COPY the ZIP flag

} /* dir_fileinit2 */


PWIN32_FIND_DATA  dir_getpfd(DIRITEM item)
{
   if(item)
      return( &item->di_sfd );
   else
      return NULL;   // no PFD if no FILE (left or right)
}

/***************************************************************************
 * Function: dir_isfilevalid
 *
 * Purpose:
 *
 * Is this a valid file or not 
 */
BOOL
dir_isvalidfile(LPTSTR path)
{
        DWORD dwAttrib;

        dwAttrib = GetFileAttributes(path);
        if (dwAttrib == -1) {
                return(FALSE);
        }
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
                return(FALSE);
        }
        return(TRUE);
} /* dir_isvalidfile */

BOOL dir_isvalidfile2(LPTSTR path, PFD pfd)
{
   HANDLE   hFind = FindFirstFile( path, pfd );
   if( hFind && (hFind != INVALID_HANDLE_VALUE) )
   {
      FindClose(hFind);
      if( !( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
         return(TRUE);
   }
   return(FALSE);
}

/***************************************************************************
 * Function: dir_isvaliddir
 *
 * Purpose:
 *
 * Is this a valid directory ? 
 */
BOOL
dir_isvaliddir(LPSTR path)
{
        DWORD dwAttrib;

        dwAttrib = GetFileAttributes(path);
        if (dwAttrib == -1) {
                return(FALSE);
        }
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
                return(TRUE);
        }
        return(FALSE);
} /* dir_isvaliddir */

// FIX20030626 - "D:\" does NOT pass, but it SHOULD, shouldn't it?
BOOL dir_isvaliddir2(LPTSTR path, PFD pfd)
{
   HANDLE   hFind = FindFirstFile( path, pfd );

   //if( hFind && (hFind != INVALID_HANDLE_VALUE) )
   if( VFH(hFind) )
   {
      FindClose(hFind);
      if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         return(TRUE);
   }
   else
   {
      static TCHAR _s_szpathtmp[264];
      INT      len;
      LPTSTR   ptmp = _s_szpathtmp;

      strcpy(ptmp,path);
      len = strlen(ptmp);
      if(len)
      {
         len--;
         if(( ptmp[len] == '\\' ) || ( ptmp[len] == '/' ))
         {
            // ptmp[len] = 0; or
            //strcat(ptmp,"."); // add the ROOT directory for the user
            //strcat(ptmp,"*.*"); // add the wild card for the user
            // return to first idea
            ptmp[len] = 0; // FIX20051203 - remove the trailing slash
            hFind = FindFirstFile( ptmp, pfd );
            if( VFH(hFind) )
            {
               FindClose(hFind);
               if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                  return(TRUE);
            }
         } else if ( ptmp[len] == ':' ) {
            // is a DRIVE
            strcat(ptmp,"\\*.*");
            hFind = FindFirstFile( ptmp, pfd );
            if( VFH(hFind) )
            {
               FindClose(hFind);
               //if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
               return(TRUE);
            }
         }
      }
   }

   return(FALSE);
}

BOOL  NotInXDL( DIRECT dir, LPTSTR name, PFD pfd )
{
   // assume NOT in LIST
   PLE   pH = &gsXDirsList;
   if( !IsListEmpty(pH) )
   {
      if( Match2XList( pH, name, IS_DIR ) ) // if it IS IN THE LIST
      {
         dir_add2missedfiles( dir, name, pfd );   // add it to a skipped list kept
         return FALSE;  // then DUMP this DIRECTORY
      }
   }

   if( gbExclude )   // if we have an ACTIVE exclude LIST
   {
      if( MatchesExclude( name, IS_DIR ) )
      {
         dir_add2missedfiles( dir, name, pfd );
         return FALSE;  // dump this DIRECTORY
      }
   }
   return TRUE;
}

// FIX20071020 - check if THIS directory component is IN g_sInDirs
int IsInIDirList( PTSTR pb )
{
   static XLST _s_xlst;
   int iret = 0;  // assume NOT in LIST
   PLE   pH = &g_sInDirs;
   PTSTR lpb2 = &_s_xlst.x_szBody[0];
   PTSTR lpe2 = &_s_xlst.x_szExt[0];
   PLE   pN;
   if( strchr(pb,'.' ) ) { // if we have a FULL STOP in the input
      SplitExt2( lpb2, lpe2, pb ); // then split it into two
   } else {
      strcpy( lpb2, pb ); // if NO full stop, then NO EXTENT, just 'body'
      *lpe2 = 0;
   }

   Traverse_List( pH, pN )
   {
      PXLST pil = (PXLST)pN;
      PTSTR lpb1 = &pil->x_szBody[0];
      PTSTR lpe1 = &pil->x_szExt[0];
      if( ( WildComp2( lpb1, lpb2 ) ) &&
          ( WildComp2( lpe1, lpe2 ) ) )
      {
         iret = 1;
         break;
      }
   }
   return iret;
}

//function
int CheckIDirectory( PTSTR path )
{
   static TCHAR _s_checkIbuff[MAX_PATH * 2];
   int iret = 1; // assume IS in LIST, which it IS if list is NULL/EMPTY!!!
   PLE   pH = &g_sInDirs;
   if( !IsListEmpty(pH) ) {
      DWORD dwi, len, len2, c;
      PTSTR pb = _s_checkIbuff; // build components of this path into here
      len = strlen(path);
      len2 = 0;
      iret = 0; // assume NOT in the LIST
      for(dwi = 0; dwi < len; dwi++) {
         c = path[dwi];
         if(( c == '\\' ) || ( c == '/' )) {
            if(( dwi + 1 ) < len ) {
               // not LAST char
               pb[len2] = 0;
               if( len2 && IsInIDirList(pb) ) { // FIX20071020 - check g_sInDirs -I:d:<dir>
                  // THIS directory component is IN g_sInDirs
                  return 1;   // this COMPONENT is in INCLUDE dir list
               }
               len2 = 0;      // start again
            }
         } else {
            pb[len2++] = (TCHAR)c;
         }
      }
      pb[len2] = 0;
      if( len2 ) {
         iret = IsInIDirList( pb );
      }
   }
   return iret;
}

void Process_Dir_Entry( DIRECT dir, PTSTR name, PFD pfd, BOOL bZip, int isi )
{
   if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
   {
      dir->dwDirsTot++;   // got a DIRECTORY
      if( ( strcmp(name, "." ) != 0  ) &&
          ( strcmp(name, "..") != 0  ) )
      {
         g_dwScDirs++;
         if( NotInXDL(dir, name, pfd) )
         {
            dir_adddirect(dir, name, pfd, bZip);
         }
         else
         {
            dir->dwDirsExcl++;  // count an EXCLUDED directory
         }
      }
   }
   else    // if( !bIsDir )
   {
      dir->dwFilsTot++;
      g_dwScFiles++;
         //dir_addfile(dir, name, filesize);
      if( isi ) {
         dir_addfile2(dir, name, pfd);
      } else {
         dir->dwFilsExcl++;   // count an EXCLUDED
         dir_add2missedfiles( dir, name, pfd );
      }
   }
}

/***************************************************************************
 * Function: dir_scan
 *
 * Purpose:
 *
 * Scan the directory given. Add all files to the list
 * in alphabetic order, and add all directories in alphabetic
 * order to the list of child DIRITEMs. If bRecurse is true, go on to
 * recursive call dir_scan for each of the child DIRITEMs
 *
 * If file matches file in exclude list, EXCLUDE from list
 *
 */
void
dir_scan(DIRECT dir, BOOL bRecurse, INT level)
{
   //LPTSTR    lpb = &g_szBuf1[0];
   // PSTR      path = &g_szBuf2[0];
   PSTR  path = 0;   // (PSTR)MALLOC(264+32);
   int   isidir = 1; // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
        //int       size;
        DIRECT    child;
        BOOL      bMore;
        //long      filesize;
        //BOOL      bIsDir;
        LPSTR     name;
        HANDLE    hFind;
        //WIN32_FIND_DATA finddata;
        PFD       pfd = &g_sFD;
        //char debugmsg[200];
   long  cnt;

   path = (PSTR)MALLOC( (264+32) );
   if(!path)
      return;

        dir->dwDirsTot  = 0;
        dir->dwDirsExcl = 0;
        dir->dwFilsTot  = 0;
        dir->dwFilsExcl = 0;
        //sprintf(lpb, "scandir: %s %s\n",
        //         dir->relname, bRecurse?"recursive":"non-recursive" );
        if( dir->d_prelname && gbAddRelNm )
        {
           strcpy( path, "Comparing dir [" );
           //strcpy( path, "Comparing..." );
           // strcpy( path, LoadRcString(IDS_COMPARING) );   // load string into set of
           strcat( path, dir->d_prelname );
           strcat( path, "]... " );
           SetStatus(path);
        }

        /* make the complete search string including *.* */
        //size  = strlen(dir->head->rootname);
        //size += strlen(dir->relname);
        /* add on one null and *.* */
        //size += 4;
        //path = LocalLock(LocalAlloc(LHND, size));

        strcpy( path, dir->head->rootname );

        /* omit the . at the beginning of the relname, and the
         * .\ if there is a trailing \ on the rootname
         */
        if( *CharPrev( path, (path + strlen(path)) ) == '\\' )
                strcat(path, &dir->d_prelname[2]);
        else
                strcat(path, &dir->d_prelname[1]);

        // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
        isidir = CheckIDirectory( path );

        strcat(path, "*.*");

        /* read all entries in the directory */
        //hFind = FindFirstFile(path, &finddata);
        hFind = FindFirstFile( path, pfd );
        //bMore = ( hFind && ( hFind != INVALID_HANDLE_VALUE ) );   // (HANDLE) -1);
        //if( hFind && ( hFind != INVALID_HANDLE_VALUE ) )   // (HANDLE) -1);
        if( VFH(hFind) )   // (HANDLE) -1);
           bMore = TRUE;
        else
           bMore = FALSE;

        if( !gbAddRelNm )
        {
           LPTSTR   lpb  = &gszTmpBuf[0];
           sprintf(lpb, "Scanning: [%s] ...",
              path );
           if(!bMore)
              strcat(lpb,"None!...");

           SetStatus(lpb); // set STATUS text, and
           SBSetTimedTxt(lpb, -1, FALSE);
           sprtf("%s"MEOR, lpb); // add to LOG
        }

        //LocalUnlock(LocalHandle ( (PSTR) path));
        //LocalFree(LocalHandle ( (PSTR) path));
        name = (LPSTR) &pfd->cFileName[0];
        cnt = 0;
        while( bMore )
        {
           cnt++;
           // FIX20091125 // FIX20091231 - If -r- on command line
           if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
               // if a DIRECTORY
               if ( !g_bNOTRecursive ) { // FIX20091125 // and NO -r-, then ADD IT
                   // would prefer perhaps add it, but mark it scanned
                   // but that can come later...
                   Process_Dir_Entry( dir, name, pfd, FALSE, isidir );
               }
           } else {
               // if a FILE
               Process_Dir_Entry( dir, name, pfd, FALSE, isidir );
           }
           bMore = FindNextFile( hFind, pfd );
        }

        //if( hFind && ( hFind != INVALID_HANDLE_VALUE ) )   // (HANDLE) -1);
        if( VFH(hFind) )   // (HANDLE) -1);
        {
           FindClose(hFind);
        }

        dir->d_bScanned = TRUE;

        dir->pos = DL_FILES;

        if( !gbAddRelNm )
        {
           LPTSTR   lpb  = &gszTmpBuf[0];
           sprintf(lpb, "Scanned: [%s] Fnd dirs(excl)=%d(-%d) fils(excl)=%d(-%d)...",
              path,
              dir->dwDirsTot,
              dir->dwDirsExcl,
              dir->dwFilsTot,
              dir->dwFilsExcl );

           SetStatus(lpb); // set STATUS text, and
           SBSetTimedTxt(lpb, 5, TRUE);
           sprtf("%s"MEOR, lpb); // add to LOG
        }

        if( bRecurse )
        {
           List_TRAVERSE(dir->directs, child)
           {
              //if( dir->d_bIsZip )
              //   dir_scan_zip(child, TRUE); // #ifdef ADD_ZIP_SUPPORT
              //else
                 dir_scan(child, TRUE, (level+1) );
           }
        }

   MFREE(path);   // toss the memory

} /* dir_scan */


/***************************************************************************
 * Function: dir_addfile
 *
 * Purpose:
 *
 * Add the file 'path' to the list of files in dir, in order.
 */
void
dir_addfile_NOT_USED(DIRECT dir, LPSTR path, long size)
{
        DIRITEM pfile;

        //AnsiLowerBuff(path, lstrlen(path));  // needless if utils_CompPath() take account?
        // Added a utils_CompPath2() that does it own case thing!

        List_TRAVERSE(dir->diritems, pfile) {
                /////if (lstrcmpi(pfile->name, path) > 0) {
                //if (utils_CompPath(pfile->name, path) > 0) {
                if( utils_CompPath2( pfile->di_pname, path ) > 0 )
                {
                   /* goes before this one */
                   pfile = List_NewBefore(dir->diritems, pfile, sizeof(struct diritem));
//                   dir_fileinit(pfile, dir, path, size);
                   return;
                }
        }
        /* goes at end */
        pfile = List_NewLast(dir->diritems, sizeof(struct diritem));
//        dir_fileinit(pfile, dir, path, size);
} /* dir_addfile */


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : dir_addfile2
// Return type: VOID 
// Arguments  : DIRECT dir
//            : LPSTR path
//            : PFD pfd = WIN32_FIND_DATA
// Description: Add a FILE from a folder to the LIST, in alphabetic order
//              But IF gbExclude is TRUE the name must be first checked
// in the PLE pH = &gsExclLists[?]; list. Do NOT add the file is EXCLUDED
// Also if it MATCHES -xf:<name> command line list, if any, then DUMP
//
///////////////////////////////////////////////////////////////////////////////
typedef struct tagMSDFIL {
   LIST_ENTRY  list; // LIST ENTRY structure
   FD          fd,fd2;  // file data - fd2 for right
   DWORD       dwflg;
   TCHAR       path[264];
}MSDFIL, * PMSDFIL;

static MSDFIL _s_sMSDFIL;
//LIST_ENTRY  g_missedfiles = { &g_missedfiles, &g_missedfiles };
//DWORD       g_dwmissedcnt = 0;
DWORD       g_dwdupedcnt = 0;
typedef struct tagMSDSTATS {
   DWORD dw_msd_total; // total files in list
   DWORD dw_msd_dupes; // files with same relative names
   DWORD dw_msd_files; // file item
   DWORD dw_msd_dirs;  // directory item
   DWORD dwmaxlen;   // max lenght of relative path
   FILETIME ftearliest; // ealiest date
   FILETIME ftlatest;   // latest date
   LARGE_INTEGER  liTot, liMax, liMin; // total bytes
}MSDSTATS, * PMSDSTATS;

MSDSTATS g_sMSDSTATS[3];
PMSDSTATS   g_pms = &g_sMSDSTATS[0];
#define  g_xtotal (*g_pms).dw_msd_total
#define  g_xdupes (*g_pms).dw_msd_dupes
#define  g_xfiles (*g_pms).dw_msd_files
#define  g_xdirs  (*g_pms).dw_msd_dirs
#define  g_xmaxlen (*g_pms).dwmaxlen
#define  g_xearly (*g_pms).ftearliest
#define  g_xlate  (*g_pms).ftlatest
#define  g_xliTot (*g_pms).liTot
#define  g_xliMax (*g_pms).liMax
#define  g_xliMin (*g_pms).liMin

void Add_Stat_Info( PTSTR lpb )
{
   sprintf(EndBuf(lpb),
      "Scanned %u dirs, found %u files.",
      g_xdirs, g_xfiles );
}

DWORD getmisscnt( VOID )
{
// return( g_xtotal );
   return( g_xtotal + g_xdupes );   // the GRAND total
}
VOID  resetmisscnt( VOID )
{
   ZeroMemory(g_pms, sizeof(MSDSTATS));
   g_xliMin.LowPart = (DWORD)-1;
}

INT   InitExcludeList( HWND hwnd )
{
   INT      icnt = 0;
   LPTSTR   lpb = &gszTmpBuf[0];
   PLE      ph = &gsXMissList;
   PLE      pn;
   LPTSTR   lpf;
   LRESULT  lr;
   INT      i;
   PMSDFIL  pmf;
   DWORD    dwl,dwmax;
   SYSTEMTIME     st;
   LARGE_INTEGER  li;


   SendMessage( hwnd, LB_RESETCONTENT, 0, 0 );

   ListCount2(ph,&i);

   if( i == 0 )
   {
      ShowWindow(hwnd, SW_HIDE);
      EnableWindow(hwnd, FALSE);
      return 0;
   }
   if( g_hfCN8 )
      SendMessage(hwnd, WM_SETFONT, (WPARAM) g_hfCN8, 0L);
   else if( g_hFixedFont8 )
      SendMessage(hwnd, WM_SETFONT, (WPARAM) g_hFixedFont8, 0L);
   else
   {
      LOGFONT        lf;
      HFONT          hf;
      GetObject( GetStockObject (SYSTEM_FIXED_FONT), sizeof (LOGFONT), (LPSTR) &lf);
      hf = CreateFontIndirect(&lf);
      if(hf)
         SendMessage(hwnd, WM_SETFONT, (WPARAM) hf, 0L);
   }

   *lpb = 0;
   dwmax = g_xmaxlen;
   if( dwmax == 0 )
   {
      Traverse_List( ph, pn )
      {
         pmf = (PMSDFIL)pn;
         lpf = &pmf->path[0]; // get path pointer
         dwl = strlen(lpf);
         if(dwl > dwmax)
            dwmax = dwl;
      }
   }

   Traverse_List( ph, pn )
   {
      pmf = (PMSDFIL)pn;
      lpf = &pmf->path[0]; // get path pointer
      dwl = strlen(lpf);
      strcpy(lpb,lpf);
      strcat(lpb," ");
      while(strlen(lpb) < dwmax)
         strcat(lpb," ");

      //FT2LST( &pta->ta_sFDL.ftLastWriteTime, &st );
      FT2LST( &pmf->fd.ftLastWriteTime, &st );
      AppendDateTime( lpb, &st );
      strcat(lpb, " ");
      if( pmf->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
      {
         strcat( lpb, " <DIR>");
      }
      else
      {
         PSTR  pstr;
         li.LowPart  = pmf->fd.nFileSizeLow;
         li.HighPart = pmf->fd.nFileSizeHigh;
         pstr = GetI64Stg( &li );
         i = strlen(pstr);
         strcat(lpb," ");
         i++;
         //while( i < 14 )
         while( i < 9 )
         {
            strcat(lpb," ");
            i++;
         }
         strcat(lpb, pstr);
      }

      lr = SendMessage( hwnd,   // handle to destination window 
               LB_INSERTSTRING,          // message to send
               (WPARAM)-1,          // item index
               (LPARAM) lpb );      // string (LPCTSTR)

      if(( lr == LB_ERR ) ||
         ( lr == LB_ERRSPACE ) )
      {
         return (INT)-1;
      }
      icnt++;
   }

      // IF multiple selection LIST BOX, use
      //SendMessage( hwnd, LB_SETSEL, (WPARAM) TRUE, (LPARAM) lr ); // item index, else
//      SendMessage( hwnd, LB_SETCURSEL, (WPARAM)lr, 0 );  // in SINGLE selection LIST BOX
//      sprtf( "LB_SETSEL: Selection set to index %d."MEOR, lr );

   return icnt;
}



VOID  dir_add2missedfiles( DIRECT dir, LPTSTR path, PFD pfd )
{
   DWORD dwl;
   PLE   pn;
//   PLE   ph = &g_missedfiles;
   PLE   ph = &gsXMissList;
   PLE   pnn = 0;  // new structure = (PLE)MALLOC( sizeof(MSDFIL) );
   PMSDFIL  pmf = &_s_sMSDFIL;  // = (PMSDFIL)pnn;
   PMSDFIL  pmf2;
   LPTSTR   lpf1, lpf2;
   LPTSTR   prn = dir->d_prelname;
   LARGE_INTEGER  li;

   lpf1 = &pmf->path[0];   // build a FULL PATH NAME
   *lpf1 = 0;

#ifdef   ADD_ROOT_NAME
   if( dir->head->rootname )
      strcpy( lpf1, dir->head->rootname );
#endif   // #ifdef   ADD_ROOT_NAME

   dwl = strlen(lpf1);
   if(dwl)
   {
      if(prn)
      {
         if(lpf1[dwl-1] == '\\')
            strcat(lpf1, &prn[2] );
         else
            strcat(lpf1, &prn[1] );
      }
   }
   else
   {
      if(prn)
         strcat(lpf1, prn);
   }
   strcat(lpf1,path);

   Traverse_List( ph, pn )
   {
      pmf2 = (PMSDFIL)pn;
      lpf2 = &pmf2->path[0];
      if( strcmpi( lpf1, lpf2 ) == 0 )
      {
//         g_dwdupedcnt++;
         if(( pmf2->dwflg & tf_IsLeft ) &&
            ( g_dwActScan & tf_IsRight) )  // keep left/right and zip flags
         {
            memcpy(&pmf2->fd2, pfd, sizeof(FD));
            pmf2->dwflg |= tf_IsRight; // show we have TWO files
         }
         else if(( pmf2->dwflg & tf_IsRight) &&
            ( g_dwActScan & tf_IsLeft ) )  // keep left/right and zip flags
         {
            memcpy(&pmf2->fd2, pfd, sizeof(FD));
            pmf2->dwflg |= tf_IsLeft; // show we have TWO files
         }

         g_xdupes++;

         pmf = pmf2;
         goto Add_Item;
         //return;
      }
   }

   pnn = (PLE)MALLOC( sizeof(MSDFIL) );
   pmf = (PMSDFIL)pnn;
   if(!pmf)
   {
      chkme( "C:ERROR: Memory FAILED" );
      return;
   }

   // set the new file pointer
   lpf2 = &pmf->path[0];
   strcpy(lpf2,lpf1);   // copy in the relative name

   memcpy(&pmf->fd, pfd, sizeof(FD));

   pmf->dwflg = g_dwActScan;  // keep left/right and zip flags

//   g_dwmissedcnt++;
   g_xtotal++;
   dwl = strlen(lpf1);
   if( dwl > g_xmaxlen )
      g_xmaxlen = dwl;

Add_Item:

   if( pmf->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
   {
      g_xdirs++;  // note - we do NOT know how many FILES and FOLDERS were missed really
   }
   else
   {
      li.LowPart = pmf->fd.nFileSizeLow;
      li.HighPart = pmf->fd.nFileSizeHigh;
      if( li.QuadPart > g_xliMax.QuadPart )
         g_xliMax.QuadPart = li.QuadPart;
      if( li.QuadPart < g_xliMin.QuadPart )
         g_xliMin.QuadPart = li.QuadPart;
      g_xliTot.QuadPart += li.QuadPart;   // accumulate TOTAL bytes excluded
      g_xfiles++;
   }

   if(pnn)
   {
      // select where to put it in the list
      if( g_bSortExcl )
      {
         Traverse_List( ph, pn )
         {
            pmf2 = (PMSDFIL)pn;
            lpf2 = &pmf2->path[0];
            if( utils_CompPath2( lpf1, lpf2) > 0 )
            {
               InsertBefore(ph,pnn);
               return;
            }
         }
      }
   
      InsertTailList(ph,pnn);
   }

}

VOID dbgstop( VOID )
{
   int i;
   i = 0;
}

VOID  dir_addfile2(DIRECT dir, LPSTR path, PFD pfd)
{
   DIRITEM  pfile;
   PLE      pH = &gsXFileList;   // the EXCLUDE file list

   //AnsiLowerBuff(path, lstrlen(path));  // needless?
   //if( strcmpi(path,"js.obj") == 0 )
   //   dbgstop();

   if( !IsListEmpty(pH) )
   {
      if( Match2XList( pH, path, IS_FILE ) ) // if it IS IN THE LIST
      {
         dir->dwFilsExcl++;   // count an EXCLUDED
         dir_add2missedfiles( dir, path, pfd );
         return;  // all done
      }
   }

   if( gbExclude )   // if we have an ACTIVE exclude LIST
   {
      if( MatchesExclude( path, IS_FILE ) )
      {
         dir->dwFilsExcl++;   // count an EXCLUDED
         dir_add2missedfiles( dir, path, pfd );
         return;  // all done
      }
   }

   if( !IsListEmpty( &g_sInFiles ) ) { // -I<mask> or dtat\root* input file list
      PLE ph = &g_sInFiles;
      PLE pn;
      BOOL  inc = FALSE; // assume NOT
      //if( strcmpi(path, "Repository") == 0 ) {
      //   dbgstop();
      //}
      Traverse_List( ph, pn ) {
         PXLST pil = (PXLST)pn;
         if( MatchWild2( path, pil->x_szBody, pil->x_szExt ) ) {
            inc = TRUE;
            break;
         }
      }
      if( !inc ) {
         dir_add2missedfiles( dir, path, pfd );
         return;
      }
   }
   
   List_TRAVERSE(dir->diritems, pfile)
   {
      /////if (lstrcmpi(pfile->name, path) > 0) {
      if( utils_CompPath2(pfile->di_pname, path) > 0 )
      {
         /* goes before this one */
         pfile = List_NewBefore(dir->diritems, pfile, sizeof(struct diritem));
         dir_fileinit2(pfile, dir, path, pfd);
         return;
      }
   }

   /* goes at end */
   pfile = List_NewLast(dir->diritems, sizeof(struct diritem));
   dir_fileinit2(pfile, dir, path, pfd);

} /* dir_addfile2 */


/***************************************************************************
 * Function: dir_addirect
 *
 * Purpose:
 *
 * Add a new directory in alphabetic order on
 * the list dir->directs
 *
 */
void
dir_adddirect(DIRECT dir, LPSTR path, PFD pfd, BOOL bZip)
{
        DIRECT child;
        LPSTR finalel;
        //char achTempName[256];
        LPTSTR    lpb = &g_szBuf1[0];
        //AnsiLowerBuff(path, lstrlen(path));
        List_TRAVERSE(dir->directs, child)
        {

                int cmpval;
                /* we need to compare the child name with the new name.
                 * the child name is a relname with a trailing
                 * slash - so compare only the name up to but
                 * not including the final slash.
                 */
                finalel = dir_finalelem(child->d_prelname);
                /*
                 * we cannot use strnicmp since this uses a different
                 * collating sequence to lstrcmpi. So copy the portion
                 * we are interested in to a null-term. buffer.
                 */
                //strncpy(achTempName, finalel, strlen(finalel)-1);
                //achTempName[strlen(finalel)-1] = '\0';
                //cmpval = utils_CompPath(achTempName, path);

                strncpy(lpb, finalel, strlen(finalel)-1);
                lpb[strlen(finalel)-1] = '\0';
                cmpval = utils_CompPath2(lpb, path);
                if( cmpval > 0 )
                {
                        /* goes before this one */
                        child = List_NewBefore(dir->directs, child, sizeof(struct direct));
                        dir_dirinit(child, dir->head, dir, path, pfd);
                        return;
                }
        }

        /* goes at end */
        child = List_NewLast(dir->directs, sizeof(struct direct));
        dir_dirinit(child, dir->head, dir, path, pfd);

} /* dir_adddirect */


/***************************************************************************
 * Function: dir_cleardirect
 *
 * Purpose:
 *
 * Free all memory associated with a DIRECT (including freeing
 * child lists). Don't de-alloc the direct itself (allocated on a list)
 */
void
dir_cleardirect(DIRECT dir)
{
        DIRITEM pfile;
        DIRECT child;

        /* clear contents of files list */
        List_TRAVERSE(dir->diritems, pfile)
        {
           gmem_free(hHeap, pfile->di_pname, (strlen(pfile->di_pname) + 1), "dir_fileinit" );
           pfile->di_pname = 0;  // ZERO when released

                if( (pfile->localname) && (pfile->bLocalIsTemp) )
                {

                        /*
                         * the copy will have copied the attributes,
                         * including read-only. We should unset this bit
                         * so we can delete the temp file.
                         */
                        SetFileAttributes(pfile->localname,
                                GetFileAttributes(pfile->localname)
                                        & ~FILE_ATTRIBUTE_READONLY);
                        DeleteFile(pfile->localname);
                   gmem_free(hHeap, pfile->localname, 256, "NOT USED!" );
                        pfile->localname = NULL;
                }

        }

        List_Destroy(&dir->diritems);

        /* clear contents of dirs list (recursively) */
        List_TRAVERSE(dir->directs, child)
        {
                dir_cleardirect(child);
        }
        List_Destroy(&dir->directs);

       gmem_free(hHeap, dir->d_prelname, (strlen(dir->d_prelname) + 1), "dir_dirinit" );

       dir->d_prelname = NULL;

} /* dir_cleardirect */

/***************************************************************************
 * Function: dir_finalelem
 *
 * Purpose:
 *
 * Return a pointer to the final element in a path. Note that
 * we may be passed relnames with a trailing final slash - ignore this
 * and return the element before that final slash.
 *
 */
LPSTR
dir_finalelem(LPSTR path)
{
        LPSTR chp;
        int size;

        /* is the final character a slash ? */
        size = strlen(path) - 1;
        if( *(chp = CharPrev(path, (path + strlen(path)) )) == '\\')
        {
                /* find the slash before this */
                while (chp > path)
                {
                        if (*(chp = CharPrev(path, chp)) == '\\')
                        {
                                /* skip the slash itself */
                                chp++;
                                break;
                        }
                }
                return(chp);
        }

        // ELSE, no trailing back slash
        /* look for final slash */
        chp = strrchr(path, '\\');
        if(chp != NULL)
        {
                return(chp+1);
        }

        /* no slash - is there a drive letter ? */
        chp = strrchr(path, ':');
        if (chp != NULL)
        {
                return(chp+1);
        }

        /* this is a final-element anyway */
        return(path);

} /* dir_finalelem */

/***************************************************************************
 * Function: dir_getpathsize
 *
 * Purpose:
 *
 * Find the size of a file given a pathname to it 
 * FIX2020      - use stat
 *
 */
long
dir_getpathsize(LPSTR path)
{
    struct stat buf;
    if (stat(path, &buf) == 0)
        return buf.st_size;
    return 0;
} /* dir_getpathsize */


/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// NEW

/***************************************************************************
 * Function: dir_copy
 *
 * Purpose:
 *
 * Create a copy of the file, in the new root directory. Creates sub-dirs as
 * necessary. 
 *
 * Returns TRUE for success and FALSE for failure.
 */
//BOOL dir_copy(DIRITEM item, LPTSTR pdroot, PVOID pv1, PVOID pv2)
BOOL dir_copy( PVOID pv1, PVOID pv2 )
{
   BOOL        bOK = FALSE;
   PCFDLGSTR   pcfds = (PCFDLGSTR)pv2;
   PCPYTST     pct   = (PCPYTST)pv1;
   LPTSTR      pmsg  = &g_szMsg[0];
   LPSTR       relname, fullname;
   LPSTR       pstart, pdest, pel;
   HANDLE      hFind;
   LPTSTR      pnp = &g_szNewPath[0];
   INT_PTR     i, state;
   DIRITEM     item;
   LPTSTR      pdroot;
   BOOL        bIsDir;
   PFD         pfdc = &pct->ct_sFDCpy;    // just for any EXISTING DESTINATION file
   PFD         pfds = &pct->ct_sFDSrc;    // this is the SOURCE information
   DWORD       dwVerFlag, dwCpyOpts;

   *pmsg = 0;
   if( !VALIDPCFDS( pcfds ) )
   {
      strcpy( pmsg, "Copy FAILED because PCFDLGSTR is INVALID!"MEOR );
      return FALSE;
   }
   item = pct->ct_diritem;
   if( !item )
   {
      strcpy( pmsg, "Copy FAILED because DIRITEM is NULL!"MEOR );
      return FALSE;
   }

   relname = 0;
   fullname = 0;

   /* check that the newroot directory itself exists */
   // ********* THE DESTINATION ************
   pdroot = &pcfds->cf_szDest[0];   // ROOT OF DESTINATION MUST EXIST
   bIsDir = dir_isvaliddir(pdroot);
   if( !bIsDir )
   {
      if( pcfds->cf_bExpanded )  // pcfds->cf_iSelected = iSel;
      {
         if( !dir_isvalidfile(pdroot) )
         {
            sprintf( pmsg, "Action FAILED because"MEOR
               "dest root=[%s]"MEOR
               "is NOT A VALID file or folder!"MEOR,
               pdroot );
            return FALSE;
         }
      }
      else
      {
            sprintf( pmsg, "Action FAILED because"MEOR
               "dest root=[%s]"MEOR
               "is NOT A VALID folder!"MEOR,
               pdroot );
            return FALSE;
      }
   }

   /* name of file relative to the tree root */
   relname  = dir_getrelname(item);
   fullname = dir_getfullname(item);
   dwVerFlag = pcfds->dwVerFlag; // extract verification FLAG
   dwCpyOpts = pcfds->dwCpyOpts; // and the COPY OPTIONS FLAG

   state = pct->ct_iState; // extract the STATE

   /*
    * build the new pathname by concatenating the new root and
    * the old relative name. add one path element at a time and
    * ensure that the directory exists, creating it if necessary.
    */
   strcpy(pnp, pdroot);
   strcpy( &pct->ct_szCopyFull[0], fullname );  // copy the FULL name
   strcpy( &pct->ct_szCopyTo[0], pnp );         // and the FULL destination name

   if( bIsDir )
   {
      /* add separating slash if not already there */
      // The CharPrev function retrieves a pointer to
      // the preceding character in a string. 
      // Parameters
      //    lpszStart [in] Pointer to the beginning of the string.
      //    lpszCurrent [in] Pointer to a character in a null-terminated string. 
      if( *CharPrev(pnp, (pnp + strlen(pnp)) ) != '\\' )
         strcat(pnp, "\\");
   
      pstart = relname;
      while( (pel = strchr(pstart, '\\')) != NULL )
      {
         /* found another element ending in slash. incr past the "\" */
         pel++;
            /* ignore the simple root name of ".\"! */
         if( strncmp(pstart, ".\\", 2) != 0 )
         {
            pdest = &pnp[strlen(pnp)];
            strncpy(pdest, pstart, pel - pstart);
            pdest[pel - pstart] = '\0';
   
            /* create subdir if necessary */
            if( !dir_isvaliddir(pnp) )
            {
//               if( pcfds->dwCpyOpts & DELETE_ONLY )   // = INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
//               if( pcfds->dwCpyOpts & INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
               if( dwCpyOpts & INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
               {
                  sprintf(EndBuf(pmsg),
                     "Delete FAILED as folder does NOT exist!"MEOR
                     "[%s] directory!"MEOR,
                     pnp );
                  goto Tidy_Exit;
               }

               if( _mkdir(pnp) != 0 ) { // with FIX20090128
                  // oops, can not create this new folder!
                  // FIX20090128
                  // THIS COULD BE BECAUSE THERE IS ALREADY A FILE OF THIS NAME
                  PTSTR ptmp = GetNxtBuf();
                  strcpy(ptmp,pnp);
                  if( *CharPrev(ptmp, (ptmp + strlen(ptmp)) ) == '\\' )
                     ptmp[ strlen(ptmp) - 1 ] = 0;
                  if( dir_isvalidfile(ptmp) ) {
                     if( dir_filedelete( ptmp ) ) {
                        if( _mkdir(pnp) == 0 ) {   // after FIX20090128
                           goto Done_Create; // FIX20090128 - done it
                        }
                     }
                     sprintf(EndBuf(pmsg),
                        "Copy FAILED as unable to create"MEOR
                        "[%s] directory!"MEOR
                        "Due to a FILE of the SAME name exists!"MEOR,
                        pnp );
                  } else {
                     sprintf(EndBuf(pmsg),
                        "Copy FAILED as unable to create"MEOR
                        "[%s] directory!"MEOR,
                        pnp );
                  }
                  goto Tidy_Exit;

               }
               else
               {
Done_Create:
                  sprintf(EndBuf(pmsg),
                     "Created [%s]"MEOR,
                     pnp );
               }
            }
         }
         pstart = pel;
      }
   
      /* there are no more slashes, so pstart points at the final element */
      strcat(pnp, pstart); // final destination qualified path if COPY, 
      // OR the file to DELETE if DELETE_ONLY case
   }

   // *** SPECIAL DELETE CASE ***
   // NOTE: Even though it is COPY from LEFT,
   // is this a RIGHT ONLY file, thus the DIRITEM is the RIGHT
   // thus they HAVE TO BE THE SAME **************************
//#ifdef   ADDUPDATE2
//   if( pcfds->dwCpyOpts & DELETE_ONLY )   // = INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
//   if(( pcfds->dwCpyOpts & INCLUDE_RIGHTONLY ) &&   // *** DELETE CASE ***
   if(( dwCpyOpts & INCLUDE_RIGHTONLY ) &&   // *** DELETE CASE ***
      ( state == STATE_FILERIGHTONLY  ) )
   {
      // compare the SOURCE with DESTINATION file
      if( strcmpi( fullname, pnp ) )
      {
         sprintf( EndBuf(pmsg), "WHOA: Got "MEOR
            "[%s] and "MEOR
            "[%s] NOT SAME????",
            fullname,
            pnp );
         chkme( pmsg );
         goto Tidy_Exit;
      }

      //if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH) )
      hFind = FindFirstFile( pnp, pfdc ); // find information - DESTINATION file
      if( VFH(hFind) )
      {
         FindClose(hFind); // done with this

         if( ( dwVerFlag & VERIFY_EACH ) ||
            !( (dwVerFlag & REVIEW_LIST) || (dwVerFlag & MAKE_BACKUP) ) )
         {
            if( dwVerFlag & VERIFY_EACH )
               *pmsg = 0;
            else
            {
               strcpy( pmsg, "NOTE: NO verify, review or backup flags"MEOR
                  "then each DELETE will be verified"MEOR
                  "unless [Copy All] chosen, when the backup FLAG"MEOR
                  "will be added."MEOR );
            }
            pct->ct_dwFlag &= ~(TT_FLAGS);
            pct->ct_dwFlag |= AddToolText( pmsg, pct->ct_ci );
            AddOKCancel( pmsg );
            i = MB2(NULL,pmsg,
               "*** VERIFY FILE DELETE ***",
               SD_MB_DEF );   // (MB_ICONINFORMATION|MB_OKCANCEL));
            if( i == IDC_COPYALL )
            {
               pcfds->dwVerFlag &= ~(VERIFY_EACH | VERIFY_OLDER);
               pcfds->dwVerFlag |= MAKE_BACKUP;
               dwVerFlag = pcfds->dwVerFlag;
            }
            else if( i == IDC_ABORTALL )
            {
                pct->ct_dwFlag |= (flg_Abort|flg_User);
                goto Tidy_Exit;
            }
            else if( i == IDCANCEL )
            {
               pct->ct_dwFlag |= flg_User;   // flag as a USER skip
               goto Tidy_Exit;
            }
         }

         if( pcfds->dwVerFlag & MAKE_BACKUP )
         {
            bOK = dir_filedelete( pnp );
            if( bOK )
            {
               pcfds->cf_liToBin.LowPart  += pfdc->nFileSizeLow;
               pcfds->cf_liToBin.HighPart += pfdc->nFileSizeHigh;
            }
         }
         else
         {
            bOK = DeleteFile( pnp );
         }
         if( bOK )
         {
            pcfds->cf_liDeleted.LowPart  += pfdc->nFileSizeLow;
            pcfds->cf_liDeleted.HighPart += pfdc->nFileSizeHigh;
         }
      }
      else
      {
         sprintf( EndBuf(pmsg),
            "DELETE failed due to destination"MEOR
            "[%s]"MEOR
            "DOES NOT EXIST!"MEOR,
            pnp );
      }
      goto Tidy_Exit;   // done all we can here

   }

// #endif   // #ifdef   ADDUPDATE2
   // ok, it really is COPY (and NOT delete
   // *************************************
   if( strcmpi( fullname, pnp ) == 0 )
   {
      sprintf( EndBuf(pmsg),
         "Copy WILL fail due to source"MEOR
         "[%s] and destination"MEOR
         "[%s] are equivalent!"MEOR,
         fullname,
         pnp );
      goto Tidy_Exit;
   }

   hFind = FindFirstFile( fullname, pfds );  // get SOURCE information
   if( VFH(hFind) )  // this was ALREADY done in dir_copytest() but,
   {                 // it COULD have been REMOVED meantime
      BOOL  bvdst = FALSE; // assume DESTINATION is NOT valid

      FindClose(hFind); // finished with the SOURCE find.

      // now the destination
      hFind = FindFirstFile( pnp, pfdc ); // COPY find file information
      if(VFH(hFind))
         bvdst = TRUE;  // *** MARK THE SOURCE PFD AS VALID ***

      if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER) )
      {
         // if destination exist, move to 'Recyle Bin' (I HOPE)
         // hFind = FindFirstFile( pnp, pfdc ); // COPY find file information
         if( pcfds->dwVerFlag & (VERIFY_EACH | VERIFY_OLDER) )
         {
            *pmsg = 0;
            pct->ct_dwFlag &= ~(TT_FLAGS);
            pct->ct_dwFlag |= AddToolText( pmsg, pct->ct_ci );
            if( ( pcfds->dwVerFlag & VERIFY_EACH ) ||
                ( pct->ct_dwFlag & TT_YOUNGER    ) )
            {
               AddOKCancel( pmsg );
               i = MB2(NULL,pmsg,
                  "VERIFY FILE COPY",
                  SD_MB_DEF );   // (MB_ICONINFORMATION|MB_OKCANCEL));
               if( i == IDC_COPYALL )
               {
                  pcfds->dwVerFlag &= ~(VERIFY_EACH | VERIFY_OLDER);
                  if( VFH(hFind) )
                     FindClose(hFind);
                  hFind = 0;
               }
               else if( i == IDC_ABORTALL )
               {
                  pct->ct_dwFlag |= (flg_Abort|flg_User);
                  if( VFH(hFind) )
                     FindClose(hFind);
                  hFind = 0;
                  goto Tidy_Exit;
               }
               else if( i == IDCANCEL )
               {
                  pct->ct_dwFlag |= flg_User;   // flag as a USER skip
                  if( VFH(hFind) )
                     FindClose(hFind);
                  hFind = 0;
                  goto Tidy_Exit;
               }
            }
         }
         if( VFH(hFind) )
         {
            FindClose(hFind);
            hFind = 0;
            if( pfdc->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
            {
               // must clear attribute first
               SetFileAttributes(pnp, FILE_ATTRIBUTE_ARCHIVE);
            }
            if( pcfds->dwVerFlag & MAKE_BACKUP )
            {
               // copy to backup delete - garbage can - trash can
               if( dir_filedelete( pnp ) )   // if( bOK )
               {
                  pcfds->cf_liToBin.LowPart  += pfdc->nFileSizeLow;
                  pcfds->cf_liToBin.HighPart += pfdc->nFileSizeHigh;
               }
            }
            else
            {
               // delete what is there
               bOK = DeleteFile( pnp );
            }
            if( bOK )
            {
               pcfds->cf_liDeleted.LowPart  += pfdc->nFileSizeLow;
               pcfds->cf_liDeleted.HighPart += pfdc->nFileSizeHigh;
            }
         }
      }
      if( VFH(hFind) )
      {
         FindClose(hFind);
         hFind = 0;
         if( pfdc->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
         {
            // must clear attribute first
            SetFileAttributes(pnp, FILE_ATTRIBUTE_ARCHIVE);
         }
      }

      bOK = CopyFile(fullname, pnp, FALSE); // COPY with OVERWRITE
      if(bOK)
      {
#ifndef  USEOLDSETATS2
         nLocalCopies++;   // set a successful COPY done

         // ensure ACHIVE bit set on a COPY made
         SetFileAttributes(pnp,
               ( pfds->dwFileAttributes | FILE_ATTRIBUTE_ARCHIVE ) );

         // FindFirstFile() - count the VOLUME copied
         pcfds->cf_liCopied.LowPart  += pfds->nFileSizeLow;
         pcfds->cf_liCopied.HighPart += pfds->nFileSizeHigh;

#else // #ifdef  USEOLDSETATS2

         HANDLE   hFile;

         /* having successfuly copied the file, now copy the times, attributes */
         // NOTE: This appears ENTIRELY redundant, since it appears
         // CopyFile() above also set the date/time and attributes
         // ******************************************************
         hfile = CreateFile(pnp, GENERIC_WRITE, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
         // BUT IT IS A GOOD SECOND TEST THAT IT ALL WORKED
         if( VFH(hfile) )
         {
            SetFileTime(hfile, &pfds->ftCreationTime,
                         &pfds->ftLastAccessTime,
                         &pfds->ftLastWriteTime);

            CloseHandle(hfile);
   
            // ensure ACHIVE bit set on a COPY made
            SetFileAttributes(pnp,
               ( pfds->dwFileAttributes | FILE_ATTRIBUTE_ARCHIVE ) );
   
            nLocalCopies++;   // set a successful COPY done

            sprintf(EndBuf(pmsg),
               "Successful copy of"MEOR
               "[%s] completed."MEOR,
               pnp );

            // FindFirstFile()
            pcfds->cf_liCopied.LowPart  += pfds->nFileSizeLow;
            pcfds->cf_liCopied.HighPart += pfds->nFileSizeHigh;

         }
         else
         {
            nLocalFails++;
            bOK = FALSE;
            sprintf(EndBuf(pmsg),
               "Copy FAILED as UNABLE to OPEN"MEOR
               "[%s] that should exist!"MEOR,
               pnp );
         }

#endif   // #ifdef  USEOLDSETATS2

      }
      else
      {
         sprintf(EndBuf(pmsg),
            "CopyFile() FAILED! From"MEOR
            "[%s] to "MEOR
            "[%s]!"MEOR,
            fullname,
            pnp );
      }
   }
   else
   {
      // FAILED to FIND the SOURCE of the COPY
      sprintf( EndBuf(pmsg),
         "Copy FAILED because"MEOR
         "[%s]"MEOR
         "is NOT a VALID FILE!"MEOR,
         fullname );
   }

Tidy_Exit:

   // tidy up on exit
   dir_freerelname( item, relname );
   dir_freefullname(item, fullname);

   return(bOK);

} /* dir_copy */


BOOL  dir_deletetest( PCPYTST pct, DIRITEM diritem )
{
   BOOL  bRet = FALSE;
   LPTSTR pfn = dir_getfullname(diritem);
   if(pfn)
   {
               // is this a valid file - ie can be deleted
//               if( dir_isvalidfile(pfn) )
      if( dir_isvalidfile2(pfn,&pct->ct_sFDDst) )
      {
         memcpy( &pct->ct_sFDSrc, &pct->ct_sFDDst, sizeof(WIN32_FIND_DATA) );
         strcpy( &pct->ct_szCopyFull[0], pfn );  // copy the FULL name
         strcpy( &pct->ct_szCopyRel[0],  pfn );  // and keep RELATIVE name
         SplitFN( NULL, &pct->ct_szSTitle[0], pfn );   // and get JUST the file
//       icnt3++;
         bRet = TRUE;
      }
      dir_freefullname(diritem, pfn);
   }
   return bRet;
}

// actually EXACTLY the SAME as dir_copy, except it DOES nothing
// except reports that it looks possible to do the copy!
BOOL dir_copytest( PVOID pv1, PCPYTST pct )
{
   BOOL        bOK = FALSE;
   PCFDLGSTR   pcfds = (PCFDLGSTR)pv1;
   LPTSTR      pmsg  = &g_szMsg[0];
   LPSTR       relname, fullname;
   LPSTR       pstart, pdest, pel;
   //HANDLE      hfile;
   HANDLE      hFind;
   LPTSTR      pnp = &g_szNewPath[0];
   INT         state;
   DIRITEM     item;
   LPTSTR      pdroot;
   BOOL        bIsDir;
   PFD         pfd = &pct->ct_sFDSrc;
   PFD         pfdc = &pct->ct_sFDCpy;    // just for any EXISTING file
   //PFD         pfds = &pct->ct_sFDSrc;    // this is the SOURCE information
   *pmsg = 0;
   if( !VALIDPCFDS( pcfds ) )
   {
      strcpy( pmsg, "Copy FAILED because PCFDLGSTR is INVALID!"MEOR );
      return FALSE;
   }
   item = pct->ct_diritem;
   if( !item )
   {
      strcpy( pmsg, "Copy FAILED because DIRITEM is NULL!"MEOR );
      return FALSE;
   }

   state = pct->ct_iState; // get the STATE
   pct->ct_NewDir[0] = 0;  // no NEW directory yet

   relname  = 0;
   fullname = 0;
   // OK, decided dir_copytest can include a DELETE right only test


   /* check that the newroot directory itself exists */
   // NOTE: For DELETE this must exist!!!
   pdroot = &pcfds->cf_szDest[0];   // ROOT OF DESTINATION MUST EXIST
   bIsDir = dir_isvaliddir(pdroot); // for EITHER copy or delete functions
   if( !bIsDir )
   {
      if( pcfds->cf_bExpanded )  // pcfds->cf_iSelected = iSel;
      {
         if( !dir_isvalidfile(pdroot) )
         {
            sprintf( pmsg, "Action FAILED because"MEOR
               "dest root=[%s]"MEOR
               "is NOT A VALID file or folder!"MEOR,
               pdroot );
            return FALSE;
         }
      }
      else
      {
            sprintf( pmsg, "Action FAILED because"MEOR
               "dest root=[%s]"MEOR
               "is NOT A VALID folder!"MEOR,
               pdroot );
            return FALSE;
      }
   }

   /* name of file relative to the tree root */
   relname  = dir_getrelname(item);
   fullname = dir_getfullname(item);

   /*
    * build the new pathname by concatenating the new root and
    * the old relative name. add one path element at a time and
    * ensure that the directory exists, creating it if necessary.
    */
   strcpy(pnp, pdroot);
   strcpy( &pct->ct_szCopyFull[0], fullname );  // copy the FULL name
   strcpy( &pct->ct_szCopyRel[0],  relname  );    // and keep RELATIVE name

   SplitFN( NULL, &pct->ct_szSTitle[0], fullname );   // and get JUST the file
   strcpy( &pct->ct_szCopyTo[0], pnp );         // and the FULL destination name
   if( bIsDir )
   {
      /* add separating slash if not already there */
      // The CharPrev function retrieves a pointer to
      // the preceding character in a string. 
      // Parameters
      //    lpszStart [in] Pointer to the beginning of the string.
      //    lpszCurrent [in] Pointer to a character in a null-terminated string. 
      if( *CharPrev(pnp, (pnp + strlen(pnp)) ) != '\\' )
         strcat(pnp, "\\");
   
      pstart = relname;
      while( (pel = strchr(pstart, '\\')) != NULL )
      {
         /* found another element ending in slash. incr past the "\" */
         pel++;
            /* ignore the simple root name of ".\"! */
         if( strncmp(pstart, ".\\", 2) != 0 )
         {
            pdest = &pnp[strlen(pnp)];
            strncpy(pdest, pstart, pel - pstart);
            pdest[pel - pstart] = '\0';
   
            /* create subdir if necessary */
            if( !dir_isvaliddir(pnp) )
            {
//#ifdef   ADDUPDATE2
//               if( pcfds->dwCpyOpts & DELETE_ONLY )   // = INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
               if( state == STATE_FILERIGHTONLY )
               {
                  sprintf(EndBuf(pmsg),
                     "Delete FAILED as folder does NOT exist!"MEOR
                     "[%s] directory!"MEOR,
                     pnp );
                  goto Tidy_Exit;
               }
//#endif   // #ifdef   ADDUPDATE2

               //if( _mkdir(pnp) != 0 )
               //{
               // oops, can not create this new folder!
               //   sprintf(EndBuf(pmsg),
               //      "Copy FAILED as unable to create"MEOR
               //      "[%s] directory!"MEOR,
               //      pnp );
               //   goto Tidy_Exit;
               //}
               //else
               strcpy( &pct->ct_NewDir[0], pnp );  // keep the NEW directory
               {
                  sprintf(EndBuf(pmsg),
                     "WILL created [%s]"MEOR,
                     pnp );
               }
            }
         }
         pstart = pel;
      }
   
      /* there are no more slashes, so pstart points at the final element */
      strcat(pnp, pstart); // final destination qualified path if COPY, 
      // OR the file to DELETE if DELETE_ONLY case
   }

   // *** SPECIAL DELETE CASE ***
   // NOTE: Even though it is COPY from LEFT,
   // is this a RIGHT ONLY file, thus the DIRITEM is the RIGHT
   // thus they HAVE TO BE THE SAME **************************
//#ifdef   ADDUPDATE2
//   if( pcfds->dwCpyOpts & DELETE_ONLY )   // = INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
   if(( pcfds->dwCpyOpts & INCLUDE_RIGHTONLY ) &&  // *** DELETE CASE ***
      ( state == STATE_FILERIGHTONLY         ) )
   {
      if( strcmpi( fullname, pnp ) )
      {
         LPTSTR lps;
         lps = ((state == STATE_SAME) ? "Same" :
         (state == STATE_DIFFER) ? "Diff" :
         (state == STATE_FILELEFTONLY) ? "Left" :
         (state == STATE_FILERIGHTONLY) ? "Rite" : "Unk");
         sprintf( EndBuf(pmsg), "WHOA: Got "MEOR
            "[%s] and "MEOR
            "[%s] NOT SAME????"MEOR
            "state = %s",
            fullname,
            pnp,
            lps );
         chkme( pmsg );
         goto Tidy_Exit;
      }
      //if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH) )
      hFind = FindFirstFile( pnp, &pct->ct_sFDDst ); // DESTINATION find file information
      pct->ct_bFound = FALSE;
      if( VFH(hFind) )
      {
         pct->ct_bFound = TRUE;
         FindClose(hFind); // done with this
         memcpy( pfd, &pct->ct_sFDDst, sizeof(WIN32_FIND_DATA) );
//         if( ( pcfds->dwVerFlag & VERIFY_EACH ) ||
//            !( (pcfds->dwVerFlag & REVIEW_LIST) || (pcfds->dwVerFlag & MAKE_BACKUP) ) )
//         {
//            if( pcfds->dwVerFlag & VERIFY_EACH )
//               *pmsg = 0;
//            else
//            {
//               strcpy( pmsg, "NOTE: NO verify, review or backup flags"MEOR
//                  "then each DELETE will be verified"MEOR
//                  "unless [Copy All] chosen, when the backup FLAG"MEOR
//                  "will be added."MEOR );
//            }
//            pct->ct_dwFlag &= ~(TT_FLAGS);
//            pct->ct_dwFlag |= AddToolText( pmsg, pct->ct_ci );
//            AddOKCancel( pmsg );
//            i = MB2(NULL,pmsg,"*** VERIFY FILE DELETE ***", SD_MB_DEF);
//            if( i == IDC_COPYALL )
//            {
//               pcfds->dwVerFlag &= ~(VERIFY_EACH | VERIFY_OLDER);
//               pcfds->dwVerFlag |= MAKE_BACKUP;
//            }
//            else if( i == IDC_ABORTALL )
//            {
//                pct->ct_dwFlag |= (flg_Abort|flg_User);
//                goto Tidy_Exit;
//            }
//            else if( i == IDCANCEL )
//            {
//               pct->ct_dwFlag |= flg_User;   // flag as a USER skip
//               goto Tidy_Exit;
//            }
//         }
//         if( pcfds->dwVerFlag & MAKE_BACKUP )
//         {
//            bOK = dir_filedelete( pnp );
//         }
//         else
//         {
//            bOK = DeleteFile( pnp );
//         }
         bOK = TRUE;    // True FOR delete
      }
      else
      {
         sprintf( EndBuf(pmsg),
            "DELETE WILL fail due to destination"MEOR
            "[%s]"MEOR
            "DOES NOT EXIST!"MEOR,
            pnp );
      }
      goto Tidy_Exit;
   }

// #endif   // #ifdef   ADDUPDATE2

   // ok, it really is COPY (and NOT delete)
   // **************************************
   if( strcmpi( fullname, pnp ) == 0 )
   {
      sprintf( EndBuf(pmsg),
         "Copy WILL fail due to source"MEOR
         "[%s] and destination"MEOR
         "[%s] are equivalent!"MEOR,
         fullname,
         pnp );
      goto Tidy_Exit;
   }

   hFind = FindFirstFile( fullname, pfd );
   pct->ct_bFound = FALSE;
   if( VFH(hFind) )
   {
      pct->ct_bFound = TRUE;
      FindClose(hFind);
//   PFD         pfdc = &pct->ct_sFDCpy;    // just for any EXISTING file
      hFind = FindFirstFile( pnp, pfdc ); // DESTINATION find file information
      // warning: appears CopyFile() fails if destination is READ ONLY
      pct->ct_bDestFound = FALSE;
      if( VFH(hFind) )
      {
         pct->ct_bDestFound = TRUE;
         pct->ct_bDestRO = FALSE;
         if( pfdc->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
         {
            // this should be a WARNING flag on this item
            pct->ct_bDestRO = TRUE; // *** WATCH FOR THIS FLAG ***
         }
      }

//      if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER) )
//      {
         // if destination exist, move to 'Recyle Bin' (I HOPE)
//         hFind = FindFirstFile( pnp, &pct->ct_sFDD ); // DESTINATION find file information
//         if( pcfds->dwVerFlag & (VERIFY_EACH | VERIFY_OLDER) )
//         {
//            *pmsg = 0;
//            pct->ct_dwFlag &= ~(TT_FLAGS);
//            pct->ct_dwFlag |= AddToolText( pmsg, pct->ct_ci );
//            if( ( pcfds->dwVerFlag & VERIFY_EACH ) ||
//                ( pct->ct_dwFlag & TT_YOUNGER    ) )
//            {
//               AddOKCancel( pmsg );
//               i = MB2(NULL,pmsg,"VERIFY FILE COPY", SD_MB_DEF);
//               if( i == IDC_COPYALL )
//               {
//                  pcfds->dwVerFlag &= ~(VERIFY_EACH | VERIFY_OLDER);
//                  if( VFH(hFind) )
//                     FindClose(hFind);
//                  hFind = 0;
//               }
//               else if( i == IDC_ABORTALL )
//               {
//                  pct->ct_dwFlag |= (flg_Abort|flg_User);
//                  if( VFH(hFind) )
//                     FindClose(hFind);
//                  hFind = 0;
//                  goto Tidy_Exit;
//               }
//               else if( i == IDCANCEL )
//               {
//                  pct->ct_dwFlag |= flg_User;   // flag as a USER skip
//                  if( VFH(hFind) )
//                     FindClose(hFind);
//                  hFind = 0;
//                  goto Tidy_Exit;
//               }
//            }
//         }
//         if( VFH(hFind) )
//         {
//            FindClose(hFind);
//            if( pcfds->dwVerFlag & MAKE_BACKUP )
//            {
//               dir_filedelete( pnp );
//            }
//            hFind = 0;
//         }
//      }
        if(VFH(hFind))
           FindClose(hFind);
        hFind = 0;

//      bOK = CopyFile(fullname, pnp, FALSE); // COPY with OVERWRITE
//      if(bOK)
//      {
//         /* having successfuly copied the file, now copy the times, attributes */
//         hfile = CreateFile(pnp, GENERIC_WRITE, 0, NULL,
//                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//         if( VFH(hfile) )
//         {
//            SetFileTime(hfile, &pfd->ftCreationTime,
//                         &pfd->ftLastAccessTime,
//                         &pfd->ftLastWriteTime);
//
//            CloseHandle(hfile);
//   
//            SetFileAttributes(pnp, pfd->dwFileAttributes);
//   
//            nLocalCopies++;
//
            sprintf(EndBuf(pmsg),
               "Successful copy of"MEOR
               "[%s] completed."MEOR,
               pnp );
//         }
//         else
//         {
//            nLocalFails++;
//            bOK = FALSE;
//            sprintf(EndBuf(pmsg),
//               "Copy FAILED as UNABLE to OPEN"MEOR
//               "[%s] that should exist!"MEOR,
//               pnp );
//         }
//      }
//      else
//      {
//         sprintf(EndBuf(pmsg),
//            "CopyFile() FAILED! From"MEOR
//            "[%s] to "MEOR
//            "[%s]!"MEOR,
//            fullname,
//            pnp );
//      }
      bOK = TRUE; // appears OK
   }
   else
   {
      if( pct->ct_NewDir[0] == 0 )  // no NEW directory yet
      {
         sprintf( EndBuf(pmsg),
            "Copy FAILED because"MEOR
            "[%s]"MEOR
            "is NOT a VALID FILE!"MEOR,
            fullname );
      }
      else
      {
         bOK = TRUE; // have to ASSUME create of FOLDER will be OK
      }
   }

Tidy_Exit:

   // tidy up on exit
   dir_freerelname( item, relname );
   dir_freefullname(item, fullname);

   return(bOK);

} /* dir_copytest */


#ifdef ADD_ZIP_SUPPORT
VOID  dir_scan_zip(DIRECT dir, BOOL bRecurse) // #ifdef ADD_ZIP_SUPPORT
{
   static FDX _s_sfdx;
   DIRECT    child;
   BOOL      bMore;
   LPTSTR     name, pathname;
   HANDLE    hFind;
   PSTR      path = &g_szBuf2[0];
   PFDX     pfdx = &_s_sfdx;
   //PFD       pfd = &g_sFD;
   PFD       pfd = &pfdx->sFD;

   //if( dir->d_prelname )
   {
           //strcpy( path, LoadRcString(IDS_COMPARING) );   // load string into set of
           //strcat( path, dir->d_prelname );
         sprintf( path, " Scanning %s", dir->head->rootname );   // this is the ZIP name
           strcat( path, "  " );
           SetStatus(path);
   }

   strcpy( path, dir->head->rootname );   // this is the ZIP name

   pathname = &pfdx->szPath[0];
   /* omit the . at the beginning of the relname, and the
    * .\ if there is a trailing \ on the rootname
    */
   if( strcmp( dir->d_prelname, ".\\" ) )
   {
      // NOT the ROOT, so ADD the relative folder name AFTER the ZIP
      // For example, if 'comparing' say "E:\FG\FG-06.ZIP"
      // then this will produce
      // "E:\FG\FG-06.ZIP\FlightGear\ to pass to FindFirstZip
      // (a) it is thus highly unlikely to pass the dir_isvalidfile()
      // test, and (b) most certainly will NOT pass the IsValidZip()
      // test, so we will KNOW it is a folder path within the ZIP
      if( *CharPrev( path, (path + strlen(path)) ) == '\\' )
         strcat(path, &dir->d_prelname[2]);
      else
         strcat(path, &dir->d_prelname[1]);
      //strcat(path, "*.*");
   }
   /* read all entries in the directory */

   // we have a PATH to come from the ZIP
   //hFind = FindFirstFile(path, &finddata);
   //hFind = FindFirstZip( path, pfd );
   hFind = FindFirstZip( path, pfdx ); // #ifdef ADD_ZIP_SUPPORT
        //bMore = ( hFind && ( hFind != INVALID_HANDLE_VALUE ) );   // (HANDLE) -1);
        //if( hFind && ( hFind != INVALID_HANDLE_VALUE ) )   // (HANDLE) -1);
        if( VFH(hFind) )   // (HANDLE) -1);
           bMore = TRUE;
        else
           bMore = FALSE;

        dir->dwDirsTot  = 0;
        dir->dwDirsExcl = 0;
        dir->dwFilsTot  = 0;
        dir->dwFilsExcl = 0;
        //LocalUnlock(LocalHandle ( (PSTR) path));
        //LocalFree(LocalHandle ( (PSTR) path));
        name = (LPSTR) &pfd->cFileName[0];
        while( bMore )
        {

                //bIsDir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                //name = (LPSTR) &finddata.cFileName;
                //filesize = finddata.nFileSizeLow;
                //bIsDir = (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                //filesize = pfd->nFileSizeLow;
                if( pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                {
                   if ( ( strcmp(name, "." ) != 0    ) &&
                        ( strcmp(name, "..") != 0    ) )
                   {
                      dir->dwDirsTot++;
                      if( NotInXDL( dir, name, pfd ) )
                      {
                         dir_adddirect(dir, name, pfd, TRUE);
                      }
                      else
                      {
                         dir->dwDirsExcl++;
                      }
                   }
                   // ignore "." and ".." folders/directories
                }
                else    // if( !bIsDir )
                {
                   dir->dwFilsTot++;
                        //dir_addfile(dir, name, filesize);
                        dir_addfile2(dir, name, pfd);
                }

                //bMore = FindNextFile(hFind, &finddata);
                //bMore = FindNextZip( hFind, pfd );
                bMore = FindNextZip( hFind, pfdx ); // #ifdef ADD_ZIP_SUPPORT
        }

        //if( hFind && ( hFind != INVALID_HANDLE_VALUE ) )   // (HANDLE) -1);
        if( VFH(hFind) )   // (HANDLE) -1);
        {
           FindCloseZip(hFind);
        }

        dir->d_bScanned = TRUE;
        dir->d_bIsZip   = TRUE;

        dir->pos = DL_FILES;

        if( bRecurse )
        {
           List_TRAVERSE(dir->directs, child)
           {
              dir_scan_zip(child, TRUE); // #ifdef ADD_ZIP_SUPPORT
           }
        }

} /* dir_scan_zip #ifdef ADD_ZIP_SUPPORT */
#endif // #ifdef ADD_ZIP_SUPPORT


BOOL  dir_iszip( DIRITEM di )
{
   if( !di )
      return FALSE;

   return( di->di_bIsZip );
}

#ifdef ADD_ZIP_SUPPORT
INT   dir_openzip( PVOID pv ) // #ifdef ADD_ZIP_SUPPORT
{
   POZF     pozf = (POZF)pv; // #ifdef ADD_ZIP_SUPPORT
   LPTSTR   zname;
   LPTSTR   fname, p;
   DIRITEM  di = pozf->pDI;
   int      fh = 0;
   //int      i;

   zname = dir_getfullname(di);
   fname = dir_getrelname(di);

   strcpy( pozf->pZName, zname );
   p = fname;
   if( *p == '.' )
      p++;
   if( *p == '\\' )
      p++;
   strcpy( pozf->pFName, p );

   //     fh = OpenFile(fname, &os, OF_READ|OF_SHARE_DENY_NONE);
   fh = OpenZIPFile( pozf );   // return a BUFFER of file data // #ifdef ADD_ZIP_SUPPORT

   dir_freerelname( di, fname);
   dir_freefullname(di, zname);

   return fh;

} /* dir_openzip */
#endif // #ifdef ADD_ZIP_SUPPORT

BOOL  dir_iszipfile( LPTSTR lpf )
{
   BOOL  bRet = FALSE;
#ifdef ADD_ZIP_SUPPORT
   if(( dir_isvalidfile(lpf) ) &&
      ( IsValidZip(lpf)      ) ) // #ifdef ADD_ZIP_SUPPORT
   {
      bRet = TRUE;
   }
#endif // #ifdef ADD_ZIP_SUPPORT
   return bRet;
}

BOOL  dir_setzipflag( PDWORD pdw, LPTSTR lplf, LPTSTR lprf )
{
   BOOL  bRet = FALSE;
   if(pdw)
   {
      DWORD dwo = *pdw;
      DWORD dwc = dwo;

      if( lplf && *lplf && dir_iszipfile( lplf ) )
         dwo |= tf_LeftisZip;
      else
         dwo &= ~(tf_LeftisZip);

      if( lprf && *lprf && dir_iszipfile( lprf ) )
         dwo |= tf_RightisZip;
      else
         dwo &= ~(tf_RightisZip);

      *pdw = dwo;  // return updated flag

      if( dwo != dwc )
         bRet = TRUE;

   }
   return bRet;
}

DWORD dir_filecount( DIRLIST dl )
{
   DIRECT   d;
   DIRITEM  di;
   DWORD    dwc = 0;
   LIST     lst;
   LPTSTR   lpn, lpr;
#ifdef   ADDOUTLST
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb1 = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
#endif   // #ifdef   ADDOUTLST

   if( !dl )
      return 0;

   di = dir_firstitem(dl);
#ifdef   ADDOUTLST
   *lpb2 = 0;
   GetModulePath(lpb2);
   strcat( lpb2, "TEMPLI01.TXT" );
   GetNxtDif( lpb2 );

   if(di)
   {
      sprintf(lpb, "Listing %#x DIRLIST."MEOR, dl );
   }
   else
   {
      sprintf(lpb, "Appears NO DIRITEM for %#x LIST!"MEOR, dl );
   }
   Out2File( lpb2, lpb );
#endif   // #ifdef   ADDOUTLST
   while(di)
   {
      lpn = di->di_pname;
      d = di->di_pdirect;
      if(d)
      {
         LARGE_INTEGER  li;
         //SYSTEMTIME     st;
         lpr = d->d_prelname;
         lst = d->diritems;
         //if(lst)
         //   dwc += List_Count( lst, 0, 0 );
         dwc++;
         li.LowPart  = di->di_sfd.nFileSizeLow;
         li.HighPart = di->di_sfd.nFileSizeHigh;
#ifdef   ADDOUTLST
         //FT2LST( &di->di_sfd.ftLastWriteTime, &st );
         sprintf(lpb,
            "%4d %s %s %s%s"MEOR,
            dwc,
            GetI64StgRLen( &li, 14 ),
            GetFDTStg( &di->di_sfd.ftLastWriteTime ),
            lpr,
            lpn );
         Out2File( lpb2, lpb );
#endif   // #ifdef   ADDOUTLST
      }
      di = dir_nextitem(dl, di, TRUE);
   }

#ifdef   ADDOUTLST
   sprintf(lpb, "Listed %d files/folders."MEOR, dwc );
   Out2File( lpb2, lpb );
   Out2File( lpb2, 0 );   // and CLOSE this file
#endif   // #ifdef   ADDOUTLST
   return dwc;
}

DWORD dir_directorycount( DIRLIST dl )
{
   DIRECT d;

   if( !dl )
      return 0;

   d = dl->dl_pdot;

   if( !d )
      return 0;

   return( List_Count( d->directs, 0, 0 ) );

}

// DWORD	GetDriveCount( DWORD dlist )
DWORD	dir_countdrives( DWORD dlist )
{
	int		i = 32;
	DWORD	mask  = 1;
	DWORD	dwCnt = 0;

	while( i-- )
	{
		if( dlist & mask )
		{
			dwCnt++;
		}
		mask = mask << 1;
	}
	return dwCnt;
}

//PDRLIST GetDriveList( HWND hwnd, LPVOID lpv, PDRLIST lpPrev,
//					 BOOL bRetTxt )
BOOL  dir_isvaliddrive_TBD( LPTSTR pdrive )
{
	BOOL		flg = FALSE;
	int		curdrive;
	DWORD		dlist, dstg;
	LPTSTR		lpd = gszTmpBuf2;
	UINT	OldMode;
	DWORD	dcount;

	dlist  = GetLogicalDrives();
	//dcount = GetDriveCount( dlist );
	dcount = dir_countdrives( dlist );

	if( dcount == 0 )
      return FALSE;

//    GetDSFunction();

	dstg = GetLogicalDriveStrings( 1024, // size of buffer MXDBUF
							 lpd ); // address of buffer for drive strings 

	OldMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	/* ========================================== */
	/* return to here when done                   */
	curdrive = _getdrive();
    /* ========================================== */
    /* process logical drive letters in string    */
    /* ========================================== */

   // *** TBD ***


	/* ========================================== */
	/* Switch BACK to original DRIVE ============ */
	_chdrive( curdrive );
	/* ========================================== */
	OldMode = SetErrorMode( OldMode );

//	DBWaitOFF;

	/* actually just return a global pointer to an instance */
	return flg;

}

//BOOL dir_copy( PVOID pv1, PVOID pv2 )
static FD   _s_fd1, _s_fd2;
BOOL dir_copy2( LPTSTR psrcname, LPTSTR relname, LPTSTR pdroot,
               DWORD dwVerFlag )
{
   BOOL        bOK = FALSE;
   //PCFDLGSTR   pcfds = (PCFDLGSTR)pv2;
   //PCPYTST     pct   = (PCPYTST)pv1;
   LPTSTR      pmsg  = &g_szMsg[0];
//   LPSTR       relname, fullname;
   LPTSTR       pstart, pdest, pel;
   HANDLE      hFind;
   LPTSTR      pnp = &g_szNewPath[0];
   INT         i;
//   INT         state;
//   DIRITEM     item;
//   LPTSTR      pdroot;
   BOOL        bIsDir;
   PFD         pfdc = &_s_fd1;   // pct->ct_sFDCpy;    // just for any EXISTING file
   PFD         pfds = &_s_fd2;   // pct->ct_sFDSrc;    // this is the SOURCE information

   *pmsg = 0;
   //if( !VALIDPCFDS( pcfds ) )
   //{
   //   strcpy( pmsg, "Copy FAILED because PCFDLGSTR is INVALID!"MEOR );
   //   return FALSE;
   //}
   //item = pct->ct_diritem;
   //if( !item )
   //{
   //   strcpy( pmsg, "Copy FAILED because DIRITEM is NULL!"MEOR );
   //   return FALSE;
   //}

   //relname = 0;
   //fullname = 0;

   /* check that the newroot directory itself exists */
   //pdroot = &pcfds->cf_szDest[0];   // ROOT OF DESTINATION MUST EXIST
   bIsDir = dir_isvaliddir(pdroot);
   if( !bIsDir )
   {
      //if( pcfds->cf_bExpanded )  // pcfds->cf_iSelected = iSel;
      //{
      //   if( !dir_isvalidfile(pdroot) )
      //   {
      //      sprintf( pmsg, "Action FAILED because"MEOR
      //         "dest root=[%s]"MEOR
      //         "is NOT A VALID file or folder!"MEOR,
      //         pdroot );
      //      return FALSE;
      //   }
      //}
      //else
      {
            sprintf( pmsg, "Action FAILED because"MEOR
               "dest root=[%s]"MEOR
               "is NOT A VALID folder!"MEOR,
               pdroot );
            return FALSE;
      }
   }

   /* name of file relative to the tree root */
   //relname  = dir_getrelname(item);
   //fullname = dir_getfullname(item);

   //state = pct->ct_iState; // extract the STATE

   /*
    * build the new pathname by concatenating the new root and
    * the old relative name. add one path element at a time and
    * ensure that the directory exists, creating it if necessary.
    */
   strcpy(pnp, pdroot);
   //strcpy( &pct->ct_szCopyFull[0], fullname );  // copy the FULL name
   //strcpy( &pct->ct_szCopyTo[0], pnp );         // and the FULL destination name

   if( bIsDir )
   {
      /* add separating slash if not already there */
      // The CharPrev function retrieves a pointer to
      // the preceding character in a string. 
      // Parameters
      //    lpszStart [in] Pointer to the beginning of the string.
      //    lpszCurrent [in] Pointer to a character in a null-terminated string. 
      if( *CharPrev(pnp, (pnp + strlen(pnp)) ) != '\\' )
         strcat(pnp, "\\");
   
      pstart = relname;
      while( (pel = strchr(pstart, '\\')) != NULL )
      {
         /* found another element ending in slash. incr past the "\" */
         pel++;

         /* if any to ignore, like say the simple root name of ".\"! */
         //if( strncmp(pstart, ".\\", 2) != 0 ) {
         pdest = &pnp[strlen(pnp)];
         strncpy(pdest, pstart, pel - pstart);
         pdest[pel - pstart] = '\0';

         /* create subdir if necessary */
         if( !dir_isvaliddir(pnp) )
         {
//               if( pcfds->dwCpyOpts & DELETE_ONLY )   // = INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
//               if( pcfds->dwCpyOpts & INCLUDE_RIGHTONLY )   // *** DELETE CASE ***
//               {
//                  sprintf(EndBuf(pmsg),
//                     "Delete FAILED as folder does NOT exist!"MEOR
//                     "[%s] directory!"MEOR,
//                     pnp );
//                  goto Tidy_Exit;
//               }

            if( _mkdir(pnp) != 0 ) { // with FIX20090128
               // oops, can not create this new folder!
               // FIX20090128
               // THIS COULD BE BECAUSE THERE IS ALREADY A FILE OF THIS NAME
               PTSTR ptmp = GetNxtBuf();
               strcpy(ptmp,pnp);
               if( *CharPrev(ptmp, (ptmp + strlen(ptmp)) ) == '\\' )
                  ptmp[ strlen(ptmp) - 1 ] = 0;
               if( dir_isvalidfile(ptmp) ) {
                  if( dir_filedelete( ptmp ) ) {
                     if( _mkdir(pnp) == 0 ) {   // after FIX20090128
                        goto Done_Create; // FIX20090128 - done it
                     }
                  }
               }
               // oops, can not create this new folder!
               sprintf(EndBuf(pmsg),
                  "Copy FAILED as unable to create"MEOR
                  "[%s] directory!"MEOR,
                  pnp );
               goto Tidy_Exit;
            }
            else
            {
Done_Create:
               sprintf(EndBuf(pmsg),
                  "Created [%s]"MEOR,
                  pnp );
            }
         }

         pstart = pel;

      }
   
      /* there are no more slashes, so pstart points at the final element */
      strcat(pnp, pstart); // final destination qualified path if COPY, 
      // OR the file to DELETE if DELETE_ONLY case
   }

   // ok, it really is COPY (and NOT delete)
   // **************************************
   if( strcmpi( psrcname, pnp ) == 0 )
   {
      sprintf( EndBuf(pmsg),
         "Copy WILL fail due to source"MEOR
         "[%s] and destination"MEOR
         "[%s] are equivalent!"MEOR,
         psrcname,
         pnp );
      goto Tidy_Exit;
   }

   hFind = FindFirstFile( psrcname, pfds );  // get SOURCE information
   if( VFH(hFind) )  // this was ALREADY done in dir_copytest() but,
   {                 // it COULD have been REMOVED meantime
      FindClose(hFind); // finished with the SOURCE find.

      // now the destination
      hFind = FindFirstFile( pnp, pfdc ); // COPY find file information
      //if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH | VERIFY_OLDER) )
      if( dwVerFlag & MAKE_BACKUP )
      {
         // if destination exist, move to 'Recyle Bin' (I HOPE)
         if( VFH(hFind) )
         {
            FindClose(hFind);
            hFind = 0;
            if( pfdc->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
            {
               // must clear attribute first
               SetFileAttributes(pnp, FILE_ATTRIBUTE_ARCHIVE);
            }
            //if( pcfds->dwVerFlag & MAKE_BACKUP )
            if( dwVerFlag & MAKE_BACKUP )
            {
               // copy to backup delete - garbage can - trash can
               dir_filedelete( pnp );
            }
            else
            {
               // delete what is there
               bOK = DeleteFile( pnp );
            }
         }
      }

      // else no backup to worry about
      if( VFH(hFind) )
      {
         FindClose(hFind);
         hFind = 0;
         if( pfdc->dwFileAttributes & FILE_ATTRIBUTE_READONLY )
         {
            // must clear attribute first
            SetFileAttributes(pnp, FILE_ATTRIBUTE_ARCHIVE);
         }
      }

      bOK = CopyFile(psrcname, pnp, FALSE); // COPY with OVERWRITE

      if(bOK)
      {
#ifndef  USEOLDSETATS2
         nLocalCopies++;   // set a successful COPY done

#else // #ifdef  USEOLDSETATS2

         HANDLE   hFile;

         /* having successfuly copied the file, now copy the times, attributes */
         // NOTE: This appears ENTIRELY redundant, since it appears
         // CopyFile() above also set the date/time and attributes
         // ******************************************************
         hfile = CreateFile(pnp, GENERIC_WRITE, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
         // BUT IT IS A GOOD SECOND TEST THAT IT ALL WORKED
         if( VFH(hfile) )
         {
            SetFileTime(hfile, &pfds->ftCreationTime,
                         &pfds->ftLastAccessTime,
                         &pfds->ftLastWriteTime);

            CloseHandle(hfile);
   
            SetFileAttributes(pnp, pfds->dwFileAttributes);
   
            nLocalCopies++;   // set a successful COPY done

            sprintf(EndBuf(pmsg),
               "Successful copy of"MEOR
               "[%s] completed."MEOR,
               pnp );
         }
         else
         {
            nLocalFails++;
            bOK = FALSE;
            sprintf(EndBuf(pmsg),
               "Copy FAILED as UNABLE to OPEN"MEOR
               "[%s] that should exist!"MEOR,
               pnp );
         }

#endif   // #ifdef  USEOLDSETATS2

      }
      else
      {
         sprintf(EndBuf(pmsg),
            "CopyFile() FAILED! From"MEOR
            "[%s] to "MEOR
            "[%s]!"MEOR,
            psrcname,
            pnp );
      }
   }
   else
   {
      // FAILED to FIND the SOURCE of the COPY
      sprintf( EndBuf(pmsg),
         "Copy FAILED because"MEOR
         "[%s]"MEOR
         "is NOT a VALID FILE!"MEOR,
         psrcname );
   }

Tidy_Exit:

   // tidy up on exit
   //dir_freerelname( item, relname );
   //dir_freefullname(item, fullname);

   i = 0;

   return bOK;

} // end - dir_copy2(...)

LPSTR dir_getpathptr(DIRITEM cur)
{
   if( !cur )
      return NULL;
   return( cur->di_pdirect->d_prelname );
}


// eof - scandir.c
