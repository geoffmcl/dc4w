
/****************************** Module Header *******************************
* Module Name: COMPLIST.C
*
* Supports a list of compitems, where each compitem represents
* a pair of matching files, or an unmatched file.
*
* Functions:
*
* complist_filedialog()
* complist_dirdialog()
* complist_args()
* complist_getitems()
* complist_delete()
* complist_savelist()
* complist_copyfiles()
* complist_match()
* complist_new()
* NEW
* complist_isfiles
* In dc4wDlg.c - Do_DIR_DLG()
*                Do_SAVELIST_DLG(PVOID)
*
* Comments:
*
* We build lists of filenames from two pathnames (using the
* scandir module) and then traverse the two lists comparing names.
* Where the names match, we create a CompItem from the matching
* names. Where there is an unmatched name, we create a compitem for it.
*
* We may also be asked to create a complist for two individual files:
* here we create a single compitem for them as a matched pair even if
* the names don't match.
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"
#include <commctrl.h>
//#include  "complist.h"
//#define  DBGCMPITEMS    // add an OUTPUT for each COMPARE CMP=<stuff>
#undef  DBGCMPITEMS    // remove an OUTPUT for each COMPARE CMP=<stuff>

extern   BOOL  bAbort;         /* defined in dc4w.c  Read only here */
extern   INT   giSelection;    /* selected row in table*/
extern   COMPITEM view_getcompitem(PVIEW view);
extern   VOID  compitem_retfullname( LPTSTR lpb, COMPITEM ci, DWORD dwo );
extern   INT   compitem_addfullright( LPTSTR lpb, COMPITEM ci );
extern   INT   compitem_addfullleft( LPTSTR lpb, COMPITEM ci );
extern   HFONT g_hFixedFont8, g_hfCN8, g_hFF8bold;    // LOGFONT creation
extern   void  gtab_setcursor( HCURSOR hCur );
extern   DWORD dir_filecount( DIRLIST dl );
extern   VOID  SetCopyOptions( PDWORD pdw );  // gdwCpyOpts &| outline_include
//extern   DWORD       g_dwmissedcnt;
extern   DWORD       g_dwviewedcnt;
extern   DWORD getmisscnt( VOID );
extern   VOID  resetmisscnt( VOID );
extern   VOID  CheckCommonSet( HWND hDlg, DWORD dwi );
extern   BOOL  dir_filedelete( LPTSTR lpf );
extern   VOID  SetBPS( LPTSTR pb, double dsz, double etm );
extern   BOOL  IsInView( DWORD dwo, UINT state, DWORD dwFlag );

/*
 * The COMPLIST handle is typedef-ed to be a pointer to one
 * of these struct complist
 *
 * It uses dirlist struct dirlist {
 *  char     rootname[264]; == name of root of tree ==
 *  BOOL     dl_bFile;      == TRUE if root of tree is file, not dir ==
 *  BOOL     dl_bIsZip;  // is NOT really a FILE - it is a ZIP of FILES
 *  INT      nCopies;    // copies made
 *  DIRECT   dl_pdot;    == dir  for '.' - for tree root dir ==
 *  };
 * and for each directory / folder of the file system is kept as a tree using
 * typedef struct direct {
        LPTSTR    d_prelname;     == name of dir relative to DIRLIST root ==
        DIRLIST   head;           == back ptr (to get fullname) ==
        struct direct * parent; == parent directory (NULL if above tree root)==
        BOOL      d_bScanned;   == TRUE if scanned ==
        BOOL      d_bIsZip;     // TRUE if a ZIP file
        FD        d_sfd;         // keep the information gathered
        LIST diritems;          == list of DIRITEMs for files in cur. dir ==
        LIST directs;           == list of DIRECTs for child dirs ==
        int pos;                == where are we begin, files, dirs ==
        struct direct * curdir; == subdir being scanned (ptr to list element)==
   } FAR * DIRECT;
   == Values for direct.pos ==
   #define DL_FILES        1       == reading files from the diritems ==
   #define DL_DIRS         2       == in the dirs: List_Next on curdir ==
 *
 *
 */
struct complist {
   DIRLIST  left;    // left list of files
   DIRLIST  right;   // right list of files
   LIST     items;   // list of COMPITEMs
   DWORD    cl_id;   // current ID
};

/* ---- module-wide data -------------------------------------*/

LSTSTATS g_sLstStats;

/* data for communicating between the SaveList dlg and complist_savelist() */
//#define  MXFNB       264

TCHAR szFilCnt[] = "(%d files)";

INT      complist_count;   // a displayable ROW counter

//BOOL  bVerify = TRUE;
//BOOL  bChgVer = FALSE;
//BOOL  bDlgSingle = FALSE;  // single (selected or expanded) file ONLY

// NOT YET USED - Idea to allow the user to immediately modify
// the errant PATH/FILE entered, but needs lots more work!
//TCHAR gszNewSel[MXFNB];
//TCHAR gszNLeft[MXFNB];
//TCHAR gszNRite[MXFNB];

TCHAR dlg_head[MXFNB];                /* BEFORE filename */
TCHAR dlg_tail[MXFNB];                /* AFTER filename */

/* checkbox options */
//BOOL dlg_identical, dlg_differ, dlg_left, dlg_right;
BOOL     dlg_recursive = FALSE;
DWORD    dlg_flg = 0;

/* data for Directory and SaveList */
//TCHAR    dialog_leftname[MXFNB];
//TCHAR    dialog_rightname[MXFNB];
TCHAR    dlg_leftname[MXFNB];
TCHAR    dlg_rightname[MXFNB];
/*
 * data used by dodlg_copyfiles
 */

//TCHAR dlg_root[MXFNB];  // temporary buffer for COPY TO destination
//TCHAR gszCopyTo[264] = {"\0"};
// BOOL  bChgCT = FALSE;

TCHAR gszFName[MXFNB];
TCHAR gszFileExt[MXFNB];
TCHAR gszFileOpenSpec[MXFNB];
OFSTRUCT g_os1, g_os2;
OPENFILENAME g_sofn;
TCHAR gachFilters[MXFNB];
TCHAR gachPath[MXFNB];

HFONT ghFixedFont = 0;

BOOL  dlg_order = TRUE;    // put out the LIST separately

/*------------------------timing for performance measurements-----------------*/
static DWORD TickCount;         /* time operation started, then time taken*/

BOOL complist_match(COMPLIST cl, PVIEW view, BOOL fDeep, BOOL fExact);
COMPLIST complist_new(void);

// NEW
VOID  complist_getstats( PVOID pcfds, COMPLIST cl );
VOID  complist_setdlgstgs( PVOID pcfds, COMPLIST cl );
//INT   complist_showcopydlg( PLE pHead, INT icnt );
INT   complist_showcopydlg( PVOID pcfds, INT icnt, PINT picnt );
VOID  complist_cleanup( VOID );
VOID  complist_setpcfds( PVOID pcfds, INT state, DWORD dwFlag );
VOID  AppendCopyOpts( LPTSTR lpb, DWORD dwo );
VOID  complist_setstats( COMPLIST cl );
VOID  AppendCopyOptLetters( LPTSTR lpb, DWORD dwo );
VOID  AppendCopyOpts( LPTSTR lpb, DWORD dwo );

//extern   LRESULT compitem_addLV( COMPITEM ci );
//#define  ADDLISTVIEW(a) compitem_addLV(a)
LONG  complist_getrow( COMPLIST cl, COMPITEM citem )
{
   COMPITEM    ci;
   LONG     index = 0;
   DWORD    dwo   = outline_include;

   if(!cl || !citem )
      return -1;

   List_TRAVERSE(cl->items, ci)
   {
      if( ci == citem )
         return index;  // return the ROW NUMBER of this COMPITEM

      // is this a 'displayable' item = new row on the screen, when displayed
      if( IsInView( dwo, compitem_getstate(ci), compitem_getflag(ci) ) )
         index++;
   }

   return -1;
}


VOID  complist_setstats( COMPLIST cl ) // set/reset g_sLstStats
{
   COMPITEM    ci;
   INT         state;
   DWORD       dwFlag, dwo;
   PLSTSTATS   pls = &g_sLstStats;


//   if( pls->dwid == cl->cl_id )
//      return;
//   BEGINLVUPDATE; // freeze updates
//   CLEARLISTVIEW; // restart the LISTVIEW, if any

   ZeroMemory(pls, sizeof(LSTSTATS));
   pls->dwid = cl->cl_id;  // set the ID
   dwo = outline_include;
   List_TRAVERSE(cl->items, ci)
   {
      pls->dwtot++;  // bump the total
      // complist_setpcfds( pcfds, compitem_getstate(ci), compitem_getflag(ci) );
      state = compitem_getstate(ci);
      dwFlag = compitem_getflag(ci);
      if( IsInView( dwo, state, dwFlag ) )
      {
         pls->dwinview++;  // count of those DISPLAYABLE
//         ADDLISTVIEW(ci);
      }
      switch(state)
      {
      case STATE_SAME:
//   case STATE_COMPARABLE:
//   case STATE_SIMILAR:
         //strcat(lpb, "Same");
         pls->dwsame++;

         if( dwFlag & TT_DATEDIFF )
            pls->dwsamex++;

         break;

      case STATE_DIFFER:
         //strcat(lpb, "Differ");
         if( dwFlag & TT_OLDER )
            pls->dwolder++;
         else
            pls->dwnewer++;
         break;

      case STATE_FILELEFTONLY:
//   case STATE_LEFTONLY:
//      strcat(lpb, "Left only");
         //strcat(lpb, "Copy new");
         pls->dwleft++;
         break;

      case STATE_FILERIGHTONLY:
//   case STATE_RIGHTONLY:
//      strcat(lpb, "Right only");
         //strcat(lpb, "*Delete*");
         pls->dwright++;
         break;

//   case STATE_MOVEDLEFT:    /* this is the left file version */
//      strcat(lpb, "Left");
//      break;

//   case STATE_MOVEDRIGHT:  /* this is the right file version*/
//      strcat(lpb, "Right");
//      break;
      default:
         //sprintf(EndBuf(lpb), "?UNK?(%d)", state );
         pls->dwunk++;
         break;

      }
    
   } // traverse list

   pls->dwleftcnt  = dir_filecount( cl->left  );
   pls->dwrightcnt = dir_filecount( cl->right );

   sprtf( "LS: %d(%d,%d), s=%d(%d) n=%d o=%d l=%d r=%d v=%d."MEOR,
      g_ListTotal,
      g_LeftCnt,
      g_RightCnt,
      g_SameCnt,
      g_SameExa,
      g_NewerCnt,
      g_OlderCnt,
      g_LeftOnly,
      g_RightOnly,
      g_InView );

//   ENDLVUPDATE; // un-freeze and update

}

TCHAR g_szListStg[264];

VOID  complist_showstats( VOID )
{
   DWORD    dwm = getmisscnt();
   PLSTSTATS   pls = &g_sLstStats;
//   sprintf( g_szListStg, "T=%d (L=%d,R=%d) Sm=%d New=%d Old=%d LO=%d RO=%d",
//      pls->dwtot, // the total
   sprintf( g_szListStg, "%d:%d = s%d n%d o%d Lo%d Ro%d",
//      pls->dwtot, // the total
      pls->dwleftcnt, pls->dwrightcnt,
      pls->dwsame,   // same
      pls->dwnewer,  // newer
      pls->dwolder,  // older
      pls->dwleft,   // only in left = ready to copy
      pls->dwright );   // only in right = probably all DELETE - What about ZIP backup
   if( dwm )
      sprintf(EndBuf(g_szListStg), " *eXc=%d*", dwm );

   if( g_SameExa )
   {
      sprintf(EndBuf(g_szListStg), " *Exa=%d*", g_SameExa );
   }

#ifdef   ADDSTATS2   // activate another item on the status bar
   SetListStats( g_szListStg );
#endif   // #ifdef   ADDSTATS2   // activate another item on the status bar

   //if( pls->dwid == cl->cl_id )
   sprtf( "Total %d (L=%d,R=%d): Same %d Newer %d Left-New %d Right-Orph %d"MEOR,
      pls->dwtot, // the total
      pls->dwleftcnt, pls->dwrightcnt,
      pls->dwsame,   // same
      pls->dwnewer,  // newer
      pls->dwleft,   // only in left = ready to copy
      pls->dwright );   // only in right = probably all DELETE - What about ZIP backup

   sprtf("%s"MEOR, g_szListStg);
//      pls->dwolder,  // older
//      pls->dwleft,
//      pls->dwright );
//         pls->dwunk;
}


/***************************************************************************
 * Function: complist_filedialog
 *
 * Purpose:
 *
 * Builds a complist by putting up two dialogs to allow the user to
 * select two files. This will build a Complist with one CompItem (even
 * if the names don't match).
 *
 ***************************************************************************/
COMPLIST
complist_filedialog(PVIEW view)
{
   COMPLIST cl;

   /* ask for the filenames */
   strcpy(gszFileExt, ".c");
   strcpy(gszFileOpenSpec, "*.*");
   strcpy(gszFName,"");

   if( !complist_open(LoadRcString(IDS_SELECT_FIRST_FILE),
            gszFileExt, gszFileOpenSpec, &g_os1, gszFName) )
   {
      return(NULL);
   }

   strcpy(gszFileExt, ".c");
   strcpy(gszFileOpenSpec, "*.*");
   strcpy(gszFName,"");

   if( !complist_open(LoadRcString(IDS_SELECT_SECOND_FILE),
            gszFileExt, gszFileOpenSpec, &g_os2, gszFName) )
   {
      return(NULL);
   }

   /* alloc a new structure */
   cl = complist_new(); // where cl->items = List_Create(); for COMPITEMS
   cl->cl_id = GetTickCount();   // set a unique ID
   g_dwScDirs = 0;
   g_dwScFiles = 0;
   g_dwLeftDirs = 0;
   g_dwLeftFiles = 0;
   g_dwRightDirs = 0;
   g_dwRightFiles = 0;

   sprtf( MEOR"filedialog: Building NEW left/right lists ..."MEOR );
   // ===========================================================
   cl->left  = dir_buildlist(g_os1.szPathName, TRUE, tf_IsLeft );
   cl->right = dir_buildlist(g_os2.szPathName, TRUE, tf_IsRight);
   // ===========================================================
   if( !cl->left || !cl->right )
   {
      complist_delete(cl);
      return(NULL);
   }

   /* register with the view (must be done after the list is non-null) */
   view_setcomplist(view, cl, FALSE);

   // in a FILE to FILE compare, NATURALLY fDeep (recursive) is FALSE
   complist_match(cl, view, FALSE, TRUE);
   // ===============================================================

   complist_setstats(cl);  // gather statistics

   return(cl);

}/* complist_filedialog */


COMPLIST complist_newlist2( LPTSTR pleft, LPTSTR pright, PVIEW view, DWORD dwFlg )
{
   COMPLIST cl;
   /* alloc a new structure */
   cl = complist_new(); // where cl->items = List_Create(); for COMPITEMS
   cl->cl_id = GetTickCount();   // set a unique ID
   sprtf( MEOR"newlist2: Building NEW left/right lists ..."MEOR );
   g_dwScDirs = 0;
   g_dwScFiles = 0;
   g_dwLeftDirs = 0;
   g_dwLeftFiles = 0;
   g_dwRightDirs = 0;
   g_dwRightFiles = 0;

   if( dwFlg & tf_LeftisZip )
      cl->left = dir_buildlist(pleft,   TRUE, Left_Zip);
   else
      cl->left  = dir_buildlist(pleft,  TRUE, tf_IsLeft);

   if( dwFlg & tf_RightisZip )
      cl->right = dir_buildlist(pright, TRUE, Right_Zip);
   else
      cl->right = dir_buildlist(pright, TRUE, tf_IsRight);

   if( !cl->left || !cl->right )
   {
      complist_delete(cl);
      return(NULL);
   }

   /* register with the view (must be done after the list is non-null) */
   view_setcomplist(view, cl, FALSE);

   // in a FILE to FILE compare, NATURALLY fDeep (recursive) is FALSE
   complist_match(cl, view, FALSE, TRUE);
   // ===============================================================

   complist_setstats(cl);  // gather statistics

   return(cl);

}

//       dwi |= IS_FILE_ITEM;
//      if( IsValidZip(pDir) ) // #ifdef ADD_ZIP_SUPPORT
//         dwi |= IS_ZIP_FILE;
//   else if( dir_isvaliddir(pDir) )
//      dwi |= IS_DIR_ITEM;
extern   BOOL  IsFileComp( VOID );

COMPLIST
complist_newlist_NOT_USED( LPTSTR pleft, LPTSTR pright, PVIEW view, BOOL bDeep, DWORD dwFlg )
{
   COMPLIST  cl;
   PCLASTR   pcla = &g_sCLA;

   //if( GOOD_COMP1( g_dwActComp ) )
   if( IsFileComp() )
   {
      // if a PAIR of files
      return( complist_newlist2( pleft, pright, view, dwFlg ) );
   }

   ZeroMemory(pcla, sizeof(CLASTR));
   pcla->pCaller= "complist_newlist - g_sCLA";
   pcla->pLeft  = pleft;
   pcla->pRight = pright;
   pcla->pView  = view;
#ifndef USE_GLOBAL_RECURSIVE
   pcla->bDeep  = bDeep;
#endif // #ifndef USE_GLOBAL_RECURSIVE
   pcla->dwFlg  = dwFlg;   // carries advice on left/right is a ZIP file
   cl = complist_args( pcla ); // NOT USED
   return cl;

}

