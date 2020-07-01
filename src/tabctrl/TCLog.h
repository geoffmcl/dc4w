
// TCLog.h
#ifndef	_TCLog_HH
#define	_TCLog_HH


#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

#ifndef  MEOR
#define  MEOR  "\r\n"
#define  VFH(a)   ( a && ( a != INVALID_HANDLE_VALUE ) )
#define  PLE   PLIST_ENTRY
#endif   // MEOR

extern   VOID  OpenDiagFile( VOID );
extern   VOID  CloseDiagFile( VOID );
extern   int   _cdecl sprtf( LPTSTR lpf, ... );
extern   int   _cdecl chkme( LPTSTR lpf, ... );

extern   VOID  GetModulePath( LPTSTR lpb );

#ifdef  __cplusplus
}
#endif  // __cplusplus


#endif   // #ifndef	_TCLog_HH
// eof - TCLog.h
