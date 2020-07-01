

// dc4wLog.c
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"
//#include  "dc4wLog.h"

// DIAGNOSTIC OUTPUT
TCHAR    gszDefTmp[] = "TEMPDC4W.TXT";
HANDLE   ghDiag = 0;
TCHAR    gszTmpOut[264];

//   if( VFH(ghDiag) )
// FIX20010522 - Always use RUNTIME
// but remove DEBUG if !NDEBUG
VOID  OpenDiagFile( VOID )
{
   LPTSTR   lpf = gszTmpOut;
   //GetModulePath( lpf );
   GetAppData(lpf);
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
//// #ifdef   NDEBUG 20061127 - Why ONLY with NO DEBUG???
   // if NO DEBUG, offer to EXIT APPLICATION IMMEDIATELY
   // **************************************************
   if(( lpb[0] == 'C' ) && ( lpb[1] == ':' ) && ( lpb[2] == 'E' ))
   {
      // *** CRITICAL ERROR - USUALLY MEMORY FAILURE ***
      strcat(lpb, MEOR"CRITICAL ERROR: Can ONLY EXIT!");
      MB( hwndClient, lpb, "CRITICAL ERROR",
         MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL );
      exit( -1 );
   } else {
      // some type of WARNING: Maybe application can CONTINUE ...
      strcat(lpb, MEOR"Exit application?");
      i = MB( hwndClient, lpb, "CHECK ME PLEASE",
         MB_YESNO | MB_ICONSTOP | MB_SYSTEMMODAL );
      if( i == IDYES )
         exit( -1 );
   }
   // **************************************************
//// #endif   // NDEBUG
   return i;
}

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
            chkme( "C:ERROR: MEMORY FAILED for OUTLST structure!"MEOR
               "WILL EXIT APPLICATION!" );
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

// eof - dc4wLog.c