/***************************************************************************
 * Function: complist_dirdialog
 *
 * Purpose:
 *
 * Builds a new complist by querying the user for two directory
 * names (IDM_DIR) and scanning those in parallel.
 *
 * Names that match in the same directory will be paired - unmatched
 * names will go in a compitem on their own.
 *
 ***************************************************************************/
COMPLIST
complist_dirdialog(PVIEW view)
{
        //DLGPROC   lpProc;
        INT       fOK;
        COMPLIST  cl;

#ifndef USE_GLOBAL_RECURSIVE
        dlg_recursive = gbRecur;    // establish from global
#else // !#ifndef USE_GLOBAL_RECURSIVE
        dlg_recursive = (g_bNOTRecursive ? FALSE : TRUE);    // FIX20091125 // establish from global
#endif // #ifndef USE_GLOBAL_RECURSIVE y/n

        /* put up a dialog for the two pathnames */
        fOK = Do_DIR_DLG();   // uses IDD_DIRECTORY resource

        if( fOK != IDOK )
        {
           return (NULL);
        }

        //cl = complist_newlist( dialog_leftname, dialog_rightname,
        //          view, dlg_recursive,
        //          dlg_flg );  // a check has already been done on whether either is a ZIP
//   COMPLIST  cl;
        {
           PCLASTR   pcla = &g_sCLA;
           ZeroMemory(pcla, sizeof(CLASTR));
            pcla->pCaller= "complist_dirdialog - g_sCLA";
            pcla->pLeft  = dlg_leftname;
            pcla->pRight = dlg_rightname;
            pcla->pView  = view;
#ifndef USE_GLOBAL_RECURSIVE // FIX20091125
            pcla->bDeep  = dlg_recursive;
#else // #ifndef USE_GLOBAL_RECURSIVE
            if ( dlg_recursive )
                g_bNOTRecursive = FALSE; // FIX20091125
            else
                g_bNOTRecursive = TRUE; // FIX20091125
#endif // #ifndef USE_GLOBAL_RECURSIVE y/n

            pcla->dwFlg  = dlg_flg;   // carries advice on left/right is a ZIP file
            cl = complist_args( pcla );
        }
        return cl;

} /* complist_dirdialog */

#define  MX4MBUF     760
#define  MX1LINE     40


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : FindAFile
// Return type: BOOL 
// Arguments  : LPTSTR pDir
//            : LPTSTR lpf
// Description: Given a VALID folder, try to find this file on that
//              path.
///////////////////////////////////////////////////////////////////////////////
INT   FindAFile( LPTSTR lpm, LPTSTR pDir, LPTSTR lpf, INT level, DWORD dwMax )
{
   INT      bRet = 0;
   LPTSTR   lpw1 = LocalAlloc(LPTR, (264*3));
   LPTSTR   lpw2, lpw3;
   HANDLE   hFind;
   WIN32_FIND_DATA   fd;
   if(!lpw1)
      return FALSE;
   if((strlen(pDir) == 2)&&(pDir[1] == ':')) // DO NOT play with the ROOT of a directory
      return FALSE;
   lpw2 = &lpw1[264];
   lpw3 = &lpw2[264];
   strcpy(lpw1,pDir);
   strcat(lpw1,"\\");
   strcpy(lpw2,lpw1);   // get a COPY of this original
   strcat(lpw1,"*.*");  // create a general FIND mask
   hFind = FindFirstFile(lpw1, &fd);
   if( VFH(hFind) )
   {
      do
      {
         if( strlen(lpm) > dwMax )
         {
            if( !InStr(lpm, ", etc...") )
               strcat(lpm, ", etc...");
            break;
         }
         strcpy(lpw3,lpw2);
         strcat(lpw3,&fd.cFileName[0]);
         if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         {
            if( fd.cFileName[0] != '.' )
            {
               if( strcmpi( &fd.cFileName[0], lpf ) == 0 )
               {
                  sprintf(EndBuf(lpm), "Found [%s]"MEOR, lpw3);
                  bRet++;
               }
               else
               {
                  bRet += FindAFile( lpm, lpw3, lpf, (level + 1), dwMax );
               }
            }
         }
         else
         {
               if( strcmpi( &fd.cFileName[0], lpf ) == 0 )
               {
                  sprintf(EndBuf(lpm), "Found [%s]"MEOR, lpw3);
                  bRet++;
               }
         }
      } while( FindNextFile( hFind, &fd ) );

      FindClose(hFind);
   }
   LocalFree(lpw1);
   return bRet;
}

VOID Add2List( PLE ph, PTSTR ps )
{
   INT len = strlen(ps);
   if(len) {
      PLE pn = MALLOC( sizeof(LE) + len + 1 );
      if(pn) {
         PTSTR ns = (PTSTR)(pn + 1);
         strcpy(ns,ps);
         InsertTailList(ph,pn);
      }
   }
}

LE   tmpFileList = { &tmpFileList, &tmpFileList };
VOID  AddInfo( LPTSTR lpb, LPTSTR pDir, DWORD dwMax )
{
   static TCHAR _s_szaddinfo[264];
   static TCHAR _s_szaddinfo2[264];
   BOOL     bdnadd = FALSE;
//   LPTSTR   lpt = &gszTmpBuf[0];
   LPTSTR   lpt = _s_szaddinfo;
   LPTSTR   p;
   HANDLE   hFind;
   WIN32_FIND_DATA   fd;
   INT      i, ii;
//   LPTSTR   lpf = &g_szBuf2[0];
   LPTSTR   lpf = _s_szaddinfo2;
   strcpy(lpt, pDir);
   p = strrchr(lpt, '\\');   // get last
   if(p)
   {
      strcpy(lpf, &p[1]);  // get the FILE (if there is one)
      while(p)
      {
         *p = 0;
         if((strlen(lpt) == 2)&&(lpt[1] == ':'))
            break;
         hFind = FindFirstFile(lpt, &fd);
         if( VFH(hFind) )
         {
            sprintf(EndBuf(lpb), MEOR"Have located %s:-"MEOR
               "[%s] What follows appears INVALID!"MEOR,
               ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "FOLDER" : "FILE" ),
               lpt );
            FindClose(hFind);
            bdnadd = TRUE;
            ii = 0;
            if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                ( (ii=FindAFile(lpb,lpt,lpf,0,dwMax)) > 0 ) )
            {
               // we have FOUND this file - this could be the item sought
               if( strlen(lpb) < dwMax )
               {
                  sprintf(EndBuf(lpb), "Maybe %s %d is what is sought?"MEOR,
                     ( (ii == 1) ? "this" : "one of these" ),
                     ii );
               }
            }
            i = strlen(lpt);
            *p = '\\';  // put it back
            p++;
            *p = 0;
            strcat(p,"*.*");
            hFind = 0;
            if( strlen(lpb) < dwMax )
               hFind = FindFirstFile(lpt, &fd);
            if( VFH(hFind) )
            {
               INT   j, k, tot;
               PLE   ph = &tmpFileList;
               j = k = tot = 0;
               do
               {
                  if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                     if( strcmp(fd.cFileName,".") && strcmp(fd.cFileName,"..") ) {
                        tot++;
                        if( j == 0 )
                        {
                           if(ii)
                              strcat(lpb, MEOR"Also found: ");
                           else
                              strcat(lpb, MEOR"Found: ");
                        }
                        else if( j > MX1LINE )
                        {
                           strcat(lpb, MEOR);
                           j = 0;
                        }
                        else
                           strcat(lpb, ", ");

                        j += (strlen( &fd.cFileName[0] ) + 3);
                        strcat(lpb, &fd.cFileName[0] );
                        sprintf(EndBuf(lpb), "<%s>",
                           ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "D" : "F" ) );
                        k++;
                     }
               
                  } else {
                     // if a FILE
                     tot++;
                     Add2List( ph, fd.cFileName );
                  }
                  // watch out for TOO big
                  if( strlen(lpb) > dwMax )
                  {
                     strcat(lpb, ", etc...");
                     break;
                  }
                  // ======================
               }while( FindNextFile(hFind, &fd) );

               FindClose(hFind);
               if( !IsListEmpty(ph) && ( strlen(lpb) < dwMax ) ) {
                  // ok to add some file names ...
                  PLE   pn;
                  Traverse_List(ph,pn)
                  {
                     PTSTR ps = (PTSTR)(pn + 1);
                     if( j == 0 ) {
                        if(ii)
                           strcat(lpb, MEOR"Also found: ");
                        else
                           strcat(lpb, MEOR"Found: ");
                     } else if( j > MX1LINE ) {
                        strcat(lpb, MEOR);
                        j = 0;
                     } else
                        strcat(lpb, ", ");

                     j += (strlen(ps) + 3);
                     strcat(lpb, ps );
                     strcat(lpb, "<F>");
                     k++;
                     // watch out for TOO big
                     if( strlen(lpb) > dwMax )
                     {
                        strcat(lpb, ", etc...");
                        break;
                     }
                  }
               }

               if(k)
               {
                  if( ( k == 1 ) && ( ii == 0 ) ) {
                     strcat(lpb, " - ONLY 1 item!");
                  } else if( strlen(lpb) > dwMax ) {
                     sprintf(EndBuf(lpb),
                        MEOR"Total %d items but there are more! (%d or more...)",
                        k, tot );
                  } else {
                     sprintf(EndBuf(lpb),
                        MEOR"Total %d items. Choose one of these!",
                        k );
                  }
               }
               KillLList(ph);
            }
            break;
         }
         p = strrchr(lpt, '\\');
      }
   }

//   if( !bdnadd )
//   {
      // NO folders! Only chance is DRIVE
//      INT   iPos = InStr(lpt, ':');
//      if( iPos > 0 )
//      {
//      }
//   }

}

BOOL  HelpMsg( LPTSTR lpt, LPTSTR pPath, LPTSTR pNew_NOT_USED_YET )
{
   BOOL     bRet = FALSE;
   LPTSTR   lpb = &g_szBuf1[0];

   sprintf(lpb, LoadRcString(IDS_COULDNT_FIND), pPath);

   AddInfo(lpb, pPath, MX4MBUF);

   MB(NULL, lpb, lpt, MB_OK | MB_ICONSTOP);

   return bRet;

}

// FIX20051203 - allow a file mask to be added on input
BOOL got_file_mask( PTSTR pLeft, PTSTR pRite )
{
   // split file path, name
   PTSTR pp1 = GetStgBuf();
   PTSTR pf1 = GetStgBuf();
   PTSTR pp2 = GetStgBuf();
   PTSTR pf2 = GetStgBuf();
   size_t len;
   SplitFN( pp1, pf1, pLeft );
   SplitFN( pp2, pf2, pRite );
   if( strcmpi( pf1, pf2 ) == 0 ) {
      Add2StgList( &g_sInFiles, pf1 );
      strcpy( pLeft, pp1 );   // back the others to FOLDER
      strcpy( pRite, pp2 );   // assume pointer ok to change value

      len = strlen(pLeft);
      if(len && ((pLeft[len-1] == '\\')||(pLeft[len-1] == '/')))
         pLeft[len-1] = 0;

      len = strlen(pRite);
      if(len && ((pRite[len-1] == '\\')||(pRite[len-1] == '/')))
         pRite[len-1] = 0;

      return TRUE;
   }
   return FALSE;
}

/* **************************************************************************
 * Function: complist_args
 *
 * Purpose:
 *
 * Given two pathname strings, scan the directories and traverse them
 * in parallel comparing matching names.
 *
 * The RECURSIVE flag (fDeep) is PASSED on to complist_match(), which in turn
 *    passes it to dir_nextitem(), which will go down the LIST if TRUE
 *
 ************************************************************************** */
extern   TCHAR gszLefti[];
extern   TCHAR gszRitei[];
BOOL  g_bArgsDeep = TRUE;

COMPLIST complist_args( PVOID pv )
{
   PCLASTR   pcla = (PCLASTR)pv;
   COMPLIST  cl;
//   LPTSTR    lpb = &gszNewSel[0];
   LPTSTR    pLeft, pRite;
   PVIEW     view;
   BOOL      fDeep;
   //BOOL      bIsZip;

   if( !pcla )
      return NULL;

   pLeft = pcla->pLeft;
   pRite = pcla->pRight;
   view  = pcla->pView;
#ifndef USE_GLOBAL_RECURSIVE  // FIX20091125
   fDeep = pcla->bDeep;
#else // #ifndef USE_GLOBAL_RECURSIVE
   if ( g_bNOTRecursive )
       fDeep = FALSE; // FIX20091125
   else
       fDeep = TRUE; // FIX20091125
#endif // #ifndef USE_GLOBAL_RECURSIVE
   g_bArgsDeep = fDeep;

   KillLList( &gsXMissList ); // = EXCLUDE these FILES or DIRECTORIES
   resetmisscnt();
#ifdef   ADDSTATS2   // activate another item on the status bar
   g_szListStg[0] = 0;
   SetListStats( g_szListStg );
#endif   // #ifdef   ADDSTATS2   // activate another item on the status bar

   g_dwviewedcnt = 0;   // new list - none viewed yet

   /* alloc a new complist */
   cl        = complist_new();  // where cl->items = List_Create();  for COMPITEMS
   cl->cl_id = GetTickCount();  // set a unique ID
   g_dwScDirs = 0;
   g_dwScFiles = 0;
   g_dwLeftDirs = 0;
   g_dwLeftFiles = 0;
   g_dwRightDirs = 0;
   g_dwRightFiles = 0;

   if( pcla->pCaller == 0 )
      pcla->pCaller = "<blank!>";

   sprtf( MEOR"args: NEW [%s : %s] list for [%s]"MEOR,
      pLeft, pRite,
      pcla->pCaller );

   if( pcla->dwFlg & tf_LeftisZip )
   {
      cl->left = dir_buildlist(pLeft, TRUE, Left_Zip);
//      cl->left = dir_buildlist(pLeft, FALSE, Left_Zip);
   }
   else
   {
      cl->left = dir_buildlist(pLeft, TRUE, tf_IsLeft);
      if ( cl->left == NULL ) {
         // FAILED
         if ( got_file_mask( pLeft, pRite ) ) {
            cl->left = dir_buildlist(pLeft, TRUE, tf_IsLeft);
         }
      }
   }


   /* check that we could find the paths, and report if not */
   //lpb  = gszNLeft;
   //*lpb = 0;
   //while( cl->left == NULL )
   if( cl->left == NULL )
   {
      // not yet completed - always returns FALSE
      HelpMsg( "ERROR FIRST ITEM", pLeft, NULL );
      //if( HelpMsg( "ERROR FIRST ITEM", pLeft, lpb ) )
      //{
      //   cl->left = dir_buildlist( lpb, TRUE );
      //   if(cl->left)
      //      pLeft = lpb;
      //}
      //else
      {
         complist_delete(cl);
         return(NULL);
      }
   }

   g_dwLeftDirs = g_dwScDirs;
   g_dwLeftFiles = g_dwScFiles;
   g_dwScDirs = 0;
   g_dwScFiles = 0;

   if( pcla->dwFlg & tf_RightisZip )
   {
      cl->right = dir_buildlist(pRite, TRUE, Right_Zip);
   }
   else
   {
      cl->right = dir_buildlist(pRite, TRUE, tf_IsRight);
   }
   //lpb  = gszNRite;
   //*lpb = 0;
   //while( cl->right == NULL )
   if( cl->right == NULL )
   {
      // not yet completed - always returns FALSE
      HelpMsg( "ERROR SECOND ITEM", pRite, NULL );
      //if( HelpMsg( "ERROR SECOND ITEM", pRite, lpb ) )
      //{
      //   cl->right = dir_buildlist( lpb, TRUE );
      //   if(cl->right)
      //      pRite = lpb;
      //}
      //else
      {
         complist_delete(cl);
         return(NULL);
      }
   }

   g_dwRightDirs = g_dwScDirs;
   g_dwRightFiles = g_dwScFiles;
   g_dwScDirs = 0;
   g_dwScFiles = 0;
   /* register with the view (must be done after building lists) */
   // and NOTE BOTH cl->left and cl->right are VALID
   // ==============================================
   view_setcomplist(view, cl, fDeep);   // pass on recursive flag
   // has output sprtf( "CMP=[%s : %s]"MEOR, dlg_leftname, dlg_rightname );
   // in service complist_setdlgstgs(pcfds, cl);   // ensure dialog_leftname/dialog_rightname are valid

   complist_match(cl, view, fDeep, TRUE);

   sprtf( "Count in LEFT  list is %d."MEOR, dir_filecount( cl->left  ) );
   sprtf( "Count in RIGHT list is %d."MEOR, dir_filecount( cl->right ) );

   complist_setstats(cl);  // gather statistics

   complist_showstats();

   // if it succeeded, then update global items for INI
   if( strcmpi( gszLefti, pLeft ) )
   {
     strcpy( gszLefti, pLeft );
     bChgLf = TRUE;
   }
   if( strcmpi( gszRitei, pRite ) )
   {
     strcpy( gszRitei, pRite );
     bChgRt = TRUE;
   }

#ifndef USE_GLOBAL_RECURSIVE
   ToggleBool( &gbRecur, &bChgRec, fDeep );
#endif // #ifndef USE_GLOBAL_RECURSIVE

   return(cl);

} /* complist_args */

