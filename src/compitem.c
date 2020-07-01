
/****************************** Module Header *******************************
* Module Name: COMPITEM.C
*
* Module which does the comparison between two files. 
*
* Functions:
*
* ci_copytext()
* ci_makecomposite()
* ci_compare()
* ci_onesection()
* compitem_new()
* compitem_delete()
* compitem_discardsections()
* compitem_getcomposite()
* compitem_getleftsections()
* compitem_getrightsections()
* compitem_getleftfile()
* compitem_getrightfile()
* compitem_getstate()
* compitem_gettext_tag()
* compitem_gettext_result()
* compitem_getfilename()
* compitem_frefilename()
*
* Comments:
*
* This module uses the structure compitem which is a data type that knows
* about two files, and can compare them. The result of the comparison
* is a list of sections for each file, and a composite list of sections
* representing the comparison of the two files.
*
* A compitem has a state (one of the integer values defined in state.h)
* representing the result of the comparison. It can also be
* queried for the text result (text equivalent of the state) as well
* as the tag - or title for this compitem (usually a text string containing
* the name(s) of the files being compared).
*
* A compitem will supply a composite section list even if the files are
* the same, or if there is only one file. The composite section list will
* only be built (and the files read in) when the compitem_getcomposite()
* call is made (and not at compitem_new time).
* 
*  this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/

#include "dc4w.h"

extern   PVIEW  current_view;
#ifdef ADD_ZIP_SUPPORT
extern   LONG  CompFTZip( LONG lg, FILETIME * pft1, FILETIME * pft2 );
#endif // #ifdef ADD_ZIP_SUPPORT

#define  ADDLNWRAP      // until devel finished
#define  DBG_LOG1    // sprtf
// from file.c
//struct filedata {
//        DIRITEM diritem;        /* handle to file name information */
//        LIST lines;             /* NULL if lines not read in */
//};

//#define  TT_LEFT     0x00000001
//#define  TT_RIGHT    0x00000002
//#define  TT_BOTH     ( TT_LEFT | TT_RIGHT )
//#define  TT_SMALLER  0x00000080
//#define  TT_LARGER   0x00000040
//#define  TT_YOUNGER  0x00000020
//#define  TT_OLDER    0x00000010

struct compitem {

        FILEDATA left;          /* handle for left-hand file */
        FILEDATA right;         /* handle for right-hand file */

        LIST secs_composite;    /* list of sections (composite file)*/
        LIST secs_left;         /* list of sections (left file) */
        LIST secs_right;        /* list of sections (right file) */

        int    state;           /* compitem state - result of compare */
        DWORD  ci_dwFlag;  // per above flags
        DWORD  ci_Index;   // index in the LISTVIEW (if avail.)

        LONG   ci_lgTime;  // time compare results
        LONG   ci_lgSize;  // size compare results

        BOOL   bDiscard;        /* true if not alloc-ed on list */

        BOOL   bTagIsRel;       /* tag text is RELATIVE name */

        // some allocated/de-allocated pointers to strings
        LPTSTR ci_pTag;            /* text for tag (title of compitem) */
        LPTSTR ci_pResult; /* text equivalent of state and other information */

};


LPSTR ci_copytext(LPSTR in);
void  ci_makecomposite(COMPITEM ci);
void  ci_compare(COMPITEM ci);
INT   compitem_exact( COMPITEM ci );
BOOL  compitem_wrtdiff2name( LPTSTR fname, COMPITEM item, DWORD dwOpts, PVOID pv );
VOID compitem_retfullname( LPTSTR lpb, COMPITEM ci, DWORD dwo );

// note - gbSimple = FALSE;    // build my own 'compund' result string for display

BOOL  gbDateDiff = TRUE;   // make DATE difference different

#define  ADDSP1         " "
#define  DEF_IDENT_STG  "Identical"

#define  DEF_STS_STG    "<"DEF_IDENT_STG"> "

TCHAR szNODT[]   = "??/??/?? ??:??";
TCHAR szNoDSM[]  = "<DATE? but=size> ";
TCHAR szNoDDF[]  = "<size & date?> ";

TCHAR szSTS[] = "Same_State";
//TCHAR szSTSStg[32] = { DEF_STS_STG };

#ifdef   USEORGSTGS
TCHAR szSTSStg[] = "<"DEF_IDENT_STG"> ";
// if( lg < 0 )
TCHAR szOldSMX[] = "<SAME but Earlier> ";
TCHAR szNewSMX[] = "<SAME but Later  > ";
// if( lg < 0 )
TCHAR szOldDFX[] = "<Earlier DIFFERENT> ";
TCHAR szNewDFX[] = "<Later   DIFFERENT> ";
// if( lg < 0 )
TCHAR szOldSM[]  = "<EARLIER but=size> ";
TCHAR szNewDF[]  = "<LATER   but=size> ";
TCHAR szDSD[]    = "<size but=date> ";
// if( lg < 0 )   // left First file time is less than right second file time.
TCHAR szDOld[]   = "<size & earlier> ";
TCHAR szDNew[]   = "<size & later> ";
#else    // !USEORGSTGS
#ifdef   USEORGCONST
// Try a CONSTANT first item SIZE
TCHAR szSTSStg[] = "<"DEF_IDENT_STG">         ";
// if( lg < 0 )
TCHAR szOldSMX[] = "<SAME but Earlier>  ";
TCHAR szNewSMX[] = "<SAME but Later  >  ";
// if( lg < 0 )
TCHAR szOldDFX[] = "<Earlier DIFFERENT> ";
TCHAR szNewDFX[] = "<Later   DIFFERENT> ";
// if( lg < 0 )
TCHAR szOldSM[]  = "<EARLIER but=size>  ";
TCHAR szNewDF[]  = "<LATER   but=size>  ";
TCHAR szDSD[]    = "<size but=date>     ";
// if( lg < 0 )   // left First file time is less than right second file time.
TCHAR szDOld[]   = "<size & earlier>    ";
TCHAR szDNew[]   = "<size & later>      ";

#else // !#ifdef   USEORGCONST

// Try Newer/Older
TCHAR szSTSStg[]  = "<"DEF_IDENT_STG">     "ADDSP1;
TCHAR szSTSStg2[] = "<"DEF_IDENT_STG"?Zip> "ADDSP1;
// if( lg < 0 )
TCHAR szOldSMX[] = "<OLDER bxt SAME>"ADDSP1;
TCHAR szNewSMX[] = "<Newer bxt SAME>"ADDSP1;
// if( lg < 0 )
TCHAR szOldDFX[] = "<OLDER X SIZE>  "ADDSP1;
TCHAR szNewDFX[] = "<Newer X SIZE>  "ADDSP1;
// if( lg < 0 )
TCHAR szOldSM[]  = "<OLDER but=size>"ADDSP1;
TCHAR szNewDF[]  = "<Newer but=size>"ADDSP1;
TCHAR szDSD[]    = "<=date but=SIZE>"ADDSP1;
// if( lg < 0 )   // left First file time is less than right second file time.
TCHAR szDOld[]   = "<Older & size>  "ADDSP1;
TCHAR szDNew[]   = "<newer & size>  "ADDSP1;
#endif   // #ifdef   USEORGCONST y/n

TCHAR szOIL[]    = "<Only in Left>  "ADDSP1;
TCHAR szOIR[]    = "<Only in Right> "ADDSP1;
#endif   // #ifdef   USEORGSTGS y/n

//TCHAR g_sz10I64u[]   = "%10I64u ";  // sprintf using LARGE_INTEGER (64-bits)
TCHAR g_sz10I64u[]   = "%9I64u ";  // sprintf using LARGE_INTEGER (64-bits)

#ifdef ADD_LIST_VIEW

//#ifdef   ADDLISTVIEW2   // test out FULL addition to LISTVIEW
//#ifdef   ADDLVALL2
LRESULT compitem_addLV( COMPITEM ci )
{
   LPTSTR   lps[LV_DEF_COLS];
   DWORD    col;

   if( !ci ||
      ( !ci->left && !ci->right ) ||
      !g_hListView )
      return -1;

   for( col = 0; col < LV_DEF_COLS; col++ )
      lps[col] = compitem_getcoltext(ci,col);

   return( LVInsertItem( g_hListView, &lps[0], LV_DEF_COLS, (LPARAM)ci ) );

}

LRESULT compitem_addLV_ORG( COMPITEM ci )
{
   LRESULT  lRet = -1;
   LPTSTR   pr;
   PWIN32_FIND_DATA  pfd1, pfd2;
   DIRITEM leftname;
   DIRITEM rightname;
   DIRITEM di;

   if( !ci ||
      ( !ci->left && !ci->right ) ||
      !g_hListView )
      return -1;

   pr = 0;
   di = leftname = rightname = 0;
   pfd1 = pfd2 = 0;
   if( ci->left )
   {
      leftname = file_getdiritem(ci->left);
      pr = dir_getrelname(leftname);
      if(pr)
         di = leftname;
   }
   if( ci->right )
   {
      rightname = file_getdiritem(ci->right);
      if(!pr)
      {
         pr = dir_getrelname(rightname);
         if(pr)
            di = rightname;
      }
   }

   // note zero returned is NO DIRITEM on left or right - THERE MUST BE ONE!!!
   pfd1 = dir_getpfd(leftname);
   pfd2 = dir_getpfd(rightname);

   lRet = addci2lv( ci->ci_pTag, pr, ci->ci_pResult, pfd1, pfd2,
      (LPARAM)ci );

   if(pr && di)
      dir_freerelname(di, pr);

   return lRet;

}
#endif // #ifdef ADD_LIST_VIEW

//#endif   // #ifdef   ADDLVALL2
//#endif   // #ifdef   ADDLISTVIEW2

// FIX20070509 - This is ONLY called IF gbIgnDT is ON - only do partial compare
// But what to COMPARE - If Year,Month,day are the SAME
// SIMPLY CAN NOT BRING MYSELF TO RETURN ZERO IN EVERY CASE !!!
// which would be the TRUE meaning of 'Ignore Date/Time' ;=()
LONG  CompFTOne( LONG lg, FILETIME * pft1, FILETIME * pft2 )
{
   SYSTEMTIME  st1, st2;
   LONG        lg2 = lg;
   if(( FT2LST( pft1, &st1 ) ) &&
      ( FT2LST( pft2, &st2 ) ) )
   {
      if(( st1.wYear == st2.wYear ) &&
         ( st1.wMonth == st2.wMonth ) &&
         ( st1.wDay == st2.wDay ) ) {
         // usually ONLY difference is the HOUR
         // but sometime also the SECOND
         if(( st1.wHour == st2.wHour ) &&
            ( st1.wMinute == st2.wMinute )) {
            return 0; // That is CLOSE ENOUGH // FIX20070509
         }
         // FIX20070525 another small change ...
         // Allow it is the SAME TIME, if Year, Month, Day, Hour are the SAME,
         // and Minutes = +-1 minutes
         if(( st1.wHour == st2.wHour ) &&
            ( abs(st1.wMinute - st2.wMinute) == 1) ) {
            return 0; // That is CLOSE ENOUGH // FIX20070525
         }

         if(( st1.wMinute == st2.wMinute ) &&
            ( st1.wSecond == st2.wSecond ) ) {
            return 0;   // second is sometime the WRONG value!!!
         }
         if( st1.wMinute == st2.wMinute ) {
            return 0; // FIX20070509 - we have SAME YEAR, MONTH, DAY, so to the minute
         }
      }
   }
   return lg2;
}


