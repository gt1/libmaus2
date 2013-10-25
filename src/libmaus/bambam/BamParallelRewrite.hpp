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

#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELREWRITE_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELREWRITE_HPP

#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamWriter.hpp>

namespace libmaus
{
	namespace bambam
	{
		//! bam header rewriting interface
		struct BamHeaderRewriteCallback
		{
			//! destructor
			virtual ~BamHeaderRewriteCallback() {}
			
			/**
			 * rewrite callback
			 *
			 * @param header BAM header
			 * @return rewritten/modified BAM header
			 **/
			virtual libmaus::bambam::BamHeader::unique_ptr_type operator()(libmaus::bambam::BamHeader const & header) const = 0;
		};
	
		//! class for parallel rewriting of a BAM file
		struct BamParallelRewrite
		{
			//! decoder type
			typedef libmaus::bambam::BamParallelRewriteDecoder decoder_type;
			//! writer type
			typedef libmaus::bambam::BamParallelRewriteWriter writer_type;

			//! parallel input stream
			libmaus::lz::BgzfInflateDeflateParallelInputStream stream;
			//! decoder
			decoder_type dec;
			//! rewritten header
			libmaus::bambam::BamHeader::unique_ptr_type rewrittenheader;
			//! writer
			writer_type writer;

			/**
			 * @return default blocks per thread
			 **/
			static uint64_t getDefaultBlocksPerThread() { return 4; }

			/**
			 * constructor
			 *
			 * @param in input stream
			 * @param out output stream
			 * @param level zlib compression level for output stream
			 * @param numthreads number of threads used for decompression and compression
			 * @param blocksperthread number of memory (bgzf) blocks per thread
			 **/
			BamParallelRewrite(
				std::istream & in,
				std::ostream & out,
				int const level, // Z_DEFAULT_COMPRESSION
				uint64_t const numthreads,
				uint64_t const blocksperthread = getDefaultBlocksPerThread(),
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * rblockoutputcallbacks = 0
			)
			: stream(in,out,level,numthreads,blocksperthread), dec(stream), writer(stream.bgzf,dec.getHeader(),rblockoutputcallbacks) {}

			/**
			 * constructor
			 *
			 * @param in input stream
			 * @param header output BAM header
			 * @param out output stream
			 * @param level zlib compression level for output stream
			 * @param numthreads number of threads used for decompression and compression
			 * @param blocksperthread number of memory (bgzf) blocks per thread
			 **/
			BamParallelRewrite(
				std::istream & in,
				libmaus::bambam::BamHeader const & header,
				std::ostream & out,
				int const level, // Z_DEFAULT_COMPRESSION
				uint64_t const numthreads,
				uint64_t const blocksperthread = getDefaultBlocksPerThread(),
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * rblockoutputcallbacks = 0
			)
			: stream(in,out,level,numthreads,blocksperthread), dec(stream), writer(stream.bgzf,header,rblockoutputcallbacks) {}

			/**
			 * constructor
			 *
			 * @param in input stream
			 * @param rewritecallback BAM header rewriting callback
			 * @param out output stream
			 * @param level zlib compression level for output stream
			 * @param numthreads number of threads used for decompression and compression
			 * @param blocksperthread number of memory (bgzf) blocks per thread
			 **/
			BamParallelRewrite(
				std::istream & in,
				BamHeaderRewriteCallback const & rewritecallback,
				std::ostream & out,
				int const level, // Z_DEFAULT_COMPRESSION
				uint64_t const numthreads,
				uint64_t const blocksperthread = getDefaultBlocksPerThread(),
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * rblockoutputcallbacks = 0
			)
			: stream(in,out,level,numthreads,blocksperthread), dec(stream), rewrittenheader(rewritecallback(dec.getHeader())), writer(stream.bgzf,*rewrittenheader,rblockoutputcallbacks) {}
			
			/**
			 * @return decoder
			 */
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return dec;
			}
			
			/**
			 * read next alignment
			 *
			 * @return true iff another alignment was available
			 */
			bool readAlignment()
			{
				return dec.readAlignment();
			}
			
			/**
			 * get most recently read alignment. only valid if most recent call of
			 * readAlignment returned true and readAlignment was called for this object
			 * at least once
			 *
			 * @return alignment
			 */
			libmaus::bambam::BamAlignment & getAlignment()
			{
				return dec.getAlignment();
			}

			/**
			 * get most recently read alignment (unmodifiable). only valid if most recent call of
			 * readAlignment returned true and readAlignment was called for this object
			 * at least once
			 *
			 * @return alignment
			 */
			libmaus::bambam::BamAlignment const & getAlignment() const
			{
				return dec.getAlignment();
			}
			
			/**
			 * @return BAM writer object
			 **/
			writer_type & getWriter()
			{
				return writer;
			}	
		};
	}
}
#endif