/***************************************************************************
 * Function: complist_getitems
 *
 * Purpose:
 *
 * Gets the handle to the list of COMPITEMs. The list continues to be
 * owned by the COMPLIST, so don't delete except by calling complist_delete.
 *
 ***************************************************************************/
LIST
complist_getitems(COMPLIST cl)
{
        if (cl == NULL) {
                return(NULL);
        }

        return(cl->items);
}

/***************************************************************************
 * Function: complist_delete
 *
 * Purpose:
 *
 * Deletes a complist and all associated CompItems and DIRLISTs. Note this
 * does not delete any VIEW - the VIEW owns the COMPLIST and not the other
 * way around.
 *
 **************************************************************************/
void
complist_delete(COMPLIST cl)
{
        COMPITEM item;

        if (cl == NULL) {
                return;
        }

        /* delete the two directory scan lists */
        dir_delete(cl->left);
        dir_delete(cl->right);

        /* delete the compitems in the list */
        List_TRAVERSE(cl->items, item)
        {
             compitem_delete(item);
        }

        /* delete the list itself */
        List_Destroy(&cl->items);

        gmem_free(hHeap, (LPSTR) cl, sizeof(struct complist), "complist_new" );

        complist_cleanup();
}

VOID  FixTail( LPTSTR lpb, LPTSTR lph )
{
   INT   ip1 = InStr(dlg_tail, "\\?%s");
   if( ip1 )
   {
      LPTSTR p1 = strrchr(lph,'\\');    // get last of these
      if(p1)
      {
         DWORD dwi = (DWORD)(p1 - lph);  // offset less beginning
         if(dwi)
         {
            INT   il1 = strlen(lpb);
            INT   il2 = strlen(lph);
            LPTSTR   p2 = Left(lph, dwi); // get the folder
            LPTSTR   p3 = Left(dlg_tail, ip1);  // get any BEFORE %s.
            // got folder to insert in place of the %s
            lpb[ (il1 - (strlen(dlg_tail) - 1)) ] = 0;  // take OFF current tail
            strcat(lpb, p3);     // add back BEFORE the "%s."
            if( ( strlen(p2) == 1 ) && ( p2[0] == '.' ) )
               strcat(lpb, p2);
            else
            {
               if(p2[0] == '.')
                  p2++;
               if(p2[0] == '\\')
                  p2++;
               strcat(lpb,p2);
               strcat(lpb, "\\.");
            }
         }
      }
   }
}

//   sprintf( EndBuf(lpb), "%s"MEOR, compitem_gettext_tag(ci) );

#define  OUTBUF(b)\
   dwi = strlen(b);\
   dww = 0;\
   if( ( WriteFile( fh, b, dwi, &dww, NULL ) ) && ( dwi == dww ) ) { \
      dwr += dwi;\
   } else { \
      bOK = FALSE; \
      goto Save_End; \
   }

VOID  GetListLine( LPTSTR lpb, LPTSTR lpb2, COMPITEM ci, DWORD dwo )
{
   *lpb = 0;
   *lpb2 = 0;

   if( dwo & FULL_NAMES )
   {
      // this will be the full root path, plust drive
      compitem_retfullname( lpb2, ci, dwo );
   }
   else if( dwo & ADD_REL_PATH )
   {
      // but I like to remove the starting ".\" !!!
      //strcat(lpb2, compitem_gettext_tag(ci) );
      // and the whole relative path some times
      strcat(lpb2, compitem_gettext_tag3(ci) );
   }
   else
   {
      // This is a RELATIVE path,
      // as above, but no starting ".\" root!!!
      strcat(lpb2, compitem_gettext_tag2(ci) );
   }

   if( dwo & INCLUDE_TAGS )
   {
      strcat(lpb, dlg_head);
      strcat(lpb, lpb2);
      strcat(lpb, dlg_tail);
      FixTail(lpb, lpb2);
   }
   else
   {
      strcat( lpb, lpb2 );
   }

   if( dwo & ADD_COMMENTS )
   {
      if( gbUseCSV )    // comma separate block
         strcat(lpb,",");

      strcat(lpb," ");
      strcat(lpb, compitem_gettext_result(ci));
   }

   strcat(lpb,MEOR);

}

#define  OUTLISTLINE    {\
   GetListLine( lpb, lpb2, ci, dwo );\
   OUTBUF(lpb);\
}

// getting too messy for a MACRO
#define  OUTLISTLINE_XXX   \
   *lpb = 0; *lpb2 = 0;\
   if( dwo & FULL_NAMES )\
      compitem_retfullname( lpb2, ci, dwo ); \
   else if( dwo & ADD_REL_PATH )\
      strcat(lpb2, compitem_gettext_tag3(ci) );\
   else\
      strcat(lpb2, compitem_gettext_tag2(ci) );\
   if( dwo & INCLUDE_TAGS ) { \
      strcat(lpb, dlg_head);\
      strcat(lpb, lpb2);\
      strcat(lpb, dlg_tail);\
      FixTail(lpb, lpb2);\
   } else { strcat( lpb, lpb2 ); } \
   if( dwo & ADD_COMMENTS ) {strcat(lpb," "); strcat(lpb, compitem_gettext_result(ci));} \
   strcat(lpb,MEOR);\
   OUTBUF(lpb)

/***************************************************************************
 * Function: complist_savelist
 *
 * Purpose: Action MENU item IDM_SAVELIST, or
 *    from command line during wd_initial() thread
 *
 * Writes out to a text file the list of compitems as relative filenames
 * one per line.
 *
 * If savename is non-null, use this as the filename for output; otherwise,
 * query the user via a dialog for the filename and include options.
 *
 * FIX20010717 - Put EVERYTHING into the CFDLGSTR, including the SAVE FILE
 * and    COMPLIST di_sCL;           // = list of file (in OUTLINE)
 * part of DDIFF    ta_sDI;     // and incorporate the DDIFF DLG structure
 *    TCHAR    di_szFile[264];   // output file
 *   BOOL     di_bWrap;         // if WRAP output lines
 *  DWORD    di_dwWidth;       // width of output
 *  DWORD    di_dwOpts;        // various BIT options
 *  PVOID    di_pTargs;        // back pointer to main structure
 *  PLE      di_pDFList;       // list of DIFF SAVE FILE list
 * which are part of TARGS    cf_sTARGS;        // structure TARGS
 *
 **************************************************************************/
// from call - complist_savelist(view_getcomplist(current_view), NULL, gdwFileOpts );
DWORD complist_savelist(COMPLIST cl, LPSTR savename, UINT options)   // = IDM_SAVELIST
{
   static BOOL done_init = FALSE;
   BOOL        bOK;
   int         state;
   DWORD       dwFlag;
   HCURSOR     hcurs;
   COMPITEM    ci;
   DWORD       nFiles = 0;
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   LPTSTR      lpb  = &g_szBuf1[0];
   LPTSTR      lpb2 = &g_szBuf2[0];
   UINT        icnt, icntn;
   HANDLE      fh;
   DWORD       dwi, dww, dwr, dwo;
   //PTARGS      pta = &pcfds->cf_sTARGS;
   LPTSTR      poutfile = &pcfds->cf_sTARGS.ta_sDI.di_szFile[0];

   if(( !cl                ) ||
      ( !VALIDPCFDS(pcfds) ) )
   {
      return 0;   // no COMPLIST, or invalid pcfds, then no go
   }

   pcfds->cf_sTARGS.ta_sDI.di_sCL     = cl;   // store the COMPLIST
#ifdef   COMBARGS
   if( pcfds->cf_sTARGS.ta_sTA.view == 0 )
      pcfds->cf_sTARGS.ta_sTA.view    = pcfds->cf_pView;       // copy the view pointer
#else // !#ifdef   COMBARGS
   if( pcfds->cf_sTARGS.ta_pView == 0 )
      pcfds->cf_sTARGS.ta_pView       = pcfds->cf_pView;       // copy the view pointer
#endif   // #ifdef   COMBARGS
   pcfds->cf_sTARGS.ta_sDI.di_pTargs  = &pcfds->cf_sTARGS;
   pcfds->cf_sTARGS.ta_sDI.di_pDFList = &gsFileList;  // list file list
   // ***************************************************************************************
   pcfds->cf_sTARGS.ta_sDI.di_dwOpts  = options;   // gdwFileOpts;  // fill in GLOBAL options
   // ***************************************************************************************
   pcfds->cf_sTARGS.ta_psCFDLGSTR     = pcfds;

   if(( savename == NULL ) ||
      ( *savename == 0   ) )
   {
       // that is the call came from IDM_SAVELIST
      // get the LIST stats
      complist_getstats( pcfds, cl );
      //complist_setdlgstgs( pcfds, cl );
      if( *poutfile == 0 )
      {
         // FIX20051024 - using MSVC7.1 - ensure a NEW FILE passed to dialog
         // FIX20051028 - This SHOULD be in the CWD, NOT runtime!!!
         // LPTSTR lpss = GetModulePathStg(); // new function using GetStgBuf()
         LPTSTR lpss = GetCWDStg(); // new function using GetStgBuf()
         strcpy(poutfile, gszListFil); // get the LAST file used
         if( *poutfile == 0 ) { // if NONE
            strcpy(poutfile, lpss); // use module path
            strcat(poutfile, "TEMPL001.TXT"); // and first LIST FILE NAME
         } else { // we HAVE a last USED, but (a) is it the same path, and
             // (b) does it ALREADY exist
             SplitFN(lpb, lpb2, poutfile); // prepared to mess with it
             if( strcmpi(lpss, lpb) ) {
                 // different PATH
                 strcpy(poutfile, lpss); // use the CURRENT PATH
                 strcat(poutfile, lpb2);
             }
         }
         GetNxtDif(poutfile);   // bump to NEXT non-existant file
      }

      bOK = Do_SAVELIST_DLG( pcfds ); // pass it all into the DIALOG

      if( bOK != IDOK ) /* user cancelled from dialog box */
         return 0; // then back out of here!!!

      bOK = TRUE;    // set as all ok

   }
   else
   {
       // a SAVE name has been given, then NO DIALOG
      strcpy( poutfile, savename );
      bOK = FALSE;    // set for no INI update
   }

   // get OPTIONS, potentially modified by the DIALOG
   dwo = pcfds->cf_sTARGS.ta_sDI.di_dwOpts;   // gdwFileOpts;  // fill in GLOBAL options
   // done later - if( gdwFileOpts != dwo ) { // fill in GLOBAL options

   sprtf( "Writing LIST to [%s]"MEOR, poutfile );
   sprtf( "Save Opts [%s]"MEOR, SaveOpts2Stg(dwo, TRUE) );

   /* try to open the file */
   //fh = OpenFile(savename, &os, OF_CREATE|OF_READWRITE|OF_SHARE_DENY_WRITE);
   fh = OpnFil( poutfile, ( ( dwo & APPEND_FILE ) ? TRUE : FALSE ) );
   if( !VFH(fh) )
   {
      sprintf(lpb, LoadRcString(IDS_CANT_OPEN), savename);
      MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
      return 0;
   }

   hcurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

   dwr = 0;

   /* write out the header lines */
   if( dwo & INCLUDE_HEADER )
   {
      //LPTSTR   pb1, pb2, pb3, pb4;
      LPTSTR   lhead, rhead;
      //pb1 = lpb2;
      //pb2 = &pb1[32];
      //pb3 = &pb2[32];
      //pb4 = &pb3[32];
      lhead = dir_getroot_list(cl->left);
      rhead = dir_getroot_list(cl->right);

      if( dwo & ADD_X_HDR )   // multi-lined version
      {
         sprintf(lpb,
            "; In the compare of:"MEOR
            "; [%s] with"MEOR
            "; [%s]"MEOR
            "; with options [%s]."MEOR,
            lhead,
            rhead,
            SaveOpts2Stg( dwo, TRUE ) );
      }
      else
      {
         // compact, but use a max. wrap width of about 75
         if( ( strlen(lhead) + 20 ) > 75 )
         {
            // compacting is almost impossible, so no try (yet)
            //sprintf(lpb,
            //   "; In the compare of:"MEOR
            //   "; [%s] with"MEOR,
            //   lhead );
            sprintf(lpb,
               "; In the compare of:"MEOR
               "; [%s] with"MEOR
               "; [%s]"MEOR
               "; with options [%s]."MEOR,
               lhead,
               rhead,
               SaveOpts2Stg( dwo, TRUE ) );
         }
         else
         {
            sprintf(lpb,
               "; In the compare of:[%s] with ",
               lhead );
            if( ( strlen(lpb) + strlen(rhead) ) < 75 )
            {
               //LPTSTR   lpo = SaveOpts2Stg( dwo, TRUE );
               LPTSTR   lpo = SaveOpts2Stg( dwo, FALSE );   // get SHORT 'command line' vers
               strcat(lpb,rhead);
               strcat(lpb, " " );
               //  12345678901234
               // "with options [%s]"MEOR, lpo );
               //if( ( strlen(lpb) + strlen(lpo) + 14 ) < 75 )
               //   sprintf(EndBuf(lpb), "with options [%s]"MEOR, lpo );
               //else
               //   sprintf(EndBuf(lpb), MEOR"; with options [%s]"MEOR, lpo );
               //  12345678901234
               // "with [-S%s]"MEOR, lpo );
               if( ( strlen(lpb) + strlen(lpo) + 8 ) < 75 )
                  sprintf(EndBuf(lpb), "with [-S:%s]"MEOR, lpo );
               else
                  sprintf(EndBuf(lpb), MEOR"; with [-S:%s]"MEOR, lpo );

            }
            else
            {
               sprintf(EndBuf(lpb), MEOR"; [%s]"MEOR
                  "; with options [%s]."MEOR,
                  rhead,
                  SaveOpts2Stg( dwo, TRUE ) );
            }
         }
      }

      dir_freeroot_list(cl->left, lhead);
      dir_freeroot_list(cl->right, rhead);

      OUTBUF(lpb);

   }

   /* traverse the list of compitems looking for the
    * ones we are supposed to include
    * 18 April, 2001 - Traverse 8 times, writting the respective files,
    * after putting a HEADER with respective COUNT
    *
    */
   if( dlg_order )
   {
      // ORDER the output of file names to the LIST
      if( dwo & INCLUDE_SAME )
      {
         // include if files are the same
         icnt = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_SAME )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Identical file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Identical file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_SAME )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do IDENTICAL

      if( dwo & INCLUDE_DIFFER )
      {
         // include if different files to be output to list
         icnt = icntn = 0;
         List_TRAVERSE(cl->items, ci)
         {
            dwFlag = compitem_getflag(ci);   // get COMPARE flag = ci->ci_dwFlag
            state = compitem_getstate(ci);
            if( state == STATE_DIFFER )
            {
                if ((dwo == INCLUDE_DIFFER)&&
                    (dwFlag & TT_GOTZERO)) {
                        continue;
                }
               icnt++;
               if( dwFlag & TT_YOUNGER )
               {
                  icntn++;
               //if( lg < 0 )   // Left/First file time is less than Right/second file time.
               //   ci->ci_dwFlag |= TT_OLDER;    // left is older - local change/update
               //else
               //   ci->ci_dwFlag |= TT_YOUNGER;    // left is newer - suggests update
               }
            }
         }

         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Differing file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               //sprintf(lpb, "; Differing file count is %d."MEOR, icnt );
               sprintf(lpb, "; Differing file(s) is %d. Newer = %d."MEOR, icnt, icntn );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               dwFlag = compitem_getflag(ci);   // get COMPARE flag = ci->ci_dwFlag
               state = compitem_getstate(ci);
               if( state == STATE_DIFFER )
               {
                  if ((dwo == INCLUDE_DIFFER)&&
                      (dwFlag & TT_GOTZERO)) {
                      continue;
                  }
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do DIFFER

      if( dwo & INCLUDE_LEFTONLY )
      {
         icnt = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_FILELEFTONLY )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Left only file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Left only file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_FILELEFTONLY )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do LEFTONLY

      if( dwo & INCLUDE_RIGHTONLY )
      {
         icnt = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_FILERIGHTONLY )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Right only file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Right only file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }

            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_FILERIGHTONLY )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do LEFTONLY
   }
   else
   {
      // NOT in dialog order
      // Just as they occur in the list, and are elegible for output
      List_TRAVERSE(cl->items, ci)
      {
              /* check if files of this type are to be listed */
              state = compitem_getstate(ci);
              if ((state == STATE_SAME) && ( !(dwo & INCLUDE_SAME) ) )
                      continue;
              else if ((state == STATE_DIFFER) && ( !(dwo & INCLUDE_DIFFER) ) )
                      continue;
              else if ((state == STATE_FILELEFTONLY) && ( !(dwo & INCLUDE_LEFTONLY) ) )
                      continue;
              else if ((state == STATE_FILERIGHTONLY) && ( !(dwo & INCLUDE_RIGHTONLY) ) )
                      continue;
              nFiles++;
              /* output the list line */
              OUTLISTLINE;
      }
   }

   /* write tail line */
   if( dwo & INCLUDE_HEADER )
   {
      sprintf(lpb, LoadRcString(IDS_FILES_LISTED), nFiles);

      OUTBUF(lpb);
   }