/***************************************************************************
 * Function: compitem_new
 *
 * Purpose:
 *
 * Returns a handle to a new compitem - given the filenames for the
 * left and right files to be compared. Either left or right or neither
 * (but not both) may be null. In this case we set the state accordingly.
 *
 * The parameters are handles to DIRITEM objects: these allow us to get the
 * the name of the file relative to the compare roots (needed for the tag)
 * and the absolute name of the file (needed for opening the file).
 *
 * Comments:
 *
 * If the list parameter is not null, the List_New* functions are used to
 * allocate memory for the compitem. We remember this (in the bDiscard flag)
 * so we do not delete the compitem if it was allocated on the list.
 *
 * If the list parameter is null, the memory
 * for the compitem is allocated from the gmem_* heap initialised by the app.
 *
 * In the current application fExact, passed by complist_match() is ALWAYS TRUE!
 *
 * BIG WORK ADDITION:
 * If gbExact is ON, when a file has the SAME size, but different DATE
 * it is immediately EXPANDED, and a FULL compare is done,
 * then decide if IS or IS NOT identical, except DATE CHANGED.
 * 1 Jan 2002 - If gbIgnDT is ON, then set the SAME (almost)
 *
 * currently
 *   return( ci->ci_pResult );  // return pointer to 'typically'
 *  // <Newer but=size> 19/12/01 09:00 18/12/01 09:16 378
 *  // but *** TBD *** there should be more modifications of this data
 *  // under user's control. - like CSV (a comma separated database)
 ****************************************************************************/

//  NOTE: can be called with a ZERO LIST pointer (list), in which
//  case the allocated COMPITEM will be returned, else a new items
//  will be added to the LIST given.

COMPITEM compitem_new(DIRITEM leftname, DIRITEM rightname, LIST list, BOOL fDispos )
{
   static TCHAR _s_szcinewbuf[264];
   COMPITEM          ci;
   LPSTR             str1, str2, pr1, pr2;
//   LPTSTR            lpb = &gszTmpBuf[0];
   LPTSTR            lpb = _s_szcinewbuf;
   PWIN32_FIND_DATA  pfd1, pfd2;
   LONG              lg, lfs, lg2;
   LARGE_INTEGER     li1, li2;
   LPSYSTEMTIME      pst1, pst2;  // receives system time
   BOOL              b1, b2;
   /*
    * Allocate the memory for the compitem, either at the end of the
    * list or in the gmem_* heap.
    */
   if( list == NULL )
   {
      /* no list passed */
      ci = (COMPITEM) gmem_get(hHeap, sizeof(struct compitem), "compitem_new" );
      memset(ci, 0, sizeof(struct compitem));
      ci->bDiscard = TRUE;
   }
   else
   {
      /* add to end of list */
      ci = (COMPITEM) List_NewLast(list, sizeof(struct compitem));
      ci->bDiscard = FALSE;
   }

   ci->secs_composite = NULL;
   ci->secs_left      = NULL;
   ci->secs_right     = NULL;
   // start with LEFT and RIGHT as NOTHING
   ci->left           = NULL;
   ci->right          = NULL;
   // and
   ci->ci_dwFlag      = 0;    // no flags yet

   /*
    * Make a filedata for each of the files that are non-null.
    * Filedata objects are responsible for reading the file and
    * accessing the lines in it.
    * Don't read in the file(s) until we need to.
    *
    */

   if( leftname != NULL )
   {
      ci->left = file_new(leftname, FALSE);  // fill in the LEFT name
      if( ci->left == NULL )
      {
         chkme( "WARNING: file_new(left) FAILED!"MEOR );
         return(NULL);
      }
   }

   if( rightname != NULL )
   {
      ci->right = file_new(rightname, FALSE);   // and right name
      if( ci->right == NULL )
      {
         chkme( "WARNING: file_new(right) FAILED!"MEOR );
         return(NULL);
      }
   }

   /* See if we have one or two files, and set the state accordingly
    * NOTE WELL - we *MUST* have one of the two - L and/or right - */
   if( !ci->left && !ci->right )
   {
      /* two NULL files - this is wrong!!!!!! */
      chkme( "ERROR: compitem_new() FAILED for left AND right!"MEOR );
      return(NULL);
   }


   /* Set the tag (title field) for this item. If the
    * two files have names that match, we use just that name -
    * otherwise we use both names separated by a colon 'left : right'.
    *
    * In both cases, use the names relative to compare root (the
    * names will certainly be different if we compare the abs paths)
    */
   pst1 = &g_sST1;
   pst2 = &g_sST2;
   pr1 = dir_getrelname(leftname);
   pr2 = dir_getrelname(rightname);
   pfd1 = dir_getpfd(leftname);
   pfd2 = dir_getpfd(rightname);
   li1.QuadPart = 0;
   li2.QuadPart = 0;
   b1 = b2 = FALSE;
   if(pfd1)
   {
      b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
      li1.LowPart  = pfd1->nFileSizeLow;
      li1.HighPart = pfd1->nFileSizeHigh;
   }
   if(pfd2)
   {
      b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );
      li2.LowPart  = pfd2->nFileSizeLow;
      li2.HighPart = pfd2->nFileSizeHigh;
   }

   lg  = 0; // time compare
   lfs = 0; // size compare
   if( pfd1 && pfd2 )
   {
      lg = CompareFileTime( &pfd1->ftLastWriteTime, &pfd2->ftLastWriteTime );
      if( li1.QuadPart == li2.QuadPart )
         lfs = 0;
      else if( li1.QuadPart < li2.QuadPart )
         lfs = -1;
      else // if( li1.QuadPart > li2.QuadPart )
         lfs =  1;

   }

   // pTag is an allocated pointer filled with the RELATIVE file name
   /* If only one file - set name to that */
   if( ci->left == NULL )
   {
      ci->ci_pTag = ci_copytext(pr2);
      ci->bTagIsRel = TRUE;
   }
   else if( ci->right == NULL )
   {
      ci->ci_pTag = ci_copytext(pr1);
      ci->bTagIsRel = TRUE;
   }
   else
   {
      if( strcmpi(pr1, pr2) == 0 )
      {
         ci->ci_pTag = ci_copytext(pr2);
         ci->bTagIsRel = TRUE;
      }
      else
      {
         sprintf(lpb, "%s : %s", pr1, pr2);
         ci->ci_pTag = ci_copytext(lpb);
         ci->bTagIsRel = FALSE;
      }
   }

   if( ci->left == NULL )
   {
      strcpy(lpb, &szOIR[0] );    // <Only in Right>
   }
   else if( ci->right == NULL )
   {
      strcpy(lpb, &szOIL[0] );    // <Only in Left>
   }
   else  // got a LEFT and RIGHT
   {
      if( li1.QuadPart == li2.QuadPart )
      {
         // same SIZE
         if( lg == 0 )
         {
            // and same DATE
            strcpy( lpb, &szSTSStg[0] );  // "<Identical> "
         }
         else if( lg < 0 )
         {
            strcpy(lpb, &szOldSMX[0] );    // was "<SAME but Earlier> "
         }
         else
         {
            strcpy(lpb, &szNewSMX[0] );    // was "<SAME but Later  > "
         }
      }
      else  // NOT same size
      {
         // later can LOAD and really compare, but for now
         if( lg == 0 )
         {
            strcpy( lpb, &szDSD[0] );  // "<size but=date> "
         }
         else if( lg < 0 )   // First file time is less than second file time.
         {
            strcpy( lpb, &szDOld[0] ); // "<size & earlier> "
         }
         else
         {
            strcpy( lpb, &szDNew[0] ); // "<size & later> " );
         }
      }

   }

//   dir_freerelname(leftname,  pr1);
//   dir_freerelname(rightname, pr2);

   // set string to display for this item - L/R/Both
   if( gbSimple )
   {
      if( ci->left == NULL )
      {
              str2 = dir_getroot_item(rightname);
              sprintf(lpb, LoadRcString(IDS_ONLY_IN), str2);
              dir_freeroot_item(rightname, str2);
   
              ci->ci_pResult = ci_copytext(lpb);
              ci->state = STATE_FILERIGHTONLY;
              ci->ci_dwFlag = TT_RIGHT;   // only single
      }
      else if( ci->right == NULL )
      {
              str1 = dir_getroot_item(leftname);
              sprintf(lpb, LoadRcString(IDS_ONLY_IN), str1);
              dir_freeroot_item(leftname, str1);
   
              ci->ci_pResult = ci_copytext(lpb);
              ci->state = STATE_FILELEFTONLY;
              ci->ci_dwFlag = TT_LEFT;    // only single left
      }
      else
      {
         /* two files - are they the same ? compare
          * the file sizes
          */
         //pst1 = &g_sST1;
         //pst2 = &g_sST2;
         //pfd1 = dir_getpfd(leftname);
         //pfd2 = dir_getpfd(rightname);
         //lg = CompareFileTime( &pfd1->ftLastWriteTime, &pfd2->ftLastWriteTime );
         //li1.LowPart  = pfd1->nFileSizeLow;
         //li1.HighPart = pfd1->nFileSizeHigh;
         //li2.LowPart  = pfd2->nFileSizeLow;
         //li2.HighPart = pfd2->nFileSizeHigh;

         ci->ci_dwFlag = TT_BOTH;    // both left and right
         // FIX20081125 - NEW completely IGNORE file time, but still put in FLAG
         if( lg < 0 )
            ci->ci_dwFlag |= TT_DATEOLD;
         else if( lg > 0 )
            ci->ci_dwFlag |= TT_DATENEW;

         if( li1.QuadPart == li2.QuadPart )
         {
            ci->state = STATE_SAME;
            if( lg == 0 )  // SAME DATE TOO *****
            {
                ci->ci_pResult = ci_copytext(LoadRcString(IDS_IDENTICAL));
            }
            else
            {
               if( lg < 0 )
               {
                  // strcpy(lpb, &szOldSMX[0] );    // was "<SAME but Earlier> "
                  ci->ci_pResult = ci_copytext(LoadRcString(IDS_STRING51));
               // if( lg < 0 ) // Left/First file time is less than Right/second file time.
                  ci->ci_dwFlag |= TT_OLDER;    // left is older
               }
               else  // if( lg > 0 )
               {
                  ci->ci_dwFlag |= TT_YOUNGER;    // left is older
                  // strcpy(lpb, &szNewSMX[0] );    // was "<SAME but Later  > "
                  ci->ci_pResult = ci_copytext(LoadRcString(IDS_STRING52));
               }

               ci->ci_dwFlag |= TT_DATEDIFF;  // signal DATE different

            }
         }
         else  // not same size
         {

//              li1.LowPart = dir_getfilesize(leftname);
//              li2.LowPart = dir_getfilesize(rightname);
//              //if( dir_getfilesize(leftname) != dir_getfilesize(rightname) ) 
//              if( li1.LowPart != li2.LowPart ) 
//              {
            ci->state = STATE_DIFFER;

//                  if( li1.LowPart < li2.LowPart )
            if( li1.QuadPart < li2.QuadPart )
            {
                     ci->ci_dwFlag |= TT_SMALLER;    // left is smaller
                     strcpy(lpb, LoadRcString(IDS_STRING53));  // smaller
            }
            else
            {
                     ci->ci_dwFlag |= TT_LARGER;    // left is larger
                     strcpy(lpb, LoadRcString(IDS_STRING54));  // larger
            }

//          ci->ci_pResult = ci_copytext(LoadRcString(IDS_DIFFERENT_SIZES));
            strcat(lpb,", ");
            if( lg == 0 )  // ***** SAME DATE, DIFF SIZE *****
            {
               // no flags to add
               strcat(lpb, LoadRcString(IDS_STRING57));  // =date
            }
            else
            {
               if( lg < 0 )
               {
                  // strcpy(lpb, &szOldSMX[0] );    // was "<SAME but Earlier> "
                  //ci->ci_pResult = ci_copytext(LoadRcString(IDS_STRING51));
               // if( lg < 0 ) // Left/First file time is less than Right/second file time.
                  ci->ci_dwFlag |= TT_OLDER;    // left is older
                  strcat(lpb, LoadRcString(IDS_STRING55));  // Earlier
               }
               else  // if( lg > 0 )
               {
                  ci->ci_dwFlag |= TT_YOUNGER;    // left is older
                  // strcpy(lpb, &szNewSMX[0] );    // was "<SAME but Later  > "
                  //ci->ci_pResult = ci_copytext(LoadRcString(IDS_STRING52));
                  strcat(lpb, LoadRcString(IDS_STRING56));  // Later
               }

               ci->ci_dwFlag |= TT_DATEDIFF;  // signal DATE different

            }

            ci->ci_pResult = ci_copytext(lpb);

         }  // same size y/n
      }  // left, right or both
   }
   else  // !gbSimple
   {
      // else I want to EXPAND the information shown with size, date and time
      // ********************************************************************
      if( ci->left == NULL )
      {
         pst2 = &g_sST2;
         //pfd2 = dir_getpfd(rightname);
         str2 = dir_getroot_item(rightname);
         li2.LowPart  = pfd2->nFileSizeLow;
         li2.HighPart = pfd2->nFileSizeHigh;

         // TIME TROUBLE - This is NOT sufficient
         //b2 = FileTimeToSystemTime( &pfd2->ftLastWriteTime, pst2 );
         b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );

#ifdef   USEORGSTGS
         // Only in %s
         sprintf(lpb, LoadRcString(IDS_ONLY_IN), str2);
         sprintf(EndBuf(lpb), "%10I64u ", li2 );
         AppendDateTime( lpb, pst2 );
#else    // !#ifdef   USEORGSTGS

         strcpy(lpb, &szOIR[0] );    // <Only in Right>
         if( gbUseCSV )
            strcat(lpb,",");

         AppendDateTime( lpb, pst2 );
         //sprintf(EndBuf(lpb), "%10I64u ", li2 );
         sprintf(EndBuf(lpb), g_sz10I64u, li2 );
         strcat(lpb, str2);
#endif   // #ifdef   USEORGSTGS y/n

         ci->ci_pResult = ci_copytext(lpb);
         ci->state = STATE_FILERIGHTONLY;
         ci->ci_dwFlag = TT_RIGHT;   // only single

         dir_freeroot_item(rightname, str2);
      }
      else if( ci->right == NULL )
      {
         pst1 = &g_sST1;
         //pfd1 = dir_getpfd(leftname);
         str1 = dir_getroot_item(leftname);
         li1.LowPart  = pfd1->nFileSizeLow;
         li1.HighPart = pfd1->nFileSizeHigh;
         // TIME TROUBLE - This is NOT sufficient
         //b1 = FileTimeToSystemTime( &pfd1->ftLastWriteTime, pst1 );
         b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );

