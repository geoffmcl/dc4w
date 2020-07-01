

// dc4wSB.c
// this is public domain software - praise me, if ok, just don't blame me!
// implementation of a status bar window

#include "dc4w.h"
// #include "dc4wSB.h"

// ewmSB.c (from ewSB.c (from DDBSB.c))
// #define	_GMSB1		/* this is system include */
//#include	"ewm.h"

#ifdef	ADDSTATUS
#define  USESTAT3    // this is the newer code
#define  ADDTT2STAT  // try adding TOOLTIP to STATUS bar

//#include  "ewmSB.h"
//#include    <commctrl.h>	// Common controls
//#ifdef	ADDPOST
//#include	"DDBPost.h"		// Add Post Office Flags
//#endif	// ADDPOST

#ifdef	WIN32
extern	DWORD	GetTextExtent( HDC hdc, LPSTR lpS, int len );
#endif	// WIN32

static TCHAR sszTmDef[] = TEXT("00:0000");
//TCHAR sszLC[] = TEXT("Ln:0000 Col:0000 ");
TCHAR gszLC[264] = { TEXT("Ln:000 Col:000 Chr:000") };
//TCHAR sszLC[] = TEXT("Ln:0000 Col:0000 ");
//TCHAR gszLC[264] = { TEXT("Ln:1 Col:1 Chr:1") };
INT   giChgLC    = 0;
#ifndef  MXSBPARTS
#define  MXSBPARTS  DEF_MX_PARTS 
#endif   // MXSBPARTS

#define  PART_TEXT      0
#define  PART_TIME      1

TCHAR g_szSBTT[] = "Program Status Messages,"MEOR
"and the Time HH:MM."MEOR
"Also use the bottom right for window sizing.";

//#ifdef   USESTAT3

TCHAR sszDef[] = "Ready";
TCHAR gszM1[264] = { "\0" }; // current message

PSB	glpSB = 0;

// local
BOOL	GetStringLens( PSB pSB );
VOID  SBSetParts( PSB pSB, int iWid );
void	SBUpdateTime( PSB pSB, int ih, int im );
BOOL	SBSetText( PSB pSB, LPSTR lpText, int iPane );
VOID  SBSetLnCol( PSB pSB, DWORD dwLn, DWORD dwCol, DWORD dwChr );
void	SBSizePanes( PSB pSB, int iWidth );
VOID  SBTimer( PSB pSB );
int	SBInit( HWND hPar, HWND hSB, PSB pSB );
int	SBCreate( HWND hwnd, PSB pSB, BOOL bHide );
VOID  SBSetDefault( VOID );
VOID  SBUpdateTT( LPTSTR lps );

