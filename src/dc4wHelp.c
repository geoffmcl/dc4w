

// dc4wHelp.c
// this is public domain software - praise me, if ok, just don't blame me!
#include "dc4w.h"
#include "dc4wHelp.h"

extern   PVOID Add2StgList( PLE pH, LPTSTR lpb );
extern   PLE  GetSaveOpts( INT chr );

#define  CHKMEM(a)   if( !a ) {\
   MessageBox( NULL, "ERROR: MEMORY FAILED! ABORTING on OK!", "MEMORY ERROR", MB_OK|MB_ICONEXCLAMATION );\
   exit(-1); \
}

// see { szOpt, szIgnDT,  it_Bool,(LPTSTR)&gbIgnDT,         &gbChgIDT, 0, 0 }
// BOOL  g_bWriteAZip = FALSE;   // to actually WRITE zip = NOT PRESENTLY USED
// -N
// FIX20051222 - Reverse logic on ZIPPING - presently defaults ON, and -N to stop
// So, keep -n to say NO ZIP, but -n+ or -na (all) will cause a ZIP to be written
// BOOL  g_bNoAZip = FALSE;
BOOL  g_bNoAZip = TRUE; // This is the NEW default
BOOL  g_bZipAll = FALSE;   // -NA to override the exclusions
/***************************************************************************
 * Function: dc4w_usage
 *
 * Purpose:
 *
 * Complain to command line users about poor syntax,
 * will ONE DAY be replaced by proper help file.
 */
static TCHAR sszUse3[] = "HELP ON SAVE COMMAND"MEOR;
static TCHAR sszUse1[] = "COMMAND LINE ERROR"MEOR;
//         case 'C':   // case flag
//         case 'D':   // write a DIFFERENCE file
//         case 'E':   // -E no expand if only 1 in wd_initial
//         case 'I': // include file names FIX20051204
//         case 'L':  // shallow - no recursive?
//         case 'N':  // no zip
//         case 'R':    // Recursive
//         case 'S':   // Save a FILE LIST
//         case 'T': // ignore date/time
//         case 'X':   // EXCLUDE D=Directories F=Files, or -X+11 to exclude List11 [Exclude]
//         case 'Z':   // one of the COMPARES is a ZIP file
static TCHAR sszUse2[] = "Brief usage is :-"MEOR
   APPNAME" LeftDir|File [RightDir|File]"MEOR
   "[-r[-|+]]=Recursive [-L] = Shallow [-N=No A.Zip]"MEOR
   "[-s[slrd][:outfile]] = Save File List"MEOR
   "[-d[slrd][:outfile]] = Write Difference File"MEOR
   "-Ifilename, or mask, to add input file names"MEOR
   "note: if outfile given file written and app exits"MEOR
   "[-x[-|+[nn]]] = Disable/Enable exclude list [num]"MEOR
   "[-xd:<[@]dirs>|-xf:<[@]files>] = Specific exclude items."MEOR
#ifdef ADD_X_CVS // FIX20090509 - Add #ifdef ADD_X_CVS to add -xd[fp|cvs|svn|git|all]
   "[-xdfp] [-xdcvs] [-xdsvn] [-xdgit] [-xdall]"MEOR
   " Exclude FrontPage, CVS, SVN, git, or all these (hidden) folders."MEOR
#else
   "[-xdfp] = Exclude FrontPage (hidden) folders."MEOR
#endif
   "-zzipfile.zip = Indicates it is a ZIP file";


int  DoMB( LPTSTR lpb )
{
   INT retval = MB(NULL,
         lpb,
         APPNAME,
         (MB_ICONSTOP|MB_OKCANCEL|MB_SYSTEMMODAL|MB_DEFBUTTON2));
      if( ( retval == IDCANCEL ) ||
         (  retval == (int)-1  ) )
      {
         return 1; // FIX20130808
         // exit(1); // no need to forec an exit
      }
      return 0; // to continue // FIX20130808
}

int dc4w_usage_save(LPTSTR pcmd)
{
    int iret = 0;
   LPTSTR   p;
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpe;
   PLE      ph,pn;
   DWORD    dwi,dwc;
   LPTSTR   cp;

   p = strchr(pcmd,'?');
   strcpy( lpb, sszUse1 );
   if(p)
      strcpy( lpb, sszUse3 );

   ph = GetSaveOpts( 0 );
   if(ph)
   {
      ListCount2(ph,&dwc);
      if(dwc)
      {
         dwi = 0;
         Traverse_List( ph, pn )
         {
            dwi++;   // space between
            cp = (LPTSTR)((PLE)pn + 1);
            dwi += strlen(cp);
         }
         if(dwi)
         {
            sprintf(EndBuf(lpb),MEOR"Cmd=%s"MEOR, pcmd );
            lpe = (LPTSTR)MALLOC( (strlen(lpb) + dwi + 256 + (dwc * 3) ) );
            if(lpe)
            {
               DWORD dwlen;
               strcpy(lpe, lpb);
               dwi = 0;
               dwlen = 0;
               Traverse_List( ph, pn )
               {
                  if(dwi)
                     strcat(lpe," ");
                  dwi++;   // space between
                  cp = (LPTSTR)((PLE)pn + 1);
                  //dwi += strlen(cp);
                  strcat(lpe,cp);
                  dwlen += strlen(cp) + 1;
                  if(dwlen > 65)
                  {
                     strcat(lpe,MEOR);
                     dwlen = 0;
                  }
               }
               if(dwlen)
                  strcat(lpe,MEOR);

               strcat(lpe, "Click OK to continue."MEOR
               "Click CANCEL to exit application" );

               iret = DoMB( lpe );

               MFREE( lpe );

            }
         }
      }
   }
   return iret;
}

