
/*
 * SECTION.H
 * // this is public domain software - praise me, if ok, just don't blame me!
 */
#ifndef  _SECTION_HH
#define  _SECTION_HH

/* handle to a section */
typedef struct section * SECTION;


SECTION section_new(LINE first, LINE last, LIST list);
void section_delete(SECTION section);
BOOL section_match(SECTION section1, SECTION section2);
LINE section_getfirstline(SECTION section);
LINE section_getlastline(SECTION section);
SECTION section_getlink(SECTION section);
SECTION section_getcorrespond(SECTION section);
void section_setstate(SECTION section, int state);
int section_getstate(SECTION section);
int section_getlinecount(SECTION section);
int section_getleftbasenr(SECTION section);
int section_getrightbasenr(SECTION section);
LIST section_makelist(LIST linelist, BOOL left);
void section_deletelist(LIST sections);
LIST section_makecomposite(LIST secsleft, LIST secsright);
BOOL section_matchlists(LIST secsleft, LIST secsright);

// new
extern   DWORD section_getflag(SECTION section);
extern   VOID  section_setcompstats(DWORD dwLeft, DWORD dwRight, INT iFD );
extern   VOID  ResetLineStats( LIST compo );
extern   LIST section_makeone(LIST file); // make 'one composite' of the existing file

#endif   // _SECTION_HH
/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - section.h
