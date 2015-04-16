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


#if ! defined(COMPACTENCODER4_HPP)
#define COMPACTENCODER4_HPP

#include <fstream>
#include <libmaus/aio/GenericOutput.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * encoder class for writing a sequence of 4 bit integers which can be decoded
		 * using the CompactDecoder4 class
		 **/
		struct CompactEncoder4
		{
			private:
			//! ouput stream
			std::ofstream ostr;
			//! buffer for output stream
			::libmaus::aio::SynchronousGenericOutput<uint8_t>::unique_ptr_type SGO;
			//! current partial output byte
			uint8_t c;
			//! true if number of elements written so far is odd (and partial information is in c)
			bool odd;
			
			public:
			/**
			 * constructor
			 *
			 * @param filename name of output file
			 * @param n number of elements to be written
			 **/
			CompactEncoder4(std::string const & filename, uint64_t const n)
			: ostr(filename.c_str(),std::ios::binary), odd(false)
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(ostr,n);
				SGO = ::libmaus::aio::SynchronousGenericOutput<uint8_t>::unique_ptr_type(new ::libmaus::aio::SynchronousGenericOutput<uint8_t>(ostr,64*1024));
			}
			
			/**
			 * put next number val
			 *
			 * @param val number to be written
			 **/
			void put(uint8_t const val)
			{
				if ( ! odd )
				{
					c = val;
				}
				else
				{
					c |= (val << 4);
					SGO->put(c);
				}
				odd = !odd;
			}
			
			/**
			 * flush output
			 **/
			void flush()
			{
				if ( odd )
					put(0);
				SGO->flush();
			}
		};
	}
}
#endif
