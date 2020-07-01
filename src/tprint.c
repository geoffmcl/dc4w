
/****************************** Module Header *******************************
* Module Name: TPRINT.C
*
* Print functions.
*
* Functions:
*
* gtab_print()
* gtab_printsetup()
* gtab_prtwidths()
* gtab_printjob()
* AbortProc()
* AbortDlg()
* gtab_printpage()
* gtab_setrects()
* gtab_printhead()
*
* Comments:
*
* See table.h for interface description.
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"
#include "tpriv.h"

/* in tpaint.c, calls GetTextExtentPoint */
extern int GetTextExtent(HDC, LPSTR, int);

extern HANDLE hLibInst;

/* function prototypes */
lpTable gtab_printsetup(HWND hwnd, lpTable ptab, HANDLE heap,
        lpPrintContext pcontext);
BOOL gtab_prtwidths(HWND hwnd, lpTable ptab, HANDLE heap, lpPrintContext
        pcontext);
void gtab_printjob(HWND hwnd, lpTable ptab, lpPrintContext pcontext);
int APIENTRY AbortProc(HDC hpr, int code);
int APIENTRY AbortDlg(HWND hdlg, UINT msg, UINT wParam, LONG lParam);
BOOL gtab_printpage(HWND hwnd, lpTable ptab, lpPrintContext pcontext, int page);
void gtab_setrects(lpPrintContext pcontext, LPRECT rcinner, LPRECT rcouter);
void gtab_printhead(HWND hwnd, HDC hdc, lpTable ptab, lpTitle head, int page);


/***************************************************************************
 * Function: gtab_print
 *
 * Purpose:
 *
 * Prints a table.
 */
void
gtab_print(HWND hwnd, lpTable ptab, HANDLE heap, lpPrintContext pcontext)
{
        BOOL fNoContext, fNoMargin, fNoPD;
        lpTable ptab_prt;

        fNoContext = FALSE;
        fNoPD = FALSE;
        fNoMargin = FALSE;

        if (pcontext == NULL) {
                fNoContext = TRUE;
                pcontext = (lpPrintContext) gmem_get(heap, sizeof(PrintContext), "gtab_print" );
                pcontext->head = pcontext->foot = NULL;
                pcontext->margin = NULL;
                pcontext->pd = NULL;
                pcontext->id = 0;
        }
        if (pcontext->pd == NULL) {
                fNoPD = TRUE;
        }
        if (pcontext->margin == NULL) {
                fNoMargin = TRUE;
        }
        ptab_prt = gtab_printsetup(hwnd, ptab, heap, pcontext);

        if (ptab_prt != NULL) {
                gtab_printjob(hwnd, ptab_prt, pcontext);

                gtab_deltable(hwnd, ptab_prt);
        }
        if (fNoMargin) {
           gmem_free(heap, (LPSTR)pcontext->margin, sizeof(Margin), "gtab_printsetup0" );
                pcontext->margin = NULL;
        }
        if (fNoPD) {
                if (pcontext->pd->hDevMode != NULL) {
                        GlobalFree(pcontext->pd->hDevMode);
                }
                if (pcontext->pd->hDevNames != NULL) {
                        GlobalFree(pcontext->pd->hDevNames);
                }
           gmem_free(heap, (LPSTR) pcontext->pd, sizeof(PRINTDLG), "gtab_printsetup1" );
                pcontext->pd = NULL;
        }
        if (fNoContext) {
           gmem_free(heap, (LPSTR) pcontext, sizeof(PrintContext), "gtab_print");
        }
}



/***************************************************************************
 * Function: gtab_printsetup
 *
 * Purpose:
 *
 * Sets up printercontext - builds lpTable for printer, incl. sizing
 * and initialises pcontext fields that may be null.
 */
