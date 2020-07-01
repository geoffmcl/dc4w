

// Browse.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef  _Browse_HH
#define  _Browse_HH

#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

extern   BOOL  GetNewFileName( HWND hWnd, LPTSTR lpFile, LPTSTR lpRes,
                     BOOL bChkExist );
extern   BOOL FgmGetFil2( HWND hWnd, LPTSTR lpFilNam, LPTSTR lpRes );
extern   BOOL FgmGetDir2( HWND hWnd, LPTSTR lpDir, LPTSTR lpPath );

#ifdef  __cplusplus
// extern  "C"
}
#endif  // __cplusplus


#endif   // _Browse_HH
// eof - Browse.h


