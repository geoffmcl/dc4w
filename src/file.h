
/*
 * FILE.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _FILE_HH
#define  _FILE_HH

struct filedata {

        DIRITEM diritem;        /* handle to file name information */
        LIST lines;             /* NULL if lines not read in */

        INT linecnt, maxlen;  // line count - if read in, and maximum length
        BOOL   bReadIn; // TRUE if file read of data done
        int fd_file_flag;   // 0x0001 if NULL found in line data == BINARY FILE
};

/* handle to filedata */
typedef struct filedata * FILEDATA;

FILEDATA file_new(DIRITEM fiName, BOOL bRead);
DIRITEM file_getdiritem(FILEDATA fi);
void file_delete(FILEDATA fi);
LIST file_getlinelist(FILEDATA fi);
void file_discardlines(FILEDATA fi);
void file_reset(FILEDATA fi);
// new
INT file_getlinecount(FILEDATA fd);
INT file_getmaxline(FILEDATA fd);

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
#endif   // _FILE_HH
// eof - file.h