void	SBUpdateTime( PSB pSB, int ih, int im )
{
	static char	_sszTm[64];
   LPTSTR   lpt = &_sszTm[0];
	wsprintf( lpt,
		"%02d:%02d",
		ih,
		im );
//		lpWS->wHour,
//		lpWS->wMinute );
	SBSetText( pSB, lpt, PART_TIME );
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SBSetText
// Return type: BOOL 
// Arguments  : PSB pSB
//            : LPSTR lpText
//            : int iPane
// Description: Set the given text in the given pane of the status bar
//              
///////////////////////////////////////////////////////////////////////////////
BOOL	SBSetText( PSB pSB, LPSTR lpText, int iPane )
{
	BOOL	bFlg;
	HWND	hStat;
	bFlg = FALSE;
	if( lpText )
   {
      hStat = pSB->sb_hStatus;
      if( ( hStat ) &&
         ( iPane < pSB->sb_nPanes  ) )
      {
   		SendMessage( hStat,
	   		SB_SETTEXT,
		   	(WPARAM)iPane,
			   (LPARAM)lpText );

         if( iPane == 0 )
         {
            strcpy(gszM1,lpText);   // copy text to "current"
         }

		   bFlg = TRUE;
      }
	}
	return bFlg;
}

#define  STATUS_BAR_HEIGHT    20

DWORD  SBSize( PSB pSB, WPARAM wParam, LPARAM lParam )
{
   if(( pSB ) &&
      ( pSB->sb_hStatus ) )
   {
      // position and size SB
      SendMessage(pSB->sb_hStatus, WM_SIZE, wParam, lParam);
      // and adjust pane sizes
      SBSizePanes( pSB, LOWORD(lParam) ); // = nWidth;

      
   }
   return STATUS_BAR_HEIGHT;
}
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SBSetLnCol
// Return type: VOID 
// Arguments  : PSB pSB
//            : DWORD dwLn
//            : DWORD dwCol
//            : DWORD dwChr
// Description: Set Line (Row) and Column
//              
///////////////////////////////////////////////////////////////////////////////
VOID  SBSetLnCol( PSB pSB, DWORD dwLn, DWORD dwCol, DWORD dwChr )
{
   static TCHAR sszLC[264];
   LPTSTR   lpb = &sszLC[0];

   // (dwLineNumber + 1),
   // (sCurrSel.cpMin - dwLineIndex + 1) );
   wsprintf(lpb,
      "Ln:%d Col:%d Chr:%d",
      dwLn,
      dwCol,
      (dwChr + dwCol) );

   if( strcmp( lpb, gszLC ) )
   {
      giChgLC = TRUE;
      strcpy( gszLC, lpb );
      pSB->sb_bDnLens = FALSE;
      SBSizePanes( pSB, pSB->sb_iWidth );
      // SB_SETTEXT to invalidate this section and send WM_DRAWITEM
//    		SendMessage( pSB->sb_hStatus,
//	      	SB_SETTEXT,
//		      (WPARAM)( 1 | SBT_OWNERDRAW ),   // set OWNER DRAW (WM_DRAWITEM)
//            0 );
   }

   // finally put the intended text
   SBSetText( pSB, lpb, 1 );

}

// called with a NEW width
void	SBSizePanes( PSB pSB, int iWidth )
{
	HWND	hStat;
	int		dx;
	if( pSB )
   {
      hStat = pSB->sb_hStatus;
      dx = iWidth;
      if( hStat )
      {
         if( dx <= 0 )
            dx = 1;

   		// Set the status bar to have three parts.
         pSB->sb_iWidth = dx;    // set WIDTH
         if( !pSB->sb_bDnLens )
            GetStringLens( pSB );

		   if( pSB->sb_bDnLens )
		   {
            SBSetParts( pSB, dx );
            SendMessage( hStat,
               SB_SETPARTS,
               (WPARAM)pSB->sb_nPanes,       // count of PANES
               (LPARAM)&pSB->sb_iParts[0] ); // array of widths
		   }
      }
	}
}

VOID  SBTimer( PSB pSB )
{
   if( pSB->sb_hStatus )
   {
      SYSTEMTIME st;
      GetLocalTime(&st);
      SBUpdateTime(pSB,
         (st.wHour   & 0xffff),
         (st.wMinute & 0xffff) );
   }
}

VOID  SBSetParts( PSB pSB, int iWid )
{
   int   dx;
//   RECT  rc;

//   rc.right = rc.left = 0;
   dx = iWid;
   if(dx)
   {
      //if( pSB->sb_hStatus )
      //{
      //   if( GetWindowRect(pSB->sb_hStatus,&rc) )
      //   {
      //      if( (rc.right - rc.left) > dx )
      //      {
      //         dx = rc.right - rc.left;
      //      }
      //   }
      //}
         if( dx < pSB->sb_idxTm )
         {
            // wow, very small stuff
            pSB->sb_iParts[PART_TEXT] = 0;
            pSB->sb_iParts[PART_TIME] = -1; // TIME should take ALL
         }
         else
         {
            dx -= pSB->sb_idxTm;    // reduce width by time size
            pSB->sb_iParts[PART_TEXT] = dx;
            pSB->sb_iParts[PART_TIME] = -1; // TIME should take ALL

            //dx -= pSB->sb_idxTm;    // reduce width by time size
            //if( dx < pSB->sb_idxLC )
            //{
            //   pSB->sb_iParts[0] = 0;  // first part is ZERO
            //   pSB->sb_iParts[1] = dx; // "little" part for Line/Column
            //   pSB->sb_iParts[2] = -1; // the balance
            //}
            //else
            //{
            //   dx -= pSB->sb_idxLC; // reduce by Line / Column size
            //   pSB->sb_iParts[0] = dx;
            //   pSB->sb_iParts[1] = pSB->sb_iParts[0] + pSB->sb_idxLC;
            //   pSB->sb_iParts[2] = -1;
            //}
         }

   }
   else
   {
            // wow, NO WIDTH
            pSB->sb_iParts[PART_TEXT] = 0;    // this should take all
            pSB->sb_iParts[PART_TIME] = -1;
            //pSB->sb_iParts[2] = -1;
   }

//   sprtf( "SB PARTS: LC=%d LN=%d TN=%d lc=%d tm=%d Wid=%d."MEOR,
//      pSB->sb_iParts[0],
//      pSB->sb_iParts[1],
//      pSB->sb_iParts[2],
//      pSB->sb_idxLC,
//      pSB->sb_idxTm,
//      iWid );
}

BOOL	GetStringLens( PSB pSB )
{
	BOOL	   flg = FALSE;
	HWND	   hWnd;
	if( pSB )
   {
      hWnd = pSB->sb_hStatus;
      if(hWnd)
      {
      	HDC		hDC;
	   	if( !pSB->sb_bDnLens )
		   {
            hDC = GetDC( hWnd );
            if(hDC)
            {
      		   DWORD	   dwExt;
               LPTSTR   lpt = &sszTmDef[0];
               LPTSTR   lpm = &gszLC[0];
               DWORD    dwl;

               dwl = strlen(lpt);
               if(dwl)
               {
                  dwExt   = GetTextExtent( hDC, lpt, dwl );
			         pSB->sb_idxTm = (int)LOWORD(dwExt);
			         pSB->sb_idyTm = (int)HIWORD(dwExt);
                  flg++;
               }

               dwl = strlen(lpm);
               if(dwl)
               {
			         dwExt   = GetTextExtent( hDC, lpm, dwl );
   			      pSB->sb_idxLC = (int)LOWORD(dwExt);
	   		      pSB->sb_idyLC = (int)HIWORD(dwExt);
		   	      flg++;
               }
               if( flg == 2 )
                  pSB->sb_bDnLens = flg = TRUE;
               else
                  flg = FALSE;

               ReleaseDC( hWnd, hDC );
            }
         }
      }
	}
	return flg;
}

int	SBInit( HWND hPar, HWND hSB, PSB pSB )
{
	int		i = 0;
   RECT     rc;
//	lpWS->nWindows++;
	if( ( hSB  ) &&
	    ( hPar ) )
	{
      glpSB = pSB;   // NOTE: Allocated memory
      pSB->sb_hStatus = hSB;
		pSB->sb_hFrame  = hPar;

      GetClientRect(hPar,&rc);
		pSB->sb_nPanes = DEF_MX_PARTS;      // was MXSBPARTS
   	// Set the status bar to have THREE parts.
		if( GetStringLens( pSB ) )
		{
         SBSetParts(pSB, rc.right);
   		SendMessage( hSB,
            SB_SETPARTS,
            (WPARAM)pSB->sb_nPanes,
            (LPARAM)&pSB->sb_iParts[0] );
   		i++;
		}
		// set the text for the status bar
		//SetSBperID( lpWS, -1, 0 );
		//SetSBperID( lpWS, -1, 1 );
#ifdef  ADDTT2STAT  // try adding TOOLTIP to STATUS bar
//      DWORD dwstyle = WS_CHILD | WS_BORDER | WS_VISIBLE | SBT_TOOLTIPS;	// window styles
      SendMessage( hSB, SB_SETTIPTEXT, 0, (LPARAM) &g_szSBTT[0] );
      SendMessage( hSB, SB_SETTIPTEXT, 1, (LPARAM) &g_szSBTT[0] );

#else // !
//      DWORD dwstyle = WS_CHILD | WS_BORDER | WS_VISIBLE;	// window styles
#endif   // y/n
	}
	return i;

}

int	SBCreate( HWND hwnd, PSB pSB, BOOL bHide )
{
	int	i = (int)-1;
	if(( hwnd ) &&
		( IsWindow(hwnd) ) &&
		( pSB           ) )
	{
#ifdef  ADDTT2STAT  // try adding TOOLTIP to STATUS bar
      DWORD dwstyle = WS_CHILD | WS_BORDER | WS_VISIBLE | SBT_TOOLTIPS;	// window styles
#else // !
      DWORD dwstyle = WS_CHILD | WS_BORDER | WS_VISIBLE;	// window styles
#endif   // y/n

		// Ensure the common controls are loaded
		InitCommonControls();
		// got the size and position of the parent window
		// First, create the status window.
		pSB->sb_hStatus =
			CreateStatusWindow( dwstyle,	// window styles
			"",				// default window text
			hwnd,		// parent window
			ID_STATUS );	// ID

		if( pSB->sb_hStatus )
		{
			if( SBInit( hwnd, pSB->sb_hStatus, pSB ) )
			{
				i = 0;
            if( bHide )
            {
               pSB->sb_bHidden = FALSE;
               SBToggleHide( pSB );
            }
			}
		}
	}
	return i;
}

VOID  SBSetDefault( VOID )
{
   PSB pSB = glpSB;
   if( ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      //if( strcmp( sszDef, gszM1 ) ) // TCHAR sszDef[] = "Ready";
      //if( strcmp( lpm, gszM1 ) ) // TCHAR sszDef[] = "Ready";
      {
         //strcpy( gszM1, sszDef );
         //strcpy( gszM1, lpm );
         //gdwSBSecs = 0;
         SBSetText( pSB, sszDef, 0 );
      }
   }
}

VOID  SBHide( PSB pSB )
{
   if( ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      ShowWindow( pSB->sb_hStatus, SW_HIDE );
      pSB->sb_bOff = TRUE;
   }
}
VOID  SBShow( PSB pSB )
{
   if( ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      ShowWindow( pSB->sb_hStatus, SW_SHOW );
      pSB->sb_bOff = FALSE;
   }
}
VOID  SBToggleHide( PSB pSB )
{
   if( ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      pSB->sb_bHidden = ! (pSB->sb_bHidden);
//      ShowWindow( pSB->sb_hStatus,
//         (pSB->sb_bHidden ? SW_HIDE : SW_SHOW) );
      if(pSB->sb_bHidden)
         SBHide(pSB);
      else
         SBShow(pSB);
   }
}

#ifdef   ADDTMDTXT
TCHAR gszTimedTxt[264] = { "\0" };
DWORD gdwTimedTxt;
DWORD gdwSBSecs = 0;
DWORD gdwSBSec2 = 0;

VOID  DoSBSecond( HWND hWnd )
{
   // gszM1
   if( gdwSBSecs )
   {
      if(gdwSBSecs != -1)
         gdwSBSecs--;

      if( gdwSBSecs == 0 )
      {
         SBSetDefault();
      }
      else if( gszTimedTxt[0] )
      {
         TCHAR _s_sztimetxt[264];
         LPTSTR ptxt = _s_sztimetxt;
         PSB pSB = glpSB;
         gdwTimedTxt++;
         sprintf(ptxt,"%s - %d of %d",
            gszTimedTxt,
            gdwTimedTxt,
            gdwSBSec2 );
         if(( pSB             ) &&
            ( pSB->sb_hStatus ) )
         {
            SBSetText( pSB, ptxt, 0 );
         }
      }
   }
}
///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SBSetTimedTxt
// Return type: VOID 
// Arguments  : LPTSTR lps
//            : DWORD dwSecs
//            : BOOL bAddTic
// Description: Set TIMED text = timedtext
//              
///////////////////////////////////////////////////////////////////////////////
VOID  SBSetTimedTxt( LPTSTR lps, DWORD dwSecs, BOOL bAddTic )
{
   PSB pSB = glpSB;
   if( ( lps ) &&
       ( *lps ) &&
       ( dwSecs ) &&
       ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      strcpy(gszTimedTxt,lps);   // set the timed text base
      if( bAddTic )
      {
         gdwTimedTxt = 0;           // and counter
      }
      else
      {
         gszTimedTxt[0] = 0;
         gdwTimedTxt = (DWORD)-1;
      }

      SBSetText( pSB, lps, PART_TEXT );  // set the TEXT
      gdwSBSecs = dwSecs;        // set the TIMEOUT
      gdwSBSec2 = dwSecs;        // set the TIMEOUT
   }
}  
#endif   // #ifdef   ADDTMDTXT

VOID  SBUpdateTT( LPTSTR lps )
{
   PSB pSB = glpSB;
   if( ( lps ) &&
       ( *lps ) &&
       ( pSB ) &&
       ( pSB->sb_hStatus ) )
   {
      SBSetText( pSB, lps, 0 );  // set the TEXT
   }
}  


//#else // !USESTAT3
//#endif   // USESTAT3 y/n

#endif	// ADDSTATUS
// eof - DDBSB.c
// eof - ewmSB.c
// eof - dc4wSB.c
