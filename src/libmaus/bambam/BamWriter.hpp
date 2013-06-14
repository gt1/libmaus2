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
#if ! defined(LIBMAUS_BAMBAM_BAMWRITER_HPP)
#define LIBMAUS_BAMBAM_BAMWRITER_HPP

#include <libmaus/bambam/BamAlignmentEncoderBase.hpp>
#include <libmaus/bambam/BamHeader.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/lz/BgzfDeflateParallel.hpp>
#include <libmaus/lz/BgzfInflateDeflateParallel.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * stream base for serial BAM file writing
		 **/
		struct BamWriterSerialStreamBase
		{
			public:
			//! stream type
			typedef ::libmaus::lz::BgzfDeflate<std::ostream> stream_type;
			//! this type
			typedef BamWriterSerialStreamBase this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			//! output stream pointer
			::libmaus::util::unique_ptr<std::ofstream>::type Postr;
			//! output stream reference
			std::ostream & ostr;
			//! compressor object
			::libmaus::lz::BgzfDeflate<std::ostream> bgzfos;
		
			public:
			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterSerialStreamBase(std::ostream & rostr, int const level = Z_DEFAULT_COMPRESSION) : Postr(), ostr(rostr), bgzfos(ostr,level) {}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterSerialStreamBase(std::string const & filename, int const level = Z_DEFAULT_COMPRESSION) : Postr(new std::ofstream(filename.c_str(),std::ios::binary)), ostr(*Postr), bgzfos(ostr,level) {}
			
			/**
			 * destructor
			 **/
			~BamWriterSerialStreamBase()
			{
				bgzfos.addEOFBlock();
				ostr.flush();
			}
			
			/**
			 * @return output stream
			 **/
			stream_type & getStream()
			{
				return bgzfos;
			}
		};
		
		struct BamWriterSerialStreamBaseWrapper
		{
			//! wrapped object
			BamWriterSerialStreamBase bwssb;

			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterSerialStreamBaseWrapper(std::ostream & rostr, int const level = Z_DEFAULT_COMPRESSION) : bwssb(rostr,level) {}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterSerialStreamBaseWrapper(std::string const & filename, int const level = Z_DEFAULT_COMPRESSION) : bwssb(filename,level) {}
		};

		/**
		 * stream base for parallel BAM file writing
		 **/
		struct BamWriterParallelStreamBase
		{
			public:
			//! stream type
			typedef ::libmaus::lz::BgzfDeflateParallel stream_type;
			//! this type
			typedef BamWriterParallelStreamBase this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			//! output stream pointer
			::libmaus::util::unique_ptr<std::ofstream>::type Postr;
			//! output stream reference
			std::ostream & ostr;
			//! compressor object
			::libmaus::lz::BgzfDeflateParallel bgzfos;
		
			public:
			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterParallelStreamBase(std::ostream & rostr, uint64_t const numthreads, int const level = Z_DEFAULT_COMPRESSION) 
			: Postr(), ostr(rostr), bgzfos(ostr,numthreads,4*numthreads,level) {}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterParallelStreamBase(std::string const & filename, uint64_t const numthreads, int const level = Z_DEFAULT_COMPRESSION) 
			: Postr(new std::ofstream(filename.c_str(),std::ios::binary)), ostr(*Postr), bgzfos(ostr,numthreads,4*numthreads,level) {}
			
			/**
			 * destructor
			 **/
			~BamWriterParallelStreamBase()
			{
				bgzfos.flush();
				ostr.flush();
			}
			
			/**
			 * @return output stream
			 **/
			stream_type & getStream()
			{
				return bgzfos;
			}
		};

		struct BamWriterParallelStreamBaseWrapper
		{
			//! wrapped stream
			BamWriterParallelStreamBase bwpsb;
			
			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterParallelStreamBaseWrapper(std::ostream & rostr, uint64_t const numthreads, int const level = Z_DEFAULT_COMPRESSION) 
			: bwpsb(rostr,numthreads,level) {}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriterParallelStreamBaseWrapper(std::string const & filename, uint64_t const numthreads, int const level = Z_DEFAULT_COMPRESSION) 
			: bwpsb(filename,numthreads,level) {}
			
		};
	
		/**
		 * BAM file writing class template
		 **/
		template<typename _base_type>
		struct BamWriterTemplate : public ::libmaus::bambam::BamAlignmentEncoderBase
		{
			//! base type
			typedef _base_type base_type;
			//! this type
			typedef BamWriterTemplate<base_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			//! stream pointer
			typename base_type::unique_ptr_type Pstream;
			//! stream
			base_type & stream;
			//! encoding table
			::libmaus::bambam::BamSeqEncodeTable seqtab;
			//! encoding buffer
			::libmaus::fastx::UCharBuffer ubuffer;
			//! BAM header pointer
			::libmaus::bambam::BamHeader::unique_ptr_type pheader;
			//! BAM header
			::libmaus::bambam::BamHeader & header;
			
			public:
			/**
			 * constructor from bgzf encoder object
			 *
			 * @param rstream bgzf encoder object
			 * @param rheader BAM header
			 **/
			BamWriterTemplate(base_type & rstream, ::libmaus::bambam::BamHeader const & rheader)
			: Pstream(), stream(rstream), pheader(UNIQUE_PTR_MOVE(rheader.uclone())), header(*pheader)
			{
				header.produceHeader();
				header.serialise(getStream());			
			}

			/**
			 * destructor, writes EOF block and flushes stream
			 **/
			~BamWriterTemplate()
			{
			}

			/**
			 * encode a complete alignment block
			 *
			 * @param name iterator for name
			 * @param namelen length of query name
			 * @param refid reference id
			 * @param pos position
			 * @param mapq mapping quality
			 * @param flags alignment flags
			 * @param cigar encoded cigar array
			 * @param cigarlen number of cigar operations
			 * @param nextrefid reference id of next/mate
			 * @param nextpos position of next/matex
			 * @param tlen template length
			 * @param seq sequence
			 * @param seqlen length of query sequence
			 * @param qual quality string
			 * @param qualoffset quality offset (default 33)
			 **/	
			template<
				typename name_iterator,
				typename cigar_iterator,
				typename seq_iterator,
				typename qual_iterator
			>
			void encodeAlignment(
				name_iterator name,
				uint32_t const namelen,
				int32_t const refid,
				int32_t const pos,
				uint32_t const mapq,
				uint32_t const flags,
				cigar_iterator cigar,
				uint32_t const cigarlen,
				int32_t const nextrefid,
				int32_t const nextpos,
				uint32_t const tlen,
				seq_iterator seq,
				uint32_t const seqlen,
				qual_iterator qual,
				uint8_t const qualoffset = 33
			)
			{
				::libmaus::bambam::BamAlignmentEncoderBase::encodeAlignment(ubuffer,seq,
					name,namelen,refid,pos,mapq,flags,cigar,cigarlen,nextrefid,nextpos,
					tlen,seq,seqlen,qual,qualoffset);
			}

			/**
			 * encode a complete alignment block
			 *
			 * @param name string containing query name
			 * @param refid reference id
			 * @param pos position
			 * @param mapq mapping quality
			 * @param flags alignment flags
			 * @param cigar string containing the plain text cigar string
			 * @param nextrefid reference id of next/mate
			 * @param nextpos position of next/matex
			 * @param tlen template length
			 * @param seq string containing the query string
			 * @param qual string containing the quality string
			 * @param qualoffset quality offset (default 33)
			 **/	
			void encodeAlignment(
				std::string const & name,
				int32_t const refid,
				int32_t const pos,
				uint32_t const mapq,
				uint32_t const flags,
				std::string const & cigar,
				int32_t const nextrefid,
				int32_t const nextpos,
				uint32_t const tlen,
				std::string const & seq,
				std::string const & qual,
				uint8_t const qualoffset = 33
			)
			{
				::libmaus::bambam::BamAlignmentEncoderBase::encodeAlignment(ubuffer,seqtab,
					name,refid,pos,mapq,flags,cigar,nextrefid,nextpos,tlen,seq,qual,qualoffset);
			}

			/**
			 * put auxiliary tag with string content
			 *
			 * @param tag two character aux id
			 * @param value field content
			 **/
			template<typename value_type>
			void putAuxString(std::string const & tag, value_type const & value)
			{
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxString< ::libmaus::fastx::UCharBuffer,value_type>(ubuffer,tag,value);
			}

			/**
			 * put auxiliary tag with number content
			 *
			 * @param tag two character aux id
			 * @param type field type
			 * @param value field content
			 **/
			template<typename value_type>
			void putAuxNumber(
				std::string const & tag,
				char const type, 
				value_type const & value
			)
			{
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber< ::libmaus::fastx::UCharBuffer, value_type>(ubuffer,tag,type,value);
			}

			/**
			 * put auxiliary tag with number array content
			 *
			 * @param tag two character aux id
			 * @param type field type
			 * @param values field content vector
			 **/
			template<typename value_type>
			void putAuxNumberArray(
				std::string const & tag, 
				char const type, 
				std::vector<value_type> const & values
			)
			{
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumberArray<value_type>(ubuffer,tag,type,values);
			}
			
			/**
			 * write encoded alignment block to stream
			 **/
			void commit()
			{
				::libmaus::bambam::BamAlignmentEncoderBase::writeToStream(ubuffer,getStream());
			}

			/**
			 * @return compressor stream
			 **/
			typename base_type::stream_type & getStream()
			{
				return stream.getStream();
			}
		};
		
		struct BamWriter : public BamWriterSerialStreamBaseWrapper, public BamWriterTemplate<BamWriterSerialStreamBase>
		{
			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriter(
				std::ostream & rostr, 
				::libmaus::bambam::BamHeader const & rheader, 
				int const level = Z_DEFAULT_COMPRESSION
			)
			: BamWriterSerialStreamBaseWrapper(rostr,level), BamWriterTemplate<BamWriterSerialStreamBase>(BamWriterSerialStreamBaseWrapper::bwssb,rheader)
			{
			}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamWriter(
				std::string const & filename, 
				::libmaus::bambam::BamHeader const & rheader, 
				int const level = Z_DEFAULT_COMPRESSION
			)
			: BamWriterSerialStreamBaseWrapper(filename,level), BamWriterTemplate<BamWriterSerialStreamBase>(BamWriterSerialStreamBaseWrapper::bwssb,rheader)
			{
			}
		};

		struct BamParallelWriter : public BamWriterParallelStreamBaseWrapper, public BamWriterTemplate<BamWriterParallelStreamBase>
		{
			/**
			 * constructor for stream
			 *
			 * @param rostr output stream
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamParallelWriter(
				std::ostream & rostr, 
				uint64_t const numthreads,
				::libmaus::bambam::BamHeader const & rheader, 
				int const level = Z_DEFAULT_COMPRESSION
			)
			: BamWriterParallelStreamBaseWrapper(rostr,numthreads,level), BamWriterTemplate<BamWriterParallelStreamBase>(BamWriterParallelStreamBaseWrapper::bwpsb,rheader)
			{
			}
			/**
			 * constructor for file
			 *
			 * @param filename output file name
			 * @param rheader BAM header
			 * @param level zlib compression level
			 **/
			BamParallelWriter(
				std::string const & filename, 
				uint64_t const numthreads,
				::libmaus::bambam::BamHeader const & rheader, 
				int const level = Z_DEFAULT_COMPRESSION
			)
			: BamWriterParallelStreamBaseWrapper(filename,numthreads,level), BamWriterTemplate<BamWriterParallelStreamBase>(BamWriterParallelStreamBaseWrapper::bwpsb,rheader)
			{
			}
		};
	}
}
#endif
