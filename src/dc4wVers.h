
// dc4wVers.h - a short sort of version history
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wVers_H
#define	_dc4wVers_H
// NOTE WELL: This should ALSO be adjusted in the VS_VERSION_INFO
// in the resources. This NUMBER should agree with what is there.
// FIX20200617 - first MSVC 16 2019 x64 build in Dell03
#define  VER_DATE    "28 June, 2020"  // FIX20200617
// VER_DATE    "17 June, 2020"  // FIX20200617
#define  _MajVersion	5
#define  _MinVersion	0
#define  _SubVersion	0
#define  _MakeNumber	2
#define ADD_ZIP_SUPPORT
#ifndef WINVER
#define WINVER 0x0A00
#endif // !WINVER
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif // !_WIN32_WINNT

#undef ADD_LIST_VIEW  // FIX20200621: Remove this confusing, and maybe unfinished function

//#define _WIN32_IE 0x0400
//#define _WIN32_IE 0x0501
//#define WINVER 0x0501

// FIX20120415 - Fresh build in Win7-PC
// VER_DATE    "15 April, 2012"  // FIX20120415
// MakeNumber  52  // in sync wth upd.bat and version info

// FIX20120221 - Add option, exclude repos folders (.git,.svn,CVS)
// There is already an -xall which does -
// INT Add_X_ALL_repos(VOID) { INT c = 0;
//  c = Add_FP_Directories(); c += Add_CVS_Directories(); c += Add_SVN_Directories();
//  c += Add_GIT_Directories(); gbXAllRepos = TRUE; // marked ADDED XCLUDE ALL REPOS directories
// now add a MENU item - the message is IDM_EXCLUDEREPOS
//  VER_DATE    "21 February, 2012"  // FIX20120221
//  MakeNumber  51  // in sync wth upd.bat and version info

// FIX20120116 - Have OPTION when writing a LIST, especially full compare, and ignore binary...
// maybe -ofid - [F]ull compare - [I]gnore binary - [D]ate ignore - ...
// VER_DATE    "16 January, 2012"  // FIX20120116
// MakeNumber  50  // in sync with upd.bat and version info ...

// FIX20110201 - REALLY ignore file times - 100% IGNORE
//  VER_DATE    "1 February, 2011"  // FIX20110201
//  MakeNumber  49  // in sync with upd.bat and version info ...

// FIX20100717 - Add DONEUPDCHK2 - Fast one file update
// VER_DATE    "27 July, 2010"  // FIX20100727
// MakeNumber  48  // in sync with upd.bat and version info ...

// FIX20100417 - Add (temporary) exclude to context menu
// VER_DATE    "17 April, 2010"  // FIX20100417
// MakeNumber  47  // in sync with upd.bat and version info ...

// FIX20091224 - it seems with -r-, it STILL scans _ALL_ subdirectories
// use g_bNOTRecursive = W.ws_bNOTRecursive
#define USE_GLOBAL_RECURSIVE
// VER_DATE    "04 February, 2010"  // FIX20091125
// MakeNumber  45  // in sync with upd.bat and version info ...

// FIX20091125 - if -xdall, etc, then save and OFF, bNoExcludes
// and add INCLUDE_TAGS check box to 
// VER_DATE    "25 November, 2009"  // FIX20091125
// MakeNumber  44  // in sync with upd.bat and version info ...

// FIX20090811 - Add F5 key to do 'Refresh'
// but this required F5 Next File, be changed to F2
// MENUITEM "R&efresh Display\tF5",        IDM_REFRESH
// MENUITEM "Expand Next &File\tF2",       IDM_VIEW_NEXT
// AND exclude repos dirs IDM_EXCLUDEREPOS
// VER_DATE    "11 August, 2009"  // FIX20090811

// FIX20090509 - Add #ifdef ADD_X_CVS to add -xd[fp|cvs|svn|git|all]
//   "[-xdfp] [-xdcvs] [-xdsvn] [-xdgit] [-xdall]"MEOR
//   " Exclude FrontPage, CVS, SVN, git, or all these (hidden) folders."MEOR
// MakeNumber  43  // in sync with upd.bat and version info ...
// VER_DATE    "9 May, 2009"  // FIX20090509
// define it - add CVS,SVN,GIT if -xdall used
#define  ADD_X_CVS

// FIX20090128
// Copy/update problem when the destination has READ ONLY attribute.
// MakeNumber  42  // in sync with upd.bat and version info ...
// VER_DATE    "28 January, 2009"  // FIX20090128

