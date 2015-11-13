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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTENCODERBASE_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTENCODERBASE_HPP

#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus2/bambam/BamAlignmentReg2Bin.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>
#include <libmaus2/bambam/BamSeqEncodeTable.hpp>
#include <libmaus2/bambam/CigarStringParser.hpp>
#include <libmaus2/bambam/EncoderBase.hpp>
#include <libmaus2/util/PutObject.hpp>

namespace libmaus2
{
	namespace bambam
	{

		/**
		 * BAM encoding base class
		 **/
		struct BamAlignmentEncoderBase : public EncoderBase, public BamAlignmentReg2Bin
		{
			/**
			 * calculate end position of an alignment
			 *
			 * @param pos start position
			 * @param cigar encoded cigar string
			 * @param cigarlen number of cigar operations
			 * @return end position of alignment
			 **/
			template<typename cigar_iterator>
			static uint32_t endpos(
				int32_t const pos,
				cigar_iterator cigar,
				uint32_t const cigarlen
			)
			{
				uint32_t end = pos;

				for (uint32_t i = 0; i < cigarlen; ++i)
				{
					cigar_operation const & cop = cigar[i];
					uint32_t const op = cop.first;
					uint32_t const len = cop.second;

					switch ( op )
					{
						case 0: // M
						case 2: // D
						case 3: // S
						case 7: // =
						case 8: // X
							end += len;
					}
				}
				return end;
			}

			/**
			 * put a single little endian number v in D at offset; the length of
			 * v is extracted from the data type (value_type) of v
			 *
			 * @param D memory block
			 * @param offset byte offset
			 * @param v value to be stored
			 **/
			template<typename value_type>
			static void putLESingle(uint8_t * D, uint64_t const offset, value_type const v)
			{
				#if defined(LIBMAUS2_HAVE_i386)
				switch ( sizeof(value_type) )
				{
					case 1: *(reinterpret_cast< uint8_t *>(D+offset)) = v; break;
					case 2: *(reinterpret_cast<uint16_t *>(D+offset)) = v; break;
					case 4: *(reinterpret_cast<uint32_t *>(D+offset)) = v; break;
					case 8: *(reinterpret_cast<uint32_t *>(D+offset)) = v; break;
					default:
					{
						::libmaus2::util::PutObject<uint8_t *> P(D + offset);
						putLE< ::libmaus2::util::PutObject<uint8_t *>,value_type >(P,v);
						break;
					}
				}
				#else
				::libmaus2::util::PutObject<uint8_t *> P(D + offset);
				putLE< ::libmaus2::util::PutObject<uint8_t *>,value_type >(P,v);
				#endif
			}

			/**
			 * put reference id v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putRefId(uint8_t * D, int32_t const v) { putLESingle<int32_t>(D,0,v); }
			/**
			 * put position v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putPos(uint8_t * D, int32_t const v) { putLESingle<int32_t>(D,4,v); }
			/**
			 * put reference id of next/mate v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putNextRefId(uint8_t * D, int32_t const v) { putLESingle<int32_t>(D,20,v); }
			/**
			 * put position of next/mate v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putNextPos(uint8_t * D, int32_t const v) { putLESingle<int32_t>(D,24,v); }
			/**
			 * put template length v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putTlen(uint8_t * D, int32_t const v) { putLESingle<int32_t>(D,28,v); }
			/**
			 * put flags and cigar len compound v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putFlagsCigarLen(uint8_t * D, uint32_t const v) { putLESingle<uint32_t>(D,12,v); }
			/**
			 * put flags v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putFlags(uint8_t * D, uint32_t const v) { putLESingle<uint16_t>(D,14,v & 0xFFFFul); }
			/**
			 * put cigar len v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putCigarLen(uint8_t * D, uint32_t const v)
			{
				// put low 16 bits
				putLESingle<uint16_t>(D,12,v & 0xFFFFul);

				// get flags
				uint16_t flags = static_cast<uint16_t>(D[14]) | (static_cast<uint16_t>(D[15])<<8);
				// flag for value requiring more than 16 bits
				uint16_t const flag32 = static_cast<uint16_t>(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FCIGAR32);

				// does value fit in the lower 16 bits?
				if ( expect_true(v <= 0xFFFFul) )
				{
					// erase LIBMAUS2_BAMBAM_FCIGAR32 flag
					flags &= ~flag32;
					// store flags
					putFlags(D,flags);
				}
				else
				{
					// set LIBMAUS2_BAMBAM_FCIGAR32 flag
					flags |= flag32;
					// store flags
					putFlags(D,flags);
					// put top 16 bits in bin field
					putLESingle<uint16_t>(D,10,v>>16);
				}
			}
			/**
			 * put bin value v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putBin(uint8_t * D, uint16_t const v)
			{
				// get flags
				uint16_t const flags = static_cast<uint16_t>(D[14]) | (static_cast<uint16_t>(D[15])<<8);

				// put bin value if bin field is not used for storing top 16 bits of cigar length value
				if ( expect_true( !(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FCIGAR32) ) )
					putLESingle<uint16_t>(D,10,v);
			}

			/**
			 * put length of query sequence v in D
			 *
			 * @param D alignment block
			 * @param v new value
			 **/
			static void putSeqLen(uint8_t * D, uint32_t const v) { putLESingle<uint32_t>(D,16,v); }
			/**
			 * put mapping quality in D
			 *
			 * @param D alignment block
			 * @param v new value for mapping quality
			 **/
			static void putMapQ(uint8_t * D, uint8_t const v)
			{
				// single byte
				D[9] = v;
			}

