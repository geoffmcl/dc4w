// browse2.c

#include "dc4w.h"
// gmGetFil.c
//#include	<windows.h>
#include	<shlobj.h>
#include <objidl.h>
//#include	"gmGetFil.h"

/*  F  G E T  F I L E  N A M E */
/*----------------------------------------------------------------------------
    %%Function: FgmGetFil
OPENFILENAME g_ofnOpen  = {0};
TCHAR g_szFileOpen[264] = "\0";
TCHAR g_szFilterOpen[]  = "All\0*.*\0Text\0*.TXT\0";
INT   g_iIndexOpen      = 1;
 
----------------------------------------------------------------------------*/
extern OPENFILENAME g_ofnOpen;
extern TCHAR g_szFilterOpen[]; // = "All\0*.*\0Text\0*.TXT\0";

TCHAR g_szTitleOpen[] = "Choose a File Name";

//BOOL FGetFileName( LPSTR szFileName )
//BOOL FgmGetFil2( HWND hWnd, HINSTANCE hInst, LPTSTR lpFilt, LPTSTR lpTitle,
//                LPTSTR lpFilNam )
BOOL FgmGetFil2( HWND hWnd, LPTSTR lpFilNam, LPTSTR lpRes )
{
	OPENFILENAME * pofn = &g_ofnOpen;
   LPTSTR lpFilt = &g_szFilterOpen[0]; // get OPEN filter
   LPTSTR lpTitle = &g_szTitleOpen[0];

//	SetEmptySz(szFileName);
//	ClearStruct(&ofn);
//	*lpFilNam = 0;
	ZeroMemory( pofn, sizeof(OPENFILENAME) );

	pofn->lStructSize   = sizeof(OPENFILENAME);
//	ofn.hwndOwner     = ghwndMain;
	pofn->hwndOwner     = hWnd;
	pofn->hInstance     = g_hInst;
//	pofn->hInstance     = hInst;
//	ofn.lpstrFilter   = _szFilter;
	pofn->lpstrFilter   = lpFilt;
	pofn->nFilterIndex  = 1L;
//	ofn.lpstrFile     = (LPSTR) szFileName;
	pofn->lpstrFile     = lpFilNam;
	pofn->nMaxFile      = MAX_PATH; // really sizeof(szFileName)
//	ofn.lpstrTitle    = "Pick a file to send";
	pofn->lpstrTitle    = lpTitle;
	pofn->lpstrInitialDir = NULL;

	pofn->Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

	if( !GetOpenFileName(pofn) )
	{
		//return fFalse;
		return FALSE;
	}

   if( lpRes )
   {
      strcpy(lpRes, pofn->lpstrFile);  // = lpFilNam;
   }

//	return fTrue;
	return TRUE;

}



/*  F  G E T  D I R E C T O R Y */
/*----------------------------------------------------------------------------
    %%Function: FgmGetDir
 uses -
 typedef struct _browseinfo { 
    HWND          hwndOwner; 
    LPCITEMIDLIST pidlRoot; 
    LPTSTR        pszDisplayName; 
    LPCTSTR       lpszTitle; 
    UINT          ulFlags; 
    BFFCALLBACK   lpfn; 
    LPARAM        lParam; 
    int           iImage;

} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO; 
----------------------------------------------------------------------------*/
// forward reference
int CALLBACK BrowseCBProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );
TCHAR g_szDir[264] = "\0";
LPTSTR   g_lpDir = &g_szDir[0];

//BOOL FGetDirectory( HWND hWnd, LPSTR szDir )
BOOL FgmGetDir2( HWND hWnd, LPTSTR lpDir, LPTSTR lpPath )
{
	BOOL  fRet = FALSE;
//	CHAR  szPath[MAX_PATH];
	LPITEMIDLIST pidl;
	LPITEMIDLIST pidlRoot;
	LPMALLOC lpMalloc;
   HRESULT  hRes;
	BROWSEINFO bi = {
		hWnd,
		NULL,
		lpDir,
		"Select a Directory",
		BIF_RETURNONLYFSDIRS,
		BrowseCBProc,
      0L,
      0	};

   if( *g_lpDir == 0 )
   {
      strcpy(g_lpDir, lpDir);    // keep our START directory
   }
   else if( *lpDir )
   {
      if( strcmpi(g_lpDir, lpDir) )
      {
         strcpy(g_lpDir, lpDir); // set new ACTIVE
      }
   }

//	szPath[0] = 0;
	*lpPath = 0;
	pidl = pidlRoot = 0;
   hRes = CoInitialize(0); //Reserved; must be NULL
   // void CoUninitialize( );

   bi.ulFlags |= BIF_STATUSTEXT;    // add in STATUS text
   if( hRes == S_OK )
   {
      bi.ulFlags |= BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
   }
     
	if (0 != SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot))
		return FALSE;
	if (NULL == pidlRoot)
		return FALSE;

	bi.pidlRoot = pidlRoot;
	pidl = SHBrowseForFolder(&bi);

	if (NULL != pidl)
		fRet = SHGetPathFromIDList( pidl, lpDir );
	else
		fRet = FALSE;

	// Get the shell's allocator to free PIDLs
	if( !SHGetMalloc(&lpMalloc) && (NULL != lpMalloc) )
	{
		if (NULL != pidlRoot)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
//			lpMalloc->Free(pidlRoot);
//         lpMalloc->Free(pidlRoot);
		}

		if (NULL != pidl)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
//			lpMalloc->Free(pidl);
		}

//		lpMalloc->lpVtbl->Release(lpMalloc);
//		lpMalloc->Free(lpMalloc);
	}

   if( fRet )
      strcpy( lpPath, lpDir );   // pass back the NEW

//   hRes = CoInitialize(0); //Reserved; must be NULL
   CoUninitialize();

	return fRet;
}


// eof - browse2.c
