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

			//! valid query name symbols table
			static uint8_t const qnameValidTable[256];

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
				// fixed length fields are 32 bytes
				if ( blocksize < 32 )
					return libmaus_bambam_alignment_validity_block_too_small;

				uint8_t const * blocke = D.begin() + blocksize;					
				uint8_t const * readname = reinterpret_cast<uint8_t const *>(::libmaus::bambam::BamAlignmentDecoderBase::getReadName(D.begin()));
				uint8_t const * const readnamea = readname;
				
				// look for end of readname
				while ( readname != blocke && *readname )
					++readname;
								
				// read name extends beyond block
				if ( readname == blocke )
					return libmaus_bambam_alignment_validity_queryname_extends_over_block;

				// readname should be pointing to a null byte now
				if ( readname != blocke )
					assert ( ! *readname );

				// check whether actual length of read name is consistent with
				// numerical value given in block
				uint64_t const lreadname = getLReadName();
				if ( (readname-readnamea)+1 != static_cast<ptrdiff_t>(lreadname) )
					return libmaus_bambam_alignment_validity_queryname_length_inconsistent;
				if ( !(readname-readnamea) )
					return libmaus_bambam_alignment_validity_queryname_empty;
				for ( uint64_t i = 0; i < lreadname-1; ++i )
					if ( ! qnameValidTable[readnamea[i]] )
						return libmaus_bambam_alignment_validity_queryname_contains_illegal_symbols;

				// check if cigar string ends before block end
				uint8_t const * cigar = ::libmaus::bambam::BamAlignmentDecoderBase::getCigar(D.begin());
				if ( cigar + getNCigar()*sizeof(uint32_t) > blocke )
					return libmaus_bambam_alignment_validity_cigar_extends_over_block;

				// check if sequence ends before block end
				uint8_t const * seq = ::libmaus::bambam::BamAlignmentDecoderBase::getSeq(D.begin());
				if ( seq + ((getLseq()+1)/2) > blocke )
					return libmaus_bambam_alignment_validity_sequence_extends_over_block;
				
				// check if quality string ends before block end
				uint8_t const * const qual = ::libmaus::bambam::BamAlignmentDecoderBase::getQual(D.begin());
				if ( qual + getLseq() > blocke )
					return libmaus_bambam_alignment_validity_quality_extends_over_block;
				
				// check whether all cigar operators are known	
				for ( uint64_t i = 0; i < getNCigar(); ++i )
				{
					uint64_t const cigop = getCigarFieldOp(i);
					if ( cigop > ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDIFF )
						return libmaus_bambam_alignment_validity_unknown_cigar_op;
				}

				// check if cigar string is consistent with sequence length
				if ( 
					isMapped() && 
					static_cast<int64_t>(getLseqByCigar()) != getLseq() 
				)
					return libmaus_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length;
					
				int32_t const pos = getPos();
				if ( pos < -1 || pos > (((1ll<<29)-1)-1) )
					return libmaus_bambam_alignment_validity_invalid_mapping_position;
				int32_t const nextpos = getNextPos();
				if ( nextpos < -1 || nextpos > (((1ll<<29)-1)-1) )
				{
					return libmaus_bambam_alignment_validity_invalid_next_mapping_position;
				}
				int32_t const tlen = getTlen();
				if ( 
					tlen < ((-(1ll<<29))+1) 
					|| 
					tlen > ((1ll<<29)-1)
				)
					return libmaus_bambam_alignment_validity_invalid_tlen;
					
				uint64_t const lseq = getLseq();
				uint8_t const * const quale = qual + lseq;
				for ( uint8_t const * qualc = qual; qualc != quale; ++qualc )
					if ( static_cast<int>(*qualc) > static_cast<int>('~'-33) )
					{
						if ( *qualc == 255 )
						{
							if ( qualc - qual )
							{
								return libmaus_bambam_alignment_validity_invalid_quality_value;
							}
							
							while ( qualc != quale )
								if ( *(qualc++) != 255 )
									return libmaus_bambam_alignment_validity_invalid_quality_value;
							
							// go back by one to leave loop above
							--qualc;
						}
						else
							return libmaus_bambam_alignment_validity_invalid_quality_value;
					}

				uint8_t const * aux = ::libmaus::bambam::BamAlignmentDecoderBase::getAux(D.begin());

				while ( aux != D.begin()+blocksize )
				{
					if ( ((D.begin()+blocksize)-aux) < 3 )
						return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
						
					switch ( aux[2] )
					{
						case 'A':
							if ( ((D.begin()+blocksize)-aux) < 3+1 )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							if ( aux[3] < '!' || aux[3] > '~' )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'c':
						case 'C':
							if ( ((D.begin()+blocksize)-aux) < 3+1 )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 's':
						case 'S':
							if ( ((D.begin()+blocksize)-aux) < 3+2 )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'i':
						case 'I':
						case 'f':
							if ( ((D.begin()+blocksize)-aux) < 3+4 )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'B':
						{
							if ( ((D.begin()+blocksize)-aux) < 3 + 1 /* data type */ + 4 /* length of array */ )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							/* length of array */
							uint64_t const alen = static_cast<int32_t>(::libmaus::bambam::BamAlignmentDecoderBase::getLEInteger(aux+4,4));
							/* valid element data types */
							switch ( aux[3] )
							{
								case 'c':
								case 'C':
									if ( ((D.begin()+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4+1*alen) )
										return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								case 's':
								case 'S':
									if ( ((D.begin()+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4 + 2*alen) )
										return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								case 'i':
								case 'I':
								case 'f':
									if ( ((D.begin()+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4 + 4*alen) )
										return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								default:
									return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							}
							break;
						}
						case 'Z':
						{
							uint8_t const * p = aux+3;
							uint8_t const * const pe = D.begin() + blocksize;
							
							while ( p != pe && *p )
								++p;
							
							// if terminator byte 0 is not inside block
							if ( p == pe )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;

							break;
						}
						case 'H':
						{
							uint8_t const * const pa = aux+3;
							uint8_t const * p = pa;
							uint8_t const * const pe = D.begin() + blocksize;
							
							while ( p != pe && *p )
								++p;
							
							// if terminator byte 0 is not inside block
							if ( p == pe )
								return libmaus_bambam_alignment_validity_invalid_auxiliary_data;

							break;
						}
						default:
							return libmaus_bambam_alignment_validity_invalid_auxiliary_data;
							break;
					}

					aux = aux + ::libmaus::bambam::BamAlignmentDecoderBase::getAuxLength(aux);
				}


				return libmaus_bambam_alignment_validity_ok;
			}

			/**
			 * check alignment validity including reference sequence ids
			 *
			 * @param header bam header
			 * @return alignment validty code
			 **/
			libmaus_bambam_alignment_validity valid(::libmaus::bambam::BamHeader const & header) const
			{
				libmaus_bambam_alignment_validity validity = valid();
				
				if ( validity != libmaus_bambam_alignment_validity_ok )
					return validity;
				
				int32_t const refseq = getRefID();
				if ( !((refseq == -1) || refseq < static_cast<int64_t>(header.chromosomes.size())) )
					return libmaus_bambam_alignment_validity_invalid_refseq;

				int32_t const nextrefseq = getNextRefID();
				if ( !((nextrefseq == -1) || nextrefseq < static_cast<int64_t>(header.chromosomes.size())) )
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
				replaceCigarString(cigar.begin(),cigar.size());
			}

			/**
			 * replace query sequence in the alignment by the given string
			 *
			 * @param sequence replacement sequence
			 **/
			void replaceSequence(std::string const & sequence)
			{
				BamSeqEncodeTable const seqenc;
				replaceSequence(seqenc,sequence);
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
				std::string const & sequence
			)
			{
				replaceSequence(seqenc,sequence.begin(),sequence.size());
			}
			
			/**
			 * replace query sequence in the alignment by the given string using
			 * the given encode table
			 *
			 * @param seqenc sequence encoding table
			 * @param seq sequence start iterator
			 * @param seqlen sequence length
			 **/
			template<typename seq_iterator>
			void replaceSequence(
				BamSeqEncodeTable const & seqenc,
				seq_iterator seq, 
				uint32_t const seqlen
			)
			{
				uint64_t const pre    = getNumPreSeqBytes();
				uint64_t const oldseq = getNumSeqBytes();
				uint64_t const newseq = (2*seqlen)+1;
				uint64_t const post   = getNumPostSeqBytes();
				
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( pre + newseq + post );
				
				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				::libmaus::bambam::BamAlignmentEncoderBase::encodeSeq(buffer,seqenc,seq,seqlen);

				for ( uint64_t i = 0; i < post; ++i )
					buffer.put ( D [ pre + oldseq + i ] );
					
				D = buffer.abuffer;
				blocksize = buffer.length;
				
				putSeqLen(seqlen);
			}

			/**
			 * replace cigar string by the given encoded string
			 *
			 * @param cigar pre encoded cigar string iterator
			 * @param cigarlen number of cigar operations
			 **/
			template<typename cigar_iterator>
			void replaceCigarString(cigar_iterator cigar, uint32_t const cigarlen)
			{
				uint64_t const pre    = getNumPreCigarBytes();
				uint64_t const oldcig = getNumCigarBytes();
				uint64_t const post   = getNumPostCigarBytes();
				
				::libmaus::fastx::EntityBuffer<uint8_t,D_array_alloc_type> buffer( getNumPreCigarBytes() + cigarlen * sizeof(uint32_t) + getNumPostCigarBytes() );
				
				for ( uint64_t i = 0; i < pre; ++i )
					buffer.put ( D [ i ] );

				::libmaus::bambam::BamAlignmentEncoderBase::encodeCigar(buffer,cigar,cigarlen);

				for ( uint64_t i = 0; i < post; ++i )
					buffer.put ( D [ pre + oldcig + i ] );
					
				D = buffer.abuffer;
				blocksize = buffer.length;
				
				putCigarLen(cigarlen);
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
			int64_t getRank() const
			{
				uint8_t const * p = 
					::libmaus::bambam::BamAlignmentDecoderBase::getAux(D.get(),blocksize,"ZR");
				
				// check format	
				if ( 
					(! p)
					||
					(p[0] != 'Z')
					||
					(p[1] != 'R')
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
			 * @param bamheader BAM header object
			 * @return read group id or -1 if not defined
			 **/
			int64_t getReadGroupId(::libmaus::bambam::BamHeader const & bamheader) const
			{
				return bamheader.getReadGroupId(getReadGroup());
			}
			
			/**
			 * @param bamheader BAM header object
			 * @return library id or -1 if not defined
			 **/
			int64_t getLibraryId(::libmaus::bambam::BamHeader const & bamheader) const
			{
				return bamheader.getLibraryId(getReadGroup());
			}
			
			/**
			 * @param bamheader BAM header object
			 * @return library name for this alignment
			 **/
			std::string getLibraryName(::libmaus::bambam::BamHeader const & bamheader) const
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
					D.get(),blocksize,bamheader.chromosomes,auxiliary);
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
			
			/**
			 * @return sequence length field
			 **/
			int32_t getLseq() const
			{
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
				return reinterpret_cast<BamAlignmentFixedSizeData const *>(D.get())->Lseq;
				#else
				return ::libmaus::bambam::BamAlignmentDecoderBase::getLseq(D.get());
				#endif
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
                                                
		};
	}
}
#endif
