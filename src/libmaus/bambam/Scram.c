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
#include <io_lib/sam_header.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

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

	#if defined(LIBMAUS_SCRAM_VERBOSE_DECODING)
	scram_set_option(sdecoder, CRAM_OPT_VERBOSITY, 1);
	#endif
	
	if ( object->mode && object->mode[0] == 'r' && object->mode[1] == 'c' )
		scram_set_option(sdecoder, CRAM_OPT_DECODE_MD, 1);

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

#if defined(LIBMAUS_HAVE_IO_LIB_INPUT_CALLBACKS)
libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_New_Cram_Input_Callback(
	char const * rfilename,
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename
)
{
	libmaus_bambam_ScramDecoder * object = 0;
	size_t filenamelen = 0;
	size_t referencefilenamelen = 0;
	SAM_hdr * samheader = 0;
	scram_fd * sdecoder = 0;
	
	object = (libmaus_bambam_ScramDecoder *)malloc(sizeof(libmaus_bambam_ScramDecoder));
	
	if ( ! object )
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	memset(object,0,sizeof(*object));
	
	filenamelen = strlen(rfilename);
	
	if ( ! (object->filename = (char *)malloc(filenamelen+1)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	memcpy(object->filename,rfilename,filenamelen);
	object->filename[filenamelen] = 0;

	if ( rreferencefilename )
	{
		referencefilenamelen = strlen(rreferencefilename);
		if ( ! (object->referencefilename = (char *)malloc(referencefilenamelen+1)) )
			return libmaus_bambam_ScramDecoder_Delete(object);
		memcpy(object->referencefilename,rreferencefilename,referencefilenamelen);
		object->referencefilename[referencefilenamelen] = 0;
	}
	
	if ( !(object->decoder = scram_open_cram_via_callbacks(
		object->filename,
		(cram_io_allocate_read_input_t)callback_allocate_function,
		(cram_io_deallocate_read_input_t)callback_deallocate_function,
		bufsize)) 
	)
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	sdecoder = (scram_fd *)(object->decoder);

	#if defined(LIBMAUS_SCRAM_VERBOSE_DECODING)
	scram_set_option(sdecoder, CRAM_OPT_VERBOSITY, 1);
	#endif

	scram_set_option(sdecoder, CRAM_OPT_DECODE_MD, 1);

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
#endif

libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_New_Range(char const * rfilename, char const * rmode, char const * rreferencefilename, char const * rref, int64_t const start, int64_t const end)
{
	libmaus_bambam_ScramDecoder * object = 0;
	size_t filenamelen = 0;
	size_t modelen = 0;
	size_t referencefilenamelen = 0;
	SAM_hdr * samheader = 0;
	scram_fd * sdecoder = 0;
	int64_t refid = -1;
	char * ref = 0;
	cram_range r;

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

	#if defined(LIBMAUS_SCRAM_VERBOSE_DECODING)
	scram_set_option(sdecoder, CRAM_OPT_VERBOSITY, 1);
	#endif

	if ( object->mode && object->mode[0] == 'r' && object->mode[1] == 'c' )
		scram_set_option(sdecoder, CRAM_OPT_DECODE_MD, 1);

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

	/* range decoding is not supported for non cram formats */
	if ( sdecoder->is_bam )
		return libmaus_bambam_ScramDecoder_Delete(object);

	/* load cram index, returns -1 on failure */
	if ( cram_index_load(sdecoder->c, object->filename) < 0 )
		return libmaus_bambam_ScramDecoder_Delete(object);

	/* check if we have a sequence name */
	if ( ! rref )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	/* duplicate reference id string */
	ref = strdup(rref);

	/* check */
	if ( ! ref )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	/* get numerical id of ref seq */
	refid = sam_hdr_name2ref(sdecoder->c->header, ref);
	
	/* dealloc duplicate */
	free(ref);
	
	/* get whether ref id is valid */
	if ( refid < 0 || refid >= sdecoder->c->header->nref )
		return libmaus_bambam_ScramDecoder_Delete(object);

	// fprintf(stderr,"***%s %u\n",sdecoder->c->header->ref[refid].name,sdecoder->c->header->ref[refid].len);

	/* set up range object */
	r.refid = refid;
	r.start = start;

	if ( end >= 0 )
		r.end = end;
	else
		r.end = INT_MAX;

	/* set range */
	if (scram_set_option(sdecoder, CRAM_OPT_RANGE, &r))
		return libmaus_bambam_ScramDecoder_Delete(object);
				
	return object;
}

#if defined(LIBMAUS_HAVE_IO_LIB_INPUT_CALLBACKS)
libmaus_bambam_ScramDecoder * libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range(
	char const * rfilename,
	scram_cram_io_allocate_read_input_t   callback_allocate_function,
	scram_cram_io_deallocate_read_input_t callback_deallocate_function,
	size_t const bufsize,
	char const * rreferencefilename,
	char const * rref,
	int64_t const start,
	int64_t const end
)
{
	libmaus_bambam_ScramDecoder * object = 0;
	size_t filenamelen = 0;
	size_t referencefilenamelen = 0;
	SAM_hdr * samheader = 0;
	scram_fd * sdecoder = 0;
	int64_t refid = -1;
	char * ref = 0;
	cram_range r;

	object = (libmaus_bambam_ScramDecoder *)malloc(sizeof(libmaus_bambam_ScramDecoder));
	
	if ( ! object )
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	memset(object,0,sizeof(*object));
	
	filenamelen = strlen(rfilename);
	
	if ( ! (object->filename = (char *)malloc(filenamelen+1)) )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	memcpy(object->filename,rfilename,filenamelen);
	object->filename[filenamelen] = 0;

	if ( rreferencefilename )
	{
		referencefilenamelen = strlen(rreferencefilename);
		if ( ! (object->referencefilename = (char *)malloc(referencefilenamelen+1)) )
			return libmaus_bambam_ScramDecoder_Delete(object);
		memcpy(object->referencefilename,rreferencefilename,referencefilenamelen);
		object->referencefilename[referencefilenamelen] = 0;
	}
	
	if ( !(object->decoder = scram_open_cram_via_callbacks(
		object->filename,
		(cram_io_allocate_read_input_t)callback_allocate_function,
		(cram_io_deallocate_read_input_t)callback_deallocate_function,
		bufsize)) 
	)
		return libmaus_bambam_ScramDecoder_Delete(object);
		
	sdecoder = (scram_fd *)(object->decoder);

	#if defined(LIBMAUS_SCRAM_VERBOSE_DECODING)
	scram_set_option(sdecoder, CRAM_OPT_VERBOSITY, 1);
	#endif

	scram_set_option(sdecoder, CRAM_OPT_DECODE_MD, 1);

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

	/* range decoding is not supported for non cram formats */
	if ( sdecoder->is_bam )
		return libmaus_bambam_ScramDecoder_Delete(object);

	/* load cram index, returns -1 on failure */
	if ( cram_index_load(sdecoder->c, object->filename) < 0 )
		return libmaus_bambam_ScramDecoder_Delete(object);

	/* check if we have a sequence name */
	if ( ! rref )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	/* duplicate reference id string */
	ref = strdup(rref);

	/* check */
	if ( ! ref )
		return libmaus_bambam_ScramDecoder_Delete(object);
	
	/* get numerical id of ref seq */
	refid = sam_hdr_name2ref(sdecoder->c->header, ref);
	
	/* dealloc duplicate */
	free(ref);
	
	/* get whether ref id is valid */
	if ( refid < 0 || refid >= sdecoder->c->header->nref )
		return libmaus_bambam_ScramDecoder_Delete(object);

	// fprintf(stderr,"***%s %u\n",sdecoder->c->header->ref[refid].name,sdecoder->c->header->ref[refid].len);

	/* set up range object */
	r.refid = refid;
	r.start = start;

	if ( end >= 0 )
		r.end = end;
	else
		r.end = INT_MAX;

	/* set range */
	if (scram_set_option(sdecoder, CRAM_OPT_RANGE, &r))
		return libmaus_bambam_ScramDecoder_Delete(object);
				
	return object;
}

#endif

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

/**
 * construct a header from a given text
 *
 * @param headertext plain text header
 * @return scram header object
 **/
libmaus_bambam_ScramHeader * libmaus_bambam_ScramHeader_New(char const * headertext)
{
	libmaus_bambam_ScramHeader * header = 0;
	size_t const headerlen = headertext ? strlen(headertext) : 0;
	
	header = (libmaus_bambam_ScramHeader *)malloc(sizeof(libmaus_bambam_ScramHeader));
	
	if ( ! header )
		return libmaus_bambam_ScramHeader_Delete(header);
		
	memset(header,0,sizeof(libmaus_bambam_ScramHeader));
	
	header->text = (char*)malloc(headerlen+1);
	
	if ( ! header->text )
		return libmaus_bambam_ScramHeader_Delete(header);
		
	memset(header->text,0,headerlen+1);
	memcpy(header->text,headertext,headerlen);
	
	header->header = sam_hdr_parse(header->text,headerlen);

	if ( ! header->header )
		return libmaus_bambam_ScramHeader_Delete(header);
		
	return header;		
}

/**
 * deallocate scram header object
 *
 * @param header object
 * @return null
 **/
libmaus_bambam_ScramHeader * libmaus_bambam_ScramHeader_Delete(libmaus_bambam_ScramHeader * header)
{
	if ( header && header->header )
	{
		sam_hdr_free((SAM_hdr *)header->header);
		header->header = 0;
	}
	if ( header && header->text )
	{
		free(header->text);
		header->text = 0;
	}
	if ( header )
	{
		free(header);
	}
	
	return 0;
}

/**
 * construct an encoder
 *
 * @param header scram header object
 * @param rfilename output file name (- for standard output)
 * @param rmode file mode
 * @param rreferencefilename name of reference for cram output
 **/
libmaus_bambam_ScramEncoder * libmaus_bambam_ScramEncoder_New(
	char const * headertext,
	char const * rfilename, char const * rmode, char const * rreferencefilename,
	int const rverbose
)
{
	libmaus_bambam_ScramEncoder * encoder = 0;
	scram_fd * sencoder = 0;
	
	encoder = (libmaus_bambam_ScramEncoder *)malloc(sizeof(libmaus_bambam_ScramEncoder));
	
	if ( !encoder )
		return libmaus_bambam_ScramEncoder_Delete(encoder);
		
	memset(encoder,0,sizeof(libmaus_bambam_ScramEncoder));
		
	encoder->filename = (char *)malloc(strlen(rfilename)+1);
	
	if ( ! encoder->filename )
		return libmaus_bambam_ScramEncoder_Delete(encoder);
	
	memset(encoder->filename,0,strlen(rfilename)+1);
	memcpy(encoder->filename,rfilename,strlen(rfilename));

	encoder->mode = (char *)malloc(strlen(rmode)+1);

	if ( ! encoder->mode )
		return libmaus_bambam_ScramEncoder_Delete(encoder);
	
	memset(encoder->mode,0,strlen(rmode)+1);
	memcpy(encoder->mode,rmode,strlen(rmode));
	
	if ( rreferencefilename )
	{
		encoder->referencefilename = (char *)malloc(strlen(rreferencefilename)+1);
	
		if ( ! encoder->referencefilename )
			return libmaus_bambam_ScramEncoder_Delete(encoder);
	
		memset(encoder->referencefilename,0,strlen(rreferencefilename)+1);
		memcpy(encoder->referencefilename,rreferencefilename,strlen(rreferencefilename));
	}
	
	encoder->header = libmaus_bambam_ScramHeader_New(headertext);
	
	if ( ! encoder->header )
		return libmaus_bambam_ScramEncoder_Delete(encoder);

	encoder->encoder = scram_open(encoder->filename,encoder->mode);

	if ( ! encoder->encoder )
		return libmaus_bambam_ScramEncoder_Delete(encoder);

	sencoder = (scram_fd *)(encoder->encoder);

	scram_set_option(sencoder, CRAM_OPT_VERBOSITY, rverbose);
	scram_set_header(sencoder, (SAM_hdr *)encoder->header->header);

	if ( encoder->referencefilename) 
		scram_set_option(sencoder, CRAM_OPT_REFERENCE, encoder->referencefilename);
	else
		scram_set_option(sencoder, CRAM_OPT_REFERENCE, 0);

	if ( scram_write_header(sencoder) )
		return libmaus_bambam_ScramEncoder_Delete(encoder);
		
	return encoder;
}

/**
 * deallocate scram encoder object
 *
 * @param object encoder object
 * @return null
 **/
libmaus_bambam_ScramEncoder * libmaus_bambam_ScramEncoder_Delete(libmaus_bambam_ScramEncoder * object)
{
	if ( object && object->filename )
	{
		free(object->filename);
		object->filename = 0;
	}
	if ( object && object->mode )
	{
		free(object->mode);
		object->mode = 0;
	}
	if ( object && object->referencefilename )
	{
		free(object->referencefilename);
		object->referencefilename = 0;
	}
	if ( object && object->header )
	{
		libmaus_bambam_ScramHeader_Delete(object->header);
		object->header = 0;
	}
	if ( object && object->encoder )
	{
		scram_fd * encoder = (scram_fd *)(object->encoder);
		scram_close(encoder);
		object->encoder = 0;
	
	}
	if ( object && object->buffer )
	{
		free(object->buffer);
		object->buffer = 0;
		object->buffersize = 0;
	}
	if ( object )
	{
		free(object);
		object = 0;
	}
	
	return 0;
}

/**
 * encode sequence
 *
 * @param seq sequence (BAM block)
 * @param len length of BAM block in bytes
 * @return -1 on failure
 **/
int libmaus_bambam_ScramEncoder_Encode(libmaus_bambam_ScramEncoder * encoder, uint8_t const * seq, uint64_t const len)
{
	uint64_t const metasize = 2*sizeof(uint32_t);
	uint64_t const pad = 1;

	if ( len+metasize+pad > encoder->buffersize )
	{
		uint64_t const newbufsize = len+metasize+pad;
	
		free(encoder->buffer);
		encoder->buffer = 0;
		encoder->buffersize = 0;
		
		encoder->buffer = (char *)malloc(newbufsize);
		
		if ( ! encoder->buffer )
			return -1;
		
		encoder->buffersize = newbufsize;
	}
	assert ( len+metasize+pad <= encoder->buffersize );
	
	((uint32_t *)(encoder->buffer))[0] = len+metasize+pad;
	((uint32_t *)(encoder->buffer))[1] = len;
	/* copy block data */
	memcpy(encoder->buffer + metasize , seq , len);
	/* terminating null byte for io_lib code */
	encoder->buffer[metasize + len] = 0;

	return scram_put_seq( (scram_fd *)(encoder->encoder), (bam_seq_t *)(encoder->buffer) );	
}
#endif /* defined(LIBMAUS_HAVE_IO_LIB)*/
