
/****************************** Module Header *******************************
* Module Name: GMEM.C
*
* Memory utility functions.
*
* Functions:
*
* gmem_panic()
* gmem_init()
* gmem get()
* gmem free()
* gmem_freeall()
*
* Comments:
*
* Global heap functions - allocate and free many small
* pieces of memory by calling global alloc for large pieces
* and breaking them up. A heap contains a critical section, so
* multiple simultaneous calls to gmem get and gmem free will be
* protected.
*
* gmem_freeall should not be called until all other users have finished
* with the heap.
*
* Out-of-memory is not something we regard as normal.
* If we cannot allocate memory - we put up an abort-retry-ignore
* error, and only return from the function if the user selects ignore.
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"

int gmem_panic(void);

#ifdef   NDEBUG
#undef   SHOWMEM2
#else
#define  SHOWMEM2
#endif   // NDEBUG y/n

/* ensure BLKSIZE is multiple of sizeof(DWORD) */
#define BLKSIZE         64               /* blk size in bytes */
#define ALLOCSIZE       32768
#define NBLKS           (ALLOCSIZE / BLKSIZE)
#define MAPSIZE         (NBLKS / 8)
#define MAPLONGS        (MAPSIZE / sizeof(DWORD))
#define TO_BLKS(x)      (((x) + BLKSIZE - 1) / BLKSIZE)

typedef struct seghdr {
        HANDLE hseg;
        CRITICAL_SECTION critsec;
        struct seghdr FAR * pnext;
        long nblocks;
        DWORD segmap[MAPLONGS];
} SEGHDR, FAR * SEGHDRP;


/* anything above this size, we alloc directly from global heap */
#define MAXGALLOC       20000

#ifdef   SHOWMEM2
#define  BIGMEMSHOW

// output memory statistics
#define     mf_InUse    0x00000001
#define     mf_Shown    0x00000002
#define     mf_Max      0x00000004

#define     mf_NoHand   0x10000000
#define     mf_NoMem    0x20000000
#define     mf_NoCnt    0x40000000
#define     mf_NoLen    0x80000000

typedef struct _MEMS {
   int      m_flg;
   int      m_len;
   int      m_cnt;
   int      m_use;
   int      m_us2;
#ifdef   BIGMEMSHOW
   TCHAR    m_nm[264];
#endif   // BIGMEMSHOW
}MEMS, * PMEMS;

//#define  MXMEMS      2000
//#define  MXMEMS      4000
//#define  MXMEMS      8000
#define  MXMEMS      16000
#define  MXHEAPS     2
//#define  GETILEN(a)      ( ( a + 15 ) & 0xfffffff0 )
#define  GETILEN(a)      a

int      iMemHeaps   = 0;
DWORD    dwMemErr    = 0;
PMEMS    g_psMems[MXHEAPS];
HANDLE   hHeaps[MXHEAPS];
DWORD    iMemMax[MXHEAPS];
int      iMemCnt[MXHEAPS];
int      iMemVerb = 0;
BOOL        gfSort = TRUE;
BOOL        gfStLen = TRUE;
LIST_ENTRY  pErrLst[MXHEAPS];