VOID add_useage( PTSTR lpb, PTSTR msg )
{
   if( msg && *msg )
      strcat( lpb, msg );
   else
      strcat( lpb, "General Error"MEOR );
   EnsureCrLf(lpb);
   strcat( lpb, sszUse2 );
   EnsureCrLf(lpb);
   if(strlen(g_szCmd) &&
      ((strlen(lpb) + strlen(g_szCmd)) < (1024 - 60)) ) {
         strcat(lpb,"CMD:[");
         strcat(lpb,g_szCmd);
         strcat(lpb,"]"MEOR);
   }
}

// FIX20060917 - add Alt+? brief help dialog - show_help()
// and also from 'Brief Help' button IDC_BUTTON1 on ABOUT dialog
VOID  show_help( VOID )
{
   PTSTR lpb = &gszTmpBuf[0];
   *lpb = 0;
   add_useage( lpb, "BRIEF HELP MESSAGE" );
   MB(NULL, lpb, APPNAME, (MB_ICONINFORMATION | MB_OK) );
}

int dc4w_usage(LPTSTR msg)
{
    int iret = 0;
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpe;
   strcpy( lpb, sszUse1 );
   add_useage( lpb, msg );
   strcat( lpb, "Click OK to continue."MEOR
      "Click CANCEL to exit application" );
   //if (msg==NULL)
   //   msg = LoadRcString(IDS_USAGE_STR);
   //LoadString( g_hInst, IDS_DC4W_USAGE, szBuf, sizeof(szBuf));
   // MessageBox()
   lpe = 0;
   // FIX20130808 // 20130808 - always put message box on first command error
   //if( !g_lpAskMsg )
   //   lpe = (LPTSTR)MALLOC( (strlen(lpb) + 1));
   //if(lpe)
   //{
   //   strcpy(lpe,lpb);
   //   g_bAskCont = TRUE;   // set for timer to find
   //   g_lpAskMsg = lpe;
   //   g_bCmdError++;       // bump the COMMAND line errors
   //}
   //else
   {
      strcat(lpb,MEOR);
      strcat(lpb,"Command error!");
      iret = DoMB(lpb);
   }
   return iret;
}

// helper services
INT   GetList2( PLE pHead, PTSTR cp )
{
   INT   c, d, k, l;
   LPTSTR   lpb = &gszTmpBuf[0];
   c = *cp;
   d = 0;
   k = 0;
   l = 0;
   while(c)
   {
      if( c == '"' )    // if a QUOTE
      {
         if( k && ( d == '"' ) )
         {
            lpb[k] = 0;
            if( Add2StgList( pHead, lpb ) )
               l++;
            else
            {
               l = 0;
               k = 0;
               break;
            }
            k = 0;
         }
         else if( ( k == 0 ) && ( d == 0 ) )
         {
            // start of quotes
            d = c;
         }
         else
         {
               l = 0;
               k = 0;
               break;
         }
      }
      else if( c == ';' )  // if a SEMI-COLON
      {
         if(k)
         {
            lpb[k] = 0;
            if( Add2StgList( pHead, lpb ) )
               l++;
            else
            {
               l = 0;
               k = 0;
               break;
            }
         }
         k = 0;
      }
      else
      {
         lpb[k++] = (TCHAR)c;
      }

      cp++;    // bump to next
      c = *cp; // and get char
   }
   if(k)
   {
      lpb[k] = 0;
      if( Add2StgList( pHead, lpb ) )
         l++;
      else
         l = 0;
   }

   if(l)
      return 0;   // SUCCESS
   else
      return 1;   // FAILED
}

