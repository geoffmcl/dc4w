

// TCLog.cpp - was dc4wLog.c

//#include	"dc4w.h"
//#include  "dc4wLog.h"
#include "stdafx.h"

// DIAGNOSTIC OUTPUT
TCHAR    gszDefTmp[] = "TEMPTC1.TXT";
HANDLE   ghDiag = 0;
TCHAR    gszTmpOut[264];
TCHAR    gszTmpBuf[1024];

DWORD   OutStg( HANDLE fh, LPTSTR lps )
{
   DWORD   i, j;
   j = 0;
   if( i = strlen(lps) )
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


//   if( VFH(ghDiag) )
// FIX20010522 - Always use RUNTIME
// but remove DEBUG if !NDEBUG
VOID  OpenDiagFile( VOID )
{
   LPTSTR   lpf = gszTmpOut;
   GetModulePath( lpf );
   strcat(lpf, gszDefTmp);
   ghDiag = OpnFil( lpf, FALSE );
}

VOID  CloseDiagFile( VOID )
{
   if( VFH(ghDiag) )
      CloseHandle(ghDiag);
   ghDiag = 0;
}


VOID  prt( LPTSTR lps )
{
   if( ghDiag == 0 )
      OpenDiagFile();
   if( VFH(ghDiag) )
   {
      int   i = OutStg( ghDiag, lps );
      if( i == (int)-1 )
      {
         CloseDiagFile();
         ghDiag = INVALID_HANDLE_VALUE;
      }
   }
}

int   _cdecl sprtf( LPTSTR lpf, ... )
{
   static TCHAR _s_sprtfbuf[1024];
   LPTSTR   lpb = &_s_sprtfbuf[0];
   int   i;
   va_list arglist;
   va_start(arglist, lpf);
   i = vsprintf( lpb, lpf, arglist );
   va_end(arglist);
   prt(lpb);
   return i;
}


int   _cdecl chkme( LPTSTR lpf, ... )
{
   static TCHAR _s_chkmebuf[1024];
   LPTSTR   lpb = &_s_chkmebuf[0];
   int   i;
   va_list arglist;
   va_start(arglist, lpf);
   i = vsprintf( lpb, lpf, arglist );
   va_end(arglist);
   sprtf(lpb);
#ifdef   NDEBUG
   // if NO DEBUG, offer to EXIT APPLICATION IMMEDIATELY
   // **************************************************
   strcat(lpb, MEOR"Exit application?");
   i = MB( hwndClient, lpb, "CHECK ME PLEASE",
      MB_YESNO | MB_ICONSTOP | MB_SYSTEMMODAL );
   if( i == IDYES )
      exit( -1 );
   // **************************************************
#endif   // NDEBUG
   return i;
}

#ifdef   ADDOUTLIST2
// ===========================
LIST_ENTRY  g_sOutList = { &g_sOutList, &g_sOutList };

typedef  struct tagOUTLST {
   LIST_ENTRY  pList;
   HANDLE      hOut;
   TCHAR       szFile[264];
}OUTLST, * POUTLST;

VOID  Out2File( LPTSTR lpf, LPTSTR lpb )
{
   PLE      ph = &g_sOutList;
   PLE      pn;
   POUTLST  pol, pola;
   LPTSTR   lpof;

   pola = 0;
   Traverse_List(ph, pn)
   {
      pol = (POUTLST)pn;
      lpof = &pol->szFile[0];
      if( strcmpi( lpof, lpf ) == 0 )
      {
         pola = pol;    // keep pointer
         break;
      }
   }

   if(lpb)
   {
      if(pola)    // if already in LIST
      {
            if( VFH( pola->hOut ) )
            {
               DWORD dww;
               DWORD dwl = strlen(lpb);
               if(( !WriteFile( pola->hOut, lpb, dwl, &dww, NULL ) ) ||
                  ( dww != dwl ) )
               {
                  CloseHandle( pola->hOut );
                  pola->hOut = INVALID_HANDLE_VALUE;
                  MFREE(pn);
               }
            }
      }
      else
      {
         // NOT in LIST
         pn = MALLOC( sizeof(OUTLST) );
         if(pn)
         {
            pola = (POUTLST)pn;
            strcpy( &pola->szFile[0], lpf ); // fill in FILE name
            pola->hOut = CreateFile( lpf, // file name
               GENERIC_READ | GENERIC_WRITE,                      // access mode
               0,                          // share mode
               0, // SD
               CREATE_ALWAYS,                // how to create
               FILE_ATTRIBUTE_NORMAL,                 // file attributes
               0 );        // handle to template file
            if( VFH( pola->hOut ) )
            {
               DWORD dww;
               DWORD dwl = strlen(lpb);
               if(( !WriteFile( pola->hOut, lpb, dwl, &dww, NULL ) ) ||
                  ( dww != dwl ) )
               {
                  CloseHandle( pola->hOut );
                  pola->hOut = INVALID_HANDLE_VALUE;
                  MFREE(pn);
               }
               else
               {
                  InsertTailList(ph,pn);
               }
            }
            else
            {
               chkme( "ERROR: Unable to create [%s] file!"MEOR, lpf );
            }
         }
         else
         {
            chkme( "MEMORY FAILED for OUTLST structure!"MEOR );
            exit(-1);
         }
      }
   }
   else  // no buffer pointer = CLOSE this OUT
   {
      if(pola)    // if already in LIST
      {
            if( VFH( pola->hOut ) )
            {
               CloseHandle( pola->hOut );
            }
            pola->hOut = 0;
            RemoveEntryList(pn); // remove this entry
            MFREE(pn);  // and toos the memory
      }
   }
}

VOID  KillOutList( VOID )
{
   PLE      ph = &g_sOutList;
   PLE      pn;
   POUTLST  pol;

   // CLOSE any open files
   Traverse_List(ph, pn)
   {
      pol = (POUTLST)pn;
      if( VFH(pol->hOut) )
         CloseHandle( pol->hOut );
   }
   // and TOSS the memory
   KillLList( &g_sOutList );
}

VOID  InitOutList( VOID )
{
   InitLList( &g_sOutList );
}

#endif   // #ifdef   ADDOUTLIST2

// eof - TCLog.cpp
