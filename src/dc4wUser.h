

// dc4wUser.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wUser_H
#define	_dc4wUser_H


/* messages to Table class */
#define  MYWMOFF        (WM_USER + 567)   // try to ensure unique

/* data, or nrows has changed  wParam/lParam null*/
#define TM_REFRESH      (MYWMOFF + 0 )    // from WM_USER up

/* nr cols/props/layout has changed  - wparam/lparam null */
#define TM_NEWLAYOUT    (MYWMOFF + 1 )    // from WM_USER up

/* Close old id, and display new - wParam null, lParam has new id */
#define TM_NEWID        (MYWMOFF + 2 )    // WM_USER+

/* Select and show this area - wParam null, lParam is lpTableSelection */
#define TM_SELECT       (MYWMOFF + 3 )    // WM_USER++

/* Print current table - wParam null, lParam either null
 * or lpPrintContext.
 */
#define TM_PRINT        (MYWMOFF + 4 )    // WM_USER++

/* Return the top row in the window. If wParam is TRUE, then set
 * lParam to be the new toprow. top row is the number of rows scrolled down
 * from the top. Thus the first visible non-fixed row is toprow+fixedrows
 */
#define TM_TOPROW       (MYWMOFF + 5 )    // WM_USER++


/* Return the end row visible. This is the 0-based rownr of the last
 * visible row in the window
 */
#define TM_ENDROW       (MYWMOFF + 6 )    // WM_USER++

/* New rows have been added to the end of the table, but no other
 * rows or cols or properties have been changed.
 * wParam contains the new total nr of rows. lParam contains the id
 * in case this has changed.
 */
#define TM_APPEND       (MYWMOFF + 7 )    // WM_USER++

// NEW
#ifdef   ADDOLDDIFF
// write to a "difference" file - later abandoned
#define  TM_WRITEDIFF   ( MYWMOFF + 8 )    // WM_USER++
#endif   // #ifdef   ADDOLDDIFF

// return total row count
#define  TM_ROWCOUNT    ( MYWMOFF + 9 )    // WM_USER++
// try combination TM_SELECT, with an ENTER to expand the item
#define  TM_ENTER       ( MYWMOFF + 10)   // WM_USER++

/*-- private messages -- */
/* Send this to the main window. Return value is the VIEW handle */
#define TM_CURRENTVIEW  ( MYWMOFF + 50 )  // WM_USER
/* send these window messages to the class */

#define SM_NEW          ( MYWMOFF + 60 )     /* wParam handle for new status line */
#define SM_SETTEXT      ( MYWMOFF + 61 )     /* wparam: item id, lparam new label*/

#define  MWM_TABLE      ( MYWMOFF + 100)  // table class message exchange

#endif	// _dc4wUser_H
// eof - dc4wUser.h