			/**
			 * encode cigar string in buffer
			 *
			 * @param buffer output buffer
			 * @param cigar encoded cigar array
			 * @param cigarlen number of cigar operations
			 **/
			template<typename buffer_type, typename cigar_iterator>
			static void encodeCigar(
				buffer_type & buffer,
				cigar_iterator cigar,
				uint32_t const cigarlen
			)
			{
				// cigar
				for ( uint32_t i = 0; i < cigarlen; ++i )
				{
					cigar_operation const & cop = cigar[i];
					uint32_t const op = cop.first;
					uint32_t const len = cop.second;
					putLE< buffer_type,uint32_t>(buffer,(len << 4) | op);
				}
			}

			/**
			 * encode query sequence
			 *
			 * @param buffer output buffer
			 * @param seqenc sequence encoding helper structure for mapping symbols
			 * @param seq sequence iterator
			 * @param seqlen length of query sequence
			 **/
			template<typename buffer_type, typename seq_iterator>
			static void encodeSeq(
				buffer_type & buffer,
				BamSeqEncodeTable const & seqenc,
				seq_iterator seq,
				uint32_t const seqlen)
			{
				// sequence
				for ( uint32_t i = 0; i < (seqlen >> 1); ++i )
				{
					uint8_t const high = seqenc[(*(seq++))];
					uint8_t const low  = seqenc[(*(seq++))];

					if ( high >= 16 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentEncoderBase::encodeSeq: " << seq[-2] << " is not a valid BAM symbol" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( low >= 16 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentEncoderBase::encodeSeq: " << seq[-1] << " is not a valid BAM symbol" << std::endl;
						lme.finish();
						throw lme;
					}

					buffer.put( (high << 4) | low );
				}
				if ( seqlen & 1 )
				{
					uint8_t const high = seqenc[(*(seq++))];

					if ( high >= 16 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentEncoderBase::encodeSeq: " << seq[-1] << " is not a valid BAM symbol" << std::endl;
						lme.finish();
						throw lme;
					}

					buffer.put( high<<4 );
				}
			}

