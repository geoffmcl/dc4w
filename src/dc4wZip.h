
// dc4wZip.h
// this is public domain software - praise me, if ok, just don't blame me!
#ifndef	_dc4wZip_H
#define	_dc4wZip_H

// Licence COPIED from original Info-Zip UNZIP.H
// *********************************************
/*---------------------------------------------------------------------------
This is version 2000-Apr-09 of the Info-ZIP copyright and license.
The definitive version of this document should be available at
ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely.

Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.

For the purposes of this copyright and license, "Info-ZIP" is defined as
the following set of individuals:

   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
   Jean-loup Gailly, Hunter Goatley, Ian Gorman, Chris Herborth, Dirk Haase,
   Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz, David Kirschbaum,
   Johnny Lee, Onno van der Linden, Igor Mandrichenko, Steve P. Miller,
   Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs, Kai Uwe Rommel,
   Steve Salisbury, Dave Smith, Christian Spieler, Antoine Verheijen,
   Paul von Behren, Rich Wales, Mike White

This software is provided "as is," without warranty of any kind, express
or implied.  In no event shall Info-ZIP or its contributors be held liable
for any direct, indirect, incidental, special or consequential damages
arising out of the use of or inability to use this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. Redistributions of source code must retain the above copyright notice,
       definition, disclaimer, and this list of conditions.

    2. Redistributions in binary form must reproduce the above copyright
       notice, definition, disclaimer, and this list of conditions in
       documentation and/or other materials provided with the distribution.

    3. Altered versions--including, but not limited to, ports to new operating
       systems, existing ports with new graphical interfaces, and dynamic,
       shared, or static library versions--must be plainly marked as such
       and must not be misrepresented as being the original source.  Such
       altered versions also must not be misrepresented as being Info-ZIP
       releases--including, but not limited to, labeling of the altered
       versions with the names "Info-ZIP" (or any variation thereof, including,
       but not limited to, different capitalizations), "Pocket UnZip," "WiZ"
       or "MacZip" without the explicit permission of Info-ZIP.  Such altered
       versions are further prohibited from misrepresentative use of the
       Zip-Bugs or Info-ZIP e-mail addresses or of the Info-ZIP URL(s).

    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
       "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its own source 
       and binary releases.
  ---------------------------------------------------------------------------*/
#ifdef ADD_ZIP_SUPPORT
extern	BOOL     IsValidZip( LPTSTR lpf ); // #ifdef ADD_ZIP_SUPPORT
//extern   HANDLE   FindFirstZip( LPTSTR lpf, PWIN32_FIND_DATA pfd );
//extern   BOOL     FindNextZip( HANDLE hFind, PWIN32_FIND_DATA pfd );
extern   HANDLE   FindFirstZip( LPTSTR lpf, PFDX pfdx ); // #ifdef ADD_ZIP_SUPPORT
extern   BOOL     FindNextZip( HANDLE hFind, PFDX pfdx );
extern   VOID     FindCloseZip( HANDLE hFind );

// return a BUFFER of decompressed file data
typedef struct tagOZF {
   DIRITEM  pDI;     // directory item being opened
   LPTSTR   pZName;  // ZIP file name
   LPTSTR   pFName;  // file in ZIP to extract
   LPTSTR   pData;   // pointer to uncompressed data
   DWORD    dwLen;   // LEGTH of uncompressed data
   TCHAR    szFName[264];  // buffer for file name
   TCHAR    szZName[264];  // buffer for zip name
}OZF, * POZF;

// return DECOMPRESSED data in above structure
// if TRUE, user to free(pData)
extern   INT   OpenZIPFile( POZF pozf );
#endif // #ifdef ADD_ZIP_SUPPORT

#endif	// _dc4wZip_H
// eof - dc4wZip.h
