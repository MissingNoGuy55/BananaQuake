/*****************************************************************************
* Missi: Version of unzip.cpp and portions of zlib, modified to take advantage
* of std::ifstream (8/29/2024)
******************************************************************************/

#include <stdio.h>
#include <string.h>

#ifndef __linux__
#include <windows.h>
#endif

#include <fstream>
#include "unzip.h"
#include "unzip_ifstream.h"
//#include "cmdlib.h"

/* unzip.h -- IO for uncompress .zip files using zlib 
   Version 0.15 beta, Mar 19th, 1998,

   Copyright (C) 1998 Gilles Vollant

   This unzip package allow extract file from .ZIP file, compatible with PKZip 2.04g
	 WinZip, InfoZip tools and compatible.
   Encryption and multi volume ZipFile (span) are not supported.
   Old compressions used by old PKZip 1.x are not supported

   THIS IS AN ALPHA VERSION. AT THIS STAGE OF DEVELOPPEMENT, SOMES API OR STRUCTURE
   CAN CHANGE IN FUTURE VERSION !!
   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/
/* for more info about .ZIP format, see 
	  ftp://ftp.cdrom.com/pub/infozip/doc/appnote-970311-iz.zip
   PkWare has also a specification at :
	  ftp://ftp.pkware.com/probdesc.zip */

/* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.1.3, July 9th, 1998

  Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu


  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
*/