#ifdef   USEORGSTGS
         // Only in %s
         wsprintf(lpb, LoadRcString(IDS_ONLY_IN), str1);
         sprintf(EndBuf(lpb), "%10I64u ", li1 );
         AppendDateTime( lpb, pst1 );
#else    // !#ifdef   USEORGSTGS

         strcpy(lpb, &szOIL[0] );    // <Only in Left>
         if( gbUseCSV )
            strcat(lpb,",");

         AppendDateTime( lpb, pst1 );
         //sprintf(EndBuf(lpb), "%10I64u ", li1 );
         sprintf(EndBuf(lpb), g_sz10I64u, li1 );
         strcat(lpb, str1);

#endif   // #ifdef   USEORGSTGS y/n

         ci->ci_pResult = ci_copytext(lpb);
         ci->state = STATE_FILELEFTONLY;
         ci->ci_dwFlag = TT_LEFT;    // only single left
         dir_freeroot_item(leftname, str1);
      }
      else
      {
         /* two files - are they the same ? compare the file sizes */
         // =========================================================
         ci->ci_dwFlag = TT_BOTH;    // both left and right
//         pst1 = &g_sST1;
//         pst2 = &g_sST2;
//         pfd1 = dir_getpfd(leftname);
//         pfd2 = dir_getpfd(rightname);
//         lg = CompareFileTime( &pfd1->ftLastWriteTime, &pfd2->ftLastWriteTime );

//         li1.LowPart  = pfd1->nFileSizeLow;
//         li1.HighPart = pfd1->nFileSizeHigh;
//         li2.LowPart  = pfd2->nFileSizeLow;
//         li2.HighPart = pfd2->nFileSizeHigh;
         // The FileTimeToSystemTime function converts a 64-bit file time
         // to system time format. 
         // BOOL FileTimeToSystemTime(
         //    CONST FILETIME *lpFileTime,  // file time to convert
         //    LPSYSTEMTIME lpSystemTime )  // receives system time
         // TIME TROUBLE - This is NOT sufficient
         //b1 = FileTimeToSystemTime( &pfd1->ftLastWriteTime, pst1 );
         //b2 = FileTimeToSystemTime( &pfd2->ftLastWriteTime, pst2 );
//         b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
//         b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );
         // FIX20081125 - NEW completely IGNORE file time, but still put in FLAG
         if( lg < 0 )
            ci->ci_dwFlag |= TT_DATEOLD;
         else if( lg > 0 )
            ci->ci_dwFlag |= TT_DATENEW;

         if( li1.QuadPart == li2.QuadPart )
         {
            // they at least have the SAME size
            if( lg == 0 )
            {
               // *************************************************
               // and same DATE
               // *************** this is taken as EQUAL files ****
               ci->state = STATE_SAME;
               strcpy( lpb, &szSTSStg[0] );  // "<Identical> "
               if( gbUseCSV )
                  strcat(lpb,",");

               if( b1 && b2 )
                  AppendDateTime( lpb, pst1 );
               else
                  strcat(lpb, &szNODT[0] );  // "??/??/?? ??:??"

               sprintf(EndBuf(lpb), "%10I64u ", li1.QuadPart );
            }
            else
            {
               // same size, different dates
               // There is a case here for
               // building the section lists and composite lists now,
               // and REALLY REALLY detemine that the file are EQUAL,
               // just a date change, alteration
#ifdef ADD_ZIP_SUPPORT
               if( ( dir_iszip( leftname ) ) || ( dir_iszip( rightname ) ) )
               {
                  // one (or both) are zip files
                  // TIME TROUBLE
                  lg2 = CompFTZip( lg,
                     &pfd1->ftLastWriteTime,
                     &pfd2->ftLastWriteTime );
                  if( lg2 == 0 )
                  {
                     ci->state = STATE_SAME;
                     strcpy( lpb, &szSTSStg2[0] );  // "<Identical?Zip> "
                     if( gbUseCSV )
                        strcat(lpb,",");
                     if( b1 && b2 )
                     {
                        AppendDateTime( lpb, pst1 );
                        strcat( lpb, " " );
                        AppendDateTime( lpb, pst2 );
                        //if( dir_iszip( leftname ) )
                        //   AppendDateTime( lpb, pst2 );
                        //else
                        //   AppendDateTime( lpb, pst1 );
                     }
                     else
                        strcat(lpb, &szNODT[0] );  // "??/??/?? ??:??"

                     sprintf(EndBuf(lpb), "%10I64u ", li1.QuadPart );

                     goto Copy_Text;

                  }
               }
#endif // #ifdef ADD_ZIP_SUPPORT

               // *******************************************************
               // ci->ci_dwFlag = TT_BOTH;    // have both left and right
               // ci->ci_dwFlag |= TT_DATEDIFF;  // signal DATE different
               // but only if EXACT is in use - Mch2002
               if( gbExact )  // IDM_OPTEXACT - full line by line compare
               {
                  ci->state = compitem_exact( ci );   // load, and compare data, line by line
                  if( ci->state == STATE_SAME )
                  {
                     if( lg < 0 )
                        strcpy(lpb, &szOldSMX[0] );    // was "<SAME but Earlier> "
                     else
                        strcpy(lpb, &szNewSMX[0] );    // was "<SAME but Later  > "
                     ci->ci_dwFlag |= TT_DATEDIFF;  // signal DATE different
                  }
                  else
                  {
                     if( lg < 0 )
                        strcpy(lpb, &szOldDFX[0] );   // "<Earlier DIFFERENT> " );
                     else
                        strcpy(lpb, &szNewDFX[0] );   // "<Later   DIFFERENT> " );
                  }
               }
               else  // !gbExact = NO full compare of file data 
               {
                  lg2 = lg;
                  if( gbIgnDT ) {
                     if( gbIgnDT2 ) // FIX20081125
                        lg2 = 0; // FORCED IGNORE DATE/TIME COMPLETELY
                     else
                     {
                        lg2 = CompFTOne( lg,
                           &pfd1->ftLastWriteTime,
                           &pfd2->ftLastWriteTime );
                     }
                  }
                  if( lg2 == 0 ) {
                     ci->state = STATE_SAME; // SET SAME STATE
                     if( lg < 0 )
                        strcpy(lpb, &szOldSMX[0] );    // was "<SAME but Earlier> "
                     else
                        strcpy(lpb, &szNewSMX[0] );    // was "<SAME but Later  > "
                     ci->ci_dwFlag |= TT_DATEDIFF;  // signal DATE different
                  } else {
                     //if( gbDateDiff )  // presently PERMANENTLY TRUE
                        ci->state = STATE_DIFFER;
                     //else
                     //   ci->state = STATE_SAME;

                     if( lg < 0 )
                        strcpy(lpb, &szOldSM[0] );  // "<EARLIER but=size> " );
                     else
                        strcpy(lpb, &szNewDF[0] );  // "<LATER   but=size> " );
                  }
               }

               if( gbUseCSV )
                  strcat(lpb,",");

               if( lg < 0 )   // Left/First file time is less than Right/second file time.
                  ci->ci_dwFlag |= TT_OLDER;    // left is older
               else
                  ci->ci_dwFlag |= TT_YOUNGER;    // left is older

               if( b1 && b2 )
               {
                  AppendDateTime( lpb, pst1 );
                  strcat( lpb, " " );
                  AppendDateTime( lpb, pst2 );
               }
               else
               {
                  strcpy(lpb, &szNoDSM[0] );    // "<DATE? but=size> "
                  strcat(lpb, &szNODT[0] );     // = ??/??/?? ??:??"
               }

               // add one size to string (since they are the SAME SIZE
               sprintf(EndBuf(lpb), " %I64u", li1.QuadPart );

            }
         }
         else  // NOT same SIZE
         {
            // they are NOT the same size
            // ##########################
            // but, for example, due to line endings CR|CRLF|LF ...
            // they could actually be the SAME FILE ********** FIX20050129 - line ending!
            if( li1.QuadPart < li2.QuadPart )
               ci->ci_dwFlag |= TT_SMALLER;    // left is smaller
            else
               ci->ci_dwFlag |= TT_LARGER;    // left is larger

            // but only if EXACT is in use - Mch2002
            if( gbExact ) {  // IDM_OPTEXACT - full line by line compare
               ci->state = compitem_exact( ci );   // load, and compare data, line by line
            } else {
               ci->state = STATE_DIFFER;
            }
            if( b1 && b2 )
            {
               if( lg == 0 )
               {
                  strcpy( lpb, &szDSD[0] );  // "<size but=date> "
                  if( gbUseCSV )
                     strcat(lpb,",");
                  AppendDateTime( lpb, pst1 );
               }
               else
               {
                  if( lg < 0 )   // First file time is less than second file time.
                  {
                     strcpy( lpb, &szDOld[0] ); // "<size & earlier> "
                     ci->ci_dwFlag |= TT_OLDER;    // left is older
                  }
                  else
                  {
                     strcpy( lpb, &szDNew[0] ); // "<size & later> " );
                     ci->ci_dwFlag |= TT_YOUNGER;    // left is older
                  }
                  if( gbUseCSV )
                     strcat(lpb,",");

                  AppendDateTime( lpb, pst1 );
                  strcat( lpb, " " );
                  AppendDateTime( lpb, pst2 );
               }
            }
            else
            {
               strcpy(lpb, &szNoDDF[0] );    // "<size & date?> "
               if( gbUseCSV )
                  strcat(lpb,",");
               strcat(lpb, &szNODT[0] );     // ="??/??/?? ??:??"
            }

            // add BOTH sizes to the string
            strcat( lpb, " " );
            sprintf(EndBuf(lpb), "%I64u ", li1.QuadPart );
            sprintf(EndBuf(lpb), "%I64u ", li2.QuadPart );
         }

Copy_Text:

         // allocate buffer, and copy result text into it.
         ci->ci_pResult = ci_copytext(lpb);
         // like for example
         // <Newer but=size>[gbUseCSV] 19/12/01 09:00 18/12/01 09:16 378
         // but this limits the 'style' of output - *** TBD ** should use
         // separate columns in a list control, rather than the
         // present 'table' window, which is quite 'spikey' !!!

      }
      // ********************************************************************

   }   // gbSimple y/n

