
/****************************** Module Header *******************************
* Module Name: LINE.C
*
* Functions that handle lines of text to be output.
*
* Functions:
*
* line_new()
* line_delete()
* line_reset()
* line_gettext()
* line_gettabbedlength()
* line_getlink()
* line_getlinenr()
* line_compare()
* line_link()
* line_isblank()
*
* Comments:
*
* LINE is a data type representing a string of ascii text along with 
* a line number.
*
* A LINE can compare itself to another line, and maintain a link if the
* lines are similar. 
*
* Comparisons between lines take note of the global option flag
* ignore_blanks, defined elsewhere. If this is true, we ignore
* differences in spaces and tabs when comparing lines, and when
* generating hashcodes.
*
* Links and are only generated once. To clear the link call line_reset.
*
* Lines can be allocated on a list. If a null list handle is passed, the
* line will be allocated using gmem get() from the hHeap defined and
* initialised elsewhere.
* this is public domain software - praise me, if ok, just don't blame me!
****************************************************************************/
#include "dc4w.h"

// NOTE: In line.h, we have typedef struct fileline FAR * LINE;
struct fileline {

        UINT flags;     /* see below */
        LPTSTR ptext;     /* null-terminated copy of line text */
        DWORD  len;  // since line may be binary, keep length for gmem_free!

        DWORD hash;     /* hashcode for line */
        LINE link;      /* handle for linked line */
        UINT linenr;    /* line number (any arbitrary value) */
};

/* (UINT) flags values (or-ed) */
#define  LF_DISCARD     0x00000001  /* if true, alloced from gmem heap */
#define  LF_HASHVALID   0x00000002  /* if true, hashcode need not be recalced */

#define  LF_HASTABS     0x00000010  // contains one or more tab chars
#define  LF_HASCR       0x00000020  // ends with CR only
#define  LF_HASCRLF     0x00000040  // end CR/LF
#define  LF_HASLF       0x00000080  // ends with LF only
#define  LF_HASSPS      0x00000100
#define  LF_HASSIGS     0x00000200
#define  LF_HASNONPRT   0x00000400

// some day I would like to differentiate between C/C++ comment differences
// and CODE differences - Something like this set of FLAGS
#define  LF_ISCOMM1     0x80000000  // is within /* ... */
#define  LF_COMMBGN     0x40000000  // contains "/*" open comment
#define  LF_COMMEND     0x20000000  // contains "*/" end comment
#define  LF_ISCOMM2     0x10000000  // contains "//" comment

/***************************************************************************
 * Function: line_new
 *
 * Purpose:
 *
 * Creates a new line and makes a copy of the text.
 *
 * If the list is non-null, allocate on the list. If null, alloc from
 * gmem_get.
 *
 ***************************************************************************/
LINE
line_new(LPSTR text, int linelength, UINT linenr, LIST list)
{
        LINE line;
        LPTSTR lpl;
        /* alloc a line. from the list if there is a list */
        if (list) {
                line = List_NewLast(list, sizeof(struct fileline));
                if (line == NULL) {
                        return(NULL);
                }
                line->flags = 0;
        } else {
                line = (LINE) gmem_get(hHeap, sizeof(struct fileline), "line_new0" );
                if (line == NULL) {
                        return(NULL);
                }
                line->flags = LF_DISCARD;
        }

        /* alloc space for the text. remember the null character */
        //line->text = gmem_get(hHeap, (linelength + 1), "line_new" );
        //strncpy(line->text, text, linelength);
        //line->text[linelength] = '\0';

        lpl = gmem_get(hHeap, (linelength + 1), "line_new" );
        // this is a BIG problem for BINARY files
        strncpy(lpl, text, linelength);
        line->len = (DWORD)linelength;
        lpl[linelength] = '\0';
        line->ptext = lpl;
        //if( strlen( line->ptext ) != (DWORD)linelength )
        //{
        //   chkme( "WARNING: line_new say len = %d, but final line is %d!!!"MEOR,
        //      linelength, strlen( line->text ) );
        //}

        line->link   = NULL;       // start with NO link to another line
        line->linenr = linenr;

        return(line);
}

/***************************************************************************
 * Function: line_delete
 *
 * Purpose:
 *
 * Deletes a line and frees up all associated memory and if the line
 * was not alloc-ed from a list, frees up the line struct itself
 *
 ***************************************************************************/
void
line_delete(LINE line)
{
        if (line == NULL) {
                return;
        }

        /* free up text space */
        //gmem_free(hHeap, line->text, (strlen(line->text)+1), "line_new" );
        //NOTE: It is possible to load 'binary' lines, so we KEEP the length!
        gmem_free(hHeap, line->ptext, ( line->len + 1 ), "line_new" );
        line->ptext = 0;   // no line memory

        /* free up line itself only if not on list */
        if (line->flags & LF_DISCARD)
        {
                gmem_free(hHeap, (LPSTR) line, sizeof(struct fileline), "line_new0" );
                line->flags &= ~(LF_DISCARD);
        }
}

/***************************************************************************
 * Function: line_reset
 *
 * Purpose:
 *
 * Clears the link and force recalc of the hash code.
 *
 ***************************************************************************/