// FIX20081125 - NEW completely IGNORE file time,
// but INFO still put in FLAG ci->ci_dwFlag SEE gbIgnDT2, not on MENU yet
// Only through preference dialog - implies gbIgnDT is ON also
// MakeNumber  41  // in sync with upd.bat and version info ...
// VER_DATE    "25 November, 2008"  // FIX20081125

// FIX20080728 - added dc4wDel1.c module, new function IDM_DELETELEFTFILE added
// and EVENTUALLY shifted all the ZIP stuff into a STATIC library
// MakeNumber  40  // in sync with upd.bat and version info ...
// VER_DATE    "28 July, 2008"  // FIX20080415

// FIX20080415 - Ensure a particular exclude list IS included
// MakeNumber  39  // in sync with upd.bat and version info ...
// VER_DATE    "15 April, 2008"  // FIX20080415

// FIX20080121 - Remove DUPLICATES in POPUP context menu
//  MakeNumber  39  // in sync with upd.bat file ...
//  VER_DATE    "21 January, 2008"  // FIX20080121

// FIX20071020 - check if EACH directory component is IN g_sInDirs -i:d:<dir>
// and added -i:[d|f]:@inputfile or ';' separated list of file or directories
// MakeNumber  38  // in sync with upd.bat file ...
// VER_DATE    "20 October, 2007"  // FIX20071020

//  MakeNumber  37  // jumped 2 numbers to sync with upd.bat file ...
//  VER_DATE    "9 May, 2007"  // FIX20070509 small change in DATE compare, with gbIgnDT on
// VER_DATE    "28 March, 2007"  // and added the following 'static'

// FIX20061115 - add WM_MOUSEWHEEL - need to define _WIN32_WINNT >= 0x400
// or _WIN32_WINDOWS > 0x400 to enable this define in winuser.h
// Also add a compiler string to ABOUT dialog - see dc4wcomp.h
// Had to MANUALLY adjust dc4w.rc and resource.h, since MSVC8 Express (FREE)
// does NOT support resource editor ;=((
// module LAST compile date and time

// MakeNumber  35  // jumped 2 numbers to sync with upd.bat file ...
// VER_DATE    "15 November, 2006"
#ifndef  _WIN32_WINDOWS
#define  _WIN32_WINDOWS    0x500
#endif

// FIX20061104 - move to Dell01 machine, using MSVC8
//  MakeNumber  32
//  VER_DATE    "4 November, 2006"
#pragma warning(disable:4996)

// FIX20060917 - add Alt+? brief help dialog - show_help()
// MakeNumber  31
// VER_DATE    "17 September, 2006"

// FIX20060823 -xdfp: to EXCLUDE FrontPage (hidden) folders
// MakeNumber  30
//  VER_DATE    "23 August, 2006"

// FIX20060709 -I:D:<directory or mask> or -I:F:<filename or mask>
// MakeNumber  29
// VER_DATE    "9 July, 2006"

// FIX20060216 - remove automatic expansion when only ONE difference!
// MakeNumber  28
// VER_DATE    "19 February, 2006"

// FIX20060126 - Add menu item to put full path name, relative name, or just file title
// and add -xf:@input.list or -xd:@input.file.list
//  MakeNumber  26
//  VER_DATE    "26 January, 2006"

// FIX20051222 - Reverse logic on ZIPPING - presently defaults ON, and -N to stop
// So, keep -n, or -n- to say NO ZIP, but -n+ or -na (all) will cause a ZIP to be written            g_bNoAZip = TRUE;
// MakeNumber  25
// VER_DATE    "22 December, 2005"

// FIX20051203 - MSVC7.1 - Allow like an OPT-IN mask, like ...\data\Temp*
// and allow -I<mask> -I<file> to INCLUDE files in LISTING
// MakeNumber  24
// VER_DATE    "4 December, 2005"

// FIX20051105 - MSVC7.1 - Show, and be able to DISABLE EXCLUDED - *eXc=nnn*
// MakeNumber  23
// VER_DATE    "5 November, 2005"

// FIX20051028 - using MSVC7.1 - Made mistake in FIX20051024 -
// MakeNumber  22   // NEW FILE in SAVE FILE LIST should be CWD
// VER_DATE    "28 October, 2005" // NOT runtime folder!!! Drat

// FIX20051024 - using MSVC7.1 - Add update selected file, in context menu, and
// ensure a NEW FILE in the SAVE FILE LIST dialog - IDD_SAVELIST
//  MakeNumber  21   // // FIX20051024 - using MSVC7.1
//  VER_DATE    "24 October, 2005" // in PRO-1(F) machine

