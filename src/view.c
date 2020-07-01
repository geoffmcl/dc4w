
/****************************** Module Header *******************************
* Module Name: VIEW.C
*
* Maps rows in window to items in COMPLIST
*
* Functions:
*
* view_new()
* view_setcomplist()
* view_getcomplist()
* view_close()
* view_delete()
* view_outline()
* view_expand()
* view_gettext()
* view_getlinenr_left()
* view_getlinenr_right()
* view_getwidth()
* view_getrowcount()
* view_getstate()
* view_getitem()
* view_isexpanded()
* view_getcurrenttag()
* view_newitem()
* view_changeviewoptions()
* view_changediffoptions()
* view_findchange()
* view_outline_opt()
* view_freemappings()
* view_findrow()
* view_expand_item()
* NEW
* INT  view_haschange( PVIEW view, BOOL bCnt );
* BOOL  view_haszip( PVIEW view );
*
* Comments:
*
* A view owns a COMPLIST, and talks to a table window. The table window
* shows 3 columns: line nr, tag and text. We also need to supply a state
* for each row (used to select colour scheme).
*
* The COMPLIST can give us a list of its COMPITEMs. Each of these can give
* us a tag (eg the filenames compared) and the text (usually the compare
* result), and the state. We make the line number from the
* COMPITEM's place in the list.
*
* If we are asked to switch to 'expand' mode, we ask the selected COMPITEM
* for its composite section list. We can then get the state (and thus
* the tag) from each SECTION, and the line nr and text from the LINEs within
* each section.
*
* When moving between expand and outline, and when refreshing the view
* for some option change, we have to be careful to keep the current row
* and the selected row in the table what the user would expect.
*
* Functions in this module can be called from the UI thread (to refresh
* the display) and simultaneously from a worker thread to update the
* view mapping (view_setcomplist, view_newitem). We use a critical section
* to manage the synchronisation. We need to protect all access/modification
* to the view structure elements (particularly bExpand, rows, pLines and
* pItems), BUT we must not hold the critical section over any calls
* to SendMessage.
*
* We use the global options in dc4w.h, and we allocate memory from the
* heap hHeap which has been initialised elsewhere. Points in time-intensive
* loops call Poll() defined elsewhere.
*
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"
//#include  "view.h"
#define  DBG_LOG1 // sprtf

extern   INT   complist_count;
extern   BOOL  bChgCLst;
extern   VOID  SetOutlineStatus( PVIEW view );
extern   VOID  SetExpandStatus( PVIEW view );
extern   BOOL  g_bNeedPaint;  // set when NOTHING to display - paint coloured screen
extern   BOOL  g_bColourCycle;   // advise timer to put up colour display
extern   VOID  compitem_setmaxlen( COMPITEM ci, PINT ptl, PINT prest );
extern   VOID  SetAppTitle( VOID );
extern   DWORD g_dwLVCount;

/*
 * data structures
 */

/* in expand mode, we keep an array of one of these per screen line. */
typedef struct viewline {

   LINE line;              /* handle to LINE for this row */
   SECTION section;        /* handle to section containing this line */
   int nr_left;            /* line nr in left file */
   int nr_right;           /* line nr in right file */

} VIEWLINE, FAR * PVIEWLINE;


/*
 * The users PVIEW handle is in fact a pointer to this structure
 */
struct viewstr {

   HWND     hwnd;          /* the table window to send notifies to */
   COMPLIST cl;            /* the complist that we own */
   BOOL     bExpand;       /* true if we are in expand mode */
   BOOL     bRefresh;   // one time REFRESH view flag
   BOOL     bRecursive; // keep the fact of RECURSIVE or not
   INT      iSelect;    // ROW in table used for expansion
   COMPITEM ciSelect;      /* selected compitem (in expand mode) */
   int      v_iRows; // number of 'displayable' rows in the outline file list of this view
   int      v_iTot;  // total in FILE LIST (COMITEMS) of this VIEW

   OUTSTATS v_sOutStats;   // OUTLINE counters

   char     nrtext[32];    /* we use this in view_gettext for the line
                            * number column. overwritten on each call
                            */
   int      maxtag, maxrest;/* column widths in characters for cols 1, 2 */
   /* if we are in outline mode, we map the row number to one entry
    * in this array of COMPITEM handles. this pointer will
    * be NULL in expand mode
    */
   COMPITEM * pItems;

   /* in expand mode we use this array of line and section handles */
   PVIEWLINE pLines;

};

DWORD g_dwviewedcnt = 0;   // files viewed during this compitem

CRITICAL_SECTION CSView;
static BOOL bDoneInit = FALSE;

#define ViewEnter()     EnterCriticalSection(&CSView);
#define ViewLeave()     LeaveCriticalSection(&CSView);

INT   view_outline_opt(PVIEW view, BOOL bRedraw);

void  view_freemappings(PVIEW view);
int   view_findrow(PVIEW view, int number, BOOL bRight);
BOOL  view_expand_item(PVIEW view, COMPITEM ci);


/***************************************************************************
 * Function: view_new
 *
 * Purpose:
 *
 * Create a new view. At this point, we are told the table window handle,
 * and nothing else.
 *
 */
PVIEW view_new( HWND hwndTable )
{
   PVIEW view;

   if( !bDoneInit )
   {
           InitializeCriticalSection(&CSView);
           bDoneInit = TRUE;
   }

   /* alloc the view from the heap */
   view = (PVIEW) gmem_get(hHeap, sizeof(struct viewstr), "view_new" );
   if(view)
   {

      ZeroMemory( view, sizeof(struct viewstr) );
   
      /* set the default fields */
      view->hwnd     = hwndTable;
//      view->cl       = NULL;
//      view->bExpand  = FALSE;
//      view->bRefresh = FALSE;
//      view->ciSelect = NULL;
//      view->rows     = 0;
//      view->pItems   = NULL;
//      view->pLines   = NULL;
      g_bListSave = 0; //	W.ws_bListSave // g_ INT
      g_dwUseFlag = 0;  // clear all save actions

#ifdef ADD_LIST_VIEW
      CLEARLISTVIEW; // clear the LIST VIEW
#endif // #ifdef ADD_LIST_VIEW

   }

   g_sCFDLGSTR.cf_pView = view;  // update (another) global structure

   return(view);

}

VOID   Add2CompList( PVIEW view, LPTSTR both )
{
   PLE   ph, pn;
   BOOL  bAdd = TRUE;
   INT   i    = 0;

   ph = &gsCompList;
   TrimIB(both);     // remove leading and trailing SPACES (if any)

   Traverse_List( ph, pn )
   {
      if( strcmpi( both, (LPTSTR)((PLE)pn + 1) ) == 0 )
      {
         // found it alread in the LIST
         bAdd = FALSE;  // do NOT add it
         if(i)    // if it is NOT already the first
         {
            RemoveEntryList(pn);    // remove it from the LINKED LIST
            InsertHeadList(ph,pn);  // in INSERT at the HEAD of the LIST
            bChgCLst = TRUE;  // set CHANGE of the LIST
         }
         break;
      }
      i++;
   }

   if( bAdd )
   {
      // add this NEW list at the HEAD
      pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(both) + 1) );
      if(pn)
      {
         strcpy( (LPTSTR)((PLE)pn + 1), both ); // add the string
         InsertHeadList(ph,pn);  // and insert it at the head
         bChgCLst = TRUE;  // and SET change for INI write
      }
   }
}

//VOID  view_updatepcfds( PVOID pv, PVIEW view )
//{
//   PCFDLGSTR pcfds = (PCFDLGSTR)pv;
//   if( !pv )
//      pcfds = &g_sCFDLGSTR;
   
   //pcfds->bExpanded = view_isexpanded( view );
   //pcfds->iSelected = giSelection; // selected row in table (if not -1)
   //pcfds->iSelected = view_getiselect( view ); // selected row in table (if not -1)
   //pcfds->cf_pView     = view;