lpTable
gtab_printsetup(HWND hwnd, lpTable ptab, HANDLE heap, lpPrintContext pcontext)
{
        lpTable pprttab;
        PRINTDLG FAR * pd;
        int ncols, i;
        ColPropsList cplist;

        /* set fields for context that user left null */
        if (pcontext->margin == NULL) {
                pcontext->margin = (lpMargin) gmem_get(heap, sizeof(Margin), "gtab_printsetup0");
                if (pcontext->margin == NULL) {
                        return(NULL);
                }
                pcontext->margin->left = 10;
                pcontext->margin->right = 10;
                pcontext->margin->top = 15;
                pcontext->margin->bottom = 15;
                pcontext->margin->topinner = 15;
                pcontext->margin->bottominner = 15;
        }

        if (pcontext->pd == NULL) {
                pd = (PRINTDLG FAR *) gmem_get(heap, sizeof(PRINTDLG), "gtab_printsetup1");
                if (pd == NULL) {
                        return(NULL);
                }
                pcontext->pd = pd;

                pd->lStructSize = sizeof(PRINTDLG);
                pd->hwndOwner = hwnd;
                pd->hDevMode = (HANDLE) NULL;
                pd->hDevNames = (HANDLE) NULL;
                pd->Flags = PD_RETURNDC|PD_RETURNDEFAULT;

                if (PrintDlg(pd) == FALSE) {
                        return(NULL);
                }
        }

        /* now create a Table struct by querying the owner */
        pprttab = (lpTable) gmem_get(heap, sizeof(Table), "gtab_printsetup2");
        if (pprttab == NULL) {
                return(NULL);
        }
        pprttab->hdr = ptab->hdr;

        /* get the row/column count from owner window */
        if (pcontext->id == 0) {
                pprttab->hdr.id = ptab->hdr.id;
        } else {
                pprttab->hdr.id = pcontext->id;
        }
        pprttab->hdr.props.valid = 0;
        pprttab->hdr.sendscroll = FALSE;
        if (gtab_sendtq(hwnd, TQ_GETSIZE, (LPARAM)&pprttab->hdr) == FALSE) {
                return(NULL);
        }

        /* alloc and init the col data structs */
        ncols = pprttab->hdr.ncols;
        pprttab->pcolhdr = (lpColProps) gmem_get(heap, (sizeof(ColProps) * ncols), "gtab_printsetup3" );
        if (pprttab->pcolhdr == NULL) {
             gmem_free(heap, (LPSTR)pprttab, sizeof(Table),"gtab_printsetup2");
             return(NULL);
        }

        /* init col properties to default */
        for (i=0; i < ncols; i++) {
                pprttab->pcolhdr[i].props.valid = 0;
                pprttab->pcolhdr[i].nchars = 0;
        }
        /* get the column props from owner */
        cplist.plist = pprttab->pcolhdr;
        cplist.id = pprttab->hdr.id;
        cplist.startcol = 0;
        cplist.ncols = ncols;
        gtab_sendtq(hwnd, TQ_GETCOLPROPS, (LPARAM)&cplist);


        pprttab->scrollscale = 1;
        pprttab->pcellpos = (lpCellPos) gmem_get(heap, (sizeof(CellPos) * ptab->hdr.ncols), "gtab_printsetup4" );
        if (pprttab->pcellpos == NULL) {
           gmem_free(heap, (LPSTR) pprttab->pcolhdr, (sizeof(ColProps) * ncols), "gtab_printsetup3" );
           gmem_free(heap, (LPSTR)pprttab, sizeof(Table), "gtab_printsetup2" );
                return(NULL);
        }
                

        pprttab->pdata = NULL;
        pprttab->nlines = 0;

        if (!gtab_prtwidths(hwnd, pprttab, heap, pcontext)) {
           gmem_free(heap, (LPSTR)pprttab->pcellpos,(sizeof(CellPos) * ptab->hdr.ncols),"gtab_printsetup4");
           gmem_free(heap, (LPSTR)pprttab, sizeof(Table), "gtab_printsetup2" );
                return(NULL);
        }
        return(pprttab);
}


/***************************************************************************
 * Function: gtab_prtwidths
 *
 * Purpose:
 *
 * Calc the height/width settings and alloc line data 
 */
