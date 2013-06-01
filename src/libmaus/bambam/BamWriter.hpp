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
#include <libmaus/lz/Deflate.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * BAM file writing class
		 **/
		struct BamWriter : public ::libmaus::bambam::BamAlignmentEncoderBase
		{
			//! this type
			typedef BamWriter this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			//! output stream pointer
			::libmaus::util::unique_ptr<std::ofstream>::type Postr;
			//! output stream reference
			std::ostream & ostr;
			//! compressor object
			::libmaus::lz::BgzfDeflate<std::ostream> bgzfos;
			//! encoding table
			::libmaus::bambam::BamSeqEncodeTable seqtab;
			//! encoding buffer
			::libmaus::fastx::UCharBuffer ubuffer;
			//! BAM header
			::libmaus::bambam::BamHeader header;
			
			public:
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
			: Postr(), ostr(rostr), bgzfos(ostr,level), header(rheader)
			{
				header.produceHeader();
				header.serialise(bgzfos);
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
			: Postr(new std::ofstream(filename.c_str(),std::ios::binary)), ostr(*Postr), bgzfos(ostr,level), header(rheader)
			{
				header.produceHeader();
				header.serialise(bgzfos);	
			}

			/**
			 * destructor, writes EOF block and flushes stream
			 **/
			~BamWriter()
			{
				// bgzfos.flush();
				bgzfos.addEOFBlock();
				ostr.flush();
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
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxString<value_type>(ubuffer,tag,value);
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
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber<value_type>(ubuffer,tag,type,value);
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
				::libmaus::bambam::BamAlignmentEncoderBase::writeToStream(ubuffer,bgzfos);
			}

			/**
			 * @return compressor stream
			 **/
			::libmaus::lz::BgzfDeflate<std::ostream> & getStream()
			{
				return bgzfos;
			}
		};
	}
}
#endif