//}

//VOID  view_initpcfds( PVOID pv, PVIEW view )
//{
//   PCFDLGSTR pcfds = (PCFDLGSTR)pv;
//   view_updatepcfds( pv, view );
//}

/***************************************************************************
 * Function: view_setcomplist
 *
 * Purpose:
 *
 * We have to separate view_new and view_setcomplist because we need
 * to give the view handle to the complist and the complist handle to the
 * view. So do a view_new to create a null view; then complist_new() to
 * which you pass a view handle. The complist will then register itself
 * with the view by calling this function. During the build of the complist,
 * it will also update us by calling view_additem, so that we can refresh
 * the display.
 *
 * Here we should initialise an outline view of the complist.
 *
 * We also talk to the status bar using SetNames to set the names of
 * the two items.
 *
 * And we also KEEP if it is recursive or not - used in dir_scan()
 *
 * NOTE BOTH cl->left and cl->right are VALID before calling here
 *
 */
BOOL
view_setcomplist(PVIEW view, COMPLIST cl, BOOL fDeep)
{
   LPTSTR   left, right, both;
   LPTSTR   lpb1, lpb2;
   INT      ilen;
   PCFDLGSTR   pcfds = &g_sCFDLGSTR;

   if(view == NULL)
      return FALSE;

   /* there can be only one call to this per VIEW */
   if(view->cl != NULL)
      return FALSE;

   ViewEnter();

   view->cl         = cl;     // set the COMPLIST for this view
   view->bRecursive = fDeep;  // keep the recursive flag

   /* set names on status bar to root names of left and right trees */
   // NOTE BOTH cl->left and cl->right are VALID before calling here
   left  = complist_getroot_left(cl);
   right = complist_getroot_right(cl);

   ilen = ( strlen(left) + strlen(right) + 8 );
   both = gmem_get(hHeap, ilen, "view_setcomplist" );

   lpb1 = left;
   lpb2 = right;
   // build full name compare for gsCompList
   sprintf(both, "%s : %s", lpb1, lpb2);
   // now ADD this string to the COMPARE LIST (gsCompList)
   Add2CompList( view, both );

   // but for the status line we may SHORTEN the names
   // if TOO long
   if( (ilen+4) >= SF_MAXLABEL )
   {
      // adjust the message DOWN to maximum LABEL size
      lpb1 = &gszTmpBuf[0];
      lpb2 = &lpb1[((SF_MAXLABEL / 2)+8)];

      CopyShortName(left, lpb1, ((SF_MAXLABEL-8)/2));
      CopyShortName(right,lpb2, ((SF_MAXLABEL-8)/2));
   }

   // build status bar central box static text string
   sprintf(both, " %s : %s  ", lpb1, lpb2);

   ViewLeave();

   // Set the names field - the central box in the status bar 
   // uses SendMessage(hwndStatus, SM_SETTEXT, IDL_NAMES, (DWORD)&AppTitle[0]);
   SetNames(both);


   ViewEnter();

   gmem_free(hHeap, both, ilen, "view_setcomplist" );

   // and NOTE BOTH cl->left and cl->right are VALID
   complist_freeroot_left(cl, left);
   complist_freeroot_right(cl, right);

   ViewLeave();

   //view_initpcfds( pcfds, view );

   complist_setdlgstgs(pcfds, cl);   // ensure dialog_leftname/dialog_rightname are valid

   // this call is done BEFORE match, so don't set disp_number
   //disp_number = view_outline(view);
   view_outline(view);

   return(TRUE);
}


/***************************************************************************
 * Function: view_getcomplist
 *
 * Purpose:
 *
 * Return a handle to the complist owned by this view
 */
COMPLIST
view_getcomplist(PVIEW view)
{
        if (view == NULL) {
                return(NULL);
        }

        return(view->cl);
}

COMPITEM
view_getcompitem(PVIEW view)
{
   if( !view )
      return NULL;
   return view->ciSelect;
}
/***************************************************************************
 * Function: view_close
 *
 * Purpose:
 *
 * Close a view. Notify the table window that this view should be
 * closed. When the table window has finished with it, it will send
 * a TQ_CLOSE notify that should result in view_delete being called
 * and the memory being freed.
 */
void
view_close(PVIEW view)
{
        if (view == NULL) {
                return;
        }

        SendMessage(view->hwnd, TM_NEWID, 0, 0);
}


/***************************************************************************
 * Function: view_delete
 *
 * Purpose:
 *
 * Delete a view and all associated data.
 *
 * This function should only be called in response to the table window
 * sending a TQ_CLOSE message. To close the view, call view_close and
 * wait for the TQ_CLOSE before calling this.
 *
 * We delete the associated COMPLIST and all its associated structures.
 */
void
view_delete(PVIEW view)
{
   if (view == NULL)
   {
      return;
   }

   /* we have two arrays that are used for the mapping - an array
    * of compitem handles in outline mode, and an array of
    * VIEWLINE structures in expand mode
    */

   view_freemappings(view);

   complist_delete(view->cl);

   //gmem_free(hHeap, (LPSTR) view, sizeof(struct view), "view_new" );
   gmem_free(hHeap, (LPSTR) view, sizeof(struct viewstr), "view_new" );
}


/***************************************************************************
 * Function: view_outline
 *
 * Purpose:
 *
 * Build an outline mode mapping where one row represents one COMPITEM in
 * the list. Check the global option flag outline_include to see which items
 * we should include.
 *
 * If we were in expand mode, then set as the selection the row in outline mode
 * that we were expanding. Also remember to free up the expand mode mapping
 * array
 *
 * Once we have built the new mapping, notify the table window to
 * redraw itself.
 *
 * For example a click on the case IDM_INCSAME: menu item would toggle the bit
 * in outline_include ^= INCLUDE_SAME;, fix the MENU, then called
 * view_changeviewoptions(), which if in OUTLINE mode calls here.
 *
 */

INT view_outline(PVIEW view)
{
   if( !view )
      return 0;   // (INT)-1;

   /* all work done by view_outline_opt - this function
    * gives us the option of not updating the display
    */
   return( view_outline_opt(view, TRUE) );
}



/***************************************************************************
 * Function: view_expand
 *
 * Purpose:
 *
 * Switch to expand mode, expanding the given row into a view
 * of the differences in that file.
 *
 * Map the given row nr into a compitem handle, and then
 * call the internal function with that.
 */
#define  IsValidRow(a,b)   ( ( b >= 0 ) && ( b < a->v_iRows ) )

BOOL view_expand(PVIEW view, long row)
{
   COMPITEM ci;
   BOOL bRet;

   ViewEnter();

   if( (view == NULL) || (view->bExpand) )
   {
      /* no view, or already expanded */
      ViewLeave();
      return(FALSE);
   }

   // if (row >= view->v_iRows)
   if( !IsValidRow(view,row) )
   {
      /* no such row */
      ViewLeave();
      return FALSE;
   }

   /* remember the compitem we are expanding */
   view->iSelect = row;

   g_sCFDLGSTR.cf_bExpanded = TRUE;
   g_sCFDLGSTR.cf_iSelected = row;

   // get the COMPITEM itself
   ci = view->pItems[row];

   // Put the view->ciSelect = ci;
   bRet = view_expand_item(view, ci);

   // view_expand_item does the...
   // ViewLeave();
   return(bRet);
}


/***************************************************************************
 * Function: view_gettext
 *
 * Purpose:
 *
 * Return the text associated with a given column of a given row.
 * Return a pointer that does not need to be freed after use - ie
 * a pointer into our data somewhere, not a copy
 */