void  add2mems( HANDLE hHeap, LPTSTR lps, int ilen, LPTSTR lpn )
{
   DWORD dwi;
   LPTSTR   lpd;
   PLE      pn;
   for( dwi = 0; dwi < MXHEAPS; dwi++ )
   {
      if( hHeaps[dwi] == hHeap )
         break;
   }
   if( dwi < MXHEAPS )
   {
      PMEMS pm  = g_psMems[dwi];   // get this block
      DWORD dwm = iMemCnt[dwi];  // and count of blocks filled
      INT   i   = GETILEN(ilen); // round up the length
      DWORD dwj;
      if( i == 16 )
         dwj = 0;
      for( dwj = 0; dwj < dwm; dwj++ )
      {
         if( pm->m_len == i )
         {
            // already have this SIZE
#ifdef   BIGMEMSHOW
            if( strcmp( &pm->m_nm[0], lpn ) == 0 )
            {
               pm->m_cnt++;   // in use counter
               pm->m_use++;   // used counter
               break;
            }
#else    // !BIGMEMSHOW
            pm->m_cnt++;   // in use counter
            pm->m_use++;   // used counter
            break;
#endif   // BIGMEMSHOW y/n
         }
         pm++;
      }
      if( dwj == dwm )
      {
         // a NEW SIZE
         if( dwm >= iMemMax[dwi] )
         {
            PMEMS  pm2;
            DWORD  dwm2 = dwm + MXMEMS;   // bump the count
            pm2 = LocalReAlloc( pm, // handle to local memory object
                              (dwm2 * sizeof(MEMS)), // new block size
                              LPTR );
            if( pm2 )
            {
               // switch to the NEW pointer
               g_psMems[dwi] = pm2;    // update the old pointer,
               iMemMax[dwi] = dwm2;  // and its count of blocks
               pm = &pm2[dwm];      // get an updated pointer
            } else {
               chkme( "C:ERROR: add2mems: REALLOCATION: From %d to %d.", dwm, dwm2 );
            }
         }
         if( dwm < iMemMax[dwi] )
         {
            pm->m_cnt = 1; // start use count of this length for this heap
            pm->m_len = i; // keep THIS length
            //pm->m_flg = 0; // set the flag at zero
            pm->m_use = 1; // and use count at 1
#ifdef   BIGMEMSHOW
            strcpy( &pm->m_nm[0], lpn );
#endif   // BIGMEMSHOW
            iMemCnt[dwi]++;   // bump the count of used blocks (ie blocks with lengths)
         }
         else
         {
            lpd = &g_szBuf2[0];
            sprintf( lpd, "C:ERROR: add2mems: OOPS: Reallocate of MORE memory for FAILED!!!" );
            dwMemErr |= mf_NoMem;
            pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(lpd) + 1) );
            if(pn)
            {
               strcpy( (LPTSTR)((PLE)pn + 1), lpd );
               InsertTailList( &pErrLst[dwi], pn );
            }
            chkme(lpd);
         }
      }
   }
   else
   {
      lpd = &g_szBuf2[0];
      sprintf( "C:ERROR: add2mems: MEM ERROR 1: Handle %#x does NOT exist!", hHeap );
      pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(lpd) + 1) );
      if(pn)
      {
         strcpy( (LPTSTR)((PLE)pn + 1), lpd );
         InsertTailList( &pErrLst[0], pn );
      }
      dwMemErr |= mf_NoHand;
      chkme(lpd);
   }
}

void  sub2mems( HANDLE hHeap, LPTSTR lps, int ilen, LPTSTR lpn )
{
   DWORD    dwi;
   LPTSTR   lpd;
   PLE      pn;
   for( dwi = 0; dwi < MXHEAPS; dwi++ )
   {
      if( hHeaps[dwi] == hHeap )
         break;
   }
   if( dwi < MXHEAPS )
   {
      PMEMS pm  = g_psMems[dwi];   // get this block
      DWORD dwm = iMemCnt[dwi];  // and count of blocks filled
      INT   i   = GETILEN(ilen); // round up the length
      DWORD dwj;
      for( dwj = 0; dwj < dwm; dwj++ )
      {
         if( pm->m_len == i )    // has to have the SAME size
         {
#ifdef   BIGMEMSHOW
            if( strcmp( &pm->m_nm[0], lpn ) )   // has to have SAME name
            {
               pm++;
               continue;
            }
#endif   // BIGMEMSHOW
            if( pm->m_cnt )
               pm->m_cnt--;   // reduce use count
            else
            {
               PLE   ph = &pErrLst[dwi];
               lpd = &g_szBuf2[0];
               sprintf( lpd, "MEMERR: Free of h=%p len=%d with NO count!"MEOR,
                  hHeap, i );
               Traverse_List( ph, pn )
               {
                  if( strcmp( (LPTSTR)((PLE)pn + 1), lpd ) == 0 )
                  {
                     *lpd = 0;
                     break;
                  }
               }
               if( *lpd )
               {
                  pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(lpd) + 1) );
                  if(pn)
                  {
                     strcpy( (LPTSTR)((PLE)pn + 1), lpd );
                     InsertTailList( ph, pn );
                  }
               }
               pm->m_flg |= mf_NoCnt;  // set the NO COUNT - Freeing something
               // that is already freed!!!
               dwMemErr  |= mf_NoCnt;
            }
            break;
         }
         pm++;
      }
      if( dwj == dwm )
      {
         lpd = &g_szBuf2[0];
#ifdef   BIGMEMSHOW
         sprintf( lpd, "MEMERR: Free of h=%p len=%d name=[%s] does NOT exist"MEOR,
            hHeap, i, lpn );
#else // !BIGMEMSHOW
         sprintf( lpd, "MEMERR: Free of h=%p len=%d does NOT exist"MEOR,
            hHeap, i );
#endif // BIGMEMSHOW y/n
         pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(lpd) + 1) );
         if(pn)
         {
            strcpy( (LPTSTR)((PLE)pn + 1), lpd );
            InsertTailList( &pErrLst[dwi], pn );
         }
         dwMemErr |= mf_NoLen;
         chkme(lpd);
      }
   }
   else
   {
      lpd = &g_szBuf2[0];
      sprintf( "MEM ERROR 2: Handle %#x does NOT exist!", hHeap );
      pn = (PLE)MALLOC( (sizeof(LIST_ENTRY) + strlen(lpd) + 1) );
      if(pn)
      {
         strcpy( (LPTSTR)((PLE)pn + 1), lpd );
         InsertTailList( &pErrLst[0], pn );
      }
      dwMemErr |= mf_NoHand;
      chkme(lpd);
   }
}

