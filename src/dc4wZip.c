

// dc4wZip.c
// this is public domain software - praise me, if ok, just don't blame me!
// Handles BOTH the reading/unzipping of a ZIP file, and
// the creation of a ZIP of the LIST about to be overwritten by an update
// This 2nd function, not yet COMPLETED, would allow a BACKUP or UNDO if
// done CAREFULLY.

#include	"dc4w.h"
//#include  "dc4wZip.h"

#ifdef ADD_ZIP_SUPPORT  // ZIP file support
///////////////////////////////////////////////////////////////////

#define UNZIP_INTERNAL
#include "izuz\iz_unzip.h"      /* includes, typedefs, macros, prototypes, etc. */

#define   INSPERPATH    // add 'files' from zip per folder sort

extern   int getNTfiletime(struct Globals * pg, FILETIME * pModFT, FILETIME * pAccFT, FILETIME * pCreFT );
extern   int gmuzToMemory(__GPRO__ char * zip, char * file, UzpBuffer *retstr );
extern   BOOL  GetListMember( LPTSTR lpb, DWORD dwNum );
extern   BOOL  g_bZipAll;   // -NA to override the exclusions
extern VOID prt( LPTSTR lps );
extern INT_PTR  Do_IDM_EDITZIPUP( VOID );

#undef   SHOWTIMES
#define   SHOWFL2    // write a LIST file - from the ZIP list
#define   SHWCMP2    // diagnostic compare of ZIP file times

#define  MXCFNDS     16    // max. concurrent finds

#define  dwf_Done1   1  // flag is shifed left by dwFndLev

#define  W_PATH      '\\'
#define  U_PATH      '/'

// NOTE: Have done g_dwTZID = GetTimeZoneInformation( &g_sTZ );   // time zone
// which gives -
//typedef struct _TIME_ZONE_INFORMATION { 
//    LONG       Bias; 
//    WCHAR      StandardName[ 32 ]; 
//    SYSTEMTIME StandardDate; 
//    LONG       StandardBias; 
//    WCHAR      DaylightName[ 32 ]; 
//    SYSTEMTIME DaylightDate; 
//    LONG       DaylightBias; 
//} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION;
// if g_dwTZID is NOT = TIME_ZONE_ID_INVALID
// where
// Members
// Bias 
// Specifies the current bias, in minutes, for local time translation on this computer.
// The bias is the difference, in minutes, between Coordinated Universal Time (UTC)
// and local time. All translations between UTC and local time are based on the
// following formula: 
// UTC = local time + bias 
// This member is required. 


// ***STRUGGLING WITH TIME***
// Now ShowTime() outputs
// DIR listing show
//  Directory of D:\DTEMP\temp2\FlightGear
//ACLOCAL  M4          4,699  17/02/00  21:34 aclocal.m4
// Directory of D:\FG2\FlightGear
//ACLOCAL  M4          4,699  17/02/00  21:34 aclocal.m4
//CHECK: [FlightGear/aclocal.m4] <F> (1)
//Unix=[Thu Feb 17 21:34:30 2000] GMT=[Thu Feb 17 20:34:30 2000]
//  FT=[17/02/00 22:34] ST =[17/02/2000 20:34]
// List file originally showed
// .\FlightGear\aclocal.m4 <Newer but=size>17/02/00 20:34 17/02/00 19:34 4699
// BUT changed LIST file to show
//.\FlightGear\aclocal.m4 <Newer but=size> 17/02/00 22:34 17/02/00 21:34 4699
// ***A ONE HOUR DIFFERENCE***


typedef  struct tagZIPFILE {
   LIST_ENTRY  pList;   // MUST be first entry

   DWORD       dwOrder; // zip order of file
   DWORD       dwRank;  // rank, in alphabetic list

   DWORD       dwLevel; // zero if ROOT
   DWORD       dwFlag;  // flag of bits

   DWORD       dwCompSize; // compressed size
   DWORD       dwRatio; // ration
   time_t      uxTime;
   TCHAR       szDC4WName[264];     // *** FULL ZIP NAME ***

   BOOL        bSys2FT; // convertion of SYSTEMTIME to FILETIME

   SYSTEMTIME  sST;     // local time

   WIN32_FIND_DATA   sFD;  // maybe better

   TCHAR       szZipPath[264];   // path, (if there is one)

   TCHAR       szModZName[264];   // may be shortened by root

   TCHAR       szRes1[264];   // reserved memory

}ZIPFILE, * PZIPFILE;

BOOL  bUseUTime = TRUE;
USERFUNCTIONS  sUsrFun;
//#  define CONSTRUCTGLOBALS()  Uz_Globs *pG = globalsCtor()
Uz_Globs * g_pG = 0;
BOOL  bAddIDsp = FALSE;
//TCHAR g_szCurZip[264] = { "\0" };
TCHAR gzszTmpBuf[1024];
DWORD g_iActZip = 0; // active ZIP length
BOOL  g_bInZipWait = FALSE;

int UZ_EXP MyInfo(pG, buf, size, flag)
    zvoid *pG;   /* globals struct:  always passed */
    uch *buf;    /* preformatted string to be printed */
    ulg size;    /* length of string (may include nulls) */
    int flag;    /* flag bits */
{
   ulg   i, k;
   INT   c, d, acrlf, iRet;
   LPTSTR   po = &gzszTmpBuf[0];
   uch   *  endbuf = buf + (unsigned)size;
   k = 0;
   c = d = 0;
   acrlf = 0;
   iRet  = 0;
   if(MSG_TNEWLN(flag))
   {
      /* request for END OF LINE characters to be added */
      if(( !size && !((Uz_Globs *)pG)->sol) ||
         (  size && (endbuf[-1] != '\n')  ) )
      {
         acrlf = 1;  // signal MEOR appended
      }
   }
   for( i = 0; i < size; i++ )
   {
      c = buf[i];
      if( c < '  ' )
      {
         if( c == '\r' )
         {
            if( (i+1) < size )
            {
               if( buf[i+1] != '\n' )
               {
                  po[k++] = (TCHAR)c;
                  c = '\n';
               }
            }
            else
            {
                  po[k++] = (TCHAR)c;
                  c = '\n';
            }
         }
         else if( c == '\n' )
         {
            if( d != '\r' )
               po[k++] = '\r';
         }
         else if( c == '\t' )
         {
            c = ' ';
         }
         else
         {
            c = 0;
         }
      }

      if(c)
      {
         po[k++] = (TCHAR)c;
         d = c;
         if( k >= MXTMPBUF )
         {
            po[k] = 0;
            if(iRet)
            {
               if( bAddIDsp )
               {
                  sprtf(po);
               }
            }
            else
            {
               if( bAddIDsp )
               {
                  TrimIB(po);
                  sprtf("I:[%s]"MEOR, po);
                  // WriteFile(hOut, po, k, &dwm, NULL );
               }
            }
            iRet += k;
            k = 0;
         }
      }
   }

   if(k || acrlf)
   {
      //   ( ( c != '\n' ) &&
      //     ( bEnsEOL   ) ) )
      if( acrlf )
      {
         po[k++] = '\r';
         po[k++] = '\n';
      }
      if(iRet)
      {
               if( bAddIDsp )
               {
                  sprtf(po);
               }
      }
      else
      {
               if( bAddIDsp )
               {
   
                  TrimIB(po);
                  sprtf("I:[%s]"MEOR, po);
               }
      }

      //WriteFile(hOut, po, k, &dwm, NULL );
      iRet += k;
   }

   ((Uz_Globs *)pG)->sol = (endbuf[-1] == '\n');

   return 0;
}

BOOL  IsValidZip( LPTSTR lpf ) // #ifdef ADD_ZIP_SUPPORT
{
   BOOL  bRet = FALSE;
   char * fnames[2];
   int   retcode;

   CONSTRUCTGLOBALS();
   g_pG = &G;

   G.message = (MsgFn *)MyInfo;
   uO.jflag = 1;
   uO.tflag = 1;
   uO.overwrite_none = 0;
   G.extract_flag = FALSE;
    //uO.zipinfo_mode &&
    //!uO.cflag &&
    //!uO.tflag &&
    uO.vflag = 0;
    //!uO.zflag
    //!uO.T_flag
    uO.qflag = 2;                        /* turn off all messages */
    G.fValidate = TRUE;
    fnames[0] = lpf;
    fnames[1] = 0;
    //G.pfnames = (char **)&fnames[0];    /* assign default filename vector */
    G.pfnames = &fnames[0];    /* assign default filename vector */
//#ifdef WINDLL
//    Wiz_NoPrinting(TRUE);
//#endif
//   if (archive == NULL) {      /* something is screwed up:  no filename */
//        DESTROYGLOBALS();
//        return PK_NOZIP;
//    }
    //G.wildzipfn = (char *)malloc(FILNAMSIZ + 1);
    //strcpy(G.wildzipfn, archive);
    G.wildzipfn = lpf;
//#if (defined(WINDLL) && !defined(CRTL_CP_IS_ISO))
//    _ISO_INTERN(G.wildzipfn);
//#endif

   G.process_all_files = TRUE;         /* for speed */

   retcode = process_zipfiles(__G);

   DESTROYGLOBALS();

   if( retcode == PK_OK )
      bRet = TRUE;

   return bRet;
}

typedef struct tagFNDSTR {
   LIST_ENTRY  sList;   // always at top
   HANDLE      hFind;         // find handle
   DWORD       dwFndLev;      // up to MXFNDS
   TCHAR       szZipFile[264];   // ZIP we are finding in
   TCHAR       szFndPath[264];   // and PATH for THIS find, and its
   DWORD       dwDep;         // depth of paths
   PLE         pList;   // pointer to ZIP file list
   PLE         pDirs;   // and a corresponding FOLDER list
}FNDSTR, * PFNDSTR;

LIST_ENTRY  sFindList = { &sFindList, &sFindList };
LIST_ENTRY  sDirList  = { &sDirList,  &sDirList  };

PLE   gpCurrList = 0;
DWORD gdwMaxDep, gdwMinDep;
DWORD gdwOrder = 0;

// ctime()
// Convert a time value to a string and adjust for local time zone settings.
// char *ctime( const time_t *timer );
// where timer = the number of seconds elapsed since midnight (00:00:00),
// January 1, 1970, coordinated universal time (UTC). 
#define YR_BASE  1900
// mktime()
// Converts the local time to a calendar value.
// time_t mktime( struct tm *timeptr );
time_t sys_to_ux_time(SYSTEMTIME * pst)
{
    time_t m_time;
    struct tm *tm;
    time_t now = time(NULL);

    tm = localtime(&now);

    tm->tm_isdst = -1;          /* let mktime determine if DST is in effect */

    /* dissect date */
    // tm_year = Year (current year minus 1900)
    // wYear   Specifies the current year. 
    tm->tm_year = (int)(pst->wYear - YR_BASE);
    // tm_mon Month (0 to 11; January = 0)
    // wMonth Specifies the current month; January = 1, February = 2, and so on. 
    tm->tm_mon  = (int)(pst->wMonth  - 1);
    tm->tm_mday = (int)(pst->wDay);

    /* dissect time */
    tm->tm_hour = (int)pst->wHour;
    tm->tm_min  = (int)pst->wMinute;
    tm->tm_sec  = (int)pst->wSecond;

    m_time = mktime(tm);

    return m_time;
}

VOID  ShowTime( LPTSTR pName, DWORD dwAttr, DWORD dwLevel,
               time_t ltime, FILETIME * pft, SYSTEMTIME * pst )
{
   struct tm * gmt;
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb2 = &lpb[64];
   LPTSTR   lpb3 = &lpb2[64];
   LPTSTR   lpb4 = GetFDTStg( pft );

   // Convert a time value to a string and adjust for local time zone settings.
   strcpy(lpb, ctime( &ltime ));
   TrimIB(lpb);

   // gmtime returns a pointer to a structure of type tm.
   // The fields of the returned structure hold the evaluated value
   // of the timer argument in UTC rather than in local time.
   // Each of the structure fields is of type int, ...
   gmt = gmtime( &ltime );
   strcpy(lpb2, asctime(gmt));
   TrimIB(lpb2);

   *lpb3 = 0;
   AppendDateTime2( lpb3, pst ); // include 4 digit year

   sprtf( "CHECK: [%s] <%s> (%d)"MEOR
      "Unix=[%s] GMT=[%s]"MEOR
      "  FT=[%s] ST =[%s]"MEOR,
      pName,
      ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ? "D" : "F"),
      dwLevel,
      lpb, lpb2, lpb4, lpb3 );

//   if( g_pG )
//   {
//      FILETIME ModFT;    /* File time type defined in NT, `last modified' time */
//      FILETIME AccFT;    /* NT file time type, `last access' time */
//      FILETIME CreFT;    /* NT file time type, `file creation' time */
//      INT   gotTime = getNTfiletime( g_pG, &ModFT, &AccFT, &CreFT );
//      if(gotTime)
//      {
//         sprtf( "NTtime: Mod=%s Acc=%s Crt=%s"MEOR,
//            ((gotTime & EB_UT_FL_MTIME) ? GetFDTStg( &ModFT ) : "<none>"),
//            ((gotTime & EB_UT_FL_ATIME) ? GetFDTStg( &AccFT ) : "<none>"),
//            ((gotTime & EB_UT_FL_CTIME) ? GetFDTStg( &CreFT ) : "<none>") );
//      }
//   }
}