Save_End:

   /* - close file and we are finished */
   if( VFH(fh) )
      CloseHandle(fh);

   if( bOK )
   {
      if( dwo != gdwFileOpts )
      {
         gdwFileOpts = dwo;
         bChgFO = TRUE;    // changed options
         if( (outline_include & INC_OUTLINE2) != (dwo & INC_OUTLINE2) )
         {
            outline_include &= ~(INC_OUTLINE2);
            outline_include |= (dwo & INC_OUTLINE2);
            bChgInc = TRUE;
         }
         // *** REPAINT DISPLAY ACCORDING TO NEW display characteristics
         PostMessage( hwndClient, WM_COMMAND, IDM_REFRESH, 0 );
         // ************************************************************
      }

      Add2SList( &gsFileList, &bChgFLst, poutfile );

      if( strcmpi( poutfile, gszListFil ) )
      {
         strcpy(gszListFil, poutfile);
         bChgLF = TRUE;
      }

      g_bListSave++; //	W.ws_bListSave // g_ INT
      // user actions
      g_dwUseFlag |= uf_DnListOut;  // written out to a list file

   }
   else if( ( savename == NULL ) || ( *savename == 0 ) )
   {
      MB(NULL, "File WRITE ERROR!", APPNAME, MB_ICONSTOP|MB_OK);
   }

   SetCursor(hcurs);

   return nFiles; // return the number written

} /* complist_savelist */

// StatetoString
#define  CLDBG2

#ifdef   USEOLDCOPY2

DIRITEM  complist_dirLR( COMPITEM ci, DWORD dwo )
{
   DIRITEM  diritem;
           if( dwo & COPY_FROMLEFT )
           {
              if( dwo & COPY_FROMRIGHT )
                 return NULL;
              diritem = file_getdiritem(compitem_getleftfile(ci));
           }
           else if( dwo & COPY_FROMRIGHT )
           {
              if( dwo & COPY_FROMLEFT )
                 return NULL;
              diritem = file_getdiritem(compitem_getrightfile(ci));
           }
   return diritem;
}

#endif   // #ifdef   USEOLDCOPY2

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_getdiritem
// Return type: DIRITEM 
// Arguments  : COMPITEM ci
//            : DWORD dwo
// Description: Return the DIRITEM of the LEFT or the RIGHT,
//              according to the OPTIONS given
// BUT it had become overly CLUMSY with ***DELETE***
// so I 'removed' the delete flag, and went with KISS (keep it simple stupid)
// but *MUST* tolerate the small blip on STATE_DIFFER
///////////////////////////////////////////////////////////////////////////////
DIRITEM  complist_getdiritem( COMPITEM ci, DWORD dwo )
{
   DIRITEM  diritem = 0;
   INT      state = compitem_getstate(ci);
   // DWORD    dwFlag = compitem_getflag(ci);
   switch( state )
   {
   case STATE_SAME:
      if( dwo & INCLUDE_SAME )
         diritem = file_getdiritem(compitem_getleftfile(ci));
      break;
   case STATE_DIFFER:
      if( dwo & INCLUDE_DIFFER )
      {
         DWORD    dwFlag = compitem_getflag(ci);
         if( dwFlag & TT_YOUNGER )
         {
            if( dwo & INCLUDE_NEWER )
               diritem = file_getdiritem(compitem_getleftfile(ci));
         }
         else if( dwFlag & TT_OLDER )
         {
            if( dwo & INCLUDE_OLDER )
               diritem = file_getdiritem(compitem_getleftfile(ci));
         }
      }
      break;
   case STATE_FILELEFTONLY:
      if( dwo & INCLUDE_LEFTONLY )
         diritem = file_getdiritem(compitem_getleftfile(ci));
      break;

   case STATE_FILERIGHTONLY:
      if( dwo & INCLUDE_RIGHTONLY )
         diritem = file_getdiritem(compitem_getrightfile(ci));
      break;

   }
   return diritem;
}

#ifndef   USEOLDCOPY2
#define  complist_dirLR(ci,dwo)  file_getdiritem( compitem_getleftfile(ci) )
#endif   // !USEOLDCOPY2

DIRITEM  complist_getlistdiritem( COMPITEM ci, DWORD dwo )
{
   DIRITEM  diritem = NULL;
   INT      state   = compitem_getstate(ci);
   DWORD    dwFlag  = compitem_getflag(ci);

#ifdef   CLDBG3
   LPTSTR   lpb = &g_szBuf1[0];
   strcpy(lpb,    "Left :" );
   if( !compitem_addfullleft( lpb, ci ) )
      strcat(lpb, " ***NONE***" );
   strcat(lpb,MEOR"Right:");
   if( !compitem_addfullright( lpb, ci ) )
      strcat(lpb, " ***NONE***" );
   strcat(lpb,MEOR);
   strcat(lpb, "State is ");
   StateToString(lpb, state, dwFlag);
   strcat(lpb,MEOR);
   AppendCopyOpts( lpb, dwo );
   strcat(lpb,MEOR);
#endif   // CLDBG2

   switch( state )
   {
   case STATE_SAME:
      if( dwo & INCLUDE_SAME )
         diritem = file_getdiritem( compitem_getleftfile(ci) );
      break;

   case STATE_DIFFER:
      if( dwo & INCLUDE_DIFFER )
      {
         if( dwFlag & TT_OLDER )
         {
            if( dwo & INCLUDE_OLDER )
               diritem = complist_dirLR(ci,dwo);
         }
         else if( dwFlag & TT_YOUNGER )
         {
            if( dwo & INCLUDE_NEWER )
               diritem = complist_dirLR(ci,dwo);
         }
         else
         {

            chkme( "WARNING: An option flags contain DIFFER, but NOT younger or older?" );
            if( dwo & (INCLUDE_OLDER | INCLUDE_NEWER) )
               diritem = complist_dirLR(ci,dwo);
         }
      }
      break;

   case STATE_FILELEFTONLY:
      if( dwo & INCLUDE_LEFTONLY )
         diritem = file_getdiritem( compitem_getleftfile(ci) );
      break;


   case STATE_FILERIGHTONLY:
      if( dwo & INCLUDE_RIGHTONLY )
         diritem = file_getdiritem( compitem_getrightfile(ci) );
      break;

   default:
      chkme( "WARNING: What is this STATE of an outline file item?"MEOR );
      break;
   }

   return diritem;
}



VOID  SetFailedMsg( INT state, LPTSTR lpb, DIRITEM diritem, DWORD dwo )
{
   LPTSTR pstr = dir_getfullname(diritem);
   if( ( dwo & INCLUDE_RIGHTONLY ) &&
      ( state == STATE_FILERIGHTONLY ) )
   {
      sprintf( lpb, "FAILED in DELETE of"MEOR
         "[%s]!",
         pstr );
   }
   else
   {
      sprintf(lpb, LoadRcString(IDS_FAILED_TO_COPY), pstr);
   }

   dir_freefullname(diritem, pstr);
   EnsureCrLf(lpb);
   strcat(lpb, "Options are = ");
   AppendCopyOpts(lpb, dwo);
   EnsureCrLf(lpb);

}

VOID  AddOKCancel( LPTSTR lpb )
{
   strcat(lpb, MEOR"Click OK to continue."MEOR
      "Click CANCEL to ABORT!"MEOR );
}

//typedef struct tagCC {
//   DWORD cnt1; // total in LIST
//   DWORD cnt2; // pass the compare vs state test
//   DWORD cnt3; // passed actual copy test criteria
//   BOOL  bUseOpt; // passing options to use for test
//   DWORD dwopts;
//   BOOL  bTestCpy;
//} CC, * PCC;
//VOID complist_countlistable( PCC pcc )
//VOID complist_countcopiable( PCC pcc )

VOID complist_countlistable_OLD( PCC pcc )
{
//   PCFGDLGSTR pcfds = &g_sCFDLGSTR;
   COMPLIST cl = view_getcomplist( current_view );
//   DWORD    dwo = g_sCFDLGSTR.dwCpyOpts;   // get copy options
//   DWORD    dwo = gdwCpyOpts;   // get copy options
   DWORD    dwo;  // = gdwCpyOpts;   // get copy options
//   DWORD    dwo = pcfds->dwCpyOpts;   // get copy options
   DIRITEM   diritem;
   COMPITEM  ci;
   UINT        icnt1, icnt2, icnt3;
   INT         state, dwFlag;
   BOOL        binc;

   if( pcc->bUseOpt )
      dwo = pcc->dwopts;   // use OPTIONS passed by CALLER
   else
      SetCopyOptions( &dwo ); // else use gdwCpyOpts & outline_include

   // **************************************
   icnt1 = icnt2 = icnt3 = 0;   // copy of actions to be done
   List_TRAVERSE(cl->items, ci)
   {
      icnt1++; // total list COUNT
      /* check if files of this type are to be copied */
      //diritem = complist_getdiritem( ci, dwo );
      //diritem = complist_getlistdiritem( ci, dwo );   // simple check state vs flag
      state   = compitem_getstate(ci);
      dwFlag  = compitem_getflag(ci);
      diritem = 0;
      binc = FALSE;
      switch( state )
      {
      case STATE_SAME:
         if( dwo & INCLUDE_SAME )
         {
            diritem = file_getdiritem( compitem_getleftfile(ci) );
            binc = TRUE;
         }
         break;
   
      case STATE_DIFFER:
         if( dwo & INCLUDE_DIFFER )
         {
            if( dwFlag & TT_OLDER )
            {
               if( dwo & INCLUDE_OLDER )
               {
                  diritem = complist_dirLR(ci,dwo);
                  binc = TRUE;
               }
            }
            else if( dwFlag & TT_YOUNGER )
            {
               if( dwo & INCLUDE_NEWER )
               {
                  diritem = complist_dirLR(ci,dwo);
                  binc = TRUE;
               }
            }
            else
            {
               chkme( "WARNING: An option flags contain DIFFER, but NOT youger or older?" );
               if( dwo & (INCLUDE_OLDER | INCLUDE_NEWER) )
               {
                  diritem = complist_dirLR(ci,dwo);
                  binc = TRUE;
               }
            }
         }
         break;
   
      case STATE_FILELEFTONLY:
         if( dwo & INCLUDE_LEFTONLY )
         {
            diritem = file_getdiritem( compitem_getleftfile(ci) );
            binc = TRUE;
         }
         break;
   
      case STATE_FILERIGHTONLY:
         if( dwo & INCLUDE_RIGHTONLY )
         {
            diritem = file_getdiritem( compitem_getrightfile(ci) );
            binc = TRUE;
         }
         break;
   
      default:
         chkme( "WARNING: What is this STATE of an outline file item?"MEOR );
         break;
      }

      // under current Options Switch value,
      // will it be displayed in the outline view
      if( !diritem )
      {
         if(binc)
            chkme( "WARNING: An included item FAILED???!!!???"MEOR );

         continue;   // no - does not count
      }
      icnt2++;

      if(( state == STATE_FILELEFTONLY  ) ||
         ( state == STATE_FILERIGHTONLY ) )
      {
         icnt3++; // most likely copies, or delete item
      }

   } /* traverse */
   // **************************************

   pcc->cnt1 = icnt1;
   pcc->cnt2 = icnt2;
   pcc->cnt3 = icnt3;
}

VOID complist_countlistable( PCC pcc )
{
//   PCFGDLGSTR pcfds = &g_sCFDLGSTR;
   COMPLIST cl = view_getcomplist( current_view );
//   DWORD    dwo = gdwCpyOpts;   // get copy options
   DWORD       dwo = outline_include;  // the file list is per OUTLINE display bits
   UINT        icnt1, icnt2, icnt3;
//   complist_countlistable_OLD( pcc );

   complist_setstats( cl );

   if( pcc->bUseOpt )
      dwo = pcc->dwopts;   // use OPTIONS passed by CALLER
   // **************************************

   icnt1 = g_ListTotal; // - total in 'combined' file list
   icnt2 = icnt3 = 0;   // copy of actions to be done
// g_LeftCnt   - total in left tree
// g_RightCnt  - Total in right tree
   if( dwo & INCLUDE_SAME )
      icnt2 += g_SameCnt;  // add count of files that are the SAME

   if( dwo & INCLUDE_DIFFER )
   {
      if( dwo & INCLUDE_OLDER )
         icnt2 += g_OlderCnt; //  - Maybe destination ALSO modified

      if( dwo & INCLUDE_NEWER )
         icnt2 += g_NewerCnt; //  - if date newer - ie ready for update
   }

   if( dwo & INCLUDE_LEFTONLY )
      icnt2 += g_LeftOnly; //  - files ONLY in left tree

   if( dwo & INCLUDE_RIGHTONLY )
      icnt2 += g_RightOnly;   // - files ONLY in right tree

   icnt3 += g_LeftOnly +   // - files ONLY in left tree
            g_RightOnly;   // - files ONLY in right tree

   //if(( pcc->cnt1 != icnt1 ) ||
   //   ( pcc->cnt2 != icnt2 ) )   //      ( pcc->cnt3 != icnt3 ) )
   //{
      //chkme( "WARNING: Got 1=%d 2=%d 3=%d vs 1=%d 2=%d 3=%d!"MEOR,
      //   icnt1, icnt2, icnt3,
      //   pcc->cnt1, pcc->cnt2, pcc->cnt3 );
   //   chkme( "WARNING: Got 1=%d 2=%d vs 1=%d 2=%d!"MEOR,
   //      icnt1, icnt2,
   //      pcc->cnt1, pcc->cnt2 );
   //}

   pcc->cnt1 = icnt1;   // list TOTAL
   pcc->cnt2 = icnt2;
   pcc->cnt3 = icnt3;
}

VOID complist_countcopiable( PCC pcc )
{
   static CPYTST _s_ct;
//   PCFGDLGSTR pcfds = &g_sCFDLGSTR;
   COMPLIST cl = view_getcomplist( current_view );
//   DWORD    dwo = g_sCFDLGSTR.dwCpyOpts;   // get copy options
//   DWORD    dwo = pcfds->dwCpyOpts;   // get copy options
//   DWORD       dwo = gdwCpyOpts;    // get current COPY options
   DWORD       dwo;    // get current COPY options
   DIRITEM     diritem;
   COMPITEM    ci;
   UINT        icnt1, icnt2, icnt3;
   PCPYTST     pct = &_s_ct;
   INT         state;   // = compitem_getstate(ci);

   SetCopyOptions( &dwo );

   // **************************************
   icnt1 = icnt2 = icnt3 = 0;   // copy of actions to be done
   List_TRAVERSE(cl->items, ci)
   {
      icnt1++; // total list COUNT
      /* check if files of this type are to be copied */
      diritem = complist_getdiritem( ci, dwo );

      // under current Copy Option Switch, it will be displayed in the outline view
      if( !diritem )
         continue;

      state = compitem_getstate(ci);
      icnt2++;

      if( pcc->bTestCpy )
      {
         if( state == STATE_FILERIGHTONLY )
         {
            // this is a *** DELETE *** orphaned files = files only in RIGHT
            // so a 'copy test' is as simple as does it exist
            //LPTSTR pnp = &g_szNewPath[0];
            if( dir_deletetest( pct, diritem ) )
               icnt3++;
         }
         else
         {
            // test the COPY
            ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
            pct->ct_ci      = ci;   // set the COMPITEM
            pct->ct_diritem = diritem;
            pct->ct_iState  = state;   // compitem_getstate(ci);    // state of the entry
            pct->ct_dwFlag  = compitem_getflag(ci);
            //pct->ct_pcfds   = pcfds;   // and always a pointer to primary structure
            pct->ct_pcfds   = 0;   // and always a pointer to primary structure
            if( dir_copytest( 0, pct ) )
            {
               icnt3++; // successful copies, or close to as can be guessed
               // without actually DOING the COPY #$%#$*&
            }
            //else {
               //*lpb = 0;
            // SetFailedMsg( state, lpb, diritem, dwo );
            // chkme( "WILL FAIL [%s]"MEOR, lpb ); }
         }
      }

   } /* traverse */
   // **************************************
   pcc->cnt1 = icnt1;
   pcc->cnt2 = icnt2;
   pcc->cnt3 = icnt3;

}



VOID  OutCopyOpts( LPTSTR pmsg, DWORD dwo )
{

   LPTSTR lpb2 = GetStgBuf();
   *lpb2 = 0;
   //AppendCopyOptLetters( lpb2,
   AppendCopyOpts( lpb2,
      (dwo & INC_OUTLINE2 ) );
   sprtf( "%s = [%s]"MEOR, pmsg, lpb2 );
}