DWORD  AddCString( LPTSTR lpd, PMEMS pm )
{
   DWORD dwr = 0;
   if( pm->m_cnt )
   {
      // we have an OUTSTANDING count on this item
      dwr++;
#ifdef   BIGMEMSHOW
      if( *lpd )
      {
         sprtf( "%s"MEOR, lpd );
         *lpd = 0;
      }
#endif   // BIGMEMSHOW
      sprintf(EndBuf(lpd), "%5u %5u %5u", pm->m_len, pm->m_cnt, pm->m_use );
#ifdef   BIGMEMSHOW
      strcat( lpd, " " );
      strcat( lpd, &pm->m_nm[0]);
      sprtf( "%s"MEOR, lpd );
      *lpd = 0;
#endif   // BIGMEMSHOW
   }
   else if( pm->m_flg & mf_NoCnt )
   {
      // this memory was freed TOO many times
      dwr++;
#ifdef   BIGMEMSHOW
      if( *lpd )
      {
         sprtf( "%s"MEOR, lpd );
         *lpd = 0;
      }
#endif   // BIGMEMSHOW
      sprintf(EndBuf(lpd), "%5u  Errs %5u", pm->m_len, pm->m_use );
#ifdef   BIGMEMSHOW
      strcat( lpd, " " );
      strcat( lpd, &pm->m_nm[0]);
      sprtf( "%s"MEOR, lpd );
      *lpd = 0;
#endif   // BIGMEMSHOW
   }
   else
   {
      sprintf(EndBuf(lpd), "%5u  Free %5u", pm->m_len, pm->m_use );
   }
   return dwr;
}