DWORD    GetDepth( LPTSTR lpf )
{
   DWORD    dwi = 0;
   LPTSTR   lpb = &g_szBuf1[0];
   LPTSTR   p;
   strcpy(lpb,lpf);  // make copy (to modify)
   p = strrchr(lpb, W_PATH);
   while(p)
   {
      dwi++;
      *p = 0;
      p = strrchr(lpb,W_PATH);
   }
   return dwi;
}

VOID  Forward2Back( LPTSTR lpf )     // and CHANGE back to 'Windows' paths
{
   INT   i = strlen(lpf);
   INT   j;
   for( j = 0; j < i; j++ )
   {
      if( lpf[j] == U_PATH )
         lpf[j] = W_PATH;
   }
}

VOID  Back2Forward( LPTSTR lpf )     // and CHANGE back to 'Windows' paths
{
   INT   i = strlen(lpf);
   INT   j;
   for( j = 0; j < i; j++ )
   {
      if( lpf[j] == W_PATH )
         lpf[j] = U_PATH;
   }
}

PZIPFILE   CopyZipFileStr( PZIPFILE pzf )
{
   PZIPFILE pzf2 = (PZIPFILE)MALLOC(sizeof(ZIPFILE));
   if( !pzf2 )
   {
      chkme( "C:ERROR: CopyZipFileStr: No alloc. memory!"MEOR );
               return 0;
   }

   memcpy( pzf2, pzf, sizeof(ZIPFILE) );

   return pzf2;
}

//VOID  AddDirLev( LPTSTR lpd, DWORD dwlev, PFD pfd )

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : AddDirLev
// Return type: VOID 
// Arguments  : LPTSTR lpf
//            : LPTSTR lpd
//            : DWORD dwlev
//            : PFD pfd
// Description: Add a DIRECTORY, taken from a FILE in the ZIP,
//              thus is a sort of artificial entry, more
// so that FidnFirstZip/FindNextZip function very 'correctly'
// returning FOLDERS, and only if -reiterate into subs-
// then successive calls will be using the FOLDER, thus
// the DEPTH of the call ...
///////////////////////////////////////////////////////////////////////////////
VOID  AddDirLev( LPTSTR lpf, LPTSTR lpd, DWORD dwlev, PFD pfd )
{
   PLE      phdirs = &sDirList;
   PLE      pnd;
   PZIPFILE pzf2;
   LPTSTR   lpd2;
   INT      ii;
   PFD      pfd2;

   ii = 1;
   Traverse_List( phdirs, pnd )
   {
      pzf2 = (PZIPFILE)pnd;
      if( pzf2->dwLevel == dwlev )
      {
         lpd2 = &pzf2->sFD.cFileName[0];
         ii = strcmp( lpd, lpd2 );
         if( ii == 0 )
            return;  // already exists, at this level
//           break;
      }
   }

   if( ii != 0 )
   {
            pzf2 = (PZIPFILE)MALLOC(sizeof(ZIPFILE));
            if( !pzf2 )
            {
               chkme( "C:ERROR: AddDirLev: No alloc. memory!"MEOR );
               return;
            }

            ZeroMemory( pzf2, sizeof(ZIPFILE));

            // set a DIRECTORY/PATH/FOLDER, at a level
            lpd2 = &pzf2->szZipPath[0];
            strcpy( lpd2, lpd );
            pzf2->dwLevel = dwlev;

            // use the DATE/TIME info of the FILE NAME from the ZIP
            // tha first 'exposed' this FOLDER ... maybe not the
            // 'best' date/time for a FOLDER, but will do for now
            // =============================
            pfd2 = &pzf2->sFD;
            memcpy( pfd2, pfd, sizeof(FD) );
            pfd2->nFileSizeLow  = 0;
            pfd2->nFileSizeHigh = 0;
            pfd2->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

            strcpy( &pzf2->sFD.cFileName[0], lpd );

            // ****************************************************************
            strcpy( &pzf2->szModZName[0], lpf );   // keep FULL ZIP NAME always
            strcpy( &pzf2->szDC4WName[0],    lpf );   // version used - maybe modified
            // ****************************************************************

            InsertTailList(phdirs, &pzf2->pList);

    }

}

#ifdef   GMUZ
// WIN32_FIND_DATA 
// GetLocalTime()
// /* Display UTC. */
// time_t ltime;
// struct tm *gmt
//    time( &ltime );
//    printf( "UNIX time and date:\t\t\t%s", ctime( &ltime ) );
//    gmt = gmtime( &ltime );
//    printf( "Coordinated universal time:\t\t%s", asctime( gmt ) );
//
//                               dwsize         compsiz        ratio,
//  month,    day,     year      hour,     minute    second
//  path  unix-time
// NOTES ON pName
// It appears FILES are given as
// "CMPFG.BAT",  "MSVC6/MSG-01.TXT" or "MSVC6/FGFS32/FlightGear.dsw"
// It appears DIRECTORIES are given as
// <none>        "MSVC6/"           or "MSVC6/FGFS32/".
// I want both the FILE CMPFG.BAT and the FOLDER "MSVC6/"
// to be at the SAME level, ie ZERO
// and both "MSVC6/MSG-01.TXT" and "MSVC6/FGFS32" to be level 1
// then level 2, 3 etc ...
void WINAPI SendAppMsg(ulg dwSize, ulg dwCompSz, unsigned ratio,
   unsigned month, unsigned day, unsigned year, unsigned hour, unsigned minute, unsigned second,
   LPSTR pName, time_t uTime, unsigned dwAttr )
{
   PZIPFILE pzf = (PZIPFILE)MALLOC(sizeof(ZIPFILE));
   if(pzf)
   {
      PLE   ph = gpCurrList;
//      PLE   phdirs = &sDirList;
      LPTSTR   ptmp = &gszTmpBuf[0];
      LPTSTR   p, lpd, lpf;
      PZIPFILE pzf2;
      LPTSTR   lpd2, lpf2;
      PFD      pfd;
//      PFD      pfd2;

      //DWORD    dep;

      ZeroMemory(pzf,sizeof(ZIPFILE)); // ensure it is ALL blank
      pzf->dwCompSize   = dwCompSz;
      pzf->dwRatio      = ratio;
      // number of seconds elapsed since midnight (00:00:00), January 1, 1970, UTC 
      pzf->uxTime       = uTime;
      lpf               = &pzf->szModZName[0];   // copy the COMPLETE ZIP entry
      pfd               = &pzf->sFD;   // and a 'simulated' WIN32 pad
      if( bUseUTime )
      {
         struct tm * gmt;
         // FILETIME    ft;
         // gmtime returns a pointer to a structure of type tm.
         // The fields of the returned structure hold the evaluated value
         // of the timer argument in UTC rather than in local time.
         gmt = gmtime( &uTime );
         pzf->sST.wHour         = gmt->tm_hour;
         pzf->sST.wMinute       = gmt->tm_min;
         pzf->sST.wSecond       = gmt->tm_sec;
         // tm_mday Day of month (1 to 31)
         // wDay Specifies the current day of the month. 
         pzf->sST.wDay          = gmt->tm_mday;
         // tm_mon Month (0 to 11; January = 0)
         // wMonth Specifies the current month; January = 1, February = 2, and so on. 
         pzf->sST.wMonth        = gmt->tm_mon + 1;
         // tm_year = Year (current year minus 1900)
         // wYear   Specifies the current year. 
         pzf->sST.wYear         = gmt->tm_year + 1900;
         // The SystemTimeToFileTime function converts a system time to a file time.
         pzf->bSys2FT = SystemTimeToFileTime(
                     &pzf->sST,                    // system time
                     &pzf->sFD.ftLastWriteTime );  // &ft // file time
         // AWK - The following is WORSE
         //if( pzf->bSys2FT )
         //{
         //   // The FileTimeToLocalFileTime function converts a file time
         //   // to a local file time.
         //   pzf->bSys2FT = FileTimeToLocalFileTime(
         //            &ft,     // UTC file time to convert
         //            &pzf->sFD.ftLastWriteTime );  // converted file time
         //}
      }
      else
      {
         // establish the SYSTEMTIME stuff
         pzf->sST.wHour         = hour;
         pzf->sST.wMinute       = minute;
         pzf->sST.wSecond       = second;
         pzf->sST.wDay          = day;
         pzf->sST.wMonth        = month;
         pzf->sST.wYear         = year;
         //pzf->sST.wDayOfWeek    = 0;
         //pzf->sST.wMilliseconds = 0;
         // The FILETIME structure is a 64-bit value representing
         // the number of 100-nanosecond intervals since January 1, 1601 (UTC).
         pzf->bSys2FT = SystemTimeToFileTime(
                     &pzf->sST,                    // system time
                     &pzf->sFD.ftLastWriteTime );  // file time
      }

      //pzf->dwLevel = 0;
      //pzf->dwFlag  = 0;    // no bits yet
      pzf->sFD.nFileSizeLow = dwSize;
      //pzf->sFD.nFileSizeHigh = 0;

      strcpy(ptmp, pName);    // copy the NAME
      Forward2Back(ptmp);     // and CHANGE back to 'Windows' paths
      //dep = GetDepth(ptmp);
      //strcpy( &pzf->szModZName[0], ptmp );   // copy the COMPLETE ZIP entry
      strcpy( lpf, ptmp );   // copy the COMPLETE ZIP entry
      strcpy( &pzf->szDC4WName[0], lpf ); // used ... modified
      lpd = &pzf->szZipPath[0];    // and KEEP the separated PATH
      // Now we can check for a PATH
      p = strrchr(ptmp, W_PATH);
      if(p)
      {
         p++;
         if( *p ) // if FOLDER(S) *PLUS* FILENAME
         {
            pzf->dwLevel++;      // it has DEPTH
            // simulate the 'title' portion of a file name
            strcpy( &pzf->sFD.cFileName[0], p );
            // ENSURE IT IS A FILE ONLY
            if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
            {
               chkme( "WARNING: What appears to be a FILE has FOLDER attribute!"MEOR
                  "[%s]"MEOR,
                  pName );
               dwAttr = FILE_ATTRIBUTE_NORMAL;
            }
            p--;
            *p = 0;  // kill trailing slash
            //strcpy( &pzf->szPath[0], ptmp );    // and KEEP the separated PATH
            strcpy( lpd, ptmp );    // and KEEP the separated PATH
            // now we have a PATH as well
            p = strrchr(ptmp,W_PATH);  // and search for MORE
            while(p)
            {
               pzf->dwLevel++;            // it has MORE DEPTH
               *p = 0;                    // kill it
               p = strrchr(ptmp,W_PATH);  // and find next
            }
         }
         else  // NOTHING following the PATH == A FOLDER NAME ONLY
         {
            p--;
            *p = 0;  // kill this trailing '/'
            p = strrchr(ptmp,W_PATH);
            if(p)
            {
               pzf->dwLevel++;      // it has DEPTH, like "this\that\"
               p++;
               strcpy( &pzf->sFD.cFileName[0], p );
               p--;  // back up
               *p = 0;  // remove trailing slash
               strcpy( &pzf->szZipPath[0], ptmp );    // and KEEP the separated PATH "this\that"
               p = strrchr(ptmp,W_PATH);  // and search for MORE
               while(p)
               {
                  pzf->dwLevel++;            // it has MORE DEPTH
                  *p = 0;                    // kill it
                  p = strrchr(ptmp,W_PATH);  // and find next
               }
            }
            else
            {
               // else a level ZERO directory
               strcpy( &pzf->szZipPath[0], ptmp );    // and KEEP the separated PATH
               strcpy( &pzf->sFD.cFileName[0], ptmp );
            }

            // ENSURE IT IS A DIRECTORY
            if( !( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) )
            {
               chkme( "WARNING: What appears to be a FOLDER does NOT have the attribute!"MEOR
                  "[%s]"MEOR,
                  pName );
               dwAttr = FILE_ATTRIBUTE_DIRECTORY;
            }
         }
      }
      else
      {
         // a level ZERO file name
         strcpy( &pzf->sFD.cFileName[0], pName );
            // ENSURE IT IS *NOT* A DIRECTORY
            if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
            {
               chkme( "WARNING: What appears to be a FILE has FOLDER attribute!"MEOR
                  "[%s]"MEOR,
                  pName );
               dwAttr = FILE_ATTRIBUTE_NORMAL;
            }
      }

      pzf->sFD.dwFileAttributes = dwAttr;    // store the ATTRIBUTE

      gdwOrder++; // bump the ZIP order of output

      pzf->dwOrder = gdwOrder;   // keep that, if needed

#ifdef   INSPERPATH
      //lpf = &pzf->szModZName[0];   // copy the COMPLETE ZIP entry
      //lpd = &pzf->szZipPath[0];    // and KEEP the separated PATH (if any)
      {
         PLE      pn2;
//         PZIPFILE pzf2;
//         LPTSTR   lpd2, lpf2;
         INT      i = 0;

         Traverse_List(ph,pn2)
         {
            pzf2 = (PZIPFILE)pn2;
            lpd2 = &pzf2->szZipPath[0];
            i = strcmp( lpd, lpd2 );
            if( i == 0 )
            {
               lpf2 = &pzf2->szModZName[0];
               i = strcmp( lpf, lpf2 );
               if( i < 0 )
               {
                  break;
               }
            }
            else if( i < 0 )
            {
               break;
            }
         }

         if( i < 0 )
         {
            InsertBefore(pn2, &pzf->pList );
         }
         else
         {
            InsertTailList(ph, &pzf->pList);
         }
      }
#else // !INSPERPATH

      InsertTailList(ph, &pzf->pList);

#endif   // INSPERPATH

#ifdef   SHOWTIMES
      ShowTime( pName, dwAttr, pzf->dwLevel,
         uTime, &pzf->sFD.ftLastWriteTime, &pzf->sST );
      //sprtf( "Lev=%d for [%s]"MEOR, pzf->dwLevel, pName );
#endif   // SHOWTIMES

      if( pzf->dwLevel < gdwMinDep )   // it has MORE DEPTH
         gdwMinDep = pzf->dwLevel;

      if( pzf->dwLevel > gdwMaxDep )
         gdwMaxDep = pzf->dwLevel;

      if( *lpd )
      {
//         PLE      phdirs = &sDirList;
//         PLE      pnd;
//         INT      ii = 1;
//         DWORD    dep = pzf->dwLevel;  // it has MORE DEPTH
         DWORD    dwo = 0;
//      LPTSTR   ptmp = &gszTmpBuf[0];
         LPTSTR   lpb = GetStgBuf();

         strcpy(lpb, lpd);   // get copy of DIRECTORY/FOLDER(S)
         p = strchr(lpb,W_PATH);  // find first
         while(p)
         {
            *p = 0;  // kill it
            // keep where this came from
            // ie, the directory tree it first appeared in ...
            //AddDirLev( lpd, lpb, dwo, pfd );
            AddDirLev( lpf, lpb, dwo, pfd );
            p++;
            lpb = p;
            p = strchr(lpb,W_PATH);  // find first
            dwo++;   // bump level
         }
         if( *lpb )
            AddDirLev( lpf, lpb, dwo, pfd );
//            AddDirLev( lpd, lpb, dwo, pfd );
      }
   }  // allocated memory
   else
   {
      chkme( "C:ERROR: SendAppMsg: No alloc. memory!"MEOR );
   }
}

