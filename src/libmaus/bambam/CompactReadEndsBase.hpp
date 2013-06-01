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
#if ! defined(LIBMAUS_UTIL_COMPACTREADENDSBASE_HPP)
#define LIBMAUS_UTIL_COMPACTREADENDSBASE_HPP

#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/util/CountPutObject.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/utf8.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * base class for compact read ends storage
		 **/
		struct CompactReadEndsBase
		{
			/*
			 * decode number in utf8 representation; p will be right behind the number when the function returns
			 *
			 * @param p input iterator
			 * @return decoded length
			 */
			static uint32_t decodeLength(uint8_t const * & p)
			{
				::libmaus::util::GetObject<uint8_t const *> G(p);
				uint32_t const len = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				p = G.p;
				return len;
			}

			/*
			 * decode number in utf8 representation; p will be right behind the number when the function returns
			 *
			 * @param p input iterator
			 * @return decoded length
			 */
			static uint32_t decodeLength(uint8_t * & p)
			{
				::libmaus::util::GetObject<uint8_t *> G(p);
				uint32_t const len = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				p = G.p;
				return len;
			}
			
			/**
			 * get length of number in utf8 representation
			 *
			 * @param n number
			 * @return length of n in utf8 representation
			 **/
			static uint64_t getNumberLength(uint32_t const n)
			{
				::libmaus::util::CountPutObject P;
				::libmaus::util::UTF8::encodeUTF8(n,P);
				return P.c;
			}
			
			/*
			 * get length of compact entry in bytes
			 *
			 * @param R read ends object
			 * @return length of R in compact representation in bytes
			 */
			static uint64_t getEntryLength(::libmaus::bambam::ReadEnds const & R)
			{
				::libmaus::util::CountPutObject P;
				R.put(P);
				return P.c;
			}

			/*
			 * get length of compact entry plus one alignment in bytes
			 *
			 * @param R read ends object
			 * @param p alignment
			 * @return length of R in compact representation in bytes plus length of p in bytes
			 **/
			static uint64_t getEntryLength(
				::libmaus::bambam::ReadEnds const & R,
				::libmaus::bambam::BamAlignment const & p
			)
			{
				::libmaus::util::CountPutObject P;
				R.put(P);
				p.serialise(P);
				return P.c;
			}

			/*
			 * get length of compact entry plus two alignments in bytes
			 *
			 * @param R read ends object
			 * @param p first alignment
			 * @param q second alignment
			 * @return length of R in compact representation in bytes plus length of p and q in bytes
			 **/
			static uint64_t getEntryLength(
				::libmaus::bambam::ReadEnds const & R,
				::libmaus::bambam::BamAlignment const & p,
				::libmaus::bambam::BamAlignment const & q
			)
			{
				::libmaus::util::CountPutObject P;
				R.put(P);
				p.serialise(P);
				q.serialise(P);
				return P.c;
			}
		};
	}
}
#endif