LPSTR view_gettext(PVIEW view, long row, int col)
{
        int line;
        int state;
        LPSTR pstr;


        if (view == NULL) {
                return (NULL);
        }

        ViewEnter();

        //if (row >= view->v_iRows) {
        if( !IsValidRow(view,row) )
        {
                ViewLeave();
                return(NULL);
        }

        if (view->bExpand)
        {
                /* we are in expand mode */
                
                state = section_getstate(view->pLines[row].section);

                switch(col)
                {
                case 0: /* line number - from left or right file */
                   //   case IDM_NONRS: // this is a display toggle
                   //TCHAR szShwNums[] = "ShowLineNumbers";
                   line = 0;
                   //if( gbShowNums )
                   if( gdwDiffOpts & INCLUDE_LINENUMS )
                   {
                     //   case IDM_LNRS: case IDM_RNRS:
                     //TCHAR szUseRt[] = "UseRightNumbers"; // just the source of the number - if abv ON
                     if(gbUseRight)
                     {
                        line = view->pLines[row].nr_right;
                        if( state == STATE_MOVEDLEFT )
                        {
                           line = -line;
                        }
                     }
                     else
                     {
                        line = view->pLines[row].nr_left;
                        if (state == STATE_MOVEDRIGHT)
                        {
                           line = -line;
                        }
                     }
                   }

#ifdef   USEOLDIDM
                        /* line numbers can be from either original file
                         * this is a menu-selectable option
                         */
                        switch(line_numbers)
                        {
                        case IDM_NONRS:
                                pstr = NULL;
                                break;

                        case IDM_LNRS:
                                line = view->pLines[row].nr_left;
                                if (state == STATE_MOVEDRIGHT) {
                                        line = -line;
                                }
                                break;

                        case IDM_RNRS:
                                line = view->pLines[row].nr_right;
                                if (state == STATE_MOVEDLEFT) {
                                        line = -line;
                                }
                                break;
                        }
#endif   // #ifdef   USEOLDIDM

                        if (line == 0) {
                                ViewLeave();
                                return(NULL);
                        }

                        if (line < 0) {
                                /* lines that are moved appear twice.
                                 * show the correct-sequence line nr
                                 * for the out-of-seq. copy in brackets.
                                 */
                                wsprintf((LPTSTR)view->nrtext, "(%d)", abs(line));
                        } else  {
                                wsprintf((LPTSTR)view->nrtext, "%d", line);
                        }
                        pstr = view->nrtext;
                        break;

                case 1:
                        /* tag text - represents the state of the line */

                        switch(state) {
                        case STATE_SAME:
                                pstr = "    ";
                                break;

                        case STATE_LEFTONLY:
                                pstr = " <! ";
                                break;

                        case STATE_RIGHTONLY:
                                pstr = " !> ";
                                break;

                        case STATE_MOVEDLEFT:
                                pstr = " <- ";
                                break;

                        case STATE_MOVEDRIGHT:
                                pstr = " -> ";
                                break;
                        }
                        break;

                case 2:
                        /* main text - line */
                        pstr = line_gettext(view->pLines[row].line);
                        break;
                }
        } else {
                /* outline mode */
                switch(col) {
                case 0:
                        /* row number - just the line number */
                        wsprintf((LPTSTR)view->nrtext, "%d", row+1);
                        pstr = view->nrtext;
                        break;

                case 1:
                        /* tag */
                        //pstr = compitem_gettext_tag(view->pItems[row]);
                        pstr = compitem_gettext_tags(view->pItems[row]);
                        break;

                case 2:
                        /* result text - this is presently a comment,
                         * then date/time size string */
                        pstr = compitem_gettext_result(view->pItems[row]);
                        break;
                }
        }
        ViewLeave();
        return(pstr);
}

/***************************************************************************
 * Function: view_getlinenr_left
 *
 * Purpose:
 *
 * Return the line number that this row had in the original left
 * file. 0 if not in expand mode. 0 if this row was not in the left file.
 * -(linenr) if this row is a MOVED line, and this is the right file
 * copy
 */
int
view_getlinenr_left(PVIEW view, long row)
{
        int state, line;

        //if ((view == NULL) || (row >= view->v_iRows) || !view->bExpand) {
        if ((view == NULL) || ( !IsValidRow(view,row) ) || !view->bExpand)
        {
                return 0;
        }

        ViewEnter();
        state = section_getstate(view->pLines[row].section);
        line = view->pLines[row].nr_left;
        if (state == STATE_MOVEDRIGHT) {
                line = -line;
        }
        ViewLeave();

        return(line);
}

/***************************************************************************
 * Function: view_getlinenr_right
 *
 * Purpose:
 *
 * Return the line number that this row had in the original right
 * file. 0 if not in expand mode. 0 if this row was not in the right file.
 * -(linenr) if this row is a MOVED line, and this is the left file
 * copy
 */
int
view_getlinenr_right(PVIEW view, long row)
{
        int state, line;

        //if ((view == NULL) || (row > view->v_iRows) || !view->bExpand) {
        if ((view == NULL) || ( !IsValidRow(view,row) ) || !view->bExpand)
        {
                return 0;
        }

        ViewEnter();

        state = section_getstate(view->pLines[row].section);
        line = view->pLines[row].nr_right;
        if (state == STATE_MOVEDLEFT) {
                line = -line;
        }
        ViewLeave();

        return(line);
}


/***************************************************************************
 * Function: view_getwidth
 *
 * Purpose:
 *
 * Find the maximum width in characters for the given column 
 */
int
view_getwidth(PVIEW view, int col)
{
        if (view == NULL) {
                return(0);
        }

        switch(col) {
        case 0:
                /* line nr column - always 5 characters wide */
                return(5);

        case 1:
                /* this is a proportional font field, so add on a margin
                 * for error
                 */
                return(view->maxtag + (view->maxtag / 20));
        case 2:
                /* this now includes the tab expansion allowance */
                return(view->maxrest);
        default:
                return(0);
        }
}

/***************************************************************************
 * Function: view_getrowcount
 * Purpose: How many rows 'displayable' are there in this view ? 
 */
long
view_getrowcount(PVIEW view)
{
        if (view == NULL) {
                return(0);
        }

        return(view->v_iRows);
}

/***************************************************************************
 * Function: view_gettotcount
 * Purpose: How many files (COMPITEMS) are there in this view's LIST ? 
 */
long view_gettotcount(PVIEW view)
{
   if( !view )
      return 0;
   return( view->v_iTot );
}

/***************************************************************************
 * Function: view_getstate
 *
 * Purpose:
 *
 * Return the state for the current row. This is used
 * to select the text colour for the row
 *
 * States for sections are obtained from section_getstate (and apply, and
 * to all lines in that section. States for compitems are obtained
 * from compitem_getstate.
 *
 * Current RETURN is one of -
 * == STATE_SAME, STATE_DIFFER, STATE_FILELEFTONLY, or STATE_FILERIGHTONLY ==
 * OR ZERO (0) if view pointer is NULL, or if the ROW is out of RANGE.
 *
 */
int
view_getstate(PVIEW view, long row)
{
   int state;

   if(view == NULL)
   {
           return(0);
   }

   ViewEnter();
   //if(( row <  0             ) ||
   //   ( row >= view->v_iRows ) )
   if( !(IsValidRow(view,row)) )
   {
           state = 0;
   }
   else if (view->bExpand)
   {
           /* its a line state that's needed */
           state = section_getstate(view->pLines[row].section);
   }
   else  // in OUTLINE
   {
           /* its a compitem state */
           state = compitem_getstate(view->pItems[row]);
   }
   ViewLeave();

   return(state);
}