/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* @(#) $Id: unzip.cpp,v 1.1.1.3 2000/01/11 16:37:27 ttimo Exp $ */

#ifndef _ZCONF_H
#define _ZCONF_H

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
			(1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
	 make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

						/* Type declarations */

#ifndef OF /* function prototypes */
#define OF(args)  args
#endif

typedef unsigned char  Byte;  /* 8 bits */
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */
typedef Byte    *voidp;

#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

#endif /* _ZCONF_H */

#define ZLIB_VERSION "1.1.3"

/* 
	 The 'zlib' compression library provides in-memory compression and
  decompression functions, including integrity checks of the uncompressed
  data.  This version of the library supports only one compression method
  (deflation) but other algorithms will be added later and will have the same
  stream interface.

	 Compression can be done in a single step if the buffers are large
  enough (for example if an input file is mmap'ed), or can be done by
  repeated calls of the compression function.  In the latter case, the
  application must provide more input and/or consume the output
  (providing more output space) before each call.

	 The library also supports reading and writing files in gzip (.gz) format
  with an interface similar to that of stdio.

	 The library does not install any signal handler. The decoder checks
  the consistency of the compressed data, so the library should never
  crash even in case of corrupted input.
*/

/*
   The application must update next_in and avail_in when avail_in has
   dropped to zero. It must update next_out and avail_out when avail_out
   has dropped to zero. The application must initialize zalloc, zfree and
   opaque before calling the init function. All other fields are set by the
   compression library and must not be updated by the application.

   The opaque value provided by the application will be passed as the first
   parameter for calls of zalloc and zfree. This can be useful for custom
   memory management. The compression library attaches no meaning to the
   opaque value.

   zalloc must return Z_NULL if there is not enough memory for the object.
   If zlib is used in a multi-threaded application, zalloc and zfree must be
   thread safe.

   On 16-bit systems, the functions zalloc and zfree must be able to allocate
   exactly 65536 bytes, but will not be required to allocate more than this
   if the symbol MAXSEG_64K is defined (see zconf.h). WARNING: On MSDOS,
   pointers returned by zalloc for objects of exactly 65536 bytes *must*
   have their offset normalized to zero. The default allocation function
   provided by this library ensures this (see zutil.c). To reduce memory
   requirements and avoid any allocation of 64K objects, at the expense of
   compression ratio, compile the library with -DMAX_WBITS=14 (see zconf.h).

   The fields total_in and total_out can be used for statistics or
   progress reports. After compression, total_in holds the total size of
   the uncompressed data and may be saved for use in the decompressor
   (particularly if the decompressor wants to decompress everything in
   a single step).
*/

						/* constants */

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1 /* will be removed, use Z_SYNC_FLUSH instead */
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
/* Allowed flush values; see deflate() below for details */

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)
/* Return codes for the compression/decompression functions. Negative
 * values are errors, positive values are used for special but normal events.
 */

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
/* compression levels */

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_DEFAULT_STRATEGY    0
/* compression strategy; see deflateInit2() below for details */

#define Z_BINARY   0
#define Z_ASCII    1
#define Z_UNKNOWN  2
/* Possible values of the data_type field */

#define Z_DEFLATED   8
/* The deflate compression method (the only one supported in this version) */

#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */

#define zlib_version zlibVersion()
/* for compatibility with versions < 1.0.2 */

						/* basic functions */

const char * zlibVersion OF(());
/* The application can compare zlibVersion and ZLIB_VERSION for consistency.
   If the first character differs, the library code actually used is
   not compatible with the zlib.h header file used by the application.
   This check is automatically made by deflateInit and inflateInit.
 */

/* 
int deflateInit OF((z_streamp strm, int level));

	 Initializes the internal stream state for compression. The fields
   zalloc, zfree and opaque must be initialized before by the caller.
   If zalloc and zfree are set to Z_NULL, deflateInit updates them to
   use default allocation functions.

	 The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
   1 gives best speed, 9 gives best compression, 0 gives no compression at
   all (the input data is simply copied a block at a time).
   Z_DEFAULT_COMPRESSION requests a default compromise between speed and
   compression (currently equivalent to level 6).

	 deflateInit returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if level is not a valid compression level,
   Z_VERSION_ERROR if the zlib library version (zlib_version) is incompatible
   with the version assumed by the caller (ZLIB_VERSION).
   msg is set to null if there is no error message.  deflateInit does not
   perform any compression: this will be done by deflate().
*/


int deflate OF((z_streamp strm, int flush));
/*
	deflate compresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may introduce some
  output latency (reading input without producing any output) except when
  forced to flush.

	The detailed semantics are as follows. deflate performs one or both of the
  following actions:

  - Compress more input starting at next_in and update next_in and avail_in
	accordingly. If not all input can be processed (because there is not
	enough room in the output buffer), next_in and avail_in are updated and
	processing will resume at this point for the next call of deflate().

  - Provide more output starting at next_out and update next_out and avail_out
	accordingly. This action is forced if the parameter flush is non zero.
	Forcing flush frequently degrades the compression ratio, so this parameter
	should be set only when necessary (in interactive applications).
	Some output may be provided even if flush is not set.

  Before the call of deflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating avail_in or avail_out accordingly; avail_out
  should never be zero before the call. The application can consume the
  compressed output when it wants, for example when the output buffer is full
  (avail_out == 0), or after each call of deflate(). If deflate returns Z_OK
  and with zero avail_out, it must be called again after making room in the
  output buffer because there might be more output pending.

	If the parameter flush is set to Z_SYNC_FLUSH, all pending output is
  flushed to the output buffer and the output is aligned on a byte boundary, so
  that the decompressor can get all input data available so far. (In particular
  avail_in is zero after the call if enough output space has been provided
  before the call.)  Flushing may degrade compression for some compression
  algorithms and so it should be used only when necessary.

	If flush is set to Z_FULL_FLUSH, all output is flushed as with
  Z_SYNC_FLUSH, and the compression state is reset so that decompression can
  restart from this point if previous compressed data has been damaged or if
  random access is desired. Using Z_FULL_FLUSH too often can seriously degrade
  the compression.

	If deflate returns with avail_out == 0, this function must be called again
  with the same value of the flush parameter and more output space (updated
  avail_out), until the flush is complete (deflate returns with non-zero
  avail_out).

	If the parameter flush is set to Z_FINISH, pending input is processed,
  pending output is flushed and deflate returns with Z_STREAM_END if there
  was enough output space; if deflate returns with Z_OK, this function must be
  called again with Z_FINISH and more output space (updated avail_out) but no
  more input data, until it returns with Z_STREAM_END or an error. After
  deflate has returned Z_STREAM_END, the only possible operations on the
  stream are deflateReset or deflateEnd.
  
	Z_FINISH can be used immediately after deflateInit if all the compression
  is to be done in a single step. In this case, avail_out must be at least
  0.1% larger than avail_in plus 12 bytes.  If deflate does not return
  Z_STREAM_END, then it must be called again as described above.

	deflate() sets strm->adler to the adler32 checksum of all input read
  so (that is, total_in bytes).

	deflate() may update data_type if it can make a good guess about
  the input data type (Z_ASCII or Z_BINARY). In doubt, the data is considered
  binary. This field is only for information purposes and does not affect
  the compression algorithm in any manner.

	deflate() returns Z_OK if some progress has been made (more input
  processed or more output produced), Z_STREAM_END if all input has been
  consumed and all output has been produced (only when flush is set to
  Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
  if next_in or next_out was NULL), Z_BUF_ERROR if no progress is possible
  (for example avail_in or avail_out was zero).
*/


int deflateEnd OF((z_streamp strm));
/*
	 All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

	 deflateEnd returns Z_OK if success, Z_STREAM_ERROR if the
   stream state was inconsistent, Z_DATA_ERROR if the stream was freed
   prematurely (some input or output was discarded). In the error case,
   msg may be set but then points to a static string (which must not be
   deallocated).
*/


/* 
int inflateInit OF((z_streamp strm));

	 Initializes the internal stream state for decompression. The fields
   next_in, avail_in, zalloc, zfree and opaque must be initialized before by
   the caller. If next_in is not Z_NULL and avail_in is large enough (the exact
   value depends on the compression method), inflateInit determines the
   compression method from the zlib header and allocates all data structures
   accordingly; otherwise the allocation will be deferred to the first call of
   inflate.  If zalloc and zfree are set to Z_NULL, inflateInit updates them to
   use default allocation functions.

	 inflateInit returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_VERSION_ERROR if the zlib library version is incompatible with the
   version assumed by the caller.  msg is set to null if there is no error
   message. inflateInit does not perform any decompression apart from reading
   the zlib header if present: this will be done by inflate().  (So next_in and
   avail_in may be modified, but next_out and avail_out are unchanged.)
*/


int inflate OF((z_streamp strm, int flush));
/*
	inflate decompresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may some
  introduce some output latency (reading input without producing any output)
  except when forced to flush.

  The detailed semantics are as follows. inflate performs one or both of the
  following actions:

  - Decompress more input starting at next_in and update next_in and avail_in
	accordingly. If not all input can be processed (because there is not
	enough room in the output buffer), next_in is updated and processing
	will resume at this point for the next call of inflate().

  - Provide more output starting at next_out and update next_out and avail_out
	accordingly.  inflate() provides as much output as possible, until there
	is no more input data or no more space in the output buffer (see below
	about the flush parameter).

  Before the call of inflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating the next_* and avail_* values accordingly.
  The application can consume the uncompressed output when it wants, for
  example when the output buffer is full (avail_out == 0), or after each
  call of inflate(). If inflate returns Z_OK and with zero avail_out, it
  must be called again after making room in the output buffer because there
  might be more output pending.

	If the parameter flush is set to Z_SYNC_FLUSH, inflate flushes as much
  output as possible to the output buffer. The flushing behavior of inflate is
  not specified for values of the flush parameter other than Z_SYNC_FLUSH
  and Z_FINISH, but the current implementation actually flushes as much output
  as possible anyway.

	inflate() should normally be called until it returns Z_STREAM_END or an
  error. However if all decompression is to be performed in a single step
  (a single call of inflate), the parameter flush should be set to
  Z_FINISH. In this case all pending input is processed and all pending
  output is flushed; avail_out must be large enough to hold all the
  uncompressed data. (The size of the uncompressed data may have been saved
  by the compressor for this purpose.) The next operation on this stream must
  be inflateEnd to deallocate the decompression state. The use of Z_FINISH
  is never required, but can be used to inform inflate that a faster routine
  may be used for the single inflate() call.

	 If a preset dictionary is needed at this point (see inflateSetDictionary
  below), inflate sets strm-adler to the adler32 checksum of the
  dictionary chosen by the compressor and returns Z_NEED_DICT; otherwise 
  it sets strm->adler to the adler32 checksum of all output produced
  so (that is, total_out bytes) and returns Z_OK, Z_STREAM_END or
  an error code as described below. At the end of the stream, inflate()
  checks that its computed adler32 checksum is equal to that saved by the
  compressor and returns Z_STREAM_END only if the checksum is correct.

	inflate() returns Z_OK if some progress has been made (more input processed
  or more output produced), Z_STREAM_END if the end of the compressed data has
  been reached and all uncompressed output has been produced, Z_NEED_DICT if a
  preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
  corrupted (input stream not conforming to the zlib format or incorrect
  adler32 checksum), Z_STREAM_ERROR if the stream structure was inconsistent
  (for example if next_in or next_out was NULL), Z_MEM_ERROR if there was not
  enough memory, Z_BUF_ERROR if no progress is possible or if there was not
  enough room in the output buffer when Z_FINISH is used. In the Z_DATA_ERROR
  case, the application may then call inflateSync to look for a good
  compression block.
*/


int inflateEnd OF((z_streamp strm));
/*
	 All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

	 inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
   was inconsistent. In the error case, msg may be set but then points to a
   static string (which must not be deallocated).
*/

						/* Advanced functions */

/*
	The following functions are needed only in some special applications.
*/

/*   
int deflateInit2 OF((z_streamp strm,
									 int  level,
									 int  method,
									 int  windowBits,
									 int  memLevel,
									 int  strategy));

	 This is another version of deflateInit with more compression options. The
   fields next_in, zalloc, zfree and opaque must be initialized before by
   the caller.

	 The method parameter is the compression method. It must be Z_DEFLATED in
   this version of the library.

	 The windowBits parameter is the base two logarithm of the window size
   (the size of the history buffer).  It should be in the range 8..15 for this
   version of the library. Larger values of this parameter result in better
   compression at the expense of memory usage. The default value is 15 if
   deflateInit is used instead.

	 The memLevel parameter specifies how much memory should be allocated
   for the internal compression state. memLevel=1 uses minimum memory but
   is slow and reduces compression ratio; memLevel=9 uses maximum memory
   for optimal speed. The default value is 8. See zconf.h for total memory
   usage as a function of windowBits and memLevel.

	 The strategy parameter is used to tune the compression algorithm. Use the
   value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
   filter (or predictor), or Z_HUFFMAN_ONLY to force Huffman encoding only (no
   string match).  Filtered data consists mostly of small values with a
   somewhat random distribution. In this case, the compression algorithm is
   tuned to compress them better. The effect of Z_FILTERED is to force more
   Huffman coding and less string matching; it is somewhat intermediate
   between Z_DEFAULT and Z_HUFFMAN_ONLY. The strategy parameter only affects
   the compression ratio but not the correctness of the compressed output even
   if it is not set appropriately.

	  deflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as an invalid
   method). msg is set to null if there is no error message.  deflateInit2 does
   not perform any compression: this will be done by deflate().
*/
							
int deflateSetDictionary OF((z_streamp strm,
											 const Byte *dictionary,
											 uInt  dictLength));
/*
	 Initializes the compression dictionary from the given byte sequence
   without producing any compressed output. This function must be called
   immediately after deflateInit, deflateInit2 or deflateReset, before any
   call of deflate. The compressor and decompressor must use exactly the same
   dictionary (see inflateSetDictionary).

	 The dictionary should consist of strings (byte sequences) that are likely
   to be encountered later in the data to be compressed, with the most commonly
   used strings preferably put towards the end of the dictionary. Using a
   dictionary is most useful when the data to be compressed is short and can be
   predicted with good accuracy; the data can then be compressed better than
   with the default empty dictionary.

	 Depending on the size of the compression data structures selected by
   deflateInit or deflateInit2, a part of the dictionary may in effect be
   discarded, for example if the dictionary is larger than the window size in
   deflate or deflate2. Thus the strings most likely to be useful should be
   put at the end of the dictionary, not at the front.

	 Upon return of this function, strm->adler is set to the Adler32 value
   of the dictionary; the decompressor may later use this value to determine
   which dictionary has been used by the compressor. (The Adler32 value
   applies to the whole dictionary even if only a subset of the dictionary is
   actually used by the compressor.)

	 deflateSetDictionary returns Z_OK if success, or Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent (for example if deflate has already been called for this stream
   or if the compression method is bsort). deflateSetDictionary does not
   perform any compression: this will be done by deflate().
*/

int deflateCopy OF((z_streamp dest,
									z_streamp source));
/*
	 Sets the destination stream as a complete copy of the source stream.

	 This function can be useful when several compression strategies will be
   tried, for example when there are several ways of pre-processing the input
   data with a filter. The streams that will be discarded should then be freed
   by calling deflateEnd.  Note that deflateCopy duplicates the internal
   compression state which can be quite large, so this strategy is slow and
   can consume lots of memory.

	 deflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
   (such as zalloc being NULL). msg is left unchanged in both source and
   destination.
*/

int deflateReset OF((z_streamp strm));
/*
	 This function is equivalent to deflateEnd followed by deflateInit,
   but does not free and reallocate all the internal compression state.
   The stream will keep the same compression level and any other attributes
   that may have been set by deflateInit2.

	  deflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/

int deflateParams OF((z_streamp strm,
					  int level,
					  int strategy));
/*
	 Dynamically update the compression level and compression strategy.  The
   interpretation of level and strategy is as in deflateInit2.  This can be
   used to switch between compression and straight copy of the input data, or
   to switch to a different kind of input data requiring a different
   strategy. If the compression level is changed, the input available so far
   is compressed with the old level (and may be flushed); the new level will
   take effect only at the next call of deflate().

	 Before the call of deflateParams, the stream state must be set as for
   a call of deflate(), since the currently available input may have to
   be compressed and flushed. In particular, strm->avail_out must be non-zero.

	 deflateParams returns Z_OK if success, Z_STREAM_ERROR if the source
   stream state was inconsistent or if a parameter was invalid, Z_BUF_ERROR
   if strm->avail_out was zero.
*/

/*   
int inflateInit2 OF((z_streamp strm,
									 int  windowBits));

	 This is another version of inflateInit with an extra parameter. The
   fields next_in, avail_in, zalloc, zfree and opaque must be initialized
   before by the caller.

	 The windowBits parameter is the base two logarithm of the maximum window
   size (the size of the history buffer).  It should be in the range 8..15 for
   this version of the library. The default value is 15 if inflateInit is used
   instead. If a compressed stream with a larger window size is given as
   input, inflate() will return with the error code Z_DATA_ERROR instead of
   trying to allocate a larger window.

	  inflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as a negative
   memLevel). msg is set to null if there is no error message.  inflateInit2
   does not perform any decompression apart from reading the zlib header if
   present: this will be done by inflate(). (So next_in and avail_in may be
   modified, but next_out and avail_out are unchanged.)
*/

int inflateSetDictionary OF((z_streamp strm,
											 const Byte *dictionary,
											 uInt  dictLength));
/*
	 Initializes the decompression dictionary from the given uncompressed byte
   sequence. This function must be called immediately after a call of inflate
   if this call returned Z_NEED_DICT. The dictionary chosen by the compressor
   can be determined from the Adler32 value returned by this call of
   inflate. The compressor and decompressor must use exactly the same
   dictionary (see deflateSetDictionary).

	 inflateSetDictionary returns Z_OK if success, Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent, Z_DATA_ERROR if the given dictionary doesn't match the
   expected one (incorrect Adler32 value). inflateSetDictionary does not
   perform any decompression: this will be done by subsequent calls of
   inflate().
*/

int inflateSync OF((z_streamp strm));
/* 
	Skips invalid compressed data until a full flush point (see above the
  description of deflate with Z_FULL_FLUSH) can be found, or until all
  available input is skipped. No output is provided.

	inflateSync returns Z_OK if a full flush point has been found, Z_BUF_ERROR
  if no more input was provided, Z_DATA_ERROR if no flush point has been found,
  or Z_STREAM_ERROR if the stream structure was inconsistent. In the success
  case, the application may save the current current value of total_in which
  indicates where valid compressed data was found. In the error case, the
  application may repeatedly call inflateSync, providing more input each time,
  until success or end of the input data.
*/

int inflateReset OF((z_streamp strm));
/*
	 This function is equivalent to inflateEnd followed by inflateInit,
   but does not free and reallocate all the internal decompression state.
   The stream will keep attributes that may have been set by inflateInit2.

	  inflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/


						/* utility functions */

/*
	 The following utility functions are implemented on top of the
   basic stream-oriented functions. To simplify the interface, some
   default options are assumed (compression level and memory usage,
   standard memory allocation functions). The source code of these
   utility functions can easily be modified if you need special options.
*/

int compress OF((Byte *dest,   uLong *destLen,
								 const Byte *source, uLong sourceLen));
/*
	 Compresses the source buffer into the destination buffer.  sourceLen is
   the byte length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be at least 0.1% larger than
   sourceLen plus 12 bytes. Upon exit, destLen is the actual size of the
   compressed buffer.
	 This function can be used to compress a whole file at once if the
   input file is mmap'ed.
	 compress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer.
*/

int compress2 OF((Byte *dest,   uLong *destLen,
								  const Byte *source, uLong sourceLen,
								  int level));
/*
	 Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

	 compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/

int uncompress OF((Byte *dest,   uLong *destLen,
								   const Byte *source, uLong sourceLen));
/*
	 Decompresses the source buffer into the destination buffer.  sourceLen is
   the byte length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be large enough to hold the
   entire uncompressed data. (The size of the uncompressed data must have
   been saved previously by the compressor and transmitted to the decompressor
   by some mechanism outside the scope of this compression library.)
   Upon exit, destLen is the actual size of the compressed buffer.
	 This function can be used to decompress a whole file at once if the
   input file is mmap'ed.

	 uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer, or Z_DATA_ERROR if the input data was corrupted.
*/


typedef voidp gzFile;

gzFile gzopen  OF((const char *path, const char *mode));
/*
	 Opens a gzip (.gz) file for reading or writing. The mode parameter
   is as in fopen ("rb" or "wb") but can also include a compression level
   ("wb9") or a strategy: 'f' for filtered data as in "wb6f", 'h' for
   Huffman only compression as in "wb1h". (See the description
   of deflateInit2 for more information about the strategy parameter.)

	 gzopen can be used to read a file which is not in gzip format; in this
   case gzread will directly read from the file without decompression.

	 gzopen returns NULL if the file could not be opened or if there was
   insufficient memory to allocate the (de)compression state; errno
   can be checked to distinguish the two cases (if errno is zero, the
   zlib error is Z_MEM_ERROR).  */

gzFile gzdopen  OF((int fd, const char *mode));
/*
	 gzdopen() associates a gzFile with the file descriptor fd.  File
   descriptors are obtained from calls like open, dup, creat, pipe or
   fileno (in the file has been previously opened with fopen).
   The mode parameter is as in gzopen.
	 The next call of gzclose on the returned gzFile will also close the
   file descriptor fd, just like fclose(fdopen(fd), mode) closes the file
   descriptor fd. If you want to keep fd open, use gzdopen(dup(fd), mode).
	 gzdopen returns NULL if there was insufficient memory to allocate
   the (de)compression state.
*/

int gzsetparams OF((gzFile file, int level, int strategy));
/*
	 Dynamically update the compression level or strategy. See the description
   of deflateInit2 for the meaning of these parameters.
	 gzsetparams returns Z_OK if success, or Z_STREAM_ERROR if the file was not
   opened for writing.
*/

int    gzread  OF((gzFile file, voidp buf, unsigned len));
/*
	 Reads the given number of uncompressed bytes from the compressed file.
   If the input file was not in gzip format, gzread copies the given number
   of bytes into the buffer.
	 gzread returns the number of uncompressed bytes actually read (0 for
   end of file, -1 for error). */

int    gzwrite OF((gzFile file, 
				   const voidp buf, unsigned len));
/*
	 Writes the given number of uncompressed bytes into the compressed file.
   gzwrite returns the number of uncompressed bytes actually written
   (0 in case of error).
*/

int    gzprintf OF((gzFile file, const char *format, ...));
/*
	 Converts, formats, and writes the args to the compressed file under
   control of the format string, as in fprintf. gzprintf returns the number of
   uncompressed bytes actually written (0 in case of error).
*/

int gzputs OF((gzFile file, const char *s));
/*
	  Writes the given null-terminated string to the compressed file, excluding
   the terminating null character.
	  gzputs returns the number of characters written, or -1 in case of error.
*/

char * gzgets OF((gzFile file, char *buf, int len));
/*
	  Reads bytes from the compressed file until len-1 characters are read, or
   a newline character is read and transferred to buf, or an end-of-file
   condition is encountered.  The string is then terminated with a null
   character.
	  gzgets returns buf, or Z_NULL in case of error.
*/

int    gzputc OF((gzFile file, int c));
/*
	  Writes c, converted to an unsigned char, into the compressed file.
   gzputc returns the value that was written, or -1 in case of error.
*/

int    gzgetc OF((gzFile file));
/*
	  Reads one byte from the compressed file. gzgetc returns this byte
   or -1 in case of end of file or error.
*/

int    gzflush OF((gzFile file, int flush));
/*
	 Flushes all pending output into the compressed file. The parameter
   flush is as in the deflate() function. The return value is the zlib
   error number (see function gzerror below). gzflush returns Z_OK if
   the flush parameter is Z_FINISH and all output could be flushed.
	 gzflush should be called only when strictly necessary because it can
   degrade compression.
*/

long gzseek OF((gzFile file,
					  long offset, int whence));
/* 
	  Sets the starting position for the next gzread or gzwrite on the
   given compressed file. The offset represents a number of bytes in the
   uncompressed data stream. The whence parameter is defined as in lseek(2);
   the value SEEK_END is not supported.
	 If the file is opened for reading, this function is emulated but can be
   extremely slow. If the file is opened for writing, only forward seeks are
   supported; gzseek then compresses a sequence of zeroes up to the new
   starting position.

	  gzseek returns the resulting offset location as measured in bytes from
   the beginning of the uncompressed stream, or -1 in case of error, in
   particular if the file is opened for writing and the new starting position
   would be before the current position.
*/

int    gzrewind OF((gzFile file));
/*
	 Rewinds the given file. This function is supported only for reading.

   gzrewind(file) is equivalent to (int)gzseek(file, 0L, SEEK_SET)
*/

long    gztell OF((gzFile file));
/*
	 Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.

   gztell(file) is equivalent to gzseek(file, 0L, SEEK_CUR)
*/

int gzeof OF((gzFile file));
/*
	 Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/

int    gzclose OF((gzFile file));
/*
	 Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state. The return value is the zlib
   error number (see function gzerror below).
*/

const char * gzerror OF((gzFile file, int *errnum));
/*
	 Returns the error message for the last error which occurred on the
   given compressed file. errnum is set to zlib error number. If an
   error occurred in the file system and not in the compression library,
   errnum is set to Z_ERRNO and the application may consult errno
   to get the exact error code.
*/

						/* checksum functions */

/*
	 These functions are not related to compression but are exported
   anyway because they might be useful in applications using the
   compression library.
*/

uLong adler32 OF((uLong adler, const Byte *buf, uInt len));

/*
	 Update a running Adler-32 checksum with the bytes buf[0..len-1] and
   return the updated checksum. If buf is NULL, this function returns
   the required initial value for the checksum.
   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
   much faster. Usage example:

	 uLong adler = adler32(0L, Z_NULL, 0);

	 while (read_buffer(buffer, length) != EOF) {
	   adler = adler32(adler, buffer, length);
	 }
	 if (adler != original_adler) error();
*/

uLong crc32   OF((uLong crc, const Byte *buf, uInt len));
/*
	 Update a running crc with the bytes buf[0..len-1] and return the updated
   crc. If buf is NULL, this function returns the required initial value
   for the crc. Pre- and post-conditioning (one's complement) is performed
   within this function so it shouldn't be done by the application.
   Usage example:

	 uLong crc = crc32(0L, Z_NULL, 0);

	 while (read_buffer(buffer, length) != EOF) {
	   crc = crc32(crc, buffer, length);
	 }
	 if (crc != original_crc) error();
*/

// private stuff to not include cmdlib.h
/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif


						/* various hacks, don't look :) */

/* deflateInit and inflateInit are macros to allow checking the zlib version
 * and the compiler's view of z_stream:
 */
int deflateInit_ OF((z_streamp strm, int level,
									 const char *version, int stream_size));
int inflateInit_ OF((z_streamp strm,
									 const char *version, int stream_size));
int deflateInit2_ OF((z_streamp strm, int  level, int  method,
									  int windowBits, int memLevel,
									  int strategy, const char *version,
									  int stream_size));
int inflateInit2_ OF((z_streamp strm, int  windowBits,
									  const char *version, int stream_size));
#define deflateInit(strm, level) \
		deflateInit_((strm), (level),       ZLIB_VERSION, sizeof(z_stream))
#define inflateInit(strm) \
		inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))