// Switch -xf:<file[s]> or -xd:<dir[s]>
// or -xf:@input.file
INT   GetList( PLE pHead, PTSTR cp )
{
   INT iret = 0;
   INT c = *cp; // get FIRST char after -fd: or
   HANDLE hFile;
   if( c != '@' ) {
      return GetList2( pHead, cp );
   }
   // else, we have an INPUT file list
   hFile = CreateFile( &cp[1],
            GENERIC_READ,
            FILE_SHARE_READ,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0 );
   if( VFH( hFile ) )
   {
      DWORD dwh = 0;
      DWORD dws = GetFileSize( hFile, &dwh );
      if( dwh ) { // File is TOO BIG
         CloseHandle(hFile);
         return 1;
      }
      if(dws) {
         HANDLE map = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
         if( VFH(map) ) {
            PVOID view = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
            if(view) {
               DWORD i, j;
               PTSTR pb = (PSTR)view;
               PTSTR lpb = &gszTmpBuf[0];
               TCHAR c;
               j = 0;
               for( i = 0; i < dws; i++ )
               {
                  c = pb[i];
                  if( c >= ' ' ) {
                     if( c == ' ' ) {
                        if(j) { // only if NOT first
                           lpb[j++] = c;
                        }
                     } else {
                        lpb[j++] = c;
                     }
                  } else {
                     if(j) {
                        lpb[j] = 0;
                        while(j--) { // clear trailing SPACES
                           if( lpb[j] > ' ' ) {
                              break;
                           }
                           lpb[j] = 0;
                        }
                        if( *lpb == '"' ) { // remove quotes
                           if( lpb[j] == '"' ) {
                              lpb[j] = 0;
                           }
                           lpb++; // got to next char
                        }
                        if( !Add2StgList( pHead, lpb ) ) {
                           iret = 1;
                           break;
                        }
                        lpb = &gszTmpBuf[0]; // get buffer back
                        j = 0;
                     }
                  }
               }
               if(j) { // any added at end of file buffer
                  lpb[j] = 0;
                  while(j--) { // remove trailing spaces
                     if( lpb[j] > ' ' ) {
                        break;
                     }
                     lpb[j] = 0;
                  }
                  if( *lpb == '"' ) { // remove quotes
                     if( lpb[j] == '"' ) {
                        lpb[j] = 0;
                     }
                     lpb++;
                  }
                  if( !Add2StgList( pHead, lpb ) ) {
                     iret = 1;
                  }
               }
               UnmapViewOfFile(view);
               CloseHandle(map);
               CloseHandle(hFile);
            } else {
               CloseHandle(map);
               CloseHandle(hFile);
               iret = 1;
            }
         } else {
            CloseHandle(hFile);
            iret = 1;
         }
      } else {
         // quietly forget it
         CloseHandle(hFile);
      }
   } else {
      iret = 1; // FAILED
   }
   return iret;   // SUCCESS
}

//typedef struct tagPISTR {
//   HANDLE   hFile;
//   INT      iCnt;
//   LPTSTR   pBuf;
//   LPTSTR * pArgs;
//}PISTR, * PPISTR;
//#define  MXARGS   64    // should be ENOUGH

BOOL  Getpis( PPISTR ppis, LPTSTR lpf )
{
   BOOL     bRet = FALSE;
   LPTSTR   lptmp;

   ppis->iCnt  = 0;
   ppis->pBuf  = 0;
   ppis->pArgs = 0;
   // read this input file
   ppis->hFile = CreateFile( lpf,
            GENERIC_READ,
            FILE_SHARE_READ,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0 );
   if( VFH( ppis->hFile ) )
   {
      DWORD dws, dwh;
      dws = GetFileSize( ppis->hFile, &dwh );
      if( dws && !dwh )
      {
         LPTSTR   lpb;
         dwh = dws + (MXARGS * sizeof(LPVOID));
         if( lpb = LocalAlloc(LPTR,dwh) )
         {
            ppis->pBuf = lpb; // this is for freeing the pointer
            if( ( ReadFile( ppis->hFile, lpb, dws, &dwh, 0) ) &&
                ( dws == dwh ) )
            {
               LPTSTR * lpv;
               int      icnt;
               TCHAR    c, d;
               DWORD    dwi;

               lpv = (LPTSTR *) (LPTSTR) ( lpb + dws + 2 );
               icnt = 0;
               lpv[icnt++] = "dummy";
               // =======================================
               for( dwh = 0; dwh < dws; dwh++ )
               {
                  c = lpb[dwh];
                  if( c > ' ' )
                  {
                     if( c == ';' )
                     {
                        dwh++;
                        for( ; dwh < dws; dwh++ )
                        {
                           if( lpb[dwh] < ' ' )
                              break;
                        }
                     }
                     else
                     {
                        // begin of an ARGUMENT
                        lptmp = &lpb[dwh];
                        lpv[icnt] = lptmp; // &lpb[dwh];
                        d = *lptmp;    // get first char
                        c = d;
                        dwh++;
                        for( ; dwh < dws; dwh++ )
                        {
                           c = lpb[dwh];
                           if( c < ' ' )
                              break;
                           else if( ( d != '"' ) && // not "protect" by DOUBLE QUOTES
                              ( c == ';' ) ) // this is a comment tailing a command
                              break;
                        }
                        lpb[dwh] = 0;  // zero terminate the string
                        dwi = dwh - 1; // remove any trailing space
                        while(dwi)
                        {
                           if( lpb[dwi] > ' ' )
                              break;
                           lpb[dwi--] = 0;
                        }
                        // FIX20010329 - Ensure if the argument is "encased"
                        // in double quotes, remove them NOW (like command)
                        dwi = strlen(lptmp);    // get length of the command
                        if( ( dwi > 2      ) &&
                           ( d == '"' ) &&
                           ( lptmp[dwi-1] == '"' ) )
                        {
                           dwi--;
                           lptmp[dwi] = 0;   // kill trailing DOUBLE QUOTE
                           dwi--;
                           lptmp++;          // bump past leading DOUBLE QUOTE
                           lpv[icnt] = lptmp; // set this new pointer
                        }
                        if( dwi )
                        {
                           icnt++;  // bump the argument count
                        }
                        if( c >= ' ' )
                        {
                           // ensure we go to the end of the line
                           dwh++;   // get past the ZERO - FIX20020324
                           for( ; dwh < dws; dwh++ )
                           {
                              c = lpb[dwh];
                              if( c < ' ' )
                                 break;
                           }
                        }
                     }
                  }
               }  // for the length of the read in buffer
               // =======================================
               if( icnt > 1 )
               {
                  ppis->iCnt = icnt;
                  ppis->pArgs = lpv;
               }
               // note we quietly forget BLANK input files!!!
               bRet = TRUE;
            }
         }
      }
   }

   return bRet;
}