#ifdef   ADDLISTVIEW2   // test out FULL addition to LISTVIEW
#ifdef   ADDLVALL2
   if(pr1)
      addci2lv( ci->pTag, pr1, ci->ci_pResult, pfd1, pfd2, (LPARAM)ci );
   else
      addci2lv( ci->pTag, pr2, ci->ci_pResult, pfd1, pfd2, (LPARAM)ci );
#endif   // #ifdef   ADDLVALL2
#endif   // #ifdef   ADDLISTVIEW2

   dir_freerelname(leftname,  pr1);
   dir_freerelname(rightname, pr2);

   /* Building the section lists and composite lists can wait until needed. */

   return(ci);

} /* compitem_new */

/***************************************************************************
 * Function: compitem_delete
 *
 * Purpose:
 *
 * Deletes a compitem and free all associated data.
 *
 * Comments:
 *
 * If the ci->bDiscard flag is set, the compitem was alloc-ed on a list,
 * and should not be discarded (the list itself will be deleted).
 *
 * The DIRDATA we were passed are not deleted. the filedata, lines
 * and sections are.
 ***************************************************************************/
void compitem_delete(COMPITEM ci)
{
        if (ci == NULL) {
                return;
        }

        compitem_discardsections(ci);

        /* Delete the two filedatas (and associated line lists) */
        file_delete(ci->left);
        file_delete(ci->right);

        /* text we allocated */
        gmem_free(hHeap, ci->ci_pTag,    (strlen(ci->ci_pTag) + 1),   "ci_copytext" );
        gmem_free(hHeap, ci->ci_pResult, (strlen(ci->ci_pResult) + 1),"ci_copytext" );
        ci->ci_pTag    = 0;
        ci->ci_pResult = 0;

        /* Free the compitem struct itself if not alloced on a list */
        if (ci->bDiscard) {
                gmem_free(hHeap, (LPSTR) ci, sizeof(struct compitem), "compitem_new" );
        }
}


/*
/***************************************************************************
 * Function: compitem_discardsections
 *
 * Purpose:
 *
 * To discard sections - throw away cached information relating to the
 * comparison (but not the files if they are read into memory). This
 * is used to force a re-compare if changes in the comparison options
 * are made
 *
 ***************************************************************************/
void compitem_discardsections(COMPITEM ci)
{
        /* delete the lists of sections we built */
        if (ci == NULL) {
                return;
        }
        if (ci->secs_composite) {
                section_deletelist(ci->secs_composite);
                ci->secs_composite = NULL;
        }
        if (ci->secs_left) {
                section_deletelist(ci->secs_left);
                ci->secs_left = NULL;
        }
        if (ci->secs_right) {
                section_deletelist(ci->secs_right);
                ci->secs_right = NULL;
        }

        /* reset the line lists to throw away cached hash codes and links */
        if (ci->left != NULL) {
                file_reset(ci->left);
        }
        if (ci->right != NULL) {
                file_reset(ci->right);
        }

}

/***************************************************************************
 * Function: compitem_getcomposite
 *
 * Purpose:
 *
 * To get the handle for the composite section list 
 *
 ***************************************************************************/
LIST compitem_getcomposite(COMPITEM ci)
{
        if (ci == NULL) {
                return NULL;
        }
        /*
         * do the comparison if we haven't already done it
         */
        if( ci->secs_composite == NULL )
        {
                ci_makecomposite(ci);
        }

        return(ci->secs_composite);
}

/***************************************************************************
 * Function: compitem_getleftsections
 *
 * Purpose:
 *
 * To get the handle for the list of sections in the left file 
 *
 ***************************************************************************/
LIST compitem_getleftsections(COMPITEM ci)
{
        if (ci == NULL) {
                return NULL;
        }
        /*
         * do the comparison if we haven't already done it
         */
        if (ci->secs_composite == NULL) {
                ci_makecomposite(ci);
        }

        return(ci->secs_left);
}

/***************************************************************************
 * Function: compitem_getrightsections
 *
 * Purpose:
 *
 * To get the handle for the list of sections in the right file 
 *
 ***************************************************************************/
LIST compitem_getrightsections(COMPITEM ci)
{
        if (ci == NULL) {
                return NULL;
        }
        /*
         * do the comparison if we haven't already done it
         */
        if (ci->secs_composite == NULL) {
                ci_makecomposite(ci);
        }

        return(ci->secs_right);
}

/***************************************************************************
 * Function: compitem_getleftfile
 *
 * Purpose:
 *
 * To get the handle to the left file itself
 *
 ***************************************************************************/
FILEDATA compitem_getleftfile(COMPITEM ci)
{
        if( ci == NULL )
        {
                return(NULL);
        }
        return(ci->left);
}

/***************************************************************************
 * Function: compitem_getrightfile
 *
 * Purpose:
 *
 * To get the handle to the right file itself 
 *
 ***************************************************************************/
FILEDATA compitem_getrightfile(COMPITEM ci)
{
        if (ci == NULL) {
                return(NULL);
        }
        return(ci->right);
}

/***************************************************************************
 * Function: compitem_getstate
 *
 * Purpose:
 *
 * To get the state (compare result) of this compitem 
 *
 * Current RETURN is one of -
 * == STATE_SAME, STATE_DIFFER, STATE_FILELEFTONLY, or STATE_FILERIGHTONLY ==
 *
 ***************************************************************************/
int compitem_getstate(COMPITEM ci)
{
        if (ci == NULL)
        {
                return(0);
        }
        return(ci->state);
}

DWORD compitem_getflag( COMPITEM ci )
{
   if( !ci )
      return 0;

   //  if( lg < 0 )   // Left/First file time is less than Right/second file time.
   //      ci->ci_dwFlag |= TT_OLDER;    // left is older
   //  else
   //       ci->ci_dwFlag |= TT_YOUNGER;    // left is older
   return ( ci->ci_dwFlag );
}

DWORD compitem_addflag( COMPITEM ci, DWORD dwf )
{
   if( !ci )
      return 0;
   ci->ci_dwFlag |= dwf;
   return ( ci->ci_dwFlag );
}
DWORD compitem_addflag2( COMPITEM ci, DWORD dwf, LRESULT index )
{
   if( !ci )
      return 0;
   ci->ci_dwFlag |= dwf;
   ci->ci_Index = index;

   return ( ci->ci_dwFlag );
}
DWORD compitem_getindex( COMPITEM ci )
{
   if(!ci)
      return 0;
   return ( ci->ci_Index );
}

DWORD compitem_subflag( COMPITEM ci, DWORD dwf )
{
   if( !ci )
      return 0;
   ci->ci_dwFlag &= ~(dwf);
   return ( ci->ci_dwFlag );
}

/***************************************************************************
 * Function: compitem_gettext_tag
 *
 * Purpose:
 *
 * To get the tag (text for the compitem title) 
 * In the OUTLINE file list the 'tag' is the RELATIVE name of the file
 * If just left, then the left relative name. Likewise with right only.
 * If BOTH, and they are the same, then just one of them, else
 * it is composite of "leftname : rightname" ...
 *
 * NOTE: Since the callers to here do NOT always check for a NULL return
 *       then change to NEVER returning a NULL - FIX20010708 - grm
 *
 ***************************************************************************/
LPSTR
compitem_gettext_tag(COMPITEM ci)
{
   if(( ci          == NULL ) ||
      ( ci->ci_pTag == NULL ) )
   {
      return( "<null>" );
      //return(NULL);
   }

   return( ci->ci_pTag );
}

LPTSTR compitem_gettext_tag2(COMPITEM ci)
{
   if(( ci          == NULL ) ||
      ( ci->ci_pTag == NULL ) )
   {
      return( "<null>" );
      //return(NULL);
   }
   else
   {
      LPTSTR   lpt = compitem_gettext_tag(ci);
      LPTSTR   lpb = GetStgBuf();
      if( *lpt == '.' )
         lpt += 2;
      strcpy(lpb,lpt);

      return lpb;
   }
}

// This is often referred to as the files 'title', (and extent)
// It removes all relative folder information from the tag
LPTSTR compitem_gettext_tag3(COMPITEM ci) // ADD_REL_PATH bit in gdwFileOpts
{
   if(( ci          == NULL ) ||
      ( ci->ci_pTag == NULL ) )
   {
      return( "<nul3>" );
      //return(NULL);
   }
   else
   {
      LPTSTR   lpt = compitem_gettext_tag2(ci);
      LPTSTR   lpb = GetStgBuf();
      LPTSTR   p = strrchr(lpt, '\\');
      if(p) // if we have a path
      {
         p++;
         strcpy(lpb,p);
      }
      else
         strcpy(lpb,lpt);  // get the tag 2

      return lpb;
   }
}

LPTSTR compitem_gettext_tags(COMPITEM ci)
{
   DWORD    dwo = gdwFileOpts;
   LPTSTR   lpb;  // = GetStgBuf();
   if(( ci          == NULL ) ||
      ( ci->ci_pTag == NULL ) )
   {
      return( "<nult>" );
      //return(NULL);
   }
   if( dwo & FULL_NAMES )
   {
      // this will be the full root path, plust drive
      lpb = GetStgBuf();
      *lpb = 0;
      compitem_retfullname( lpb, ci, dwo );
   }
   else if( dwo & ADD_REL_PATH )
   {
      // but I like to remove the starting ".\" !!!
      //strcat(lpb2, compitem_gettext_tag(ci) );
      // and the whole relative path some times
      lpb = compitem_gettext_tag3(ci);
   }
   else
   {
      // This is a RELATIVE path,
      // as above, but no starting ".\" root!!!
      lpb = compitem_gettext_tag2(ci);
   }

   //return( ci->pTag );
   return lpb;
}

VOID  compitem_setmaxlen( COMPITEM ci, PINT ptl, PINT prest )
{
   char * cpt = compitem_gettext_tags(ci);
   char * cpr = compitem_gettext_result(ci);
   INT   ilent, ilenr;
   INT   max_tag, max_rest;

   max_tag  = *ptl;
   max_rest = *prest;
   ilent = ilenr = 0;
   if(cpt){ ilent = strlen(cpt); }
   if(cpr){ ilenr = strlen(cpr); }

   //ilen = (int)strlen(compitem_gettext_tags(ci));
   // should perhaps use list control as mentioned else where ...
   //view->maxtag  = max( view->maxtag, ilen );
   //maxtag = max( maxtag, ilen );
   if( ilent > max_tag ) {
      DBG_LOG1( "Bumped 'max_tag' from %d to %d ... tag[%s]"MEOR, max_tag, ilent, cpt );
      max_tag = ilent;
   }
   // *** TBD *** note embedded just two columns concept here!
   // ilen = (int)strlen( compitem_gettext_result(ci) );
   // view->maxrest = max( view->maxrest, ilen );
   // maxrest = max( maxrest, ilen );
   if( ilenr > max_rest ) {
      DBG_LOG1( "Bumped 'max_rest' from %d to %d ... tag[%s]"MEOR, max_rest, ilenr, cpr );
      max_rest = ilenr;
   }

   *ptl   = max_tag;
   *prest = max_rest;
}

