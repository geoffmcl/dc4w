

// dc4wBrowse.c
#include "dc4w.h"

/*  F  G E T  F I L E  N A M E */
/*----------------------------------------------------------------------------
    %%Function: FgmGetFil
 
----------------------------------------------------------------------------*/
//BOOL FGetFileName( LPSTR szFileName )
BOOL FgmGetFil( HWND hWnd, HINSTANCE hInst, LPTSTR lpFilt, LPTSTR lpTitle, LPSTR lpFilNam )
{
	OPENFILENAME ofn;

//	SetEmptySz(szFileName);
//	ClearStruct(&ofn);
	*lpFilNam = 0;
	ZeroMemory( &ofn, sizeof(OPENFILENAME) );

	ofn.lStructSize   = sizeof(OPENFILENAME);
//	ofn.hwndOwner     = ghwndMain;
	ofn.hwndOwner     = hWnd;
//	ofn.hInstance     = (HANDLE) ghInst;
	ofn.hInstance     = hInst;
//	ofn.lpstrFilter   = _szFilter;
	ofn.lpstrFilter   = lpFilt;
	ofn.nFilterIndex  = 1L;
//	ofn.lpstrFile     = (LPSTR) szFileName;
	ofn.lpstrFile     = lpFilNam;
	ofn.nMaxFile      = MAX_PATH; // really sizeof(szFileName)
//	ofn.lpstrTitle    = "Pick a file to send";
	ofn.lpstrTitle    = lpTitle;
	ofn.lpstrInitialDir = NULL;

	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

	if( !GetOpenFileName(&ofn) )
	{
		//return fFalse;
		return FALSE;
	}

//	return fTrue;
	return TRUE;

}



/*  F  G E T  D I R E C T O R Y */
/*----------------------------------------------------------------------------
    %%Function: FgmGetDir
 
----------------------------------------------------------------------------*/
//BOOL FGetDirectory( HWND hWnd, LPSTR szDir )

/* ==========================================
BOOL FgmGetDir( HWND hWnd, LPSTR lpDir )
{
	BOOL  fRet = FALSE;
	CHAR  szPath[MAX_PATH];
	LPITEMIDLIST pidl;
	LPITEMIDLIST pidlRoot;
	LPMALLOC lpMalloc;
	BROWSEINFO bi = {
		hWnd,
		NULL,
		szPath,
		"Select a Directory",
		BIF_RETURNONLYFSDIRS,
		NULL, 0L, 0	};


	szPath[0] = 0;
	pidl = pidlRoot = 0;

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
	if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
	{
		if (NULL != pidlRoot)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
//			lpMalloc->Free(pidlRoot);
		}

		if (NULL != pidl)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
//			lpMalloc->Free(pidl);
		}

		lpMalloc->lpVtbl->Release(lpMalloc);
	}

	return fRet;
}

    ============================================ */

BOOL FgmGetFil2( HWND hWnd, LPTSTR lpFilNam, LPTSTR lpRes )
{
   BOOL  bNew = FALSE;
   return bNew;
}

BOOL FgmGetDir2( HWND hWnd, LPTSTR lpDir, LPTSTR lpPath )
{
   BOOL  bNew = FALSE;
   return bNew;
}

// eof - dc4wBrowse.c
