/*
    libmaus
    Copyright (C) 2015 German Tischler
    Copyright (C) 2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_IO_LIB_CALLBACK_PROTOTYPES_H)
#define LIBMAUS_BAMBAM_IO_LIB_CALLBACK_PROTOTYPES_H

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _cram_data_write_block_type
{
	cram_data_write_block_type_internal,
	cram_data_write_block_type_block_final,
	cram_data_write_block_type_file_final
} cram_data_write_block_type;

// Enqueue a package of work to compress a CRAM slice.
typedef void
(*cram_enque_compression_work_package_function_t)(void *userdata, void *workpackage);

// Callback to indicate the block has been compressed
typedef void
(*cram_compression_work_package_finished_t)(void *userdata, size_t const inblockid, int const final);

// Write function for compressed blocks, provided by libmaus.
// Inblockid is the same as supplied by the dispatcher.
// Outblockid increments from 0 per unique inblockid.
typedef void
(*cram_data_write_function_t)(void *userdata,
			      ssize_t const inblockid,
			      size_t const outblockid,
			      char const *data,
			      size_t const n,
			      cram_data_write_block_type const blocktype
);

/**
 * Allocate cram encoder and return compression context
 *
 * @param samheader SAM header text
 * @param samheaderlength length of SAM header in bytes
 * @param workenqueuefunction function which will be called to enque
 *        compression work packages
 *
 * @return NULL on failure.
 **/
extern void *cram_allocate_encoder(void *userdata,
			    char const *sam_header,
			    size_t const sam_headerlength,
			    cram_data_write_function_t write_func);

/**
 * dellocate cram encoder
 *
 * @param context data structure returned by cram_allocate_encoder
 **/
extern void cram_deallocate_encoder(void *context);

/**
 * Notify cram encoder there is more data to be compressed
 *
 * @param userdata pointer supplied back to callback functions
 * @param context compression context returned by
 *        cram_allocate_encoder
 * @param inblockid running id of input block
 * @param block of alignments (uncompressed bam format)
 * @param blocksize length of block in sizeof(char)
 * @param final 1 if this is the last block passed, 0 otherwise
 * @param workenqueuefunction callback for queueing work in the thread
 *        pool
 * @param workfinishedfunction callback notifying caller that
 *        compression of this block is done
 * @param writefunction function called from writing compressed data
 *
 * @return 0 on success;
 *        -1 on failure
 **/
extern int cram_enque_compression_block(
	void *userdata,
	void *context,
	size_t const inblockid,
	char const **block,
	size_t const *blocksize,
	size_t const numblocks,
	int const final,
	cram_enque_compression_work_package_function_t workenqueuefunction,
	cram_data_write_function_t writefunction,
	cram_compression_work_package_finished_t workfinishedfunction);

/**
 * Work package dispatch function for cram encoding
 *
 * @param Workpackage containing task to perform and all function
 *        pointers necessary to communicate back to dispatcher.
 *
 * @return 0 on success;
 *        -1 on failure
 **/
extern int cram_process_work_package(void *workpackage);

#if defined(__cplusplus)
}
#endif

#endif