#else // !GMUZ

// NOT USED
void WINAPI SendAppMsg(ulg dwSize, ulg dwCompressedSize, unsigned ratio,
                       unsigned month, unsigned day, unsigned year,
                       unsigned hour, unsigned minute, char uppercase,
                       LPSTR szPath, LPSTR szMethod, ulg dwCRC, char chCrypt)
{

}

#endif   // GMUZ

#if   0  // out of use

static TCHAR _s_szCommDir[264];
static DWORD _s_dwOffset;

VOID   SetShortest( PLE ph )
{
   PLE      pn;
   DWORD    dwmin = (DWORD)-1;
   DWORD    dwo, dwi;
   LPTSTR   lpd;
   PZIPFILE pzf;

   dwo = 0;
   _s_dwOffset = 0;
   _s_szCommDir[0] = 0;
   Traverse_List( ph, pn )
   {
      dwo++;
      pzf = (PZIPFILE)pn;
      lpd = &pzf->szZipPath[0];
      dwi = strlen(lpd);
      //if( strlen(lpd) < dwmin )
      if( dwi && ( dwi < dwmin ) )
      {
         //dwmin = strlen(lpd);
         dwmin = dwi;
         _s_dwOffset = dwo;
         strcpy(_s_szCommDir, lpd); // keep shortest
      }
   }
}

#endif   // #if   0  // out of use

