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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENT_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENT_HPP

#include <libmaus/bambam/AlignmentValidity.hpp>
#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/bambam/BamAlignmentEncoderBase.hpp>
#include <libmaus/bambam/BamHeader.hpp>
#include <libmaus/fastx/FASTQEntry.hpp>
#include <libmaus/hashing/hash.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/bitio/BitVector.hpp>

namespace libmaus
{
	namespace bambam
	{
		#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
		#pragma pack(push,1)
		/**
		 * quick bam alignment base block access structure for little endian machines
		 **/
		struct BamAlignmentFixedSizeData
		{
			//! reference id
			int32_t  RefID;
			//! position on reference id
		        int32_t  Pos;
		        //! name length
		        uint8_t  NL;
		        //! mapping quality
		        uint8_t  MQ;
		        //! bin
		        uint16_t Bin;
		        //! number of cigar operations
		        uint16_t NC;
		        //! flags
		        uint16_t Flags;
		        // length of sequence
		        int32_t  Lseq;
		        //! mate/next segment reference id
		        int32_t  NextRefID;
		        //! mate/next segment position on reference id
		        int32_t  NextPos;
		        //! template length
		        int32_t  Tlen;
		};
		#pragma pack(pop)
		#endif

		/**
		 * bam alignment
		 **/
		struct BamAlignment
		{
			//! this type
			typedef BamAlignment this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			//! memory allocation type of data block
			static const ::libmaus::autoarray::alloc_type D_array_alloc_type = ::libmaus::autoarray::alloc_type_memalign_cacheline;
			//! D array type
			typedef ::libmaus::autoarray::AutoArray<uint8_t,D_array_alloc_type> D_array_type;
		
			//! data array
			D_array_type D;
			//! number of bytes valid in D
			uint64_t blocksize;
			
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
				blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(in,4);
				// std::cerr << "Block size " << blocksize << std::endl;
				D = D_array_type(blocksize,false);
				in.read(reinterpret_cast<char *>(D.begin()),blocksize);
				
				if ( static_cast<int64_t>(in.gcount()) != static_cast<int64_t>(blocksize) )
				{
					::libmaus::exception::LibMausException se;
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
				libmaus_bambam_alignment_validity const validity = valid();	
				if ( validity != libmaus_bambam_alignment_validity_ok )
				{
					::libmaus::exception::LibMausException se;
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
			libmaus_bambam_alignment_validity valid() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::valid(D.begin(),blocksize);
			}

			/**
			 * check alignment validity including reference sequence ids
			 *
			 * @param header bam header
			 * @return alignment validty code
			 **/
			template<typename header_type>
			libmaus_bambam_alignment_validity valid(header_type const & header) const
			{
				libmaus_bambam_alignment_validity validity = valid();
				
				if ( validity != libmaus_bambam_alignment_validity_ok )
					return validity;
				
				int32_t const refseq = getRefID();
				if ( !((refseq == -1) || refseq < static_cast<int64_t>(header.getNumRef())) )
					return libmaus_bambam_alignment_validity_invalid_refseq;

				int32_t const nextrefseq = getNextRefID();
				if ( !((nextrefseq == -1) || nextrefseq < static_cast<int64_t>(header.getNumRef())) )
					return libmaus_bambam_alignment_validity_invalid_next_refseq;
				
				return libmaus_bambam_alignment_validity_ok;
			}

			/**
			 * @return number of bytes before the cigar string in the alignment
			 **/
			uint64_t getNumPreCigarBytes() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumPreCigarBytes(D.begin());
			}
			
			/**
			 * @return number of bytes in the cigar string representation of the alignment
			 **/
			uint64_t getNumCigarBytes() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumCigarBytes(D.begin());
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumPreNameBytes(D.begin());
			}

			/**
			 * @return number of bytes in the name
			 **/
			uint64_t getNumNameBytes() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumNameBytes(D.begin());
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
			uint32_t getCigarOperations(libmaus::autoarray::AutoArray<cigar_operation> & cigop) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarOperations(D.begin(),cigop);
			}

