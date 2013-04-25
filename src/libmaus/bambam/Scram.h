/**
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
**/

#if ! defined(LIBMAUS_BAMBAM_SCRAM_H)
#define LIBMAUS_BAMBAM_SCRAM_H

#include <libmaus/LibMausConfig.hpp>

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef struct _libmaus_bambam_ScramDecoder
{
	char * filename;
	char * mode;
	char * referencefilename;

	void * decoder;
	void * vseq;
	
	char * header;
	uint64_t headerlen;
	
	uint8_t const * buffer;
	uint64_t        blocksize;
} libmaus_bambam_ScramDecoder;

typedef libmaus_bambam_ScramDecoder *(* libmaus_bambam_ScramDecoder_New_Type)(char const * rfilename, char const * rmode, char const * rreferencefilename);
typedef libmaus_bambam_ScramDecoder *(* libmaus_bambam_ScramDecoder_Delete_Type)(libmaus_bambam_ScramDecoder * object);
typedef int (* libmaus_bambam_ScramDecoder_Decode_Type)(libmaus_bambam_ScramDecoder * object);

libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_New(char const * rfilename, char const * rmode, char const * rreferencefilename);
libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_Delete(libmaus_bambam_ScramDecoder * object);
int libmaus_bambam_ScramDecoder_Decode(libmaus_bambam_ScramDecoder * object);

#if defined(__cplusplus)
}
#endif

#endif
