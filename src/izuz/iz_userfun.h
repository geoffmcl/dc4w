

// UserFunc.h
#ifndef	_UserFunc_H
#define	_UserFunc_H
#include <windows.h>

#ifdef   GMUZ
//                               dwsize         compsiz        ratio,
//  month,    day,     year      hour,     minute    second
//  path  unix-time
typedef void (WINAPI APPMESSAGE)(unsigned long, unsigned long, unsigned,
   unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
   LPSTR, time_t, unsigned);

typedef struct {
   APPMESSAGE  *SendApplicationMessage;
   unsigned long TotalSizeComp;
   unsigned long TotalSize;
   unsigned long CompFactor;       /* "long" applied for proper alignment, only */
   unsigned long NumMembers;
} USERFUNCTIONS, * LPUSERFUNCTIONS;

#else    // !GMUZ

typedef int (WINAPI APPPRNT) (LPSTR, unsigned long);
typedef int (WINAPI APPPASSWORD) (LPSTR, int, LPCSTR, LPCSTR);
typedef int (WINAPI APPSERVICE) (LPCSTR, unsigned long);
typedef void (WINAPI APPSND) (void);
typedef int (WINAPI APPREPLACE)(LPSTR);
typedef void (WINAPI APPMESSAGE)(unsigned long, unsigned long, unsigned,
   unsigned, unsigned, unsigned, unsigned, unsigned,
   char, LPSTR, LPSTR, unsigned long, char);

typedef struct {
   APPPRNT *print;
   APPSND  *sound;
   APPREPLACE  *replace;
   APPPASSWORD *password;
   APPMESSAGE  *SendApplicationMessage;
   APPSERVICE  *ServCallBk;
   unsigned long TotalSizeComp;
   unsigned long TotalSize;
   unsigned long CompFactor;       /* "long" applied for proper alignment, only */
   unsigned long NumMembers;
   WORD cchComment;
} USERFUNCTIONS, * LPUSERFUNCTIONS;

#endif   // GMUZ y/n

#endif	// _USerFunc_H
// eof - UserFunc.h
