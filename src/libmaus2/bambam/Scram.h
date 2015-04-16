/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if ! defined(LIBMAUS_BAMBAM_SCRAM_H)
#define LIBMAUS_BAMBAM_SCRAM_H

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(LIBMAUS_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef size_t (*scram_cram_io_C_FILE_fread_t)(void *ptr, size_t size, size_t nmemb, void *stream);
typedef int    (*scram_cram_io_C_FILE_fseek_t)(void * fd, off_t offset, int whence);
typedef off_t   (*scram_cram_io_C_FILE_ftell_t)(void * fd);

typedef struct {
    void                         *user_data;
    scram_cram_io_C_FILE_fread_t  fread_callback;
    scram_cram_io_C_FILE_fseek_t  fseek_callback;
    scram_cram_io_C_FILE_ftell_t  ftell_callback;
} scram_cram_io_input_t;

typedef scram_cram_io_input_t * (*scram_cram_io_allocate_read_input_t)  (char const * filename, int const decompress);
typedef scram_cram_io_input_t * (*scram_cram_io_deallocate_read_input_t)(scram_cram_io_input_t * obj);

typedef struct _libmaus2_bambam_ScramDecoder
{
	/** input file name */
	char * filename;
	/** input mode r, rb, or rc for SAM, BAM and CRAM */
	char * mode;
	/** reference file name */
	char * referencefilename;

	/** scram decoder */
	void * decoder;
	/** scram object */
	void * vseq;
	
	/** SAM/BAM text header */
	char * header;
	/** length of SAM/BAM header */
	uint64_t headerlen;
	
	/** alignment block */
	uint8_t const * buffer;
	/** length of alignment block in bytes */
	uint64_t        blocksize;
} libmaus2_bambam_ScramDecoder;

typedef struct _libmaus2_bambam_ScramHeader
{
	/** text */
	char * text;
	/** header structure */
	void * header;
} libmaus2_bambam_ScramHeader;

typedef struct _libmaus2_bambam_ScramEncoder
{
	/** output file name */
	char * filename;
	/** output mode */
	char * mode;
	/** reference file name */
	char * referencefilename;
	/** header object */
	libmaus2_bambam_ScramHeader * header;
	
	/** scram encoder */
	void * encoder;
	
	/** output buffer */
	char * buffer;
	/** size of buffer in bytes */
	uint64_t buffersize;
} libmaus2_bambam_ScramEncoder;

typedef libmaus2_bambam_ScramDecoder *(* libmaus2_bambam_ScramDecoder_New_Type)(char const * rfilename, char const * rmode, char const * rreferencefilename);
typedef libmaus2_bambam_ScramDecoder *(* libmaus2_bambam_ScramDecoder_New_Cram_Input_Callback_Type)(
	char const * filename,
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename
);
typedef libmaus2_bambam_ScramDecoder *(* libmaus2_bambam_ScramDecoder_New_Range_Type)(
	char const * rfilename, 
	char const * rmode, 
	char const * rreferencefilename, 
	char const * ref, int64_t 
	const start, 
	int64_t const end
);
typedef libmaus2_bambam_ScramDecoder *(* libmaus2_bambam_ScramDecoder_New_Cram_Input_Callback_Range_Type)(
	char const * rfilename, 
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename, 
	char const * ref, int64_t 
	const start, 
	int64_t const end
);
typedef libmaus2_bambam_ScramDecoder *(* libmaus2_bambam_ScramDecoder_Delete_Type)(libmaus2_bambam_ScramDecoder * object);
typedef int (* libmaus2_bambam_ScramDecoder_Decode_Type)(libmaus2_bambam_ScramDecoder * object);

typedef libmaus2_bambam_ScramHeader *(* libmaus2_bambam_ScramHeader_New_Type   )(char const * headertext);
typedef libmaus2_bambam_ScramHeader *(* libmaus2_bambam_ScramHeader_Delete_Type)(libmaus2_bambam_ScramHeader * header);

typedef libmaus2_bambam_ScramEncoder *(* libmaus2_bambam_ScramEncoder_New_Type)(
	char const * headertext,
	char const * rfilename, char const * rmode, char const * rreferencefilename,
	int const rverbose
);
typedef libmaus2_bambam_ScramEncoder *(* libmaus2_bambam_ScramEncoder_Delete_Type)(libmaus2_bambam_ScramEncoder * object);
typedef int (* libmaus2_bambam_ScramEncoder_Encode_Type)(libmaus2_bambam_ScramEncoder * encoder, uint8_t const * seq, uint64_t const len);

/**
 * allocate scram decoder
 *
 * @param rfilename input file name, - for stdin
 * @param rmode input mode; r, rb or rc for SAM, BAM or CRAM
 * @param rreferencefilename reference filename, null pointer for none
 * @return scram decoder object or null if creation failed
 **/
libmaus2_bambam_ScramDecoder * libmaus2_bambam_ScramDecoder_New(char const * rfilename, char const * rmode, char const * rreferencefilename);
/**
 * allocate CRAM decoder via callbacks
 *
 * @param rfilename input file name, - for stdin
 * @param callback_allocate_function file allocation callback
 * @param callback_deallocate_function file deallocation callback
 * @param bufsize buffer size for input
 * @param rreferencefilename reference filename, null pointer for none
 * @return scram decoder object or null if creation failed
 **/
libmaus2_bambam_ScramDecoder * libmaus2_bambam_ScramDecoder_New_Cram_Input_Callback(
	char const * filename,
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename
);
/**
 * allocate scram decoder
 *
 * @param rfilename input file name, - for stdin
 * @param rmode input mode; r, rb or rc for SAM, BAM or CRAM
 * @param rreferencefilename reference filename, null pointer for none
 * @param ref reference sequence id
 * @param start range lower end
 * @param end range upper end
 * @return scram decoder object or null if creation failed
 **/
libmaus2_bambam_ScramDecoder * libmaus2_bambam_ScramDecoder_New_Range(char const * rfilename, char const * rmode, char const * rreferencefilename, char const * ref, int64_t const start, int64_t const end);
/**
 * allocate CRAM decoder via callbacks
 *
 * @param rfilename input file name, - for stdin
 * @param callback_allocate_function file allocation callback
 * @param callback_deallocate_function file deallocation callback
 * @param bufsize buffer size for input
 * @param rreferencefilename reference filename, null pointer for none
 * @return scram decoder object or null if creation failed
 **/
libmaus2_bambam_ScramDecoder * libmaus2_bambam_ScramDecoder_New_Cram_Input_Callback_Range(
	char const * filename,
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename,
	char const * ref, 
	int64_t const start,
	int64_t const end
);
/**
 * deallocate scram decoder
 *
 * @param scram decoder object
 * @return null pointer
 **/
libmaus2_bambam_ScramDecoder * libmaus2_bambam_ScramDecoder_Delete(libmaus2_bambam_ScramDecoder * object);
/**
 * decode next alignment
 *
 * @param object decoder object
 * @return 0=ok, -1=eof, -2=failure
 **/
int libmaus2_bambam_ScramDecoder_Decode(libmaus2_bambam_ScramDecoder * object);

/**
 * construct a header from a given text
 *
 * @param headertext plain text header
 * @return scram header object
 **/
libmaus2_bambam_ScramHeader * libmaus2_bambam_ScramHeader_New(char const * headertext);
/**
 * deallocate scram header object
 *
 * @param header object
 * @return null
 **/
libmaus2_bambam_ScramHeader * libmaus2_bambam_ScramHeader_Delete(libmaus2_bambam_ScramHeader * header);

/**
 * construct an encoder
 *
 * @param header scram header object
 * @param rfilename output file name (- for standard output)
 * @param rmode file mode
 * @param rreferencefilename name of reference for cram output
 **/
libmaus2_bambam_ScramEncoder * libmaus2_bambam_ScramEncoder_New(
	char const * headertext,
	char const * rfilename, 
	char const * rmode, 
	char const * rreferencefilename,
	int const rverbose
);
/**
 * deallocate scram encoder object
 *
 * @param object encoder object
 * @return null
 **/
libmaus2_bambam_ScramEncoder * libmaus2_bambam_ScramEncoder_Delete(libmaus2_bambam_ScramEncoder * object);
/**
 * encode sequence
 *
 * @param seq sequence (BAM block)
 * @param len length of BAM block in bytes
 * @return -1 on failure
 **/
int libmaus2_bambam_ScramEncoder_Encode(libmaus2_bambam_ScramEncoder * encoder, uint8_t const * seq, uint64_t const len);

#if defined(__cplusplus)
}
#endif

#endif