// Is the pTag the RELATIVE name, or NOT (ie null of composite)
BOOL  compitem_ispTagrel(COMPITEM ci)
{
   if( !ci )
      return FALSE;  // well, sort of

   return( ci->bTagIsRel );
}

/***************************************************************************
 * Function: compitem_gettext_result
 *
 * Purpose:
 *
 * To get the result text (text equiv of state) 
 *
 * NOTE: Like above compitem_gettext_tag,
 *       since the callers to here do NOT always check for a NULL return
 *       then change to NEVER returning a NULL - FIX20010708 - grm
 *
 ***************************************************************************/
LPSTR
compitem_gettext_result(COMPITEM ci)
{
   if(( ci             == NULL ) ||
      ( ci->ci_pResult == NULL ) )
   {
      return( "<Null>" );
      //return(NULL);
   }

   return( ci->ci_pResult );  // return pointer to 'typically'
   // <Newer but=size> 19/12/01 09:00 18/12/01 09:16 378
   // but *** TBD *** there should be more modifications of this data
   // under user's control. - like CSV (a comma separated database)
}

/***************************************************************************
 * Function: compitem_getfilename
 *
 * Purpose:
 *
 * To return the name of the file associated with this compitem. The option
 * argument (one of CI_LEFT, CI_RIGHT, CI_COMP) indicates which file
 * is required.
 *
 * Comments:
 *
 * CI_LEFT and CI_RIGHT just result in calls to dir_getopenname to get
 * an open-able filename.
 *
 * For CI_COMP, we create a temporary file, write out all the text in the
 * composite section list to this file, and then pass the name of the
 * temporary file to the caller. This file will be deleted on
 * the call to compitem_freefilename().
 * 
 ***************************************************************************/
LPSTR
compitem_getfilename(COMPITEM item, int option)
{
        LPSTR fname;
        LINE line;
        LPSTR tag, text;
        SECTION sec;
        OFSTRUCT os;
        int fh;

        if (item == NULL) {
                return(NULL);
        }

        switch(option) {
        case CI_LEFT:
                if (item->left != NULL) {
                        return(dir_getopenname(file_getdiritem(item->left)));
                } else {
                        return(NULL);
                }

        case CI_RIGHT:
                if (item->right != NULL) {
                        return(dir_getopenname(file_getdiritem(item->right)));
                } else {
                        return(NULL);
                }

        case CI_COMP:

                /* caller has asked for the filename of the composite file.
                 * we need to create a temporary file and write the
                 * lines in the composite section list out to it.
                 */
                fname = gmem_get(hHeap, MAX_PATH, "compitem_getfilename");
                GetTempPath(MAX_PATH, fname);
                GetTempFileName(fname, "wdf", 0, fname);

                fh = OpenFile(fname, &os, OF_READWRITE|OF_SHARE_DENY_NONE);
                if (fh < 0) {
                        MB(NULL,
                           LoadRcString(IDS_CANT_OPEN_TMP_FILE),
                           APPNAME,
                           MB_OK | MB_ICONSTOP);
                        return(NULL);
                }

                /* make sure the composite list has been built */

                if (item->secs_composite == NULL) {
                        ci_makecomposite(item);
                }

                /* write out every line in every section on the composite
                 * list to the temp file.
                 */
                List_TRAVERSE(item->secs_composite, sec)
                {

                        /* get the tag field based on the section state*/
                        switch(section_getstate(sec))
                        {
                        case STATE_SAME:
                                tag = "    ";
                                break;

                        case STATE_LEFTONLY:
                                tag = " <! ";
                                break;
                        case STATE_RIGHTONLY:
                                tag = " !> ";
                                break;

                        case STATE_MOVEDLEFT:
                                tag = " <- ";
                                break;

                        case STATE_MOVEDRIGHT:
                                tag = " -> ";
                                break;
                        }

                        /* write out each line in this section.
                         * non-standard traverse of list as we only
                         * want to go from section first to section last
                         * inclusive.
                         */
                        for (line = section_getfirstline(sec);
                             line != NULL;
                             line = List_Next(line) )
                        {

                                text = line_gettext(line);

                                /* write out to file */
                                _lwrite(fh, tag, lstrlen(tag));
                                _lwrite(fh, text, lstrlen(text));

                                if (line == section_getlastline(sec))
                                {
                                        break;
                                }
                        }
                }

                /* now close the file and return its name */
                _lclose(fh);
                return(fname);


        default:
                MB(NULL,
                   LoadRcString(IDS_BAD_ARGUMENT),
                   APPNAME,
                   MB_OK | MB_ICONSTOP);
                return(NULL);
        }
}

/***************************************************************************
 * Function: compitem_freefilename
 *
 * Purpose:
 *
 * Free memory created by a call to compitem_getfilename. If a temporary
 * file was created, this may cause it to be deleted. The option argument must
 * be the same as passed to the original compitem_getfilename call.
 *
 * If we created a temporary file for CI_COMP, then delete it; otherwise,
 * just pass the name to dir_freeopenname.
 *
 ***************************************************************************/
void
compitem_freefilename(COMPITEM item, int option, LPSTR filename)
{
        OFSTRUCT os;


        if ((item == NULL) || (filename == NULL)) {
                return;
        }

        switch(option) {

        case CI_LEFT:
                dir_freeopenname(file_getdiritem(item->left), filename);
                break;

        case CI_RIGHT:
                dir_freeopenname(file_getdiritem(item->right), filename);
                break;

        case CI_COMP:

                /* this is a temporary file we created. Delete it. */
                OpenFile(filename, &os, OF_DELETE);

                gmem_free(hHeap, filename, MAX_PATH, "compitem_getfilename" );
                break;
        }
}


/***************************************************************************
 * Function: ci_copytext
 *
 * Purpose:
 *
 * To alloc a buffer large enough for the text string and copy the text into
 * it and return a pointer to the string.
 *
 * NOTE: If NO pointer passed, then still return a 1 byte buffer with 0
 *       so that it always returns a result!
 *
 ***************************************************************************/
LPSTR
ci_copytext(LPSTR in)
{
   LPSTR out;

   if (in == NULL) {
          out = gmem_get(hHeap, 1, "ci_copytext" );
          out[0] = '\0';
   } else {
          out = gmem_get(hHeap, (strlen(in) + 1), "ci_copytext" );
          strcpy(out, in);
   }
   return(out);
}

#define  FD_NONE        0
#define  FD_RIGHT       1
#define  FD_LEFT        2

/***************************************************************************
 * Function: ci_onesection
 *
 * Purpose:
 *
 * To make a list containing a single section from the whole list of lines 
 *
 ***************************************************************************/
LIST ci_onesection(FILEDATA file, INT iFD )
{
        LIST   lines;
        LIST   sections;
        DWORD  dwc;  // = file_getlinecount(file);

        lines = file_getlinelist(file);   // read if required
        dwc   = file_getlinecount(file);

//#ifdef   USELCDIRECT
        {
           SECTION section;
            // use LIST CREATE directly - avoids stat collecting
            /* create a null list */
            sections = List_Create();
            /* tell the section to create itself on the end of this list. */
            section = section_new(List_First(lines), List_Last(lines), sections);
            section_setstate(section, STATE_SAME);
        }
//#endif   // USELCDIRECT

        if( iFD )
        {
           // stat items only
           section_makeone(lines);
           section_setcompstats(  dwc, 0, iFD );
        }

        return(sections);
}



/***************************************************************************
 * Function: ci_makecomposite
 *
 * Purpose:
 *
 * Compare the two files and build the composite list. This function is
 * called whenever we need one of the section lists and only does the 
 * comparison if the composite list does not already exist.
 *
 ***************************************************************************/
void ci_makecomposite(COMPITEM ci)
{
        if (ci->secs_composite != NULL)
        {
                return;
        }

        /* if there is only one file, make a single item list
         * of sections
         */
        if( ci->left == NULL )
        {
                ci->secs_left = NULL;
                ci->secs_right = ci_onesection(ci->right, FD_NONE);

                /* make a second list, not a pointer to the first
                 * or we will get confused when deleting
                 */
                ci->secs_composite = ci_onesection(ci->right, FD_RIGHT);
                return;
        }
        else if( ci->right == NULL )
        {
                ci->secs_right = NULL;
                ci->secs_left = ci_onesection(ci->left, FD_NONE);

                /* make a second list, not a pointer to the first
                 * or we will get confused when deleting
                 */
                ci->secs_composite = ci_onesection(ci->left, FD_LEFT);
                return;
        }

        /* we have two files - we need to compare them fully */
        ci_compare(ci);
        if ((ci && ci->left && (ci->left->fd_file_flag & ff_found_zero))||
            (ci && ci->right && (ci->right->fd_file_flag & ff_found_zero)))
            ci->ci_dwFlag |= TT_GOTZERO;
}

/***************************************************************************
 * Function: ci_compare
 *
 * Purpose:
 *
 * Compare files and build a composite list.
 *
 * Comments:
 *
 * Comparison method:
 *
 *    0   Break each file into lines and hash each line.  Lines which 
 *        don't match can be rapidly eliminated by comparing the hash code.
 *
 *        Store the hash codes in a binary search tree that
 *        will give for each hash code the number of times that it
 *        occurred in each file and one of the lines where it occurred
 *        in each file.  The tree is used to rapidly find the partner
 *        of a line which occurs exactly once in each file.
 *
 *    1   Make a section covering the whole file (for both)
 *        and link unique lines between these sections (i.e. link lines
 *        which occur exactly once in each file as they must match each other).
 *        These are referred to as anchor points.
 *
 *    2   Build section lists for both files by traversing line lists and
 *        making a section for each set of adjacent lines that are unmatched
 *        and for each set of adjacent lines that match a set of adjacent
 *        lines in the other file.  In making a section we start from a
 *        known matching line and work both forwards and backwards through
 *        the file including lines which match, whether they are unique or not.
 *
 *    3   Establish links between sections that match
 *        and also between sections that don't match but do
 *        correspond (by position in file between matching sections)
 *
 *    4   For each section pair that don't match but do correspond,
 *        link up matching lines unique within that section.  (i.e. do
 *        the whole algorithm again on just this section).
 *
 *    There may be some lines which occur many times over in each file.
 *    As these occurrences are matched up, so the number left to match
 *    reduces, and may reach one in each file.  At this point these two
 *    can be matched.  Therefore we...
 *
 *    Repeat steps 0-4 until no more new links are added, but (especially
 *    in step 0) we only bother with lines which have not yet been matched.
 *    This means that a line which has only one unmatched instance in each
 *    file gets a count of one and so is a new anchor point.
 *
 *    Finally build a composite list from the two lists of sections.
 *
 ***************************************************************************/
