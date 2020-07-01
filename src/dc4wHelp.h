

// dc4wHelp.h
#ifndef  _dc4wHelp_HH

#define  MXARGS   64    // should be ENOUGH

typedef struct tagPISTR {
   HANDLE   hFile;
   INT      iCnt;
   LPTSTR   pBuf;
   LPTSTR * pArgs;
}PISTR, * PPISTR;

extern   int dc4w_usage(LPTSTR msg);
extern   int  ProcessArgs( PTHREADARGS ta, int argc, char * * argv );
extern   BOOL  Getpis( PPISTR ppis, LPTSTR lpf );

#endif   // _dc4wHelp_HH
// eof - dc4wHelp.h

