
// Browse.cpp
// this is public domain software - praise me, if ok, just don't blame me!
// CAN NOT INCLUDE THIS!!! #include "dc4wVers.h" // before ALL else
/* SUPPRESS UGLY MESSAGE FROM MSVC8 */
#if   (defined(_MSC_VER) && (_MSC_VER > 1300))
#pragma warning( disable:4244 )
#endif   /* #if   (defined(_MSC_VER) && (_MSC_VER > 1300)) */

#include "stdafx.h"
#include "dc4w.h"
#include "FilDlg.h"
// #include "Browse.h:

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef   USECPPDLG
/////////////////////////////////////////////////////////////////////////////
// CFilDlg

IMPLEMENT_DYNAMIC(CFilDlg, CFileDialog)

CFilDlg::CFilDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
}


BEGIN_MESSAGE_MAP(CFilDlg, CFileDialog)
	//{{AFX_MSG_MAP(CFilDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// eof - FileDialog


TCHAR szDefExt[] = "txt";

//void CCa2Files::BrowseForName(UINT uiCaller)
void BrowseForName(UINT uiCaller, LPTSTR lpFile, LPTSTR pTitle, LPTSTR pNew,
                   LPTSTR  pFilter )
{
//	CString str;
	int		i;

//	if( uiCaller == 0 )
//		str = m_stgSaveFile;
//	else
//		str = m_stgDelFile;

//	CFileDialog dlg( FALSE,
	CFilDlg dlg( FALSE,
		szDefExt,
		lpFile,  // str,
		OFN_HIDEREADONLY,
		pFilter );  // szFilter );
		//this );

//	if( uiCaller == 0 )
//		str = "HISTORY SAVE FILE";
//	else
//		str = "DELETE SAVE FILE";

//	dlg.m_ofn.lpstrTitle = str;
	dlg.m_ofn.lpstrTitle = pTitle;

	i = dlg.DoModal();

	if( i = IDOK )
	{
//		str = dlg.GetPathName();
      strcpy( pNew, dlg.GetPathName() );
//		if( uiCaller == 0 )
//		{
//			m_stgSaveFile = str;
//			EdSaveFile().SetWindowText( m_stgSaveFile );
//		}
//		else
//		{
//			m_stgDelFile = str;
//			EdDelFile().SetWindowText( m_stgDelFile );
//		}
	}
}

#else    // !#ifdef   USECPPDLG

OPENFILENAME g_ofnOpen  = {0};
TCHAR g_szFileOpen[264] = "\0";
TCHAR g_szFilterOpen[]  = "All\0*.*\0Text\0*.TXT\0";
INT   g_iIndexOpen      = 1;

// void	Do_IDM_FIL_LOAD( HWND hWnd)
//typedef struct _browseinfo { 
//    HWND hwndOwner; 
//    LPCITEMIDLIST pidlRoot; 
//    LPTSTR pszDisplayName; 
//    LPCTSTR lpszTitle; 
//    UINT ulFlags; 
//    BFFCALLBACK lpfn; 
//    LPARAM lParam; 
//    int iImage; 
//} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO; 
//Members
BROWSEINFO  g_BI;


BOOL  GetNewFolderName( HWND hWnd, LPTSTR lpDir, LPTSTR lpRes )
{
   LPBROWSEINFO lpbi = &g_BI;
   LPITEMIDLIST   lpil;

   g_BI.hwndOwner = hWnd;
   lpil = SHBrowseForFolder( lpbi );

   return FALSE;
}

BOOL  GetNewFileName( HWND hWnd, LPTSTR lpFile, LPTSTR lpRes,
                     BOOL bChkExist )
{
   static BOOL bDnInit = FALSE;
	HANDLE	hf;              // file handle
   //TCHAR  cs[264];
   OPENFILENAME * pofn = &g_ofnOpen;       // common dialog box structure
   LPTSTR   lpf = &g_szFileOpen[0];

   if( !bDnInit )
   {
	   // Initialize OPENFILENAME
	   ZeroMemory(pofn, sizeof(OPENFILENAME));
	   pofn->lStructSize = sizeof(OPENFILENAME);
	   pofn->hwndOwner = hWnd;
	   pofn->lpstrFile = g_szFileOpen;
	   pofn->nMaxFile  = 256;
      //	pofn->lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	   pofn->lpstrFilter = g_szFilterOpen;
	   pofn->nFilterIndex = g_iIndexOpen;
	   pofn->lpstrFileTitle = NULL;
	   pofn->nMaxFileTitle = 0;
	   pofn->lpstrInitialDir = NULL;
	   pofn->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

      bDnInit = TRUE;
   }

   if( lpFile && *lpFile )
   {
      strcpy(lpf, lpFile); // get User's current FILE
   }

   //strcpy(cs,g_szFileOpen);

	// Display the Open dialog box.
	if( GetOpenFileName(pofn)==TRUE )
	{
      if( bChkExist )
      {
		   hf = CreateFile(g_szFileOpen,
			   GENERIC_READ,
			   0,
			   (LPSECURITY_ATTRIBUTES) NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
         (HANDLE) NULL );
		   if(VFH(hf))
		   {
            // we have a file to READ
			   CloseHandle(hf);
            if(lpRes)
               strcpy(lpRes, g_szFileOpen);
            return TRUE;
		   }
      }
      else
      {
         if(lpRes)
            strcpy(lpRes, g_szFileOpen);
         return TRUE;
      }
	}
   return FALSE;
}

