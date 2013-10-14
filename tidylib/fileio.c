/* fileio.c -- does standard I/O

  (c) 1998-2006 (W3C) MIT, ERCIM, Keio University
  See tidy.h for the copyright notice.

  CVS Info :

    $Author: arnaud02 $ 
    $Date: 2006/01/25 14:17:35 $ 
    $Revision: 1.9 $ 

  Default implementations of Tidy input sources
  and output sinks based on standard C FILE*.

*/

#include <stdio.h>

#include "fileio.h"
#include "tidy.h"


typedef struct _fp_input_source
{
    FILE*        fp;
    TidyBuffer   unget;
} FileSource;

static int TIDY_CALL filesrc_getByte( void* sourceData )
{
  FileSource* fin = (FileSource*) sourceData;
  int bv;
  if ( fin->unget.size > 0 )
    bv = tidyBufPopByte( &fin->unget );
  else
    bv = fgetc( fin->fp );
  return bv;
}

static Bool TIDY_CALL filesrc_eof( void* sourceData )
{
  FileSource* fin = (FileSource*) sourceData;
  Bool isEOF = ( fin->unget.size == 0 );
  if ( isEOF )
    isEOF = feof( fin->fp ) != 0;
  return isEOF;
}

static void TIDY_CALL filesrc_ungetByte( void* sourceData, byte bv )
{
  FileSource* fin = (FileSource*) sourceData;
  tidyBufPutByte( &fin->unget, bv );
}

void TIDY_CALL initFileSource( TidyInputSource* inp, FILE* fp )
{
  FileSource* fin = NULL;

  inp->getByte    = filesrc_getByte;
  inp->eof        = filesrc_eof;
  inp->ungetByte  = filesrc_ungetByte;

  fin = (FileSource*) MemAlloc( sizeof(FileSource) );
  ClearMemory( fin, sizeof(FileSource) );
  fin->fp = fp;
  inp->sourceData = fin;
}

void TIDY_CALL freeFileSource( TidyInputSource* inp, Bool closeIt )
{
    FileSource* fin = (FileSource*) inp->sourceData;
    if ( closeIt && fin && fin->fp )
      fclose( fin->fp );
    tidyBufFree( &fin->unget );
    MemFree( fin );
}

void TIDY_CALL filesink_putByte( void* sinkData, byte bv )
{
  FILE* fout = (FILE*) sinkData;
  fputc( bv, fout );
}

void TIDY_CALL initFileSink( TidyOutputSink* outp, FILE* fp )
{
  outp->putByte  = filesink_putByte;
  outp->sinkData = fp;
}

/*
 * local variables:
 * mode: c
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * eval: (c-set-offset 'substatement-open 0)
 * end:
 */