void  memshow( HANDLE hHeap )
{
   DWORD    dwi;
   LPTSTR   lpd = &gszTmpBuf[0];
   for( dwi = 0; dwi < MXHEAPS; dwi++ )
   {
      if( hHeaps[dwi] == hHeap )
         break;
   }
   if( dwi < MXHEAPS )
   {
      PMEMS    pm  = g_psMems[dwi];   // get this block
      DWORD    dwm = iMemCnt[dwi];  // and count of blocks filled
      DWORD    dwj, dwk, dwl;
      INT      i;
      PLE      ph, pn;
      DWORD    dwec   = 0;    // error count
      BOOL     bDnHdr = FALSE;
      DWORD    dwMax, dwMin;

      if( !pm )
      {
         sprtf( "MEMERR: Allocation already freed!"MEOR );
         return;
      }
      sprtf( "MEM: gmem_freeall (h=0x%x) Total %d sizes."MEOR,
         hHeap,
         dwm );
      ph = &pErrLst[dwi];

      if( dwMemErr )
         dwec = 1;
      else
         dwec = 0;

      Traverse_List( ph, pn )
      {
         sprtf( (LPTSTR)((PLE)pn + 1) );
         dwec++;
      }
      *lpd = 0;
      dwMax = 0;
      dwMin = -1;
      for( dwj = 0; dwj < dwm; dwj++ )
      {
         pm->m_flg &= ~(mf_Shown);  // clear out the SHOWN flag
         pm->m_us2  = 0;            // and RANK
         if( (DWORD)pm->m_len > dwMax )
            dwMax = (DWORD)pm->m_len;
         if( (DWORD)pm->m_len < dwMin )
            dwMin = (DWORD)pm->m_len;

         pm++;
      }
      pm  = g_psMems[dwi];   // get this block again
      dwk = 0;             // start RANK
      while( dwk < dwm )
      {
         pm  = g_psMems[dwi];   // get this block again
         i = 0;   // and start lenght
         for( dwj = 0; dwj < dwm; dwj++ )
         {
            if( pm->m_us2 == 0 ) // if NOT already RANKED
            {
               if( gfStLen )
               {
                  if( pm->m_len > i )
                  {
                     i = pm->m_len;
                     dwl = dwj;
                  }
               }
               else  // !gfStLen
               {
                  if( pm->m_use > i )
                  {
                     i = pm->m_use;
                     dwl = dwj;
                  }
               }
            }
            pm++;
         }

         dwk++;               // bump the RANK
         pm  = g_psMems[dwi];   // get this block again
         pm[dwl].m_us2 = dwk; // set the RANK for this item
      }

      if( dwec )
      {
         if( !bDnHdr )
         {
            sprtf( " Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use"MEOR );
            bDnHdr = TRUE;
         }
      }
      pm  = g_psMems[dwi];   // get this block again
      *lpd = 0;
      if( gfSort )
      {
         dwk = 0;
         dwl = 1;
         while( dwk < dwm )
         {
            pm  = g_psMems[dwi];   // get this block again
            for( dwj = 0; dwj < dwm; dwj++ )
            {
               if( pm->m_us2 == (int)dwl )
                  break;
               pm++;
            }
            dwec += AddCString( lpd, pm );
            if( strlen(lpd) > 63 )
            {
               if( dwec )
               {
                  if( !bDnHdr )
                  {
                     sprtf( " Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use"MEOR );
                     bDnHdr = TRUE;
                  }
                  sprtf( "%s"MEOR, lpd );
               }
               *lpd = 0;
            }
            dwk++;
            dwl++;
         }
      }
      else
      {
         pm  = g_psMems[dwi];   // get this block again
         for( dwj = 0; dwj < dwm; dwj++ )
         {
            dwec += AddCString(lpd, pm);
            if( strlen(lpd) > 63 )
            {
               //sprtf( "%s"MEOR, lpd );
               if( dwec )
               {
                  if( !bDnHdr )
                  {
                     sprtf( " Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use"MEOR );
                     bDnHdr = TRUE;
                  }
                  sprtf( "%s"MEOR, lpd );
               }
               *lpd = 0;
            }
            pm++;
         }
      }

      if( *lpd )
      {
         if( dwec )
         {
            if( !bDnHdr )
            {
               sprtf( " Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use Size   Cnt   Use"MEOR );
               bDnHdr = TRUE;
            }
            sprtf( "%s"MEOR, lpd );
         }
      }
      if( dwMemErr )
      {
         sprintf(lpd, "Have %#x errors [", dwMemErr );
         if( dwMemErr & mf_NoHand )
         {
            strcat(lpd, "NoHand");
            dwMemErr &= ~(mf_NoHand);
         }
         if( dwMemErr & mf_NoMem )
         {
            strcat(lpd, "NoMem");
            dwMemErr &= ~(mf_NoMem);
         }
         if( dwMemErr & mf_NoCnt )
         {
            strcat(lpd, "NoCnt");
            dwMemErr &= ~(mf_NoCnt);
         }
         if( dwMemErr & mf_NoLen )
         {
            strcat(lpd, "NoLen");
            dwMemErr &= ~(mf_NoLen);
         }
         strcat(lpd, "]"MEOR);
         sprtf(lpd);
      }
      else if( dwec == 0 )
      {
         sprtf( "MEM: No errors in range %d to %d bytes allocated and freed."MEOR,
            dwMin,
            dwMax );
      }

      g_psMems[dwi] = 0;  // kill this list
      KillLList( ph );
   }
   else
   {
      sprintf(lpd, "MEM ERROR 3: Handle %p does NOT exist!"MEOR, hHeap );
      dwMemErr |= mf_NoHand;
      chkme(lpd);
   }
}

VOID  InitMemStats( SEGHDRP hp, HANDLE hNew, LPTSTR lpn )
{
   DWORD  dwi = 0;
   sprtf( "MEM: Init: Allocate %d of memory at 0x%x.(h=0x%x for %s)"MEOR,
              ALLOCSIZE,
              hp,
              hNew,
              lpn );
   if( iMemHeaps < MXHEAPS )
   {
      PMEMS pm = (PMEMS)LocalAlloc( LPTR, (sizeof(MEMS) * MXMEMS) );
      if( pm )
      {
         dwi = iMemHeaps;
         g_psMems[dwi] = pm;
         ZeroMemory( pm, (sizeof(MEMS) * MXMEMS) );
         iMemMax[dwi] = MXMEMS;
         hHeaps[dwi]  = hNew;
         iMemCnt[dwi] = 0;
         InitLList( &pErrLst[dwi] );
         iMemHeaps++;
      }
   }
}