BOOL
gtab_prtwidths(HWND hwnd, lpTable ptab, HANDLE heap, lpPrintContext pcontext)
{
        TEXTMETRIC tm;
        int cx, cxtotal, i, curx, cury;
        lpProps hdrprops, cellprops;
        lpCellPos xpos, ypos;
        RECT rcinner, rcouter;

        hdrprops = &ptab->hdr.props;

        // what FONT (LOGFONT) is selected into the pcontext
        GetTextMetrics(pcontext->pd->hDC, &tm);
        ptab->avewidth = tm.tmAveCharWidth;
        ptab->rowheight = tm.tmHeight + tm.tmExternalLeading;

        if (hdrprops->valid & P_HEIGHT) {
                ptab->rowheight = hdrprops->height;
        }

        /* set sizes for headers */
        gtab_setrects(pcontext, &rcinner, &rcouter);

        /* set width/pos for each col. */
        cxtotal = 0;
        curx = rcinner.left;
        for (i = 0; i < ptab->hdr.ncols; i++) {
                cellprops = &ptab->pcolhdr[i].props;
                xpos = &ptab->pcellpos[i];

                if (cellprops->valid & P_WIDTH) {
                        cx = cellprops->width;
                } else if (hdrprops->valid & P_WIDTH) {
                        cx = hdrprops->width;
                } else {
                        cx = ptab->pcolhdr[i].nchars + 1;
                        cx *= ptab->avewidth;
                }
                /* add 2 for intercol spacing */
                cx += 2;

                xpos->px_size = cx;
                xpos->start = curx + 1;
                xpos->clipstart = xpos->start;
                xpos->clipend = xpos->start + xpos->px_size - 2;
                xpos->clipend = min(xpos->clipend, rcinner.right);

                cxtotal += xpos->px_size;
                curx += xpos->px_size;
        }
        ptab->rowwidth = cxtotal;

        if(pcontext->head != NULL) {
                xpos = &pcontext->head->xpos;
                ypos = &pcontext->head->ypos;

                xpos->start = rcouter.left + 1;
                xpos->clipstart = rcouter.left + 1;
                xpos->clipend = rcouter.right - 1;
                xpos->px_size = rcouter.right - rcouter.left;

                ypos->start = rcouter.top;
                ypos->clipstart = rcouter.top;
                ypos->clipend = rcinner.top;
                ypos->px_size = ptab->rowheight;
        }

        if (pcontext->foot != NULL) {
                xpos = &pcontext->foot->xpos;
                ypos = &pcontext->foot->ypos;

                xpos->start = rcouter.left + 1;
                xpos->clipstart = rcouter.left + 1;
                xpos->clipend = rcouter.right - 1;
                xpos->px_size = rcouter.right - rcouter.left;

                ypos->start = rcouter.bottom - ptab->rowheight;
                ypos->clipstart = rcinner.bottom;
                ypos->clipend = rcouter.bottom;
                ypos->px_size = ptab->rowheight;
        }

        /* set nr of lines per page */
        ptab->nlines = (rcinner.bottom - rcinner.top) / ptab->rowheight;
        if (!gtab_alloclinedata(hwnd, heap, ptab)) {
                return(FALSE);
        }
        /* set line positions */
        cury = rcinner.top;
        for (i = 0; i < ptab->nlines; i++) {
                ypos = &ptab->pdata[i].linepos;
                ypos->start = cury;
                ypos->clipstart = ypos->start;
                ypos->clipend = ypos->start + ypos->px_size;
                ypos->clipend = min(ypos->clipend, rcinner.bottom);
                cury += ypos->px_size;
        }
        return(TRUE);
}


/* static information for this module */
BOOL bAbort;
FARPROC lpAbortProc;
DLGPROC lpAbortDlg;
HWND hAbortWnd;
int npage;
int pages;

/***************************************************************************
 * Function: gtab_printjob
 *
 * Purpose:
 *
 * Sets up print job and dialogs
 */ 
void
gtab_printjob(HWND hwnd, lpTable ptab, lpPrintContext pcontext)
{
        int moveables;
        int endpage;
        int startpage = 1;
        HDC hpr;
        int status;
        HANDLE hcurs;
        static char str[256];
        DOCINFO di;

        CHAR szPage[60];  /* for LoadString */


        hcurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

        moveables = ptab->nlines - ptab->hdr.fixedrows;
        pages = (int) (ptab->hdr.nrows - ptab->hdr.fixedrows + moveables - 1)
                        / moveables;
        endpage = pages;

        if (pcontext->pd->Flags & PD_PAGENUMS) {
                startpage = pcontext->pd->nFromPage;
                endpage = pcontext->pd->nToPage;
        }
        hpr = pcontext->pd->hDC;

        lpAbortDlg = (DLGPROC) MakeProcInstance((WNDPROC) AbortDlg, hLibInst);
        lpAbortProc = (FARPROC) MakeProcInstance((WNDPROC)AbortProc, hLibInst);

        SetAbortProc(hpr, (ABORTPROC) lpAbortProc);

        ZeroMemory( &di, sizeof( DOCINFO ) );
        di.lpszDocName = "Table";
        di.cbSize = sizeof( DOCINFO );
        di.lpszOutput = NULL;

        StartDoc(hpr, &di);

        bAbort = FALSE;

        /* add abort modeless dialog later!! */
        hAbortWnd = CreateDialog(hLibInst,
           MAKEINTRESOURCE(IDD_GABRTDLG),
           hwnd, lpAbortDlg);
        if (hAbortWnd != NULL) {
                ShowWindow(hAbortWnd, SW_NORMAL);
                EnableWindow(hwnd, FALSE);
        }
        SetCursor(hcurs);

        for (npage = startpage; npage<=endpage; npage++) {
                LoadString(hLibInst, IDS_PAGE_STR, szPage, sizeof(szPage));
                wsprintf(str, szPage,  npage, pages);
                SetDlgItemText(hAbortWnd, IDC_LPAGENR, str);
                status = gtab_printpage(hwnd, ptab, pcontext, npage);
                if (status < 0) {
                        AbortDoc(hpr);
                        break;
                }
        }
        if (status >= 0) {
                EndDoc(hpr);
        }
        
        if (hAbortWnd != NULL) {
                EnableWindow(hwnd, TRUE);
                DestroyWindow(hAbortWnd);
        }
        FreeProcInstance((WNDPROC) lpAbortDlg);
        FreeProcInstance(lpAbortProc);

        DeleteDC(hpr);
}