DWORD view_getflag(PVIEW view, long row)
{
   DWORD dwFlag = 0;

   if(view == NULL)
   {
      return(0);
   }

   ViewEnter();
//   if(( row <  0             ) ||
//      ( row >= view->v_iRows ) )
   if( !(IsValidRow(view,row)) )
   {
      dwFlag = 0;
   }
   else if (view->bExpand)
   {
           /* its a line state that's needed */
           //state = section_getstate(view->pLines[row].section);
           //dwFlag = section_getflag(view->pLines[row].section);
   }
   else  // in OUTLINE
   {
           /* its a compitem state */
           dwFlag = compitem_getflag(view->pItems[row]);
   }
   ViewLeave();

   return dwFlag;
}

/***************************************************************************
 * Function: view_gethandle
 *
 * Purpose:
 *
 * Return a handle to the current compitem. In expand mode,
 * returns the handle to the compitem we are expanding. In outline
 * mode, returns the handle to the compitem for the given row, if valid,
 * or NULL otherwise. row is only used if not in expand mode.
 */
COMPITEM
view_getitem(PVIEW view, long row)
{
   COMPITEM ci;
   if( view == NULL )
   {
      return(NULL);
   }

   ViewEnter();
   if( view->bExpand )
   {  // in EXPANDED
      ci = view->ciSelect;    // we have already put the SELECTED COMPITEM here
   }
   else
   {  // in OUTLINE - Only if the requested ROW is valid
      //if( ( row >= 0 ) && ( row < view->v_iRows ) )
      if( IsValidRow(view,row) )
      {
         ci = view->pItems[row];
      }
      else
      {
         ci = NULL;
      }
   }
   ViewLeave();

   return(ci);
}

/***************************************************************************
 * Function: view_isexpanded
 *
 * Purpose:
 *
 * Return TRUE if the current mapping is expanded mode
 */
BOOL
view_isexpanded(PVIEW view)      
{
        if (view == NULL) {
                return(FALSE);
        }
        return(view->bExpand);
}

/***************************************************************************
 * Function: view_getcurrenttag
 *
 * Purpose:
 *
 * Return a text string describing the view. This is NULL in outline mode,
 * or the tag text for the current compitem in expanded mode
 */
LPSTR
view_getcurrenttag(PVIEW view)
{
        LPSTR str;

        if ((view == NULL) || (!view->bExpand)) {
                return(NULL);
        } else {
                ViewEnter();

                str = compitem_gettext_tag(view->ciSelect);

                ViewLeave();
                return(str);

        }
}


/***************************************************************************
 * Function: view_newitem
 *
 * Purpose:
 *
 * Notify that CompItems have been added to the complist.
 *
 * Rebuild the view (if in outline mode), and refresh the table. Use
 * the table message TM_APPEND if possible (if column widths have not
 * change). If we have to do TM_NEWLAYOUT, then ensure we scroll
 * back to the right row afterwards.
 *
 * This causes a Poll() to take place. We return TRUE if an abort is
 * pending - in this case, the caller should abandon the scan loop.
 *
 * Enter the critical section for this function since this can be
 * called from the worker thread while the UI thread is using the
 * view that we are about to change.
 *
 * EXCEPT THAT WE DON'T DARE.  We cannot ever call SendMessage from the
 * worker thread within CSView.  If there is conflict, it will hang.
 */
BOOL
view_newitem(PVIEW view)
{
   int   max_tag, max_rest;
   LRESULT  rownr;

   if ((view == NULL) || (view->bExpand))
   {
      /* not in outline mode - nothing to do */
      return(Poll());
   }
        

   /* save some state about the present mapping */
   max_tag  = view->maxtag;
   max_rest = view->maxrest;

   /*
    * re-do the outline mapping, but don't tell the table
    * class yet until we CHECK the new SIZES of columns.
    */

   complist_count = view_outline_opt(view, FALSE);

   /* have the column widths changed ? */
   if ((max_tag < view->maxtag) || (max_rest < view->maxrest))
   {
      /* yes - need complete redraw */

      /* find the row at the top of the window */
      rownr = SendMessage(view->hwnd, TM_TOPROW, FALSE, 0);

      /* switch to new mapping */
      SendMessage(view->hwnd, TM_NEWLAYOUT, 0, (LRESULT) view);

       /* return to old row if possible - we know
        * that row is still there since we have only added
        * rows, and not changed any of the existing mapping
        *
        * Alas this is no longer true.  However the table class
        * will defend itself against calls for a bogus top row.
        */
      if (rownr >= 0)
      {
         SendMessage(view->hwnd, TM_TOPROW, TRUE, rownr);
         
      }
      
   }
   else
   {
      /* no - we can just append */

      /*
       * The mapping may have
       * changed since we released the critsec. however we are still
       * safe. The table will not allow us to reduce the number of
       * rows, so the worst that can happen is that the table will
       * think there are too many rows, and the table message handler
       * will handle this correctly (return null for the text).
       * The only visible effect is therefore that the scrollbar
       * position is wrong.
       */

      SendMessage(view->hwnd, TM_APPEND, view->v_iRows, (LRESULT) view);

   }

   /* Poll to keep the UI updated on NT. Returns true if abort pending.
    */
   return( Poll() );

}

/* **************************************************************************
 * Function: view_changeviewoptions
 *
 * Purpose:
 *
 * The view mapping options (eg outline_include, expand_mode) have changed -
 * re-do the mapping and then scroll back to the same position in the window
 * if possible.
 *
 * For example a click on the case IDM_INCSAME: menu item would toggle the bit
 * in outline_include ^= INCLUDE_SAME;, fix the MENU, then call here.
 *
 */
void view_changeviewoptions(PVIEW view)
{
   LRESULT  row;
   int   state, number;
   BOOL  bRight;

   if( !view )
      return;

   /* find what row we are currently on. Do this BEFORE we enter CSView */
   row = SendMessage(view->hwnd, TM_TOPROW, FALSE, 0);

   ViewEnter();

   if( !view->bExpand ) {
           /* outline mode. maintaining current position is
            * unimportant
            */
           disp_number = view_outline(view);   // returns COUNT of rows of display
           ViewLeave();
           SetOutlineStatus(view);  // show the number of FILES listed in the view
           return;
   }
   // else

   disp_number = -1;
   /* expanded mode */
   /* save the line number on one side (and remember which side) */
   if( row >= view->v_iRows )
   {
           number = -1;
   }
   else
   {
           state = section_getstate(view->pLines[row].section);
           if ((state == STATE_MOVEDRIGHT) ||
               (state == STATE_RIGHTONLY))
           {
                       bRight = TRUE;
                       number = view->pLines[row].nr_right;
           }
           else
           {
                   bRight = FALSE;
                   number = view->pLines[row].nr_left;
           }
   }

   /* make the new mapping */
   view_expand_item(view, view->ciSelect);

   /* find the nearest row in the new view */
   if (number >= 0)
   {

           ViewEnter();
           row = view_findrow(view, number, bRight);
           ViewLeave();
   
           /* scroll this row to top of window */
           if (row >= 0) {

                   SendMessage(view->hwnd, TM_TOPROW, TRUE, row);
                   return;
           }
   }
}

/***************************************************************************
 * Function: view_changediffoptions
 *
 * Purpose:
 *
 * The compare options have changed - re-do the compare completely
 * and make the new mapping. Retain current position in the file.
 */