#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
		deflateInit2_((strm),(level),(method),(windowBits),(memLevel),\
					  (strategy),           ZLIB_VERSION, sizeof(z_stream))
#define inflateInit2(strm, windowBits) \
		inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))


const char   * zError           OF((int err));
int            inflateSyncPoint OF((z_streamp z));
const uLong * get_crc_table    OF(());

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

extern const char *z_errmsg[10]; /* indexed by 2-zlib_error */
/* (size given to avoid silly warnings with Visual C++) */

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
  return (strm->msg = (char*)ERR_MSG(err), (err))
/* To be used only when the state is known to be valid */

		/* common constants */

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif
/* default windowBits for decompression. MAX_WBITS is for compression only */

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */

		/* target dependencies */

		/* Common defaults */

#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

#ifndef F_OPEN
#  define F_OPEN(name, mode) fopen((name), (mode))
#endif

		 /* functions */

#ifdef HAVE_STRERROR
   extern char *strerror OF((int));
#  define zstrerror(errnum) strerror(errnum)
#else
#  define zstrerror(errnum) ""
#endif

#define zmemcpy memcpy
#define zmemcmp memcmp
#define zmemzero(dest, len) memset(dest, 0, len)

/* Diagnostic functions */
#ifdef _ZIP_DEBUG_
   int z_verbose = 0;
