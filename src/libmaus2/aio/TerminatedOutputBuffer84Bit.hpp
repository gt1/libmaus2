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
#if ! defined(TERMINATEDOUTPUTBUFFER84BIT_HPP)
#define TERMINATEDOUTPUTBUFFER84BIT_HPP

#include <libmaus2/aio/OutputBuffer84Bit.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * specialisation of OutputBuffer84Bit introducing a shift and unique terminator symbols sequences
		 **/
		struct TerminatedOutputBuffer84Bit : public OutputBuffer84Bit
		{
			private:
			//! terminator base
			uint64_t const base;
			//! terminator exponent (length of terminator sequences in symbols)
			unsigned int const expo;
			//! terminator buffer
			::libmaus2::autoarray::AutoArray<uint8_t> termbuf;

			public:
			/**
			 * construct
			 *
			 * @param filename output file name
			 * @param bufsize output buffer size
			 * @param rbase terminator base
			 * @param rexpo terminator exponent (number of symbols per terminator sequence)
			 **/
			TerminatedOutputBuffer84Bit(
				std::string const & filename,
				uint64_t const bufsize,
				uint64_t const rbase,
				unsigned int const rexpo
			) : OutputBuffer84Bit(filename,bufsize), base(rbase), expo(rexpo), termbuf(expo)
			{

			}

			/**
			 * put shift symbol c
			 *
			 * @param c symbol to be put
			 **/
			void putBase(uint8_t const c)
			{
				put( c + base + 1 );
			}

			/**
			  * put terminator num
			  *
			  * @param num terminator number
			  **/
			void putTerm(uint64_t num)
			{
				uint8_t * p = termbuf.get() + termbuf.getN();
				for ( unsigned int i = 0; i < expo; ++i )
				{
					*(--p) = (num % base) + 1;
					num /= base;
				}
				assert ( p == termbuf.get() );
				for ( unsigned int i = 0; i < expo; ++i )
					put( *(p++) );
			}
		};
	}
}
#endif
