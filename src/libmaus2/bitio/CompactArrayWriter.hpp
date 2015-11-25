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
#if ! defined(LIBMAUS2_BITIO_COMPACTARRAYWRITER_HPP)
#define LIBMAUS2_BITIO_COMPACTARRAYWRITER_HPP

#include <libmaus2/bitio/FastWriteBitWriter.hpp>

namespace libmaus2
{
	namespace bitio
	{
		/**
		 * this class writes serialised CompactArray objects
		 * element by element, without the need to have the full object
		 * in memory. The resulting stream can be read back using
		 * the CompactDecoderWrapper class
		 **/
		struct CompactArrayWriter
		{
			typedef CompactArrayWriter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus2::aio::OutputStreamInstance::unique_ptr_type COS;
			::std::ostream & out;
			::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO;
			::libmaus2::bitio::FastWriteBitWriterBuffer64Sync::unique_ptr_type FWBW;

			uint64_t const n;
			uint64_t const b;

			void setup()
			{
				SGO.put(b);
				SGO.put(n);
				SGO.put( (n*b+63)/64 );
				SGO.put( (n*b+63)/64 );

				::libmaus2::aio::SynchronousGenericOutput<uint64_t>::iterator_type it(SGO);
				::libmaus2::bitio::FastWriteBitWriterBuffer64Sync::unique_ptr_type tFWBW(
                                                new ::libmaus2::bitio::FastWriteBitWriterBuffer64Sync(it)
                                        );
				FWBW = UNIQUE_PTR_MOVE(tFWBW);
			}

			CompactArrayWriter(
				std::string const & filename,
				uint64_t const rn,
				uint64_t const rb
			)
			:
				COS(new ::libmaus2::aio::OutputStreamInstance(filename)),
				out(*COS),
				SGO(out,8*1024),
				n(rn),
				b(rb)
			{
				setup();
			}
			CompactArrayWriter(
				std::ostream & rout,
				uint64_t const rn,
				uint64_t const rb
			)
			:
				COS(),
				out(rout),
				SGO(out,8*1024),
				n(rn),
				b(rb)
			{
				setup();
			}
			~CompactArrayWriter()
			{
				flush();
			}

			void put(uint64_t const v)
			{
				FWBW->write(v,b);
			}

			template<typename iterator>
			void write(iterator it, uint64_t n)
			{
				while ( n-- )
					put(*(it++));
			}

			void flush()
			{
				FWBW->flush();
				SGO.flush();
				out.flush();
			}
		};
	}
}
#endif
