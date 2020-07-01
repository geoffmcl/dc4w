// dc4wDel1.c
// Delete ONE left only file
// FIX20080728 - this module, new function IDM_DELETELEFTFILE added
#include "dc4w.h"
extern   INT   giSelection;    /* selected row in table*/
extern   BOOL  dir_filedelete( LPTSTR lpf ); /* delete using SHELL, actually move to Trash */

BOOL  CopyLeftFullFilePath( PTSTR pFile, COMPITEM ci )
{
   BOOL  done = FALSE;
   if(ci) {
      FILEDATA pfd = compitem_getleftfile(ci);
      DIRITEM  pdi;
      LPTSTR   p;
      if( pfd ) {
         pdi = file_getdiritem(pfd);
         p   = dir_getfullname(pdi);
         if(p) {
            strcpy(pFile, p);
            done = TRUE;
         }
         dir_freefullname(pdi,p);
      }
   }
   return done;
}

DWORD AddLeftFileText( LPTSTR lptstr, COMPITEM ci )
{
   static TCHAR _s_szlefttxt[264];
   DWORD             dwi = 0;
   PWIN32_FIND_DATA  pfd1;
   LARGE_INTEGER     li1;
   LPSYSTEMTIME      pst1;  // receives system time
   BOOL              b1;
   LPTSTR            lpb = _s_szlefttxt;
   LPTSTR            lpl = &g_szBuf1[0];

   pfd1 = &g_sFDL;
   pst1 = &g_sST1;
   b1 = FALSE;
   *lpl = 0;
   if(ci) {
      CopyLeftFullFilePath( lpl, ci );
   }
   if( ci && lptstr )
   {
      //LPTSTR lpfn = compitem_getfilename(ci, CI_LEFT);
      *lpb = 0;
      if(*lpl) {
         strcpy(lpb, lpl);
         //compitem_freefilename(ci, CI_LEFT, lpfn);
      }

      if(lptstr) {
         if(*lptstr)
            strcat(lptstr, MEOR);
         strcat(lptstr, "Left ");
      }
      if( *lpb ) {
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
   }
   return dwi;
}

BOOL Can_Delete_Left_File( VOID )
{
   int         failed = -1;
   COMPITEM    ci = view_getitem( current_view, giSelection );
   LPTSTR      lpl = &g_szBuf2[0];
   PWIN32_FIND_DATA  pfd1 = &g_sFDL;
   if(ci) {
      INT state = compitem_getstate(ci);
      if( state == STATE_FILELEFTONLY ) {
         DIRITEM diritem = file_getdiritem(compitem_getleftfile(ci));
         *lpl = 0;
         if(diritem &&
            CopyLeftFullFilePath( lpl, ci ) &&
            *lpl &&
            dir_isvalidfile2( lpl, pfd1 ) )
         {
            return TRUE;
         }
      }
   }
   return FALSE;
}

VOID  Do_IDM_DELETELEFTFILE( HWND hWnd )
{
   LPTSTR      pmsg  = &g_szMsg[0];
   INT_PTR     i     = -1;
   int         failed = -1;
   COMPITEM    ci = view_getitem( current_view, giSelection );
   LPTSTR      lpl = &g_szBuf2[0];
   PWIN32_FIND_DATA  pfd1 = &g_sFDL;
   char * errmsg = "Deletion FAILED!"MEOR
      "No selection made";
   if(ci) {
      INT state = compitem_getstate(ci);
      if( state == STATE_FILELEFTONLY ) {
         DIRITEM diritem = file_getdiritem(compitem_getleftfile(ci));
         *lpl = 0;
         if(diritem &&
            CopyLeftFullFilePath( lpl, ci ) &&
            *lpl &&
            dir_isvalidfile2( lpl, pfd1 ) ) {
            // get a short FILE name
            char * cp = dir_getnameptr(diritem);
            sprintf(pmsg, "Delete LEFT ONLY File"MEOR
               "[%s]"MEOR
               MEOR
               "WARNING: This permanently removes this file!", cp);
            AddLeftFileText( pmsg, ci );
            strcat(pmsg, MEOR
               MEOR"Click OK to proceed to delete.");
            i = MB_DELETE(NULL,pmsg,
                        "*** VERIFY LEFT FILE DELETE ***",
                        (MB_ICONINFORMATION|MB_OKCANCEL) );
             if( i == IDOK )
             {
                sprintf( pmsg, "Confirm DELETION of %s?", cp );
                i = MB(NULL, pmsg, "DELETION CONFIRMATION",
                   MB_OKCANCEL | MB_ICONSTOP );
                if( i == IDOK )
                {
                   // DO THE DELETION NOW
                   // *******************
                   if( dir_filedelete( lpl ) ) {
                      sprintf(pmsg, "Done deletion of file"MEOR
                         "[%s]"MEOR
                         "Actually, moved to the Trash Can."MEOR
                         "On OK, will refresh display.", lpl );
                      MB(NULL, pmsg, "DELETION RESULTS",
                      MB_OK | MB_ICONINFORMATION );
                      failed = 0;
                   } else {
                     errmsg = "Deletion FAILED!"MEOR
                        "Shell function failed to delete file!";
                   }
                } else {
                  errmsg = "Deletion FAILED!"MEOR
                     "Ok NOT clicked on 2nd confirmation!";
                }
             } else {
               errmsg = "Deletion FAILED!"MEOR
                  "Ok NOT clicked!";
             }
         } else {
            if( !diritem ) {
               errmsg = "Deletion FAILED!"MEOR
                  "Could NOT obtain left directory item!";
            } else if( *lpl == 0 ) {
               errmsg = "Deletion FAILED!"MEOR
                  "Could NOT obtain full path to left item!";
            } else if( !dir_isvalidfile2( lpl, pfd1 ) ) {
               errmsg = "Deletion FAILED!"MEOR
                  "Does NOT appear to be a valid file!";
            } else {
               errmsg = "Deletion FAILED!"MEOR
                  "Does NOT appear to be a valid left file!";
            }
         }
      } else {
         errmsg = "Deletion FAILED!"MEOR
            "Not LEFT ONLY file ...";
      }
   }
   if(failed == -1) {
      i = MB(NULL, errmsg, "DELETION RESULTS",
             MB_OK | MB_ICONINFORMATION );
   } else {
      PostMessage( hwndClient, WM_COMMAND, IDM_REFRESH, 0 );
   }
}

// eof dc4wDel1.c

