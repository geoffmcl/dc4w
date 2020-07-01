

// dc4wDlgR.c
// this is public domain software - praise me, if ok, just don't blame me!
// review the COPY LIST, before doing the COPY

#include "dc4w.h"

extern   LSTSTATS sLstStats;
extern   VOID  MyCommonDlgResults( HWND hDlg );
extern   TCHAR dlg_leftname[];
extern   TCHAR dlg_rightname[];
extern   DWORD dlg_flg;
extern   VOID  CheckCommonSet( HWND hDlg, DWORD dwi );
extern   VOID  MyCommonDlgInit2( HWND hDlg );

#define  MXLVNUM        9     // size of the SIZE field

BOOL  bIsApply;

// ===========================================================
// LIST VIEW STUFF
BOOL  bDnLvInit;
#define  DLGLVCC     5
static TCHAR szString[DLGLVCC][20] = {  TEXT("File Name"), 
                                 TEXT("Size"), 
                                 TEXT("Date"), 
                                 TEXT("Time"), 
                                 TEXT("Information") };
static INT iSizes[DLGLVCC] = { 270, 90, 80, 60, 140 };
static DWORD   dwlvcFlag[DLGLVCC];

#ifdef ADD_LIST_VIEW

/* *************************************************************************
   SwitchView()
   ************************************************************************* */
