
// state.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef  _STATE_HH
#define  _STATE_HH

/*
 * STATE.H
 *
 * Definition of the results of comparisons for files and for lines
 * within files.
 *
 * These need to be globally declared so that the UI code in dc4w.c can
 * map states to the colour scheme (to correctly highlight changed lines).
 *
 * They apply to files (compitem_getstate() ) and to sections in the
 * composite list (section_getstate). All lines within a section have the
 * same state. The UI code will use the view_getstate() function to find the
 * state for a given line on the screen.
 *
 * New: use view_getflag() since most now require , int state, DWORD dwFlag
 * as part of the call input, since these items are COMPLIMENTARY
 * see dc4wTT.h for the TT_?????? bits held in dwFlag.
 *
 */

/* Applies to both lines or files: they are the same */
#define STATE_SAME              1

// not used in present implementation 
#define STATE_COMPARABLE        2
#define STATE_SIMILAR           3

/* Applies only to files */
/* - Files differ (and can be expanded) */
#define STATE_DIFFER            4

/* They are only in the left or right tree */
#define STATE_FILELEFTONLY      5
#define STATE_FILERIGHTONLY     6


/* Applies to lines only */
/* the line only exists in one of the lists */
#define STATE_LEFTONLY          7       /* line only in left file */
#define STATE_RIGHTONLY         8       /* line only in right file */


/* The line is the same in both files, but in
 * different places (thus the line will appear twice in the composite list,
 * once with each of these two states
 */
#define STATE_MOVEDLEFT         9       /* this is the left file version */
#define STATE_MOVEDRIGHT        10      /* this is the right file version*/

/* In processing the sections to build the composite list, we need to
 * track which sections have been processed.  After this the left and
 * right lists of sections are of no further interest so this state
 * only exist 'during' the left/right compare, but not used in composite.
 *
 */
#define STATE_MARKED            99


#endif   // _STATE_HH
/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - state.h