BOOL  inoutlist( DWORD dwo, INT state, DWORD dwFlag )
{
   switch(state)
   {
   case STATE_SAME:
      if( dwo & INCLUDE_SAME )
         return TRUE;
      else
         return FALSE;
      break;

   case STATE_DIFFER:
      if( dwo & INCLUDE_DIFFER )
      {
         if( dwFlag & TT_OLDER )
         {
            if(dwo & INCLUDE_OLDER)
               return TRUE;
            else
               return FALSE;
         }
         else if( dwFlag & TT_YOUNGER )
         {
            if(dwo & INCLUDE_NEWER)
               return TRUE;
            else
               return FALSE;
         }
         return FALSE;
      }
      else
         return FALSE;
      break;

   case STATE_FILELEFTONLY:
      if( dwo & INCLUDE_LEFTONLY )
         return TRUE;
      else
         return FALSE;
      break;

   case STATE_FILERIGHTONLY:
      if( dwo & INCLUDE_RIGHTONLY )
         return TRUE;
      else
         return FALSE;
      break;

   }

   return FALSE;

}

/* **************************************************************************
 * Function: complist_copyfiles
 *
 * Purpose:
 *
 * To copy files to a new directory newroot. if newroot is NULL, query the user
 * via a dialog to get the new dir name and options.
 *
 * Options are either COPY_FROMLEFT or COPY_FROMRIGHT
 * (indicating which tree is to be the source of the files),
 * plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT
 * (INCLUDE_LEFT and INCLUDE_RIGHT are treated the same here since
 * the COPY_FROM* option indicates which side to copy from).
 *
 * In present implementation ONLY called from wd_copy THREAD with newroot
 * as NULL.
 *
 * Returns TRUE if from and to are the current compare folders, and we have
 * successfully completed a COPY operation. There is NEED to REFRESH the view
 *
 * Presently ONLY called from DWORD wd_copy(LPVOID arg) thread,
 * which in turn is ONLY created in VOID  Do_IDM_COPYFILES( HWND hWnd ),
 * which is the result of the MENU item - IDM_COPYFILES
 * NOTE: In present implementation newroot is always NULL.
 * AND have already called UpdCopyDelStrs( pcfds )
 *
 * Is actually passed    PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
 *
 ************************************************************************** */
BOOL complist_deleteright( PVIEW view, PLE pHead )
{
   BOOL        bRet = FALSE;
   COMPLIST    cl;
   DWORD       dwo = (outline_include & INC_OUTLINE2);
   COMPITEM    ci;
   int         state;
   INT         icnt, icnt2;
//   INT         icnt3;
   DWORD       dwFlag;
   DIRITEM     diritem;
   PCPYTST     pct;     // allocated item, added to LIST
   LPTSTR      pfn;  // = dir_getfullname(diritem);
   PLE         pn;

   cl = view_getcomplist( view );   // pcfds->cf_pView );
   if( !cl )
      return FALSE;

   icnt = icnt2 = 0;   // copy of actions to be done
   List_TRAVERSE(cl->items, ci)
   {
      /* check if files of this type are to be copied */
      diritem = complist_getdiritem( ci, dwo );

      if( !diritem )
         continue;

      icnt++;
      state = compitem_getstate(ci);
      dwFlag = compitem_getflag(ci);
      if( state == STATE_FILERIGHTONLY )
      {
            pct = (PCPYTST) MALLOC( sizeof(CPYTST) );
            if(pct)
            {
               ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
               pct->ct_ci      = ci;
               pct->ct_diritem = diritem;
               pct->ct_iState  = state;    // state of the entry
               pct->ct_dwFlag  = dwFlag;  // compitem_getflag(ci);
//               pct->ct_pcfds   = pcfds;    // have primary pointer ALWAYS avaiable
               // if( dir_copytest( pcfds, pct ) )
               if( dir_deletetest( pct, diritem ) )
               {
                  InsertTailList(pHead, (PLE)pct );
                  icnt2++;
               }
               else
               {
                  //*lpb = 0;
                  //SetFailedMsg( state, lpb, diritem, dwo );
                  chkme( "WILL FAIL"MEOR );  // [%s]"MEOR, lpb );
               }
            }
            else
            {
               chkme( "C:ERROR: Unable to do ... because get MEMORY FAILED!"MEOR );
               //MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
               return FALSE;
            }

//            pfn = dir_getfullname(diritem);
//         if(pfn)
//         {
//            sprtf( "To DELETE [%s]"MEOR, pfn );
//            dir_freefullname(diritem, pfn);
//         }

      }
   }

   if( icnt2 )
   {
      PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
      LPTSTR      pmsg  = &g_szMsg[0];
      BOOL        bOK;
      INT         i;

      bRet = icnt2;
      pcfds->nFiles = pcfds->nFails = pcfds->nSkips = 0;
      Traverse_List( pHead, pn )
      {
         pct = (PCPYTST)pn;
         pfn = &pct->ct_szCopyFull[0];
         sprtf( "To DELETE [%s]"MEOR, pfn );
      //if( pcfds->dwVerFlag & (MAKE_BACKUP | VERIFY_EACH) )
//      hFind = FindFirstFile( pnp, pfdc ); // COPY find file information
//      if( VFH(hFind) )
//      {
//         FindClose(hFind); // done with this

         if( ( pcfds->dwVerFlag & VERIFY_EACH ) ||
            !( (pcfds->dwVerFlag & REVIEW_LIST) || (pcfds->dwVerFlag & MAKE_BACKUP) ) )
         {
            if( pcfds->dwVerFlag & VERIFY_EACH )
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
               (MB_ICONINFORMATION|MB_ALLOKNOALL) );
//               (MB_ICONINFORMATION|MB_OKCANCEL) );
            if( i == IDC_COPYALL )
            {
               pcfds->dwVerFlag &= ~(VERIFY_EACH | VERIFY_OLDER);
               pcfds->dwVerFlag |= MAKE_BACKUP;
            }
            else if( i == IDC_ABORTALL )
            {
                pct->ct_dwFlag |= (flg_Abort|flg_User);
                //goto Tidy_Exit;
                break;
            }
            else if( i == IDCANCEL )
            {
               pct->ct_dwFlag |= flg_User;   // flag as a USER skip
               //goto Tidy_Exit;
               pcfds->nSkips++;
               continue;
            }
         }

         if( pcfds->dwVerFlag & MAKE_BACKUP )
         {
            bOK = dir_filedelete( pfn );
         }
         else
         {
            bOK = DeleteFile( pfn );
         }
         if( bOK )
         {
            pcfds->nFiles++;
         }
         else
         {
            // what to do on ERROR????
            pcfds->nFails++;
         }
      }
   }

   return bRet;
}

//   PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
BOOL complist_copyfiles( PVOID pv ) // IDM_COPYFILES and IDM_FILECOPY - on thread
{
#define  nFiles   pcfds->nFiles
#define  nFails   pcfds->nFails
#define  nSkips   pcfds->nSkips
   BOOL        bRet = FALSE;
   PCFDLGSTR   pcfds = (PCFDLGSTR)pv;  // actually g_sCFDLGSTR
   COMPLIST    cl;
   //int       nFiles = 0;
   //int       nFails = 0;
   //int       nSkips = 0;
   //static BOOL done_init = FALSE;
   LPSTR     pstr;
   LPTSTR    lpb = &g_szBuf1[0];
   DIRITEM   diritem;
   BOOL      bOK;
   COMPITEM  ci;
   int       state;
   INT         it;
   INT         icnt, icnt2;
   INT         icnt3;
   PLIST_ENTRY pHead = &gsCopyList; // LIST to use
   PLIST_ENTRY pNext;
   PCPYTST     pct;     // allocated item, added to LIST
   double      db;
   BOOL        bbrk = FALSE;
   DWORD       dwo, dwb, dworg;
   LPTSTR      pdst;

   if( !VALIDPCFDS( pcfds ) )
      return FALSE;

   cl = view_getcomplist( pcfds->cf_pView );
   if( !cl )
      return FALSE;

   //FreeLList(pHead,pNext);

   //pcfds->bSingle = FALSE;  // clear the SINGLE copy flag
   nFiles = 0;
   nFails = 0;
   nSkips = 0;
   pcfds->cf_liCopied.QuadPart = pcfds->cf_liDeleted.QuadPart =
      pcfds->cf_liToBin.QuadPart = 0;

   //if( !done_init )
   //{
      /*
       * one-time initialisation of dialog defaults
       */
   //   dlg_root[0] = '\0';   // zero the temporary buffer
      //done_init = TRUE;
   //}
   pdst = &pcfds->cf_szDest[0];      // clear this DESTINATION
   *pdst = 0;

   dwb = gdwCpyOpts;   // get copy at beginning
   //pcfds->dwCpyOpts = dwb; // = g_s????????
   dwo  = dwb & ~(INC_OUTLINE2);
   dwo |= outline_include & INC_OUTLINE2;
   pcfds->dwCpyOpts = dwo;
   dworg = dwo;
   OutCopyOpts( "CopyOptions BEFORE dialog", dwo );

   //pcfds->bVerify = gbVerify; // extract the GLOBAL flag - see REVIEW_LIST in gdwVerFlag
   pcfds->dwVerFlag = gdwVerFlag;   // extract GLOBAL flag

   //if( dwb != options )
   //   pcfds->dwCpyOpts = options;

   // get some sort of DEFAULT values
   ci = view_getcompitem(current_view);

   diritem = file_getdiritem(compitem_getleftfile(ci));

   //UpdGlobCopyDelStrs();

   //if( newroot == NULL )
   complist_getstats( pcfds, cl );  // get LIST statistics

      /* put up dialog to query rootname and options */

      /* store the left and right rootnames so that the dlg proc
       * can display them in the dialog.
       */
      //pstr = dir_getroot_list(cl->left);
      //strcpy(dialog_leftname, pstr);
      //dir_freeroot_list(cl->left, pstr);

      //pstr = dir_getroot_list(cl->right);
      //strcpy(dialog_rightname, pstr);
      //dir_freeroot_list(cl->right, pstr);
Redo_Dialog:

   FreeLList(pHead,pNext);

   pcfds->bSingle = FALSE;  // clear the SINGLE copy flag
   //pdst = &pcfds->cf_szDest[0];      // clear this DESTINATION

   *pdst = 0;
      do
      {
         // see dc4wDlgR.c for -
         bOK = Do_COPYFILES_DLG( pcfds ); // IDM_COPYFILES\IDM_COPYFILES2

         if( bOK != IDOK ) /* user cancelled from dialog box */
         {
            return FALSE;
         }

         //if( strlen(dlg_root) == 0 )
         if( *pdst )
         {
            sprtf( "COPY destination = [%s]"MEOR, pdst );
         }
         else
         {
            // destination buffer remains NULL
            MB(NULL, LoadRcString(IDS_ENTER_DIR_NAME),
                                      APPNAME, MB_ICONSTOP|MB_OK);
         }
      } while ( *pdst == 0 );    // (strlen(dlg_root) == 0);

   dwo = pcfds->dwCpyOpts;   // extract the potentially updated COPY options
   if( dwo == dworg )
      sprtf( "Copy options remain UNCHANGED"MEOR );
   else
      OutCopyOpts( "CopyOtions AFTER dialog", dwo );

   // NOTE: This 'startcopy' clears a local variable for the COUNT
   // and nCopies in the DIRLIST item, if not zero
//   if( ( dwo & COPY_FROMLEFT ) ||
//       ( pcfds->bSingle      ) ) 
   if( pcfds->bSingle )
   {
      if( !dir_startcopy(cl->left) )
      {
         // oops, left list is NULL
         if( pcfds->bSingle )
         {
            strcpy(lpb, "WARNING: Unable to do single copy because LEFT directory list is NULL" );
         }
         else
         {
            strcpy(lpb, "WARNING: Unable to do copy from left because LEFT directory list is NULL" );
         }

         MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);

         return FALSE;
      }
   }
   else
   {
      if( !dir_startcopy(cl->right) )
      {
         strcpy(lpb, "WARNING: Unable to do copy from right because RIGHT directory list is NULL" );
         MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
         return FALSE;
      }
   }

   // traverse the list first to get the COPY count
   icnt = icnt2 = 0;   // copy of actions to be done

   if( pcfds->bSingle )
   {
      //ci = view_getcompitem( pcfds->cf_pView ); // if expanded
      //if( !ci )   // else in outline mode, so get SELECTED item
      // NOTE: If EXPANDED then return current expanded COMPITEM,
      // else if in OUTLINE, the ONLY if the cf_iSelected is VALID
      ci = view_getitem( pcfds->cf_pView, pcfds->cf_iSelected ); 
      if(ci)
      {
         state = compitem_getstate(ci);
         diritem = file_getdiritem(compitem_getleftfile(ci));
         //else
         //   diritem = file_getdiritem(compitem_getrightfile(ci));
         if(diritem)
         {
            pct = (PCPYTST) MALLOC( sizeof(CPYTST) );
            if(pct)
            {
               ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
               pct->ct_ci      = ci;
               pct->ct_diritem = diritem;
               pct->ct_iState  = state;    // state of the entry
               pct->ct_dwFlag  = compitem_getflag(ci);
               pct->ct_pcfds   = pcfds;    // have primary pointer ALWAYS avaiable
               if( dir_copytest( pcfds, pct ) )
               {
                  InsertTailList(pHead, (PLE)pct );
                  icnt2++;
               }
               else
               {
                  *lpb = 0;
                  SetFailedMsg( state, lpb, diritem, dwo );
                  chkme( "WILL FAIL [%s]"MEOR, lpb );
               }
            }
            else
            {
               strcpy(lpb, "WARNING: Unable to do single copy from left because get MEMORY FAILED!" );
               MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
               return FALSE;
            }
            icnt++;
         }
         else
         {
            strcpy(lpb, "WARNING: Unable to do single copy from left because FAILED to get DIRITEM of left!" );
            MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
            return FALSE;
         }
      }
      goto Done_Trav1;
   }

   // else we process the LIST of COMPITEM's
   // **************************************
   icnt = icnt2 = 0;   // copy of actions to be done
   List_TRAVERSE(cl->items, ci)
   {
      /* check if files of this type are to be copied */
      diritem = complist_getdiritem( ci, dwo );

      if( !diritem )
         continue;

      state = compitem_getstate(ci);
      if( !inoutlist( dwo, state, compitem_getflag(ci) ) )
         chkme( "HEY! This diritem is NOT in the LIST?"MEOR );

      pct = (PCPYTST) MALLOC( sizeof(CPYTST) );
      if(pct)
      {
         ZeroMemory( pct, sizeof(CPYTST) );  // clear out EVERYTHING
         pct->ct_ci      = ci;   // set the COMPITEM
         pct->ct_diritem = diritem;
         pct->ct_iState  = state;    // state of the entry
         pct->ct_dwFlag  = compitem_getflag(ci);
         pct->ct_pcfds   = pcfds;   // and always a pointer to primary structure
         InsertTailList(pHead, (PLE)pct );
         if( state == STATE_FILERIGHTONLY )
         {
            if( dir_deletetest( pct, diritem ) )
            {
               icnt2++;
            }
            else
            {
               *lpb = 0;
               SetFailedMsg( state, lpb, diritem, dwo );
               chkme( "WILL FAIL [%s]"MEOR, lpb );
            }
         }
         else
         {
            if( dir_copytest( pcfds, pct ) )
            {
               icnt2++;
            }
            else
            {
               *lpb = 0;
               SetFailedMsg( state, lpb, diritem, dwo );
               chkme( "WILL FAIL [%s]"MEOR, lpb );
            }
         }
      }
      icnt++;

   } /* traverse */
   // **************************************

