
/****************************** Module Header *******************************
* Module Name: dc4wUtil.C
*
* standard file-reading utilities.
*
* Functions:
*
* readfile_new()
* readfile_next()
* readfile_delete()
* utils_CompPath() and its LOWER CASE mate utils_CompPath2()
* has_string()
* utils_isblank()
* StringInput()
* dodlg_stringin()
* LPTSTR GetModulePathStg( VOID )
* LPTSTR GetCWDStg( VOID )
* Comments:
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>

#include "dc4w.h"
// #include "dc4wUtil.h"
/*
 * we need an instance handle. this should be the dll instance
 */
extern   HANDLE hLibInst;
extern   VOID  compitem_retrelname( LPTSTR lpb, COMPITEM ci );

/*
 * -- forward declaration of procedures -----------------------------------
 */
int FAR PASCAL dodlg_stringin(HWND hDlg, UINT message, UINT wParam, LONG lParam);
void    Dbl2Stg( LPSTR lps, double factor, int prec );

/*-- readfile: buffered line input ------------------------------*/

/*
 * set of functions to read a line at a time from a file, using
 * a buffer to read a block at a time from the file
 *
 */

static UINT dc4w_lread( HFILE hFile, LPVOID lpBuffer, UINT uBytes )
{
    UINT read = _lread(hFile,lpBuffer,uBytes);
    return read;
}

/* FIX20120116 - Definition moved the dc4wUtil.h - to propogate the 'flag'
 * a FILEBUFFER handle is a pointer to a struct filebuffer
#define MMX_LINE_BUF 512
struct filebuffer {
        int fh;         /=* open file handle *=/
        PSTR start;     /=* offset within buffer of next character *=/
        PSTR last;      /=* offset within buffer of last valid char read in *=/
        int file_flag;       /=* set 0x0001 if binary *=/
        char buffer[MMX_LINE_BUF];
};
 */

/***************************************************************************
 * Function: readfile_new
 *
 * Purpose:
 *
 * Initialise a filebuffer and return a handle to it
 */
FILEBUFFER APIENTRY
readfile_new(int fh)
{
        FILEBUFFER fbuf;

        fbuf = (FILEBUFFER) LocalLock(LocalAlloc(LHND, sizeof(struct filebuffer)));
        if (fbuf == NULL) {
                return(NULL);
        }

        fbuf->fh = fh;
        fbuf->start = fbuf->buffer;
        fbuf->last = fbuf->buffer;
        fbuf->file_flag = 0;
        /* return file pointer to beginning of file */
        _llseek(fh, 0, 0);

        return(fbuf);
}

/***************************************************************************
 * Function: readfile_next
 *
 * Purpose:
 *
 * Get the next line from a file. Returns a pointer to the line
 * in the buffer - so copy it before changing it.
 *
 * The line is *not* null-terminated. *plen is set to the length of the
 * line.
 */

LPSTR APIENTRY
readfile_next(FILEBUFFER fbuf, int * plen)
{
        PSTR cstart;

        /* look for an end of line in the buffer we have*/
        for (cstart = fbuf->start; cstart < fbuf->last; cstart++) {
            if (*cstart == 0) {
                // Good indication that it is BINARY
                fbuf->file_flag |= ff_found_zero;
                return (NULL);
            }

                if (*cstart == '\n') {
                        *plen = (cstart - fbuf->start) + 1;
                        cstart = fbuf->start;
                        fbuf->start += *plen;
                        return(cstart);
                }

        }

        /* no cr in this buffer - this buffer contains a partial line.
         * copy the partial up to the beginning of the buffer, and
         * adjust the pointers to reflect this move
         */
        Old_strncpy(fbuf->buffer, fbuf->start, fbuf->last - fbuf->start);
        fbuf->last = &fbuf->buffer[fbuf->last - fbuf->start];
        fbuf->start = fbuf->buffer;

        /* read in to fill the block */
        fbuf->last += dc4w_lread(fbuf->fh, fbuf->last,
                        &fbuf->buffer[sizeof(fbuf->buffer)] - fbuf->last);

        /* look for an end of line in the newly filled buffer */
        for (cstart = fbuf->start; cstart < fbuf->last; cstart++) {

                if (*cstart == '\n') {
                        *plen = (cstart - fbuf->start) + 1;
                        cstart = fbuf->start;
                        fbuf->start += *plen;
                        return(cstart);
                }
        }


        /* still no end of line. either the buffer is empty -
         * because of end of file - or the line is longer than
         * the buffer. in either case, return all that we have
         */
        *plen = fbuf->last - fbuf->start;
        { // for JAPAN (nChars != nBytes)
            PSTR ptr;
            for(ptr=fbuf->start;ptr<fbuf->last;ptr++) ;
            if(ptr!=fbuf->last && *plen) {
                --(*plen);
                --(fbuf->last);
                _llseek(fbuf->fh,-1,1);
            }
        }
        cstart = fbuf->start;
        fbuf->start += *plen;
        if (*plen == 0) {
                return(NULL);
        } else {
                return(cstart);
        }
}


/***************************************************************************
 * Function: readfile_delete
 *
 * Purpose:
 *
 * Delete a FILEBUFFER - close the file handle and free the buffer
 */
void APIENTRY
readfile_delete(FILEBUFFER fbuf)
{
        _lclose(fbuf->fh);

        LocalUnlock(LocalHandle( (PSTR) fbuf));
        LocalFree(LocalHandle( (PSTR) fbuf));
}


/* ----------- things for strings-------------------------------------*/


/*
 * Compare two pathnames, and if not equal, decide which should come first.
 * Both path names should be lower cased by AnsiLowerBuff before calling.
 *
 * Returns 0 if the same, -1 if left is first, and +1 if right is first.
 *
 * The comparison is such that all filenames in a directory come before any
 * file in a subdirectory of that directory.
 *
 * Given direct\thisfile v. direct\subdir\thatfile, we take
 * thisfile < thatfile   even though it is second alphabetically.
 * We do this by picking out the shorter path
 * (fewer path elements), and comparing them up till the last element of that
 * path (in the example: compare the 'dir\' in both cases.)
 * If they are the same, then the name with more path elements is
 * in a subdirectory, and should come second.
 *
 * We have had trouble with apparently multiple collating sequences and
 * the position of \ in the sequence.  To eliminate this trouble
 * a. EVERYTHING is mapped to lower case first (actually this is done
 *    before calling this routine).
 * b. All comparison is done by using lstrcmpi with two special cases.
 *    1. Subdirs come after parents as noted above
 *    2. \ must compare low so that fred2\x > fred\x in the same way
 *       that fred2 < fred.  Unfortunately in ANSI '2' < '\\'
 *
 */
int APIENTRY
utils_CompPath_NOT_USED(LPSTR left, LPSTR right)
{
        int compval;            // provisional value of comparison

        if (left==NULL) return -1;        // empty is less than anything else
        else if (right==NULL) return 1;  // anything is greater than empty

        for (; ; ) {
                if (*left=='\0' && *right=='\0') return 0;
                if (*left=='\0')  return -1;
                if (*right=='\0')  return 1;
                if (IsDBCSLeadByte(*left) || IsDBCSLeadByte(*right)) {
                        if (*right != *left) {
                                compval = (*left - *right);
                                break;
                        }
                        ++left;
                        ++right;
                        if (*right != *left) {
                                compval = (*left - *right);
                                break;
                        }
                        ++left;
                        ++right;
                } else {
                if (*right==*left)  {++left; ++right; continue;}
                if (*left=='\\') {compval = -1; break;}
                if (*right=='\\') {compval = 1; break;}
                compval = (*left - *right);
                break;
                }
        }

        /* We have detected a difference.  If the rest of one
           of the strings (including the current character) contains
           some \ characters, but the other one does not, then all
           elements up to the last element of the one with the fewer
           elements are equal and so the other one lies in a subdir
           and so compares greater i.e. x\y\f > x\f
           Otherwise compval tells the truth.
        */

        left = strchr(left, '\\');
        right = strchr(right, '\\');
        if (left && !right) return 1;
        if (right && !left) return -1;

        return compval;

} /* utils_CompPath */



///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : utils_CompPath2
// Return type: int APIENTRY 
// Arguments  : LPTSTR pl
//            : LPTSTR pr
// Description: Compare two strings, and return the first DIFFERENCE, if any.
//              I do not particular like the DOUBLE BYTE handling here,
// but shall leave it as is for now.
// This function take both string to LOWER CASE for the compare, to compare
// FILE NAMES regardless of CASE. But does NOT harm the current passed string. 
///////////////////////////////////////////////////////////////////////////////
int APIENTRY utils_CompPath2(LPTSTR pl, LPTSTR pr)
{
   static TCHAR   sszp1[264];
   static TCHAR   sszp2[264];
   int      compval;            // provisional value of comparison
   LPTSTR   left, right;

   if(pl==NULL)
      return -1;        // empty is less than anything else
   else if(pr==NULL)
      return 1;  // anything is greater than empty
   if( (*pl=='\0') && (*pr=='\0') )
      return 0;

   // ok, we must do a compare
   left  = &sszp1[0];
   right = &sszp2[0];
   // copy the contents
   strcpy(left, pl);
   strcpy(right,pr);
   // compare ONLY lower case characters
   AnsiLowerBuff( left,  strlen(left)  );
   AnsiLowerBuff( right, strlen(right) );

   for( ; ; )  // forever, until one or the other string expires
   {
      if( (*left=='\0') && (*right=='\0') )
         return 0;   // return the SAME

      if( *left=='\0' ) // if the left ran out
         return -1;     // return negative

      if( *right=='\0' )   // if the right ran out
         return 1;      // return positive

      if( IsDBCSLeadByte(*left) || IsDBCSLeadByte(*right) )
      {
         // process in pairs
         if(*right != *left)
         {
            compval = (*left - *right);
            break;
         }
         ++left;
         ++right;
         if( *right != *left )
         {
            compval = (*left - *right);
            break;
         }
         ++left;
         ++right;

      }
      else
      {
         if( *right == *left )
         {
            // if they are the SAME, bump and continue
            ++left;
            ++right;
            continue;
         }

         if( *left == '\\' )
         {
            compval = -1;
            break;
         }

         if( *right=='\\' )
         {
            compval = 1;
            break;
         }

         compval = (*left - *right);
         break;
         
      }
      
   }

   /* We have detected a difference.  If the rest of one
      of the strings (including the current character) contains
      some \ characters, but the other one does not, then all
      elements up to the last element of the one with the fewer
      elements are equal and so the other one lies in a subdir
      and so compares greater i.e. x\y\f > x\f
      Otherwise compval tells the truth.
    */

   left  = strchr(left,  '\\');
   right = strchr(right, '\\');

   if( left && !right )
      return 1;

   if( right && !left )
      return -1;

   return compval;

} /* utils_CompPath2 */