void
line_reset(LINE line)
{
        if (line == NULL) {
                return;
        }

        line->link = NULL;

        line->flags &= ~( LF_HASHVALID );
}


/***************************************************************************
 * Function: line_gettext
 *
 * Purpose:
 *
 * Returns a pointer to the line text
 *
 * added Dec 2001 - *NEVER* return a NULL
 ***************************************************************************/
LPSTR
line_gettext(LINE line)
{
   static TCHAR _s_null[] = "\0";
        if (line == NULL) {
                return(_s_null);
        }
        if( line->ptext )
           return (line->ptext);
        else
           return(_s_null);
}

/***************************************************************************
 * Function: line_gettabbedlength
 *
 * Purpose:
 *
 * Returns the length of line in characters, expanding tabs. 
 *
 ***************************************************************************/
int
line_gettabbedlength(LINE line, int tabstops)
{
        int length = 0;
        LPSTR chp;

        if (line == NULL) {
                return(0);
        }
        chp = line->ptext; // get text pointer
        if( !chp )
           return 0;

        //for (length = 0, chp = line->text; *chp != '\0'; chp++) {
        for( ; *chp != '\0'; chp++ )
        {
                if (*chp == '\t')
                {
                   // set length as the next TAB out from here
                        length = (length + tabstops) / tabstops * tabstops;
                }
                else
                {
                        length++;
                }
        }
        return(length);
}


/***************************************************************************
 * Function: line_gethashcode
 *
 * Purpose:
 *
 * Returns the hashcode for this line 
 *
 ***************************************************************************/
DWORD
line_gethashcode(LINE line)
{
   DWORD dwFlag = 0;
        if (line == NULL)
        {
                return(0);
        }

        if( !(line->flags & LF_HASHVALID) )
        {
           /* hashcode needs to be recalced */
           //line->hash = hash_string(line->ptext, ignore_blanks);
           if( ignore_blanks )
              dwFlag |= hf_ignoreblanks;
           if( gbIgnEOL )
              dwFlag |= hf_ignoreeol;
           if( gbIgnCase )
              dwFlag |= hf_ignorecase;

           line->hash = hash_string(line->ptext, dwFlag);

           line->flags |= LF_HASHVALID;

        }
        return (line->hash);
}

/***************************************************************************
 * Function: line_getlink
 *
 * Purpose:
 *
 * Returns the handle for the line that is linked to this line (the
 * result of a successful line_link() operation). This line is
 * identical in text to the linked line (allowing for ignore_blanks).
 *
 ***************************************************************************/
LINE
line_getlink(LINE line)
{
        if (line == NULL) {
                return(NULL);
        }

        return(line->link);
}

/***************************************************************************
 * Function: line_getlinenr
 *
 * Purpose:
 *
 * Returns the line number associated with this line 
 *
 ***************************************************************************/
UINT
line_getlinenr(LINE line)
{
        if (line == NULL) {
                return(0);
        }

        return(line->linenr);
}

#define  SetLnFlag( line1, c1 ) { \
if( line1 && c1 ) { \
if( c1 == '\r' ) { \
line1->flags |= LF_HASCR;\
} else if( c1 == '\n' ) {\
if( line1->flags & LF_HASCR ) {\
line1->flags &= ~(LF_HASCR);\
line1->flags |=   LF_HASCRLF;\
} else {\
line1->flags |=   LF_HASLF;\
} } else if( c1 == '\t' ) { \
line1->flags |= LF_HASTABS;\
} else if( c1 < ' ' ) {\
line1->flags |= LF_HASNONPRT;\
} else if( c1 == ' ' ) {\
line1->flags |= LF_HASSPS;\
} else {\
line1->flags |= LF_HASSIGS;\
} } }

VOID      SetLnFlag2( LINE line1, INT c1 )
{
   if( line1 && c1 )
   {
      if( c1 == '\r' )
      {
         line1->flags |= LF_HASCR;
      }
      else if( c1 == '\n' )
      {
         if( line1->flags & LF_HASCR )
         {
            line1->flags &= ~(LF_HASCR);
            line1->flags |=   LF_HASCRLF;
         }
         else
         {
            line1->flags |=   LF_HASLF;
         }
      }
      else if( c1 == '\t' )
      {
         line1->flags |= LF_HASTABS;
      }
      else if( c1 < ' ' )
      {
         line1->flags |= LF_HASNONPRT;
      }
      else if( c1 == ' ' )
      {
         line1->flags |= LF_HASSPS;
      }
      else
      {
         line1->flags |= LF_HASSIGS;
      }
   }
}

/***************************************************************************
 * Function: line_compare
 *
 * Purpose:
 *
 * Compares two lines and returns TRUE if they are the same.
 *
 ***************************************************************************/