// FIX20050226 - column error when entering expanded mode - compare of two files
//  MakeNumber  20   // FIX20050226 - column error when entering expanded mode
//  VER_DATE    "26 February, 2005" // in PRO-1 machine

// MakeNumber  19   // FIX20050129 - using MSVC7 .NET 2003
// VER_DATE    "29 January, 2005" // in PRO-1 machine
//  MakeNumber  18   // FIX20041218 - uninit'ed 'sect' in section.c
//  VER_DATE    "18 December, 2004" // in PRO-1 machine

// MakeNumber  17   // fixed FileList & DirList, input,output,edit
// VER_DATE    "25 July, 2003"  // added 'Exclude <filename>' context item

//  MakeNumber  16   // default (BOOL)  bAutoZip = TRUE = write LIST
//  VER_DATE    "26 June, 2003"  // added g_bWriteAZip default = FALSE
// but NOT implemented at this time. and FIX20030626 - dc4w d:\ d:\that FAILED!
// and try to remove a freeze in certain circumstances.

//#define  MakeNumber  15   // some exclude from zip fixes - added -NA to force ALL
//#define  VER_DATE    "15 November, 2002"  // If AutoZip then add -x@d:\temp\tempexcl.txt
//  MakeNumber  14   // some exclude fixes
//  VER_DATE    "08 October., 2002"  // -x+12 is ok
// MakeNumber  13   // consolidate ZIPUP save functions
// VER_DATE    "19 May, 2002"  // zip consolidation

//  12   // add list status to status bar display
// "19 January, 2002"  // list info to status bar

//  11   // even if date AND size different, if "ignore blanks"
//  MakeNumber  10   // more UI fixes - context menu and outline_include
// and "full line compare" are on, be able to exclude those that are the same.
// "1 January, 2002"  // be able to 'ignore' date/time and 'spaces'
// VER_DATE    "31 December, 2001"  // looking ok
// when comparing two files. Also have a maximum different in size = more TT_... bits.

// MajVersion     4
// MinVersion     0
// SubVersion     0
//  MakeNumber     1
//  MakeNumber     2     // Added gbExclude nd gsExclLists
//  MakeNumber     3     // Changed exclude output to *.bak;*.old;temp*.*;etc
//  MakeNumber     4     // Lots of changes and tidying
//  MakeNumber     5     // tiny fix in dir_copy
//  MakeNumber  6   // add -E[+|-] g_bNoExp to inhibit expand 1
//  MakeNumber  7   // add [BROWSE] button - folders or files
//  MakeNumber  8   // change toggle of 'Show Different'
//  MakeNumber  9   // enable toggles in expanded mode
#define  NEWCOLS2       // and more flxible column creation

//  VER_DATE    "14 April, 2001"  // control changes in INI file
//  VER_DATE    "9 May, 2001"  // change exclude output
//  VER_DATE    "26 May, 2001"  // use COMBO boxes
//  VER_DATE    "9 June, 2001"  // changes and tidying
//  VER_DATE    "5 August, 2001"  // tiny fix in dir_copy - looking good
//  VER_DATE    "27 August, 2001"  // add -E[+|-] g_bNoExp to inhibit expand 1
// add [BROWSE] button - folders or files
// VER_DATE    "6 December, 2001"  // add gbAutoSel if no cmd line
// VER_DATE    "22 December, 2001"  // fix difference toggle

#define		APPNAME		"DC4W"
#define		APPCLASS	   "DC4WCLASS"

#define     MXFNB       1024    // was 264
#define     MXTMPBUFS   32      // 16 // GetNxtBuf()

#define     ADDSTATUS      // add a status bar window
#define     ADDTMDTXT      // and add timed text messages

/* Is this adequate for long pathnames on a hi-res screen? */
#define     SF_MAXLABEL     80   /* no more than 80 in an item within the bar */

#define  ADDSTATS2   // activate another item on the status bar
// #define IDL_LISTSTATS   506   // folder compare list information

#undef   ADDZIPIT    // be able to offer a zipup option
#define  ADDXCLUDE   // Like -x[*.bak;temp*.*;*.old] or -x@FileList
#define  ADDTIMER
#define  TIMER_ID       0x1234
#define  TIMER_TIC      200      // each 200 ms

#define  MI64LEN        14

#define MB_ALLOKNOALL   0x00000008L    // my 4 button VERIFY dialog

#endif	// _dc4wVers_H
// eof - dc4wVers.h