#  define Assert(cond,msg) assert(cond);
   //{if(!(cond)) Sys_Error(msg);}
#  define Trace(x) {if (z_verbose>=0) Sys_Error x ;}
#  define Tracev(x) {if (z_verbose>0) Sys_Error x ;}
#  define Tracevv(x) {if (z_verbose>1) Sys_Error x ;}
#  define Tracec(c,x) {if (z_verbose>0 && (c)) Sys_Error x ;}
#  define Tracecv(c,x) {if (z_verbose>1 && (c)) Sys_Error x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif


typedef uLong (*check_func) OF((uLong check, const Byte *buf, uInt len));
voidp zcalloc OF((voidp opaque, unsigned items, unsigned size));
void   zcfree  OF((voidp opaque, voidp ptr));

#define ZALLOC(strm, items, size) \
		   (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidp)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}


#if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES) && \
					  !defined(CASESENSITIVITYDEFAULT_NO)
#define CASESENSITIVITYDEFAULT_NO
#endif


#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (65536)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
# define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) free(p);}
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)



/* ===========================================================================
	 Read a byte from a gz_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/

/*
static int unzlocal_getByte(FILE *fin,int *pi)
{
	unsigned char c;
	int err = fread(&c, 1, 1, fin);
	if (err==1)
	{
		*pi = (int)c;
		return UNZ_OK;
	}
	else
	{
		if (ferror(fin)) 
			return UNZ_ERRNO;
		else
			return UNZ_EOF;
	}
}
*/

/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets 
*/
static int unzlocal_getShort_IFStream (cxxifstream* fin, uLong *pX)
{
	short	v = 0;

	fin->read((char*)&v, sizeof(v));

	//fread( &v, sizeof(v), 1, fin );

	*pX = __LittleShort( v);
	return UNZ_OK;

/*
	uLong x ;
	int i;
	int err;

	err = unzlocal_getByte(fin,&i);
	x = (uLong)i;
	
	if (err==UNZ_OK)
		err = unzlocal_getByte(fin,&i);
	x += ((uLong)i)<<8;
   
	if (err==UNZ_OK)
		*pX = x;
	else
		*pX = 0;
	return err;
*/
}

static int unzlocal_getLong_IFStream (cxxifstream *fin, uLong *pX)
{
	int		v = 0;

	fin->read((char*)&v, sizeof(v));

	//fread( &v, sizeof(v), 1, fin );

	*pX = __LittleLong( v);
	return UNZ_OK;

/*
	uLong x ;
	int i;
	int err;

	err = unzlocal_getByte(fin,&i);
	x = (uLong)i;
	
	if (err==UNZ_OK)
		err = unzlocal_getByte(fin,&i);
	x += ((uLong)i)<<8;

	if (err==UNZ_OK)
		err = unzlocal_getByte(fin,&i);
	x += ((uLong)i)<<16;

	if (err==UNZ_OK)
		err = unzlocal_getByte(fin,&i);
	x += ((uLong)i)<<24;
   
	if (err==UNZ_OK)
		*pX = x;
	else
		*pX = 0;
	return err;
*/
}


/* My own strcmpi / strcasecmp */
static int strcmpcasenosensitive_internal (const char* fileName1,const char* fileName2)
{
	for (;;)
	{
		char c1=*(fileName1++);
		char c2=*(fileName2++);
		if ((c1>='a') && (c1<='z'))
			c1 -= 0x20;
		if ((c2>='a') && (c2<='z'))
			c2 -= 0x20;
		if (c1=='\0')
			return ((c2=='\0') ? 0 : -1);
		if (c2=='\0')
			return 1;
		if (c1<c2)
			return -1;
		if (c1>c2)
			return 1;
	}
}


#ifdef  CASESENSITIVITYDEFAULT_NO
#define CASESENSITIVITYDEFAULTVALUE 2
#else
#define CASESENSITIVITYDEFAULTVALUE 1
#endif

#ifndef STRCMPCASENOSENTIVEFUNCTION
#define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
#endif

/* 
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
																or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
		(like 1 on Unix, 2 on Windows)

*/
extern int unzStringFileNameCompare_IFStream (const char* fileName1,const char* fileName2,int iCaseSensitivity)
{
	if (iCaseSensitivity==0)
		iCaseSensitivity=CASESENSITIVITYDEFAULTVALUE;

	if (iCaseSensitivity==1)
		return strcmp(fileName1,fileName2);

	return STRCMPCASENOSENTIVEFUNCTION(fileName1,fileName2);
} 

#define BUFREADCOMMENT (0x400)