/***************************************************************************
 * Function: AbortProc
 *
 * Purpose:
 *
 * Abort procedure for print job
 */
int APIENTRY
AbortProc(HDC hpr, int code)
{

        MSG msg;

        if (!hAbortWnd) {
                return(TRUE);
        }
        while (!bAbort && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (!IsDialogMessage(hAbortWnd, &msg)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                }
        }
        return(!bAbort);
}

/***************************************************************************
 * Function: AbortDlg
 *
 * Purpose:
 *
 * Dialog for abort procedure
 */
int APIENTRY
AbortDlg(HWND hdlg, UINT msg, UINT wParam, LONG lParam)
{
        switch(msg) {

        case WM_COMMAND:
                bAbort = TRUE;
                DestroyWindow (hdlg);
                return TRUE;

        case WM_INITDIALOG:
                return TRUE;
        }
        return(FALSE);
}

/***************************************************************************
 * Function: gtab_printpage
 *
 * Purpose:
 *
 * Print a single page. page number is 1-based
 */
BOOL
gtab_printpage(HWND hwnd, lpTable ptab, lpPrintContext pcontext, int page)
{
        HDC hpr;
        int moveables, i;
        int x1, y1, x2, y2;

        hpr = pcontext->pd->hDC;
        StartPage(hpr);

        moveables = ptab->nlines - ptab->hdr.fixedrows;
        ptab->toprow = moveables * (page-1);
        gtab_invallines(hwnd, ptab, ptab->hdr.fixedrows, moveables);

        for (i =0; i < ptab->nlines; i++) {
                gtab_paint(hwnd, hpr, ptab, i);
        }
        if ((ptab->hdr.vseparator) && (ptab->hdr.fixedcols > 0)) {
                x1 = ptab->pcellpos[ptab->hdr.fixedcols -1].clipend+1;
                y1 = ptab->pdata[0].linepos.clipstart;
                y2 = ptab->pdata[ptab->nlines-1].linepos.clipend;
                MoveToEx(hpr, x1, y1, NULL);
                LineTo(hpr, x1, y2);
        }
        if ((ptab->hdr.hseparator) && (ptab->hdr.fixedrows > 0)) {
                y1 = ptab->pdata[ptab->hdr.fixedrows-1].linepos.clipend;
                x1 = ptab->pcellpos[0].clipstart;
                x2 = ptab->pcellpos[ptab->hdr.ncols-1].clipend;
                MoveToEx(hpr, x1, y1, NULL);
                LineTo(hpr, x2, y1);
        }

        if (pcontext->head != NULL) {
                gtab_printhead(hwnd, hpr, ptab, pcontext->head, page);
        }
        if (pcontext->foot != NULL) {
                gtab_printhead(hwnd, hpr, ptab, pcontext->foot, page);
        }

        return(EndPage(hpr));
}


/***************************************************************************
 * Function: gtab_setrects
 *
 * Purpose:
 *
 * Calculate the outline positions in pixels for the headers
 * (outer rect) and for the page itself (inner rect). Based on
 * page size and PrintContext margin info (which is in millimetres).
 */