			/**
			 * encode pre mapped query sequence
			 *
			 * @param buffer output buffer
			 * @param seq sequence iterator
			 * @param seqlen length of query sequence
			 **/
			template<typename buffer_type, typename seq_iterator>
			static void encodeSeqPreMapped(
				buffer_type & buffer,
				seq_iterator seq,
				uint32_t const seqlen)
			{
				static uint8_t const enctable[256] = {
					1,2,4,8,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
					15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
				// sequence
				for ( uint32_t i = 0; i < (seqlen >> 1); ++i )
				{
					uint8_t const high = enctable[static_cast<int>((*(seq++)))];
					uint8_t const low  = enctable[static_cast<int>((*(seq++)))];
					buffer.put( (high << 4) | low );
				}
				if ( seqlen & 1 )
				{
					uint8_t const high = enctable[static_cast<int>((*(seq++)))];
					buffer.put( high<<4 );
				}
			}

			/**
			 * encode a complete alignment block
			 *
			 * @param buffer output buffer
			 * @param seqenc sequence encoding helper object for encoding sequence symbols
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
				typename qual_iterator,
				::libmaus2::autoarray::alloc_type alloc_type
			>
			static void encodeAlignment(
				::libmaus2::fastx::EntityBuffer<uint8_t,alloc_type> & buffer,
				BamSeqEncodeTable const & seqenc,
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
				uint8_t const qualoffset = 33,
				bool const resetBuffer = true
			)
			{
				typedef ::libmaus2::fastx::EntityBuffer<uint8_t,alloc_type> buffer_type;

				if ( ! libmaus2::bambam::BamAlignmentDecoderBase::nameValid(name,name+namelen) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentEncoderBase::encodeAlignment(): name " << std::string(name,name+namelen) << " is invalid (cannot be stored in a BAM file)" << std::endl;
					lme.finish();
					throw lme;
				}

				// typedef ::libmaus2::fastx::UCharBuffer UCharBuffer;

				if ( resetBuffer )
					buffer.reset();

				uint32_t const bin =
					(cigarlen > 0xFFFFul)
					?
					(cigarlen >> 16)
					:
					(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP) ?
						((pos < 0) ? 4680 : reg2bin(pos,0))
						:
						reg2bin(pos,endpos(pos,cigar,cigarlen)
					);
				uint32_t const cflags = (cigarlen > 0xFFFFul) ? (flags | libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FCIGAR32) : flags;

				assert ( namelen+1 < (1ul << 8) );
				assert ( mapq < (1ul << 8) );
				assert ( bin < (1ul << 16) );
				assert ( flags < (1ul << 16) );
				assert ( cigarlen < (1ul << 16) );

				putLE<buffer_type, int32_t>(buffer,refid); // offset 0
				putLE<buffer_type, int32_t>(buffer,pos);   // offset 4
				putLE<buffer_type,uint32_t>(buffer,((bin & 0xFFFFul) << 16)|((mapq & 0xFF) << 8)|(namelen+1)); // offset 8
				putLE<buffer_type,uint32_t>(buffer,((cflags&0xFFFFu)<<16)|(cigarlen&0xFFFFu)); // offset 12
				putLE<buffer_type, int32_t>(buffer,seqlen); // offset 16
				putLE<buffer_type, int32_t>(buffer,nextrefid); // offset 20
				putLE<buffer_type, int32_t>(buffer,nextpos); // offset 24
				putLE<buffer_type, int32_t>(buffer,tlen); // offset 28

				// name
				buffer.put(name,namelen);
				buffer.put(0);

				// encode cigar string
				encodeCigar(buffer,cigar,cigarlen);

				// encode sequence
				encodeSeq(buffer,seqenc,seq,seqlen);

				// encode quality
				for ( uint32_t i = 0; i < seqlen; ++i )
					buffer.put ( (*(qual++)) - qualoffset );
			}

			/**
			 * encode a complete alignment block with pre mapped sequence
			 *
			 * @param buffer output buffer
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
				typename qual_iterator,
				::libmaus2::autoarray::alloc_type alloc_type
			>
			static void encodeAlignmentPreMapped(
				::libmaus2::fastx::EntityBuffer<uint8_t,alloc_type> & buffer,
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
				uint8_t const qualoffset = 33,
				bool const resetBuffer = true
			)
			{
				typedef ::libmaus2::fastx::EntityBuffer<uint8_t,alloc_type> buffer_type;

				if ( ! libmaus2::bambam::BamAlignmentDecoderBase::nameValid(name,name+namelen) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentEncoderBase::encodeAlignment(): name " << std::string(name,name+namelen) << " is invalid (cannot be stored in a BAM file)" << std::endl;
					lme.finish();
					throw lme;
				}

				// typedef ::libmaus2::fastx::UCharBuffer UCharBuffer;

				if ( resetBuffer )
					buffer.reset();

				uint32_t const bin =
					(cigarlen > 0xFFFFul)
					?
					(cigarlen >> 16)
					:
					(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP) ?
						((pos < 0) ? 4680 : reg2bin(pos,0))
						:
						reg2bin(pos,endpos(pos,cigar,cigarlen)
					);
				uint32_t const cflags = (cigarlen > 0xFFFFul) ? (flags | libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FCIGAR32) : flags;

				assert ( namelen+1 < (1ul << 8) );
				assert ( mapq < (1ul << 8) );
				assert ( bin < (1ul << 16) );
				assert ( flags < (1ul << 16) );
				assert ( cigarlen < (1ul << 16) );

				putLE<buffer_type, int32_t>(buffer,refid); // offset 0
				putLE<buffer_type, int32_t>(buffer,pos);   // offset 4
				putLE<buffer_type,uint32_t>(buffer,((bin & 0xFFFFul) << 16)|((mapq & 0xFF) << 8)|(namelen+1)); // offset 8
				putLE<buffer_type,uint32_t>(buffer,((cflags&0xFFFFu)<<16)|(cigarlen&0xFFFFu)); // offset 12
				putLE<buffer_type, int32_t>(buffer,seqlen); // offset 16
				putLE<buffer_type, int32_t>(buffer,nextrefid); // offset 20
				putLE<buffer_type, int32_t>(buffer,nextpos); // offset 24
				putLE<buffer_type, int32_t>(buffer,tlen); // offset 28

				// name
				buffer.put(name,namelen);
				buffer.put(0);

				// encode cigar string
				encodeCigar(buffer,cigar,cigarlen);

				// encode sequence
				encodeSeqPreMapped(buffer,seq,seqlen);

				// encode quality
				for ( uint32_t i = 0; i < seqlen; ++i )
					buffer.put ( (*(qual++)) - qualoffset );
			}

			/**
			 * encode a complete alignment block
			 *
			 * @param buffer output buffer
			 * @param seqenc sequence encoding helper object for encoding sequence symbols
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
			static void encodeAlignment(
				::libmaus2::fastx::UCharBuffer & buffer,
				BamSeqEncodeTable const & seqenc,
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
				uint8_t const qualoffset = 33,
				bool const resetBuffer = true
			)
			{
				std::vector<cigar_operation> cigvec =
					CigarStringParser::parseCigarString(cigar);
				encodeAlignment(
					buffer,seqenc,
					name.begin(),name.size(),
					refid,pos,mapq,flags,
					cigvec.begin(),cigvec.size(),
					nextrefid,nextpos,tlen,
					seq.begin(),seq.size(),
					qual.begin(),qualoffset,
					resetBuffer);
			}

			/**
			 * put auxiliary tag for id tag as string
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param value character string
			 **/
			template<typename buffer_type, typename value_type>
			static void putAuxString(
				buffer_type & data,
				std::string const & tag,
				value_type const & value
			)
			{
				assert ( tag.size() == 2 );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('Z');

				char const * c = reinterpret_cast<char const *>(value);
				uint64_t const len = strlen(c);
				for ( uint64_t i = 0; i < len; ++i )
					data.bufferPush(c[i]);
				data.bufferPush(0);
			}

			/**
			 * put auxiliary tag for id tag as string
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param value character string
			 **/
			template<typename buffer_type, typename value_type>
			static void putAuxString(
				buffer_type & data,
				char const * tag,
				value_type const & value
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('Z');

				char const * c = reinterpret_cast<char const *>(value);
				uint64_t const len = strlen(c);
				for ( uint64_t i = 0; i < len; ++i )
					data.bufferPush(c[i]);
				data.bufferPush(0);
			}

			/**
			 * put auxiliary tag for id tag as string
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param pa character string start iterator
			 * @param pe character string end iterator
			 **/
			template<typename buffer_type, typename iterator_type>
			static void putAuxString(
				buffer_type & data,
				char const * tag,
				iterator_type pa,
				iterator_type pe
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('Z');

				while ( pa != pe )
					data.bufferPush( *(pa++) );
				data.bufferPush(0);
			}

			/**
			 * put auxiliary tag for id tag as string
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param pa character string start iterator
			 * @param pe character string end iterator
			 **/
			template<typename buffer_type, typename iterator_type>
			static void putAuxHexString(
				buffer_type & data,
				char const * tag,
				iterator_type pa,
				iterator_type pe
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('H');

				while ( pa != pe )
					data.bufferPush( *(pa++) );
				data.bufferPush(0);
			}

			//! number reinterpretation union
			union numberpun
			{
				//! float value
				float fvalue;
				//! uint32_t value
				uint32_t uvalue;
			};

			/**
			 * put aux field for tag with number content
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param type number type (see sam spec)
			 * @param value number
			 **/
			template<typename buffer_type, typename value_type>
			static void putAuxNumber(
				buffer_type & data,
				std::string const & tag,
				char const type,
				value_type const & value
			)
			{
				assert ( tag.size() == 2 );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush(type);

				switch ( type )
				{
					case 'A':
						putLE< buffer_type,int8_t>(data,value);
						break;
					case 'c':
						putLE< buffer_type,int8_t>(data,value);
						break;
					case 'C':
						putLE< buffer_type,uint8_t>(data,value);
						break;
					case 's':
						putLE< buffer_type,int16_t>(data,value);
						break;
					case 'S':
						putLE< buffer_type,uint16_t>(data,value);
						break;
					case 'i':
						putLE< buffer_type,int32_t>(data,value);
						break;
					case 'I':
						putLE< buffer_type,uint32_t>(data,value);
						break;
					case 'f':
					{
						numberpun np;
						np.fvalue = value;

						putLE< buffer_type,uint32_t>(
							data, np.uvalue
						);
					}
						break;
				}
			}

			/**
			 * put aux field for tag with number content
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param type number type (see sam spec)
			 * @param value number
			 **/
			template<typename buffer_type, typename value_type>
			static void putAuxNumber(
				buffer_type & data,
				char const * const tag,
				char const type,
				value_type const & value
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush(type);

				switch ( type )
				{
					case 'A':
						putLE< buffer_type,int8_t>(data,value);
						break;
					case 'c':
						putLE< buffer_type,int8_t>(data,value);
						break;
					case 'C':
						putLE< buffer_type,uint8_t>(data,value);
						break;
					case 's':
						putLE< buffer_type,int16_t>(data,value);
						break;
					case 'S':
						putLE< buffer_type,uint16_t>(data,value);
						break;
					case 'i':
						putLE< buffer_type,int32_t>(data,value);
						break;
					case 'I':
						putLE< buffer_type,uint32_t>(data,value);
						break;
					case 'f':
					{
						numberpun np;
						np.fvalue = value;

						putLE< buffer_type,uint32_t>(
							data, np.uvalue
						);
					}
						break;
				}
			}

			/**
			 * put aux field for tag with number array content
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param type number type (see sam spec)
			 * @param values vector of numbers
			 **/
			template<typename buffer_type, typename value_type>
			static void putAuxNumberArray(
				buffer_type & data,
				std::string const & tag,
				char const type,
				std::vector<value_type> const & values
			)
			{
				assert ( tag.size() == 2 );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('B');
				data.bufferPush(type);

				putLE<buffer_type,uint32_t>(data,values.size());

				for ( uint64_t i = 0; i < values.size(); ++i )
				{
					switch ( type )
					{
						case 'A':
							putLE< buffer_type,int8_t>(data,values[i]);
							break;
						case 'c':
							putLE< buffer_type,int8_t>(data,values[i]);
							break;
						case 'C':
							putLE< buffer_type,uint8_t>(data,values[i]);
							break;
						case 's':
							putLE< buffer_type,int16_t>(data,values[i]);
							break;
						case 'S':
							putLE< buffer_type,uint16_t>(data,values[i]);
							break;
						case 'i':
							putLE< buffer_type,int32_t>(data,values[i]);
							break;
						case 'I':
							putLE< buffer_type,uint32_t>(data,values[i]);
							break;
						case 'f':
						{
							numberpun np;
							np.fvalue = values[i];

							putLE< buffer_type,uint32_t>(
								data, np.uvalue
							);
						}
							break;
					}
				}
			}

			/**
			 * put aux field for tag with number array content
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param type number type (see sam spec)
			 * @param values vector of numbers
			 * @param n length of vector
			 **/
			template<typename buffer_type, typename iterator_type>
			static void putAuxNumberArray(
				buffer_type & data,
				std::string const & tag,
				char const type,
				iterator_type values,
				uint64_t const n
			)
			{
				assert ( tag.size() == 2 );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('B');
				data.bufferPush(type);

				putLE<buffer_type,uint32_t>(data,n);

				for ( uint64_t i = 0; i < n; ++i )
				{
					switch ( type )
					{
						case 'A':
							putLE< buffer_type,int8_t>(data,*(values++));
							break;
						case 'c':
							putLE< buffer_type,int8_t>(data,*(values++));
							break;
						case 'C':
							putLE< buffer_type,uint8_t>(data,*(values++));
							break;
						case 's':
							putLE< buffer_type,int16_t>(data,*(values++));
							break;
						case 'S':
							putLE< buffer_type,uint16_t>(data,*(values++));
							break;
						case 'i':
							putLE< buffer_type,int32_t>(data,*(values++));
							break;
						case 'I':
							putLE< buffer_type,uint32_t>(data,*(values++));
							break;
						case 'f':
						{
							numberpun np;
							np.fvalue = *(values++);

							putLE< buffer_type,uint32_t>(
								data, np.uvalue
							);
						}
							break;
					}
				}
			}

			/**
			 * put aux field for tag with number array content
			 *
			 * @param data output buffer
			 * @param tag aux tag
			 * @param type number type (see sam spec)
			 * @param values vector of numbers
			 * @param n length of vector
			 **/
			template<typename buffer_type, typename iterator_type>
			static void putAuxNumberArray(
				buffer_type & data,
				char const * const tag,
				char const type,
				iterator_type values,
				uint64_t const n
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				data.bufferPush(tag[0]);
				data.bufferPush(tag[1]);
				data.bufferPush('B');
				data.bufferPush(type);

				putLE<buffer_type,uint32_t>(data,n);

				for ( uint64_t i = 0; i < n; ++i )
				{
					switch ( type )
					{
						case 'A':
							putLE< buffer_type,int8_t>(data,*(values++));
							break;
						case 'c':
							putLE< buffer_type,int8_t>(data,*(values++));
							break;
						case 'C':
							putLE< buffer_type,uint8_t>(data,*(values++));
							break;
						case 's':
							putLE< buffer_type,int16_t>(data,*(values++));
							break;
						case 'S':
							putLE< buffer_type,uint16_t>(data,*(values++));
							break;
						case 'i':
							putLE< buffer_type,int32_t>(data,*(values++));
							break;
						case 'I':
							putLE< buffer_type,uint32_t>(data,*(values++));
							break;
						case 'f':
						{
							numberpun np;
							np.fvalue = *(values++);

							putLE< buffer_type,uint32_t>(
								data, np.uvalue
							);
						}
							break;
					}
				}
			}

			/**
			 * write create alignment in buffer to stream
			 *
			 * @param buffer memory buffer containing alignment
			 * @param stream output stream
			 **/
			template<typename stream_type>
			static void writeToStream(
				::libmaus2::fastx::UCharBuffer & buffer,
				stream_type & stream)
			{
				putLE<stream_type,uint32_t>(stream,buffer.length);
				stream.write(reinterpret_cast<char const *>(buffer.buffer),buffer.length);
			}

			/**
			 * make mate pair information of two alignments consistent (inspired by Picard code)
			 *
			 * @param rec1u pointer to alignment block for read 1
			 * @param rec2u pointer to alignment block for read 2
			 * @return pair containing the value of the MQ field to be added for the alignment blocks (-1 for none)
			 */
			static std::pair<int16_t,int16_t> fixMateInformation(uint8_t * rec1u, uint8_t * rec2u)
			{
				static uint32_t const next_rev_flag = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE;
				static uint32_t const next_unmap_flag = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP;
				uint32_t const rec1flags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(rec1u);
				uint32_t const rec2flags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(rec2u);
				bool const rec1unmap = libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(rec1flags);
				bool const rec2unmap = libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(rec2flags);
				int16_t mq1, mq2;

				// both mapped
				if ( (!rec1unmap) && (!rec2unmap) )
				{
					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(rec1u,libmaus2::bambam::BamAlignmentDecoderBase::getRefID(rec2u));
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(rec1u,libmaus2::bambam::BamAlignmentDecoderBase::getPos(rec2u));
					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(rec1u,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(rec2flags) ? (rec1flags | next_rev_flag) : (rec1flags & (~next_rev_flag))) & (~next_unmap_flag) );

					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(rec2u,libmaus2::bambam::BamAlignmentDecoderBase::getRefID(rec1u));
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(rec2u,libmaus2::bambam::BamAlignmentDecoderBase::getPos(rec1u));
					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(rec2u,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(rec1flags) ? (rec2flags | next_rev_flag) : (rec2flags & (~next_rev_flag))) & (~next_unmap_flag) );

					int64_t const insertSize = libmaus2::bambam::BamAlignmentDecoderBase::computeInsertSize(rec1u,rec2u);
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(rec1u,insertSize);
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(rec2u,-insertSize);

					mq1 = libmaus2::bambam::BamAlignmentDecoderBase::getMapQ(rec2u);
					mq2 = libmaus2::bambam::BamAlignmentDecoderBase::getMapQ(rec1u);
				}
				// both unmapped
				else if ( rec1unmap && rec2unmap )
				{
					libmaus2::bambam::BamAlignmentEncoderBase::putRefId(rec1u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putPos(rec1u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(rec1u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(rec1u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(rec1u,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(rec2flags) ? (rec1flags | next_rev_flag) : (rec1flags & (~next_rev_flag))) | (next_unmap_flag) );
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(rec1u,0);

					libmaus2::bambam::BamAlignmentEncoderBase::putRefId(rec2u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putPos(rec2u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(rec2u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(rec2u,-1);
					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(rec2u,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(rec1flags) ? (rec2flags | next_rev_flag) : (rec2flags & (~next_rev_flag))) | (next_unmap_flag) );
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(rec2u,0);

					mq1 = -1;
					mq2 = -1;
				}
				// one mapped and other one unmapped
				else
				{
					uint8_t * mappedu = rec1unmap ? rec2u : rec1u;
					uint8_t * unmappedu = rec1unmap ? rec1u : rec2u;
					uint32_t const mappedflags = rec1unmap ? rec2flags : rec1flags;
					uint32_t const unmappedflags = rec1unmap ? rec1flags : rec2flags;

					// copy refid and pos from mapped to unmapped
					libmaus2::bambam::BamAlignmentEncoderBase::putRefId(unmappedu,libmaus2::bambam::BamAlignmentDecoderBase::getRefID(mappedu));
					libmaus2::bambam::BamAlignmentEncoderBase::putPos(unmappedu,libmaus2::bambam::BamAlignmentDecoderBase::getPos(mappedu));
					// copy refid and pos from mapped to mapped next
					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(mappedu,libmaus2::bambam::BamAlignmentDecoderBase::getRefID(mappedu));
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(mappedu,libmaus2::bambam::BamAlignmentDecoderBase::getPos(mappedu));

					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(mappedu,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(unmappedflags) ? (mappedflags | next_rev_flag) : (mappedflags & (~next_rev_flag))) | (next_unmap_flag) );
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(mappedu,0);

					libmaus2::bambam::BamAlignmentEncoderBase::putNextRefId(unmappedu,libmaus2::bambam::BamAlignmentDecoderBase::getRefID(mappedu));
					libmaus2::bambam::BamAlignmentEncoderBase::putNextPos(unmappedu,libmaus2::bambam::BamAlignmentDecoderBase::getPos(mappedu));

					libmaus2::bambam::BamAlignmentEncoderBase::putFlags(unmappedu,(libmaus2::bambam::BamAlignmentDecoderBase::isReverse(mappedflags) ? (unmappedflags | next_rev_flag) : (unmappedflags & (~next_rev_flag))) & (~next_unmap_flag) );
					libmaus2::bambam::BamAlignmentEncoderBase::putTlen(unmappedu,0);

					if ( rec1unmap )
					{
						mq1 = libmaus2::bambam::BamAlignmentDecoderBase::getMapQ(rec2u);
						mq2 = -1;
					}
					else
					{
						mq1 = -1;
						mq2 = libmaus2::bambam::BamAlignmentDecoderBase::getMapQ(rec1u);
					}
				}

				return std::pair<int16_t,int16_t>(mq1,mq2);
			}
		};
	}
}
#endif