TCHAR g_szZipRoot2[264];

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : CheckZipLists
// Return type: INT 
// Arguments  : PLE phz
//            : PLE phdirs
// Description: Add any missing DIRECTORIES/FOLDER,
//              like fg79\FlightGear\misc is 3 folders at appropriate levels
// called from }  // near end - GetFileList()
///////////////////////////////////////////////////////////////////////////////
INT  CheckZipLists( PLE phz, PLE phdirs )
{
   DWORD       dep;
   INT         i, icnt, iadd;
   PZIPFILE    pzf, pzf2;
   LPTSTR      lpd, lpd2;
   PLE         pn, pnz, pnd;

   g_szZipRoot2[0] = 0;

   ListCount2(phdirs, &icnt);

   if( !icnt )
      return 0;

Do_Again:

   i = 0;
   pn = 0;
   Traverse_List( phdirs, pnd )
   {
      pzf2 = (PZIPFILE)pnd;
      //lpd2 = &pzf2->szPath[0];
      //lpd2 = &pzf2->sFD.cFileName[0];
      dep  = pzf2->dwLevel;
      if( dep == 0 )
      {
         i++;
         pn = pnd;
      }
   }

   if( i == 1 )
   {
      i = 0;
      pzf2 = (PZIPFILE)pn;
      lpd2 = &pzf2->szZipPath[0];
      Traverse_List( phz, pnz )
      {
         pzf = (PZIPFILE)pnz;
         lpd = &pzf->szZipPath[0];
         if( pzf->dwLevel == 0 )
            i++;
         else if( InStr( lpd, lpd2 ) != 1 )
            i++;
      }

      if( i == 0 )
      {
         LPTSTR lpb;
         DWORD    i2;
         // We CAN remove this, and reduce the DEPTH of the files
         // as they came from the ZIP
         // ie reduce each depth of each by one

         // accumulate the ROOT being removed
         if( g_szZipRoot2[0] )
            strcat(g_szZipRoot2,"\\");
         strcat(g_szZipRoot2, lpd2); // build up the ROOT being 'deleted'

         i = strlen(lpd2); // get LENGTH of this FOLDER name
         Traverse_List( phz, pnz )
         {
            pzf = (PZIPFILE)pnz;
            lpd = &pzf->szZipPath[0];
            i2  = strlen(lpd);
            //lpb = Right( lpd, (strlen(lpd) - i) );
            lpb = Right( lpd, (i2 - i) );
            //strcpy(lpd, &pzf->szPath[i+1]);
            if( *lpb == '\\' )
               lpb++;
            strcpy(lpd, lpb);
            pzf->dwLevel--;
         }

         RemoveEntryList(pn);
         MFREE(pn);

         i = 0;
         Traverse_List( phdirs, pnd )
         {
            pzf2 = (PZIPFILE)pnd;
            pzf2->dwLevel--;
            if( pzf2->dwLevel == 0 )
               i++;
         }

         if( i == 1 )
            goto Do_Again;

      }
   }

   if( g_szZipRoot2[0] )
   {
      sprtf( "Removed BASE [%s] from ZIP list."MEOR, g_szZipRoot2 );
   }

   iadd = 0;

   // for EACH in the DIRECTORY/FOLDER list
   Traverse_List( phdirs, pnd )
   {
      pzf2 = (PZIPFILE)pnd;
      //lpd2 = &pzf2->szPath[0];
      lpd2 = &pzf2->sFD.cFileName[0];
      dep  = pzf2->dwLevel;
      // search the MAIN zip list
      Traverse_List( phz, pnz )
      {
         pzf = (PZIPFILE)pnz;
         lpd = &pzf->sFD.cFileName[0];
         if(( pzf->dwLevel == dep                                  ) &&
            ( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
         {
            if( strcmp( lpd, lpd2 ) == 0 )
            {
               lpd2 = 0;
               break;   // do next in DIRECTORY/FOLDER list
            }
         }
      }

      if( lpd2 )  // if we still have it, then it must be added
      {
         PFD   pfd;
         pzf2 = CopyZipFileStr( pzf2 );
         if(pzf2)
         {
            // and position it within the MAIN list, alphabetically
            Traverse_List(phz, pnz)
            {
               pzf = (PZIPFILE)pnz;
               pfd = &pzf->sFD;
               lpd = &pfd->cFileName[0];
               if( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
               {
                  i = strcmp( &pzf2->sFD.cFileName[0], lpd );
                  if( i < 0 )
                  {
                     InsertBefore(pnz, &pzf2->pList);
                     pzf2 = 0;
                     break;
                  }
               }
            }

            if(pzf2)
            {
               InsertHeadList(phz, &pzf2->pList);
            }
            iadd++;
         }
      }
   }
   return iadd;
}

#if   0  // out of use

INT  CheckZipLists_NOT_USED( PLE phz, PLE phdirs )
{
   INT   iret = 0;
   PLE      pnz, pnd;
   LPTSTR   p, lpb, lpc, lpd, lpd2;
   PZIPFILE pzf, pzf2;
   INT      i, icnt;
   DWORD    iPos, dwl, dwl2;
//   DWORD    dep;

   i = 0;

   ListCount2(phdirs, &icnt);
   //if( !icnt )
   if( icnt < 2 )
      return 0;   // naught to do

//#if   0  // this no longer works, but do I want it anyway

   SetShortest( phdirs );  // get SHORTEST of the DIR'S

   lpc = &_s_szCommDir[0];
   Traverse_List( phdirs, pnd )
   {
      pzf2 = (PZIPFILE)pnd;
      lpd2 = &pzf2->szPath[0];
      iPos = InStr( lpd2, lpc );
      if( iPos != 1 )
         break;
      i++;
   }

   if( i == icnt )
   {
      // we have a 'common' ancester folder/directory set
      dwl = strlen(lpc);
   }
   else
   {
      *lpc = 0;   // no common 'lower' portion
      lpc  = 0;   // and no common pointer
      dwl  = 0;
   }

   //ClearAllDone( phdirs );
   Traverse_List( phdirs, pnd )
   {
      pzf2 = (PZIPFILE)pnd;
      lpd2 = &pzf2->szPath[0];
      dwl2 = strlen(lpd2);
      i    = 1;
      lpb  = GetStgBuf();
      if(lpc && dwl && (dwl <= dwl2))
      {
         if(( dwl == dwl2 ) &&
            ( strcmp( lpc, lpd2 ) == 0 ) )
         {
            // this is the BASE folder, for this ZIP set
            continue;   // skip it for now
         }
         //strcpy(lpb, &lpd2[dwl]);
         p = &lpd2[dwl];
         if( *p == '\\' )
            p++;  // move to FOLDER name
         strcpy(lpb, p);   // set SHORTEST folder name
      }
      else
      {
         strcpy(lpb,lpd2);
      }


      Traverse_List( phz, pnz )
      {
         pzf = (PZIPFILE)pnz;
         lpd = &pzf->szPath[0];
         if( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         {
            //i = strcmp( lpd, lpd2 );
            i = strcmp( lpd, lpb );
            if( i == 0 )
            {
               break;
            }
         }
      }
      if(i)
      {
         // Add this directory to the MASTER list
         iret++;
      }
   }

//#endif   // #if   0  // this no longer works, but do I want it anyway

   return iret;
}
#endif   // #if   0  // out of use

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : GetFileList
// Return type: BOOL 
// Arguments  : LPTSTR lpf
//            : PLE ph
// Description: Extract a file list from a ZIP file,
//              and store them in a }ZIPFILE, * PZIPFILE;
// structure.
///////////////////////////////////////////////////////////////////////////////
BOOL  GetFileList( LPTSTR lpf, PLE ph )
{
   INT   i;

   CONSTRUCTGLOBALS();
   g_pG = &G;

   G.message = (MsgFn *)MyInfo;

   gpCurrList = ph;

   gdwMaxDep  = 0;
   gdwMinDep  = (DWORD)-1;
   gdwOrder   = 0;   // zip extraction order

   G.incnt_leftover = 0;
   ZeroMemory( &sUsrFun, sizeof(USERFUNCTIONS));

   sUsrFun.SendApplicationMessage = SendAppMsg;
   G.lpUserFunctions = &sUsrFun;

   uO.zipinfo_mode = FALSE;
   G.extract_flag = FALSE;
   G.wildzipfn = lpf;
   G.process_all_files = TRUE;      /* for speed */
   G.filespecs = 0;
   G.xfilespecs = 0;
   uO.qflag = 2;  /* turn off all messages */
   uO.vflag = 1;  // set ONLY list. ie -l


   i = process_zipfiles(__G);

   if( i == PK_OK )  // add in any MISSING DIRECTORIES/FOLDERS
      CheckZipLists( ph, &sDirList );

   DESTROYGLOBALS();

   if( i == PK_OK )
      return TRUE;
   else
      return FALSE;
}  // end - GetFileList()


DWORD ClearDoneFlag( PLE pf, DWORD dwfl )
{
   DWORD    dwi = 0;
   PLE      pn;
   PZIPFILE pzf;
   DWORD    dwdf = ~(dwf_Done1 << dwfl);
   Traverse_List(pf,pn)
   {
      pzf = (PZIPFILE)pn;
      pzf->dwFlag &= dwdf;    // FindFisrt CLEARS ALL flags at this level
      dwi++;
   }
   return dwi;
}

DWORD ClearAllDone( PLE pf )
{
   DWORD    dwi = 0;
   PLE      pn;
   PZIPFILE pzf;
   Traverse_List(pf,pn)
   {
      pzf = (PZIPFILE)pn;
      pzf->dwFlag = 0;
   }
   return dwi;
}

// We have done the FIRST big listing
// now should be RECURSIVE into the FOLDERS
// form scandir.c
// NOT the ROOT, so ADD the relative folder name AFTER the ZIP
// For example, if 'comparing' say "E:\FG\FG-06.ZIP"
// then this will produce
// "E:\FG\FG-06.ZIP\FlightGear\ to pass to FindFirstZip
// (a) it is thus highly unlikely to pass the dir_isvalidfile()
// test, and (b) most certainly will NOT pass the IsValidZip()
// test, so we will KNOW it is a folder path within the ZIP

#if   0  // discontinue this find first 

//HANDLE   FindFirstZip2( LPTSTR lpf, PWIN32_FIND_DATA pfd )
HANDLE   FindFirstZip2( LPTSTR lpf, PFDX pfdx )
{
   PFD      pfd = &pfdx->sFD;
   HANDLE   hFind = 0;
   LPTSTR   ppath = &pfdx->szPath[0];

   if( InStr( lpf, &g_szActZip[0] ) == 1 )   // like strbgn
   {
      LPTSTR   ptmp = &gszTmpBuf[0];
      INT      ilen = strlen(&g_szActZip[0]);
      PLE      ph = &sFindList;
      //PLE      pn = MALLOC(sizeof(FNDSTR));
      PLE      pn = 0;
      PLE      pf;
      PZIPFILE pzf;
      DWORD    dwfl; // find level (up to MXFNDS) (was 16)

      ListCount2(ph, &dwfl);
      if( dwfl >= MXCFNDS )
      {
         chkme( "ERROR: Maximum concurrent finds %d EXCEEDED!"MEOR, MXCFNDS );
         return 0;
      }
      pn = MALLOC(sizeof(FNDSTR));
      if(pn)
      {
         PFNDSTR pfs = (PFNDSTR)pn;
         BOOL     bFound = FALSE;
         LPTSTR   lpf2;
         DWORD    dep;
         DWORD    dwdf = (dwf_Done1 << dwfl);

         pfs->dwFndLev = dwfl;         // set FIND level (concurrent to MXFNDS)
         pf = &g_sZipFiles;
         strcpy(ptmp, &lpf[ilen+1]);   // get the FOLDER
         dep = GetDepth(ptmp);
         if(*ptmp && dep)
            ptmp[ (strlen(ptmp)-1) ] = 0;   // kill the TRAILING
         if(( *ptmp ) &&
            ( !IsListEmpty( pf ) ) )   //  ( ClearDoneFlag( pf, dep ) ) )
         {
            PLE   pn2;
            strcpy( &pfs->szFile[0], &g_szActZip[0] );   // KEEP the ZIP file name
            strcpy( &pfs->szPath[0], ptmp );    // path MINUS trailing slash
            hFind = (HANDLE)pn;
            pfs->hFind = hFind;
            pfs->dwDep = dep;       // set the DEPTH into the folder structure
            Traverse_List(pf,pn2)
            {
               pzf = (PZIPFILE)pn2;
               if(!( pzf->dwFlag & dwdf  ) &&
                   ( pzf->dwLevel == dep ) )
               {
                  //lpf2 = &pzf->szModZName[0];
                  lpf2 = &pzf->szPath[0];
                  //if( InStr( lpf2, ptmp ) == 1 )
                  if( strcmpi( lpf2, ptmp ) == 0 )
                  {
                     // first to return
                     memcpy( pfd, &pzf->sFD, sizeof(WIN32_FIND_DATA) );
                     pzf->dwFlag |= dwdf;
                     bFound = TRUE;
                     strcpy( ppath, &pzf->szModZName[0] );   // = &pfdx->szPath[0];
                     break;
                  }
               }
            }

            if(bFound)
               InsertTailList(ph,pn);  // add to LIST of FINDINGS
         }

         if( !hFind || !bFound )
         {
            MFREE(pn);
            hFind = 0;
         }
      }
   }
   return   hFind;
}
#endif   // #if   0  // discontinue this find first 

//          pf = &g_sZipFiles;
#ifdef   SHOWFL2
TCHAR szTmpFL[] = "TEMPLIST.TXT";
TCHAR szTmpMI[] = "TEMPMISS.TXT";
TCHAR gszActDir[264];   // active directory
#define  wi(a)    WriteFile(h, a, strlen(a), &dww, NULL)
#endif   // SHOWFL2

#define  addsp(a,b)  while(strlen(a) < b) strcat(a," ")

VOID  WRiteFileList( LPTSTR lpzf, PLE pf, BOOL bflg, DWORD dwfl )
{
#ifdef   SHOWFL2
   PLE      pn;
   DWORD    dwc, dww, dwo;
   PZIPFILE pzf;
   LPTSTR   lpf, lpft;
   LPTSTR   lpb = &gszTmpBuf[0];
   LPTSTR   lpb1 = &g_szBuf1[0];
   LPTSTR   lpb2 = &g_szBuf2[0];
   HANDLE   h;
   DWORD    dwtot = 0;
   DWORD    dwdf = (dwf_Done1 << dwfl);
   LPTSTR   p, lpd;

   ListCount2(pf, &dwc);
   if( !dwc )
      return;

   if( bflg )
      lpf = &szTmpFL[0];   // TEMPLIST.TXT
   else
      lpf = &szTmpMI[0];

   h = CreateFile( lpf,   // file name
      GENERIC_READ | GENERIC_WRITE,                      // access mode
      0,                          // share mode
      0, // SD
      CREATE_ALWAYS,                // how to create
      FILE_ATTRIBUTE_NORMAL,                 // file attributes
      0 );        // handle to template file

   if( !VFH(h) )
      return;

   //ListCount2(pf, &dwc);

//   sprintf(lpb, "List of %d files in [%s]..."MEOR, dwc, lpzf );
   sprintf(lpb, "List of %d items in [%s]..."MEOR, dwc, lpzf );
   wi(lpb);

   if( bflg )
      ClearDoneFlag(pf, dwfl);
   //   ClearAllDone(pf);

   dwo = dwtot = 0;
   Traverse_List( pf, pn )
   {
      pzf = (PZIPFILE)pn;
      if( !(pzf->dwFlag & dwdf) &&
           ( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
      {
         lpf  = &pzf->szModZName[0];   // FULL zip name
         lpd  = &pzf->szZipPath[0];       // PATH, and
         lpft = &pzf->sFD.cFileName[0];   // FILE TITLE
         dwtot++;

         sprintf(lpb2, "%4d  <DIR>      ", dwtot );

         sprintf(lpb, "%s %s %s",
               lpb2,
               GetFDTStg( &pzf->sFD.ftLastWriteTime ),
               lpft );

         //if(*lpd)
         //   sprintf(EndBuf(lpb)," in %s ", lpd );
         strcpy(lpb2,lpf);
         p = strrchr(lpb2, '\\');   // get LAST
         if(*lpf)
            sprintf(EndBuf(lpb)," [%s]", lpf);


         addsp(lpb,75);

         sprintf(EndBuf(lpb), " (%d)", pzf->dwLevel);
         strcat(lpb,MEOR);
         wi(lpb);
         pzf->dwFlag |= dwdf;
         dwo++;
      }
   }

   if( dwtot == dwc )
      goto End_List;

   Traverse_List( pf, pn )
   {
      pzf = (PZIPFILE)pn;
      if( !( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
          !( pzf->dwFlag & dwdf ) )
      {
         lpf  = &pzf->szModZName[0];   // FULL zip name
         lpd  = &pzf->szZipPath[0];       // PATH, and
         lpft = &pzf->sFD.cFileName[0];   // FILE TITLE

         p = strrchr(lpf, '\\');

         //if( strrchr(lpf, '\\') == 0 )
         if(p)
         {
            // file name contains a DIRECTORY
            //lpd = &pzf->szPath[0];
            if( strcmp( lpd, gszActDir ) )
            {
               strcpy(gszActDir, lpd);
               //sprintf(lpb, "     Directory of %s"MEOR, lpd );
               //wi(lpb);
            }
         }
         else
         {
            // just a FILE TITLE
            if(dwo == 0)
            {
               sprintf(lpb, "     Directory of BASE"MEOR );
               wi(lpb);
            }
         }

         dwtot++;
         sprintf(lpb2, "%4d %12d", dwtot, pzf->sFD.nFileSizeLow );
         if( *lpd )
         {
            // show file and path separately
            sprintf(lpb, "%s %s %s in %s",
               lpb2,
               GetFDTStg( &pzf->sFD.ftLastWriteTime ),
               lpft,
               lpd );
         }
         else
         {
            sprintf(lpb, "%s %s %s",
               lpb2,
               GetFDTStg( &pzf->sFD.ftLastWriteTime ),
               lpf );
         }
         addsp(lpb,75);
         sprintf(EndBuf(lpb), " (%d)", pzf->dwLevel);
         strcat(lpb,MEOR);
         wi(lpb);
         pzf->dwFlag |= dwdf;
         dwo++;

      }
   }

   if( dwtot == dwc )
      goto End_List;


   Traverse_List( pf, pn )
   {
      pzf = (PZIPFILE)pn;
      if( !(pzf->dwFlag & dwdf) &&
           ( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
      {
         PLE      pn2;
         PZIPFILE pzf2;
         BOOL     bdnh;
         LPTSTR   lpf2;
         DWORD    dwl;

         bdnh = FALSE;
         dwo  = 0;
         lpf = &pzf->szModZName[0];
         dwl = strlen(lpf);
         Traverse_List( pf, pn2 )
         {
            pzf2 = (PZIPFILE)pn2;
            lpf2 = &pzf2->szModZName[0];
            if(( pn != pn2 ) && !( pzf2->dwFlag & dwdf ) &&
               !( pzf2->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
               if(( InStr( lpf2, lpf ) == 1 ) &&
                  ( strrchr( &lpf2[dwl], '\\' ) == 0 ) )
               {
                  if(dwo == 0)
                  {
                     dwtot++;
                     sprintf(lpb, "%4d Directory of %s", dwtot, lpf );
                     sprintf(EndBuf(lpb), " (%d)", pzf->dwLevel);
                     strcat(lpb,MEOR);
                     wi(lpb);
                  }
                  dwtot++;
                  sprintf(lpb2, "%4d %12d", dwtot, pzf2->sFD.nFileSizeLow );
                  sprintf(lpb, "%s %s %s",
                     lpb2,
                     GetFDTStg( &pzf2->sFD.ftLastWriteTime ),
                     &lpf2[dwl] );
                  sprintf(EndBuf(lpb), " (%d)", pzf2->dwLevel);
                  strcat(lpb,MEOR);
                  wi(lpb);
                  dwo++;
                  pzf2->dwFlag |= dwdf;  // done this FILE in a DIRECTORY
               }
            }
         }
         pzf->dwFlag |= dwdf;   // done this DIRECTORY
      }
   }

   if( dwtot == dwc )
      goto End_List;

   dwo = 0;
   Traverse_List( pf, pn )
   {
      pzf = (PZIPFILE)pn;
      if( !(pzf->dwFlag & dwdf) )
      {
         if(dwo == 0)
         {
            sprintf(lpb, "     Directory of MISSED"MEOR );
            wi(lpb);
         }

         dwtot++;
         if( pzf->sFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            sprintf(lpb2, "%4d  <DIR>    ", dwtot );
         else
            sprintf(lpb2, "%4d %10d ", dwtot, pzf->sFD.nFileSizeLow );
         sprintf(lpb, "%s %s %s",
            lpb2,
            GetFDTStg( &pzf->sFD.ftLastWriteTime ),
            &pzf->szModZName[0] );
         sprintf(EndBuf(lpb), " (%d)", pzf->dwLevel);
         strcat(lpb,MEOR);
         wi(lpb);
         dwo++;
      }
   }

End_List:

   sprintf(lpb, "Written List of %d files."MEOR, dwtot );
   wi(lpb);

   CloseHandle(h);

   if( bflg )
      ClearDoneFlag(pf, dwfl);
   //   ClearAllDone(pf);

#endif   // SHOWFL2
}

VOID  ChkMissedList( VOID ) // #ifdef ADD_ZIP_SUPPORT
{
#ifdef   SHOWFL2
   WRiteFileList( &g_szActZip[0], &g_sZipFiles, FALSE, 31 );
#endif   // SHOWFL2
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : FindFirstZip
// Return type: HANDLE 
// Arguments  : LPTSTR lpf
//            : PWIN32_FIND_DATA pfd
// Description: Passed the ZIP name, EXTRACT all the files
//              in the ZIP (if any)
///////////////////////////////////////////////////////////////////////////////
//HANDLE   FindFirstZip( LPTSTR lpf, PWIN32_FIND_DATA pfd ) // #ifdef ADD_ZIP_SUPPORT
HANDLE   FindFirstZip( LPTSTR lpfstr, PFDX pfdx ) // #ifdef ADD_ZIP_SUPPORT
{
   HANDLE   hFind = 0;
   BOOL     bFound = FALSE;
   DWORD    dwfl;
   LPTSTR   lpf = lpfstr;
   LPTSTR   lpa = g_szActZip;
   DWORD    iPos = InStr(lpf,lpa);
   LPTSTR   lpb, lpd;

   lpd = 0;
   if( iPos == 1 )
   {
      lpb = GetStgBuf();
      lpd = &lpfstr[ strlen(lpa) + 1 ];   // point to directory/file added
      strcpy(lpb,lpf);
      lpb[ strlen(lpa) ] = 0;
      lpf = lpb;
   }

   if(( iPos           == 1   ) ||
      (( dir_isvalidfile(lpf) ) &&
       ( IsValidZip( lpf )    )) ) // #ifdef ADD_ZIP_SUPPORT
   {
      PLE      ph = &sFindList;
      PLE      pn;
      PLE      pf;
      PZIPFILE pzf;
      PFD      pfd = &pfdx->sFD;
      LPTSTR   ppath = &pfdx->szPath[0];

      ListCount2(ph, &dwfl);
      if( dwfl >= MXCFNDS )
      {
         chkme( "ERROR: Maximum concurrent finds %d EXCEEDED!"MEOR, MXCFNDS );
         return 0;
      }

      pn = MALLOC(sizeof(FNDSTR));
      if(pn)
      {
         PFNDSTR pfs = (PFNDSTR)pn;
         DWORD dep    = 0;
         DWORD dwdf = (dwf_Done1 << dwfl);
         LPTSTR lpfnd = &pfs->szFndPath[0];

         ZeroMemory(pfs, sizeof(FNDSTR));    // ensure ALL zero
         pfs->dwFndLev = dwfl;

         pf = &g_sZipFiles;
         strcpy( &pfs->szZipFile[0], lpf );

         //InitLList( &pfs->sFiles );

         if(( iPos != 1 ) &&
            ( strcmp( &g_szActZip[0], lpf ) ) )
         {
            strcpy( &g_szActZip[0], lpf );   // update to NEW file
            g_iActZip = strlen(lpf);   // set LENGTH
            KillLList( pf );     // remove any OLD list
            KillLList( &sDirList );
            if( !GetFileList( lpf, pf ) )
            {
               // if ERROR, remove LIST
               KillLList( pf );
               g_szActZip[0] = 0;   // update to NEW file
               g_iActZip = 0;   // set LENGTH
            }
            else
            {
               WRiteFileList( lpf, pf, TRUE, dwfl );
            }
         }
   
         //if( GetFileList( lpf, pf ) )
//         if(( !IsListEmpty( pf ) ) &&
//            ( ClearDoneFlag( pf, dwfl ) ) )
         if( !IsListEmpty( pf ) )
         {
            if( iPos != 1 )
               ClearDoneFlag( pf, dwfl );

            hFind = (HANDLE)pn;
            pfs->hFind = hFind;
//            if(dep < gdwMinDep)
//               dep = gdwMinDep;  // begin the min. DEPTH
            if(lpd)
            {
               DWORD dwi;
               dep = GetDepth(lpd); // set the DEPTH of the search
               strcpy(lpfnd,lpd);   // keep the FIND path string
               dwi = strlen(lpfnd);
               if( dwi && ( lpfnd[ (dwi-1) ] == '\\' ) )
                  lpfnd[ (dwi-1) ] = 0;
            }

            pfs->dwDep = dep;
            InsertTailList(ph,pn);  // add this to ACTIVE FINDS

            Traverse_List(pf,pn)
            {
               pzf = (PZIPFILE)pn;
               //if( pzf->dwLevel == (dep + gdwMinDep) )
               if( pzf->dwLevel == dep )
               {
                  // first to return
                  memcpy( pfd, &pzf->sFD, sizeof(WIN32_FIND_DATA) );
                  pzf->dwFlag |= dwdf;
                  strcpy( ppath, &pzf->szModZName[0] );
                  bFound = TRUE;
                  break;
               }
            }
         }

         if( !hFind || !bFound )
         {
            KillLList(pf);
            MFREE(pn);
            hFind = 0;
            g_szActZip[0] = 0;   // update to NEW file
            g_iActZip = 0;   // set LENGTH
         }
      }
   }
#if   0  // discontinue this find first 
   else
   {
      //hFind = FindFirstZip2( lpf, pfd );
      hFind = FindFirstZip2( lpf, pfdx );
   }
#endif   // #if   0  // discontinue this find first 
   if( !VFH(hFind) )
   {
      bFound = FALSE;   
   }
   return hFind;
}

//BOOL     FindNextZip( HANDLE hFind, PWIN32_FIND_DATA pfd )
BOOL     FindNextZip( HANDLE hFind, PFDX pfdx )
{
   BOOL     bFound = FALSE;
   PLE      ph = &sFindList;
   PLE      pn = 0;
   PFNDSTR  pfs = 0;
   PFD      pfd = &pfdx->sFD;
   LPTSTR   ppath = &pfdx->szPath[0];
   Traverse_List(ph,pn)
   {
      pfs = (PFNDSTR)pn;
      if( pfs->hFind == hFind )
         break;
   }
   if( pn && pfs && ( pfs->hFind == hFind ) )
   {
      //PLE   pf = &pfs->sFiles;
      PLE      pf = &g_sZipFiles;
      PZIPFILE pzf;  // search the ZIP list for the NEXT
      DWORD    dep = pfs->dwDep;
      LPTSTR   lpfnd, lpf2;
      DWORD    dwfl = pfs->dwFndLev;
      DWORD    dwdf = (dwf_Done1 << dwfl);

      lpfnd = &pfs->szFndPath[0];
      Traverse_List(pf,pn)
      {
         pzf = (PZIPFILE)pn;  // cast it to my pointer
         if( ( pzf->dwLevel == dep ) &&
            !( pzf->dwFlag & dwdf ) )
         {
            if(*lpfnd)
            {
               lpf2 = &pzf->szZipPath[0];
               if( strcmpi( lpfnd, lpf2 ) == 0 )
               {
                  // next to return
                  memcpy( pfd, &pzf->sFD, sizeof(WIN32_FIND_DATA) );
                  pzf->dwFlag |= dwdf;
                  bFound = TRUE;
                  if(dep == 0)
                  {
                     chkme( "HOW COME we have a PATH but DEPTH is ZERO!"MEOR );
                  }
                  break;
               }
            }
            else  // NO PATH == BASE level=0 case
            {
               // next to return
               memcpy( pfd, &pzf->sFD, sizeof(WIN32_FIND_DATA) );
               pzf->dwFlag |= dwdf;
               bFound = TRUE;
               strcpy( ppath, &pzf->szModZName[0] );
               if(dep != 0)
               {
                  chkme( "HOW COME we have NO PATH but DEPTH is NOT ZERO!"MEOR );
               }
               break;
            }
         }
      }
   }
   return bFound;
}

VOID     FindCloseZip( HANDLE hFind )
{
   PLE      ph = &sFindList;
   PLE      pn = 0;
   PFNDSTR  pfs = 0;
   Traverse_List(ph,pn)
   {
      pfs = (PFNDSTR)pn;
      if( pfs->hFind == hFind )
         break;
   }
   if( pn && pfs && ( pfs->hFind == hFind ) )
   {
      RemoveEntryList(pn);
      MFREE(pn);
   }
}

#define  TRYCOMP2

LONG  CompFTZip( LONG lg, FILETIME * pft1, FILETIME * pft2 )
{
   SYSTEMTIME  st1, st2;
   LPTSTR      lpb = &gszTmpBuf[0];
   LONG        lg2 = lg;

   if(( FT2LST( pft1, &st1 ) ) &&
      ( FT2LST( pft2, &st2 ) ) )
   {
      //FILETIME ft;
      // UTC file time to convert
      // converted file time
#ifdef   SHWCMP2
      sprintf( lpb,
         "Comparing %s with %s ... ",
         GetFDTSStg( pft1 ),
         GetFDTSStg( pft2 ) );
      sprtf( lpb );
#endif   // #ifdef   SHWCMP2
#ifdef   TRYCOMP2
      if( g_dwTZID != TIME_ZONE_ID_INVALID ) {
         // VALID TIME ZONE
         // FIX20070328 - forget seconds ( st1.wSecond == st2.wSecond ) )
         // add year, month, day check
         if(( st1.wMinute == st2.wMinute ) &&
            ( st1.wYear == st2.wYear     ) &&
            ( st1.wMonth == st2.wMonth   ) &&
            ( st1.wDay == st2.wDay       ) )
         {
            time_t t1, t2;
            //if( abs( st1.wHour - st2.wHour ) == 1 )
            //   return 0;
            // FIX20070328 - make second and ms EQUAL
            st1.wSecond = st2.wSecond;
            st1.wMilliseconds = st2.wMilliseconds;
            t1 = sys_to_ux_time( &st1 );
            t2 = sys_to_ux_time( &st2 );
            if(( t1 != (time_t)-1 ) &&
               ( t2 != (time_t)-1 ) )
            {
               if(abs( g_sTZ.Bias ) ==
                  abs( (int)(t1 / 60) - (int)(t2 / 60) ) )
               {
#ifdef   SHWCMP2
                  sprtf( "found EQUAL with bias %d ..."MEOR, g_sTZ.Bias );
#endif   // #ifdef   SHWCMP2
                  return 0;
               } else {
#ifdef   SHWCMP2
                  sprtf( "still NO %d vs %d ..."MEOR, abs( g_sTZ.Bias ),
                     abs( (int)(t1 / 60) - (int)(t2 / 60) ) );
#endif   // #ifdef   SHWCMP2
               }
            }
            else
            {
#ifdef   SHWCMP2
               sprtf( "Unix conversion FAILED ..."MEOR );
#endif   // #ifdef   SHWCMP2
            }
         }
         else
         {
            // still not the SAME???
#ifdef   SHWCMP2
            sprtf( "Year, Month, Day, or MInutes NOT SAME ..."MEOR );
#endif   // #ifdef   SHWCMP2
         }
      } else {
#ifdef   SHWCMP2
         sprtf( "EEK: Timezone NOT VALID ..."MEOR );
#endif   // #ifdef   SHWCMP2
      }
#else // !TRYCOMP2
      if( FileTimeToLocalFileTime( pft1, &ft ) )
      {
         sprintf( lpb,
            "Comparing %s with %s ..."MEOR,
            GetFDTStg( &ft  ),
            GetFDTStg( pft2 ) );
         if( CompareFileTime( &ft, pft2 ) == 0 )
            return 0;
      }
      if( FileTimeToLocalFileTime( pft2, &ft ) )
      {
         sprintf( lpb,
            "Comparing %s with %s ..."MEOR,
            GetFDTStg( pft1  ),
            GetFDTStg( &ft   ) );
         if( CompareFileTime( &ft, pft1 ) == 0 )
            return 0;
      }
      if( LocalFileTimeToFileTime( pft1, &ft ) )
      {
         sprintf( lpb,
            "Comparing %s with %s ..."MEOR,
            GetFDTStg( &ft  ),
            GetFDTStg( pft2 ) );
         if( CompareFileTime( &ft, pft2 ) == 0 )
            return 0;
      }
      if( LocalFileTimeToFileTime( pft2, &ft ) )
      {
         sprintf( lpb,
            "Comparing %s with %s ..."MEOR,
            GetFDTStg( pft1  ),
            GetFDTStg( &ft   ) );
         if( CompareFileTime( &ft, pft1 ) == 0 )
            return 0;
      }
#endif   // #ifdef   TRYCOMP2
   }
   else
   {
      sprintf( lpb,
         "Compare of %s with %s FAILED!"MEOR,
         GetFDTStg( pft1 ),
         GetFDTStg( pft2 ) );
      chkme( lpb );
   }
   return lg2;
}


INT   UnzFile( POZF pozf )
{
   INT      i   = 0;
   LPTSTR   lpb = 0;
   UzpBuffer g_sUzb;

   CONSTRUCTGLOBALS();

   G.message = (MsgFn *)MyInfo;

   uO.qflag = 2;                    /* turn off all messages */
   g_sUzb.strptr = 0;
   g_sUzb.strlength = 0;

   i = gmuzToMemory(__G__ pozf->pZName, pozf->pFName, &g_sUzb );

   if(( i == PK_OK ) &&
      ( g_sUzb.strptr ) &&
      ( g_sUzb.strlength ) )
   {
      pozf->pData = g_sUzb.strptr;
      pozf->dwLen = g_sUzb.strlength;
      i = TRUE;   // user to FREE
   }
   else
   {
      if( g_sUzb.strptr )
         free( g_sUzb.strptr );
   }

   DESTROYGLOBALS();

   return i;
}

INT   OpenZIPFile( POZF pozf )   // return a BUFFER of file data
{
   INT   i = 0;
   LPTSTR zname = pozf->pZName;
   LPTSTR fname = pozf->pFName;

   if(( dir_isvalidfile(zname) ) &&
      ( IsValidZip( zname )    ) ) // #ifdef ADD_ZIP_SUPPORT
   {
      LPTSTR   lpf = &g_szActZip[0];
      PLE      pf  = &g_sZipFiles;
      PLE      pn;
      PZIPFILE pzf;
      BOOL     bFnd = FALSE;

      if( strcmpi( zname, lpf ) )
      {
         strcpy( lpf, zname );
         KillLList( pf );     // remove any OLD list
         if( !GetFileList( lpf, pf ) )
         {
            // if ERROR, remove LIST
            KillLList( pf );
         }
//         else
//         {
//            WRiteFileList( lpf, pf, TRUE, 31 );
//         }
      }
      Traverse_List( pf, pn )
      {
         pzf = (PZIPFILE)pn;
         lpf = &pzf->szModZName[0];
         if( strcmpi( lpf, fname ) == 0 )
         {
            bFnd = TRUE;
            break;
         }
      }
      if( bFnd )
      {
         Back2Forward(fname); // switch to UNIX form
         i = UnzFile( pozf );
         Forward2Back(fname); // switch to UNIX form
      }
   }
   return i;
}

// ******* ZIPUP A LIST *********

//TCHAR szZpCmd[] = "ZipCommand";
//extern   TCHAR szZipUp[];  // [264] = {"Zip8 -a TEMPZ001.zip @tempz001.lst"};
extern   TCHAR szZipCmd[];  // [264] = {"Zip8 -a TEMPZ001.zip @tempz001.lst"};
//BOOL  bChgZp = FALSE;
extern   INT      giSelection;    // = -1 if NONE, or is selected row in table
//extern   TCHAR szZpCmd2[]; // = "ZipCommand2";
extern   TCHAR szZipCmd2[];   // [264] = { DEF_ZIP_CMD2 };
extern   BOOL  bChgZp2;       // = FALSE;

extern   int   complist_writelist( COMPLIST cl, LPTSTR poutfile, DWORD dwo,
                         BOOL bSetGlob, BOOL bDoMB );
//#ifndef  ADDZIPUP
VOID  getnxtlist( LPTSTR lpIn )
{
   static TCHAR _s_szOrg[264];
   static TCHAR _s_szDir[264];
   static TCHAR _s_szFile[264];
   static TCHAR _s_szBase[264];
   static TCHAR _s_szExt[264];
   LPTSTR   lpd, lpf, lpb, lpe;
   DWORD    ilen;
   INT      c, icnt;

   if( dir_isvalidfile( lpIn ) )
   {
      strcpy(_s_szOrg,lpIn);  // get copy of original
      lpd = _s_szDir;
      lpf = _s_szFile;
      lpb = _s_szBase;
      lpe = _s_szExt;
      icnt = 0;
      do
      {
         SplitFN( lpd, lpf, lpIn );   // remove any directory
         SplitExt2( lpb, lpe, lpf );   // split into base name and extent
         // work on the BASE name
         ilen = strlen(lpb);
         if( ilen == 0 )
            return;  // nothing to change here
         while(ilen)
         {
            if( icnt > 3 )
               break;   // only backup a max. of 999 + 1 times
            ilen--;  // back up one char
            c = lpb[ilen]; // get LAST char
            if( ISNUM(c) )
            {
               if( c < '9' )
               {
                  c++;
                  lpb[ilen] = (TCHAR)c;
                  break;
               }
               else if(ilen)
               {
                  lpb[ilen] = '0';
                  // and cycle in while for some more while
                  icnt++;  // count the '9' backup
               }
               else
               {
                  strcpy(lpIn,_s_szOrg);  // FAILED - put back original
                  return;
               }
            }
            else
            {
               lpb[ilen] = '0'; // change to a ZERO
               icnt++;  // count the '9' backup - well maybe count '0's added
               break;
            }
         }
         // assemble it back
         strcpy(lpIn,lpd);
         strcat(lpIn,lpb);
         if(*lpe)
            strcat(lpIn,".");
         strcat(lpIn,lpe);

      } while( dir_isvalidfile( lpIn ) );
   }
}

VOID Set_Zip_Info( PTSTR pb )
{
   DWORD len = strlen(pb);
   if( len == 0 )
      return;
   if( g_dwZipMax ) {
      if((len + 1) < g_dwZipMax) {
         strcpy(g_pZipInfo,pb);
      } else {
         DWORD nl = (g_dwZipMax + len + 256);
         PTSTR pn = MALLOC(nl);
         if(pn) {
            if(g_pZipInfo)MFREE(g_pZipInfo);
            g_pZipInfo = pn;
            strcpy(g_pZipInfo,pb);
            g_dwZipMax = nl;
         }
      }
   } else {
      g_dwZipMax = strlen(pb) + 1;
      if(g_dwZipMax < 1024)
         g_dwZipMax = 1024;
      g_pZipInfo = MALLOC(g_dwZipMax);
      if(g_pZipInfo) {
         strcpy(g_pZipInfo,pb);
      } else {
         g_dwZipMax = 0;
      }
   }
}

VOID Add_Zip_Info( PTSTR pb )
{
   DWORD len = strlen(pb);
   if( len == 0 )
      return;
   if( g_dwZipMax && g_pZipInfo ) {
      DWORD len2 = strlen(g_pZipInfo);
      if((len2 + len + 1) < g_dwZipMax) {
         strcat(g_pZipInfo,pb);
      } else {
         DWORD nl = (len2 + len + 256);
         PTSTR pn = MALLOC(nl);
         if(pn) {
            strcpy(pn,g_pZipInfo);
            strcat(pn,pb);
            MFREE(g_pZipInfo);
            g_pZipInfo = pn;
            g_dwZipMax = nl;
         }
      }
   } else {
      Set_Zip_Info(pb);
   }
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : do_ziplist
// Return type: LONG 
// Argument   : PEDITARGS pe
// Description: Input in pe->pCmd should have an @file-name in it.
//              This will be extracted, and used as the LIST name
// for the OUTPUT of *** ALL FILES LIKELY TO BE EFFECTED BY AN UPDATE ***
// This is a list output, using a FIXED set of 'compare' criteria
// At present this INCLUDES everything ONLY in right side, since the
// introduction of the DELETE function means these can be DELETED.
// FIX20021115 - Unless -NA = zip all is used, then EXCLUDE RIGHT ONLY
// files of the group: *.obj;*.sbr;*.idb;*.pdb;*.ncb;*.opt
///////////////////////////////////////////////////////////////////////////////
LONG  do_ziplist( PEDITARGS pe )
{
   PVIEW  view      = pe->view;
   DWORD  dwo       = pe->option;
   int    selection = pe->selection;
   LPTSTR   pcmd    = &pe->cCmdLine[0];
   LPTSTR   lpb = &gszTmpBuf[0];

//   COMPITEM item;
//        LPSTR fname;
//   static char cmdline[256];
//   int currentline;
//   char * pOut = cmdline;
   char * pOut = pcmd;
   char * pIn = pe->pCmd;  // like "Zip8 -a tempz001.zip @tempz001.lst"
//   DWORD             dwi = gdwFileOpts;   // get the WRITE FILE LIST options
   //STARTUPINFO          si;
//   STARTUPINFO       sSI;
//   STARTUPINFO  *    psi = &sSI;
//   PROCESS_INFORMATION pi;
   //COMPLIST    cl = view_getcomplist(current_view);
   COMPLIST    cl = pe->cl;
   char *      p;
   LONG        cnt = 0;


//   p = strchr(pIn,'@');
   p = strrchr(pIn,'@');
   if(p)
   {
      p++;
      getnxtlist(p); // role the FILENAME numerically to it is UNIQUE
      strcpy(pcmd,p);   // and MAKE that the *** ZIP INPUT LIST FILE ***
   }
   else
   {
      strcpy(pcmd,"tempz001.lst");  // then we will CREATE a name to use
   }

   //dwi = gdwFileOpts; // use to when writing OUTLINE list of files
   //if( ( dwi & (INCLUDE_SAME | INCLUDE_NEWER | INCLUDE_OLDER | INCLUDE_LEFTONLY |
   //   INCLUDE_RIGHTONLY ) ) == 0 )
   //dwo = ( dwi & (INCLUDE_SAME | INCLUDE_NEWER | INCLUDE_OLDER | INCLUDE_LEFTONLY |
   //   INCLUDE_RIGHTONLY ) );
   //if( dwo == 0 )
   if(( dwo &
      (INCLUDE_SAME | INCLUDE_ALLDIFF | INCLUDE_LEFTONLY | INCLUDE_RIGHTONLY) ) == 0 )
   {
      // then there can be NO LIST!
      // * caller should have checked this BEFORE now ...
      return 0xfDead000;   // note NEGATIVE error value
   }

//   dwo |= FULL_NAMES;
//   dwo |= FRIGHT_NAME;
//   dwo &= ~(FLEFT_NAME | COMBINED_NAME);

   cnt = complist_writelist( cl, pcmd, dwo, FALSE, FALSE );

   if( cnt == 0 )
   {
      // nothing found matching the 'compare' criteria
      // * caller should have checked this BEFORE now ...
      return 0xf0Dead00;   // note negative
   }
   else
   {
      //LPTSTR   lpb = &gszTmpBuf[0];
      // success in writing a LIST file
      LPTSTR   lpinp = &pe->sCmdLn.szInp[0];
      sprintf(lpb, "Written list file [%s] ..."MEOR, lpinp);
      sprtf(lpb);
      Add_Zip_Info(lpb);
//      LPTSTR   pinp = g_sZipCmd.szInp;
//      if( strcmp( pinp, pcmd ) )
//      {
//         strcpy(pinp, pcmd);
      // 3 = input file name
   //   if( strcmp(lpinp,pinp) )
   //   {
//         gbChgZp2[3] = TRUE;
   //      k++;
   //   }
//      }
   }

   return cnt;

}

//typedef struct tagCMDLN {
//   TCHAR    szCmd[264];
//   TCHAR    szSws[264];
//   TCHAR    szZip[264];
//   TCHAR    szInp[264];
//}CMDLN, * PCMDLN;
static EDITARGS _s_sActCmd;
static   TCHAR  _s_szActCmd[264];
static   TCHAR  _s_szNewCmd[264];
extern   TCHAR szZipCmd[];  // [264] = {"c:\\mdos\\Zip8.bat -a -P -o TEMPZ001.zip @tempz001.lst"};
extern   BOOL  bChgZp;  // = FALSE;


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : ReSetCmdLnStruc
// Return type: BOOL 
// Argument   : PEDITARGS pe
// Description: Transfer the ZIPUP components, in an allocated edit arguments
//              to the FIXED WORK structure, to be later written to the INI file
// NOTE: It set the CHANGE flag for each of up to 8 items
// Presently only 4 components are used for this ZIP command.
// 1=Runtime.exe, like "C:\\mdos\\Zip8.bat"
// 2=Switches, like -a minimum - maybe add if none
// 3=Output ZIP file name, like TEMPZ001.zip
// 4=Input list file, preceeded by an '@' character, like @tempz001.lst
///////////////////////////////////////////////////////////////////////////////
BOOL  ReSetCmdLnStruc( PEDITARGS pe )
{
//   PCMDLN   pcmdln = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
   PEDITARGS   pe2 = &g_sZipArgs;   //	sFW.fw_sZipArgs //   g_- Arguments for ZIP of a LIST
   LPTSTR   plst = &pe->cCmdLine[0];
   // NEW command line components - for this zipped list
   LPTSTR   lpcmd = &pe->sCmdLn.szCmd[0];
   LPTSTR   lpswi = &pe->sCmdLn.szSws[0];
   LPTSTR   lpzip = &pe->sCmdLn.szZip[0];
   LPTSTR   lpinp = &pe->sCmdLn.szInp[0];
   LPTSTR   lpcmp = &pe->sCmdLn.szCmp[0];
   LPTSTR   lpenv = &pe->sCmdLn.szEnv[0];
   // INI command line components - output as LastZip
   LPTSTR   pcmd = &pe2->sCmdLn.szCmd[0];
   LPTSTR   pswi = &pe2->sCmdLn.szSws[0];
   LPTSTR   pzip = &pe2->sCmdLn.szZip[0];
   LPTSTR   pinp = &pe2->sCmdLn.szInp[0];
   LPTSTR   penv = &pe2->sCmdLn.szEnv[0];
   LPTSTR   pcmp = &pe2->sCmdLn.szCmp[0];

   //LPTSTR   pccmd = _s_szNewCmd;
   LPTSTR   pccmd = _s_szActCmd;
   LPTSTR   picmd = szZipCmd;
   LPTSTR   p;
   INT      i, k;

   i = k = 0;
   if( g_bZipAll )
      picmd = szZipCmd;
   else
      picmd = szZipCmd2;

   // 0 = command/exe to run
   if( strcmp(lpcmd,pcmd) )
   {
      strcpy(pcmd, lpcmd);
      gbChgZp2[i] = TRUE;
      k++;
   }

   i++;
   // 1 = switches to apply
   if( strcmp(lpswi,pswi) )
   {
      strcpy(pswi, lpswi);
      gbChgZp2[i] = TRUE;
      k++;
   }

   i++;
   // 2 = zip file name to write
   if( strcmp(lpzip,pzip) )
   {
      strcpy(pzip, lpzip);
      gbChgZp2[i] = TRUE;
      k++;
   }

   i++;
   // 3 = input file name
   p = lpinp;
   if( *p == '@' )
   {
      p++;
      if( strcmp( p, plst ) )
      {
         strcpy( p, plst );
      }
   }

   if( strcmp(lpinp,pinp) )
   {
      strcpy(pinp, lpinp);
      gbChgZp2[i] = TRUE;
      k++;
   }

   i++;
   // 4 = env not used
   if( strcmp(lpenv,penv) )
   {
      gbChgZp2[i] = TRUE;
      k++;
   }

   if( GetListMember( lpcmp, 0 ) )
   {
      if( strcmp( lpcmp, pcmp ) )
      {
         strcpy(pcmp, lpcmp);
         gbChgZp2[ZP_DIRCMP] = TRUE;
      }
   }
   // up to 7 = total 8 reserved

   // Then the CHANGE @Input file name
   //if( strcmp( _s_szNewCmd, szZipUp ) ) // [264] = {"c:\\mdos\\Zip8.bat -a -P -o TEMPZ001.zip @tempz001.lst"};
   if( strcmp( pccmd, picmd ) ) // [264] = {"c:\\mdos\\Zip8.bat -a -P -o TEMPZ001.zip @tempz001.lst"};
   {
      //strcpy( szZipUp, _s_szNewCmd );
      //strcpy( picmd, pccmd );
      p = GetStgBuf();
      strcpy(p, pcmd);
      if( *pswi )
      {
         strcat(p," ");
         strcat(p,pswi);
      }
      strcat(p," ");
      strcat(p, pzip);
      strcat(p, " ");
      strcat(p, pinp);
      if( strcmpi( picmd, p ) )
      {
         chkme( "WARNING: This update was in-buffer!"MEOR );
         strcpy(picmd, p);
      }
      bChgZp = TRUE;
      k++;
   }

   return k;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION   : SetCmdLnStruc
// Return type: BOOL 
// Argument   : PEDITARGS pe
// Description: Process of splitting a single command line into ZIPUP sCmdLn
//              components. It is NOT really complete, but it
// works on a 'simple' zip commands ...
///////////////////////////////////////////////////////////////////////////////
BOOL  SetCmdLnStruc( PEDITARGS pe )
{
   LPTSTR   lpcmd = &pe->sCmdLn.szCmd[0];
   LPTSTR   lpswi = &pe->sCmdLn.szSws[0];
   LPTSTR   lpzip = &pe->sCmdLn.szZip[0];
   LPTSTR   lpinp = &pe->sCmdLn.szInp[0];
   LPTSTR   lpenv = &pe->sCmdLn.szEnv[0];
//   PCMDLN   pcmdln = &g_sZipCmd;  // g_sZipArgs.sCmdLn //    sFW.fw_sZipCmd
   LPTSTR   pcmd  = pe->pCmd; // get single command line
   // to be broken into components - Cmd, Sws, Zip, Inp, etc ...
   INT      c, i, ilen, k, sk;

   // *** ZERO ALL COMPONENTS ***
   *lpcmd = 0;
   *lpswi = 0;
   *lpzip = 0;
   *lpinp = 0;
   *lpenv = 0;

   ilen = strlen(pcmd);
   strcpy( _s_szActCmd, pcmd );  // COPY current single line into ACTIVE command

   sk = 0;
   for( i = 0; i < ilen; i++ )
   {
      c = pcmd[i];
      if( c > ' ' )
      {
         if(( c == '-') || ( c == '/' ) || ( c == '@' ) )
         {
            // switch or INPUT file
            if( c == '@' )
            {
               if( *lpinp )
                  return FALSE;
               k = 0;
               lpinp[k++] = (TCHAR)c;
               i++;
               for( ; i < ilen; i++ )
               {
                  c = pcmd[i];
                  if( c > ' ' )
                     lpinp[k++] = (TCHAR)c;
                  else
                     break;
               }
               lpinp[k] = 0;
            }
            else  // command line SWITCH
            {
               // switch
               if(sk)   // if previous
                  lpswi[sk++] = ' ';   // add a space
               lpswi[sk++] = (TCHAR)c; // then begin switch
               i++;  // accumulation while > ' '
               for( ; i < ilen; i++ )
               {
                  c = pcmd[i];
                  if( c > ' ' )
                     lpswi[sk++] = (TCHAR)c;
                  else
                     break;
               }
               lpswi[sk] = 0; // zero terminate SWITCH accumulation
            }
         }
#ifdef   ADDENVEXP2
         else if( ( c == ':' ) && strchr( &pcmd[i+1], ':' ) )
         {
            // we have :something:
            // assume an environment variable
            i++;
            k = 0;
            for( ; i < ilen; i++ )
            {
                  c = pcmd[i];
                  if( ( c > ' ' ) && ( c != ':' ) )
                     lpenv[k++] = (TCHAR)c;  // accumulate chars
                  else
                     break;
            }
            lpenv[k] = 0;  // close item
            if( c == ':' )
            {



            }
         }
#endif   // #ifdef   ADDENVEXP2
         else
         {
            // not switch or input
            if( *lpcmd == 0 )
            {
               // FIRST non-switched command is the runtime
               k = 0;   // start counter
               lpcmd[k++] = (TCHAR)c;  // insert first command
               i++;  // TO NEXT CHAR, untill <= ' '
               for( ; i < ilen; i++ )
               {
                  c = pcmd[i];
                  if( c > ' ' )
                     lpcmd[k++] = (TCHAR)c;
                  else
                     break;
               }
               lpcmd[k] = 0;  // now have a COMMAND
            }
            else if( *lpzip == 0 )
            {
               // SECOND non-switched command is OUTPUT zipup file name
               k = 0;   // start counter
               lpzip[k++] = (TCHAR)c;  // inser first command
               i++;  // until <= ' '
               for( ; i < ilen; i++ )
               {
                  c = pcmd[i];
                  if( c > ' ' )
                     lpzip[k++] = (TCHAR)c;
                  else
                     break;
               }
               lpzip[k] = 0;  // zero-terminate ZIP file name
            }
            else
            {
               // have filled command and zip inputs
               return FALSE;
            }
         }
      }
   }

   if(( *lpzip ) && ( *lpcmd ) )
   {
      getnxtlist(lpzip);
      strcpy(pcmd, lpcmd);
      strcat(pcmd, " ");
      if( *lpswi )
      {
         strcat(pcmd,lpswi);
         strcat(pcmd," ");
      }
      strcat(pcmd, lpzip);
      strcat(pcmd, " "  );

      strcat(pcmd, lpinp);

      strcpy( _s_szNewCmd, pcmd );

      memcpy( &_s_sActCmd, pe, sizeof(EDITARGS) );

      // get ZIP output file name, if none from previous
      // ************************
      strcpy( g_szCurZip, lpzip ); 
//      {
//         PEDITARGS   pe2 = &g_sZipArgs;
//         LPTSTR   pzip = &pe2->sCmdLn.szZip[0];
//         if( *pzip == 0 )
//         {
//            strcpy(pzip,lpzip);
//         }
//      }
      return TRUE;

   }

   return FALSE;
}




#define  ZIP_OK    0x0Abceed
#define  LIST_OK   0x0Abc000

DWORD LaunchZipCmd( PEDITARGS pe )
{
   LPTSTR pcmd = pe->pCmd;
   static STARTUPINFO _s_sSI;
   STARTUPINFO  *    psi = &_s_sSI;
   PROCESS_INFORMATION pi;
   PTHREADARGS pta = (PTHREADARGS)pe->pThreadArgs; // extract thread arguments
   LPTSTR   lpb = &pta->ta_szTmpBuf[0];

   pe->dwError = 0;   // no error
   /* Launch the process and waits for it to complete */
   ZeroMemory(psi, sizeof(STARTUPINFO));
   psi->cb          = sizeof(STARTUPINFO);
        //psi->lpReserved  = NULL;
        //psi->lpReserved2 = NULL;
        //psi->cbReserved2 = 0;
        //si.lpTitle = (LPSTR)cmdline; 
   psi->lpTitle     = pcmd; 
        //psi->lpDesktop   = (LPTSTR)NULL;
   psi->dwFlags     = STARTF_FORCEONFEEDBACK;
   // sprtf("CMD=[%s]"MEOR, pcmd); // output the COMMAND LINE passed
   strcpy(lpb,"CMD=[");
   strcat(lpb,pcmd);
   strcat(lpb,"]"MEOR);
   prt(lpb);
   Add_Zip_Info(lpb);
//        si.cb          = sizeof(STARTUPINFO);
//        si.lpReserved  = NULL;
//        si.lpReserved2 = NULL;
//        si.cbReserved2 = 0;
//        //si.lpTitle = (LPSTR)cmdline; 
//        si.lpTitle     = pcmd; 
//        si.lpDesktop   = (LPTSTR)NULL;
//        si.dwFlags     = STARTF_FORCEONFEEDBACK;

   if( !CreateProcess(NULL,
                        pcmd, // cmdline,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        (LPTSTR)NULL,
                        psi,
                        &pi) )

   {
           LPTSTR lpe = &pe->cErrMsg[0];
           pe->dwError = GetLastError();  // get error indications
           sprintf(lpb, "ERROR: YEEK! Failed to launch zip8 batch!" );
           SBSetTimedTxt(lpb, 45, FALSE );

           sprintf(lpe, "ERROR: YEEK! Failed to launch zip8 batch!"MEOR
              "with CMD[%s]" MEOR "Error id = %d (%#x)",
              pcmd,
              pe->dwError,
              pe->dwError );
           Add_Zip_Info(lpe);
           Add_Zip_Info(MEOR);
           sprtf( MEOR"%s"MEOR, lpe );
           if( pe->bDoMB )
           {
                //MB(hwndClient, "YEEK! Failed to launch zip8 batch!!!",
                MB(hwndClient, lpe, APPNAME, MB_ICONSTOP|MB_OK);
           }
           //goto error;
           if( pe->dwError )
              return pe->dwError;
           else
              return 0x000Dead;
   }
   else
   {
      strcpy(lpb,"Thread entering WaitForSingleObject - INFINITE wait");
      SBSetTimedTxt(lpb, -1, FALSE);
      Add_Zip_Info(lpb);
      Add_Zip_Info(MEOR);
      g_bInZipWait = TRUE;
      // ****************************************************************
      /* wait for completion. (hopefully) thread will be blocked */
      pe->dwWait = WaitForSingleObject(pi.hProcess, INFINITE);
      g_bInZipWait = FALSE;
      // ****************************************************************

      /* close process and thread handles */
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);

      pe->dwError = 0;   // no error
      switch(pe->dwWait)
      {
      case WAIT_ABANDONED:
           pe->dwError = 0x0Abad0ed;
           break;

      case WAIT_OBJECT_0:
           pe->bZipOk = TRUE;
           pe->dwError = ZIP_OK;
           break;

      case WAIT_TIMEOUT:
           pe->dwError = 0x0ffff;
           break;

      case WAIT_FAILED:
           pe->dwError = GetLastError();
           break;
      default:
           pe->dwError = 0x0fffff;
           break;
      }

      ReSetCmdLnStruc( pe ); // get the ZIP COMMAND line components
      sprintf(lpb,"Thread exit WaitForSingleObject %s = %#X ",
         ( pe->bZipOk ? "ok" : "ERROR" ),
         pe->dwError );
      SBSetTimedTxt(lpb, -1, FALSE);
      Add_Zip_Info(lpb);
      Add_Zip_Info(MEOR);
      sprtf("%s"MEOR,lpb);
   }

// error:

//        gmem_free(hHeap, (LPSTR) pe, sizeof(EDITARGS), "do_zipthread" );

   return pe->dwError;  // return the actions

}

LPTSTR   GetPEWaitStg( PEDITARGS pe )
{
   LPTSTR   lpe = GetStgBuf();

   *lpe = 0;

        switch(pe->dwWait)
        {
        case WAIT_ABANDONED:
           strcpy(lpe,"Wait Abandoned ... mutex not signaled");
           break;

        case WAIT_OBJECT_0:
           //pe->bZipOk = TRUE;
           strcpy(lpe, "mutext of object signaled - bZipOk");
           break;

        case WAIT_TIMEOUT:
           strcpy(lpe,"Wait Timeout ... mutex not signaled");
           break;

        case WAIT_FAILED:
           pe->dwError = GetLastError();
           sprintf(lpe,"Wait Error: %d ... mutex not signaled",
              pe->dwError );
           break;
        default:
           sprintf(lpe,"Wait not listed! %d (%#x)",
              pe->dwWait,pe->dwWait);
           break;

        }

   return lpe;

}

PEDITARGS   GetZipMem( PTHREADARGS pta )
{
   COMPLIST    cl;
   PEDITARGS   pe;

   cl = view_getcomplist(current_view);

   if( cl == 0 )
      return 0;

   pe = (PEDITARGS) gmem_get(hHeap, sizeof(EDITARGS), "do_ziplist" );
   if( !pe )
      return 0;

   ZeroMemory(pe, sizeof(EDITARGS));
   pe->view      = current_view;
   //pe->option    = (INCLUDE_SAME | INCLUDE_NEWER | INCLUDE_OLDER );
   pe->option    = INCLUDE_ALLDIFF; // forget SAME, only NEW or OLDER
   pe->option   |= INCLUDE_RIGHTONLY;  // and items that may be DELETED!!! 14Mch2002
   pe->option   |= FULL_NAMES | FRIGHT_NAME | INCLUDE_HEADER;
   //dwo &= ~(FLEFT_NAME | COMBINED_NAME);
   pe->selection = giSelection; // copy the GLOBAL selection row (-1 if NOT)
   if( g_bZipAll )
      pe->pCmd      = szZipCmd;      // get the INI command
   else
   {
      pe->pCmd      = szZipCmd2;      // get the INI command2, with EXCLUSION input file
   }
   //pe->cl        = view_getcomplist(current_view);
   pe->cl        = cl;  // view_getcomplist(current_view);

   pe->pThreadArgs = pta;  // pass on the THREAD ARGUMENT pointer
   // ===============================================================
   // Desc: Process of splitting a single command line into ZIPUP sCmdLn
   SetCmdLnStruc( pe ); // split the ZIP COMMAND line into components
   // ===============================================================
   return pe;
}

INT  WriteZIPList(PTHREADARGS pta, DWORD dwt_NOT_USED ) // #ifdef ADD_ZIP_SUPPORT
{
   LONG        lg;
   BOOL        bRet = 1;
   PEDITARGS   pe;   // = GetZipMem();
   LPTSTR      lpb = &pta->ta_szTmpBuf[0];   // setup a work buffer
   // allocated if called from a THREAD, or GLOBAL if from a command.

   pe = GetZipMem(pta);
   if( !pe )
      return 0;

   pe->pThreadArgs = pta;

   Set_Zip_Info("Current Work Directory: ");
   Add_Zip_Info(GetCWDStg());
   Add_Zip_Info(MEOR);

   lg = do_ziplist( pe );
   if( lg > 0 )
   {
      LPTSTR lps = GetPEWaitStg( pe );
      sprintf(lpb,
         "Zip list done - %ld files. *Mutex* = [%s]",
         lg,
         lps );
      SBSetTimedTxt(lpb, 20, FALSE );
      sprtf( "Zip list done - %ld files."MEOR
         " *Mutex* = [%s]"MEOR, lg,
         lps );
   } else { // if(( lg == 0 ) || ( lg < 0  ) )
      if( lg == 0xf0Dead00 )
      {
         sprintf(lpb, "No ZIP made, since NO overwrites or deletes available!" );
         SBSetTimedTxt(lpb, 20, FALSE );
         sprtf("%s"MEOR,lpb);
      }
      //gmem_free(hHeap, (LPSTR) pe, sizeof(EDITARGS), "do_ziplist" );
      //return 0;
      bRet = 0;
   }

   Add_Zip_Info( lpb );
   Add_Zip_Info( MEOR );

   if( bRet ) {
      if( LaunchZipCmd( pe ) != ZIP_OK )
         bRet = 0;
   }
   if( bRet )
   {
      // success should mean a valid LIST file, and a valid ZIP
      // could TEST for existance of ZIP components
      memcpy( &g_sZAwd_init, pe, sizeof(EDITARGS) ); // keep the RUNTIME arguments
   }

   gmem_free(hHeap, (LPSTR) pe, sizeof(EDITARGS), "do_ziplist" );

   if(g_pZipInfo) {
      MB(hwndClient, g_pZipInfo, APPNAME, MB_ICONINFORMATION|MB_OK);
      MFREE(g_pZipInfo);
      g_dwZipMax = 0;
      g_pZipInfo = 0;
   }

   return bRet;
}


// do_zipup
LONG  do_zipup_NOT_USED( PEDITARGS pe )
{

   PVIEW  view = pe->view;
   int    option = pe->option;
   int    selection = pe->selection;
   LPTSTR   pcmd = &pe->cCmdLine[0];
   COMPITEM item;
//        LPSTR fname;
//   static char cmdline[256];
   int currentline;
//   char * pOut = cmdline;
   char * pOut = pcmd;
//   char * pIn = szZipUp;
   char * pIn = pe->pCmd;  // like "Zip8 -a tempz001.zip @tempz001.lst"
   DWORD             dwi = gdwFileOpts;   // get the WRITE FILE LIST options
   DWORD             dwo;
   //STARTUPINFO          si;
   STARTUPINFO       sSI;
   STARTUPINFO  *    psi = &sSI;
   PROCESS_INFORMATION pi;
   //COMPLIST    cl = view_getcomplist(current_view);
   COMPLIST    cl = pe->cl;
   char *      p;

   p = strchr(pIn,'@');
   if(p)
   {
      p++;
      strcpy(pcmd,p);
   }
   else
   {
      strcpy(pcmd,"tempz001.lst");
   }

   //dwi = gdwFileOpts; // use to when writing OUTLINE list of files
   //if( ( dwi & (INCLUDE_SAME | INCLUDE_NEWER | INCLUDE_OLDER | INCLUDE_LEFTONLY |
   //   INCLUDE_RIGHTONLY ) ) == 0 )
   dwo = ( dwi & (INCLUDE_SAME | INCLUDE_NEWER | INCLUDE_OLDER | INCLUDE_LEFTONLY |
      INCLUDE_RIGHTONLY ) );
   if( dwo == 0 )
   {
      return 0x0Dead000;
   }

   dwo |= FULL_NAMES;
   dwo |= FRIGHT_NAME;
   dwo &= ~(FLEFT_NAME | COMBINED_NAME);

   option = complist_writelist( cl, pcmd, dwo, FALSE, FALSE );
   if( option == 0 )
   {
      return 0x0Dead00;
   }

   *pcmd = 0;

        // NOTE: Returns EXPANDED item if expanded, else
        // only if the 'selection' row is VALID
        item = view_getitem(view, selection);
        if( item == NULL )
        {
//           return -1;
        }

//        fname = compitem_getfilename(item, option);
//        if( 0 == fname ) {
//            MB(hwndClient, LoadRcString(IDS_FILE_DOESNT_EXIST),
//                       APPNAME, MB_ICONSTOP|MB_OK);
//            goto error; }

//       switch ( option )
//        {
            currentline = 1;
//            break;
//        }

        while( *pIn )
        {
            switch( *pIn )
            {
            case '%':
                pIn++;
                switch ( *pIn )
                {
//                case 'p':
//                    strcpy( (LPTSTR)pOut, fname );
//                    while ( *pOut )
//                        pOut++;
//                    break;

                case 'l':
                    _ltoa( currentline, pOut, 10 );
                    while ( *pOut )
                        pOut++;
                    break;

                default:
                    if (IsDBCSLeadByte(*pIn) && *(pIn+1))
                    {
                        *pOut++ = *pIn++;
                    }
                    *pOut++ = *pIn;
                    break;
                }
                pIn++;
                break;

            default:
                if (IsDBCSLeadByte(*pIn) && *(pIn+1))
                {
                    *pOut++ = *pIn++;
                }
                *pOut++ = *pIn++;
                break;
            }
        }


        /* Launch the process and waits for it to complete */
        ZeroMemory(psi, sizeof(STARTUPINFO));
        psi->cb          = sizeof(STARTUPINFO);
        //psi->lpReserved  = NULL;
        //psi->lpReserved2 = NULL;
        //psi->cbReserved2 = 0;
        //si.lpTitle = (LPSTR)cmdline; 
        psi->lpTitle     = pcmd; 
        //psi->lpDesktop   = (LPTSTR)NULL;
        psi->dwFlags     = STARTF_FORCEONFEEDBACK;

//        si.cb          = sizeof(STARTUPINFO);
//        si.lpReserved  = NULL;
//        si.lpReserved2 = NULL;
//        si.cbReserved2 = 0;
//        //si.lpTitle = (LPSTR)cmdline; 
//        si.lpTitle     = pcmd; 
//        si.lpDesktop   = (LPTSTR)NULL;
//        si.dwFlags     = STARTF_FORCEONFEEDBACK;

        if( !CreateProcess(NULL,
                        pcmd, // cmdline,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        (LPTSTR)NULL,
                        psi,
                        &pi))
        {
           LPTSTR lpe = &pe->cErrMsg[0];
           pe->dwError = GetLastError();  // get error indications
           sprintf(lpe, "ERROR: YEEK! Failed to launch zip8 batch!"MEOR
              "with CMD[%s]" MEOR "Error id = %d (%#x)",
              pcmd,
              pe->dwError,
              pe->dwError );
           sprtf( MEOR"%s"MEOR, lpe );
           if( pe->bDoMB )
           {
                //MB(hwndClient, "YEEK! Failed to launch zip8 batch!!!",
                MB(hwndClient, lpe, APPNAME, MB_ICONSTOP|MB_OK);
           }
           goto error;
        }

        /* wait for completion. */
        WaitForSingleObject(pi.hProcess, INFINITE);

        /* close process and thread handles */
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        /* finished with the filename. deletes it if it was a temp. */
        //compitem_freefilename(item, option, fname);

        /*
         * refresh cached view always .  A common trick is to edit the
         * composite file and then save it as a new left or right file.
         * Equally the user can edit the left and save as a new right.
         */

        /* We want to force both files to be re-read, but it's not a terribly
         * good idea to throw the lines away on this thread.  Someone might
         * be reading them on another thread!
         */
        /* file_discardlines(compitem_getleftfile(item)) */
        /* file_discardlines(compitem_getrightfile(item)) */

        /* force the compare to be re-done */
        //PostMessage(hwndClient, WM_COMMAND, IDM_UPDATE, (LONG)item);

error:

        gmem_free(hHeap, (LPSTR) pe, sizeof(EDITARGS), "do_zipthread" );

        return 0x0Face000;

}

#if   0

void do_zipthread(PVIEW view, int option)
{
   PEDITARGS   pe;
   HANDLE      thread;
   CC          sCC;
   PCC         pcc = &sCC;
//   DWORD       dwi = gdwFileOpts;   // get the WRITE FILE LIST options
   DWORD       dwi = (DWORD)option;   // combined with outline_include
   COMPLIST    cl = view_getcomplist(view);

   if( !cl )
      return;

   //if(dwi)
   {
      // just want to write out a list of names
      // options = gdwFileOpts
      ZeroMemory(pcc,sizeof(CC));   // zero the memory block
      pcc->bUseOpt = TRUE;
      pcc->dwopts  = dwi;
      complist_countlistable( pcc );
      if( pcc->cnt2 == 0 )
      {
         return;  // nothing to do
      }
   }
   sprtf( "Starting ZIPUP of %d items ..."MEOR, pcc->cnt2 );

        pe = (PEDITARGS) gmem_get(hHeap, sizeof(EDITARGS), "do_zipthread" );
        ZeroMemory(pe, sizeof(EDITARGS));
        pe->view      = view;
        pe->option    = option;
        pe->selection = giSelection; // copy the GLOBAL selection row (-1 if NOT)
        pe->pCmd      = szZipCmd;
        pe->cl        = cl;
        //pe->bDoMB     = FALSE;
        thread = NULL;
#ifdef   USETHDTD2
        thread = CreateThread( NULL,
           0,
           (LPTHREAD_START_ROUTINE)do_zipup,
           (LPVOID) pe,
           0,
           &threadid );
#endif   // USETHDTD2
        if (thread == NULL)
        {
                /* The createthread failed, do without the extra thread - just
                 * call the function synchronously
                 */
                 do_zipup(pe);
        }
        else
        {
           CloseHandle(thread);  // get thread rolling
           Sleep(10);   // and some CPU
        }
} /* do_zipthread */

#endif   // 0


INT_PTR  Do_ID_FILE_CREATEZIPFILE( VOID ) // #ifdef ADD_ZIP_SUPPORT
{
   INT_PTR iret = Do_IDM_EDITZIPUP();
   return iret;
}

///////////////////////////////////////////////////////////////////
#endif // #ifdef ADD_ZIP_SUPPORT  // ZIP file support

// ******************************
// eof - dc4wZip.c
