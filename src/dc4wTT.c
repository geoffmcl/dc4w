
// dc4wTT.c - TOOLTIP control
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"
#include <shlwapi.h>
// #include "dc4wTT.h"

#define PACKVERSION(major,minor) MAKELONG(minor,major)

//#define  TT_LEFT     0x00000001
//#define  TT_RIGHT    0x00000002
//#define  TT_BOTH     ( TT_LEFT | TT_RIGHT )
//#define  TT_SMALLER  0x00000080
//#define  TT_LARGER   0x00000040
//#define  TT_YOUNGER  0x00000020
//#define  TT_OLDER    0x00000010
//#define  TT_MARKED   0x80000000  // marked for exclusion
//#define  TT_FLAGS (TT_LEFT|TT_RIGHT|TT_SMALLER|TT_LARGER|TT_YOUNGER|TT_OLDER)

TCHAR gszLf[] = "Left  ";
TCHAR gszRt[] = "Right ";

//typedef struct tagTOOLINFO{
//    UINT      cbSize; 
//    UINT      uFlags; 
//    HWND      hwnd; 
//    WPARAM    uId; 
//    RECT      rect; 
//    HINSTANCE hinst; 
//    LPTSTR    lpszText; 
//#if (_WIN32_IE >= 0x0300)
//    LPARAM lParam;
//#endif
//} TOOLINFO, NEAR *PTOOLINFO, FAR *LPTOOLINFO; 
VOID  EnableToolTip( VOID )
{
   if( g_hwndTT )
      SendMessage( g_hwndTT, TTM_ACTIVATE, (WPARAM)!gbNoTT, 0 );
}