Done_Trav1:

   db = 0.0;
   if( icnt && ( icnt != icnt2 ) )
   {
      *lpb = 0;
      if(diritem)
      {
         LPTSTR pfn = dir_getfullname(diritem);
         if(pfn)
         {
            sprintf(EndBuf(lpb),
               "In the matter of copying"MEOR
               "[%s] to"MEOR
               "[%s]."MEOR,
               pfn,
               pdst );  // dlg_root );
            dir_freefullname(diritem, pfn);
         }
      }

      sprintf( EndBuf(lpb), "It appears one or more"MEOR
         "of the COPIES will FAIL!"MEOR
         "Only %d of total %d"MEOR
         "seem copiable!",
         icnt2, icnt );

      if( dir_isvalidfile( pdst ) )    // dlg_root
      {
         strcat( lpb, MEOR"Perhaps because the DESTINATION"MEOR
            "appears to be a FILE!" );
      }

      strcat(lpb, MEOR"Contiue with copy REGARDLESS?");

      bOK = MB(hwndClient, lpb,
         "COPY ERROR DETECTED",
         MB_YESNO|MB_ICONINFORMATION);

      if( bOK != IDYES )
         return FALSE;

      //pcfds->bVerify = TRUE;   // also ADD the verification message at the end
      pcfds->dwVerFlag |= REVIEW_LIST;


   }

   if( icnt == 0 )
   {
      // Found NO files to COPY\r\nAborting process.
      strcpy(lpb, LoadRcString(IDS_NO_COPY) );
      EnsureCrLf(lpb);
      strcat(lpb, "Copy options = ");
      AppendCopyOpts(lpb, dwo);
      EnsureCrLf(lpb);

      MB(hwndClient,
         lpb,
         APPNAME,
         MB_OK|MB_ICONINFORMATION);
      return FALSE;

   }
   else // if( icnt == icnt2 )
   {
      if( strcmpi( dlg_rightname, pcfds->cf_szDest ) ) {
         pcfds->dwVerFlag |= REVIEW_LIST; // ADD REVIEW
         pcfds->bSingle = 0; // CLEAR SINGLE
      }

      //if( pcfds->bVerify || pcfds->bSingle )
      if( ( pcfds->dwVerFlag & REVIEW_LIST ) ||
          ( pcfds->bSingle                 ) )
      {

         // if VERIFY *or* bSingle is ON, use ONLY the list created
         if( pcfds->bSingle )
         {
            ListCount2( pHead, &icnt );
         }
         else
         {
            // show the list for verification and amendment
            //icnt = complist_showcopydlg( pHead, icnt );
            pcfds->cf_pList = pHead;
            //icnt = complist_showcopydlg( pcfds, icnt );
            if( complist_showcopydlg( pcfds, icnt, &icnt ) )
            {
               icnt = icnt2 = 0;
               goto Redo_Dialog;
            }
         }

         if( icnt == 0 )
            return FALSE;  // quietly exit since none to copy

         icnt3 = 0;  // count of DELETE items, always done LAST of ALL
         TickCount = GetTickCount();
         it = SetBTime();
         Traverse_List( pHead, pNext )
         {
            if( bAbort )
               break;  /* fall into end_copy processing */
            pct = (PCPYTST)pNext;
            if( !( pct->ct_dwFlag & flg_Delete ) )
            {
               if(( pct->ct_iState == STATE_FILERIGHTONLY ) &&
                  ( dwo & INCLUDE_RIGHTONLY               ) )  //= pcfds->dwCpyOpts;   // extract the potentially updated COPY options
               {
                  icnt3++; // count a delete
                  continue;
               }

               pct->ct_dwFlag |= flg_DnAction;  // add in completed flag
               /* actually COPY the file to the new root directory
                * ************************************************ */
               pct->ct_dwFlag &= ~(flg_User|flg_Abort);   // ensure NO USER SKIP
               if( dir_copy( pct, pcfds ) == FALSE )
               {
                  if( pct->ct_dwFlag & flg_User )   // USER SKIP
                  {
                     nSkips++;
                     if( pct->ct_dwFlag & flg_Abort )   // USER ABORT
                     {
                        bbrk = TRUE;
                        break;
                     }
                  }
                  else
                  {
                     nFails++;
                     pstr = dir_getrelname(diritem);
                     sprintf(lpb, LoadRcString(IDS_FAILED_TO_COPY), pstr);
                     dir_freerelname(diritem, pstr);
                     db += GetETime(it);
                     if( MB(NULL, lpb, NULL, MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
                     {
                        bbrk = TRUE;
                        break;
                     }
                     it = SetBTime();
                  }
               }
               else
                   nFiles++;

               sprintf(lpb, LoadRcString(IDS_COPYING), nFiles);
               SetStatus(lpb);

            }

            /* allow user interface to continue */
            // but at the moment just returnns the bAbort flag,
            // since this SHOULD be running on a thread, with
            // the UI also stil running.
            if( Poll() )
            {
               /* abort requested */
               TickCount = GetTickCount()-TickCount;
               db += GetETime(it);
               MB(hwndClient, LoadRcString(IDS_COPY_ABORTED),
                           APPNAME, MB_OK|MB_ICONINFORMATION);
               bbrk = TRUE;
               break;
            }
         }  // traverse the list, excluding delete items

         if( icnt3 ) { // if there are deletes, now is the time

         Traverse_List( pHead, pNext )
         {
            if( bAbort )
               break;  /* fall into end_copy processing */

            pct = (PCPYTST)pNext;
            if(( pct->ct_dwFlag & flg_Delete          ) ||
               ( pct->ct_iState != STATE_FILERIGHTONLY) ||
               !( dwo & INCLUDE_RIGHTONLY             ) )  //= pcfds->dwCpyOpts;   // extract the potentially updated COPY options
            {
               continue;
            }

            {
               pct->ct_dwFlag |= flg_DnAction;  // add in completed flag
               /* actually COPY the file to the new root directory
                * ************************************************ */
               pct->ct_dwFlag &= ~(flg_User|flg_Abort);   // ensure NO USER SKIP
               //if( dir_copy( pct->ct_diritem, dlg_root, pct, pcfds ) == FALSE )
               if( dir_copy( pct, pcfds ) == FALSE )
               {
                  if( pct->ct_dwFlag & flg_User )   // USER SKIP
                  {
                     nSkips++;
                     if( pct->ct_dwFlag & flg_Abort )   // USER ABORT
                     {
                        bbrk = TRUE;
                        break;
                     }
                  }
                  else
                  {
                     nFails++;
                     pstr = dir_getrelname(diritem);
                     //sprintf(lpb, LoadRcString(IDS_FAILED_TO_COPY), pstr);
                     sprintf(lpb, "Failed to delete file"MEOR
                        "[%s]", pstr);
                     dir_freerelname(diritem, pstr);
                     db += GetETime(it);
                     if( MB(NULL, lpb, NULL, MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
                     {
                        bbrk = TRUE;
                        break;
                     }
                     it = SetBTime();
                  }
               }
               else
                   nFiles++;

               //sprintf(lpb, LoadRcString(IDS_COPYING), nFiles);
               sprintf(lpb, "%4d files requested. Deleting...", nFiles );
               SetStatus(lpb);

            }

            /* allow user interface to continue */
            // but at the moment just returnns the bAbort flag,
            // since this SHOULD be running on a thread, with
            // the UI also stil running.
            if( Poll() )
            {
               /* abort requested */
               TickCount = GetTickCount()-TickCount;
               db += GetETime(it);
               //MB(hwndClient, LoadRcString(IDS_COPY_ABORTED),
               MB(hwndClient, "Delete aborted by user",
                           APPNAME, MB_OK|MB_ICONINFORMATION);
               bbrk = TRUE;
               break;
            }

         }  // traverse the list, only doing delete items
         }  // only if there are delete items in the list

         if( pcfds->bSingle )
            pcfds->dwVerFlag &= ~(REVIEW_LIST);
            //pcfds->bVerify = FALSE;

         goto Copy_Done;
      }  // bVerify (or SINGLE) flag
   }

   // ***************************************
   // USE THE UNVERIFIED LIST
   // ***************************************
   TickCount = GetTickCount();
   it = SetBTime();

   /*
    * traverse the list of compitems copying files as necessary
    */
   List_TRAVERSE(cl->items, ci)
   {
           if( bAbort )
           {
              break;  /* fall into end_copy processing */
           }

            /* check if files of this type are to be copied */
           state = compitem_getstate(ci);
           diritem = complist_getdiritem( ci, dwo );

           if( !diritem )
              continue;

           pct = (PCPYTST)pHead;
           Traverse_List( pHead, pNext )
           {
              pct = (PCPYTST)pNext;
              if( pct->ct_diritem == diritem )
                 break;
           }
           if( pct->ct_diritem != diritem )
           {
              chkme( "ERROR: Failed to FIND the correct pct structure!!!"MEOR );
              nFails++;
              SetFailedMsg( state, lpb, diritem, dwo );
              strcat(lpb, MEOR"ERROR:Failed to FIND pct structure!"MEOR);
              AddOKCancel(lpb);
              sprtf(lpb);
              db += GetETime(it);
              if( MB(NULL, lpb, NULL, MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
              {
                 bbrk = TRUE;
                 break;
              }
              it = SetBTime();
           }

           /*
            * actually COPY the file to the new root directory
            * ************************************************
            */
           //pct->ct_szMsg[0] = 0;
           g_szMsg[0] = 0;
           pct->ct_dwFlag &= ~(flg_User|flg_Abort);   // ensure NO USER SKIP
           //if( dir_copy(diritem, dlg_root, pct, pcfds) == FALSE )
           // ******* THE ACTUAL COPY *******
           if( dir_copy(pct, pcfds) == FALSE )
           {
              if( pct->ct_dwFlag & flg_User )   // USER SKIP
              {
                 nSkips++;
                     if( pct->ct_dwFlag & flg_Abort )   // USER ABORT
                     {
                        bbrk = TRUE;
                        break;
                     }
              }
              else
              {
                   nFails++;
                   SetFailedMsg( state, lpb, diritem, dwo );
                   if( g_szMsg[0] )
                   {
                      strcat( lpb, &g_szMsg[0] );
                      EnsureCrLf(lpb);
                   }
                   AddOKCancel(lpb);
                   sprtf(lpb);
                   db += GetETime(it);
                   if( MB(NULL, lpb, NULL, MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL)
                   {
                       /* user pressed cancel - abort current operation*/
                       /* fall through to end-copy processing */
                      bbrk = TRUE;
                      break;
                   }
                   it = SetBTime();
              }
           }
           else
           {
                   nFiles++;
           }

           sprintf(lpb, LoadRcString(IDS_COPYING), nFiles);
           SetStatus(lpb);

           /*
            * allow user interface to continue
            */
           if( Poll() )
           {
                   /* abort requested */
                   TickCount = GetTickCount()-TickCount;
                   db += GetETime(it);
                   MB(hwndClient, LoadRcString(IDS_COPY_ABORTED),
                           APPNAME, MB_OK|MB_ICONINFORMATION);
                   bbrk = TRUE;
                   break;
           }

   } /* traverse */

Copy_Done:

   // this is really redundant since dir_copy() returns TRUE=success or FALSE
   // and a local COUNT is kept
//   if( ( dwo & COPY_FROMLEFT ) ||
//       ( pcfds->bSingle      ) )     // single ONLY copies form LEFT to RIGHT
   if( pcfds->bSingle )     // single ONLY copies from LEFT to RIGHT
   {
      if( dir_endcopy(cl->left) > nFails )
         nFails = dir_endcopy(cl->left);
   }
   else
   {
      if( dir_endcopy(cl->right) > nFails )
         nFails = dir_endcopy(cl->right);
   }

   if( !bbrk )
      db += GetETime(it);

   if( nFails > 0 )
   {
      sprintf(lpb, LoadRcString(IDS_COPY_FAILED), nFails);
      sprintf(EndBuf(lpb), MEOR"Dest: [%s]", pcfds->cf_szDest );
      if( nFiles )
      {
         sprintf(EndBuf(lpb),
            MEOR"But is appears %d copied successfully",
            nFiles );
         if( nSkips )
            sprintf(EndBuf(lpb), MEOR"Skipped %d files", nSkips );
         sprintf(EndBuf(lpb), MEOR"in %s seconds (tm=%d)",
            Dbl2Str(db, 5), it );
         bRet = TRUE;   // appears there were some successful copies
      }
      else
      {
         strcat(lpb, MEOR"and there were NO successful copies");
         if( nSkips )
            sprintf(EndBuf(lpb), MEOR"Skipped %d files", nSkips );
      }
      //pcfds->bVerify = TRUE;
      pcfds->dwVerFlag |= REVIEW_LIST;
   }
   else
   {
      //if( pcfds->bVerify )
      //if( pcfds->dwVerFlag & REVIEW_LIST )
      {
         PLARGE_INTEGER pli;
         LARGE_INTEGER  lit;
         LPTSTR         pb = GetStgBuf();

         lit.QuadPart = 0;

         sprintf(lpb, LoadRcString(IDS_COPY_COMPLETE), nFiles);

         sprintf(EndBuf(lpb), MEOR"Dest: [%s]", pcfds->cf_szDest );

         if( nSkips )
            sprintf(EndBuf(lpb), MEOR"Skipped %d files", nSkips );

         sprintf(EndBuf(lpb), MEOR"Action took %s secs (tm=%d)",
            Dbl2Str(db, 5 ), it );
         //if( strcmpi( gszCopyTo, pdst ) ) // dlg_root
         //{
         //   strcpy( gszCopyTo, pdst ); // dlg_root;
         //   bChgCT = TRUE;
         //}

         // get TRANSFERS to garbage bin cache
         pli = &pcfds->cf_liToBin;
         if( pli->QuadPart )
         {
            sprintf(EndBuf(lpb), MEOR"Transferred %s to *GARBAGE BIN*",
               GetI64StgRLen2( pli, 5 ) );
            lit.QuadPart += pcfds->cf_liToBin.QuadPart;  // add to TOTAL disk xfers
         }

         // get COPYING DONE
         pli = &pcfds->cf_liCopied;
         lit.QuadPart += pli->QuadPart;
         if( lit.QuadPart > 1024 )
         {
            SetBPS( pb, (double)lit.QuadPart, db );
            sprintf(EndBuf(lpb), MEOR"Disk Transfer rate (appx.) %s.", pb );
         }

         pli = &pcfds->cf_liDeleted;
         if( pli->QuadPart )
         {
            sprintf(EndBuf(lpb), MEOR"Deleted some %s of date.",
               GetI64StgRLen2( pli, 5 ) );
         }

      }

      bRet = TRUE;   // made fully successful copy
   }

   TickCount = GetTickCount()-TickCount;

   //if( pcfds->bVerify )
   //if( pcfds->dwVerFlag & REVIEW_LIST )
   {
      MB(hwndClient, lpb,
         APPNAME,
         MB_OK|MB_ICONINFORMATION);
   }

   if( bRet ) // if SUCCESS, and NO AND ( !pcfds->bSingle ) )
   {
      if( dwo != dwb )  // options have CHANGED
      {
         gdwCpyOpts = dwo;
         bChgCO     = TRUE;       // set change for INI update
      }
      //if( pcfds->bVerify != gbVerify )
      //{
      //   gbVerify = pcfds->bVerify;
      //   bChgVer  = TRUE;
      //}
      if( pcfds->dwVerFlag != gdwVerFlag )
      {
         gdwVerFlag = pcfds->dwVerFlag;
         bChgVF     = TRUE;
      }

      if( strcmpi( gszCopyTo, pdst ) ) // dlg_root
      {
         strcpy( gszCopyTo, pdst ); // dlg_root;
         bChgCT = TRUE;
      }
   }

   *lpb = 0;
   SetStatus(lpb);

   return bRet;

} /* complist_copyfiles */


/***************************************************************************
 * Function: complist_match
 *
 * Purpose:
 *
 * Matches up two lists of filenames
 *
 * Commentsz:
 *
 * We can find out from the DIRLIST handle whether the original list
 * was a file or a directory name.
 * If the user typed:
 *      two file names  - match these two item even if the names differ
 *
 *      two dirs        - match only those items whose names match
 *
 *      one file and one dir
 *                      - try to find a file of that name in the dir.
 *
 * This function returns TRUE if the complist_match was ok, or FALSE if it was
 * aborted in some way.
 *
 * In the current application fExact is ALWAYS TRUE!
 *
 ***************************************************************************/
BOOL
complist_match(COMPLIST cl, PVIEW view, BOOL fDeep, BOOL fExact )
{
   BOOL  bRet = FALSE;

   LPSTR lname;
   LPSTR rname;
   DIRITEM leftitem, rightitem;
   int cmpvalue;

   TickCount = GetTickCount();

   complist_count = 0;   // start a ROW counter

#ifdef ADD_LIST_VIEW
   // which is UPDATED in view_newitem
   CLEARLISTVIEW;
#endif // #ifdef ADD_LIST_VIEW

   if( dir_isfile(cl->left) )
   {
      if( dir_isfile(cl->right) )
      {
         /* two files */
         /* there should be one item in each list - make
          * a compitem by matching these two and append it to the
          * list
          */

         compitem_new( dir_firstitem(cl->left),
                       dir_firstitem(cl->right),
                       cl->items,      // LIST to be added to +++
                       TT_BOTH);
//                                  dir_firstitem(cl->right), cl->items, fExact);

         view_newitem(view);

         bRet = TRUE;

         goto Exit_Match;

      }

      /* left is file, right is dir */
      leftitem  = dir_firstitem(cl->left);
      rightitem = dir_firstitem(cl->right);
      lname = dir_getrelname(leftitem);
      while( rightitem != NULL )
      {
         rname = dir_getrelname(rightitem);
         cmpvalue = strcmpi(lname, rname);
         dir_freerelname(rightitem, rname);

         if( cmpvalue == 0 )
         {
            /* this is the match */
//            compitem_new(leftitem, rightitem, cl->items, fExact);
            compitem_new(leftitem, rightitem, cl->items, TT_BOTH);

            view_newitem(view);

            dir_freerelname(leftitem, lname);

            bRet = TRUE;

            goto Exit_Match;

         }

         rightitem = dir_nextitem(cl->right, rightitem, fDeep);

      }

      /* not found */
      dir_freerelname(leftitem, lname);
//      compitem_new(leftitem, NULL, cl->items, fExact);
      compitem_new(leftitem, NULL, cl->items, TT_LEFT);
      view_newitem(view);
      bRet = TRUE;
      goto Exit_Match;

   }
   else if( dir_isfile(cl->right) )
   {
      /* left is dir, right is file */
      /* loop through the left dir, looking for
       * a file that has the same name as rightitem
       */

      leftitem  = dir_firstitem(cl->left);
      rightitem = dir_firstitem(cl->right);
      rname = dir_getrelname(rightitem);
      while( leftitem != NULL )
      {
         lname = dir_getrelname(leftitem);
         cmpvalue = lstrcmpi(lname, rname);
         dir_freerelname(leftitem, lname);

         if( cmpvalue == 0 )
         {
            /* this is THE match */
//            compitem_new(leftitem, rightitem, cl->items, fExact);
            compitem_new(leftitem, rightitem, cl->items, TT_BOTH);
            view_newitem(view);

            dir_freerelname(rightitem, rname);

            bRet = TRUE;
            goto Exit_Match;
         }

         leftitem = dir_nextitem(cl->left, leftitem, fDeep);
      }

      /* not found */
      dir_freerelname(rightitem, rname);
//      compitem_new(NULL, rightitem, cl->items, fExact);
      compitem_new(NULL, rightitem, cl->items, TT_RIGHT);
      view_newitem(view);
      bRet = TRUE;
      goto Exit_Match;
   }

   /* two directories */
   /* traverse the two lists in parallel comparing the relative names */
   // OR ZIP FILES, which are TREATED like DIRECTORIES, since in a way
   // that is what they are !!!
   // ****************************************************************

   leftitem  = dir_firstitem(cl->left);
   rightitem = dir_firstitem(cl->right);

   while( (leftitem != NULL) && (rightitem != NULL) )
   {

           lname = dir_getrelname(leftitem);
           rname = dir_getrelname(rightitem);
#ifdef   DBGCMPITEMS
           sprtf( "Matching [%s] with [%s] fDeep=%s"MEOR, lname, rname,
              (fDeep ? "True" : "False") );
#endif   // DBGCMPITEMS
           //cmpvalue = utils_CompPath(lname, rname);
           cmpvalue = utils_CompPath2(lname, rname);

           dir_freerelname(leftitem, lname);
           dir_freerelname(rightitem, rname);

           if( cmpvalue == 0 )
           {
//                   compitem_new(leftitem, rightitem, cl->items, fExact);
                   compitem_new(leftitem, rightitem, cl->items, TT_BOTH);
                   if( view_newitem(view) )
                   {
                      goto Exit_Match;
                   }

                   // get new LEFT and RIGHT - this can result in a SCAN
                   leftitem  = dir_nextitem(cl->left, leftitem, fDeep);
                   rightitem = dir_nextitem(cl->right, rightitem, fDeep);

           }
           else if( cmpvalue < 0 )
           {
//                   compitem_new(leftitem, NULL, cl->items, fExact);
                   compitem_new(leftitem, NULL, cl->items, TT_LEFT);
                   if( view_newitem(view) )
                   {
                      goto Exit_Match;
                   }

                   // get new leftitem
                   leftitem = dir_nextitem(cl->left, leftitem, fDeep);
           }
           else
           {
//                   compitem_new(NULL, rightitem, cl->items, fExact);
                   compitem_new(NULL, rightitem, cl->items, TT_RIGHT);
                   if( view_newitem(view) )
                   {
                      goto Exit_Match;
                   }

                   // get new right
                   rightitem = dir_nextitem(cl->right, rightitem, fDeep);
           }
   }

   /* any left over are unmatched */
   while( leftitem != NULL )
   {
//           compitem_new(leftitem, NULL, cl->items, fExact);
           compitem_new(leftitem, NULL, cl->items, TT_LEFT);
           if( view_newitem(view) )
           {
              goto Exit_Match;
           }
           leftitem = dir_nextitem(cl->left, leftitem, fDeep);
   }

   while( rightitem != NULL )
   {
//           compitem_new(NULL, rightitem, cl->items, fExact);
           compitem_new(NULL, rightitem, cl->items, TT_RIGHT);
           if( view_newitem(view) )
           {
              goto Exit_Match;
           }
           rightitem = dir_nextitem(cl->right, rightitem, fDeep);
   }

   bRet = TRUE;

Exit_Match:

   TickCount = GetTickCount() - TickCount;

   return bRet;

} /* complist_match */

/* return time last operation took in milliseconds */
DWORD complist_querytime(void)
{       return TickCount;
}


/***************************************************************************
 * Function: complist_new
 *
 * Purpose:
 *
 * Allocates a new complist and initialise it 
 *
 **************************************************************************/
COMPLIST complist_new(void)
{
   COMPLIST cl;

   cl = (COMPLIST) gmem_get(hHeap, sizeof(struct complist), "complist_new" );
   cl->left  = NULL;
   cl->right = NULL;
   cl->items = List_Create();

   return cl;

} /* complist_new */


/***************************************************************************
 * Function: complist_open
 *
 * Purpose:
 *      
 * Puts up dialog asking the user to select an existing file to open.
 *
 * Parameters:
 *
 *      prompt - message to user indicating purpose of file
 *               (to be displayed somewhere in dialog box.
 *
 *      pext   - default file extension if user enters file without
 *               extension.
 *
 *      pspec  - default file spec (eg *.*)
 *
 *      osp    - OFSTRUCT representing file, if successfully open.
 *
 *      pfn    - buffer where filename (just final element) is returned.
 *
 * Returns:
 *
 * TRUE - if file selected and exists (tested with OF_EXIST).
 *
 * FALSE - if dialog cancelled. If user selects a file that we cannot
 *           open, we complain and restart the dialog.
 *
 * Comments:
 *
 *           if TRUE is returned, the file will have been successfully opened,
 *           for reading and then closed again.
 *           THe fully qualified PATH/FILE is returned in osp->szPathName.
 *
 **************************************************************************/

BOOL FAR PASCAL
complist_open(LPSTR prompt, LPSTR pext, LPSTR pspec, OFSTRUCT FAR *osp, LPSTR pfn)
{
    OPENFILENAME * pofn = &g_sofn;
    LPSTR chp;
    int fh;

    /* build filter-pair buffer to contain one pair - the spec filter,
     * twice (one of the pair should be the filter, the second should be
     * the title of the filter - we don't have a title so we use the
     * filter both times. remember double null at end of list of strings.
     */
    strcpy(gachFilters, pspec);             // filter + null
    chp = &gachFilters[strlen(gachFilters)+1];      //2nd string just after null
    strcpy(chp, pspec);                    // filter name (+null)
    chp[strlen(chp)+1] = '\0';            // double null at end of list
    /*
     * initialise arguments to dialog proc
     */
    pofn->lStructSize = sizeof(OPENFILENAME);
    pofn->hwndOwner = NULL;
    pofn->hInstance = NULL;
    pofn->lpstrFilter = gachFilters;
    pofn->lpstrCustomFilter = (LPSTR)NULL;
    pofn->nMaxCustFilter = 0L;
    pofn->nFilterIndex = 1L;              // first filter pair in list
    gachPath[0] = 0;                // start the PATH as zero
    pofn->lpstrFile = gachPath;     // we need to get the full path to open
    pofn->nMaxFile = 256;           // sizeof(achPath);
    pofn->lpstrFileTitle = pfn;     // return final elem of name here
    pofn->nMaxFileTitle = 256;      // sizeof(fn);
    pofn->lpstrInitialDir = NULL;
    pofn->lpstrTitle = prompt;      // dialog title is good place for prompt text
    pofn->Flags = OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST;
    pofn->lpstrDefExt = pext;
    pofn->nFileOffset = 0;
    pofn->nFileExtension = 0;
    pofn->lCustData = 0;

    /*
     * loop until the user cancels, or selects a file that we can open
     */
    do {
        if( !GetOpenFileName(pofn) )
        {
            return(FALSE);
        }
        // if this is SUCCESSFUL, then osp->szPathName[]
        // contains the fully qualified PATH returned to caller.
        fh = OpenFile(gachPath, osp, OF_READ);
        if( fh == HFILE_ERROR )
        {
            if( MB(NULL,
               LoadRcString(IDS_COULDNT_BE_OPENED),
               LoadRcString(IDS_FILEOPEN),
               MB_OKCANCEL|MB_ICONSTOP) == IDCANCEL)
            {
                return(FALSE);
            }
        }
    } while (fh == HFILE_ERROR);

    _lclose(fh);

    return(TRUE);
}

/***************************************************************************
 * Function: complist_getroot_left
 *
 * Purpose:
 *
 * Gets the root names of the left tree used to build this complist.
 *
 **************************************************************************/
LPSTR
complist_getroot_left(COMPLIST cl)
{
        return( dir_getroot_list(cl->left));
}

/***************************************************************************
 * Function: complist_getroot_right
 *
 * Purpose:
 *
 * Gets the root names of the right tree used to build this complist.
 *
 **************************************************************************/
LPSTR
complist_getroot_right(COMPLIST cl)
{
        return( dir_getroot_list(cl->right));
}
/***************************************************************************
 * Function: complist_freeroot_*
 *
 * Purpose:
 *
 * Frees up memory allocated in a call to complist_getroot*() 
 *
 **************************************************************************/
void
complist_freeroot_left(COMPLIST cl, LPSTR path)
{
        dir_freeroot_list(cl->left, path);
}

void
complist_freeroot_right(COMPLIST cl, LPSTR path)
{
        dir_freeroot_list(cl->right, path);
}


/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

// NEW functions
/***************************************************************************
 * Function: complist_isfilee
 *
 * Purpose: Check the left and the right are FILES,
 *          NOT directories
 *
 * struct complist {
 *      DIRLIST left;           -- left list of files --
 *      DIRLIST right;          -- right list of files --
 *      LIST items;             -- list of COMPITEMs --
 * } * COMPLIST;
 *
 ***************************************************************************/
BOOL
complist_isfiles( COMPLIST cl )
{
   BOOL  bRet = FALSE;
   if( ( cl ) &&
       ( dir_isfile( cl->left  ) ) &&
       ( dir_isfile( cl->right ) ) )
   {
      bRet = TRUE;
   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_setpcfds
// Return type: VOID 
// Arguments  : PVOID pcfds
//            : INT state
//            : DWORD dwFlag
// Description: Set the member of the STATS structure according the the STATE
//              of the COMPLIST item.
///////////////////////////////////////////////////////////////////////////////
VOID  complist_setpcfds( PVOID pv, INT state, DWORD dwFlag )
{
   PCFDLGSTR pcfds = (PCFDLGSTR)pv;
   if(pcfds)
   {
      if( state == STATE_SAME )
      {
         pcfds->dwSame++;   // bump SAME
         pcfds->dwTLeft++;  // and TWO totals
         pcfds->dwTRite++;
      }
      else if( state == STATE_DIFFER )
      {
         pcfds->dwDiff++;   // bump DIFF

         if( dwFlag & TT_YOUNGER )
            pcfds->dwNewer++;

         pcfds->dwTLeft++;  // and TWO totals
         pcfds->dwTRite++;
      }
      else if( state == STATE_FILELEFTONLY )
      {
         pcfds->dwLeft++;   // bump LEFT only
         pcfds->dwTLeft++;  // and TOTAL left
      }
      else if( state == STATE_FILERIGHTONLY )
      {
         pcfds->dwRite++;   // bump RIGHT only
         pcfds->dwTRite++;  // and RIGHT total
      }
      else
      {
         pcfds->dwUnk++;    // AWK! Bump NO STATE FLAG - THIS WOULD BE AN ERROR
      }

      // FIX20081125 - NEW completely IGNORE file time, but still put in FLAG
      if( dwFlag & TT_DATEOLD )
         pcfds->dwNFTOlder++;
      else if( dwFlag && TT_DATENEW )
         pcfds->dwNFTNewer++;
      else
         pcfds->dwNFTSame++;

   }
}

// *** TBD *** These are to be completely replaced by
// the defined -
// g_ListTotal - total in 'combined' file list
// g_LeftCnt   - total in left tree
// g_RightCnt  - Total in right tree
// g_SameCnt   - if folders are equal these 4 total would be the SAME

// g_NewerCnt  - if date newer - ie ready for update
// g_OlderCnt  - Maybe destination ALSO modified
// g_LeftOnly  - files ONLY in left tree
// g_RightOnly - files ONLY in right tree
VOID  clear_pcfdsstats( PCFDLGSTR pcfds )
{
   if( VALIDPCFDS(pcfds) )
   {
      pcfds->dwSame  = 0;
      pcfds->dwTLeft = 0;
      pcfds->dwTRite = 0;
      pcfds->dwDiff  = 0;
      pcfds->dwNewer = 0;
      pcfds->dwLeft  = 0;
      pcfds->dwRite  = 0;
      pcfds->dwUnk   = 0;
      // FIX20081125 - NEW completely IGNORE file time,
      // but INFO still put in FLAG ci->ci_dwFlag
      // SEE gbIgnDT2
      pcfds->dwNFTNewer = 0;
      pcfds->dwNFTOlder = 0;
      pcfds->dwNFTSame  = 0;
   }
}

DWORD    g_dwCL_ID = 0;
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_getstats
// Return type: VOID 
// Arguments  : PCFDLGSTR pcfds
//            : COMPLIST cl
// Description: Traverse a COMPLIST gathering statisitc information
//              into the passed structure.
///////////////////////////////////////////////////////////////////////////////
VOID  complist_getstats( PCFDLGSTR pcfds, COMPLIST cl )
{
   if( !VALIDPCFDS(pcfds) )
      return;

   if( g_dwCL_ID != cl->cl_id )
   {
      COMPITEM ci;

      g_dwCL_ID = cl->cl_id;  // set the OWNER of the stats

      clear_pcfdsstats( pcfds );

      List_TRAVERSE(cl->items, ci)
      {
         /* check if files of this type are to be copied */
         complist_setpcfds( pcfds, compitem_getstate(ci), compitem_getflag(ci) );
      } /* traverse */
   }
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_setdlgstgs
// Return type: VOID 
// Argument   : PCFDLGSTR pcfds
//            : COMPLIST cl
// Description: Copy the current ROOT items to BOTH copy_leftname, and
//              copy_rightname, as well as to dialog_leftname and
// dialog_rightname.
///////////////////////////////////////////////////////////////////////////////
VOID
complist_setdlgstgs( PVOID pv, COMPLIST cl )
{
   PCFDLGSTR pcfds = (PCFDLGSTR)pv;
   gszLeftName[0] = 0;
   gszRightName[0] = 0;

   if(( VALIDPCFDS(pcfds) ) &&
      (cl                 ) )
   {
      LPTSTR   pstr;
      LPTSTR   lpf;
      /* store the left and right rootnames so that dodlg_savelist
       * can display them in the dialog.
       */
      lpf = &pcfds->cf_szLeft[0];
      *lpf = 0;
      pstr = dir_getroot_list(cl->left);
      if(pstr)
      {
         //strcpy(dialog_leftname, pstr);
         strcpy(dlg_leftname, pstr);
         strcpy(lpf,             pstr);
         strcpy(gszLeftName,     pstr);
      }
      dir_freeroot_list(cl->left, pstr);

      lpf = &pcfds->cf_szRight[0];
      *lpf = 0;
      pstr = dir_getroot_list(cl->right);
      if(pstr)
      {
         //strcpy(dialog_rightname, pstr);
         strcpy(dlg_rightname, pstr);
         strcpy(lpf,              pstr);
         strcpy(gszRightName,     pstr);
      }
      dir_freeroot_list(cl->right, pstr);

      sprtf( "CMP=[%s : %s]"MEOR, dlg_leftname, dlg_rightname );

   }
}

static TCHAR   sszFC[] = "(%d)";

LSTSTATS sLstStats;
#define  sls   sLstStats

#define  _s_dwTotal  sls.dwtot
#define  _s_dwSame   sls.dwsame
#define  _s_dwNewer  sls.dwnewer
#define  _s_dwOlder  sls.dwolder
#define  _s_dwLeft   sls.dwleft
#define  _s_dwRight  sls.dwright

#define  SDL2( a, b ) { \
   sprintf(lpb,sszFC,a);\
   SetDlgItemText(hDlg,b,lpb);\
}

VOID  MyCommonDlgResults( HWND hDlg )
{
   TCHAR buf[128];
   LPTSTR   lpb = &buf[0];
   PLSTSTATS   pls = &sLstStats;

//   SDL( szLstTot, view_gettotcount( current_view ), IDC_LISTTOTAL );
   SDL2( pls->dwtot, IDC_LISTTOTAL2 );

   SDL2( _s_dwSame, IDC_LABIDENT2 );

   // set DIALOG labels
   //SDL( szFilCnt, pcfds->dwTLeft, IDC_LABLEFT );
   //SDL( szFilCnt, pcfds->dwTRite, IDC_LABRITE );
   SDL2( _s_dwLeft, IDC_LABLEFT2 );    // Left tree only
   SDL2( _s_dwRight, IDC_LABRITE2 );    // right tree only
   SDL2( pls->dwdeleted, IDC_LABDEL2 );   // set the 'deleted' count
   // COPY FILES INIT
//   sprintf(lpb, szFilCnt, pcfds->dwDiff );
//   SetDlgItemText( hDlg, IDC_LABDIFF, lpb );
//   SDL( szFilCnt, g_dwNewer, IDC_LABDIFF );   // newer
//   SDL( szFilCnt, (g_dwDiff - g_dwNewer), IDC_LABDIFF2 );  // older
   SDL2( pls->dwnewer, IDC_LABDIFF3 );   // newer
   SDL2( pls->dwolder, IDC_LABDIFF4 );  // older

   *lpb = 0;
//   if( g_dwUnk )
//      sprintf(lpb, szFilCnt, g_dwUnk );
   if( pls->dwunk )
      sprintf(lpb, "%d UNK!", pls->dwunk );
//   SetDlgItemText( hDlg, IDC_LABUNK2, lpb );

   //if(bchg)
   //if( !g_bHadPost )
//   {
//      Set_Action_Line( hDlg, 0 );
      //g_bHadPost = TRUE;
//   }
}


#define	UseComCtrls(a)	\
{\
	INITCOMMONCONTROLSEX _iccex;\
	_iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);\
	_iccex.dwICC = a;\
	InitCommonControlsEx(&_iccex);\
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_cleanup
// Return type: VOID 
// Argument   : VOID
// Description: When the COMPLIST is being delete, also ensure any memory
//              associated with this DOUBLE LINKED list is also freed.
///////////////////////////////////////////////////////////////////////////////
VOID
complist_cleanup( VOID )
{
   KillLList( &gsCopyList );
}


INT
complist_savecount(PVIEW view, UINT options)
{
   LIST     li;
   INT      i, state;
   COMPITEM ci;

   if( !view )
      return 0;

   li = complist_getitems(view_getcomplist(view));
   ci = (COMPITEM) List_First(li);
   for( i = 0; ci != NULL; ci = (COMPITEM) List_Next(ci) )
   {
      state = compitem_getstate(ci);
      if( ( (options & INCLUDE_SAME) && (state == STATE_SAME)               ) ||
          ( (options & INCLUDE_DIFFER) && (state == STATE_DIFFER)           ) ||
          ( (options & INCLUDE_LEFTONLY) && (state == STATE_FILELEFTONLY)   ) ||
          ( (options & INCLUDE_RIGHTONLY) && (state == STATE_FILERIGHTONLY) ) )
      {
         i++;  // count a 'displayable', in this case 'writeable' item
      }
   }
   return i;
}

VOID  AppendCopyOpts( LPTSTR lpb, DWORD dwo )
{
   DWORD bRet = 0;

   if( dwo == 0 )
   {
      strcat(lpb, "<nul!>");
      return;
   }

   if( dwo & INCLUDE_SAME )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"Same");
      bRet++;
   }
   if( dwo & INCLUDE_DIFFER )
   {
//      if(bRet)
//         strcat(lpb,"|");
//      strcat(lpb,"Differ");
//      bRet++;

      if( dwo & INCLUDE_NEWER )
      {
         if(bRet)
            strcat(lpb,"|");
         strcat(lpb,"Newer");
         bRet++;
      }

      if( dwo & INCLUDE_OLDER )
      {
         if(bRet)
            strcat(lpb,"|");
         strcat(lpb,"Older");
         bRet++;
      }
   }
   if( dwo & INCLUDE_LEFTONLY )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"LeftO");
      bRet++;
   }
   if( dwo & INCLUDE_RIGHTONLY )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"RightO");
      bRet++;
   }

   if( dwo & INCLUDE_MOVELEFT )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"MLeft");
      bRet++;
   }
   if( dwo & INCLUDE_MOVERIGHT )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"MRight");
      bRet++;
   }