/*
  Locate the Central directory of a zipfile (at the end, just before
	the global comment)
*/
static uLong unzlocal_SearchCentralDir_IFStream(cxxifstream *fin)
{
	unsigned char* buf;
	uLong uSizeFile;
	uLong uBackRead;
	uLong uMaxBack=0xffff; /* maximum size of global comment */
	uLong uPosFound=0;

	fin->seekg(0, cxxifstream::end);
	
	if (!fin->good())
		return 0;

	uSizeFile = (uLong)fin->tellg();
	
	if (uMaxBack>uSizeFile)
		uMaxBack = uSizeFile;

	buf = (unsigned char*)malloc(BUFREADCOMMENT+4);
	if (buf==NULL)
		return 0;

	uBackRead = 4;
	while (uBackRead<uMaxBack)
	{
		uLong uReadSize,uReadPos ;
		int i;
		if (uBackRead+BUFREADCOMMENT>uMaxBack) 
			uBackRead = uMaxBack;
		else
			uBackRead+=BUFREADCOMMENT;
		uReadPos = uSizeFile-uBackRead ;
		
		uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ? 
					 (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
		
		fin->seekg(uReadPos, cxxifstream::beg);

		/*if (fseek(fin, uReadPos, SEEK_SET) != 0)
			break;*/

		if (!fin->good())
			break;

		/*if (fread(buf,(uInt)uReadSize,1,fin)!=1)
			break;*/

		fin->read((char*)buf, (uInt)uReadSize);

		for (i = (int)uReadSize - 3; (i--) > 0;)
		{
			if (((*(buf + i)) == 0x50) && ((*(buf + i + 1)) == 0x4b) &&
				((*(buf + i + 2)) == 0x05) && ((*(buf + i + 3)) == 0x06))
			{
				uPosFound = uReadPos + i;
				break;
			}
		}

		if (uPosFound!=0)
			break;
	}
	free(buf);
	return uPosFound;
}

extern unzFile unzReOpen_IFStream (const char* path, cxxifstream* file)
{
	unz_ifstream_s *s;

	s=(unz_ifstream_s*)malloc(sizeof(unz_ifstream_s));
	memcpy(s, (unz_ifstream_s*)file, sizeof(unz_ifstream_s));

	s->file = file;
	return (unzFile)s;
}

/*
  Open a Zip file. path contain the full pathname (by example,
	 on a Windows NT computer "c:\\test\\zlib109.zip" or on an Unix computer
	 "zlib/zlib109.zip".
	 If the zipfile cannot be opened (file don't exist or in not valid), the
	   return value is NULL.
	 Else, the return value is a unzFile Handle, usable with other function
	   of this unzip package.
*/
extern unzFile unzOpen_IFStream (const char* path)
{
	unz_ifstream_s us;
	unz_ifstream_s *s;
	uLong central_pos,uL;
	cxxifstream* fin = new cxxifstream;

	uLong number_disk;          /* number of the current dist, used for 
								   spaning ZIP, unsupported, always 0*/
	uLong number_disk_with_CD;  /* number the the disk with central dir, used
								   for spaning ZIP, unsupported, always 0*/
	uLong number_entry_CD;      /* total number of entries in
								   the central dir 
								   (same than number_entry on nospan) */

	int err=UNZ_OK;

	fin->open(path, cxxifstream::in | cxxifstream::binary);
	if (!fin->is_open())
		return NULL;

	central_pos = unzlocal_SearchCentralDir_IFStream(fin);
	if (central_pos==0)
		err=UNZ_ERRNO;

	fin->seekg(central_pos, cxxifstream::beg);

	if (fin->bad() || fin->eof() || fin->fail())
		err=UNZ_ERRNO;

	/* the signature, already checked */
	if (unzlocal_getLong_IFStream (fin,&uL)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* number of this disk */
	if (unzlocal_getShort_IFStream (fin,&number_disk)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* number of the disk with the start of the central directory */
	if (unzlocal_getShort_IFStream(fin,&number_disk_with_CD)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* total number of entries in the central dir on this disk */
	if (unzlocal_getShort_IFStream(fin,&us.gi.number_entry)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* total number of entries in the central dir */
	if (unzlocal_getShort_IFStream(fin,&number_entry_CD)!=UNZ_OK)
		err=UNZ_ERRNO;

	if ((number_entry_CD!=us.gi.number_entry) ||
		(number_disk_with_CD!=0) ||
		(number_disk!=0))
		err=UNZ_BADZIPFILE;

	/* size of the central directory */
	if (unzlocal_getLong_IFStream(fin,&us.size_central_dir)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* offset of start of central directory with respect to the 
		  starting disk number */
	if (unzlocal_getLong_IFStream(fin,&us.offset_central_dir)!=UNZ_OK)
		err=UNZ_ERRNO;

	/* zipfile comment length */
	if (unzlocal_getShort_IFStream(fin,&us.gi.size_comment)!=UNZ_OK)
		err=UNZ_ERRNO;

	if ((central_pos<us.offset_central_dir+us.size_central_dir) && 
		(err==UNZ_OK))
		err=UNZ_BADZIPFILE;

	if (err!=UNZ_OK)
	{
		fin->close();
		return NULL;
	}

	us.file=fin;
	us.byte_before_the_zipfile = central_pos -
							(us.offset_central_dir+us.size_central_dir);
	us.central_pos = central_pos;
	us.pfile_in_zip_read = NULL;
	

	s=(unz_ifstream_s*)malloc(sizeof(unz_ifstream_s));
	*s=us;
//	unzGoToFirstFile((unzFile)s);	
	return (unzFile)s;	
}


/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with unzipOpenCurrentFile (see later),
	these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */
extern int unzClose_IFStream(unzFile file)
{
	unz_ifstream_s* s;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;

	if (s->pfile_in_zip_read!=NULL)
		unzCloseCurrentFile(file);

	s->file->close();
	free(s);
	return UNZ_OK;
}


/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */
extern int unzGetGlobalInfo_IFStream (unzFile file,unz_global_info *pglobal_info)
{
	unz_ifstream_s* s;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;
	*pglobal_info=s->gi;
	return UNZ_OK;
}


/*
   Translate date/time from Dos format to tm_unz (readable more easilty)
*/
static void unzlocal_DosDateToTmuDate (uLong ulDosDate, tm_unz* ptm)
{
	uLong uDate;
	uDate = (uLong)(ulDosDate>>16);
	ptm->tm_mday = (uInt)(uDate&0x1f) ;
	ptm->tm_mon =  (uInt)((((uDate)&0x1E0)/0x20)-1) ;
	ptm->tm_year = (uInt)(((uDate&0x0FE00)/0x0200)+1980) ;

	ptm->tm_hour = (uInt) ((ulDosDate &0xF800)/0x800);
	ptm->tm_min =  (uInt) ((ulDosDate&0x7E0)/0x20) ;
	ptm->tm_sec =  (uInt) (2*(ulDosDate&0x1f)) ;
}

/*
  Get Info about the current file in the zipfile, with internal only info
*/
static int unzlocal_GetCurrentFileInfoInternal_IFStream(cxxifstream* file,
													unzFile originalHandle,
												  unz_file_info *pfile_info,
												  unz_file_info_internal 
												  *pfile_info_internal,
												  char *szFileName,
												  uLong fileNameBufferSize,
												  void *extraField,
												  uLong extraFieldBufferSize,
												  char *szComment,
												  uLong commentBufferSize)
{
	unz_s* s;
	unz_file_info file_info;
	unz_file_info_internal file_info_internal;
	int err=UNZ_OK;
	uLong uMagic;
	long lSeek=0;

	s = (unz_s*)originalHandle;

	if (!file->is_open())
		return UNZ_PARAMERROR;
	
	file->seekg(s->pos_in_central_dir + s->byte_before_the_zipfile, cxxifstream::beg);

	if (!file->good())
		err=UNZ_ERRNO;

	/*if (fseek(file,s->pos_in_central_dir+s->byte_before_the_zipfile,SEEK_SET)!=0)
		err=UNZ_ERRNO;*/


	/* we check the magic */
	if (err == UNZ_OK)
	{
		if (unzlocal_getLong_IFStream(file,&uMagic) != UNZ_OK)
			err=UNZ_ERRNO;
		else if (uMagic!=0x02014b50)
			err=UNZ_BADZIPFILE;
	}

	if (unzlocal_getShort_IFStream(file,&file_info.version) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.version_needed) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.flag) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.compression_method) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(file,&file_info.dosDate) != UNZ_OK)
		err=UNZ_ERRNO;

	unzlocal_DosDateToTmuDate(file_info.dosDate,&file_info.tmu_date);

	if (unzlocal_getLong_IFStream(file,&file_info.crc) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(file,&file_info.compressed_size) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(file,&file_info.uncompressed_size) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.size_filename) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.size_file_extra) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.size_file_comment) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.disk_num_start) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(file,&file_info.internal_fa) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(file,&file_info.external_fa) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(file,&file_info_internal.offset_curfile) != UNZ_OK)
		err=UNZ_ERRNO;

	lSeek+=file_info.size_filename;
	if ((err==UNZ_OK) && (szFileName!=NULL))
	{
		uLong uSizeRead ;
		if (file_info.size_filename<fileNameBufferSize)
		{
			*(szFileName+file_info.size_filename)='\0';
			uSizeRead = file_info.size_filename;
		}
		else
			uSizeRead = fileNameBufferSize;

		if ((file_info.size_filename > 0) && (fileNameBufferSize > 0))
		{
			/*if (fread(szFileName, (uInt)uSizeRead, 1, file) != 1)
				err = UNZ_ERRNO;*/

			file->read(szFileName, (uInt)uSizeRead);

			if (!file->good())
				err = UNZ_ERRNO;
		}
		lSeek -= uSizeRead;
	}

	
	if ((err==UNZ_OK) && (extraField!=NULL))
	{
		uLong uSizeRead ;
		if (file_info.size_file_extra<extraFieldBufferSize)
			uSizeRead = file_info.size_file_extra;
		else
			uSizeRead = extraFieldBufferSize;

		if (lSeek != 0)
		{
			/*if (fseek(file,lSeek,SEEK_CUR)==0)
				lSeek=0;
			else
				err=UNZ_ERRNO;*/

			file->seekg(lSeek, cxxifstream::beg);
			if (file->good())
				lSeek = 0;
			else
				err = UNZ_ERRNO;
		}
		if ((file_info.size_file_extra > 0) && (extraFieldBufferSize > 0))
		{
			/*if (fread(extraField, (uInt)uSizeRead, 1, file) != 1)
				err = UNZ_ERRNO;*/
			file->read((char*)extraField, (uInt)uSizeRead);

			if (!file->good())
				err = UNZ_ERRNO;
		}
		lSeek += file_info.size_file_extra - uSizeRead;
	}
	else
		lSeek+=file_info.size_file_extra; 

	
	if ((err==UNZ_OK) && (szComment!=NULL))
	{
		uLong uSizeRead ;
		if (file_info.size_file_comment<commentBufferSize)
		{
			*(szComment+file_info.size_file_comment)='\0';
			uSizeRead = file_info.size_file_comment;
		}
		else
			uSizeRead = commentBufferSize;

		if (lSeek != 0)
		{
			/*if (fseek(file, lSeek, SEEK_CUR) == 0)
				lSeek = 0;
			else
				err = UNZ_ERRNO;*/
			file->seekg(lSeek, cxxifstream::beg);
			if (file->good())
				lSeek = 0;
			else
				err = UNZ_ERRNO;
		}
		if ((file_info.size_file_comment > 0) && (commentBufferSize > 0))
		{
			/*if (fread(szComment, (uInt)uSizeRead, 1, file) != 1)
				err = UNZ_ERRNO;*/

			file->read(szComment, (uInt)uSizeRead);

			if (!file->good())
				err = UNZ_ERRNO;
		}
		lSeek+=file_info.size_file_comment - uSizeRead;
	}
	else
		lSeek+=file_info.size_file_comment;

	if ((err==UNZ_OK) && (pfile_info!=NULL))
		*pfile_info=file_info;

	if ((err==UNZ_OK) && (pfile_info_internal!=NULL))
		*pfile_info_internal=file_info_internal;

	return err;
}

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int unzGetCurrentFileInfoPosition_IFStream(cxxifstream* file, unzFile originalHandle, unsigned long* pos)
{
	unz_ifstream_s* s;

	if (originalHandle == NULL)
		return UNZ_PARAMERROR;
	s = (unz_ifstream_s*)originalHandle;

	*pos = s->pos_in_central_dir;
	return UNZ_OK;
}

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int unzSetCurrentFileInfoPosition_IFStream(cxxifstream* file, unzFile originalHandle, unsigned long pos)
{
	unz_s* s;
	int err;

	if (!file->is_open())
		return UNZ_PARAMERROR;
	s = (unz_s*)originalHandle;

	s->pos_in_central_dir = pos;
	err = unzlocal_GetCurrentFileInfoInternal_IFStream(file, originalHandle, &s->cur_file_info,
		&s->cur_file_info_internal,
		NULL, 0, NULL, 0, NULL, 0);
	s->current_file_ok = (err == UNZ_OK);
	return UNZ_OK;
}

/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem.
*/
extern int unzGetCurrentFileInfo_IFStream(cxxifstream* file, unzFile originalHandle, unz_file_info *pfile_info,
									char *szFileName, uLong fileNameBufferSize,
									void *extraField, uLong extraFieldBufferSize,
									char *szComment, uLong commentBufferSize)
{
	return unzlocal_GetCurrentFileInfoInternal_IFStream(file, originalHandle, pfile_info,NULL,
												szFileName,fileNameBufferSize,
												extraField,extraFieldBufferSize,
												szComment,commentBufferSize);
}

/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/
extern int unzGoToFirstFile_IFStream (cxxifstream* file, unzFile originalHandle)
{
	int err=UNZ_OK;
	unz_ifstream_s* s;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)originalHandle;
	s->pos_in_central_dir=s->offset_central_dir;
	s->num_file=0;
	err=unzlocal_GetCurrentFileInfoInternal_IFStream(file, originalHandle, &s->cur_file_info,
											 &s->cur_file_info_internal,
											 NULL,0,NULL,0,NULL,0);
	s->current_file_ok = (err == UNZ_OK);
	return err;
}


/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/
extern int unzGoToNextFile_IFStream (cxxifstream* file, unzFile originalHandle)
{
	unz_ifstream_s* s;	
	int err;

	if (originalHandle ==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)originalHandle;
	if (!s->current_file_ok)
		return UNZ_END_OF_LIST_OF_FILE;
	if (s->num_file+1==s->gi.number_entry)
		return UNZ_END_OF_LIST_OF_FILE;

	s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
			s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment ;
	s->num_file++;
	err = unzlocal_GetCurrentFileInfoInternal_IFStream(file, originalHandle, &s->cur_file_info,
											   &s->cur_file_info_internal,
											   NULL,0,NULL,0,NULL,0);
	s->current_file_ok = (err == UNZ_OK);
	return err;
}