void ci_compare(COMPITEM ci)
{
        LIST lines_left, lines_right;
        SECTION whole_left, whole_right;
        BOOL bChanges;
        INT iSects, iLinks;

        /* get the list of lines for each file */
        lines_left  = file_getlinelist(ci->left );
        lines_right = file_getlinelist(ci->right);

        if((lines_left  == NULL) ||
           (lines_right == NULL) )
        {
                ci->secs_left = NULL;
                ci->secs_right = NULL;
                ci->secs_composite = NULL;
                return;
        }

        iSects = 0;
        iLinks = 0;
        do
        {

                /* we have made no changes so far this time round the
                 * loop
                 */
                bChanges = FALSE;

                /* make a section covering the whole file */
                whole_left = section_new(List_First(lines_left),
                                         List_Last(lines_left), NULL);

                whole_right = section_new(List_First(lines_right),
                                         List_Last(lines_right), NULL);

                /* link up matching unique lines between these sections */
                if( section_match(whole_left, whole_right) )
                {
                        bChanges = TRUE;
                        iSects++;   // could not 'exactly' match
                }

                /* delete the two temp sections */
                section_delete(whole_left);
                section_delete(whole_right);

                /* discard previous section lists if made */
                if( ci->secs_left )
                {
                        section_deletelist(ci->secs_left);
                        ci->secs_left = NULL;
                }
                if( ci->secs_right )
                {
                        section_deletelist(ci->secs_right);
                        ci->secs_right = NULL;
                }

                /* build new section lists for both files */
                ci->secs_left  = section_makelist(lines_left, TRUE  );
                ci->secs_right = section_makelist(lines_right, FALSE);

                /* match up sections - make links and corresponds between
                 * sections. Attempts to section_match corresponding
                 * sections that are not matched. returns true if any
                 * further links were made
                 */
                if( section_matchlists(ci->secs_left, ci->secs_right) )
                {
                        bChanges = TRUE;
                        iLinks++;
                }

        /* repeat as long as we keep adding new links */

        } while (bChanges);

        /* all possible lines linked, and section lists made .
         * combine the two section lists to get a view of the
         * whole comparison - the composite section list. This also
         * sets the state of each section in the composite list.
         */

        ci->secs_composite = section_makecomposite(ci->secs_left, ci->secs_right);
//        lines_left  = file_getlinelist(ci->left );
//        lines_right = file_getlinelist(ci->right);
        section_setcompstats( file_getlinecount(ci->left ),
            file_getlinecount(ci->right), 3 );

}


/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/


// NEW