void SwitchView(HWND hwndListView, DWORD dwView)
{
	LONG_PTR dwStyle = GetWindowLongPtr(hwndListView, GWL_STYLE);
	SetWindowLongPtr(hwndListView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
	//ResizeListView(hwndListView, GetParent(hwndListView));
}

/* *************************************************************************
   AddExStyle()
   ************************************************************************* */
void AddExStyle(HWND hwndListView, DWORD dwNewStyle)
{
	LRESULT dwStyle = SendMessage(hwndListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle |= dwNewStyle;
	SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
}

/* *************************************************************************
   RemoveExStyle()
   ************************************************************************* */
void RemoveExStyle(HWND hwndListView, LRESULT dwNewStyle)
{
	LRESULT dwStyle = SendMessage(hwndListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle &= ~dwNewStyle;
	SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SetDefView
// Return type: VOID 
// Argument   : HWND hwndListView
// Description: Part if the LISTVIEW CONTROL initialisation
//              
///////////////////////////////////////////////////////////////////////////////
VOID	SetDefView(HWND hwndListView)
{
   LOGFONT  lf;
	//DWORD	dwf = 0;
	//SetViewType( hwndListView, g_iType );
   SwitchView( hwndListView, LVS_REPORT );   // ensure in REPORT mode

	//dwf = (DWORD)SendMessage( hwndListView,
	//	LVM_GETEXTENDEDLISTVIEWSTYLE,
	//	0, 
	//	0 );

	//   case IDM_HOVERSELECT:
	//lvStyle( &g_iAddTrack, LVS_EX_TRACKSELECT );
   LVRemoveExStyle( hwndListView, LVS_EX_TRACKSELECT );
	//   case IDM_CHECKBOXES:
	//lvStyle( &g_iAddCheck, LVS_EX_CHECKBOXES );
	LVAddExStyle( hwndListView, LVS_EX_CHECKBOXES );
	//   case IDM_GRIDLINES:
	//lvStyle( &g_iAddGrid,  LVS_EX_GRIDLINES  );
   LVRemoveExStyle( hwndListView, LVS_EX_GRIDLINES );
	//	 case IDM_FULLROWSELECT:
	//lvStyle( &g_iAddFull,  LVS_EX_FULLROWSELECT );
	LVAddExStyle( hwndListView,  LVS_EX_FULLROWSELECT );
	//	 case IDM_HEADERDRAGDROP:
	//lvStyle( &g_iAddHDrag, LVS_EX_HEADERDRAGDROP );
   LVRemoveExStyle( hwndListView, LVS_EX_HEADERDRAGDROP );
	//	 case IDM_SUBITEMIMAGES:
	//lvStyle( &g_iAddISub,  LVS_EX_SUBITEMIMAGES );
   LVRemoveExStyle( hwndListView, LVS_EX_SUBITEMIMAGES );

//	InvalidateRect( hwndListView, NULL, TRUE );
//	UpdateWindow( hwndListView );
   // FontInitialize

   GetObject( GetStockObject (SYSTEM_FIXED_FONT), sizeof (LOGFONT), (LPSTR) &lf);
   //ghFixedFont = CreateFontIndirect(&lf);
   //if(ghFixedFont)
   //   SendMessage(hwndListView, WM_SETFONT, (WPARAM) ghFixedFont, 0L);

   if(g_hFixedFont8)
      SendMessage(hwndListView, WM_SETFONT, (WPARAM) g_hFixedFont8, 0L);

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : InitListView
// Return type: BOOL 
// Argument   : HWND hwndListView
// Description: Massage the LIST control to the desired form. On this dialog
//              there is NO user control of this.
///////////////////////////////////////////////////////////////////////////////
BOOL InitListView(HWND hwndListView)
{
	LV_COLUMN   lvColumn;
   RECT        rc;
	INT         i, j, k;

	// set listview default view
	SetDefView( hwndListView );

   // but this does NOT work, perhpas because the list view is using some
   // other units.
   GetWindowRect( hwndListView, &rc );
   j = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL);    // width in pixel
   
	//initialize the columns
	//lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvColumn.fmt = LVCFMT_LEFT;
	for(i = 0; i < DLGLVCC; i++)
	{
		//make the secondary columns smaller
      j -= iSizes[i];
   	lvColumn.cx = iSizes[i];
		lvColumn.pszText = szString[i];
		//SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)i, (LPARAM)&lvColumn);
      k = ListView_InsertColumn(hwndListView,   // HWND hwnd, 
         i,       // iCol,
         &lvColumn );   //  const LPLVCOLUMN pcol
      if( k == -1 )
         return FALSE;  // *** FAILED ***

	}

	return TRUE;
}

#endif // #ifdef ADD_LIST_VIEW

// helper function(s)
VOID  StateToString2( LPTSTR lpb, PCPYTST pct )
{
   int state = pct->ct_iState;
   DWORD dwFlag = pct->ct_dwFlag;
   PLSTSTATS      pls = &sLstStats;
   switch(state)
   {
   case STATE_SAME:
//   case STATE_COMPARABLE:
//   case STATE_SIMILAR:
      strcat(lpb, "Same");
      pls->dwsame++;
      break;
   case STATE_DIFFER:
      //strcat(lpb, "Differ");
      if( dwFlag & TT_YOUNGER )
      {
         strcat(lpb, "Update");
         pls->dwnewer++;
      }
      else
      {
         strcat(lpb, "OLDER!");
         pls->dwolder++;
      }
      break;

   case STATE_FILELEFTONLY:
//   case STATE_LEFTONLY:
//      strcat(lpb, "Left only");
      strcat(lpb, "Copy new");
      pls->dwleft++;
      break;

   case STATE_FILERIGHTONLY:
//   case STATE_RIGHTONLY:
//      strcat(lpb, "Right only");
      strcat(lpb, "*Delete*");
      pls->dwright++;
      break;

//   case STATE_MOVEDLEFT:    /* this is the left file version */
//      strcat(lpb, "Left");
//      break;

//   case STATE_MOVEDRIGHT:  /* this is the right file version*/
//      strcat(lpb, "Right");
//      break;
   default:
      sprintf(EndBuf(lpb), "?UNK?(%d)", state );
      pls->dwunk++;
      break;
   }
}

INT   sc_GetColumnText( PCPYTST pct, INT iCol, LPTSTR lpb )
{
   INT   i = 0;
   DWORD dwi = 0;
   LARGE_INTEGER  li;
   LPTSTR   ptmp;
   switch(iCol)
   {
   case 0:
      {
         if( InStr( &pct->ct_szCopyRel[0], ".\\" ) == 1 )
            dwi = 2;


	      i = sprintf(lpb, "%s", &pct->ct_szCopyRel[dwi] );
      }
      break;
   case 1:
      {
         // column 2
         li.LowPart  = pct->ct_sFDSrc.nFileSizeLow;
         li.HighPart = pct->ct_sFDSrc.nFileSizeHigh;
         ptmp = GetI64Stg( &li );
         *lpb = 0;
         dwi = strlen(ptmp);
         while(dwi < MXLVNUM)
         {
            strcat(lpb, " ");
            dwi++;
         }
         strcat(lpb, ptmp);
         i = strlen(lpb);
      }
      break;
   case 2:
      {
         // column 3
	   	i = sprintf(lpb, "%s", GetFDStg(&pct->ct_sFDSrc.ftLastWriteTime) );
      }
      break;
   case 3:
      {
         // column 4
	      i = sprintf(lpb, "%s", GetFTStg(&pct->ct_sFDSrc.ftLastWriteTime) );

      }
      break;

   case 4:
      {
         // column 5
         PCFDLGSTR pcfds = pct->ct_pcfds;     // extract the PRIMARY COPY FILE DIALOG STRUCTURE
         LPTSTR pdst = &pcfds->cf_szDest[0];  // get the pointer to the DESTINATION

         *lpb = 0;
         if( pct->ct_NewDir[0] )
         {
            //ptmp = RetDiffStg( &dlg_root[0], &pct->ct_NewDir[0] );
            ptmp = RetDiffStg( pdst, &pct->ct_NewDir[0] );
            if( ptmp[0] == '\\' )
               strcpy(ptmp, &ptmp[1] );
            else if( InStr( ptmp, ".\\" ) == 1 )
               strcpy( ptmp, &ptmp[2] );
            dwi = strlen(ptmp);
            if(dwi)
            {
               if( ptmp[dwi - 1] == '\\' )
               {
                  dwi--;
                  ptmp[dwi] = 0;
               }
               if(dwi)
                  sprintf(EndBuf(lpb), "New[%s] ", ptmp );
            }
         }

         //StateToString(lpb, pct->ct_iState);
         StateToString2(lpb, pct);

         i = strlen(lpb);

      }
      break;
   }
   return i;
}

/* *****************************************************************************
   InsertListViewItems
   ***************************************************************************** */
BOOL InsertListViewItems( HWND hDlg, HWND hwndListView, PLE pHead, INT icnt )
{
   BOOL           bRet = FALSE;
	LV_ITEM        lvItem;
   UINT           nIndex;
   LPTSTR         lpb = &gszTmpBuf[0];
   PLE            pNext;
   PCPYTST        pct;
//   LARGE_INTEGER  li;
//   LPTSTR         pdst, ptmp;
//   DWORD          dwi;
   INT            cnt = 0;
//   PCFDLGSTR      pcfds;
   PLSTSTATS      pls = &sLstStats;

	SendMessage( hwndListView, WM_SETREDRAW, FALSE, 0 );

	//empty the list
	SendMessage( hwndListView, LVM_DELETEALLITEMS, 0, 0 );

   ZeroMemory( pls, sizeof(LSTSTATS) );

   if( IsListEmpty(pHead) )
      goto Done_List;

   pNext = pHead->Flink;   // get first item (or ANY item for that matter)
   pct = (PCPYTST)pNext;      // cast it to my COPY stucture
//   pcfds = pct->ct_pcfds;     // extract the PRIMARY COPY FILE DIALOG STRUCTURE
//   pdst = &pcfds->cf_szDest[0];  // get the pointer to the DESTINATION

   Traverse_List( pHead, pNext )
   {
      pls->dwtot++;  // bump TOTAL in LIST
      pct = (PCPYTST)pNext;   // cast to structure
      if( pct->ct_dwFlag & flg_Delete )
      {
         cnt++;
         pls->dwdeleted++;
         continue;
      }

   	lvItem.lParam = (LPARAM)pct;	// 32-bit value to associate with item
      // column 1
      // add just the RELATIVE name
      // NOTE: This was set up in dir_copytest() or dir_deletetest() functions
      sc_GetColumnText( pct, 0, lpb );
		//fill in the LV_ITEM structure for the first item
		lvItem.mask       = LVIF_TEXT | LVIF_PARAM;
		lvItem.pszText    = lpb;   // keep same buffer thru-out
      lvItem.cchTextMax = 256;
		lvItem.iImage     = 0;
		lvItem.iItem      = (INT)SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
		lvItem.iSubItem   = 0;

		//add the item and get the index
		nIndex = (UINT)SendMessage( hwndListView, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem );
      if(nIndex == (UINT)-1)
         break;

      // column 2
      sc_GetColumnText( pct, 1, lpb );
		//set the text and images for the sub-items
		//ImageList_GetImageInfo(himl, i, &ii);
		//wsprintf(lpb, "%d", pct->ct_sFD.nFileSizeLow );
//      li.LowPart  = pct->ct_sFDSrc.nFileSizeLow;
//      li.HighPart = pct->ct_sFDSrc.nFileSizeHigh;
//      ptmp = GetI64Stg( &li );
//      *lpb = 0;
//      dwi = strlen(ptmp);
//      while(dwi < MXLVNUM)
//      {
//         strcat(lpb, " ");
//         dwi++;
//      }
//      strcat(lpb, ptmp);

		lvItem.mask = LVIF_TEXT;   // reduce mask to just TEXT stuff
		lvItem.iSubItem = 1;       // start sub-item number

		if( !SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem) )
         break;

      // column 3
      sc_GetColumnText( pct, 2, lpb );
		lvItem.iSubItem = 2;
		if( !SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem) )
         break;

      // column 4
      sc_GetColumnText( pct, 3, lpb );
		lvItem.iSubItem = 3;
		if( !SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem) )
         break;
      // column 5
      sc_GetColumnText( pct, 4, lpb );
		lvItem.iSubItem = 4;
		if( !SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem) )
         break;

      ListView_SetCheckState( hwndListView, nIndex, TRUE ); // and CHECK the item

      cnt++;   // count another entry added
	}

   if( cnt == icnt )
   {
      bRet = TRUE;
      MyCommonDlgResults( hDlg );   // show the LIST distribution
   }

Done_List:

	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);

	UpdateWindow(hwndListView);

   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_WM_INITDIALOG