// and a line number option
   if( dwo & INCLUDE_LINENUMS )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"LNums");
      bRet++;
   }
   if( dwo & INCLUDE_TAGS )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"Tags");
      bRet++;
   }
   if( dwo & APPEND_FILE )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"Append");
      bRet++;
   }
   if( dwo & INCLUDE_HEADER )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"Header");
      bRet++;
   }

/* Copy file options are either COPY_FROMLEFT or COPY_FROMRIGHT 
 * (indicating which  * tree is to be the source of the files), plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT (INCLUDE_LEFT
 * and INCLUDE_RIGHT are treated the same here since the COPY_FROM* option
 * indicates which side to copy from). *** DISCONTINUED *** 
   if( dwo & COPY_FROMLEFT )  // copy files from left tree to right tree
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"CLeft");
      bRet++;
   }
   if( dwo & COPY_FROMRIGHT )
   {
      if(bRet)
         strcat(lpb,"|");
      strcat(lpb,"CRight");
      bRet++;
   }
  * **** */

}

VOID  AppendCopyOptLetters( LPTSTR lpb, DWORD dwo )
{
   DWORD bRet = 0;

   if( dwo == 0 )
   {
      strcat(lpb, "<0!>");
      return;
   }

   if( dwo & INCLUDE_SAME )
      strcat(lpb,"S");

   if( dwo & INCLUDE_DIFFER )
   {
      strcat(lpb,"D");
      if( dwo & INCLUDE_NEWER )
         strcat(lpb,"n");
      if( dwo & INCLUDE_OLDER )
         strcat(lpb,"o");
   }

   if( dwo & INCLUDE_LEFTONLY )
      strcat(lpb,"L");

   if( dwo & INCLUDE_RIGHTONLY )
      strcat(lpb,"R");

   if( dwo & INCLUDE_MOVELEFT )
      strcat(lpb,"Ml");

   if( dwo & INCLUDE_MOVERIGHT )
      strcat(lpb,"Mr");

// and a line number option
   if( dwo & INCLUDE_LINENUMS )
      strcat(lpb,"Ln");

   if( dwo & INCLUDE_TAGS )
      strcat(lpb,"T");

   if( dwo & APPEND_FILE )
      strcat(lpb,"A");

   if( dwo & INCLUDE_HEADER )
      strcat(lpb,"H");

/* Copy file options are either COPY_FROMLEFT or COPY_FROMRIGHT 
 * (indicating which  * tree is to be the source of the files), plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT (INCLUDE_LEFT
 * and INCLUDE_RIGHT are treated the same here since the COPY_FROM* option
 * indicates which side to copy from). *** DISCONTINUED ***
   if( dwo & COPY_FROMLEFT )  // copy files from left tree
   {
      strcat(lpb,"CL");
   }
   if( dwo & COPY_FROMRIGHT )
   {
      strcat(lpb,"CR");
   }
 * ************** */

}


