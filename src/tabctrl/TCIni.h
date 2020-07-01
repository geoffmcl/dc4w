
// TCIni.h
#ifndef	_TCIni_HH
#define	_TCInit_HH


#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

extern   VOID  ReadINI( LPTSTR lpini );
extern   VOID  WriteINI( LPTSTR lpini, BOOL bNoReset );
extern   BOOL  SetChgAll( BOOL bChg );
extern   BOOL  InitFixedWork( VOID );
extern   VOID  WriteTCINI( VOID );

#ifdef  __cplusplus
}
#endif  // __cplusplus


#endif   // #ifndef	_TCIni_HH
// eof - TCIni.h