BOOL compitem_settargs( COMPITEM ci, PVOID pv )
{
   BOOL  bRet = FALSE;
   PTARGS   pta = (PTARGS)pv;
   PDDIFF   pdd;
   LPTSTR   pstr;
   LPTSTR   lpf;

   if( !ci || !pta )
      goto Exit_Set;

   pstr = compitem_getfilename(ci, CI_LEFT);
   if(pstr)
   {
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
      strcpy( lpf, pstr );   // fill in LEFT name
      compitem_freefilename(ci, CI_LEFT, pstr);
      if( !dir_isvalidfile2( lpf, &pta->ta_sFDL ) )
         goto Exit_Set;
   }
   else
      goto Exit_Set;

   pstr = compitem_getfilename(ci, CI_RIGHT);
   if(pstr)
   {
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
      strcpy( lpf, pstr);   // fill in RIGHT name
      compitem_freefilename(ci, CI_RIGHT, pstr);
      if( !dir_isvalidfile2( lpf, &pta->ta_sFDR ) )
         goto Exit_Set;
   }
   else
      goto Exit_Set;

   bRet = TRUE;

Exit_Set:

   if( bRet )
   {
      pdd = &pta->ta_sDI;
      pdd->di_sCL = 0;
      pdd->di_sCI = ci;
   }

   return bRet;

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : compitem_savediff
// Return type: VOID 
// Arguments  : COMPITEM ci
//            : LPTSTR pDiffName
//            : UINT options
// Description: If in an EXPANDED state, we can WRITE the "difference" to
//              a file given via the DIALOG box.
// Called from MENU item IDM_WRITEDIFF
// NOTE: Presently ONLY called will pDiffName = NULL, thus a DIALOG BOX
// is invoked to ask questions. And this is ONLY when already EXPANDED
// mode, thus we 'know' which pair of files we are doing.
// There is presently considerable overlap between IDD_DLG_DIFF and
// IDD_SAVEDIFF. The IDD_SAVEDIFF is a newer different idea.
// This is when in OUTLINE mode, and it goes through the whole LIST of
// differences, and writes EACH to the difference file. It was built to
// replace the simple dialog yes/no box previously used.
//
///////////////////////////////////////////////////////////////////////////////
VOID
compitem_savediff(COMPITEM ci, LPTSTR pDiffName, UINT options)
{
   BOOL     bOK;
   PTARGS   pta;
   PDDIFF   pdd;
   LPTSTR   lpf;
   LPTSTR   pstr;
   DWORD    dwo = options;
   //DWORD dwo = gdwDiffOpts;   // extract current DIFF Options
   if( !ci )
      return;

   pta = (PTARGS)gmem_get( hHeap, sizeof(TARGS), "compitem_savdiff" );
   if( !pta )
      return;

   ZeroMemory(pta, sizeof(TARGS));
   pta->ta_uSize   = sizeof(TARGS);
#ifdef   COMBARGS
   pta->ta_sTA.view = current_view;
#else // !#ifdef   COMBARGS
   pta->ta_pView   = current_view;
#endif   // #ifdef   COMBARGS
   pta->ta_iCallID = taCID_DifDlg;

   pstr = compitem_getfilename(ci, CI_LEFT);
   //if( pstr = dir_getroot_list(cl->left) )
   //fdl = compitem_getleftfile(ci);
   //if(fdl)
   if(pstr)
   {
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
      strcpy( lpf, pstr );   // fill in LEFT name
      compitem_freefilename(ci, CI_LEFT, pstr);
      //dir_freeroot_list(cl->left, pstr);
      if( !dir_isvalidfile2( lpf, &pta->ta_sFDL ) )
         goto Exit_Save;
   }
   else
      goto Exit_Save;

   //if( pstr = dir_getroot_list(cl->right) )
   //fdr = compitem_getrightfile(ci);
   pstr = compitem_getfilename(ci, CI_RIGHT);
   //if(fdr)
   if(pstr)
   {
      //dir = file_getdiritem(fdr);
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
      strcpy( lpf, pstr);   // fill in RIGHT name
      compitem_freefilename(ci, CI_RIGHT, pstr);
      //dir_freeroot_list(cl->right, pstr);
      if( !dir_isvalidfile2( lpf, &pta->ta_sFDR ) )
         goto Exit_Save;
   }
   else
      goto Exit_Save;

   pdd = &pta->ta_sDI;

   pdd->di_sCL = 0;
   pdd->di_sCI = ci;
   pdd->di_dwOpts = dwo;   // store the REQUESTED options (for DIALOG display and return)
   lpf = &pdd->di_szFile[0];  // set pointer to FILE NAME
   if( pdd->di_dwWidth == 0 )
   {
      if( gdwWrapWid == 0 )
         pdd->di_dwWidth = DEF_WRAP_WIDTH;
      else
         pdd->di_dwWidth = gdwWrapWid;
   }

   if( ( pDiffName  ) &&
       ( *pDiffName ) )
   {
      //strcpy( &pdd->di_szFile[0], pDiffName );
      strcpy( lpf, pDiffName );
      // This needs to be FIXED here, but NOT presently used
      //if( LOWORD(options) || ( dwo & INCLUDE_LINENUM ) )
      //   pdd->di_bIncLn = TRUE;
      //else
      //   pdd->di_bIncLn = FALSE;
      //pdd->di_bDiff  = HIWORD(options);
      bOK = TRUE;
   }
   else // if( pDiffName == NULL )
   {
      if( !(*lpf) )
      {
         //sprtf( "ADVICE: No current pdd->di_szFile[] so using global"MERO
         //   "\t[%s] in gszDifFil!"MEOR, gszDif2Fil );
         //GetModulePath(lpf);
         strcpy(lpf, gszDif2Fil);
         if( !(*lpf) )
         {
            GetModulePath(lpf);
            strcat(lpf, "TEMPD001.TXT");
         }
         if( *lpf )
            GetNxtDif(lpf);   // bump to NEXT non-existant file
      }

      if( Do_DIFF_Dlg( pta ) == IDOK )  // put up IDD_DLG_DIFF
         bOK = TRUE;
      // later maybe change to IDD_SAVEDIFF, but we shall see.
      // Actually they are now ALMOST the SAME - July 2001
   }

   if( bOK )
   {
      //if( g_pDifName )
      //   compitem_freediffname( g_pDifName );

      //g_pDifName = compitem_getdiffname( ci, FALSE, pta );
      //g_pDifName = compitem_getdiffname( ci, INC_ALLXSAME, pta );

      // TM_WRITEDIFF
      //SendMessage( hwndRCD,
		//	TM_WRITEDIFF,
		//	0,
      //   (LPARAM)pta );
// case 'S': ta->diffopts |= INCLUDE_SAME;
// case 'L': ta->diffopts |= INCLUDE_LEFTONLY;
// case 'R': ta->diffopts |= INCLUDE_RIGHTONLY;
// case 'D': ta->diffopts |= INCLUDE_ALLDIFF;   // 3 bits newer/older
// case 'M': ta->diffopts |= INC_ALLMOVE;
// case 'N': ta->diffopts |= INCLUDE_LINENUMS;
// case 'T': ta->diffopts |= INCLUDE_TAGS;
// case 'A': ta->diffopts |= APPEND_FILE;
// case 'H': ta->diffopts |= INCLUDE_HEADER;
      //LPTSTR   fname =  &pta->ta_sDI.di_szFile[0];
      //DWORD    dwOpts = INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY|INCLUDE_ALLDIFF|INC_ALLMOVE;
      //dwOpts |= INCLUDE_LINENUMS|INCLUDE_TAGS|APPEND_FILE|INCLUDE_HEADER;
      //if( !pta->ta_sDI.di_bIncLn )
      //   dwOpts &= ~(INCLUDE_LINENUMS);
      //if( !pta->ta_sDI.di_bAppend )    // append to output file (else CREATE NEW)
      //   dwOpts &= ~(APPEND_FILE);
      //COMPITEM ci = view_getcompitem(view);
      //DWORD    dwo = INX_ALLXSAME;
      //         if( compitem_settargs( ci, pta ) )
      // bRet += compitem_writediff( ci, &h, dwo, pta );

      dwo = pta->ta_sDI.di_dwOpts;
      if( compitem_wrtdiff2name( lpf, ci, dwo, pta ) )
      {
         //LPTSTR   lpb = &gszTmpBuf[0];
         // if a SUCCESSFUL file write, then UPDATE
         // various INI items.
         //*lpb = 0;
         if( dwo != gdwDiffOpts )
         {
            sprtf( "Updated INI gdwDiffOPts to %#x."MEOR, dwo );
            gdwDiffOpts = dwo;
            bChgDO = TRUE;
         }

         Add2SList( &gsDiffList, &bChgDLst, lpf );
         // gszDif2Fil / bChgDF
         //{ szOpt, szDF,   it_String, &gszDif2Fil[0],          &bChgDF,  0, 0 },
         if( strcmp( lpf, gszDif2Fil ) )
         {
            strcpy( gszDif2Fil, lpf );
            bChgDF = TRUE;
         }
         if( gdwWrapWid != pdd->di_dwWidth )
         {
// gdwWrapWid / bChgWW
//TCHAR szWW[]  = "WrapWidth";
            gdwWrapWid = pdd->di_dwWidth;
            bChgWW = TRUE;
         }
      }
   }

Exit_Save:

   gmem_free( hHeap, (LPSTR)pta, sizeof(TARGS), "compitem_savdiff" );
}


// BIG WORK ADDITION - Like ToExpand() I want to immediately EXPAND this item,
// compare the files LINE by LINE, and only then decide if they are DIFFERENT
// like ci_compare(), and like view_expand(PVIEW view, long row)
// remember the compitem we are expanding view->iSelect = row;
// get the COMPITEM itself ci = view->pItems[row];
// Put the view->ciSelect = ci; bRet = view_expand_item(view, ci);
// What does view-expand-item do? A LOT
// BOOL view_expand_item(PVIEW view, COMPITEM ci) {
// LIST li; SECTION sh; LINE line1, line2; int i, base_left, base_right, state;
// get the composite section list li = compitem_getcomposite(view->ciSelect);
// then view->bExpand = TRUE; and view_freemappings(view);
// loop through totalling the lines in sections that we should include
// view->rows = 0;
// for (sh = (SECTION) List_First(li); sh != NULL; sh = (SECTION) List_Next(sh)) {
// state = section_getstate(sh);
// if (expand_mode == IDM_RONLY) and state is STATE_LEFTONLY or STATE_MOVEDLEFT, continue;
// else if (expand_mode == IDM_LONLY) and state rightonly or movedright, continue
// else include all lines in this section view->rows += section_getlinecount(sh);
// Then allocate the memory for the mapping array 
// with view->pLines = (PVIEWLINE) gmem get(hHeap, view->rows * sizeof(VIEWLINE));
// loop through the sections again filling in the mapping array 
// i = 0; view->maxtag = 5; view->maxrest = 0;
// for (sh = (SECTION) List_First(li); sh != NULL; sh = (SECTION) List_Next(sh)) {
// state = section_getstate(sh); if RONLY or LONLY and state matches, continue
// else find the base line number in each file 
//                base_left = section_getleftbasenr(sh);
//                base_right = section_getrightbasenr(sh);
// Then add each line in section to the view. section_getfirst()
// returns us to a handle that is in a list. We can
// call List_Next and will eventually get to the
// line returned by section_getlast(). Sections always have at least one line
//     line1 = section_getfirstline(sh);
//     line2 = section_getlastline(sh);
//     for (; line1 != NULL; line1 = (LINE) List_Next(line1)) {
//                      view->pLines[i].line = line1;
//                      view->pLines[i].section = sh;
// calculate the line number for this line by
// incrementing the base nr for this section
// view->pLines[i].nr_left = base_left;
// if (base_left != 0) base_left++;
// view->pLines[i].nr_right = base_right;
// if (base_right != 0) base_right++;
// increment index into view i++;
// check the column widths view->maxrest = max(view->maxrest,
//                    (line_gettabbedlength(line1, 8)));
// if end of section ? like if (line1 == line2) break;
// We must NOT hold a critical section here as SendMessage may hang
//        ViewLeave();
// inform table window of revised mapping 
// SendMessage(view->hwnd, TM_NEWLAYOUT, 0, (DWORD) view);
// return(TRUE); }
// That would NOW paint the EXPANDED VIEW - but I do NOT actually NEED a paint
// Do my difference count, then DESTROY IT ALL
// any questions?
// NEW TABLE LAYOUT would
//   case TM_NEWLAYOUT:
//           if (ptab != NULL) {
//                   gtab_deltable(hwnd, ptab);
// then if ( (ptab = gtab_buildtable(hwnd, lParam)) != NULL) {
// and InvalidateRect(hwnd, NULL, TRUE);    // cause a PAINT of the table changes
// When we go back to OUTLINE -
// INT view_outline_opt(PVIEW view, BOOL bRedraw) {
//   int      prev_row = -1;      /* the row nr of the previously-expanded row*/
//   int      i;                  /* nr of includable items */
//   LIST     li; COMPITEM ci; int      state; TableSelection select;
// check that view_setcomplist has already been called. if not, nothing to do
//   if(view->cl == NULL) return 0;
// ViewEnter();
// clear the mode flag and free up memory associated with expand mode
//   view->bExpand = FALSE;
//   view_freemappings(view);
// traverse the list of compitems counting up the number of includable items
// li = complist_getitems(view->cl);
//   ci = (COMPITEM) List_First(li);
//   dwo = outline_include;
//   for( i = 0; ci != NULL; ci = (COMPITEM) List_Next(ci) ) {
//      state = compitem_getstate(ci);
//      if( ( (dwo & INCLUDE_SAME) && (state == STATE_SAME)               ) ||
//          ( (dwo & INCLUDE_DIFFER) && (state == STATE_DIFFER)           ) ||
//          ( (dwo & INCLUDE_LEFTONLY) && (state == STATE_FILELEFTONLY)   ) ||
//          ( (dwo & INCLUDE_RIGHTONLY) && (state == STATE_FILERIGHTONLY) ) )
//      { i++;  // count a 'displayable' item } }
// allocate an array big enough for all of these
//   view->pItems = (COMPITEM FAR *) gmem get(hHeap, i * sizeof(COMPITEM));
// view->rows = i;
INT
compitem_exact( COMPITEM ci )
{
   INT      iRet = STATE_SAME;
   LIST     li = compitem_getcomposite(ci);  // get the LIST
   SECTION  sh;

   if(li)
   {
        for (sh = (SECTION) List_First(li); sh != NULL; sh = (SECTION) List_Next(sh))
        {
           if( section_getstate(sh) != STATE_SAME )
           {
              iRet = STATE_DIFFER;  // any difference == DIFFERENT
              break;
           }
        }
   }
   else
   {
      chkme( "CHECKME: Failed to get LIST!" );
   }

   compitem_discardsections(ci);

   return iRet;
}


BOOL AddFileTxt( LPTSTR lpb, LPTSTR pfn, PWIN32_FIND_DATA  pfd1, LPSYSTEMTIME pst1 )
{
   DWORD             dwi = 0;
   //LONG              lg;
   LARGE_INTEGER     li1;
   BOOL              b1;

   b1 = FALSE;
   if( dir_isvalidfile2( pfn, pfd1 ) )
   {
      // TIME TROUBLE - This is NOT sufficient
      //b1 = FileTimeToSystemTime( &pfd1->ftLastWriteTime, pst1 );
      b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
      if(b1)
      {
         li1.LowPart  = pfd1->nFileSizeLow;
         li1.HighPart = pfd1->nFileSizeHigh;
         strcat(lpb, GetI64StgRLen( &li1, MI64LEN ) );
         strcat(lpb," ");
         AppendDateTime( lpb, pst1 );
      }
   }
   return b1;
}


#define  WF(b)\
   dwi = strlen(b);\
   dww = 0;\
   if( ( WriteFile( fh, b, dwi, &dww, NULL ) ) &&\
       ( dwi == dww ) ) { \
       dwr += dwi;\
   } else { \
      CloseHandle(fh);\
      *pfh = INVALID_HANDLE_VALUE;\
      return 0;\
   }


DWORD GetGoodBreak( LPTSTR text, DWORD dws, DWORD dwhl )
{
   DWORD    dwjj;
   DWORD    dwkk = 0;
   LPTSTR   ptmp;
   DWORD    dwh = dws / 2;    // half the size
   for( dwjj = (dws-dwhl); dwjj > 0; dwjj-- )
   {
      ptmp = &text[dwjj];
      //if( text[dwjj] <= ' ' )
      if( *ptmp <= ' ' )
      {
         // good BREAK line point
         break;
      }
      else if( dwkk >= dwh )     // ( dws / 2)
      {
         // we have searched back HALF of the WRAP length
         if( dwkk == dwh )
         {
            DWORD dwn;
            // on first such, again look for a different break
            //for(dwn = ((dws / 2) - 1); dwn > 0; dwn--)
            for( dwn = (dws-dwhl); dwn > dwh; dwn-- )
            {
               ptmp = &text[dwn];
               if( !ISALPHANUM(*ptmp) )
               {
                  // nearly as good a break point
                  dwjj = dwn;
                  dwn = 0;
                  break;
               }
            }
            if( dwn == 0 )
               break;
         }
         else if( !ISALPHANUM(*ptmp) ) // text[dwjj] ) )
         {
             break;
         }
      }
      dwkk++;
   }
   return dwjj;
}

#define  ADDPAD(p,l) while( strlen(p) < l )strcat(p," ")

DWORD  WriteDiffHdr( PTARGS pta, HANDLE * pfh, PDWORD pdwr, PDWORD pdwOpts )
{
   LPTSTR   lpb;
   DWORD    dwi, dww;
   HANDLE   fh = *pfh;
   BOOL     b1, b2;
   DWORD    dwr = *pdwr;
   DWORD    dwOpts = *pdwOpts;
   LPTSTR   lpf;
   if(pta)
   {
      lpb = &gszTmpBuf[0];
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szFirst[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szFirst[0];
#endif   // #ifdef   COMBARGS
      sprintf(lpb, "// Compare [%s]"MEOR, lpf );
      WF(lpb);
      b1 = b2 = FALSE;
      if( dwOpts & INCLUDE_HEADER )
      {
         strcpy(lpb, "// Size: ");
         b1 = AddFileTxt( lpb, lpf, &g_sFDL, &g_sST1 );
         if(b1)
         {
            strcat(lpb, MEOR);
            WF(lpb);
         }
      }
#ifdef   COMBARGS
      lpf = &pta->ta_sTA.szSecond[0];
#else // !#ifdef   COMBARGS
      lpf = &pta->ta_szSecond[0];
#endif   // #ifdef   COMBARGS
      sprintf(lpb, "// With    [%s]"MEOR, lpf );
      WF(lpb);
      if( dwOpts & INCLUDE_HEADER )
      {
         strcpy(lpb, "// Size: ");
         b2 = AddFileTxt( lpb, lpf, &g_sFDR, &g_sST2 );
         if(b2)
         {
            strcat(lpb, MEOR);
            WF(lpb);
            if(b1)
            {
               LONG lg = CompareFileTime( &g_sFDL.ftLastWriteTime, &g_sFDR.ftLastWriteTime );
               strcpy(lpb, "// Time: ");
               if( lg == 0 )
                  strcat( lpb, "Same date and time"MEOR );
               else if( lg < 0 )
                  strcat( lpb, "Left is EARLIER - CHECK!"MEOR );
               else
                  strcat( lpb, "Left is newer - Update?"MEOR );
               WF(lpb);
            }
         }
      }
   }
   *pdwr = dwr;
   return dwr;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : compitem_writediff
// Return type: DWORD 
// Arguments  : COMPITEM ci
//            : HANDLE * pfh
//            : DWORD dwOpts
//            : PVOID pv
// Description: Presently called in OUTLINE mode from IDM_WRITEDIFF,
//              after IDD_SAVEDIFF dialog has collected the file name, and
// options for the write. Then EACH file in the outline list is EXPANDED,
// and this call is during that EXPANSION of a list item that has a
// difference.
///////////////////////////////////////////////////////////////////////////////
DWORD  compitem_writediff( COMPITEM ci, HANDLE * pfh, DWORD dwOpts, PVOID pv )
{
   DWORD    dwr = 0; // data written to the file
   PTARGS   pta = (PTARGS)pv;
   BOOL     bDnHd = FALSE;
   LPTSTR   lpb  = GetNxtBuf(); // &gszTmpBuf[0];
   LPTSTR   text = GetNxtBuf(); // &gszTmpBuf2[0];
   LPTSTR   lpt;
   LINE     line;
   LPTSTR   tag;
   SECTION  sec;
   int      state;
   UINT     lnnum;
   DWORD    dwi, dww, dws, dwhl, dwtlen;
   HANDLE   fh = 0;
   //BOOL     b1, b2;
   BOOL     bDnSep;
   BOOL     bDoWrap;

   if(pfh)
      fh = *pfh;

   if( !VFH(fh) )
      return 0;

   dwi = (dwOpts & ~(INC_OTHERS));  // get 'actual' options - ie not cosmetic
   if( dwi == 0 )
      return 0;    // no options ON

   /* make sure the composite list has been built */
   if( ci->secs_composite == NULL )
   {
      ci_makecomposite(ci);
   }

   dws = pta->ta_sDI.di_dwWidth;
   bDoWrap = FALSE;
   if( ( dwOpts & WRAP_LINES ) &&
       ( dws > 32 ) && ( dws < (MXTMPBUF - 2)) )
   {
      bDoWrap = TRUE;
   }
   /* write out every line in every section on the composite
    * list to the temp file.
    */
   bDnSep = FALSE;
   List_TRAVERSE(ci->secs_composite, sec)
   {
      /* get the tag field based on the section state*/
      state = section_getstate(sec);
      tag   = 0;  // start with NO tag text
      switch(state)
      {
      case STATE_SAME:
              if( dwOpts & INCLUDE_SAME ) // if option on
                 tag = "    ";            // set tag pointer
              else
                 bDnSep = FALSE;
              break;

      case STATE_LEFTONLY:
              if( dwOpts & INCLUDE_LEFTONLY )
                 tag = " <! ";
              else
                 bDnSep = FALSE;
              break;

      case STATE_RIGHTONLY:
              if( dwOpts & INCLUDE_RIGHTONLY )
                 tag = " !> ";
              else
                 bDnSep = FALSE;
              break;

      case STATE_MOVEDLEFT:
              if( dwOpts & INCLUDE_MOVELEFT )
                 tag = " <- ";
              else
                 bDnSep = FALSE;
              break;

      case STATE_MOVEDRIGHT:
              if( dwOpts & INCLUDE_MOVERIGHT )
                 tag = " -> ";
              else
                 bDnSep = FALSE;
              break;
      }

      if( !tag )     // if no tag pointer after switch
         continue;   // continue to next line

      /* write out each line in this section.
       * non-standard traverse of list as we only
       * want to go from section first to section last
       * inclusive.
       */

      for(line = section_getfirstline(sec); line != NULL; line = List_Next(line) )
      {
         lpt = line_gettext(line);
         if(lpt)
            strcpy(text,lpt);
         else
            *text = 0;
         dwtlen = RTrimIB(text);  // and remove any Cr/Lf stuff from END
         lnnum = line_getlinenr(line);
         if( !bDnHd )
         {
            WriteDiffHdr( pta, pfh, &dwr, &dwOpts );
            bDnHd = TRUE;  // Header is only once
         }

         if( !bDnSep )
         {
            WF("====================================================="MEOR);
            bDnSep = TRUE;
         }

         /* write out to file */
         *lpb = 0;

         if( dwOpts & INCLUDE_LINENUMS )
            sprintf(EndBuf(lpb), "%5d ", lnnum);

         if( dwOpts & INCLUDE_TAGS )
            sprintf(EndBuf(lpb), "%s", tag);

         //if( dwOpts & WRAP_LINES )
         dwhl = strlen(lpb);  // get HEADER length
#ifdef   ADDLNWRAP
         if( bDoWrap )
         {
            if( (dwhl + dwtlen) > (dws + 4) )
            {
               DWORD dwii, dwjj, dwkk;
               dwii = strlen(text);    // get text length
               dwkk = 0;
               dwjj = GetGoodBreak( text, dws, (dwhl+dwkk) );
               if(dwjj)
               {
Next_block:
                  //strcat(lpb,text);
                  strncat(lpb,text,dwjj);    // only copy part of line
                  lpb[dwhl+dwjj+dwkk] = 0;   // ensure ZERO termination
                  text = &text[dwjj];        // get next text pointer
                  EnsureCrLf2(lpb);          // add Cr/Lf
                  WF(lpb);                   // write it out
                  *lpb = 0;                  // restart buffer
                  ADDPAD(lpb, dwhl);
                  strcat(lpb, "w ");
                  dwkk = 2;
                  dwtlen = strlen(text);
                  if( (dwhl + dwkk + dwtlen) > (dws + 4) )
                  {
                     dwjj = GetGoodBreak( text, dws, (dwhl+dwkk) );
                     if(dwjj)
                        goto Next_block;
                  }
               }
            }
         }
         else
#endif   // #ifdef   ADDLNWRAP
         {
            if( (dwhl + dwtlen) >= (MXTMPBUF - 2) )
            {
               chkme( "WARNING: This is unchecked coding!!!"MEOR );
               if( *lpb )
               {
                  WF(lpb);
                  *lpb = 0;
               }
               while( strlen(text) > (MXTMPBUF-2) )
               {
                  strncpy(lpb, text, (MXTMPBUF-2) );
                  lpb[MXTMPBUF-2] = 0;
                  WF(lpb);
                  text = &text[MXTMPBUF-2];
               }
            }
         }
         
         strcat(lpb, text);

         EnsureCrLf2(lpb);

         WF(lpb);

         if( line == section_getlastline(sec) )
            break;
      }
   }

   if(dwr)
   {
      lpb = MEOR;
      WF(lpb);
   }
   else  // we WROTE nothing
   {
         if( !bDnHd )
         {
            WriteDiffHdr( pta, pfh, &dwr, &dwOpts );
            bDnHd = TRUE;  // Header is only once
         }

         strcpy(lpb, "No difference data with these options."MEOR );
         WF(lpb);

   }

   return dwr;
}


BOOL  compitem_wrtdiff2name( LPTSTR fname, COMPITEM item, DWORD dwOpts, PVOID pv )
{
   HANDLE   fh;
   DWORD    dwr;

   if( ( item == NULL ) ||
       ( dwOpts == 0  ) )
   {
      return   FALSE;
   }

   //like case CI_COMP:
   fh = OpnFil( fname, ( ( dwOpts & APPEND_FILE ) ? TRUE : FALSE ) );
   //fh = OpenFile(fname, &os, OF_READWRITE|OF_SHARE_DENY_NONE);
   //if(fh <= 0)
   if( !VFH(fh) )
   {
      //MB(NULL, LoadRcString(IDS_CANT_OPEN_TMP_FILE), NULL, MB_OK | MB_ICONSTOP);
      return   FALSE;
   }

   dwr = compitem_writediff( item, (HANDLE *)&fh, dwOpts, pv );

   /* now close the file and return its name */
   //_lclose(fh);
   CloseHandle(fh);

   return( dwr ? TRUE : FALSE );

}

BOOL  compitem_wrtdiff2name_NOT_USED( LPTSTR fname, COMPITEM item, DWORD dwOpts, PVOID pv )
{
   OFSTRUCT os;
   int      fh;
   DWORD    dwr;

   if( ( item == NULL ) ||
       ( dwOpts == 0  ) )
   {
      return   FALSE;
   }

   //like case CI_COMP:
   fh = OpenFile(fname, &os, OF_READWRITE|OF_SHARE_DENY_NONE);
   if(fh <= 0)
   {
      //MB(NULL, LoadRcString(IDS_CANT_OPEN_TMP_FILE), NULL, MB_OK | MB_ICONSTOP);
      return   FALSE;
   }

   dwr = compitem_writediff( item, (HANDLE *)&fh, dwOpts, pv );

   /* now close the file and return its name */
   _lclose(fh);

   return( dwr ? TRUE : FALSE );

}

LPTSTR compitem_getdiffname( COMPITEM item, DWORD dwOpts, PVOID pv )
{
   LPTSTR   fname;
   //OFSTRUCT os;
   //int      fh;

   if( ( item == NULL ) ||
       ( dwOpts == 0  ) )
   {
      return(NULL);
   }

   //like case CI_COMP:

   /* caller has asked for the filename of the DIFFERENCE file.
    * we need to create a temporary file and write the
    * lines in the composite section list out to it.
    */
   fname = gmem_get(hHeap, MAX_PATH, "compitem_getdiffname" );
   GetTempPath(MAX_PATH, fname);
   GetTempFileName(fname, "dc4", 0, fname);  // ask system for unique file name

   if( !compitem_wrtdiff2name( fname, item, dwOpts, pv ) )
   {
      gmem_free(hHeap, fname, MAX_PATH, "compitem_getdiffname" );
      fname = 0;
   }

   return(fname);

}

VOID  compitem_freediffname(LPTSTR filename)
{
   OFSTRUCT os;

   if( !filename )
      return;

   // like case CI_COMP:
   /* this is a temporary file we created. Delete it. */
   OpenFile(filename, &os, OF_DELETE);

   gmem_free(hHeap, filename, MAX_PATH, "compitem_getdiffname" );

}



///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : compitem_retfullname
// Return type: VOID 
// Arguments  : LPTSTR lpb
//            : COMPITEM ci
//            : DWORD dwo
// Description: Copy the FULL FILE NAME into the buffer supplied
//              If just one, left or right, then ONLY one returned
// and if exactly the SAME, which they should NEVER be, then only ONE
// ELSE if BOTH, then return the name per the OPTION flag
///////////////////////////////////////////////////////////////////////////////
VOID compitem_retfullname( LPTSTR lpb, COMPITEM ci, DWORD dwo )
{
   PSTR     str1, str2;
   DIRITEM  di1, di2;

   if( ci == 0 )
   {
      *lpb = 0;
      return;
   }

   di1 = file_getdiritem(ci->left);
   di2 = file_getdiritem(ci->right);

   str1 = dir_getfullname(di1);
   str2 = dir_getfullname(di2);

   /* If only one file - set name to that */
   if( ( ci->left == NULL ) && ( ci->right == NULL ) )
   {
      strcpy(lpb, "<both nul>");
   }
   else if( ci->left == NULL )
   {
      strcpy(lpb, str2);   // just return RIGHT
   }
   else if( ci->right == NULL )
   {
      strcpy(lpb, str1);   // just return LEFT
   }
   else  // WE HAVE BOTH NAMES
   {
      if( strcmpi(str1, str2) == 0 )
      {
         strcpy(lpb, str1);   // if SAME just return one
      }
      else  // they are DIFFERENT, as they should ALWAYS be
      {
         // then per the OPTION flag
         if( dwo & FLEFT_NAME )
            strcpy(lpb, str1);   // just return LEFT
         else if( dwo & FRIGHT_NAME )
            strcpy(lpb, str2);   // just return RIGHT name
         else // if( dwo & COMBINED_NAME )
            sprintf(lpb, "%s : %s", str1, str2);   // else return a COMPOSITE
      }
   }

   dir_freefullname(di1, str1);
   dir_freefullname(di2, str2);

}

INT   compitem_addfullleft( LPTSTR lpb, COMPITEM ci )
{
   INT      i = 0;
   PSTR     str;
   DIRITEM  di;

   if( ( !ci      ) ||
      ( !ci->left ) )
      return i;

   di = file_getdiritem(ci->left);
   if(di)
   {
      str = dir_getfullname(di);
      if(str)
      {
         i = strlen(str);
         if(lpb)
            strcat(lpb, str);
      }

      dir_freefullname(di, str);
   }

   return i;

}

INT   compitem_addfullright( LPTSTR lpb, COMPITEM ci )
{
   INT      i = 0;
   PSTR     str;
   DIRITEM  di;

   if( ( !ci      ) ||
      ( !ci->right ) )
      return i;

   di = file_getdiritem(ci->right);
   if(di)
   {
      str = dir_getfullname(di);
      if(str)
      {
         i = strlen(str);
         if(lpb)
            strcat(lpb, str);
      }

      dir_freefullname(di, str);
   }

   return i;

}

VOID
compitem_retrelname( LPTSTR lpb, COMPITEM ci )
{
   PSTR  str1, str2;
   if( ci == 0 )
   {
      *lpb = 0;
      return;
   }
   str1 = dir_getrelname(file_getdiritem(ci->left));
   str2 = dir_getrelname(file_getdiritem(ci->right));
   /* If only one file - set name to that */
   if( ( ci->left == NULL ) && ( ci->right == NULL ) )
   {
      strcpy(lpb, "<nul>");
   }
   else if( ci->left == NULL )
   {
      strcpy(lpb, str2);
   }
   else if( ci->right == NULL )
   {
      strcpy(lpb, str1);
   }
   else
   {
      if( strcmpi(str1, str2) == 0 )
      {
         strcpy(lpb, str2);
      }
      else
      {
         sprintf(lpb, "%s : %s", str1, str2);
      }
   }

   dir_freerelname(file_getdiritem(ci->left),  str1);
   dir_freerelname(file_getdiritem(ci->right), str2);

}

// eof - compitems.c