BOOL BrowseForName( HWND hWnd, LPTSTR lpFile, LPTSTR pTitle, LPTSTR pNew, LPTSTR  pFilter )
{
   BOOL  bRet = FALSE;
   LPTSTR lpb = GetStgBuf();
   if( GetDirectoryOnly( lpb, lpFile ) )
   {

   }
   return bRet;
}


// gmGetFil.c
#include	<windows.h>
#include	<shlobj.h>
//#include	"gmGetFil.h"

/*  F  G E T  F I L E  N A M E */
/*----------------------------------------------------------------------------
    %%Function: FgmGetFil
OPENFILENAME g_ofnOpen  = {0};
TCHAR g_szFileOpen[264] = "\0";
TCHAR g_szFilterOpen[]  = "All\0*.*\0Text\0*.TXT\0";
INT   g_iIndexOpen      = 1;
 
----------------------------------------------------------------------------*/
TCHAR g_szTitleOpen[] = "Choose a File Name";

//BOOL FGetFileName( LPSTR szFileName )
//BOOL FgmGetFil2( HWND hWnd, HINSTANCE hInst, LPTSTR lpFilt, LPTSTR lpTitle,
//                LPTSTR lpFilNam )
BOOL FgmGetFil2( HWND hWnd, LPTSTR lpFilNam, LPTSTR lpRes )
{
	OPENFILENAME * pofn = &g_ofnOpen;  // common
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
//			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
			lpMalloc->Free(pidlRoot);
		}

		if (NULL != pidl)
		{
//			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
			lpMalloc->Free(pidl);
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

TCHAR szBgnDir[MAX_PATH] = { "\0" };

int CALLBACK BrowseCBProc(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData
    )
{
   LPTSTR   lpd = &szBgnDir[0];
   switch(uMsg)
   {
//Value identifying the event. This can be one of the following values: 
   case BFFM_INITIALIZED:
      //Indicates the browse dialog box has finished initializing. The lParam value is zero. 
      SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)g_lpDir );
      if( g_lpDir && *g_lpDir )
      {
         sprtf( "BFFM_INIT: Started with [%s]" MEOR, g_lpDir );
         strcpy(lpd,g_lpDir);  // get a COPY of the START
         SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpd);
      }
      else
      {
         sprtf( "BFFM_INIT: Started with BLANK." MEOR );
         if( *lpd == 0 )
         {
            if( GetCurrentDirectory( (MAX_PATH * sizeof(TCHAR)), lpd) )
            {
               // WParam is TRUE since you are passing a path.
               // It would be FALSE if you were passing a pidl.
               SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpd);
            }
         }
         else
         {
               SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpd);
         }
      }
      break;

   case BFFM_SELCHANGED:
      //Indicates the selection has changed. The lParam parameter points to the item 
      //identifier list for the newly selected item.
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;
         *lpd = 0;
         // Set the status window to the currently selected path.
         if( SHGetPathFromIDList( pidl, lpd ) )
         {
            SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)lpd );
         }
         sprtf( "BFFM_SEL: pidl=%x d=[%s]" MEOR, pidl, lpd );
      }
      break;

   case BFFM_VALIDATEFAILED:
      //Version 4.71. Indicates the user typed an invalid name into the edit box of 
      //the browse dialog box. The lParam parameter is the address of a character 
      //buffer that contains the invalid name. An application can use this message to 
      //inform the user that the name entered was not valid. Return zero to allow the 
      //dialog to be dismissed or nonzero to keep the dialog displayed.
      {
         LPTSTR lpb = (LPTSTR)lParam;
         if( lpb && *lpb )
         {
            sprtf( "BFFM_VAL: d=[%s]" MEOR, lpb );
         }
      }
      return 1;
      break;
   }
   return 0;
}

#endif   // #ifdef   USECPPDLG


// eof - Browse.cpp
