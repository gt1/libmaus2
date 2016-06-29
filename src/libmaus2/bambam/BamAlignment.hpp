/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENT_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENT_HPP

#include <libmaus2/bambam/BamAlignmentFixedSizeData.hpp>
#include <libmaus2/bambam/AlignmentValidity.hpp>
#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus2/bambam/BamAlignmentEncoderBase.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/fastx/FASTQEntry.hpp>
#include <libmaus2/hashing/hash.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/math/IntegerInterval.hpp>
#include <libmaus2/bambam/CigarStringReader.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * bam alignment
		 **/
		struct BamAlignment
		{
			//! this type
			typedef BamAlignment this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			//! memory allocation type of data block
			static const ::libmaus2::autoarray::alloc_type D_array_alloc_type = ::libmaus2::autoarray::alloc_type_memalign_pagesize;
			//! D array type
			typedef ::libmaus2::autoarray::AutoArray<uint8_t,D_array_alloc_type> D_array_type;

			//! data array
			D_array_type D;
			//! number of bytes valid in D
			uint64_t blocksize;

			size_t byteSize() const
			{
				return
					D.byteSize() + sizeof(uint64_t);
			}

			/**
			 * swap this alignment block with another object
			 *
			 * @param other block to swap with
			 **/
			void swap(BamAlignment & other)
			{
				std::swap(D,other.D);
				std::swap(blocksize,other.blocksize);
			}

			/**
			 * copy data from other alignment into this object
			 *
			 * @param from alignment block to be copied
			 **/
			void copyFrom(BamAlignment const & from)
			{
				if ( from.blocksize > D.size() )
				{
					blocksize = 0;
					D = D_array_type(from.blocksize,false);
				}

				// set new block size
				blocksize = from.blocksize;
				// copy data
				std::copy(from.D.begin(),from.D.begin()+from.blocksize,D.begin());
			}

			/**
			 * copy data from other alignment into this object
			 *
			 * @param fromD alignment block data to be copied
			 * @param fromblocksize length of fromD in bytes
			 **/
			void copyFrom(uint8_t const * fromD, uint64_t const fromblocksize)
			{
				if ( fromblocksize > D.size() )
				{
					blocksize = 0;
					D = D_array_type(fromblocksize,false);
				}

				// set new block size
				blocksize = fromblocksize;
				// copy data
				std::copy(fromD,fromD+fromblocksize,D.begin());
			}

			/**
			 * constructor for invalid/empty alignment
			 **/
			BamAlignment()
			: blocksize(0)
			{

			}

			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 **/
			template<typename input_type>
			BamAlignment(input_type & in)
			{
				blocksize = ::libmaus2::bambam::DecoderBase::getLEInteger(in,4);
				// std::cerr << "Block size " << blocksize << std::endl;
				D = D_array_type(blocksize,false);
				in.read(reinterpret_cast<char *>(D.begin()),blocksize);

				if ( static_cast<int64_t>(in.gcount()) != static_cast<int64_t>(blocksize) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to read block of size " << blocksize << std::endl;
					se.finish();
					throw se;
				}

				checkAlignment();
			}

			/**
			 * check alignment for syntactical validity, throws exception if invalid
			 **/
			void checkAlignment() const
			{
				libmaus2_bambam_alignment_validity const validity = valid();
				if ( validity != libmaus2_bambam_alignment_validity_ok )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Invalid alignment: " << validity << std::endl;
					se.finish();
					throw se;
				}
			}

			/**
			 * check alignment for syntactical validity, throws exception if invalid
			 *
			 * @param header SAM/BAM header
			 **/
			void checkAlignment(libmaus2::bambam::BamHeader const & header) const
			{
				libmaus2_bambam_alignment_validity const validity = valid(header);
				if ( validity != libmaus2_bambam_alignment_validity_ok )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Invalid alignment: " << validity << std::endl;
					se.finish();
					throw se;
				}
			}

			/**
			 * check alignment validity excluding reference sequence ids
			 *
			 * @return alignment validty code
			 **/
			libmaus2_bambam_alignment_validity valid() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::valid(D.begin(),blocksize);
			}

			/**
			 * check alignment validity including reference sequence ids
			 *
			 * @param header bam header
			 * @return alignment validty code
			 **/
			template<typename header_type>
			libmaus2_bambam_alignment_validity valid(header_type const & header) const
			{
				libmaus2_bambam_alignment_validity validity = valid();

				if ( validity != libmaus2_bambam_alignment_validity_ok )
					return validity;

				int32_t const refseq = getRefID();
				if ( !((refseq == -1) || refseq < static_cast<int64_t>(header.getNumRef())) )
					return libmaus2_bambam_alignment_validity_invalid_refseq;

				int32_t const nextrefseq = getNextRefID();
				if ( !((nextrefseq == -1) || nextrefseq < static_cast<int64_t>(header.getNumRef())) )
					return libmaus2_bambam_alignment_validity_invalid_next_refseq;

				return libmaus2_bambam_alignment_validity_ok;
			}

			/**
			 * @return number of bytes before the cigar string in the alignment
			 **/
			uint64_t getNumPreCigarBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumPreCigarBytes(D.begin());
			}

			/**
			 * @return number of bytes in the cigar string representation of the alignment
			 **/
			uint64_t getNumCigarBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumCigarBytes(D.begin());
			}

			/**
			 * @return number of bytes after the cigar string in the alignment representation
			 **/
			uint64_t getNumPostCigarBytes() const
			{
				return blocksize - (getNumPreCigarBytes() + getNumCigarBytes());
			}

			/**
			 * @return number of bytes before the name
			 **/
			uint64_t getNumPreNameBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumPreNameBytes(D.begin());
			}

			/**
			 * @return number of bytes in the name
			 **/
			uint64_t getNumNameBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumNameBytes(D.begin());
			}

			/**
			 * @return number of bytes after the name
			 **/
			uint64_t getNumPostNameBytes() const
			{
				return blocksize - (getNumPreNameBytes() + getNumNameBytes());
			}

			/**
			 * git cigar operations vector
			 *
			 * @param D alignment block
			 * @param A array for storing vector
			 * @return number of cigar operations
			 **/
			uint32_t getCigarOperations(libmaus2::autoarray::AutoArray<cigar_operation> & cigop) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarOperations(D.begin(),cigop);
			}

			void getMappingPositionPairs(std::vector<std::pair<int64_t,int64_t> > & V) const
			{
				if ( isUnmap() )
					return;

				libmaus2::autoarray::AutoArray<cigar_operation> Acigop, Bcigop;
				uint64_t const numciga = getCigarOperations(Acigop);
				libmaus2::bambam::CigarStringReader<cigar_operation const *> RA(Acigop.begin(),Acigop.begin()+numciga);
				::libmaus2::bambam::BamFlagBase::bam_cigar_ops opA;

				int64_t readposa = 0;
				while (
					RA.peekNext(opA) &&
					opA != BamFlagBase::LIBMAUS2_BAMBAM_CMATCH &&
					opA != BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL &&
					opA != BamFlagBase::LIBMAUS2_BAMBAM_CDIFF
				)
				{
					RA.getNext(opA);

					switch ( opA )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
						case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CINS:
							readposa += 1;
							break;
						case BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
						default:
							break;
					}
				}

				int64_t refposa = getPos();

				while ( RA.getNext(opA) )
				{
					switch ( opA )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
							V.push_back(std::pair<int64_t,int64_t>(refposa,readposa));
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
							refposa += 1;
							readposa += 1;
							break;
						case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CINS:
							readposa += 1;
							break;
						case BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
						case BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
							refposa += 1;
						default:
							break;
					}
				}
			}

			/**
			 *
			 **/
			static bool cross(
				BamAlignment const & A,
				BamAlignment const & B
			)
			{
				if ( A.isUnmap() || B.isUnmap() )
					return false;
				assert ( A.isMapped() && B.isMapped() );

				if ( A.getRefID() != B.getRefID() )
					return false;
				if ( A.isReverse() != B.isReverse() )
					return false;

				std::vector<std::pair<int64_t,int64_t> > VA;
				std::vector<std::pair<int64_t,int64_t> > VB;
				A.getMappingPositionPairs(VA);
				B.getMappingPositionPairs(VB);
				std::sort(VA.begin(),VA.end());
				std::sort(VB.begin(),VB.end());
				VA.resize(std::unique(VA.begin(),VA.end())-VA.begin());
				VB.resize(std::unique(VB.begin(),VB.end())-VB.begin());
				std::copy(VB.begin(),VB.end(),std::back_insert_iterator<std::vector<std::pair<int64_t,int64_t> > >(VA));
				std::sort(VA.begin(),VA.end());

				uint64_t low = 0;
				while ( low < VA.size() )
				{
					uint64_t high = low+1;
					while ( high < VA.size() && VA[low] == VA[high] )
						++high;

					if ( high-low > 1 )
						return true;

					low = high;
				}

				return false;
			}

			/**
			 * @return number of bytes before the query sequence in the alignment
			 **/
			uint64_t getNumPreSeqBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumPreSeqBytes(D.begin());
			}

			/**
			 * @return number of bytes representing the query sequence in the alignment
			 **/
			uint64_t getNumSeqBytes() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNumSeqBytes(D.begin());
			}

			/**
			 * @return number of bytes after the query sequence in the alignment
			 **/
			uint64_t getNumPostSeqBytes() const
			{
				return blocksize - (getNumPreSeqBytes() + getNumSeqBytes());
			}

			/**
			 * replace cigar string in the alignment by the given string
			 *
			 * @param cigarstring replacement cigarstring
			 **/
			void replaceCigarString(std::string const & cigarstring)
			{
				std::vector<cigar_operation> cigar = ::libmaus2::bambam::CigarStringParser::parseCigarString(cigarstring);
				libmaus2::bambam::BamAlignment::D_array_type T;
				replaceCigarString(cigar.begin(),cigar.size(),T);
			}

			/**
			 * replace cigar string in the alignment by the given string
			 *
			 * @param cigarstring replacement cigarstring
			 * @param T temporary array
			 **/
			void replaceCigarString(std::string const & cigarstring, libmaus2::bambam::BamAlignment::D_array_type & T)
			{
				std::vector<cigar_operation> cigar = ::libmaus2::bambam::CigarStringParser::parseCigarString(cigarstring);
				replaceCigarString(cigar.begin(),cigar.size(),T);
			}

			/**
			 * replace query sequence in the alignment by the given string
			 *
			 * @param sequence replacement sequence
			 **/
			void replaceSequence(std::string const & sequence, std::string const & quality)
			{
				BamSeqEncodeTable const seqenc;
				replaceSequence(seqenc,sequence,quality);
			}

			/**
			 * replace query sequence in the alignment by the given string using
			 * the given encode table
			 *
			 * @param seqenc sequence encoding table
			 * @param sequence replacement sequence
			 **/
			void replaceSequence(
				BamSeqEncodeTable const & seqenc,
				std::string const & sequence,
				std::string const & quality
			)
			{
				libmaus2::autoarray::AutoArray<uint8_t,D_array_alloc_type> T;
				replaceSequence(seqenc,sequence.begin(),quality.begin(),sequence.size(),T);
			}

			/**
			 * replace query sequence in the alignment by the given string using
			 * the given encode table
			 *
			 * @param seqenc sequence encoding table
			 * @param seq sequence start iterator
			 * @param seqlen sequence length
			 **/
			template<typename seq_iterator, typename qual_iterator>
			void replaceSequence(
				BamSeqEncodeTable const & seqenc,
				seq_iterator seq,
				qual_iterator qual,
				uint32_t const seqlen,
				libmaus2::autoarray::AutoArray<uint8_t,D_array_alloc_type> & T
			)
			{
				uint64_t const oldlen = getLseq();
				uint64_t const pre    = getNumPreSeqBytes();
				uint64_t const oldseq = getNumSeqBytes();
				// uint64_t const newseq = (seqlen+1)/2;
				uint64_t const post   = getNumPostSeqBytes();

				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( T, 0 /* pre + newseq + post */ );

				// pre seq data
				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				// seq data
				::libmaus2::bambam::BamAlignmentEncoderBase::encodeSeq(buffer,seqenc,seq,seqlen);

				// quality data
				for ( uint64_t i = 0; i < seqlen; ++i )
					buffer.put(*(qual++) - 33);

				// post seq,qual data
				for ( uint64_t i = 0; i < post-oldlen; ++i )
					buffer.put ( D [ pre + oldseq + oldlen + i ] );

				D.swap(buffer.abuffer);
				T.swap(buffer.abuffer);
				blocksize = buffer.length;

				putSeqLen(seqlen);
			}

			/**
			 * erase query sequence in the alignment by the given string using
			 * the given encode table
			 **/
			void eraseSequence()
			{
				uint64_t const oldlen = getLseq();
				uint64_t const pre    = getNumPreSeqBytes();
				uint64_t const oldseq = getNumSeqBytes();
				uint64_t const post   = getNumPostSeqBytes();

				uint8_t const * copyfrom = D.begin() + pre + oldseq + oldlen;
				uint8_t * copyto = D.begin() + pre;
				uint64_t copylen = post-oldlen;

				::std::memmove(copyto,copyfrom,copylen);

				blocksize = pre + copylen;
				putSeqLen(0);
			}

			template<typename iterator>
			void replaceName(iterator ita, uint64_t const n)
			{
				uint64_t const pre = getNumPreNameBytes();
				uint64_t const oldname = getNumNameBytes();
				uint64_t const post = getNumPostNameBytes();
				uint64_t const newsize =  pre + (n+1) + post ;

				if ( newsize > D.size() )
					D.resize(newsize);

				assert ( D.size() >= newsize );

				memmove(D.begin() + pre + (n+1),D.begin() + pre + oldname,post);
				std::copy(ita,ita+n,D.begin()+pre);
				D[pre+n] = 0;

				blocksize = newsize;

				D[8] = n+1;
			}

			/**
			 * replace cigar string by the given encoded string
			 *
			 * @param cigar pre encoded cigar string iterator
			 * @param cigarlen number of cigar operations
			 **/
			template<typename cigar_iterator>
			void replaceCigarString(
				cigar_iterator cigar, uint32_t const cigarlen,
				libmaus2::bambam::BamAlignment::D_array_type & T
			)
			{
				uint64_t const pre    = getNumPreCigarBytes();
				uint64_t const oldcig = getNumCigarBytes();
				uint64_t const post   = getNumPostCigarBytes();

				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( T,0 /* getNumPreCigarBytes() + cigarlen * sizeof(uint32_t) + getNumPostCigarBytes() */ );

				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				::libmaus2::bambam::BamAlignmentEncoderBase::encodeCigar(buffer,cigar,cigarlen);

				for ( uint64_t i = 0; i < post; ++i )
					buffer.put ( D [ pre + oldcig + i ] );

				D.swap(buffer.abuffer);
				T.swap(buffer.abuffer);
				blocksize = buffer.length;

				putCigarLen(cigarlen);
			}

			/**
			 * erase the cigar string of an alignment block and return the new block size
			 *
			 * @param D alignment block data
			 * @param blocksize input block size
			 * @return new block size
			 **/
			static uint64_t eraseCigarString(uint8_t * const D, uint64_t blocksize)
			{
				uint64_t const pre    = ::libmaus2::bambam::BamAlignmentDecoderBase::getNumPreCigarBytes(D);
				uint64_t const oldcig = ::libmaus2::bambam::BamAlignmentDecoderBase::getNumCigarBytes(D);
				uint64_t const post   = ::libmaus2::bambam::BamAlignmentDecoderBase::getNumPostCigarBytes(D,blocksize);

				// move post cigar string bytes to position of old cigar string
				memmove(
					D + pre, // dest
					D + pre + oldcig,  // src
					post // n
				);

				blocksize -= oldcig;

				::libmaus2::bambam::BamAlignmentEncoderBase::putCigarLen(D,0);

				return blocksize;
			}

			/**
			 * erase the cigar string
			 **/
			void eraseCigarString()
			{
				blocksize = eraseCigarString(D.begin(),blocksize);
			}

			/**
			 * load alignment from stream in
			 *
			 * @param in input stream
			 * @return shared pointer to new alignment object
			 **/
			template<typename input_type>
			static shared_ptr_type load(input_type & in)
			{
				if ( in.peek() < 0 )
					return shared_ptr_type();
				else
				{
					shared_ptr_type ptr(new this_type(in));
					return ptr;
				}
			}

			/**
			 * serialise this alignment to output stream out
			 *
			 * @param out output stream
			 **/
			template<typename output_type>
			void serialise(output_type & out) const
			{
				::libmaus2::bambam::EncoderBase::putLE<output_type,uint32_t>(out,blocksize);
				out.write(reinterpret_cast<char const *>(D.begin()),blocksize);
			}

			/**
			 * @return number of bytes in serialised representation
			 **/
			uint64_t serialisedByteSize() const
			{
				return sizeof(uint32_t) + blocksize;
			}

			/**
			 * @return hash value computed from query name
			 **/
			uint64_t hash() const
			{
				return ::libmaus2::hashing::EvaHash::hash64(reinterpret_cast<uint8_t const *>(getName()),
					getLReadName());
			}

			/**
			 * clone this alignment and return the cloned alignment in a unique pointer
			 *
			 * @return unique pointer containing cloned alignment
			 **/
			unique_ptr_type uclone() const
			{
				unique_ptr_type P(new BamAlignment);
				P->D = D.clone();
				P->blocksize = blocksize;
				return UNIQUE_PTR_MOVE(P);
			}

			/**
			 * clone this alignment and return the cloned alignment in a shared pointer
			 *
			 * @return shared pointer containing cloned alignment
			 **/
			shared_ptr_type sclone() const
			{
				shared_ptr_type P(new BamAlignment);
				P->D = D.clone();
				P->blocksize = blocksize;
				return P;
			}

			/**
			 * get auxiliary field for tag as string, returns empty string if tag is not present
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return contents of auxiliary field identified by tag
			 **/
			std::string getAuxAsString(char const * const tag) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsString(D.get(),blocksize,tag);
			}

			/**
			 * get auxiliary field for tag (case insensative) as string, returns empty
			 * string if tag is not present
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return contents of auxiliary field identified by tag
			 **/
			std::string getAuxAsStringNC(char const * const tag) const
			{
			    std::string ret = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsString(D.get(),blocksize,tag);

			    if (ret.empty()) {
			    	char alt[3];

				strncpy(alt, tag, 2);
				alt[2] = '\0';

				if (isupper(alt[0])) {
			    	    alt[0] = tolower(alt[0]);
				    alt[1] = tolower(alt[1]);
				} else {
			    	    alt[0] = toupper(alt[0]);
				    alt[1] = toupper(alt[1]);
				}

				ret = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsString(D.get(),blocksize,alt);
			    }

			    return ret;
			}

			/**
			 * get auxiliary field for tag as number, throws an exception if tag is not present or
			 * tag content is not numeric
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return contents of auxiliary field identified by tag as number
			 **/
			template<typename N>
			N getAuxAsNumber(char const * const tag) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag);
			}

			/**
			 * get auxiliary field for tag as number
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @param num space for storing number
			 * @return true if field is present and can be extracted
			 **/
			template<typename N>
			bool getAuxAsNumber(char const * const tag, N & num) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag,num);
			}

			/**
			 * get auxiliary field for tag (case insensative) as number, throws an exception if tag is
			 * not present or tag content is not numeric
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return contents of auxiliary field identified by tag as number
			 **/
			template<typename N>
			N getAuxAsNumberNC(char const *const tag) const
			{
			    char alt[3];

			    strncpy(alt, tag, 2);
			    alt[2] = '\0';

			    if (isupper(alt[0])) {
			    	alt[0] = tolower(alt[0]);
				alt[1] = tolower(alt[1]);
			    } else {
			    	alt[0] = toupper(alt[0]);
				alt[1] = toupper(alt[1]);
			    }

			    N num;

			    if (::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag,num)) {
			    	return num;
			    } else {
			    	return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,alt);
			    }
			}

			/**
			 * get auxiliary field for tag (case insensative) as number
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @param num space for storing number
			 * @return true if field is present and can be extracted
			 **/
			template<typename N>
			bool getAuxAsNumberNC(char const *const tag, N & num) const
			{
			    char alt[3];

			    strncpy(alt, tag, 2);
			    alt[2] = '\0';

			    if (isupper(alt[0])) {
			    	alt[0] = tolower(alt[0]);
				alt[1] = tolower(alt[1]);
			    } else {
			    	alt[0] = toupper(alt[0]);
				alt[1] = toupper(alt[1]);
			    }

			    if (::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag,num)) {
			    	return true;
			    } else {
			    	return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,alt,num);
			    }
			}


			/**
			 * check if auxiliary field for tag is present
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return true iff tag is present
			 **/
			bool hasAux(char const * const tag) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,tag) != 0;
			}


			/**
			 * check if auxiliary field for tag (case insensative) is present
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return true iff tag is present
			 **/
			bool hasAuxNC(char const * const tag) const
			{
			    char alt[3];

			    strncpy(alt, tag, 2);
			    alt[2] = '\0';

			    if (isupper(alt[0])) {
			    	alt[0] = tolower(alt[0]);
				alt[1] = tolower(alt[1]);
			    } else {
			    	alt[0] = toupper(alt[0]);
				alt[1] = toupper(alt[1]);
			    }

			    if (::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,tag) != 0) {
			    	return true;
			    } else {
			    	return ::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,alt) != 0;
			    }
			}


			/**
			 * add auxiliary field for id tag containing a number array representing V
			 *
			 * @param tag aux id
			 * @param type number type
			 * @param v number value
			 **/
			template<typename value_type>
			void putAuxNumber(char const * const tag, char const type, value_type const v)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumber(data,tag,type,v);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a number array representing V
			 *
			 * @param tag aux id
			 * @param V number array
			 **/
			template<typename value_type>
			void putAuxNumber(std::string const & tag, char const type, value_type const v)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumber(data,tag,type,v);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a string value
			 *
			 * @param tag aux id
			 * @param value string
			 **/
			void putAuxString(std::string const & tag, std::string const & value)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(data,tag,value.c_str());

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a string value
			 *
			 * @param tag aux id
			 * @param value string
			 **/
			void putAuxString(char const * tag, char const * value)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(data,tag,value);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a number array representing V
			 *
			 * @param tag aux id
			 * @param V number array
			 **/
			void putAuxNumberArray(std::string const & tag, std::vector<uint8_t> const & V)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',V);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a number array representing V
			 *
			 * @param tag aux id
			 * @param V number array
			 * @param n length of array V
			 **/
			template<typename iterator_type>
			void putAuxNumberArray(std::string const & tag, iterator_type values, uint64_t const n)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',values,n);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * add auxiliary field for id tag containing a number array representing V
			 *
			 * @param tag aux id
			 * @param V number array
			 * @param n length of array V
			 **/
			template<typename iterator_type>
			void putAuxNumberArray(char const * const tag, iterator_type values, uint64_t const n)
			{
				::libmaus2::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',values,n);

				D = data.abuffer;
				blocksize = data.length;
			}

			/**
			 * container wrapper mapping the put operation to a push_back on the container
			 **/
			template<typename container_type>
			struct PushBackPutObject
			{
				//! wrapped container
				container_type & cont;

				/**
				 * constructor
				 *
				 * @param rcont container to be wrapped
				 **/
				PushBackPutObject(container_type & rcont) : cont(rcont) {}

				/**
				 * insert c in the back of the wrapped container
				 *
				 * @param c element to be inserted at the end of the wrapped container
				 **/
				void put(typename container_type::value_type const c)
				{
					cont.push_back(c);
				}
			};

			/**
			 * put utf-8 representation of num in the container cont
			 *
			 * @param cont sequence container
			 * @param num number to be encoded
			 **/
			template<typename container_type>
			static void putUtf8Integer(container_type & cont, uint32_t const num)
			{
				PushBackPutObject<container_type> PBPO(cont);
				::libmaus2::util::UTF8::encodeUTF8(num,PBPO);
			}

			/**
			 * put rank value as auxiliary tag contaning a number sequence (eight byte big endian)
			 *
			 * @param tag aux field id
			 * @param rank field content
			 **/
			void putRank(std::string const & tag, uint64_t const rank /*, ::libmaus2::bambam::BamHeader const & bamheader */)
			{
				std::vector<uint8_t> V;

				#if 0
				if (retval == 0) retval = lhs.read2Coordinate - rhs.read2Coordinate;
				if (retval == 0) retval = (int) (lhs.read1IndexInFile - rhs.read1IndexInFile);
				if (retval == 0) retval = (int) (lhs.read2IndexInFile - rhs.read2IndexInFile);
                            	#endif

				#if 0
				// put library id
				putUtf8Integer(V,getLibraryId(bamheader));
				// chromosome id + 1
				putUtf8Integer(V,getRefIDChecked()+1);
				// position + 1
				putUtf8Integer(V,getCoordinate()+1);
				// orientation
				V.push_back(isReverse() ? 1 : 0);
				// next id + 1
				putUtf8Integer(V,getNextRefIDChecked()+1);
				// next pos + 1
				putUtf8Integer(V,getNextPosChecked()+1);
				#endif

				// rank of read (big endian for lexicographic sorting)
				for ( uint64_t i = 0; i < sizeof(uint64_t); ++i )
					V.push_back( (rank >> (8*sizeof(uint64_t)-8*(i+1))) & 0xFF );

				putAuxNumberArray(tag,V);
			}

			/**
			 * decode rank aux field
			 *
			 * @return rank field or -1 if not present or invalid
			 **/
			int64_t getRank(char const * tag = "ZR") const
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				uint8_t const * p =
					::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,tag);

				// check format
				if (
					(! p)
					||
					(p[0] != tag[0])
					||
					(p[1] != tag[1])
					||
					(p[2] != 'B')
					||
					(p[3] != 'C')
					||
					(p[4] != 8)
					||
					(p[5] != 0)
					||
					(p[6] != 0)
					||
					(p[7] != 0)
				)
					return -1;
				// decode
				else
				{
					uint64_t r = 0;

					for ( unsigned int i = 0; i < sizeof(uint64_t); ++i )
					{
						r <<= 8;
						r |= p[8+i];
					}

					return r;
				}
			}

			/**
			 * @return 32 bit hash value computed from alignment
			 **/
			uint32_t hash32() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::hash32(D.begin());
			}

			/**
			 * @return length of query sequence as encoded in the cigar string
			 **/
			uint64_t getLseqByCigar() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getLseqByCigar(D.begin());
			}

			/**
			 * @return iff sequence length encoded in cigar string is consistent with the length field
			 **/
			bool isCigarLengthConsistent() const
			{
				return static_cast<int64_t>(getLseqByCigar()) == getLseq();
			}

			/**
			 * @return contents of the RG aux field (null pointer if not present)
			 **/
			char const * getReadGroup() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D.begin(),blocksize,"RG");
			}

			/**
			 * @param tag aux field tag
			 * @return contents of the aux field (null pointer if not present)
			 **/
			char const * getAuxString(char const * tagname) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D.begin(),blocksize,tagname);
			}

			/**
			 * @param tag aux field tag (case insensative)
			 * @return contents of the aux field (null pointer if not present)
			 **/
			char const * getAuxStringNC(char const * const tag) const
			{
			    const char *ret = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D.get(),blocksize,tag);

			    if (!ret) {
			    	char alt[3];

				strncpy(alt, tag, 2);
				alt[2] = '\0';

				if (isupper(alt[0])) {
			    	    alt[0] = tolower(alt[0]);
				    alt[1] = tolower(alt[1]);
				} else {
			    	    alt[0] = toupper(alt[0]);
				    alt[1] = toupper(alt[1]);
				}

				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D.get(),blocksize,alt);
			    }

			    return ret;
			}

			/**
			 * @param bamheader BAM header object
			 * @return read group id or -1 if not defined
			 **/
			template<typename header_type>
			int64_t getReadGroupId(header_type const & bamheader) const
			{
				return bamheader.getReadGroupId(getReadGroup());
			}

			/**
			 * @param bamheader BAM header object
			 * @return library id or -1 if not defined
			 **/
			template<typename header_type>
			int64_t getLibraryId(header_type const & bamheader) const
			{
				return bamheader.getLibraryId(getReadGroup());
			}

			/**
			 * @param bamheader BAM header object
			 * @return library name for this alignment
			 **/
			template<typename header_type>
			std::string getLibraryName(header_type const & bamheader) const
			{
				return bamheader.getLibraryName(getReadGroup());
			}

			/**
			 * format alignment as SAM file line
			 *
			 * @param ostr output stream
			 * @param bamheader BAM header object
			 * @param auxiliary formatting auxiliary object
			 * @return SAM file line as string
			 **/
			template<typename header_type>
			void formatAlignment(
				std::ostream & ostr,
				header_type const & bamheader,
				::libmaus2::bambam::BamFormatAuxiliary & auxiliary
			) const
			{
				::libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(
					ostr,D.get(),blocksize,bamheader,auxiliary);
			}

			/**
			 * format alignment as SAM file line
			 *
			 * @param bamheader BAM header object
			 * @param auxiliary formatting auxiliary object
			 * @return SAM file line as string
			 **/
			template<typename header_type>
			std::string formatAlignment(
				header_type const & bamheader,
				::libmaus2::bambam::BamFormatAuxiliary & auxiliary
			) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(
					D.get(),blocksize,bamheader,auxiliary);
			}

			/**
			 * format alignment as SAM file line
			 *
			 * @param bamheader BAM header object
			 * @return SAM file line as string
			 **/
			template<typename header_type>
			std::string formatAlignment(header_type const & bamheader) const
			{
				::libmaus2::bambam::BamFormatAuxiliary auxiliary;
				return formatAlignment(bamheader,auxiliary);
			}

			/**
			 * format alignment as FastQ
			 *
			 * @param auxiliary formatting auxiliary object
			 * @return FastQ entry as string
			 **/
			std::string formatFastq(::libmaus2::bambam::BamFormatAuxiliary & auxiliary) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::formatFastq(D.get(),auxiliary);
			}

			/**
			 * format alignment as FastQ
			 *
			 * @return FastQ entry as string
			 **/
			std::string formatFastq() const
			{
				::libmaus2::bambam::BamFormatAuxiliary auxiliary;
				return formatFastq(auxiliary);
			}

			/**
			 * @return query name
			 **/
			char const * getName() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(D.get());
			}

			/**
			 * @return reference id
			 **/
			int32_t getRefID() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->RefID;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getRefID(D.get());
				#endif
			}

			/**
			 * @return checked reference id (maps values smaller than -1 to -1)
			 **/
			int32_t getRefIDChecked() const
			{
				int32_t const refid = getRefID();

				if ( refid < 0 )
					return -1;
				else
					return refid;
			}

			/**
			 * @return alignment position
			 **/
			int32_t getPos() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Pos;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(D.get());
				#endif
			}

			/**
			 * @return checked alignment position (maps values smaller than -1 to -1)
			 **/
			int32_t getPosChecked() const
			{
				int32_t const pos = getPos();

				if ( pos < 0 )
					return -1;
				else
					return pos;
			}

			/**
			 * @return reference id for next/mate
			 **/
			int32_t getNextRefID() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NextRefID;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNextRefID(D.get());
				#endif
			}
			/**
			 * @return checked reference id for next/mate (returns -1 for values smaller -1)
			 **/
			int32_t getNextRefIDChecked() const
			{
				int32_t const nextrefid = getNextRefID();

				if ( nextrefid < 0 )
					return -1;
				else
					return nextrefid;
			}

			/**
			 * @return position of next/mate
			 **/
			int32_t getNextPos() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NextPos;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNextPos(D.get());
				#endif
			}
			/**
			 * @return checked position of next/mate (returns -1 for values smaller than -1)
			 **/
			int32_t getNextPosChecked() const
			{
				int32_t const nextpos = getNextPos();

				if ( nextpos < 0 )
					return -1;
				else
					return nextpos;
			}

			/**
			 * @return end of alignment on reference
			 **/
			uint64_t getAlignmentEnd() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getAlignmentEnd(D.get());
			}

			/**
			 * @return bin field
			 **/
			uint32_t getBin() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Bin;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getBin(D.get());
				#endif
			}

			/**
			 * @return mapping quality field
			 **/
			uint32_t getMapQ() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->MQ;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getMapQ(D.get());
				#endif
			}

			/**
			 * @return length of read name
			 **/
			uint32_t getLReadName() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NL;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getLReadName(D.get());
				#endif
			}

			/**
			 * @return flags field
			 **/
			uint32_t getFlags() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Flags;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(D.get());
				#endif
			}

			/**
			 * @return number of cigar operations field
			 **/
			uint32_t getNCigar() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NC;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getNCigar(D.get());
				#endif
			}

			/**
			 * @return cigar string
			 **/
			std::string getCigarString() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarString(D.get());
			}

			/**
			 * get cigar string
			 *
			 * @param A array to store string, will be reallocated if necessary
			 * @return l length of stored cigar string (without terminating nul byte)
			 **/
			size_t getCigarString(libmaus2::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarString(D.begin(),A);
			}

			/**
			 * @return string representation of flags
			 **/
			std::string getFlagsS() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getFlagsS(D.get());
			}

			/**
			 * return the i'th cigar operation as character (as in SAM)
			 *
			 * @param i cigar operator index (0 based)
			 * @return the i'th cigar operation as character
			 **/
			char getCigarFieldOpAsChar(uint64_t const i) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarFieldOpAsChar(D.get(),i);
			}

			/**
			 * return the i'th cigar operator as integer (see bam_cigar_ops enumeration)
			 *
			 * @param i cigar operator index (0 based)
			 * @return the i'th cigar operation as integer
			 **/
			uint32_t getCigarFieldOp(uint64_t const i) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarFieldOp(D.get(),i);
			}
			/**
			 * get length of i'th cigar operation
			 *
			 * @param i cigar operator index (0 based)
			 * @return length of i'th cigar operation
			 **/
			uint32_t getCigarFieldLength(uint64_t const i) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarFieldLength(D.get(),i);
			}

			/**
			 * get i'th cigar operation field (in BAM encoding)
			 *
			 * @param i cigar operator index (0 based)
			 * @return i'th cigar operation field
			 **/
			uint32_t getCigarField(uint64_t const i) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarField(D.get(),i);
			}

			/**
			 * @return template length field
			 **/
			int32_t getTlen() const
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Tlen;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getTlen(D.get());
				#endif
			}

			static int32_t getLseq(uint8_t const * D)
			{
				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D)->Lseq;
				#else
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getLseq(D);
				#endif
			}

			/**
			 * @return sequence length field
			 **/
			int32_t getLseq() const
			{
				return getLseq(D.begin());
			}

			/**
			 * decode query sequence to array A, which is extended if necessary
			 *
			 * @param A reference to array
			 * @return length of sequence
			 **/
			uint64_t decodeRead(::libmaus2::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeRead(D.get(),A);
			}
			/**
			 * decode reverse complement of query sequence to array A, which is extended if necessary
			 *
			 * @param A reference to array
			 * @return length of sequence
			 **/
			uint64_t decodeReadRC(::libmaus2::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeReadRC(D.get(),A);
			}
			/**
			 * decode quality string
			 *
			 * @param A reference to array
			 * @return length of quality string
			 **/
			uint64_t decodeQual(::libmaus2::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeQual(D.get(),A);
			}
			/**
			 * decode reverse of quality string
			 *
			 * @param A reference to array
			 * @return length of quality string
			 **/
			uint64_t decodeQualRC(::libmaus2::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeQualRC(D.get(),A);
			}

			/**
			 * @return computed bin
			 **/
			uint64_t computeBin() const
			{
				uint64_t const rbeg = getPos();
				uint64_t const rend = rbeg + getReferenceLength();
				uint64_t const bin = libmaus2::bambam::BamAlignmentEncoderBase::reg2bin(rbeg,rend);
				return bin;
			}

			/**
			 * @return length of alignment on reference
			 **/
			uint64_t getReferenceLength() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getReferenceLength(D.get());
			}

			/**
			 * @return interval of mapping reference positions (pos,pos+getReferenceLength()-1) or empty if not mapped
			 **/
			libmaus2::math::IntegerInterval<int64_t> getReferenceInterval() const
			{
				if ( isUnmap() || (!getReferenceLength()) )
					return libmaus2::math::IntegerInterval<int64_t>::empty();
				else
					return libmaus2::math::IntegerInterval<int64_t>(getPos(), getPos() + getReferenceLength() - 1);
			}

			/**
			 * @return query sequence as string
			 **/
			std::string getRead() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeRead(A);
				return std::string(A.begin(),A.begin()+len);
			}

			/**
			 * @return reverse complement of query sequence as string
			 **/
			std::string getReadRC() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeReadRC(A);
				return std::string(A.begin(),A.begin()+len);
			}

			/**
			 * @return quality string as string
			 **/
			std::string getQual() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeQual(A);
				return std::string(A.begin(),A.begin()+len);
			}
			/**
			 * @return reverse quality string as string
			 **/
			std::string getQualRC() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeQualRC(A);
				return std::string(A.begin(),A.begin()+len);
			}

			/**
			 * @return iff alignment has the pair flag set
			 **/
			bool isPaired() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isPaired(getFlags()); }
			/**
			 * @return iff alignment has the proper pair flag set
			 **/
			bool isProper() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isProper(getFlags()); }
			/**
			 * @return iff alignment has the unmapped flag set
			 **/
			bool isUnmap() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(getFlags()); }
			/**
			 * @return iff alignment does not have the unmapped flag set
			 **/
			bool isMapped() const { return !isUnmap(); }
			/**
			 * @return iff alignment has the mate unmapped flag set
			 **/
			bool isMateUnmap() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isMateUnmap(getFlags()); }
			/**
			 * @return iff alignment has the mate unmapped flag not set
			 **/
			bool isMateMapped() const { return !isMateUnmap(); }
			/**
			 * @return iff alignment has the reverse strand flag set
			 **/
			bool isReverse() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isReverse(getFlags()); }
			/**
			 * @return iff alignment has the mate reverse strand flag set
			 **/
			bool isMateReverse() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isMateReverse(getFlags()); }
			/**
			 * @return iff alignment has read 1 flag set
			 **/
			bool isRead1() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isRead1(getFlags()); }
			/**
			 * @return iff alignment has read 2 flag set
			 **/
			bool isRead2() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isRead2(getFlags()); }
			/**
			 * @return iff alignment has the secondary alignment flag set
			 **/
			bool isSecondary() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isSecondary(getFlags()); }
			/**
			 * @return iff alignment has the supplementary alignment flag set
			 **/
			bool isSupplementary() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isSupplementary(getFlags()); }
			/**
			 * @return iff alignment has the quality control failed flag set
			 **/
			bool isQCFail() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isQCFail(getFlags()); }
			/**
			 * @return iff alignment has the duplicate entry flag set
			 **/
			bool isDup() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::isDup(getFlags()); }

			/**
			 * @return coordinate of the leftmost query sequence base (as opposed to the coordinate of the leftmost aligned query sequence base)
			 **/
			int64_t getCoordinate() const
			{
				if ( !isUnmap() )
					return ::libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(D.get());
				else
					return -1;
			}

			/**
			 * get number of insertions(+)/padding(+)/deletions(-) before first matching/mismatching base
			 *
			 * @param D alignment block
			 * @return number of insertions
			 **/
			int64_t getOffsetBeforeMatch() const
			{
				if ( !isUnmap() )
					return ::libmaus2::bambam::BamAlignmentDecoderBase::getOffsetBeforeMatch(D.get());
				else
					return 0;
			}

			/**
			 * get quality value
			 *
			 * @return sum of quality score values of at least 15
			 **/
			uint64_t getScore() const { return ::libmaus2::bambam::BamAlignmentDecoderBase::getScore(D.get()); }

			/**
			 * format alignment to FASTQEntry object
			 *
			 * @param pattern fastq object
			 * @param patid pattern id
			 **/
			void toPattern(::libmaus2::fastx::FASTQEntry & pattern, uint64_t const patid) const
			{
				pattern.patid = patid;
				pattern.sid = getName();

				if ( isPaired() )
				{
					if ( isRead1() )
						pattern.sid += "/1";
					else
						pattern.sid += "/2";
				}

				pattern.spattern = isReverse() ? getReadRC() : getRead();
				pattern.pattern = pattern.spattern.c_str();
				pattern.patlen = pattern.spattern.size();
				pattern.plus.resize(0);
				pattern.quality = isReverse() ? getQualRC() : getQual();
				pattern.mapped = 0;
				pattern.transposed = 0;
				pattern.smapped.resize(0);
				pattern.stransposed.resize(0);
			}

			/**
			 * @param A first alignment
			 * @param B second alignment
			 * @return true iff A and B form a pair
			 **/
			static bool isPair(BamAlignment const & A, BamAlignment const & B)
			{
				// compare name
				if ( ! A.isPaired() || ! B.isPaired() )
					return false;
				if ( strcmp(A.getName(),B.getName()) )
					return false;

				if ( A.isRead1() && B.isRead2() )
					return true;
				if ( A.isRead2() && B.isRead1() )
					return true;

				return false;
			}

			/**
			 * replace reference id by v
			 *
			 * @param v new reference id
			 **/
			void putRefId(int32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putRefId(D.get(),v); }
			/**
			 * replace position by v
			 *
			 * @param v new position
			 **/
			void putPos(int32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putPos(D.get(),v); }
			/**
			 * replace mate reference id by v
			 *
			 * @param v new mate reference id
			 **/
			void putNextRefId(int32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(D.get(),v); }
			/**
			 * replace mate position by v
			 *
			 * @param v new mate position
			 **/
			void putNextPos(int32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(D.get(),v); }
			/**
			 * replace template length by v
			 *
			 * @param v new template length
			 **/
			void putTlen(int32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putTlen(D.get(),v); }
			/**
			 * replace flags by v
			 *
			 * @param v new flags
			 **/
			void putFlags(uint32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putFlags(D.get(),v); }
			/**
			 * replace flags by v
			 *
			 * @param v new flags
			 **/
			void putMapQ(uint8_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putMapQ(D.get(),v); }
			/**
			 * replace number of cigar operations by v
			 *
			 * @param v new number of cigar operations
			 **/
			void putCigarLen(uint32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putCigarLen(D.get(),v); }
			/**
			 * replace sequence length field by v
			 *
			 * @param v new sequence length field
			 **/
			void putSeqLen(uint32_t const v) { ::libmaus2::bambam::BamAlignmentEncoderBase::putSeqLen(D.get(),v); }

			/**
			 * format alignment as fastq
			 *
			 * @param it output iterator
			 * @return output iterator after formatting sequence
			 **/
			template<typename iterator>
			iterator putFastQ(iterator it) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::putFastQ(D.get(),it);
			}

			/**
			 * @return length of alignment formatted as fastq entry
			 **/
			uint64_t getFastQLength() const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::getFastQLength(D.get());
			}

			/**
			 * decode query sequence to S
			 *
			 * @param S output iterator
			 * @param seqlen length of query sequence
			 * @return output iterator after writing the query sequence
			 **/
			template<typename iterator>
			iterator decodeRead(iterator S, uint64_t const seqlen) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeRead(D.get(),S,seqlen);
			}

			/**
			 * decode reverse complement of query sequence to S
			 *
			 * @param S output iterator
			 * @param seqlen length of query sequence
			 * @return output iterator after writing the reverse complement of the query sequence
			 **/
			template<typename iterator>
			iterator decodeReadRCIt(iterator S, uint64_t const seqlen) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeReadRCIt(D.get(),S,seqlen);
			}

			/**
			 * decode quality string to S
			 *
			 * @param it output iterator
			 * @param seqlen length of quality string
			 * @return output iterator after writing the quality string
			 **/
                        template<typename iterator>
                        iterator decodeQual(iterator it, uint64_t const seqlen) const
                        {
                        	return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeQualIt(D.get(),it,seqlen);
                        }

			/**
			 * decode reverse quality string to S
			 *
			 * @param it output iterator
			 * @param seqlen length of quality string
			 * @return output iterator after writing the reverse quality string
			 **/
                        template<typename iterator>
                        iterator decodeQualRcIt(iterator it, uint64_t const seqlen) const
                        {
                        	return ::libmaus2::bambam::BamAlignmentDecoderBase::decodeQualRcIt(D.get(),it,seqlen);
                        }

			/**
			 * filter auxiliary tags keeping only those in a given list
			 *
			 * @param tags list of tag identifiers to be kept
			 **/
			void filterAux(BamAuxFilterVector const & tags)
			{
				blocksize = ::libmaus2::bambam::BamAlignmentDecoderBase::filterAux(D.begin(),blocksize,tags);
			}

			/**
			 * erase auxiliary tags
			 **/
			void eraseAux()
			{
				blocksize = ::libmaus2::bambam::BamAlignmentDecoderBase::eraseAux(D.begin());
			}

			/**
			 * filter auxiliary tags keeping and remove those in the list
			 *
			 * @param tags list of tag identifiers to be removed
			 **/
			void filterOutAux(BamAuxFilterVector const & tags)
			{
				blocksize = ::libmaus2::bambam::BamAlignmentDecoderBase::filterOutAux(D.begin(),blocksize,tags);
			}

			/**
			 * sort aux fields by tag id
			 *
			 * @param sortbuffer buffer for sorting
			 **/
			void sortAux(BamAuxSortingBuffer & sortbuffer)
			{
				::libmaus2::bambam::BamAlignmentDecoderBase::sortAux(D.begin(),blocksize,sortbuffer);
			}

			/**
			 * enumerate aux tags in array A; A will be resized if needed
			 *
			 * @param A array for storing aux tag markers
			 * @return number of markers stored
			 **/
			uint64_t enumerateAuxTags(libmaus2::autoarray::AutoArray < std::pair<uint8_t,uint8_t> > & A) const
			{
				return ::libmaus2::bambam::BamAlignmentDecoderBase::enumerateAuxTags(D.begin(),blocksize,A);
			}

			/**
			 * copy aux tags from one alignment to another
			 *
			 * @param O source alignment
			 * @param filter tag set to be copied
			 **/
			void copyAuxTags(libmaus2::bambam::BamAlignment const & O, BamAuxFilterVector const & filter)
			{
				/*
				 * compute number of bytes we will add to this alignment block
				 */
				uint8_t const * aux = libmaus2::bambam::BamAlignmentDecoderBase::getAux(O.D.begin());
				uint64_t addlength = 0;

				while ( aux < O.D.begin() + O.blocksize )
				{
					uint64_t const length = libmaus2::bambam::BamAlignmentDecoderBase::getAuxLength(aux);

					if ( filter(aux[0],aux[1]) )
						addlength += length;

					aux += length;
				}

				// resize block array if necessary
				if ( blocksize + addlength > D.size() )
					D.resize(blocksize + addlength);

				// copy aux fields
				aux = libmaus2::bambam::BamAlignmentDecoderBase::getAux(O.D.begin());
				uint8_t * outp = D.begin() + blocksize;

				while ( aux < O.D.begin() + O.blocksize )
				{
					uint64_t const length = libmaus2::bambam::BamAlignmentDecoderBase::getAuxLength(aux);

					if ( filter(aux[0],aux[1]) )
					{
						std::copy(aux,aux+length,outp);
						outp += length;
					}

					aux += length;
				}

				blocksize += addlength;
			}


			/**
			 * replace the query sequence and quality string of this alignment block by its reverse complement in place
			 **/
			static void reverseComplementInplace(uint8_t * const D)
                        {
                        	uint64_t const lseq = getLseq(D);
                        	libmaus2::bambam::BamAlignmentDecoderBase::reverseComplementInplace(libmaus2::bambam::BamAlignmentDecoderBase::getSeq(D),lseq);
                        	uint8_t * qual = libmaus2::bambam::BamAlignmentDecoderBase::getQual(D);
                        	std::reverse(qual,qual+lseq);
                        }

			/**
			 * replace the query sequence and quality string of this alignment block by its reverse complement in place
			 **/
			void reverseComplementInplace()
                        {
                        	reverseComplementInplace(D.begin());
                        }

                        /**
                         * check whether this alignment is clipped
                         *
                         * @return true iff cigar string contains clipping operations
                         **/
                         bool isClipped() const
                         {
                         	if ( isUnmap() )
                         		return false;

				// check whether all cigar operators are known
				for ( uint64_t i = 0; i < getNCigar(); ++i )
				{
					uint64_t const cigop = getCigarFieldOp(i);
					if (
						cigop == ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP
						||
						cigop == ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP
					)
						return true;
				}

				return false;
			}

			static bool isMatchOp(BamFlagBase::bam_cigar_ops const o)
			{
				return
					o == BamFlagBase::LIBMAUS2_BAMBAM_CMATCH
					||
					o == BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL
					||
					o == BamFlagBase::LIBMAUS2_BAMBAM_CDIFF;
			}

			uint64_t getRefPosForReadPos(uint64_t const readq) const
			{
				libmaus2::autoarray::AutoArray<cigar_operation> cigop;
				uint32_t const ncig = getCigarOperations(cigop);
				uint64_t refadv = 0;

				uint64_t refpos = getPos();
				uint64_t readpos = 0;

				for ( uint64_t i = 0; i < ncig; ++i )
				{
					if ( isMatchOp(static_cast<BamFlagBase::bam_cigar_ops>(cigop[i].first)) )
						refadv = 1;

					for ( int64_t j = 0; j < cigop[i].second; ++j )
					{
						if ( readpos == readq )
							return refpos;

						switch ( cigop[i].first )
						{
							case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
							case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
							case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
								readpos += 1;
								refpos += refadv;
								break;
							case BamFlagBase::LIBMAUS2_BAMBAM_CINS:
								readpos += 1;
								break;
							case BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
								refpos += refadv;
								break;
							case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
								refpos += refadv;
								break;
							case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
								readpos += 1;
								break;
							case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
							case BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
								break;
						}
					}
				}

				if ( refpos == readq )
					return refpos;
				else
					return std::numeric_limits<uint64_t>::max();
			}

			/**
			 * compute insert size
			 *
			 * @param A first alignment
			 * @param B second alignment
			 * @return note that when storing insert size on the secondEnd, the return value must be negated.
			 */
			static int64_t computeInsertSize(BamAlignment const & A, BamAlignment const & B)
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::computeInsertSize(A.D.begin(),B.D.begin());
			}

			/**
			 * make mate pair information of two alignments consistent (inspired by Picard code)
			 */
			static void fixMateInformation(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				libmaus2::bambam::BamAuxFilterVector const & MQfilter
				)
			{
				rec1.filterOutAux(MQfilter);
				rec2.filterOutAux(MQfilter);
				fixMateInformationPreFiltered(rec1,rec2);
			}

			void putAuxIntegerNumberFast(char const t1, char const t2, int32_t i)
			{
				if ( blocksize + 3 + sizeof(int32_t) > D.size() )
					D.resize(blocksize + 3 + sizeof(int32_t));

				D[blocksize++] = t1;
				D[blocksize++] = t2;
				D[blocksize++] = 'i';

				for ( unsigned int j = 0; j < sizeof(int32_t); ++j, i >>= 8 )
					D[blocksize++] = (i & 0xFF);
			}

			/**
			 * make mate pair information of two alignments consistent (inspired by Picard code)
			 */
			static void fixMateInformationPreFiltered(libmaus2::bambam::BamAlignment & rec1, libmaus2::bambam::BamAlignment & rec2)
			{
				std::pair<int16_t,int16_t> const MQ = libmaus2::bambam::BamAlignmentEncoderBase::fixMateInformation(rec1.D.get(),rec2.D.get());

				if ( MQ.first >= 0 )
					rec1.putAuxIntegerNumberFast('M','Q',MQ.first);
				if ( MQ.second >= 0 )
					rec2.putAuxIntegerNumberFast('M','Q',MQ.second);
			}

			/**
			 * add quality score of mate as aux field
			 */
			static void addMateBaseScore(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				libmaus2::bambam::BamAuxFilterVector const & MSfilter
				)
			{
				uint64_t const score1 = rec1.getScore();
				uint64_t const score2 = rec2.getScore();

				rec1.filterOutAux(MSfilter);
				rec2.filterOutAux(MSfilter);

				rec1.putAuxNumber("ms",'i',score2);
				rec2.putAuxNumber("ms",'i',score1);
			}


			/**
			 * add quality score of mate as aux field
			 */
			static void addMateBaseScorePreFiltered(libmaus2::bambam::BamAlignment & rec1, libmaus2::bambam::BamAlignment & rec2)
			{
				uint64_t const score1 = rec1.getScore();
				uint64_t const score2 = rec2.getScore();

				rec1.putAuxIntegerNumberFast('m', 's', score2);
				rec2.putAuxIntegerNumberFast('m', 's', score1);
			}

			/**
			 * add mapping coordinate of mate as aux field
			 */
			static void addMateCoordinate(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				libmaus2::bambam::BamAuxFilterVector const & MCfilter
				)
			{
				uint64_t const coord1 = rec1.getCoordinate();
				uint64_t const coord2 = rec2.getCoordinate();

				rec1.filterOutAux(MCfilter);
				rec2.filterOutAux(MCfilter);

				rec1.putAuxNumber("mc",'i',coord2);
				rec2.putAuxNumber("mc",'i',coord1);
			}

			/**
			 * add mapping coordinate of mate as aux field
			 */
			static void addMateCoordinatePreFiltered(libmaus2::bambam::BamAlignment & rec1, libmaus2::bambam::BamAlignment & rec2)
			{
				uint64_t const coord1 = rec1.getCoordinate();
				uint64_t const coord2 = rec2.getCoordinate();

				rec1.putAuxIntegerNumberFast('m', 'c', coord2);
				rec2.putAuxIntegerNumberFast('m', 'c', coord1);
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTag(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				libmaus2::bambam::BamAuxFilterVector const & MTfilter,
				char const * tagname
				)
			{
				char const * MT1 = rec1.getAuxString(tagname);
				char const * MT2 = rec2.getAuxString(tagname);

				rec1.filterOutAux(MTfilter);
				rec2.filterOutAux(MTfilter);

				if ( MT2 )
					rec1.putAuxString("mt",MT2);
				if ( MT1 )
					rec2.putAuxString("mt",MT1);
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTagPreFiltered(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				char const * tagname
				)
			{
				char const * MT1 = rec1.getAuxString(tagname);
				char const * MT2 = rec2.getAuxString(tagname);

				if ( MT2 )
					rec1.putAuxString("mt",MT2);
				if ( MT1 )
					rec2.putAuxString("mt",MT1);
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTag(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				libmaus2::bambam::BamAuxFilterVector const & MTfilter,
				std::string const & tagname
				)
			{
				addMateTag(rec1,rec2,MTfilter,tagname.c_str());
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTagPreFiltered(
				libmaus2::bambam::BamAlignment & rec1,
				libmaus2::bambam::BamAlignment & rec2,
				std::string const & tagname
				)
			{
				addMateTagPreFiltered(rec1,rec2,tagname.c_str());
			}

			/**
			 * @return true if read fragment contains any non A,C,G or T
			 **/
			bool hasNonACGT() const
			{
				return BamAlignmentDecoderBase::hasNonACGT(D.begin());
			}

			void fillMd(::libmaus2::bambam::MdStringComputationContext & context)
			{
				if ( context.diff )
				{
					if ( context.eraseold )
						filterOutAux(context.auxvec);
					putAuxString("MD",context.md.get());
					putAuxNumber("NM",'i',context.nm);
				}
			}

			/**
			 * calculate MD and NM fields
			 *
			 * @param context temporary space and result storage
			 * @param pointer to reference at position of first non clipping op
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			void calculateMd(
				::libmaus2::bambam::MdStringComputationContext & context,
				it_a itref,
				bool const warnchanges = true
			)
			{
				libmaus2::bambam::BamAlignmentDecoderBase::calculateMd(D.begin(),blocksize,context,itref,warnchanges);
				fillMd(context);
			}

			/**
			 * get soft clipped bases at beginning of read
			 *
			 * @return soft clipped bases at beginning of read
			 **/
			uint64_t getFrontSoftClipping() const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getFrontSoftClipping(D.begin());
			}
			/**
			 * get soft clipped bases at end of read
			 *
			 * @return soft clipped bases at end of read
			 **/
			uint64_t getBackSoftClipping() const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getBackSoftClipping(D.begin());
			}

			/**
			 * @return clipped query sequence as string
			 **/
			std::string getClippedRead() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeRead(A);
				return std::string(
					A.begin() + getFrontSoftClipping(),
					A.begin() + len - getBackSoftClipping()
				);
			}

			/**
			 * @return front clipped bases
			 **/
			std::string getFrontClippedBases() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				decodeRead(A);
				return std::string(
					A.begin(),
					A.begin() + getFrontSoftClipping()
				);
			}

			/**
			 * @return back clipped bases
			 **/
			std::string getBackClippedBases() const
			{
				::libmaus2::autoarray::AutoArray<char> A;
				uint64_t const len = decodeRead(A);
				return std::string(
					A.begin() + len - getBackSoftClipping(),
					A.begin() + len
				);
			}

			/**
			 * @return number of deleted bases before first match
			 **/
			uint64_t getFrontDel() const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getFrontDel(D.begin());
			}

			/**
			 * add mate cigar string aux field
			 *
			 * @param A first read alignment block
			 * @param B second read alignment block
			 * @param C aux array
			 * @param MCfilter aux filter for removing any previously existing MC aux fields
			 **/
			static void addMateCigarString(
				libmaus2::bambam::BamAlignment & A,
				libmaus2::bambam::BamAlignment & B,
				libmaus2::autoarray::AutoArray<char> & C,
				libmaus2::bambam::BamAuxFilterVector const & MCfilter
			)
			{
				A.filterOutAux(MCfilter);
				B.filterOutAux(MCfilter);

				B.getCigarString(C);
				A.putAuxString("MC", C.begin());

				A.getCigarString(C);
				B.putAuxString("MC", C.begin());
			}

			int64_t getNextCoordinate(libmaus2::autoarray::AutoArray<cigar_operation> & Aop) const
			{
				if ( isMateUnmap() )
					return -1;
				else
					return libmaus2::bambam::BamAlignmentDecoderBase::getNextCoordinate(D.begin(),blocksize,Aop);
			}

			int64_t getNextCoordinate(libmaus2::autoarray::AutoArray<cigar_operation> & Aop, size_t const numcigop) const
			{
				if ( isMateUnmap() )
					return -1;
				else
					return libmaus2::bambam::BamAlignmentDecoderBase::getNextCoordinate(D.begin(),Aop.begin(),Aop.begin()+numcigop);
			}

			size_t getNextCigarVector(libmaus2::autoarray::AutoArray<cigar_operation> & Aop) const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getNextCigarVector(D.begin(),blocksize,Aop);
			}

			int64_t getNextUnclippedStart(libmaus2::autoarray::AutoArray<cigar_operation> & Aop, size_t const numcigop) const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedStart(D.begin(),Aop.begin(),Aop.begin()+numcigop);
			}

			int64_t getNextUnclippedEnd(libmaus2::autoarray::AutoArray<cigar_operation> & Aop, size_t const numcigop) const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedEnd(D.begin(),Aop.begin(),Aop.begin()+numcigop);
			}

			void getCigarStats(libmaus2::autoarray::AutoArray<uint64_t> & A, bool const erase = true) const
			{
				libmaus2::bambam::BamAlignmentDecoderBase::getCigarStats(D.begin(),A,erase);
			}

			std::vector< PileVectorElement > getPileVector(libmaus2::autoarray::AutoArray<cigar_operation> & cigopin, libmaus2::autoarray::AutoArray<char> & readdata, uint64_t const readid = 0) const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getPileVector(D.begin(),cigopin,readdata,readid);
			}

			std::vector< PileVectorElement > getPileVector(uint64_t const readid = 0) const
			{
				libmaus2::autoarray::AutoArray<cigar_operation> cigopin;
				libmaus2::autoarray::AutoArray<char> readdata;
				return getPileVector(cigopin,readdata,readid);
			}

			void fillCigarHistogram(uint64_t H[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES]) const
			{
				libmaus2::bambam::BamAlignmentDecoderBase::fillCigarHistogram(D.begin(),H);
			}

			std::pair<uint64_t,uint64_t> getErrorRatePair() const
			{
				uint64_t H[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES];
				std::fill(&H[0],&H[sizeof(H)/sizeof(H[0])],0ull);
				fillCigarHistogram(H);

				if ( H [ BamFlagBase::LIBMAUS2_BAMBAM_CMATCH ] )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignment::getErrorRate(): cannot compute error rate in presence of M cigar operator (replace it by = and X)" << std::endl;
					lme.finish();
					throw lme;
				}

				uint64_t const ins = H[BamFlagBase::LIBMAUS2_BAMBAM_CINS];
				uint64_t const del = H[BamFlagBase::LIBMAUS2_BAMBAM_CINS];
				uint64_t const dif = H[BamFlagBase::LIBMAUS2_BAMBAM_CDIFF];
				uint64_t const eq = H[BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL];

				uint64_t const totalops = ins + del + dif + eq;
				uint64_t const errops = ins + del + dif;

				return std::pair<uint64_t,uint64_t>(errops,totalops);
			}

			double getErrorRate() const
			{
				std::pair<uint64_t,uint64_t> const P = getErrorRatePair();
				return static_cast<double>(P.first) / static_cast<double>(P.second);
			}

			libmaus2::math::IntegerInterval<int64_t> getCoveredReadInterval() const
			{
				return libmaus2::bambam::BamAlignmentDecoderBase::getCoveredReadInterval(D.begin());
			}
		};
	}
}
#endif