//like  complist_savelist( COMPLIST cl, LPTSTR p )
//   COMPLIST    cl = view_getcomplist(current_view);
int   complist_writelist( COMPLIST cl, LPTSTR poutfile, DWORD dwo,
                         BOOL bSetGlob, BOOL bDoMB )
{
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;
   LPTSTR      lpb  = &g_szBuf1[0];
   LPTSTR      lpb2 = &g_szBuf2[0];
   HANDLE      fh;
   DWORD       dwi, dww, dwr;
   BOOL        bOK;
   INT         icnt, icntn, state;
   COMPITEM    ci;
   DWORD       dwFlag;
//   COMPLIST    cl = view_getcomplist(current_view);
//   LPTSTR      p;

   sprtf( "Writing LIST to [%s]"MEOR, poutfile );
   sprtf( "Save Opts [%s]"MEOR, SaveOpts2Stg(dwo, TRUE) );

   /* try to open the file */
   //fh = OpenFile(savename, &os, OF_CREATE|OF_READWRITE|OF_SHARE_DENY_WRITE);
   fh = OpnFil( poutfile, ( ( dwo & APPEND_FILE ) ? TRUE : FALSE ) );
   if( !VFH(fh) )
   {
      if( bDoMB )
      {
         sprintf(lpb, LoadRcString(IDS_CANT_OPEN), poutfile);
         MB(NULL, lpb, APPNAME, MB_ICONSTOP|MB_OK);
      }
      return 0;
   }

   //hcurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

   bOK = TRUE;
   dwr = 0;

   /* write out the header lines */
   if( dwo & INCLUDE_HEADER )
   {
      //LPTSTR   pb1, pb2, pb3, pb4;
      LPTSTR   lhead, rhead;
      //pb1 = lpb2;
      //pb2 = &pb1[32];
      //pb3 = &pb2[32];
      //pb4 = &pb3[32];
      lhead = dir_getroot_list(cl->left);
      rhead = dir_getroot_list(cl->right);

      if( dwo & ADD_X_HDR )   // multi-lined version
      {
         sprintf(lpb,
            "; In the compare of:"MEOR
            "; [%s] with"MEOR
            "; [%s]"MEOR
            "; with options [%s]."MEOR,
            lhead,
            rhead,
            SaveOpts2Stg( dwo, TRUE ) );
      }
      else
      {
         // compact, but use a max. wrap width of about 75
         if( ( strlen(lhead) + 20 ) > 75 )
         {
            // compacting is almost impossible, so no try (yet)
            //sprintf(lpb,
            //   "; In the compare of:"MEOR
            //   "; [%s] with"MEOR,
            //   lhead );
            sprintf(lpb,
               "; In the compare of:"MEOR
               "; [%s] with"MEOR
               "; [%s]"MEOR
               "; with options [%s]."MEOR,
               lhead,
               rhead,
               SaveOpts2Stg( dwo, TRUE ) );
         }
         else
         {
            sprintf(lpb,
               "; In the compare of:[%s] with ",
               lhead );
            if( ( strlen(lpb) + strlen(rhead) ) < 75 )
            {
               //LPTSTR   lpo = SaveOpts2Stg( dwo, TRUE );
               LPTSTR   lpo = SaveOpts2Stg( dwo, FALSE );   // get SHORT 'command line' vers
               strcat(lpb,rhead);
               strcat(lpb, " " );
               //  12345678901234
               // "with options [%s]"MEOR, lpo );
               //if( ( strlen(lpb) + strlen(lpo) + 14 ) < 75 )
               //   sprintf(EndBuf(lpb), "with options [%s]"MEOR, lpo );
               //else
               //   sprintf(EndBuf(lpb), MEOR"; with options [%s]"MEOR, lpo );
               //  12345678901234
               // "with [-S%s]"MEOR, lpo );
               if( ( strlen(lpb) + strlen(lpo) + 8 ) < 75 )
                  sprintf(EndBuf(lpb), "with [-S:%s]"MEOR, lpo );
               else
                  sprintf(EndBuf(lpb), MEOR"; with [-S:%s]"MEOR, lpo );

            }
            else
            {
               sprintf(EndBuf(lpb), MEOR"; [%s]"MEOR
                  "; with options [%s]."MEOR,
                  rhead,
                  SaveOpts2Stg( dwo, TRUE ) );
            }
         }
      }

      dir_freeroot_list(cl->left, lhead);
      dir_freeroot_list(cl->right, rhead);

      OUTBUF(lpb);

   }

   /* traverse the list of compitems looking for the
    * ones we are supposed to include
    * 18 April, 2001 - Traverse 8 times, writting the respective files,
    * after putting a HEADER with respective COUNT
    *
    */
   if( dlg_order )
   {
      // ORDER the output of file names to the LIST
      if( dwo & INCLUDE_SAME )
      {
         // include if files are the same
         icnt = 0;   // there is a GLOBAL value for this
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_SAME )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Identical file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Identical file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_SAME )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do IDENTICAL

      if( dwo & INCLUDE_DIFFER )
      {
         // *** TBD *** This need to correctly obey the INCLUDE_YOUNGER
         // and INCLUDE_OLDER flags, rather than just ALL DIFFERENCES
         // include if different files to be output to list
         icnt = icntn = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            dwFlag = compitem_getflag(ci);   // get COMPARE flag
            if( state == STATE_DIFFER )
            {
               //dwFlag = compitem_getflag(ci);   // get COMPARE flag
               icnt++;
               if( dwFlag & TT_YOUNGER )
               {
                  icntn++;
               //if( lg < 0 )   // Left/First file time is less than Right/second file time.
               //   ci->ci_dwFlag |= TT_OLDER;    // left is older - local change/update
               //else
               //   ci->ci_dwFlag |= TT_YOUNGER;    // left is newer - suggests update
               }
            }
            else if( ( state == STATE_SAME ) && ( dwFlag & TT_DATEDIFF ) )
            {
               icnt++;
               if( dwFlag & TT_YOUNGER )
               {
                  icntn++;
               //if( lg < 0 )   // Left/First file time is less than Right/second file time.
               //   ci->ci_dwFlag |= TT_OLDER;    // left is older - local change/update
               //else
               //   ci->ci_dwFlag |= TT_YOUNGER;    // left is newer - suggests update
               }
            }
         }

         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Differing file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               //sprintf(lpb, "; Differing file count is %d."MEOR, icnt );
               sprintf(lpb, "; Differing file(s) is %d. Newer = %d."MEOR, icnt, icntn );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               dwFlag = compitem_getflag(ci);   // get COMPARE flag
               if( state == STATE_DIFFER )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
               else if( ( state == STATE_SAME ) && ( dwFlag & TT_DATEDIFF ) )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do DIFFER

      if( dwo & INCLUDE_LEFTONLY )
      {
         icnt = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_FILELEFTONLY )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Left only file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Left only file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }
            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_FILELEFTONLY )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do LEFTONLY

      if( dwo & INCLUDE_RIGHTONLY )
      {
         icnt = 0;
         List_TRAVERSE(cl->items, ci)
         {
            state = compitem_getstate(ci);
            if( state == STATE_FILERIGHTONLY )
               icnt++;
         }
         if(icnt == 0)
         {
            if( dwo & INCLUDE_HEADER )
            {
               strcpy(lpb, "; Right only file count is ZERO (0)!"MEOR );
               OUTBUF(lpb);
            }
         }
         else
         {
            if( dwo & INCLUDE_HEADER )
            {
               sprintf(lpb, "; Right only file count is %d."MEOR, icnt );
               OUTBUF(lpb);
            }

            List_TRAVERSE(cl->items, ci)
            {
               state = compitem_getstate(ci);
               if( state == STATE_FILERIGHTONLY )
               {
                  nFiles++;
                  /* output the list line */
                  OUTLISTLINE;
               }
            }   // traverse list
         }   // has count or not
      }   // do LEFTONLY
   }
   else
   {
      // NOT in dialog order
      // Just as they occur in the list, and are elegible for output
      List_TRAVERSE(cl->items, ci)
      {
              /* check if files of this type are to be listed */
              state = compitem_getstate(ci);
              dwFlag = compitem_getflag(ci);   // get COMPARE flag
              //if ((state == STATE_SAME) && ( !(dwo & INCLUDE_SAME) ) )
              if( state == STATE_SAME )
              {
                 if( !(dwo & INCLUDE_SAME) )
                 {
                    if( !(dwFlag & TT_DATEDIFF) )
                      continue;
                 }
              }
              else if ((state == STATE_DIFFER) && ( !(dwo & INCLUDE_DIFFER) ) )
                      continue;
              else if ((state == STATE_FILELEFTONLY) && ( !(dwo & INCLUDE_LEFTONLY) ) )
                      continue;
              else if ((state == STATE_FILERIGHTONLY) && ( !(dwo & INCLUDE_RIGHTONLY) ) )
                      continue;
              nFiles++;
              /* output the list line */
              OUTLISTLINE;
      }
   }

   /* write tail line */
   if( dwo & INCLUDE_HEADER )
   {
      sprintf(lpb, LoadRcString(IDS_FILES_LISTED), nFiles);

      OUTBUF(lpb);
   }

Save_End:

   /* - close file and we are finished */
   if( VFH(fh) )
      CloseHandle(fh);

   if( bOK )
   {
      if( bSetGlob ) // should mostly be FALSE
      {
         // since we are only just writing a list of file names
         if( dwo != gdwFileOpts )
         {
            gdwFileOpts = dwo;
            bChgFO = TRUE;    // changed options
            if( (outline_include & INC_OUTLINE2) != (dwo & INC_OUTLINE2) )
            {
               outline_include &= ~(INC_OUTLINE2);
               outline_include |= (dwo & INC_OUTLINE2);
               bChgInc = TRUE;
            }
            // *** REPAINT DISPLAY ACCORDING TO NEW display characteristics
            PostMessage( hwndClient, WM_COMMAND, IDM_REFRESH, 0 );
            // ************************************************************
         }
   
         Add2SList( &gsFileList, &bChgFLst, poutfile );
   
         if( strcmpi( poutfile, gszListFil ) )
         {
            strcpy(gszListFil, poutfile);
            bChgLF = TRUE;
         }
   
         g_bListSave++; //	W.ws_bListSave // g_ INT
         // user actions
         g_dwUseFlag |= uf_DnListOut;  // written out to a list file
      }
   }
   else  // if( ( savename == NULL ) || ( *savename == 0 ) )
   {
      if( bDoMB )
      {
         MB(NULL, "File WRITE ERROR!", APPNAME, MB_ICONSTOP|MB_OK);
      }
      nFiles = 0;
   }

   //SetCursor(hcurs);

   return nFiles; // return the number written

}  // complist_writelist( COMPLIST cl, LPTSTR poutfile, DWORD dwo )

// eof - complist.c