/*
  Try locate the file szFileName in the zipfile.
  For the iCaseSensitivity signification, see unzipStringFileNameCompare

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/
extern int unzLocateFile_IFStream(unzFile file, const char *szFileName, int iCaseSensitivity)
{
	unz_ifstream_s* s;	
	int err;

	
	uLong num_fileSaved;
	uLong pos_in_central_dirSaved;


	if (file==NULL)
		return UNZ_PARAMERROR;

	if (strlen(szFileName)>=UNZ_MAXFILENAMEINZIP)
		return UNZ_PARAMERROR;

	s=(unz_ifstream_s*)file;
	if (!s->current_file_ok)
		return UNZ_END_OF_LIST_OF_FILE;

	num_fileSaved = s->num_file;
	pos_in_central_dirSaved = s->pos_in_central_dir;

	err = unzGoToFirstFile(file);

	while (err == UNZ_OK)
	{
		char szCurrentFileName[UNZ_MAXFILENAMEINZIP+1];
		unzGetCurrentFileInfo(file,NULL,
								szCurrentFileName,sizeof(szCurrentFileName)-1,
								NULL,0,NULL,0);
		if (unzStringFileNameCompare(szCurrentFileName,
										szFileName,iCaseSensitivity)==0)
			return UNZ_OK;
		err = unzGoToNextFile(file);
	}

	s->num_file = num_fileSaved ;
	s->pos_in_central_dir = pos_in_central_dirSaved ;
	return err;
}


/*
  Read the static header of the current zipfile
  Check the coherency of the static header and info in the end of central
		directory about this file
  store in *piSizeVar the size of extra info in static header
		(filename and size of extra field data)
*/
static int unzlocal_CheckCurrentFileCoherencyHeader_IFStream (unz_ifstream_s* s, uInt* piSizeVar,
													uLong *poffset_local_extrafield,
													uInt *psize_local_extrafield)
{
	uLong uMagic,uData,uFlags;
	uLong size_filename;
	uLong size_extra_field;
	int err=UNZ_OK;

	*piSizeVar = 0;
	*poffset_local_extrafield = 0;
	*psize_local_extrafield = 0;

	/*if (fseek(s->file,s->cur_file_info_internal.offset_curfile +
								s->byte_before_the_zipfile,SEEK_SET)!=0)
		return UNZ_ERRNO;*/

	s->file->seekg(s->cur_file_info_internal.offset_curfile +
		s->byte_before_the_zipfile, cxxifstream::beg);

	if (!s->file->good())
		return UNZ_ERRNO;

	if (err==UNZ_OK)
		if (unzlocal_getLong_IFStream(s->file,&uMagic) != UNZ_OK)
			err=UNZ_ERRNO;
		else if (uMagic!=0x04034b50)
			err=UNZ_BADZIPFILE;

	if (unzlocal_getShort_IFStream(s->file,&uData) != UNZ_OK)
		err=UNZ_ERRNO;
/*
	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
		err=UNZ_BADZIPFILE;
*/
	if (unzlocal_getShort_IFStream(s->file,&uFlags) != UNZ_OK)
		err=UNZ_ERRNO;

	if (unzlocal_getShort_IFStream(s->file,&uData) != UNZ_OK)
		err=UNZ_ERRNO;
	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compression_method))
		err=UNZ_BADZIPFILE;

	if ((err==UNZ_OK) && (s->cur_file_info.compression_method!=0) &&
						 (s->cur_file_info.compression_method!=Z_DEFLATED))
		err=UNZ_BADZIPFILE;

	if (unzlocal_getLong_IFStream(s->file,&uData) != UNZ_OK) /* date/time */
		err=UNZ_ERRNO;

	if (unzlocal_getLong_IFStream(s->file,&uData) != UNZ_OK) /* crc */
		err=UNZ_ERRNO;
	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.crc) &&
							  ((uFlags & 8)==0))
		err=UNZ_BADZIPFILE;

	if (unzlocal_getLong_IFStream(s->file,&uData) != UNZ_OK) /* size compr */
		err=UNZ_ERRNO;
	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compressed_size) &&
							  ((uFlags & 8)==0))
		err=UNZ_BADZIPFILE;

	if (unzlocal_getLong_IFStream(s->file,&uData) != UNZ_OK) /* size uncompr */
		err=UNZ_ERRNO;
	else if ((err==UNZ_OK) && (uData!=s->cur_file_info.uncompressed_size) && 
							  ((uFlags & 8)==0))
		err=UNZ_BADZIPFILE;


	if (unzlocal_getShort_IFStream(s->file,&size_filename) != UNZ_OK)
		err=UNZ_ERRNO;
	else if ((err==UNZ_OK) && (size_filename!=s->cur_file_info.size_filename))
		err=UNZ_BADZIPFILE;

	*piSizeVar += (uInt)size_filename;

	if (unzlocal_getShort_IFStream(s->file,&size_extra_field) != UNZ_OK)
		err=UNZ_ERRNO;
	*poffset_local_extrafield= s->cur_file_info_internal.offset_curfile +
									SIZEZIPLOCALHEADER + size_filename;
	*psize_local_extrafield = (uInt)size_extra_field;

	*piSizeVar += (uInt)size_extra_field;

	return err;
}
												
/*
  Open for reading data the current file in the zipfile.
  If there is no error and the file is opened, the return value is UNZ_OK.
*/
extern int unzOpen_IFStreamCurrentFile (unzFile file)
{
	int err=UNZ_OK;
	int Store;
	uInt iSizeVar;
	unz_ifstream_s* s;
	file_in_zip_read_info_ifstream_s* pfile_in_zip_read_info;
	uLong offset_local_extrafield;  /* offset of the static extra field */
	uInt  size_local_extrafield;    /* size of the static extra field */

	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;
	if (!s->current_file_ok)
		return UNZ_PARAMERROR;

	if (s->pfile_in_zip_read != NULL)
		unzCloseCurrentFile(file);

	if (unzlocal_CheckCurrentFileCoherencyHeader_IFStream(s,&iSizeVar,
				&offset_local_extrafield,&size_local_extrafield)!=UNZ_OK)
		return UNZ_BADZIPFILE;

	pfile_in_zip_read_info = (file_in_zip_read_info_ifstream_s*)
										malloc(sizeof(file_in_zip_read_info_ifstream_s));
	if (pfile_in_zip_read_info==NULL)
		return UNZ_INTERNALERROR;

	pfile_in_zip_read_info->read_buffer=(char*)malloc(UNZ_BUFSIZE);
	pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
	pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
	pfile_in_zip_read_info->pos_local_extrafield=0;

	if (pfile_in_zip_read_info->read_buffer==NULL)
	{
		free(pfile_in_zip_read_info);
		return UNZ_INTERNALERROR;
	}

	pfile_in_zip_read_info->stream_initialised=0;
	
	if ((s->cur_file_info.compression_method!=0) &&
		(s->cur_file_info.compression_method!=Z_DEFLATED))
		err=UNZ_BADZIPFILE;
	Store = s->cur_file_info.compression_method==0;

	pfile_in_zip_read_info->crc32_wait=s->cur_file_info.crc;
	pfile_in_zip_read_info->crc32=0;
	pfile_in_zip_read_info->compression_method =
			s->cur_file_info.compression_method;
	pfile_in_zip_read_info->file=s->file;
	pfile_in_zip_read_info->byte_before_the_zipfile=s->byte_before_the_zipfile;

	pfile_in_zip_read_info->stream.total_out = 0;

	if (!Store)
	{
	  pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
	  pfile_in_zip_read_info->stream.zfree = (free_func)0;
	  pfile_in_zip_read_info->stream.opaque = (voidp)0; 
	  
	  err=inflateInit2(&pfile_in_zip_read_info->stream, -MAX_WBITS);
	  if (err == Z_OK)
		pfile_in_zip_read_info->stream_initialised=1;
		/* windowBits is passed < 0 to tell that there is no zlib header.
		 * Note that in this case inflate *requires* an extra "dummy" byte
		 * after the compressed stream in order to complete decompression and
		 * return Z_STREAM_END. 
		 * In unzip, i don't wait absolutely Z_STREAM_END because I known the 
		 * size of both compressed and uncompressed data
		 */
	}
	pfile_in_zip_read_info->rest_read_compressed = 
			s->cur_file_info.compressed_size ;
	pfile_in_zip_read_info->rest_read_uncompressed = 
			s->cur_file_info.uncompressed_size ;

	
	pfile_in_zip_read_info->pos_in_zipfile = 
			s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + 
			  iSizeVar;
	
	pfile_in_zip_read_info->stream.avail_in = (uInt)0;


	s->pfile_in_zip_read = pfile_in_zip_read_info;
	return UNZ_OK;
}