/***************************************************************************
 * Function: hash_string
 *
 * Purpose:
 *
 * Generate a hashcode for a null-terminated ascii string.
 *
 * If bIgnoreBlanks is set, then ignore all spaces and tabs in calculating
 * the hashcode.
 *
 * Multiply each character by a function of its position and sum these.
 * The function chosen is to multiply the position by successive
 * powers of a large number.
 * The large multiple ensures that anagrams generate different hash
 * codes.
 */
#define LARGENUMBER     6293815

//#define  hf_ignoreblanks   0x00000001
//#define  hf_ignoreeol      0x00000002

//DWORD APIENTRY
//hash_string(LPSTR string, BOOL bIgnoreBlanks)
DWORD hash_string( LPTSTR string, DWORD dwFlag )
{
   DWORD sum = 0;
   DWORD multiple = LARGENUMBER;
   int index = 1;
   INT   c;
   BOOL  bIC, bIE, bIB;

   if( !string )
      return 0;

   bIC = (( dwFlag & hf_ignorecase ) ? TRUE : FALSE );
   bIE = (( dwFlag & hf_ignoreeol ) ? TRUE : FALSE );
   bIB = (( dwFlag & hf_ignoreblanks ) ? TRUE : FALSE );
   c = *string;
   while( c != 0 )
   {
      //if( dwFlag & hf_ignoreeol )
      if( bIE )
      {
         // ignore Cr and Lf!!!
         while( (c == '\r') || (c == '\n') )
         {
            //string = CharNext(string);
            string++;
            c = *string;
         }

      }
//      if( bIgnoreBlanks )
//      if( dwFlag & hf_ignoreblanks )
      if( bIB )
      {
         // for me, IGNORE blanks should ALSO ignore Cr and Lf!!!
         while( (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') )
         {
            //string = CharNext(string);
            string++;
            c = *string;
         }
      }
      if( c )
      {
         if( bIC )
            c = toupper(c);

         sum += multiple * index++ * c;   // (*string++);
         multiple *= LARGENUMBER;
         string++;
         c = *string;
      }
   }

   return(sum);   // return SUM according to the FLAGS

}

DWORD APIENTRY
hash_string_ORG(LPSTR string, BOOL bIgnoreBlanks)
{

        DWORD sum = 0;
        DWORD multiple = LARGENUMBER;
        int index = 1;

        while (*string != '\0')
        {
           if( bIgnoreBlanks )
           {
              while( (*string == ' ') || (*string == '\t') )
              {
                 string = CharNext(string);
                 
              }
              
           }

           sum += multiple * index++ * (*string++);
           multiple *= LARGENUMBER;
        }
        return(sum);
}

/***************************************************************************
 * Function: utils_isblank
 *
 * Purpose:
 *
 * Return TRUE iff the string is blank.  Blank means the same as
 * the characters which are ignored in hash_string when ignore_blanks is set
 */
BOOL APIENTRY
utils_isblank(LPTSTR string)
{
   INT   c;
   if( !string )
      return TRUE;   // well - sort of :-)

   c = *string;
   // 1. while ( (*string == ' ') || (*string == '\t')) {
   //             string = CharNext(string); } or
   while( (c == ' ') || (c == '\t') )
   {
      string = CharNext(string);
      c = *string;
   }

   // /* having skipped all the blanks, do we see the end delimiter? */
   // return (*string == '\0' || *string == '\r' || *string == '\n');
   // having skipped any LEADING blanks, are we at the end-of-line
   return( c == '\0' || c == '\r' || c == '\n' );
}



/* --- simple string input -------------------------------------- */

/*
 * static variables for communication between function and dialog
 */
LPSTR dlg_result;
int dlg_size;
LPSTR dlg_prompt, dlg_default, dlg_caption;

/***************************************************************************
 * Function: StringInput
 *
 * Purpose:
 *
 * Input of a single text string, using a simple dialog.
 *
 * Returns TRUE if ok, or FALSE if error or user canceled. If TRUE,
 * puts the string entered into result (up to resultsize characters).
 *
 * Prompt is used as the prompt string, caption as the dialog caption and
 * default as the default input. All of these can be null.
 */

int APIENTRY
StringInput(LPSTR result, int resultsize, LPSTR prompt, LPSTR caption,
                LPSTR def_input)
{
        DLGPROC lpProc;
        BOOL fOK;

        /* copy args to static variable so that winproc can see them */

        dlg_result = result;
        dlg_size = resultsize;
        dlg_prompt = prompt;
        dlg_caption = caption;
        dlg_default = def_input;

        lpProc = (DLGPROC)MakeProcInstance((WNDPROC)dodlg_stringin, hLibInst);
        fOK = DialogBox(hLibInst,
           MAKEINTRESOURCE(IDD_STRINGINPUT),
           GetFocus(), lpProc);
        FreeProcInstance((WNDPROC)lpProc);

        return(fOK);
}

/***************************************************************************
 * Function: dodlg_stringin
 *
 */
int FAR PASCAL
dodlg_stringin(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
        switch(message) {

        case WM_INITDIALOG:
                CenterDialog( hDlg, hwndClient );
                if (dlg_caption != NULL) {
                        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM) dlg_caption);
                }
                if (dlg_prompt != NULL) {
                        SetDlgItemText(hDlg, IDD_LABEL, dlg_prompt);
                }
                if (dlg_default) {
                        SetDlgItemText(hDlg, IDD_FILE, dlg_default);
                }
                return(TRUE);

        case WM_COMMAND:
                switch(GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDCANCEL:
                        EndDialog(hDlg, FALSE);
                        return(TRUE);

                case IDOK:
                        GetDlgItemText(hDlg, IDD_FILE, dlg_result, dlg_size);
                        EndDialog(hDlg, TRUE);
                        return(TRUE);
                }
        }
        return (FALSE);
}

#ifdef ADD_DBCS_FUNCS2 // FIX20050129 - using MSVC7 .NET 2003

/***************************************************************************
 * Function: My_mbschr
 *
 * Purpose:
 *
 * DBCS version of strchr
 *
 */
unsigned char * _CRTAPI1 My_mbschr(
    unsigned char *psz, unsigned short uiSep)
{
    while (*psz != '\0' && *psz != uiSep) {
        psz = CharNext(psz);
    }
    return *psz == uiSep ? psz : NULL;
}
/***************************************************************************
 * Function: My_mbsncpy
 *
 * Purpose:
 *
 * DBCS version of strncpy
 *
 */
unsigned char * _CRTAPI1 My_mbsncpy(
	unsigned char *psz1, const unsigned char *psz2, size_t Length)
{
        int nLen = (int)Length;
	unsigned char *pszSv = psz1;

	while (0 < nLen) {
		if (*psz2 == '\0') {
			*psz1++ = '\0';
			nLen--;
		} else if (IsDBCSLeadByte(*psz2)) {
			if (nLen == 1) {
				*psz1 = '\0';
			} else {
				*psz1++ = *psz2++;
				*psz1++ = *psz2++;
			}
			nLen -= 2;
		} else {
			*psz1++ = *psz2++;
			nLen--;
		}
	}
	return pszSv;
}

#endif // #ifdef ADD_DBCS_FUNCS2

#define  MXRCSTGS    16
#define  MXRCSTG     512
static   TCHAR szRcBuf[ (MXRCSTGS * MXRCSTG) ];
static   INT   iNxtRc;

LPTSTR   GetRcStgBuf( VOID )
{
   iNxtRc++;
   if(iNxtRc >= MXRCSTGS)
      iNxtRc = 0;
   return &szRcBuf[ (iNxtRc * MXRCSTG) ];
}

/***************************************************************************
 * Function: LoadRcString
 *
 * Purpose: Loads a resource string from string table and returns a pointer
 *          to the string.
 *
 * Parameters: wID - resource string id
 *
 */
LPTSTR APIENTRY LoadRcString(UINT wID)
{
   LPTSTR   lps = GetRcStgBuf();
   LoadString(g_hInst,  // (HANDLE)GetModuleHandle(NULL),
      wID,
      lps,
      MXRCSTG );
   return lps;
}


/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/



///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : CenterDialog
// Return type: BOOL 
// Arguments  : HWND hChild
//            : HWND hParent
// Description: 
// Added from grmLib.c - April, 2001 (FROM GMUtils.c - December, 1999)
// ===================================================================
//  FUNCTION: CenterDialog(HWND, HWND)
//	(was CenterWindow in GMUtils)
//  PURPOSE:  Center one window over another.
//  PARAMETERS:
//    hwndChild - The handle of the window to be centered.
//    hwndParent- The handle of the window to center on.
//  RETURN VALUE:
//    TRUE  - Success
//    FALSE - Failure
//  COMMENTS:
//    Dialog boxes take on the screen position that they were designed
//    at, which is not always appropriate. Centering the dialog over a
//    particular window usually results in a better position.
///////////////////////////////////////////////////////////////////////////////
BOOL CenterDialog( HWND hChild, HWND hParent )
{
	BOOL	bret = FALSE;
    RECT    rcChild, rcParent;
    int     cxChild, cyChild, cxParent, cyParent;
    int     cxScreen, cyScreen, xNew, yNew;
    HDC     hdc;
	HWND	hwndChild, hwndParent;

	if( ( hwndChild = hChild   ) &&
		( hwndParent = hParent ) )
	{

		// Get the Height and Width of the child window
		if( GetWindowRect( hwndChild, &rcChild ) )
		{
			cxChild = rcChild.right - rcChild.left;
			cyChild = rcChild.bottom - rcChild.top;

			// Get the Height and Width of the parent window
			if( GetWindowRect( hwndParent, &rcParent ) )
			{
				cxParent = rcParent.right - rcParent.left;
				cyParent = rcParent.bottom - rcParent.top;

				// Get the display limits
				if( hdc = GetDC(hwndChild) )
				{
					cxScreen = GetDeviceCaps(hdc, HORZRES);
					cyScreen = GetDeviceCaps(hdc, VERTRES);
					ReleaseDC( hwndChild, hdc );

					// Calculate new X position,
					// then adjust for screen
					xNew = rcParent.left +
						( (cxParent - cxChild) / 2 );
					if( xNew < 0 )
					{
						xNew = 0;
					}
					else if( (xNew + cxChild) > cxScreen )
					{
						xNew = cxScreen - cxChild;
					}
					// Calculate new Y position,
					// then adjust for screen
					yNew = rcParent.top  +
						( (cyParent - cyChild) / 2 );
					if( yNew < 0 )
					{
						yNew = 0;
					}
					else if( (yNew + cyChild) > cyScreen )
					{
						yNew = cyScreen - cyChild;
					}

					// Set it, and return
					bret = SetWindowPos( hwndChild,
                        NULL,
                        xNew, yNew,
                        0, 0,
                        SWP_NOSIZE | SWP_NOZORDER );
				}
			}
		}
	}
	return bret;
}
// END CenterDialog(HWND,HWND) ADDED FROM GMUtils.c
// December, 1999
// =====================================