// Return type: INT_PTR 
// Arguments  : HWND hDlg
//            : LPARAM lParam
// Description: The big INIT for the SHOW COPY dialog box.
//              
///////////////////////////////////////////////////////////////////////////////
INT_PTR sc_WM_INITDIALOG( HWND hDlg, LPARAM lParam )
{
   INT_PTR     iRet = TRUE;
   //PLE      pHead = (PLE)lParam;
   PCFDLGSTR   pcfds = (PCFDLGSTR)lParam;
   PLE         pHead = pcfds->cf_pList;      // list of files to COPY to
   DWORD       dwi   = pcfds->dwCpyOpts;  // get current COPY OPTIONS
   INT         icnt = 0;
   HWND        hWnd = 0;
   
   ListCount2( pHead, &icnt );
   if( !icnt )
   {
      EndDialog( hDlg, (int)-2 );
      return FALSE;
   }

   //ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
   //ic.dwICC  = ICC_LISTVIEW_CLASSES;
   UseComCtrls( ICC_LISTVIEW_CLASSES ); // LIST box

#ifdef ADD_LIST_VIEW
   // this should be near last
   //CenterDialog( hDlg, hwndClient );

   hWnd = GetDlgItem( hDlg, IDC_LIST1 );  // get the LIST control
   if( !hWnd )
   {
      EndDialog( hDlg, (int)-2 );
      return FALSE;
   }

   InitListView( hWnd );  // set default desired view

	if( !InsertListViewItems( hDlg, hWnd, pHead, icnt ) )
   {
      EndDialog( hDlg, (int)-2 );
      return FALSE;
   }
#endif // #ifdef ADD_LIST_VIEW

   //SET_PROP( hDlg, SHOW_ATOM, pHead );
   SET_PROP( hDlg, SHOW_ATOM, pcfds );

   //SetDlgItemText(hDlg, IDC_LAB_FROM, dialog_leftname );
   //SetDlgItemText(hDlg, IDC_LAB_TO,   dialog_rightname );
   SetDlgItemText(hDlg, IDC_LAB_FROM, dlg_leftname );
   if( strcmpi( dlg_rightname, pcfds->cf_szDest ) ) {
      LPTSTR lpb2 = &g_szBuf2[0];
      sprintf(lpb2, "Note, was [%s], now [%s]! CHECK CAREFULLY",
         dlg_rightname, pcfds->cf_szDest );
      SetDlgItemText(hDlg, IDC_LAB_TO,   lpb2 );
   } else {
      SetDlgItemText(hDlg, IDC_LAB_TO,   dlg_rightname );
   }

   CheckCommonSet( hDlg, dwi );

   MyCommonDlgInit2( hDlg );
   // set left tree, like sprintf(lpb, "Left %d", pcfds->dwLeft );
   // to items like SetDlgItemText( hDlg, IDC_LABLEFT, lpb );

   // this should be near last
   CenterDialog( hDlg, hwndClient );

   return iRet;
}