/*
  Read bytes from the current file.
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of byte copied if somes bytes are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
	(UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/
extern int unzReadCurrentFile_IFStream  (unzFile file, cxxifstream* stream, void *buf, unsigned len)
{
	int err=UNZ_OK;
	uInt iRead = 0;
	unz_s* s;
	file_in_zip_read_info_s* pfile_in_zip_read_info;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_s*)file;
	pfile_in_zip_read_info=s->pfile_in_zip_read;

	if (pfile_in_zip_read_info==NULL)
		return UNZ_PARAMERROR;


	if ((pfile_in_zip_read_info->read_buffer == NULL))
		return UNZ_END_OF_LIST_OF_FILE;
	if (len==0)
		return 0;

	pfile_in_zip_read_info->stream.next_out = (Byte*)buf;

	pfile_in_zip_read_info->stream.avail_out = (uInt)len;
	
	if (len>pfile_in_zip_read_info->rest_read_uncompressed)
		pfile_in_zip_read_info->stream.avail_out = 
		  (uInt)pfile_in_zip_read_info->rest_read_uncompressed;

	while (pfile_in_zip_read_info->stream.avail_out>0)
	{
		if ((pfile_in_zip_read_info->stream.avail_in==0) &&
			(pfile_in_zip_read_info->rest_read_compressed>0))
		{
			uInt uReadThis = UNZ_BUFSIZE;
			if (pfile_in_zip_read_info->rest_read_compressed<uReadThis)
				uReadThis = (uInt)pfile_in_zip_read_info->rest_read_compressed;
			if (uReadThis == 0)
				return UNZ_EOF;
			if (s->cur_file_info.compressed_size == pfile_in_zip_read_info->rest_read_compressed)
			{
				/*if (fseek(filetest,
					pfile_in_zip_read_info->pos_in_zipfile +
					pfile_in_zip_read_info->byte_before_the_zipfile, SEEK_SET) != 0)
				{
					return UNZ_ERRNO;
				}*/

				stream->seekg(pfile_in_zip_read_info->pos_in_zipfile +
					pfile_in_zip_read_info->byte_before_the_zipfile, cxxifstream::beg);

				if (!stream->good())
					return UNZ_ERRNO;
			}
			/*if (fread(pfile_in_zip_read_info->read_buffer,uReadThis,1,
						 filetest)!=1)
				return UNZ_ERRNO;*/

			stream->read(pfile_in_zip_read_info->read_buffer, uReadThis);

			if (!stream->good())
			{
				return UNZ_ERRNO;
			}

			pfile_in_zip_read_info->pos_in_zipfile += uReadThis;

			pfile_in_zip_read_info->rest_read_compressed-=uReadThis;
			
			pfile_in_zip_read_info->stream.next_in = 
				(Byte*)pfile_in_zip_read_info->read_buffer;
			pfile_in_zip_read_info->stream.avail_in = (uInt)uReadThis;
		}

		if (pfile_in_zip_read_info->compression_method==0)
		{
			uInt uDoCopy,i ;
			if (pfile_in_zip_read_info->stream.avail_out < 
							pfile_in_zip_read_info->stream.avail_in)
				uDoCopy = pfile_in_zip_read_info->stream.avail_out ;
			else
				uDoCopy = pfile_in_zip_read_info->stream.avail_in ;
				
			for (i=0;i<uDoCopy;i++)
				*(pfile_in_zip_read_info->stream.next_out+i) =
						*(pfile_in_zip_read_info->stream.next_in+i);
					
			pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,
								pfile_in_zip_read_info->stream.next_out,
								uDoCopy);
			pfile_in_zip_read_info->rest_read_uncompressed-=uDoCopy;
			pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
			pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
			pfile_in_zip_read_info->stream.next_out += uDoCopy;
			pfile_in_zip_read_info->stream.next_in += uDoCopy;
			pfile_in_zip_read_info->stream.total_out += uDoCopy;
			iRead += uDoCopy;
		}
		else
		{
			uLong uTotalOutBefore,uTotalOutAfter;
			const Byte *bufBefore;
			uLong uOutThis;
			int flush=Z_SYNC_FLUSH;

			uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
			bufBefore = pfile_in_zip_read_info->stream.next_out;

			/*
			if ((pfile_in_zip_read_info->rest_read_uncompressed ==
					 pfile_in_zip_read_info->stream.avail_out) &&
				(pfile_in_zip_read_info->rest_read_compressed == 0))
				flush = Z_FINISH;
			*/
			err=inflate(&pfile_in_zip_read_info->stream,flush);

			uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
			uOutThis = uTotalOutAfter-uTotalOutBefore;
			
			pfile_in_zip_read_info->crc32 = 
				crc32(pfile_in_zip_read_info->crc32,bufBefore,
						(uInt)(uOutThis));

			pfile_in_zip_read_info->rest_read_uncompressed -=
				uOutThis;

			iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);
			
			if (err==Z_STREAM_END)
				return (iRead==0) ? UNZ_EOF : iRead;
			if (err!=Z_OK) 
				break;
		}
	}

	if (err==Z_OK)
		return iRead;
	return err;
}

/*
  Read extra field from the current file (opened by unzOpen_IFStreamCurrentFile)
  This is the static-header version of the extra field (sometimes, there is
	more info in the static-header version than in the central-header)

  if buf==NULL, it return the size of the static extra field that can be read

  if buf!=NULL, len is the size of the buffer, the extra header is copied in
	buf.
  the return value is the number of bytes copied in buf, or (if <0) 
	the error code
*/
extern int unzGetLocalExtrafield_IFStream (unzFile file,void *buf,unsigned len)
{
	unz_ifstream_s* s;
	file_in_zip_read_info_ifstream_s* pfile_in_zip_read_info;
	uInt read_now;
	uLong size_to_read;

	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;
	pfile_in_zip_read_info=s->pfile_in_zip_read;

	if (pfile_in_zip_read_info==NULL)
		return UNZ_PARAMERROR;

	size_to_read = (pfile_in_zip_read_info->size_local_extrafield - 
				pfile_in_zip_read_info->pos_local_extrafield);

	if (buf==NULL)
		return (int)size_to_read;
	
	if (len>size_to_read)
		read_now = (uInt)size_to_read;
	else
		read_now = (uInt)len ;

	if (read_now==0)
		return 0;
	
	/*if (fseek(pfile_in_zip_read_info->file,
			  pfile_in_zip_read_info->offset_local_extrafield + 
			  pfile_in_zip_read_info->pos_local_extrafield,SEEK_SET)!=0)
		return UNZ_ERRNO;*/

	pfile_in_zip_read_info->file->seekg(pfile_in_zip_read_info->offset_local_extrafield +
		pfile_in_zip_read_info->pos_local_extrafield, cxxifstream::beg);

	if (!pfile_in_zip_read_info->file->good())
		return UNZ_ERRNO;

	/*if (fread(buf,(uInt)size_to_read,1,pfile_in_zip_read_info->file)!=1)
		return UNZ_ERRNO;*/

	pfile_in_zip_read_info->file->read((char*)buf, (uInt)size_to_read);

	return (int)read_now;
}

/*
  Open for reading data the current file in the zipfile.
  If there is no error and the file is opened, the return value is UNZ_OK.
*/
extern int unzOpenCurrentFile_IFStream(unzFile file)
{
	int err = UNZ_OK;
	int Store;
	uInt iSizeVar;
	unz_ifstream_s* s;
	file_in_zip_read_info_ifstream_s* pfile_in_zip_read_info;
	uLong offset_local_extrafield;  /* offset of the static extra field */
	uInt  size_local_extrafield;    /* size of the static extra field */

	if (file == NULL)
		return UNZ_PARAMERROR;
	s = (unz_ifstream_s*)file;
	if (!s->current_file_ok)
		return UNZ_PARAMERROR;

	if (s->pfile_in_zip_read != NULL)
		unzCloseCurrentFile_IFStream(file);

	if (unzlocal_CheckCurrentFileCoherencyHeader_IFStream(s, &iSizeVar,
		&offset_local_extrafield, &size_local_extrafield) != UNZ_OK)
		return UNZ_BADZIPFILE;

	pfile_in_zip_read_info = (file_in_zip_read_info_ifstream_s*)
		malloc(sizeof(file_in_zip_read_info_ifstream_s));
	if (pfile_in_zip_read_info == NULL)
		return UNZ_INTERNALERROR;

	pfile_in_zip_read_info->read_buffer = (char*)malloc(UNZ_BUFSIZE);
	pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
	pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
	pfile_in_zip_read_info->pos_local_extrafield = 0;

	if (pfile_in_zip_read_info->read_buffer == NULL)
	{
		free(pfile_in_zip_read_info);
		return UNZ_INTERNALERROR;
	}

	pfile_in_zip_read_info->stream_initialised = 0;

	if ((s->cur_file_info.compression_method != 0) &&
		(s->cur_file_info.compression_method != Z_DEFLATED))
		err = UNZ_BADZIPFILE;
	Store = s->cur_file_info.compression_method == 0;

	pfile_in_zip_read_info->crc32_wait = s->cur_file_info.crc;
	pfile_in_zip_read_info->crc32 = 0;
	pfile_in_zip_read_info->compression_method =
		s->cur_file_info.compression_method;
	pfile_in_zip_read_info->file = s->file;
	pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;

	pfile_in_zip_read_info->stream.total_out = 0;

	if (!Store)
	{
		pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
		pfile_in_zip_read_info->stream.zfree = (free_func)0;
		pfile_in_zip_read_info->stream.opaque = (voidp)0;

		err = inflateInit2(&pfile_in_zip_read_info->stream, -MAX_WBITS);
		if (err == Z_OK)
			pfile_in_zip_read_info->stream_initialised = 1;
		/* windowBits is passed < 0 to tell that there is no zlib header.
		 * Note that in this case inflate *requires* an extra "dummy" byte
		 * after the compressed stream in order to complete decompression and
		 * return Z_STREAM_END.
		 * In unzip, i don't wait absolutely Z_STREAM_END because I known the
		 * size of both compressed and uncompressed data
		 */
	}
	pfile_in_zip_read_info->rest_read_compressed =
		s->cur_file_info.compressed_size;
	pfile_in_zip_read_info->rest_read_uncompressed =
		s->cur_file_info.uncompressed_size;


	pfile_in_zip_read_info->pos_in_zipfile =
		s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER +
		iSizeVar;

	pfile_in_zip_read_info->stream.avail_in = (uInt)0;


	s->pfile_in_zip_read = pfile_in_zip_read_info;
	return UNZ_OK;
}