void
view_changediffoptions(PVIEW view)
{
        int state, number;
        LRESULT row;
        BOOL bRight;
        LIST li;
        COMPITEM ci;

        if (view == NULL) {
                return;
        }

        /*
         * get current row before entering critsec.
         */
        row = SendMessage(view->hwnd, TM_TOPROW, FALSE, 0);

        ViewEnter();

        /* find the current line number so we can go back to it
         * (only if we are in expanded mode
         */
        if (view->bExpand) {

                state = section_getstate(view->pLines[row].section);
                if ((state == STATE_MOVEDRIGHT) ||
                    (state == STATE_RIGHTONLY)) {
                            bRight = TRUE;
                            number = view->pLines[row].nr_right;
                } else {
                        bRight = FALSE;
                        number = view->pLines[row].nr_left;
                }
        }

        /* To force a recompare using the new options, we must
         * tell each compitem to discard its current compare result.
         * We need to traverse the list of compitems calling this
         * for each compare.
         */
        li = complist_getitems(view->cl);

        for (ci = (COMPITEM) List_First(li); ci != NULL; ci = (COMPITEM) List_Next(ci)) {
                compitem_discardsections(ci);
        }

        /* if we are in outline mode, we have nothing more to do */
        if (!view->bExpand) {
                ViewLeave();
                return;
        }

        view_expand_item(view, view->ciSelect);

        /* find the nearest row in the new view */
        ViewEnter();
        row = view_findrow(view, number, bRight);
        ViewLeave();

        /* scroll this row to top of window */
        if (row >= 0) {
                SendMessage(view->hwnd, TM_TOPROW, TRUE, row);
        }

   SetAppTitle(); // line up the title with the new options
}

extern   LONG  complist_getrow( COMPLIST cl, COMPITEM ci );

LONG  view_findCI( PVIEW view, COMPITEM ci )
{
   LONG  i, j;
   if(!view)
      return -1;
   ViewEnter();
   j = view->v_iRows;
   if(!j)
   {
      ViewLeave();
      return -1;
   }

   if(view->pItems)  // this pointer is NULL in expanded mode
   {
      for( i = 0; i < j; i++ )
      {
         if( view->pItems[i] == ci )
         {
            ViewLeave();
            return i;
         }
      }
   }
   else
   {
      // cl = view_getcomplist(view);
      COMPLIST cl = view->cl;
      if(cl) // get the LIST of CI item, and convert to an index/offset/row number
      {  // if and when displayed by the table class
         i = complist_getrow( cl, ci );
         ViewLeave();
         return i;
      }
   }

   ViewLeave();
   return -1;
}

/***************************************************************************
 * Function: view_findchange
 *
 * Purpose:
 *
 * Find the next changed - ie non-same - row in a given direction.
 * For outline mode we find the next STATE_DIFFER. For expand mode, we
 * find the next section
 */
long view_findchange(PVIEW view, long startrow, BOOL bForward)
{
   long i;

   if (view == NULL)
   {
      //return(0); - this seems wrong. If no "view" then return NO FIND
      return( -1 );   // !!!!!
   }

   ViewEnter();

        if (bForward)
        {

                if (startrow >= view->v_iRows)
                {
                        ViewLeave();
                        return(-1);
                }

                if( !view->bExpand )
                {

                        /* look for next compitem with an expandable state*/
                        for (i = startrow; i < view->v_iRows; i++)
                        {
                                if (compitem_getstate(view->pItems[i]) == STATE_DIFFER)
                                {
                                        ViewLeave();
                                        return(i);
                                }
                        }
                        /* none found */
                        ViewLeave();
                        return(-1);
                }
                else
                {
                        /*
                         * find the next line that matches, then go on to the
                         * next line that does not match
                         *
                         */
                        for (i= startrow; i < view->v_iRows; i++)
                        {
                                if (section_getstate(view->pLines[i].section)
                                        == STATE_SAME)
                                {
                                                break;
                                }
                        }
                        for ( ; i < view->v_iRows; i++)
                        {
                                if (section_getstate(view->pLines[i].section)
                                        != STATE_SAME)
                                {
                                                ViewLeave();
                                                return(i);
                                }
                        }

                        ViewLeave();

                        return(-1);
                }
        }
        else
        {
                /* same search backwards */
                if (startrow <= 0)
                {
                        ViewLeave();
                        return(-1);
                }
                if (view->bExpand)
                {
                        /* search backwards for first row that is not
                         * changed (has state SAME). then carry on for
                         * the next changed row.
                         */
                        for (i = startrow; i >= 0; i--)
                        {
                                if (section_getstate(view->pLines[i].section)
                                        == STATE_SAME)
                                {
                                                break;
                                }
                        }
                        for ( ; i >= 0; i--)
                        {
                                if (section_getstate(view->pLines[i].section)
                                        != STATE_SAME)
                                {
                                                ViewLeave();
                                                return(i);
                                }
                        }
                        ViewLeave();
                        return(-1);
                }
                else
                {
                        for (i = startrow; i >= 0; i--)
                        {
                                if(compitem_getstate(view->pItems[i]) == STATE_DIFFER)
                                {
                                        ViewLeave();
                                        return(i);
                                }
                        }
                        ViewLeave();
                        return(-1);
                }
        }
}




///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : view_haschange
// Return type: INT 
// Arguments  : PVIEW view
//            : BOOL bCnt
// Description: Whether expanded or outline, if bCnt return COUNT of differences
//              else return of first difference.
///////////////////////////////////////////////////////////////////////////////
INT  view_haschange( PVIEW view, BOOL bCnt )
{
   INT  bRet = 0;
   if( view )
   {
      INT   j = view->v_iRows;
      INT   i;
      ViewEnter();
      if( view->bExpand )
      {
         // comparing LINES within a pair of files
         for( i = 0; i < j; i++ )
         {
            if( section_getstate(view->pLines[i].section) != STATE_SAME )
            {
               bRet++;
               if( bCnt )
               {
                  i++;  // move on until we reach the SAME again
                  for( ; i < j; i++ )
                  {
                     if( section_getstate(view->pLines[i].section) == STATE_SAME )
                        break;
                  }
               }
               else
                  break;   // all done on first
            }
         }
      }
      else  // an outline view
      {
         // comparing a LIST of files - each pItems[i] is a FILE NAME,
         // or PAIR of names.
         for( i = 0; i < j; i++ )
         {
            if( compitem_getstate(view->pItems[i]) == STATE_DIFFER )
            {
               bRet++;
               if( !bCnt )
                  break;   // all done on first
            }
         }

      }
      ViewLeave();
   }
   return bRet;
}

/***************************************************************************
 * Function: view_findrow
 *
 * Purpose:
 *
 * Find the new row number for the line numbered 'number'
 * or the nearest line if possible. If bRight is true, number is
 * a right file number; otherwise it is a left file number.
 *
 * We must be in expand mode
 */
int     
view_findrow(PVIEW view, int number, BOOL bRight)
{
        int i;

        if (!view->bExpand) {   
                return(0);
        }

        for (i = 0; i < view->v_iRows; i++)
        {

                if (bRight) {
                        if (view->pLines[i].nr_right == number) {

                                /* found the exact number */
                                return(i);

                        } else if (view->pLines[i].nr_right > number) {

                                /* passed our line -stop here */
                                return(i);
                        }
                } else {
                        if (view->pLines[i].nr_left == number) {

                                /* found the exact number */
                                return(i);

                        } else if (view->pLines[i].nr_left > number) {

                                /* passed our line -stop here */
                                return(i);
                        }
                }
        }
        return(-1);
}

/***************************************************************************
 * Function: view_freemappings
 *
 * Purpose:
 *
 * Free memory associated with the expand mode or outline mode mappings
 * called whenever we rebuild the mapping, and on deletion
 */