void
gtab_setrects(lpPrintContext pcontext, LPRECT rcinner, LPRECT rcouter)
{
        HDC hpr;
        int hpixels, hmms;
        int vpixels, vmms;
        int h_pixpermm, v_pixpermm;

        hpr = pcontext->pd->hDC;
        hpixels = GetDeviceCaps(hpr, HORZRES);
        vpixels = GetDeviceCaps(hpr, VERTRES);
        vmms = GetDeviceCaps(hpr, VERTSIZE);
        hmms = GetDeviceCaps(hpr, HORZSIZE);

        h_pixpermm = hpixels / hmms;
        v_pixpermm = vpixels / vmms;

        rcouter->top = (pcontext->margin->top * v_pixpermm);
        rcouter->bottom = vpixels - (pcontext->margin->bottom * v_pixpermm);
        rcouter->left = (pcontext->margin->left * h_pixpermm);
        rcouter->right = hpixels - (pcontext->margin->right * h_pixpermm);

        rcinner->left = rcouter->left;
        rcinner->right = rcouter->right;
        rcinner->top = rcouter->top +
                (pcontext->margin->topinner * v_pixpermm);
        rcinner->bottom = rcouter->bottom -
                (pcontext->margin->bottominner * v_pixpermm);
}


/***************************************************************************
 * Function: gtab_printhead
 *
 * Purpose:
 *
 * Print header information
 */
void
gtab_printhead(HWND hwnd, HDC hdc, lpTable ptab, lpTitle head, int page)
{
        RECT rc, rcbox;
        int i, cx, x, y, tab;
        UINT align;
        LPSTR chp, tabp;
        DWORD fcol, bkcol;
        char str[256];

        rc.top = head->ypos.clipstart;
        rc.bottom = head->ypos.clipend;
        rc.left = head->xpos.clipstart;
        rc.right = head->xpos.clipend;

        /* update page number */
        chp = str;
        for (i = 0; i < lstrlen(head->ptext); i++) {
                switch(head->ptext[i]) {

                case '#':
                        chp += wsprintf(chp, "%d", page);
                        break;
                
                case '$':
                        chp += wsprintf(chp, "%d", pages);
                        break;
                
                default:
                        if (IsDBCSLeadByte(head->ptext[i]) &&
                            head->ptext[i+1]) {
                                *chp = head->ptext[i];
                                chp++;
                                i++;
                        }
                        *chp++ = head->ptext[i];
                        break;
                }
        }
        *chp = '\0';
        chp = str;

        if (head->props.valid & P_ALIGN) {
                align = head->props.alignment;
        } else {
                align = P_LEFT;
        }

        /* set colours if not default */
        if (head->props.valid & P_FCOLOUR) {
                fcol = SetTextColor(hdc, head->props.forecolour);
        }
        if (head->props.valid & P_BCOLOUR) {
                bkcol = SetBkColor(hdc, head->props.backcolour);
        }

        /* calc offset of text within cell for right-align or centering */
        if (align == P_LEFT) {
                cx = ptab->avewidth/2;
        } else {
                cx = LOWORD(GetTextExtent(hdc, chp, lstrlen(chp)));
                if (align == P_CENTRE) {
                        cx = (head->xpos.px_size - cx) / 2;
                } else {
                        cx = head->xpos.px_size - cx - (ptab->avewidth/2);
                }
        }
        cx += head->xpos.start;

        /* expand tabs on output */
        tab = ptab->avewidth * 8;
        x = 0;
        y = head->ypos.start;

        for ( ; (tabp = strchr(chp, '\t')) != NULL; ) {
                /* perform output upto tab char */
                ExtTextOut(hdc, x+cx, y, ETO_CLIPPED, &rc, chp, (UINT)(tabp-chp), NULL);
                
                /* advance past the tab */
                x += LOWORD(GetTextExtent(hdc, chp, (int)(tabp - chp)));
                x = ( (x + tab) / tab) * tab;
                chp = ++tabp;
        }

        /*no more tabs - output rest of string */
        ExtTextOut(hdc, x+cx, y, ETO_CLIPPED, &rc, chp, lstrlen(chp), NULL);

        /* reset colours to original if not default */
        if (head->props.valid & P_FCOLOUR) {
                SetTextColor(hdc, fcol);
        }
        if (head->props.valid & P_BCOLOUR) {
                SetBkColor(hdc, bkcol);
        }

        /* now box cell if marked */
        if (head->props.valid & P_BOX) {
                if (head->props.box != 0) {
                        rcbox.top = head->ypos.start;
                        rcbox.bottom = rcbox.top + head->ypos.px_size;
                        rcbox.left = head->xpos.start;
                        rcbox.right = rcbox.left + head->xpos.px_size;
                        gtab_boxcell(hwnd, hdc, &rcbox, &rc, head->props.box);
                }
        }
}

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - tprint.c