/*
  Close the file in zip opened with unzipOpenCurrentFile
  Return UNZ_CRCERROR if all the file was read but the CRC is not good
*/
extern int unzCloseCurrentFile_IFStream (unzFile file)
{
	int err=UNZ_OK;

	unz_ifstream_s* s;
	file_in_zip_read_info_ifstream_s* pfile_in_zip_read_info;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;
	pfile_in_zip_read_info=s->pfile_in_zip_read;

	if (pfile_in_zip_read_info==NULL)
		return UNZ_PARAMERROR;


	if (pfile_in_zip_read_info->rest_read_uncompressed == 0)
	{
		if (pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait)
			err=UNZ_CRCERROR;
	}


	free(pfile_in_zip_read_info->read_buffer);
	pfile_in_zip_read_info->read_buffer = NULL;
	if (pfile_in_zip_read_info->stream_initialised)
		inflateEnd(&pfile_in_zip_read_info->stream);

	pfile_in_zip_read_info->stream_initialised = 0;
	free(pfile_in_zip_read_info);

	s->pfile_in_zip_read=NULL;

	return err;
}


/*
  Get the global comment string of the ZipFile, in the szComment buffer.
  uSizeBuf is the size of the szComment buffer.
  return the number of byte copied or an error code <0
*/
extern int unzGetGlobalComment_IFStream(unzFile file, char *szComment, uLong uSizeBuf)
{
	unz_ifstream_s* s;
	uLong uReadThis ;
	if (file==NULL)
		return UNZ_PARAMERROR;
	s=(unz_ifstream_s*)file;

	uReadThis = uSizeBuf;
	if (uReadThis>s->gi.size_comment)
		uReadThis = s->gi.size_comment;

	/*if (fseek(s->file,s->central_pos+22,SEEK_SET)!=0)
		return UNZ_ERRNO;*/

	s->file->seekg(s->central_pos + 22, cxxifstream::beg);

	if (!s->file->good())
		return UNZ_ERRNO;

	if (uReadThis>0)
	{
	  *szComment='\0';
	  /*if (fread(szComment,(uInt)uReadThis,1,s->file)!=1)
		return UNZ_ERRNO;*/

		s->file->read(szComment, (uInt)uReadThis);
	}

	if ((szComment != NULL) && (uSizeBuf > s->gi.size_comment))
		*(szComment+s->gi.size_comment)='\0';
	return (int)uReadThis;
}

/* infblock.h -- header to use infblock.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_blocks_state;
typedef struct inflate_blocks_state inflate_blocks_statef;

extern inflate_blocks_statef * inflate_blocks_new OF((
	z_streamp z,
	check_func c,               /* check function */
	uInt w));                   /* window size */

extern int inflate_blocks OF((
	inflate_blocks_statef *,
	z_streamp ,
	int));                      /* initial return code */

extern void inflate_blocks_reset OF((
	inflate_blocks_statef *,
	z_streamp ,
	uLong *));                  /* check value on output */

extern int inflate_blocks_free OF((
	inflate_blocks_statef *,
	z_streamp));

extern void inflate_set_dictionary OF((
	inflate_blocks_statef *s,
	const Byte *d,  /* dictionary */
	uInt  n));       /* dictionary length */

extern int inflate_blocks_sync_point OF((
	inflate_blocks_statef *s));

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

/* Table for deflate from PKZIP's appnote.txt. */
static const uInt border[] = { /* Order of the bit length code lengths */
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model). */

typedef struct inflate_huft_s inflate_huft;

struct inflate_huft_s {
  union {
	struct {
	  Byte Exop;        /* number of extra bits or operation */
	  Byte Bits;        /* number of bits in this code or subcode */
	} what;
	uInt pad;           /* pad structure to a power of 2 (4 bytes for */
  } word;               /*  16-bit, 8 bytes for 32-bit int's) */
  uInt base;            /* literal, length base, distance base,
						   or table offset */
};

/* Maximum size of dynamic tree.  The maximum found in a long but non-
   exhaustive search was 1004 huft structures (850 for length/literals
   and 154 for distances, the latter actually the result of an
   exhaustive search).  The actual maximum is not known, but the
   value below is more than safe. */
#define MANY 1440

extern int inflate_trees_bits OF((
	uInt *,                    /* 19 code lengths */
	uInt *,                    /* bits tree desired/actual depth */
	inflate_huft * *,       /* bits tree result */
	inflate_huft *,             /* space for trees */
	z_streamp));                /* for messages */

extern int inflate_trees_dynamic OF((
	uInt,                       /* number of literal/length codes */
	uInt,                       /* number of distance codes */
	uInt *,                    /* that many (total) code lengths */
	uInt *,                    /* literal desired/actual bit depth */
	uInt *,                    /* distance desired/actual bit depth */
	inflate_huft * *,       /* literal/length tree result */
	inflate_huft * *,       /* distance tree result */
	inflate_huft *,             /* space for trees */
	z_streamp));                /* for messages */

extern int inflate_trees_fixed OF((
	uInt *,                    /* literal desired/actual bit depth */
	uInt *,                    /* distance desired/actual bit depth */
	inflate_huft * *,       /* literal/length tree result */
	inflate_huft * *,       /* distance tree result */
	z_streamp));                /* for memory allocation */


/* infcodes.h -- header to use infcodes.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_codes_state;
typedef struct inflate_codes_state inflate_codes_statef;

extern inflate_codes_statef *inflate_codes_new OF((
	uInt, uInt,
	inflate_huft *, inflate_huft *,
	z_streamp ));

extern int inflate_codes OF((
	inflate_blocks_statef *,
	z_streamp ,
	int));

extern void inflate_codes_free OF((
	inflate_codes_statef *,
	z_streamp ));

/* infutil.h -- types and macros common to blocks and codes
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#ifndef _INFUTIL_H
#define _INFUTIL_H

typedef enum {
	  TYPE,     /* get type bits (3, including end bit) */
	  LENS,     /* get lengths for stored */
	  STORED,   /* processing stored block */
	  TABLE,    /* get table lengths */
	  BTREE,    /* get bit lengths tree for a dynamic block */
	  DTREE,    /* get length, distance trees for a dynamic block */
	  CODES,    /* processing fixed or dynamic block */
	  DRY,      /* output remaining window bytes */
	  DONE,     /* finished last block, done */
	  BAD}      /* got a data error--stuck here */
inflate_block_mode;

/* inflate blocks semi-private state */
struct inflate_blocks_state {

  /* mode */
  inflate_block_mode  mode;     /* current inflate_block mode */

  /* mode dependent information */
  union {
	uInt left;          /* if STORED, bytes left to copy */
	struct {
	  uInt table;               /* table lengths (14 bits) */
	  uInt index;               /* index into blens (or border) */
	  uInt *blens;             /* bit lengths of codes */
	  uInt bb;                  /* bit length tree depth */
	  inflate_huft *tb;         /* bit length decoding tree */
	} trees;            /* if DTREE, decoding info for trees */
	struct {
	  inflate_codes_statef 
		 *codes;
	} decode;           /* if CODES, current state */
  } sub;                /* submode */
  uInt last;            /* true if this block is the last block */

  /* mode independent information */
  uInt bitk;            /* bits in bit buffer */
  uLong bitb;           /* bit buffer */
  inflate_huft *hufts;  /* single malloc for tree space */
  Byte *window;        /* sliding window */
  Byte *end;           /* one byte after sliding window */
  Byte *read;          /* window read pointer */
  Byte *write;         /* window write pointer */
  check_func checkfn;   /* check function */
  uLong check;          /* check on output */

};


/* defines for inflate input/output */
/*   update pointers and return */
#define UPDBITS {s->bitb=b;s->bitk=k;}
#define UPDIN {z->avail_in=n;z->total_in+=p-z->next_in;z->next_in=p;}
#define UPDOUT {s->write=q;}
#define UPDATE {UPDBITS UPDIN UPDOUT}
#define LEAVE {UPDATE return inflate_flush(s,z,r);}
/*   get bytes and bits */
#define LOADIN {p=z->next_in;n=z->avail_in;b=s->bitb;k=s->bitk;}
#define NEEDBYTE {if(n)r=Z_OK;else LEAVE}
#define NEXTBYTE (n--,*p++)
#define NEEDBITS(j) {while(k<(j)){NEEDBYTE;b|=((uLong)NEXTBYTE)<<k;k+=8;}}
#define DUMPBITS(j) {b>>=(j);k-=(j);}
/*   output bytes */
#define WAVAIL (uInt)(q<s->read?s->read-q-1:s->end-q)
#define LOADOUT {q=s->write;m=(uInt)WAVAIL;}
#define WRAP {if(q==s->end&&s->read!=s->window){q=s->window;m=(uInt)WAVAIL;}}
#define FLUSH {UPDOUT r=inflate_flush(s,z,r); LOADOUT}
#define NEEDOUT {if(m==0){WRAP if(m==0){FLUSH WRAP if(m==0) LEAVE}}r=Z_OK;}
#define OUTBYTE(a) {*q++=(Byte)(a);m--;}
/*   load static pointers */
#define LOAD {LOADIN LOADOUT}

#endif

								
/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
	  stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
	  15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
	  would be more efficient, but perhaps a little confusing.)  If two
	  codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
	  sent if there are none.  (History: a pre 2.0 version of PKZIP would
	  store blocks with no distance codes, but this was discovered to be
	  too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
	  zero distance codes, which is sent as one code of zero bits in
	  length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
	  end-of-block.  Note however that the static length tree defines
	  288 codes just to fill out the Huffman codes.  Codes 286 and 287
	  cannot be used though, since there is no length base or extra bits
	  defined for them.  Similarily, there are up to 30 distance codes.
	  However, static trees define 32 codes (all 5 bits) to fill out the
	  Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
	  The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
	  literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
	  (1+6+6).  Therefore, to output three times the length, you output
	  three codes (1+1+1), whereas to output four times the same length,
	  you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
	  only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
	  really is 258.  The last length deserves its own, short code
	  since it gets used a lot in very redundant files.  The length
	  258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
	  single stream of lengths.  It is possible (and advantageous) for
	  a repeat code (16, 17, or 18) to go across the boundary between
	  the two sets of lengths.
 */

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

#define BASE 65521L /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#undef DO1
#undef DO2
#undef DO4
#undef DO8

#define DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

#define iNEEDBYTE {if(z->avail_in==0)return r;r=f;}
#define iNEXTBYTE (z->avail_in--,z->total_in++,*z->next_in++)

