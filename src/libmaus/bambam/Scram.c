/*
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
*/

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_IO_LIB)
#include <libmaus/bambam/Scram.h>
#include <io_lib/scram.h>
#include <string.h>
#include <unistd.h>

libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_New(char const * rfilename, char const * rmode, char const * rreferencefilename)
{
	libmaus_bambam_ScramDecoder * object = 0;
	size_t filenamelen = 0;
	size_t modelen = 0;
	size_t referencefilenamelen = 0;
	SAM_hdr * samheader = 0;
	scram_fd * sdecoder = 0;
	
	object = (libmaus_bambam_ScramDecoder *)malloc(sizeof(libmaus_bambam_ScramDecoder));
	
	if ( ! object )
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	memset(object,0,sizeof(*object));
	
	modelen = strlen(rmode);
	filenamelen = strlen(rfilename);
	
	if ( ! (object->filename = (char *)malloc(filenamelen+1)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
	if ( ! (object->mode = (char *)malloc(modelen+1)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	memcpy(object->filename,rfilename,filenamelen);
	object->filename[filenamelen] = 0;
	memcpy(object->mode,rmode,modelen);
	object->mode[modelen] = 0;

	if ( rreferencefilename )
	{
		referencefilenamelen = strlen(rreferencefilename);
		if ( ! (object->referencefilename = (char *)malloc(referencefilenamelen+1)) )
			return libmaus_bambam_ScramDecoder_Delete(object);
		memcpy(object->referencefilename,rreferencefilename,referencefilenamelen);
		object->referencefilename[referencefilenamelen] = 0;
	}
	
	if ( !(object->decoder = scram_open(object->filename,object->mode)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	sdecoder = (scram_fd *)(object->decoder);

	scram_set_option(sdecoder, CRAM_OPT_VERBOSITY, 1);

	if ( !(sdecoder->is_bam) && object->referencefilename ) 
	{
		cram_load_reference(sdecoder->c, object->referencefilename);
		if ( !sdecoder->c->refs ) 
			return libmaus_bambam_ScramDecoder_Delete(object);
	}

	samheader = scram_get_header(sdecoder);
	
	if ( ! samheader )
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	if ( ! (object->header = (char *)malloc(samheader->text->length)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	memcpy ( object->header,samheader->text->str,samheader->text->length );
	object->headerlen = samheader->text->length;
		
	return object;
}
libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_Delete(libmaus_bambam_ScramDecoder * object)
{
	if ( object )
	{
		if ( object->filename )
		{
			free(object->filename);
			object->filename = 0;
		}
		if ( object->header )
		{
			free(object->header);
			object->header = 0;
		}
		if ( object->mode )
		{
			free(object->mode);
			object->mode = 0;
		}
		if ( object->decoder )
		{
			scram_fd * decoder = (scram_fd *)(object->decoder);
			scram_close(decoder);
			object->decoder = 0;
		}
		if ( object->vseq )
		{
			free(object->vseq);
			object->vseq = 0;
		}
		
		free(object);
	}

	return 0;
}

int libmaus_bambam_ScramDecoder_Decode(libmaus_bambam_ScramDecoder * object)
{
	scram_fd * sdecoder = 0;
	bam_seq_t * seq = 0;
	int r = -1;
	
	if ( ! object )
		return -1;

	sdecoder = (scram_fd *)object->decoder;
	
	r = scram_get_seq(sdecoder,(bam_seq_t**)(&(object->vseq)));
	
	if ( r < 0 || ! (object->vseq) )
	{
		/* fprintf(stderr, "got code r=%d, eof is %d\n", r, scram_eof(sdecoder)); */
	
		if ( !scram_eof(sdecoder) )
			return -2;
		else
			return -1;
	}

	seq = (bam_seq_t *)(object->vseq);
	object->blocksize = seq->blk_size;
	object->buffer = ((uint8_t const *)seq) + 2*sizeof(uint32_t);
	
	return 0;
}
#endif /* defined(LIBMAUS_HAVE_IO_LIB)*/
