
/****************************** Module Header *******************************
* Module Name: GUTILS.C
*
* Entry point for GUTILS.DLL
*
* Functions:
*
* DllMain()
*
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"

/* dll global data - note: now NOT a DLL, but just an object in the DC4W app. */
HANDLE hLibInst;
extern BOOL gtab_init(void);
extern void gtab_deltools(void);
extern BOOL StatusInit(HANDLE);
extern void StatusDeleteTools(void);

BOOL APIENTRY DummyDllMain(HANDLE hInstance, DWORD dwReason, LPVOID reserved)
{
   if( dwReason == DLL_PROCESS_ATTACH )
   {
      hLibInst = hInstance;

      if( !gtab_init() )
         return FALSE;

      if( !StatusInit(hLibInst) )
      {
         gtab_deltools();
         return FALSE;
      }
   }
   else if( dwReason == DLL_PROCESS_DETACH )
   {
      gtab_deltools();
      StatusDeleteTools();
   }
   return(TRUE);
}

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - gutils.c