// added April, 2001
VOID  AppendDateTime( LPTSTR lpb, LPSYSTEMTIME pst )
{
   sprintf(EndBuf(lpb),
      "%02d/%02d/%02d %02d:%02d",
      (pst->wDay & 0xffff),
      (pst->wMonth & 0xffff),
      (pst->wYear % 100),
      (pst->wHour & 0xffff),
      (pst->wMinute & 0xffff) );
}

VOID  AppendDateTime2( LPTSTR lpb, LPSYSTEMTIME pst )
{
   sprintf(EndBuf(lpb),
      "%02d/%02d/%04d %02d:%02d",
      (pst->wDay & 0xffff),
      (pst->wMonth & 0xffff),
      (pst->wYear &0xffff),
      (pst->wHour & 0xffff),
      (pst->wMinute & 0xffff) );
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : OpnFil
// Return type: HANDLE 
// Arguments  : LPTSTR lpf
//            : BOOL bAppend
// Description: Open for appending, or create a file
//              in WIN32 mode.
///////////////////////////////////////////////////////////////////////////////
HANDLE   OpnFil( LPTSTR lpf, BOOL bAppend )
{
   HANDLE   iRet = 0;
   HANDLE   fh   = 0;
   if( bAppend )
   {
      // try to open existing file
      fh = CreateFile( lpf,
         (GENERIC_READ | GENERIC_WRITE),
         0,
         NULL,
         OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL,
         0 );
      if( VFH( fh ) )
      {
         SetFilePointer( fh,  // handle to file
            0, // bytes to move pointer
            0, // bytes to move pointer
            FILE_END );
         // _lseek( fh, 0, SEEK_END ); // go to the END
      }
   }

   if( !VFH( fh ) )
   {
      /* try to open (CREATE) the file */
      fh = CreateFile( lpf,
         (GENERIC_READ | GENERIC_WRITE),
         0,
         NULL,
         CREATE_ALWAYS,
         FILE_ATTRIBUTE_NORMAL,
         0 );
   }

   if( VFH( fh ) )
   {
      iRet = fh;
   }
   else
   {
      sprintf( gszTmpBuf, "OOPS: FAILED to OPEN/CREATE file [%s]!", lpf );
      chkme( gszTmpBuf );
   }

   return iRet;
}

// Nov 2000 - some added utility functions - grm
#define         MXSTGS          256    // was 32
#define         MXONE           264

// ==========================================================
// _sGetSStg - return a pointer to a static buffer
// =========
//static  LPTSTR  _sGetSStg( void )
LPTSTR  GetStgBuf( void )
{
        LPTSTR  prs;
        static TCHAR szP2S[ (MXSTGS * MXONE) ];
        static LONG  iNP2S = 0;
        // NOTE: Can be called only MXSTGS times before repeating
        prs = &szP2S[ (iNP2S * MXONE) ];       // Get 1 of ? buffers
        iNP2S++;
        if( iNP2S >= MXSTGS )
                iNP2S = 0;
        *prs = 0;
        return prs;
}

#define  _sGetSStg   GetStgBuf

LPTSTR   GetDTStg( VOID )
{
   SYSTEMTIME  st;
   LPTSTR      lpb = _sGetSStg();

   GetLocalTime( &st );
   *lpb = 0;
   AppendDateTime( lpb, &st );

   return lpb;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : GetI64Stg
// Return type: LPTSTR 
// Argument   : PLARGE_INTEGER pli
// Description: Give a LARGE INTEGER, return a NICE looking number string,
//              with COMMAS in place.
//
///////////////////////////////////////////////////////////////////////////////
LPTSTR   GetI64Stg( PLARGE_INTEGER pli )
{
   TCHAR    buf[32];
   int      i,j,k;
   LARGE_INTEGER  li;
   LPTSTR   lps = _sGetSStg();

   li = *pli;
   sprintf(buf, "%I64d", li.QuadPart );
   *lps = 0;   // clear any previous
   if( i = strlen(buf) )  // get its length
   {
      k = 32;
      j = 0;
      lps[k+1] = 0;  // esure ZERO termination
      while( ( i > 0 ) && ( k >= 0 ) )
      {
         i--;     // back up one
         if( j == 3 )   // have we had a set of 3?
         {
            lps[k--] = ',';   // ok, add a comma
            j = 0;            // and restart count of digits
         }
         lps[k--] = buf[i];   // move the buffer digit
         j++;
      }
      k++;  // back to LAST position
      lps = &lps[k]; // pointer to beginning of 'nice" number
   }
   return lps;
}

// DWORD FFFFFFFF =            4,294,967,295
// so a good right format size 1234567890123 = 13+
LPTSTR   GetI64StgRLen( PLARGE_INTEGER pli, DWORD dwl )
{
   LPTSTR lps;
   LPTSTR lps1 = GetI64Stg( pli );
   DWORD  dwi  = strlen(lps1);
   if( dwi >= dwl )
   {
      // nothing to do here
      lps = lps1; // already larger than request
   }
   else
   {
      DWORD    dwj = (dwl - dwi);   // get lafet space count
      lps  = _sGetSStg();  // get another buffer
      *lps = 0;            // kill any previous
      while(dwj--)
         strcat(lps," ");  // add space fill to left
      strcat(lps, lps1);   // finally add in number
   }
   strcat(lps, " ");    // add SPACE after number
   return lps;
}

// BGN *=*=*=*=* GetI64StgRLen2 - FIX20010724 =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
#define  MY_KB       1024
//#define  MY_MB     ( MY_KB * 1000 )
#define  MY_MB     ( MY_KB * MY_KB )

// As above, but BIG values are reduced to KB, or MB
// Get BYTES size to neat string
LPTSTR   GetI64StgRLen2( PLARGE_INTEGER pli, DWORD dwl )
{
   LPTSTR   lps;
   LPTSTR   lpf;
   LARGE_INTEGER  li;
   li = *pli;
   if( li.QuadPart < MY_KB )
   {
      lps = GetI64StgRLen( pli, dwl );
      lpf = "B";  // just B for BYTES
   }
   else
   {
      double   db   = (double)li.QuadPart;    // convert to double
      double   db10;
      DWORD    prec;

      lps  = _sGetSStg();  // get another buffer
      if( db >= MY_MB )
      {
         db10 = ((db * 10.0) / MY_MB);
         db   = ( db10 / 10.0 ); // only retain 1 decimal place
         lpf = "MB";    // Millions, actually 1024^2, Mega Bytes - MB
      }
      else
      {
         db10 = ((db * 10.0) / MY_KB);
         db   = ( db10 / 10.0 );
         lpf = "KB";    // Thousands, actually 1024,  Kilo Bytes = KB
      }

      *lps = 0;            // kill any previous
      if(      db < 10000.0 )
         prec = 6;
      else if( db < 100000.0 )
         prec = 7;
      else if( db < 1000000.0 )
         prec = 8;
      else if( db < 10000000.0 )
         prec = 9;
      else
         prec = 10;

      if( prec > dwl )
         prec = dwl;

      Dbl2Stg( lps, db, prec );

      prec = strlen(lps);
      if( prec < dwl )
      {
         LPTSTR   lps2 = _sGetSStg();  // get another buffer
         DWORD    dwj = (dwl - prec);   // get left space count
         *lps2 = 0;
         while( dwj-- )
            strcat(lps2," ");
         strcat(lps2,lps);
         lps = lps2; // return second buffer
      }

      strcat(lps, " ");
   }

   // append the token
   strcat(lps, lpf); // add in the TOKEN of SIZE

   return lps;
}
// END *=*=*=*=* GetI64StgRLen2 - FIX20010724 =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

// convert UTC FILETIME to local FILETIME and then
//     local   FILETIME to local SYSTEMTIME
BOOL     FT2LST( FILETIME * pft, SYSTEMTIME * pst )
{
   BOOL  bRet = FALSE;
   FILETIME    ft;
   if( ( FileTimeToLocalFileTime( pft, &ft ) ) && // UTC file time converted to local
       ( FileTimeToSystemTime( &ft, pst)     ) )
   {
      bRet = TRUE;
   }
   return bRet;
}
// convert UTC FILETIME to
// local FILETIME and then
// to a date/time string
LPTSTR	GetFDTStg( FILETIME * pft )
{
   LPTSTR   lps = _sGetSStg();
   SYSTEMTIME  st;
   //FILETIME    ft;
   //if( ( FileTimeToLocalFileTime( pft, &ft ) ) && // UTC file time converted to local
   //    ( FileTimeToSystemTime( &ft, &st)     ) )
   if( FT2LST( pft, &st ) )
   {
      sprintf(lps,
         "%02d/%02d/%02d  %02d:%02d",
         (st.wDay & 0xffff),
         (st.wMonth & 0xffff),
         (st.wYear % 100),
         (st.wHour & 0xffff),
         (st.wMinute & 0xffff) );
   }
   else
   {
		strcpy( lps, "??/??/??  ??:??" );
   }
   return lps;
}

LPTSTR	GetFDTSStg( FILETIME * pft )
{
   LPTSTR   lps = _sGetSStg();
   SYSTEMTIME  st;
   //FILETIME    ft;
   //if( ( FileTimeToLocalFileTime( pft, &ft ) ) && // UTC file time converted to local
   //    ( FileTimeToSystemTime( &ft, &st)     ) )
   if( FT2LST( pft, &st ) )
   {
      sprintf(lps,
         "%02d/%02d/%04d %02d:%02d:%02d",
         (st.wDay & 0xffff),
         (st.wMonth & 0xffff),
         (st.wYear & 0xffff),
         (st.wHour & 0xffff),
         (st.wMinute & 0xffff),
         (st.wSecond & 0xffff) );
   }
   else
   {
		strcpy( lps, "??/??/?? ??:??:??" );
   }
   return lps;
}

LPTSTR	GetFDStg( FILETIME * pft )
{
   LPTSTR   lps = _sGetSStg();
   SYSTEMTIME  st;
   //FILETIME    ft;
   //if( ( FileTimeToLocalFileTime( pft, &ft ) ) && // UTC file time converted to local
   //    ( FileTimeToSystemTime( &ft, &st)     ) )
   if( FT2LST( pft, &st ) )
   {
      sprintf(lps,
         "%02d/%02d/%02d",
         (st.wDay & 0xffff),
         (st.wMonth & 0xffff),
         (st.wYear % 100) );
   }
   else
   {
		strcpy( lps, "??/??/??" );
   }
   return lps;
}

LPTSTR	GetFTStg( FILETIME * pft )
{
   LPTSTR   lps = _sGetSStg();
   SYSTEMTIME  st;
   //FILETIME    ft;
   //if( ( FileTimeToLocalFileTime( pft, &ft ) ) && // UTC file time converted to local
   //    ( FileTimeToSystemTime( &ft, &st)     ) )
   if( FT2LST( pft, &st ) )
   {
      sprintf(lps,
         "%02d:%02d",
         (st.wHour & 0xffff),
         (st.wMinute & 0xffff) );
   }
   else
   {
		strcpy( lps, "??:??" );
   }
   return lps;
}

int   OutStg( HANDLE fh, LPTSTR lps )
{
   int   i, j;

   j = 0;
   if( i = lstrlen(lps) )
   {
      if( ( WriteFile( fh, lps, i, &j, NULL ) ) &&
          ( j == i ) )
      {
         // success
         i = j;
      }
      else
      {
         j = (int)-1;
      }
   }
   return j;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SigLen
// Return type: DWORD 
// Argument   : LPTSTR lps
// Description: Count the SIGNIFICANT characters in a line
//              
///////////////////////////////////////////////////////////////////////////////
DWORD SigLen( LPTSTR lps )
{
   DWORD dwl = 0;
   DWORD dwi,dwj;
   if( dwj = strlen(lps) )
   {
      for( dwi = 0; dwi < dwj; dwi++ )
      {
         if( lps[dwi] > ' ' )
            dwl++;
      }
   }
   return dwl;
}


//    source, desitation and max. size
LPTSTR	CopyShortName( LPTSTR lps, LPTSTR lpd, DWORD siz )
{
	DWORD		i, i2, i3, k, j;
	k = 0;
	if( (siz > 10) &&
		((i = lstrlen( lps )) > (siz+3) ) )
	{
		i2 = (siz-1) / 2;
		i3 = i2 + (i - (i2 * 2));
		for( j = 0; j < i; j++ )
		{
			if( j < i2 )
			{
				lpd[k++] = lps[j];
			}
			else if( j == i2 )
			{
				lpd[k++] = '.';
				lpd[k++] = '.';
				lpd[k++] = '.';
			}
			else if( j > i3 )
			{
				lpd[k++] = lps[j];
			}
		}
		lpd[k] = 0;
	}
	else
	{
		lstrcpy( lpd, lps );
	}

	return	lpd;
}

int MB( HWND hWnd,          // handle to owner window
  LPTSTR lpText,     // text in message box
  LPTSTR lpCaption,  // message box title
  UINT uType         // message box style
)
{
   int      i;
   LPTSTR   lpcap = lpCaption;
   HWND     hwnd  = hWnd;

   if( !lpcap )
   {
      lpcap = APPNAME;
   }
   else
   {
      sprtf("MB:[%s]"MEOR, lpcap);
   }

   if( !hwnd )
      hwnd = hwndClient;

   sprtf("msg[");
   sprtf(lpText);
   sprtf("]"MEOR);

   dc4w_UI(TRUE);
   i = MessageBox( hwnd,          // handle to owner window
      lpText,     // text in message box
      lpcap,      // message box title
      uType );    // message box style
   dc4w_UI(FALSE);

   sprtf( "Done MB - int = %d"MEOR, i );

   return i;
}

// ==================================================

// some timing functions
// ===============================================================================
#define  MXTMS    32    // NOTE this MAXIMUM at any one time, else unpredictable

typedef  struct { /* tm */
   BOOL  tm_bInUse;
   BOOL  tm_bt;
   LARGE_INTEGER  tm_lif, tm_lib, tm_lie;
   DWORD tm_dwtc;
}GTM, * PGTM;

GTM   sGtm[MXTMS];
int   iNxt = 0;

VOID  InitTimers( VOID )
{
   int   i;
   PGTM  ptm = &sGtm[0];

   for( i = 0; i < MXTMS; i++ )
   {
      ptm->tm_bInUse = FALSE;
      ptm++;
   }
}

int  GetTimer( PGTM * pptm )
{
   PGTM  ptm = &sGtm[0];
   int   i = (int)-1;
   int   j;
   for( j = 0; j < MXTMS; j++ )
   {
      if( !ptm->tm_bInUse )
      {
         ptm->tm_bInUse = TRUE;
         i = j;
         *pptm = ptm;
         break;
      }
      ptm++;
   }
   return i;
}

/* =================================
 * int  SetBTime( void )
 *
 * Purpose: Set the beginning timer, and return the INDEX of that timer
 *
 * Return: Index (offset) of timer SET
 *
 */
int  SetBTime( void )
{
   PGTM  ptm;
   int   i;
   i = GetTimer( &ptm );
   if( i != (int)-1 )
   {
      if( ptm->tm_bt = QueryPerformanceFrequency( &ptm->tm_lif ) )
         QueryPerformanceCounter( &ptm->tm_lib ); // counter value
      else
         ptm->tm_dwtc = GetTickCount(); // ms since system started
   }
   return i;
}

/* =================================
 * double GetETime( int i )
 *
 * Purpose: Return ELAPSED time as double (in seconds) of the index given
 *
 * Return: If index is with the range of 0 to (MXTMS - 1) then
 *          compute ELAPSED time as a double in SECONDS
 *
 *         Else results indeterminate
 *
 */
double GetETime( int i )
{
   DWORD          dwd;
   double         db;
   LARGE_INTEGER  lid;
   PGTM           ptm;
   if( ( i < 0     ) ||
       ( i >= MXTMS ) )
   {
      db = (double)100.0;  // return an idiot number!!!
   }
   else
   {
      ptm = &sGtm[i];
      if( ptm->tm_bt )
      {
         QueryPerformanceCounter( &ptm->tm_lie ); // counter value
         lid.QuadPart = ( ptm->tm_lie.QuadPart - ptm->tm_lib.QuadPart ); // get difference
         db  = (double)lid.QuadPart / (double)ptm->tm_lif.QuadPart;
      }
      else
      {
         dwd = (GetTickCount() - ptm->tm_dwtc);   // ms elapsed
         db = ((double)dwd / 1000.0);
      }
      // make this timer available
      ptm->tm_bInUse = FALSE;
   }

   return db;

}

double GetRTime( int i )
{
   DWORD          dwd;
   double         db;
   LARGE_INTEGER  lid;
   PGTM           ptm;
   if( ( i < 0     ) ||
       ( i >= MXTMS ) )
   {
      db = (double)100.0;  // return an idiot number!!!
   }
   else
   {
      ptm = &sGtm[i];
      if( ptm->tm_bt )
      {
         QueryPerformanceCounter( &ptm->tm_lie ); // counter value
         lid.QuadPart = ( ptm->tm_lie.QuadPart - ptm->tm_lib.QuadPart ); // get difference
         db  = (double)lid.QuadPart / (double)ptm->tm_lif.QuadPart;
      }
      else
      {
         dwd = (GetTickCount() - ptm->tm_dwtc);   // ms elapsed
         db = ((double)dwd / 1000.0);
      }
      // DO NOT make this timer available
      // ptm->tm_bInUse = FALSE;
   }

   return db;

}

/* Oct 99 update - retrieved from DDBData.c */
// ******************************************************************************
// ===========================================================
// void Buffer2Stg( LPSTR lps, LPSTR lpb, int decimal,
//				 int sign, int precision )
//
// Purpose: Convert the string of digits from the _ecvt
//			function to a nice human readbale form.
//
// 1999 Sept 7 - Case of removing ?.?0000 the zeros
//
// ===========================================================
void Buffer2Stg( LPSTR lps, LPSTR lpb, int decimal,
				 int sign, int precision )
{
	int		i, j, k, l, m, sig, cad;
	char	c;

	k = 0;					// Start at output beginning
	cad = 0;				// Count AFTER the decimal
	j = lstrlen( lpb );		// Get LENGTH of buffer digits

	if( sign )				// if the SIGN flag is ON
		lps[k++] = '-';		// Fill in the negative

	l = decimal;
	if( l < 0 )
	{
		// A NEGATIVE decimal position
		lps[k++] = '0';
		lps[k++] = '.';
		cad++;
		while( l < 0 )
		{
			lps[k++] = '0';
			l++;
			cad++;
		}
	}
	else if( ( decimal >= 0 ) &&
		( decimal < precision ) )
	{
		// Possible shortened use of the digit string
		// ie possible LOSS of DIGITS to fit the precision requested.
		if( decimal == 0 )
		{
			if( ( precision - 1 ) < j )
			{
				//chkme( "NOTE: precision -1 is LT digits! Possible LOSS!!" );
				j = precision - 1;
			}
		}
		else
		{
			if( precision < j )
			{
//				chkme( "NOTE: precision is LT digits! Possible LOSS!!" );
				j = precision;
			}
		}
	}

	sig = 0;	// Significant character counter
	// Process each digit of the digit list in the buffer
	// or LESS than the list if precision is LESS!!!
	for( i = 0; i < j; i++ )
	{
		c = lpb[i];		// Get a DIGIT
		if( i == decimal )	// Have we reached the DECIMAL POINT?
		{
			// At the DECIMAL point
			if( i == 0 )	
			{
				// if no other digits BEFORE the decimal
				lps[k++] = '0';	// then plonk in a zero now
			}
			lps[k++] = '.';	// and ADD the decimal point
			cad++;
		}
		// Check for adding a comma for the THOUSANDS
		if( ( decimal > 0 ) &&
			( sig ) &&
			( i < decimal ) )
		{
			m = decimal - i;
			if( (m % 3) == 0 )
				lps[k++] = ',';	// Add in a comma
		}
		lps[k++] = c;	// Add this digit to the output
		if( sig )		// If we have HAD a significant char
		{
			sig++;		// Then just count another, and another etc
		}
		else if( c > '0' )
		{
			sig++;	// First SIGNIFICANT character
		}
		if( cad )
			cad++;
	}	// while processing the digit list

	// FIX980509 - If digit length is LESS than decimal position
	// =========================================================
	if( ( decimal > 0 ) &&
		( i < decimal ) )
	{
		c = '0';
		while( i < decimal )
		{
			if( ( decimal > 0 ) &&
				( sig ) &&
				( i < decimal ) )
			{
				m = decimal - i;
				if( (m % 3) == 0 )
					lps[k++] = ',';	// Add in a comma
			}
			lps[k++] = c;	// Add this digit to the output
			i++;
		}
	}
	// =========================================================
	if( cad )
		cad--;
	lps[k] = 0;		// zero terminate the output
	// FIX990907 - Remove unsightly ZEROs after decimal point
    for( i = 0; i < k; i++ )
    {
        if( lps[i] == '.' )
            break;
    }
    if( ( i < k ) &&
        ( lps[i] == '.' ) )
    {
        i++;
        if( lps[i] == '0' )
        {
            while( k > i )
            {
                k--;
                if( lps[k] == '0' )
                    lps[k] = 0;
                else
                    break;
            }
            if( k > i )
            {
                // we have backed to a not '0' value so STOP
            }
            else
            {
                // we backed all the way, so remove the DECIMAL also
                i--;
                lps[i] = 0;
            }
        }
        else
        {
            while( k > i )
            {
                k--;
                if( lps[k] == '0' )
                    lps[k] = 0;
                else
                    break;
            }
        }
    }

}

void    Dbl2Stg( LPSTR lps, double factor, int prec )
{
    int             decimal, sign, precision;
    char *  buffer;

    if( prec )
        precision = prec;
    else
        precision = 16;

    buffer = _ecvt( factor, precision, &decimal, &sign );

    Buffer2Stg( lps, buffer, decimal, sign, precision );
}

//extern	void	Dbl2Stg( LPSTR lps, double factor, int prec );
LPTSTR   Dbl2Str( double factor, int prec )
{
   LPTSTR   lps = _sGetSStg();
   *lps = 0;
   Dbl2Stg( lps, factor, prec );
   return lps;
}

// ******************************************************************************

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : ConditionText
// Return type: INT     - final length of conditioned text
// Argument   : LPTSTR lpt
// Description: Make a buffer of TEXT have only SINGLE SPACE in place of
//              multiple spaces, or other control characters.
// That is REMOVE Cr, Lf, Space and Tab, adding only ONE space!!!
///////////////////////////////////////////////////////////////////////////////
INT   ConditionText( LPTSTR lpt )
{
   INT   i, j, k, c, sp;
   j = k = sp = 0;
   if( lpt )
      j = strlen(lpt);
   for( i = 0; i < j; i++ )
   {
      c = lpt[i];
      if( c > ' ' )  // if greater than SPACEY
      {
         lpt[k++] = (TCHAR)c;    // add it
         sp = 0;  // and clear SPACEY
      }
      else  // it is space or below
      {
         if( k && ( sp == 0 ) )  // if we have HAD chars, and NO spacey
         {
            lpt[k++] = ' ';   // add ONE space
            sp = 1;  // and set SPACEY done
         }
      }
   }
   // finally clear any SPACEY tail
   while( k-- )
   {
      if( lpt[k] > ' ' )
         break;
      lpt[k] = 0;
   }
   return k;
}

VOID  ToggleBit( PDWORD pdwFlag, DWORD dwBit, PBOOL pChg, BOOL bFlg )
{
   DWORD dwFlag = *pdwFlag;
   if(bFlg)    // if to ON
   {
      // check if bit present
      if( !(dwFlag & dwBit) )
      {
         // add bit
         dwFlag |= dwBit;
         if( pChg )  // and set change
            *pChg = TRUE;
      }
   }
   else
   {
      if( dwFlag & dwBit )
      {
         dwFlag &= ~(dwBit);  // squirial out the bit
         if( pChg )  // and set change
            *pChg = TRUE;
      }
   }
   *pdwFlag = dwFlag;   // return update
}

VOID  ToggleBool( PBOOL pBool, PBOOL pChg, BOOL flg )
{
   BOOL  b = *pBool;
   if(flg)  // if toggling to ON
   {
      if( !b ) // and NOT ON
      {
         *pBool = TRUE;
         *pChg  = TRUE; // set change
      }
   }
   else  // !flg = toggling to OFF
   {
      if(b) // and is ON
      {
         *pBool = FALSE;
         *pChg  = TRUE; // set change
      }
   }
}

BOOL  ChangedWP( WINDOWPLACEMENT * pw1, WINDOWPLACEMENT * pw2 )
{
   BOOL  bChg = FALSE;
   if( ( pw1->length != sizeof(WINDOWPLACEMENT) ) ||
       ( pw2->length != sizeof(WINDOWPLACEMENT) ) ||
       ( pw1->showCmd != pw2->showCmd ) ||
       ( pw1->ptMaxPosition.x != pw2->ptMaxPosition.x ) ||
       ( pw1->ptMaxPosition.y != pw2->ptMaxPosition.y ) ||
       ( !EqualRect( &pw1->rcNormalPosition, &pw2->rcNormalPosition ) ) )
   {
      bChg = TRUE;
   }
   return bChg;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : InStr
// Return type: INT 
// Arguments  : LPTSTR lpb
//            : LPTSTR lps
// Description: Return the position of the FIRST instance of the string in lps
//              Emulates the Visual Basic function.
///////////////////////////////////////////////////////////////////////////////
INT   InStr( LPTSTR lpb, LPTSTR lps )  // extracted from FixFObj.c
{
   INT   iRet = 0;
   INT   i, j, k, l, m;
   TCHAR    c;
   i = strlen(lpb);
   j = strlen(lps);
   if( i && j && ( i >= j ) )
   {
      c = *lps;   // get the first we are looking for
      l = i - ( j - 1 );   // get the maximum length to search
      for( k = 0; k < l; k++ )
      {
         if( lpb[k] == c )
         {
            // found the FIRST char so check until end of compare string
            for( m = 1; m < j; m++ )
            {
               if( lpb[k+m] != lps[m] )   // on first NOT equal
                  break;   // out of here
            }
            if( m == j )   // if we reached the end of the search string
            {
               iRet = k + 1;  // return NUMERIC position (that is LOGICAL + 1)
               break;   // and out of the outer search loop
            }
         }
      }  // for the search length
   }
   return iRet;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Left
// Return type: LPTSTR 
// Arguments  : LPTSTR lpl
//            : DWORD dwi
// Description: Return the LEFT prortion of a string
//              Emulates the Visual Basic function
///////////////////////////////////////////////////////////////////////////////
LPTSTR   Left( LPTSTR lpl, DWORD dwi )
{
   LPTSTR   lps = _sGetSStg();
   DWORD    dwk;
   for( dwk = 0; dwk < dwi; dwk++ )
      lps[dwk] = lpl[dwk];
   lps[dwk] = 0;
   return lps;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Right
// Return type: LPTSTR 
// Arguments  : LPTSTR lpl
//            : DWORD dwl
// Description: Returns a buffer containing the RIGHT postion of a string
//              Emulates the Visual Basic function.
///////////////////////////////////////////////////////////////////////////////
LPTSTR   Right( LPTSTR lpl, DWORD dwl )
{
   LPTSTR   lps = _sGetSStg();
   DWORD    dwk = strlen(lpl);
   DWORD    dwi;
   *lps = 0;
   if( ( dwl ) &&
      ( dwk ) &&
      ( dwl <= dwk ) )
   {
      if( dwl == dwk )  // is it right ALL
         dwi = 0;
      else
         dwi = dwk - dwl;
      strcpy(lps, &lpl[dwi] );
   }

   return lps;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Mid
// Return type: LPTSTR 
// Arguments  : LPTSTR lpl
//            : DWORD dwb
//            : DWORD dwl
// Description: Returns a buffer containing the MIDDLE portion of a string.
//              Emulates the Visual Basic function.
///////////////////////////////////////////////////////////////////////////////
LPTSTR   Mid( LPTSTR lpl, DWORD dwb, DWORD dwl )
{
   LPTSTR   lps = _sGetSStg();
//   LPTSTR   pt;
   DWORD    dwk = strlen(lpl);
   DWORD    dwi, dwr;
   *lps = 0;
   if( ( dwl ) && 
      ( dwb ) &&
      ( dwl ) &&
      ( dwb <= dwk ) &&
      ( dwl <= (dwk - (dwb - 1)) ) )
   {
      dwr = 0;
      for(dwi = (dwb - 1); (dwi < dwk), (dwr < dwl); dwi++ )
      {
//         pt = &lpl[dwi];
         lps[dwr++] = lpl[dwi];
      }
      lps[dwr] = 0;
   }
   return lps;
}


LPTSTR   RetDiffStg( LPTSTR lps, LPTSTR lpd )
{
   LPTSTR   lpr = _sGetSStg();
   INT      iPos = InStr(lpd, lps); // does lpd contain lps?
   if(iPos > 0)
   {
      if( iPos == 1 )
         strcpy(lpr, &lpd[ strlen(lps) ] );
      else
      {
         strcpy(lpr, Left( lpd, (iPos - 1) ) );
         strcat(lpr, "...");
         strcat(lpr, Right( lpd, (strlen(lps) - (iPos + strlen(lps)))) );
      }
   }
   else
      strcpy(lpr, lpd);
   return lpr;
}

// *********************************************************
// EXTRACTED FROM utils\grmLib.c - May 2001 - AND MODIFIED

BOOL	GotWild( LPTSTR lps )
{
	BOOL	   flg = FALSE;
	INT      i, j;
	INT      c;
	i = strlen( lps ); 
	if(i)
	{
		for( j = 0; j < i; j++ )
		{
			c = lps[j];
			if( ( c == '*' ) || ( c == '?' ) )
			{
				flg = TRUE;
				break;
			}
		}
	}
	return flg;
}

BOOL	SplitExt2( LPTSTR lpb, LPTSTR lpe, LPTSTR lpf )
{
	BOOL	   flg = FALSE;
   DWORD    i;
   LPTSTR   p;
   LPTSTR   lpr = _sGetSStg();
   i = strlen(lpf);
   *lpb = 0;
   *lpe = 0;
	if(i)
	{
      strcpy(lpr,lpf);
      p = strrchr(lpr, '.');  // get LAST full stop = "."
		if(p)
		{
         *p = 0;
         p++;
         strcpy(lpb,lpr); // note if file is .cvsignore
         strcpy(lpe,p); // body will be null
         flg = TRUE;
      }
      else
      {
         strcpy(lpb,lpr);  // no extent
      }
	}
	return	flg;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : WildComp2
// Return type: BOOL 
// Arguments  : LPSTR lp1
//            : LPSTR lp2
// Description: Compare two components of a file name
//              FIX20010503 - Should NOT return TRUE on "c" with "cab"
//             FIX20051212 - dc4wUtil/WildComp2() - Somehow an ERROR crept into here
///////////////////////////////////////////////////////////////////////////////
BOOL	WildComp2( LPTSTR lp1, LPTSTR lp2 )
{
	BOOL	   flg = FALSE;
	DWORD    i1, i2, j1, j2, ilen;
	INT      c, d;

	i1 = strlen(lp1);
	i2 = strlen(lp2);
	if( i1 && i2 )    // if BOTH have lengths
	{
      ilen = i1;
      if( i2 > i1 )
         ilen = i2;
		j2 = 0;
		for( j1 = 0; j1 < ilen; j1++ )
		{
			c = toupper(lp1[j1]);   // extract next char from each
			d = toupper(lp2[j2]);
         if( c == d )
         {
            j2++;
            continue;
         }
			// they are NOT equal
         if( c == 0 ) { // FIX20051204 - root* == root
            // have reached the end of one, if the last char is '*'
            // then WE HAVE A MATCH
            if( ( d == '*' ) && ((j2 + 1) == ilen) ) {
               flg = TRUE;
            }
            break;
         }

         if( d == 0 ) {
            if( ( c == '*' ) && ((j2 + 1) == ilen) ) {
               flg = TRUE;
            }
            break;
         }

			{
				if( c == '*' )
				{
               j1++;
               if( lp1[j1] == 0 )   // get NEXT
               {
                  // ended with this asteric, so
                  flg = TRUE; // this matches all the rest of 2
					   break;   // out of here with a MATCH
               }
               // else we have somehting like *abc, which mean the asteric
               // matched what ever was in 2, up until this letter encountered
               c = toupper(lp1[j1]);
               j2++;    // asteric matched at least this one
      			if( lp2[j2] == 0 )   // 2 ended, but 1 has more
                  break;   // so no MATCH
               for( ; j2 < ilen; j2++ )
               {
         			d = toupper(lp2[j2]);
                  if( c == d )
                     break;
                  if( d == 0 )
                     break;
               }
               if( c == d )
               {
                  // found next of 1 in 2
                  j2++;
                  continue;
               }
               // else the char in 1 not present in two;
               break;   // no MATCH
				}

				if( d == '*' )
				{
               j2++;
               if( lp2[j2] == 0 )
               {
                  // 2 ends with asteric, so matches all rest in 1
                  flg = TRUE;
                  break;
               }
               d = toupper(lp2[j2]);
               j1++;    // asteric matched at least this one
      			if( lp1[j1] == 0 )   // 1 ended, but 2 has more
                  break;   // so no MATCH
               for( ; j1 < ilen; j1++ )   // find the 2 in 1
               {
         			c = toupper(lp1[j1]);
                  if( c == d )   // found it?
                     break;
                  if( c == 0 )   // or ran out of chars
                     break;
               }
               if( c == d )
               {
                  // found next of 2 in 1
                  j2++;
                  continue;
               }
               // else the char in 2 is not present in 1
					break;
				}

            if( ( c == '?' ) || ( d == '?' ) )
				{
					// One match char ok.
				}
				else
				{
					if( toupper( c ) != toupper( d ) )
						break;
				}
			}
			j2++;
		}
		if( !flg && ( j1 == ilen ) )
			flg = TRUE;
	}
   else
   {
      // FIX20010509 - Fix temp*.* should match tempf
      // Here the extension of tempf is nul, and thus should match with "*"
      if( ( i1 == 0 ) && ( i2 == 0 ) )
      {
         // two blanks is a PERFECT match
         flg = TRUE;
      }
      else if( i1 == 0 )
      {
         // the first is BLANK. This would be a MATCH if an "*" or "?" in 2, no?
         if( ( i2 == 1 ) &&
             ( ( *lp2 == '*' ) || ( *lp2 == '?' ) ) )
             flg = TRUE;
      }
      else  // if( i2 == 0 )
      {
         if( ( i1 == 1 ) &&
             ( ( *lp1 == '*' ) || ( *lp1 == '?' ) ) )
             flg = TRUE;
      }
   }
	return flg;
}

BOOL	MatchWild( LPTSTR lp1, LPTSTR lp2 )
{
	// One of the other HAS WILD CHAR(S)
   LPTSTR   lpb1 = gszBody1;  // FILE NAME, ususally from system call
   LPTSTR   lpe1 = gszExt1;
	LPTSTR   lpb2 = gszBody2;  // user input masks, like  zlib*;*.obj;... etc
   LPTSTR   lpe2 = gszExt2;
	SplitExt2( lpb1, lpe1, lp1 );
	SplitExt2( lpb2, lpe2, lp2 );

   // FIX20021007 - refinement of the wild compare
   if(( lpe2[0] ==  0                 ) &&
      ( strchr( lp2, '.' ) == 0       ) &&   // no extent given
      ( lp2[(strlen(lp2) - 1)] == '*' ) )    // and last is an asterix
      strcpy(lpe2,"*"); // add to extent

	if( ( WildComp2( lpb1, lpb2 ) ) &&
		 ( WildComp2( lpe1, lpe2 ) ) )
	{
		return TRUE;
	}

   return FALSE;
}

// KindOfMatch
BOOL	MatchWild2( LPTSTR lp1, LPTSTR lpb2, LPTSTR lpe2 )
{
	// One of the other HAS WILD CHAR(S)
   LPTSTR   lpb1 = gszBody1;  // FILE NAME, ususally from system call
   LPTSTR   lpe1 = gszExt1;
//	LPTSTR   lpb2 = gszBody2;  // user input masks, like  zlib*;*.obj;... etc
//   LPTSTR   lpe2 = gszExt2;
	SplitExt2( lpb1, lpe1, lp1 );
//	SplitExt2( lpb2, lpe2, lp2 );

   // FIX20021007 - refinement of the wild compare
//   if(( lpe2[0] ==  0                 ) &&
//      ( strchr( lp2, '.' ) == 0       ) &&   // no extent given
//      ( lp2[(strlen(lp2) - 1)] == '*' ) )    // and last is an asterix
//      strcpy(lpe2,"*"); // add to extent

	if( ( WildComp2( lpb1, lpb2 ) ) &&
		 ( WildComp2( lpe1, lpe2 ) ) )
	{
		return TRUE;
	}

   return FALSE;
}


#ifdef   REV_TO_OLD
BOOL	MatchFiles( LPTSTR lp1, LPTSTR lp2 )
{
	BOOL	flg = FALSE;   // assume they DO NOT MATCH
	if( lp1 && lp2 &&
		*lp1 && *lp2 )
	{
		if( !GotWild( lp1 ) &&
			 !GotWild( lp2 ) )
		{
         // neither have wild cards 
         // so just do a COMPARE
			if( strcmpi( lp1, lp2 ) == 0 )
				flg = TRUE;
		}
		else
		{
         return( MatchWild( lp1, lp2 ) );
		}
	}
	return flg;
}
#endif   // #ifdef   REV_TO_OLD

// EXTRACTED FROM utils\grmLib.c - May 2001 - and MODIFIED
// *********************************************************
// increment, bump a file name - get next file
VOID  GetNxtDif( LPTSTR lpf )
{
   if( dir_isvalidfile(lpf) )
   {
      LPTSTR   lpb1 = gszBody1;
      LPTSTR   lpe1 = gszExt1;
      LPTSTR   lpb  = &gszTmpBuf[0];
      LPTSTR   lpb2 = &gszTmpBuf2[0];
      LPTSTR   p;
      DWORD    i;
      INT      c;
      strcpy(lpb2,lpf); // copy the file name
      p = strrchr(lpb2, '\\');
      if(p)
         p++;
      else
         p = lpb2;
      strcpy(lpb, p);   // get just the file name
      *p = 0;           // and any PATH in lpb2
		SplitExt2( lpb1, lpe1, lpb );    // get the BODY and extension separated
      i = strlen(lpb1);
      if( i < 8 )
      {
         strcat(lpb1,"0");    // append a ZERO
      }
      else
      {
         while(i)
         {
            i--;  // back up one
            c = lpb1[i];
            if( ISNUM(c) )
            {
               if( c < '9' )
               {
                  c++;
                  lpb1[i] = c;
                  break;
               }
               else
               {
                  c = '0';
                  lpb1[i] = c;
               }
            }
            else
            {
               c = '0';
               lpb1[i] = c;
               break;
            }
         }
      }
      // we have an adjusted body file name
      strcpy(lpf, lpb2);   // add any PATH
      strcat(lpf, lpb1);   // add the body
      strcat(lpf, "." );   // add the dot
      strcat(lpf, lpe1);   // and the extension
      GetNxtDif(lpf);   // and try this NEW name
   }
}

VOID  Setg_szNxtDif( VOID )
{
   LPTSTR   lpf = &g_szNxtDif[0];
   if( *lpf == 0 )
   {
      GetModulePath(lpf);
      strcat( lpf, "TEMPD001.TXT" );
   }
   GetNxtDif(lpf);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : TrimIB
// Return type: DWORD 
// Argument   : LPTSTR lps
// Description: Remove spacey stuf from BEGINNING and TAIL
//              NOTE WELL: Buffer is ALTERED, hence "IB"=IN BUFFER
///////////////////////////////////////////////////////////////////////////////
DWORD TrimIB( LPTSTR lps )
{
   DWORD dwk;
   dwk = LTrimIB(lps);
   dwk = RTrimIB(lps);
   return dwk;
}

DWORD LTrimIB( LPTSTR lps )
{
   DWORD    dwr = strlen(lps);
   LPTSTR   p   = lps;
   DWORD    dwk = 0;
   while(dwr)
   {
      if( *p > ' ' )
         break;
      dwk++;   // count chars to die
      p++;     // bump pointer
      dwr--;   // update return length
   }
   if(dwk)  // if chars to die
      strcpy(lps,p);    // copy remainder up to beginning
   return dwr;          // return length or reduced length
}

DWORD RTrimIB( LPTSTR lps )
{
   DWORD    dwr = strlen(lps);
   LPTSTR   p   = lps;
   DWORD    dwk;
   dwk = dwr;        // copy of length
   while(dwk--)      // while not zero - post decrement
   {
      if( lps[dwk] > ' ' ) // is this above spacey
         break;   // yup - out of here
      lps[dwk] = 0;  // else zero it
      dwr--;   // reduce overall length
   }
   return dwr; // return length or reduced length
}


LPTSTR   GetRelNameStg( COMPITEM ci )
{
   LPTSTR   lpr = _sGetSStg();
   *lpr = 0;
   compitem_retrelname( lpr, ci );
   return lpr;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : Add2SList
// Return type: PLE - pointer to LIST_ENTRY head.
// Arguments  : PLE ph - Head of the LIST to add it to.
//            : PBOOL pb - pointer to change, if required.
//            : LPTSTR lps - and of course the STRING TO ADD
// Description: Add a STRING to a 'simple' double linked list.
//              The allocation is the LIST_ENTRY header, plus
// the LENGTH of the string + 1 for the nul.
// This function ALWAYS places this NEW string, or the existing string if there
// at the HEAD of the LIST, and set CHANGE for an INI write.
///////////////////////////////////////////////////////////////////////////////
PLE   Add2SList( PLE ph, PBOOL pb, LPTSTR lps )
{
   PLE   pn;
   BOOL  bAdd = TRUE;
   INT   i    = 0;
   DWORD dwl  = 0;
   
   if(lps)  // if given a string pointer
      dwl = strlen(lps);   // get its LENGTH

   if( !dwl )  // if NO LENGTH
      return 0;   // out of here

   // else traverse the list,
   // looking for this string
   Traverse_List( ph, pn )
   {
      if( strcmpi( lps, (LPTSTR)((PLE)pn + 1) ) == 0 )
      {
         // found ithis STRING already in the LIST
         bAdd = FALSE;  // so do NOT add it
         if(i)    // BUT if it is NOT already the first
         {
            RemoveEntryList(pn);    // remove it from the LINKED LIST
            InsertHeadList(ph,pn);  // in INSERT at the HEAD of the LIST
            if(pb)
               *pb = TRUE;  // set CHANGE of the LIST
         }
         break;
      }
      i++;  // count another in the list
   }

   if( bAdd )
   {
      // add this NEW string at the HEAD of the LIST
      pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + dwl + 1) );
      if(pn)
      {
         strcpy( (LPTSTR)((PLE)pn + 1), lps ); // add the string
         InsertHeadList(ph,pn);  // and insert it at the head
         if(pb)
            *pb = TRUE;  // and SET change for INI write
      }
   }
   return pn;
}

VOID  EnsureCrLf( LPTSTR lps )
{
   DWORD dwi = strlen(lps);
   if(dwi)
   {
      dwi--;
      if( lps[dwi] >= ' ' )
         strcat(lps, MEOR);
   }
}

VOID  EnsureCrLf2( LPTSTR lps )
{
   DWORD dwi = strlen(lps);
   while(dwi)
   {
      dwi--;
      if( lps[dwi] > ' ' )
         break;
      lps[dwi] = 0;  // kill any previous
   }
   if( *lps )  // if we have something
      strcat(lps, MEOR);   // append a normal Cr/Lf pair
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SplitFN
// Return type: void 
// Arguments  : LPTSTR pPath
//            : LPTSTR pFile
//            : LPTSTR pFullName
// Description: Split the pFullName into a PATH, including the final \, and
//              a clean FILE NAME ONLY
///////////////////////////////////////////////////////////////////////////////
void  SplitFN( LPTSTR pPath, LPTSTR pFile, LPTSTR pFullName )
{
   int      i, j, k;
   TCHAR    c;

   j = 0;
   if( pFullName )
      j = strlen(pFullName);
   if( j )
   {
      k = 0;
      for( i = 0; i < j; i++ )
      {
         c = pFullName[i];
         if( ( c == ':' ) || ( c == '\\' ) )
         {
            k = i;
         }
      }
      if( k )  // get LAST ':' or '\'
      {
         if( k < j )
            k++;
         if( pPath ) // if pPath given
         {
            strncpy(pPath,pFullName,k);
            pPath[k] = 0;
         }
         if( pFile ) // if pFile given
         {
            strcpy(pFile, &pFullName[k]);
         }
      }
      else
      {
         if( pFile ) // if pFile given
            strcpy(pFile, pFullName);  // then there is NO PATH
         if( pPath )
            *pPath = 0;
      }
   }
}

DWORD GetLastErrorMsg( LPTSTR lpm, DWORD dwLen, DWORD dwErr )
{
   PVOID lpMsgBuf = 0;
   DWORD    dwr;

   dwr = FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,   //	GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
		0,
		NULL );
   
   //dwr = strlen(lpMsgBuf);
   if( ( dwr == 0 ) || ( dwr >= dwLen ) )
      dwr = (DWORD)-1;
   else
      strcat(lpm, lpMsgBuf);

   //	printf("%s:%s\n",lpm,(LPCTSTR)lpMsgBuf);
   // Free the buffer.
   if( lpMsgBuf )
      LocalFree( lpMsgBuf );

   return dwr;
}

BOOL  Chk4Debug( LPTSTR lpd )
{
   BOOL     bret = FALSE;
   LPTSTR ptmp = &gszTmpBuf[0];
   LPTSTR   p;
   DWORD  dwi;

   strcpy(ptmp, lpd);
   dwi = strlen(ptmp);
   if(dwi)
   {
      dwi--;
      if(ptmp[dwi] == '\\')
      {
         ptmp[dwi] = 0;
         p = strrchr(ptmp, '\\');
         if(p)
         {
            p++;
            if( strcmpi(p, "DEBUG") == 0 )
            {
               *p = 0;
               strcpy(lpd,ptmp);    // use this
               bret = TRUE;
            }
         }
      }
   }
   return bret;
}

VOID  GetModulePath( LPTSTR lpb )
{
   LPTSTR   p;
   GetModuleFileName( NULL, lpb, 256 );
   p = strrchr( lpb, '\\' );
   if( p )
      p++;
   else
      p = lpb;
   *p = 0;
#ifndef  NDEBUG
   Chk4Debug( lpb );
#endif   // !NDEBUG

}

LPTSTR GetModulePathStg( VOID )
{
    LPTSTR   lps = GetStgBuf();
    GetModulePath( lps );
    return lps;
}

LPTSTR GetCWDStg( VOID )
{
    LPTSTR lps = GetStgBuf();
    size_t len;
    *lps = 0;
    _getcwd( lps, 256 );
    len = strlen(lps);
    if( len ) {
        if( lps[len-1] != '\\' )
            strcat(lps, "\\");
    }
    return lps;
}

// Options2Stg
typedef struct tagBIT2STG {
   DWORD    dwBit;
   LPTSTR   pStg;
   LPTSTR   pStgLong;
}BIT2STG, * PBIT2STG;

BIT2STG sSaveOpts[] = {
   { INCLUDE_SAME,      "S", "Same"    },
   { INCLUDE_DIFFER,    "D", "Differ"  },
   { INCLUDE_LEFTONLY,  "L", "Left"    },
   { INCLUDE_RIGHTONLY, "R", "Right"   },
   { APPEND_FILE,       "A", "Append"  },
   { INCLUDE_HEADER,    "H", "Header"  },
   { FULL_NAMES,        "F", "Full"    },
   { FLEFT_NAME,        "1", "FLeft"   },
   { FRIGHT_NAME,       "2", "FRight"  },
   { COMBINED_NAME,     "3", "FComb"   },
   { ADD_COMMENTS,      "C", "Results" },
   { ADD_REL_PATH,      ".", "Title"   },
   { ADD_X_HDR,         "X", "Lined"   }, // multi-lined version

   // end of table - must be last
   { 0,                 0,   0   }
};

PLE  GetSaveOpts( INT chr )
{
   LPTSTR   lps;
   LPTSTR   lpb = GetStgBuf();   // get a sting buffer
   PBIT2STG pb  = &sSaveOpts[0];
   DWORD    dwi;
   PLE      plh, pn;
   LPTSTR   cp;

   plh = (PLE)MALLOC( sizeof(LIST_ENTRY) );
   if( !plh )
      return 0;
   InitLList(plh);   // init the LIST

   lps = pb->pStg;
   if( chr == 0 )
   {
      while( lps )
      {
         //sprintf(lpb, "%s = %s ", lps, pb->pStgLong );
         sprintf(lpb, " %s = %s", lps, pb->pStgLong );
         dwi = strlen(lpb);

         pn = (PLE)MALLOC( sizeof(LIST_ENTRY) + dwi + 1 );
         if( !pn )
            return 0;

         cp = (LPTSTR)((PLE)pn + 1);

         strcpy( cp, lpb );   // copy string

         InsertTailList(plh,pn); // add to list end

         pb++; // next save option
         lps = pb->pStg;   // get pointer to 'switch' char
      }  // for whole list of SAVE options
   }
   else
   {
      // *** TBD *** return the string matching the character
      // to be checked ...
      while( lps )
      {
         if( *lps == chr )
         {
            sprintf(lpb, " %s = %s", lps, pb->pStgLong );
            dwi = strlen(lpb);
            pn = (PLE)MALLOC( sizeof(LIST_ENTRY) + dwi + 1 );
            if( !pn )
               return 0;

            cp = (LPTSTR)((PLE)pn + 1);

            strcpy( cp, lpb );   // copy string

            InsertTailList(plh,pn); // add to list end

            break;
         }
         pb++;
         lps = pb->pStg;
      }
   }

   return plh;
}

DWORD GetSaveBits( INT chr )
{
   LPTSTR   lps;
   PBIT2STG pb  = &sSaveOpts[0];

   lps = pb->pStg;
   while( lps )
   {
      if( *lps == (TCHAR)chr )
         return pb->dwBit;
      pb++;
      lps = pb->pStg;
   }
   return 0;
}

LPTSTR  SaveOpts2Stg( DWORD dwo, BOOL bLong )
{
   LPTSTR   lps = _sGetSStg();
   PBIT2STG pb  = &sSaveOpts[0];

   *lps = 0;
   while( dwo && pb->pStg )
   {
      if( dwo & pb->dwBit )
      {
         if(bLong)
         {
            if( *lps )
               strcat(lps,"|");
            strcat(lps, pb->pStgLong);
         }
         else
            strcat(lps,pb->pStg);

         dwo &= ~( pb->dwBit );
      }

      pb++;
   }

   if( ( *lps == 0 ) &&
      ( bLong ) )
      strcpy(lps, "<none>");

   return lps;
}

#if 0 // 00000000000000000000000000000000000000000000000000000
size_t to_narrow(const wchar_t* src, char* dest, size_t dest_len) 
{
    size_t i;
    wchar_t code;

    i = 0;

    while (src[i] != '\0' && i < (dest_len - 1)) {
        code = src[i];
        if (code < 128)
            dest[i] = (char)(code);
        else {
            dest[i] = '?';
            if (code >= 0xD800 && code <= 0xD8FF)
                // lead surrogate, skip the next code unit, which is the trail
                i++;
        }
        i++;
    }

    dest[i] = '\0';
    return i - 1;
}

char* wchar_to_char(const wchar_t* pwchar)
{
    // get the number of characters in the string.
    int currentCharIndex = 0;
    char currentChar = pwchar[currentCharIndex];

    while (currentChar != '\0')
    {
        currentCharIndex++;
        currentChar = pwchar[currentCharIndex];
    }

    const int charCount = currentCharIndex + 1;

    // allocate a new block of memory size char (1 byte) instead of wide char (2 bytes)
    char* filePathC = (char*)malloc(sizeof(char) * charCount);

    for (int i = 0; i < charCount; i++)
    {
        // convert to char (1 byte)
        char character = pwchar[i];

        *filePathC = character;

        filePathC += sizeof(char);

    }
    filePathC += '\0';

    filePathC -= (sizeof(char) * charCount);

    return filePathC;
}
#endif // #if 0 // 00000000000000000000000000000000000000000000000000000


//typedef struct _TIME_ZONE_INFORMATION { 
//    LONG       Bias; 
//    WCHAR      StandardName[ 32 ]; 
//    SYSTEMTIME StandardDate; 
//    LONG       StandardBias; 
//    WCHAR      DaylightName[ 32 ]; 
//    SYSTEMTIME DaylightDate; 
//    LONG       DaylightBias; 
//} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION;

VOID  GetTZI( VOID )
{
    char cp[256];
    size_t len;
#if 0 // 00000 seems to do nothing - still seem rubbish results 00000
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValue(NULL, SE_TIME_ZONE_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
#endif // 0000000000000000000000000000000000000000000000000000000000000000000

    // Retrieve the current time zone information
    memset(&g_sTZ, 0, sizeof(TIME_ZONE_INFORMATION));
    g_dwTZID = GetTimeZoneInformation( &g_sTZ );   // time zone
    if( g_dwTZID != TIME_ZONE_ID_INVALID )
    {
      LPTSTR   lpd = &gszTmpBuf[0];
      sprintf( lpd, "TZ: Bias=%d", g_sTZ.Bias );
      len = wcslen(g_sTZ.StandardName);
      if (len) {
          //len = to_narrow(g_sTZ.StandardName, cp, 256);
          len = wcstombs(cp, g_sTZ.StandardName, len);
          cp[len] = 0;
          // cp = W2A(g_sTZ.StandardName);
          sprintf(EndBuf(lpd), " SN=[%s] ", cp);
      } 
      else
         strcat(lpd, " SN=<null> ");

      AppendDateTime2( lpd, &g_sTZ.StandardDate );

      sprintf(EndBuf(lpd), " SB=%d", g_sTZ.StandardBias );

      len = wcslen(g_sTZ.DaylightName);
      if (len) {
          len = wcstombs(cp, g_sTZ.DaylightName, len);
          cp[len] = 0;
          sprintf(EndBuf(lpd), " DN=[%s] ", cp);
      }
      else
         strcat( lpd, " DN=<null> " );

      AppendDateTime2( lpd, &g_sTZ.DaylightDate );

      sprintf(EndBuf(lpd), " DB=%d"MEOR, g_sTZ.DaylightBias );

      sprtf(lpd);
   }
   else
   {
      sprtf( "WARNING: Timezone Information is INVALID!"MEOR );
   }
}

static char gszAppData[264] = "\0";
#ifndef PATH_SEP
#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
#endif // PATH_SEP
#ifdef _WIN32
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif

#define MDT_NONE 0
#define	MDT_FILE 1
#define	MDT_DIR 2

static struct stat buf;

int is_file_or_directory(const char* path)
{
    if (!path)
        return MDT_NONE;
    if (stat(path, &buf) == 0)
    {
        if (buf.st_mode & M_IS_DIR)
            return MDT_DIR;
        else
            return MDT_FILE;
    }
    return MDT_NONE;
}

size_t get_last_file_size() { return buf.st_size; }


int create_dir(const char* pd)
{
    int iret = 1;
    int res;
    if (is_file_or_directory(pd) != MDT_DIR) {
        size_t i, j, len = strlen(pd);
        char ps, ch, pc = 0;
        char tmp[260];
        j = 0;
        iret = 0;
        tmp[0] = 0;
#ifdef _WIN32
        ps = '\\';
#else
        ps = '/'
#endif

            for (i = 0; i < len; i++) {
                ch = pd[i];
                if ((ch == '\\') || (ch == '/')) {
                    ch = ps;
                    if ((pc == 0) || (pc == ':')) {
                        tmp[j++] = ch;
                        tmp[j] = 0;
                    }
                    else {
                        tmp[j] = 0;
                        if (is_file_or_directory(tmp) != MDT_DIR) {
                            res = mkdir(tmp);
                            if (res != 0) {
                                return 0; // FAILED
                            }
                            if (is_file_or_directory(tmp) != MDT_DIR) {
                                return 0; // FAILED
                            }
                        }
                        tmp[j++] = ch;
                        tmp[j] = 0;
                    }
                }
                else {
                    tmp[j++] = ch;
                    tmp[j] = 0;
                }
                pc = ch;
            } // for lenght of path
        if ((pc == '\\') || (pc == '/')) {
            iret = 1; // success
        }
        else {
            if (j && pc) {
                tmp[j] = 0;
                if (is_file_or_directory(tmp) == MDT_DIR) {
                    iret = 1; // success
                }
                else {
                    res = mkdir(tmp);
                    if (res != 0) {
                        return 0; // FAILED
                    }
                    if (is_file_or_directory(tmp) != MDT_DIR) {
                        return 0; // FAILED
                    }
                    iret = 1; // success
                }
            }
        }
    }
    return iret;
}

void GetAppData(PTSTR lpini)
{
    char* pd;
    if (!gszAppData[0]) {
        //pd = getenv("PROGRAMDATA"); // UGH - do not have permissions -  how to GET permissions
        //if (!pd) {
        //	pd = getenv("ALLUSERSPROFILE");
        //}
        pd = getenv("APPDATA");
        if (!pd) {
            pd = getenv("LOCALAPPDATA");
        }
        if (pd) {
            strcpy(gszAppData, pd);
            strcat(gszAppData, PATH_SEP);
            strcat(gszAppData, "dc4w");
            if (!create_dir(gszAppData)) {
                gszAppData[0] = 0;
            }
            else {
                strcat(gszAppData, PATH_SEP);
            }
        }
    }

    if (gszAppData[0]) {
        strcpy(lpini, gszAppData);
    }
    else {
        GetModulePath(lpini);    // does GetModuleFileName( NULL, lpini, 256 );
    }
}



LPTSTR   Rect2Stg( PRECT pr )
{
   LPTSTR   lpb = GetStgBuf();
   sprintf(lpb, "(%d,%d,%d,%d)",
      pr->left,
      pr->top,
      pr->right,
      pr->bottom );
   return lpb;
}

// This gets complicated
// User can enter just a DIRECTORY name, like d:\foo
// or a directory plus file, like d:\foo\filename
// or a directory plus file containing wild characters, like d:\foo\*.c
// How to definitively EXTRACT the directory ONLY

BOOL  GetDirectoryOnly( LPTSTR lpb2, LPTSTR lpb )
{
   BOOL     bGot = FALSE;
   //LPTSTR   lpb1 = &gszBuf1[0];
   LPTSTR   lpb1 = GetStgBuf();
   BOOL     bRet = TRUE;
   BOOL     bWild; // check for any WILD characters
   //LPTSTR   p;
   strcpy( lpb1, lpb );
   strcpy( lpb2, lpb );
   //p = strrchr(lpb1,'\\');    // get LAST slash
   while( bRet )
   {
      WIN32_FIND_DATA   fd;
      HANDLE   hFind = FindFirstFile(lpb1, &fd);
      // bWild = IsWild(lpb1); // check for any WILD characters
      bWild = GotWild(lpb1);
      if( VFH(hFind) )
      {
         if(( !bWild ) &&  // if NO wild characters, AND it is a DIRECTORY
            ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
         {
            strcpy(lpb2,lpb1);   // this is DEFINITIVE
            bRet = FALSE;
            bGot = TRUE;
         }
 			FindClose(hFind);
      }
      if( bRet )
      {
         // we MISSED - why?
         DWORD dwi = strlen(lpb1);
         if(dwi)
         {
            dwi--;   // back to last char
            lpb1[dwi] = 0;
            if(dwi)
            {
               dwi--;
               if(lpb1[dwi] == '\\')
                  lpb1[dwi] = 0;
            }
         }
         if( !dwi )
         {
            bRet = FALSE;
         }
      }
   }
   return bGot;
}


LPTSTR   GetMinTxt( LPTSTR pb1, LPTSTR pb2, LPTSTR pSep )
{
   LPTSTR lpb1 = pb1;
   LPTSTR lpb2 = pb2;
   LPTSTR   lpb = GetStgBuf();
   LPTSTR   cp1, cp2;
   LPTSTR   ptmp = &gszTmpBuf[0];
   while( ( *lpb1 <= ' ' ) && ( *lpb1 != 0 ) )
      lpb1++;
   while( ( *lpb2 <= ' ' ) && ( *lpb2 != 0 ) )
      lpb2++;

   cp1 = cp2 = 0;
   while( *lpb1 && ( toupper(*lpb1) == toupper(*lpb2) ) )
   {
      //lpb1++;
      //lpb2++;
      if( ( *lpb1 == '\\' ) || ( *lpb1 == '/' ) || ( *lpb1 == ':' ) )
      {
         cp1 = lpb1 + 1;  // keep this 'good' break point
         cp2 = lpb2 + 1;
      }
      lpb1++;
      lpb2++;
   }
   if( *lpb1 )
   {
      if( cp1 )
      {
         //sprintf(ptmp, "%s : %s", cp1, cp2);
         sprintf(ptmp, "%s%s%s", cp1, pSep, cp2);
         ptmp[256] = 0;
         strcpy(lpb,ptmp);
      }
      else
      {
         //sprintf(ptmp, "%s : %s", lpb1, lpb2);
         sprintf(ptmp, "%s%s%s", lpb1, pSep, lpb2);
         ptmp[256] = 0;
         strcpy(lpb,ptmp);
      }
   }
   else
   {
      //if( cp1 )
      //   strcpy(lpb,cp1);
      strcpy( lpb, pb1 );
   }
   return lpb;
}

TCHAR _s_szTmpBuf[264];
TCHAR _s_szBuf1[264];

//typedef struct tagFOUNDLIST {
//   LIST_ENTRY  sList;   // at head
//   TCHAR       szDir[264];
//   TCHAR       szFile[264];
//   TCHAR       szExt[264];
//   WIN32_FIND_DATA   sFD;
//}FOUNDLIST, * PFOUNDLIST;

DWORD  dc4wProcessDir( LPTSTR lpdir, PLE pFileList, BOOL bReCur, DWORD level )
{
//   LPTSTR            lpmask = &g_szTmpBuf[0];
//   LPTSTR            lpd    = &gszBuf1[0];
   LPTSTR            lpmask = &_s_szTmpBuf[0];
   LPTSTR            lpd    = &_s_szBuf1[0];
   WIN32_FIND_DATA	fd;
   HANDLE            hFind;
   //PLE               ph = &g_sFoundList;
   PLE               ph = pFileList;
   PLE               pn;
   PFOUNDLIST        pfnd;
   LPTSTR            lpf;
   DWORD             dwCnt = 0;

   strcpy( lpmask, lpdir );

   if( lpdir[ (strlen(lpdir) - 1) ] != '\\' )
      strcat( lpmask, "\\*.*" );
   else
      strcat( lpmask, "*.*"   );

   hFind = FindFirstFile( lpmask, &fd );
   lpf = &fd.cFileName[0];
	if( VFH(hFind) )
	{
      do
      {
         if(( *lpf == '.' ) &&
            ( ( strcmp(lpf,".") == 0 ) || ( strcmp(lpf,"..") == 0 ) ) )
         {
            // forget these
         }
         else
         {
            // we have a FILE or DIRECTORY
            if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
               // it is a DIRECTORY
               //if( g_bRecur )
               if( bReCur )
               {
                  strcpy( lpd, lpdir );
                  strcat( lpd, "\\"  );
                  strcat( lpd, lpf   );
                  // get count of each iterative call down the tree
                  dwCnt += dc4wProcessDir( lpd, pFileList, bReCur, (level + 1) );
               }
            }
            else
            {
               // just another FILE
               pn = (PLE)MALLOC( sizeof(FOUNDLIST) );
               if(pn)
               {
                  pfnd = (PFOUNDLIST)pn;
                  strcpy( &pfnd->szDir[0], lpdir );
                  //SplitExt( &pfnd->szFile[0], &pfnd->szExt[0], lpf  );
                  SplitExt2( &pfnd->szFile[0], &pfnd->szExt[0], lpf  );
                  memcpy( &pfnd->sFD, &fd, sizeof(WIN32_FIND_DATA) );
                  InsertTailList(ph,pn);
                  dwCnt++; // count a FILE found - and added to LIST
               }
               else
               {
                  sprtf( "ERROR: Memory FAILED!" );
                  exit(1);
               }

            }
         }
      } while( FindNextFile( hFind, &fd ) );
 	   FindClose(hFind);
   }
   else
   {
      //printf( "WARNING: Failed to find ANYTHING for"PEOR
      sprtf( "WARNING: Failed to find ANYTHING for"MEOR
         "[%s]"MEOR,
         lpdir );
   }
   return dwCnt;

}

PTSTR GetNxtBuf( VOID )
{
   g_iNxtBuf++;
   if(g_iNxtBuf >= MXTMPBUFS)
      g_iNxtBuf = 0;
   return &g_szNxtBuf[ g_iNxtBuf * MXFNB ];
}

// eof - dc4wUtils.c
