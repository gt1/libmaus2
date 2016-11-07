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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTDECODERBASE_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTDECODERBASE_HPP

#include <libmaus2/bambam/CigarDecoder.hpp>
#include <libmaus2/bambam/BamAuxSortingBuffer.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bambam/AlignmentValidity.hpp>
#include <libmaus2/bambam/BamAlignmentReg2Bin.hpp>
#include <libmaus2/bambam/BamAuxFilterVector.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>
#include <libmaus2/bambam/BamFormatAuxiliary.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/bambam/DecoderBase.hpp>
#include <libmaus2/bambam/MdStringComputationContext.hpp>
#include <libmaus2/hashing/hash.hpp>
#include <libmaus2/bambam/CigarOperation.hpp>
#include <libmaus2/bambam/CigarStringParser.hpp>
#include <libmaus2/math/IPower.hpp>
#include <libmaus2/bambam/PileVectorElement.hpp>
#include <libmaus2/math/IntegerInterval.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * class containing static base functions for BAM alignment encoding and decoding
		 **/
		struct BamAlignmentDecoderBase : public ::libmaus2::bambam::DecoderBase, public ::libmaus2::bambam::BamFlagBase
		{
			//! valid query name symbols table
			static uint8_t const qnameValidTable[256];

			/**
			 * @param flags flag word
			 * @return true iff flags have the paired flag set
			 **/
			static bool isPaired(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FPAIRED; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the proper pair flag set
			 **/
			static bool isProper(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FPROPER_PAIR; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the unmapped flag set
			 **/
			static bool isUnmap(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FUNMAP; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the mate unmapped flag set
			 **/
			static bool isMateUnmap(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FMUNMAP; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the reverse flag set
			 **/
			static bool isReverse(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FREVERSE; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the mate reverse flag set
			 **/
			static bool isMateReverse(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FMREVERSE; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the read 1 flag set
			 **/
			static bool isRead1(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FREAD1; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the read 2 flag set
			 **/
			static bool isRead2(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FREAD2; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the secondary alignment flag set
			 **/
			static bool isSecondary(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FSECONDARY; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the quality control failed flag set
			 **/
			static bool isQCFail(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FQCFAIL; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the duplicate flag set
			 **/
			static bool isDup(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FDUP; }
			/**
			 * @param flags flag word
			 * @return true iff flags have the supplementary flag set
			 **/
			static bool isSupplementary(uint32_t const flags) { return flags & LIBMAUS2_BAMBAM_FSUPPLEMENTARY; }

			/**
			 * convert single bit flag to a string representation
			 *
			 * @param flag single bit flag
			 * @return string representation of flag
			 **/
			static char const * flagToString(uint32_t const flag)
			{
				switch ( flag )
				{
					case LIBMAUS2_BAMBAM_FPAIRED: return "LIBMAUS2_BAMBAM_FPAIRED";
					case LIBMAUS2_BAMBAM_FPROPER_PAIR: return "LIBMAUS2_BAMBAM_FPROPER_PAIR";
					case LIBMAUS2_BAMBAM_FUNMAP: return "LIBMAUS2_BAMBAM_FUNMAP";
					case LIBMAUS2_BAMBAM_FMUNMAP: return "LIBMAUS2_BAMBAM_FMUNMAP";
					case LIBMAUS2_BAMBAM_FREVERSE: return "LIBMAUS2_BAMBAM_FREVERSE";
					case LIBMAUS2_BAMBAM_FMREVERSE: return "LIBMAUS2_BAMBAM_FMREVERSE";
					case LIBMAUS2_BAMBAM_FREAD1: return "LIBMAUS2_BAMBAM_FREAD1";
					case LIBMAUS2_BAMBAM_FREAD2: return "LIBMAUS2_BAMBAM_FREAD2";
					case LIBMAUS2_BAMBAM_FSECONDARY: return "LIBMAUS2_BAMBAM_FSECONDARY";
					case LIBMAUS2_BAMBAM_FQCFAIL: return "LIBMAUS2_BAMBAM_FQCFAIL";
					case LIBMAUS2_BAMBAM_FDUP: return "LIBMAUS2_BAMBAM_FDUP";
					case LIBMAUS2_BAMBAM_FSUPPLEMENTARY: return "LIBMAUS2_BAMBAM_FSUPPLEMENTARY";
					default: return "LIBMAUS2_BAMBAM_F?";
				}
			}

			/**
			 * convert a set of flags to a string representation
			 *
			 * @param flags flag bit set
			 * @return string representation of flags
			 **/
			static std::string flagsToString(uint32_t const flags)
			{
				unsigned int numflags = 0;
				for ( unsigned int i = 0; i < 11; ++i )
					if ( (flags & (1u << i)) )
						numflags++;

				std::ostringstream ostr;
				unsigned int procflags = 0;

				for ( unsigned int i = 0; i < 11; ++i )
					if ( (flags & (1u << i)) )
					{
						ostr << flagToString((flags & (1u << i)));
						if ( procflags++ != numflags )
							ostr << ";";
					}

				return ostr.str();
			}

			/**
			 * compute 32 bit hash value from alignment block D
			 *
			 * @param D alignment block
			 * @return hash value for D
			 **/
			static uint32_t hash32(uint8_t const * D)
			{
				return ::libmaus2::hashing::EvaHash::hash(
					reinterpret_cast<uint8_t const *>(getReadName(D)),
					getLReadName(D)-1
				);
			}

			/**
			 * write FastQ representation of alignment D into array T; T is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param T output array
			 * @return number of bytes written
			 **/
			static uint64_t putFastQ(
				uint8_t const * D,
				libmaus2::autoarray::AutoArray<uint8_t> & T
			)
			{
				uint64_t const len = getFastQLength(D);
				if ( T.size() < len )
					T = libmaus2::autoarray::AutoArray<uint8_t>(len);
				putFastQ(D,T.begin());
				return len;
			}

			/**
			 * write FastQ + aux representation of alignment D into array T; T is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param T output array
			 * @return number of bytes written
			 **/
			static uint64_t putFastQAux(
				uint8_t const * D,
				uint64_t const blocksize,
				BamAuxFilterVector const & tags,
				libmaus2::autoarray::AutoArray<uint8_t> & T
			)
			{
				bool anyaux = false;
				uint64_t const len = getFastQAuxLength(D,blocksize,tags,anyaux);
				if ( T.size() < len )
					T = libmaus2::autoarray::AutoArray<uint8_t>(len);
				putFastQAux(D,blocksize,tags,anyaux,T.begin());
				return len;
			}


			/**
			 * write FastQ representation of alignment D into array T; T is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param T output array
			 * @return number of bytes written
			 **/
			static uint64_t putFastQTryOQ(
				uint8_t const * D,
				uint64_t const blocksize,
				libmaus2::autoarray::AutoArray<uint8_t> & T
			)
			{
				uint64_t const len = getFastQLength(D);
				if ( T.size() < len )
					T = libmaus2::autoarray::AutoArray<uint8_t>(len);
				putFastQTryOQ(D,blocksize,T.begin());
				return len;
			}

			/**
			 * write FastA representation of alignment D into array T; T is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param T output array
			 * @return number of bytes written
			 **/
			static uint64_t putFastA(
				uint8_t const * D,
				libmaus2::autoarray::AutoArray<uint8_t> & T
			)
			{
				uint64_t const len = getFastALength(D);
				if ( T.size() < len )
					T = libmaus2::autoarray::AutoArray<uint8_t>(len);
				putFastA(D,T.begin());
				return len;
			}

			/**
			 * write FastQ representation of alignment D into array T; T is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param ranka rank of first mate
			 * @param rankb rank of second mate
			 * @param T output array
			 * @return number of bytes written
			 **/
			static uint64_t putFastQRanks(
				uint8_t const * D,
				uint64_t const ranka,
				uint64_t const rankb,
				libmaus2::autoarray::AutoArray<uint8_t> & T
			)
			{
				uint64_t const len = getFastQLengthRanks(D,ranka,rankb);
				if ( T.size() < len )
					T = libmaus2::autoarray::AutoArray<uint8_t>(len);
				putFastQRanks(D,ranka,rankb,T.begin());
				return len;
			}

			/**
			 * get length of FastQ representation for alignment block D in bytes
			 *
			 * @param D alignment block
			 * @return length of FastQ entry
			 **/
			static uint64_t getFastQLength(uint8_t const * D)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);

				return
					1 + namelen + ((flags & LIBMAUS2_BAMBAM_FPAIRED) ? 2 : 0) + 1 + // name line
					lseq + 1 + // seq line
					1 + 1 + // plus line
					lseq + 1 // quality line
					;
			}

			/**
			 * get length of FastQ + aux representation for alignment block D in bytes
			 *
			 * @param D alignment block
			 * @return length of FastQ entry
			 **/
			static uint64_t getFastQAuxLength(uint8_t const * D, uint64_t const blocksize, BamAuxFilterVector const & tags, bool & anyaux)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				libmaus2::util::CountPutObject CPO;
				printAuxBlock(CPO,D,blocksize,tags,false);
				uint64_t const auxlen = CPO.c ? (1 + CPO.c) : 0;
				anyaux = (auxlen != 0);

				return
					1 + namelen + ((flags & LIBMAUS2_BAMBAM_FPAIRED) ? 2 : 0) + auxlen + 1 + // name line
					lseq + 1 + // seq line
					1 + 1 + // plus line
					lseq + 1 // quality line
					;
			}

			/**
			 * get length of FastA representation for alignment block D in bytes
			 *
			 * @param D alignment block
			 * @return length of FastA entry
			 **/
			static uint64_t getFastALength(uint8_t const * D)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);

				return
					1 + namelen + ((flags & LIBMAUS2_BAMBAM_FPAIRED) ? 2 : 0) + 1 + // name line
					lseq + 1 // seq line
					;
			}

			/**
			 * get length of FastQ representation for alignment block D in bytes
			 * with ranks added to name
			 *
			 * @param D alignment block
			 * @param ranka rank of first read
			 * @param rankb rank of second read
			 * @return length of FastQ entry
			 **/
			static uint64_t getFastQLengthRanks(uint8_t const * D, uint64_t const ranka, uint64_t const rankb)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				uint64_t const lra = getDecimalNumberLength(ranka);
				uint64_t const lrb = getDecimalNumberLength(rankb);

				return
					1 + namelen +
					((flags & LIBMAUS2_BAMBAM_FPAIRED) ? (2 + lra + lrb) : (1+lra)) +
					((flags & LIBMAUS2_BAMBAM_FPAIRED) ? 2 : 0) + 1 + // name line
					lseq + 1 + // seq line
					1 + 1 + // plus line
					lseq + 1 // quality line
					;
			}

			/**
			 * get length of number rank in decimal representation
			 *
			 * @param rank
			 * @return length of number rank in decimal representation
			 **/
			static uint64_t getDecimalNumberLength(uint64_t rank)
			{
				if ( ! rank )
				{
					return 1;
				}
				else
				{
					unsigned int p = 0;

					if ( rank >= libmaus2::math::IPower<10,16>::n )
						rank /= libmaus2::math::IPower<10,16>::n, p += 16;
					if ( rank >= libmaus2::math::IPower<10, 8>::n )
						rank /= libmaus2::math::IPower<10, 8>::n, p += 8;
					if ( rank >= libmaus2::math::IPower<10, 4>::n )
						rank /= libmaus2::math::IPower<10, 4>::n, p += 4;
					if ( rank >= libmaus2::math::IPower<10, 2>::n )
						rank /= libmaus2::math::IPower<10, 2>::n, p += 2;
					if ( rank >= libmaus2::math::IPower<10, 1>::n )
						rank /= libmaus2::math::IPower<10, 1>::n, p += 1;
					if ( rank )
						rank /= 10, p += 1;

					assert ( ! rank );

					return p;
				}
			}

			/**
			 * format number rank to iterator it as decimal
			 *
			 * @param it iterator
			 * @param rank number to be formatted
			 * @return iterator after output
			 **/
			template<typename iterator>
			static iterator putNumberDecimal(iterator it, uint64_t rank)
			{
				if ( ! rank )
				{
					*(it++) = '0';
				}
				else
				{
					// 20 decimal digits is enough for a 64 bit number
					#if defined(_MSC_VER) || defined(__MINGW32__)
					uint8_t * S = reinterpret_cast<uint8_t *>(_alloca(20));
					#else
					uint8_t * S = reinterpret_cast<uint8_t *>(alloca(20));
					#endif

					uint8_t * SA = S;

					// generate digits
					while ( rank )
					{
						*(S++) = rank % 10;
						rank /= 10;
					}

					assert ( ! rank );

					// write them out
					while ( S != SA )
						*(it++) = (*(--S)) + '0';
				}

				return it;
			}

			/**
			 * write FastQ representation of alignment block D to iterator it
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @return output iterator after writing
			 **/
			template<typename iterator>
			static iterator putFastQ(uint8_t const * D, iterator it)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '@';
				while ( rn != rne )
					*(it++) = *(rn++);

				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS2_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}

				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);

				*(it++) = '\n';

				*(it++) = '+';
				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
				{
					uint8_t const * const quale = getQual(D);
					uint8_t const * qualc = quale + lseq;

					while ( qualc != quale )
						*(it++) = *(--qualc) + 33;
				}
				else
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + lseq;

					while ( qual != quale )
						*(it++) = (*(qual++)) + 33;
				}
				*(it++) = '\n';

				return it;
			}

			template<typename iterator>
			static iterator putFastQAux(
				uint8_t const * D,
				uint64_t const blocksize,
				BamAuxFilterVector const & tags,
				bool const anyaux,
				iterator it
			)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '@';
				while ( rn != rne )
					*(it++) = *(rn++);

				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS2_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}

				if ( anyaux )
				{
					*(it++) = ' ';
					libmaus2::util::PutObject<iterator> PO(it);
					printAuxBlock(PO,D,blocksize,tags,false);
					it = PO.p;
				}

				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);

				*(it++) = '\n';

				*(it++) = '+';
				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
				{
					uint8_t const * const quale = getQual(D);
					uint8_t const * qualc = quale + lseq;

					while ( qualc != quale )
						*(it++) = *(--qualc) + 33;
				}
				else
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + lseq;

					while ( qual != quale )
						*(it++) = (*(qual++)) + 33;
				}
				*(it++) = '\n';

				return it;
			}

			/**
			 * write FastQ representation of alignment block D to iterator it
			 * if present use OQ field instead of quality field
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @param altqual alternative quality string
			 * @return output iterator after writing
			 **/
			template<typename iterator>
			static iterator putFastQTryOQ(uint8_t const * D, uint64_t const blocksize, iterator it)
			{
				char const * const altqual = getAuxString(D,blocksize,"OQ");

				// use OQ field if it is present and has the correct length
				if ( altqual && (static_cast<int64_t>(strlen(altqual)) == static_cast<int64_t>(getLseq(D))) )
					return putFastQ(D,it,altqual);
				else
					return putFastQ(D,it);
			}

			/**
			 * write FastQ representation of alignment block D to iterator it
			 * use alternative quality string
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @param altqual alternative quality string
			 * @return output iterator after writing
			 **/
			template<typename iterator>
			static iterator putFastQ(uint8_t const * D, iterator it, char const * const altqual)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '@';
				while ( rn != rne )
					*(it++) = *(rn++);

				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS2_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}

				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);

				*(it++) = '\n';

				*(it++) = '+';
				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
				{
					char const * const quale = altqual;
					char const * qualc = quale + lseq;

					while ( qualc != quale )
						*(it++) = *(--qualc);
				}
				else
				{
					char const * qual = altqual;
					char const * const quale = qual + lseq;

					while ( qual != quale )
						*(it++) = (*(qual++));
				}
				*(it++) = '\n';

				return it;
			}

			/**
			 * write FastA representation of alignment block D to iterator it
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @return output iterator after writing
			 **/
			template<typename iterator>
			static iterator putFastA(uint8_t const * D, iterator it)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '>';
				while ( rn != rne )
					*(it++) = *(rn++);

				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS2_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}

				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);

				*(it++) = '\n';

				return it;
			}

			/**
			 * write FastQ representation of alignment block D to iterator it
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @return output iterator after writing
			 **/
			template<typename iterator>
			static iterator putFastQRanks(uint8_t const * D, uint64_t const ranka, uint64_t const rankb, iterator it)
			{
				uint32_t const flags = getFlags(D);
				uint64_t const namelen = getLReadName(D)-1;
				uint64_t const lseq = getLseq(D);
				char const * rn = getReadName(D);
				char const * rne = rn + namelen;

				*(it++) = '@';

				it = putNumberDecimal(it,ranka);
				*(it++) = '_';

				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					it = putNumberDecimal(it,rankb);
					*(it++) = '_';
				}

				while ( rn != rne )
					*(it++) = *(rn++);


				if ( (flags & LIBMAUS2_BAMBAM_FPAIRED) )
				{
					if ( (flags & LIBMAUS2_BAMBAM_FREAD1) )
					{
						*(it++) = '/';
						*(it++) = '1';
					}
					else
					{
						*(it++) = '/';
						*(it++) = '2';
					}
				}

				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
					it = decodeReadRCIt(D,it,lseq);
				else
					it = decodeRead(D,it,lseq);

				*(it++) = '\n';

				*(it++) = '+';
				*(it++) = '\n';

				if ( flags & LIBMAUS2_BAMBAM_FREVERSE )
				{
					uint8_t const * const quale = getQual(D);
					uint8_t const * qualc = quale + lseq;

					while ( qualc != quale )
						*(it++) = *(--qualc) + 33;
				}
				else
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + lseq;

					while ( qual != quale )
						*(it++) = (*(qual++)) + 33;
				}
				*(it++) = '\n';

				return it;
			}

			/**
			 * get little endian integer from ring buffer array
			 *
			 * @param D ring buffer
			 * @param dn size of ring buffer (must be a power of 2)
			 * @param o start offset in D
			 * @param l length of integer in bytes
			 * @return decoded integer
			 **/
			static uint32_t getLEIntegerWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t o,
				unsigned int const l)
			{
				uint32_t v = 0;
				for ( unsigned int i = 0; i < l; ++i, o=((o+1)&(dn-1)) )
					v |= (static_cast<uint32_t>(D[o]) << (8*i));
				return v;
			}

			/**
			 * get BinMQNL field from alignment block at offset o in ring buffer of length dn
			 *
			 * @param D ring buffer
			 * @param dn length of ring buffer (must be a power of 2)
			 * @param o start offset of alignment block in D
			 * @return decoded BinMQNL field
			 **/
			static uint32_t getBinMQNLWrapped  (
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return static_cast<uint32_t>(getLEIntegerWrapped(D,dn,(o+8)&(dn-1),4)); }

			/**
			 * get LReadNameWrapped field from alignment block at offset o in ring buffer of length dn
			 *
			 * @param D ring buffer
			 * @param dn length of ring buffer (must be a power of 2)
			 * @param o start offset of alignment block in D
			 * @return decoded LReadNameWrapped field
			 **/
			static uint32_t getLReadNameWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return (getBinMQNLWrapped(D,dn,o) >> 0) & 0xFF; }

			/**
			 * get read name offset in ring buffer
			 *
			 * @param dn length of ring buffer (must be a power of 2)
			 * @param o start offset of alignment block in D
			 **/
			static uint32_t getReadNameOffset(uint64_t const dn, uint64_t const o)
			{
				return (o+32)&(dn-1);
			}

			/**
			 * get FlagNC field from alignment block at offset o in ring buffer of length dn
			 *
			 * @param D ring buffer
			 * @param dn length of ring buffer (must be a power of 2)
			 * @param o start offset of alignment block in D
			 * @return decoded FlagNC field
			 **/
			static uint32_t getFlagNCWrapped  (
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return static_cast<uint32_t>(getLEIntegerWrapped(D,dn,(o+12)&(dn-1),4)); }

			/**
			 * get Flags field from alignment block at offset o in ring buffer of length dn
			 *
			 * @param D ring buffer
			 * @param dn length of ring buffer (must be a power of 2)
			 * @param o start offset of alignment block in D
			 * @return decoded Flags field
			 **/
			static uint32_t getFlagsWrapped(
				uint8_t const * D,
				uint64_t const dn,
				uint64_t const o
			) { return (getFlagNCWrapped(D,dn,o) >> 16) & 0xFFFF; }

			/**
			 * get reference id from alignment block D
			 *
			 * @param D alignment block
			 * @return reference id from D
			 **/
			static  int32_t     getRefID    (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+0,4)); }
			/**
			 * get position from alignment block D
			 *
			 * @param D alignment block
			 * @return position from D
			 **/
			static  int32_t     getPos      (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+4,4)); }
			/**
			 * get BinMQNL from alignment block D
			 *
			 * @param D alignment block
			 * @return BinMQNL from D
			 **/
			static uint32_t     getBinMQNL  (uint8_t const * D) { return static_cast<uint32_t>(getLEInteger(D+8,4)); }
			/**
			 * get FlagNC from alignment block D
			 *
			 * @param D alignment block
			 * @return FlagNC from D
			 **/
			static uint32_t     getFlagNC   (uint8_t const * D) { return static_cast<uint32_t>(getLEInteger(D+12,4)); }
			/**
			 * get length of sequence from alignment block D
			 *
			 * @param D alignment block
			 * @return length of sequence from D
			 **/
			static  int32_t     getLseq     (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+16,4)); }
			/**
			 * get reference id of next/mate from alignment block D
			 *
			 * @param D alignment block
			 * @return reference id of next/mate from D
			 **/
			static  int32_t     getNextRefID(uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+20,4)); }
			/**
			 * get position of next/mate from alignment block D
			 *
			 * @param D alignment block
			 * @return position of next/mate from D
			 **/
			static  int32_t     getNextPos  (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+24,4)); }
			/**
			 * get template length from alignment block D
			 *
			 * @param D alignment block
			 * @return template length from D
			 **/
			static  int32_t     getTlen     (uint8_t const * D) { return static_cast<int32_t>(getLEInteger(D+28,4)); }
			/**
			 * get read name from alignment block D
			 *
			 * @param D alignment block
			 * @return read name length from D
			 **/
			static char const * getReadName (uint8_t const * D) { return reinterpret_cast< char    const*>(D+32); }
			/**
			 * get read name from alignment block D
			 *
			 * @param D alignment block
			 * @return read name length from D
			 **/
			static char * getReadName (uint8_t * D) { return reinterpret_cast< char *>(D+32); }
			/**
			 * get bin from alignment block D
			 *
			 * @param D alignment block
			 * @return read name length from D
			 **/
			static uint32_t     getBin      (uint8_t const * D)
			{
				// bin flag stores part of cigar string length
				uint16_t const flags = getFlags(D);

				if ( expect_false(flags & LIBMAUS2_BAMBAM_FCIGAR32) )
				{
					if ( flags & LIBMAUS2_BAMBAM_FUNMAP )
					{
						if ( getPos(D) < 0 )
							return 4680; // reg2bin(-1,0)
						else
							return BamAlignmentReg2Bin::reg2bin(getPos(D),0);
					}
					else
						// compute bin from alignment data
						return computeBin(D);
				}
				else
				{
					// get bin from field
					return (getBinMQNL(D) >> 16) & 0xFFFFu;
				}
			}
			/**
			 * get mapping quality from alignment block D
			 *
			 * @param D alignment block
			 * @return mapping quality from D
			 **/
			static uint32_t     getMapQ     (uint8_t const * D) { return (getBinMQNL(D) >>  8) & 0xFFu; }
			/**
			 * get length of read name from alignment block D
			 *
			 * @param D alignment block
			 * @return length of read name from D
			 **/
			static uint32_t     getLReadName(uint8_t const * D) { return ((getBinMQNL(D) >>  0) & 0xFFu); }
			/**
			 * get read name from alignment block D as string object
			 *
			 * @param D alignment block
			 * @return read name from D as string object
			 **/
			static std::string  getReadNameS(uint8_t const * D) { char const * c = getReadName(D); return std::string(c,c+getLReadName(D)-1); }
			/**
			 * get encoded cigar string from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded cigar string from D
			 **/
			static uint8_t const * getCigar(uint8_t const * D) { return reinterpret_cast<uint8_t const *>(getReadName(D) + getLReadName(D)); }
			/**
			 * get encoded cigar string from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded cigar string from D
			 **/
			static uint8_t * getCigar(uint8_t * D) { return reinterpret_cast<uint8_t *>(getReadName(D) + getLReadName(D)); }
			/**
			 * get i'th encoded cigar field from alignment block D
			 *
			 * @param D alignment block
			 * @param i cigar operation index
			 * @return i'th encoded cigar field from D
			 **/
			static uint32_t getCigarField(uint8_t const * D, uint64_t const i) { return getLEInteger(getCigar(D)+4*i,4); }
			/**
			 * get length of i'th cigar operation from alignment block D
			 *
			 * @param D alignment block
			 * @param i cigar operation index
			 * @return length of i'th cigar operation from D
			 **/
			static uint32_t getCigarFieldLength(uint8_t const * D, uint64_t const i) {  return (getCigarField(D,i) >> 4) & ((1ull<<(32-4))-1); }
			/**
			 * get code of i'th cigar operation from alignment block D
			 *
			 * @param D alignment block
			 * @param i cigar operation index
			 * @return code of i'th cigar operation from D
			 **/
			static uint32_t getCigarFieldOp(uint8_t const * D, uint64_t const i) {  return (getCigarField(D,i) >> 0) & ((1ull<<(4))-1); }

			/**
			 * get a CIGAR vector decoder for an alignment block
			 *
			 * @param D alignment block
			 * @return cigar decoder
			 **/
			static CigarRunLengthDecoder getCigarRunLengthDecoder(uint8_t const * D)
			{
				return CigarRunLengthDecoder(getCigar(D),getNCigar(D));
			}

			static CigarDecoder getCigarDecoder(uint8_t const * D)
			{
				return CigarDecoder(CigarRunLengthDecoder(getCigar(D),getNCigar(D)));
			}

			/**
			 * get number of insertions(+)/padding(+)/deletions(-) before first matching/mismatching base
			 *
			 * @param D alignment block
			 * @return number of insertions
			 **/
			static int64_t getOffsetBeforeMatch(uint8_t const * D)
			{
				uint32_t const numops = getNCigar(D);

				uint8_t const * p = getCigar(D);
				int64_t icnt = 0;
				bool running = true;

				for ( uint64_t i = 0; running && i < numops; ++i, p += 4 )
				{
					uint32_t const lop = getLEInteger(p,4);
					uint32_t const len = lop >> 4;
					uint32_t const op = lop & ((1ul<<4)-1);

					switch ( op )
					{
						// insertion
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS:
							icnt += static_cast<int64_t>(len);
							break;
						// padding (silent insertion, base not in this read, so ignore)
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
							break;
						// deletion from reference
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						// reference/intron skip (deletion)
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
							icnt -= static_cast<int64_t>(len);
							break;
						// match/mismatch
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
							running = false;
							break;
					}
				}

				return icnt;
			}

			/**
			 * compute length of number in decimal notation
			 *
			 * @param n number
			 * @return length of n in decimal notation
			 **/
			static unsigned int getDecLength(uint64_t n)
			{
				if ( ! n )
					return 1;

				unsigned int l = 0;
				while ( n )
				{
					n /= 10;
					l += 1;
				}

				return l;
			}

			template<typename iterator>
			static iterator putDecimalNumber(uint64_t n, iterator p)
			{
				char c[22];
				char * ce = &c[sizeof(c)];
				char * cp = ce;

				if ( !n )
				{
					*(--cp) = '0';
				}
				else
				{
					while ( n )
					{
						*(--cp) = (n % 10)+'0';
						n /= 10;
					}
				}

				while ( cp != ce )
					*(p++) = *(cp++);

				return p;
			}

			/**
			 * compute length of cigar operation vector in string form
			 *
			 * @param D alignment block
			 * @return length of cigar operation vector in string form
			 **/
			static unsigned int getCigarStringLength(uint8_t const * D)
			{
				unsigned int l = 0;
				uint32_t const numops = getNCigar(D);

				uint8_t const * p = getCigar(D);

				for ( uint64_t i = 0; i < numops; ++i, p += 4 )
				{
					uint32_t const lop = getLEInteger(p,4);
					unsigned int const ll = lop >> 4;
					l += ( 1 + getDecLength(ll) );
				}

				return l;
			}

			template<typename iterator>
			static iterator getCigarString(uint8_t const * D, iterator c)
			{
				uint32_t const numops = getNCigar(D);
				uint8_t const * p = getCigar(D);

				for ( uint64_t i = 0; i < numops; ++i, p += 4 )
				{
					uint32_t const lop = getLEInteger(p,4);
					uint32_t const ll = lop >> 4;
					uint32_t const op = lop & ((1ul<<4)-1);
					c = putDecimalNumber(ll,c);
					*(c++) = cigarOpToChar(op);
				}

				*(c++) = 0;

				return c;
			}

			static void fillCigarHistogram(uint8_t const * D, uint64_t H[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES])
			{
				uint32_t const numops = getNCigar(D);
				uint8_t const * p = getCigar(D);

				for ( uint64_t i = 0; i < numops; ++i, p += 4 )
				{
					uint32_t const lop = getLEInteger(p,4);
					uint32_t const ll = lop >> 4;
					uint32_t const op = lop & ((1ul<<4)-1);
					H[op] += ll;
				}
			}

			/**
			 * put cigar string into string buffer
			 **/
			static size_t getCigarString(uint8_t const * D, libmaus2::autoarray::AutoArray<char> & A)
			{
				size_t const l = getCigarStringLength(D);

				if ( A.size() < l+1 )
					A.resize(l+1);

				getCigarString(D,A.begin());

				return l;
			}

			/**
			 * get cigar as string
			 **/
			static std::string getCigarAsString(uint8_t const * D)
			{
				libmaus2::autoarray::AutoArray<char> A;
				uint64_t l = getCigarString(D,A);
				return std::string(A.begin(),A.begin()+l);
			}

			/**
			 * get cigar operations vector
			 *
			 * @param D alignment block
			 * @param A array for storing vector
			 * @return number of cigar operations
			 **/
			static uint32_t getCigarOperations(uint8_t const * D,
				libmaus2::autoarray::AutoArray<cigar_operation> & cigop)
			{
				uint32_t const numops = getNCigar(D);

				if ( numops > cigop.size() )
					cigop = libmaus2::autoarray::AutoArray<cigar_operation>(numops,false);

				uint8_t const * p = getCigar(D);

				for ( uint64_t i = 0; i < numops; ++i, p += 4 )
				{
					uint32_t const lop = getLEInteger(p,4);
					cigop[i] = cigar_operation(lop & ((1ul<<4)-1),lop >> 4);
				}

				return numops;
			}

			/**
			 * get read interval covered by alignment in D
			 *
			 * @param D alignment block
			 * @return number of reference sequence bases covered by alignment in D
			 **/
			static libmaus2::math::IntegerInterval<int64_t> getCoveredReadInterval(uint8_t const * D)
			{
				int64_t const frontclipping = getFrontClipping(D);
				int64_t const backclipping = getBackClipping(D);
				int64_t const length = getReadLengthByCigar(D);
				int64_t const beg = frontclipping;
				int64_t const end = length - backclipping - 1;
				return libmaus2::math::IntegerInterval<int64_t>(beg,end);
			}

			/**
			 * get number of bytes before the encoded cigar string in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bytes before the encoded cigar string in D
			 **/
			static uint64_t getNumPreCigarBytes(uint8_t const * D)
			{
				return getCigar(D) - D;
			}
			/**
			 * get number of encoded cigar string bytes in alignment block D
			 *
			 * @param D alignment block
			 * @return number of encoded cigar string bytes in D
			 **/
			static uint64_t getNumCigarBytes(uint8_t const * D)
			{
				return getNCigar(D) * sizeof(uint32_t);
			}

			/**
			 * @return number of bytes after the cigar string in the alignment representation
			 **/
			static uint64_t getNumPostCigarBytes(uint8_t const * D, uint64_t const blocksize)
			{
				return blocksize - (getNumPreCigarBytes(D) + getNumCigarBytes(D));
			}


			/**
			 * get number of bytes before the encoded query sequence in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bytes before the encoded query sequence in D
			 **/
			static uint64_t getNumPreSeqBytes(uint8_t const * D)
			{
				return getSeq(D)-D;
			}

			/**
			 * get number of bytes before the encoded name in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bytes before the encoded name in D
			 **/
			static uint64_t getNumPreNameBytes(uint8_t const * D)
			{
				return reinterpret_cast<uint8_t const *>(getReadName(D))-D;
			}

			/**
			 * get number of encoded name bytes in alignment block D
			 *
			 * @param D alignment block
			 * @return number of encoded name bytes in D
			 **/
			static uint64_t getNumNameBytes(uint8_t const *D)
			{
				return getLReadName(D);
			}

			/**
			 * get number of encoded query sequence bytes in alignment block D
			 *
			 * @param D alignment block
			 * @return number of encoded query sequence bytes in D
			 **/
			static uint64_t getNumSeqBytes(uint8_t const * D)
			{
				return ( getLseq(D) + 1 ) / 2;
			}

			/**
			 * map encoded cigar operation i to the sam character representation
			 *
			 * @param i encoded cigar operation
			 * @return sam character representation of encoded cigar operation i
			 **/
			static char cigarOpToChar(uint32_t const i)
			{
				//                       012345678
				static char const C[] = "MIDNSHP=X";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;

				if ( i < bound )
					return C[i];
				else
					return '?';
			}

			/**
			 * get length of query sequence as encoded in the cigar string of the alignment block D
			 *
			 * @param D alignment block
			 * @return length of query sequence as encoded in the cigar string of D
			 **/
			static uint64_t getLseqByCigar(uint8_t const * D)
			{
				uint64_t seqlen = 0;
				uint64_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					// MIS=X
					switch ( op )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: // M
						case BamFlagBase::LIBMAUS2_BAMBAM_CINS: // I
						case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP: // S
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: // =
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: // X
							seqlen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}

				return seqlen;
			}

			/**
			 * get length of read sequence as encoded in the cigar string of the alignment block D
			 *
			 * @param D alignment block
			 * @return length of query sequence as encoded in the cigar string of D
			 **/
			static uint64_t getReadLengthByCigar(uint8_t const * D)
			{
				uint64_t seqlen = 0;
				uint64_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					// MISH=X
					switch ( op )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: // M
						case BamFlagBase::LIBMAUS2_BAMBAM_CINS: // I
						case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP: // S
						case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP: // S
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: // =
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: // X
							seqlen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}

				return seqlen;
			}

			/**
			 * get number of reference sequence bases covered by alignment in D
			 *
			 * @param D alignment block
			 * @return number of reference sequence bases covered by alignment in D
			 **/
			static uint64_t getReferenceLength(uint8_t const * D)
			{
				uint64_t reflen = 0;
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					switch ( op )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: // M
						case BamFlagBase::LIBMAUS2_BAMBAM_CDEL: // D
						case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP: // N
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: // =
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: // X
							reflen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}

				return reflen;
			}

			/**
			 * get number of reference sequence bases covered by alignment in D starting from first match/mismatch
			 *
			 * @param D alignment block
			 * @return number of reference sequence bases covered by alignment in D
			 **/
			static uint64_t getReferenceAdvance(uint8_t const * D)
			{
				uint64_t reflen = 0;
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				uint32_t i = 0;
				for ( ; i < ncigar ; cigar += 4, ++i )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if (
						op == BamFlagBase::LIBMAUS2_BAMBAM_CMATCH ||
						op == BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL ||
						op == BamFlagBase::LIBMAUS2_BAMBAM_CDIFF
					)
						break;
				}

				for ( ; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					switch ( op )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: // M
						case BamFlagBase::LIBMAUS2_BAMBAM_CDEL: // D
						case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP: // N
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: // =
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: // X
							reflen += (v >> 4) & ((1ull<<(32-4))-1);
							break;
					}
				}

				return reflen;
			}

			/**
			 * get number of reference sequence bases covered by alignment in D starting from first match/mismatch
			 *
			 * @param D alignment block
			 * @return number of reference sequence bases covered by alignment in D
			 **/
			static uint64_t getFrontDel(uint8_t const * D)
			{
				uint64_t frontdel = 0;
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);

				for ( uint64_t i = 0 ; i < ncigar ; cigar += 4, ++i )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if (
						op == BamFlagBase::LIBMAUS2_BAMBAM_CMATCH ||
						op == BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL ||
						op == BamFlagBase::LIBMAUS2_BAMBAM_CDIFF
					)
						break;
					else if (
						op == BamFlagBase::LIBMAUS2_BAMBAM_CDEL
					)
						frontdel += (v >> 4) & ((1ull<<(32-4))-1);
				}

				return frontdel;
			}

			/**
			 * get number of reference sequence bases covered by alignment described by cigar vector
			 *
			 * @param ita cigar vector start iterator
			 * @param ite cigar vector end iterator
			 * @return number of reference sequence bases covered by alignment in D
			 **/
			template<typename iterator>
			static uint64_t getReferenceLengthVector(iterator ita, iterator ite)
			{
				uint64_t reflen = 0;

				for ( ; ita != ite ; ++ita )
				{
					switch ( ita->first )
					{
						case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: // M
						case BamFlagBase::LIBMAUS2_BAMBAM_CDEL: // D
						case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP: // N
						case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: // =
						case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: // X
							reflen += ita->second;
							break;
					}
				}

				return reflen;
			}

			/**
			 * get number of bases clipped of the front of query sequence by cigar operations H or S in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bases clipped of the front of query sequence by cigar operations H or S in alignment block D
			 **/
			static uint64_t getFrontClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);
				uint64_t frontclip = 0;

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if ( op == BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP /* S */ || op == BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP /* H */ )
						frontclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					else
						break;
				}

				return frontclip;
			}


			/**
			 * get number of front clipped bases
			 *
			 * @param ita cigar vector start iterator
			 * @param ite cigar vector end iterator
			 * @return number of front clipped bases
			 **/
			template<typename iterator>
			static uint64_t getFrontClippingVector(iterator ita, iterator ite)
			{
				uint64_t clip = 0;

				for ( ; ita != ite ; ++ita )
				{
					uint32_t const op = ita->first;
					uint32_t const len = ita->second;

					if ( op == BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP /* S */ || op == BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP /* H */ )
						clip += len;
					else
						break;
				}

				return clip;
			}

			/**
			 * get number of bases clipped of the front of query sequence by cigar operations S in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bases clipped of the front of query sequence by cigar operations S in alignment block D
			 **/
			static uint64_t getFrontSoftClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D);
				uint64_t frontclip = 0;

				for ( uint32_t i = 0; i < ncigar; ++i, cigar+=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if (
						op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP
						||
						op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP
					)
					{
						if ( op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP )
							frontclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					}
					else
						break;
				}

				return frontclip;
			}

			/**
			 * get number of bases clipped of the back of the query sequence by cigar operations H or S in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bases clipped of the back of the query sequence by cigar operations H or S in alignment block D
			 **/
			static uint64_t getBackClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D) + 4*ncigar - 4;
				uint64_t backclip = 0;

				for ( uint32_t i = 0; i < ncigar; ++i, cigar-=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if ( op == BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP /* S */ || op == BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP /* H */ )
						backclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					else
						break;
				}

				return backclip;
			}


			/**
			 * get number of back clipped bases
			 *
			 * @param ita cigar vector start iterator
			 * @param ite cigar vector end iterator
			 * @return number of back clipped bases
			 **/
			template<typename iterator>
			static uint64_t getBackClippingVector(iterator ita, iterator ite)
			{
				uint64_t clip = 0;

				while ( ita != ite )
				{
					--ite;

					uint32_t const op = ite->first;
					uint32_t const len = ite->second;

					if ( op == BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP /* S */ || op == BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP /* H */ )
						clip += len;
					else
						break;
				}

				return clip;
			}

			/**
			 * get number of bases clipped of the back of the query sequence by cigar operations S in alignment block D
			 *
			 * @param D alignment block
			 * @return number of bases clipped of the back of the query sequence by cigar operations S in alignment block D
			 **/
			static uint64_t getBackSoftClipping(uint8_t const * D)
			{
				uint32_t const ncigar = getNCigar(D);
				uint8_t const * cigar = getCigar(D) + 4*ncigar - 4;
				uint64_t backclip = 0;

				for ( uint32_t i = 0; i < ncigar; ++i, cigar-=4 )
				{
					uint32_t const v = getLEInteger(cigar,4);
					uint8_t const op = v & ((1ull<<(4))-1);

					if (
						op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP
						||
						op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP
					)
					{
						if ( op == libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP )
							backclip += static_cast<int64_t>((v >> 4) & ((1ull<<(32-4))-1));
					}
					else
						break;
				}

				return backclip;
			}

			/**
			 * get first position of aligned base on reference (1 based coordinate)
			 *
			 * @param D alignment block
			 * @return first position of aligned base on reference (1 based coordinate)
			 **/
			static uint64_t getAlignmentStart(uint8_t const * D)
			{
				return getPos(D) + 1;
			}

			/**
			 * get first position of aligned base on reference (1 based coordinate)
			 *
			 * @param p pos
			 * @return first position of aligned base on reference (1 based coordinate)
			 **/
			static int64_t getAlignmentStart(int64_t const p)
			{
				return p+1;
			}

			/**
			 *
			 **/
			static int64_t getNextAlignmentStart(uint8_t const * D)
			{
				return getAlignmentStart(getNextPos(D));
			}

			/**
			 * get last position of aligned base on reference
			 *
			 * @param D alignment block
			 * @return last position of aligned base on reference
			 **/
			static uint64_t getAlignmentEnd(uint8_t const * D)
			{
				return getAlignmentStart(D) + getReferenceLength(D) - 1;
			}

			/**
			 * get last position of aligned base on reference
			 *
			 * @param p position
			 * @param ita cigar vector start
			 * @param ite cigar vector end
			 * @return last position of aligned base on reference
			 **/
			template<typename iterator>
			static int64_t getAlignmentEnd(int64_t const p, iterator ita, iterator ite)
			{
				return getAlignmentStart(p) + getReferenceLengthVector(ita,ite) - 1;
			}

			/**
			 * get end of next alignment
			 *
			 * @param D alignment block
			 * @param ita start of next cigar
			 * @param ite end of next cigar
			 * @return end of next alignment
			 **/
			template<typename iterator>
			static int64_t getNextAlignmentEnd(uint8_t const * D, iterator ita, iterator ite)
			{
				return getAlignmentEnd(getNextPos(D),ita,ite);
			}

			/**
			 * get alignment start minus front clipping in alignment block D
			 *
			 * @param D alignment block
			 * @return alignment start minus front clipping in alignment block D
			 **/
			static int64_t getUnclippedStart(uint8_t const * D)
			{
				return
					static_cast<int64_t>(getAlignmentStart(D)) -
					static_cast<int64_t>(getFrontClipping(D));
			}

			/**
			 * get unclipped start of next
			 *
			 * @param D alignment block
			 * @param ita start of next cigar
			 * @param ite end of next cigar
			 * @return unclipped start of next
			 **/
			template<typename iterator>
			static int64_t getNextUnclippedStart(uint8_t const * D, iterator ita, iterator ite)
			{
				return
					static_cast<int64_t>(getNextAlignmentStart(D)) -
					static_cast<int64_t>(getFrontClippingVector(ita,ite));
			}

			/**
			 * get alignment end plus back clipping in alignment block D
			 *
			 * @param D alignment block
			 * @return alignment end plus back clipping in alignment block D
			 **/
			static int64_t getUnclippedEnd(uint8_t const * D)
			{
				return
					static_cast<int64_t>(getAlignmentEnd(D)) +
					static_cast<int64_t>(getBackClipping(D));
			}

			/**
			 * get next unclipped end
			 **/
			template<typename iterator>
			static int64_t getNextUnclippedEnd(uint8_t const * D, iterator ita, iterator ite)
			{
				return
					static_cast<int64_t>(getNextAlignmentEnd(D,ita,ite))+
					static_cast<int64_t>(getBackClippingVector(ita,ite));
			}

			/**
			 * get coordinate (position of unclipped 5' end) from alignment block D
			 *
			 * @param D alignment block
			 * @return coordinate (position of unclipped 5' end) from D
			 **/
			static int64_t getCoordinate(uint8_t const * D)
			{
				if ( isReverse(getFlags(D)) )
				{
					return getUnclippedEnd(D);
				}
				else
				{
					return getUnclippedStart(D);
				}
			}

			/**
			 * get coordinate of next
			 *
			 * @param D alignment block
			 * @param ita start of next cigar
			 * @param ite end of next cigar
			 * @return coordinate of next
			 **/
			template<typename iterator>
			static int64_t getNextCoordinate(uint8_t const * D, iterator ita, iterator ite)
			{
				if ( isMateReverse(getFlags(D)) )
				{
					return getNextUnclippedEnd(D,ita,ite);
				}
				else
				{
					return getNextUnclippedStart(D,ita,ite);
				}
			}

			/**
			 *
			 **/
			static size_t getNextCigarVector(uint8_t const * D, uint64_t const blocksize, libmaus2::autoarray::AutoArray<cigar_operation> & Aop)
			{
				char const * MC = getAuxString(D,blocksize,"MC");

				if ( ! MC )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentDecoderBase::getNextCigarVector: MC aux field is not present" << std::endl;
					lme.finish();
					throw lme;
				}

				return libmaus2::bambam::CigarStringParser::parseCigarString(MC,Aop);
			}

			/**
			 *
			 **/
			static int64_t getNextCoordinate(uint8_t const * D, uint64_t const blocksize, libmaus2::autoarray::AutoArray<cigar_operation> & Aop)
			{
				char const * MC = getAuxString(D,blocksize,"MC");

				if ( ! MC )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentDecoderBase::getNextCoordinate: MC aux field is not present" << std::endl;
					lme.finish();
					throw lme;
				}

				size_t const numcigop = libmaus2::bambam::CigarStringParser::parseCigarString(MC,Aop);

				return getNextCoordinate(D,Aop.begin(),Aop.begin()+numcigop);
			}

			/**
			 * get operator of i'th cigar operation as character from alignment block D
			 *
			 * @param D alignment block
			 * @param i cigar operation index
			 * @return operator of i'th cigar operation as character from D
			 **/
			static char getCigarFieldOpAsChar(uint8_t const * D, uint64_t const i) { return cigarOpToChar(getCigarFieldOp(D,i)); }
			/**
			 * get flags from alignment block D
			 *
			 * @param D alignment block
			 * @return flags from D
			 **/
			static uint32_t     getFlags      (uint8_t const * D) { return (getFlagNC(D) >> 16) & 0xFFFFu; }
			/**
			 * get string representation of flags from alignment block D
			 *
			 * @param D alignment block
			 * @return string representation of flags from D
			 **/
			static std::string  getFlagsS     (uint8_t const * D) { return flagsToString(getFlags(D)); }
			/**
			 * get number of cigar operations from alignment block D
			 *
			 * @param D alignment block
			 * @return number of cigar operations from D
			 **/
			static uint32_t     getNCigar     (uint8_t const * D)
			{
				uint32_t const low = (getFlagNC(D) >>  0) & 0xFFFFu;

				if (
					expect_false
					(
					getFlags(D) & LIBMAUS2_BAMBAM_FCIGAR32
					)
				)
				{
					return (getBinMQNL(D) & 0xFFFF0000ul) | low;
				}
				else
				{
					return low;
				}
			}
			/**
			 * get decoded cigar string from alignment block D
			 *
			 * @param D alignment block
			 * @return decoded cigar string from D
			 **/
			static void getCigarString(std::ostream & ostr, uint8_t const * D)
			{
				for ( uint64_t i = 0; i < getNCigar(D); ++i )
					ostr << getCigarFieldLength(D,i) << getCigarFieldOpAsChar(D,i);
			}
			/**
			 * get decoded cigar string from alignment block D
			 *
			 * @param D alignment block
			 * @return decoded cigar string from D
			 **/
			static std::string getCigarString(uint8_t const * D)
			{
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < getNCigar(D); ++i )
					ostr << getCigarFieldLength(D,i) << getCigarFieldOpAsChar(D,i);
				return ostr.str();
			}
			/**
			 * get encoded query sequence from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded query sequence from D
			 **/
			static uint8_t const * getSeq(uint8_t const * D)
			{
				return getCigar(D) + 4 * getNCigar(D);
			}
			/**
			 * get encoded query sequence from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded query sequence from D
			 **/
			static uint8_t * getSeq(uint8_t * D)
			{
				return getCigar(D) + 4 * getNCigar(D);
			}
			/**
			 * get encoded quality string from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded quality string from D
			 **/
			static uint8_t const * getQual(uint8_t const * D)
			{
				return getSeq(D) + ((getLseq(D)+1)/2);
			}
			/**
			 * get encoded quality string from alignment block D
			 *
			 * @param D alignment block
			 * @return encoded quality string from D
			 **/
			static uint8_t * getQual(uint8_t * D)
			{
				return getSeq(D) + ((getLseq(D)+1)/2);
			}
			/**
			 * get start of auxiliary data from alignment block D
			 *
			 * @param D alignment block
			 * @return get start of auxiliary data from D
			 **/
			static uint8_t const * getAux(uint8_t const * D)
			{
				return getQual(D) + getLseq(D);
			}
			/**
			 * get start of auxiliary data from alignment block D
			 *
			 * @param D alignment block
			 * @return get start of auxiliary data from D
			 **/
			static uint8_t * getAux(uint8_t * D)
			{
				return getQual(D) + getLseq(D);
			}

			/** get score from alignment block D; the score is the sum of the base quality values not less 15
			 *
			 * @param D alignment block
			 * @return quality score
			 **/
			static uint64_t getScore(uint8_t const * D)
			{
				uint64_t score = 0;
				uint8_t const * rqual = getQual(D);
				uint8_t const * equal = rqual+getLseq(D);
				for ( uint8_t const * qual = rqual; qual != equal; ++qual )
					if ( *qual >= 15 )
						score += *qual;

				return score;
			}

			/**
			 * get length of primary auxiliary field for type c
			 *
			 * @param c aux field type
			 * @return length of primary field of type c
			 **/
			static uint64_t getPrimLengthByType(uint8_t const c)
			{
				switch ( c )
				{
					case 'A': case 'c': case 'C': return sizeof(int8_t);
					case 's': case 'S': return sizeof(int16_t);
					case 'i': case 'I': return sizeof(int32_t);
					case 'f':           return sizeof(float);
					default: return 0;
				}
			}

			/**
			 * get length of auxiliary field at D
			 *
			 * @param D encoded auxiliary field
			 * @return length of auxiliary field at D
			 **/
			static uint64_t getAuxLength(uint8_t const * D)
			{
				switch ( D[2] )
				{
					case 'A': case 'c': case 'C':
						return 2+1+sizeof(int8_t);
					case 's': case 'S':
						return 2+1+sizeof(int16_t);
					case 'i': case 'I':
						return 2+1+sizeof(int32_t);
					case 'f':
						return 2+1+sizeof(float);
					case 'Z':
					case 'H':
					{
						uint64_t len = 2+1;
						D += len;
						while ( *D )
							len++, D++;
						len++;
						return len;
					}
					case 'B':
					{
						uint8_t const eltype = D[3];
						uint32_t const numel = getLEInteger(D+4,4);
						return 2/*tag*/+1/*B*/+1/*type*/+4/* array length */+numel*getPrimLengthByType(eltype);
					}
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to handle type " << D[2] << "(" << static_cast<int>(D[2]) << ")" << " in BamAlignmentDecoderBase::getAuxLength()" << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			/**
			 * convert auxiliary field at D to a string
			 *
			 * @param D start of auxiliart field
			 * @return string representation of auxiliary field starting at D
			 **/
			static std::string auxValueToString(uint8_t const * D)
			{
				std::ostringstream ostr;

				// std::cerr << "\n[D] tag " << D[0] << D[1] << " type " << D[2] << std::endl;

				switch ( D[2] )
				{
					case 'A': return std::string(1,D[3]);
					case 'c':
					{
						int8_t const v = reinterpret_cast<int8_t const *>(D+3)[0];
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'C':
					{
						uint8_t const v = reinterpret_cast<uint8_t const *>(D+3)[0];
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 's':
					{
						int16_t const v = static_cast<int16_t>(getLEInteger(D+3,2));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'S':
					{
						uint16_t const v = static_cast<uint16_t>(getLEInteger(D+3,2));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'i':
					{
						int32_t const v = static_cast<int32_t>(getLEInteger(D+3,4));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'I':
					{
						uint32_t const v = static_cast<uint32_t>(getLEInteger(D+3,4));
						ostr << static_cast<int64_t>(v);
						break;
					}
					case 'f':
					{
						uint32_t const u = static_cast<uint32_t>(getLEInteger(D+3,4));
						union numberpun
						{
							float fvalue;
							uint32_t uvalue;
						};
						numberpun np;
						np.uvalue = u;
						float const v = np.fvalue;
						ostr << v;
						break;
					}
					case 'H':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr << *(p++);
						break;
					}
					case 'Z':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr << *(p++);
						break;
					}
					case 'B':
					{
						uint8_t const type = D[3];
						uint32_t const len = getLEInteger(D+4,4);
						ostr << type;

						uint8_t const * p = D+8;
						for ( uint64_t i = 0; i < len; ++i )
						{
							ostr << ",";
							switch ( type )
							{
								case 'A':
								{
									ostr << *p;
									p += 1;
									break;
								}
								case 'c':
								{
									int8_t const v = static_cast<int8_t>(getLEInteger(p,1));
									p += 1;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'C':
								{
									uint8_t const v = static_cast<uint8_t>(getLEInteger(p,1));
									p += 1;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 's':
								{
									int16_t const v = static_cast<int16_t>(getLEInteger(p,2));
									p += 2;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'S':
								{
									uint16_t const v = static_cast<uint16_t>(getLEInteger(p,2));
									p += 2;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'i':
								{
									int32_t const v = static_cast<int32_t>(getLEInteger(p,4));
									p += 4;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'I':
								{
									uint32_t const v = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;
									ostr << static_cast<int64_t>(v);
									break;
								}
								case 'f':
								{
									union numberpun
									{
										float fvalue;
										uint32_t uvalue;
									};
									numberpun np;
									np.uvalue = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;
									ostr << np.fvalue;
									break;
								}
							}
						}
						break;
					}
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to handle type " << D[2] << "(" << static_cast<int>(D[2]) << ")" << " in BamAlignmentDecoderBase::auxValueToString()" << std::endl;
						se.finish();
						throw se;
					}
				}

				return ostr.str();
			}

			/**
			 * convert auxiliary field at D to a string
			 *
			 * @param D start of auxiliart field
			 * @return string representation of auxiliary field starting at D
			 **/
			template<typename stream_type>
			static void auxValueToString(stream_type & ostr, uint8_t const * D)
			{
				// std::cerr << "\n[D] tag " << D[0] << D[1] << " type " << D[2] << std::endl;

				switch ( D[2] )
				{
					case 'A':
					{
						ostr.put(D[3]);
						break;
					}
					case 'c':
					{
						int8_t const v = reinterpret_cast<int8_t const *>(D+3)[0];

						if ( v < 0 )
						{
							ostr.put('-');
							printNumber16(ostr,static_cast<uint16_t>(-static_cast<int16_t>(v)));
						}
						else
						{
							printNumber16(ostr,v);
						}
						break;
					}
					case 'C':
					{
						uint8_t const v = reinterpret_cast<uint8_t const *>(D+3)[0];
						printNumber16(ostr,v);
						break;
					}
					case 's':
					{
						int16_t const v = static_cast<int16_t>(getLEInteger(D+3,2));

						if ( v < 0 )
						{
							ostr.put('-');
							printNumber16(ostr,static_cast<uint16_t>(-static_cast<int32_t>(v)));
						}
						else
						{
							printNumber16(ostr,v);
						}
						break;
					}
					case 'S':
					{
						uint16_t const v = static_cast<uint16_t>(getLEInteger(D+3,2));
						printNumber16(ostr,v);
						break;
					}
					case 'i':
					{
						int32_t const v = static_cast<int32_t>(getLEInteger(D+3,4));

						if ( v < 0 )
						{
							ostr.put('-');
							printNumber32(ostr,static_cast<uint32_t>(-static_cast<int64_t>(v)));
						}
						else
						{
							printNumber32(ostr,v);
						}

						break;
					}
					case 'I':
					{
						uint32_t const v = static_cast<uint32_t>(getLEInteger(D+3,4));
						printNumber32(ostr,v);
						break;
					}
					case 'f':
					{
						uint32_t const u = static_cast<uint32_t>(getLEInteger(D+3,4));
						union numberpun
						{
							float fvalue;
							uint32_t uvalue;
						};
						numberpun np;
						np.uvalue = u;
						float const v = np.fvalue;
						char fbuf[256];
						int const r = snprintf(&fbuf[0],sizeof(fbuf),"%f",v);

						if ( r < 0 || r >= static_cast<int>(sizeof(fbuf)) )
						{
							std::ostringstream fostr;
							fostr << v;
							std::string const s = fostr.str();
							ostr.write(s.c_str(),s.size());
						}
						else
						{
							ostr.write(&fbuf[0],r);
						}

						break;
					}
					case 'H':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr.put(*(p++));
						break;
					}
					case 'Z':
					{
						uint8_t const * p = D+3;
						while ( *p )
							ostr.put(*(p++));
						break;
					}
					case 'B':
					{
						uint8_t const type = D[3];
						uint32_t const len = getLEInteger(D+4,4);
						ostr.put(type);

						uint8_t const * p = D+8;
						for ( uint64_t i = 0; i < len; ++i )
						{
							ostr.put(',');
							switch ( type )
							{
								case 'A':
								{
									ostr.put(*p);
									p += 1;
									break;
								}
								case 'c':
								{
									int8_t const v = static_cast<int8_t>(getLEInteger(p,1));
									p += 1;

									if ( v < 0 )
									{
										ostr.put('-');
										printNumber16(ostr,-static_cast<int16_t>(v));
									}
									else
									{
										printNumber16(ostr,v);
									}

									break;
								}
								case 'C':
								{
									uint8_t const v = static_cast<uint8_t>(getLEInteger(p,1));
									p += 1;
									printNumber16(ostr,v);
									break;
								}
								case 's':
								{
									int16_t const v = static_cast<int16_t>(getLEInteger(p,2));
									p += 2;
									if ( v < 0 )
									{
										ostr.put('-');
										printNumber16(ostr,static_cast<uint16_t>(-static_cast<int32_t>(v)));
									}
									else
									{
										printNumber16(ostr,v);
									}

									break;
								}
								case 'S':
								{
									uint16_t const v = static_cast<uint16_t>(getLEInteger(p,2));
									p += 2;
									printNumber16(ostr,v);
									break;
								}
								case 'i':
								{
									int32_t const v = static_cast<int32_t>(getLEInteger(p,4));
									p += 4;

									if ( v < 0 )
									{
										ostr.put('-');
										printNumber32(ostr,static_cast<uint32_t>(-static_cast<int64_t>(v)));
									}
									else
									{
										printNumber32(ostr,v);
									}

									break;
								}
								case 'I':
								{
									uint32_t const v = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;
									printNumber32(ostr,v);
									break;
								}
								case 'f':
								{
									union numberpun
									{
										float fvalue;
										uint32_t uvalue;
									};
									numberpun np;
									np.uvalue = static_cast<uint32_t>(getLEInteger(p,4));
									p += 4;


									float const v = np.fvalue;
									char fbuf[256];
									int const r = snprintf(&fbuf[0],sizeof(fbuf),"%f",v);

									if ( r < 0 || r >= static_cast<int>(sizeof(fbuf)) )
									{
										std::ostringstream fostr;
										fostr << v;
										std::string const s = fostr.str();
										ostr.write(s.c_str(),s.size());
									}
									else
									{
										ostr.write(&fbuf[0],r);
									}


									break;
								}
							}
						}
						break;
					}
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to handle type " << D[2] << "(" << static_cast<int>(D[2]) << ")" << " in BamAlignmentDecoderBase::auxValueToString()" << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			static uint8_t rcEncoded(uint8_t const i)
			{
				static uint8_t const C[] = { 0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15 };
				return C[i];
			}

			/**
			 * decode encoded sequence symbol without applying boundary checks
			 *
			 * @param c encoded sequence symbol
			 * @return decoded symbol for c
			 **/
			static uint8_t decodeSymbolUnchecked(uint8_t const c)
			{
				static char const C[]     = "=ACMGRSVTWYHKDBN";
				return C[c];
			}

			/**
			 * decode encoded sequence symbol
			 *
			 * @param c encoded sequence symbol
			 * @return decoded symbol for c
			 **/
			static uint8_t decodeSymbol(uint8_t const c)
			{
				static char const C[]     = "=ACMGRSVTWYHKDBN";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;

				if ( c < bound )
					return C[c];
				else
					return '?';
			}

			/**
			 * decode reverse complement of encoded sequence symbol without applying boundary checks
			 *
			 * @param c encoded sequence symbol
			 * @return decoded symbol for c
			 **/
			static uint8_t decodeSymbolRCUnchecked(uint8_t const c)
			{
				static char const C[] = "=TGKCYSBAWRDMHVN";
				return C[c];
			}
			/**
			 * decode reverse complement of encoded sequence symbol
			 *
			 * @param c encoded sequence symbol
			 * @return decoded symbol for c
			 **/
			static uint8_t decodeSymbolRC(uint8_t const c)
			{
				static char const C[] = "=TGKCYSBAWRDMHVN";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;

				if ( c < bound )
					return C[c];
				else
					return '?';
			}

			/**
			 * replace the query sequence of length seqlen of alignment block D by its reverse complement in place
			 *
			 * @param D alignment block
			 * @param seqlen length of query sequence
			 **/
			static void reverseComplementInplace(uint8_t * D, uint64_t const seqlen)
			{
				if ( seqlen & 1 )
				{
					// point p at last used byte
					uint8_t * p = D + (seqlen>>1);

					while ( p != D )
					{
						uint8_t const back = (((*(p--)) >> 4) & 0xF); // e
						uint8_t const front = (*p &0xF); // d

						// swap the two and apply complement
						p[1] = (rcEncoded(back) << 4) | rcEncoded(front);
					}

					assert ( p == D );
					*p = rcEncoded( ((*p) >> 4) & 0xF) << 4;

					std::reverse(D, D + ((seqlen>>1)+1) );
				}
				else if ( seqlen )
				{
					uint8_t * f = D;
					uint8_t * r = D + ((seqlen>>1)-1);

					while ( f < r )
					{
						uint8_t const a = *(f);
						uint8_t const b = *(r);

						uint8_t const t = rcEncoded((a >> 4)&0xF) | (rcEncoded(a      &0xF)<<4);

						*(f++) = rcEncoded((b >> 4)&0xF) | (rcEncoded(b      &0xF)<<4);
						*(r--) = t;
					}

					if ( f == r )
					{
						uint8_t const a = *f;

						*f =
							rcEncoded((a >> 4)&0xF)
							|
							(rcEncoded(a      &0xF)<<4);
					}
				}
			}

			/**
			 * decode encoded sequence of length seqlen to iterator S
			 *
			 * @param D alignment block
			 * @param S output iterator
			 * @param seqlen length of query sequence
			 * @return iterator after decoding
			 **/
			template<typename iterator>
			static iterator decodeRead(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * cseq = getSeq(D);

				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(S++) = decodeSymbolUnchecked((c >> 4) & 0xF);
					*(S++) = decodeSymbolUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(S++) = decodeSymbolUnchecked((c >> 4) & 0xF);
				}

				return S;
			}
			/**
			 * decode reverse complement of encoded sequence of length seqlen to iterator S
			 *
			 * @param D alignment block
			 * @param S output iterator
			 * @param seqlen length of query sequence
			 * @return iterator after decoding
			 **/
			template<typename iterator>
			static uint64_t decodeReadRC(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * cseq = getSeq(D);
				S += seqlen;

				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
					*(--S) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}

				return seqlen;
			}
			/**
			 * decode encoded sequence of length seqlen to iterator S; this version uses S at a forward iterator only
			 *
			 * @param D alignment block
			 * @param S output iterator
			 * @param seqlen length of query sequence
			 * @return iterator after decoding
			 **/
			template<typename iterator>
			static iterator decodeReadRCIt(uint8_t const * D, iterator S, uint64_t const seqlen)
			{
				uint8_t const * const cseqe = getSeq(D);
				uint8_t const *       cseq  = cseqe + ((seqlen+1)/2);

				if ( seqlen & 1 )
					*(S++) = decodeSymbolRCUnchecked( ((*(--cseq)) >> 4) & 0xF );
				while ( cseq != cseqe )
				{
					uint8_t const c = *(--cseq);
					*(S++) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
					*(S++) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}

				return S;
			}
			/**
			 * decode query sequence in alignment block D to array A; A is extended if it is too small
			 *
			 * @param D alignment block
			 * @param A output array
			 * @return length of decoded query sequence
			 **/
			static uint64_t decodeRead(uint8_t const * D, ::libmaus2::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus2::autoarray::AutoArray<char>(seqlen);
				char * S = A.begin();

				decodeRead(D,S,seqlen);

				return seqlen;
			}
			/**
			 * decode query sequence in alignment block D to array A; A is extended if it is too small;
			 * the query sequence is returned as a string object
			 *
			 * @param D alignment block
			 * @param A output array
			 * @return string object containing the decode query sequence
			 **/
			static std::string decodeReadS(uint8_t const * D, ::libmaus2::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = decodeRead(D,A);
				return std::string(A.begin(),A.begin()+seqlen);
			}
			/**
			 * decode reverse complement of query sequence in alignment block D to array A; A is extended if it is too small;
			 * the decoded sequence is returned as a string object
			 *
			 * @param D alignment block
			 * @param A output array
			 * @return string object containing the reverse complement of the decoded query sequence
			 **/
			static uint64_t decodeReadRC(uint8_t const * D, ::libmaus2::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus2::autoarray::AutoArray<char>(seqlen);
				uint8_t const * cseq = getSeq(D);
				char * S = A.begin()+seqlen;
				for ( uint64_t i = 0; i < (seqlen>>1); ++i )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
					*(--S) = decodeSymbolRCUnchecked((c >> 0) & 0xF);
				}
				if ( seqlen & 1 )
				{
					uint8_t const c = *(cseq++);
					*(--S) = decodeSymbolRCUnchecked((c >> 4) & 0xF);
				}

				return seqlen;
			}
			/**
			 * decode quality string to array A; A is reallocated if it is too small
			 *
			 * @param D alignment block
			 * @param A output array
			 * @return length of quality string
			 **/
			static uint64_t decodeQual(uint8_t const * D, ::libmaus2::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus2::autoarray::AutoArray<char>(seqlen);
				uint8_t const * qual = getQual(D);
				char * S = A.begin();
				for ( uint64_t i = 0; i < seqlen; ++i )
				{
					*(S++) = (*(qual++))+33;
				}
				return seqlen;
			}

			/**
			 * decode quality string of length seqlen to output iterator it
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @param seqlen length of quality string
			 * @return output iterator after decoding
			 **/
			template<typename iterator>
			static iterator decodeQualIt(uint8_t const * D, iterator it, unsigned int const seqlen)
			{
				if ( seqlen )
				{
					uint8_t const * qual = getQual(D);
					uint8_t const * const quale = qual + seqlen;

					if ( qual[0] == 255 )
					{
						while ( qual++ != quale )
							*it++ = '*';
					}
					else
					{
						while ( qual != quale )
							*it++ = (*(qual++))+ 33;
					}
				}

				return it;
			}

			/**
			 * decode reverse quality string of length seqlen to output iterator it
			 *
			 * @param D alignment block
			 * @param it output iterator
			 * @param seqlen length of quality string
			 * @return output iterator after decoding
			 **/
			template<typename iterator>
			static iterator decodeQualRcIt(uint8_t const * D, iterator it, unsigned int const seqlen)
			{
				if ( seqlen )
				{
					uint8_t const * qual  = getQual(D) + seqlen;
					uint8_t const * const quala = getQual(D);

					if ( qual[0] == 255 )
					{
						while ( qual-- != quala )
							*it++ = '*';
					}
					else
					{
						while ( qual != quala )
							*it++ = (*(--qual))+ 33;
					}
				}

				return it;
			}

			/**
			 * decode quality string from alignment block D to a string
			 *
			 * @param D alignment block
			 * @return decoded quality string
			 **/
			static std::string decodeQual(uint8_t const * D)
			{
				uint64_t const seqlen = getLseq(D);
				uint8_t const * qual = getQual(D);
				::std::string s(reinterpret_cast<char const *>(qual),reinterpret_cast<char const *>(qual+seqlen));
				for ( uint64_t i = 0; i < seqlen; ++i )
					s[i] += 33;
				return s;
			}

			/**
			 * decode reverse quality string from alignment block D to array A; array A is extended if it is too small
			 *
			 * @param D alignment block
			 * @param A output array
			 * @return length of decoded quality string
			 **/
			static uint64_t decodeQualRC(uint8_t const * D, ::libmaus2::autoarray::AutoArray<char> & A)
			{
				uint64_t const seqlen = getLseq(D);
				if ( seqlen > A.size() )
					A = ::libmaus2::autoarray::AutoArray<char>(seqlen);
				uint8_t const * qual = getQual(D);
				char * S = A.begin() + seqlen;
				for ( uint64_t i = 0; i < seqlen; ++i )
				{
					*(--S) = (*(qual++))+33;
				}
				return seqlen;
			}

			/**
			 * get auxiliary area in alignment block E of size blocksize for given tag
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @return pointer to aux area or null pointer if tag is not present
			 **/
			static uint8_t const * getAux(
				uint8_t const * E, uint64_t const blocksize,
				char const * const tag
			)
			{
				assert ( tag );
				assert ( tag[0] );
				assert ( tag[1] );
				assert ( ! tag[2] );

				uint8_t const * aux = getAux(E);

				while ( aux < E+blocksize && *aux )
				{
					if ( aux[0] == tag[0] && aux[1] == tag[1] )
						return aux;

					aux = aux + getAuxLength(aux);
				}

				return 0;
			}

			/**
			 * enumerate aux tags in array A; A will be resized if needed
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param A array for storing aux tag markers
			 * @return number of markers stored
			 **/
			static uint64_t enumerateAuxTags(
				uint8_t const * E, uint64_t const blocksize,
				libmaus2::autoarray::AutoArray < std::pair<uint8_t,uint8_t> > & A
			)
			{
				uint8_t const * aux = getAux(E);
				uint64_t cnt = 0;

				while ( aux < E+blocksize )
				{
					if ( cnt == A.size() )
						A.resize(std::max(static_cast<uint64_t>(1),2*A.size()));

					assert ( cnt < A.size() );

					A[cnt++] = std::pair<uint8_t,uint8_t>(aux[0],aux[1]);

					aux = aux + getAuxLength(aux);
				}

				return cnt;
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
			 * get auxiliary area in alignment block E of size blocksize for given tag as number
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @return number
			 **/
			template<typename N>
			static N getAuxAsNumber(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * p = getAux(E,blocksize,tag);

				if ( ! p )
				{
					libmaus2::exception::LibMausException se;
					se.getStream()
						<< "BamAlignmentDecoderBase::getAuxAsNumber called non present tag " << tag
						<< " for read "
						<< getReadName(E)
						<< std::endl;
					se.finish();
					throw se;
				}

				switch ( p[2] )
				{
					case 'A': return static_cast<N>(static_cast<int8_t>(p[3]));
					case 'c': return static_cast<N>(static_cast<int8_t>(p[3]));
					case 'C': return static_cast<N>(static_cast<uint8_t>(p[3]));
					case 's':
						return static_cast<N>(
							static_cast<int16_t>(
								(static_cast<uint16_t>(p[3])<< 0) |
								(static_cast<uint16_t>(p[4])<< 8)
							)
						);
					case 'S':
						return static_cast<N>(
							static_cast<uint16_t>(
								(static_cast<uint16_t>(p[3])<< 0) |
								(static_cast<uint16_t>(p[4])<< 8)
							)
						);
					case 'i':
						return static_cast<N>(
							static_cast<int32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							)
						);
					case 'I':
						return static_cast<N>(
							static_cast<uint32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							)
						);
					case 'f':
					{
						uint32_t const v =
							static_cast<uint32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							);
						numberpun np;
						np.uvalue = v;
						return np.fvalue;
					}
					default:
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "Unknown type of number " << p[2] << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			/**
			 * get auxiliary area in alignment block E of size blocksize for given tag as array of numbers
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @return numbers
			 **/
			template<typename N>
			static std::vector<N> getAuxAsNumberArray(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * p = getAux(E,blocksize,tag);

				if ( ! p )
				{
					libmaus2::exception::LibMausException se;
					se.getStream()
						<< "BamAlignmentDecoderBase::getAuxAsNumberArray called non present tag " << tag
						<< " for read "
						<< getReadName(E)
						<< std::endl;
					se.finish();
					throw se;
				}

				if ( p[2] != 'B' )
				{
					libmaus2::exception::LibMausException se;
					se.getStream()
						<< "BamAlignmentDecoderBase::getAuxAsNumberArray called for non array field " << tag
						<< " for read "
						<< getReadName(E)
						<< std::endl;
					se.finish();
					throw se;
				}

				uint32_t const l =
					(static_cast<uint32_t>(p[4]) << 0) |
					(static_cast<uint32_t>(p[5]) << 8) |
					(static_cast<uint32_t>(p[6]) << 16) |
					(static_cast<uint32_t>(p[7]) << 24);

				std::vector<N> V(l);

				switch ( p[3] )
				{
					case 'c':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 1 )
							V.push_back(static_cast<int8_t>(*q));
					}
					case 'C':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 1 )
							V.push_back(static_cast<uint8_t>(*q));
					}
					case 's':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 2 )
							V.push_back(
								static_cast<int16_t>(
									(static_cast<uint16_t>(q[0]) << 0) |
									(static_cast<uint16_t>(q[1]) << 8)
								)
							);
					}
					case 'S':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 2 )
							V.push_back(
								static_cast<uint16_t>(
									(static_cast<uint16_t>(q[0]) << 0) |
									(static_cast<uint16_t>(q[1]) << 8)
								)
							);
					}
					case 'i':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 4 )
							V.push_back(
								static_cast<int32_t>(
									(static_cast<uint32_t>(q[0]) << 0) |
									(static_cast<uint32_t>(q[1]) << 8) |
									(static_cast<uint32_t>(q[2]) << 16) |
									(static_cast<uint32_t>(q[3]) << 24)
								)
							);
					}
					case 'I':
					{
						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 4 )
							V.push_back(
								static_cast<uint32_t>(
									(static_cast<uint32_t>(q[0]) << 0) |
									(static_cast<uint32_t>(q[1]) << 8) |
									(static_cast<uint32_t>(q[2]) << 16) |
									(static_cast<uint32_t>(q[3]) << 24)
								)
							);
					}
					case 'f':
					{

						uint8_t const * q = p + 8;

						for ( uint64_t i = 0; i < l; ++i, q += 4 )
						{
							uint32_t const v =
								static_cast<uint32_t>(
									(static_cast<uint32_t>(q[0])<< 0) |
									(static_cast<uint32_t>(q[1])<< 8) |
									(static_cast<uint32_t>(q[2])<<16) |
									(static_cast<uint32_t>(q[3])<<24)
								);
							numberpun np;
							np.uvalue = v;
							V.push_back ( np.fvalue );
						}
					}
					default:
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "getAuxAsNumberArray: Unknown type of number " << p[3] << std::endl;
						se.finish();
						throw se;
					}
				}

				return V;
			}

			/**
			 * get auxiliary area in alignment block E of size blocksize for given tag as number
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @param num place to store number
			 * @return true if field is present
			 **/
			template<typename N>
			static bool getAuxAsNumber(
				uint8_t const * E, uint64_t const blocksize, char const * const tag, N & num
			)
			{
				uint8_t const * p = getAux(E,blocksize,tag);

				// field is not present
				if ( ! p )
					return false;

				switch ( p[2] )
				{
					case 'A':
						num = static_cast<N>(static_cast<int8_t>(p[3]));
						return true;
					case 'c':
						num = static_cast<N>(static_cast<int8_t>(p[3]));
						return true;
					case 'C':
						num = static_cast<N>(static_cast<uint8_t>(p[3]));
						return true;
					case 's':
						num = static_cast<N>(
							static_cast<int16_t>(
								(static_cast<uint16_t>(p[3])<< 0) |
								(static_cast<uint16_t>(p[4])<< 8)
							)
						);
						return true;
					case 'S':
						num = static_cast<N>(
							static_cast<uint16_t>(
								(static_cast<uint16_t>(p[3])<< 0) |
								(static_cast<uint16_t>(p[4])<< 8)
							)
						);
						return true;
					case 'i':
						num = static_cast<N>(
							static_cast<int32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							)
						);
						return true;
					case 'I':
						num = static_cast<N>(
							static_cast<uint32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							)
						);
						return true;
					case 'f':
					{
						uint32_t const v =
							static_cast<uint32_t>(
								(static_cast<uint32_t>(p[3])<< 0) |
								(static_cast<uint32_t>(p[4])<< 8) |
								(static_cast<uint32_t>(p[5])<<16) |
								(static_cast<uint32_t>(p[6])<<24)
							);
						numberpun np;
						np.uvalue = v;
						num = np.fvalue;
						return true;
					}
					default:
					{
						return false;
					}
				}
			}

			/**
			 * get auxiliary area in alignment block E of size blocksize for given tag as number
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @return pointer to aux area or null pointer if tag is not present
			 **/

			/**
			 * sort aux fields by tag id
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param sortbuffer buffer for sorting
			 **/
			static void sortAux(uint8_t * E, uint64_t const blocksize, BamAuxSortingBuffer & sortbuffer)
			{
				// reset sort buffer
				sortbuffer.reset();

				// pointer to auxiliary area
				uint8_t * aux = getAux(E);
				// start pointer of aux area
				uint8_t * const auxa = aux;

				// scane aux block
				while ( aux < E+blocksize )
				{
					// length of this aux field in bytes
					uint64_t const auxlen = getAuxLength(aux);
					// push entry
					sortbuffer.push(BamAuxSortingBufferEntry(aux[0],aux[1],aux-auxa,auxlen));
					// go to next tag
					aux = aux + auxlen;
				}

				// sort entries by name
				sortbuffer.sort();

				// total number of aux bytes
				uint64_t const auxtotal = aux-auxa;

				// check buffer size
				if ( auxtotal > sortbuffer.U.size() )
					sortbuffer.U = libmaus2::autoarray::AutoArray<uint8_t>(auxtotal,false);

				// reorder aux blocks
				uint8_t * outp = sortbuffer.U.begin();
				for ( uint64_t i = 0; i < sortbuffer.fill; ++i )
				{
					BamAuxSortingBufferEntry const & entry = sortbuffer.B[i];
					uint64_t const auxoff = entry.offset;
					uint64_t const auxlen = entry.length;

					std::copy(auxa + auxoff, auxa + auxoff + auxlen, outp);

					outp += auxlen;
				}

				std::copy(sortbuffer.U.begin(),sortbuffer.U.begin()+auxtotal,auxa);
			}
			/**
			 * filter auxiliary tags keeping only those in a given list
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag list of tag identifiers to be kept (null terminated)
			 * @return updated (reduced) block size
			 **/
			static uint64_t filterAux(
				uint8_t * E, uint64_t const blocksize,
				BamAuxFilterVector const & tags
			)
			{
				// pointer to auxiliary area
				uint8_t * aux = getAux(E);
				// offset from start of block (length of pre aux data)
				uint64_t auxoff = (aux - E);
				// output pointer
				uint8_t * outp = aux;
				// size of reduced aux area
				uint64_t auxsize = 0;

				while ( aux < E+blocksize )
				{
					// search for current tag
					bool const tagvalid = tags(aux[0],aux[1]);

					// length of this aux field in bytes
					uint64_t const auxlen = getAuxLength(aux);

					// copy data if tag is valid
					if ( tagvalid )
					{
						if ( aux != outp )
							std::copy(aux,aux+auxlen,outp);

						outp += auxlen;
						auxsize += auxlen;
					}

					// go to next tag
					aux = aux + auxlen;
				}

				// return updated block size
				return auxoff + auxsize;
			}

			/**
			 * erase all aux tags
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @return updated (reduced) block size
			 **/
			static uint64_t eraseAux(uint8_t * E)
			{
				// return updated block size
				return getAux(E)-E;
			}

			/**
			 * filter out auxiliary tags in the given list
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag list of tag identifiers to be kept (null terminated)
			 * @return updated (reduced) block size
			 **/
			static uint64_t filterOutAux(
				uint8_t * E, uint64_t const blocksize,
				BamAuxFilterVector const & tags
			)
			{
				// pointer to auxiliary area
				uint8_t * aux = getAux(E);
				// offset from start of block (length of pre aux data)
				uint64_t auxoff = (aux - E);
				// output pointer
				uint8_t * outp = aux;
				// size of reduced aux area
				uint64_t auxsize = 0;

				while ( aux < E+blocksize )
				{
					// search for current tag
					bool const tagvalid = !tags(aux[0],aux[1]);

					// length of this aux field in bytes
					uint64_t const auxlen = getAuxLength(aux);

					// copy data if tag is valid
					if ( tagvalid )
					{
						if ( aux != outp )
							std::copy(aux,aux+auxlen,outp);

						outp += auxlen;
						auxsize += auxlen;
					}

					// go to next tag
					aux = aux + auxlen;
				}

				// return updated block size
				return auxoff + auxsize;
			}

			/**
			 * get auxiliary area in alignment block E of size blocksize for given tag as a string
			 * if it is of type Z; returns null pointer if tag is not present or not of type Z
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag two byte aux tag identifier
			 * @return pointer to aux area data as string or null pointer if tag is not present or is not of type Z
			 **/
			static char const * getAuxString(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * data = getAux(E,blocksize,tag);

				if ( data && data[2] == 'Z' )
					return reinterpret_cast<char const *>(data+3);
				else
					return 0;
			}

			/**
			 * get aux area ZR decoded as rank
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @return aux area for tag ZR decoded as rank (or -1 if invalid)
			 **/
			static int64_t getRank(uint8_t const * E, uint64_t const blocksize)
			{
				uint8_t const * p = getAux(E,blocksize,"ZR");

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
			 * get aux area ZR
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @return aux area for tag ZR or null pointer if not present
			 **/
			static uint8_t const * getAuxRankData(
				uint8_t const * E, uint64_t const blocksize
			)
			{
				return getAux(E,blocksize,"ZR");
			}

			/**
			 * get aux area ZR (rank) decoded as eight byte number
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @return decoded rank field
			 **/
			static uint64_t getAuxRank(uint8_t const * E, uint64_t const blocksize)
			{
				uint8_t const * data = getAuxRankData(E,blocksize);

				if ( ! data )
					return 0;

				assert ( data[0] == 'Z' );
				assert ( data[1] == 'R' );

				if ( data[2] != 'B' )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong data type " << data[2] << std::endl;
					se.finish();
					throw se;
				}
				if ( data[3] != 'C' )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong data type " << data[3] << std::endl;
					se.finish();
					throw se;
				}
				if ( data[4] != 8 || data[5] != 0 || data[6] != 0 || data[7] != 0 )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Rank aux tag ZR has wrong array size" << std::endl;
					se.finish();
					throw se;
				}

				return
					(static_cast<uint64_t>(data[8]) << 56) |
					(static_cast<uint64_t>(data[9]) << 48) |
					(static_cast<uint64_t>(data[10]) << 40) |
					(static_cast<uint64_t>(data[11]) << 32) |
					(static_cast<uint64_t>(data[12]) << 24) |
					(static_cast<uint64_t>(data[13]) << 16) |
					(static_cast<uint64_t>(data[14]) <<  8) |
					(static_cast<uint64_t>(data[15]) <<  0);
			}

			/**
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @return contents of the RG aux field (null pointer if not present)
			 **/
			static char const * getReadGroup(uint8_t const * E, uint64_t const blocksize)
			{
				return getAuxString(E,blocksize,"RG");
			}

			/**
			 * get aux value for tag from alignment block E of size blocksize as string
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param tag aux tag id
			 * @return decoded value of aux field for tag as string object
			 **/
			static std::string getAuxAsString(uint8_t const * E, uint64_t const blocksize, char const * const tag)
			{
				uint8_t const * D = getAux(E,blocksize,tag);

				if ( D )
					return auxValueToString(D);
				else
					return std::string();
			}

			template<typename stream_type>
			static void printNumber16(stream_type & stream, uint16_t v)
			{
				char pp[5];
				char * pqe = &pp[5];
				char * pq = pqe;

				do
				{
					*(--pq) = (v % 10)+'0';
					v /= 10;
				} while ( v );

				stream.write(pq,pqe-pq);
			}

			template<typename stream_type>
			static void printNumber32(stream_type & stream, uint32_t v)
			{
				char pp[10];
				char * pqe = &pp[10];
				char * pq = pqe;

				do
				{
					*(--pq) = (v % 10)+'0';
					v /= 10;
				} while ( v );

				stream.write(pq,pqe-pq);
			}

			template<typename stream_type>
			static void printNumberSigned(stream_type & stream, int64_t v)
			{
				char pp[22];
				char * pqe = &pp[22];
				char * pq = pqe;

				bool const sign = v < 0;
				if ( sign )
					v = -v;

				do
				{
					*(--pq) = (v % 10)+'0';
					v /= 10;
				} while ( v );

				if ( sign )
					*(--pq) = '-';

				stream.write(pq,pqe-pq);
			}

			template<typename stream_type>
			static void printAuxBlock(
				stream_type & ostr, uint8_t const * E, uint64_t const blocksize
			)
			{
				uint8_t const * aux = getAux(E);
				uint8_t const * const auxe = E+blocksize;

				while ( aux < auxe && *aux )
				{
					ostr.put('\t');
					ostr.put(aux[0]);
					ostr.put(aux[1]);
					ostr.put(':');
					switch ( aux[2] )
					{
						case 'c':
						case 'C':
						case 's':
						case 'S':
						case 'i':
						case 'I': ostr.put('i'); break;
						default: ostr.put(aux[2]); break;
					}
					ostr.put(':');
					auxValueToString(ostr,aux);
					aux = aux + getAuxLength(aux);
				}
			}

			template<typename stream_type>
			static void printAuxBlock(
				stream_type & ostr, uint8_t const * E, uint64_t const blocksize, BamAuxFilterVector const & tags,
				bool indent = false
			)
			{
				uint8_t const * aux = getAux(E);
				uint8_t const * const auxe = E+blocksize;

				while ( aux < auxe && *aux )
				{
					if ( tags(aux[0],aux[1]) )
					{
						if ( indent )
							ostr.put('\t');
						else
							indent = true;

						ostr.put(aux[0]);
						ostr.put(aux[1]);
						ostr.put(':');
						switch ( aux[2] )
						{
							case 'c':
							case 'C':
							case 's':
							case 'S':
							case 'i':
							case 'I': ostr.put('i'); break;
							default: ostr.put(aux[2]); break;
						}
						ostr.put(':');
						auxValueToString(ostr,aux);
					}
					aux = aux + getAuxLength(aux);
				}
			}
			/**
			 * format alignment block E of size blocksize as SAM file line using the header
			 * and the temporary memory block auxdata
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param bam header
			 * @param auxdata temporary memory block
			 * @return alignment block formatted as SAM line stored in string object
			 **/
			template<typename header_type>
			static void formatAlignment(
				std::ostream & ostr,
				uint8_t const * E,
				uint64_t const blocksize,
				header_type const & header,
				::libmaus2::bambam::BamFormatAuxiliary & auxdata
			)
			{
				uint16_t const flags = getFlags(E);

				// put read name
				char const * rn = getReadName(E);
				size_t const lrn = getLReadName(E)-1;
				ostr.write(rn,lrn);
				ostr.put('\t');

				printNumber16(ostr,flags);
				ostr.put('\t');

				char const * refidname = header.getRefIDName(getRefID(E));
				size_t const refidnamelen = strlen(refidname);
				ostr.write(refidname,refidnamelen);
				ostr.put('\t');

				printNumber32(ostr,getPos(E)+1);
				ostr.put('\t');

				printNumber16(ostr,getMapQ(E));
				ostr.put('\t');

				if ( (! getNCigar(E)) )
				{
					ostr.put('*');
				}
				else
				{
					getCigarString(ostr,E);
				}
				ostr.put('\t');

				if ( getRefID(E) == getNextRefID(E) && getRefID(E) >= 0 )
					ostr.put('=');
				else
				{
					char const * nextrefid = header.getRefIDName(getNextRefID(E));
					size_t const nextrefidlen = strlen(nextrefid);
					ostr.write(nextrefid,nextrefidlen);
				}

				ostr.put('\t');

				printNumber32(ostr,getNextPos(E)+1);
				ostr.put('\t');

				printNumberSigned(ostr,getTlen(E));
				ostr.put('\t');

				uint64_t const seqlen = decodeRead(E,auxdata.seq);
				if ( seqlen )
					ostr.write(auxdata.seq.begin(),seqlen);
				else
					ostr.put('*');
				ostr.put('\t');

				if ( seqlen && getQual(E)[0] == 255 )
				{
					ostr.put('*');
				}
				else
				{
					uint64_t const quallen = decodeQual(E,auxdata.qual);
					ostr.write(auxdata.qual.begin(),quallen);
				}

				printAuxBlock(ostr,E,blocksize);
			}

			/**
			 * format alignment block E of size blocksize as SAM file line using the header
			 * and the temporary memory block auxdata
			 *
			 * @param E alignment block
			 * @param blocksize size of alignment block
			 * @param bam header
			 * @param auxdata temporary memory block
			 * @return alignment block formatted as SAM line stored in string object
			 **/
			template<typename header_type>
			static std::string formatAlignment(
				uint8_t const * E, uint64_t const blocksize,
				header_type const & header,
				::libmaus2::bambam::BamFormatAuxiliary & auxdata
			)
			{
				std::ostringstream ostr;
				formatAlignment(ostr,E,blocksize,header,auxdata);
				return ostr.str();
			}

			/**
			 * format alignment block E as FastQ
			 *
			 * @param E alignment block
			 * @param auxdata temporary memory block
			 * @return alignment block formatted as FastQ
			 **/
			static std::string formatFastq(
				uint8_t const * E,
				::libmaus2::bambam::BamFormatAuxiliary & auxdata
			)
			{
				std::ostringstream ostr;

				uint32_t const flags = getFlags(E);

				// name
				ostr << "@" << getReadNameS(E);

				if ( isPaired(flags) )
				{
					if ( isRead1(flags) )
						ostr << "/1";
					else
						ostr << "/2";
				}

				ostr.put('\n');

				uint64_t const seqlen = isReverse(flags) ? decodeReadRC(E,auxdata.seq) : decodeRead(E,auxdata.seq);
				std::string const seqs(auxdata.seq.begin(),auxdata.seq.begin()+seqlen);
				ostr << seqs << "\n";

				ostr << "+\n";

				uint64_t const quallen = isReverse(flags) ? decodeQualRC(E,auxdata.qual) : decodeQual(E,auxdata.qual);
				std::string const squal(auxdata.qual.begin(),auxdata.qual.begin()+quallen);
				ostr << squal << "\n";

				return ostr.str();
			}

			static void getCigarStats(uint8_t const * B, libmaus2::autoarray::AutoArray<uint64_t> & A, bool const erase = true)
			{
				if ( A.size() < libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES )
					A.resize(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES);

				if ( erase )
					std::fill(A.begin(),A.begin() + libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CTHRES, 0ull);

				uint64_t const ncig = getNCigar(B);
				uint32_t const * cigp = reinterpret_cast<uint32_t const *>(getCigar(B));

				for ( uint64_t i = 0 ; i < ncig; ++i )
				{
					uint32_t const cigv = *(cigp++);
					uint32_t const op = cigv & 0xf;
					uint32_t const cnt = cigv >> 4;
					A [ op ] += cnt;
				}
			}

			static double getIdentityFractionFromCigarStats(libmaus2::autoarray::AutoArray<uint64_t> & A)
			{
				if ( A[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH] )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentDecoderBase::getIdentity(): cannot handle CIGAR strings containing M operations" << std::endl;
					lme.finish();
					throw lme;
				}

				uint64_t const eq = A[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL];
				uint64_t const di = A[libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF];

				if ( eq+di )
					return static_cast<double>(eq) / static_cast<double>(eq+di);
				else
					return 0.0;
			}

			static double getIdentityFraction(uint8_t const * B, libmaus2::autoarray::AutoArray<uint64_t> & A)
			{
				getCigarStats(B,A,true);
				return getIdentityFractionFromCigarStats(A);
			}


			static std::vector< PileVectorElement > getPileVector(
				uint8_t const * B,
				libmaus2::autoarray::AutoArray<cigar_operation> & cigopin,
				libmaus2::autoarray::AutoArray<char> & readdata,
				uint64_t const readid = 0
			)
			{
				static const bool calmd_preterm[] =
				{
					true, // LIBMAUS2_BAMBAM_CMATCH = 0,
					false, // LIBMAUS2_BAMBAM_CINS = 1,
					false, // LIBMAUS2_BAMBAM_CDEL = 2,
					false, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					false, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					false, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					false, // LIBMAUS2_BAMBAM_CPAD = 6,
					true, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					true // LIBMAUS2_BAMBAM_CDIFF = 8
				};
				static const uint8_t calmd_readadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					1, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_refadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					0, // LIBMAUS2_BAMBAM_CINS = 1,
					1, // LIBMAUS2_BAMBAM_CDEL = 2,
					1, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				uint32_t const numcigin = getCigarOperations(B,cigopin);
				uint64_t i = 0;
				int64_t refpos = getPos(B);
				int64_t const refid = getRefID(B);
				decodeRead(B, readdata);
				char const * it_r = readdata.begin();
				uint64_t readpos = 0;

				for ( ; i < numcigin && (! calmd_preterm[cigopin[i].first]); ++i )
				{
					int32_t const op = cigopin[i].first;
					int32_t const len = cigopin[i].second;
					readpos += calmd_readadvance[op] * len;
					it_r += calmd_readadvance[op] * len;
				}

				std::vector< PileVectorElement > Vout;

				for ( ; i < numcigin; ++i )
				{
					int32_t const op = cigopin[i].first;
					int32_t const len = cigopin[i].second;

					switch ( op )
					{
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS:
						{
							for ( int64_t i = 0; i < len; ++i )
							{
								PileVectorElement PP
								(
									refid,
									readid,
									refpos,
									-len+i,
									readpos+i,
									static_cast<int32_t>(readdata.size() - (readpos+i)) - 1,
									it_r[i]
								);
								Vout.push_back(PP);
							}
							break;
						}
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
						{
							for ( int64_t i = 0; i < len; ++i )
							{
								PileVectorElement PP(refid,readid,refpos+i,0,readpos,static_cast<int32_t>(readdata.size()-readpos)-1,'-');
								Vout.push_back(PP);
							}
							break;
						}
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
						{
							for ( int64_t i = 0; i < len; ++i )
							{
								PileVectorElement PP
								(
									refid,
									readid,
									refpos+i,
									0,
									readpos+i,
									static_cast<int32_t>(readdata.size() - (readpos+i)) - 1,
									it_r[i]
								);
								Vout.push_back(PP);
							}
							break;
						}
					}

					refpos += calmd_refadvance[op] * len;
					readpos += calmd_readadvance[op] * len;
					// itref += calmd_refadvance[op] * len;
					it_r += calmd_readadvance[op] * len;
				}

				return Vout;
			}

			template<typename it_a>
			static uint64_t recalculateCigar(
				uint8_t const * B,
				it_a itref,
				libmaus2::autoarray::AutoArray<cigar_operation> & cigopin,
				libmaus2::autoarray::AutoArray<char> & readdata
			)
			{
				static const bool calmd_preterm[] =
				{
					true, // LIBMAUS2_BAMBAM_CMATCH = 0,
					false, // LIBMAUS2_BAMBAM_CINS = 1,
					false, // LIBMAUS2_BAMBAM_CDEL = 2,
					false, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					false, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					false, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					false, // LIBMAUS2_BAMBAM_CPAD = 6,
					true, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					true // LIBMAUS2_BAMBAM_CDIFF = 8
				};
				static const uint8_t calmd_readadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					1, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_refadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					0, // LIBMAUS2_BAMBAM_CINS = 1,
					1, // LIBMAUS2_BAMBAM_CDEL = 2,
					1, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				// decode cigar operation vector
				uint32_t const numcigin = getCigarOperations(B,cigopin);
				// decode read data
				decodeRead(B, readdata);

				uint64_t i = 0;

				int64_t refpos = getPos(B);
				int64_t readpos = 0;
				char const * it_r = readdata.begin();

				std::vector<libmaus2::bambam::BamFlagBase::bam_cigar_ops> Vcigopout;

				for ( ; i < numcigin && (! calmd_preterm[cigopin[i].first]); ++i )
				{
					int32_t const op = cigopin[i].first;
					int32_t const len = cigopin[i].second;

					for ( int64_t j = 0; j < len; ++j )
						Vcigopout.push_back(static_cast<libmaus2::bambam::BamFlagBase::bam_cigar_ops>(op));
					readpos += calmd_readadvance[op] * cigopin[i].second;
				}

				it_r += readpos;

				for ( ; i < numcigin; ++i )
				{
					int32_t const op = cigopin[i].first;
					int32_t const len = cigopin[i].second;

					switch ( op )
					{
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						{
							for ( int64_t j = 0; j < len; ++j )
							{
								char const c_ref = *(itref);
								char const c_read = *(it_r);
								bool const match = c_ref == c_read;

								if ( match )
									Vcigopout.push_back(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL);
								else
									Vcigopout.push_back(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF);

								refpos += 1;
								readpos += 1;
								itref += 1;
								it_r += 1;
							}
							break;
						}
						default:
						{
							for ( int64_t j = 0; j < len; ++j )
								Vcigopout.push_back(static_cast<libmaus2::bambam::BamFlagBase::bam_cigar_ops>(op));

							refpos += calmd_refadvance[op] * len;
							readpos += calmd_readadvance[op] * len;
							itref += calmd_refadvance[op] * len;
							it_r += calmd_readadvance[op] * len;
							break;
						}
					}
				}

				std::vector< std::pair<int32_t,int32_t> > Vout;
				uint64_t low = 0;
				while ( low != Vcigopout.size() )
				{
					uint64_t high = low+1;
					while ( high < Vcigopout.size() && Vcigopout[high] == Vcigopout[low] )
						++high;

					Vout.push_back(std::pair<int32_t,int32_t>(Vcigopout[low],high-low));

					low = high;
				}

				assert ( readpos == getLseq(B) );
				assert ( refpos == getPos(B) + getReferenceAdvance(B) );

				if ( Vout.size() > cigopin.size() )
					cigopin.resize(Vout.size());

				std::copy(Vout.begin(),Vout.end(),cigopin.begin());

				return Vout.size();
			}

			/**
			 * calculate MD and NM fields
			 *
			 * @param B alignment block
			 * @param blocksize length of block in bytes
			 * @param context temporary space and result storage
			 * @param itref pointer to reference at position of first non clipping op
			 * @param seq pointer to encoded query sequence
			 * @param readlength length of query sequence
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			static void calculateMd(
				uint8_t const * B,
				uint64_t const blocksize,
				::libmaus2::bambam::MdStringComputationContext & context,
				it_a itref,
				uint8_t const * seq,
				uint64_t const readlength,
				bool const warnchanges = true
			)
			{
				static const bool calmd_preterm[] =
				{
					true, // LIBMAUS2_BAMBAM_CMATCH = 0,
					false, // LIBMAUS2_BAMBAM_CINS = 1,
					false, // LIBMAUS2_BAMBAM_CDEL = 2,
					false, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					false, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					false, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					false, // LIBMAUS2_BAMBAM_CPAD = 6,
					true, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					true // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_insmult[] =
				{
					0, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					0, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					0 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_readadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					1, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_refadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					0, // LIBMAUS2_BAMBAM_CINS = 1,
					1, // LIBMAUS2_BAMBAM_CDEL = 2,
					1, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				context.diff = false;

				if ( ! libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(
					libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B)
					)
				)
				{
					// number of cigar operations
					libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> & cigop = context.cigop;
					uint32_t const numcigop = libmaus2::bambam::BamAlignmentDecoderBase::getCigarOperations(B,cigop);

					// length of unrolled cigar operatios
					uint64_t cigsum = 0;
					for ( uint64_t i = 0; i < numcigop; ++i )
						cigsum += cigop[i].second;

					context.checkSize(cigsum);

					// insertions and deletions
					uint64_t numins = 0;
					uint64_t numdel = 0;
					uint64_t nummis = 0;
					// position on read
					uint64_t readpos = 0;
					// index on RL cigar string
					uint64_t cigi = 0;

					it_a const itreforg = itref;
					uint64_t a = 0;
					char * mdp = context.md.begin();

					for ( ; cigi != numcigop; ++cigi )
					{
						int32_t const cigo = cigop[cigi].first;
						uint64_t const cigp = cigop[cigi].second;

						if ( calmd_preterm[cigo] )
						{
							uint64_t l = 0;
							while ( l != cigp )
							{
								uint64_t h = l;

								uint64_t const startoff = readpos + h;
								uint8_t const * lseq = seq + (startoff>>1);
								if ( startoff & 1 )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c1 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(c & 0xF);

									if (  context.T0[c1] != context.T1[itref[h]] )
										goto cmpdone;

									++h;
								}

								while ( cigp - h > 1 )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>4)&0xF);

									if ( context.T0[c0] != context.T1[itref[h]] )
										goto cmpdone;

									++h;

									uint8_t const c1 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>0)&0xF);

									if ( context.T0[c1] != context.T1[itref[h]] )
										goto cmpdone;

									++h;
								}

								if ( cigp - h )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>4)&0xF);

									if (
										context.T0[c0]
										==
										context.T1[itref[h]]
									)
										++h;
								}

								cmpdone:

								if ( h != cigp )
								{
									mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,h-l+a);
									*(mdp++) = itref[h];
									h += 1;
									a = 0;
									nummis += 1;
								}
								else
								{
									a += h-l;
								}

								l = h;
							}
						}
						else if ( cigo == ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL )
						{
							mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,a);
							a = 0;
							*(mdp++) =  '^';
							for ( uint64_t i = 0; i < cigp; ++i )
								*(mdp++) = itref[i];

							numdel += cigp;
						}

						readpos += calmd_readadvance[cigo] * cigp;
						itref   += calmd_refadvance[cigo] * cigp;
						numins  += calmd_insmult[cigo] * cigp;
					}
					mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,a);
					*(mdp++) = 0;

					context.nm = numins+numdel+nummis;

					char const * const prevmd = libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(B,blocksize,"MD");
					int32_t prevnm = -1;
					bool const haveprevnm = libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int32_t>(B,blocksize,"NM",prevnm);

					context.mddiff = (!prevmd) || strcmp(context.md.get(),prevmd);
					context.nmdiff = (!haveprevnm) || (prevnm != static_cast<int32_t>(context.nm));
					context.diff = context.mddiff | context.nmdiff;

					if ( warnchanges )
					{
						if ( context.mddiff && prevmd )
							std::cerr << "[D] " << libmaus2::bambam::BamAlignmentDecoderBase::getReadName(B) << ":" << libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B) << " update MD from " << prevmd << " to " << context.md.get() << "\n";
						if ( haveprevnm && (prevnm != static_cast<int32_t>(context.nm)) )
							std::cerr << "[D] " << libmaus2::bambam::BamAlignmentDecoderBase::getReadName(B) << ":" << libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B) << " update NM from " << prevnm << " to " << context.nm << "\n";
					}

					if ( readpos != readlength )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentDecoderBase::calculateMd: readpos=" << readpos << " != " << readlength << std::endl;
						lme.finish();
						throw lme;
					}
					if ( itref != itreforg + libmaus2::bambam::BamAlignmentDecoderBase::getReferenceLength(B) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentDecoderBase::calculateMd: itrefdif=" << itref-itreforg << " != reflength " << libmaus2::bambam::BamAlignmentDecoderBase::getReferenceLength(B) << std::endl;
						lme.finish();
						throw lme;
					}
					context.eraseold = prevmd || haveprevnm;
				}

			}

			/**
			 * calculate MD and NM fields
			 *
			 * @param B alignment block
			 * @param blocksize length of block in bytes
			 * @param context temporary space and result storage
			 * @param itref pointer to reference at position of first non clipping op
			 * @param seq pointer to encoded query sequence
			 * @param readlength length of query sequence
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			static void calculateMdMapped(
				uint8_t const * B,
				uint64_t const blocksize,
				::libmaus2::bambam::MdStringComputationContext & context,
				it_a itref,
				uint8_t const * seq,
				uint64_t const readlength,
				bool const warnchanges = true
			)
			{
				static const bool calmd_preterm[] =
				{
					true, // LIBMAUS2_BAMBAM_CMATCH = 0,
					false, // LIBMAUS2_BAMBAM_CINS = 1,
					false, // LIBMAUS2_BAMBAM_CDEL = 2,
					false, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					false, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					false, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					false, // LIBMAUS2_BAMBAM_CPAD = 6,
					true, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					true // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_insmult[] =
				{
					0, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					0, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					0 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_readadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					1, // LIBMAUS2_BAMBAM_CINS = 1,
					0, // LIBMAUS2_BAMBAM_CDEL = 2,
					0, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					1, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				static const uint8_t calmd_refadvance[] =
				{
					1, // LIBMAUS2_BAMBAM_CMATCH = 0,
					0, // LIBMAUS2_BAMBAM_CINS = 1,
					1, // LIBMAUS2_BAMBAM_CDEL = 2,
					1, // LIBMAUS2_BAMBAM_CREF_SKIP = 3,
					0, // LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
					0, // LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
					0, // LIBMAUS2_BAMBAM_CPAD = 6,
					1, // LIBMAUS2_BAMBAM_CEQUAL = 7,
					1 // LIBMAUS2_BAMBAM_CDIFF = 8
				};

				context.diff = false;

				if ( ! libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(
					libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B)
					)
				)
				{
					// number of cigar operations
					libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> & cigop = context.cigop;
					uint32_t const numcigop = libmaus2::bambam::BamAlignmentDecoderBase::getCigarOperations(B,cigop);

					// length of unrolled cigar operatios
					uint64_t cigsum = 0;
					for ( uint64_t i = 0; i < numcigop; ++i )
						cigsum += cigop[i].second;

					context.checkSize(cigsum);

					// insertions and deletions
					uint64_t numins = 0;
					uint64_t numdel = 0;
					uint64_t nummis = 0;
					// position on read
					uint64_t readpos = 0;
					// index on RL cigar string
					uint64_t cigi = 0;

					it_a const itreforg = itref;
					uint64_t a = 0;
					char * mdp = context.md.begin();

					for ( ; cigi != numcigop; ++cigi )
					{
						int32_t const cigo = cigop[cigi].first;
						uint64_t const cigp = cigop[cigi].second;

						if ( calmd_preterm[cigo] )
						{
							uint64_t l = 0;
							while ( l != cigp )
							{
								uint64_t h = l;

								uint64_t const startoff = readpos + h;
								uint8_t const * lseq = seq + (startoff>>1);
								if ( startoff & 1 )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c1 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(c & 0xF);

									if (  context.T0[c1] != context.T1[libmaus2::fastx::remapChar(itref[h])] )
										goto cmpdone;

									++h;
								}

								while ( cigp - h > 1 )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>4)&0xF);

									if ( context.T0[c0] != context.T1[libmaus2::fastx::remapChar(itref[h])] )
										goto cmpdone;

									++h;

									uint8_t const c1 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>0)&0xF);

									if ( context.T0[c1] != context.T1[libmaus2::fastx::remapChar(itref[h])] )
										goto cmpdone;

									++h;
								}

								if ( cigp - h )
								{
									uint8_t const c = *(lseq++);
									uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c>>4)&0xF);

									if (
										context.T0[c0]
										==
										context.T1[libmaus2::fastx::remapChar(itref[h])]
									)
										++h;
								}

								cmpdone:

								if ( h != cigp )
								{
									mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,h-l+a);
									*(mdp++) = libmaus2::fastx::remapChar(itref[h]);
									h += 1;
									a = 0;
									nummis += 1;
								}
								else
								{
									a += h-l;
								}

								l = h;
							}
						}
						else if ( cigo == ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL )
						{
							mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,a);
							a = 0;
							*(mdp++) =  '^';
							for ( uint64_t i = 0; i < cigp; ++i )
								*(mdp++) = libmaus2::fastx::remapChar(itref[i]);

							numdel += cigp;
						}

						readpos += calmd_readadvance[cigo] * cigp;
						itref   += calmd_refadvance[cigo] * cigp;
						numins  += calmd_insmult[cigo] * cigp;
					}
					mdp = ::libmaus2::bambam::MdStringComputationContext::putNumber(mdp,a);
					*(mdp++) = 0;

					context.nm = numins+numdel+nummis;

					char const * const prevmd = libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(B,blocksize,"MD");
					int32_t prevnm = -1;
					bool const haveprevnm = libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int32_t>(B,blocksize,"NM",prevnm);

					context.mddiff = (!prevmd) || strcmp(context.md.get(),prevmd);
					context.nmdiff = (!haveprevnm) || (prevnm != static_cast<int32_t>(context.nm));
					context.diff = context.mddiff | context.nmdiff;

					if ( warnchanges )
					{
						if ( context.mddiff && prevmd )
							std::cerr << "[D] " << libmaus2::bambam::BamAlignmentDecoderBase::getReadName(B) << ":" << libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B) << " update MD from " << prevmd << " to " << context.md.get() << "\n";
						if ( haveprevnm && (prevnm != static_cast<int32_t>(context.nm)) )
							std::cerr << "[D] " << libmaus2::bambam::BamAlignmentDecoderBase::getReadName(B) << ":" << libmaus2::bambam::BamAlignmentDecoderBase::getFlags(B) << " update NM from " << prevnm << " to " << context.nm << "\n";
					}

					if ( readpos != readlength )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentDecoderBase::calculateMd: readpos=" << readpos << " != " << readlength << std::endl;
						lme.finish();
						throw lme;
					}
					if ( itref != itreforg + libmaus2::bambam::BamAlignmentDecoderBase::getReferenceLength(B) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "BamAlignmentDecoderBase::calculateMd: itrefdif=" << itref-itreforg << " != reflength " << libmaus2::bambam::BamAlignmentDecoderBase::getReferenceLength(B) << std::endl;
						lme.finish();
						throw lme;
					}
					context.eraseold = prevmd || haveprevnm;
				}

			}



			/**
			 * calculate MD and NM fields
			 *
			 * @param B alignment block
			 * @param blocksize length of block in bytes
			 * @param context temporary space and result storage
			 * @param pointer to reference at position of first non clipping op
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			static void calculateMd(
				uint8_t const * B,
				uint64_t const blocksize,
				::libmaus2::bambam::MdStringComputationContext & context,
				it_a itref,
				bool const warnchanges = true
			)
			{
				calculateMd(B,blocksize,context,itref,libmaus2::bambam::BamAlignmentDecoderBase::getSeq(B),libmaus2::bambam::BamAlignmentDecoderBase::getLseq(B),warnchanges);
			}

			/**
			 * calculate MD and NM fields
			 *
			 * @param B alignment block
			 * @param blocksize length of block in bytes
			 * @param context temporary space and result storage
			 * @param pointer to reference at position of first non clipping op
			 * @param warnchanges warn about changes on stderr if previous values are present
			 **/
			template<typename it_a>
			static void calculateMdMapped(
				uint8_t const * B,
				uint64_t const blocksize,
				::libmaus2::bambam::MdStringComputationContext & context,
				it_a itref,
				bool const warnchanges = true
			)
			{
				calculateMdMapped(B,blocksize,context,itref,libmaus2::bambam::BamAlignmentDecoderBase::getSeq(B),libmaus2::bambam::BamAlignmentDecoderBase::getLseq(B),warnchanges);
			}

			/**
			 * check whether s contains a valid BAM query name
			 *
			 * @param s string to be tested
			 * @return true iff s contains a valid BAM query name
			 */
			static bool nameValid(std::string const & s)
			{
				if ( s.size() < 1 || s.size() > 255 )
					return false;

				unsigned char const * p = reinterpret_cast<unsigned char const *>(s.c_str());
				unsigned char const * pe = p + s.size();

				while ( p != pe )
					if ( !qnameValidTable[*(p++)] )
						return false;

				return true;
			}

			/**
			 * check whether c contains a valid BAM query name
			 *
			 * @param c string to be tested
			 * @return true iff c contains a valid BAM query name
			 */
			static bool nameValid(char const * c)
			{
				unsigned char const * p = reinterpret_cast<unsigned char const *>(c);

				while ( *p )
					if ( !qnameValidTable[*(p++)] )
						return false;

				ptrdiff_t len = p - reinterpret_cast<unsigned char const *>(c);

				if ( len < 1 || len > 255 )
					return false;

				return true;
			}

			/**
			 * check whether character range contains a valid BAM query name
			 *
			 * @param c string to be tested
			 * @return true iff c contains a valid BAM query name
			 */
			template<typename iterator>
			static bool nameValid(iterator p, iterator pe)
			{
				ptrdiff_t len = pe-p;

				if ( len < 1 || len > 255 )
					return false;

				while ( p != pe )
					if ( !qnameValidTable[static_cast<uint8_t>(*(p++))] )
						return false;

				return true;
			}

			/**
			 * check alignment validity excluding reference sequence ids
			 *
			 * @param D alignment block
			 * @param blocksize length of alignment block in bytes
			 * @return alignment validty code
			 **/
			static libmaus2_bambam_alignment_validity valid(uint8_t const * D, uint64_t const blocksize)
			{
				// fixed length fields are 32 bytes
				if ( blocksize < 32 )
					return libmaus2_bambam_alignment_validity_block_too_small;

				uint8_t const * blocke = D + blocksize;
				uint8_t const * readname = reinterpret_cast<uint8_t const *>(::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(D));
				uint8_t const * const readnamea = readname;

				// look for end of readname
				while ( readname != blocke && *readname )
					++readname;

				// read name extends beyond block
				if ( readname == blocke )
					return libmaus2_bambam_alignment_validity_queryname_extends_over_block;

				// readname should be pointing to a null byte now
				if ( readname != blocke )
					assert ( ! *readname );

				// check whether actual length of read name is consistent with
				// numerical value given in block
				uint64_t const lreadname = ::libmaus2::bambam::BamAlignmentDecoderBase::getLReadName(D);
				if ( (readname-readnamea)+1 != static_cast<ptrdiff_t>(lreadname) )
					return libmaus2_bambam_alignment_validity_queryname_length_inconsistent;
				if ( !(readname-readnamea) )
					return libmaus2_bambam_alignment_validity_queryname_empty;
				for ( uint64_t i = 0; i < lreadname-1; ++i )
					if ( ! qnameValidTable[readnamea[i]] )
						return libmaus2_bambam_alignment_validity_queryname_contains_illegal_symbols;

				// check if cigar string ends before block end
				uint8_t const * cigar = ::libmaus2::bambam::BamAlignmentDecoderBase::getCigar(D);
				uint64_t const ncig = ::libmaus2::bambam::BamAlignmentDecoderBase::getNCigar(D);
				if ( cigar + ncig*sizeof(uint32_t) > blocke )
					return libmaus2_bambam_alignment_validity_cigar_extends_over_block;

				// check if sequence ends before block end
				uint8_t const * seq = ::libmaus2::bambam::BamAlignmentDecoderBase::getSeq(D);
				uint64_t const seqlen = ::libmaus2::bambam::BamAlignmentDecoderBase::getLseq(D);
				if ( seq + ((seqlen+1)/2) > blocke )
					return libmaus2_bambam_alignment_validity_sequence_extends_over_block;

				// check if quality string ends before block end
				uint8_t const * const qual = ::libmaus2::bambam::BamAlignmentDecoderBase::getQual(D);
				if ( qual + seqlen > blocke )
					return libmaus2_bambam_alignment_validity_quality_extends_over_block;

				// check whether all cigar operators are known
				for ( uint64_t i = 0; i < ncig; ++i )
				{
					uint64_t const cigop = ::libmaus2::bambam::BamAlignmentDecoderBase::getCigarFieldOp(D,i);
					if ( cigop > ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF )
						return libmaus2_bambam_alignment_validity_unknown_cigar_op;
				}

				uint32_t const flags = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(D);
				bool const unmapped = ::libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(flags);
				bool const mapped = !unmapped;
				bool const qcfail = ::libmaus2::bambam::BamAlignmentDecoderBase::isQCFail(flags);
				bool const secondary = ::libmaus2::bambam::BamAlignmentDecoderBase::isSecondary(flags);
				// annotation (see SAM spec section 3.2, padded)
				bool const annot = qcfail && secondary;

				// check whether cigar string is consistent with sequence length
				if (
					mapped
					&&
					(!annot)
					&&
					seqlen
					&&
					static_cast<int64_t>(::libmaus2::bambam::BamAlignmentDecoderBase::getLseqByCigar(D)) != static_cast<int64_t>(seqlen)
				)
					return libmaus2_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length;

				int32_t const pos = ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(D);
				if ( pos < -1 )
				{
					return libmaus2_bambam_alignment_validity_invalid_mapping_position;
				}
				int32_t const nextpos = ::libmaus2::bambam::BamAlignmentDecoderBase::getNextPos(D);
				if ( nextpos < -1 )
				{
					return libmaus2_bambam_alignment_validity_invalid_next_mapping_position;
				}
				int32_t const tlen = ::libmaus2::bambam::BamAlignmentDecoderBase::getTlen(D);
				if ( tlen < ((-(1ll<<31))+1) )
					return libmaus2_bambam_alignment_validity_invalid_tlen;

				uint8_t const * const quale = qual + seqlen;
				for ( uint8_t const * qualc = qual; qualc != quale; ++qualc )
					if ( static_cast<int>(*qualc) > static_cast<int>('~'-33) )
					{
						if ( *qualc == 255 )
						{
							if ( qualc - qual )
							{
								return libmaus2_bambam_alignment_validity_invalid_quality_value;
							}

							while ( qualc != quale )
								if ( *(qualc++) != 255 )
									return libmaus2_bambam_alignment_validity_invalid_quality_value;

							// go back by one to leave loop above
							--qualc;
						}
						else
							return libmaus2_bambam_alignment_validity_invalid_quality_value;
					}

				uint8_t const * aux = ::libmaus2::bambam::BamAlignmentDecoderBase::getAux(D);

				while ( aux != D+blocksize )
				{
					if ( ((D+blocksize)-aux) < 3 )
						return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;

					switch ( aux[2] )
					{
						case 'A':
							if ( ((D+blocksize)-aux) < 3+1 )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							if ( aux[3] < '!' || aux[3] > '~' )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'c':
						case 'C':
							if ( ((D+blocksize)-aux) < 3+1 )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 's':
						case 'S':
							if ( ((D+blocksize)-aux) < 3+2 )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'i':
						case 'I':
						case 'f':
							if ( ((D+blocksize)-aux) < 3+4 )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							break;
						case 'B':
						{
							if ( ((D+blocksize)-aux) < 3 + 1 /* data type */ + 4 /* length of array */ )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							/* length of array */
							uint64_t const alen = static_cast<int32_t>(::libmaus2::bambam::BamAlignmentDecoderBase::getLEInteger(aux+4,4));
							/* valid element data types */
							switch ( aux[3] )
							{
								case 'c':
								case 'C':
									if ( ((D+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4+1*alen) )
										return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								case 's':
								case 'S':
									if ( ((D+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4 + 2*alen) )
										return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								case 'i':
								case 'I':
								case 'f':
									if ( ((D+blocksize)-aux) < static_cast<ptrdiff_t>(3+1+4 + 4*alen) )
										return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
									break;
								default:
									return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							}
							break;
						}
						case 'Z':
						{
							uint8_t const * p = aux+3;
							uint8_t const * const pe = D + blocksize;

							while ( p != pe && *p )
								++p;

							// if terminator byte 0 is not inside block
							if ( p == pe )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;

							break;
						}
						case 'H':
						{
							uint8_t const * const pa = aux+3;
							uint8_t const * p = pa;
							uint8_t const * const pe = D + blocksize;

							while ( p != pe && *p )
								++p;

							// if terminator byte 0 is not inside block
							if ( p == pe )
								return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;

							break;
						}
						default:
							return libmaus2_bambam_alignment_validity_invalid_auxiliary_data;
							break;
					}

					aux = aux + ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxLength(aux);
				}


				return libmaus2_bambam_alignment_validity_ok;
			}

			/**
			 * @param D alignment block
			 * @param blocksize length of alignment block in bytes
			 * @return true if read fragment contains any non A,C,G or T
			 **/
			static bool hasNonACGT(uint8_t const * D)
			{
				static const uint8_t calmd_eqcheck[16] =
				{
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
				};

				// length of read in bases
				uint64_t const readlength = libmaus2::bambam::BamAlignmentDecoderBase::getLseq(D);
				// encoded sequence
				uint8_t const * seq = libmaus2::bambam::BamAlignmentDecoderBase::getSeq(D);

				uint64_t const fullwords = readlength >> 4; // 2 bases per byte -> 16 bases per 64 bit word
				uint64_t const restbases = readlength - (fullwords<<4); // number of rest bases
				uint8_t const * rseq = seq;
				unsigned int seqbits = 0;
				uint8_t prod = 1;

				for ( uint64_t i = 0; i < fullwords; ++i )
				{
					uint64_t v = 0;

					for ( unsigned int i = 0; i < 8; ++i )
					{
						uint8_t const u = *(rseq++);
						v <<= 8;
						v |= u;
						prod &= calmd_eqcheck[(u>>4) & 0xF];
						prod &= calmd_eqcheck[(u>>0) & 0xF];
					}

					seqbits += libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(v);
				}
				if ( restbases )
				{
					uint64_t v = 0;

					unsigned int const restbytes = (restbases >> 1);
					assert ( restbytes < 8 );
					for ( unsigned int i = 0; i < restbytes; ++i )
					{
						uint8_t const u = *(rseq++);
						v <<= 8;
						v |= u;
						prod &= calmd_eqcheck[(u>>4) & 0xF];
						prod &= calmd_eqcheck[(u>>0) & 0xF];
					}
					if ( restbases & 1 )
					{
						uint8_t const u = *(rseq++);
						v <<= 8;
						v |= (u >> 4) & 0xF;
						prod &= calmd_eqcheck[(u>>4) & 0xF];
					}

					seqbits += libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(v);
				}

				if ( prod )
					return seqbits != readlength;
				else
				{
					rseq = seq;

					for ( uint64_t i = 0; i < (readlength>>1); ++i )
					{
						uint8_t const c = *(rseq++);
						uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c >> 4) & 0xFF);
						uint8_t const c1 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c >> 0) & 0xFF);

						switch ( c0 )
						{
							case 'A': case 'C': case 'G': case 'T':
								break;
							default:
								return true;
						}
						switch ( c1 )
						{
							case 'A': case 'C': case 'G': case 'T':
								break;
							default:
								return true;
						}
					}
					if ( readlength & 1 )
					{
						uint8_t const c = *(rseq++);
						uint8_t const c0 = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked((c >> 4) & 0xFF);
						switch ( c0 )
						{
							case 'A': case 'C': case 'G': case 'T':
								break;
							default:
								return true;
						}
					}

					return false;
				}
			}

			/**
			 * @return computed bin
			 **/
			static uint64_t computeBin(uint8_t const * D)
			{
				uint64_t const rbeg = getPos(D);
				uint64_t const rend = rbeg + getReferenceLength(D);
				uint64_t const bin = ::libmaus2::bambam::BamAlignmentReg2Bin::reg2bin(rbeg,rend);
				return bin;
			}

			/**
			 * compute insert size (inspired by Picard code)
			 *
			 * @param A first alignment
			 * @param B second alignment
			 * @return note that when storing insert size on the secondEnd, the return value must be negated.
			 */
			static int64_t computeInsertSize(uint8_t const * Au, uint8_t const * Bu)
			{
				uint32_t const Aflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(Au);
				uint32_t const Bflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(Bu);
				bool const Aunmapped = libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(Aflags);
				bool const Bunmapped = libmaus2::bambam::BamAlignmentDecoderBase::isUnmap(Bflags);

				// unmapped end?
				if (Aunmapped || Bunmapped) { return 0; }
				// different ref seq?
				if (
					libmaus2::bambam::BamAlignmentDecoderBase::getRefID(Au)
					!=
					libmaus2::bambam::BamAlignmentDecoderBase::getRefID(Bu)
				) { return 0; }

				// compute 5' end positions
				bool const Areverse = libmaus2::bambam::BamAlignmentDecoderBase::isReverse(Aflags);
				bool const Breverse = libmaus2::bambam::BamAlignmentDecoderBase::isReverse(Bflags);
				int64_t const A5  = Areverse ? libmaus2::bambam::BamAlignmentDecoderBase::getAlignmentEnd(Au) : libmaus2::bambam::BamAlignmentDecoderBase::getPos(Au);
				int64_t const B5  = Breverse ? libmaus2::bambam::BamAlignmentDecoderBase::getAlignmentEnd(Bu) : libmaus2::bambam::BamAlignmentDecoderBase::getPos(Bu);

				// return insert size for (A,B), use negative value for (B,A)
				return B5 - A5;
			}

			static  int32_t     getRefIDChecked(uint8_t const * D)
			{
				int32_t const refid = getRefID(D);
				return (refid<0) ? -1 : refid;
			}

			static  int32_t     getNextRefIDChecked(uint8_t const * D)
			{
				int32_t const nextrefid = getNextRefID(D);
				return (nextrefid<0) ? -1 : nextrefid;
			}

			template<typename header_type>
			static int64_t getReadGroupId(uint8_t const * E, uint64_t const blocksize, header_type const & bamheader)
			{
				return bamheader.getReadGroupId(getReadGroup(E,blocksize));
			}
		};
	}
}
#endif