BOOL
line_compare(LINE line1, LINE line2)
{
   LPTSTR   p1, p2;
   INT      c1, c2;
   /* Assert: At least one of them is not null ??? */
   if( (line1 == NULL) || (line2 == NULL) )
   {
      /* null line handles do not compare */
      return(FALSE);
   }

   /* check that the hashcodes match - NOTE: hashcode MODIFIED by 'ignore' flags!!! */
   if( line_gethashcode(line1) != line_gethashcode(line2) )
   {
      return(FALSE);
   }

   /* hashcodes match - are the lines really the same ? */
   /* note that this is coupled to gutils\utils.c in definition of blank */
   p1 = line_gettext(line1);
   p2 = line_gettext(line2);

// Japanese friendy - NEXT YEAR, NOT NOW - UNICODE VERSION
   // add FLAGS to line
   // #define  LF_HASTABS     0x00000010  // contains one or more tab chars
   // #define  LF_HASCR       0x00000020  // ends with CR only
   // #define  LF_HASCRLF     0x00000040  // end CR/LF
   // #define  LF_HASLF       0x00000080  // ends with LF only

   c1 = *p1;
   c2 = *p2;
   do
   {
      SetLnFlag( line1, c1 );
      SetLnFlag( line2, c2 );

      if( gbIgnEOL )
      {
         while( (c1 == '\r') || (c1 == '\n') )
         {
            p1++; // to next
            c1 = *p1;   // and fetch it
            SetLnFlag( line1, c1 );
         }
         while( (c2 == '\r') || (c2 == '\n') )
         {
            p2++; // to next
            c2 = *p2;   // and fetch it
            SetLnFlag( line2, c2 );
         }
      }
      if( ignore_blanks )
      {
         while( (c1 != 0) && (c1 <= ' ') )
         {
            p1++; // to next
            c1 = *p1;   // and fetch it
            SetLnFlag( line1, c1 );
         }

         while( (c2 != 0) && (c2 <= ' ') )
         {
            p2++;
            c2 = *p2;
            SetLnFlag( line2, c2 );
         }
      }

      if( gbIgnCase )
      {
         c1 = toupper(c1);
         c2 = toupper(c2);
      }

      if( c1 != c2 )
         return(FALSE);

      if(c1)
      {
         p1++;
         c1 = *p1;
      }
      else
         break;

      if(c2)
      {
         p2++;
         c2 = *p2;
      }
      else
         break;

   } while( (c1 != 0) && (c2 != 0) );

   if( c1 != c2 )
      return(FALSE);
   else
      return(TRUE);
}

BOOL
line_compare_ORG(LINE line1, LINE line2)
{
        LPSTR p1, p2;

        /* Assert: At least one of them is not null ??? */

        if ((line1 == NULL) || (line2 == NULL)) {
                /* null line handles do not compare */
                return(FALSE);
        }

        /* check that the hashcodes match */
        if (line_gethashcode(line1) != line_gethashcode(line2)) {
                return(FALSE);
        }

        /* hashcodes match - are the lines really the same ? */
        /* note that this is coupled to gutils\utils.c in definition of blank */
        p1 = line_gettext(line1);
        p2 = line_gettext(line2);

// Japanese friendy
        do {
                if (ignore_blanks) {
                        while ( (*p1 == ' ') || (*p1 == '\t')) {
                                p1 = CharNext(p1);
                        }
                        while ( (*p2 == ' ') || (*p2 == '\t')) {
                                p2 = CharNext(p2);
                        }
                }
                if (IsDBCSLeadByte(*p1) && *(p1+1) != '\0'
                &&  IsDBCSLeadByte(*p2) && *(p2+1) != '\0') {
                        if (*p1 != *p2 || *(p1+1) != *(p2+1)) {
                                return(FALSE);
                        }
                        p1 += 2;
                        p2 += 2;
                } else {
                        if (*p1 != *p2) {
                                return(FALSE);
                        }
                        p1++;
                        p2++;
                }
        } while ( (*p1 != '\0') && (*p2 != '\0'));

        return(TRUE);
}


/***************************************************************************
 * Function: line_link
 *
 * Purpose:
 *
 * Attempts to link two lines and returns TRUE if succesful.
 *
 * This will fail if either line is NULL, or already linked, or if
 * they differ.
 *
 ***************************************************************************/
BOOL
line_link(LINE line1, LINE line2)
{
        if ( (line1 == NULL) || (line2 == NULL)) {
                return(FALSE);
        }

        if ( (line1->link != NULL) || (line2->link != NULL)) {
                return(FALSE);
        }

        if (line_compare(line1, line2)) {
                line1->link = line2;
                line2->link = line1;
                return(TRUE);
        } else {
                return(FALSE);
        }
}


/***************************************************************************
 * Function: line_isblank
 *
 * Purpose:
 *
 * Returns TRUE iff line is blank.  NULL => return FALSE 
 *
 ***************************************************************************/
BOOL line_isblank(LINE line)
{
//   return line!=NULL && utils_isblank(line->ptext);
   LPTSTR   lpl;
   if( line == NULL )
      return TRUE;   // well sort of :-)

   lpl = line->ptext;   // extract the pointer
   if( !lpl )
      return TRUE;   // more sortta

   if( *lpl > ' ' )  // many lines are like this
      return FALSE;

   return( utils_isblank(line->ptext) );
}

/******************************************************************************\
*       This was a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/
// eof - line.c