BOOL  ProcessInput( PTHREADARGS ta, LPTSTR lpf )
{
   BOOL     bRet = FALSE;  // assume FAIL for the moment
   PISTR    pis;
   PPISTR   ppis = &pis;

   ZeroMemory( ppis, sizeof(PISTR) );

   if( Getpis( ppis, lpf ) )
   {
      if( ( ppis->iCnt > 1 ) && ( ppis->pArgs ) )
         if (ProcessArgs( ta, ppis->iCnt, ppis->pArgs ))
             return FALSE;
      // note we quietly forget BLANK input files!!!
      bRet = TRUE;
   }

   // clean up what needs to be cleaned
   if( ppis->pBuf )
      LocalFree(ppis->pBuf);

   if( VFH( ppis->hFile ) )
      CloseHandle( ppis->hFile );

   return bRet;
}

PTSTR pszFrontPage[] = {
   { "_derived" },
   { "_private" },
   { "_vti_cnf" },
   { "_vti_pvt" },
   { 0 }
};

PTSTR pszCVS[] = {
   { "CVS" },
   { 0 }
};

PTSTR pszSVN[] = {
   { ".svn" },
   { 0 }
};

PTSTR pszGIT[] = {
   { ".git" },
   { 0 }
};

// pointer to set of string pointers,
// to be added to the directory exclusion list
INT Add_Dir_List( PTSTR * ps )
{
   INT   c = 1;
   while(*ps) {
      PTSTR pc = *ps;
      c = GetList( &gsXDirsList, pc );
      if(c) break;
      ps++;
   }
   return c;
}

// FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS
// INT SubList( PLE pH, PTSTR lpb )
// opposite of
// PVOID Add2StgList( PLE pH, LPTSTR lpb ) {
// LPTSTR lps; INT i; PXLST pxl;
// i = (strlen(lpb) + sizeof(XLST));
// pxl = (PXLST)MALLOC(i);
INT SubList( PLE pH, PTSTR lpb )
{
    // find this entry in the list, remove it, and delete it
    PLE pN;
    PXLST pxl;
    PTSTR lps;
    Traverse_List( pH, pN )
    {
        pxl = (PXLST)pN;
        lps = &pxl->x_szStg[0];
        if( strcmpi( lps, lpb ) == 0 ) {
            // found in LIST
            RemoveEntryList(pN);    // extract entry
            MFREE(pN);              // and toss (delete) the memory
            return 0;   
        }
    }
    return 0;
}


INT Del_Dir_List(PTSTR * ps)
{
   INT   c = 1;
   while(*ps) {
      PTSTR pc = *ps;   // get directory name
      c = SubList( &gsXDirsList, pc );
      if(c) break;
      ps++;
   }
   return c;
}


INT Add_FP_Directories( VOID )
{
   PTSTR * ps = &pszFrontPage[0];
   return Add_Dir_List(ps);
}
INT Add_CVS_Directories( VOID )
{
   PTSTR * ps = &pszCVS[0];
   return Add_Dir_List(ps);
}
INT Add_SVN_Directories( VOID )
{
   PTSTR * ps = &pszSVN[0];
   return Add_Dir_List(ps);
}
INT Add_GIT_Directories( VOID )
{
   PTSTR * ps = &pszGIT[0];
   return Add_Dir_List(ps);
}

// reverse the process
// FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS toggle
INT Del_FP_Directories( VOID )
{
   PTSTR * ps = &pszFrontPage[0];
   return Del_Dir_List(ps);
}

INT Del_CVS_Directories( VOID )
{
   PTSTR * ps = &pszCVS[0];
   return Del_Dir_List(ps);
}

INT Del_SVN_Directories( VOID )
{
   PTSTR * ps = &pszSVN[0];
   return Del_Dir_List(ps);
}

INT Del_GIT_Directories( VOID )
{
   PTSTR * ps = &pszGIT[0];
   return Del_Dir_List(ps);
}

INT Add_X_ALL_repos(VOID)
{
    INT c = 0;
    c = Add_FP_Directories();
    c += Add_CVS_Directories();
    c += Add_SVN_Directories();
    c += Add_GIT_Directories();
    gbXAllRepos = TRUE; // marked ADDED XCLUDE ALL REPOS directories
    // =========================================================
    return c;
}

// FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS toggle
INT Del_X_ALL_repos(VOID)
{
    INT c = 0;
    c = Del_FP_Directories();
    c += Del_CVS_Directories();
    c += Del_SVN_Directories();
    c += Del_GIT_Directories();
    gbXAllRepos = FALSE; // marked REMOVED XCLUDE ALL REPOS directories
    // =========================================================
    return c;
}

// FIX20090811 - exclude repos dirs IDM_EXCLUDEREPOS
INT Do_IDM_EXCLUDEREPOS(VOID)
{
    if( gbXAllRepos )
        Del_X_ALL_repos();
    else 
        Add_X_ALL_repos();
    return 1;
}

// got -x<something>
// FIX20090509 - Add #ifdef ADD_X_CVS to add -xd[fp|cvs|svn|git|all]
INT Get_X_Options( PTSTR cp_in )
{
   PTSTR cp = cp_in;
   INT cmd, c;
   cmd = toupper(*cp);  // get after the -X
   if( (cmd == '-') || (cmd == '+') ) { // got MINUS or PLUS
      if( cmd == '-' ) {
         // -x- DISABLES EXCLUDES
         gbExclude = FALSE;   // disable GLOBAL EXCLUDE list
      } else {
         gbExclude = TRUE;
         cp++;
         if( *cp ) {
            if( !ISNUM(*cp) )
               goto Bad_XP;

            c = atoi( cp );   // get number
            if( c && ( c <= MXXLSTS ) )
               giCurXSel = c;
            else
               goto Bad_XP;
         }
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      }
      return 0;
   }
   cp++; // bump to next
   c = toupper(*cp); // must be a colon,
   cp++; // bump to next - first dir or file name (can be in " ")
   // or FIX20060823 -xdfp to EXCLUDE FrontPage (hidden) folders
   if( *cp ) {
      // FIX20090509 - Add #ifdef ADD_X_CVS to add -xd[fp|cvs|svn|git|all]
#ifdef ADD_X_CVS
      if( stricmp( cp_in, "dfp" ) == 0 ) {
         c = Add_FP_Directories();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else if( stricmp( cp_in, "dcvs" ) == 0 ) {
         c = Add_CVS_Directories();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else if( stricmp( cp_in, "dsvn" ) == 0 ) {
         c = Add_SVN_Directories();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else if( stricmp( cp_in, "dgit" ) == 0 ) {
         c = Add_GIT_Directories();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else if( stricmp( cp_in, "dall" ) == 0 ) {
         // c = Add_FP_Directories();c += Add_CVS_Directories();
         // c += Add_SVN_Directories(); c += Add_GIT_Directories();
         c = Add_X_ALL_repos();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else
#endif
      if (( cmd == 'D' ) && ( c == 'F' ) &&
         ( strlen(cp) == 1 ) && ( toupper(*cp) == 'P' ) ) {
         c = Add_FP_Directories();
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      } else if( ( *cp ) && ( c == ':' ) &&
         ( ( cmd == 'D' ) || ( cmd == 'F' ) ) ) {
         if( cmd == 'D' )
            c = GetList( &gsXDirsList, cp );
         else
            c = GetList( &gsXFileList, cp );
         ToggleBool( &bNoExcludes, &bChgNoExcl, FALSE ); // FIX20091125 - if -xdall, etc, OFF bNoExcludes
      }
   } else {
      c = '1';
   }
   return c; //  if( c )  // if an ERROR

Bad_XP:  // a BAD -x parameter
   if( !c ) {
      c = '2';
      ////dc4w_usage( "ERROR: Switch -x is -x-, -x+[nn], -xf:<file[s]> or -xd:<dir[s]>!" );
   }
   return c;
}

VOID Cleanpis( PPISTR ppis )
{
   if(ppis) {
      if( ppis->pBuf )
         LocalFree(ppis->pBuf);
      if( VFH( ppis->hFile ) )
         CloseHandle( ppis->hFile );
      ppis->pBuf = 0;
      ppis->hFile = 0;
   }
}

// FIX20071020 - allow LIST, rather than just ONE
// a list would be ';' separated,
// and a list in a file would start with a '@'
int AddStgs2List( PLE ph, PTSTR ps )
{
   int   iret = 0;
   size_t len = strlen(ps);
   if(len) {
      if( strchr(ps, ';') ) {
         // deal with ';' separated LIST
         PTSTR p, p1;
         PTSTR pb = MALLOC( len + 1 ); // allocate memory
         CHKMEM(pb); // check allocation
         p1 = pb;    // copy to mutable pointer
         strcpy(p1,ps); // copy in full string
         p = strchr(p1,';');  // first sirch for ';' separator
         while(p) {
            *p = 0;  // kill the spearator
            Add2StgList( ph, p1 );  // add ONE
            p++;  // get to next char
            p1 = p;  // update mutable pointer
            p = strchr(p1,';');  // and get next spearator
         }
         if(*p1)  // if some remains, as there should be ...
            Add2StgList( ph, p1 );  // do LAST
         MFREE(pb);  // free the MEMORY
      } else if( *ps == '@' ) {
         // deal with INPUT file, containing list
         PPISTR   ppis = MALLOC(sizeof(PISTR)); // allocate
         CHKMEM(ppis);  // check memory
         ps++; // get to FILE NAME
         ZeroMemory( ppis, sizeof(PISTR) ); // zero our work structure
         if( Getpis( ppis, ps ) ) { // allocate and load file
            if( ( ppis->iCnt > 1 ) && ( ppis->pArgs ) ) {
               int i = 1;  // start at arg 1
               for( ; i < ppis->iCnt; i++ ) {   // iterate through arguments
                  iret |= AddStgs2List( ph, ppis->pArgs[i] ); // adding each file
                  if(iret) // if error, give up iteration
                     break;
               }
            }
            // clean up what needs to be cleaned
            Cleanpis( ppis );
         } else {
            iret |= 1;  // flag an error - could NOT load file
         }
         MFREE(ppis);
      } else {
         Add2StgList( ph, ps );  // assumed just ONE
      }
   }
   // silent ignore a BLANK, or should I???!!!
   return iret;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : ProcessArgs
// Return type: VOID 
// Arguments  : PTHREADARGS ta
//            : int argc
//            : char * * argv
// Description: Physical decode of command arguments, with the results all
//              placed in the passed THREADARGS pointer.
// Arguments supported:
// @InputFile - Read and process an INPUT file, line by line, excluding ; comments
//
// FirstFile|Path - First file/path argument
//    if( ta->pFirst == NULL ) {
//            strcpy( ta->szFirst, argv[i] );
//            ta->pFirst = &ta->szFirst[0];
//
// SecondFile|Path - Second file/path argument
//    else if( ta->pSecond == NULL ) {
//            strcpy( ta->szSecond, argv[i] );
//            ta->pSecond = &ta->szSecond[0];
//
// -S[slrd][:outfile] - note: if outfile given file written and exit app.
//    where included are s = same, l = leftonly, r = rightonly, d = different
//    If NO INCLUDE then the DEFAULT is -
//               ta->saveopts = (INCLUDE_LEFTONLY) | (INCLUDE_ALLDIFF);
//
// -D[slrd][:outfile] - note: if outfile given file written and app exit
//    Write a DIFFERENCE file where included items are (lines that are)
//    s = same, l = leftonly, r = rightonly, d = different, m = moved
//    and other options -
//    n = line numbers, t = tags, a = append file, h = extra header info
//    If left ZERO the default is -
//    -dlrdmntah:<outfile>
//
// FIX20051222 - Reverse logic on ZIPPING - presently defaults ON, and -N to stop
// So, keep -n to say NO ZIP, but -n+ or -na (all) will cause a ZIP to be written// -N- = g_bNoAZip = TRUE;
// -N+ = g_bNoAZip = FALSE; // strange logic???
//
///////////////////////////////////////////////////////////////////////////////
int  ProcessArgs( PTHREADARGS ta, int argc, char * * argv )
{
    int         iret = 0;
   INT         i;
   LPTSTR      cp;
   INT         cmd, c;
   DWORD       dwo, len;
   BOOL        flg;

   for( i = 1; i < argc; i++ )
   {
      cp = argv[i];
      c = *cp;
      sprtf( "CMD: [%s]"MEOR, cp );
      if( g_szCmd[0] ) strcat(g_szCmd," ");
      strcat(g_szCmd, cp);
      /* is this an option ? */
      if( (c == '-') || (c == '/') )
      {
         cp++;
         cmd = toupper(*cp);
         c = cmd;
         switch( cmd )
         {
         case 'C':   // case flag
            cp++;
            flg = TRUE; // gbIgnCase = TRUE;
            if( *cp )
            {
               if(( *cp == '+' ) || ( *cp == '1' ) )
                  flg = TRUE; // gbIgnCase = TRUE;
               else if(( *cp == '-' ) || ( *cp == '0' ) )
                  flg = FALSE; // gbIgnCase = FALSE;
               else
               {
                  if(dc4w_usage(NULL))
                      return 1;
               }
            }
            ToggleBool( &gbIgnCase, &bChgIgC, flg );
            break;

         case 'D':   // write a DIFFERENCE file
            cp++;
            if( *cp != ':' )
            {
               while( *cp )
               {
                  c = toupper(*cp);
                  switch( c )
                  {
                  case 'S':
                     ta->diffopts |= INCLUDE_SAME;
                     break;
                  case 'L':
                     ta->diffopts |= INCLUDE_LEFTONLY;
                     break;
                  case 'R':
                     ta->diffopts |= INCLUDE_RIGHTONLY;
                     break;
                  case 'D':
//                     ta->diffopts |= INCLUDE_DIFFER;
                     ta->diffopts |= INCLUDE_ALLDIFF; // younger and older
                     break;
                  case 'M':
                     ta->diffopts |= INC_ALLMOVE;
                     break;
                  case 'N':
                     ta->diffopts |= INCLUDE_LINENUMS;
                     break;
                  case 'T':
                     ta->diffopts |= INCLUDE_TAGS;
                     break;
                  case 'A':
                     ta->diffopts |= APPEND_FILE;
                     break;
                  case 'H':
                     ta->diffopts |= INCLUDE_HEADER;
                     break;
                  default:
                     if (dc4w_usage(NULL))
                         return 1;
                  }

                  cp++;
                  if( *cp == ':' )
                     break;
               }
            }
            if( *cp == ':' )
            {
               cp++;
               if( *cp )
               {
                  ta->pDiffList = &ta->szDiffList[0];
                  strcpy( ta->pDiffList, cp );
               }
            }

            break;

         case 'E':   // -E no expand if only 1 in wd_initial
            // FIX20060216 - remove automatic expansion when only ONE difference!
            // g_bNoExp = TRUE; // set default to NO EXPANSION use -E+ to override
            g_bNoExp = TRUE;
            cp++;
            if( *cp == '+' )
               g_bNoExp = FALSE;
            else if( *cp == '-' )
               g_bNoExp = TRUE;
            break;

         case 'I':
            cp++;
            if( *cp ) {
               // FIX20060709 - separate input FILES and FOLDERS
               if( *cp == ':' )
               {
                  cp++;
                  cmd = toupper(*cp);
                  cp++;
                  len = strlen(cp);
                  if((len > 1) && (( cmd == 'D' ) || ( cmd == 'F' )) && ( *cp == ':' )) {
                     cp++;
                     if( cmd == 'D' ) {
                        // FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
                        if( AddStgs2List( &g_sInDirs, cp ) )
                           goto errant_I;
                     } else if( cmd == 'F' ) {
                        if( AddStgs2List( &g_sInFiles, cp ) )
                           goto errant_I;
                     }
                  } else {
                     if( dc4w_usage( "ERROR: Switch -I:?: must be only (D)irectory or (F)ile name/mask!" ) )
                         return 1;
                  }
               } else {
                  if( AddStgs2List( &g_sInFiles, cp ) )
                     goto errant_I;
               }
            } else {
errant_I:
               if( dc4w_usage( "ERROR: Switch -I problem. Must have file name or mask!" ) )
                   return 1;
            }
            break;

         case 'L':
            ta->fShall = TRUE;
            break;

         case 'N':  // no zip
            g_bNoAZip = TRUE;
// FIX20051222 - Reverse logic on ZIPPING - presently defaults ON, and -N to stop
// So, keep -n to say NO ZIP, but -n+ or -na (all) will cause a ZIP to be written            g_bNoAZip = TRUE;
            cp++;
            while(*cp) {
               c = toupper(*cp);
               if( c == 'A' ) {
                  g_bNoAZip = FALSE;
                  g_bZipAll = TRUE;
               } else if ( c == '-' ) {
                  g_bNoAZip = TRUE;
               } else if ( c == '+' ) {
                  g_bNoAZip = FALSE; // PLUS MEANS ALLOW AUTOZIP TO RUN
               } else {
                  if (dc4w_usage( "ERROR: Switch -N only takes -, +, or A (for ALL)!" )) 
                      return 1;
               }
               cp++; // bump to NEXT char
            }
            break;

         case 'R':    // Recursive
         //case 'T':
            cp++;
            if( *cp )
            {
               if( *cp == '+' )
               {
#ifndef USE_GLOBAL_RECURSIVE
                  ta->fDeep  = TRUE;
#endif // #ifndef USE_GLOBAL_RECURSIVE
                  ta->fShall = FALSE;
                  g_bNOTRecursive = FALSE;
               }
               else if( *cp == '-' )
               {
#ifndef USE_GLOBAL_RECURSIVE    // FIX20091125
                  ta->fDeep = FALSE;
#endif // #ifndef USE_GLOBAL_RECURSIVE
                  ta->fShall = TRUE;
                  g_bNOTRecursive = TRUE;   // FIX20091125
               }
               else if( ( cmd == 'R' ) && ( toupper( *cp ) == 'E' ) )
                  ta->fReverse = TRUE;
               else {
                  if( dc4w_usage( "ERROR: Switch -R only takes + or -!" ) )
                      return 1;
               }
            } else {
#ifndef USE_GLOBAL_RECURSIVE
               ta->fDeep = TRUE;
#endif // #ifndef USE_GLOBAL_RECURSIVE
               g_bNOTRecursive = FALSE;
            }
            break;

         case 'S':   // Save a FILE LIST
            /* read letters for the save option: s,l,r,d */
            // -s[slrd][:outfile]   // note: if outfile given
            // then application will EXIT after doing the output.
            cp++;
            if( *cp != ':' )
            {
               // collect options - Each letter is bits
               while( *cp )
               {
                  c = toupper(*cp);
                  dwo = GetSaveBits( c ); // see BIT2STG sSaveOpts[] = { in dc4wUtil.c
                  if(dwo)
                  {
                     //case 'S': ta->saveopts |= INCLUDE_SAME;
                     //case 'L': ta->saveopts |= INCLUDE_LEFTONLY;
                     //case 'R': ta->saveopts |= INCLUDE_RIGHTONLY;
                     //case 'D': ta->saveopts |= INCLUDE_ALLDIFF;
                     if(cp[1] == '-')
                     {
                        cp++;
                        ta->saveopts &= ~(dwo);
                     }
                     else
                        ta->saveopts |= dwo;

                     // special case
                     if(dwo & FULL_NAMES)
                     {
                        if( ISNUM(cp[1]) )
                        {
                           cp++;
                           c = *cp;
                           if( ( c == '1' ) || ( c == '2' ) || ( c == '3' ) )
                           {
                              if(c == '1')
                                 dwo = FLEFT_NAME;
                              else if(c == '2')
                                 dwo = FRIGHT_NAME;
                              else
                                 dwo = COMBINED_NAME;
                              ta->saveopts |= dwo;
                           }
                           else
                           {
                              if (dc4w_usage(NULL))
                                  return 1;
                           }
                        }
                        else
                        {
                           ta->saveopts |= FLEFT_NAME;
                        }
                     }
                  }
                  else
                  {
                     char * pcmd = argv[i];
                     if( dc4w_usage_save( pcmd ) )
                         return 1;
                  }

                  cp++;    // bump to NEXT
                  if( *cp == ':' )  // reached OUPUT file
                     break;
               }
            }

            // if there IS an OUPUT file
            if( *cp == ':' )
            {
               cp++;
               if( *cp ) {
                  // make sure we have some of
                  // say (INCLUDE_LEFTONLY | INCLUDE_ALLDIFF | INCLUDE_HEADER);
                  // upper bits are ALL options, and diff is now 2 (newer|older)
                   if( ta->saveopts == INCLUDE_DIFFER ) {
                       // just 'D', then
                       gbExact = TRUE;  // set FULL FILE COMPARE
                       gbIgnDT = TRUE;  // and IGNORE DATETIME of file
                       gbIgnDT2 = TRUE;
                   }
                   if( ta->saveopts == 0 ) {
                     ta->saveopts = INC_ALLXSM;
                   } else if(( ta->saveopts & INC_OUTLINE) == 0 ) {
                     ta->saveopts = (INCLUDE_ALLDIFF | INC_ALLONLY);
                   }
                   ta->pSaveList = &ta->szSaveList[0];
                   strcpy( ta->pSaveList, cp );  // NOTE: setting this causes prog. to exit
               }
            }
            break;

         case 'T':
            cp++;
            cmd = toupper(*cp);
            flg = FALSE;
            if(( cmd == 0 ) || ( cmd == 'E' ) || ( cmd == '+' )) {
               flg = TRUE; //  TRUE;
            } else if(( cmd == 'O' )||( cmd == '-' )) {
               flg = FALSE;   // FALSE;
            } else {
               if (dc4w_usage( "ERROR: T switch is -T[E|+] only!" ))
                   return 1;
            }
            ToggleBool( &gbIgnDT, &gbChgIDT, flg );
            break;
         case 'X':   // EXCLUDE D=Directories F=Files, or -X+11 to exclude List11 [Exclude]
            // note: -x+0 is an error, since default is ZERO = First of list = List1= in INI
            cp++; // bump to next
            c = Get_X_Options(cp);
            if( c ) { // if an ERROR
// Bad_XP:  // a BAD -x parameter
               if( dc4w_usage( "ERROR: Switch -x is -x-, -x+[nn], -xf:<file[s]> or -xd:<dir[s]> or -xdfp!" ) )
                   return 1;
            }
            break;

#ifdef ADD_ZIP_SUPPORT
#ifdef   GMUZ
         case 'Z':   // one of the COMPARES is a ZIP file
            cp++; // bump to next
            if(( dir_isvalidfile(cp) ) &&
               ( IsValidZip(cp)      ) ) // #ifdef ADD_ZIP_SUPPORT
            {
               if( ta->pFirst == NULL )
               {
                  strcpy( ta->szFirst, cp );
                  ta->pFirst = &ta->szFirst[0];
                  ta->taFlag |= tf_LeftisZip;
               }
               else if( ta->pSecond == NULL )
               {
                  strcpy( ta->szSecond, cp );
                  ta->pSecond = &ta->szSecond[0];
                  ta->taFlag |= tf_RightisZip;
               }
               else
               {
                  if (dc4w_usage(NULL))
                      return 1;
               }
            }
            else
            {
               if (dc4w_usage(NULL))
                   return 1;
            }
            break;
#endif   // GMUZ
#endif // #ifdef ADD_ZIP_SUPPORT

         case '?':  // FIX20130808
            if (dc4w_usage("Request to show this HELP"))
                return 1;
            break;
         default:
            if (dc4w_usage(NULL))
                return 1;
         }
      }
      else if( c == '@' )
      {
         // an INPUT file
         cp++;
         if( !ProcessInput( ta, cp ) )
         {
            if (dc4w_usage(NULL))
                return 1;
         }
      }
      else
      {
         if( ta->pFirst == NULL )
         {
            strcpy( ta->szFirst, argv[i] );
            ta->pFirst = &ta->szFirst[0];
         }
         else if( ta->pSecond == NULL )
         {
            strcpy( ta->szSecond, argv[i] );
            ta->pSecond = &ta->szSecond[0];
         }
         else
         {
            if (dc4w_usage(NULL))
                return 1;
         }
      }
   }
   return iret;
}


// eof - dc4wHelp.c