			/**
			 * @return number of bytes before the query sequence in the alignment
			 **/
			uint64_t getNumPreSeqBytes() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumPreSeqBytes(D.begin());
			}
			
			/**
			 * @return number of bytes representing the query sequence in the alignment
			 **/
			uint64_t getNumSeqBytes() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNumSeqBytes(D.begin());
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
				std::vector<cigar_operation> cigar = ::libmaus::bambam::CigarStringParser::parseCigarString(cigarstring);
				libmaus::bambam::BamAlignment::D_array_type T;
				replaceCigarString(cigar.begin(),cigar.size(),T);
			}

			/**
			 * replace cigar string in the alignment by the given string
			 *
			 * @param cigarstring replacement cigarstring
			 * @param T temporary array
			 **/
			void replaceCigarString(std::string const & cigarstring, libmaus::bambam::BamAlignment::D_array_type & T)
			{
				std::vector<cigar_operation> cigar = ::libmaus::bambam::CigarStringParser::parseCigarString(cigarstring);
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
				libmaus::autoarray::AutoArray<uint8_t,D_array_alloc_type> T;
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
				libmaus::autoarray::AutoArray<uint8_t,D_array_alloc_type> & T
			)
			{
				uint64_t const oldlen = getLseq();
				uint64_t const pre    = getNumPreSeqBytes();
				uint64_t const oldseq = getNumSeqBytes();
				// uint64_t const newseq = (seqlen+1)/2;
				uint64_t const post   = getNumPostSeqBytes();
				
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( T, 0 /* pre + newseq + post */ );
				
				// pre seq data
				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				// seq data
				::libmaus::bambam::BamAlignmentEncoderBase::encodeSeq(buffer,seqenc,seq,seqlen);
				
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
				libmaus::bambam::BamAlignment::D_array_type & T
			)
			{
				uint64_t const pre    = getNumPreCigarBytes();
				uint64_t const oldcig = getNumCigarBytes();
				uint64_t const post   = getNumPostCigarBytes();
				
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( T,0 /* getNumPreCigarBytes() + cigarlen * sizeof(uint32_t) + getNumPostCigarBytes() */ );
				
				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				::libmaus::bambam::BamAlignmentEncoderBase::encodeCigar(buffer,cigar,cigarlen);

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
				uint64_t const pre    = ::libmaus::bambam::BamAlignmentDecoderBase::getNumPreCigarBytes(D);
				uint64_t const oldcig = ::libmaus::bambam::BamAlignmentDecoderBase::getNumCigarBytes(D);
				uint64_t const post   = ::libmaus::bambam::BamAlignmentDecoderBase::getNumPostCigarBytes(D,blocksize);
				
				// move post cigar string bytes to position of old cigar string
				memmove(
					D + pre, // dest
					D + pre + oldcig,  // src
					post // n
				);
				
				blocksize -= oldcig;
				
				::libmaus::bambam::BamAlignmentEncoderBase::putCigarLen(D,0);
				
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
				::libmaus::bambam::EncoderBase::putLE<output_type,uint32_t>(out,blocksize);
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
				return ::libmaus::hashing::EvaHash::hash64(reinterpret_cast<uint8_t const *>(getName()),
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAuxAsString(D.get(),blocksize,tag);
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag);
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAuxAsNumber<N>(D.get(),blocksize,tag,num);
			}

			/**
			 * check if auxiliary field for tag is present
			 *
			 * @param tag two letter auxiliary tag identifier
			 * @return true iff tag is present
			 **/
			bool hasAux(char const * const tag) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,tag) != 0;
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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber(data,tag,type,v);

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumber(data,tag,type,v);

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxString(data,tag,value.c_str());

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);

				::libmaus::bambam::BamAlignmentEncoderBase::putAuxString(data,tag,value);

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',V);

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',values,n);

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
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> data(D,blocksize);
				
				::libmaus::bambam::BamAlignmentEncoderBase::putAuxNumberArray(data,tag,'C',values,n);

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
				::libmaus::util::UTF8::encodeUTF8(num,PBPO);				
			}
	
			/**
			 * put rank value as auxiliary tag contaning a number sequence (eight byte big endian)
			 *
			 * @param tag aux field id
			 * @param rank field content
			 **/
			void putRank(std::string const & tag, uint64_t const rank /*, ::libmaus::bambam::BamHeader const & bamheader */)
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
					::libmaus::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,tag);
				
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::hash32(D.begin());
			}

			/**
			 * @return length of query sequence as encoded in the cigar string
			 **/
			uint64_t getLseqByCigar() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getLseqByCigar(D.begin());
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAuxString(D.begin(),blocksize,"RG");
			}

			/**
			 * @param tag aux field tag
			 * @return contents of the aux field (null pointer if not present)
			 **/
			char const * getAuxString(char const * tagname) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAuxString(D.begin(),blocksize,tagname);
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
			 * @param bamheader BAM header object
			 * @param auxiliary formatting auxiliary object
			 * @return SAM file line as string
			 **/
			std::string formatAlignment(
				::libmaus::bambam::BamHeader const & bamheader,
				::libmaus::bambam::BamFormatAuxiliary & auxiliary
			) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::formatAlignment(
					D.get(),blocksize,bamheader,auxiliary);
			}
			
			/**
			 * format alignment as SAM file line
			 *
			 * @param bamheader BAM header object
			 * @return SAM file line as string
			 **/
			std::string formatAlignment(::libmaus::bambam::BamHeader const & bamheader) const
			{
				::libmaus::bambam::BamFormatAuxiliary auxiliary;
				return formatAlignment(bamheader,auxiliary);
			}
			
			/**
			 * format alignment as FastQ
			 *
			 * @param auxiliary formatting auxiliary object
			 * @return FastQ entry as string
			 **/
			std::string formatFastq(::libmaus::bambam::BamFormatAuxiliary & auxiliary) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::formatFastq(D.get(),auxiliary);
			}
			
			/**
			 * format alignment as FastQ
			 *
			 * @return FastQ entry as string
			 **/
			std::string formatFastq() const
			{
				::libmaus::bambam::BamFormatAuxiliary auxiliary;
				return formatFastq(auxiliary);				
			}
			
			/**
			 * @return query name
			 **/
			char const * getName() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(D.get());
			}
			
			/**
			 * @return reference id
			 **/
			int32_t getRefID() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->RefID;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(D.get());				
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
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Pos;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getPos(D.get());
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
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NextRefID;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNextRefID(D.get());
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
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NextPos;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNextPos(D.get());
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAlignmentEnd(D.get());
			}
			
			/**
			 * @return bin field
			 **/
			uint32_t getBin() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Bin;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getBin(D.get());
				#endif
			}
			
			/**
			 * @return mapping quality field
			 **/
			uint32_t getMapQ() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->MQ;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getMapQ(D.get());
				#endif
			}
			
			/**
			 * @return length of read name
			 **/
			uint32_t getLReadName() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NL;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getLReadName(D.get());
				#endif
			}
			
			/**
			 * @return flags field
			 **/
			uint32_t getFlags() const
			{			
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Flags;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(D.get());
				#endif
			}
			
			/**
			 * @return number of cigar operations field
			 **/
			uint32_t getNCigar() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->NC;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getNCigar(D.get());			
				#endif
			}
			
			/**
			 * @return cigar string
			 **/
			std::string getCigarString() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarString(D.get());
			}
			
			/**
			 * @return string representation of flags
			 **/
			std::string getFlagsS() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getFlagsS(D.get());			
			}
			
			/**
			 * return the i'th cigar operation as character (as in SAM)
			 *
			 * @param i cigar operator index (0 based)
			 * @return the i'th cigar operation as character
			 **/
			char getCigarFieldOpAsChar(uint64_t const i) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarFieldOpAsChar(D.get(),i);
			}
			
			/**
			 * return the i'th cigar operator as integer (see bam_cigar_ops enumeration)
			 *
			 * @param i cigar operator index (0 based)
			 * @return the i'th cigar operation as integer
			 **/
			uint32_t getCigarFieldOp(uint64_t const i) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarFieldOp(D.get(),i);
			}
			/**
			 * get length of i'th cigar operation
			 *
			 * @param i cigar operator index (0 based)
			 * @return length of i'th cigar operation
			 **/
			uint32_t getCigarFieldLength(uint64_t const i) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarFieldLength(D.get(),i);
			}
			
			/**
			 * get i'th cigar operation field (in BAM encoding)
			 *
			 * @param i cigar operator index (0 based)
			 * @return i'th cigar operation field
			 **/
			uint32_t getCigarField(uint64_t const i) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getCigarField(D.get(),i);
			}
			
			/**
			 * @return template length field
			 **/
			int32_t getTlen() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Tlen;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getTlen(D.get());
				#endif
			}
			
			static int32_t getLseq(uint8_t const * D)
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D)->Lseq;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getLseq(D);
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
			uint64_t decodeRead(::libmaus::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeRead(D.get(),A);
			}
			/**
			 * decode reverse complement of query sequence to array A, which is extended if necessary
			 *
			 * @param A reference to array
			 * @return length of sequence
			 **/
			uint64_t decodeReadRC(::libmaus::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeReadRC(D.get(),A);
			}
			/**
			 * decode quality string
			 *
			 * @param A reference to array
			 * @return length of quality string
			 **/
			uint64_t decodeQual(::libmaus::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeQual(D.get(),A);
			}
			/**
			 * decode reverse of quality string
			 *
			 * @param A reference to array
			 * @return length of quality string
			 **/
			uint64_t decodeQualRC(::libmaus::autoarray::AutoArray<char> & A) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeQualRC(D.get(),A);
			}
			
			/**
			 * @return computed bin
			 **/
			uint64_t computeBin() const
			{
				uint64_t const rbeg = getPos();
				uint64_t const rend = rbeg + getReferenceLength();
				uint64_t const bin = libmaus::bambam::BamAlignmentEncoderBase::reg2bin(rbeg,rend);
				return bin;
			}
			
			/**
			 * @return length of alignment on reference
			 **/
			uint64_t getReferenceLength() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getReferenceLength(D.get());				
			}
			
			/**
			 * @return query sequence as string
			 **/
			std::string getRead() const
			{
				::libmaus::autoarray::AutoArray<char> A;
				uint64_t const len = decodeRead(A);
				return std::string(A.begin(),A.begin()+len);
			}

			/**
			 * @return reverse complement of query sequence as string
			 **/
			std::string getReadRC() const
			{
				::libmaus::autoarray::AutoArray<char> A;
				uint64_t const len = decodeReadRC(A);
				return std::string(A.begin(),A.begin()+len);
			}
			
			/**
			 * @return quality string as string
			 **/
			std::string getQual() const
			{
				::libmaus::autoarray::AutoArray<char> A;
				uint64_t const len = decodeQual(A);
				return std::string(A.begin(),A.begin()+len);
			}
			/**
			 * @return reverse quality string as string
			 **/
			std::string getQualRC() const
			{
				::libmaus::autoarray::AutoArray<char> A;
				uint64_t const len = decodeQualRC(A);
				return std::string(A.begin(),A.begin()+len);
			}
			
			/**
			 * @return iff alignment has the pair flag set
			 **/
			bool isPaired() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isPaired(getFlags()); }
			/**
			 * @return iff alignment has the proper pair flag set
			 **/
			bool isProper() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isProper(getFlags()); }
			/**
			 * @return iff alignment has the unmapped flag set
			 **/
			bool isUnmap() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isUnmap(getFlags()); }
			/**
			 * @return iff alignment does not have the unmapped flag set
			 **/
			bool isMapped() const { return !isUnmap(); }
			/**
			 * @return iff alignment has the mate unmapped flag set
			 **/
			bool isMateUnmap() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isMateUnmap(getFlags()); }
			/**
			 * @return iff alignment has the mate unmapped flag not set
			 **/
			bool isMateMapped() const { return !isMateUnmap(); }
			/**
			 * @return iff alignment has the reverse strand flag set
			 **/
			bool isReverse() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isReverse(getFlags()); }
			/**
			 * @return iff alignment has the mate reverse strand flag set
			 **/
			bool isMateReverse() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isMateReverse(getFlags()); }
			/**
			 * @return iff alignment has read 1 flag set
			 **/
			bool isRead1() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isRead1(getFlags()); }
			/**
			 * @return iff alignment has read 2 flag set
			 **/
			bool isRead2() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isRead2(getFlags()); }
			/**
			 * @return iff alignment has the secondary alignment flag set
			 **/
			bool isSecondary() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isSecondary(getFlags()); }
			/**
			 * @return iff alignment has the supplementary alignment flag set
			 **/
			bool isSupplementary() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isSupplementary(getFlags()); }
			/**
			 * @return iff alignment has the quality control failed flag set
			 **/
			bool isQCFail() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isQCFail(getFlags()); }
			/**
			 * @return iff alignment has the duplicate entry flag set
			 **/
			bool isDup() const { return ::libmaus::bambam::BamAlignmentDecoderBase::isDup(getFlags()); }
			
			/**
			 * @return coordinate of the leftmost query sequence base (as opposed to the coordinate of the leftmost aligned query sequence base)
			 **/
			int64_t getCoordinate() const 
			{ 
				if ( !isUnmap() )
					return ::libmaus::bambam::BamAlignmentDecoderBase::getCoordinate(D.get()); 
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
					return ::libmaus::bambam::BamAlignmentDecoderBase::getOffsetBeforeMatch(D.get()); 
				else
					return 0;
			}
			
			/**
			 * get quality value
			 * 
			 * @return sum of quality score values of at least 15
			 **/
			uint64_t getScore() const { return ::libmaus::bambam::BamAlignmentDecoderBase::getScore(D.get()); }
			
			/**
			 * format alignment to FASTQEntry object
			 *
			 * @param pattern fastq object
			 * @param patid pattern id
			 **/
			void toPattern(::libmaus::fastx::FASTQEntry & pattern, uint64_t const patid) const
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
			void putRefId(int32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putRefId(D.get(),v); }
			/**
			 * replace position by v
			 *
			 * @param v new position
			 **/
			void putPos(int32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putPos(D.get(),v); }
			/**
			 * replace mate reference id by v
			 *
			 * @param v new mate reference id
			 **/
			void putNextRefId(int32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putNextRefId(D.get(),v); }
			/**
			 * replace mate position by v
			 *
			 * @param v new mate position
			 **/
			void putNextPos(int32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putNextPos(D.get(),v); }
			/**
			 * replace template length by v
			 *
			 * @param v new template length
			 **/
			void putTlen(int32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putTlen(D.get(),v); }
			/**
			 * replace flags by v
			 *
			 * @param v new flags
			 **/
			void putFlags(uint32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putFlags(D.get(),v); }
			/**
			 * replace flags by v
			 *
			 * @param v new flags
			 **/
			void putMapQ(uint8_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putMapQ(D.get(),v); }
			/**
			 * replace number of cigar operations by v
			 *
			 * @param v new number of cigar operations
			 **/
			void putCigarLen(uint32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putCigarLen(D.get(),v); }
			/**
			 * replace sequence length field by v
			 *
			 * @param v new sequence length field
			 **/			
			void putSeqLen(uint32_t const v) { ::libmaus::bambam::BamAlignmentEncoderBase::putSeqLen(D.get(),v); }

			/**
			 * format alignment as fastq
			 *
			 * @param it output iterator
			 * @return output iterator after formatting sequence
			 **/
			template<typename iterator>
			iterator putFastQ(iterator it) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::putFastQ(D.get(),it);
			}
			
			/**
			 * @return length of alignment formatted as fastq entry
			 **/
			uint64_t getFastQLength() const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getFastQLength(D.get());
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeRead(D.get(),S,seqlen);
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
				return ::libmaus::bambam::BamAlignmentDecoderBase::decodeReadRCIt(D.get(),S,seqlen);
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
                        	return ::libmaus::bambam::BamAlignmentDecoderBase::decodeQualIt(D.get(),it,seqlen);
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
                        	return ::libmaus::bambam::BamAlignmentDecoderBase::decodeQualRcIt(D.get(),it,seqlen);
                        }

			/**
			 * filter auxiliary tags keeping only those in a given list
			 *
			 * @param tags list of tag identifiers to be kept
			 **/
			void filterAux(BamAuxFilterVector const & tags)
			{
				blocksize = ::libmaus::bambam::BamAlignmentDecoderBase::filterAux(D.begin(),blocksize,tags);
			}
			
			/**
			 * remove all auxiliary fields
			 *
			 * @param D alignment block data
			 * @return new size of block
			 **/
			static uint64_t eraseAux(uint8_t const * const D)
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::getAux(D)-D;
			}

			/**
			 * remove all auxiliary fields
			 **/
			void eraseAux()
			{
				blocksize = eraseAux(D.begin());
			}

			/**
			 * filter auxiliary tags keeping and remove those in the list
			 *
			 * @param tags list of tag identifiers to be removed
			 **/
			void filterOutAux(BamAuxFilterVector const & tags)
			{
				blocksize = ::libmaus::bambam::BamAlignmentDecoderBase::filterOutAux(D.begin(),blocksize,tags);
			}

			/**
			 * sort aux fields by tag id
			 *
			 * @param sortbuffer buffer for sorting
			 **/
			void sortAux(BamAuxSortingBuffer & sortbuffer)
			{
				::libmaus::bambam::BamAlignmentDecoderBase::sortAux(D.begin(),blocksize,sortbuffer);
			}                        

			/**
			 * enumerate aux tags in array A; A will be resized if needed
			 *
			 * @param A array for storing aux tag markers
			 * @return number of markers stored
			 **/
			uint64_t enumerateAuxTags(libmaus::autoarray::AutoArray < std::pair<uint8_t,uint8_t> > & A) const
			{
				return ::libmaus::bambam::BamAlignmentDecoderBase::enumerateAuxTags(D.begin(),blocksize,A);
			}
			
			/**
			 * copy aux tags from one alignment to another
			 *
			 * @param O source alignment
			 * @param filter tag set to be copied
			 **/
			void copyAuxTags(libmaus::bambam::BamAlignment const & O, BamAuxFilterVector const & filter)
			{
				/*
				 * compute number of bytes we will add to this alignment block
				 */
				uint8_t const * aux = libmaus::bambam::BamAlignmentDecoderBase::getAux(O.D.begin());
				uint64_t addlength = 0;
				
				while ( aux < O.D.begin() + O.blocksize )
				{
					uint64_t const length = libmaus::bambam::BamAlignmentDecoderBase::getAuxLength(aux);
					
					if ( filter(aux[0],aux[1]) )
						addlength += length;
					
					aux += length;	
				}
				
				// resize block array if necessary
				if ( blocksize + addlength > D.size() )
					D.resize(blocksize + addlength);
				
				// copy aux fields
				aux = libmaus::bambam::BamAlignmentDecoderBase::getAux(O.D.begin());
				uint8_t * outp = D.begin() + blocksize;

				while ( aux < O.D.begin() + O.blocksize )
				{
					uint64_t const length = libmaus::bambam::BamAlignmentDecoderBase::getAuxLength(aux);
					
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
                        	libmaus::bambam::BamAlignmentDecoderBase::reverseComplementInplace(libmaus::bambam::BamAlignmentDecoderBase::getSeq(D),lseq);
                        	uint8_t * qual = libmaus::bambam::BamAlignmentDecoderBase::getQual(D);
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
						cigop == ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP 
						||
						cigop == ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CHARD_CLIP 
					)
						return true;
				}
				
				return false;
			}

			/**
			 * compute insert size (inspired by Picard code)
			 *
			 * @param A first alignment
			 * @param B second alignment
			 * @return note that when storing insert size on the secondEnd, the return value must be negated.
			 */
			static int64_t computeInsertSize(BamAlignment const & A, BamAlignment const & B)
			{
				// unmapped end?
				if (A.isUnmap() || B.isUnmap()) { return 0; }
				// different ref seq?
				if (A.getRefID() != B.getRefID()) { return 0; }

				// compute 5' end positions
				int64_t const A5  = A.isReverse() ? A.getAlignmentEnd() : A.getPos();
				int64_t const B5  = B.isReverse() ? B.getAlignmentEnd() : B.getPos();
				
				// return insert size for (A,B), use negative value for (B,A)
				return B5 - A5;
			}
			
			/**
			 * make mate pair information of two alignments consistent (inspired by Picard code)
			 */
			static void fixMateInformation(
				libmaus::bambam::BamAlignment & rec1, 
				libmaus::bambam::BamAlignment & rec2,
				libmaus::bambam::BamAuxFilterVector const & MQfilter
				)
			{
				static uint32_t const next_rev_flag = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FMREVERSE;
				static uint32_t const next_unmap_flag = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FMUNMAP;
				
				// both mapped
				if (!rec1.isUnmap() && !rec2.isUnmap()) 
				{
					rec1.putNextRefId(rec2.getRefID());
					rec1.putNextPos(rec2.getPos());
					rec1.putFlags( (rec2.isReverse() ? (rec1.getFlags() | next_rev_flag) : (rec1.getFlags() & (~next_rev_flag))) & (~next_unmap_flag) );

					rec2.putNextRefId(rec1.getRefID());
					rec2.putNextPos(rec1.getPos());
					rec2.putFlags( (rec1.isReverse() ? (rec2.getFlags() | next_rev_flag) : (rec2.getFlags() & (~next_rev_flag))) & (~next_unmap_flag) );

					rec1.filterOutAux(MQfilter);
					rec2.filterOutAux(MQfilter);
					rec1.putAuxNumber("MQ", 'i', rec2.getMapQ());
					rec2.putAuxNumber("MQ", 'i', rec1.getMapQ());

					int64_t const insertSize = computeInsertSize(rec1, rec2);
					rec1.putTlen(insertSize);
					rec2.putTlen(-insertSize);
				}
				// both unmapped
				else if (rec1.isUnmap() && rec2.isUnmap())
				{
					rec1.putRefId(-1);
					rec1.putPos(-1);
					rec1.putNextRefId(-1);
					rec1.putNextPos(-1);
					rec1.putFlags( (rec2.isReverse() ? (rec1.getFlags() | next_rev_flag) : (rec1.getFlags() & (~next_rev_flag))) | (next_unmap_flag) );
					rec1.putTlen(0);

					rec2.putRefId(-1);
					rec2.putPos(-1);
					rec2.putNextRefId(-1);
					rec2.putNextPos(-1);
					rec2.putFlags( (rec1.isReverse() ? (rec2.getFlags() | next_rev_flag) : (rec2.getFlags() & (~next_rev_flag))) | (next_unmap_flag) );
					rec2.putTlen(0);
					
					rec1.filterOutAux(MQfilter);
					rec2.filterOutAux(MQfilter);
				}
				// one mapped and other one unmapped
				else
				{
					libmaus::bambam::BamAlignment & mapped   = rec1.isUnmap() ? rec2 : rec1;
					libmaus::bambam::BamAlignment & unmapped = rec1.isUnmap() ? rec1 : rec2;
					
					unmapped.putRefId(mapped.getRefID());
					unmapped.putPos(mapped.getPos());

					mapped.putNextRefId(unmapped.getRefID());
					mapped.putNextPos(unmapped.getPos());
					mapped.putFlags( (unmapped.isReverse() ? (mapped.getFlags() | next_rev_flag) : (mapped.getFlags() & (~next_rev_flag))) | (next_unmap_flag) );					
					mapped.putTlen(0);

					unmapped.putNextRefId(mapped.getRefID());
					unmapped.putNextPos(mapped.getPos());
					unmapped.putFlags( (mapped.isReverse() ? (unmapped.getFlags() | next_rev_flag) : (unmapped.getFlags() & (~next_rev_flag))) & (~next_unmap_flag) );
					unmapped.putTlen(0);

					mapped.filterOutAux(MQfilter);
					unmapped.filterOutAux(MQfilter);
					
					unmapped.putAuxNumber("MQ", 'i', mapped.getMapQ());
				}
			}

			/**
			 * add quality score of mate as aux field
			 */
			static void addMateBaseScore(
				libmaus::bambam::BamAlignment & rec1, 
				libmaus::bambam::BamAlignment & rec2,
				libmaus::bambam::BamAuxFilterVector const & MSfilter
				)
			{
				uint64_t const score1 = rec1.getScore();
				uint64_t const score2 = rec2.getScore();

				rec1.filterOutAux(MSfilter);
				rec2.filterOutAux(MSfilter);
				
				rec1.putAuxNumber("MS",'i',score2);
				rec2.putAuxNumber("MS",'i',score1);								
			}

			/**
			 * add mapping coordinate of mate as aux field
			 */
			static void addMateCoordinate(
				libmaus::bambam::BamAlignment & rec1, 
				libmaus::bambam::BamAlignment & rec2,
				libmaus::bambam::BamAuxFilterVector const & MCfilter
				)
			{
				uint64_t const coord1 = rec1.getCoordinate();
				uint64_t const coord2 = rec2.getCoordinate();

				rec1.filterOutAux(MCfilter);
				rec2.filterOutAux(MCfilter);
				
				rec1.putAuxNumber("MC",'i',coord2);
				rec2.putAuxNumber("MC",'i',coord1);								
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTag(
				libmaus::bambam::BamAlignment & rec1, 
				libmaus::bambam::BamAlignment & rec2,
				libmaus::bambam::BamAuxFilterVector const & MTfilter,
				char const * tagname
				)
			{
				char const * MT1 = rec1.getAuxString(tagname);
				char const * MT2 = rec2.getAuxString(tagname);

				rec1.filterOutAux(MTfilter);
				rec2.filterOutAux(MTfilter);
				
				if ( MT2 )
					rec1.putAuxString("MT",MT2);
				if ( MT1 )
					rec2.putAuxString("MT",MT1);								
			}

			/**
			 * add tag of mate as aux field if present
			 */
			static void addMateTag(
				libmaus::bambam::BamAlignment & rec1, 
				libmaus::bambam::BamAlignment & rec2,
				libmaus::bambam::BamAuxFilterVector const & MTfilter,
				std::string const & tagname
				)
			{
				addMateTag(rec1,rec2,MTfilter,tagname.c_str());
			}

			/**
			 * @return true if read fragment contains any non A,C,G or T
			 **/
			bool hasNonACGT() const
			{
				return BamAlignmentDecoderBase::hasNonACGT(D.begin());
			}
			
			void fillMd(::libmaus::bambam::MdStringComputationContext & context)
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
			 * @param pointer to reference at position of first match/mismatch
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			void calculateMd(
				::libmaus::bambam::MdStringComputationContext & context,
				it_a itref,
				bool const warnchanges = true
			)
			{	
				libmaus::bambam::BamAlignmentDecoderBase::calculateMd(D.begin(),blocksize,context,itref,warnchanges);
				fillMd(context);	
			}
			
			/**
			 * get soft clipped bases at beginning of read
			 *
			 * @return soft clipped bases at beginning of read
			 **/
			uint64_t getFrontSoftClipping() const
			{
				return libmaus::bambam::BamAlignmentDecoderBase::getFrontSoftClipping(D.begin());
			}
			/**
			 * get soft clipped bases at end of read
			 *
			 * @return soft clipped bases at end of read
			 **/
			uint64_t getBackSoftClipping() const
			{
				return libmaus::bambam::BamAlignmentDecoderBase::getBackSoftClipping(D.begin());
			}

			/**
			 * @return clipped query sequence as string
			 **/
			std::string getClippedRead() const
			{
				::libmaus::autoarray::AutoArray<char> A;
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
				::libmaus::autoarray::AutoArray<char> A;
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
				::libmaus::autoarray::AutoArray<char> A;
				uint64_t const len = decodeRead(A);
				return std::string(
					A.begin() + len - getBackSoftClipping(),
					A.begin() + len
				);
			}
		};
	}
}
#endif