#endif   // SHOWMEM2

/***************************************************************************
 * Function: gmem_init
 *
 * Purpose:
 *
 * init heap - create first segment
 */
HANDLE APIENTRY
gmem_init( LPTSTR lpn )
{
        HANDLE hNew;
        SEGHDRP hp;

        /* retry all memory allocations after calling gmem_panic */
        do {
                hNew = GlobalAlloc(GHND, ALLOCSIZE);
                if (hNew == NULL) {
                        if (gmem_panic() == IDIGNORE) {
                                return(NULL);
                        }
                }
        } while  (hNew == NULL);

        hp = (SEGHDRP) GlobalLock(hNew);
        if (hp == NULL) {
                return(NULL);
        }
        hp->hseg = hNew;
        InitializeCriticalSection(&hp->critsec);
        hp->pnext = NULL;
        gbit_init(hp->segmap, NBLKS);
        gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
        hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));
#ifdef   SHOWMEM2
        InitMemStats( hp, hNew, lpn );
#endif   // SHOWMEM2
        return(hNew);
}

/***************************************************************************
 * Function: gmem_get
 *
 * Purpose:
 *
 * Get memory from heap
 */
LPSTR APIENTRY
gmem_get(HANDLE hHeap, int len, LPTSTR lpn)
{
        SEGHDRP chainp;
        HANDLE hNew;
        SEGHDRP hp;
        LPSTR chp;
        long nblks;
        long start;
        long nfound;


        /* the heap is always locked (in gmem_init)- so having got the
         * pointer, we can always safely unlock it
         */
        chainp = (SEGHDRP) GlobalLock(hHeap);
        GlobalUnlock(hHeap);

        if (len < 1) {
                return(NULL);
        }

        /*
         * too big to be worth allocing from heap - get from globalalloc
         */
        if (len > MAXGALLOC) {
                /* retry all memory allocations after calling gmem_panic */
                do {
                        hNew = GlobalAlloc(GHND, len);
                        if (hNew == NULL) {
                                if (gmem_panic() == IDIGNORE) {
                                        return(NULL);
                                }
                        }
                } while  (hNew == NULL);

                chp = GlobalLock(hNew);
                if (chp == NULL) {
                        return(NULL);
                }
#ifdef   SHOWMEM2
               add2mems(hHeap,chp,len,lpn);
#endif   // SHOWMEM2
                return(chp);
        }


        /*
         * get critical section during all access to the heap itself
         */
        EnterCriticalSection(&chainp->critsec);

        nblks = TO_BLKS(len + sizeof(HANDLE));

        for (hp = chainp; hp !=NULL; hp = hp->pnext) {
                if (hp->nblocks >= nblks) {
                        nfound = gbit_findfree(hp->segmap, nblks,NBLKS, &start);
                        if (nfound >= nblks) {
                                gbit_alloc(hp->segmap, start, nblks);
                                hp->nblocks -= nblks;

                                /* convert blocknr to pointer
                                 * store seg handle in block
                                 */
                                chp = (LPSTR) hp;
                                chp = &chp[ (start-1) * BLKSIZE];
                                * ( (HANDLE FAR *) chp) = hp->hseg;
                                chp += sizeof(HANDLE);

                                break;
                        }
                }
        }
        if (hp == NULL) {
                /* retry all memory allocations after calling gmem_panic */
                do {
                        hNew = GlobalAlloc(GHND, ALLOCSIZE);
                        if (hNew == NULL) {
                                if (gmem_panic() == IDIGNORE) {
                                        LeaveCriticalSection(&chainp->critsec);
                                        return(NULL);
                                }
                        }
                } while  (hNew == NULL);

                hp = (SEGHDRP) GlobalLock(hNew);
                if (hp == NULL) {
                        LeaveCriticalSection(&chainp->critsec);
                        return(NULL);
                }
                hp->pnext = chainp->pnext;
                hp->hseg = hNew;
                chainp->pnext = hp;
                gbit_init(hp->segmap, NBLKS);
                gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
                hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));
                nfound = gbit_findfree(hp->segmap, nblks, NBLKS, &start);
                if (nfound >= nblks) {
                        gbit_alloc(hp->segmap, start, nblks);
                        hp->nblocks -= nblks;

                        /* convert block nr to pointer */
                        chp = (LPSTR) hp;
                        chp = &chp[ (start-1) * BLKSIZE];
                        /* add a handle into the block and skip past */
                        * ( (HANDLE FAR *) chp) = hp->hseg;
                        chp += sizeof(HANDLE);
                }
        }
        LeaveCriticalSection(&chainp->critsec);
        memset(chp, 0, len);
