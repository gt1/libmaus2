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
#if ! defined(LIBMAUS_BITIO_BITVECTOROUTPUT_HPP)
#define LIBMAUS_BITIO_BITVECTOROUTPUT_HPP

#include <libmaus2/aio/SynchronousGenericOutput.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct BitVectorOutput
		{
			libmaus2::aio::CheckedOutputStream::unique_ptr_type pout;
			libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO;
			uint64_t v;
			unsigned int b;
			
			BitVectorOutput(std::string const & filename) : pout(new libmaus2::aio::CheckedOutputStream(filename)), SGO(*pout,8*1024), v(0), b(64) {}
			BitVectorOutput(std::ostream & out) : pout(), SGO(out,8192), v(0), b(64) {}
			
			void writeBit(bool const bit)
			{
				v <<= 1;
				v |= static_cast<uint64_t>(bit);
				
				if ( ! --b )
				{
					SGO.put(v);
					v = 0;
					b = 64;
				}
			}
			
			void flush()
			{
				// number of bits in file
				uint64_t const numbits = (64-b) + SGO.getWrittenWords() * 8 * sizeof(uint64_t);
				if ( b != 64 )
					SGO.put(v);
				SGO.put(numbits);
				SGO.flush();
				if ( pout )
					pout->flush();
			}
		};
	}
}
#endif