VOID  SetOk2Ok( HWND hDlg )
{
   SetDlgItemText( hDlg, IDOK, "Ok" );
   bIsApply = FALSE;
}

VOID  SetOk2Apply( HWND hDlg )
{
   SetDlgItemText( hDlg, IDOK, "Apply" );
   bIsApply = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_OK
// Return type: BOOL 
// Argument   : HWND hDlg
// Description: Handler for the OK button. NOTE sometimes it says APPLY,
//              and any changes will be APPLIED to the LIST before it
// becomes OK again.
///////////////////////////////////////////////////////////////////////////////
BOOL  sc_OK( HWND hDlg )
{
   BOOL  bRet = FALSE;
   HWND  hwndLV = GetDlgItem( hDlg, IDC_LIST1 );
   //PLE   pHead = (PLE)GET_PROP( hDlg, SHOW_ATOM );
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP( hDlg, SHOW_ATOM );
   PLE         pHead = pcfds->cf_pList;
   if( hwndLV && pHead )
   {
      LVITEM   lvi;
      LPTSTR   lpb = &gszTmpBuf[0];
      INT      icnt = ListView_GetItemCount( hwndLV );
      INT   i, j;
      j = 0;
      bDnLvInit = FALSE;
      for( i = 0; i < icnt; i++ )
      {
         lvi.mask = LVIF_PARAM;
         lvi.iItem = i;    // Zero-based index of the item to which this structure refers
         lvi.iSubItem = 0;
         lvi.state = 0;
         lvi.stateMask = 0;
         lvi.pszText = lpb;
         *lpb = 0;
         lvi.cchTextMax = 256;
         lvi.iImage = 0;
         lvi.lParam = 0;
         if( ( ListView_GetItem( hwndLV, &lvi ) ) &&
             ( lvi.lParam ) )
         {
            PCPYTST pct = (PCPYTST)lvi.lParam;
            if( ListView_GetCheckState( hwndLV, i ) )
            {
               j++;
               pct->ct_dwFlag &= ~( flg_Delete );
            }
            else
            {
               pct->ct_dwFlag |= flg_Delete;
            }
         }
      }
      if( ( j == icnt ) &&
          ( !bIsApply ) )
      {
         EndDialog( hDlg, IDOK );
         bRet = TRUE;
      }
      else
      {
         ListCount2( pHead, &icnt );
         InsertListViewItems( hDlg, hwndLV, pHead, icnt );
         SetOk2Ok( hDlg );
      }
      bDnLvInit = TRUE;
   }
   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_RESTORE
// Return type: INT_PTR 
// Argument   : HWND hDlg
// Description: Handler for the RESTORE button
//              
///////////////////////////////////////////////////////////////////////////////
INT_PTR  sc_RESTORE( HWND hDlg )
{
   INT_PTR  iRet = FALSE;
   HWND  hwndLV = GetDlgItem( hDlg, IDC_LIST1 );
   //PLE   pHead = (PLE)GET_PROP( hDlg, SHOW_ATOM );
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP( hDlg, SHOW_ATOM );
   PLE         pHead = pcfds->cf_pList;
   PLE   pNext;
   PCPYTST pct;
   INT      i = 0;
   if( hwndLV && pHead )
   {
      bDnLvInit = FALSE;
      Traverse_List( pHead, pNext )
      {
         pct = (PCPYTST)pNext;
         if( pct->ct_dwFlag & flg_Delete )
         {
            pct->ct_dwFlag &= ~( flg_Delete );
            i++;
         }
      }
      if(i)
      {
         ListCount2( pHead, &i );
         InsertListViewItems( hDlg, hwndLV, pHead, i );
         if( bIsApply )
            SetOk2Ok( hDlg );
      }
      bDnLvInit = TRUE;
   }
   return iRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_DELETE
// Return type: INT_PTR 
// Argument   : HWND hDlg
// Description: Handler for the DELETE button
//              
///////////////////////////////////////////////////////////////////////////////
INT_PTR  sc_DELETE( HWND hDlg )
{
   INT_PTR  iRet = FALSE;
   HWND  hwndLV = GetDlgItem( hDlg, IDC_LIST1 );
   //PLE   pHead = (PLE)GET_PROP( hDlg, SHOW_ATOM );
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP( hDlg, SHOW_ATOM );
   PLE         pHead = pcfds->cf_pList;
   if( hwndLV && pHead )
   {
      LVITEM   lvi;
      LPTSTR   lpb = &gszTmpBuf[0];
      INT      icnt = ListView_GetItemCount( hwndLV );
      INT      i, j;
      j = 0;
      bDnLvInit = FALSE;
      for( i = 0; i < icnt; i++ )
      {
         lvi.mask = LVIF_PARAM | LVIF_STATE;
         lvi.iItem = i;    // Zero-based index of the item to which this structure refers
         lvi.iSubItem = 0;
         lvi.state = 0;
         lvi.stateMask = -1;  // could be just LVIS_SELECTED;
         lvi.pszText = lpb;
         *lpb = 0;
         lvi.cchTextMax = 256;
         lvi.iImage = 0;
         lvi.lParam = 0;
         if( ( ListView_GetItem( hwndLV, &lvi ) ) &&
             ( lvi.lParam ) )
         {
            PCPYTST pct = (PCPYTST)lvi.lParam;
            if( lvi.state & LVIS_SELECTED )
            {
               pct->ct_dwFlag |= flg_Delete;
               j++;
            }
         }
      }
      if(j)
      {
         ListCount2( pHead, &i );
         InsertListViewItems( hDlg, hwndLV, pHead, i );
         if( bIsApply )
            SetOk2Ok(hDlg);
      }
      bDnLvInit = TRUE;

   }
   return iRet;
}

BOOL  sc_INVERT( HWND hDlg )
{
   BOOL  bRet = FALSE;
   HWND  hwndLV = GetDlgItem( hDlg, IDC_LIST1 );
   //PLE   pHead = (PLE)GET_PROP( hDlg, SHOW_ATOM );
   PCFDLGSTR   pcfds = (PCFDLGSTR)GET_PROP( hDlg, SHOW_ATOM );
   PLE         pHead = pcfds->cf_pList;
   if( hwndLV && pHead )
   {
      LVITEM   lvi;
      LPTSTR   lpb = &gszTmpBuf[0];
      INT      icnt = ListView_GetItemCount( hwndLV );   // get COUNT in LIST
      INT   i, j;
      j = 0;

      bDnLvInit = FALSE;
      for( i = 0; i < icnt; i++ )
      {
         lvi.mask = LVIF_PARAM;
         lvi.iItem = i;    // Zero-based index of the item to which this structure refers
         lvi.iSubItem = 0;
         lvi.state = 0;
         lvi.stateMask = 0;
         lvi.pszText = lpb;
         *lpb = 0;
         lvi.cchTextMax = 256;
         lvi.iImage = 0;
         lvi.lParam = 0;
         if( ( ListView_GetItem( hwndLV, &lvi ) ) &&
             ( lvi.lParam ) )
         {
            //PCPYTST pct = (PCPYTST)lvi.lParam;
            if( ListView_GetCheckState( hwndLV, i ) )
               bRet = FALSE;
            else
               bRet = TRUE;

            ListView_SetCheckState( hwndLV, i, bRet );
         }
      }

      if( !bIsApply )
         SetOk2Apply(hDlg);

      bDnLvInit = TRUE;
      bRet = TRUE;

   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_WM_COMMAND
// Return type: INT_PTR 
// Arguments  : HWND hDlg
//            : WPARAM wParam
//            : LPARAM lParam
// Description: Very typical dialog box COMMAND handler
//              
///////////////////////////////////////////////////////////////////////////////
INT_PTR sc_WM_COMMAND( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  iRet = 0;
   DWORD    cmd  = LOWORD(wParam);
   switch(cmd)
   {
   case IDOK:
      sc_OK( hDlg );
      iRet = TRUE;
      break;

   case IDC_BUT_OPTS:
      EndDialog( hDlg, IDC_BUT_OPTS );
      iRet = TRUE;
      break;

   case IDCANCEL:
      EndDialog( hDlg, IDCANCEL );
      iRet = TRUE;
      break;

   case IDC_BUT_RESTORE:
      iRet = sc_RESTORE( hDlg );
      break;

   case IDC_BUT_DELETE:
      iRet = sc_DELETE( hDlg );
      break;

   case IDC_BUT_INVERT:
      iRet = sc_INVERT( hDlg );
      break;

   }

   return iRet;
}

INT CALLBACK pctCompareProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
   INT      iRet = 0;
   LPTSTR   lps1 = g_szBuf1;
   LPTSTR   lps2 = g_szBuf2;
   INT      i1, i2;
   PSORTPARAM  ps = (PSORTPARAM)lParamSort;
   INT   iCol = ps->ccol;

   *lps1 = 0;
   *lps2 = 0;
   i1 = sc_GetColumnText( (PCPYTST)lParam1, iCol, lps1 );
   i2 = sc_GetColumnText( (PCPYTST)lParam2, iCol, lps2 );
   iRet = strcmp(lps1,lps2);

   if( ps->brev )
   {
      if(iRet < 0)
         iRet = +1;
      else if(iRet > 0)
         iRet = -1;
   }

   return iRet;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : sc_WM_NOTIFY
// Return type: INT_PTR 
// Arguments  : HWND hDlg
//            : WPARAM wParam
//            : LPARAM lParam
// Description: Handle some of the NOTIFY messages from the LIST control
//              Essentially, if MODIFIED, the the OK become APPLY, and
// this must be APPLIED before the OK returns.
//typedef struct tagNMLISTVIEW{
//    NMHDR   hdr;
//    int     iItem;
//    int     iSubItem;
//    UINT    uNewState;
//    UINT    uOldState;
//    UINT    uChanged;
//    POINT   ptAction;
//    LPARAM  lParam;
//} NMLISTVIEW, FAR *LPNMLISTVIEW;
///////////////////////////////////////////////////////////////////////////////
INT_PTR  sc_WM_NOTIFY( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
   INT_PTR  bRet = FALSE;
   // WM_NOTIFY 
   int      idCtrl = (int) wParam;
   LPNMHDR  pnmh   = (LPNMHDR) lParam;
   if( ( bDnLvInit ) &&
       ( idCtrl == IDC_LIST1 ) &&
       ( pnmh ) )
   {
      UINT  code = pnmh->code;
      LPNMLISTVIEW plv = (LPNMLISTVIEW)pnmh;
      switch(code)
      {
      case LVN_ITEMCHANGING:  //  (LVN_FIRST-0)
         //sprtf( "LVN_ITEMCHANGING item = %d"MEOR, plv->iItem );
         break;
      case LVN_ITEMCHANGED:   //  (LVN_FIRST-1)
         sprtf( "LVN_ITEMCHANGED item = %d"MEOR, plv->iItem );
         SetOk2Apply( hDlg );
         break;

      case LVN_INSERTITEM: // (LVN_FIRST-2)
      case LVN_DELETEITEM: // (LVN_FIRST-3)
      case LVN_DELETEALLITEMS:   //  (LVN_FIRST-4)
         break;
      case LVN_COLUMNCLICK:
		   {
//   	lvItem.lParam = (LPARAM)pct;	// 32-bit value to associate with item
			   INT ccol   = plv->iSubItem;
            INT iItem  = plv->iItem;
            SORTPARAM   sp;
            if( ccol < DLGLVCC )
            {
               sp.ccol = ccol;
               sp.brev = (dwlvcFlag[ccol] & dwflg_Rev);   // array [DLGLVCC];

   			   ListView_SortItems( plv->hdr.hwndFrom,	// ListView HANDLE
                  pctCompareProc,	// Compare PROC
                  &sp );

               dwlvcFlag[ccol] ^= dwflg_Rev;   // array [DLGLVCC];   // next time reverse
            }
         }
         break;

      default:
         break;
      }
   }
   return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SHOWCOPYDLG
// Return type: INT_PTR CALLBACK 
// Arguments  : HWND hDlg // handle to dialog box
//            : UINT uMsg // message
//            : WPARAM wParam // first message parameter
//            : LPARAM lParam // second message parameter
// Description: Very TYPICAL dialog box handler. Nothing special here. 
//              from resource MAKEINTRESOURCE(IDD_SHOWCOPY),
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK SHOWCOPYDLG(
  HWND hDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
   INT_PTR iRet = 0;
   switch(uMsg)
   {
   case WM_INITDIALOG:
      iRet = sc_WM_INITDIALOG( hDlg, lParam );
      bDnLvInit = TRUE;
      break;
   case WM_COMMAND:
      iRet = sc_WM_COMMAND( hDlg, wParam, lParam );
      break;
   case WM_NOTIFY:
//      sprtf( "WM_NOTIFY: To SHOWCOPYDLG %#x %#x"MEOR, wParam, lParam );
      iRet = sc_WM_NOTIFY( hDlg, wParam, lParam );
      break;
   case WM_DESTROY:
      REMOVE_PROP( hDlg, SHOW_ATOM );
      break;
   }
   return iRet;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : complist_showcopydlg
// Return type: INT 
// Arguments  : PVOID pv
//            : INT icnt
// Description: Put up a DIALOG to show the LIST that is about to be COPIED,
//              and allow single elements to be DELETED. Actually they
// are NEVER deleted, just marked with the flg_Delete! This allows RESTORE
// to work fine.
// Is actually passed    PCFDLGSTR   pcfds = &g_sCFDLGSTR;   // get GLOBAL structure
//
///////////////////////////////////////////////////////////////////////////////
INT complist_showcopydlg( PVOID pv, INT icnt, PINT picnt )
{
   INT      iRet = 0;
   INT      i;
   PCFDLGSTR pcfds = (PCFDLGSTR)pv;

   bDnLvInit = FALSE;
   bIsApply  = FALSE;

   dc4w_UI(TRUE);
   i = DialogBoxParam( g_hInst,
      MAKEINTRESOURCE(IDD_SHOWCOPY),
      hwndClient,
      SHOWCOPYDLG,
      (LPARAM)pcfds );
   dc4w_UI(FALSE);
   *picnt = 0; // set list count ZERO
   if( i == (INT)-2 )
   {
      MB( NULL, "ERROR: Initialisation of the SHOW COPY LIST dialog FAILED"MEOR
         "No files will be COPIED!",
         "ERROR SHOW COPY LIST",
         (MB_OK | MB_ICONINFORMATION) );
   }
   else if( i == IDOK )
   {
      //ListCount2(pcfds->cf_pList, &iRet);
      ListCount2( pcfds->cf_pList, picnt );
   }
   else if( i == IDC_BUT_OPTS )
   {
      iRet++;
   }

//   if( ghFixedFont )
//      DeleteObject( ghFixedFont );

   return iRet;
}

// eof - dc4wDlgR.c