#ifdef   SHOWMEM2
        add2mems(hHeap,chp,len,lpn);
#endif   // SHOWMEM2
        return(chp);
}

/***************************************************************************
 * Function: gmem_free
 *
 * Purpose:
 *
 * Free memory alloced
 */
void APIENTRY
gmem_free(HANDLE hHeap, LPSTR ptr, int len, LPTSTR lpn )
{
        SEGHDRP chainp;
        SEGHDRP hp;
        HANDLE hmem;
        long nblks, blknr;
        LPSTR chp;

        if (len < 1) {
                return;
        }

        /*
         * allocs greater than MAXGALLOC are too big to be worth
         * allocing from the heap - they will have been allocated
         * directly from globalalloc
         */
        if (len > MAXGALLOC) {
                hmem = GlobalHandle( (LPSTR) ptr);
                GlobalUnlock(hmem);
                GlobalFree(hmem);
#ifdef   SHOWMEM2
                sub2mems(hHeap,ptr,len,lpn);
#endif   // SHOWMEM2
                return;
        }

        chainp = (SEGHDRP) GlobalLock(hHeap);
        EnterCriticalSection(&chainp->critsec);


        /* just before the ptr we gave the user, is the handle to
         * the block
         */
        chp = (LPSTR) ptr;
        chp -= sizeof(HANDLE);
        hmem = * ((HANDLE FAR *) chp);
        hp = (SEGHDRP) GlobalLock(hmem);

        nblks = TO_BLKS(len + sizeof(HANDLE));

        /* convert ptr to block nr */
        blknr = TO_BLKS( (unsigned) (chp - (LPSTR) hp) ) + 1;

        gbit_free(hp->segmap, blknr, nblks);
        hp->nblocks += nblks;

        GlobalUnlock(hmem);

        LeaveCriticalSection(&chainp->critsec);

        GlobalUnlock(hHeap);

#ifdef   SHOWMEM2
        sub2mems(hHeap,ptr,len,lpn);
#endif   // SHOWMEM2

}

/***************************************************************************
 * Function: gmem_freeall
 *
 * Purpose:
 *
 * Free heap
 */
void APIENTRY
gmem_freeall(HANDLE hHeap)
{
        SEGHDRP chainp;
        HANDLE hSeg;

        chainp = (SEGHDRP) GlobalLock(hHeap);
        /* this segment is always locked - so we need to unlock
         * it here as well as below
         */
        GlobalUnlock(hHeap);

        /* finished with the critical section  -
         * caller must ensure that at this point there is no
         * longer any contention
         */
        DeleteCriticalSection(&chainp->critsec);

        while (chainp != NULL) {
                hSeg = chainp->hseg;
                chainp = chainp->pnext;
                GlobalUnlock(hSeg);
                GlobalFree(hSeg);
        }

#ifdef   SHOWMEM2
         memshow( hHeap );
#endif   // SHOWMEM2
}

/***************************************************************************
 * Function: gmem_panic
 *
 * Purpose:
 *
 * A memory allocation attempt has failed. Return IDIGNORE to ignore the
 * error and return NULL to the caller, and IDRETRY to retry the allocation
 * attempt.
 */
int
gmem_panic(void)
{
        int code;
    	extern HANDLE hLibInst;
	TCHAR szBuf1[512];
    	TCHAR szBuf2[512];

        LoadString(hLibInst, IDS_MEMORY_ALLOC_FAIL, szBuf1, sizeof(szBuf1));
        LoadString(hLibInst, IDS_OUT_OF_MEMORY, szBuf2, sizeof(szBuf2));

        code = MB(NULL, szBuf1, szBuf2,
                        MB_ICONSTOP|MB_ABORTRETRYIGNORE);
        if (code == IDABORT) {
                /* abort this whole process */
                ExitProcess(1);
        }

        return(code);
}

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - gmem.c