void
view_freemappings(PVIEW view)
{

        if (view->pLines)
        {
           gmem_free(hHeap, (LPSTR) view->pLines,
              (view->v_iRows * sizeof(VIEWLINE)),"view_expand_item");

           view->pLines = NULL;
        }
        else if (view->pItems)
        {

           /* previous outline mapping array is still there - free it
            * before we build a new one
            */
           gmem_free(hHeap, (LPSTR) view->pItems,
              (view->v_iRows * sizeof(COMPLIST)),"view_outline_opt" );
           
           view->pItems = NULL;
        }
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : IsInView
// Return type: BOOL 
// Arguments  : DWORD dwo
//            : UINT state
//            : DWORD dwFlag
// Description: states are SINGULAR - but we can have many 'nuances'
//              of whether it is really different. This is particularly
// true if a file ONLY has a change in TIME, not SIZE
// THe options flags simply state, as 'bits', what has been selected.
///////////////////////////////////////////////////////////////////////////////
BOOL  IsInView( DWORD dwo, UINT state, DWORD dwFlag )
{
   if( state == STATE_DIFFER )
   {
      if( dwFlag & TT_OLDER )
      {
         if( dwo & INCLUDE_OLDER )
            return TRUE;   // = i++;  // a displayable DIFF

      }
      else  // if( dwFlag & TT_YOUNGER )
      {
         if( dwo & INCLUDE_NEWER )
            return TRUE;   // =i++;  // a displayable DIFF
      }
      //if( lg < 0 )   // Left/First file time is less than Right/second file time.
      //   ci->ci_dwFlag |= TT_OLDER;    // left is older
      //else
      //   ci->ci_dwFlag |= TT_YOUNGER;    // left is newer = update?
//            ( (dwo & INCLUDE_DIFFER)    && (state == STATE_DIFFER)        ) ||
   }
   else if( ( (dwo & INCLUDE_SAME)      && (state == STATE_SAME)          ) ||
            ( (dwo & INCLUDE_LEFTONLY)  && (state == STATE_FILELEFTONLY)  ) ||
            ( (dwo & INCLUDE_RIGHTONLY) && (state == STATE_FILERIGHTONLY) ) )
   {
      return TRUE;   // = i++;  // count a 'displayable' item
   }

   return FALSE;
}

//extern   LRESULT compitem_addLV( COMPITEM ci );
//#define  ADDLISTVIEW(a) compitem_addLV(a)


/***************************************************************************
 * Function: view_outline_opt
 *
 * Purpose:
 *
 * Build a view outline to map one row to a COMPITEM handle by traversing
 * the list of COMPITEMs obtained from our complist.
 * Optionally tell the table class to redraw (if bRedraw), and if so,
 * scroll the new table to select the row that represents the
 * file we were expanding, if possible
 *
 * Same as view_outline, except here the BOOL bRedraw can be TRUE or FALSE
 * For example a click on the case IDM_INCSAME: menu item would toggle the bit
 * in outline_include ^= INCLUDE_SAME;, fix the MENU, then called
 * view_changeviewoptions(), which if in OUTLINE mode calls view_outline()
 * which calls here with bRedraw as TRUE.
 *
 * I want to ADD a message "warning" if nothing is display matching the
 * current "options" - maybe here? But in situation of -R resursion,
 * this service is called MULTIPLE times, thus my message must
 * be back up the 'tree'.
 *
 * First step is to have this service return its COUNT!
 *
 * In fact this is called after every file scanned, from a directory, is or
 * is not matched with a file scanned from the right directory, and also
 * for those not found in the right side ...
 *
 * Originally I wanted to process the LISTVIEW control here also, but that
 * seems to sometime lead to a 'locked' application ...
 *
 */
INT view_outline_opt(PVIEW view, BOOL bRedraw)
{
   int      prev_row = -1;      /* the row nr of the previously-expanded row*/
   int      i;            /* nr of includable items */
   int      itot; // total in OUTLINE list
   LIST     li;
   COMPITEM ci;
   int      state;   //, iter;
   TableSelection select;
   DWORD    dwo, dwFlag;
//   INT      ilen;
   /*
    * check that view_setcomplist has already been called. if not,
    * nothing to do
    */
   if(view->cl == NULL)
      return 0;

   DBG_LOG1( "Freeing view ..."MEOR );
   ViewEnter();

   /* clear the mode flag and free up memory associated with expand mode */
   view->bExpand = FALSE;
   g_sCFDLGSTR.cf_bExpanded = FALSE;

   view_freemappings(view);

   CLROSTATS( &g_sOutStats ); // = ZeroMemory(a, sizeof(OUTSTATS))

   /* traverse the list of compitems counting up the number of
    * includable items - according to the outline_include FLAG
    */
   li = complist_getitems(view->cl);

   dwo = outline_include;
   ci = (COMPITEM) List_First(li);
   itot = 0;
   for( i = 0; ci != NULL; ci = (COMPITEM) List_Next(ci) )
   {
      itot++;  // count a COMP items
      state  = compitem_getstate(ci);
      dwFlag = compitem_getflag(ci);
      if( IsInView( dwo, state, dwFlag ) )
      {
#ifdef   ADD2LV2
         if( !( dwFlag & TT_INLISTVW ) )
         {
            LRESULT lRes = ADDLISTVIEW(ci);
            if(lRes != -1)
            {
               compitem_addflag2(ci, TT_INLISTVW, lRes);
            }
         }
#endif   // #ifdef   ADD2LV2
         i++;  // BUMP displayable count
      }
#ifdef ADD_LIST_VIEW
      else if( dwFlag & TT_INLISTVW )
      {
         // REMLISTVIEW(ci);
//typedef struct tagLVFINDINFO {
//    UINT flags;
//    LPCTSTR psz;
//    LPARAM lParam;
//    POINT pt;
//    UINT vkDirection;
//} LVFINDINFO, FAR* LPFINDINFO;
         static LVFINDINFO _s_lvfi;
         LVFINDINFO * plvfi = &_s_lvfi;
         DWORD index;   // = compitem_getindex(ci); - this fails after 1st delete!!!

         ZeroMemory(plvfi, sizeof(LVFINDINFO));
         plvfi->flags  = LVFI_PARAM;
         plvfi->lParam = (LPARAM)ci;
         index = ListView_FindItem( g_hListView,  // HWND hwnd,
                     -1,   // int iStart, = from beginning
                     plvfi ); // const LPLVFINDINFO plvfi
         if(index != -1)
         {
            if( ListView_DeleteItem( g_hListView, index ) )
            {
               if( g_dwLVCount ) // got items
                  g_dwLVCount--; // removed an item
            }
            else
            {
               chkme( "view_outline_opt: An index given can NOT be deleted!!! i = %d"MEOR, index );
            }
         }
         else
            chkme( "YEEK! Failed to FIND a record to DELETE!"MEOR );

         // but DON'T go thru this again, unless later re-included, etc, etc, etc
         compitem_subflag(ci, TT_INLISTVW);
      }
#endif // #ifdef ADD_LIST_VIEW

   }

   /*
    * allocate an array big enough
    * for all of these possible screen lines
    * = displayable with this outline_include 
    * note: if there are NO 'displayable' items for this outline view,
    * then i = 0, and gmem_get() returns a NULL!!!
    *
    */
   view->pItems = (COMPITEM FAR *) gmem_get(hHeap, (i * sizeof(COMPITEM)),
      "view_outline_opt" );

   view->v_iRows = i;      // set the COUNT
   view->v_iTot  = itot;   // and TOTAL count

   /* keep track of the column widths - NOTE two(2) COLUMNS embedded */
   view->maxtag  = 0;   // retstart cumulative counters
   view->maxrest = 0;

   /*
    * loop through again filling the array, and at the same time looking
    * out for the handle of the previously expanded item
    *
    */
   // but only if there is a COUNT
   if(i)
   {
      dwo = outline_include;
      ci = (COMPITEM) List_First(li);
      for( i = 0; ci != NULL; ci = (COMPITEM) List_Next(ci) )
      {
         state = compitem_getstate(ci);
         dwFlag = compitem_getflag(ci);
         //if( ( (dwo & INCLUDE_SAME) && (state == STATE_SAME) ) ||
         //    ( (dwo & INCLUDE_DIFFER) && (state == STATE_DIFFER) ) ||
         //    ( (dwo & INCLUDE_LEFTONLY) && (state == STATE_FILELEFTONLY) ) ||
         //    ( (dwo & INCLUDE_RIGHTONLY) && (state == STATE_FILERIGHTONLY) ) )
         if( IsInView( dwo, state, dwFlag ) )
         {
            view->pItems[i] = ci;   // store the COMPITEM for this ON DISPLAY line
            if( ci == view->ciSelect ) // if equals file selected, then KEEP this row
            {
               prev_row = i;
            }
   
            /* check the column widths in characters */
            //view->maxtag  = max( view->maxtag,  (int)strlen(compitem_gettext_tag(ci)) );
            // *** TBD *** note embedded just two columns concept here!
            //view->maxrest = max( view->maxrest, (int)strlen(compitem_gettext_result(ci)) );
            // at least AVOID possible TWO strlen calls by
            //ilen = (int)strlen(compitem_gettext_tag(ci));
            //ilen = (int)strlen(compitem_gettext_tags(ci));
            // should perhaps use list control as mentioned else where ...
            //view->maxtag  = max( view->maxtag, ilen );
            // *** TBD *** note embedded just two columns concept here!
            //ilen = (int)strlen( compitem_gettext_result(ci) );
            //view->maxrest = max( view->maxrest, ilen );
            // or even better - optimised only a little, to get the
            // average width of the 'tag' and the 'rest'
            compitem_setmaxlen( ci, &view->maxtag, &view->maxrest );

            i++;  // count an item
   
          }
      }
   }

   // all done
   ViewLeave();

   DBG_LOG1( "New view memory ... count = %d on %d (%s)"MEOR,
      i, itot, (bRedraw ? "draw" : "nod") );
   /* inform table of new layout of table - force refresh */       
   if( bRedraw )  // && !g_bNoUpdate )
   {
      SendMessage(view->hwnd, TM_NEWLAYOUT, 0, (LRESULT) view);
   
      /* scroll to and highlight the row that represents the file
       * we were previously expanding
       */
      if( prev_row != -1 )
      {
         select.startrow  = prev_row;
         select.startcell = 0;
         select.nrows     = 1;
         select.ncells    = 1;
         SendMessage(view->hwnd, TM_SELECT, 0, (LRESULT) (LPSTR) &select);
      }

//      for( iter = 0; iter < i; iter++ )
//      {
//            view->pItems[i] = ci;   // store the COMPITEM for this ON DISPLAY line
//         ADDLISTVIEW(view->pItems[iter]); // ci);
//      }

   }

   return i;   // return COUNT of displayable LINES
}

#ifdef ADD_LIST_VIEW
INT view_update_lv(PVIEW view)
{
   int      i;            /* nr of includable items */
   int      itot; // total in OUTLINE list
   LIST     li;
   COMPITEM ci;
   int      state;   //, iter;
   DWORD    dwo, dwFlag;
   /*
    * check that view_setcomplist has already been called. if not,
    * nothing to do
    */
   if(( !view            ) ||
      ( view->cl == NULL ) ||
      ( !g_hListView     ) )
      return 0;

   ViewEnter();

   /* traverse the list of compitems counting up the number of
    * includable items - according to the outline_include FLAG
    */
   li = complist_getitems(view->cl);
   dwo = outline_include;
   ci = (COMPITEM) List_First(li);
   itot = 0;
   for( i = 0; ci != NULL; ci = (COMPITEM) List_Next(ci) )
   {
      itot++;  // count a COMP items
      state  = compitem_getstate(ci);
      dwFlag = compitem_getflag(ci);
      if( IsInView( dwo, state, dwFlag ) )
      {
#ifdef ADD_LIST_VIEW
          if( !( dwFlag & TT_INLISTVW ) )
         {
            LRESULT lRes = ADDLISTVIEW(ci);
            if(lRes != -1)
            {
               compitem_addflag2(ci, TT_INLISTVW, lRes);
            }
         }
#endif // #ifdef ADD_LIST_VIEW
          i++;  // BUMP displayable count
      }
#ifdef ADD_LIST_VIEW
      else if( dwFlag & TT_INLISTVW )
      {
         // REMLISTVIEW(ci);
//typedef struct tagLVFINDINFO {
//    UINT flags;
//    LPCTSTR psz;
//    LPARAM lParam;
//    POINT pt;
//    UINT vkDirection;
//} LVFINDINFO, FAR* LPFINDINFO;
         static LVFINDINFO _s_lvfi;
         LVFINDINFO * plvfi = &_s_lvfi;
         DWORD index;   // = compitem_getindex(ci); - this fails after 1st delete!!!

         ZeroMemory(plvfi, sizeof(LVFINDINFO));
         plvfi->flags  = LVFI_PARAM;
         plvfi->lParam = (LPARAM)ci;
         index = ListView_FindItem( g_hListView,  // HWND hwnd,
                     -1,   // int iStart, = from beginning
                     plvfi ); // const LPLVFINDINFO plvfi
         if(index != -1)
         {
            if( ListView_DeleteItem( g_hListView, index ) )
            {
               if( g_dwLVCount ) // got items
                  g_dwLVCount--; // removed an item
            }
            else
               chkme( "view_update_lv: An index given can NOT be deleted!!! i = %d"MEOR, index );
         }
         else
            chkme( "YEEK! Failed to FIND a record to DELETE!"MEOR );

         // but DON'T go thru this again, unless later re-included, etc, etc, etc
         compitem_subflag(ci, TT_INLISTVW);
      }
#endif // #ifdef ADD_LIST_VIEW

   }

   // all done
   ViewLeave();

   return i;   // return COUNT of displayable LINES
}
#endif // #ifdef ADD_LIST_VIEW


BOOL  IsInViewOpts( INT state, DWORD dwFlag )
{
               
                if (expand_mode == IDM_RONLY)
                {
                   // only lines from right file
                        if ((state == STATE_LEFTONLY) ||    // there is NO right line, or
                            (state == STATE_MOVEDLEFT))
                        {
                                    //continue;
                           return FALSE;
                        }
                }
                else if (expand_mode == IDM_LONLY)
                {
                        if ((state == STATE_RIGHTONLY) ||
                            (state == STATE_MOVEDRIGHT))
                        {
                                    //continue;
                           return FALSE;
                        }
                }

   switch(state)
   {
   case STATE_SAME:  // left and right identical (usually excl. space, line ends)
      if( !( gdwDiffOpts & INCLUDE_SAME ) )
         return FALSE;
      break;
   case STATE_LEFTONLY:
      if( !( gdwDiffOpts & INCLUDE_LEFTONLY ) )
         return FALSE;
      break;

   case STATE_RIGHTONLY:
      if( !( gdwDiffOpts & INCLUDE_RIGHTONLY ) )
         return FALSE;
      break;

   case STATE_MOVEDLEFT:
      if( !( gdwDiffOpts & INCLUDE_MOVELEFT ) )
         return FALSE;
      break;

   case STATE_MOVEDRIGHT:
      if( !( gdwDiffOpts & INCLUDE_MOVERIGHT ) )
         return FALSE;
      break;

   }
   return TRUE;
}

//   INVO(a,b) INT state, DWORD dwFlag )
#define  INVO(a,b)    IsInViewOpts(b, section_getflag(a))

/***************************************************************************
 * Function: view_expand_item
 *
 * Purpose:
 *
 * Expand a view - given the handle to the compitem to expand.
 *
 * Called from view_expand, and also to re-do an expanded view
 * after options change in view_changediffoptions and _changeviewoptions
 *
 * We get the composite section list from the compitem,
 * and pick out all the sections that are includable (according
 * to the global option expand_mode: we include all sections, or
 * just those in one side left or right). Once we know the count of rows,
 * allocate the mapping array: in each element of the array we keep
 * a handle to the section for that row (to get the state and hence the
 * tag text), and a handle to the line within that section (for the line text).
 *
 * We no longer insist on only expanding text files that differ - if the
 * compitem can give us a composite section list, we will map it.
 *
 * We need to be able to give a line number for a line, in either of
 * the original files according to which option is in force. Each section
 * can give us its base line number (number of first line in section) in
 * each of the two files or 0 if not present, and we track these here.
 *
 * MUST BE INSIDE CSView BEFORE CALLING HERE.
 *
 * If it came from a selection in OUTLINE, then ci = view->pItems[row];
 *
 */
BOOL
view_expand_item(PVIEW view, COMPITEM ci)
{
   LIST li;
   SECTION sh;
   LINE line1, line2;
   int i, base_left, base_right, state;

   /* remember the compitem we are expanding */
   view->ciSelect = ci;

   /* get the composite section list */
   li = compitem_getcomposite(view->ciSelect);

   if (li == NULL)
   {
                ViewLeave();
                g_sCFDLGSTR.cf_bExpanded = FALSE;
                return FALSE;
   }

   /* switch modes and free the current mapping
    *
    * NOTE: must do this AFTER the compitem_getcomposite,
    * since that can fail: if it fails it could put up a
    * message box, and that could cause a queued paint message
    * to be processed, which would cause us to use these mappings
    * and gpfault if they had been cleared first.
    */
   view->bExpand = TRUE;

   view_freemappings(view);

   /* loop through totalling the lines in sections
    * that we should include
    */
   view->v_iRows = 0; // restart ROWS available for display COUNT - EXPAND mode
   // should also reset ALL global EXPANDED stats ...
   // **************************************************************************
   for (sh = (SECTION) List_First(li); sh != NULL; sh = (SECTION) List_Next(sh))
   {
      state = section_getstate(sh);

      if( !INVO( sh, state ) )
         continue;  // not in this view of the file lines

      /* include all lines in this section */
      view->v_iRows += section_getlinecount(sh);  // count of AVAILABLE lines
   }
     
   /*
    * allocate the memory for the mapping array
    * NOTE: if no lines are 'displayable' with this expanded mode include what
    * then iRows = 0, and gmem_get() returns a NULL!!! (by design)
    *
    */
   view->pLines = (PVIEWLINE) gmem_get(hHeap,
        (view->v_iRows * sizeof(VIEWLINE)), "view_expand_item" );
     
   /* loop through the sections again filling in the mapping array */
   i = 0;
   // columns are file line number - either left or right number
   view->maxtag = 5;  // " " = same "<!"=left "!>"=right or moved
   view->maxrest = 0; // this is the actual line data from the file
   // *** TBD *** unconditionally the line terminator(s) are removed
   // and NOT remembered. It is also correct NOT to ignore this 'diff'!
   for (sh = (SECTION) List_First(li); sh != NULL; sh = (SECTION) List_Next(sh))
   {
      state = section_getstate(sh);
             
      if( !INVO( sh, state ) )
         continue;  // not in this view of the file lines

      /* find the base line number in each file */
      base_left = section_getleftbasenr(sh);
      base_right = section_getrightbasenr(sh);

      /* add each line in section to the view. section_getfirst()
       * returns us to a handle that is in a list. We can
       * call List_Next and will eventually get to the
       * line returned by section_getlast(). Sections always have
       * at least one line
       */
      line1 = section_getfirstline(sh);
      line2 = section_getlastline(sh);

             for (; line1 != NULL; line1 = (LINE) List_Next(line1))
             {

                     view->pLines[i].line = line1;
                     view->pLines[i].section = sh;

                     /* calculate the line number for this line by
                      * incrementing the base nr for this section
                      */
             
                     view->pLines[i].nr_left = base_left;
                     if (base_left != 0)
                     {
                             base_left++;
                     }

                     view->pLines[i].nr_right = base_right;
                     if (base_right != 0)
                     {
                             base_right++;
                     }

                     /* increment index into view */
                     i++;

                     /* check the column widths */
                     view->maxrest = max(view->maxrest,
                                         (line_gettabbedlength(line1, 8)));

                     /* end of section ? */
                     if (line1 == line2)
                     {
                             break;
                     }
             }
     }

     /* We must NOT hold a critical section here as SendMessage may hang */
     ViewLeave();

     /*inform table window of revised mapping */
     SendMessage(view->hwnd, TM_NEWLAYOUT, 0, (LPARAM) view);

     if( !(compitem_getflag( ci ) & TT_VIEWED ) )
     {
        compitem_addflag( ci, TT_VIEWED );  // in EXPANDED moded - lines loaded - compared
        // this pair, or at least one file is having the once over **********************
        g_dwviewedcnt++;
     }

     return(TRUE);
}


/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// NEW

BOOL  view_setrefresh( PVIEW view, BOOL flg )
{
   BOOL  bRet = FALSE;
   if( view )
   {
      bRet = view->bRefresh;
      view->bRefresh = flg;
   }
   return bRet;
}

BOOL  view_getrefresh( PVIEW view )
{
   BOOL  bRet = FALSE;
   if( view )
   {
      bRet = view->bRefresh;
      view->bRefresh = FALSE;
   }
   return bRet;
}

BOOL  view_getrecursive( PVIEW view )
{
   BOOL  bRet = FALSE;
   if(view)
      bRet = view->bRecursive;
   return bRet;
}

INT   view_getiselect( PVIEW view )
{
   if( !view )
      return -1;
   /* remember the compitem we are expanding */
   return view->iSelect;   // row;
}

// Get the view's COMPITEM
// A COMPITEM contains (one of or both)
//struct compitem {
//  FILEDATA left;          /* handle for left-hand file */
//  FILEDATA right;         /* handle for right-hand file */
BOOL  view_haszip( PVIEW view )
{
   BOOL  bRet = FALSE;
   COMPITEM ci = view_getitem( view, 0 );
   if(ci)
   {
      if(( dir_iszip( file_getdiritem( compitem_getleftfile( ci) ) ) ) ||
         ( dir_iszip( file_getdiritem( compitem_getrightfile(ci) ) ) ) )
      {
         bRet = TRUE;
      }
   }
   return bRet;
}

LPTSTR   view_getfilcntstg( PVIEW view )
{
   LPTSTR   lpb = GetStgBuf();
   INT      icnt = view_getrowcount(view);
   INT      itot = view_gettotcount(view);
   g_bColourCycle = FALSE;
   if( icnt == itot )
   {
      if(icnt)
      {
         sprintf(lpb, "List of %d", icnt);
      }
      else
      {
         strcpy(lpb, "No COMPARE list active! Use File / Compare ... to start.");
         g_bColourCycle = TRUE;
      }
   }
   else
   {
      if( icnt )  // if we have VIEWABLE
      {
         sprintf(lpb, "List %d of %d", icnt, itot);
      }
      else  // we have NOTHING to display with the PRESENT option settings
      {
         //if( g_dwSame == (DWORD) itot )
         //if(( g_dwLeft == 0 ) &&
         //   ( g_dwRite == 0 ) &&  // no orphanes
         //   ( g_dwDiff == 0 ) )
         if( g_DirsEqual ) // if they are the same
         {
            // they appear EXACTLY equal - can not GRUMBLE
            //sprintf(lpb, "List of %d but are ALL the same.", itot);
            // *** TBD *** This should reflect whether recursive is on
            // then maybe no 's'
            sprintf(lpb, "Folders Equal - %d files. Ctrl+I to view.", itot);
         }
         else
         {
            sprintf(lpb, "List of %d but NONE match options.", itot);
         }
         g_bNeedPaint = TRUE;
      }
   }
   return lpb;
}


// eof - view.c
