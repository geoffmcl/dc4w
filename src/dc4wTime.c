

// dc4wTime.c
// this is public domain software - praise me, if ok, just don't blame me!
#include	"dc4w.h"
#include <time.h>

extern   INT g_iThrdRun;   // incremented when thread to process argument has begun
extern   DWORD setToolText( int row );
extern   BOOL  gbDnInit; // entered into message loop
extern   VOID  DoSBSecond( HWND hWnd );

BOOL  g_bNeedPaint = FALSE;  // set when NOTHING to display - paint coloured screen
BOOL  g_bColourCycle = FALSE;   // advise timer to put up colour display

DWORD g_dwTocks = 0;       // running counter
BOOL  g_bOneShot = FALSE;  // posted IDM_DIR to bring up dialog box


#ifdef   ADDTIMER
DWORD g_dwTicks = 0;
DWORD g_dwSecs  = 0;
DWORD g_dwMins  = 0;
DWORD g_dwHours = 0;
DWORD g_dwDays  = 0;

VOID  Do_Each_Day( HWND hWnd )
{
   g_dwDays++;
}

VOID  Do_Each_Hour( HWND hWnd )
{
   g_dwHours++;
   if( g_dwHours > 24 )
   {
      Do_Each_Day(hWnd);
      g_dwHours = 0;
   }
}

VOID  Do_Each_Min( HWND hWnd )
{
   g_dwMins++;
   if( g_dwMins > 60 )
   {
      Do_Each_Hour(hWnd);
      g_dwMins = 0;
   }

#ifdef   ADDSTATUS
   SBTimer( &g_sSB );   // paint time
#endif   // #ifdef   ADDSTATUS

}


VOID  Do_Each_15Secs( HWND hWnd )
{
   if( g_bColourCycle ) // advise timer to put up colour display, and cycle thru colours
   {
      Paint_Warning( 0, 1 );  // cycle colours
   }
}

DWORD g_dwSecsTot = 0;
VOID  FireOneShot( HWND hWnd )
{
                 g_bOneShot = TRUE;
                 sprtf( "Posting an IDM_DIR to %x"MEOR, hWnd );
                 PostMessage( hWnd, WM_COMMAND, (WPARAM) IDM_DIR, 0 );
                 //sprtf( "Would post an IDM_DIR to %x"MEOR, g_hWnd );
}

VOID  Do_Each_Sec( HWND hWnd )
{
#ifdef   ADDTMDTXT
   DoSBSecond( hWnd );
#endif   // ADDTMDTXT
   // if an error in the command processor put up a dialog
   if( g_bAskCont )  // set for timer to find by the command processor
   {
      int retval;
      LPTSTR lpb = g_lpAskMsg;
      g_bAskCont = FALSE;
      if(lpb)
      {
         g_lpAskMsg = 0;

         retval = MB(hWnd,
            lpb,
            APPNAME,
            (MB_ICONSTOP|MB_OKCANCEL|MB_SYSTEMMODAL|MB_DEFBUTTON2));

         MFREE(lpb);

         if(( retval == IDCANCEL ) ||
            (  retval == (int)-1  ) )
         {
            //exit(1);
            // or perhaps
            SendMessage( hWnd, WM_COMMAND, (WPARAM) IDM_EXIT, 0 );
         }
      }
   }

//   if( ( g_dwSecsTot % 15 ) == 0 )
//   if( ( g_dwSecsTot % 5 ) == 0 )
   {
      Do_Each_15Secs( hWnd );
   }

   g_dwSecsTot++; // total seconds - very approximate

   // now increment the rough SECOND counter
   g_dwSecs++;
   if( g_dwSecs > 60 )
   {
      Do_Each_Min(hWnd);
      g_dwSecs = 0;
   }

   // only a rough estimate - timer messages can be delayed, and skipped ...
   if( g_dwSecsTot == 1 )
   {
      if( gbDnInit )
      {
         sprtf( "First one-second shot fired by %x"MEOR, hWnd );
         if(( g_iThrdRun == 0      ) && // incremented when thread to process argument has begun
            ( current_view == NULL ) )
         {
              // we have nothing to display - this is uninteresting as hell
              // just a white display with no help!!!
              // ******** put up a DIALOG and ask ******** politely which
              // but this check MUST wait until the 'scanning' thread has
              // run - maybe View will NOT be NULL
   //   gbDnInit = TRUE; // into message loop
              if( !g_bOneShot )
              {
                 FireOneShot( hWnd );
              }
         }

         if(ghMux)
            ReleaseMutex( ghMux );  //  HANDLE hMutex   // handle to mutex

      }
   }
   else
   {
      // each other second
      if(( gbDnInit ) &&
         ( g_iThrdRun == 0 ) &&
         ( current_view == NULL ) &&
         ( !g_bOneShot ) )
      {
                 FireOneShot( hWnd );
      }
   }


   // other per second checks to be added

}  // end - VOID  Do_Each_Sec( HWND hWnd )


   static BOOL _s_binexp = FALSE;
   static time_t tb;
   static time_t dsec;  // have already 'actioned' this SECOND

// DWORD g_dwTocks = 0;
// BOOL  g_bOneShot = FALSE;  // posted IDM_DIR to bring up dialog box
//   case WM_TIMER:
LRESULT	Do_WM_TIMER( HWND hWnd )
{
   time_t ct = time(NULL); // second since 'dot' - running seconds
   if( g_dwTocks == 0 )
   {
      sprtf( "First WM_TIMER being processed in %x"MEOR, hWnd );
#ifdef   ADDSTATUS
      SBTimer( &g_sSB );   // paint time
      SBSetDefault();
#endif   //#ifdef   ADDSTATUS
   }
   g_dwTicks += TIMER_TIC;
   while( g_dwTicks > 1000 )
   {
      g_dwTicks -= 1000;
      Do_Each_Sec( hWnd );
   }

   g_dwTocks++;

   if( g_bNeedPaint )     // set when NOTHING to display - paint coloured screen
   {
      g_bNeedPaint = FALSE;
      if( g_bColourCycle ) // advise timer to put up colour display, and cycle thru colours
         Paint_Warning( 0, 1 );
      else
         Paint_Warning( 0, 0 );

   }

   // timer tick view of the changing between outline mode to expanded mode
   if( view_isexpanded( current_view ) )
   {
      if( _s_binexp )
      {
         if( (ct - tb) > 10 )
         {
            if( ( (ct - tb) % 10 ) == 0 )
            {
               if( dsec != ct )
               {
                  dsec = ct;  // done this particular second
                  // sprtf( "In expanded mode for %d seconds."MEOR, (ct - tb) );
                  setToolText( -1 );
               }
            }
         }
      }
      else
      {
         tb = ct; // = time(NULL);  // start second counter
         _s_binexp = TRUE;
      }
   }
   else
   {
      if( _s_binexp )
      {
         if(ct > tb)
            sprtf( "In expanded mode for %d seconds."MEOR, (ct - tb) );
         _s_binexp = FALSE;
         setToolText( -2 );
      }
   }


   return TRUE;
}	// break;

#endif   // ADDTIMER

// eof - dcw4Time.c