/* CREATE A TOOLTIP CONTROL OVER THE ENTIRE WINDOW AREA */
BOOL  CreateMyTooltip( HWND hwnd )
{
   BOOL  bRet = FALSE;
   // struct specifying control classes to register
   INITCOMMONCONTROLSEX iccex; 
   // struct specifying info about tool in tooltip control
   LPTOOLINFO    pti = &g_sTI;
   unsigned int uid = 0;       // for ti initialization
   //LPTSTR lptstr = &g_strTT[0];
   RECT rect;                  // for client area coordinates

   if( !IsWindow(hwnd) )
      return FALSE;

   /* INITIALIZE COMMON CONTROLS */
   iccex.dwICC  = ICC_WIN95_CLASSES;
   iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   InitCommonControlsEx(&iccex);
	
   /* CREATE A TOOLTIP WINDOW */
   g_hwndTT = CreateWindowEx(WS_EX_TOPMOST,
       TOOLTIPS_CLASS,
       NULL,
       WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       hwnd,
       NULL,
       g_hInst,
       NULL );

   if( !g_hwndTT )
      return FALSE;


   SetWindowPos( g_hwndTT,
       HWND_TOPMOST,
       0,
       0,
       0,
       0,
       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   /* GET COORDINATES OF THE MAIN CLIENT AREA */
   GetClientRect (hwnd, &rect);
	
   /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
   pti->cbSize      = sizeof(TOOLINFO);
   // TTF_TRANSPARENT
   pti->uFlags      = TTF_SUBCLASS;
   pti->hwnd        = hwnd;
   pti->hinst       = g_hInst;
   pti->uId         = uid;
   //pti->lpszText    = lptstr;
   pti->lpszText    = LPSTR_TEXTCALLBACK;

   // Tooltip control will cover the whole window
   pti->rect.left   = rect.left;    
   pti->rect.top    = rect.top;
   pti->rect.right  = rect.right;
   pti->rect.bottom = rect.bottom;
   
   /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
   bRet = (BOOL) SendMessage( g_hwndTT, TTM_ADDTOOL, 0, (LPARAM) pti );
   // to change the text send TTM_UPDATETIPTEXT
   EnableToolTip();

   return bRet;
} 

VOID  Do_IDM_SHOWTOOLTIP( HWND hWnd )
{
   ToggleBool( &gbNoTT, &bChgTT, !gbNoTT );
   EnableToolTip();
}


DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

/*Because some DLLs may not implement this function, you
 *must test for it explicitly. Depending on the particular 
 *DLL, the lack of a DllGetVersion function may
 *be a useful indicator of the version.
*/
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

BOOL  IsComCtl32400( VOID )
{
   BOOL  bRet = FALSE;
//   if( GetDllVersion( TEXT("comctl32.dll") ) >= PACKVERSION(4,00) )
   if( GetDllVersion( TEXT("comctl32.dll") ) >= PACKVERSION(4,70) )
   {
      bRet = TRUE;
   }
   return bRet;
}


DWORD AddToolText( LPTSTR lptstr, COMPITEM ci )
{
   static TCHAR _s_sztooltxt[264];
   DWORD             dwi = 0;
   PWIN32_FIND_DATA  pfd1, pfd2;
   LONG              lg;
   LARGE_INTEGER     li1, li2;
   LPSYSTEMTIME      pst1, pst2;  // receives system time
   BOOL              b1, b2;
   LPTSTR            lpb = _s_sztooltxt;
   LPTSTR            lpl = &g_szBuf1[0];
   LPTSTR            lpr = &g_szBuf2[0];
   //LONG              rows;

   pfd1 = &g_sFDL;
   pfd2 = &g_sFDR;
   pst1 = &g_sST1;
   pst2 = &g_sST2;
   b1 = b2 = FALSE;
   //rows = view_getrowcount( current_view ),
   //ci = view_getitem( current_view, row);
   //if(lptstr)
   //   *lptstr = 0;   // set NO text
   *lpl = 0;
   *lpr = 0;
   if(ci)
   {
      FILEDATA pfd;
      DIRITEM  pdi;
      LPTSTR   p;
      pfd = compitem_getleftfile(ci);
      *lpl = 0;
      if(pfd)
      {
         pdi = file_getdiritem(pfd);
         p   = dir_getfullname(pdi);
         if(p)
            strcpy(lpl,p);
         else
            *lpl = 0;
         dir_freefullname(pdi,p);
      }
      pfd = compitem_getrightfile(ci);
      *lpr = 0;
      if(pfd)
      {
         pdi = file_getdiritem(pfd);
         p   = dir_getfullname(pdi);
         if(p)
            strcpy(lpr,p);
         else
            *lpr = 0;
         dir_freefullname(pdi,p);
      }
   }

   if( ci && lptstr )
   {
      //LPTSTR lpfn = compitem_getfilename(ci, CI_LEFT);
      *lpb = 0;
      if(*lpl)
      {
         strcpy(lpb, lpl);
         //compitem_freefilename(ci, CI_LEFT, lpfn);
      }

      if(lptstr)
      {
         if(*lptstr)
            strcat(lptstr, MEOR);
         strcat(lptstr, &gszLf[0]);
      }
      if( *lpb )
      {
         if( dir_isvalidfile2( lpb, pfd1 ) )
         {
            dwi |= TT_LEFT;
            // TIME TROUBLE - This is NOT sufficient
            //b1 = FileTimeToSystemTime( &pfd1->ftLastWriteTime, pst1 );
            b1 = FT2LST( &pfd1->ftLastWriteTime, pst1 );
            li1.LowPart  = pfd1->nFileSizeLow;
            li1.HighPart = pfd1->nFileSizeHigh;
            if(lptstr)
            {
               strcat(lptstr, GetI64StgRLen( &li1, MI64LEN ) );
               strcat(lptstr, " ");
               if(b1)
                  AppendDateTime( lptstr, pst1 );
               else
                  strcat( lptstr, "<FT2LST failed!>");
               sprintf( EndBuf(lptstr), MEOR"[%s]", lpb );
            }
         }
         else if(lptstr)
         {
            sprintf( EndBuf(lptstr), "(FILE INVALID!)"MEOR
               "[%s]",
               lpb );
         }
      }
      else if(lptstr)
      {
         strcat(lptstr, " - ***NONE***");
      }


      //lpfn = compitem_getfilename(ci, CI_RIGHT);
      *lpb = 0;
      if(*lpr)
      {
         strcpy(lpb, lpr);
         //compitem_freefilename(ci, CI_RIGHT, lpfn);
      }
      if(lptstr)
      {
         if(*lptstr)
            strcat(lptstr, MEOR);
         strcat(lptstr, &gszRt[0]);
      }
      if(*lpb)
      {
         if( dir_isvalidfile2( lpb, pfd2 ) )
         {
            dwi |= TT_RIGHT;
            // TIME TROUBLE - This is NOT sufficient
            //b2 = FileTimeToSystemTime( &pfd2->ftLastWriteTime, pst2 );
            b2 = FT2LST( &pfd2->ftLastWriteTime, pst2 );
            li2.LowPart  = pfd2->nFileSizeLow;
            li2.HighPart = pfd2->nFileSizeHigh;
            if(lptstr)
            {
               strcat(lptstr, GetI64StgRLen( &li2, MI64LEN ) );
               strcat(lptstr, " ");
               if(b2)
                  AppendDateTime( lptstr, pst2 );
               else
                  strcat( lptstr, "<FT2LST failed!>");
               sprintf( EndBuf(lptstr), MEOR"[%s]", lpb );
            }
         }
         else if(lptstr)
         {
            sprintf( EndBuf(lptstr), "(FILE INVALID!)"MEOR
               "[%s]", lpb );
         }
      }
      else if(lptstr)
      {
         strcat(lptstr, " - ***NONE***");
      }

      if( ( (dwi & TT_BOTH) == TT_BOTH ) &&
          ( b1 && b2  ) )
      {
         lg = CompareFileTime( &pfd1->ftLastWriteTime, &pfd2->ftLastWriteTime );
         if( lg == 0 )
         {
            if(lptstr)
               strcat( lptstr, MEOR"Same date and time");
         }
         else if( lg < 0 )
         {
            dwi |= TT_YOUNGER;
            if(lptstr)
               strcat( lptstr, MEOR"Left is EARLIER - CHECK!");
         }
         else
         {
            dwi |= TT_OLDER;
            if(lptstr)
               strcat( lptstr, MEOR"Left is newer - Update?");
         }
      }

      if( (dwi & TT_BOTH) == TT_BOTH )
      {
         if( li1.QuadPart == li2.QuadPart )
         {
            if(lptstr)
               strcat( lptstr, MEOR"Same Size");
         }
         else if( li1.QuadPart < li2.QuadPart )
         {
            dwi |= TT_SMALLER;
            if(lptstr)
               strcat( lptstr, MEOR"Left is smaller" );
         }
         else
         {
            dwi |= TT_OLDER;
            if(lptstr)
               strcat( lptstr, MEOR"Left is LARGER" );
         }
      }
   }

   return dwi;

}

// eof - dc4wTT.c
